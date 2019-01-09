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

// Keep a way to switch to gles shaders for embedded devices
#ifdef HAVE_GLES
#define SHADER_VERSION "#version 310 es \n"
#define SHADER_VERSION_TESS "#version 310 es \n#extension GL_ANDROID_extension_pack_es31a : enable \n"
#else
#define SHADER_VERSION "#version 330 core \n"
#define SHADER_VERSION_TESS "#version 420 core \n"
#endif

#define YGLLOG printf

int Ygl_useTmpBuffer();
int YglBlitBlur(u32 srcTexture, u32 targetFbo, float w, float h, float * matrix);
int YglBlitMosaic(u32 srcTexture, u32 targetFbo, float w, float h, float * matrix, int * mosaic);
int YglBlitPerLineAlpha(u32 srcTexture, u32 targetFbo, float w, float h, float * matrix, u32 perline);

extern float vdp1wratio;
extern float vdp1hratio;
extern int GlHeight;
extern int GlWidth;
static GLuint _prgid[PG_MAX] = { 0 };

#ifdef DEBUG_PROG
#define GLUSEPROG(A) printf("use prog %s (%d) @ %d\n", #A, A, __LINE__);glUseProgram(A)
#else
#define GLUSEPROG(A) glUseProgram(A)
#endif


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

int ShaderDrawTest()
{

  GLuint vertexp = glGetAttribLocation(_prgid[PG_NORMAL], (const GLchar *)"a_position");
  GLuint texcoordp = glGetAttribLocation(_prgid[PG_NORMAL], (const GLchar *)"a_texcoord");
  GLuint mtxModelView = glGetUniformLocation(_prgid[PG_NORMAL], (const GLchar *)"u_mvpMatrix");
  GLuint mtxTexture = glGetUniformLocation(_prgid[PG_NORMAL], (const GLchar *)"u_texMatrix");

  GLfloat vec[] = { 0.0f, 0.0f, -0.5f, 100.0f, 0.0f, -0.5f, 100.0f, 100.0f, -0.5f,
    0.0f, 0.0f, -0.5f, 100.0f, 100.0f, -0.5f, 0.0f, 100.0f, -0.5f };

  /*
  GLfloat vec[]={ 0.0f,0.0f,-0.5f,
  -1.0f,1.0f,-0.5f,
  1.0f,1.0f,-0.5f,
  0.0f,0.0f,-0.5f,
  -1.0f,-1.0f,-0.5f,
  1.0f,-1.0f,-0.5f,
  };
  */
  GLfloat tex[] = { 0.0f, 0.0f, 2048.0f, 0.0f, 2048.0f, 1024.0f, 0.0f, 0.0f, 2048.0f, 1024.0f, 0.0f, 1024.0f };

  //   GLfloat tex[]={ 0.0f,0.0f,1.0f,0.0f,1.0f,1.0f,
  //                   0.0f,0.0f,1.0f,1.0f,0.0f,1.0f };

  YglMatrix mtx;
  YglMatrix mtxt;
  GLuint id = glGetUniformLocation(_prgid[PG_NORMAL], (const GLchar *)"s_texture");

  YglLoadIdentity(&mtx);
  YglLoadIdentity(&mtxt);

  YglOrtho(&mtx, 0.0f, 100.0f, 100.0f, 0.0f, 1.0f, 0.0f);

  GLUSEPROG(_prgid[PG_NORMAL]);
  glUniform1i(id, 0);

  glEnableVertexAttribArray(vertexp);
  glEnableVertexAttribArray(texcoordp);

  glUniformMatrix4fv(mtxModelView, 1, GL_FALSE, (GLfloat*)&_Ygl->mtxModelView.m[0][0]);

  glVertexAttribPointer(vertexp, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)vec);
  glVertexAttribPointer(texcoordp, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)tex);


  glDrawArrays(GL_TRIANGLES, 0, 6);

  return 0;

}


void Ygl_Vdp1CommonGetUniformId(GLuint pgid, YglVdp1CommonParam * param){

  param->texsize = glGetUniformLocation(pgid, (const GLchar *)"u_texsize");
  param->sprite = glGetUniformLocation(pgid, (const GLchar *)"u_sprite");
  param->tessLevelInner = glGetUniformLocation(pgid, (const GLchar *)"TessLevelInner");
  param->tessLevelOuter = glGetUniformLocation(pgid, (const GLchar *)"TessLevelOuter");
  param->fbo = glGetUniformLocation(pgid, (const GLchar *)"u_fbo");
  param->fbowidth = glGetUniformLocation(pgid, (const GLchar *)"u_fbowidth");
  param->fboheight = glGetUniformLocation(pgid, (const GLchar *)"u_fbohegiht");
  param->mtxModelView = glGetUniformLocation(pgid, (const GLchar *)"u_mvpMatrix");
  param->mtxTexture = glGetUniformLocation(pgid, (const GLchar *)"u_texMatrix");
  param->tex0 = glGetUniformLocation(pgid, (const GLchar *)"s_texture");
}

int Ygl_uniformVdp1CommonParam(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs){

  YglProgram * prg;
  YglVdp1CommonParam * param;

  prg = p;
  param = prg->ids;

  glEnableVertexAttribArray(prg->vertexp);
  glEnableVertexAttribArray(prg->texcoordp);
  if (prg->vertexAttribute != NULL)
  {
    glEnableVertexAttribArray(prg->vaid);
  }
  else{
    glDisableVertexAttribArray(prg->vaid);
  }

  if (param == NULL) return 0;

  glUniform2f(param->texsize, YglTM_vdp1[_Ygl->drawframe]->width, YglTM_vdp1[_Ygl->drawframe]->height);

  if (param->sprite != -1){
    glUniform1i(param->sprite, 0);
  }

  if (param->tessLevelInner != -1) {
    glUniform1f(param->tessLevelInner, (float)TESS_COUNT);
  }

  if (param->tessLevelOuter != -1) {
    glUniform1f(param->tessLevelOuter, (float)TESS_COUNT);
  }

  if (param->fbo != -1){
    glUniform1i(param->fbo, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _Ygl->vdp1FrameBuff[_Ygl->drawframe]);
    glUniform1i(param->fbowidth, _Ygl->width);
    glUniform1i(param->fboheight, _Ygl->height);
#if !defined(_OGLES3_)
    if (glTextureBarrierNV) glTextureBarrierNV();
#else
    if( glMemoryBarrier ){
      glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT|GL_TEXTURE_UPDATE_BARRIER_BIT|GL_TEXTURE_FETCH_BARRIER_BIT);
    }else{
      //glFinish();
    }
#endif    
    glActiveTexture(GL_TEXTURE0); 
  }


  return 0;
}

int Ygl_cleanupVdp1CommonParam(void * p, YglTextureManager *tm){
  YglProgram * prg;
  prg = p;
  glDisableVertexAttribArray(prg->vaid);
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
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position*u_mvpMatrix; \n"
      "   v_texcoord  = a_texcoord; \n"
      "} ";
const GLchar * pYglprg_normal_v[] = {Yglprg_normal_v, NULL};

const GLchar Yglprg_normal_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                            \n"
"#endif\n"
"in highp vec4 v_texcoord;                            \n"
"uniform vec4 u_color_offset;    \n"
"uniform sampler2D s_texture;                        \n"
"out vec4 fragColor;            \n"
"void main()                                         \n"
"{                                                   \n"
"  ivec2 addr; \n"
"  addr.x = int(v_texcoord.x);                        \n"
"  addr.y = int(v_texcoord.y);                        \n"
"  vec4 txcol = texelFetch( s_texture, addr,0 );         \n"
"  int additional = int(txcol.a * 255.0);\n"
"  if((additional & 0x80)!=0)\n                                 "
"     fragColor = clamp(txcol+u_color_offset,vec4(0.0),vec4(1.0));\n                         "
"  else \n                                            "
"     discard;\n                                      "
"}                                                   \n";
const GLchar * pYglprg_normal_f[] = {Yglprg_normal_f, NULL};
static int id_normal_s_texture = -1;
static int id_normal_color_offset = -1;
static int id_normal_matrix = -1;


int Ygl_uniformNormal(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs)
{

  YglProgram * prg;
  prg = p;
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glUniform1i(id_normal_s_texture, 0);
  glUniform4fv(prg->color_offset, 1, prg->color_offset_val);
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
"uniform highp sampler2D s_texture;\n"
"uniform sampler2D s_color;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  vec4 txindex = texelFetch( s_texture, ivec2(int(v_texcoord.x),int(v_texcoord.y)) ,0 );\n"
"  if(txindex.a == 0.0) { discard; }\n"
"  vec4 txcol = texelFetch( s_color,  ivec2( ( int(txindex.g*255.0)<<8 | int(txindex.r*255.0)) ,0 )  , 0 );\n"
"  fragColor = clamp(txcol+u_color_offset,vec4(0.0),vec4(1.0));\n"
"  fragColor.a = txindex.a;\n"
"}\n";

const GLchar Yglprg_normal_cram_special_priority_f[] =
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
"  vec4 txcol = texelFetch( s_color,  ivec2( ( int(txindex.g*65280.0) | int(txindex.r*255.0)) ,0 )  , 0 );\n"
"  fragColor = clamp(txcol+u_color_offset,vec4(0.0),vec4(1.0));\n"
"  fragColor.a = txindex.a;\n"
"  gl_FragDepth = ((txindex.b*255.0/10.0) +1.0)/2.0 ; \n"
"}\n";


const GLchar * pYglprg_normal_cram_f[] = { Yglprg_normal_cram_f, NULL };
static int id_normal_cram_s_texture = -1;
static int id_normal_cram_s_color = -1;
static int id_normal_cram_color_offset = -1;
static int id_normal_cram_matrix = -1;

const GLchar * pYglprg_normal_cram_special_priority_f[] = { Yglprg_normal_cram_special_priority_f, NULL };
static int id_normal_cram_sp_s_texture = -1;
static int id_normal_cram_sp_s_color = -1;
static int id_normal_cram_sp_color_offset = -1;
static int id_normal_cram_sp_matrix = -1;


int Ygl_uniformNormalCram(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs)
{

  YglProgram * prg;
  prg = p;
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glUniform1i(id_normal_cram_s_texture, 0);
  glUniform1i(id_normal_cram_s_color, 1);
  glUniform4fv(prg->color_offset, 1, prg->color_offset_val);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);
  return 0;
}

int Ygl_cleanupNormalCram(void * p, YglTextureManager *tm)
{
  YglProgram * prg;
  glActiveTexture(GL_TEXTURE0);
  prg = p;
  return 0;
}

int Ygl_uniformNormalCramSpecialPriority(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs)
{

  YglProgram * prg;
  prg = p;
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glUniform1i(id_normal_cram_sp_s_texture, 0);
  glUniform1i(id_normal_cram_sp_s_color, 1);
  glUniform4fv(prg->color_offset, 1, prg->color_offset_val);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);
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
"  if( txindex.a > 0.5) { fragColor.a = 1.0;} else {fragColor.a = 0.0;}\n"
"}\n";

const GLchar * pYglprg_normal_cram_addcol_f[] = { Yglprg_normal_cram_addcol_f, NULL };
static int id_normal_cram_s_texture_addcol = -1;
static int id_normal_cram_s_color_addcol = -1;
static int id_normal_cram_color_offset_addcol = -1;
static int id_normal_cram_matrix_addcol = -1;


int Ygl_uniformAddColCram(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs)
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
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_SRC_ALPHA);
  return 0;
}

int Ygl_cleanupAddColCram(void * p, YglTextureManager *tm)
{
  YglProgram * prg;
  glActiveTexture(GL_TEXTURE0);
  prg = p;
  return 0;
}

void Ygl_setNormalshader(YglProgram * prg) {
  if (prg->colornumber >= 3) {
    GLUSEPROG(_prgid[PG_NORMAL]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glUniform1i(id_normal_s_texture, 0);
    glUniform4fv(id_normal_color_offset, 1, prg->color_offset_val);
    glUniformMatrix4fv(id_normal_matrix, 1, GL_FALSE, prg->matrix);
  }
  else {
    GLUSEPROG(_prgid[PG_VDP2_NORMAL_CRAM]);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glUniform1i(id_normal_cram_s_texture, 0);
    glUniform1i(id_normal_cram_s_color, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);
    glActiveTexture(GL_TEXTURE0);
    glUniform4fv(id_normal_color_offset, 1, prg->color_offset_val);
    glUniformMatrix4fv(id_normal_cram_matrix, 1, GL_FALSE, prg->matrix);
  }
}


const GLchar Yglprg_rgb_cram_line_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;\n"
"precision highp int;\n"
"#endif\n"
"in vec4 v_texcoord;\n"
"uniform vec4 u_color_offset;\n"
"uniform highp sampler2D s_texture;\n"
"uniform sampler2D s_color;\n"
"uniform int u_blendmode;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  vec4 txindex = texelFetch( s_texture, ivec2(int(v_texcoord.x),int(v_texcoord.y)) ,0 );         \n"
"  if(txindex.a > 0.0) {\n"
"    highp int highg = int(txindex.g*255.0);"
"    vec4 txcol = texelFetch( s_color, ivec2( ((highg&0x7F)<<8) | int(txindex.r*255.0) , 0 ) , 0 );\n"
"    txcol.a = txindex.a;\n"
"    if( (highg & 0x80)  != 0) {\n"
"      int coef = int(txindex.b*255.0);\n"
"      vec4 linecol;\n"
"      vec4 lineindex = texelFetch( s_texture,  ivec2( int(v_texcoord.z),int(v_texcoord.w))  ,0 );\n"
"      int lineparam = ((int(lineindex.g*255.0) & 0x7F)<<8) | int(lineindex.r*255.0); \n"
"      if( (coef & 0x80) != 0 ){\n"
"        int caddr = (lineparam&0x780) | (coef&0x7F);\n "
"        linecol = texelFetch( s_color, ivec2( caddr,0  ) , 0 );\n"
"      }else{\n"
"        linecol = texelFetch( s_color, ivec2( lineparam , 0 ) , 0 );\n"
"      }\n"
"      if( u_blendmode == 1 ) { \n"
"        txcol = mix(txcol,  linecol , 1.0-txindex.a); txcol.a = txindex.a + 0.25;\n"
"      }else if( u_blendmode == 2 ) {\n"
"        txcol = clamp(txcol+linecol,vec4(0.0),vec4(1.0)); txcol.a = txindex.a; \n"
"      }\n"
"    }\n"
"    fragColor = txcol+u_color_offset;\n"
"  }else{ \n"
"    discard;\n"
"  }\n"
"}\n";

const GLchar * pYglprg_rbg_cram_line_f[] = { Yglprg_rgb_cram_line_f, NULL };
static int id_rbg_cram_line_s_texture = -1;
static int id_rbg_cram_line_s_color = -1;
static int id_rbg_cram_line_color_offset = -1;
static int id_rbg_cram_line_blendmode = -1;
static int id_rbg_cram_line_matrix = -1;

int Ygl_uniformNormalCramLine(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs)
{

  YglProgram * prg;
  prg = p;
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glUniform1i(id_rbg_cram_line_s_texture, 0);
  glUniform1i(id_rbg_cram_line_s_color, 1);
  glUniform4fv(id_rbg_cram_line_color_offset, 1, prg->color_offset_val);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);

  // Disable blend mode if extend Color calcuration  is not enabled
  if ( (varVdp2Regs->CCCTL & 0x400) == 0 ) {
    prg->blendmode = 0;
  }
  glUniform1i(id_rbg_cram_line_blendmode, prg->blendmode);

  return 0;
}

int Ygl_cleanupNormalCramLine(void * p, YglTextureManager *tm)
{
  glActiveTexture(GL_TEXTURE0);
  return 0;
}

int Ygl_useTmpBuffer(){
  // Create Screen size frame buffer
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
  }
  return 0;
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
int Ygl_uniformMosaic(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs)
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
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->targetfbo);

  // Restore Default Matrix
  glViewport(_Ygl->m_viewport[0], _Ygl->m_viewport[1], _Ygl->m_viewport[2], _Ygl->m_viewport[3]);
  glScissor(_Ygl->m_viewport[0], _Ygl->m_viewport[1], _Ygl->m_viewport[2], _Ygl->m_viewport[3]);

  // call blit method
  YglBlitMosaic(_Ygl->tmpfbotex, _Ygl->targetfbo, _Ygl->rwidth, _Ygl->rheight, prg->matrix, prg->mosaic);

  glBindTexture(GL_TEXTURE_2D, tm->textureID);

  return 0;
}

/*------------------------------------------------------------------------------------
*  Per Line Alpha
* ----------------------------------------------------------------------------------*/
int Ygl_uniformPerLineAlpha(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs)
{
  YglProgram * prg;
  int preblend = 0;
  prg = p;

  Ygl_useTmpBuffer();
  glViewport(0, 0, _Ygl->rwidth, _Ygl->rheight);
  glScissor(0, 0, _Ygl->rwidth, _Ygl->rheight);
  prg->preblendmode = prg->blendmode;
  prg->blendmode = 0;

  if (prg->prgid == PG_VDP2_PER_LINE_ALPHA_CRAM) {
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
    glBindTexture(GL_TEXTURE_2D, tm->textureID);
  }

  return 0;
}

