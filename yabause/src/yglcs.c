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

extern int GlHeight;
extern int GlWidth;

extern int DrawVDP2Screen(Vdp2 *varVdp2Regs, int id);

static void YglSetVDP1FB(int i);

//////////////////////////////////////////////////////////////////////////////
void YglEraseWriteCSVDP1(void) {
  if (_Ygl->vdp1FrameBuff[0] != 0) vdp1_clear(_Ygl->readframe);
  YglEraseWriteVDP1();
}

//////////////////////////////////////////////////////////////////////////////

void YglCSRenderVDP1(void) {
  FrameProfileAdd("YglCSRenderVDP1 start");
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

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);

  if (_Ygl->vdp2_use_compute_shader != 0)
    VDP2Generator_init(_Ygl->width, _Ygl->height);

#if 0
   if ( (varVdp2Regs->CCCTL & 0x400) != 0 ) {
     printf("Extended Color calculation!\n");
   }
   printf("Ram mode %d\n", Vdp2Internal.ColorMode);
#endif
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
      lncl_draw[id] = lncl[vdp2screens[j]];
      id++;
    }
  }
  isBlur[6] = setupBlur(varVdp2Regs, SPRITE);
  lncl_draw[6] = lncl[6];

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
    VDP1fb = &_Ygl->vdp1FrameBuff[4];

    glViewport(0, 0, _Ygl->width, _Ygl->height);
    glScissor(0, 0, _Ygl->width, _Ygl->height);
  } else {
    VDP1fb = _Ygl->vdp1Tex;
  }

  if (_Ygl->vdp2_use_compute_shader == 0) {
    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->original_fbo);
    glDrawBuffers(NB_RENDER_LAYER, &DrawBuffers[0]);
    glClearBufferfi(GL_DEPTH_STENCIL, 0, 0, 0);
    YglBlitTexture( _Ygl->bg, prioscreens, modescreens, isRGB, isBlur, lncl_draw, VDP1fb, varVdp2Regs);
    srcTexture = _Ygl->original_fbotex[0];
  } else {
    VDP2Generator_update(_Ygl->compute_tex, _Ygl->bg, prioscreens, modescreens, isRGB, isBlur, lncl_draw, VDP1fb, varVdp2Regs);
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

static void releaseVDP1FB(int i) {
  //ne faire que la mise a jour de ce qui est ecrit.
  //Ne pas ecrire au dela de l'adresse...
  //Faire un read du FB avant un write pour recuperer le contenu.
  GLenum DrawBuffers[4]= {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3};
  if (_Ygl->vdp1FrameBuff[i*2] != 0) {
    if (_Ygl->vdp1fb_buf[i] != NULL) {
      glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->vdp1fbo);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, _Ygl->vdp1AccessTex[i]);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _Ygl->vdp1_pbo[i]);
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 512, 256, GL_RGBA, GL_UNSIGNED_BYTE, 0);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      _Ygl->vdp1fb_buf[i] = NULL;
      YglSetVDP1FB(i);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }
}

static void releaseVDP1DrawingFBMemRead(int id) {
  if (_Ygl->vdp1fb_buf_read[id] == NULL) return;
  glBindBuffer(GL_PIXEL_PACK_BUFFER, _Ygl->vdp1_pbo[id]);
  glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
  _Ygl->vdp1fb_buf_read[id] = NULL;
}

static void YglSetVDP1FB(int i){
  GLenum DrawBuffers[4]= {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3};
  if (_Ygl->vdp1IsNotEmpty[i] != 0) {
    _Ygl->vdp1On[i] = 1;
    YglGenFrameBuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->vdp1fbo);
    glDrawBuffers(2, &DrawBuffers[i*2]);
    releaseVDP1FB(i);
    releaseVDP1DrawingFBMemRead(i);
    glViewport(0,0, _Ygl->width, _Ygl->height);
    YglBlitVDP1(_Ygl->vdp1AccessTex[i], _Ygl->rwidth, _Ygl->rheight, 1);
    // clean up
    glBindFramebuffer(GL_FRAMEBUFFER, _Ygl->default_fbo);
    _Ygl->vdp1IsNotEmpty[i] = 0;
  }
}

void YglCSVdp1WriteFrameBuffer(u32 type, u32 addr, u32 val ) {
  u8 priority = Vdp2Regs->PRISA &0x7;
  int rgb = !((val>>15)&0x1);
  u16 full = 0;
  if (_Ygl->vdp1fb_buf[_Ygl->drawframe] == NULL) {
    _Ygl->vdp1fb_buf[_Ygl->drawframe] =  getVdp1DrawingFBMemWrite(_Ygl->drawframe);
  }

  switch (type)
  {
  case 0:
    T1WriteByte((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe], addr, val);
    full = T1ReadWord((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe],addr&(~0x1));
    rgb = !((full>>15)&0x1);
    T1WriteLong(_Ygl->vdp1fb_buf[_Ygl->drawframe], (addr&(~0x1))*2, VDP1COLOR(rgb, 0, priority, 0, COLOR16TO24(full&0xFFFF)));
    break;
  case 1:
    T1WriteWord((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe], addr, val);
    T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2, VDP1COLOR(rgb, 0, priority, 0, COLOR16TO24(val&0xFFFF)));
    break;
  case 2:
    T1WriteLong((u8*)_Ygl->vdp1fb_exactbuf[_Ygl->drawframe], addr, val);
    T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2+4, VDP1COLOR(rgb, 0, priority, 0, COLOR16TO24(val&0xFFFF)));
    rgb = !(((val>>16)>>15)&0x1);
    T1WriteLong((u8*)_Ygl->vdp1fb_buf[_Ygl->drawframe], addr*2, VDP1COLOR(rgb, 0, priority, 0, COLOR16TO24((val>>16)&0xFFFF)));
    break;
  default:
    break;
  }
  if (val != 0) {
    _Ygl->vdp1IsNotEmpty[_Ygl->drawframe] = 1;
  }
}

void YglCSVdp1ReadFrameBuffer(u32 type, u32 addr, void * out) {
  //releaseVDP1DrawingFBMemRead(_Ygl->drawframe);
    if (_Ygl->vdp1fb_buf_read[_Ygl->drawframe] == NULL) {
      if(_Ygl->vdp1fb_buf[_Ygl->drawframe] != NULL) {
        releaseVDP1FB(_Ygl->drawframe);
      }
      _Ygl->vdp1IsNotEmpty[_Ygl->drawframe] = 0;
      _Ygl->vdp1fb_buf_read[_Ygl->drawframe] =  getVdp1DrawingFBMemRead(_Ygl->drawframe);
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
      defalut:
        y = 0;
        x = 0;
        break;
    }
    if ((x<=_Ygl->rwidth) && (y<=_Ygl->rheight)) {
      switch (type)
      {
      case 0:
        *(u8*)out = 0x0;
        break;
      case 1:
        *(u16*)out = COLOR24TO16(T1ReadLong((u8*)_Ygl->vdp1fb_buf_read[_Ygl->drawframe], addr*2));
        break;
      case 2:
        *(u32*)out = (COLOR24TO16(T1ReadLong((u8*)_Ygl->vdp1fb_buf_read[_Ygl->drawframe], addr*2))<<16)|(COLOR24TO16(T1ReadLong((u8*)_Ygl->vdp1fb_buf_read[_Ygl->drawframe], addr*2+4)));
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
