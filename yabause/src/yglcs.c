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
extern int YglGenerateWindowCCBuffer();
extern int YglGenerateScreenBuffer();
extern void YglUpdateVdp2Reg();
extern void YglSetVdp2Window(Vdp2 *varVdp2Regs);
extern void YglSetCCWindow(Vdp2 *varVdp2Regs);
extern SpriteMode setupBlend(Vdp2 *varVdp2Regs, int layer);
extern int setupColorMode(Vdp2 *varVdp2Regs, int layer);
extern int setupShadow(Vdp2 *varVdp2Regs, int layer);
extern int setupBlur(Vdp2 *varVdp2Regs, int layer);
extern int YglDrawBackScreen();
extern u32 COLOR16TO24(u16 temp);

static void releaseVDP1DrawingFBMem(int id);

static u16 COLOR24TO16(u32 temp) {
  if (((temp >> 31)&0x1) == 0) return 0x0000;
  if (((temp >> 30)&0x1) == 0) {
    return 0x8000 | ((u32)(temp >> 3)& 0x1F) | ((u32)(temp >> 6)& 0x3E0) | ((u32)(temp >> 9)& 0x7C00);
  }
  else {
    return (temp & 0xFFFF);
  }
}

static u32 VDP1MSB(u16 temp) {
  return (((u32)temp & 0x7FFF) | ((u32)temp & 0x8000) << 1);
}

//////////////////////////////////////////////////////////////////////////////
void YglEraseWriteCSVDP1(void) {
  //REvoir le vdp1_clear
  float col[4];
  u16 color;
  int priority;
  u32 alpha = 0;
  int status = 0;
  if (_Ygl->vdp1On[_Ygl->readframe] == 0) return; //No update on the fb, no need to clear
  _Ygl->vdp1On[_Ygl->readframe] = 0;

  memset(_Ygl->vdp1fb_exactbuf[_Ygl->readframe], 0x0, 512*704*2);
  _Ygl->vdp1IsNotEmpty[_Ygl->readframe] = 0;

  _Ygl->vdp1_stencil_mode = 0;

  color = Vdp1Regs->EWDR;
  priority = 0;

  if ((color & 0x8000) && (Vdp2Regs->SPCTL & 0x20)) {
    alpha = 0;
  }
  else{
    int rgb = ((color&0x8000) == 0);
    int shadow, normalshadow, colorcalc;
    Vdp1ProcessSpritePixel(Vdp2Regs->SPCTL & 0xF, &color, &shadow, &normalshadow, &priority, &colorcalc);
//    alpha = VDP1COLOR(rgb, colorcalc, priority, 0, 0);
//on doit utiliser simplement color partout
    alpha >>= 24;
  }
  col[0] = (color & 0x1F) / 31.0f;
  col[1] = ((color >> 5) & 0x1F) / 31.0f;
  col[2] = ((color >> 10) & 0x1F) / 31.0f;
  col[3] = alpha / 255.0f;

  FRAMELOG("YglEraseWriteVDP1xx: clear %d\n", _Ygl->readframe);
  //Get back to drawframe
  vdp1_clear(_Ygl->readframe, col);

  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
}

//////////////////////////////////////////////////////////////////////////////