int Ygl_cleanupPerLineAlpha(void * p, YglTextureManager *tm)
{
  YglProgram * prg;
  prg = p;
  prg->blendmode = prg->preblendmode;

  // Bind Default frame buffer
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->targetfbo);

  // Restore Default Matrix
  glViewport(_Ygl->m_viewport[0], _Ygl->m_viewport[1], _Ygl->m_viewport[2], _Ygl->m_viewport[3]);
  glScissor(_Ygl->m_viewport[0], _Ygl->m_viewport[1], _Ygl->m_viewport[2], _Ygl->m_viewport[3]);

  if ( (prg->blendmode & 0x03) == VDP2_CC_RATE ) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }else if ((prg->blendmode&0x03) == VDP2_CC_ADD) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
  }

  // call blit method
  YglBlitPerLineAlpha(_Ygl->tmpfbotex, _Ygl->targetfbo, _Ygl->rwidth, _Ygl->rheight, prg->matrix, prg->lineTexture);

  glBindTexture(GL_TEXTURE_2D, tm->textureID);

  return 0;
}


/*------------------------------------------------------------------------------------
*  Blur
* ----------------------------------------------------------------------------------*/
int Ygl_uniformNormal_blur(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs)
{
  YglProgram * prg;
  prg = p;

  Ygl_useTmpBuffer();
  glViewport(0, 0, _Ygl->rwidth, _Ygl->rheight);
  glScissor(0, 0, _Ygl->rwidth, _Ygl->rheight);

  if (prg->prgid == PG_VDP2_BLUR_CRAM) {
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
    glBindTexture(GL_TEXTURE_2D, tm->textureID);
  }

  //glEnableVertexAttribArray(prg->vertexp);
  //glEnableVertexAttribArray(prg->texcoordp);
  //glUniform1i(id_normal_s_texture, 0);
  //glUniform4fv(prg->color_offset, 1, prg->color_offset_val);
  //glBindTexture(GL_TEXTURE_2D, tm->textureID);
  return 0;
}

int Ygl_cleanupNormal_blur(void * p, YglTextureManager *tm)
{
  YglProgram * prg;
  prg = p;

  // Bind Default frame buffer
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->targetfbo);

  // Restore Default Matrix
  glViewport(_Ygl->m_viewport[0], _Ygl->m_viewport[1], _Ygl->m_viewport[2], _Ygl->m_viewport[3]);
  glScissor(_Ygl->m_viewport[0], _Ygl->m_viewport[1], _Ygl->m_viewport[2], _Ygl->m_viewport[3]);

  if ((prg->blendmode & 0x03) == VDP2_CC_RATE) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else if ((prg->blendmode & 0x03) == VDP2_CC_ADD) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
  }

  YglBlitBlur(_Ygl->tmpfbotex, _Ygl->targetfbo, _Ygl->rwidth, _Ygl->rheight, prg->matrix);

  glBindTexture(GL_TEXTURE_2D, tm->textureID);

  return 0;
}



const GLchar Yglprg_DestinationAlpha_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                                  \n"
"#endif\n"
"in highp vec4 v_texcoord;                               \n"
"uniform vec4 u_color_offset;                            \n"
"uniform sampler2D s_texture;                            \n"
"uniform sampler2D s_depth;                              \n"
"uniform sampler2D s_dest;                               \n"
"out vec4 fragColor;                                     \n"
"void main()                                             \n"
"{                                                       \n"
"  ivec2 addr;                                           \n"
"  addr.x = int(v_texcoord.x);                           \n"
"  addr.y = int(v_texcoord.y);                           \n"
"  vec4 txcol = texelFetch( s_texture, addr,0 );         \n"
"  ivec2 screenpos = gl_FragCoord.xy;                    \n"
"  float depth = texelFetch( s_depth, screenpos,0 ).x;   \n"
"  if(txcol.a > 0.0 ){                                   \n"
"     fragColor = clamp(txcol+u_color_offset,vec4(0.0),vec4(1.0)); \n"
"     if( depth == gl_FragCoord.z )                                \n"
"         fragColor.a = texelFetch( s_dest, screenpos,0 ).x        \n"
"  }else \n                                            "
"     discard;\n                                      "
"}                                                   \n";

/*------------------------------------------------------------------------------------
 *  Window Operation
 * ----------------------------------------------------------------------------------*/
const GLchar Yglprg_window_v[] =
      SHADER_VERSION
      "uniform mat4 u_mvpMatrix;    \n"
      "layout (location = 0) in vec4 a_position;               \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position*u_mvpMatrix; \n"
      "} ";
const GLchar * pYglprg_window_v[] = {Yglprg_window_v, NULL};

const GLchar Yglprg_window_f[] =
      SHADER_VERSION
      "#ifdef GL_ES\n"
      "precision highp float;                            \n"
      "#endif\n"
      "out vec4 fragColor;            \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  fragColor = vec4( 1.0,1.0,1.0,1.0 );\n"
      "}                                                   \n";
const GLchar * pYglprg_window_f[] = {Yglprg_window_f, NULL};

int Ygl_uniformWindow(void * p )
{
   YglProgram * prg;
   prg = p;
   GLUSEPROG(prg->prgid );
   glEnableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   return 0;
}

int Ygl_cleanupWindow(void * p, YglTextureManager *tm )
{
   YglProgram * prg;
   prg = p;
   return 0;
}

/*------------------------------------------------------------------------------------
 *  VDP1 Normal Draw
 * ----------------------------------------------------------------------------------*/
const GLchar Yglprg_vdp1_normal_v[] =
      SHADER_VERSION
      "uniform mat4 u_mvpMatrix;    \n"
      "uniform vec2 u_texsize;    \n"
      "layout (location = 0) in vec4 a_position;   \n"
      "layout (location = 1) in vec4 a_texcoord;   \n"
      "out   vec4 v_texcoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position*u_mvpMatrix; \n"
      "   v_texcoord  = a_texcoord; \n"
      "   v_texcoord.x  = v_texcoord.x / u_texsize.x;\n"
      "   v_texcoord.y  = v_texcoord.y / u_texsize.y;\n"
      "} ";
const GLchar * pYglprg_vdp1_normal_v[] = {Yglprg_vdp1_normal_v, NULL};

const GLchar Yglprg_vpd1_normal_f[] =
      SHADER_VERSION
      "#ifdef GL_ES\n"
      "precision highp float;                            \n"
      "#endif\n"
      "in vec4 v_texcoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "out vec4 fragColor;            \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  vec2 addr = v_texcoord.st;                        \n"
      "  addr.s = addr.s / (v_texcoord.q);                 \n"
      "  addr.t = addr.t / (v_texcoord.q);                 \n"
      "  vec4 FragColor = texture( s_texture, addr );      \n"
      "  /*if( FragColor.a == 0.0 ) discard;*/                \n"
      "  fragColor = FragColor;\n "
      "}                                                   \n";
const GLchar * pYglprg_vdp1_normal_f[] = {Yglprg_vpd1_normal_f, NULL};
static int id_vdp1_normal_s_texture_size = -1;
static int id_vdp1_normal_s_texture = -1;

int Ygl_uniformVdp1Normal(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs )
{
   YglProgram * prg;
   prg = p;
   glEnableVertexAttribArray(prg->vertexp);
   glEnableVertexAttribArray(prg->texcoordp);
   glUniform1i(id_vdp1_normal_s_texture, 0);
   glUniform2f(id_vdp1_normal_s_texture_size, YglTM_vdp1[_Ygl->drawframe]->width, YglTM_vdp1[_Ygl->drawframe]->height);
   return 0;
}

int Ygl_cleanupVdp1Normal(void * p, YglTextureManager *tm )
{
   YglProgram * prg;
   prg = p;
   return 0;
}

/*------------------------------------------------------------------------------------
*  VDP1 GlowShading Operation with tessellation
* ----------------------------------------------------------------------------------*/
const GLchar Yglprg_vdp1_gouraudshading_tess_v[] =
SHADER_VERSION_TESS
"layout (location = 0) in vec3 a_position; \n"
"layout (location = 1) in vec4 a_texcoord; \n"
"layout (location = 2) in vec4 a_grcolor;  \n"
"uniform vec2 u_texsize;    \n"
"out vec3 v_position;  \n"
"out vec4 v_texcoord; \n"
"out vec4 v_vtxcolor; \n"
"void main() {                \n"
"   v_position  = a_position; \n"
"   v_vtxcolor  = a_grcolor;  \n"
"   v_texcoord  = a_texcoord; \n"
"   v_texcoord.x  = v_texcoord.x / u_texsize.x; \n"
"   v_texcoord.y  = v_texcoord.y / u_texsize.y; \n"
"}\n";
const GLchar * pYglprg_vdp1_gouraudshading_tess_v[] = { Yglprg_vdp1_gouraudshading_tess_v, NULL };

const GLchar Yglprg_tess_c[] =
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
const GLchar * pYglprg_vdp1_gouraudshading_tess_c[] = { Yglprg_tess_c, NULL };

const GLchar Yglprg_tess_e[] =
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
const GLchar * pYglprg_vdp1_gouraudshading_tess_e[] = { Yglprg_tess_e, NULL };

const GLchar Yglprg_tess_g[] =
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
const GLchar * pYglprg_vdp1_gouraudshading_tess_g[] = { Yglprg_tess_g, NULL };

static YglVdp1CommonParam id_gt = { 0 };


/*------------------------------------------------------------------------------------
 *  VDP1 GlowShading Operation
 * ----------------------------------------------------------------------------------*/
const GLchar Yglprg_vdp1_gouraudshading_v[] =
SHADER_VERSION
"uniform mat4 u_mvpMatrix;                \n"
"uniform vec2 u_texsize;    \n"
"layout (location = 0) in vec4 a_position;               \n"
"layout (location = 1) in vec4 a_texcoord;               \n"
"layout (location = 2) in vec4 a_grcolor;                \n"
"out  vec4 v_texcoord;               \n"
"out  vec4 v_vtxcolor;               \n"
"void main() {                            \n"
"   v_vtxcolor  = a_grcolor;              \n"
"   v_texcoord  = a_texcoord; \n"
"   v_texcoord.x  = v_texcoord.x / u_texsize.x; \n"
"   v_texcoord.y  = v_texcoord.y / u_texsize.y; \n"
"   gl_Position = a_position*u_mvpMatrix; \n"
"}\n";
const GLchar * pYglprg_vdp1_gouraudshading_v[] = {Yglprg_vdp1_gouraudshading_v, NULL};

const GLchar Yglprg_vdp1_gouraudshading_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                                                 \n"
"#endif\n"
"uniform sampler2D u_sprite;                                              \n"
"in vec4 v_texcoord;                                                 \n"
"in vec4 v_vtxcolor;                                                 \n"
"out vec4 fragColor;            \n"
"void main() {                                                            \n"
"  ivec2 addr = ivec2(textureSize(u_sprite, 0) * v_texcoord.st / v_texcoord.q); \n"
"  vec4 spriteColor = texelFetch(u_sprite,addr,0);                           \n"
"  if( spriteColor.a == 0.0 ) discard;                                      \n"
"  fragColor  = clamp(spriteColor+v_vtxcolor,vec4(0.0),vec4(1.0));     \n"
"  fragColor.a = spriteColor.a;                                        \n"
"}\n";
const GLchar * pYglprg_vdp1_gouraudshading_f[] = {Yglprg_vdp1_gouraudshading_f, NULL};

const GLchar Yglprg_vdp1_gouraudshading_spd_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                                                 \n"
"#endif\n"
"uniform sampler2D u_sprite;                                              \n"
"in vec4 v_texcoord;                                                 \n"
"in vec4 v_vtxcolor;                                                 \n"
"out vec4 fragColor;            \n"
"void main() {                                                            \n"
"  ivec2 addr = ivec2(textureSize(u_sprite, 0) * v_texcoord.st / v_texcoord.q); \n"
"  vec4 spriteColor = texelFetch(u_sprite,addr,0);                           \n"
"  fragColor  = clamp(spriteColor+v_vtxcolor,vec4(0.0),vec4(1.0));     \n"
"  fragColor.a = spriteColor.a;                                        \n"
"}\n";
const GLchar * pYglprg_vdp1_gouraudshading_spd_f[] = { Yglprg_vdp1_gouraudshading_spd_f, NULL };

static YglVdp1CommonParam id_g = { 0 };
static YglVdp1CommonParam id_spd_g = { 0 };


/*------------------------------------------------------------------------------------
 *  VDP1 GlowShading and Half Trans Operation
 * ----------------------------------------------------------------------------------*/
const GLchar Yglprg_vdp1_gouraudshading_hf_v[] =
      SHADER_VERSION
      "uniform mat4 u_mvpMatrix;                \n"
      "uniform vec2 u_texsize;    \n"
      "layout (location = 0) in vec4 a_position;               \n"
      "layout (location = 1) in vec4 a_texcoord;               \n"
      "layout (location = 2) in vec4 a_grcolor;                \n"
      "out  vec4 v_texcoord;               \n"
      "out  vec4 v_vtxcolor;               \n"
      "void main() {                            \n"
      "   v_vtxcolor  = a_grcolor;              \n"
      "   v_texcoord  = a_texcoord; \n"
      "   v_texcoord.x  = v_texcoord.x / u_texsize.x; \n"
      "   v_texcoord.y  = v_texcoord.y / u_texsize.y; \n"
      "   gl_Position = a_position*u_mvpMatrix; \n"
      "}\n";
const GLchar * pYglprg_vdp1_gouraudshading_hf_v[] = {Yglprg_vdp1_gouraudshading_hf_v, NULL};

const GLchar Yglprg_vdp1_gouraudshading_hf_f[] =
      SHADER_VERSION
      "#ifdef GL_ES\n"
      "precision highp float;                                                                     \n"
      "#endif\n"
      "uniform highp sampler2D u_sprite;                                                                  \n"
      "uniform highp sampler2D u_fbo;                                                                     \n"
      "uniform int u_fbowidth;                                                                      \n"
      "uniform int u_fbohegiht;                                                                     \n"
      "in vec4 v_texcoord;                                                                     \n"
      "in vec4 v_vtxcolor;                                                                     \n"
      "out vec4 fragColor; \n "
      "void main() {                                                                                \n"
      "  ivec2 addr = ivec2(textureSize(u_sprite, 0) * v_texcoord.st / v_texcoord.q); \n"
      "  vec4 spriteColor = texelFetch(u_sprite,addr,0);                           \n"
      "  if( spriteColor.a == 0.0 ) discard;                                                          \n"
      "  vec4 fboColor    = texelFetch(u_fbo,ivec2(gl_FragCoord.xy),0);                                                 \n"
      "  int additional = int(fboColor.a * 255.0);\n"
      "  spriteColor += vec4(v_vtxcolor.r,v_vtxcolor.g,v_vtxcolor.b,0.0);\n"
      "  if( (additional & 0x40) == 0 ) \n"
      "  { \n"
      "    fragColor = spriteColor*0.5 + fboColor*0.5;                                           \n"
      "    fragColor.a = spriteColor.a;                                                             \n"
      "  }else{                                                                                     \n"
      "    fragColor = spriteColor;                                                              \n"
      "  }                                                                                          \n"
      "}\n";
const GLchar * pYglprg_vdp1_gouraudshading_hf_f[] = {Yglprg_vdp1_gouraudshading_hf_f, NULL};


static YglVdp1CommonParam id_ght = { 0 };
static YglVdp1CommonParam id_ght_tess = { 0 };


/*------------------------------------------------------------------------------------
 *  VDP1 Half Trans Operation
 * ----------------------------------------------------------------------------------*/
const GLchar Yglprg_vdp1_halftrans_v[] =
        SHADER_VERSION
        "uniform mat4 u_mvpMatrix;                \n"
        "uniform vec2 u_texsize;    \n"
        "layout (location = 0) in vec4 a_position;               \n"
        "layout (location = 1) in vec4 a_texcoord;               \n"
        "layout (location = 2) in vec4 a_grcolor;                \n"
        "out  vec4 v_texcoord;               \n"
        "out  vec4 v_vtxcolor;               \n"
        "void main() {                            \n"
        "   v_vtxcolor  = a_grcolor;              \n"
        "   v_texcoord  = a_texcoord; \n"
        "   v_texcoord.x  = v_texcoord.x / u_texsize.x; \n"
        "   v_texcoord.y  = v_texcoord.y / u_texsize.y; \n"
        "   gl_Position = a_position*u_mvpMatrix; \n"
        "}\n";

const GLchar * pYglprg_vdp1_halftrans_v[] = {Yglprg_vdp1_halftrans_v, NULL};

const GLchar Yglprg_vdp1_halftrans_f[] =
      SHADER_VERSION
      "#ifdef GL_ES\n"
      "precision highp float;                                                                     \n"
      "#endif\n"
      "uniform highp sampler2D u_sprite;                                                           \n"
      "uniform highp sampler2D u_fbo;                                                               \n"
      "uniform int u_fbowidth;                                                                      \n"
      "uniform int u_fbohegiht;                                                                     \n"
      "in vec4 v_texcoord;                                                                     \n"
      "out vec4 fragColor; \n "
      "void main() {                                                                                \n"
      "  ivec2 addr = ivec2(textureSize(u_sprite, 0) * v_texcoord.st / v_texcoord.q); \n"
      "  vec4 spriteColor = texelFetch(u_sprite,addr,0);                           \n"
      "  if( spriteColor.a == 0.0 ) discard;                                                          \n"
      "  vec4 fboColor    = texelFetch(u_fbo,ivec2(gl_FragCoord.xy),0);                                                 \n"
      "  int additional = int(fboColor.a * 255.0);\n"
      "  if( (additional & 0x40) == 0 ) \n"
      "  {                                                                                          \n"
      "    fragColor = spriteColor*0.5 + fboColor*0.5;                                           \n"
      "    fragColor.a = fboColor.a;                                                             \n"
      "  }else{                                                                                     \n"
      "    fragColor = spriteColor;                                                              \n"
      "  }                                                                                          \n"
      "}\n";
