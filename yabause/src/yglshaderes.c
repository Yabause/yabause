/*  Copyright 2005-2006 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau
    Copyright 2011-2015 Shinya Miyamoto(devmiyax)

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/


//#ifdef __ANDROID__
#include <stdlib.h>
#include <math.h>
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"
#include "bicubic_shader.h"
#include "scanline_shader.h"
#include "common_glshader.h"

#define YGLLOG //YuiMsg

#define ALIGN(A,B) (((A)%(B))? A + (B - ((A)%(B))) : A)

#define QuoteIdent(ident) #ident
#define Stringify(macro) QuoteIdent(macro)

static int saveFB;
static void Ygl_useTmpBuffer();
static void Ygl_releaseTmpBuffer(void);
int YglBlitMosaic(u32 srcTexture, float w, float h, float * matrix, int * mosaic);
int YglBlitPerLineAlpha(u32 srcTexture, float w, float h, float * matrix, u32 perline);

extern int GlHeight;
extern int GlWidth;

static YglVdp1CommonParam _ids[PG_MAX] = { 0 };

static void Ygl_printShaderError( GLuint shader )
{
  GLsizei bufSize;

  glGetShaderiv(shader, GL_INFO_LOG_LENGTH , &bufSize);

  if (bufSize > 1) {
    GLchar *infoLog;

    infoLog = (GLchar *)malloc(bufSize);
    if (infoLog != NULL) {
      GLsizei length;
      glGetShaderInfoLog(shader, bufSize, &length, infoLog);
      YuiMsg("Shaderlog:\n%s\n", infoLog);
      free(infoLog);
    }
  }
}

// void Ygl_Vdp1CommonGetUniformId(GLuint pgid, YglProgram* param){
//
//   param->texsize = glGetUniformLocation(pgid, (const GLchar *)"u_texsize");
//   param->sprite = glGetUniformLocation(pgid, (const GLchar *)"u_sprite");
//   param->tessLevelInner = glGetUniformLocation(pgid, (const GLchar *)"TessLevelInner");
//   param->tessLevelOuter = glGetUniformLocation(pgid, (const GLchar *)"TessLevelOuter");
//   param->fbo = glGetUniformLocation(pgid, (const GLchar *)"u_fbo");
//   param->mtxModelView = glGetUniformLocation(pgid, (const GLchar *)"u_mvpMatrix");
//   param->mtxTexture = glGetUniformLocation(pgid, (const GLchar *)"u_texMatrix");
//   param->tex0 = glGetUniformLocation(pgid, (const GLchar *)"s_texture");
//
// }

int Ygl_uniformVdp1CommonParam(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs, int id){
  YglProgram * prg;
//  YglVdp1CommonParam * param;

  prg = p;
  glEnableVertexAttribArray(prg->vertexp);
  glEnableVertexAttribArray(prg->texcoordp);
  if (prg->vaid > 0) {
    if (prg->vertexAttribute != NULL)
    {
      glEnableVertexAttribArray(prg->vaid);
    }
    else{
      glDisableVertexAttribArray(prg->vaid);
    }
  }

  glUniform2f(prg->ids->texsize, YglTM_vdp1[_Ygl->drawframe]->width, YglTM_vdp1[_Ygl->drawframe]->height);
  glUniform3i(prg->ids->sysclip, (int)(Vdp1Regs->systemclipX2 * _Ygl->vdp1wratio), (int)(Vdp1Regs->systemclipY2 * _Ygl->vdp1hratio), _Ygl->vdp1height);

  if (prg->ids->sprite != -1){
    glUniform1i(prg->ids->sprite, 0);
  }

  if (prg->ids->tessLevelInner != -1) {
    glUniform1f(prg->ids->tessLevelInner, (float)TESS_COUNT);
  }

  if (prg->ids->tessLevelOuter != -1) {
    glUniform1f(prg->ids->tessLevelOuter, (float)TESS_COUNT);
  }

  if (prg->ids->fbo != -1){
    glUniform1i(prg->ids->fbo, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _Ygl->vdp1FrameBuff[_Ygl->drawframe]);
    #if !defined(_OGLES3_)
        if (glTextureBarrier) glTextureBarrier();
        else if (glTextureBarrierNV) glTextureBarrierNV();
    #else
        if( glMemoryBarrier ){
          glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT|GL_TEXTURE_UPDATE_BARRIER_BIT|GL_TEXTURE_FETCH_BARRIER_BIT);
        }else{
          //glFinish();
        }
    #endif
  }
  return 0;
}

int Ygl_cleanupVdp1CommonParam(void * p, YglTextureManager *tm){
  YglProgram * prg;
  prg = p;
  if (prg->vaid > 0) {
    glDisableVertexAttribArray(prg->vaid);
  }
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, 0);
  return 0;
}

/*------------------------------------------------------------------------------------
 *  Normal Draw
 * ----------------------------------------------------------------------------------*/
const GLchar Yglprg_normal_v[] =
      SHADER_VERSION
      "uniform mat4 u_mvpMatrix;    \n"
      "layout (location = 0) in vec4 a_position;   \n"
      "layout (location = 1) in vec4 a_texcoord;   \n"
      "out  highp vec4 v_texcoord;     \n"
      "void main()       \n"
      "{ \n"
      "   gl_Position = a_position*u_mvpMatrix; \n"
      "   v_texcoord  = a_texcoord; \n"
      "} ";
const GLchar * pYglprg_vdp2_normal_v[] = {Yglprg_normal_v, NULL};

const GLchar Yglprg_normal_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;\n"
"precision highp int;\n"
"#endif\n"
"in vec4 v_texcoord;\n"
"uniform vec4 u_color_offset;\n"
"uniform float u_emu_height;\n"
"uniform float u_vheight; \n"
"uniform highp sampler2D s_texture;\n"
"uniform sampler2D s_perline;  \n"
"uniform int is_perline; \n"
"uniform sampler2D s_color;\n"
"out vec4 fragColor;\n"
"void main()   \n"
"{  \n"
"  ivec2 addr; \n"
"  ivec2 linepos; \n "
"  vec4 color_offset = u_color_offset; \n"
"  linepos.y = 0; \n "
"  linepos.x = int( (u_vheight-gl_FragCoord.y) * u_emu_height);\n"
"  addr.x = int(v_texcoord.x);  \n"
"  addr.y = int(v_texcoord.y);  \n"
"  vec4 txcol = texelFetch( s_texture, addr,0 );         \n"
"  int msb = int(txcol.b * 255.0)&0x1; \n"
"  if (is_perline == 1) {\n"
"    vec4 perline = texelFetch( s_perline, linepos,0 ); \n"
"    if (perline == vec4(0.0)) discard;\n"
"    color_offset.rgb = (perline.rgb - vec3(0.5))*2.0;\n"
"    if (perline.a > 0.0) txcol.a = perline.a;\n"
"  } \n"
"  fragColor.rgb = clamp(txcol.rgb+color_offset.rgb,vec3(0.0),vec3(1.0));\n"
"  int blue = int(fragColor.b * 255.0) & 0xFE;\n"
"  fragColor.b = float(blue|msb)/255.0;\n" //Blue LSB bit is used for special color calculation
"  fragColor.a = txcol.a;\n"
"}  \n";

const GLchar * pYglprg_vdp2_normal_f[] = {Yglprg_normal_f, NULL};
static int id_normal_s_texture = -1;
static int id_normal_color_offset = -1;
static int id_normal_matrix = -1;
static int id_normal_vheight = -1;
static int id_normal_emu_height = -1;


int Ygl_uniformNormal(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs, int id)
{

  YglProgram * prg;
  prg = p;
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glUniform1i(id_normal_s_texture, 0);
  glUniform4fv(prg->color_offset, 1, prg->color_offset_val);
  glUniform1f(id_normal_vheight, (float)_Ygl->rheight);
  glUniform1f(id_normal_emu_height,  (float)_Ygl->rheight / (float)_Ygl->rheight);
  return 0;
}

int Ygl_cleanupNormal(void * p, YglTextureManager *tm)
{
  YglProgram * prg;
  prg = p;
  return 0;
}


//---------------------------------------------------------
// Draw pxels refering color ram
//---------------------------------------------------------

const GLchar Yglprg_normal_cram_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;\n"
"precision highp int;\n"
"#endif\n"
"in vec4 v_texcoord;\n"
"uniform vec4 u_color_offset;\n"
"uniform float u_emu_height;\n"
"uniform float u_vheight; \n"
"uniform float u_vwidth; \n"
"uniform highp sampler2D s_texture;\n"
"uniform sampler2D s_perline;  \n"
"uniform int is_perline; \n"
"uniform sampler2D s_color;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  ivec2 linepos; \n "
"  vec4 color_offset = u_color_offset; \n"
"  linepos.y = 0; \n "
"  linepos.x = int( (u_vheight-gl_FragCoord.y) * u_emu_height);\n"
"  ivec2 addr = ivec2(int(v_texcoord.x),int(v_texcoord.y));\n"
"  vec4 txindex = texelFetch( s_texture, addr ,0 );\n"
"  if(txindex.a == 0.0) { discard; }\n"
"  vec4 txcol = texelFetch( s_color,  ivec2( int(txindex.g*255.0)<<8 | int(txindex.r*255.0) ,0 )  , 0 );\n"
"  int msb = int(txindex.b * 255.0)&0x1; \n"
"  if (is_perline == 1) {\n"
"    vec4 perline = texelFetch( s_perline, linepos,0 ); \n"
"    if (perline == vec4(0.0)) discard;\n"
"    color_offset.rgb = (perline.rgb - vec3(0.5))*2.0;\n"
"    if (perline.a > 0.0) txindex.a = float(int(perline.a * 255.0) | (int(txindex.a * 255.0) & 0x7))/255.0;\n"
"  } \n"
"  fragColor = clamp(txcol+color_offset,vec4(0.0),vec4(1.0));\n"
"  int blue = int(fragColor.b * 255.0) & 0xFE;\n"
"  fragColor.b = float(blue|msb)/255.0;\n" //Blue LSB bit is used for special color calculation
"  fragColor.a = txindex.a; \n"
"}\n";


const GLchar * pYglprg_normal_cram_f[] = { Yglprg_normal_cram_f, NULL };
static int id_normal_cram_s_texture = -1;
static int id_normal_cram_s_perline = -1;
static int id_normal_cram_isperline = -1;
static int id_normal_cram_emu_height = -1;
static int id_normal_cram_emu_width = -1;
static int id_normal_cram_vheight = -1;
static int id_normal_cram_vwidth = -1;
static int id_normal_cram_s_color = -1;
static int id_normal_cram_color_offset = -1;
static int id_normal_cram_matrix = -1;


int Ygl_uniformNormalCram(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs, int id)
{

  YglProgram * prg;
  prg = p;
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glUniform1i(id_normal_cram_s_texture, 0);
  glUniform1i(id_normal_cram_s_color, 1);
  glUniform1i(id_normal_cram_s_perline, 2);
  glUniform1i(id_normal_cram_isperline, (_Ygl->perLine[id] != 0));
  if ((id == RBG0) && (_Ygl->rbg_use_compute_shader)){
    glUniform1f(id_normal_cram_emu_height, (float)_Ygl->rheight / (float)_Ygl->height);
    glUniform1f(id_normal_cram_emu_width, (float)_Ygl->rwidth / (float)_Ygl->width);
    glUniform1f(id_normal_cram_vheight, (float)_Ygl->height);
    glUniform1f(id_normal_cram_vwidth, (float)_Ygl->width);
  } else {
    glUniform1f(id_normal_cram_emu_height, 1.0f);
    glUniform1f(id_normal_cram_emu_width, 1.0f);
    glUniform1f(id_normal_cram_vheight, (float)_Ygl->rheight);
    glUniform1f(id_normal_cram_vwidth, (float)_Ygl->rwidth);
  }
  glUniform4fv(prg->color_offset, 1, prg->color_offset_val);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, _Ygl->perLine[id]);
  return 0;
}