void YglCSRenderVDP1(void) {
  FRAMELOG("YglCSRenderVDP1: drawframe =%d", _Ygl->drawframe);
  releaseVDP1DrawingFBMem(_Ygl->drawframe);
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

static void YglSetVDP1FB(int i){
  if (_Ygl->vdp1IsNotEmpty[i] != 0) {
    _Ygl->vdp1On[i] = 1;
    vdp1_set_directFB(i);
    _Ygl->vdp1IsNotEmpty[i] = 0;
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
   YglMatrix mtx;
   YglMatrix dmtx;
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
   glViewport(0, 0, _Ygl->rwidth, _Ygl->rheight);
   glGetIntegerv( GL_VIEWPORT, _Ygl->m_viewport );
   glScissor(0, 0, _Ygl->rwidth, _Ygl->rheight);
   glEnable(GL_SCISSOR_TEST);

   //glClearBufferfv(GL_COLOR, 0, colopaque);
   //glClearBufferfi(GL_DEPTH_STENCIL, 0, 0, 0);
   if ((Vdp2Regs->TVMD & 0x8000) == 0) goto render_finish;
   if (YglTM_vdp2 == NULL) goto render_finish;
   glBindTexture(GL_TEXTURE_2D, YglTM_vdp2->textureID);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

   YglUpdateVdp2Reg();
   YglSetVdp2Window(varVdp2Regs);
   YglSetCCWindow(varVdp2Regs);
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
      id++;
    }
  }
  isBlur[6] = setupBlur(varVdp2Regs, SPRITE);
  lncl_draw[6] = lncl[6];
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


  //Ici si use_win[SPRITE] est a 1, il faut faire un blit simple avec test stencil
  if (_Ygl->use_win[SPRITE] == 1) {
    glViewport(0, 0, _Ygl->width/_Ygl->vdp1wratio, _Ygl->height/_Ygl->vdp1hratio);
    glScissor(0, 0, _Ygl->width/_Ygl->vdp1wratio, _Ygl->height/_Ygl->vdp1hratio);

    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->vdp1fbowin);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 0, 0);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);

    YglBlitSimple(_Ygl->window_fbotex[SPRITE], 0);

    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);

    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_EQUAL, 1, 0xFF);

    glDrawBuffers(1, &DrawBuffers[0]);
    glClearBufferfv(GL_COLOR, 0, col);
    YglBlitSimple(_Ygl->vdp1Tex[0], 0);
    glDrawBuffers(1, &DrawBuffers[1]);
    glClearBufferfv(GL_COLOR, 0, col);
    YglBlitSimple(_Ygl->vdp1Tex[1], 0); //VDP1 CS need to handle additional texture
    glDisable(GL_STENCIL_TEST);
    VDP1fb = &_Ygl->vdp1FrameBuff[0];

    glViewport(0, 0, _Ygl->width, _Ygl->height);
    glScissor(0, 0, _Ygl->width, _Ygl->height);
  } else {
    VDP1fb = _Ygl->vdp1Tex;
  }

  if (_Ygl->vdp2_use_compute_shader == 0) {
    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->original_fbo);
    glDrawBuffers(NB_RENDER_LAYER, &DrawBuffers[0]);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 0, 0);
    YglBlitTexture( _Ygl->bg, prioscreens, modescreens, isRGB, isBlur, isShadow, lncl_draw, VDP1fb, varVdp2Regs);
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

static void releaseVDP1DrawingFBMem(int id) {
  _Ygl->vdp1fb_buf[id] = NULL;
}

static u32* getVdp1DrawingFBMem(Vdp2 *varVdp2Regs, int id) {
  //Ici le read doit etre different du write. Il faut faire un pack dans le cas du read... et un glReadPixel
  u32* fbptr = NULL;
  GLuint error;
  YglGenFrameBuffer();
  //vdp1_get_directFB(&Vdp2Lines[0], _Ygl->drawframe);
  releaseVDP1DrawingFBMem(id);
  fbptr = vdp1_get_directFB(varVdp2Regs, id);
  return fbptr;
}

void YglCSVdp1WriteFrameBuffer(u32 type, u32 addr, u32 val ) {
  u8 priority = Vdp2Regs->PRISA &0x7;
  int ispalette = !((val & 0x8000) && (Vdp2Regs->SPCTL & 0x20));
  u16 full = 0;
  u16 cc = 0;
  int shadow = 0;
  int normalshadow = 0;
  if (_Ygl->vdp1fb_buf[_Ygl->drawframe] == NULL) {
    _Ygl->vdp1fb_buf[_Ygl->drawframe] =  getVdp1DrawingFBMem(&Vdp2Lines[0], _Ygl->drawframe);
  }
  //Est ce qu'il ne faura pas voir si la prio ne change pas par pixel???
  u32 x = 0;
  u32 y = 0;
  int tvmode = (Vdp1Regs->TVMR & 0x7);
  switch( tvmode ) {
    case 0: // 16bit 512x256
    case 2: // 16bit 512x256
    case 4: // 16bit 512x256
      y = (addr >> 10)&0xFF;
      x = (addr & 0x3FF) >> 1;
      break;
    case 1: // 8bit 1024x256
      y = (addr >> 10) & 0xFF;
      x = addr & 0x3FF;
      break;
    case 3: // 8bit 512x512
      y = (addr >> 9) & 0x1FF;
      x = addr & 0x1FF;
      break;
    default:
      y = 0;
      x = 0;
      break;
  }
  // switch (type)
  // {
  // case 0:
  //   T1WriteByte((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe], addr, val);
  //   full = T1ReadWord((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe],addr&(~0x1));
  //   ispalette = !((full & 0x8000) && (Vdp2Regs->SPCTL & 0x20));
  //   if (!ispalette) {
  //     T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], (addr&(~0x1))*2, VDP1COLOR(ispalette, 0, priority, 0, COLOR16TO24(full&0xFFFF)));
  //   }
  //   else{
  //     Vdp1ProcessSpritePixel(Vdp2Regs->SPCTL & 0xF, &full, &shadow, &normalshadow, &priority, &cc);
  //     T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], (addr&(~0x1))*2, VDP1COLOR(ispalette, cc, priority, 0, VDP1MSB(full&0xFFFF)));
  //   }
  //   break;
  // case 1:
  //   T1WriteWord((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe], addr, val);
  //   if (ispalette) {
  //     Vdp1ProcessSpritePixel(Vdp2Regs->SPCTL & 0xF, &val, &shadow, &normalshadow, &priority, &cc);
  //     T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2, VDP1COLOR(ispalette, cc, priority, 0, VDP1MSB(val&0xFFFF)));
  //   }
  //   else
  //     T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2, VDP1COLOR(ispalette, 0, priority, 0, COLOR16TO24(val&0xFFFF)));
  //   break;
  // case 2:
  //   T1WriteLong((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe], addr, val);
  //   if (!ispalette) {
  //     T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2+4, VDP1COLOR(ispalette, 0, priority, 0, COLOR16TO24(val&0xFFFF)));
  //   }
  //   else{
  //     u16 temp = (val & 0xFFFF);
  //     Vdp1ProcessSpritePixel(Vdp2Regs->SPCTL & 0xF, &temp, &shadow, &normalshadow, &priority, &cc);
  //     T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2+4, VDP1COLOR(ispalette, cc, priority, 0, VDP1MSB(temp&0xFFFF)));
  //   }
  //   ispalette = !(((val>>16) & 0x8000) && (Vdp2Regs->SPCTL & 0x20));
  //   if (!ispalette) {
  //     T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2, VDP1COLOR(ispalette, 0, priority, 0, COLOR16TO24((val>>16)&0xFFFF)));
  //   }
  //   else{
  //     u16 temp = (val>>16);
  //     Vdp1ProcessSpritePixel(Vdp2Regs->SPCTL & 0xF, &temp, &shadow, &normalshadow, &priority, &cc);
  //     T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2, VDP1COLOR(ispalette, cc, priority, 0, VDP1MSB(temp&0xFFFF)));
  //   }
  //   break;
  // default:
  //   break;
  // }
  if (val != 0) {
    _Ygl->vdp1IsNotEmpty[_Ygl->drawframe] = 1;
  }
}