const GLchar * pYglprg_vdp1_halftrans_f[] = {Yglprg_vdp1_halftrans_f, NULL};

static YglVdp1CommonParam hf = {0};
static YglVdp1CommonParam hf_tess = {0};

/*------------------------------------------------------------------------------------
*  VDP1 Mesh Operaion
* ----------------------------------------------------------------------------------*/
const GLchar Yglprg_vdp1_mesh_v[] =
SHADER_VERSION
"uniform mat4 u_mvpMatrix;                \n"
"uniform vec2 u_texsize;    \n"
"layout (location = 0) in vec4 a_position;               \n"
"layout (location = 1) in vec4 a_texcoord;               \n"
"layout (location = 2) in vec4 a_grcolor;                \n"
"out  vec4 v_texcoord;               \n"
"out  vec4 v_vtxcolor;               \n"
"void main() {                            \n"
"   v_vtxcolor  = a_grcolor;              \n"
"   v_texcoord  = a_texcoord; \n"
"   v_texcoord.x  = v_texcoord.x / u_texsize.x; \n"
"   v_texcoord.y  = v_texcoord.y / u_texsize.y; \n"
"   gl_Position = a_position*u_mvpMatrix; \n"
"}\n";
const GLchar * pYglprg_vdp1_mesh_v[] = { Yglprg_vdp1_mesh_v, NULL };

#if 1
const GLchar Yglprg_vdp1_mesh_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                                                                     \n"
"#endif\n"
"uniform sampler2D u_sprite;                                                                  \n"
"uniform sampler2D u_fbo;                                                                     \n"
"uniform int u_fbowidth;                                                                      \n"
"uniform int u_fbohegiht;                                                                     \n"
"in vec4 v_texcoord;                                                                     \n"
"in vec4 v_vtxcolor;                                                                     \n"
"out highp vec4 fragColor; \n "
"void main() {                                                                                \n"
"  ivec2 addr = ivec2(textureSize(u_sprite, 0) * v_texcoord.st / v_texcoord.q); \n"
"  vec4 spriteColor = texelFetch(u_sprite,addr,0);                           \n"
"  if( spriteColor.a == 0.0 ) discard;                                                          \n"
"  if( (int(gl_FragCoord.y) & 0x01) == 0 ){ \n"
"    if( (int(gl_FragCoord.x) & 0x01) == 0 ){ \n"
"       discard;"
"    } \n"
"  }else{ \n"
"    if( (int(gl_FragCoord.x) & 0x01) == 1 ){ \n"
"       discard;"
"    } \n"
"  } \n"
"  fragColor  = clamp(spriteColor+v_vtxcolor,vec4(0.0),vec4(1.0));     \n"
"  fragColor.a = spriteColor.a;                                        \n"
"}\n";
#else
const GLchar Yglprg_vdp1_mesh_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                                                                     \n"
"#endif\n"
"uniform sampler2D u_sprite;                                                                  \n"
"uniform sampler2D u_fbo;                                                                     \n"
"uniform int u_fbowidth;                                                                      \n"
"uniform int u_fbohegiht;                                                                     \n"
"in vec4 v_texcoord;                                                                     \n"
"in vec4 v_vtxcolor;                                                                     \n"
"out highp vec4 fragColor; \n "
"void main() {                                                                                \n"
"  ivec2 addr = ivec2(textureSize(u_sprite, 0) * v_texcoord.st / v_texcoord.q); \n"
"  vec4 spriteColor = texelFetch(u_sprite,addr,0);                           \n"
"  if( spriteColor.a == 0.0 ) discard;                                                          \n"
 "  vec4 fboColor    = texelFetch(u_fbo,ivec2(gl_FragCoord.xy),0);                                                 \n"
"  spriteColor += vec4(v_vtxcolor.r,v_vtxcolor.g,v_vtxcolor.b,0.0);                           \n"
"  if( fboColor.a > 0.0  )                                                               \n"
"  {                                                                                          \n"
"    fragColor = spriteColor*0.5 + fboColor*0.5;                                           \n"
"    fragColor.a = fboColor.a ;                         \n"
"  }else{                                               \n"
"    fragColor = spriteColor;                           \n"
"    int additional = int(spriteColor.a * 255.0);       \n"
"    highp float alpha = float((additional/8)*8)/255.0; \n"
"    fragColor.a = spriteColor.a-alpha + 0.5;           \n"
"  }                                                                                          \n"
"}\n";
#endif
const GLchar * pYglprg_vdp1_mesh_f[] = { Yglprg_vdp1_mesh_f, NULL };

static YglVdp1CommonParam mesh = { 0 };
static YglVdp1CommonParam grow_tess = { 0 };
static YglVdp1CommonParam grow_spd_tess = { 0 };
static YglVdp1CommonParam mesh_tess = { 0 };

/*------------------------------------------------------------------------------------
*  VDP1 Half luminance Operaion
* ----------------------------------------------------------------------------------*/
const GLchar Yglprg_vdp1_half_luminance_v[] =
      SHADER_VERSION
      "uniform mat4 u_mvpMatrix;    \n"
      "uniform vec2 u_texsize;    \n"
      "layout (location = 0) in vec4 a_position;   \n"
      "layout (location = 1) in vec4 a_texcoord;   \n"
      "out   vec4 v_texcoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position*u_mvpMatrix; \n"
      "   v_texcoord  = a_texcoord; \n"
      "   v_texcoord.x  = v_texcoord.x / u_texsize.x;\n"
      "   v_texcoord.y  = v_texcoord.y / u_texsize.y;\n"
      "} ";
const GLchar * pYglprg_vdp1_half_luminance_v[] = {Yglprg_vdp1_half_luminance_v, NULL};

const GLchar Yglprg_vpd1_half_luminance_f[] =
      SHADER_VERSION
      "#ifdef GL_ES\n"
      "precision highp float;                            \n"
      "#endif\n"
      "in vec4 v_texcoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "out vec4 fragColor;            \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  ivec2 addr = ivec2(textureSize(s_texture, 0) * v_texcoord.st / v_texcoord.q); \n"
      "  vec4 spriteColor = texelFetch(s_texture,addr,0);                           \n"
      "  if( spriteColor.a == 0.0 ) discard;                                                          \n"
      "  fragColor.r = spriteColor.r * 0.5;\n "
      "  fragColor.g = spriteColor.g * 0.5;\n "
      "  fragColor.b = spriteColor.b * 0.5;\n "
      "  fragColor.a = spriteColor.a;\n "
      "}                                                   \n";
const GLchar * pYglprg_vdp1_half_luminance_f[] = {Yglprg_vpd1_half_luminance_f, NULL};
static YglVdp1CommonParam half_luminance = { 0 };


/*------------------------------------------------------------------------------------
*  VDP1 Shadow Operation
*    hard/vdp1/hon/p06_37.htm
* ----------------------------------------------------------------------------------*/
const GLchar Yglprg_vdp1_shadow_v[] =
SHADER_VERSION
"uniform mat4 u_mvpMatrix;                \n"
"uniform vec2 u_texsize;    \n"
"layout (location = 0) in vec4 a_position;               \n"
"layout (location = 1) in vec4 a_texcoord;               \n"
"layout (location = 2) in vec4 a_grcolor;                \n"
"out  vec4 v_texcoord;               \n"
"out  vec4 v_vtxcolor;               \n"
"void main() {                            \n"
"   v_vtxcolor  = a_grcolor;              \n"
"   v_texcoord  = a_texcoord; \n"
"   v_texcoord.x  = v_texcoord.x / u_texsize.x; \n"
"   v_texcoord.y  = v_texcoord.y / u_texsize.y; \n"
"   gl_Position = a_position*u_mvpMatrix; \n"
"}\n";

const GLchar * pYglprg_vdp1_shadow_v[] = { Yglprg_vdp1_shadow_v, NULL };

const GLchar Yglprg_vdp1_shadow_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"uniform sampler2D u_sprite;\n"
"uniform highp sampler2D u_fbo;\n"
"uniform int u_fbowidth;\n"
"uniform int u_fbohegiht;\n"
"in vec4 v_texcoord;\n"
"out vec4 fragColor; \n "
"void main() { \n"
"  ivec2 addr = ivec2(textureSize(u_sprite, 0) * v_texcoord.st / v_texcoord.q); \n"
"  vec4 spriteColor = texelFetch(u_sprite,addr,0);                           \n"
"  if( spriteColor.a == 0.0 ) discard;                                                          \n"
"  vec4 fboColor    = texelFetch(u_fbo,ivec2(gl_FragCoord.xy),0);                                                 \n"
"  int additional = int(fboColor.a * 255.0);\n"
"  if( ((additional & 0xC0)==0x80) ) { \n"
"    fragColor = vec4(fboColor.r*0.5,fboColor.g*0.5,fboColor.b*0.5,fboColor.a);\n"
"  }else{\n"
"    discard;"
"  }\n"
"}\n";
const GLchar * pYglprg_vdp1_shadow_f[] = { Yglprg_vdp1_shadow_f, NULL };

static YglVdp1CommonParam shadow = { 0 };
static YglVdp1CommonParam shadow_tess = { 0 };


/*------------------------------------------------------------------------------------
 *  VDP1 UserClip Operation
 * ----------------------------------------------------------------------------------*/
int Ygl_uniformStartUserClip(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs )
{
   YglProgram * prg;
   prg = p;
   int clear = 0;

   glEnableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   if( prg->ux1 != -1 )
   {

      GLint vertices[12];
      glColorMask( GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE );
      glStencilMask(0xffffffff);
      glClearBufferiv(GL_STENCIL, 0, &clear);

      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS,0x1,0x01);
      glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);

      // render
      vertices[0] = (int)((float)prg->ux1 * vdp1wratio);
      vertices[1] = (int)((float)prg->uy1 * vdp1hratio);
      vertices[2] = (int)((float)(prg->ux2+1) * vdp1wratio);
      vertices[3] = (int)((float)prg->uy1 * vdp1hratio);
      vertices[4] = (int)((float)(prg->ux2+1) * vdp1wratio);
      vertices[5] = (int)((float)(prg->uy2+1) * vdp1hratio);

      vertices[6] = (int)((float)prg->ux1 * vdp1wratio);
      vertices[7] = (int)((float)prg->uy1 * vdp1hratio);
      vertices[8] = (int)((float)(prg->ux2+1) * vdp1wratio);
      vertices[9] = (int)((float)(prg->uy2+1) * vdp1hratio);
      vertices[10] = (int)((float)prg->ux1 * vdp1wratio);
      vertices[11] = (int)((float)(prg->uy2+1) * vdp1hratio);

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
      glStencilFunc(GL_EQUAL,0x1,0xFF);
   }else if( prg->uClipMode == 0x03 )
   {
      glStencilFunc(GL_EQUAL,0x0,0xFF);
   }else{
      glStencilFunc(GL_ALWAYS,0,0xFF);
   }

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);

   //glDisable(GL_STENCIL_TEST);

   return 0;
}

int Ygl_cleanupStartUserClip(void * p, YglTextureManager *tm ){return 0;}

int Ygl_uniformEndUserClip(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs )
{

   YglProgram * prg;
   prg = p;
   glDisable(GL_STENCIL_TEST);
   glStencilFunc(GL_ALWAYS,0,0xFF);

   return 0;
}

int Ygl_cleanupEndUserClip(void * p, YglTextureManager *tm ){return 0;}



/*------------------------------------------------------------------------------------
 *  VDP2 Draw Frame buffer Operation
 * ----------------------------------------------------------------------------------*/
static int idvdp1FrameBuffer;
static int idcram;
static int idfrom;
static int idto;

typedef struct  {
  int idvdp1FrameBuffer;
  int idcram;
  int idfrom;
  int idto;
  int idline;
} DrawFrameBufferUniform;

#define MAX_FRAME_BUFFER_UNIFORM (48)

DrawFrameBufferUniform g_draw_framebuffer_uniforms[MAX_FRAME_BUFFER_UNIFORM];

const GLchar Yglprg_vdp1_drawfb_v[] =
      SHADER_VERSION
      "uniform mat4 u_mvpMatrix;                \n"
      "layout (location = 0) in vec4 a_position;               \n"
      "layout (location = 1) in vec2 a_texcoord;               \n"
      "out vec2 v_texcoord;                 \n"
      "void main() {                            \n"
      "   v_texcoord  = a_texcoord;             \n"
      "   gl_Position = a_position*u_mvpMatrix; \n"
      "}\n";
const GLchar * pYglprg_vdp2_drawfb_v[] = {Yglprg_vdp1_drawfb_v, NULL};

const GLchar Yglprg_vdp2_drawfb_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp sampler2D; \n"
"precision highp float;\n"
"#endif\n"
"in vec2 v_texcoord;\n"
"uniform sampler2D s_vdp1FrameBuffer;\n"
"uniform float u_from;\n"
"uniform float u_to;\n"
"uniform vec4 u_coloroffset;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  vec2 addr = v_texcoord;\n"
"  highp vec4 fbColor = texture(s_vdp1FrameBuffer,addr);\n"
"  int additional = int(fbColor.a * 255.0);\n"
"  highp float alpha = float((additional/8)*8)/255.0;\n"
"  highp float depth = (float(additional&0x07)/10.0) + 0.05;\n"
"  if( depth < u_from || depth > u_to ){\n"
"    discard;\n"
"  }else if( alpha > 0.0){\n"
"     fragColor = fbColor;\n"
"     fragColor += u_coloroffset;  \n"
"     fragColor.a = alpha + 7.0/255.0;\n"
"     gl_FragDepth = (depth+1.0)/2.0;\n"
"  }else{ \n"
"     discard;\n"
"  }\n"
"}\n";

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

const GLchar Yglprg_vdp2_drawfb_cram_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp sampler2D; \n"
"precision highp float;\n"
"#endif\n"
"layout(std140) uniform vdp2regs { \n"
" float u_pri[8]; \n"
" float u_alpha[8]; \n"
" vec4 u_coloroffset;\n"
" float u_cctl; \n"
" float u_emu_height; \n"
" float u_vheight; \n"
" int u_color_ram_offset; \n"
"}; \n"
"uniform highp sampler2D s_vdp1FrameBuffer;\n"
"uniform sampler2D s_color; \n"
"uniform sampler2D s_line; \n"
"uniform float u_from;\n"
"uniform float u_to;\n"
"in vec2 v_texcoord;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  vec2 addr = v_texcoord;\n"
"  highp vec4 fbColor = texture(s_vdp1FrameBuffer,addr);\n"
"  int additional = int(fbColor.a * 255.0);\n"
"  int opaque = int(fbColor.b * 255.0) & 0x01;\n"
"  int shadow = int(fbColor.b * 255.0) & 0x80;\n"
"  if( (additional & 0x80) == 0 ){ discard; } // show? \n"
"  int prinumber = (additional&0x07); "
"  float alpha = u_alpha[((additional>>3)&0x07)]; \n"
"  if (opaque != 0) alpha = 1.0; \n"
"  highp float depth = u_pri[ prinumber ];\n"
"  if( depth < u_from || depth > u_to ){ discard; } \n"
"  vec4 txcol=vec4(0.0,0.0,0.0,1.0);\n"
"  if( (additional & 0x40) != 0 ){  // index color? \n"
"    if( shadow != 0 ) {discard;} // draw shadow last path \n"
"    int colindex = ( int(fbColor.g*255.0)<<8 | int(fbColor.r*255.0)); \n"
"    if( colindex == 0 && prinumber == 0) { discard;} // hard/vdp1/hon/p02_11.htm 0 data is ignoerd \n"
"    colindex = colindex + u_color_ram_offset; \n"
"    txcol = texelFetch( s_color,  ivec2( colindex ,0 )  , 0 );\n"
"    fragColor = txcol;\n"
"  }else{ // direct color \n"
"    fragColor = fbColor;\n"
"  } \n"
"  fragColor = clamp(fragColor + u_coloroffset, vec4(0.0), vec4(1.0));  \n";

/*
 Color calculation option 
  hard/vdp2/hon/p09_21.htm
*/
const GLchar Yglprg_vdp2_drawfb_cram_no_color_col_f[]    = " fragColor.a = 1.0; \n";

const GLchar Yglprg_vdp2_drawfb_cram_destalpha_col_f[] = " fragColor.a = alpha; \n";

const GLchar Yglprg_vdp2_drawfb_cram_less_color_col_f[]  = " if( depth <= u_cctl ){ fragColor.a = alpha; }else{ fragColor.a = 1.0; } \n ";
const GLchar Yglprg_vdp2_drawfb_cram_equal_color_col_f[] = " if( depth == u_cctl ){ fragColor.a = alpha; }else{ fragColor.a = 1.0; } \n ";
const GLchar Yglprg_vdp2_drawfb_cram_more_color_col_f[]  = " if( depth >= u_cctl ){ fragColor.a = alpha; }else{ fragColor.a = 1.0; } \n ";
const GLchar Yglprg_vdp2_drawfb_cram_msb_color_col_f[]   = " if( txcol.a != 0.0 ){ fragColor.a = alpha; }else{ fragColor.a = 1.0; } \n ";