int Ygl_cleanupNormalCram(void * p, YglTextureManager *tm)
{
  YglProgram * prg;
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, 0);
  prg = p;
  return 0;
}

const GLchar Yglprg_normal_cram_addcol_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;\n"
"precision highp int;\n"
"#endif\n"
"in vec4 v_texcoord;\n"
"uniform vec4 u_color_offset;\n"
"uniform highp sampler2D s_texture;\n"
"uniform sampler2D s_color;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  vec4 txindex = texelFetch( s_texture, ivec2(int(v_texcoord.x),int(v_texcoord.y)) ,0 );\n"
"  if(txindex.a == 0.0) { discard; }\n"
"  vec4 txcol = texelFetch( s_color,  ivec2( ( int(txindex.g*255.0)<<8 | int(txindex.r*255.0)) ,0 )  , 0 );\n"
"  fragColor = txcol+u_color_offset;\n"
"  fragColor.a = txindex.a;\n"
"}\n";

const GLchar * pYglprg_normal_cram_addcol_f[] = { Yglprg_normal_cram_addcol_f, NULL };
static int id_normal_cram_s_texture_addcol = -1;
static int id_normal_cram_s_color_addcol = -1;
static int id_normal_cram_color_offset_addcol = -1;
static int id_normal_cram_matrix_addcol = -1;


int Ygl_uniformAddColCram(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs, int id)
{

  YglProgram * prg;
  prg = p;
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glUniform1i(id_normal_cram_s_texture_addcol, 0);
  glUniform1i(id_normal_cram_s_color_addcol, 1);
  glUniform4fv(id_normal_cram_color_offset_addcol, 1, prg->color_offset_val);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);
  return 0;
}

int Ygl_cleanupAddColCram(void * p, YglTextureManager *tm)
{
  YglProgram * prg;
  glActiveTexture(GL_TEXTURE0);
  prg = p;
  return 0;
}

static void Ygl_useTmpBuffer(){
  float col[4] = {0.0f,0.0f,0.0f,0.0f};
  // Create Screen size frame buffer
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &saveFB);
  if (_Ygl->tmpfbo == 0) {

    GLuint error;
    glGenTextures(1, &_Ygl->tmpfbotex);
    glBindTexture(GL_TEXTURE_2D, _Ygl->tmpfbotex);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _Ygl->rwidth, _Ygl->rheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &_Ygl->tmpfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->tmpfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _Ygl->tmpfbotex, 0);
  }
  // bind Screen size frame buffer
  else{
    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->tmpfbo);
    glClearBufferfv(GL_COLOR, 0, col);
  }
}

static void Ygl_releaseTmpBuffer(void) {
  glBindFramebuffer(GL_FRAMEBUFFER, saveFB);
}

int Ygl_useUpscaleBuffer(void){
  // Create Screen size frame buffer
  int up_scale = 1;
  switch (_Ygl->upmode) {
    case UP_HQ4X:
    case UP_4XBRZ:
      up_scale = 4;
      break;
    case UP_2XBRZ:
      up_scale = 2;
      break;
    default:
      up_scale = 1;
  }
  if (_Ygl->upfbo == 0) {
    GLuint error;
    glGenTextures(1, &_Ygl->upfbotex);
    glBindTexture(GL_TEXTURE_2D, _Ygl->upfbotex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, up_scale*_Ygl->rwidth, up_scale*_Ygl->rheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &_Ygl->upfbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->upfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _Ygl->upfbotex, 0);
  }
  // bind Screen size frame buffer
  else{
    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->upfbo);
  }
  return up_scale;
}

/*------------------------------------------------------------------------------------
*  Mosaic Draw
* ----------------------------------------------------------------------------------*/
int Ygl_uniformMosaic(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs, int id)
{
  YglProgram * prg;
  prg = p;

  Ygl_useTmpBuffer();
  glViewport(0, 0, _Ygl->rwidth, _Ygl->rheight);
  glScissor(0, 0, _Ygl->rwidth, _Ygl->rheight);

  if (prg->prgid == PG_VDP2_MOSAIC_CRAM ) {
    glEnableVertexAttribArray(prg->vertexp);
    glEnableVertexAttribArray(prg->texcoordp);
    glUniform1i(id_normal_cram_s_texture, 0);
    glUniform1i(id_normal_cram_s_color, 1);
    glUniform4fv(prg->color_offset, 1, prg->color_offset_val);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tm->textureID);

  }
  else {
    glEnableVertexAttribArray(prg->vertexp);
    glEnableVertexAttribArray(prg->texcoordp);
    glUniform1i(id_normal_s_texture, 0);
    glUniform4fv(prg->color_offset, 1, prg->color_offset_val);
    glBindTexture(GL_TEXTURE_2D, YglTM_vdp2->textureID);
  }

  return 0;
}

int Ygl_cleanupMosaic(void * p, YglTextureManager *tm)
{
  YglProgram * prg;
  prg = p;

  // Bind Default frame buffer
  Ygl_releaseTmpBuffer();

  // Restore Default Matrix
  glViewport(_Ygl->m_viewport[0], _Ygl->m_viewport[1], _Ygl->m_viewport[2], _Ygl->m_viewport[3]);
  glScissor(_Ygl->m_viewport[0], _Ygl->m_viewport[1], _Ygl->m_viewport[2], _Ygl->m_viewport[3]);

  // call blit method
  YglBlitMosaic(_Ygl->tmpfbotex, _Ygl->rwidth, _Ygl->rheight, prg->matrix, prg->mosaic);

  glBindTexture(GL_TEXTURE_2D, tm->textureID);

  return 0;
}

/*------------------------------------------------------------------------------------
 *  UserClip Operation
 * ----------------------------------------------------------------------------------*/
const GLchar Yglprg_userclip_v[] =
      "uniform mat4 u_mvpMatrix;    \n"
      "layout (location = 0) in vec4 a_position;               \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position*u_mvpMatrix; \n"
      "}\n";

const GLchar Yglprg_userclip_f[] =
      "#ifdef GL_ES\n"
      "precision highp float;                            \n"
      "#endif\n"
      "out vec4 fragColor;            \n"
      "void main()                                         \n"
      "{ \n"
      "  vec2 tag = vec2(0.0);\n"
      "  vec4 outColor = vec4( 0.0 );\n";

#define MESH_PROCESS \
"if( (int(gl_FragCoord.y) & 0x01) == 0 ){ \n \
  if( (int(gl_FragCoord.x) & 0x01) == 0 ){ \n \
    discard; \n \
  } \n \
}else{ \n \
  if( (int(gl_FragCoord.x) & 0x01) == 1 ){ \n \
    discard;\n \
  } \n \
} \n"

#define MESH_IMPROVED_PROCESS(A, B) \
" tag = "Stringify(A)".rg; \n \
  "Stringify(A)".rg = "Stringify(B)".rg; \n"

// we have a gouraud value, we can consider the pixel code is RGB otherwise gouraud effect is not guaranted (VDP1 doc p26)
#define GOURAUD_PROCESS(A) "\
int Rg = int(clamp((float((col"Stringify(A)" >> 00) & 0x1F)/31.0 + v_vtxcolor.r), 0.0, 1.0)*31.0);\n \
int Gg = int(clamp((float((col"Stringify(A)" >> 05) & 0x1F)/31.0 + v_vtxcolor.g), 0.0, 1.0)*31.0);\n \
int Bg = int(clamp((float((col"Stringify(A)" >> 10) & 0x1F)/31.0 + v_vtxcolor.b), 0.0, 1.0)*31.0);\n \
int MSBg = (col"Stringify(A)" & 0x8000) >> 8;\n \
"Stringify(A)".r = float(Rg | ((Gg & 0x7)<<5))/255.0;\n \
"Stringify(A)".g = float((Gg>>3) | (Bg<<2) | MSBg)/255.0;\n"

#define HALF_TRANPARENT_MIX(A, B) \
"if ((col"Stringify(B)" & 0x8000) != 0) { \
  int Rht = int(clamp(((float((col"Stringify(A)" >> 00) & 0x1F)/31.0) + (float((col"Stringify(B)" >> 00) & 0x1F)/31.0))*0.5, 0.0, 1.0)*31.0);\n \
  int Ght = int(clamp(((float((col"Stringify(A)" >> 05) & 0x1F)/31.0) + (float((col"Stringify(B)" >> 05) & 0x1F)/31.0))*0.5, 0.0, 1.0)*31.0);\n \
  int Bht = int(clamp(((float((col"Stringify(A)" >> 10) & 0x1F)/31.0) + (float((col"Stringify(B)" >> 10) & 0x1F)/31.0))*0.5, 0.0, 1.0)*31.0);\n \
  int MSBht = (col"Stringify(A)" & 0x8000) >> 8;\n \
  "Stringify(A)".r = float(Rht | ((Ght & 0x7)<<5))/255.0;\n \
  "Stringify(A)".g = float((Ght>>3) | (Bht<<2) | MSBht)/255.0;\n \
}\n"

#define HALF_LUMINANCE(A) \
"int Rhl = ((col"Stringify(A)" >> 00) & 0x1F)>>1;\n \
int Ghl = ((col"Stringify(A)" >> 05) & 0x1F)>>1;\n \
int Bhl = ((col"Stringify(A)" >> 10) & 0x1F)>>1;\n \
int MSBhl = (col"Stringify(A)" & 0x8000) >> 8;\n \
"Stringify(A)".r = float(Rhl | ((Ghl & 0x7)<<5))/255.0;\n \
"Stringify(A)".g = float((Ghl>>3) | (Bhl<<2) | MSBhl)/255.0;\n"

#define SHADOW(A) \
"if ((col"Stringify(A)" & 0x8000) != 0) { \n\
  int Rs = ((col"Stringify(A)" >> 00) & 0x1F)>>1;\n \
  int Gs = ((col"Stringify(A)" >> 05) & 0x1F)>>1;\n \
  int Bs = ((col"Stringify(A)" >> 10) & 0x1F)>>1;\n \
  int MSBs = (col"Stringify(A)" & 0x8000) >> 8;\n \
  "Stringify(A)".r = float(Rs | ((Gs & 0x7)<<5))/255.0;\n \
  "Stringify(A)".g = float((Gs>>3) | (Bs<<2) | MSBs)/255.0;\n \
} else discard;\n"


#define COLINDEX(A) \
"int col"Stringify(A)" = (int("Stringify(A)".r*255.0) | (int("Stringify(A)".g*255.0)<<8));\n"

#define RECOLINDEX(A) \
"col"Stringify(A)" = (int("Stringify(A)".r*255.0) | (int("Stringify(A)".g*255.0)<<8));\n"

#define COLZERO(A) \
"if (col"Stringify(A)" == 0) discard;\n"

#define TAGINDEX(A) \
"int tag"Stringify(A)" = (int("Stringify(A)".b*255.0) | (int("Stringify(A)".a*255.0)<<8));\n"

#define TAGZERO(A) \
"if (tag"Stringify(A)" != 0) discard;\n"

