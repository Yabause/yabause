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
FILE *ppfp = NULL;
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

void VIDCSVdp1Draw();
void VIDCSVdp1NormalSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1ScaledSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1DistortedSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1PolygonDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1PolylineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1LineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDCSVdp1UserClipping(u8 * ram, Vdp1 * regs);
void VIDCSVdp1SystemClipping(u8 * ram, Vdp1 * regs);
void YglCSRender(Vdp2 *varVdp2Regs);

extern void VIDOGLVdp1LocalCoordinate(u8 * ram, Vdp1 * regs);
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
VIDOGLVdp2Reset,
VIDOGLVdp2Draw,
YglGetGlSize,
VIDOGLSetSettingValueMode,
VIDOGLSync,
VIDOGLGetNativeResolution,
VIDOGLVdp2DispOff,
YglCSRender
};

//////////////////////////////////////////////////////////////////////////////
void VIDCSVdp1Draw()
{
  VIDOGLVdp1Draw();
  vdp1_setup();

}

//////////////////////////////////////////////////////////////////////////////

void VIDCSVdp1NormalSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  LOG_CMD("%d\n", __LINE__);

  vdp1cmd_struct cmd;
  YglTexture texture;
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
  if ((cmd.CMDSIZE & 0x8000)) {
    regs->EDSR |= 2;
    return; // BAD Command
  }

  cmd.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
  cmd.h = cmd.CMDSIZE & 0xFF;
  cmd.cor = 0;
  cmd.cog = 0;
  cmd.cob = 0;

  cmd.flip = (cmd.CMDCTRL & 0x30) >> 4;

  CONVERTCMD(cmd.CMDXA);
  CONVERTCMD(cmd.CMDYA);

  cmd.CMDXA += Vdp1Regs->localX;
  cmd.CMDYA += Vdp1Regs->localY;

  if (cmd.w == 0 || cmd.h == 0) {
    return; //bad command
  }

  cmd.CMDXB = cmd.CMDXA + cmd.w;
  cmd.CMDYB = cmd.CMDYA;
  cmd.CMDXC = cmd.CMDXA + cmd.w;
  cmd.CMDYC = cmd.CMDYA + cmd.h;
  cmd.CMDXD = cmd.CMDXA;
  cmd.CMDYD = cmd.CMDYA + cmd.h;

  if ((cmd.CMDPMOD >> 3) & 0x7u == 5) {
    // hard/vdp2/hon/p09_20.htm#no9_21
    u32 *cclist = (u32 *)&varVdp2Regs->CCRSA;
    cclist[0] &= 0x1Fu;
  }
  //gouraud
  memset(cmd.G, 0, sizeof(float)*16);
  if ((cmd.CMDPMOD & 4))
  {
    for (int i = 0; i < 4; i++){
      u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
      cmd.G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      cmd.G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      cmd.G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
    }
  }
  cmd.priority = 0;
  cmd.SPCTL = varVdp2Regs->SPCTL;
  cmd.type = NORMAL;

  vdp1_add(&cmd,0);

  LOG_CMD("%d\n", __LINE__);
}

//////////////////////////////////////////////////////////////////////////////

void VIDCSVdp1ScaledSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  vdp1cmd_struct cmd;
  YglTexture texture;
  s16 rw = 0, rh = 0;
  s16 x, y;
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
  if (cmd.CMDSIZE == 0) {
    return; // BAD Command
  }

  cmd.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
  cmd.h = cmd.CMDSIZE & 0xFF;
  cmd.cor = 0;
  cmd.cog = 0;
  cmd.cob = 0;

  cmd.flip = (cmd.CMDCTRL & 0x30) >> 4;

  CONVERTCMD(cmd.CMDXA);
  CONVERTCMD(cmd.CMDYA);
  CONVERTCMD(cmd.CMDXB);
  CONVERTCMD(cmd.CMDYB);
  CONVERTCMD(cmd.CMDXC);
  CONVERTCMD(cmd.CMDYC);

  x = cmd.CMDXA;
  y = cmd.CMDYA;
  // Setup Zoom Point
  switch ((cmd.CMDCTRL & 0xF00) >> 8)
  {
  case 0x0: // Only two coordinates
    rw = cmd.CMDXC - cmd.CMDXA;
    rh = cmd.CMDYC - cmd.CMDYA;
    break;
  case 0x5: // Upper-left
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    break;
  case 0x6: // Upper-Center
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw / 2;
    break;
  case 0x7: // Upper-Right
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw;
    break;
  case 0x9: // Center-left
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    y = y - rh / 2;
    break;
  case 0xA: // Center-center
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw / 2;
    y = y - rh / 2;
    break;
  case 0xB: // Center-right
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw;
    y = y - rh / 2;
    break;
  case 0xD: // Lower-left
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    y = y - rh;
    break;
  case 0xE: // Lower-center
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw / 2;
    y = y - rh;
    break;
  case 0xF: // Lower-right
    rw = cmd.CMDXB;
    rh = cmd.CMDYB;
    x = x - rw;
    y = y - rh;
    break;
  default: break;
  }

  cmd.CMDXA = x + Vdp1Regs->localX;
  cmd.CMDYA = y + Vdp1Regs->localY;
  cmd.CMDXB = x + rw  + Vdp1Regs->localX;
  cmd.CMDYB = y + Vdp1Regs->localY;
  cmd.CMDXC = x + rw  + Vdp1Regs->localX;
  cmd.CMDYC = y + rh + Vdp1Regs->localY;
  cmd.CMDXD = x + Vdp1Regs->localX;
  cmd.CMDYD = y + rh + Vdp1Regs->localY;

  if ((cmd.CMDPMOD >> 3) & 0x7u == 5) {
    // hard/vdp2/hon/p09_20.htm#no9_21
    u32 *cclist = (u32 *)&varVdp2Regs->CCRSA;
    cclist[0] &= 0x1Fu;
  }

