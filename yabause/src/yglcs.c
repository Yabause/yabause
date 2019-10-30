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


#include <stdlib.h>
#include <math.h>
#include "ygl.h"
#include "yui.h"
#include "vidshared.h"
#include "debug.h"
#include "frameprofile.h"
#include "error.h"
#include "vdp1/vdp1_compute.h"

#define YGLDEBUG

extern int GlHeight;
extern int GlWidth;

extern int rebuild_frame_buffer;
extern int rebuild_windows;

extern int DrawVDP2Screen(Vdp2 *varVdp2Regs, int id);

static int YglGenFrameBuffer();
extern int YglGenerateBackBuffer();
extern int YglGenerateWindowBuffer();
extern int YglGenerateScreenBuffer();
extern void YglUpdateVdp2Reg();
extern void YglSetVdp2Window(Vdp2 *varVdp2Regs);
extern SpriteMode setupBlend(Vdp2 *varVdp2Regs, int layer);
extern int setupColorMode(Vdp2 *varVdp2Regs, int layer);
extern int setupShadow(Vdp2 *varVdp2Regs, int layer);
extern int setupBlur(Vdp2 *varVdp2Regs, int layer);
extern int YglDrawBackScreen();

//////////////////////////////////////////////////////////////////////////////
void YglEraseWriteCSVDP1(void) {

  float col[4] = {0.0};
  u16 color;
  int priority;
  u32 alpha = 0;
  int status = 0;
  GLenum DrawBuffers[2]= {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1};
  _Ygl->vdp1On[_Ygl->readframe] = 0;
  if (_Ygl->vdp1FrameBuff[0] == 0) return;

  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->vdp1fbo);
  glDrawBuffers(1, &DrawBuffers[_Ygl->readframe]);

  _Ygl->vdp1_stencil_mode = 0;

  color = Vdp1Regs->EWDR;

  col[0] = (color & 0xFF) / 255.0f;
  col[1] = ((color >> 8) & 0xFF) / 255.0f;

  glClearBufferfv(GL_COLOR, 0, col);
  glClearBufferfi(GL_DEPTH_STENCIL, 0, 0, 0);
  FRAMELOG("YglEraseWriteVDP1xx: clear %d\n", _Ygl->readframe);
  //Get back to drawframe
  vdp1_clear(_Ygl->readframe, col);

  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);

}

//////////////////////////////////////////////////////////////////////////////

void YglCSRenderVDP1(void) {
  FRAMELOG("YglCSRenderVDP1: drawframe =%d", _Ygl->drawframe);
  _Ygl->vdp1Tex = vdp1_compute(&Vdp2Lines[0], _Ygl->drawframe);
  FrameProfileAdd("YglCSRenderVDP1 end");
}

void YglFrameChangeCSVDP1(){
  u32 current_drawframe = 0;
  YglCSRenderVDP1();
  current_drawframe = _Ygl->drawframe;
  _Ygl->drawframe = _Ygl->readframe;
  _Ygl->readframe = current_drawframe;

  FRAMELOG("YglFrameChangeVDP1: swap drawframe =%d readframe = %d\n", _Ygl->drawframe, _Ygl->readframe);
}

extern int WinS[enBGMAX+1];
extern int WinS_mode[enBGMAX+1];

static void YglSetVDP1FB(int i){
  if (_Ygl->vdp1IsNotEmpty != 0) {
    _Ygl->vdp1On[i] = 1;
    vdp1_set_directFB(i);
    _Ygl->vdp1IsNotEmpty = 0;
  }
}

static void YglUpdateVDP1FB(void) {
  YglSetVDP1FB(_Ygl->readframe);
}