/*------------------------------------------------------------------------------------
*  VDP1 Operation with tessellation
* ----------------------------------------------------------------------------------*/
const GLchar Yglprg_gouraud_tess_c[] =
SHADER_VERSION_TESS
"layout(vertices = 4) out; //<???? what does it means? \n"
"in vec3 v_position[];  \n"
"in vec4 v_texcoord[]; \n"
"in vec4 v_vtxcolor[]; \n"
"out vec3 tcPosition[]; \n"
"out vec4 tcTexCoord[]; \n"
"out vec4 tcColor[]; \n"
"uniform float TessLevelInner; \n"
"uniform float TessLevelOuter; \n"
" \n"
"#define ID gl_InvocationID \n"
" \n"
"void main()  \n"
"{  \n"
"	tcPosition[ID] = v_position[ID];  \n"
"	tcTexCoord[ID] = v_texcoord[ID];  \n"
"	tcColor[ID] = v_vtxcolor[ID];  \n"
" \n"
"	if (ID == 0) {  \n"
"		gl_TessLevelInner[0] = TessLevelInner;  \n"
"		gl_TessLevelInner[1] = TessLevelInner;  \n"
"		gl_TessLevelOuter[0] = TessLevelOuter;  \n"
"		gl_TessLevelOuter[1] = TessLevelOuter; \n"
"		gl_TessLevelOuter[2] = TessLevelOuter; \n"
"		gl_TessLevelOuter[3] = TessLevelOuter; \n"
"	} \n"
"} \n";

const GLchar Yglprg_gouraud_tess_e[] =
SHADER_VERSION_TESS
"layout(quads, equal_spacing, ccw) in; \n"
"in vec3 tcPosition[]; \n"
"in vec4 tcTexCoord[]; \n"
"in vec4 tcColor[]; \n"
"out vec4 teTexCoord; \n"
"out vec4 teColor; \n"
"uniform mat4 u_mvpMatrix; \n"
" \n"
"void main() \n"
"{ \n"
"	float u = gl_TessCoord.x, v = gl_TessCoord.y; \n"
"	vec3 tePosition; \n"
"	vec3 a = mix(tcPosition[0], tcPosition[3], u); \n"
"	vec3 b = mix(tcPosition[1], tcPosition[2], u); \n"
"	tePosition = mix(a, b, v); \n"
"	gl_Position = vec4(tePosition, 1)*u_mvpMatrix; \n"
"	vec4 ta = mix(tcTexCoord[0], tcTexCoord[3], u); \n"
"	vec4 tb = mix(tcTexCoord[1], tcTexCoord[2], u); \n"
"	teTexCoord = mix(ta, tb, v); \n"
"	vec4 ca = mix(tcColor[0], tcColor[3], u); \n"
"	vec4 cb = mix(tcColor[1], tcColor[2], u); \n"
"	teColor = mix(ca, cb, v); \n"
"} \n";

const GLchar Yglprg_gouraud_tess_g[] =
SHADER_VERSION_TESS
"uniform mat4 Modelview; \n"
"uniform mat3 NormalMatrix; \n"
"layout(triangles) in; \n"
"layout(triangle_strip, max_vertices = 3) out; \n"
"in vec4 teTexCoord[3]; \n"
"in vec4 teColor[3]; \n"
"out vec4 v_texcoord; \n"
"out vec4 v_vtxcolor; \n"
" \n"
"void main() \n"
"{ \n"
"	v_texcoord = teTexCoord[0]; \n"
"	v_vtxcolor = teColor[0]; \n"
"	gl_Position = gl_in[0].gl_Position; EmitVertex(); \n"
" \n"
"	v_texcoord = teTexCoord[1]; \n"
"	v_vtxcolor = teColor[1]; \n"
"	gl_Position = gl_in[1].gl_Position; EmitVertex(); \n"
" \n"
"	v_texcoord = teTexCoord[2]; \n"
"	v_vtxcolor = teColor[2]; \n"
"	gl_Position = gl_in[2].gl_Position; EmitVertex(); \n"
" \n"
"	EndPrimitive(); \n"
"} \n";
/*------------------------------------------------------------------------------------
 *  VDP1 UserClip Operation
 * ----------------------------------------------------------------------------------*/
int Ygl_uniformStartUserClip(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs, int id )
{
   YglProgram * prg;
   prg = p;

   glEnableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   if( prg->ux1 != -1 )
   {

      GLint vertices[12];
      glColorMask( GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE );
      glStencilMask(0xffffffff);
      glClearBufferfi(GL_DEPTH_STENCIL, 0, 0, 0);

      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS,0x1,0x01);
      glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);

      // render
      vertices[0] = (int)((float)prg->ux1 * _Ygl->vdp1wratio);
      vertices[1] = (int)((float)prg->uy1 * _Ygl->vdp1hratio);
      vertices[2] = (int)((float)(prg->ux2+1) * _Ygl->vdp1wratio);
      vertices[3] = (int)((float)prg->uy1 * _Ygl->vdp1hratio);
      vertices[4] = (int)((float)(prg->ux2+1) * _Ygl->vdp1wratio);
      vertices[5] = (int)((float)(prg->uy2+1) * _Ygl->vdp1hratio);

      vertices[6] = (int)((float)prg->ux1 * _Ygl->vdp1wratio);
      vertices[7] = (int)((float)prg->uy1 * _Ygl->vdp1hratio);
      vertices[8] = (int)((float)(prg->ux2+1) * _Ygl->vdp1wratio);
      vertices[9] = (int)((float)(prg->uy2+1) * _Ygl->vdp1hratio);
      vertices[10] = (int)((float)prg->ux1 * _Ygl->vdp1wratio);
      vertices[11] = (int)((float)(prg->uy2+1) * _Ygl->vdp1hratio);

      glUniformMatrix4fv( prg->mtxModelView, 1, GL_FALSE, (GLfloat*) &_Ygl->mtxModelView.m[0][0]  );
      glBindBuffer(GL_ARRAY_BUFFER, _Ygl->vertices_buf);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
      glVertexAttribPointer(prg->vertexp,2, GL_INT,GL_FALSE, 0, 0 );
      glEnableVertexAttribArray(prg->vertexp);

      glDrawArrays(GL_TRIANGLES, 0, 6);

      glColorMask( GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE );
      glStencilFunc(GL_ALWAYS,0,0x0);
      glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
      glDisable(GL_STENCIL_TEST);
   }

   glEnable(GL_STENCIL_TEST);
   glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
   if( prg->uClipMode == 0x02 )
   {
      _Ygl->vdp1_stencil_mode = 1;
      glStencilFunc(GL_EQUAL,0x1,0xFF);
   }else if( prg->uClipMode == 0x03 )
   {
      _Ygl->vdp1_stencil_mode = 2;
      glStencilFunc(GL_EQUAL,0x0,0xFF);
   }else{
      _Ygl->vdp1_stencil_mode =3;
      glStencilFunc(GL_ALWAYS,0,0xFF);
   }
   glUniform3i(prg->ids->sysclip, (int)(Vdp1Regs->systemclipX2 * _Ygl->vdp1width)/512, (int)(Vdp1Regs->systemclipY2 * _Ygl->vdp1height)/256, _Ygl->vdp1height);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);

   //glDisable(GL_STENCIL_TEST);

   return 0;
}

int Ygl_cleanupStartUserClip(void * p, YglTextureManager *tm ){return 0;}

int Ygl_uniformEndUserClip(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs, int id )
{

   YglProgram * prg;
   prg = p;
   glDisable(GL_STENCIL_TEST);
   _Ygl->vdp1_stencil_mode = 0;
   glStencilFunc(GL_ALWAYS,0,0xFF);

   return 0;
}

int Ygl_cleanupEndUserClip(void * p, YglTextureManager *tm ){return 0;}

const GLchar version_core_3_3[] = {
SHADER_VERSION
};

const GLchar version_core_4_2[] = {
SHADER_VERSION_TESS
};

const GLchar* vdp1drawversion[2]= {
  version_core_3_3,
  version_core_4_2
};

const GLchar vdp1drawstart[] = {
  "#ifdef GL_ES\n"
  "precision highp float;\n"
  "#endif\n"
  "uniform sampler2D u_sprite;\n"
  "uniform sampler2D u_fbo;\n"
  "uniform ivec3 sysClip;\n"
  "in vec4 v_texcoord;\n"
  "in vec4 v_vtxcolor; \n"
  "out vec4 fragColor; \n"
  "void main() {\n"
  "  vec4 outColor = vec4(0.0);\n"
  "  if (any(greaterThan(ivec2(gl_FragCoord.x, sysClip.z - gl_FragCoord.y), sysClip.xy))) discard;\n"
  "  ivec2 addr = ivec2(vec2(textureSize(u_sprite, 0)) * v_texcoord.st / v_texcoord.q); \n"
  "  vec4 spriteColor = texelFetch(u_sprite,addr,0);\n"
  "  vec2 tag = vec2(0.0);\n"
  COLINDEX(spriteColor)
};

//SPD Mode handling
const GLchar spd_on[] = {
COLZERO(spriteColor)
};

const GLchar spd_off[] =
{"//No Spd\n"};

const GLchar* vdp1drawcheck[2]= {
  spd_on,
  spd_off
};

//END Mode handling
const GLchar end_on[] = {
TAGINDEX(spriteColor)
TAGZERO(spriteColor)
};

const GLchar end_off[] =
{"//No End\n"};

const GLchar* vdp1drawcheckend[2]= {
  end_on,
  end_off
};

//Mesh Mode handling
const GLchar no_mesh[] =
{"//No mesh\n"};

const GLchar mesh[] = {
MESH_PROCESS
};

const GLchar improved_mesh[] = {
  "vec4 curColor = texelFetch(u_fbo,ivec2(gl_FragCoord.xy),0);\n"
  MESH_IMPROVED_PROCESS(spriteColor, curColor)
  RECOLINDEX(spriteColor)
};

const GLchar* vdp1drawmesh[3]= {
  no_mesh,
  mesh,
  improved_mesh
};

//MSB process
const GLchar no_msb[] =
{"//No MSB\n"};

const GLchar msb[] = {
  "  vec4 currentColor = texelFetch(u_fbo,ivec2(gl_FragCoord.xy),0);\n"
  "  currentColor.g = float(int(currentColor.g * 255.0)|0x80)/255.0;\n"
  "  fragColor = currentColor;\n"
  "  return;\n"
};

const GLchar* vdp1drawmsb[2]= {
  no_msb,
  msb
};

//Color calculation mode
const GLchar replace_mode[] = {
  "outColor.rg = spriteColor.rg;\n"
};

const GLchar shadow_mode[] = {
  "vec4 fboColor    = texelFetch(u_fbo,ivec2(gl_FragCoord.xy),0);\n"
  COLINDEX(fboColor)
  SHADOW(fboColor)
    "outColor.rg = fboColor.rg;\n"
};

const GLchar half_luminance_mode[] = {
  HALF_LUMINANCE(spriteColor)
  "outColor.rg = spriteColor.rg;\n"
};

const GLchar half_trans_mode[] = {
  "vec4 fboColor    = texelFetch(u_fbo,ivec2(gl_FragCoord.xy),0);\n"
  COLINDEX(fboColor)
  HALF_TRANPARENT_MIX(spriteColor, fboColor)
  "outColor.rg = spriteColor.rg;\n"
};

const GLchar gouraud_mode[] = {
  GOURAUD_PROCESS(spriteColor)
  "outColor.rg = spriteColor.rg;\n"
};

const GLchar gouraud_half_luminance_mode[] = {
  GOURAUD_PROCESS(spriteColor)
  RECOLINDEX(spriteColor)
  HALF_LUMINANCE(spriteColor)
  "outColor.rg = spriteColor.rg;\n"
};

const GLchar gouraud_half_trans_mode[] = {
  GOURAUD_PROCESS(spriteColor)
  RECOLINDEX(spriteColor)
  "vec4 fboColor    = texelFetch(u_fbo,ivec2(gl_FragCoord.xy),0);\n"
  COLINDEX(fboColor)
  HALF_TRANPARENT_MIX(spriteColor, fboColor)
  "outColor.rg = spriteColor.rg;\n"
};

const GLchar nothing_mode[] =
{"//No CC mode\n"};