const GLchar Yglprg_vdp2_drawfb_cram_less_color_add_f[]  = " if( depth <= u_cctl ){ fragColor.a = 1.0; }else{ fragColor.a = 0.0; } \n ";
const GLchar Yglprg_vdp2_drawfb_cram_equal_color_add_f[] = " if( depth == u_cctl ){ fragColor.a = 1.0; }else{ fragColor.a = 0.0; } \n ";
const GLchar Yglprg_vdp2_drawfb_cram_more_color_add_f[]  = " if( depth >= u_cctl ){ fragColor.a = 1.0; }else{ fragColor.a = 0.0; } \n ";
const GLchar Yglprg_vdp2_drawfb_cram_msb_color_add_f[]   = " if( txcol.a != 0.0 ){ fragColor.a = 1.0; }else{ fragColor.a = 0.0; } \n ";

const GLchar Yglprg_vdp2_drawfb_line_blend_f[] =
"  ivec2 linepos; \n "
"  linepos.y = 0; \n "
"  linepos.x = int((u_vheight - gl_FragCoord.y) * u_emu_height);\n"
"  vec4 lncol = texelFetch( s_line, linepos,0 );\n"
"  fragColor = (fragColor*fragColor.a) + lncol*(1.0-fragColor.a); \n";

const GLchar Yglprg_vdp2_drawfb_line_add_f[] =
"  ivec2 linepos; \n "
"  linepos.y = 0; \n "
"  linepos.x = int((u_vheight - gl_FragCoord.y) * u_emu_height);\n"
"  vec4 lncol = texelFetch( s_line, linepos,0 );\n"
"  fragColor =  fragColor + lncol * fragColor.a ;  \n";

const GLchar Yglprg_vdp2_drawfb_cram_less_line_dest_alpha_f[] =
"  ivec2 linepos; \n "
"  linepos.y = 0; \n "
"  linepos.x = int((u_vheight - gl_FragCoord.y) * u_emu_height);\n"
"  vec4 lncol = texelFetch( s_line, linepos,0 );      \n"
"  if( depth <= u_cctl ){ fragColor = (fragColor*lncol.a) + lncol*(1.0-lncol.a); } \n";

const GLchar Yglprg_vdp2_drawfb_cram_equal_line_dest_alpha_f[] =
"  ivec2 linepos; \n "
"  linepos.y = 0; \n "
"  linepos.x = int((u_vheight - gl_FragCoord.y) * u_emu_height);\n"
"  vec4 lncol = texelFetch( s_line, linepos,0 );      \n"
"  if( depth == u_cctl ){ fragColor = (lncol*lncol.a) + fragColor*(1.0-lncol.a); } \n";

const GLchar Yglprg_vdp2_drawfb_cram_more_line_dest_alpha_f[] =
"  ivec2 linepos; \n "
"  linepos.y = 0; \n "
"  linepos.x = int((u_vheight - gl_FragCoord.y) * u_emu_height);\n"
"  vec4 lncol = texelFetch( s_line, linepos,0 );      \n"
"  if( depth >= u_cctl ){ fragColor =(fragColor*lncol.a) + lncol*(1.0-lncol.a); } \n";

const GLchar Yglprg_vdp2_drawfb_cram_msb_line_dest_alpha_f[] =
"  ivec2 linepos; \n "
"  linepos.y = 0; \n "
"  linepos.x = int((u_vheight - gl_FragCoord.y) * u_emu_height);\n"
"  vec4 lncol = texelFetch( s_line, linepos,0 );      \n"
"  if( txcol.a != 0.0 ){ fragColor = (fragColor*lncol.a) + lncol*(1.0-lncol.a); }\n";



const GLchar Yglprg_vdp2_drawfb_cram_eiploge_f[] =
"  gl_FragDepth = (depth+1.0)*0.5;\n"
"}\n";

/*------------------------------------------------------------------------------------
*  VDP2 Draw Frame buffer Operation( Perline color offset using hblankin )
*  Chaos Seed
* ----------------------------------------------------------------------------------*/
const GLchar Yglprg_vdp2_drawfb_hblank_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp sampler2D; \n"
"precision highp float;\n"
"#endif\n"
"layout(std140) uniform vdp2regs { \n"
" float u_pri[8]; \n"
" float u_alpha[8]; \n"
" vec4 u_coloroffset;\n"
" float u_cctl; \n"
" float u_emu_height; \n"
" float u_vheight; \n"
" int u_color_ram_offset; \n"
"}; \n"
"uniform highp sampler2D s_vdp1FrameBuffer;\n"
"uniform sampler2D s_color; \n"
"uniform highp sampler2D s_line; \n"
"uniform float u_from;\n"
"uniform float u_to;\n"
"in vec2 v_texcoord;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  ivec2 linepos; \n "
"  linepos.y = 0; \n "
"  linepos.x = int((u_vheight - gl_FragCoord.y) * u_emu_height);\n"
"  vec4 linetex = texelFetch( s_line, linepos,0 ); "
"  vec2 addr = v_texcoord;\n"
"  highp vec4 fbColor = texture(s_vdp1FrameBuffer,addr);\n"
"  int additional = int(fbColor.a * 255.0);\n"
"  if( (additional & 0x80) == 0 ){ discard; } // show? \n"
"  highp vec4 linepri = texelFetch( s_line, ivec2(linepos.x,1+(additional&0x07)) ,0 ); \n"
"  if( linepri.a == 0.0 ) discard; \n"
"  highp float depth = ((linepri.a*255.0)/10.0)+0.05 ;\n"
"  if( depth < u_from || depth > u_to ){ discard; } \n"
"  vec4 txcol=vec4(0.0,0.0,0.0,1.0);\n"
"  if( (additional & 0x40) != 0 ){  // index color? \n"
"    int shadow = int(fbColor.b * 255.0) & 0x80;\n"
"    if( shadow!= 0 ) {discard;} // draw shadow last path \n"
"    int colindex = ( int(fbColor.g*255.0)<<8 | int(fbColor.r*255.0)); \n"
"    if( colindex == 0 && (additional&0x07) == 0 ) { discard;} // hard/vdp1/hon/p02_11.htm 0 data is ignoerd \n"
"    colindex = colindex + u_color_ram_offset; \n"
"    txcol = texelFetch( s_color,  ivec2( colindex ,0 )  , 0 );\n"
"    fragColor = txcol;\n"
"  }else{ // direct color \n"
"    fragColor = fbColor;\n"
"  } \n"
"  fragColor.r += (linetex.r-0.5)*2.0;      \n"
"  fragColor.g += (linetex.g-0.5)*2.0;      \n"
"  fragColor.b += (linetex.b-0.5)*2.0;      \n";


const GLchar Yglprg_vdp2_drawfb_cram_less_color_col_hblank_f[] = 
" if( depth <= u_cctl ){ \n" 
"  vec4 linealpha = texelFetch( s_line,  ivec2(linepos.x,(1+8+((additional>>3)&0x07))) , 0 ); "
"  fragColor.a = linealpha.a; \n"
"}else{ fragColor.a = 1.0; } \n ";

const GLchar Yglprg_vdp2_drawfb_cram_equal_color_col_hblank_f[] =
" if( depth == u_cctl ){ \n"
"  vec4 linealpha = texelFetch( s_line, ivec2(linepos.x,(1+8+((additional>>3)&0x07))) , 0 ); \n"
"  fragColor.a = linealpha.a; \n"
"}else{ fragColor.a = 1.0; } \n ";

const GLchar Yglprg_vdp2_drawfb_cram_more_color_col_hblank_f[] =
" if( depth >= u_cctl ){ \n"
"  vec4 linealpha = texelFetch( s_line,  ivec2(linepos.x,(1+8+((additional>>3)&0x07))) , 0 ); "
"  fragColor.a = linealpha.a; \n"
"}else{ fragColor.a = 1.0; } \n ";

const GLchar Yglprg_vdp2_drawfb_cram_msb_color_col_hblank_f[] =
" if( txcol.a != 0.0 ){ \n"
"  vec4 linealpha = texelFetch( s_line,  ivec2(linepos.x,(1+8+((additional>>3)&0x07))) , 0 ); "
"  fragColor.a = linealpha.a; \n"
"}else{ fragColor.a = 1.0; } \n ";

const GLchar Yglprg_vdp2_drawfb_cram_destalpha_col_hblank_f[] =
"  vec4 linealpha = texelFetch( s_line,  ivec2(linepos.x,(1+8+((additional>>3)&0x07))) , 0 ); "
"  fragColor.a = linealpha.a; \n";


//const GLchar * pYglprg_vdp2_drawfb_f[] = {Yglprg_vdp2_drawfb_f, NULL};
const GLchar * pYglprg_vdp2_drawfb_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_no_color_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_destalpha_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_destalpha_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };


const GLchar * pYglprg_vdp2_drawfb_less_color_col_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_less_color_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_equal_color_col_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_equal_color_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_more_color_col_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_more_color_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_msb_color_col_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_msb_color_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };

const GLchar * pYglprg_vdp2_drawfb_less_color_add_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_less_color_add_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_equal_color_add_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_equal_color_add_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_more_color_add_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_more_color_add_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_msb_color_add_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_msb_color_add_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };

// per line operation using Line color insertion
const GLchar * pYglprg_vdp2_drawfb_less_destalpha_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_less_line_dest_alpha_f, Yglprg_vdp2_drawfb_cram_destalpha_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_equal_destalpha_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_equal_line_dest_alpha_f, Yglprg_vdp2_drawfb_cram_destalpha_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_more_destalpha_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_more_line_dest_alpha_f, Yglprg_vdp2_drawfb_cram_destalpha_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_msb_destalpha_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_msb_line_dest_alpha_f, Yglprg_vdp2_drawfb_cram_destalpha_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };

const GLchar * pYglprg_vdp2_drawfb_less_color_col_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_less_color_col_f, Yglprg_vdp2_drawfb_line_blend_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_equal_color_col_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_equal_color_col_f, Yglprg_vdp2_drawfb_line_blend_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_more_color_col_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_more_color_col_f, Yglprg_vdp2_drawfb_line_blend_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_msb_color_col_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_msb_color_col_f, Yglprg_vdp2_drawfb_line_blend_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };

const GLchar * pYglprg_vdp2_drawfb_less_color_add_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_less_color_add_f, Yglprg_vdp2_drawfb_line_add_f,Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_equal_color_add_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_equal_color_add_f, Yglprg_vdp2_drawfb_line_add_f,Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_more_color_add_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_more_color_add_f, Yglprg_vdp2_drawfb_line_add_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_msb_color_add_line_f[] = { Yglprg_vdp2_drawfb_cram_f, Yglprg_vdp2_drawfb_cram_msb_color_add_f, Yglprg_vdp2_drawfb_line_add_f,Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };

// per line operation using hbalnk inrerruption
const GLchar * pYglprg_vdp2_drawfb_hblank_v[] = { Yglprg_vdp1_drawfb_v, NULL };

const GLchar * pYglprg_vdp2_drawfb_hblank_f[] = { Yglprg_vdp2_drawfb_hblank_f, Yglprg_vdp2_drawfb_cram_no_color_col_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_hblank_destalpha_f[] = { Yglprg_vdp2_drawfb_hblank_f, Yglprg_vdp2_drawfb_cram_destalpha_col_hblank_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };

const GLchar * pYglprg_vdp2_drawfb_less_col_hbalnk_f[] = { Yglprg_vdp2_drawfb_hblank_f, Yglprg_vdp2_drawfb_cram_less_color_col_hblank_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_equal_col_hbalnk_f[] = { Yglprg_vdp2_drawfb_hblank_f, Yglprg_vdp2_drawfb_cram_equal_color_col_hblank_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_more_col_hblank_f[] = { Yglprg_vdp2_drawfb_hblank_f, Yglprg_vdp2_drawfb_cram_more_color_col_hblank_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_msb_col_hblank_f[] = { Yglprg_vdp2_drawfb_hblank_f, Yglprg_vdp2_drawfb_cram_msb_color_col_hblank_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };

const GLchar * pYglprg_vdp2_drawfb_less_add_hblank_f[]  = { Yglprg_vdp2_drawfb_hblank_f, Yglprg_vdp2_drawfb_cram_less_color_add_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_equal_add_hblank_f[] = { Yglprg_vdp2_drawfb_hblank_f, Yglprg_vdp2_drawfb_cram_equal_color_add_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_more_add_hblank_f[]  = { Yglprg_vdp2_drawfb_hblank_f, Yglprg_vdp2_drawfb_cram_more_color_add_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };
const GLchar * pYglprg_vdp2_drawfb_msb_add_hblank_f[]  = { Yglprg_vdp2_drawfb_hblank_f, Yglprg_vdp2_drawfb_cram_msb_color_add_f, Yglprg_vdp2_drawfb_cram_eiploge_f, NULL };


const GLchar Yglprg_vdp2_drawfb_shadow_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp sampler2D; \n"
"precision highp float;\n"
"#endif\n"
"layout(std140) uniform vdp2regs { \n"
" float u_pri[8]; \n"
" float u_alpha[8]; \n"
" vec4 u_coloroffset;\n"
" float u_cctl; \n"
" float u_emu_height; \n"
" float u_vheight; \n"
"}; \n"
"uniform highp sampler2D s_vdp1FrameBuffer;\n"
"in vec2 v_texcoord;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  vec2 addr = v_texcoord;\n"
"  highp vec4 fbColor = texture(s_vdp1FrameBuffer,addr);\n"
"  int additional = int(fbColor.a * 255.0);\n"
"  if( (additional & 0x80) == 0 ){ discard; } // show? \n"
"  highp float depth = u_pri[ (additional&0x07) ];\n"
"  int shadow = int(fbColor.b * 255.0) & 0x80;\n"
"  if( (additional & 0x40) != 0 && (shadow != 0) ){  // index color and shadow? \n"
"    fragColor = vec4(0.0,0.0,0.0,0.5);\n"
"  }else{ // direct color \n"
"    discard;;\n"
"  } \n"
"  gl_FragDepth = (depth+1.0)*0.5;\n"
"}\n";

const GLchar * pYglprg_vdp2_drawfb_shadow_f[] = { Yglprg_vdp2_drawfb_shadow_f,  NULL };

void Ygl_initDrawFrameBuffershader(int id);

int YglInitDrawFrameBufferShaders() {

YGLLOG("PG_VDP2_DRAWFRAMEBUFF --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_DESTALPHA --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_DESTALPHA, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_destalpha_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_DESTALPHA);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL --START--\n");
  // color calcurate rate
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_less_color_col_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_equal_color_col_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_more_color_col_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_msb_color_col_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_LESS_ADD --START--\n");
  // color calcurate add
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_LESS_ADD, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_less_color_add_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_LESS_ADD);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_equal_color_add_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MORE_ADD --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MORE_ADD, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_more_color_add_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MORE_ADD);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MSB_ADD --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MSB_ADD, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_msb_color_add_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MSB_ADD);


  //-------------------------------------------------------------------------------
  // Line color insertion
  //-------------------------------------------------------------------------------
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_LESS_DESTALPHA_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_LESS_DESTALPHA_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_less_destalpha_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_LESS_DESTALPHA_LINE);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_EQUAL_DESTALPHA_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_EQUAL_DESTALPHA_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_equal_destalpha_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_EQUAL_DESTALPHA_LINE);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MORE_DESTALPHA_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MORE_DESTALPHA_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_more_destalpha_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MORE_DESTALPHA_LINE);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MSB_DESTALPHA_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MSB_DESTALPHA_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_msb_destalpha_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MSB_DESTALPHA_LINE);

YGLLOG("PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_less_color_col_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL_LINE);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_equal_color_col_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL_LINE);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_more_color_col_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL_LINE);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_msb_color_col_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL_LINE);

  // color calcurate add
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_LESS_ADD_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_LESS_ADD_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_less_color_add_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_LESS_ADD_LINE);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_equal_color_add_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD_LINE);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MORE_ADD_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MORE_ADD_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_more_color_add_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MORE_ADD_LINE);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MSB_ADD_LINE --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MSB_ADD_LINE, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_msb_color_add_line_f, 4, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MSB_ADD_LINE);

  //------------------------------------------------------------------
  // HBALNK per line register chnage operation
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_HBLANK --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_HBLANK, pYglprg_vdp2_drawfb_hblank_v, pYglprg_vdp2_drawfb_hblank_f, 3, NULL, NULL, NULL) != 0) { return -1; }

  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_HBLANK);

YGLLOG("PG_VDP2_DRAWFRAMEBUFF_DESTALPHA_HBLANK --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_DESTALPHA_HBLANK, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_hblank_destalpha_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_DESTALPHA_HBLANK);

  // color calcurate rate
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL_HBLANK --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL_HBLANK, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_less_col_hbalnk_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL_HBLANK);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL_HBLANK --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL_HBLANK, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_equal_col_hbalnk_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL_HBLANK);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL_HBLANK --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL_HBLANK, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_more_col_hblank_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL_HBLANK);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL_HBLANK --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL_HBLANK, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_msb_col_hblank_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL_HBLANK);

  // color calcurate add
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_LESS_ADD_HBLANK --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_LESS_ADD_HBLANK, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_less_add_hblank_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_LESS_ADD_HBLANK);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD_HBLANK --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD_HBLANK, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_equal_add_hblank_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD_HBLANK);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MORE_ADD_HBLANK --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MORE_ADD_HBLANK, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_more_add_hblank_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MORE_ADD_HBLANK);
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_MSB_ADD_HBLANK --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_MSB_ADD_HBLANK, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_msb_add_hblank_f, 3, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_MSB_ADD_HBLANK);

  //------------------------------------------------------------------
  // Shadow
