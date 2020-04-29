/*  Copyright 2003-2006 Guillaume Duhamel
    Copyright 2004 Lawrence Sebald
    Copyright 2004-2007 Theo Berkau

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

/*! \file vidcs.c
    \brief OpenGL video renderer
*/
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)

#include <math.h>
#define EPSILON (1e-10 )


#include "vidcs.h"
#include "vidshared.h"
#include "debug.h"
#include "vdp2.h"
#include "yabause.h"
#include "ygl.h"
#include "yui.h"
#include "vdp1/vdp1_compute.h"
#include "frameprofile.h"
#ifdef SPRITE_CACHE
#include "patternManager.h"
#endif

#define Y_MAX(a, b) ((a) > (b) ? (a) : (b))
#define Y_MIN(a, b) ((a) < (b) ? (a) : (b))


#define DEBUG_BAD_COORD //YuiMsg

#define  CONVERTCMD(A) {\
  s32 toto = (A);\
  if (((A)&0x7000) != 0) (A) |= 0xF000;\
  else (A) &= ~0xF800;\
  ((A) = (s32)(s16)(A));\
  if (((A)) < -1024) { DEBUG_BAD_COORD("Bad(-1024) %x (%d, 0x%x)\n", (A), (A), toto);}\
  if (((A)) > 1023) { DEBUG_BAD_COORD("Bad(1023) %x (%d, 0x%x)\n", (A), (A), toto);}\
}

#define CLAMP(A,LOW,HIGH) ((A)<(LOW)?(LOW):((A)>(HIGH))?(HIGH):(A))

#define LOG_AREA

#define LOG_CMD

static int VIDCS_renderer_started = 0;
static Vdp2 baseVdp2Regs;
//#define PERFRAME_LOG
#ifdef PERFRAME_LOG
int fount = 0;
FILE *ppfp = NULL;YglGenFrameBuffer
#endif

#define YGL_THREAD_DEBUG
//#define YGL_THREAD_DEBUG yprintf


#define COLOR_ADDt(b)      (b>0xFF?0xFF:(b<0?0:b))
#define COLOR_ADDb(b1,b2)   COLOR_ADDt((signed) (b1) + (b2))
#ifdef WORDS_BIGENDIAN
#define COLOR_ADD(l,r,g,b)   (COLOR_ADDb((l >> 24) & 0xFF, r) << 24) | \
            (COLOR_ADDb((l >> 16) & 0xFF, g) << 16) | \
            (COLOR_ADDb((l >> 8) & 0xFF, b) << 8) | \
            (l & 0xFF)
#else
#define COLOR_ADD(l,r,g,b)   COLOR_ADDb((l & 0xFF), r) | \
            (COLOR_ADDb((l >> 8 ) & 0xFF, g) << 8) | \
            (COLOR_ADDb((l >> 16 ) & 0xFF, b) << 16) | \
            (l & 0xFF000000)
#endif

#define WA_INSIDE (0)
#define WA_OUTSIDE (1)

extern int VIDOGLInit(void);
extern void VIDOGLDeInit(void);
extern void VIDOGLResize(int, int, unsigned int, unsigned int, int);
extern int VIDOGLIsFullscreen(void);
extern int VIDOGLVdp1Reset(void);
extern void VIDOGLVdp1Draw();

extern vdp2rotationparameter_struct  Vdp1ParaA;

void VIDCSVdp1Draw();
void VIDCSVdp1NormalSpriteDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1ScaledSpriteDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1DistortedSpriteDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1PolygonDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1PolylineDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1LineDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1UserClipping(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs);
void VIDCSVdp1SystemClipping(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs);
extern void YglCSRender(Vdp2 *varVdp2Regs);
extern void YglCSRenderVDP1(void);
extern void YglCSFinsihDraw(void);

extern void VIDOGLVdp1LocalCoordinate(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs);
extern int VIDOGLVdp2Reset(void);
extern void VIDOGLVdp2Draw(void);
extern void VIDOGLVdp2SetResolution(u16 TVMD);
extern void YglGetGlSize(int *width, int *height);
extern void VIDOGLGetNativeResolution(int *width, int *height, int*interlace);
extern void YglCSVdp1ReadFrameBuffer(u32 type, u32 addr, void * out);
extern void YglCSVdp1WriteFrameBuffer(u32 type, u32 addr, u32 val);
extern void VIDOGLSetSettingValueMode(int type, int value);
extern void VIDOGLSync();
extern void VIDOGLGetNativeResolution(int *width, int *height, int*interlace);
extern void VIDOGLVdp2DispOff(void);
extern int YglGenFrameBuffer(int force);
extern void vdp1GenerateBuffer(vdp1cmd_struct* cmd);