const GLchar* vdp1drawmode[8]= {
  replace_mode,
  shadow_mode,
  half_luminance_mode,
  half_trans_mode,
  gouraud_mode,
  gouraud_half_luminance_mode,
  gouraud_half_trans_mode,
  nothing_mode
};

//ENd of shaders
const GLchar vdp1drawend[] = {
  "  fragColor.rgba = vec4(outColor.rg, tag);\n"
  "}\n"
};

//Common Vertex shader
const GLchar vdp1drawvertex_normal[] = {
  "layout (location = 0) in vec4 a_position; \n"
  "layout (location = 1) in vec4 a_texcoord; \n"
  "layout (location = 2) in vec4 a_grcolor;  \n"
  "uniform vec2 u_texsize;    \n"
  "uniform mat4 u_mvpMatrix; \n"
  "out vec3 v_position;  \n"
  "out vec4 v_texcoord; \n"
  "out vec4 v_vtxcolor; \n"
  "void main() {     \n"
  "   gl_Position  = a_position*u_mvpMatrix; \n"
  "   v_vtxcolor  = a_grcolor;  \n"
  "   v_texcoord  = a_texcoord; \n"
  "   v_texcoord.x  = v_texcoord.x / u_texsize.x; \n"
  "   v_texcoord.y  = v_texcoord.y / u_texsize.y; \n"
  "}\n"
};

const GLchar vdp1drawvertex_tess[] = {
  "layout (location = 0) in vec4 a_position; \n"
  "layout (location = 1) in vec4 a_texcoord; \n"
  "layout (location = 2) in vec4 a_grcolor;  \n"
  "uniform vec2 u_texsize;    \n"
  "uniform mat4 u_mvpMatrix; \n"
  "out vec3 v_position;  \n"
  "out vec4 v_texcoord; \n"
  "out vec4 v_vtxcolor; \n"
  "void main() {     \n"
  "   v_position  = a_position.xyz; \n"
  "   v_vtxcolor  = a_grcolor;  \n"
  "   v_texcoord  = a_texcoord; \n"
  "   v_texcoord.x  = v_texcoord.x / u_texsize.x; \n"
  "   v_texcoord.y  = v_texcoord.y / u_texsize.y; \n"
  "}\n"
};

const GLchar* vdp1drawvertex[2]= {
  vdp1drawvertex_normal,
  vdp1drawvertex_tess
};

/*------------------------------------------------------------------------------------
 *  VDP2 Draw Frame buffer Operation
 * ----------------------------------------------------------------------------------*/

typedef struct  {
  int idvdp1FrameBuffer;
  int idvdp2regs;
  int idcram;
} DrawFrameBufferUniform;

#define MAX_FRAME_BUFFER_UNIFORM (128*5)

DrawFrameBufferUniform g_draw_framebuffer_uniforms[MAX_FRAME_BUFFER_UNIFORM];

const GLchar Yglprg_vdp1_drawfb_v[] =
      SHADER_VERSION
      "layout (location = 0) in vec2 a_position;   \n"
      "layout (location = 1) in vec2 a_texcoord;   \n"
      "out vec2 v_texcoord;     \n"
      "void main()       \n"
      "{ \n"
      " gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0); \n"
      " v_texcoord  = a_texcoord; \n"
      "} ";
const GLchar * pYglprg_vdp2_blit_v[] = {Yglprg_vdp1_drawfb_v, NULL};

/*
+-+-+-+-+-+-+-+-+
|S|C|A|A|A|P|P|P|
+-+-+-+-+-+-+-+-+
S show flag
C index or direct color
A alpha index
P priority

refrence:
  hard/vdp2/hon/p09_10.htm
  hard/vdp2/hon/p12_14.htm#CCCTL_

*/

//--------------------------------------------------------------------------------------------------------------
static const char vdp2blit_gl_start_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float; \n"
"#endif\n"
"in vec2 v_texcoord; \n"
"uniform int u_lncl[7];  \n"
"out vec4 finalColor; \n"
"uniform sampler2D s_vdp2reg; \n"
"uniform float u_emu_height;\n"
"uniform float u_emu_vdp1_width;\n"
"uniform float u_emu_vdp2_width;\n"
"uniform float u_vheight; \n"

"uniform vec2 vdp1Ratio;\n"
"uniform int fbon;  \n"
"uniform int screen_nb;  \n"
"uniform int mode[7];  \n"
"uniform int isRGB[6]; \n"
"uniform int isBlur[7]; \n"
"uniform int isShadow[6]; \n"
"uniform mat4 fbMat;\n"
"uniform int win_s[8]; \n"
"uniform int win_s_mode[8]; \n"
"uniform int win0[8]; \n"
"uniform int win0_mode[8]; \n"
"uniform int win1[8]; \n"
"uniform int win1_mode[8]; \n"
"uniform int win_op[8]; \n"
"uniform int ram_mode; \n"
"uniform int extended_cc; \n"
"uniform int use_sp_win; \n"
"uniform int use_trans_shadow; \n"
"uniform ivec2 tvSize;\n"
#ifdef DEBUG_BLIT
"out vec4 topColor; \n"
"out vec4 secondColor; \n"
"out vec4 thirdColor; \n"
"out vec4 fourthColor; \n"
#endif
"uniform sampler2D s_texture0;  \n"
"uniform sampler2D s_texture1;  \n"
"uniform sampler2D s_texture2;  \n"
"uniform sampler2D s_texture3;  \n"
"uniform sampler2D s_texture4;  \n"
"uniform sampler2D s_texture5;  \n"
"uniform sampler2D s_back;  \n"
"uniform sampler2D s_lncl;  \n"
"uniform sampler2D s_vdp1FrameBuffer;\n"
"uniform sampler2D s_win0;  \n"
"uniform sampler2D s_win1;  \n"
"uniform sampler2D s_color; \n"
"int PosY = int(gl_FragCoord.y);\n"
"int PosX = int(gl_FragCoord.x);\n"
;

const GLchar Yglprg_vdp2_drawfb_gl_cram_f[] =
"int getVDP2Reg(int id) {\n"
"  return int(texelFetch(s_vdp2reg, ivec2(id, 0), 0).r*255.0);\n"
"}\n"
"FBCol getFB(int x, ivec2 addr){ \n"
"  vec4 lineCoord = vec4(gl_FragCoord.x, (u_vheight-gl_FragCoord.y), 0.0, 0.0);\n"
"  int line = int(lineCoord.y * u_emu_height)*24;\n"
"  vec3 u_coloroffset = vec3(texelFetch(s_vdp2reg, ivec2(17 + line,0), 0).r, texelFetch(s_vdp2reg, ivec2(18+line,0), 0).r, texelFetch(s_vdp2reg, ivec2(19+line,0), 0).r);\n"
"  vec3 u_coloroffset_sign = vec3(texelFetch(s_vdp2reg, ivec2(20 + line, 0), 0).r, texelFetch(s_vdp2reg, ivec2(21+line,0), 0).r, texelFetch(s_vdp2reg, ivec2(22+line,0), 0).r);\n";

static const GLchar vdp2blit_gl_end_f[] =
"";

static const GLchar vdp2blit_gl_final_f[] =
#ifdef DEBUG_BLIT
"  topColor = colortop;\n"
"  secondColor = colorsecond;\n"
"  thirdColor = FBTest;\n"
"  fourthColor = FBShadow;\n"
#endif
"} \n";

const GLchar * prg_input_f[PG_MAX][9];
const GLchar * prg_input_v[PG_MAX][3];
const GLchar * prg_input_c[PG_MAX][2];
const GLchar * prg_input_e[PG_MAX][2];
const GLchar * prg_input_g[PG_MAX][2];

void initDrawShaderCode() {
  initVDP2DrawCode(vdp2blit_gl_start_f, Yglprg_vdp2_drawfb_gl_cram_f, vdp2blit_gl_end_f, vdp2blit_gl_final_f);

  //VDP1 Programs
  for (int m = 0; m<2; m++) {
    //Normal or tesselation mode
    for (int i = 0; i<2; i++) {
       // MSB or not MSB
      for (int j = 0; j<3; j++) {
       // Mesh, Mesh improve or None
        for (int k = 0; k<2; k++) {
          //SPD or not
          for (int k1 = 0; k1<2; k1++) {
            //END code or not
            for (int l = 0; l<7; l++) {
              //7 color calculation mode
              int index = l+7*(k1+2*(k+2*(j+3*(i+2*m))));
              prg_input_f[index][0] = vdp1drawversion[m];
              prg_input_f[index][1] = vdp1drawstart;
              prg_input_f[index][2] = vdp1drawcheckend[k1];
              prg_input_f[index][3] = vdp1drawcheck[k];
              prg_input_f[index][4] = vdp1drawmsb[i];
              prg_input_f[index][5] = vdp1drawmode[l];
              prg_input_f[index][6] = vdp1drawmesh[j];
              prg_input_f[index][7] = vdp1drawend;
              prg_input_f[index][8] =  NULL;

              prg_input_v[index][0] = vdp1drawversion[m];
              prg_input_v[index][1] = vdp1drawvertex[m];
              prg_input_v[index][2] = NULL;

              if(m!=1) {
                prg_input_c[index][0] = NULL;
                prg_input_c[index][1] = NULL;
                prg_input_e[index][0] = NULL;
                prg_input_e[index][1] = NULL;
                prg_input_g[index][0] = NULL;
                prg_input_g[index][1] = NULL;
              } else {
                prg_input_c[index][0] = Yglprg_gouraud_tess_c;
                prg_input_c[index][1] = NULL;
                prg_input_e[index][0] = Yglprg_gouraud_tess_e;
                prg_input_e[index][1] = NULL;
                prg_input_g[index][0] = Yglprg_gouraud_tess_g;
                prg_input_g[index][1] = NULL;
              }
            }
          }
        }
      }
    }
  }
  //Handle start and end user clip

  //Start user clip
  prg_input_f[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][0] = vdp1drawversion[0];
  prg_input_f[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][1] = Yglprg_userclip_f;
  prg_input_f[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][2] = vdp1drawcheckend[1];
  prg_input_f[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][3] = vdp1drawcheck[1];
  prg_input_f[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][4] = vdp1drawmsb[0];
  prg_input_f[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][5] = vdp1drawmode[7];
  prg_input_f[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][6] = vdp1drawmesh[0];
  prg_input_f[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][7] = vdp1drawend;
  prg_input_f[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][8] = NULL;

  prg_input_v[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][0] = vdp1drawversion[0];
  prg_input_v[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][1] = Yglprg_userclip_v;
  prg_input_v[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][2] = NULL;

  prg_input_c[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][0] = NULL;
  prg_input_c[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][1] = NULL;
  prg_input_e[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][0] = NULL;
  prg_input_e[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][1] = NULL;
  prg_input_g[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][0] = NULL;
  prg_input_g[PG_VDP1_STARTUSERCLIP - PG_VDP1_START][1] = NULL;

  //End user clip
  prg_input_f[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][0] = vdp1drawversion[0];
  prg_input_f[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][1] = Yglprg_userclip_f;
  prg_input_f[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][2] = vdp1drawcheckend[1];
  prg_input_f[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][3] = vdp1drawcheck[1];
  prg_input_f[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][4] = vdp1drawmsb[0];
  prg_input_f[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][5] = vdp1drawmode[7];
  prg_input_f[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][6] = vdp1drawmesh[0];
  prg_input_f[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][7] = vdp1drawend;
  prg_input_f[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][8] = NULL;

  prg_input_v[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][0] = vdp1drawversion[0];
  prg_input_v[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][1] = Yglprg_userclip_v;
  prg_input_v[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][7] = NULL;

  prg_input_c[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][0] = NULL;
  prg_input_c[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][1] = NULL;
  prg_input_e[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][0] = NULL;
  prg_input_e[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][1] = NULL;
  prg_input_g[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][0] = NULL;
  prg_input_g[PG_VDP1_ENDUSERCLIP - PG_VDP1_START][1] = NULL;

}