//gouraud
memset(cmd.G, 0, sizeof(float)*16);
if ((cmd.CMDPMOD & 4))
{
  for (int i = 0; i < 4; i++){
    u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
    cmd.G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
    cmd.G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
    cmd.G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
  }
}
  cmd.priority = 0;
  cmd.SPCTL = varVdp2Regs->SPCTL;
  cmd.type = SCALED;
  vdp1_add(&cmd,0);

  LOG_CMD("%d\n", __LINE__);
}

void VIDCSVdp1DistortedSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  LOG_CMD("%d\n", __LINE__);

  vdp1cmd_struct cmd;
  YglTexture texture;
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
  if (cmd.CMDSIZE == 0) {
    return; // BAD Command
  }

  cmd.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
  cmd.h = cmd.CMDSIZE & 0xFF;
  cmd.cor = 0;
  cmd.cog = 0;
  cmd.cob = 0;

  cmd.flip = (cmd.CMDCTRL & 0x30) >> 4;

  CONVERTCMD(cmd.CMDXA);
  CONVERTCMD(cmd.CMDYA);
  CONVERTCMD(cmd.CMDXB);
  CONVERTCMD(cmd.CMDYB);
  CONVERTCMD(cmd.CMDXC);
  CONVERTCMD(cmd.CMDYC);
  CONVERTCMD(cmd.CMDXD);
  CONVERTCMD(cmd.CMDYD);

  cmd.CMDXA += Vdp1Regs->localX;
  cmd.CMDYA += Vdp1Regs->localY;
  cmd.CMDXB += Vdp1Regs->localX;
  cmd.CMDYB += Vdp1Regs->localY;
  cmd.CMDXC += Vdp1Regs->localX;
  cmd.CMDYC += Vdp1Regs->localY;
  cmd.CMDXD += Vdp1Regs->localX;
  cmd.CMDYD += Vdp1Regs->localY;

  if ((cmd.CMDPMOD >> 3) & 0x7u == 5) {
    // hard/vdp2/hon/p09_20.htm#no9_21
    u32 *cclist = (u32 *)&varVdp2Regs->CCRSA;
    cclist[0] &= 0x1Fu;
  }

//gouraud
memset(cmd.G, 0, sizeof(float)*16);
if ((cmd.CMDPMOD & 4))
{
  for (int i = 0; i < 4; i++){
    u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
    cmd.G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
    cmd.G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
    cmd.G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
  }
}
  cmd.priority = 0;
  cmd.SPCTL = varVdp2Regs->SPCTL;
  cmd.type = DISTORTED;
  vdp1_add(&cmd,0);
  return;
}

void VIDCSVdp1PolygonDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  vdp1cmd_struct cmd;
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);

  CONVERTCMD(cmd.CMDXA);
  CONVERTCMD(cmd.CMDYA);
  CONVERTCMD(cmd.CMDXB);
  CONVERTCMD(cmd.CMDYB);
  CONVERTCMD(cmd.CMDXC);
  CONVERTCMD(cmd.CMDYC);
  CONVERTCMD(cmd.CMDXD);
  CONVERTCMD(cmd.CMDYD);

  cmd.CMDXA += Vdp1Regs->localX;
  cmd.CMDYA += Vdp1Regs->localY;
  cmd.CMDXB += Vdp1Regs->localX;
  cmd.CMDYB += Vdp1Regs->localY;
  cmd.CMDXC += Vdp1Regs->localX;
  cmd.CMDYC += Vdp1Regs->localY;
  cmd.CMDXD += Vdp1Regs->localX;
  cmd.CMDYD += Vdp1Regs->localY;

  //gouraud
  memset(cmd.G, 0, sizeof(float)*16);
  if ((cmd.CMDPMOD & 4))
  {
    for (int i = 0; i < 4; i++){
      u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
      cmd.G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
      cmd.G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
      cmd.G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
    }
  }
  cmd.priority = 0;
  cmd.w = 1;
  cmd.h = 1;
  cmd.flip = 0;
  cmd.cor = 0x0;
  cmd.cog = 0x0;
  cmd.cob = 0x0;
  cmd.SPCTL = varVdp2Regs->SPCTL;
  cmd.type = POLYGON;
  cmd.COLOR[0] = Vdp1ReadPolygonColor(&cmd,varVdp2Regs);

  vdp1_add(&cmd,0);
}