extern u32 FASTCALL Vdp1ReadPolygonColor(vdp1cmd_struct *cmd, Vdp2* varVdp2Regs);

VideoInterface_struct VIDCS = {
VIDCORE_CS,
"Compute Shader Video Interface",
VIDOGLInit,
VIDOGLDeInit,
VIDOGLResize,
VIDOGLIsFullscreen,
VIDOGLVdp1Reset,
VIDCSVdp1Draw,
VIDCSVdp1NormalSpriteDraw,
VIDCSVdp1ScaledSpriteDraw,
VIDCSVdp1DistortedSpriteDraw,
VIDCSVdp1PolygonDraw,
VIDCSVdp1PolylineDraw,
VIDCSVdp1LineDraw,
VIDCSVdp1UserClipping,
VIDCSVdp1SystemClipping,
VIDOGLVdp1LocalCoordinate,
YglCSVdp1ReadFrameBuffer,
YglCSVdp1WriteFrameBuffer,
YglEraseWriteCSVDP1,
YglFrameChangeCSVDP1,
vdp1GenerateBuffer,
VIDOGLVdp2Reset,
VIDOGLVdp2Draw,
YglGetGlSize,
VIDOGLSetSettingValueMode,
VIDOGLSync,
VIDOGLGetNativeResolution,
VIDOGLVdp2DispOff,
YglCSRender,
YglCSRenderVDP1,
YglGenFrameBuffer,
YglCSFinsihDraw
};

void addCSCommands(vdp1cmd_struct* cmd, int type)
{
  //Test game: Sega rally : The aileron at the start
  int Ax = (cmd->CMDXD - cmd->CMDXA);
  int Ay = (cmd->CMDYD - cmd->CMDYA);
  int Bx = (cmd->CMDXC - cmd->CMDXB);
  int By = (cmd->CMDYC - cmd->CMDYB);
  int nbStep = 0;
  unsigned int lA;
  unsigned int lB;

  lA = ceil(sqrt(Ax*Ax+Ay*Ay));
  lB = ceil(sqrt(Bx*Bx+By*By));

  cmd->uAstepx = 0.0;
  cmd->uAstepy = 0.0;
  cmd->uBstepx = 0.0;
  cmd->uBstepy = 0.0;
  cmd->nbStep = 1;
  cmd->type = type;

  nbStep = lA;
  if (lB >= lA)
    nbStep = lB;

  if(nbStep != 0) {
    cmd->nbStep = nbStep + 1;
    cmd->uAstepx = (float)Ax/(float)nbStep;
    cmd->uAstepy = (float)Ay/(float)nbStep;
    cmd->uBstepx = (float)Bx/(float)nbStep;
    cmd->uBstepy = (float)By/(float)nbStep;
  }
#ifdef DEBUG_VDP1_CMD
  printf("%d %f [%d %d][%d %d][%d %d][%d %d]\n", cmd->nbStep, (float)cmd->h / (float)cmd->nbStep,
    cmd->CMDXA, cmd->CMDYA,
    cmd->CMDXB, cmd->CMDYB,
    cmd->CMDXC, cmd->CMDYC,
    cmd->CMDXD, cmd->CMDYD
  );
#endif
  vdp1_add(cmd,0);
}

//////////////////////////////////////////////////////////////////////////////
void VIDCSVdp1Draw()
{
  VIDOGLVdp1Draw();
}

//////////////////////////////////////////////////////////////////////////////

void VIDCSVdp1NormalSpriteDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  LOG_CMD("%d\n", __LINE__);

  if (((cmd->CMDPMOD >> 3) & 0x7u) == 5) {
    // hard/vdp2/hon/p09_20.htm#no9_21
    u32 *cclist = (u32 *)&(Vdp2Lines[0].CCRSA);
    cclist[0] &= 0x1Fu;
  }
  cmd->SPCTL = Vdp2Lines[0].SPCTL;
  cmd->type = QUAD;

  vdp1_add(cmd,0);

  LOG_CMD("%d\n", __LINE__);
}