void YglCSRender(Vdp2 *varVdp2Regs) {

   GLuint cprg=0;
   GLuint srcTexture;
   GLuint *VDP1fb;
   int nbPass = 0;
   unsigned int i,j;
   double w = 0;
   double h = 0;
   double x = 0;
   double y = 0;
   float col[4] = {0.0f,0.0f,0.0f,0.0f};
   float colopaque[4] = {0.0f,0.0f,0.0f,1.0f};
   int img[6] = {0};
   int lncl[7] = {0};
   int lncl_draw[7] = {0};
   int winS_draw[enBGMAX+1] = {0};
   int winS_mode_draw[enBGMAX+1] = {0};
   int win0_draw[enBGMAX+1] = {0};
   int win0_mode_draw[enBGMAX+1] = {0};
   int win1_draw[enBGMAX+1] = {0};
   int win1_mode_draw[enBGMAX+1] = {0};
   int win_op_draw[enBGMAX+1] = {0};
   int drawScreen[enBGMAX];
   SpriteMode mode;
   GLenum DrawBuffers[8]= {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3,GL_COLOR_ATTACHMENT4,GL_COLOR_ATTACHMENT5,GL_COLOR_ATTACHMENT6,GL_COLOR_ATTACHMENT7};

   YglUpdateVDP1FB();

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);

  if (_Ygl->vdp2_use_compute_shader != 0)
    VDP2Generator_init(_Ygl->width, _Ygl->height);

   glBindVertexArray(_Ygl->vao);

   if (_Ygl->stretch == 0) {
     double dar = (double)GlWidth/(double)GlHeight;
     double par = 4.0/3.0;

     if (yabsys.isRotated) par = 1.0/par;

     w = (dar>par)?(double)GlHeight*par:GlWidth;
     h = (dar>par)?(double)GlHeight:(double)GlWidth/par;
     x = (GlWidth-w)/2;
     y = (GlHeight-h)/2;
   } else {
     w = GlWidth;
     h = GlHeight;
     x = 0;
     y = 0;
   }

   glViewport(0, 0, GlWidth, GlHeight);

   FrameProfileAdd("YglRender start");
   glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
   glClearBufferfv(GL_COLOR, 0, col);

   YglGenFrameBuffer();

  if (_Ygl->vdp2_use_compute_shader == 0) {
    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->original_fbo);
    glDrawBuffers(NB_RENDER_LAYER, &DrawBuffers[0]);
    //glClearBufferfv(GL_COLOR, 0, col);
#ifdef DEBUG_BLIT
    //glClearBufferfv(GL_COLOR, 1, col);
    //glClearBufferfv(GL_COLOR, 2, col);
    //glClearBufferfv(GL_COLOR, 3, col);
    //glClearBufferfv(GL_COLOR, 4, col);