u16 getVdp1PixelColor(u32 col){
  int istransparent = ((col>>24) & 0x80)==0;
  int ispalette = ((col>>24) & 0x40)!=0;
  int priority = (col>>24) & 0x7;
  int cc = (col>>27) & 0x7;
  if (istransparent) return 0x0;
  if (!ispalette) {
    return COLOR24TO16(col);
  }
  else{
    int idx = (col & 0x7FFF) | ((col >> 1)&0x8000);
    int type = (col >> 17) & 0xF;
    switch(type)
    {
       case 0x0:
       {
          // Type 0(2-bit priority, 3-bit color calculation, 11-bit color data)
          u16 ret = (priority & 0x3)<<14 | (cc & 0x7)<<11 | (idx & 0x7FF);
          return ret;
       }
       case 0x1:
       {
          // Type 1(3-bit priority, 2-bit color calculation, 11-bit color data)
          return (priority & 0x7)<<13 | (cc & 0x3)<<11 | (idx & 0x7FF);
       }
       case 0x2:
       {
          // Type 2(1-bit shadow, 1-bit priority, 3-bit color calculation, 11-bit color data)
          return (priority & 0x1)<<14 | (cc & 0x7)<<11 | (idx & 0x7FF);
       }
       case 0x3:
       {
          // Type 3(1-bit shadow, 2-bit priority, 2-bit color calculation, 11-bit color data)
          return (priority & 0x3)<<13 | (cc & 0x3)<<11 | (idx & 0x7FF);
       }
       case 0x4:
       {
          // Type 4(1-bit shadow, 2-bit priority, 3-bit color calculation, 10-bit color data)
          return (priority & 0x3)<<13 | (cc & 0x7)<<10 | (idx & 0x3FF);
       }
       case 0x5:
       {
          // Type 5(1-bit shadow, 3-bit priority, 1-bit color calculation, 11-bit color data)
          return (priority & 0x7)<<12 | (cc & 0x1)<<11 | (idx & 0x7FF);
       }
       case 0x6:
       {
          // Type 6(1-bit shadow, 3-bit priority, 2-bit color calculation, 10-bit color data)
          return (priority & 0x7)<<12 | (cc & 0x3)<<10 | (idx & 0x3FF);
       }
       case 0x7:
       {
          // Type 7(1-bit shadow, 3-bit priority, 3-bit color calculation, 9-bit color data)
          return (priority & 0x7)<<12 | (cc & 0x7)<<9 | (idx & 0x1FF);
       }
       case 0x8:
       {
          // Type 8(1-bit priority, 7-bit color data)
          return (priority & 0x1)<<7 | (idx & 0x7F);
       }
       case 0x9:
       {
          // Type 9(1-bit priority, 1-bit color calculation, 6-bit color data)
          return (priority & 0x1)<<7 | (cc & 0x1)<<6 | (idx & 0x3F);
       }
       case 0xA:
       {
          // Type A(2-bit priority, 6-bit color data)
          return (priority & 0x3)<<6 | (idx & 0x3F);
       }
       case 0xB:
       {
          // Type B(2-bit color calculation, 6-bit color data)
          return (cc & 0x3)<<6 | (idx & 0x3F);
       }
       case 0xC:
       {
          // Type C(1-bit special priority, 8-bit color data - bit 7 is shared)
          return (priority & 0x1)<<7 | (idx & 0xFF);
       }
       case 0xD:
       {
          // Type D(1-bit special priority, 1-bit special color calculation, 8-bit color data - bits 6 and 7 are shared)
          return (priority & 0x3)<<7 | (cc & 0x1)<<6 | (idx & 0xFF);
       }
       case 0xE:
       {
          // Type E(2-bit special priority, 8-bit color data - bits 6 and 7 are shared)
          return (priority & 0x3)<<6 | (idx & 0xFF);
       }
       case 0xF:
       {
          // Type F(2-bit special color calculation, 8-bit color data - bits 6 and 7 are shared)
          return (cc & 0x3)<<6 | (idx & 0xFF);
       }
       default:
          return 0;
      }
    }
  return 0;
}