int YglInitDrawFrameBufferShaders(int id, int CS) {
 //printf ("Use prog %d\n", id); //16
 int arrayid = id-PG_VDP2_DRAWFRAMEBUFF_NONE;
 //printf ("getArray %d\n", arrayid); //16
  compileVDP2Prog(id, pYglprg_vdp2_blit_v, CS);
  if ( arrayid < 0 || arrayid >= MAX_FRAME_BUFFER_UNIFORM) {
    abort();
  }
  g_draw_framebuffer_uniforms[arrayid].idvdp1FrameBuffer = glGetUniformLocation(_prgid[id], (const GLchar *)"s_vdp1FrameBuffer");
  g_draw_framebuffer_uniforms[arrayid].idvdp2regs = glGetUniformLocation(_prgid[id], (const GLchar *)"s_vdp2reg");
  g_draw_framebuffer_uniforms[arrayid].idcram = glGetUniformLocation(_prgid[id], (const GLchar *)"s_color");
  return 0;
}


/*------------------------------------------------------------------------------------
*  VDP2 Draw Frame buffer Operation( Shadow drawing for ADD color mode )
* ----------------------------------------------------------------------------------*/

int Ygl_uniformVDP2DrawFramebuffer(float * offsetcol, Vdp2* varVdp2Regs)
{
   int arrayid;

   int pgid = setupVDP2Prog(varVdp2Regs, 0);

  arrayid = pgid - PG_VDP2_DRAWFRAMEBUFF_NONE;

  if (_prgid[pgid] == 0) {
    if (YglInitDrawFrameBufferShaders(pgid, 0) != 0) {
      return -1;
    }
  }
  GLUSEPROG(_prgid[pgid]);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glDisableVertexAttribArray(3);

  glUniform1i(g_draw_framebuffer_uniforms[arrayid].idcram, 11);
  glActiveTexture(GL_TEXTURE11);
  glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);

  glUniform1i(g_draw_framebuffer_uniforms[arrayid].idvdp2regs, 12);
  glActiveTexture(GL_TEXTURE12);
  glBindTexture(GL_TEXTURE_2D, _Ygl->vdp2reg_tex);

  glUniform1i(g_draw_framebuffer_uniforms[arrayid].idvdp1FrameBuffer, 9);
  return _prgid[pgid];
}

/*------------------------------------------------------------------------------------
 *  VDP2 Add Blend operaiotn
 * ----------------------------------------------------------------------------------*/
int Ygl_uniformAddBlend(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs, int id )
{
   glBlendFunc(GL_ONE,GL_ONE);
   return 0;
}

int Ygl_cleanupAddBlend(void * p, YglTextureManager *tm)
{
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
   return 0;
}

int YglProgramInit()
{
   YGLLOG("PG_VDP2_NORMAL\n");

   initDrawShaderCode();
   //
   if (YglInitShader(PG_VDP2_NORMAL, pYglprg_vdp2_normal_v, 1, pYglprg_vdp2_normal_f, 1, NULL, NULL, NULL) != 0)
      return -1;
//vdp2 normal looks not to be setup as it should
  id_normal_s_texture = glGetUniformLocation(_prgid[PG_VDP2_NORMAL], (const GLchar *)"s_texture");
  //id_normal_s_texture_size = glGetUniformLocation(_prgid[PG_VDP2_NORMAL], (const GLchar *)"u_texsize");
  id_normal_color_offset = glGetUniformLocation(_prgid[PG_VDP2_NORMAL], (const GLchar *)"u_color_offset");
  id_normal_matrix = glGetUniformLocation(_prgid[PG_VDP2_NORMAL], (const GLchar *)"u_mvpMatrix");
  id_normal_vheight = glGetUniformLocation(_prgid[PG_VDP2_NORMAL], (const GLchar *)"u_vheight");
  id_normal_emu_height = glGetUniformLocation(_prgid[PG_VDP2_NORMAL], (const GLchar *)"u_emu_height");

   YGLLOG("PG_VDP2_NORMAL_CRAM\n");

  if (YglInitShader(PG_VDP2_NORMAL_CRAM, pYglprg_vdp2_normal_v, 1, pYglprg_normal_cram_f, 1, NULL, NULL, NULL) != 0)
    return -1;

  id_normal_cram_s_texture = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"s_texture");
  id_normal_cram_s_color = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"s_color");
  id_normal_cram_s_perline = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"s_perline");
  id_normal_cram_isperline = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"is_perline");
  id_normal_cram_color_offset = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"u_color_offset");
  id_normal_cram_matrix = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"u_mvpMatrix");
  id_normal_cram_emu_height = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"u_emu_height");
  id_normal_cram_vheight = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"u_vheight");
  id_normal_cram_vwidth = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"u_vwidth");


#if 0
  YGLLOG("PG_VDP2_MOSAIC\n");
  if (YglInitShader(PG_VDP2_MOSAIC, pYglprg_vdp1_replace_v, 1, pYglprg_mosaic_f, NULL, NULL, NULL) != 0)
    return -1;
  id_mosaic_s_texture = glGetUniformLocation(_prgid[PG_VDP2_MOSAIC], (const GLchar *)"s_texture");
  id_mosaic = glGetUniformLocation(_prgid[PG_VDP2_MOSAIC], (const GLchar *)"u_mosaic");
  id_mosaic_color_offset = glGetUniformLocation(_prgid[PG_VDP2_MOSAIC], (const GLchar *)"u_color_offset");
#endif

   _prgid[PG_VDP2_MOSAIC] = _prgid[PG_VDP2_NORMAL];

   _prgid[PG_VDP2_MOSAIC_CRAM] = _prgid[PG_VDP2_NORMAL_CRAM];

   return 0;
}

void initVDPProg(YglProgram* prog, int id) {
  int ret = -1;
  int init = 0;
  prog->vaid = 0;
  prog->id = 0;

  if (_prgid[id] == 0) {
    YGLLOG("Compile program %d\n",id);
    init = 1;
    YglInitShader(id, prg_input_v[id-PG_VDP1_START], 2, prg_input_f[id-PG_VDP1_START], 8, prg_input_c[id-PG_VDP1_START],prg_input_e[id-PG_VDP1_START],prg_input_g[id-PG_VDP1_START]);
  }
  if (_prgid[id] == 0) {
    YuiMsg("Prog %d is not able to compile\n", id);
    abort();
  }
  if (init == 1) {
    _ids[id].sprite = glGetUniformLocation(_prgid[id], (const GLchar *)"u_sprite");
    _ids[id].tessLevelInner = glGetUniformLocation(_prgid[id], (const GLchar *)"TessLevelInner");
    _ids[id].tessLevelOuter = glGetUniformLocation(_prgid[id], (const GLchar *)"TessLevelOuter");
    _ids[id].fbo = glGetUniformLocation(_prgid[id], (const GLchar *)"u_fbo");
    _ids[id].texsize = glGetUniformLocation(_prgid[id], (const GLchar *)"u_texsize");
    _ids[id].mtxModelView = glGetUniformLocation(_prgid[id], (const GLchar *)"u_mvpMatrix");
    _ids[id].tex0 = glGetUniformLocation(_prgid[id], (const GLchar *)"s_texture");
    _ids[id].vaid = glGetAttribLocation(_prgid[id], (const GLchar *)"a_grcolor");
    _ids[id].vertexp = glGetAttribLocation(_prgid[id], (const GLchar *)"a_position");
    _ids[id].texcoordp = glGetAttribLocation(_prgid[id], (const GLchar *)"a_texcoord");
    _ids[id].sysclip = glGetUniformLocation(_prgid[id], (const GLchar *)"sysClip");
  }
  prog->prgid=id;
  prog->prg=_prgid[id];
  prog->vaid = _ids[id].vaid;
  prog->mtxModelView = _ids[id].mtxModelView;
  switch(id) {
    case PG_VDP1_STARTUSERCLIP:
    case PG_VDP1_ENDUSERCLIP:
      prog->setupUniform = Ygl_uniformStartUserClip;
      prog->cleanupUniform = Ygl_cleanupStartUserClip;
      prog->vertexp = 0;//glGetUniformLocation(_prgid[id], (const GLchar *)"a_position");
      prog->texcoordp = -1;//glGetUniformLocation(_prgid[id], (const GLchar *)"a_texcoord");
    break;
    default:
      prog->setupUniform = Ygl_uniformVdp1CommonParam;
      prog->cleanupUniform = Ygl_cleanupVdp1CommonParam;
      prog->vertexp = _ids[id].vertexp;
      prog->texcoordp = _ids[id].texcoordp;
  }
  prog->ids = &_ids[id];
  // YGLLOG("Compile program %d success\n",id);
}

int YglProgramChange( YglLevel * level, int prgid )
{
   YglProgram* tmp;
   YglProgram* current;
#if  USEVBO
   int maxsize;
#endif

   level->prgcurrent++;

   if( level->prgcurrent >= level->prgcount)
   {
      level->prgcount++;
      tmp = (YglProgram*)malloc(sizeof(YglProgram)*level->prgcount);
      if( tmp == NULL ) return -1;
      memset(tmp,0,sizeof(YglProgram)*level->prgcount);
      memcpy(tmp,level->prg,sizeof(YglProgram)*(level->prgcount-1));
      free(level->prg);
      level->prg = tmp;

      level->prg[level->prgcurrent].currentQuad = 0;
#if  USEVBO
       level->prg[level->prgcurrent].maxQuad = 14692;
      maxsize = level->prg[level->prgcurrent].maxQuad;
      if( YglGetVertexBuffer(maxsize,
  (void**)&level->prg[level->prgcurrent].quads,
  (void**)&level->prg[level->prgcurrent].textcoords,
  (void**)&level->prg[level->prgcurrent].vertexAttribute  ) != 0 ) {
          return -1;
      }
      if( level->prg[level->prgcurrent].quads == 0 )
      {
          int a=0;
      }
#else
      level->prg[level->prgcurrent].maxQuad = 12*64;
      if ((level->prg[level->prgcurrent].quads = (float *) malloc(level->prg[level->prgcurrent].maxQuad * sizeof(float))) == NULL)
         return -1;

      if ((level->prg[level->prgcurrent].textcoords = (float *) malloc(level->prg[level->prgcurrent].maxQuad * sizeof(float) * 2)) == NULL)
         return -1;

       if ((level->prg[level->prgcurrent].vertexAttribute = (float *) malloc(level->prg[level->prgcurrent].maxQuad * sizeof(float)*2)) == NULL)
         return -1;
#endif
   }
   current = &level->prg[level->prgcurrent];
   initVDPProg(current, prgid);

   if (prgid == PG_VDP2_NORMAL)
   {
     current->setupUniform = Ygl_uniformNormal;
     current->cleanupUniform = Ygl_cleanupNormal;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = id_normal_matrix;
     current->color_offset = id_normal_color_offset;
   }
   else if (prgid == PG_VDP2_NORMAL_CRAM)
   {
     current->setupUniform = Ygl_uniformNormalCram;
     current->cleanupUniform = Ygl_cleanupNormalCram;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = id_normal_cram_matrix;
     current->color_offset = id_normal_cram_color_offset;

   }
   else if (prgid == PG_VDP2_MOSAIC)
   {
     current->setupUniform = Ygl_uniformMosaic;
     current->cleanupUniform = Ygl_cleanupMosaic;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = id_normal_matrix;

   }
   else if (prgid == PG_VDP2_MOSAIC_CRAM)
   {
     current->setupUniform = Ygl_uniformMosaic;
     current->cleanupUniform = Ygl_cleanupMosaic;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = id_normal_cram_matrix;
   }
   return 0;

}