#endif
  }
   glDepthMask(GL_FALSE);
   glViewport(0, 0, _Ygl->width, _Ygl->height);
   glGetIntegerv( GL_VIEWPORT, _Ygl->m_viewport );
   glScissor(0, 0, _Ygl->width, _Ygl->height);
   glEnable(GL_SCISSOR_TEST);

   //glClearBufferfv(GL_COLOR, 0, colopaque);
   //glClearBufferfi(GL_DEPTH_STENCIL, 0, 0, 0);
   if ((Vdp2Regs->TVMD & 0x8000) == 0) goto render_finish;
   if (YglTM_vdp2 == NULL) goto render_finish;
   glBindTexture(GL_TEXTURE_2D, YglTM_vdp2->textureID);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

   YglUpdateVdp2Reg();
   YglSetVdp2Window(varVdp2Regs);
   cprg = -1;

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, YglTM_vdp2->textureID);

  int min = 8;
  int oldPrio = 0;

  int nbPrio = 0;
  int minPrio = -1;
  int allPrio = 0;

  for (int i = 0; i < SPRITE; i++) {
    if (((i == RBG0) || (i == RBG1)) && (_Ygl->rbg_use_compute_shader)) {
      glViewport(0, 0, _Ygl->width, _Ygl->height);
      glScissor(0, 0, _Ygl->width, _Ygl->height);
      glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->rbg_compute_fbo);
      if ( i == RBG0)
        glDrawBuffers(1, &DrawBuffers[0]);
      else
        glDrawBuffers(1, &DrawBuffers[1]);
    } else {
      glViewport(0, 0, _Ygl->rwidth, _Ygl->rheight);
      glScissor(0, 0, _Ygl->rwidth, _Ygl->rheight);
      glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->screen_fbo);
      glDrawBuffers(1, &DrawBuffers[i]);
    }
    drawScreen[i] = DrawVDP2Screen(varVdp2Regs, i);
  }

  const int vdp2screens[] = {RBG0, RBG1, NBG0, NBG1, NBG2, NBG3};

  int prioscreens[6] = {0};
  int modescreens[7];
  int isRGB[6];
  int isBlur[7];
  int isShadow[7];
  glDisable(GL_BLEND);
  int id = 0;

  lncl[0] = (varVdp2Regs->LNCLEN >> 0)&0x1; //NBG0
  lncl[1] = (varVdp2Regs->LNCLEN >> 1)&0x1; //NBG1
  lncl[2] = (varVdp2Regs->LNCLEN >> 2)&0x1; //NBG2
  lncl[3] = (varVdp2Regs->LNCLEN >> 3)&0x1; //NBG3
  lncl[4] = (varVdp2Regs->LNCLEN >> 4)&0x1; //RBG0
  lncl[5] = (varVdp2Regs->LNCLEN >> 0)&0x1; //RBG1
  lncl[6] = (varVdp2Regs->LNCLEN >> 5)&0x1; //SPRITE

  for (int j=0; j<6; j++) {
    if (drawScreen[vdp2screens[j]] != 0) {
      if (((vdp2screens[j] == RBG0) ||(vdp2screens[j] == RBG1)) && (_Ygl->rbg_use_compute_shader)) {
        if (vdp2screens[j] == RBG0)
        prioscreens[id] = _Ygl->rbg_compute_fbotex[0];
        else
        prioscreens[id] = _Ygl->rbg_compute_fbotex[1];
      } else {
        prioscreens[id] = _Ygl->screen_fbotex[vdp2screens[j]];
      }
      modescreens[id] =  setupBlend(varVdp2Regs, vdp2screens[j]);
      isRGB[id] = setupColorMode(varVdp2Regs, vdp2screens[j]);
      isBlur[id] = setupBlur(varVdp2Regs, vdp2screens[j]);
      isShadow[id] = setupShadow(varVdp2Regs, vdp2screens[j]);
      lncl_draw[id] = lncl[vdp2screens[j]];
      winS_draw[id] = WinS[vdp2screens[j]];
      winS_mode_draw[id] = WinS_mode[vdp2screens[j]];
      win0_draw[id] = _Ygl->Win0[vdp2screens[j]];
      win0_mode_draw[id] = _Ygl->Win0_mode[vdp2screens[j]];
      win1_draw[id] = _Ygl->Win1[vdp2screens[j]];
      win1_mode_draw[id] = _Ygl->Win1_mode[vdp2screens[j]];
      win_op_draw[id] = _Ygl->Win_op[vdp2screens[j]];
      id++;
    }
  }
  isBlur[6] = setupBlur(varVdp2Regs, SPRITE);
  lncl_draw[6] = lncl[6];

  for (int i = 6; i < 8; i++) {
    //Update dedicated sprite window and Color calculation window
    winS_draw[i] = WinS[i];
    winS_mode_draw[i] = WinS_mode[i];
    win0_draw[i] = _Ygl->Win0[i];
    win0_mode_draw[i] = _Ygl->Win0_mode[i];
    win1_draw[i] = _Ygl->Win1[i];
    win1_mode_draw[i] = _Ygl->Win1_mode[i];
    win_op_draw[i] = _Ygl->Win_op[i];
  }

  isShadow[6] = setupShadow(varVdp2Regs, SPRITE); //Use sprite index for background suuport

  glViewport(0, 0, _Ygl->width, _Ygl->height);
  glGetIntegerv( GL_VIEWPORT, _Ygl->m_viewport );
  glScissor(0, 0, _Ygl->width, _Ygl->height);

  modescreens[6] =  setupBlend(varVdp2Regs, 6);
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->back_fbo);
  glDrawBuffers(1, &DrawBuffers[0]);
  //glClearBufferfv(GL_COLOR, 0, col);
  if ((varVdp2Regs->BKTAU & 0x8000) != 0) {
    YglDrawBackScreen();
  }else{
    glClearBufferfv(GL_COLOR, 0, _Ygl->clear);
  }

  VDP1fb = _Ygl->vdp1Tex;

  if (_Ygl->vdp2_use_compute_shader == 0) {
    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->original_fbo);
    glDrawBuffers(NB_RENDER_LAYER, &DrawBuffers[0]);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 0, 0);
    YglBlitTexture( _Ygl->bg, prioscreens, modescreens, isRGB, isBlur, isShadow, lncl_draw, VDP1fb, winS_draw, winS_mode_draw, win0_draw, win0_mode_draw, win1_draw, win1_mode_draw, win_op_draw, varVdp2Regs);
    srcTexture = _Ygl->original_fbotex[0];
  } else {
    VDP2Generator_update(_Ygl->compute_tex, _Ygl->bg, prioscreens, modescreens, isRGB, isBlur, isShadow, lncl_draw, VDP1fb, varVdp2Regs);
    srcTexture = _Ygl->compute_tex;
  }
   glViewport(x, y, w, h);
   glScissor(x, y, w, h);
   glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
   YglBlitFramebuffer(srcTexture, _Ygl->width, _Ygl->height, w, h);