void VIDCSVdp1PolylineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  LOG_CMD("%d\n", __LINE__);

  vdp1cmd_struct cmd;
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);

  CONVERTCMD(cmd.CMDXA);
  CONVERTCMD(cmd.CMDYA);
  CONVERTCMD(cmd.CMDXB);
  CONVERTCMD(cmd.CMDYB);
  CONVERTCMD(cmd.CMDXC);
  CONVERTCMD(cmd.CMDYC);
  CONVERTCMD(cmd.CMDXD);
  CONVERTCMD(cmd.CMDYD);

  cmd.CMDXA += Vdp1Regs->localX;
  cmd.CMDYA += Vdp1Regs->localY;
  cmd.CMDXB += Vdp1Regs->localX;
  cmd.CMDYB += Vdp1Regs->localY;
  cmd.CMDXC += Vdp1Regs->localX;
  cmd.CMDYC += Vdp1Regs->localY;
  cmd.CMDXD += Vdp1Regs->localX;
  cmd.CMDYD += Vdp1Regs->localY;

//gouraud
memset(cmd.G, 0, sizeof(float)*16);
if ((cmd.CMDPMOD & 4))
{
  for (int i = 0; i < 4; i++){
    u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
    cmd.G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
    cmd.G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
    cmd.G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
  }
}
  cmd.priority = 0;
  cmd.w = 1;
  cmd.h = 1;
  cmd.flip = 0;
  cmd.cor = 0x0;
  cmd.cog = 0x0;
  cmd.cob = 0x0;
  cmd.SPCTL = varVdp2Regs->SPCTL;
  cmd.type = POLYLINE;
  cmd.COLOR[0] = Vdp1ReadPolygonColor(&cmd,varVdp2Regs);

  vdp1_add(&cmd,0);
}

//////////////////////////////////////////////////////////////////////////////

void VIDCSVdp1LineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
  LOG_CMD("%d\n", __LINE__);

  vdp1cmd_struct cmd;
  Vdp2 *varVdp2Regs = &Vdp2Lines[0];

  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);

  CONVERTCMD(cmd.CMDXA);
  CONVERTCMD(cmd.CMDYA);
  CONVERTCMD(cmd.CMDXB);
  CONVERTCMD(cmd.CMDYB);
  CONVERTCMD(cmd.CMDXC);
  CONVERTCMD(cmd.CMDYC);
  CONVERTCMD(cmd.CMDXD);
  CONVERTCMD(cmd.CMDYD);

  cmd.CMDXA += Vdp1Regs->localX;
  cmd.CMDYA += Vdp1Regs->localY;
  cmd.CMDXB += Vdp1Regs->localX;
  cmd.CMDYB += Vdp1Regs->localY;
  cmd.CMDXC += Vdp1Regs->localX;
  cmd.CMDYC += Vdp1Regs->localY;
  cmd.CMDXD += Vdp1Regs->localX;
  cmd.CMDYD += Vdp1Regs->localY;

  //gouraud
  memset(cmd.G, 0, sizeof(float)*16);
  if ((cmd.CMDPMOD & 4))
  {
  for (int i = 0; i < 4; i++){
    u16 color2 = Vdp1RamReadWord(NULL, Vdp1Ram, (Vdp1RamReadWord(NULL, Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
    cmd.G[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
    cmd.G[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
    cmd.G[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
  }
  }
  cmd.priority = 0;
  cmd.w = 1;
  cmd.h = 1;
  cmd.flip = 0;
  cmd.cor = 0x0;
  cmd.cog = 0x0;
  cmd.cob = 0x0;
  cmd.SPCTL = varVdp2Regs->SPCTL;
  cmd.type = LINE;
  cmd.COLOR[0] = Vdp1ReadPolygonColor(&cmd,varVdp2Regs);

  vdp1_add(&cmd,0);
}

//////////////////////////////////////////////////////////////////////////////

void VIDCSVdp1UserClipping(u8 * ram, Vdp1 * regs)
{
  vdp1cmd_struct cmd;
  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
  cmd.type = USER_CLIPPING;
  vdp1_add(&cmd,1);
  Vdp1Regs->userclipX1 = cmd.CMDXA;
  Vdp1Regs->userclipY1 = cmd.CMDYA;
  Vdp1Regs->userclipX2 = cmd.CMDXC;
  Vdp1Regs->userclipY2 = cmd.CMDYC;
}

//////////////////////////////////////////////////////////////////////////////

void VIDCSVdp1SystemClipping(u8 * ram, Vdp1 * regs)
{
  vdp1cmd_struct cmd;
  Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
  cmd.type = SYSTEM_CLIPPING;
  vdp1_add(&cmd,1);
  Vdp1Regs->systemclipX2 = cmd.CMDXC;
  Vdp1Regs->systemclipY2 = cmd.CMDYC;
}

#endif