YGLLOG("PG_VDP2_DRAWFRAMEBUFF_SHADOW --START--\n");
  if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_SHADOW, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_shadow_f, 1, NULL, NULL, NULL) != 0) { return -1; }
  Ygl_initDrawFrameBuffershader(PG_VDP2_DRAWFRAMEBUFF_SHADOW);

  _Ygl->renderfb.prgid = _prgid[PG_VDP2_DRAWFRAMEBUFF];
  _Ygl->renderfb.setupUniform = Ygl_uniformNormal;
  _Ygl->renderfb.cleanupUniform = Ygl_cleanupNormal;
  _Ygl->renderfb.vertexp = glGetAttribLocation(_prgid[PG_VDP2_DRAWFRAMEBUFF], (const GLchar *)"a_position");
  _Ygl->renderfb.texcoordp = glGetAttribLocation(_prgid[PG_VDP2_DRAWFRAMEBUFF], (const GLchar *)"a_texcoord");
  _Ygl->renderfb.mtxModelView = glGetUniformLocation(_prgid[PG_VDP2_DRAWFRAMEBUFF], (const GLchar *)"u_mvpMatrix");

  return 0;
}


void Ygl_initDrawFrameBuffershader(int id) {

  GLuint scene_block_index;
  int arrayid = id- PG_VDP2_DRAWFRAMEBUFF;
  if ( arrayid < 0 || arrayid >= MAX_FRAME_BUFFER_UNIFORM) {
    abort();
  }

  scene_block_index = glGetUniformBlockIndex(_prgid[id], "vdp2regs");
  glUniformBlockBinding(_prgid[id], scene_block_index, FRAME_BUFFER_UNIFORM_ID);
  g_draw_framebuffer_uniforms[arrayid].idvdp1FrameBuffer = glGetUniformLocation(_prgid[id], (const GLchar *)"s_vdp1FrameBuffer");
  g_draw_framebuffer_uniforms[arrayid].idcram = glGetUniformLocation(_prgid[id], (const GLchar *)"s_color");
  g_draw_framebuffer_uniforms[arrayid].idfrom = glGetUniformLocation(_prgid[id], (const GLchar *)"u_from");
  g_draw_framebuffer_uniforms[arrayid].idto = glGetUniformLocation(_prgid[id], (const GLchar *)"u_to");
  g_draw_framebuffer_uniforms[arrayid].idline = glGetUniformLocation(_prgid[id], (const GLchar *)"s_line");
}


void Ygl_uniformVDP2DrawFramebuffer_perline(void * p, float from, float to, u32 linetexture, Vdp2 *varVdp2Regs)
{
  YglProgram * prg;
  int arrayid;

  int pgid = PG_VDP2_DRAWFRAMEBUFF_HBLANK;

  const int SPCCN = ((varVdp2Regs->CCCTL >> 6) & 0x01); // hard/vdp2/hon/p12_14.htm#NxCCEN_
  const int CCRTMD = ((varVdp2Regs->CCCTL >> 9) & 0x01); // hard/vdp2/hon/p12_14.htm#CCRTMD_
  const int CCMD = ((varVdp2Regs->CCCTL >> 8) & 0x01);  // hard/vdp2/hon/p12_14.htm#CCMD_
  const int SPLCEN = (varVdp2Regs->LNCLEN & 0x20); // hard/vdp2/hon/p11_30.htm#NxLCEN_

  prg = p;

  if ( SPCCN ) {
    const int SPCCCS = (varVdp2Regs->SPCTL >> 12) & 0x3;
    if (CCMD == 0) {  // Calculate Rate mode
      if (CCRTMD == 0) {  // Source Alpha Mode
        if (SPLCEN == 0) { // No Line Color Insertion
          glEnable(GL_BLEND);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          switch (SPCCCS)
          {
          case 0:
            pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL_HBLANK;
            break;
          case 1:
            pgid = PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL_HBLANK;
            break;
          case 2:
            pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL_HBLANK;
            break;
          case 3:
            pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL_HBLANK;
            break;
          }
        }
        else { // Line Color Insertion
          // ToDo:
        }
      }
      else { // Destination Alpha Mode

        if (SPLCEN == 0) { // No Line Color Insertion
          pgid = PG_VDP2_DRAWFRAMEBUFF_DESTALPHA_HBLANK;
        }
        else {
          // ToDo:
        }
      }
    }
    else { // Add Color Mode
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_SRC_ALPHA);
      if (SPLCEN == 0) { // No Line Color Insertion
        switch (SPCCCS)
        {
        case 0:
          pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_ADD_HBLANK;
          break;
        case 1:
          pgid = PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD_HBLANK;
          break;
        case 2:
          pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_ADD_HBLANK;
          break;
        case 3:
          pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_ADD_HBLANK;
          break;
        }
      }
      else {
        // ToDo:
      }
    }
  }
  else { // No Color Calculation
    glDisable(GL_BLEND);
    pgid = PG_VDP2_DRAWFRAMEBUFF_HBLANK;
  }


  arrayid = pgid - PG_VDP2_DRAWFRAMEBUFF;
  GLUSEPROG(_prgid[pgid]);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glDisableVertexAttribArray(3);
  _Ygl->renderfb.mtxModelView = glGetUniformLocation(_prgid[pgid], (const GLchar *)"u_mvpMatrix");

  glBindBufferBase(GL_UNIFORM_BUFFER, FRAME_BUFFER_UNIFORM_ID, _Ygl->framebuffer_uniform_id_);
  glUniform1f(g_draw_framebuffer_uniforms[arrayid].idfrom, from);
  glUniform1f(g_draw_framebuffer_uniforms[arrayid].idto, to);

  glUniform1i(g_draw_framebuffer_uniforms[arrayid].idcram, 1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);

  glUniform1i(g_draw_framebuffer_uniforms[arrayid].idvdp1FrameBuffer, 0);
  glActiveTexture(GL_TEXTURE0);

  // Setup Line color uniform
  glUniform1i(g_draw_framebuffer_uniforms[arrayid].idline, 2);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, linetexture);
  glActiveTexture(GL_TEXTURE0);
}

void Ygl_uniformVDP2DrawFrameBufferShadow(void * p) {
  int pgid = PG_VDP2_DRAWFRAMEBUFF_SHADOW;
  int arrayid = pgid - PG_VDP2_DRAWFRAMEBUFF;
  GLUSEPROG(_prgid[pgid]);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glDisableVertexAttribArray(3);
  _Ygl->renderfb.mtxModelView = glGetUniformLocation(_prgid[pgid], (const GLchar *)"u_mvpMatrix");
  glBindBufferBase(GL_UNIFORM_BUFFER, FRAME_BUFFER_UNIFORM_ID, _Ygl->framebuffer_uniform_id_);

  glUniform1i(g_draw_framebuffer_uniforms[arrayid].idvdp1FrameBuffer, 0);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void Ygl_uniformVDP2DrawFramebuffer(void * p, float from, float to, float * offsetcol, int blend, Vdp2 *varVdp2Regs)
{
   YglProgram * prg;
   int arrayid;

   int pgid = PG_VDP2_DRAWFRAMEBUFF;

   const int SPCCN = ((varVdp2Regs->CCCTL >> 6) & 0x01); // hard/vdp2/hon/p12_14.htm#NxCCEN_
   const int CCRTMD = ((varVdp2Regs->CCCTL >> 9) & 0x01); // hard/vdp2/hon/p12_14.htm#CCRTMD_
   const int CCMD = ((varVdp2Regs->CCCTL >> 8) & 0x01);  // hard/vdp2/hon/p12_14.htm#CCMD_
   const int SPLCEN = (varVdp2Regs->LNCLEN & 0x20); // hard/vdp2/hon/p11_30.htm#NxLCEN_

   prg = p;

   if ( blend && SPCCN ) {
     const int SPCCCS = (varVdp2Regs->SPCTL >> 12) & 0x3;
     if (CCMD == 0) {  // Calculate Rate mode
       if (CCRTMD == 0) {  // Source Alpha Mode
         if (SPLCEN == 0) { // No Line Color Insertion
           switch (SPCCCS)
           {
           case 0:
             pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL;
             break;
           case 1:
             pgid = PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL;
             break;
           case 2:
             pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL;
             break;
           case 3:
             pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL;
             break;
           }
         } else { // Line Color Insertion
           switch (SPCCCS)
           {
           case 0:
             pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_CCOL_LINE;
             break;
           case 1:
             pgid = PG_VDP2_DRAWFRAMEBUFF_EUQAL_CCOL_LINE;
             break;
           case 2:
             pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_CCOL_LINE;
             break;
           case 3:
             pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_CCOL_LINE;
             break;
           }
         }
       } else { // Destination Alpha Mode

         if (SPLCEN == 0) { // No Line Color Insertion
           pgid = PG_VDP2_DRAWFRAMEBUFF_DESTALPHA;
         }
         else {
           switch (SPCCCS)
           {
           case 0:
             pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_DESTALPHA_LINE;
             break;
           case 1:
             pgid = PG_VDP2_DRAWFRAMEBUFF_EQUAL_DESTALPHA_LINE;
             break;
           case 2:
             pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_DESTALPHA_LINE;
             break;
           case 3:
             pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_DESTALPHA_LINE;
             break;
           }
         }
       }
     } else { // Add Color Mode
       glEnable(GL_BLEND);
       glBlendFunc(GL_ONE, GL_SRC_ALPHA);
       if (SPLCEN == 0) { // No Line Color Insertion
         switch (SPCCCS)
         {
         case 0:
           pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_ADD;
           break;
         case 1:
           pgid = PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD;
           break;
         case 2:
           pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_ADD;
           break;
         case 3:
           pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_ADD;
           break;
         }
       }else{
         switch (SPCCCS)
         {
         case 0:
           pgid = PG_VDP2_DRAWFRAMEBUFF_LESS_ADD_LINE;
           break;
         case 1:
           pgid = PG_VDP2_DRAWFRAMEBUFF_EUQAL_ADD_LINE;
           break;
         case 2:
           pgid = PG_VDP2_DRAWFRAMEBUFF_MORE_ADD_LINE;
           break;
         case 3:
           pgid = PG_VDP2_DRAWFRAMEBUFF_MSB_ADD_LINE;
           break;
         }
       }
     }
   } else { // No Color Calculation
     glDisable(GL_BLEND);
     pgid = PG_VDP2_DRAWFRAMEBUFF;
   }

   arrayid = pgid - PG_VDP2_DRAWFRAMEBUFF;
   GLUSEPROG(_prgid[pgid]);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(3);

   _Ygl->renderfb.mtxModelView = glGetUniformLocation(_prgid[pgid], (const GLchar *)"u_mvpMatrix");

   glBindBufferBase(GL_UNIFORM_BUFFER, FRAME_BUFFER_UNIFORM_ID, _Ygl->framebuffer_uniform_id_);
   glUniform1f(g_draw_framebuffer_uniforms[arrayid].idfrom, from);
   glUniform1f(g_draw_framebuffer_uniforms[arrayid].idto, to);

   glUniform1i(g_draw_framebuffer_uniforms[arrayid].idcram, 1);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);

   glUniform1i(g_draw_framebuffer_uniforms[arrayid].idvdp1FrameBuffer, 0);
   glActiveTexture(GL_TEXTURE0);

   // Setup Line color uniform
   if (SPLCEN != 0) {
     glUniform1i(g_draw_framebuffer_uniforms[arrayid].idline, 2);
     glActiveTexture(GL_TEXTURE2);
     glBindTexture(GL_TEXTURE_2D, _Ygl->lincolor_tex);
     glActiveTexture(GL_TEXTURE0);
     glDisable(GL_BLEND);
   }

   return;
}



/*------------------------------------------------------------------------------------
*  VDP2 Draw Frame buffer Operation( Shadow drawing for ADD color mode )
* ----------------------------------------------------------------------------------*/

const GLchar Yglprg_vdp2_drawfb_addcolor_shadow_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp sampler2D; \n"
"precision highp float;\n"
"#endif\n"
"in vec2 v_texcoord;\n"
"uniform sampler2D s_vdp1FrameBuffer;\n"
"uniform float u_from;\n"
"uniform float u_to;\n"
"uniform vec4 u_coloroffset;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  vec2 addr = v_texcoord;\n"
"  highp vec4 fbColor = texture(s_vdp1FrameBuffer,addr);\n"
"  highp int additional = int(fbColor.a * 255.0);\n"
"  highp float alpha = float((additional/8)*8)/255.0;\n"
"  highp float depth = ((float(additional&0x07))/10.0) + 0.05;\n"
"  if( depth < u_from || depth > u_to ){ discard;return;}\n"
"  if( alpha <= 0.0){\n"
"     discard;\n"
"  }else if( alpha < 0.75 && fbColor.r == 0.0 && fbColor.g == 0.0 && fbColor.b == 0.0 ){\n"
"     fragColor = fbColor;\n"
"     fragColor.a = alpha;\n"
"     gl_FragDepth =  (depth+1.0)/2.0;\n"
"  }else{\n"
"     discard;\n"
"  }\n "
"}\n";


const GLchar * pYglprg_vdp2_drawfb_addcolor_shadow_f[] = { Yglprg_vdp2_drawfb_addcolor_shadow_f, NULL };

static int idvdp1FrameBuffer_addcolor_shadow;
static int idfrom_addcolor_shadow;
static int idto_addcolor_shadow;
static int idcoloroffset_addcolor_shadow;

int Ygl_uniformVDP2DrawFramebuffer_addcolor_shadow(void * p, float from, float to, float * offsetcol)
{
  YglProgram * prg;
  prg = p;

  GLUSEPROG(_prgid[PG_VDP2_DRAWFRAMEBUFF_ADDCOLOR_SHADOW]);
  glUniform1i(idvdp1FrameBuffer_addcolor_shadow, 0);
  glActiveTexture(GL_TEXTURE0);
  glUniform1f(idfrom_addcolor_shadow, from);
  glUniform1f(idto_addcolor_shadow, to);
  glUniform4fv(idcoloroffset_addcolor_shadow, 1, offsetcol);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  _Ygl->renderfb.mtxModelView = glGetUniformLocation(_prgid[PG_VDP2_DRAWFRAMEBUFF_ADDCOLOR_SHADOW], (const GLchar *)"u_mvpMatrix");
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  return 0;
}

int Ygl_cleanupVDP2DrawFramebuffer_addcolor_shadow(void * p, YglTextureManager *tm){
  
  return 0;
}

/*------------------------------------------------------------------------------------
 *  VDP2 Add Blend operaiotn
 * ----------------------------------------------------------------------------------*/
int Ygl_uniformAddBlend(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs )
{
   glBlendFunc(GL_ONE,GL_ONE);
   return 0;
}

int Ygl_cleanupAddBlend(void * p, YglTextureManager *tm)
{
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
   return 0;
}


/*------------------------------------------------------------------------------------
*  11.3 Line Color Insertion
* ----------------------------------------------------------------------------------*/
const GLchar * pYglprg_linecol_v[] = { Yglprg_normal_v, NULL };

const GLchar Yglprg_linecol_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"in highp vec4 v_texcoord;\n"
"uniform vec4 u_color_offset;\n"
"uniform float u_emu_height;\n"
"uniform float u_vheight; \n"
"uniform highp sampler2D s_texture;\n"
"uniform sampler2D s_color;\n"
"uniform sampler2D s_line;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"  ivec2 addr; \n"
"  addr.x = int(v_texcoord.x);\n"
"  addr.y = int(v_texcoord.y);\n"
"  ivec2 linepos; \n "
"  linepos.y = 0; \n "
"  linepos.x = int( (u_vheight-gl_FragCoord.y) * u_emu_height);\n"
"  vec4 txcol = texelFetch( s_texture, addr,0 );      \n"
"  vec4 lncol = texelFetch( s_line, linepos,0 );      \n"
"  if(txcol.a > 0.0){\n";

const GLchar Yglprg_linecol_main_f[] =
"    fragColor = txcol+u_color_offset+lncol;\n"
"    fragColor.a = 1.0;\n";

const GLchar Yglprg_linecol_destalpha_f[] =
"    fragColor = (txcol * (1.0-lncol.a))+(lncol*lncol.a)+u_color_offset;\n"
"    fragColor.a =txcol.a;\n";

const GLchar Yglprg_linecol_main_cram_f[] =
"    vec4 txcolc = texelFetch( s_color,  ivec2( ( int(txcol.g*255.0)<<8 | int(txcol.r*255.0)) ,0 )  , 0 );\n"
"    fragColor = txcolc+u_color_offset+lncol;\n"
"    fragColor.a = 1.0;\n";

const GLchar Yglprg_linecol_destalpha_cram_f[] =
"    vec4 txcolc = texelFetch( s_color,  ivec2( ( int(txcol.g*255.0)<<8 | int(txcol.r*255.0)) ,0 )  , 0 );\n"
"    fragColor = clamp((txcolc * lncol.a)+lncol+u_color_offset,vec4(0.0),vec4(1.0));\n"
"    fragColor.a = txcol.a;\n";

const GLchar Yglprg_linecol_finish_f[] =
"  }else{ \n"
"    discard;\n"
"  }\n"
"}\n";

const GLchar * pYglprg_linecol_f[] = { Yglprg_linecol_f, Yglprg_linecol_main_f, Yglprg_linecol_finish_f};
const GLchar * pYglprg_linecol_dest_alpha_f[] = { Yglprg_linecol_f, Yglprg_linecol_destalpha_f, Yglprg_linecol_finish_f };

const GLchar * pYglprg_linecol_cram_f[] = { Yglprg_linecol_f, Yglprg_linecol_main_cram_f, Yglprg_linecol_finish_f };
const GLchar * pYglprg_linecol_dest_alpha_cram_f[] = { Yglprg_linecol_f, Yglprg_linecol_destalpha_cram_f, Yglprg_linecol_finish_f };