void YglCSVdp1ReadFrameBuffer(u32 type, u32 addr, void * out) {
  if (_Ygl->vdp1fb_buf[_Ygl->drawframe] == NULL) {
    _Ygl->vdp1fb_buf[_Ygl->drawframe] =  getVdp1DrawingFBMem(&Vdp2Lines[0], _Ygl->drawframe);
  }
    u32 x = 0;
    u32 y = 0;
    int tvmode = (Vdp1Regs->TVMR & 0x7);
    switch( tvmode ) {
      case 0: // 16bit 512x256
      case 2: // 16bit 512x256
      case 4: // 16bit 512x256
        y = (addr >> 10)&0xFF;
        x = (addr & 0x3FF) >> 1;
        break;
      case 1: // 8bit 1024x256
        y = (addr >> 10) & 0xFF;
        x = addr & 0x3FF;
        break;
      case 3: // 8bit 512x512
        y = (addr >> 9) & 0x1FF;
        x = addr & 0x1FF;
        break;
      default:
        y = 0;
        x = 0;
        break;
    }
    if ((x<_Ygl->rwidth) && (y<_Ygl->rheight)) {
      switch (type)
      {
      case 0:
        *(u8*)out = 0x0;
        break;
      case 1:
        *(u16*)out = getVdp1PixelColor(T1ReadLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2));
        break;
      case 2:
        *(u32*)out = (getVdp1PixelColor(T1ReadLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2))<<16)|(getVdp1PixelColor(T1ReadLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2+4)));
        break;
      default:
        break;
      }
    } else {
      switch (type)
      {
      case 0:
        *(u8*)out = T1ReadByte((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe], addr);
        break;
      case 1:
        *(u16*)out = T1ReadWord((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe], addr);
        break;
      case 2:
        *(u32*)out = T1ReadLong((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe], addr);
        break;
      default:
        break;
      }
    }
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
    glDeleteTextures(2,_Ygl->vdp1FrameBuff);
    _Ygl->vdp1FrameBuff[0] = 0;
  }
  glGenTextures(2, _Ygl->vdp1FrameBuff);

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

  _Ygl->pFrameBuffer = NULL;

  if (_Ygl->vdp1_pbo[0] == 0) {
    GLuint error;
    glGenTextures(2, _Ygl->vdp1AccessTex);
    glGenBuffers(2, _Ygl->vdp1_pbo);
    YGLDEBUG("glGenBuffers %d %d\n",_Ygl->vdp1_pbo[0], _Ygl->vdp1_pbo[1]);

    glBindTexture(GL_TEXTURE_2D, _Ygl->vdp1AccessTex[0]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 512, 256);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _Ygl->vdp1_pbo[0]);
    glBufferData(GL_PIXEL_PACK_BUFFER, 0x40000*2, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, _Ygl->vdp1AccessTex[1]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 512, 256);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _Ygl->vdp1_pbo[1]);
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
  YglGenerateWindowCCBuffer();
  YglGenerateScreenBuffer();

  YGLDEBUG("YglGenFrameBuffer OK\n");
  glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
  glBindTexture(GL_TEXTURE_2D, 0);
  rebuild_frame_buffer = 0;
  rebuild_windows = 1;
  return 0;
}