//----------------------------------------------------------------------------------------
static int clear_prg = -1;

static const char vclear_img[] =
  SHADER_VERSION
  "layout (location = 0) in vec2 aPosition;   \n"
  "  \n"
  " void main(void) \n"
  " { \n"
  " gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0); \n"
  " } \n";


static const char fclear_img[] =
  SHADER_VERSION
  "#ifdef GL_ES\n"
  "precision highp float;       \n"
  "#endif\n"
  "uniform float u_emu_height; \n"
  "uniform float u_vheight; \n"
  "uniform sampler2D u_Clear;     \n"
  "out vec4 fragColor; \n"
  "void main()   \n"
  "{  \n"
"    ivec2 linepos; \n "
"    linepos.y = 0; \n "
"    linepos.x = int( (u_vheight-gl_FragCoord.y) * u_emu_height);\n"
  "  fragColor = texelFetch( u_Clear, linepos,0 ); \n"
  "} \n";

int YglDrawBackScreen() {

  float const vertexPosition[] = {
    1.0f, -1.0f,
    -1.0f, -1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f };

  if (clear_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;

    const GLchar * vclear_img_v[] = { vclear_img, NULL };
    const GLchar * fclear_img_v[] = { fclear_img, NULL };

    clear_prg = glCreateProgram();
    if (clear_prg == 0){
      return -1;
    }

    YGLLOG("DRAW_BACK_SCREEN\n");

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, vclear_img_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      clear_prg = -1;
      return -1;
    }
    glShaderSource(fshader, 1, fclear_img_v, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      clear_prg = -1;
      return -1;
    }

    glAttachShader(clear_prg, vshader);
    glAttachShader(clear_prg, fshader);
    glLinkProgram(clear_prg);
    glGetProgramiv(clear_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      YGLLOG("Link error..\n");
      Ygl_printShaderError(clear_prg);
      clear_prg = -1;
      return -1;
    }

    GLUSEPROG(clear_prg);
    glUniform1i(glGetUniformLocation(clear_prg, "u_Clear"), 0);
  }
  else{
    GLUSEPROG(clear_prg);
  }
  glUniform1f(glGetUniformLocation(clear_prg, "u_emu_height"), (float)_Ygl->rheight / (float)_Ygl->rheight);
  glUniform1f(glGetUniformLocation(clear_prg, "u_vheight"), (float)_Ygl->rheight);

  glDisable(GL_STENCIL_TEST);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, _Ygl->vertexPosition_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPosition), vertexPosition, GL_STREAM_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _Ygl->back_tex);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Clean up
  glDisableVertexAttribArray(0);

  return 0;
}

extern vdp2rotationparameter_struct  Vdp1ParaA;

int YglBlitTexture(YglPerLineInfo *bg, int* prioscreens, int* modescreens, int* isRGB, int * isBlur, int* isShadow, int* lncl, GLuint* vdp1fb, int* Win_s, int* Win_s_mode, int* Win0, int* Win0_mode, int* Win1, int* Win1_mode, int* Win_op, Vdp2 *varVdp2Regs) {
  int perLine = 0;
  int nbScreen = 6;
  int vdp2blit_prg;

  float const vertexPosition[] = {
    1.0, -1.0f,
    -1.0, -1.0f,
    1.0, 1.0f,
    -1.0, 1.0f };

  float const textureCoord[] = {
    1.0f, 0.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f
  };
    float offsetcol[4];
    YglMatrix vdp1Mat;

    YglLoadIdentity(&vdp1Mat);

    if (Vdp1Regs->TVMR & 0x02) {
      YglMatrix translate, rotation, scale, fuse;
      //Translate to center of rotation
      YglLoadIdentity(&translate);
      translate.m[0][3] = (-Vdp1ParaA.Cx - (float)_Ygl->rwidth/2.0f )* (float)_Ygl->vdp1width/512.0f;
      translate.m[1][3] = (-Vdp1ParaA.Cy - (float)_Ygl->rheight/2.0f)* (float)_Ygl->vdp1height/256.0f;

      //Il faut calculer ou partent les points...

      //Rotate and translate back to the (Xst,Yst) from (0,0)
      YglLoadIdentity(&rotation);
      rotation.m[0][0] = Vdp1ParaA.deltaX;
      rotation.m[0][1] = Vdp1ParaA.deltaY;
      rotation.m[0][3] = (Vdp1ParaA.Xst + Vdp1ParaA.Cx + (float)_Ygl->rwidth/2.0f)* (float)_Ygl->vdp1width/512.0f;
      rotation.m[1][0] = Vdp1ParaA.deltaXst;
      rotation.m[1][1] = Vdp1ParaA.deltaYst;
      rotation.m[1][3] = (Vdp1ParaA.Yst + Vdp1ParaA.Cy + (float)_Ygl->rheight/2.0f)* (float)_Ygl->vdp1height/256.0f;

      YglLoadIdentity(&scale);
      float scaleX = rotation.m[0][0]*rotation.m[0][0]+rotation.m[0][1]*rotation.m[0][1];
      float scaleY = rotation.m[1][0]*rotation.m[1][0]+rotation.m[1][1]*rotation.m[1][1];
      scale.m[0][0] = (scaleX!=0)?1/(scaleX):1.0;
      scale.m[1][1] = (scaleY!=0)?1/(scaleY):1.0;
      //merge transformations
      YglMatrixMultiply(&fuse, &rotation, &translate);
      YglMatrixMultiply(&vdp1Mat, &scale, &fuse);
    }

    glBindVertexArray(_Ygl->vao);

    vdp2blit_prg = Ygl_uniformVDP2DrawFramebuffer(offsetcol, varVdp2Regs );

    if (vdp1fb != NULL) {
      glActiveTexture(GL_TEXTURE9);
      glBindTexture(GL_TEXTURE_2D, vdp1fb[0]);
    } else _Ygl->vdp1On[_Ygl->readframe] = 0;

  int gltext[16] = {GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6, GL_TEXTURE7, GL_TEXTURE8, GL_TEXTURE9, GL_TEXTURE10, GL_TEXTURE11, GL_TEXTURE12, GL_TEXTURE13, GL_TEXTURE14, GL_TEXTURE15};


#ifdef DEBUG_BLIT
    glBindFragDataLocation(vdp2blit_prg, 1, "topColor");
    glBindFragDataLocation(vdp2blit_prg, 2, "secondColor");
    glBindFragDataLocation(vdp2blit_prg, 3, "thirdColor");
    glBindFragDataLocation(vdp2blit_prg, 4, "fourthColor");
#endif
    glBindFragDataLocation(vdp2blit_prg, 0, "finalColor");

  glUniform1i(glGetUniformLocation(vdp2blit_prg, "s_texture0"), 0);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "s_texture1"), 1);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "s_texture2"), 2);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "s_texture3"), 3);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "s_texture4"), 4);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "s_texture5"), 5);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "s_back"), 7);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "s_lncl"), 8);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "s_win0"), 14);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "s_win1"), 15);
  glUniform2f(glGetUniformLocation(vdp2blit_prg, "vdp1Ratio"), _Ygl->vdp1expandW, _Ygl->vdp1expandH);//((float)_Ygl->rwidth*(float)_Ygl->vdp1wratio * (float)_Ygl->vdp1wdensity)/((float)_Ygl->vdp1width*(float)_Ygl->vdp2wdensity), ((float)_Ygl->rheight*(float)_Ygl->vdp1hratio * (float)_Ygl->vdp1hdensity)/((float)_Ygl->vdp1height * (float)_Ygl->vdp2hdensity));
  glUniform2i(glGetUniformLocation(vdp2blit_prg, "tvSize"), _Ygl->rwidth, _Ygl->rheight);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "mode"), 7, modescreens);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "isRGB"), 6, isRGB);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "isBlur"), 7, isBlur);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "isShadow"), 6, isShadow);
  glUniformMatrix4fv(glGetUniformLocation(vdp2blit_prg, "fbMat"), 1, GL_FALSE, vdp1Mat.m);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "fbon"), (_Ygl->vdp1On[_Ygl->readframe] != 0));
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "ram_mode"), Vdp2Internal.ColorMode);
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "extended_cc"), ((varVdp2Regs->CCCTL & 0x400) != 0) );
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "use_sp_win"), ((varVdp2Regs->SPCTL>>4)&0x1));
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "use_trans_shadow"), ((varVdp2Regs->SDCTL>>8)&0x1));
  glUniform1f(glGetUniformLocation(vdp2blit_prg, "u_emu_height"),(float)_Ygl->rheight / (float)_Ygl->height);
  glUniform1f(glGetUniformLocation(vdp2blit_prg, "u_emu_vdp1_width"),(float)(_Ygl->vdp1width) / (float)(_Ygl->rwidth));
  glUniform1f(glGetUniformLocation(vdp2blit_prg, "u_emu_vdp2_width"),(float)(_Ygl->width) / (float)(_Ygl->rwidth));
  glUniform1f(glGetUniformLocation(vdp2blit_prg, "u_vheight"), (float)_Ygl->height);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "win_s"), enBGMAX+1, Win_s);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "win_s_mode"), enBGMAX+1, Win_s_mode);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "win0"), enBGMAX+1, Win0);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "win0_mode"), enBGMAX+1, Win0_mode);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "win1"), enBGMAX+1, Win1);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "win1_mode"), enBGMAX+1, Win1_mode);
  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "win_op"), enBGMAX+1, Win_op);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, _Ygl->vertexPosition_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPosition), vertexPosition, GL_STREAM_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, _Ygl->textureCoord_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoord), textureCoord, GL_STREAM_DRAW);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(1);

  glActiveTexture(GL_TEXTURE14);
  glBindTexture(GL_TEXTURE_2D, _Ygl->window_tex[0]);
  glActiveTexture(GL_TEXTURE15);
  glBindTexture(GL_TEXTURE_2D, _Ygl->window_tex[1]);

  int id = 0;
  for (int i=0; i<nbScreen; i++) {
    if (prioscreens[i] != 0) {
      glActiveTexture(gltext[i]);
      glBindTexture(GL_TEXTURE_2D, prioscreens[i]);
      id++;
    }
  }
  glUniform1i(glGetUniformLocation(vdp2blit_prg, "screen_nb"), id);

  glActiveTexture(gltext[7]);
  glBindTexture(GL_TEXTURE_2D, _Ygl->back_fbotex[0]);

  glActiveTexture(gltext[8]);
  glBindTexture(GL_TEXTURE_2D, _Ygl->lincolor_tex);

  glUniform1iv(glGetUniformLocation(vdp2blit_prg, "u_lncl"), 7, lncl); //_Ygl->prioVa

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Clean up
  for (int i = 0; i<16; i++) {
    glActiveTexture(gltext[i]);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisable(GL_BLEND);

  return 0;
}

//--------------------------------------------------------------------------------------------------------------
static int winprio_prg = -1;

static const char winprio_v[] =
      SHADER_VERSION
      "layout (location = 0) in vec2 a_position;   \n"
      "layout (location = 1) in vec2 a_texcoord;   \n"
      "out vec2 v_texcoord;     \n"
      "void main()       \n"
      "{ \n"
      " gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0); \n"
      " v_texcoord  = a_texcoord; \n"
      "} ";

static const char winprio_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float; \n"
"#endif\n"
"in vec2 v_texcoord; \n"
"uniform sampler2D s_texture;  \n"
"out vec4 fragColor; \n"
"void main()   \n"
"{  \n"
"  ivec2 addr = ivec2(textureSize(s_texture, 0) * v_texcoord.st); \n"
"  fragColor = texelFetch( s_texture, addr,0 );         \n"
"  if (all(equal(fragColor.rg,vec2(0.0)))) discard;  \n"
//"  fragColor.a = 1.0;  \n"
"}  \n";