typedef struct _LinecolUniform{
  int s_texture;
  int s_color;
  int s_line;
  int color_offset;
  int emu_height;
  int vheight;
} LinecolUniform;

LinecolUniform linecol = { 0 };
LinecolUniform linecol_cram = { 0 };
LinecolUniform linecol_destalpha = { 0 };
LinecolUniform linecol_destalpha_cram = { 0 };

int Ygl_uniformLinecolorInsert(void * p, YglTextureManager *tm, Vdp2 *varVdp2Regs)
{
  YglProgram * prg;
  LinecolUniform * param = &linecol;

  prg = p;
  if (prg->prg == _prgid[PG_LINECOLOR_INSERT_DESTALPHA]){
    param = &linecol_destalpha;
  }
  else if (prg->prg == _prgid[PG_LINECOLOR_INSERT_CRAM] ) {
    param = &linecol_cram;
    glUniform1i(param->s_color, 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);
  }
  else if (prg->prg == _prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM]) {
    param = &linecol_destalpha_cram;
    glUniform1i(param->s_color, 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _Ygl->cram_tex);
  }

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glUniform1i(param->s_texture, 0);
  glUniform1i(param->s_line, 1);
  glUniform4fv(param->color_offset, 1, prg->color_offset_val);
  glUniform1f(param->emu_height, (float)_Ygl->rheight / (float)_Ygl->height);
  //glUniform1f(param->height_ratio, (float)_Ygl->rheight/((float)_Ygl->height * (float)_Ygl->density));  
  glUniform1f(param->vheight, (float)_Ygl->height);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, _Ygl->lincolor_tex);
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_BLEND);
  return 0;
}

int Ygl_cleanupLinecolorInsert(void * p, YglTextureManager *tm)
{
  YglProgram * prg;
  prg = p;
  
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, 0);
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_BLEND);

  return 0;
}




int YglGetProgramId( int prg )
{
   return _prgid[prg];
}

int YglInitShader(int id, const GLchar * vertex[], const GLchar * frag[], int fcount, const GLchar * tc[], const GLchar * te[], const GLchar * g[] )
{
    GLint compiled,linked;
    GLuint vshader;
    GLuint fshader;
  GLuint tcsHandle = 0;
  GLuint tesHandle = 0;
  GLuint gsHandle = 0;

   _prgid[id] = glCreateProgram();
    if (_prgid[id] == 0 ) return -1;

    vshader = glCreateShader(GL_VERTEX_SHADER);
  fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, vertex, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
       YGLLOG( "Compile error in vertex shader. %d\n", id );
       Ygl_printShaderError(vshader);
       _prgid[id] = 0;
       return -1;
    }

    glShaderSource(fshader, fcount, frag, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
       YGLLOG( "Compile error in fragment shader.%d \n", id);
       Ygl_printShaderError(fshader);
       _prgid[id] = 0;
       return -1;
     }

    glAttachShader(_prgid[id], vshader);
    glAttachShader(_prgid[id], fshader);

  if (tc != NULL){
    tcsHandle = glCreateShader(GL_TESS_CONTROL_SHADER);
    if (tcsHandle == 0){
      YGLLOG("GL_TESS_CONTROL_SHADER is not supported\n");
    }
    glShaderSource(tcsHandle, 1, tc, NULL);
    glCompileShader(tcsHandle);
    glGetShaderiv(tcsHandle, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in GL_TESS_CONTROL_SHADER shader.\n");
      Ygl_printShaderError(tcsHandle);
      _prgid[id] = 0;
      return -1;
    }
    glAttachShader(_prgid[id], tcsHandle);
  }
  if (te != NULL){
    tesHandle = glCreateShader(GL_TESS_EVALUATION_SHADER);
    if (tesHandle == 0){
      YGLLOG("GL_TESS_EVALUATION_SHADER is not supported\n");
    }
    glShaderSource(tesHandle, 1, te, NULL);
    glCompileShader(tesHandle);
    glGetShaderiv(tesHandle, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in GL_TESS_EVALUATION_SHADER shader.\n");
      Ygl_printShaderError(tesHandle);
      _prgid[id] = 0;
      return -1;
    }
    glAttachShader(_prgid[id], tesHandle);
  }
  if (g != NULL){
    gsHandle = glCreateShader(GL_GEOMETRY_SHADER);
    if (gsHandle == 0){
      YGLLOG("GL_GEOMETRY_SHADER is not supported\n");
    }
    glShaderSource(gsHandle, 1, g, NULL);
    glCompileShader(gsHandle);
    glGetShaderiv(gsHandle, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in GL_TESS_EVALUATION_SHADER shader.\n");
      Ygl_printShaderError(gsHandle);
      _prgid[id] = 0;
      return -1;
    }
    glAttachShader(_prgid[id], gsHandle);
  }

    glLinkProgram(_prgid[id]);
    glGetProgramiv(_prgid[id], GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
       YGLLOG("Link error..\n");
       Ygl_printShaderError(_prgid[id]);
       _prgid[id] = 0;
       return -1;
    }
    return 0;
}

int YglProgramInit()
{
   YGLLOG("PG_NORMAL\n");
   //
   if (YglInitShader(PG_NORMAL, pYglprg_normal_v, pYglprg_normal_f, 1, NULL, NULL, NULL) != 0)
      return -1;

  id_normal_s_texture = glGetUniformLocation(_prgid[PG_NORMAL], (const GLchar *)"s_texture");
  //id_normal_s_texture_size = glGetUniformLocation(_prgid[PG_NORMAL], (const GLchar *)"u_texsize");
  id_normal_color_offset = glGetUniformLocation(_prgid[PG_NORMAL], (const GLchar *)"u_color_offset");
  id_normal_matrix = glGetUniformLocation(_prgid[PG_NORMAL], (const GLchar *)"u_mvpMatrix");

   YGLLOG("PG_VDP2_NORMAL_CRAM\n");

  if (YglInitShader(PG_VDP2_NORMAL_CRAM, pYglprg_normal_v, pYglprg_normal_cram_f, 1, NULL, NULL, NULL) != 0)
    return -1;

  id_normal_cram_s_texture = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"s_texture");
  id_normal_cram_s_color = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"s_color");
  id_normal_cram_color_offset = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"u_color_offset");
  id_normal_cram_matrix = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM], (const GLchar *)"u_mvpMatrix");

   YGLLOG("PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY\n");

  if (YglInitShader(PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY, pYglprg_normal_v, pYglprg_normal_cram_special_priority_f, 1, NULL, NULL, NULL) != 0)
    return -1;

  id_normal_cram_sp_s_texture = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY], (const GLchar *)"s_texture");
  id_normal_cram_sp_s_color = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY], (const GLchar *)"s_color");
  id_normal_cram_sp_color_offset = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY], (const GLchar *)"u_color_offset");
  id_normal_cram_sp_matrix = glGetUniformLocation(_prgid[PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY], (const GLchar *)"u_mvpMatrix");

   YGLLOG("PG_VDP2_ADDCOLOR_CRAM\n");

  if (YglInitShader(PG_VDP2_ADDCOLOR_CRAM, pYglprg_normal_v, pYglprg_normal_cram_addcol_f, 1, NULL, NULL, NULL) != 0)
    return -1;

  id_normal_cram_s_texture_addcol = glGetUniformLocation(_prgid[PG_VDP2_ADDCOLOR_CRAM], (const GLchar *)"s_texture");
  id_normal_cram_s_color_addcol = glGetUniformLocation(_prgid[PG_VDP2_ADDCOLOR_CRAM], (const GLchar *)"s_color");
  id_normal_cram_color_offset_addcol = glGetUniformLocation(_prgid[PG_VDP2_ADDCOLOR_CRAM], (const GLchar *)"u_color_offset");
  id_normal_cram_matrix_addcol = glGetUniformLocation(_prgid[PG_VDP2_ADDCOLOR_CRAM], (const GLchar *)"u_mvpMatrix");

   YGLLOG("PG_VDP2_RBG_CRAM_LINE\n");

  if (YglInitShader(PG_VDP2_RBG_CRAM_LINE, pYglprg_normal_v, pYglprg_rbg_cram_line_f, 1, NULL, NULL, NULL) != 0)
    return -1;

  id_rbg_cram_line_s_texture = glGetUniformLocation(_prgid[PG_VDP2_RBG_CRAM_LINE], (const GLchar *)"s_texture");
  id_rbg_cram_line_s_color = glGetUniformLocation(_prgid[PG_VDP2_RBG_CRAM_LINE], (const GLchar *)"s_color");
  id_rbg_cram_line_color_offset = glGetUniformLocation(_prgid[PG_VDP2_RBG_CRAM_LINE], (const GLchar *)"u_color_offset");
  id_rbg_cram_line_blendmode = glGetUniformLocation(_prgid[PG_VDP2_RBG_CRAM_LINE], (const GLchar *)"u_blendmode");
  id_rbg_cram_line_matrix = glGetUniformLocation(_prgid[PG_VDP2_RBG_CRAM_LINE], (const GLchar *)"u_mvpMatrix");
  


#if 0
  YGLLOG("PG_VDP2_MOSAIC\n");
  if (YglInitShader(PG_VDP2_MOSAIC, pYglprg_vdp1_normal_v, pYglprg_mosaic_f, NULL, NULL, NULL) != 0)
    return -1;
  id_mosaic_s_texture = glGetUniformLocation(_prgid[PG_VDP2_MOSAIC], (const GLchar *)"s_texture");
  id_mosaic = glGetUniformLocation(_prgid[PG_VDP2_MOSAIC], (const GLchar *)"u_mosaic");
  id_mosaic_color_offset = glGetUniformLocation(_prgid[PG_VDP2_MOSAIC], (const GLchar *)"u_color_offset");
#endif

   _prgid[PG_VDP2_ADDBLEND] = _prgid[PG_NORMAL];

   _prgid[PG_VDP2_BLUR] = _prgid[PG_NORMAL];
   _prgid[PG_VDP2_MOSAIC] = _prgid[PG_NORMAL];
   _prgid[PG_VDP2_PER_LINE_ALPHA] = _prgid[PG_NORMAL];

   _prgid[PG_VDP2_BLUR_CRAM] = _prgid[PG_VDP2_NORMAL_CRAM];
   _prgid[PG_VDP2_MOSAIC_CRAM] = _prgid[PG_VDP2_NORMAL_CRAM];
   _prgid[PG_VDP2_PER_LINE_ALPHA_CRAM] = _prgid[PG_VDP2_NORMAL_CRAM];

   _prgid[PG_VFP1_ENDUSERCLIP] = _prgid[PG_NORMAL];

   YGLLOG("PG_VDP1_NORMAL\n");
   //
   if (YglInitShader(PG_VDP1_NORMAL, pYglprg_vdp1_normal_v, pYglprg_vdp1_normal_f,1, NULL, NULL, NULL) != 0)
      return -1;

   id_vdp1_normal_s_texture = glGetUniformLocation(_prgid[PG_VDP1_NORMAL], (const GLchar *)"s_texture");
   id_vdp1_normal_s_texture_size = glGetUniformLocation(_prgid[PG_VDP1_NORMAL], (const GLchar *)"u_texsize");




   //-----------------------------------------------------------------------------------------------------------
   YGLLOG("PG_VFP1_GOURAUDSAHDING\n");

   if (YglInitShader(PG_VFP1_GOURAUDSAHDING, pYglprg_vdp1_gouraudshading_v, pYglprg_vdp1_gouraudshading_f, 1, NULL, NULL, NULL) != 0)
      return -1;

   Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_GOURAUDSAHDING], &id_g);

   if (YglInitShader(PG_VFP1_GOURAUDSAHDING_SPD, pYglprg_vdp1_gouraudshading_v, pYglprg_vdp1_gouraudshading_spd_f, 1, NULL, NULL, NULL) != 0)
     return -1;

   Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_GOURAUDSAHDING_SPD], &id_spd_g);


   YGLLOG("PG_VDP2_DRAWFRAMEBUFF --START--\n");

   if (YglInitDrawFrameBufferShaders() != 0) {
     return -1;
   }

   //-----------------------------------------------------------------------------------------------------------
   YGLLOG("PG_VFP1_HALFTRANS\n");

   //
   if (YglInitShader(PG_VFP1_HALFTRANS, pYglprg_vdp1_halftrans_v, pYglprg_vdp1_halftrans_f, 1, NULL, NULL, NULL) != 0)
      return -1;

   Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_HALFTRANS], &hf);


   //-----------------------------------------------------------------------------------------------------------
   YGLLOG("PG_VFP1_SHADOW\n");

   //
   if (YglInitShader(PG_VFP1_SHADOW, pYglprg_vdp1_shadow_v, pYglprg_vdp1_shadow_f, 1, NULL, NULL, NULL) != 0)
     return -1;

   Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_SHADOW], &shadow);

   shadow.sprite = glGetUniformLocation(_prgid[PG_VFP1_SHADOW], (const GLchar *)"u_sprite");
   shadow.fbo = glGetUniformLocation(_prgid[PG_VFP1_SHADOW], (const GLchar *)"u_fbo");
   shadow.fbowidth = glGetUniformLocation(_prgid[PG_VFP1_SHADOW], (const GLchar *)"u_fbowidth");
   shadow.fboheight = glGetUniformLocation(_prgid[PG_VFP1_SHADOW], (const GLchar *)"u_fbohegiht");


   //-----------------------------------------------------------------------------------------------------------
   YGLLOG("PG_VFP1_GOURAUDSAHDING_HALFTRANS\n");

   if (YglInitShader(PG_VFP1_GOURAUDSAHDING_HALFTRANS, pYglprg_vdp1_gouraudshading_hf_v, pYglprg_vdp1_gouraudshading_hf_f, 1, NULL, NULL, NULL) != 0)
      return -1;

   Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_GOURAUDSAHDING_HALFTRANS], &id_ght);

   //-----------------------------------------------------------------------------------------------------------
   YGLLOG("PG_VFP1_HALF_LUMINANCE\n");

   if (YglInitShader(PG_VFP1_HALF_LUMINANCE, pYglprg_vdp1_half_luminance_v, pYglprg_vdp1_half_luminance_f, 1, NULL, NULL, NULL) != 0)
     return -1;

   Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_HALF_LUMINANCE], &half_luminance);

   //-----------------------------------------------------------------------------------------------------------
   YGLLOG("PG_VFP1_MESH\n");

   if (YglInitShader(PG_VFP1_MESH, pYglprg_vdp1_mesh_v, pYglprg_vdp1_mesh_f, 1, NULL, NULL, NULL) != 0)
     return -1;

   Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_MESH], &mesh);



   YGLLOG("PG_WINDOW\n");
   //
   if (YglInitShader(PG_WINDOW, pYglprg_window_v, pYglprg_window_f, 1, NULL, NULL, NULL) != 0)
      return -1;

   _Ygl->windowpg.prgid=_prgid[PG_WINDOW];
   _Ygl->windowpg.setupUniform    = Ygl_uniformNormal;
   _Ygl->windowpg.cleanupUniform  = Ygl_cleanupNormal;
   _Ygl->windowpg.vertexp         = glGetAttribLocation(_prgid[PG_WINDOW],(const GLchar *)"a_position");
   _Ygl->windowpg.mtxModelView    = glGetUniformLocation(_prgid[PG_WINDOW],(const GLchar *)"u_mvpMatrix");

   _prgid[PG_VFP1_STARTUSERCLIP] = _prgid[PG_WINDOW];

   YGLLOG("PG_LINECOLOR_INSERT\n");
   //
   if (YglInitShader(PG_LINECOLOR_INSERT, pYglprg_linecol_v, pYglprg_linecol_f, 3, NULL, NULL, NULL) != 0)
     return -1;

   linecol.s_texture = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT], (const GLchar *)"s_texture");
   linecol.s_line = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT], (const GLchar *)"s_line");
   linecol.color_offset = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT], (const GLchar *)"u_color_offset");
   linecol.emu_height = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT], (const GLchar *)"u_emu_height");
   linecol.vheight = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT], (const GLchar *)"u_vheight");

   YGLLOG("PG_LINECOLOR_INSERT_DESTALPHA\n");
   if (YglInitShader(PG_LINECOLOR_INSERT_DESTALPHA, pYglprg_linecol_v, pYglprg_linecol_dest_alpha_f, 3, NULL, NULL, NULL) != 0)
     return -1;
   
   linecol_destalpha.s_texture = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA], (const GLchar *)"s_texture");
   linecol_destalpha.s_line = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA], (const GLchar *)"s_line");
   linecol_destalpha.color_offset = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA], (const GLchar *)"u_color_offset");
   linecol_destalpha.emu_height = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA], (const GLchar *)"u_emu_height");
   linecol_destalpha.vheight = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA], (const GLchar *)"u_vheight");

   YGLLOG("PG_LINECOLOR_INSERT_CRAM\n");
   if (YglInitShader(PG_LINECOLOR_INSERT_CRAM, pYglprg_linecol_v, pYglprg_linecol_cram_f, 3, NULL, NULL, NULL) != 0)
     return -1;

   linecol_cram.s_texture = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_CRAM], (const GLchar *)"s_texture");
   linecol_cram.s_color = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_CRAM], (const GLchar *)"s_color");
   linecol_cram.s_line = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_CRAM], (const GLchar *)"s_line");
   linecol_cram.color_offset = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_CRAM], (const GLchar *)"u_color_offset");
   linecol_cram.emu_height = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_CRAM], (const GLchar *)"u_emu_height");
   linecol_cram.vheight = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_CRAM], (const GLchar *)"u_vheight");

   YGLLOG("PG_LINECOLOR_INSERT_DESTALPHA_CRAM\n");
   if (YglInitShader(PG_LINECOLOR_INSERT_DESTALPHA_CRAM, pYglprg_linecol_v, pYglprg_linecol_dest_alpha_cram_f, 3, NULL, NULL, NULL) != 0)
     return -1;

   linecol_destalpha_cram.s_texture = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM], (const GLchar *)"s_texture");
   linecol_destalpha_cram.s_color = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM], (const GLchar *)"s_color");
   linecol_destalpha_cram.s_line = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM], (const GLchar *)"s_line");
   linecol_destalpha_cram.color_offset = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM], (const GLchar *)"u_color_offset");
   linecol_destalpha_cram.emu_height = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM], (const GLchar *)"u_emu_height");
   linecol_destalpha_cram.vheight = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM], (const GLchar *)"u_vheight");


   YGLLOG("PG_VDP2_DRAWFRAMEBUFF_ADDCOLOR_SHADOW\n");
   if (YglInitShader(PG_VDP2_DRAWFRAMEBUFF_ADDCOLOR_SHADOW, pYglprg_vdp2_drawfb_v, pYglprg_vdp2_drawfb_addcolor_shadow_f, 1, NULL, NULL, NULL) != 0)
     return -1;

   idvdp1FrameBuffer_addcolor_shadow = glGetUniformLocation(_prgid[PG_VDP2_DRAWFRAMEBUFF_ADDCOLOR_SHADOW], (const GLchar *)"s_vdp1FrameBuffer");;
   idfrom_addcolor_shadow = glGetUniformLocation(_prgid[PG_VDP2_DRAWFRAMEBUFF_ADDCOLOR_SHADOW], (const GLchar *)"u_from");
   idto_addcolor_shadow = glGetUniformLocation(_prgid[PG_VDP2_DRAWFRAMEBUFF_ADDCOLOR_SHADOW], (const GLchar *)"u_to");
   idcoloroffset_addcolor_shadow = glGetUniformLocation(_prgid[PG_VDP2_DRAWFRAMEBUFF_ADDCOLOR_SHADOW], (const GLchar *)"u_coloroffset");

   return 0;
}