render_finish:

  for (int i=0; i<SPRITE; i++)
    YglReset(_Ygl->vdp2levels[i]);
  glViewport(_Ygl->originx, _Ygl->originy, GlWidth, GlHeight);
  glUseProgram(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(2);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);
  glDisable(GL_STENCIL_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  OSDDisplayMessages(NULL,0,0);

  _Ygl->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE,0);
  FrameProfileAdd("YglRender end");
  return;
}


//////////////////////////////////////////////////////////////////////////////
static int YglGenFrameBuffer() {
  int status;
  GLuint error;
  float col[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  if (rebuild_frame_buffer == 0){
    return 0;
  }

  vdp1_compute_init(_Ygl->rwidth, _Ygl->rheight, (float)(_Ygl->width)/(float)(_Ygl->rwidth), (float)(_Ygl->height)/(float)(_Ygl->rheight));

  if (_Ygl->upfbo != 0){
    glDeleteFramebuffers(1, &_Ygl->upfbo);
    _Ygl->upfbo = 0;
    glDeleteTextures(1, &_Ygl->upfbotex);
    _Ygl->upfbotex = 0;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  if (_Ygl->vdp1FrameBuff[0] != 0) {
    glDeleteTextures(3,_Ygl->vdp1FrameBuff);
    _Ygl->vdp1FrameBuff[0] = 0;
  }
  glGenTextures(3, _Ygl->vdp1FrameBuff);

  glBindTexture(GL_TEXTURE_2D, _Ygl->vdp1FrameBuff[0]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _Ygl->width/_Ygl->vdp1wratio, _Ygl->height/_Ygl->vdp1hratio, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, _Ygl->vdp1FrameBuff[1]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _Ygl->width/_Ygl->vdp1wratio, _Ygl->height/_Ygl->vdp1hratio, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, _Ygl->vdp1FrameBuff[2]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _Ygl->width/_Ygl->vdp1wratio, _Ygl->height/_Ygl->vdp1hratio, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  _Ygl->pFrameBuffer = NULL;

  if (_Ygl->vdp1_pbo == 0) {
    GLuint error;
    glGenTextures(1, &_Ygl->vdp1AccessTex);
    glGenBuffers(1, &_Ygl->vdp1_pbo);
    YGLDEBUG("glGenBuffers %d\n",_Ygl->vdp1_pbo);

    glBindTexture(GL_TEXTURE_2D, _Ygl->vdp1AccessTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 512, 256);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _Ygl->vdp1_pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, 0x40000*2, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glGenFramebuffers(1, &_Ygl->vdp1AccessFB);
  }

  if (_Ygl->rboid_depth_win != 0) glDeleteRenderbuffers(1, &_Ygl->rboid_depth_win);
  glGenRenderbuffers(1, &_Ygl->rboid_depth_win);
  glBindRenderbuffer(GL_RENDERBUFFER, _Ygl->rboid_depth_win);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _Ygl->width/_Ygl->vdp1wratio, _Ygl->height/_Ygl->vdp1hratio);

  if (_Ygl->vdp1fbowin != 0)
    glDeleteFramebuffers(1, &_Ygl->vdp1fbowin);

  glGenFramebuffers(1, &_Ygl->vdp1fbowin);
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->vdp1fbowin);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _Ygl->vdp1FrameBuff[0], 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _Ygl->vdp1FrameBuff[1], 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _Ygl->rboid_depth_win);
  status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    YGLDEBUG("YglGenFrameBuffer:Framebuffer line %d status = %08X\n", __LINE__, status);
    abort();
  }

  if (_Ygl->vdp2_use_compute_shader == 1) {
    YglGenerateComputeBuffer();
  } else {
    YglGenerateOriginalBuffer();
  }

  YglGenerateBackBuffer();
  YglGenerateWindowBuffer();
  YglGenerateScreenBuffer();

  YGLDEBUG("YglGenFrameBuffer OK\n");
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
  glBindTexture(GL_TEXTURE_2D, 0);
  rebuild_frame_buffer = 0;
  rebuild_windows = 1;
  return 0;
}