int YglBlitSimple(int texture, int blend) {
  const GLchar * fblit_winprio_v[] = { winprio_v, NULL };
  const GLchar * fblit_winprio_f[] = { winprio_f, NULL };

  float const vertexPosition[] = {
    1.0, -1.0f,
    -1.0, -1.0f,
    1.0, 1.0f,
    -1.0, 1.0f };

  float const textureCoord[] = {
    1.0f, 0.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f
  };

  if (winprio_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;
    if (winprio_prg != -1) glDeleteProgram(winprio_prg);
    winprio_prg = glCreateProgram();
    if (winprio_prg == 0){
      return -1;
    }

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, fblit_winprio_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      winprio_prg = -1;
      return -1;
    }

    glShaderSource(fshader, 1, fblit_winprio_f, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      winprio_prg = -1;
      abort();
    }

    glAttachShader(winprio_prg, vshader);
    glAttachShader(winprio_prg, fshader);
    glLinkProgram(winprio_prg);
    glGetProgramiv(winprio_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      YGLLOG("Link error..\n");
      Ygl_printShaderError(winprio_prg);
      winprio_prg = -1;
      abort();
    }

    GLUSEPROG(winprio_prg);
    glUniform1i(glGetUniformLocation(winprio_prg, "s_texture"), 0);
  }
  else{
    GLUSEPROG(winprio_prg);
  }


  if (blend) {
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    glDisable(GL_BLEND);
  }
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, _Ygl->vertexPosition_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPosition), vertexPosition, GL_STREAM_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, _Ygl->textureCoord_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoord), textureCoord, GL_STREAM_DRAW);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(1);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Clean up
  glActiveTexture(GL_TEXTURE0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisable(GL_BLEND);

  return 0;
}



//--------------------------------------------------------------------------------------------------------------
static int vdp1_write_prg = -1;
static int vdp1_read_prg = -1;
static GLint vdp1MtxModelView = 0;

static const char vdp1_v[] =
      SHADER_VERSION
      "layout (location = 0) in vec2 a_position;   \n"
      "layout (location = 1) in vec2 a_texcoord;   \n"
      "out  highp vec2 v_texcoord;     \n"
      "void main()       \n"
      "{ \n"
      " gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0); \n"
      "   v_texcoord  = a_texcoord; \n"
      "} ";

static const char vdp1_write_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float; \n"
"#endif\n"
"in highp vec2 v_texcoord; \n"
"uniform sampler2D s_texture;  \n"
"out vec4 fragColor; \n"
"void main()   \n"
"{  \n"
"  ivec2 addr; \n"
"  addr.x = int( v_texcoord.x);          \n"
"  addr.y = int(v_texcoord.y);          \n"
"  vec4 tex = texelFetch( s_texture, addr,0 );         \n"
"  if (all(equal(tex.rg,vec2(0.0)))) discard;   \n"
"  fragColor.r = tex.a;         \n"
"  fragColor.g = tex.b;         \n"
"  fragColor.b = 0.0;         \n"
"  fragColor.a = 0.0;         \n"
"}  \n";

static const char vdp1_read_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float; \n"
"#endif\n"
"in highp vec2 v_texcoord; \n"
"uniform sampler2D s_texture;  \n"
"out vec4 fragColor; \n"
"void main()   \n"
"{  \n"
"  ivec2 addr; \n"
"  addr.x = int( v_texcoord.x);          \n"
"  addr.y = int(v_texcoord.y);          \n"
"  vec4 tex = texelFetch( s_texture, addr,0 );         \n"
//"  if (tex.agb == vec3(0.0)) tex.ragb = vec4(0.5, 0.0, 0.0, 0.0);   \n"
"  fragColor.r = 0.0;         \n"
"  fragColor.g = 0.0;         \n"
"  fragColor.b = tex.g;         \n"
"  fragColor.a = tex.r;         \n"
"}  \n";

int YglBlitVDP1(u32 srcTexture, float w, float h, int write) {
  const GLchar * fblit_vdp1_v[] = { vdp1_v, NULL };
  const GLchar * fblit_vdp1_write_f[] = { vdp1_write_f, NULL };
  const GLchar * fblit_vdp1_read_f[] = { vdp1_read_f, NULL };
  const int flip = 0;

  float const vertexPosition[] = {
    1.0, -1.0f,
    -1.0, -1.0f,
    1.0, 1.0f,
    -1.0, 1.0f };

  float const textureCoord[] = {
    w, h,
    0.0f, h,
    w, 0.0f,
    0.0f, 0.0f
  };
  float const textureCoordFlip[] = {
    w, 0.0f,
    0.0f, 0.0f,
    w, h,
    0.0f, h
  };

  if (vdp1_write_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;
    if (vdp1_write_prg != -1) glDeleteProgram(vdp1_write_prg);
    vdp1_write_prg = glCreateProgram();
    if (vdp1_write_prg == 0){
      return -1;
    }

    YGLLOG("BLIT_VDP1_WRITE\n");

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, fblit_vdp1_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      vdp1_write_prg = -1;
      return -1;
    }

    glShaderSource(fshader, 1, fblit_vdp1_write_f, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      vdp1_write_prg = -1;
      abort();
    }

    glAttachShader(vdp1_write_prg, vshader);
    glAttachShader(vdp1_write_prg, fshader);
    glLinkProgram(vdp1_write_prg);
    glGetProgramiv(vdp1_write_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      YGLLOG("Link error..\n");
      Ygl_printShaderError(vdp1_write_prg);
      vdp1_write_prg = -1;
      abort();
    }
    glUniform1i(glGetUniformLocation(vdp1_write_prg, "s_texture"), 0);
  }

  if (vdp1_read_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;
    if (vdp1_read_prg != -1) glDeleteProgram(vdp1_read_prg);
    vdp1_read_prg = glCreateProgram();
    if (vdp1_read_prg == 0){
      return -1;
    }

    YGLLOG("BLIT_VDP1_WRITE\n");

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, fblit_vdp1_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      vdp1_read_prg = -1;
      return -1;
    }

    glShaderSource(fshader, 1, fblit_vdp1_read_f, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      vdp1_read_prg = -1;
      abort();
    }

    glAttachShader(vdp1_read_prg, vshader);
    glAttachShader(vdp1_read_prg, fshader);
    glLinkProgram(vdp1_read_prg);
    glGetProgramiv(vdp1_read_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      YGLLOG("Link error..\n");
      Ygl_printShaderError(vdp1_read_prg);
      vdp1_read_prg = -1;
      abort();
    }
    glUniform1i(glGetUniformLocation(vdp1_read_prg, "s_texture"), 0);
  }

  if (write == 0){
    GLUSEPROG(vdp1_read_prg);
  } else {
    GLUSEPROG(vdp1_write_prg);
  }

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, _Ygl->vertexPosition_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPosition), vertexPosition, GL_STREAM_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
  if (flip == 1){
     glBindBuffer(GL_ARRAY_BUFFER, _Ygl->textureCoordFlip_buf);
     glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordFlip), textureCoordFlip, GL_STREAM_DRAW);
     glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
     glEnableVertexAttribArray(1);
  }
  else{
     glBindBuffer(GL_ARRAY_BUFFER, _Ygl->textureCoord_buf);
     glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoord), textureCoord, GL_STREAM_DRAW);
     glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
     glEnableVertexAttribArray(1);
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, srcTexture);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Clean up
  glActiveTexture(GL_TEXTURE0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

  return 0;
}

//----------------------------------------------------------------------------------------
static int blit_prg = -1;
static int blit_mode = -1;
static int scanline = -1;
static int u_w = -1;
static int u_h = -1;
static int u_l = -1;
static int u_d = -1;
static int outputSize = -1;
static int inputSize = -1;

static const char vblit_img[] =
  SHADER_VERSION
  "layout (location = 0) in vec2 aPosition;   \n"
  "layout (location = 1) in vec2 aTexCoord;   \n"
  "out  highp vec2 vTexCoord;     \n"
  "  \n"
  " void main(void) \n"
  " { \n"
  " vTexCoord = aTexCoord;     \n"
  " gl_Position = vec4(aPosition.x, aPosition.y, 0.0, 1.0); \n"
  " } \n";


static const char fblit_head[] =
  SHADER_VERSION
  "#ifdef GL_ES\n"
  "precision highp float;       \n"
  "#endif\n"
  "uniform float fWidth; \n"
  "uniform float fHeight; \n"
  "uniform vec2 lineNumber; \n"
  "uniform float decim; \n"
  "in highp vec2 vTexCoord;     \n"
  "uniform sampler2D u_Src;     \n"
  "out vec4 fragColor; \n";

static const char fblit_img[] =
  "void main()   \n"
  "{   \n"
"	fragColor = Filter( u_Src, vTexCoord ); \n";

static const char fblit_img_end[] =
  "} \n";

/////////////
static const char fblitnear_img[] =
  "vec4 Filter( sampler2D textureSampler, vec2 TexCoord ) \n"
  "{ \n"
  "     return texture( textureSampler, TexCoord ) ; \n"
  "} \n";

static const char fblitbilinear_img[] =
  "// Function to get a texel data from a texture with GL_NEAREST property. \n"
  "// Bi-Linear interpolation is implemented in this function with the  \n"
  "// help of nearest four data. \n"
  "vec4 Filter( sampler2D textureSampler_i, vec2 texCoord_i ) \n"
  "{ \n"
  "	float texelSizeX = 1.0 / fWidth; //size of one texel  \n"
  "	float texelSizeY = 1.0 / fHeight; //size of one texel  \n"
  "	int nX = int( texCoord_i.x * fWidth ); \n"
  "	int nY = int( texCoord_i.y * fHeight ); \n"
  "	vec2 texCoord_New = vec2( ( float( nX ) + 0.5 ) / fWidth, ( float( nY ) + 0.5 ) / fHeight ); \n"
  "	// Take nearest two data in current row. \n"
  "    vec4 p0q0 = texture(textureSampler_i, texCoord_New); \n"
  "    vec4 p1q0 = texture(textureSampler_i, texCoord_New + vec2(texelSizeX, 0)); \n"
  "	// Take nearest two data in bottom row. \n"
  "    vec4 p0q1 = texture(textureSampler_i, texCoord_New + vec2(0, texelSizeY)); \n"
  "    vec4 p1q1 = texture(textureSampler_i, texCoord_New + vec2(texelSizeX , texelSizeY)); \n"
  "    float a = fract( texCoord_i.x * fWidth ); // Get Interpolation factor for X direction. \n"
  "	// Fraction near to valid data. \n"
  "	// Interpolation in X direction. \n"
  "    vec4 pInterp_q0 = mix( p0q0, p1q0, a ); // Interpolates top row in X direction. \n"
  "    vec4 pInterp_q1 = mix( p0q1, p1q1, a ); // Interpolates bottom row in X direction. \n"
  "    float b = fract( texCoord_i.y * fHeight ); // Get Interpolation factor for Y direction. \n"
  "    return mix( pInterp_q0, pInterp_q1, b ); // Interpolate in Y direction. \n"
  "} \n";

/////

GLuint textureCoord_buf[2] = {0,0};

static int last_upmode = 0;