int YglTesserationProgramInit()
{
  //-----------------------------------------------------------------------------------------------------------
    YGLLOG("PG_VFP1_GOURAUDSAHDING_TESS");
    if (YglInitShader(PG_VFP1_GOURAUDSAHDING_TESS,
      pYglprg_vdp1_gouraudshading_tess_v,
      pYglprg_vdp1_gouraudshading_f,
      1,
      pYglprg_vdp1_gouraudshading_tess_c,
      pYglprg_vdp1_gouraudshading_tess_e,
      pYglprg_vdp1_gouraudshading_tess_g) != 0)
      return -1;

    Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_GOURAUDSAHDING_TESS], &grow_tess);

    YGLLOG("PG_VFP1_GOURAUDSAHDING_SPD_TESS");
    if (YglInitShader(PG_VFP1_GOURAUDSAHDING_SPD_TESS,
      pYglprg_vdp1_gouraudshading_tess_v,
      pYglprg_vdp1_gouraudshading_spd_f,
      1,
      pYglprg_vdp1_gouraudshading_tess_c,
      pYglprg_vdp1_gouraudshading_tess_e,
      pYglprg_vdp1_gouraudshading_tess_g) != 0)
      return -1;

    Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_GOURAUDSAHDING_SPD_TESS], &grow_spd_tess);

    //---------------------------------------------------------------------------------------------------------
    YGLLOG("PG_VFP1_MESH_TESS");
    if (YglInitShader(PG_VFP1_MESH_TESS,
      pYglprg_vdp1_gouraudshading_tess_v,
      pYglprg_vdp1_mesh_f,
      1,
      pYglprg_vdp1_gouraudshading_tess_c,
      pYglprg_vdp1_gouraudshading_tess_e,
      pYglprg_vdp1_gouraudshading_tess_g) != 0)
      return -1;

    Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_MESH_TESS], &mesh_tess);


    //---------------------------------------------------------------------------------------------------------
    YGLLOG("PG_VFP1_GOURAUDSAHDING_HALFTRANS_TESS");
    if (YglInitShader(PG_VFP1_GOURAUDSAHDING_HALFTRANS_TESS,
      pYglprg_vdp1_gouraudshading_tess_v,
      pYglprg_vdp1_gouraudshading_hf_f,
      1,
      pYglprg_vdp1_gouraudshading_tess_c,
      pYglprg_vdp1_gouraudshading_tess_e,
      pYglprg_vdp1_gouraudshading_tess_g) != 0)
      return -1;

    Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_GOURAUDSAHDING_HALFTRANS_TESS], &id_ght_tess);

    //---------------------------------------------------------------------------------------------------------
    YGLLOG("PG_VFP1_SHADOW_TESS");
    if (YglInitShader(PG_VFP1_SHADOW_TESS,
      pYglprg_vdp1_gouraudshading_tess_v,
      pYglprg_vdp1_shadow_f,
      1,
      pYglprg_vdp1_gouraudshading_tess_c,
      pYglprg_vdp1_gouraudshading_tess_e,
      pYglprg_vdp1_gouraudshading_tess_g) != 0)
      return -1;

    shadow_tess.sprite = glGetUniformLocation(_prgid[PG_VFP1_SHADOW_TESS], (const GLchar *)"u_sprite");
    shadow_tess.tessLevelInner = glGetUniformLocation(_prgid[PG_VFP1_SHADOW_TESS], (const GLchar *)"TessLevelInner");
    shadow_tess.tessLevelOuter = glGetUniformLocation(_prgid[PG_VFP1_SHADOW_TESS], (const GLchar *)"TessLevelOuter");
    shadow_tess.fbo = glGetUniformLocation(_prgid[PG_VFP1_SHADOW_TESS], (const GLchar *)"u_fbo");
    shadow_tess.fbowidth = glGetUniformLocation(_prgid[PG_VFP1_SHADOW_TESS], (const GLchar *)"u_fbowidth");
    shadow_tess.fboheight = glGetUniformLocation(_prgid[PG_VFP1_SHADOW_TESS], (const GLchar *)"u_fbohegiht");

    //---------------------------------------------------------------------------------------------------------
    YGLLOG("PG_VFP1_HALFTRANS_TESS");
    if (YglInitShader(PG_VFP1_HALFTRANS_TESS,
      pYglprg_vdp1_gouraudshading_tess_v,
      pYglprg_vdp1_halftrans_f,
      1,
      pYglprg_vdp1_gouraudshading_tess_c,
      pYglprg_vdp1_gouraudshading_tess_e,
      pYglprg_vdp1_gouraudshading_tess_g) != 0)
      return -1;

  Ygl_Vdp1CommonGetUniformId(_prgid[PG_VFP1_HALFTRANS_TESS], &hf_tess);

  return 0;
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
   level->prg[level->prgcurrent].prgid=prgid;
   level->prg[level->prgcurrent].prg=_prgid[prgid];
   level->prg[level->prgcurrent].vaid = 0;
   level->prg[level->prgcurrent].id = 0;

   if (prgid == PG_NORMAL)
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
   else if (prgid == PG_VDP2_NORMAL_CRAM_SPECIAL_PRIORITY)
   {
     current->setupUniform = Ygl_uniformNormalCramSpecialPriority;
     current->cleanupUniform = Ygl_cleanupNormalCram;

     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = id_normal_cram_sp_matrix;
     current->color_offset = id_normal_cram_sp_color_offset;

   }
   else if (prgid == PG_VDP2_ADDCOLOR_CRAM)
   {
     current->setupUniform = Ygl_uniformAddColCram;
     current->cleanupUniform = Ygl_cleanupAddColCram;

     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = id_normal_cram_matrix_addcol;
     current->color_offset = id_normal_cram_color_offset_addcol;

   }
   else if (prgid == PG_VDP2_RBG_CRAM_LINE)
   {
     current->setupUniform = Ygl_uniformNormalCramLine;
     current->cleanupUniform = Ygl_cleanupNormalCramLine;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = id_rbg_cram_line_matrix;
     current->color_offset = id_rbg_cram_line_color_offset;

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
   else if (prgid == PG_VDP2_BLUR_CRAM)
   {
     current->setupUniform = Ygl_uniformNormal_blur;
     current->cleanupUniform = Ygl_cleanupNormal_blur;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = id_normal_cram_matrix;
     current->color_offset = id_normal_cram_color_offset;
   }
   else if (prgid == PG_VDP2_PER_LINE_ALPHA)
   {
     current->setupUniform = Ygl_uniformPerLineAlpha;
     current->cleanupUniform = Ygl_cleanupPerLineAlpha;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->color_offset = glGetUniformLocation(_prgid[PG_VDP2_PER_LINE_ALPHA], (const GLchar *)"u_color_offset");
     current->mtxModelView = glGetUniformLocation(_prgid[PG_VDP2_PER_LINE_ALPHA], (const GLchar *)"u_mvpMatrix");

   }
   else if (prgid == PG_VDP2_PER_LINE_ALPHA_CRAM)
   {
     current->setupUniform = Ygl_uniformPerLineAlpha;
     current->cleanupUniform = Ygl_cleanupPerLineAlpha;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->color_offset = glGetUniformLocation(_prgid[PG_VDP2_PER_LINE_ALPHA], (const GLchar *)"u_color_offset");
     current->mtxModelView = glGetUniformLocation(_prgid[PG_VDP2_PER_LINE_ALPHA], (const GLchar *)"u_mvpMatrix");

   }else if( prgid == PG_VDP1_NORMAL )
   {
      current->setupUniform    = Ygl_uniformVdp1Normal;
      current->cleanupUniform  = Ygl_cleanupVdp1Normal;
      current->vertexp = 0;
      current->texcoordp = 1;
      current->mtxModelView    = glGetUniformLocation(_prgid[PG_VDP1_NORMAL],(const GLchar *)"u_mvpMatrix");
      current->mtxTexture      = glGetUniformLocation(_prgid[PG_VDP1_NORMAL],(const GLchar *)"u_texMatrix");
      current->tex0 = glGetUniformLocation(_prgid[PG_VDP1_NORMAL], (const GLchar *)"s_texture");

   }else if( prgid == PG_VFP1_GOURAUDSAHDING )
   {
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &id_g;
     current->vertexp = 0;
     current->texcoordp = 1; 
      level->prg[level->prgcurrent].vaid = 2;
      current->mtxModelView = id_g.mtxModelView;
      current->mtxTexture = id_g.mtxTexture;
      current->tex0 = id_g.tex0;
   }
   else if (prgid == PG_VFP1_GOURAUDSAHDING_SPD)
   {
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &id_spd_g;
     current->vertexp = 0;
     current->texcoordp = 1;
     level->prg[level->prgcurrent].vaid = 2;
     current->mtxModelView = id_spd_g.mtxModelView;
     current->mtxTexture = id_spd_g.mtxTexture;
     current->tex0 = id_spd_g.tex0;
   }
   else if (prgid == PG_VFP1_GOURAUDSAHDING_TESS ){
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &grow_tess;
     current->vertexp = 0;
     current->texcoordp = 1;
     level->prg[level->prgcurrent].vaid = 2;
     current->mtxModelView = grow_tess.mtxModelView;
     current->mtxTexture = -1; // glGetUniformLocation(_prgid[PG_VFP1_GOURAUDSAHDING], (const GLchar *)"u_texMatrix");
     current->tex0 = -1; // glGetUniformLocation(_prgid[PG_VFP1_GOURAUDSAHDING], (const GLchar *)"s_texture");
   }
   else if (prgid == PG_VFP1_GOURAUDSAHDING_SPD_TESS){
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &grow_spd_tess;
     current->vertexp = 0;
     current->texcoordp = 1;
     level->prg[level->prgcurrent].vaid = 2;
     current->mtxModelView = grow_tess.mtxModelView;
     current->mtxTexture = -1; // glGetUniformLocation(_prgid[PG_VFP1_GOURAUDSAHDING], (const GLchar *)"u_texMatrix");
     current->tex0 = -1; // glGetUniformLocation(_prgid[PG_VFP1_GOURAUDSAHDING], (const GLchar *)"s_texture");
   }
   else if (prgid == PG_VFP1_STARTUSERCLIP)
   {
      level->prg[level->prgcurrent].setupUniform = Ygl_uniformStartUserClip;
      level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupStartUserClip;
      current->vertexp         = 0;
      current->texcoordp       = -1;
      current->mtxModelView    = glGetUniformLocation(_prgid[PG_WINDOW],(const GLchar *)"u_mvpMatrix");
      current->mtxTexture      = -1; //glGetUniformLocation(_prgid[PG_NORMAL],(const GLchar *)"u_texMatrix");
      
   }
   else if( prgid == PG_VFP1_ENDUSERCLIP )
   {
      level->prg[level->prgcurrent].setupUniform = Ygl_uniformEndUserClip;
      level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupEndUserClip;
      current->vertexp = 0;
      current->texcoordp = 1;
      current->mtxModelView    = glGetUniformLocation(_prgid[PG_NORMAL],(const GLchar *)"u_mvpMatrix");
      current->mtxTexture      = glGetUniformLocation(_prgid[PG_NORMAL],(const GLchar *)"u_texMatrix");
      current->tex0 = glGetUniformLocation(_prgid[PG_NORMAL], (const GLchar *)"s_texture");
   }
   else if( prgid == PG_VFP1_HALFTRANS )
   {
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
    level->prg[level->prgcurrent].ids = &hf;
      current->vertexp = 0;
      current->texcoordp = 1;
      current->mtxModelView    = glGetUniformLocation(_prgid[PG_VFP1_HALFTRANS],(const GLchar *)"u_mvpMatrix");
      current->mtxTexture      = glGetUniformLocation(_prgid[PG_VFP1_HALFTRANS],(const GLchar *)"u_texMatrix");

   }
   else if (prgid == PG_VFP1_HALFTRANS_TESS)
   {
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &hf_tess;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_VFP1_HALFTRANS_TESS], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = -1;

   }
   else if (prgid == PG_VFP1_SHADOW_TESS)
   {
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &shadow_tess;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_VFP1_SHADOW_TESS], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = -1; // glGetUniformLocation(_prgid[PG_VFP1_SHADOW], (const GLchar *)"u_texMatrix");

   }
   else if (prgid == PG_VFP1_SHADOW)
   {
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &shadow;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_VFP1_SHADOW], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = glGetUniformLocation(_prgid[PG_VFP1_SHADOW], (const GLchar *)"u_texMatrix");

   }
   else if (prgid == PG_VFP1_MESH)
   {
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &mesh;
     current->vertexp = 0;
     current->texcoordp = 1;
     level->prg[level->prgcurrent].vaid = 2;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_VFP1_MESH], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = -1;
   }else if( prgid == PG_VFP1_HALF_LUMINANCE )
   {
      current->setupUniform    = Ygl_uniformVdp1Normal;
      current->cleanupUniform  = Ygl_cleanupVdp1Normal;
      current->vertexp = 0;
      current->texcoordp = 1;
      current->mtxModelView    = glGetUniformLocation(_prgid[PG_VFP1_HALF_LUMINANCE],(const GLchar *)"u_mvpMatrix");
      current->mtxTexture      = glGetUniformLocation(_prgid[PG_VFP1_HALF_LUMINANCE],(const GLchar *)"u_texMatrix");
      current->tex0 = glGetUniformLocation(_prgid[PG_VFP1_HALF_LUMINANCE], (const GLchar *)"s_texture");
   }
   else if (prgid == PG_VFP1_MESH_TESS)
   {
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &mesh_tess;
     current->vertexp = 0;
     current->texcoordp = 1;
     level->prg[level->prgcurrent].vaid = 2;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_VFP1_MESH_TESS], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = -1;
   }
   else if (prgid == PG_VFP1_GOURAUDSAHDING_HALFTRANS)
   {
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &id_ght;
     current->vertexp = 0;
     current->texcoordp = 1;
     level->prg[level->prgcurrent].vaid = 2;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_VFP1_GOURAUDSAHDING_HALFTRANS], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = -1;
   }
   else if (prgid == PG_VFP1_GOURAUDSAHDING_HALFTRANS_TESS)
   {
     level->prg[level->prgcurrent].setupUniform = Ygl_uniformVdp1CommonParam;
     level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupVdp1CommonParam;
     level->prg[level->prgcurrent].ids = &id_ght_tess;
     current->vertexp = 0;
     current->texcoordp = 1;
     level->prg[level->prgcurrent].vaid = 2;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_VFP1_GOURAUDSAHDING_HALFTRANS_TESS], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = -1;

   }
   else if( prgid == PG_VDP2_ADDBLEND )
   {
      level->prg[level->prgcurrent].setupUniform = Ygl_uniformAddBlend;
      level->prg[level->prgcurrent].cleanupUniform = Ygl_cleanupAddBlend;
      current->vertexp = 0;
      current->texcoordp = 1;
      current->mtxModelView    = glGetUniformLocation(_prgid[PG_NORMAL],(const GLchar *)"u_mvpMatrix");
      current->mtxTexture      = glGetUniformLocation(_prgid[PG_NORMAL],(const GLchar *)"u_texMatrix");
   }
   else if (prgid == PG_LINECOLOR_INSERT)
   {
     current->setupUniform = Ygl_uniformLinecolorInsert;
     current->cleanupUniform = Ygl_cleanupLinecolorInsert;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT], (const GLchar *)"u_texMatrix");
     current->color_offset = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT], (const GLchar *)"u_color_offset");
     current->tex0 = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT], (const GLchar *)"s_texture");
   }
   else if (prgid == PG_LINECOLOR_INSERT_DESTALPHA)
   {
     current->setupUniform = Ygl_uniformLinecolorInsert;
     current->cleanupUniform = Ygl_cleanupLinecolorInsert;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA], (const GLchar *)"u_texMatrix");
     current->color_offset = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA], (const GLchar *)"u_color_offset");
     current->tex0 = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA], (const GLchar *)"s_texture");
   }
   else if (prgid == PG_LINECOLOR_INSERT_CRAM)
   {
     current->setupUniform = Ygl_uniformLinecolorInsert;
     current->cleanupUniform = Ygl_cleanupLinecolorInsert;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_CRAM], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_CRAM], (const GLchar *)"u_texMatrix");
     current->color_offset = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_CRAM], (const GLchar *)"u_color_offset");
     current->tex0 = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_CRAM], (const GLchar *)"s_texture");
   }
   else if (prgid == PG_LINECOLOR_INSERT_DESTALPHA_CRAM)
   {
     current->setupUniform = Ygl_uniformLinecolorInsert;
     current->cleanupUniform = Ygl_cleanupLinecolorInsert;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM], (const GLchar *)"u_texMatrix");
     current->color_offset = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM], (const GLchar *)"u_color_offset");
     current->tex0 = glGetUniformLocation(_prgid[PG_LINECOLOR_INSERT_DESTALPHA_CRAM], (const GLchar *)"s_texture");
   }

   else if (prgid == PG_VDP2_BLUR)
   {
     current->setupUniform = Ygl_uniformNormal_blur;
     current->cleanupUniform = Ygl_cleanupNormal_blur;
     current->vertexp = 0;
     current->texcoordp = 1;
     current->mtxModelView = glGetUniformLocation(_prgid[PG_VDP2_BLUR], (const GLchar *)"u_mvpMatrix");
     current->mtxTexture = glGetUniformLocation(_prgid[PG_VDP2_BLUR], (const GLchar *)"u_texMatrix");
     current->color_offset = glGetUniformLocation(_prgid[PG_VDP2_BLUR], (const GLchar *)"u_color_offset");
     current->tex0 = glGetUniformLocation(_prgid[PG_VDP2_BLUR], (const GLchar *)"s_texture");
   }else{
      level->prg[level->prgcurrent].setupUniform = NULL;
      level->prg[level->prgcurrent].cleanupUniform = NULL;
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
  "out vec4 fragColor;            \n"
  "void main()                                         \n"
  "{                                                   \n"
"    ivec2 linepos; \n "
"    linepos.y = 0; \n "
"    linepos.x = int( (u_vheight-gl_FragCoord.y) * u_emu_height);\n"
  "  fragColor = texelFetch( u_Clear, linepos,0 ); \n"
  "} \n";

int YglDrawBackScreen(float w, float h) {

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
  glUniform1f(glGetUniformLocation(clear_prg, "u_emu_height"), (float)_Ygl->rheight / (float)_Ygl->height);
  glUniform1f(glGetUniformLocation(clear_prg, "u_vheight"), (float)_Ygl->height);

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
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  // Clean up
  glDisableVertexAttribArray(0);

  glEnable(GL_DEPTH_TEST);

  return 0;
}

//--------------------------------------------------------------------------------------------------------------
static int vdp1_prg = -1;
static GLint vdp1MtxModelView = 0;

static const char vdp1_v[] =
      SHADER_VERSION
      "layout (location = 0) in vec2 a_position;   \n"
      "layout (location = 1) in vec2 a_texcoord;   \n"
      "out  highp vec2 v_texcoord;     \n"
      "void main()                  \n"
      "{                            \n"
      " gl_Position = vec4(a_position.x, a_position.y, 0.0, 1.0); \n"
      "   v_texcoord  = a_texcoord; \n"
      "} ";

static const char vdp1_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                            \n"
"#endif\n"
"in highp vec2 v_texcoord;                            \n"
"uniform sampler2D s_texture;                        \n"
"out vec4 fragColor;            \n"
"void main()                                         \n"
"{                                                   \n"
"  ivec2 addr; \n"
"  addr.x = int( v_texcoord.x);          \n"
"  addr.y = int(v_texcoord.y);          \n"
"  vec4 tex = texelFetch( s_texture, addr,0 );         \n"
"  if (tex.agb == vec3(0.0)) tex.ragb = vec4(0.5, 0.0, 0.0, 0.0);   \n" 
"  fragColor.r = tex.a;         \n"
"  fragColor.g = tex.b;         \n"
"  fragColor.b = tex.g;         \n"
"  fragColor.a = tex.r;         \n"
"}                                                   \n";

int YglBlitVDP1(u32 srcTexture, float w, float h, int flip) {
  const GLchar * fblit_vdp1_v[] = { vdp1_v, NULL };
  const GLchar * fblit_vdp1_f[] = { vdp1_f, NULL };

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

  if (vdp1_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;
    if (vdp1_prg != -1) glDeleteProgram(vdp1_prg);
    vdp1_prg = glCreateProgram();
    if (vdp1_prg == 0){
      return -1;
    }

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, fblit_vdp1_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      vdp1_prg = -1;
      return -1;
    }

    glShaderSource(fshader, 1, fblit_vdp1_f, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      vdp1_prg = -1;
      abort();
    }

    glAttachShader(vdp1_prg, vshader);
    glAttachShader(vdp1_prg, fshader);
    glLinkProgram(vdp1_prg);
    glGetProgramiv(vdp1_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      YGLLOG("Link error..\n");
      Ygl_printShaderError(vdp1_prg);
      vdp1_prg = -1;
      abort();
    }

    GLUSEPROG(vdp1_prg);
    glUniform1i(glGetUniformLocation(vdp1_prg, "s_texture"), 0);
  }
  else{
    GLUSEPROG(vdp1_prg);
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
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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
  "out vec4 fragColor;            \n";

static const char fblit_img[] =
  "void main()                                         \n"
  "{   \n" 
"	fragColor = Filter( u_Src, vTexCoord );                                               \n";

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

int YglBlitFramebuffer(u32 srcTexture, u32 targetFbo, float w, float h, float dispw, float disph) {
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

  glBindFramebuffer(GL_FRAMEBUFFER, targetFbo);

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
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);

  return 0;
}


const GLchar vclearb_img[] =
      SHADER_VERSION
      "layout (location = 0) in vec4 a_position;   \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "} ";