//////////////////////////////////////////////////////////////////////////////

void VIDCSVdp1ScaledSpriteDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{

  if (((cmd->CMDPMOD >> 3) & 0x7u) == 5) {
    // hard/vdp2/hon/p09_20.htm#no9_21
    u32 *cclist = (u32 *)&(Vdp2Lines[0].CCRSA);
    cclist[0] &= 0x1Fu;
  }

  cmd->SPCTL = Vdp2Lines[0].SPCTL;
  cmd->type = QUAD;
  vdp1_add(cmd,0);

  LOG_CMD("%d\n", __LINE__);
}

int getBestMode(vdp1cmd_struct* cmd) {
  int ret = DISTORTED;
  if (
    ((cmd->CMDXA - cmd->CMDXD) == 0) &&
    ((cmd->CMDYA - cmd->CMDYB) == 0) &&
    ((cmd->CMDXB - cmd->CMDXC) == 0) &&
    ((cmd->CMDYC - cmd->CMDYD) == 0)
  ) {
    ret = QUAD;
  }
  return ret;
}
void VIDCSVdp1DistortedSpriteDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  LOG_CMD("%d\n", __LINE__);

  if (((cmd->CMDPMOD >> 3) & 0x7u) == 5) {
    // hard/vdp2/hon/p09_20.htm#no9_21
    u32 *cclist = (u32 *)&(Vdp2Lines[0].CCRSA);
    cclist[0] &= 0x1Fu;
  }
  //gouraud

  cmd->SPCTL = Vdp2Lines[0].SPCTL;

  if (getBestMode(cmd) == DISTORTED) {
    addCSCommands(cmd,DISTORTED);
  } else {
    cmd->type = QUAD;
    vdp1_add(cmd,0);
  }

  return;
}

void VIDCSVdp1PolygonDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  cmd->SPCTL = Vdp2Lines[0].SPCTL;
  // cmd->type = POLYGON;
  cmd->COLOR[0] = Vdp1ReadPolygonColor(cmd,&Vdp2Lines[0]);
  if (getBestMode(cmd) == DISTORTED) {
    addCSCommands(cmd,POLYGON);
  } else {
    cmd->type = QUAD_POLY;
    vdp1_add(cmd,0);
  }
  return;
}

void VIDCSVdp1PolylineDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  LOG_CMD("%d\n", __LINE__);

  cmd->SPCTL = Vdp2Lines[0].SPCTL;
  cmd->COLOR[0] = Vdp1ReadPolygonColor(cmd,&Vdp2Lines[0]);
  cmd->type = POLYLINE;

  vdp1_add(cmd,0);
}

//////////////////////////////////////////////////////////////////////////////

void VIDCSVdp1LineDraw(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  LOG_CMD("%d\n", __LINE__);

  cmd->SPCTL = Vdp2Lines[0].SPCTL;
  cmd->type = LINE;
  cmd->COLOR[0] = Vdp1ReadPolygonColor(cmd,&Vdp2Lines[0]);

  vdp1_add(cmd,0);
}

//////////////////////////////////////////////////////////////////////////////

void VIDCSVdp1UserClipping(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs)
{
  if (
    (regs->userclipX1 == cmd->CMDXA) &&
    (regs->userclipY1 == cmd->CMDYA) &&
    (regs->userclipX2 == cmd->CMDXC) &&
    (regs->userclipY2 == cmd->CMDYC)
  ) return;

  cmd->type = USER_CLIPPING;
  vdp1_add(cmd,1);
  regs->userclipX1 = cmd->CMDXA;
  regs->userclipY1 = cmd->CMDYA;
  regs->userclipX2 = cmd->CMDXC+1;
  regs->userclipY2 = cmd->CMDYC+1;
}

//////////////////////////////////////////////////////////////////////////////

void VIDCSVdp1SystemClipping(vdp1cmd_struct *cmd, u8 * ram, Vdp1 * regs)
{
  if (((cmd->CMDXC+1) == regs->systemclipX2) && (regs->systemclipY2 == (cmd->CMDYC+1))) return;
  cmd->type = SYSTEM_CLIPPING;
  vdp1_add(cmd,1);
  regs->systemclipX2 = cmd->CMDXC+1;
  regs->systemclipY2 = cmd->CMDYC+1;
}

#endif