int YglBlitFramebuffer(u32 srcTexture, float w, float h, float dispw, float disph) {
  float width = w;
  float height = h;
  int decim;
  u32 tex = srcTexture;
  const GLchar * fblit_img_v[] = { fblit_head, fblitnear_img, fblit_img, fblit_img_end, NULL };
  const GLchar * fblitbilinear_img_v[] = { fblit_head, fblitnear_img, fblit_img, fblit_img_end, NULL };
  const GLchar * fblitbicubic_img_v[] = { fblit_head, fblitbicubic_img, fblit_img, fblit_img_end, NULL };
  const GLchar * fblit_img_scanline_v[] = { fblit_head, fblitnear_img, fblit_img, Yglprg_blit_scanline_f, fblit_img_end, NULL };
  const GLchar * fblitbilinear_img_scanline_v[] = { fblit_head, fblitnear_img, fblit_img, Yglprg_blit_scanline_f, fblit_img_end, NULL };
  const GLchar * fblitbicubic_img_scanline_v[] = { fblit_head, fblitbicubic_img, fblit_img, Yglprg_blit_scanline_f, fblit_img_end, NULL };
  int aamode = _Ygl->aamode;

  float const vertexPosition[] = {
    1.0f, -1.0f,
    -1.0f, -1.0f,
    1.0f, 1.0f,
    -1.0f, 1.0f };

  float const textureCoord[16] = {
    1.0f, 0.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 1.0f };

  float nbLines = yabsys.IsPal?625.0f:525.0f;

  if (_Ygl->upmode != UP_NONE) {
    int scale = 1;
    if (last_upmode != _Ygl->upmode) {
      if (_Ygl->upfbotex != 0) glDeleteTextures(1, &_Ygl->upfbotex);
      if (_Ygl->upfbo != 0) glDeleteFramebuffers(1, &_Ygl->upfbo);
      _Ygl->upfbo = 0;
      _Ygl->upfbotex = 0;
      last_upmode = _Ygl->upmode;
    }
    scale = Ygl_useUpscaleBuffer();
    glGetIntegerv( GL_VIEWPORT, _Ygl->m_viewport );
    glViewport(0, 0, scale*_Ygl->rwidth, scale*_Ygl->rheight);
    glScissor(0, 0, scale*_Ygl->rwidth, scale*_Ygl->rheight);
    YglUpscaleFramebuffer(srcTexture, _Ygl->upfbo, _Ygl->rwidth, _Ygl->rheight, _Ygl->width, _Ygl->height);
    glViewport(_Ygl->m_viewport[0], _Ygl->m_viewport[1], _Ygl->m_viewport[2], _Ygl->m_viewport[3]);
    glScissor(_Ygl->m_viewport[0], _Ygl->m_viewport[1], _Ygl->m_viewport[2], _Ygl->m_viewport[3]);
    tex = _Ygl->upfbotex;
    width = scale*_Ygl->rwidth;
    height = scale*_Ygl->rheight;
  }

  //if ((aamode == AA_NONE) && ((w != dispw) || (h != disph))) aamode = AA_BILINEAR_FILTER;

  if ((blit_prg == -1) || (blit_mode != aamode) || (scanline != _Ygl->scanline)){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;
    const GLchar * vblit_img_v[] = { vblit_img, NULL };
    if (blit_prg != -1) glDeleteProgram(blit_prg);
    blit_prg = glCreateProgram();
    if (blit_prg == 0){
      return -1;
    }

    blit_mode = aamode;
    scanline = _Ygl->scanline;

    YGLLOG("BLIT_FRAMEBUFFER\n");

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, vblit_img_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      blit_prg = -1;
      return -1;
    }
    if (_Ygl->scanline == 0) {
      switch(aamode) {
        case AA_NONE:
          glShaderSource(fshader, 4, fblit_img_v, NULL);
          break;
        case AA_BILINEAR_FILTER:
          glShaderSource(fshader, 4, fblitbilinear_img_v, NULL);
          break;
        case AA_BICUBIC_FILTER:
          glShaderSource(fshader, 4, fblitbicubic_img_v, NULL);
          break;
      }
    } else {
      switch(aamode) {
        case AA_NONE:
          glShaderSource(fshader, 5, fblit_img_scanline_v, NULL);
          break;
        case AA_BILINEAR_FILTER:
          glShaderSource(fshader, 5, fblitbilinear_img_scanline_v, NULL);
          break;
        case AA_BICUBIC_FILTER:
          glShaderSource(fshader, 5, fblitbicubic_img_scanline_v, NULL);
          break;
      }
    }
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YuiMsg("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      blit_prg = -1;
	  return -1;
    }

    glAttachShader(blit_prg, vshader);
    glAttachShader(blit_prg, fshader);
    glLinkProgram(blit_prg);
    glGetProgramiv(blit_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
		YuiMsg("Link error..\n");
      Ygl_printShaderError(blit_prg);
      blit_prg = -1;
	  return -1;
    }

    GLUSEPROG(blit_prg);
    glUniform1i(glGetUniformLocation(blit_prg, "u_Src"), 0);
    u_w = glGetUniformLocation(blit_prg, "fWidth");
    u_h = glGetUniformLocation(blit_prg, "fHeight");
    u_l = glGetUniformLocation(blit_prg, "lineNumber");
    u_d = glGetUniformLocation(blit_prg, "decim");
  }
  else{
    GLUSEPROG(blit_prg);
  }

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, _Ygl->vertexPosition_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPosition), vertexPosition, GL_STREAM_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
  if (textureCoord_buf[yabsys.isRotated] == 0)
     glGenBuffers(1, &textureCoord_buf[yabsys.isRotated]);
  glBindBuffer(GL_ARRAY_BUFFER, textureCoord_buf[yabsys.isRotated]);
  glBufferData(GL_ARRAY_BUFFER, 8*sizeof(float), &textureCoord[yabsys.isRotated * 8], GL_STREAM_DRAW);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(1);
  glUniform1f(u_w, width);
  glUniform1f(u_h, height);
  glUniform2f(u_l, nbLines, disph);
  decim = (disph + nbLines) / nbLines;
  if (decim < 2) decim = 2;
  glUniform1f(u_d, (float)decim);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  if (aamode == AA_BILINEAR_FILTER) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Clean up
  glActiveTexture(GL_TEXTURE0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

const GLchar vclearb_img[] =
      SHADER_VERSION
      "layout (location = 0) in vec4 a_position;   \n"
      "void main()       \n"
      "{ \n"
      "   gl_Position = a_position; \n"
      "} ";

const GLchar fclearb_img[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float; \n"
"#endif\n"
"out vec4 fragColor; \n"
"void main()   \n"
"{  \n"
"  fragColor = vec4(0.0,0.0,0.0,1.0);\n   "
"}  \n";

int YglClear() {
  float const vertexPosition[] = {
    1.0, -1.0f,
    -1.0, -1.0f,
    1.0, 1.0f,
    -1.0, 1.0f };
  if (clear_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;
    const GLchar * vclearb_img_v[] = { vclearb_img, NULL };
    const GLchar * fclearb_img_v[] = { fclearb_img, NULL };

    clear_prg = glCreateProgram();
    if (clear_prg == 0){
      return -1;
    }

    YGLLOG("CLEAR\n");

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, vclearb_img_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      clear_prg = -1;
      return -1;
    }
    glShaderSource(fshader, 1, fclearb_img_v, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      clear_prg = -1;
      abort();
    }

    glAttachShader(clear_prg, vshader);
    glAttachShader(clear_prg, fshader);
    glLinkProgram(clear_prg);
    glGetProgramiv(clear_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      YGLLOG("Link error..\n");
      Ygl_printShaderError(clear_prg);
      clear_prg = -1;
      abort();
    }

    GLUSEPROG(clear_prg);
  }
  else{
    GLUSEPROG(clear_prg);
  }

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, _Ygl->vertexPosition_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPosition), vertexPosition, GL_STREAM_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Clean up
  glDisableVertexAttribArray(0);
  return 0;
}

/*
hard/vdp2/hon/p12_13.htm
*/

const GLchar mosaic_blit_v[] =
SHADER_VERSION
"uniform mat4 u_mvpMatrix;    \n"
"layout (location = 0) in vec4 a_position;   \n"
"layout (location = 1) in vec2 a_texcoord;   \n"
"out  highp vec2 v_texcoord;     \n"
"void main()       \n"
"{ \n"
"   gl_Position = a_position*u_mvpMatrix; \n"
"   v_texcoord  = a_texcoord; \n"
"} ";

const GLchar mosaic_blit_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float; \n"
"#endif\n"
"in highp vec2 v_texcoord; \n"
"uniform sampler2D u_Src;  \n"
"uniform float u_tw; \n"
"uniform float u_th; \n"
"uniform ivec2 u_mosaic;			 \n"
"out vec4 fragColor; \n"
"void main()   \n"
"{  \n"
"  ivec2 addr; \n"
"  addr.x = int(u_tw * v_texcoord.x);          \n"
"  addr.y = int(u_th) - int(u_th * v_texcoord.y);          \n"
"  addr.x = addr.x / u_mosaic.x * u_mosaic.x;    \n"
"  addr.y = addr.y / u_mosaic.y * u_mosaic.y;    \n"
"  vec4 txcol = texelFetch( u_Src, addr,0 ) ;      \n"
"  if(txcol.a > 0.0)\n      "
"     fragColor = txcol; \n  "
"  else \n      "
"     discard;\n"
"}  \n";

static int mosaic_prg = -1;
static int u_mosaic_mtxModelView = -1;
static int u_mosaic_tw = -1;
static int u_mosaic_th = -1;
static int u_mosaic = -1;

int YglBlitMosaic(u32 srcTexture, float w, float h, GLfloat* matrix, int * mosaic) {

  float vb[] = { 0, 0,
    2.0, 0.0,
    2.0, 2.0,
    0, 2.0, };

  float tb[] = { 0.0, 0.0,
    1.0, 0.0,
    1.0, 1.0,
    0.0, 1.0 };

  vb[0] = 0;
  vb[1] = 0 - 1.0;
  vb[2] = w;
  vb[3] = 0 - 1.0;
  vb[4] = w;
  vb[5] = h - 1.0;
  vb[6] = 0;
  vb[7] = h - 1.0;

  if (mosaic_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;

    const GLchar * vblit_img_v[] = { mosaic_blit_v, NULL };
    const GLchar * fblit_img_v[] = { mosaic_blit_f, NULL };

    mosaic_prg = glCreateProgram();
    if (mosaic_prg == 0) return -1;

    YGLLOG("BLIT_MOSAIC\n");

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, vblit_img_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      mosaic_prg = -1;
      return -1;
    }

    glShaderSource(fshader, 1, fblit_img_v, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      mosaic_prg = -1;
      return -1;
    }

    glAttachShader(mosaic_prg, vshader);
    glAttachShader(mosaic_prg, fshader);
    glLinkProgram(mosaic_prg);
    glGetProgramiv(mosaic_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      YGLLOG("Link error..\n");
      Ygl_printShaderError(mosaic_prg);
      mosaic_prg = -1;
      return -1;
    }

    glUniform1i(glGetUniformLocation(mosaic_prg, "u_Src"), 0);
    u_mosaic_mtxModelView = glGetUniformLocation(mosaic_prg, (const GLchar *)"u_mvpMatrix");
    u_mosaic_tw = glGetUniformLocation(mosaic_prg, "u_tw");
    u_mosaic_th = glGetUniformLocation(mosaic_prg, "u_th");
    u_mosaic = glGetUniformLocation(mosaic_prg, "u_mosaic");

  }
  else{
    GLUSEPROG(mosaic_prg);
  }

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, _Ygl->vb_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vb), vb, GL_STREAM_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, _Ygl->tb_buf);
  glBufferData(GL_ARRAY_BUFFER, sizeof(tb), tb, GL_STREAM_DRAW);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(1);
  glUniformMatrix4fv(u_mosaic_mtxModelView, 1, GL_FALSE, matrix);
  glUniform1f(u_mosaic_tw, w);
  glUniform1f(u_mosaic_th, h);
  glUniform2iv(u_mosaic, 1, mosaic);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, srcTexture);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  // Clean up
  glActiveTexture(GL_TEXTURE0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

  return 0;
}