const GLchar fclearb_img[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                            \n"
"#endif\n"
"out vec4 fragColor;            \n"
"void main()                                         \n"
"{                                                   \n"
"  fragColor = vec4(0.0,0.0,0.0,1.0);\n                         "
"}                                                   \n";

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

const GLchar blur_blit_v[] =
SHADER_VERSION
"uniform mat4 u_mvpMatrix;    \n"
"layout (location = 0) in vec4 a_position;   \n"
"layout (location = 1) in vec2 a_texcoord;   \n"
"out  highp vec2 v_texcoord;     \n"
"void main()                  \n"
"{                            \n"
"   gl_Position = a_position*u_mvpMatrix; \n"
"   v_texcoord  = a_texcoord; \n"
"} ";

const GLchar blur_blit_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                            \n"
"#endif\n"
"in highp vec2 v_texcoord;                            \n"
"uniform sampler2D u_Src;                        \n"
"uniform float u_tw; \n"
"uniform float u_th; \n"
"out vec4 fragColor;            \n"
"void main()                                         \n"
"{                                                   \n"
"  ivec2 addr; \n"
"  addr.x = int(u_tw * v_texcoord.x);          \n"
"  addr.y = int(u_th) - int(u_th * v_texcoord.y);          \n"
"  vec4 txcol = texelFetch( u_Src, addr,0 ) ;      \n"
"  if(txcol.a == 0.0) discard;\n "
"  addr.x -= 1; \n"
"  vec4 txcoll = texelFetch( u_Src, addr,0 );      \n"
"  addr.x -= 1; \n"
"  vec4 txcolll = texelFetch( u_Src, addr,0 );      \n"
"  txcol.rgb = txcol.rgb/2.0 + txcoll.rgb/4.0 + txcolll.rgb/4.0; \n"
"  fragColor = txcol; \n                        "
"}                                                   \n";

static int blur_prg = -1;
static int u_blur_mtxModelView = -1;
static int u_blur_tw = -1;
static int u_blur_th = -1;

int YglBlitBlur(u32 srcTexture, u32 targetFbo, float w, float h, GLfloat* matrix) {

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


  glBindFramebuffer(GL_FRAMEBUFFER, targetFbo);

  if (blur_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;

    const GLchar * vblit_img_v[] = { blur_blit_v, NULL };
    const GLchar * fblit_img_v[] = { blur_blit_f, NULL };

    blur_prg = glCreateProgram();
    if (blur_prg == 0) return -1;

    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, vblit_img_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      blur_prg = -1;
      return -1;
    }

    glShaderSource(fshader, 1, fblit_img_v, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      blur_prg = -1;
      return -1;
    }

    glAttachShader(blur_prg, vshader);
    glAttachShader(blur_prg, fshader);
    glLinkProgram(blur_prg);
    glGetProgramiv(blur_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      YGLLOG("Link error..\n");
      Ygl_printShaderError(blur_prg);
      blur_prg = -1;
      return -1;
    }
    GLUSEPROG(blur_prg);
    glUniform1i(glGetUniformLocation(blur_prg, "u_Src"), 0);
    u_blur_mtxModelView = glGetUniformLocation(blur_prg, (const GLchar *)"u_mvpMatrix");
    u_blur_tw = glGetUniformLocation(blur_prg, "u_tw");
    u_blur_th = glGetUniformLocation(blur_prg, "u_th");

  }
  else{
    GLUSEPROG(blur_prg);
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
  glUniformMatrix4fv(u_blur_mtxModelView, 1, GL_FALSE, matrix);
  glUniform1f(u_blur_tw, w);
  glUniform1f(u_blur_th, h);


  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, srcTexture);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  // Clean up
  glActiveTexture(GL_TEXTURE0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);

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
"void main()                  \n"
"{                            \n"
"   gl_Position = a_position*u_mvpMatrix; \n"
"   v_texcoord  = a_texcoord; \n"
"} ";

const GLchar mosaic_blit_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                            \n"
"#endif\n"
"in highp vec2 v_texcoord;                            \n"
"uniform sampler2D u_Src;                        \n"
"uniform float u_tw; \n"
"uniform float u_th; \n"
"uniform ivec2 u_mosaic;			 \n"
"out vec4 fragColor;            \n"
"void main()                                         \n"
"{                                                   \n"
"  ivec2 addr; \n"
"  addr.x = int(u_tw * v_texcoord.x);          \n"
"  addr.y = int(u_th) - int(u_th * v_texcoord.y);          \n"
"  addr.x = addr.x / u_mosaic.x * u_mosaic.x;               \n"
"  addr.y = addr.y / u_mosaic.y * u_mosaic.y;               \n"
"  vec4 txcol = texelFetch( u_Src, addr,0 ) ;      \n"
"  if(txcol.a > 0.0)\n                                 "
"     fragColor = txcol; \n                        "
"  else \n                                            "
"     discard;\n                                      "
"}                                                   \n";

static int mosaic_prg = -1;
static int u_mosaic_mtxModelView = -1;
static int u_mosaic_tw = -1;
static int u_mosaic_th = -1;
static int u_mosaic = -1;

int YglBlitMosaic(u32 srcTexture, u32 targetFbo, float w, float h, GLfloat* matrix, int * mosaic) {

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


  glBindFramebuffer(GL_FRAMEBUFFER, targetFbo);

  if (mosaic_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;

    const GLchar * vblit_img_v[] = { mosaic_blit_v, NULL };
    const GLchar * fblit_img_v[] = { mosaic_blit_f, NULL };

    mosaic_prg = glCreateProgram();
    if (mosaic_prg == 0) return -1;

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



/*
  Per line transparent
  [Resident Evil] menu not displayed #35
*/

const GLchar perlinealpha_blit_v[] =
SHADER_VERSION
"uniform mat4 u_mvpMatrix;    \n"
"layout (location = 0) in vec4 a_position;   \n"
"layout (location = 1) in vec2 a_texcoord;   \n"
"out  highp vec2 v_texcoord;     \n"
"void main()                  \n"
"{                            \n"
"   gl_Position = a_position*u_mvpMatrix; \n"
"   v_texcoord  = a_texcoord; \n"
"} ";

const GLchar perlinealpha_blit_f[] =
SHADER_VERSION
"#ifdef GL_ES\n"
"precision highp float;                            \n"
"#endif\n"
"in highp vec2 v_texcoord;                            \n"
"uniform sampler2D u_Src;                        \n"
"uniform sampler2D u_Line;                        \n"
"uniform float u_tw; \n"
"uniform float u_th; \n"
"out vec4 fragColor;            \n"
"void main()                                         \n"
"{                                                   \n"
"  ivec2 addr; \n"
"  addr.x = int(u_tw * v_texcoord.x);          \n"
"  addr.y = int(u_th) - int(u_th * v_texcoord.y);          \n"
"  vec4 txcol = texelFetch( u_Src, addr,0 ) ;      \n"
"  if(txcol.a > 0.0){\n                                 "
"    addr.x = int(u_th * v_texcoord.y);\n"
"    addr.y = 0; \n"
"    txcol.a = texelFetch( u_Line, addr,0 ).a;      \n"
"    txcol.r += (texelFetch( u_Line, addr,0 ).r-0.5)*2.0;\n"
"    txcol.g += (texelFetch( u_Line, addr,0 ).g-0.5)*2.0;\n"
"    txcol.b += (texelFetch( u_Line, addr,0 ).b-0.5)*2.0;\n"
"    if( txcol.a > 0.0 ) \n"
"       fragColor = txcol; \n                        "
"    else \n"
"       discard; \n                        "
"  }else{ \n"
"    discard; \n"
"  }\n                                            "
"}                                                   \n";

static int perlinealpha_prg = -1;
static int u_perlinealpha_mtxModelView = -1;
static int u_perlinealpha_tw = -1;
static int u_perlinealpha_th = -1;


int YglBlitPerLineAlpha(u32 srcTexture, u32 targetFbo, float w, float h, GLfloat* matrix, u32 lineTexture) {

  float vb[] = { 0, 0,
    2.0, 0.0,
    2.0, 2.0,
    0, 2.0, };

  float tb[] = { 0.0, 0.0,
    1.0, 0.0,
    1.0, 1.0,
    0.0, 1.0 };

  GLint programid;
  int id_src, id_line;

  vb[0] = 0;
  vb[1] = 0 - 1.0;
  vb[2] = w;
  vb[3] = 0 - 1.0;
  vb[4] = w;
  vb[5] = h - 1.0;
  vb[6] = 0;
  vb[7] = h - 1.0;

  glGetIntegerv(GL_CURRENT_PROGRAM, &programid);

  glBindFramebuffer(GL_FRAMEBUFFER, targetFbo);

  if (perlinealpha_prg == -1){
    GLuint vshader;
    GLuint fshader;
    GLint compiled, linked;

    const GLchar * vblit_img_v[] = { perlinealpha_blit_v, NULL };
    const GLchar * fblit_img_v[] = { perlinealpha_blit_f, NULL };

    perlinealpha_prg = glCreateProgram();
    if (perlinealpha_prg == 0) return -1;


    vshader = glCreateShader(GL_VERTEX_SHADER);
    fshader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vshader, 1, vblit_img_v, NULL);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in vertex shader.\n");
      Ygl_printShaderError(vshader);
      perlinealpha_prg = -1;
      return -1;
    }

    glShaderSource(fshader, 1, fblit_img_v, NULL);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
      YGLLOG("Compile error in fragment shader.\n");
      Ygl_printShaderError(fshader);
      perlinealpha_prg = -1;
      return -1;
    }

    glAttachShader(perlinealpha_prg, vshader);
    glAttachShader(perlinealpha_prg, fshader);
    glLinkProgram(perlinealpha_prg);
    glGetProgramiv(perlinealpha_prg, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      YGLLOG("Link error..\n");
      Ygl_printShaderError(perlinealpha_prg);
      perlinealpha_prg = -1;
      return -1;
    }
    GLUSEPROG(perlinealpha_prg);
    id_src = glGetUniformLocation(perlinealpha_prg, "u_Src");
    glUniform1i(id_src, 0);
    id_line = glGetUniformLocation(perlinealpha_prg, "u_Line");
    glUniform1i(id_line, 1);

    u_perlinealpha_mtxModelView = glGetUniformLocation(perlinealpha_prg, (const GLchar *)"u_mvpMatrix");
    u_perlinealpha_tw = glGetUniformLocation(perlinealpha_prg, "u_tw");
    u_perlinealpha_th = glGetUniformLocation(perlinealpha_prg, "u_th");

  }
  else{
    GLUSEPROG(perlinealpha_prg);
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
  glUniformMatrix4fv(u_perlinealpha_mtxModelView, 1, GL_FALSE, matrix);
  glUniform1f(u_perlinealpha_tw, w);
  glUniform1f(u_perlinealpha_th, h);

  id_src = glGetUniformLocation(perlinealpha_prg, "u_Src");
  glUniform1i(id_src, 0);
  id_line = glGetUniformLocation(perlinealpha_prg, "u_Line");
  glUniform1i(id_line, 1);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, srcTexture);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, lineTexture);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  // Clean up
  glActiveTexture(GL_TEXTURE0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  GLUSEPROG(programid);

  return 0;
}
