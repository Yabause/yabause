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

/*! \file vidogl.c
    \brief OpenGL video renderer
*/
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)

#include <math.h>
#define EPSILON (1e-10 )


#include "vidogl.h"
#include "vidshared.h"
#include "debug.h"
#include "vdp2.h"
#include "yabause.h"
#include "ygl.h"
#include "yui.h"
#include "frameprofile.h"

static Vdp2 baseVdp2Regs;
static Vdp2 * fixVdp2Regs=NULL;
//#define PERFRAME_LOG
#ifdef PERFRAME_LOG
int fount = 0;
FILE *ppfp = NULL;
#endif

#ifdef _WINDOWS
int yprintf( const char * fmt, ... )
{
  static FILE * dbugfp = NULL;
  if (dbugfp == NULL){
    dbugfp = fopen("debug.txt", "w");
  }
  if (dbugfp){
    va_list ap;
    va_start(ap, fmt);
    vfprintf(dbugfp, fmt, ap);
    va_end(ap);
    fflush(dbugfp);
  }
  return 0;
}

void OSDPushMessageDirect(char * msg){
}

#endif

#define YGL_THREAD_DEBUG
//#define YGL_THREAD_DEBUG yprintf


#if defined WORDS_BIGENDIAN
#define SAT2YAB1(alpha,temp)      (alpha | (temp & 0x7C00) << 1 | (temp & 0x3E0) << 14 | (temp & 0x1F) << 27)
#else
#define SAT2YAB1(alpha,temp)      (alpha << 24 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9)
#endif

#if defined WORDS_BIGENDIAN
#define SAT2YAB2(alpha,dot1,dot2)       ((dot2 & 0xFF << 24) | ((dot2 & 0xFF00) << 8) | ((dot1 & 0xFF) << 8) | alpha)
#else
#define SAT2YAB2(alpha,dot1,dot2)       (alpha << 24 | ((dot1 & 0xFF) << 16) | (dot2 & 0xFF00) | (dot2 & 0xFF))
#endif

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

int VIDOGLInit(void);
void VIDOGLDeInit(void);
void VIDOGLResize(int,int,unsigned int, unsigned int, int);
int VIDOGLIsFullscreen(void);
int VIDOGLVdp1Reset(void);
void VIDOGLVdp1DrawStart(void);
void VIDOGLVdp1DrawEnd(void);
void VIDOGLVdp1NormalSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1ScaledSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1DistortedSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1PolygonDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1PolylineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1LineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer);
void VIDOGLVdp1UserClipping(u8 * ram, Vdp1 * regs);
void VIDOGLVdp1SystemClipping(u8 * ram, Vdp1 * regs);
void VIDOGLVdp1LocalCoordinate(u8 * ram, Vdp1 * regs);
int VIDOGLVdp2Reset(void);
void VIDOGLVdp2DrawStart(void);
void VIDOGLVdp2DrawEnd(void);
void VIDOGLVdp2DrawScreens(void);
void VIDOGLVdp2SetResolution(u16 TVMD);
void YglGetGlSize(int *width, int *height);
void VIDOGLGetNativeResolution(int *width, int *height, int*interlace);
void VIDOGLVdp1ReadFrameBuffer(u32 type, u32 addr, void * out);
void VIDOGLVdp1WriteFrameBuffer(u32 type, u32 addr, u32 val);
void VIDOGLSetSettingValueMode(int type, int value);
void VIDOGLSync();
void VIDOGLGetNativeResolution(int *width, int *height, int*interlace);
void VIDOGLVdp2DispOff(void);

VideoInterface_struct VIDOGL = {
VIDCORE_OGL,
"OpenGL Video Interface",
VIDOGLInit,
VIDOGLDeInit,
VIDOGLResize,
VIDOGLIsFullscreen,
VIDOGLVdp1Reset,
VIDOGLVdp1DrawStart,
VIDOGLVdp1DrawEnd,
VIDOGLVdp1NormalSpriteDraw,
VIDOGLVdp1ScaledSpriteDraw,
VIDOGLVdp1DistortedSpriteDraw,
VIDOGLVdp1PolygonDraw,
VIDOGLVdp1PolylineDraw,
VIDOGLVdp1LineDraw,
VIDOGLVdp1UserClipping,
VIDOGLVdp1SystemClipping,
VIDOGLVdp1LocalCoordinate,
VIDOGLVdp1ReadFrameBuffer,
VIDOGLVdp1WriteFrameBuffer,
YglEraseWriteVDP1,
YglFrameChangeVDP1,
VIDOGLVdp2Reset,
VIDOGLVdp2DrawStart,
VIDOGLVdp2DrawEnd,
VIDOGLVdp2DrawScreens,
YglGetGlSize,
VIDOGLSetSettingValueMode,
VIDOGLSync,
VIDOGLGetNativeResolution,
VIDOGLVdp2DispOff
};

float vdp1wratio=1;
float vdp1hratio=1;
static int vdp1_interlace = 0;

int GlWidth=320;
int GlHeight=224;

int vdp1cor=0;
int vdp1cog=0;
int vdp1cob=0;

static int vdp2width;
static int vdp2height;
static int vdp2_interlace = 0;
static int nbg0priority=0;
static int nbg1priority=0;
static int nbg2priority=0;
static int nbg3priority=0;
static int rbg0priority=0;

// Rotate Screen

typedef struct{
  int useb;
  vdp2draw_struct info;
  YglTexture texture;
  int rgb_type;
  int pagesize;
  int patternshift;
  u32 LineColorRamAdress;
  vdp2draw_struct line_info;
  YglTexture line_texture;
  YglCache c;
  YglCache cline;
  int vres;
  int hres;
  int async;
  volatile int vdp2_sync_flg;
} RBGDrawInfo;

RBGDrawInfo g_rgb0;
RBGDrawInfo g_rgb1;

YabMutex * g_rotate_mtx;
RBGDrawInfo * curret_rbg = NULL;
int Vdp2DrawRotationThread_running = 0;
static void Vdp2DrawRotation_in(RBGDrawInfo * rbg);
static void Vdp2DrawRotationSync();
static void Vdp2DrawRBG0(void);

static u32 Vdp2ColorRamGetColor(u32 colorindex, int alpha);
static void Vdp2PatternAddrPos(vdp2draw_struct *info, int planex, int x, int planey, int y);
static void Vdp2DrawPatternPos(vdp2draw_struct *info, YglTexture *texture, int x, int y, int cx, int cy);
static INLINE void ReadVdp2ColorOffset(Vdp2 * regs, vdp2draw_struct *info, int mask);
static INLINE u16 Vdp2ColorRamGetColorRaw(u32 colorindex);
static void FASTCALL Vdp2DrawRotation(RBGDrawInfo * rbg );

// Window Parameter
static vdp2WindowInfo * m_vWindinfo0 = NULL;
static int m_vWindinfo0_size = -1;
static int m_b0WindowChg;
static vdp2WindowInfo * m_vWindinfo1 = NULL;
static int m_vWindinfo1_size = -1;
static int m_b1WindowChg;

static vdp2Lineinfo lineNBG0[512];
static vdp2Lineinfo lineNBG1[512];



vdp2rotationparameter_struct  paraA = {0};
vdp2rotationparameter_struct  paraB = {0};



vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue2W( vdp2rotationparameter_struct * param, int index );
vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue1W( vdp2rotationparameter_struct * param, int index );
vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue2Wm3( vdp2rotationparameter_struct * param, int index );
vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue1Wm3( vdp2rotationparameter_struct * param, int index );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode00NoK( vdp2draw_struct * info, int h, int v );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode00WithK( vdp2draw_struct * info,int h, int v );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode01NoK( vdp2draw_struct * info,int h, int v );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode01WithK( vdp2draw_struct * info,int h, int v );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02NoK( vdp2draw_struct * info,int h, int v );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKA( vdp2draw_struct * info,int h, int v );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKAWithKB(vdp2draw_struct * info, int h, int v);
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKB( vdp2draw_struct * info,int h, int v );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03NoK( vdp2draw_struct * info,int h, int v );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithKA( vdp2draw_struct * info,int h, int v );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithKB( vdp2draw_struct * info,int h, int v );
vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithK( vdp2draw_struct * info,int h, int v );


static void FASTCALL Vdp1ReadPriority(vdp1cmd_struct *cmd, int * priority, int * colorcl, int * normal_shadow );
static void FASTCALL Vdp1ReadTexture(vdp1cmd_struct *cmd, YglSprite *sprite, YglTexture *texture);

u32 FASTCALL Vdp2ColorRamGetColorCM01SC0(vdp2draw_struct * info, u32 colorindex, int alpha, u8 lowdot );
u32 FASTCALL Vdp2ColorRamGetColorCM01SC1(vdp2draw_struct * info, u32 colorindex, int alpha, u8 lowdot );
u32 FASTCALL Vdp2ColorRamGetColorCM01SC2(vdp2draw_struct * info, u32 colorindex, int alpha, u8 lowdot );
u32 FASTCALL Vdp2ColorRamGetColorCM01SC3(vdp2draw_struct * info, u32 colorindex, int alpha, u8 lowdot );
u32 FASTCALL Vdp2ColorRamGetColorCM2(vdp2draw_struct * info, u32 colorindex, int alpha, u8 lowdot );


//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL Vdp1ReadPolygonColor(vdp1cmd_struct *cmd)
{
  int shadow = 0;
  int normalshadow = 0;
  int priority = 0;
  int colorcl = 0;

  int endcnt = 0;
  int nromal_shadow = 0;
  u32 talpha = 0x00; // MSB Color calcuration mode
  u32 shadow_alpha = (u8)0xF8 - (u8)0x80;

  u8 SPD = ((cmd->CMDPMOD & 0x40) != 0);
  u8 END = ((cmd->CMDPMOD & 0x80) != 0);
  u8 MSB = ((cmd->CMDPMOD & 0x8000) != 0);
  u32 alpha = 0xFF;
  u32 color = 0x00;
  int SPCCCS = (fixVdp2Regs->SPCTL >> 12) & 0x3;

  VDP1LOG("Making new sprite %08X\n", charAddr);

  Vdp1ReadPriority(cmd, &priority, &colorcl, &nromal_shadow);
  alpha = 0xF8;
  talpha = 0xF8;
  if (((fixVdp2Regs->CCCTL >> 6) & 0x01) == 0x01)
  {
    switch (SPCCCS)
    {
    case 0:
      if (priority <= ((fixVdp2Regs->SPCTL >> 8) & 0x07))
        if ((fixVdp2Regs->CCCTL & 0x540) == 0x140){  // Add Color calcuration mode 
          alpha = 0x80;  // Key value for color calcuration
        }
        else{
          alpha = 0xF8 - ((colorcl << 3) & 0xF8);
        }
        break;
    case 1:
      if (priority == ((fixVdp2Regs->SPCTL >> 8) & 0x07))
        if ((fixVdp2Regs->CCCTL & 0x540) == 0x140){  // Add Color calcuration mode 
          alpha = 0x80;  // Key value for color calcuration
        }
        else{
          alpha = 0xF8 - ((colorcl << 3) & 0xF8);
        }
        break;
    case 2:
      if (priority >= ((fixVdp2Regs->SPCTL >> 8) & 0x07))
        if ((fixVdp2Regs->CCCTL & 0x540) == 0x140){  // Add Color calcuration mode 
          alpha = 0x80;  // Key value for color calcuration
        }
        else{
          alpha = 0xF8 - ((colorcl << 3) & 0xF8);
        }
        break;
    case 3:
      if ((fixVdp2Regs->CCCTL & 0x540) == 0x140){  // Add Color calcuration mode 
        talpha = 0x80;  // Key value for color calcuration
      }
      else{
        talpha = 0xF8 - ((colorcl << 3) & 0xF8);
      }
      break;
    }

    if (fixVdp2Regs->CCCTL & 0x200) { // Color calculation ratio mode bit ( Destination Alpha mode )
      alpha = 0xF8 - ((colorcl << 3) & 0xF8);
    }
  }

  alpha |= priority;
  talpha |= priority;
  shadow_alpha |= priority;

  switch ((cmd->CMDPMOD >> 3) & 0x7)
  {
  case 0:
  {
    // 4 bpp Bank mode
    u32 colorBank = cmd->CMDCOLR;
    u32 colorOffset = (fixVdp2Regs->CRAOFB & 0x70) << 4;

    if (MSB) color = (alpha << 24);
    else if (colorBank == 0x0000){
      color = SAT2YAB1(priority, colorBank);
    }
    else if (colorBank == nromal_shadow){
      color = (shadow_alpha << 24);
    }
    else{
      int colorindex = colorBank + colorOffset;
      if ( (colorindex & 0x8000) && (fixVdp2Regs->SPCTL & 0x20)){
        color = SAT2YAB1(alpha, colorindex);
      }
      else{
        if (SPCCCS == 0x03){
          u16 checkcol = Vdp2ColorRamGetColorRaw(colorindex);
          if (checkcol & 0x8000){
            color = Vdp2ColorRamGetColor(colorindex, talpha);
          }
          else{
            color = Vdp2ColorRamGetColor(colorindex, alpha);
          }
        }
        else{
          color = Vdp2ColorRamGetColor(colorindex, alpha);
        }
      }
    }
    break;
  }
  case 1:
  {
    // 4 bpp LUT mode
    u16 temp;
    u32 colorLut = cmd->CMDCOLR * 8;
    u32 colorOffset = (fixVdp2Regs->CRAOFB & 0x70) << 4;

      temp = T1ReadWord(Vdp1Ram, colorLut & 0x7FFFF);
      if (temp & 0x8000)
      {
        if (MSB) color = (alpha << 24);
        else color = SAT2YAB1(alpha, temp);
      }else if (temp != 0x0000)
      {
        Vdp1ProcessSpritePixel(fixVdp2Regs->SPCTL & 0xF, &temp, &shadow, &normalshadow, &priority, &colorcl);
        if (shadow != 0)
        {
          color = (shadow_alpha << 24);
        }
        else{
#ifdef WORDS_BIGENDIAN
          priority = ((u8 *)&fixVdp2Regs->PRISA)[priority ^ 1] & 0x7;
          colorcl = ((u8 *)&fixVdp2Regs->CCRSA)[colorcl ^ 1] & 0x1F;
#else
          priority = ((u8 *)&fixVdp2Regs->PRISA)[priority] & 0x7;
          colorcl = ((u8 *)&fixVdp2Regs->CCRSA)[colorcl] & 0x1F;
#endif
          alpha = 0xF8;
          if (((fixVdp2Regs->CCCTL >> 6) & 0x01) == 0x01)
          {
            switch ((fixVdp2Regs->SPCTL >> 12) & 0x03)
            {
            case 0:
              if (priority <= ((fixVdp2Regs->SPCTL >> 8) & 0x07))
                alpha = 0xF8 - ((colorcl << 3) & 0xF8);
              break;
            case 1:
              if (priority == ((fixVdp2Regs->SPCTL >> 8) & 0x07))
                alpha = 0xF8 - ((colorcl << 3) & 0xF8);
              break;
            case 2:
              if (priority >= ((fixVdp2Regs->SPCTL >> 8) & 0x07))
                alpha = 0xF8 - ((colorcl << 3) & 0xF8);
              break;
            case 3:
            {
              u16 checkcol = Vdp2ColorRamGetColorRaw(temp + colorOffset);
              if (checkcol & 0x8000){
                alpha = 0xF8 - ((colorcl << 3) & 0xF8);
              }
            }
              break;
            }
          }
          alpha |= priority;
          if (MSB) color = (alpha << 24);
          else color = Vdp2ColorRamGetColor(temp + colorOffset, alpha);
        }
      }else{
        color = 0x0;
      }

    break;
  }
  case 2:
  {
    // 8 bpp(64 color) Bank mode
    u32 colorBank = cmd->CMDCOLR & 0xFFC0;
    u32 colorOffset = (fixVdp2Regs->CRAOFB & 0x70) << 4;
    if (MSB) color = (alpha << 24);
    else if (colorBank == 0x0000){
      color = SAT2YAB1(priority, colorBank);
    }
    else if ( colorBank == nromal_shadow){
      color = (shadow_alpha << 24);
    }else{
      const int colorindex = (colorBank) + colorOffset;
      if ((colorindex & 0x8000) && (fixVdp2Regs->SPCTL & 0x20)){
        color = SAT2YAB1(alpha, colorindex);
      }
      else{
        if (SPCCCS == 0x03){
          u16 checkcol = Vdp2ColorRamGetColorRaw(colorindex);
          if (checkcol & 0x8000){
            color = Vdp2ColorRamGetColor(colorindex, talpha);
          }
          else{
            color = Vdp2ColorRamGetColor(colorindex, alpha);
          }
        }
        else{
          color = Vdp2ColorRamGetColor(colorindex, alpha);
        }
      }
    }
    break;
  }
  case 3:
  {
    // 8 bpp(128 color) Bank mode
    u32 colorBank = cmd->CMDCOLR & 0xFF80;
    u32 colorOffset = (fixVdp2Regs->CRAOFB & 0x70) << 4;
    if (MSB) color= (alpha << 24);
    else if (colorBank == 0x0000){
      color = SAT2YAB1(priority, colorBank);
    }
    else if (colorBank == nromal_shadow){
      color = (shadow_alpha << 24);
    }
    else{
      const int colorindex = (colorBank)+colorOffset;
      if ((colorindex & 0x8000) && (fixVdp2Regs->SPCTL & 0x20)){
        color = SAT2YAB1(alpha, colorindex);
      }
      else{
        if (SPCCCS == 0x03){
          u16 checkcol = Vdp2ColorRamGetColorRaw(colorindex);
          if (checkcol & 0x8000){
            color = Vdp2ColorRamGetColor(colorindex, talpha);
          }
          else{
            color = Vdp2ColorRamGetColor(colorindex, alpha);
          }
        }
        else{
          color = Vdp2ColorRamGetColor(colorindex, alpha);
        }
      }
    }
    break;
  }
  case 4:
  {
    // 8 bpp(256 color) Bank mode
    u32 colorBank = cmd->CMDCOLR & 0xFF00;
    u32 colorOffset = (fixVdp2Regs->CRAOFB & 0x70) << 4;
    if (MSB) color = (alpha << 24);
    else if ((colorBank == 0x0000) && (SPD == 0)){
      color = SAT2YAB1(priority, colorBank);
    }
    else if (color == nromal_shadow){
      color = (shadow_alpha << 24);
    }
    else{
      const int colorindex = (colorBank)+colorOffset;
      if ((colorindex & 0x8000) && (fixVdp2Regs->SPCTL & 0x20)){
        color = SAT2YAB1(alpha, colorindex);
      }
      else{
        if (SPCCCS == 0x03){
          u16 checkcol = Vdp2ColorRamGetColorRaw(colorindex);
          if (checkcol & 0x8000){
            color = Vdp2ColorRamGetColor(colorindex, talpha);
          }
          else{
            color = Vdp2ColorRamGetColor(colorindex, alpha);
          }
        }
        else{
          color = Vdp2ColorRamGetColor(colorindex, alpha);
        }
      }
    }
    break;
  }
  case 5:
  {
    // 16 bpp Bank mode
    u16 dot = cmd->CMDCOLR;
    if (dot == 0x0000){
      color = SAT2YAB1(priority, dot);
    }
    else if (MSB) color = (alpha << 24);
    else if (dot == nromal_shadow){
      color = (shadow_alpha << 24);
    }else if (SPCCCS == 0x03){
      if (dot & 0x8000){
        color = SAT2YAB1(talpha, dot);
      }
      else{
        color = Vdp2ColorRamGetColor(dot, alpha);
      }

    }
    else{
      if (dot & 0x8000){
        color = SAT2YAB1(alpha, dot);
      }
      else{
        color = Vdp2ColorRamGetColor(dot, alpha);
      }
    }
  }
    break;
  default:
    VDP1LOG("Unimplemented sprite color mode: %X\n", (cmd->CMDPMOD >> 3) & 0x7);
    break;
  }

  return color;
}



static void FASTCALL Vdp1ReadTexture(vdp1cmd_struct *cmd, YglSprite *sprite, YglTexture *texture)
{
  int shadow = 0;
  int normalshadow = 0;
  int priority = 0;
  int colorcl = 0;
  
  int ednmode;
  int endcnt = 0;
  int nromal_shadow = 0;
  u32 talpha = 0x00; // MSB Color calcuration mode
  u32 shadow_alpha = (u8)0xF8 - (u8)0x80;
  u32 charAddr = cmd->CMDSRCA * 8;
  u32 dot;
  u8 SPD = ((cmd->CMDPMOD & 0x40) != 0);
  u8 END = ((cmd->CMDPMOD & 0x80) != 0);
  u8 MSB = ((cmd->CMDPMOD & 0x8000) != 0);
  u8 MSB_SHADOW = 0;
  u32 alpha = 0xFF;
  u8 addcolor = 0;
  int SPCCCS = (fixVdp2Regs->SPCTL >> 12) & 0x3;
  VDP1LOG("Making new sprite %08X\n", charAddr);
   
  if (/*fixVdp2Regs->SDCTL != 0 &&*/ MSB != 0 ){
    MSB_SHADOW = 1;
  }

  if( (cmd->CMDPMOD & 0x20) == 0)
    ednmode = 1;
  else
    ednmode = 0;

   addcolor = ((fixVdp2Regs->CCCTL & 0x540) == 0x140);
   
   Vdp1ReadPriority(cmd, &priority, &colorcl, &nromal_shadow );
   
   alpha = 0xF8;
   talpha = 0xF8;
   // Enable sprite color control? and top color?
   if ((fixVdp2Regs->CCCTL & 0x040) == 0x040 || (fixVdp2Regs->CCCTL & 0x240) == 0x200 )
   {
     switch (SPCCCS)
     {
     case 0:
       if (priority <= ((fixVdp2Regs->SPCTL >> 8) & 0x07)){
         if (addcolor){  // Add Color calcuration mode 
           alpha = 0x80;  // Key value for color calcuration
         }
         else{
           alpha = 0xF8 - ((colorcl << 3) & 0xF8);
         }
       }
       break;
     case 1:
       if (priority == ((fixVdp2Regs->SPCTL >> 8) & 0x07)){
         if (addcolor){  // Add Color calcuration mode 
           alpha = 0x80;  // Key value for color calcuration
         }
         else{
           alpha = 0xF8 - ((colorcl << 3) & 0xF8);
         }
       }
       break;
     case 2:
       if (priority >= ((fixVdp2Regs->SPCTL >> 8) & 0x07)){
         if (addcolor){  // Add Color calcuration mode 
           alpha = 0x80;  // Key value for color calcuration
         }else{
           alpha = 0xF8 - ((colorcl << 3) & 0xF8);
         }
      }
        break;
      case 3:
      if (addcolor){  // Add Color calcuration mode 
        talpha = 0x80;  // Key value for color calcuration
      }else{
        talpha = 0xF8 - ((colorcl << 3) & 0xF8);
      }
    break;
      }

    if (fixVdp2Regs->CCCTL & 0x200) { // Color calculation ratio mode bit ( Destination Alpha mode )
       alpha = 0xF8 - ((colorcl << 3) & 0xF8);
    }
   }

   alpha |= priority;
   talpha |= priority;
   shadow_alpha |= priority;

  switch((cmd->CMDPMOD >> 3) & 0x7)
  {
  case 0:
  {
    // 4 bpp Bank mode
    u32 colorBank = cmd->CMDCOLR; // &0xFFF0;
    u32 colorOffset = (fixVdp2Regs->CRAOFB & 0x70) << 4;
    u16 i;

    for(i = 0;i < sprite->h;i++) {
      u16 j;
      j = 0;
      while(j < sprite->w) {
        dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);

        // Pixel 1
        if (((dot >> 4) == 0) && !SPD) *texture->textdata++ = 0x00;
        else if( ((dot >> 4) == 0x0F) && !END ) *texture->textdata++ = 0x00;
        else if (MSB_SHADOW){
          *texture->textdata++ = (0x80) << 24;
        }else if (((dot >> 4) | colorBank) == 0x0000){
          //u32 talpha = 0xF8 - ((colorcl << 3) & 0xF8);
          //talpha |= priority;
          *texture->textdata++ = 0; //Vdp2ColorRamGetColor(((dot >> 4) | colorBank) + colorOffset, talpha);
        }else if (((dot >> 4) | colorBank) == nromal_shadow){
          *texture->textdata++ = (shadow_alpha << 24);
        }else{
          int colorindex = ((dot >> 4) | colorBank) + colorOffset;
          if ((colorindex & 0x8000) && (fixVdp2Regs->SPCTL & 0x20)){
            *texture->textdata++ = SAT2YAB1(alpha, colorindex);
          }else{
            if (SPCCCS == 0x03){
              u16 checkcol = Vdp2ColorRamGetColorRaw(colorindex);
              if (checkcol & 0x8000){
                *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, talpha);
              }else{
                *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, alpha);
              }
            }else{
              *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, alpha);
            }
          }
        }
        j += 1;

        // Pixel 2
        if (((dot & 0xF) == 0) && !SPD) *texture->textdata++ = 0x00;
        else if( ((dot & 0xF) == 0x0F) && !END ) *texture->textdata++ = 0x00;
        else if (MSB_SHADOW){
           *texture->textdata++ = (0x80) << 24;
        }else if (((dot & 0x0F) | colorBank) == 0x0000){
          //u32 talpha = 0xF8 - ((colorcl << 3) & 0xF8);
          //talpha |= priority;
          *texture->textdata++ = 0; // Vdp2ColorRamGetColor(((dot & 0xF) | colorBank) + colorOffset, talpha);
        }else if (((dot & 0xF) | colorBank) == nromal_shadow){
          *texture->textdata++ = (shadow_alpha << 24);
        }else{
          int colorindex = ((dot & 0xF) | colorBank) + colorOffset;
          if ((colorindex & 0x8000) && (fixVdp2Regs->SPCTL & 0x20)){
            *texture->textdata++ = SAT2YAB1(alpha, colorindex);
          }else{
             if (SPCCCS == 0x03){
               u16 checkcol = Vdp2ColorRamGetColorRaw(colorindex);
               if (checkcol & 0x8000){
                 *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, talpha);
               }
               else{
                 *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, alpha);
               }
             }
             else{
               *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, alpha);
             }
           }
         }
               j += 1;

               charAddr += 1;
            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 1:
      {
         // 4 bpp LUT mode
         u16 temp;
         u32 colorLut = cmd->CMDCOLR * 8;
         u16 i;
         u32 colorOffset = (fixVdp2Regs->CRAOFB & 0x70) << 4;
         
         for(i = 0;i < sprite->h;i++)
         {
            u16 j;
            j = 0;
            endcnt = 0;
            while(j < sprite->w)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);

               if( ednmode && endcnt >= 2 )
               {
                  *texture->textdata++ = 0x00;
               }else if (((dot >> 4) == 0) && !SPD)
               {
                  *texture->textdata++ = 0;
               }else if (((dot >> 4) == 0x0F) && !END ) // 6. Commandtable end code
               {
                  *texture->textdata++ = 0x0;
                  endcnt++;
                  
               }else{
                  temp = T1ReadWord(Vdp1Ram, ((dot >> 4) * 2 + colorLut) & 0x7FFFF);
                  if (temp & 0x8000)
                  {
          	    if (MSB_SHADOW){
                      *texture->textdata++ = (0x80) << 24;
                    }
                    else{
                      *texture->textdata++ = SAT2YAB1(alpha, temp);
                    }
                  }else if( temp != 0x0000)
                  {
                     Vdp1ProcessSpritePixel(fixVdp2Regs->SPCTL & 0xF, &temp, &shadow, &normalshadow, &priority, &colorcl);
                     if( shadow != 0 ) 
                     {
                         *texture->textdata++ = (shadow_alpha << 24);
                     }else{
#ifdef WORDS_BIGENDIAN
                        priority = ((u8 *)&fixVdp2Regs->PRISA)[priority^1]&0x7;
                        colorcl =  ((u8 *)&fixVdp2Regs->CCRSA)[colorcl^1]&0x1F;
#else
                        priority = ((u8 *)&fixVdp2Regs->PRISA)[priority]&0x7;
                        colorcl =  ((u8 *)&fixVdp2Regs->CCRSA)[colorcl]&0x1F;
#endif
			if (normalshadow) {
				*texture->textdata++ = ((0x7C|priority) << 24);
                        } else {
                        	alpha = 0xF8;
                        if( ((fixVdp2Regs->CCCTL >> 6) & 0x01) == 0x01  )
                        {
                          switch (SPCCCS)
                           {
                           case 0:
                              if( priority <= ((fixVdp2Regs->SPCTL>>8)&0x07) )
                              alpha = 0xF8-((colorcl<<3)&0xF8);      
                              break;
                           case 1:
                              if( priority == ((fixVdp2Regs->SPCTL>>8)&0x07) )
                              alpha = 0xF8-((colorcl<<3)&0xF8);
                              break;
                           case 2:
                              if( priority >= ((fixVdp2Regs->SPCTL>>8)&0x07) )
                              alpha = 0xF8-((colorcl<<3)&0xF8);      
                              break;
                           case 3:
                           {
                              u16 checkcol = Vdp2ColorRamGetColorRaw((temp + colorOffset));
                              if (checkcol & 0x8000){
                                 alpha = 0xF8 - ((colorcl << 3) & 0xF8);
                              }
                           }
                           break;
                           }                     
                        }
                        alpha |= priority;
                        if (MSB_SHADOW){
                          *texture->textdata++ = (0x80) << 24;
                        }
                        else{
                          *texture->textdata++ = Vdp2ColorRamGetColor(temp + colorOffset, alpha);
                        }
			}
                     }
                  }else{
                    *texture->textdata++ = 0x0;
                  }
               }

               j += 1;

               if( ednmode && endcnt >= 2 )
               {
                  *texture->textdata++ = 0x00;
               }else if (((dot & 0xF) == 0) && !SPD)
               {
                  *texture->textdata++ = 0x00;
               }else if (((dot&0x0F) == 0x0F) && !END )
               {
                  *texture->textdata++ = 0x0;
                  endcnt++;
               }else{
                  temp = T1ReadWord(Vdp1Ram, ((dot & 0xF) * 2 + colorLut) & 0x7FFFF);
                  
                  	if (temp & 0x8000)
                  	{
            			if (MSB_SHADOW){
              			*texture->textdata++ = (0x80) << 24;
            		}
            		else{
              			*texture->textdata++ = SAT2YAB1(alpha, temp);
            		}
                  }else if( temp != 0x0000)
                  {
                     Vdp1ProcessSpritePixel(fixVdp2Regs->SPCTL & 0xF, &temp, &shadow, &normalshadow, &priority, &colorcl);
                     if( shadow != 0 ) 
                     {
             *texture->textdata++ = (shadow_alpha << 24);
                     }else{                     
#ifdef WORDS_BIGENDIAN
                        priority = ((u8 *)&fixVdp2Regs->PRISA)[priority^1]&0x7;
                        colorcl =  ((u8 *)&fixVdp2Regs->CCRSA)[colorcl^1]&0x1F;
#else
                        priority = ((u8 *)&fixVdp2Regs->PRISA)[priority]&0x7;
                        colorcl =  ((u8 *)&fixVdp2Regs->CCRSA)[colorcl]&0x1F;
#endif
                        if (normalshadow) {
				*texture->textdata++ = ((0x7C|priority) << 24);
                        } else {
                        	alpha = 0xF8;
                        if( ((fixVdp2Regs->CCCTL >> 6) & 0x01) == 0x01  )
                        {
              switch (SPCCCS)
                           {
                           case 0:
                              if( priority <= ((fixVdp2Regs->SPCTL>>8)&0x07) )
                              alpha = 0xF8-((colorcl<<3)&0xF8);      
                              break;
                           case 1:
                              if( priority == ((fixVdp2Regs->SPCTL>>8)&0x07) )
                              alpha = 0xF8-((colorcl<<3)&0xF8);      
                              break;
                           case 2:
                              if( priority >= ((fixVdp2Regs->SPCTL>>8)&0x07) )
                              alpha = 0xF8-((colorcl<<3)&0xF8);      
                              break;
               case 3:
               {
                 u16 checkcol = Vdp2ColorRamGetColorRaw((temp + colorOffset));
                 if (checkcol & 0x8000){
                   alpha = 0xF8 - ((colorcl << 3) & 0xF8);
                 }
               }
                              break;
                           } 
                        }
                        alpha |= priority;
            if (MSB_SHADOW){
              *texture->textdata++ = (0x80) << 24;
            }
            else{
              *texture->textdata++ = Vdp2ColorRamGetColor(temp + colorOffset, alpha);
            }
	    }
                     }
                  }else
                     *texture->textdata++ = 0x0;
               }

               j += 1;

               charAddr += 1;
            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 2:
      {
         // 8 bpp(64 color) Bank mode
        u32 colorBank = cmd->CMDCOLR & 0xFFC0;
         u32 colorOffset = (fixVdp2Regs->CRAOFB & 0x70) << 4;

         u16 i, j;
         
         for(i = 0;i < sprite->h;i++)
         {
            for(j = 0;j < sprite->w;j++)
            {
        dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);
               charAddr++;
               if ((dot == 0) && !SPD) *texture->textdata++ = 0x00;
               else if( (dot == 0xFF) && !END ) *texture->textdata++ = 0x00;
         else if (MSB_SHADOW){
           *texture->textdata++ = (0x80)<< 24;
         }
         else if (((dot & 0x3F) | colorBank) == nromal_shadow){
           *texture->textdata++ = (shadow_alpha << 24);
         }
         else{
           const int colorindex = ((dot&0x3F)|colorBank)+colorOffset;
           if ((colorindex & 0x8000) && (fixVdp2Regs->SPCTL & 0x20)){
             *texture->textdata++ = SAT2YAB1(alpha, colorindex);
           }
           else{
             if (SPCCCS == 0x03){
               u16 checkcol = Vdp2ColorRamGetColorRaw(colorindex);
               if (checkcol & 0x8000){
                 *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, talpha);
               }
               else{
                 *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, alpha);
               }
             }
             else{
               *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, alpha);
             }
           }
         }
            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 3:
      {
         // 8 bpp(128 color) Bank mode
        u32 colorBank = cmd->CMDCOLR & 0xFF80;
         u32 colorOffset = (fixVdp2Regs->CRAOFB & 0x70) << 4;
         u16 i, j;
         
         for(i = 0;i < sprite->h;i++)
         {
            for(j = 0;j < sprite->w;j++)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);
               charAddr++;

               if ((dot == 0) && !SPD) *texture->textdata++ = 0x00;
               else if( (dot == 0xFF) && !END ) *texture->textdata++ = 0x00;
         else if (MSB_SHADOW){
           *texture->textdata++ = (0x80) << 24;
         }
         else if (((dot & 0x7F) | colorBank) == nromal_shadow){
           *texture->textdata++ = (shadow_alpha << 24);
         }else{
           const int colorindex = ((dot&0x7F)|colorBank) + colorOffset;
           if ((colorindex & 0x8000) && (fixVdp2Regs->SPCTL & 0x20)){
             *texture->textdata++ = SAT2YAB1(alpha, colorindex);
           }
           else{
             if (SPCCCS == 0x03){
               u16 checkcol = Vdp2ColorRamGetColorRaw(colorindex);
               if (checkcol & 0x8000){
                 *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, talpha);
               }
               else{
                 *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, alpha);
               }
             }
             else{
               *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, alpha);
             }
           }
         }

            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 4:
      {
         // 8 bpp(256 color) Bank mode
        u32 colorBank = cmd->CMDCOLR & 0xFF00;
         u32 colorOffset = (fixVdp2Regs->CRAOFB & 0x70) << 4;
         u16 i, j;

         for(i = 0;i < sprite->h;i++)
         {
            for(j = 0;j < sprite->w;j++)
            {
               dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);
               charAddr++;

               if ((dot == 0) && !SPD) *texture->textdata++ = 0x00;
         else if (dot == 0x0000){ *texture->textdata++ = 0x00; }
               else if( (dot == 0xFF) && !END ) *texture->textdata++ = 0x0;
         else if (MSB_SHADOW){
           *texture->textdata++ = (0x80) << 24;
         }
         else if ((dot | colorBank) == nromal_shadow){
           *texture->textdata++ = (shadow_alpha << 24);
         }
         else{
           const int colorindex = (dot | colorBank) + colorOffset;
           if ((colorindex & 0x8000) && (fixVdp2Regs->SPCTL & 0x20)){
             *texture->textdata++ = SAT2YAB1(alpha, colorindex);
           }
           else{
             if (SPCCCS == 0x03){
               u16 checkcol = Vdp2ColorRamGetColorRaw(colorindex);
               if (checkcol & 0x8000){
                 *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, talpha);
               }
               else{
                 *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, alpha);
               }
             }
             else{
               *texture->textdata++ = Vdp2ColorRamGetColor(colorindex, alpha);
             }
           }
         }
            }
            texture->textdata += texture->w;
         }
         break;
      }
      case 5:
      {
         // 16 bpp Bank mode
         u16 i, j;

         // hard/vdp2/hon/p09_20.htm#no9_21
         // �X�v���C�g�f�[�^��RGB�`���̏ꍇ�́A�X�v���C�g�p���W�X�^0���I�������܂��B
         u8 *cclist = (u8 *)&fixVdp2Regs->CCRSA;
         cclist[0] &= 0x1F;
         u8 rgb_alpha = 0xF8 - (((cclist[0] & 0x1F) << 3) & 0xF8);
         rgb_alpha |= priority;

         for(i = 0;i < sprite->h;i++)
         {
            for(j = 0;j < sprite->w;j++)
            {
               dot = T1ReadWord(Vdp1Ram, charAddr & 0x7FFFF);
               charAddr += 2;

               //if (!(dot & 0x8000) && (fixVdp2Regs->SPCTL & 0x20)) printf("mixed mode\n");
               if (!(dot & 0x8000) && !SPD) {
                 *texture->textdata++ = 0x00;
               } else if (dot == 0x0000){ 
                 *texture->textdata++ = 0x00; 
               } else if ((dot == 0x7FFF) && !END) {
                 *texture->textdata++ = 0x0;
               } else if (MSB_SHADOW){
                  *texture->textdata++ = (0x80) << 24;
               } else if (dot == nromal_shadow){
                  *texture->textdata++ = (shadow_alpha << 24);
               } else if (SPCCCS == 0x03 && (dot&0x8000) ){
                  *texture->textdata++ = SAT2YAB1(talpha, dot);
                }else{
                  if (dot & 0x8000){
                    *texture->textdata++ = SAT2YAB1(rgb_alpha, dot);
                  }else{
                   *texture->textdata++ = Vdp2ColorRamGetColor(dot, alpha); 
                  }
               }
            }
            texture->textdata += texture->w;
         }
         break;
      }
      default:
         VDP1LOG("Unimplemented sprite color mode: %X\n", (cmd->CMDPMOD >> 3) & 0x7);
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp1ReadPriority(vdp1cmd_struct *cmd, int * priority, int * colorcl, int * normal_shadow )
{
   u8 SPCLMD = fixVdp2Regs->SPCTL;
   u8 sprite_register;
   u8 *sprprilist = (u8 *)&fixVdp2Regs->PRISA;
   u8 *cclist = (u8 *)&fixVdp2Regs->CCRSA;   
   u16 lutPri;
   u16 *reg_src = &cmd->CMDCOLR;
   int not_lut = 1;

   // is the sprite is RGB or LUT (in fact, LUT can use bank color, we just hope it won't...)
   if ((SPCLMD & 0x20) && (cmd->CMDCOLR & 0x8000))
   {
      // RGB data, use register 0
      *priority = fixVdp2Regs->PRISA & 0x7;
    *colorcl = cclist[0] & 0x1F;
      return;
   }

   if (((cmd->CMDPMOD >> 3) & 0x7) == 1) {
      u32 charAddr, dot, colorLut;

      *priority = fixVdp2Regs->PRISA & 0x7;

      charAddr = cmd->CMDSRCA * 8;
      dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);
      colorLut = cmd->CMDCOLR * 8;
      lutPri = T1ReadWord(Vdp1Ram, (dot >> 4) * 2 + colorLut);
      if (!(lutPri & 0x8000)) {
         not_lut = 0;
         reg_src = &lutPri;
      } else
         return;
   }

   {
      u8 sprite_type = SPCLMD & 0xF;
      switch(sprite_type)
      {
         case 0:
            sprite_register = (*reg_src & 0xC000) >> 14;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>11)&0x07)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[(cmd->CMDCOLR>>11)&0x07]&0x1F;
#endif
      *normal_shadow = 0x7FE;
            if (not_lut) cmd->CMDCOLR &= 0x7FF;
            break;
         case 1:
            sprite_register = (*reg_src & 0xE000) >> 13;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>11)&0x03)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[(cmd->CMDCOLR>>11)&0x03]&0x1F;
#endif
      *normal_shadow = 0x7FE;
            if (not_lut) cmd->CMDCOLR &= 0x7FF;
            break;
         case 2:
            sprite_register = (*reg_src >> 14) & 0x1;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>11)&0x07)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[(cmd->CMDCOLR>>11)&0x07]&0x1F;
#endif
      *normal_shadow = 0x7FE;
            if (not_lut) cmd->CMDCOLR &= 0x7FF;
            break;
         case 3:
            sprite_register = (*reg_src & 0x6000) >> 13;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>11)&0x03)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>11)&0x03)]&0x1F;
#endif
      *normal_shadow = 0x7FE;
            if (not_lut) cmd->CMDCOLR &= 0x7FF;
            break;
         case 4:
            sprite_register = (*reg_src & 0x6000) >> 13;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>10)&0x07)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>10)&0x07)]&0x1F;
#endif
      *normal_shadow = 0x3FE;
            if (not_lut) cmd->CMDCOLR &= 0x3FF;
            break;
         case 5:
            sprite_register = (*reg_src & 0x7000) >> 12;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>11)&0x01)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>11)&0x01)]&0x1F;
#endif
      *normal_shadow = 0x7FE;
            if (not_lut) cmd->CMDCOLR &= 0x7FF;
            break;
         case 6:
            sprite_register = (*reg_src & 0x7000) >> 12;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>10)&0x03)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>10)&0x03)]&0x1F;
#endif
      *normal_shadow = 0x3FE;
            if (not_lut) cmd->CMDCOLR &= 0x3FF;
            break;
         case 7:
            sprite_register = (*reg_src & 0x7000) >> 12;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>9)&0x07)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>9)&0x07)]&0x1F;
#endif
      *normal_shadow = 0x1FE;
            if (not_lut) cmd->CMDCOLR &= 0x1FF;
            break;
         case 8:
            sprite_register = (*reg_src & 0x80) >> 7;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
#else
            *priority = sprprilist[sprite_register] & 0x7;
#endif
      *normal_shadow = 0x7E;
            *colorcl =  cclist[0]&0x1F;
            if (not_lut) cmd->CMDCOLR &= 0x7F;
            break;
         case 9:
            sprite_register = (*reg_src & 0x80) >> 7;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>6)&0x01)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>6)&0x01)]&0x1F;
#endif
      *normal_shadow = 0x3E;
            if (not_lut) cmd->CMDCOLR &= 0x3F;
            break;
         case 10:
            sprite_register = (*reg_src & 0xC0) >> 6;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
#else
            *priority = sprprilist[sprite_register] & 0x7;
#endif
            *colorcl =  cclist[0]&0x1F;
            if (not_lut) cmd->CMDCOLR &= 0x3F;
            break;
         case 11:
            sprite_register = 0;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>6)&0x03)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>6)&0x03)]&0x1F;
#endif
      *normal_shadow = 0x3E;
            if (not_lut) cmd->CMDCOLR &= 0x3F;
            break;
         case 12:
            sprite_register = (*reg_src & 0x80) >> 7;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
#else
            *priority = sprprilist[sprite_register] & 0x7;
#endif
            *colorcl =  cclist[0]&0x1F;
      *normal_shadow = 0xFE;
            if (not_lut) cmd->CMDCOLR &= 0xFF;
            break;
         case 13:
            sprite_register = (*reg_src & 0x80) >> 7;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>6)&0x01)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>6)&0x01)]&0x1F;
#endif
      *normal_shadow = 0xFE;
            if (not_lut) cmd->CMDCOLR &= 0xFF;
            break;
         case 14:
            sprite_register = (*reg_src & 0xC0) >> 6;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[1]&0x1F;            
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[0]&0x1F;
#endif
      *normal_shadow = 0xFE;
            if (not_lut) cmd->CMDCOLR &= 0xFF;
            break;
         case 15:
            sprite_register = 0;
#ifdef WORDS_BIGENDIAN
            *priority = sprprilist[sprite_register^1] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>6)&0x03)^1]&0x1F;
#else
            *priority = sprprilist[sprite_register] & 0x7;
            *colorcl =  cclist[((cmd->CMDCOLR>>6)&0x03)]&0x1F;
#endif
      *normal_shadow = 0xFE;
            if (not_lut) cmd->CMDCOLR &= 0xFF;
            break;
         default:
            VDP1LOG("sprite type %d not implemented\n", sprite_type);

            // if we don't know what to do with a sprite, we put it on top
            *priority = 7;
            break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp1SetTextureRatio(int vdp2widthratio, int vdp2heightratio)
{
   float vdp1w=1;
   float vdp1h=1;

   // may need some tweaking

   // Figure out which vdp1 screen mode to use
   switch (Vdp1Regs->TVMR & 7)
   {
      case 0:
      case 2:
      case 3:
          vdp1w=1;
          break;
      case 1:
          vdp1w=2;
          break;
      default:
          vdp1w=1;
          vdp1h=1;
          break;
   }

   // Is double-interlace enabled?
   if (Vdp1Regs->FBCR & 0x8) {
      vdp1h=2;
      vdp1_interlace = (Vdp1Regs->FBCR & 0x4)?2:1;
  } else {
      vdp1_interlace = 0;
  }

   vdp1wratio = (float)vdp2widthratio / vdp1w;
   vdp1hratio = (float)vdp2heightratio / vdp1h;
}

//////////////////////////////////////////////////////////////////////////////
static u16 Vdp2ColorRamGetColorRaw(u32 colorindex){
  switch (Vdp2Internal.ColorMode)
  {
  case 0:
  case 1:
  {
    colorindex <<= 1;
    return T2ReadWord(Vdp2ColorRam, colorindex & 0xFFF);
  }
  case 2:
  {
    colorindex <<= 2;
    colorindex &= 0xFFF;
    return T2ReadWord(Vdp2ColorRam, colorindex);
  }
  default: break;
  }
  return 0;
}

static u32 Vdp2ColorRamGetColor(u32 colorindex, int alpha)
{
   switch(Vdp2Internal.ColorMode)
   {
      case 0:
      case 1:
      {
         u32 tmp;
         colorindex <<= 1;
         tmp = T2ReadWord(Vdp2ColorRam, colorindex & 0xFFF);
         return SAT2YAB1(alpha, tmp);
      }
      case 2:
      {
         u32 tmp1, tmp2;
         colorindex <<= 2;
         colorindex &= 0xFFF;
         tmp1 = T2ReadWord(Vdp2ColorRam, colorindex);
         tmp2 = T2ReadWord(Vdp2ColorRam, colorindex+2);
         return SAT2YAB2(alpha, tmp1, tmp2);
      }
      default: break;
   }
   return 0;
}

u32 FASTCALL Vdp2ColorRamGetColorCM01SC0(vdp2draw_struct * info, u32 colorindex, int alpha, u8 lowdot )
{
   u32 tmp;
   tmp = T2ReadWord(Vdp2ColorRam, (colorindex<<1) & 0xFFF);
   return SAT2YAB1(alpha,tmp);
}

u32 FASTCALL Vdp2ColorRamGetColorCM01SC2(vdp2draw_struct * info, u32 colorindex, int alpha, u8 lowdot )
{
   u32 tmp;
   u16 code = ((fixVdp2Regs->SFSEL & (1<< info->id)) == 0)?(fixVdp2Regs->SFCODE & 0xFF):(fixVdp2Regs->SFCODE & 0xFF00)>>8;
   tmp = T2ReadWord(Vdp2ColorRam, (colorindex<<1) & 0xFFF);

   if( info->specialcolorfunction == 0 )
   {
      return SAT2YAB1(0xFF,tmp);
   };
   if ((code & 0x1) && ((lowdot == 0x0) || (lowdot == 0x1))) return SAT2YAB1(alpha,tmp);
   if ((code & 0x2) && ((lowdot == 0x2) || (lowdot == 0x3))) return SAT2YAB1(alpha,tmp);
   if ((code & 0x4) && ((lowdot == 0x4) || (lowdot == 0x5))) return SAT2YAB1(alpha,tmp);
   if ((code & 0x8) && ((lowdot == 0x6) || (lowdot == 0x7))) return SAT2YAB1(alpha,tmp);
   if ((code & 0x10) && ((lowdot == 0x8) || (lowdot == 0x9))) return SAT2YAB1(alpha,tmp);
   if ((code & 0x20) && ((lowdot == 0xA) || (lowdot == 0xB))) return SAT2YAB1(alpha,tmp);
   if ((code & 0x40) && ((lowdot == 0xC) || (lowdot == 0xD))) return SAT2YAB1(alpha,tmp);
   if ((code & 0x80) && ((lowdot == 0xE) || (lowdot == 0xF))) return SAT2YAB1(alpha,tmp);

   return SAT2YAB1(0xFF,tmp);
   
}

u32 FASTCALL Vdp2ColorRamGetColorCM01SC1(vdp2draw_struct * info, u32 colorindex, int alpha, u8 lowdot )
{
   u32 tmp;
   tmp = T2ReadWord(Vdp2ColorRam, (colorindex<<1) & 0xFFF);   
   if( info->specialcolorfunction == 0 )
   {
      return SAT2YAB1(0xFF,tmp);
   }
   return SAT2YAB1(alpha,tmp);
}

u32 FASTCALL Vdp2ColorRamGetColorCM01SC3(vdp2draw_struct * info, u32 colorindex, int alpha, u8 lowdot )
{
   u32 tmp;
   colorindex <<= 1;
   tmp = T2ReadWord(Vdp2ColorRam, colorindex & 0xFFF);
   if( ((tmp & 0x8000) == 0) )
   {
      return SAT2YAB1(0xFF,tmp);
   }
   return SAT2YAB1(alpha,tmp);
}

u32 FASTCALL Vdp2ColorRamGetColorCM2(vdp2draw_struct * info, u32 colorindex, int alpha, u8 lowdot )
{
   u32 tmp1, tmp2;
   colorindex <<= 2;
   colorindex &= 0xFFF;
   tmp1 = T2ReadWord(Vdp2ColorRam, colorindex);
   tmp2 = T2ReadWord(Vdp2ColorRam, colorindex+2);
   return SAT2YAB2(alpha, tmp1, tmp2);   
}

static int Vdp2SetGetColor( vdp2draw_struct * info )
{
   switch(Vdp2Internal.ColorMode)
   {
      case 0:
      case 1:
         switch( info->specialcolormode )
         {
         case 0:
            info->Vdp2ColorRamGetColor = (Vdp2ColorRamGetColor_func) Vdp2ColorRamGetColorCM01SC0;
            break;
         case 1:
            info->Vdp2ColorRamGetColor = (Vdp2ColorRamGetColor_func) Vdp2ColorRamGetColorCM01SC1;
            break;
         case 2:
            info->Vdp2ColorRamGetColor = (Vdp2ColorRamGetColor_func) Vdp2ColorRamGetColorCM01SC2;
            break;
         case 3:
            info->Vdp2ColorRamGetColor = (Vdp2ColorRamGetColor_func) Vdp2ColorRamGetColorCM01SC3;
            break;
         default:             
            info->Vdp2ColorRamGetColor = (Vdp2ColorRamGetColor_func) Vdp2ColorRamGetColorCM01SC0;
            break;
         }
         break;
      case 2:
         info->Vdp2ColorRamGetColor = (Vdp2ColorRamGetColor_func) Vdp2ColorRamGetColorCM2;
         break;
      default: 
         info->Vdp2ColorRamGetColor = (Vdp2ColorRamGetColor_func) Vdp2ColorRamGetColorCM01SC0;
         break;
   }   
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
// Window

static void Vdp2GenerateWindowInfo(void)
{
    int i;
    int HShift;
    int v = 0;
    u32 LineWinAddr;

    // Is there BG uses Window0?
    if( (fixVdp2Regs->WCTLA & 0X2) || (fixVdp2Regs->WCTLA & 0X200) || (fixVdp2Regs->WCTLB & 0X2) || (fixVdp2Regs->WCTLB & 0X200) ||
        (fixVdp2Regs->WCTLC & 0X2) || (fixVdp2Regs->WCTLC & 0X200) || (fixVdp2Regs->WCTLD & 0X2) || (fixVdp2Regs->WCTLD & 0X200) || (fixVdp2Regs->RPMD == 0X03) )
    {

        // resize to fit resolusion
        if( m_vWindinfo0_size != vdp2height )
        {
            if(m_vWindinfo0 != NULL) free(m_vWindinfo0);
            m_vWindinfo0 = (vdp2WindowInfo*)malloc(sizeof(vdp2WindowInfo)*(vdp2height+8));

            for( i=0; i<vdp2height; i++ )
            {
               m_vWindinfo0[i].WinShowLine = 1; 
               m_vWindinfo0[i].WinHStart   = 0;
               m_vWindinfo0[i].WinHEnd     = 1024;
            }

            m_vWindinfo0_size = vdp2height;
            m_b0WindowChg = 1;
        }

        HShift = 0;
        if( vdp2width>=640 ) HShift = 0; else HShift = 1;


        // Line Table mode
        if( (fixVdp2Regs->LWTA0.part.U & 0x8000) )
        {
            int preHStart = -1;
            int preHEnd = -1;
            
            // start address
            LineWinAddr = (u32)((( (fixVdp2Regs->LWTA0.part.U & 0x07) << 15) | (fixVdp2Regs->LWTA0.part.L >> 1) ) << 2);
            _Ygl->win0_vertexcnt = 0;
            
            for( v = 0; v < vdp2height; v++ )
            {
                if( v < fixVdp2Regs->WPSY0 || v > fixVdp2Regs->WPEY0 )
                {
                    if( m_vWindinfo0[v].WinShowLine ) m_b0WindowChg = 1;
                    m_vWindinfo0[v].WinShowLine = 0;

                }else{
                    short HStart = Vdp2RamReadWord(LineWinAddr + (v << 2) );
                    short HEnd   = Vdp2RamReadWord(LineWinAddr + (v << 2) + 2);

                    if( HStart < HEnd )
                    {
                        HStart >>= HShift;
                        HEnd   >>= HShift;

                        if( !( m_vWindinfo0[v].WinHStart == HStart && m_vWindinfo0[v].WinHEnd == HEnd ) )
                        {
                            m_b0WindowChg = 1;
                        }

                        m_vWindinfo0[v].WinHStart = HStart;
                        m_vWindinfo0[v].WinHEnd   = HEnd;
                        m_vWindinfo0[v].WinShowLine = 1;

                    }else{
                        if( m_vWindinfo0[v].WinShowLine ) m_b0WindowChg = 1;
                        m_vWindinfo0[v].WinHStart = 0;
                        m_vWindinfo0[v].WinHEnd   = 0;
                        m_vWindinfo0[v].WinShowLine = 0;
                    
                    }

          if (v == fixVdp2Regs->WPSY0)
          {
            _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 0] = HStart;
            _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 1] = v;
            _Ygl->win0_vertexcnt++;
            _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 0] = HEnd+1;
            _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 1] = v;
            _Ygl->win0_vertexcnt++;

          }
          else if ((HStart != preHStart || HEnd != preHEnd) || v == (fixVdp2Regs->WPEY0 - 1))
          {
            if ((v - 1) != _Ygl->win0v[(_Ygl->win0_vertexcnt - 1) * 2 + 1])
            {
              _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 0] = preHStart;
              _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 1] = v;
              _Ygl->win0_vertexcnt++;
              _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 0] = preHEnd+1;
              _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 1] = v;
              _Ygl->win0_vertexcnt++;
            }

            _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 0] = HStart;
            _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 1] = v;
            _Ygl->win0_vertexcnt++;
            _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 0] = HEnd+1;
            _Ygl->win0v[_Ygl->win0_vertexcnt * 2 + 1] = v;
            _Ygl->win0_vertexcnt++;
          }

          preHStart = HStart;
          preHEnd = HEnd;


                }
            }

        // Parameter Mode
        }else{

            // Check Update
            if( !( m_vWindinfo0[0].WinHStart == (fixVdp2Regs->WPSX0>>HShift) && m_vWindinfo0[0].WinHEnd == (fixVdp2Regs->WPEX0>>HShift) ) )
            {
                m_b0WindowChg = 1;
            }

      

            for( v = 0; v < vdp2height; v++ )
            {

                m_vWindinfo0[v].WinHStart = fixVdp2Regs->WPSX0 >> HShift;
                m_vWindinfo0[v].WinHEnd   = fixVdp2Regs->WPEX0 >> HShift;
                if( v < fixVdp2Regs->WPSY0 || v >= fixVdp2Regs->WPEY0 )
                {
                    if( m_vWindinfo0[v].WinShowLine ) m_b0WindowChg = 1;
                    m_vWindinfo0[v].WinShowLine = 0;
                }else{
                   if( m_vWindinfo0[v].WinShowLine == 0) m_b0WindowChg = 1;
                    m_vWindinfo0[v].WinShowLine = 1;
                }
            }
            
      if ( fixVdp2Regs->WPSX0 < fixVdp2Regs->WPEX0 ) {
        _Ygl->win0v[0] = fixVdp2Regs->WPSX0 >> HShift;
        _Ygl->win0v[1] = fixVdp2Regs->WPSY0;
        _Ygl->win0v[2] = (fixVdp2Regs->WPEX0 >> HShift) + 1;
        _Ygl->win0v[3] = fixVdp2Regs->WPSY0;
        _Ygl->win0v[4] = fixVdp2Regs->WPSX0 >> HShift;
        _Ygl->win0v[5] = fixVdp2Regs->WPEY0 + 1;
        _Ygl->win0v[6] = (fixVdp2Regs->WPEX0 >> HShift) + 1;
        _Ygl->win0v[7] = fixVdp2Regs->WPEY0 + 1;
      }
      else{
        _Ygl->win0v[0] = 0;
        _Ygl->win0v[1] = 0;
        _Ygl->win0v[2] = 0;
        _Ygl->win0v[3] = 0;
        _Ygl->win0v[4] = 0;
        _Ygl->win0v[5] = 0;
        _Ygl->win0v[6] = 0;
        _Ygl->win0v[7] = 0;
      }
            _Ygl->win0_vertexcnt = 4;

        }
               
    // there is no Window BG
    }else{
       if( m_vWindinfo0_size != 0 )
       {
          m_b0WindowChg = 1;
       }

       
        if( m_vWindinfo0 != NULL )
        {
            free(m_vWindinfo0);
            m_vWindinfo0 = NULL;
        }
        m_vWindinfo0_size = 0;
        _Ygl->win0_vertexcnt = 0;

    }


    // Is there BG uses Window1?
    if( (fixVdp2Regs->WCTLA & 0x8) || (fixVdp2Regs->WCTLA & 0x800) || (fixVdp2Regs->WCTLB & 0x8) || (fixVdp2Regs->WCTLB & 0x800) ||
        (fixVdp2Regs->WCTLC & 0x8) || (fixVdp2Regs->WCTLC & 0x800) || (fixVdp2Regs->WCTLD & 0x8) || (fixVdp2Regs->WCTLD & 0x800)  )
    {

        // resize to fit resolution
        if( m_vWindinfo1_size != vdp2height )
        {
            if(m_vWindinfo1 != NULL) free(m_vWindinfo1);
            m_vWindinfo1 = (vdp2WindowInfo*)malloc(sizeof(vdp2WindowInfo)*vdp2height);

            for( i=0; i<vdp2height; i++ )
            {
               m_vWindinfo1[i].WinShowLine = 1; 
               m_vWindinfo1[i].WinHStart   = 0;
               m_vWindinfo1[i].WinHEnd     = 1024;
            }

            m_vWindinfo1_size = vdp2height;
            m_b1WindowChg = 1;
        }

        if( vdp2width>=640 ) HShift = 0; else HShift = 1;


        // LineTable mode
        if( (fixVdp2Regs->LWTA1.part.U & 0x8000) )
        {
            int preHStart = -1;
            int preHEnd = -1;
           
            _Ygl->win1_vertexcnt = 0;
            // start address for Window table
            LineWinAddr = (u32)((( (fixVdp2Regs->LWTA1.part.U & 0x07) << 15) | (fixVdp2Regs->LWTA1.part.L >> 1) ) << 2);
            
            for( v = 0; v < vdp2height; v++ )
            {
                if( v < fixVdp2Regs->WPSY1 || v > fixVdp2Regs->WPEY1 )
                {
                    if( m_vWindinfo1[v].WinShowLine ) m_b1WindowChg = 1;
                    m_vWindinfo1[v].WinShowLine = 0;
                }else{
                    short HStart = Vdp2RamReadWord(LineWinAddr + (v << 2) );
                    short HEnd   = Vdp2RamReadWord(LineWinAddr + (v << 2) + 2);
                    if( HStart < HEnd )
                    {
                        HStart >>= HShift;
                        HEnd   >>= HShift;

                        if( !( m_vWindinfo1[v].WinHStart == HStart && m_vWindinfo1[v].WinHEnd == HEnd ) )
                        {
                            m_b1WindowChg = 1;
                        }

                        m_vWindinfo1[v].WinHStart = HStart;
                        m_vWindinfo1[v].WinHEnd   = HEnd;
                        m_vWindinfo1[v].WinShowLine = 1;

                    }else{
                        if( m_vWindinfo1[v].WinShowLine ) m_b1WindowChg = 1;
                        m_vWindinfo1[v].WinShowLine = 0;
                    }

                        if( v == fixVdp2Regs->WPSY1  )
                        {
                           _Ygl->win1v[_Ygl->win1_vertexcnt*2+0]= HStart;
                           _Ygl->win1v[_Ygl->win1_vertexcnt*2+1]= v;                    
                           _Ygl->win1_vertexcnt++;
                           _Ygl->win1v[_Ygl->win1_vertexcnt*2+0]= HEnd+1;
                           _Ygl->win1v[_Ygl->win1_vertexcnt*2+1]= v; 
                           _Ygl->win1_vertexcnt++;
                           
                        }else if( ( HStart != preHStart || HEnd != preHEnd) || v == (fixVdp2Regs->WPEY1-1) )
                        {
                           if( (v-1) != _Ygl->win1v[(_Ygl->win1_vertexcnt-1)*2+1] )
                           {
                              _Ygl->win1v[_Ygl->win1_vertexcnt*2+0]= preHStart;
                              _Ygl->win1v[_Ygl->win1_vertexcnt*2+1]= v;                    
                              _Ygl->win1_vertexcnt++;
                              _Ygl->win1v[_Ygl->win1_vertexcnt*2+0]= preHEnd+1;
                              _Ygl->win1v[_Ygl->win1_vertexcnt*2+1]= v; 
                              _Ygl->win1_vertexcnt++;                           
                           }
                           
                           _Ygl->win1v[_Ygl->win1_vertexcnt*2+0]= HStart;
                           _Ygl->win1v[_Ygl->win1_vertexcnt*2+1]= v;                    
                           _Ygl->win1_vertexcnt++;
                           _Ygl->win1v[_Ygl->win1_vertexcnt*2+0]= HEnd+1;
                           _Ygl->win1v[_Ygl->win1_vertexcnt*2+1]= v; 
                           _Ygl->win1_vertexcnt++;                           
                        }
                        
                        preHStart = HStart;
                        preHEnd = HEnd;
                    
                }
            }

        // parameter mode
        }else{

            // check update
            if( !( m_vWindinfo1[0].WinHStart == (fixVdp2Regs->WPSX1>>HShift) && m_vWindinfo1[0].WinHEnd == (fixVdp2Regs->WPEX1>>HShift) ) )
            {
                m_b1WindowChg = 1;
            }

            for( v = 0; v < vdp2height; v++ )
            {
                m_vWindinfo1[v].WinHStart = fixVdp2Regs->WPSX1 >> HShift;
                m_vWindinfo1[v].WinHEnd   = fixVdp2Regs->WPEX1 >> HShift;
                if( v < fixVdp2Regs->WPSY1 || v > fixVdp2Regs->WPEY1 )
                {
                    if( m_vWindinfo1[v].WinShowLine ) m_b1WindowChg = 1;
                    m_vWindinfo1[v].WinShowLine = 0;
                }else{
                   if( m_vWindinfo1[v].WinShowLine == 0) m_b1WindowChg = 1;
                    m_vWindinfo1[v].WinShowLine = 1;
                }
            }
            
      if (fixVdp2Regs->WPSX1 < fixVdp2Regs->WPEX1) {
        _Ygl->win1v[0] = fixVdp2Regs->WPSX1 >> HShift;
        _Ygl->win1v[1] = fixVdp2Regs->WPSY1;
        _Ygl->win1v[2] = (fixVdp2Regs->WPEX1 >> HShift) + 1;
        _Ygl->win1v[3] = fixVdp2Regs->WPSY1;
        _Ygl->win1v[4] = fixVdp2Regs->WPSX1 >> HShift;
        _Ygl->win1v[5] = fixVdp2Regs->WPEY1 + 1;
        _Ygl->win1v[6] = (fixVdp2Regs->WPEX1 >> HShift) + 1;
        _Ygl->win1v[7] = fixVdp2Regs->WPEY1 + 1;
      }
      else{
        _Ygl->win1v[0] = 0;
        _Ygl->win1v[1] = 0;
        _Ygl->win1v[2] = 0;
        _Ygl->win1v[3] = 0;
        _Ygl->win1v[4] = 0;
        _Ygl->win1v[5] = 0;
        _Ygl->win1v[6] = 0;
        _Ygl->win1v[7] = 0;
      }
            _Ygl->win1_vertexcnt = 4;            

        }

    // no BG uses Window1
    }else{
       
       if( m_vWindinfo1_size != 0 )
       {
          m_b1WindowChg = 1;
       }

        if( m_vWindinfo1 != NULL )
        {
             free(m_vWindinfo1);
            m_vWindinfo1 = NULL;
        }
        m_vWindinfo1_size = 0;
        _Ygl->win1_vertexcnt = 0;
    }
    
   if( m_b1WindowChg || m_b0WindowChg )
   {
       YglNeedToUpdateWindow();
       m_b0WindowChg = 0;
       m_b1WindowChg = 0;      
   }

}

// 0 .. outside,1 .. inside
static INLINE int Vdp2CheckWindow(vdp2draw_struct *info, int x, int y, int area, vdp2WindowInfo * vWindinfo )
{
    if( y<0 ) return 0;
    if( y>= vdp2height ) return 0;
   // inside
    if( area == 1 )
    {
        if( vWindinfo[y].WinShowLine == 0  ) return 0;
        if( x > vWindinfo[y].WinHStart && x < vWindinfo[y].WinHEnd )
        {
            return 1;
        }else{
            return 0;
        }
    // outside
    }else{
        if( vWindinfo[y].WinShowLine == 0  ) return 1;
        if( x < vWindinfo[y].WinHStart ) return 1;
        if( x > vWindinfo[y].WinHEnd ) return 1;
        return 0;
    }
    return 0;
}

// 0 .. outside,1 .. inside 
static int FASTCALL Vdp2CheckWindowDot(vdp2draw_struct *info, int x, int y )
{
    if( info->bEnWin0 != 0 &&  info->bEnWin1 == 0 )
    {
    if (m_vWindinfo0==NULL) Vdp2GenerateWindowInfo();
        return Vdp2CheckWindow(info, x, y, info->WindowArea0, m_vWindinfo0 );
    }else if( info->bEnWin0 == 0 &&  info->bEnWin1 != 0 )
    {
    if (m_vWindinfo1 == NULL) Vdp2GenerateWindowInfo();
        return Vdp2CheckWindow(info, x, y, info->WindowArea1, m_vWindinfo1 );
    }else if( info->bEnWin0 != 0 &&  info->bEnWin1 != 0 )
    {
    if (m_vWindinfo0 == NULL || m_vWindinfo1 == NULL) Vdp2GenerateWindowInfo();
        if( info->LogicWin == 0 )
        {
            return (Vdp2CheckWindow(info, x, y, info->WindowArea0, m_vWindinfo0 )&
                    Vdp2CheckWindow(info, x, y, info->WindowArea1, m_vWindinfo1 ));
        }else{
            return (Vdp2CheckWindow(info, x, y, info->WindowArea0, m_vWindinfo0 )|
                    Vdp2CheckWindow(info, x, y, info->WindowArea1, m_vWindinfo1 ));
        }
    }
    return 0;
}

// 0 .. all outsize, 1~3 .. partly inside, 4.. all inside 
static int FASTCALL Vdp2CheckWindowRange(vdp2draw_struct *info, int x, int y, int w, int h )
{
    int rtn=0;
    
    if( info->bEnWin0 != 0 &&  info->bEnWin1 == 0 )
    {
        rtn += Vdp2CheckWindow(info, x, y, info->WindowArea0, m_vWindinfo0 );
        rtn += Vdp2CheckWindow(info, x+w, y, info->WindowArea0, m_vWindinfo0 );
        rtn += Vdp2CheckWindow(info, x+w, y+h, info->WindowArea0, m_vWindinfo0 );
        rtn += Vdp2CheckWindow(info, x, y+h, info->WindowArea0, m_vWindinfo0 );
        return rtn;
    }else if( info->bEnWin0 == 0 &&  info->bEnWin1 != 0 )
    {
        rtn += Vdp2CheckWindow(info, x, y, info->WindowArea1, m_vWindinfo1 );
        rtn += Vdp2CheckWindow(info, x+w, y, info->WindowArea1, m_vWindinfo1 );
        rtn += Vdp2CheckWindow(info, x+w, y+h, info->WindowArea1, m_vWindinfo1 );
        rtn += Vdp2CheckWindow(info, x, y+h, info->WindowArea1, m_vWindinfo1 );
        return rtn;       
    }else if( info->bEnWin0 != 0 &&  info->bEnWin1 != 0 )
    {
        if( info->LogicWin == 0 )
        {
            rtn += (Vdp2CheckWindow(info, x, y, info->WindowArea0, m_vWindinfo0 ) & 
                    Vdp2CheckWindow(info, x, y, info->WindowArea1, m_vWindinfo1 ) );
            rtn += (Vdp2CheckWindow(info, x+w, y, info->WindowArea0, m_vWindinfo0 )& 
                    Vdp2CheckWindow(info, x+w, y, info->WindowArea1, m_vWindinfo1 ) );
            rtn += (Vdp2CheckWindow(info, x+w, y+h, info->WindowArea0, m_vWindinfo0 )& 
                    Vdp2CheckWindow(info, x+w, y+h, info->WindowArea1, m_vWindinfo1 ) );
            rtn += (Vdp2CheckWindow(info, x, y+h, info->WindowArea0, m_vWindinfo0 ) & 
                    Vdp2CheckWindow(info, x, y+h, info->WindowArea1, m_vWindinfo1 ) );
            return rtn;
        }else{
            rtn += (Vdp2CheckWindow(info, x, y, info->WindowArea0, m_vWindinfo0 ) | 
                    Vdp2CheckWindow(info, x, y, info->WindowArea1, m_vWindinfo1 ) );
            rtn += (Vdp2CheckWindow(info, x+w, y, info->WindowArea0, m_vWindinfo0 ) | 
                    Vdp2CheckWindow(info, x+w, y, info->WindowArea1, m_vWindinfo1 ) );
            rtn += (Vdp2CheckWindow(info, x+w, y+h, info->WindowArea0, m_vWindinfo0 ) | 
                    Vdp2CheckWindow(info, x+w, y+h, info->WindowArea1, m_vWindinfo1 ) );
            rtn += (Vdp2CheckWindow(info, x, y+h, info->WindowArea0, m_vWindinfo0 ) | 
                    Vdp2CheckWindow(info, x, y+h, info->WindowArea1, m_vWindinfo1 ) );
            return rtn;       
        }  
    }
    return 0;
}

void Vdp2GenLineinfo( vdp2draw_struct *info )
{
   int bound = 0;
   int i;
   u16 val1,val2;
   int index = 0;
   int lineindex = 0;
   if( info->lineinc == 0 || info->islinescroll == 0 ) return;
   
   if( VDPLINE_SY(info->islinescroll)) bound += 0x04;
   if( VDPLINE_SX(info->islinescroll)) bound += 0x04;
   if( VDPLINE_SZ(info->islinescroll)) bound += 0x04;   
   
   for( i = 0; i < vdp2height; i += info->lineinc )
   {
      index = 0;
      if( VDPLINE_SX(info->islinescroll))
      {
      info->lineinfo[lineindex].LineScrollValH = T1ReadWord(Vdp2Ram, info->linescrolltbl + (i / info->lineinc)*bound);
      if ((info->lineinfo[lineindex].LineScrollValH & 0x400)) info->lineinfo[lineindex].LineScrollValH |= 0xF800; else info->lineinfo[lineindex].LineScrollValH &= 0x07FF;
         index += 4;
      }else{
      info->lineinfo[lineindex].LineScrollValH = 0;
      }

      if( VDPLINE_SY(info->islinescroll))
      {
      info->lineinfo[lineindex].LineScrollValV = T1ReadWord(Vdp2Ram, info->linescrolltbl + (i / info->lineinc)*bound + index);
      if ((info->lineinfo[lineindex].LineScrollValV & 0x400)) info->lineinfo[lineindex].LineScrollValV |= 0xF800; else info->lineinfo[lineindex].LineScrollValV &= 0x07FF;
         index += 4;
      }else{
      info->lineinfo[lineindex].LineScrollValV = 0;
      }

      if( VDPLINE_SZ(info->islinescroll))
      {
         val1=T1ReadWord(Vdp2Ram, info->linescrolltbl+(i/info->lineinc)*bound+index);
         val2=T1ReadWord(Vdp2Ram, info->linescrolltbl+(i/info->lineinc)*bound+index+2);
         //info->lineinfo[i].CoordinateIncH = (float)( (int)((val1) & 0x07) + (float)( (val2) >> 8) / 255.0f );
     info->lineinfo[lineindex].CoordinateIncH = (((int)((val1)& 0x07) << 8) | (int)((val2) >> 8));
     if (info->lineinfo[lineindex].CoordinateIncH == 0){
       info->lineinfo[lineindex].CoordinateIncH = 0x0100;
     }
         index += 4;
      }else{
      info->lineinfo[lineindex].CoordinateIncH = 0x0100;
      }

    lineindex++;
   }
}


static INLINE u32 Vdp2GetPixel4bpp(vdp2draw_struct *info, u32 addr, YglTexture *texture ){

  u32 color;

  u16 dot = T1ReadWord(Vdp2Ram, addr & 0x7FFFF);
  
  if (!(dot & 0xF000) && info->transparencyenable) color = 0x00000000;
  else color = info->Vdp2ColorRamGetColor(info, info->coloroffset + ((info->paladdr << 4) | ((dot & 0xF000) >> 12)), info->alpha, (dot & 0xF000)>>12);
  *texture->textdata++ = color;

  if (!(dot & 0xF00) && info->transparencyenable) color = 0x00000000;
  else color = info->Vdp2ColorRamGetColor(info, info->coloroffset + ((info->paladdr << 4) | ((dot & 0xF00) >> 8)), info->alpha, (dot & 0xF00)>>8);
  *texture->textdata++ = color;

  if (!(dot & 0xF0) && info->transparencyenable) color = 0x00000000;
  else color = info->Vdp2ColorRamGetColor(info, info->coloroffset + ((info->paladdr << 4) | ((dot & 0xF0) >> 4)), info->alpha, (dot & 0xF0)>>4);
  *texture->textdata++ = color;

  if (!(dot & 0xF) && info->transparencyenable) color = 0x00000000;
  else color = info->Vdp2ColorRamGetColor(info, info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)), info->alpha, (dot & 0xF));
  *texture->textdata++ = color;
  return 0;
}

static INLINE u32 Vdp2GetPixel8bpp(vdp2draw_struct *info, u32 addr, YglTexture *texture){

  u32 color;
  u16 dot = T1ReadWord(Vdp2Ram, addr & 0x7FFFF);

  u32 alpha = info->alpha;

  if (!(dot & 0xFF00) && info->transparencyenable) color = 0x00000000;
  
  else {
    color = info->Vdp2ColorRamGetColor(info, info->coloroffset + ((info->paladdr << 4) | ((dot & 0xFF00) >> 8)), alpha, (dot & 0x0F00)>>8);
  }
  *texture->textdata++ = color;

  alpha = info->alpha;
  if (!(dot & 0xFF) && info->transparencyenable) color = 0x00000000;
  else  {
    color = info->Vdp2ColorRamGetColor(info, info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF)), alpha, (dot & 0x0F));
  }
  *texture->textdata++ = color;
  return 0;
}


static INLINE u32 Vdp2GetPixel16bpp(vdp2draw_struct *info, u32 addr){
  u32 color;
  u16 dot = T1ReadWord(Vdp2Ram, addr & 0x7FFFF);
  if ((dot == 0) && info->transparencyenable) color = 0x00000000;
  else color = info->Vdp2ColorRamGetColor(info, info->coloroffset + dot, info->alpha, (dot & 0x0F));
  return color;
}

static INLINE u32 Vdp2GetPixel16bppbmp(vdp2draw_struct *info, u32 addr){
  u32 color;
  u16 dot = T1ReadWord(Vdp2Ram, addr & 0x7FFFF);
  if (!(dot & 0x8000) && info->transparencyenable) color = 0x00000000;
  else color = SAT2YAB1(0xFF, dot);
  return color;
}

static INLINE u32 Vdp2GetPixel32bppbmp(vdp2draw_struct *info, u32 addr){
  u32 color;
  u16 dot1, dot2;
  dot1 = T1ReadWord(Vdp2Ram, addr & 0x7FFFF);
  dot2 = T1ReadWord(Vdp2Ram, addr+2 & 0x7FFFF);
  if (!(dot1 & 0x8000) && info->transparencyenable) color = 0x00000000;
  else color = SAT2YAB2(info->alpha, dot1, dot2);
  return color;
}

static void FASTCALL Vdp2DrawCell(vdp2draw_struct *info, YglTexture *texture)
{
  int i, j;

  switch (info->colornumber)
  {
  case 0: // 4 BPP
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j += 4)
      {
        Vdp2GetPixel4bpp(info, info->charaddr, texture);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 1: // 8 BPP
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j += 2)
      {
        Vdp2GetPixel8bpp(info, info->charaddr, texture);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 2: // 16 BPP(palette)
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j++)
      {
        *texture->textdata++ = Vdp2GetPixel16bpp(info, info->charaddr);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 3: // 16 BPP(RGB)
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j++)
      {
        *texture->textdata++ = Vdp2GetPixel16bppbmp(info, info->charaddr);
        info->charaddr += 2;
      }
      texture->textdata += texture->w;
    }
    break;
  case 4: // 32 BPP
    for (i = 0; i < info->cellh; i++)
    {
      for (j = 0; j < info->cellw; j++)
      {
        *texture->textdata++ = Vdp2GetPixel32bppbmp(info, info->charaddr);
        info->charaddr += 4;
      }
      texture->textdata += texture->w;
    }
    break;
  }
}

static void FASTCALL Vdp2DrawBitmapLineScroll(vdp2draw_struct *info, YglTexture *texture)
{
  int i, j;
  int height = vdp2height;

  for (i = 0; i < height; i++)
  {
    int sh, sv;
    u32 baseaddr;
    vdp2Lineinfo * line;
    baseaddr = (u32)info->charaddr;
    line = &(info->lineinfo[i*info->lineinc]);

    if (VDPLINE_SX(info->islinescroll))
      sh = line->LineScrollValH + info->sh;
    else
      sh = info->sh;

    if (VDPLINE_SY(info->islinescroll))
      sv = line->LineScrollValV + info->sv;
    else
      sv = i + info->sv;

    sv &= (info->cellh - 1);
    sh &= (info->cellw - 1);
    if (line->LineScrollValH >= 0 && line->LineScrollValH < sh) {
      sv -= 1;
    }

    switch (info->colornumber){
    case 0:
      baseaddr += ((sh + sv * (info->cellw >> 2)) << 1);
      for (j = 0; j < vdp2width; j += 4)
      {
        Vdp2GetPixel4bpp(info, baseaddr, texture);
        baseaddr += 2;
      }
      break;
    case 1:
      baseaddr += sh + sv * info->cellw;
      for (j = 0; j < vdp2width; j += 2)
      {
        Vdp2GetPixel8bpp(info, baseaddr, texture);
        baseaddr += 2;
      }
      break;
    case 2:
      baseaddr += ((sh + sv * info->cellw) << 1);
      for (j = 0; j < vdp2width; j++)
      {
        *texture->textdata++ = Vdp2GetPixel16bpp(info, baseaddr);
        baseaddr += 2;

      }
      break;
    case 3:
      baseaddr += ((sh + sv * info->cellw) << 1);
      for (j = 0; j < vdp2width; j++)
      {
        *texture->textdata++ = Vdp2GetPixel16bppbmp(info, baseaddr);
        baseaddr += 2;
      }
      break;
    case 4:
      baseaddr += ((sh + sv * info->cellw) << 2);
      for (j = 0; j < vdp2width; j++)
      {
        //if (info->isverticalscroll){
        //	sv += T1ReadLong(Vdp2Ram, info->verticalscrolltbl+(j>>3) ) >> 16;
        //}
        *texture->textdata++ = Vdp2GetPixel32bppbmp(info, baseaddr);
        baseaddr += 4;
      }
      break;
    }
    texture->textdata += texture->w;
  }
}


static void FASTCALL Vdp2DrawBitmapCoordinateInc(vdp2draw_struct *info, YglTexture *texture)
{
  u32 color;
  int i, j;

  int incv = 1.0 / info->coordincy*256.0;
  int inch = 1.0 / info->coordincx*256.0;

  int lineinc = 1;
  int linestart = 0;

  int height = vdp2height;
  if ((vdp1_interlace != 0) || (height >= 448)) {
    lineinc=2;
  }

  if (vdp1_interlace != 0) {
    linestart = vdp1_interlace -1;
  }

  for (i = linestart; i < lineinc*height; i+=lineinc)
  {
    int sh, sv;
    int v;
    u32 baseaddr;
    vdp2Lineinfo * line;
    baseaddr = (u32)info->charaddr;
    line = &(info->lineinfo[i*info->lineinc]);

    v = (i*incv)>>8;
    if (VDPLINE_SZ(info->islinescroll))
      inch = line->CoordinateIncH;

    if (VDPLINE_SX(info->islinescroll))
      sh = info->sh + line->LineScrollValH;
    else
      sh = info->sh;

    if (VDPLINE_SY(info->islinescroll))
      sv = info->sv + line->LineScrollValV;
    else
      sv = v + info->sv;

    sh &= (info->cellw - 1);
    sv &= (info->cellh - 1);

    switch (info->colornumber){
    case 0:
      baseaddr = baseaddr + (sh>>1) + (sv * (info->cellw >> 1));
      for (j = 0; j < vdp2width; j++)
      {
        u32 h = ((j*inch)>>8);
        u32 addr = (baseaddr + (h >> 1));
        if (addr >= 0x80000){
          *texture->textdata++ = 0x0000;
        }else{
          u8 dot = T1ReadByte(Vdp2Ram, addr);
          if (h & 0x01){
            if (!(dot & 0x0F) && info->transparencyenable) color = 0x00000000;
            else color = info->Vdp2ColorRamGetColor(info, info->coloroffset + ((info->paladdr << 4) | ((dot & 0x0F) >> 0)), info->alpha, (dot & 0x0F));
            *texture->textdata++ = color;
          }
          else{
            if (!(dot & 0xF0) && info->transparencyenable) color = 0x00000000;
            else color = info->Vdp2ColorRamGetColor(info, info->coloroffset + ((info->paladdr << 4) | ((dot & 0xF0) >> 4)), info->alpha, (dot & 0xF0)>>4);
            *texture->textdata++ = color;
          }
        }

      }
      break;
    case 1:
      baseaddr += sh + sv * info->cellw;
      for (j = 0; j < vdp2width ; j++)
      {
        int h = ((j*inch) >> 8);
        u8 dot = T1ReadByte(Vdp2Ram, baseaddr+h);
        if (!dot && info->transparencyenable) color = 0x00000000;
        else color = info->Vdp2ColorRamGetColor(info, info->coloroffset + ((info->paladdr << 4) | dot ), info->alpha, (dot & 0xF));
        *texture->textdata++ = color;
      }

      break;
    case 2:
      baseaddr += ((sh + sv * info->cellw) << 1);
      for (j = 0; j < vdp2width; j++)
      {
        int h = ((j*inch) >> 8) << 1;
        *texture->textdata++ = Vdp2GetPixel16bpp(info, baseaddr+h);

      }
      break;
    case 3:
      baseaddr += ((sh + sv * info->cellw) << 1);
      for (j = 0; j < vdp2width; j++)
      {
        int h = ((j*inch) >> 8) << 1;
        *texture->textdata++ = Vdp2GetPixel16bppbmp(info, baseaddr+h);
      }
      break;
    case 4:
      baseaddr += ((sh + sv * info->cellw) << 2);
      for (j = 0; j < vdp2width; j++)
      {
        int h = (j*inch >> 8) << 2;
        *texture->textdata++ = Vdp2GetPixel32bppbmp(info, baseaddr+h);
      }
      break;
    }
    texture->textdata += texture->w;
  }
}

static void Vdp2DrawPatternPos(vdp2draw_struct *info, YglTexture *texture, int x, int y, int cx, int cy  )
{
  u64 cacheaddr = ((u32)(info->alpha >> 3) << 27) | 
    (info->paladdr << 20) | info->charaddr | info->transparencyenable | 
    ((info->patternpixelwh >> 4) << 1) | (((u64)(info->coloroffset >> 8) & 0x07) << 32);
  
  YglCache c;
  vdp2draw_struct tile;
  int winmode = 0;
  tile.dst = 0;
  tile.uclipmode = 0;
  tile.blendmode = info->blendmode;
  tile.linescreen = info->linescreen;
  tile.mosaicxmask = info->mosaicxmask;
  tile.mosaicymask = info->mosaicymask;
  tile.bEnWin0 = info->bEnWin0;
  tile.WindowArea0 = info->WindowArea0;
  tile.bEnWin1 = info->bEnWin1;
  tile.WindowArea1 = info->WindowArea1;
  tile.LogicWin = info->LogicWin;
  tile.lineTexture = info->lineTexture;
  tile.id = info->id;

  tile.cellw = tile.cellh = info->patternpixelwh;
  tile.flipfunction = info->flipfunction;

  if (info->specialprimode == 1)
    tile.priority = (info->priority & 0xFFFFFFFE) | info->specialfunction;
  else
    tile.priority = info->priority;

  tile.vertices[0] = x;
  tile.vertices[1] = y;
  tile.vertices[2] = (x + tile.cellw);
  tile.vertices[3] = y;
  tile.vertices[4] = (x + tile.cellh);
  tile.vertices[5] = (y + (float)info->lineinc);
  tile.vertices[6] = x;
  tile.vertices[7] = (y + (float)info->lineinc);

  // Screen culling
  //if (tile.vertices[0] >= vdp2width || tile.vertices[1] >= vdp2height || tile.vertices[2] < 0 || tile.vertices[5] < 0)
  //{
  //	return;
  //}

  if ((info->bEnWin0 != 0 || info->bEnWin1 != 0) && info->coordincx == 1.0f && info->coordincy == 1.0f )
  {                                                 // coordinate inc is not supported yet.
    winmode = Vdp2CheckWindowRange(info, x-cx, y-cy, tile.cellw, info->lineinc);
    if (winmode == 0) // all outside, no need to draw 
    {
      return;
    }
  }

  tile.cor = info->cor;
  tile.cog = info->cog;
  tile.cob = info->cob;


  if (1 == YglIsCached(_Ygl->texture_manager, cacheaddr, &c))
  {
    YglCachedQuadOffset(&tile, &c, cx, cy, info->coordincx, info->coordincy);
    return;
  }

  YglQuadOffset(&tile, texture, &c, cx, cy, info->coordincx, info->coordincy);
  YglCacheAdd(_Ygl->texture_manager, cacheaddr, &c);

  switch (info->patternwh)
  {
  case 1:
    Vdp2DrawCell(info, texture);
    break;
  case 2:
    texture->w += 8;
    Vdp2DrawCell(info, texture);
    texture->textdata -= (texture->w + 8) * 8 - 8;
    Vdp2DrawCell(info, texture);
    texture->textdata -= 8;
    Vdp2DrawCell(info, texture);
    texture->textdata -= (texture->w + 8) * 8 - 8;
    Vdp2DrawCell(info, texture);
    break;
  }
}


//////////////////////////////////////////////////////////////////////////////

static void Vdp2PatternAddr(vdp2draw_struct *info)
{
   switch(info->patterndatasize)
   {
      case 1:
      {
         u16 tmp = T1ReadWord(Vdp2Ram, info->addr);

         info->addr += 2;
         info->specialfunction = (info->supplementdata >> 9) & 0x1;
         info->specialcolorfunction = (info->supplementdata >> 8) & 0x1;

         switch(info->colornumber)
         {
            case 0: // in 16 colors
               info->paladdr = ((tmp & 0xF000) >> 12) | ((info->supplementdata & 0xE0) >> 1);
               break;
            default: // not in 16 colors
               info->paladdr = (tmp & 0x7000) >> 8;
               break;
         }

         switch(info->auxmode)
         {
            case 0:
               info->flipfunction = (tmp & 0xC00) >> 10;

               switch(info->patternwh)
               {
                  case 1:
                     info->charaddr = (tmp & 0x3FF) | ((info->supplementdata & 0x1F) << 10);
                     break;
                  case 2:
                     info->charaddr = ((tmp & 0x3FF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x1C) << 10);
                     break;
               }
               break;
            case 1:
               info->flipfunction = 0;

               switch(info->patternwh)
               {
                  case 1:
                     info->charaddr = (tmp & 0xFFF) | ((info->supplementdata & 0x1C) << 10);
                     break;
                  case 2:
                     info->charaddr = ((tmp & 0xFFF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x10) << 10);
                     break;
               }
               break;
         }

         break;
      }
      case 2: {
         u16 tmp1 = T1ReadWord(Vdp2Ram, info->addr);
         u16 tmp2 = T1ReadWord(Vdp2Ram, info->addr+2);
         info->addr += 4;
         info->charaddr = tmp2 & 0x7FFF;
         info->flipfunction = (tmp1 & 0xC000) >> 14;
         switch(info->colornumber) {
            case 0:
               info->paladdr = (tmp1 & 0x7F);
               break;
            default:
               info->paladdr = (tmp1 & 0x70);
               break;
         }
         info->specialfunction = (tmp1 & 0x2000) >> 13;
         info->specialcolorfunction = (tmp1 & 0x1000) >> 12;
         break;
      }
   }

   if (!(fixVdp2Regs->VRSIZE & 0x8000))
      info->charaddr &= 0x3FFF;

   info->charaddr *= 0x20; // thanks Runik
}


static void Vdp2PatternAddrPos(vdp2draw_struct *info, int planex, int x, int planey, int y )
{
  u32 addr = info->addr +
    (info->pagewh*info->pagewh*info->planew*planey +
    info->pagewh*info->pagewh*planex +
    info->pagewh*y +
    x)*info->patterndatasize*2;

  switch (info->patterndatasize)
  {
  case 1:
  {
    u16 tmp = T1ReadWord(Vdp2Ram, addr);

    info->specialfunction = (info->supplementdata >> 9) & 0x1;
    info->specialcolorfunction = (info->supplementdata >> 8) & 0x1;

    switch (info->colornumber)
    {
    case 0: // in 16 colors
      info->paladdr = ((tmp & 0xF000) >> 12) | ((info->supplementdata & 0xE0) >> 1);
      break;
    default: // not in 16 colors
      info->paladdr = (tmp & 0x7000) >> 8;
      break;
    }

    switch (info->auxmode)
    {
    case 0:
      info->flipfunction = (tmp & 0xC00) >> 10;

      switch (info->patternwh)
      {
      case 1:
        info->charaddr = (tmp & 0x3FF) | ((info->supplementdata & 0x1F) << 10);
        break;
      case 2:
        info->charaddr = ((tmp & 0x3FF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x1C) << 10);
        break;
      }
      break;
    case 1:
      info->flipfunction = 0;

      switch (info->patternwh)
      {
      case 1:
        info->charaddr = (tmp & 0xFFF) | ((info->supplementdata & 0x1C) << 10);
        break;
      case 2:
        info->charaddr = ((tmp & 0xFFF) << 2) | (info->supplementdata & 0x3) | ((info->supplementdata & 0x10) << 10);
        break;
      }
      break;
    }

    break;
  }
  case 2: {
    u16 tmp1 = T1ReadWord(Vdp2Ram, addr);
    u16 tmp2 = T1ReadWord(Vdp2Ram, addr+2);
    info->charaddr = tmp2 & 0x7FFF;
    info->flipfunction = (tmp1 & 0xC000) >> 14;
    switch (info->colornumber) {
    case 0:
      info->paladdr = (tmp1 & 0x7F);
      break;
    default:
      info->paladdr = (tmp1 & 0x70);
      break;
    }
    info->specialfunction = (tmp1 & 0x2000) >> 13;
    info->specialcolorfunction = (tmp1 & 0x1000) >> 12;
    break;
  }
  }

  if (!(fixVdp2Regs->VRSIZE & 0x8000))
    info->charaddr &= 0x3FFF;

  info->charaddr *= 0x20; // thanks Runik
}



//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawMapPerLine(vdp2draw_struct *info, YglTexture *texture){

  int lineindex = 0;

  int sx; //, sy;
  int mapx, mapy;
  int planex, planey;
  int pagex, pagey;
  int charx, chary;
    int dot_on_planey;
    int dot_on_pagey;
    int dot_on_planex;
    int dot_on_pagex;
    int h,v;
    const int planeh_shift = 9 + (info->planeh-1);
    const int planew_shift = 9 + (info->planew-1);
    const int plane_shift = 9;
    const int plane_mask = 0x1FF;
    const int page_shift = 9 - 7 + (64/info->pagewh);
    const int page_mask = 0x0f >> ((info->pagewh/32)-1);

  info->patternpixelwh = 8*info->patternwh;
  info->draww = (int)((float)vdp2width / info->coordincx);
  info->drawh = (int)((float)vdp2height / info->coordincy);
    
  int res_shift = 0;
  if (vdp2height >= 440) res_shift = 1;
  
  for (v = 0; v < info->drawh; v += info->lineinc){  // ToDo: info->coordincy
    int targetv = 0;
    sx = info->x + info->lineinfo[ (int)(lineindex*info->coordincy) ].LineScrollValH;
    if (VDPLINE_SY(info->islinescroll)) {
      targetv = info->y + info->lineinfo[(int)(lineindex*info->coordincy)].LineScrollValV;
    }else{
      targetv = info->y + v;
    }

    if (info->isverticalscroll)	{
      // this is *wrong*, vertical scroll use a different value per cell
      // info->verticalscrolltbl should be incremented by info->verticalscrollinc
      // each time there's a cell change and reseted at the end of the line...
      // or something like that :)
      targetv += T1ReadLong(Vdp2Ram, info->verticalscrolltbl) >> 16;
    }

    info->coordincx = info->lineinfo[(int)(lineindex*info->coordincy)].CoordinateIncH / 256.0f;
    info->coordincx = 1.0f / info->coordincx;
    if (info->coordincx < info->maxzoom) info->coordincx = info->maxzoom;
    info->draww = (int)((float)vdp2width / info->coordincx);

    // determine which chara shoud be used.
    //mapy   = (v+sy) / (512 * info->planeh);
    mapy = (targetv) >> planeh_shift;
    //int dot_on_planey = (v + sy) - mapy*(512 * info->planeh);
    dot_on_planey = (targetv)-(mapy << planeh_shift);
    mapy = mapy & 0x01;
    //planey = dot_on_planey / 512;
    planey = dot_on_planey >> plane_shift;
    //int dot_on_pagey = dot_on_planey - planey * 512;
        dot_on_pagey = dot_on_planey & plane_mask;
    planey = planey & (info->planeh - 1);
    //pagey = dot_on_pagey / (512 / info->pagewh);
    pagey = dot_on_pagey >> page_shift;
    //chary = dot_on_pagey - pagey*(512 / info->pagewh);
    chary = dot_on_pagey & page_mask;
    if (pagey < 0) pagey = info->pagewh - 1 + pagey;

    for (h = -info->patternpixelwh; h < info->draww + info->patternpixelwh; h += info->patternpixelwh){

      //mapx = (h + sx) / (512 * info->planew);
      mapx = (h + sx) >> planew_shift;
      //int dot_on_planex = (h + sx) - mapx*(512 * info->planew);
            dot_on_planex = (h + sx) - (mapx << planew_shift);
      mapx = mapx & 0x01;
      //planex = dot_on_planex / 512;
      planex = dot_on_planex >> plane_shift;
      //int dot_on_pagex = dot_on_planex - planex * 512;
            dot_on_pagex = dot_on_planex & plane_mask;
      planex = planex & (info->planew - 1);
      //pagex = dot_on_pagex / (512 / info->pagewh);
      pagex = dot_on_pagex >> page_shift;
      //charx = dot_on_pagex - pagex*(512 / info->pagewh);
      charx = dot_on_pagex & page_mask;
      if (pagex < 0) pagex = info->pagewh - 1 + pagex;

      info->PlaneAddr(info, info->mapwh * mapy + mapx, fixVdp2Regs);
      Vdp2PatternAddrPos(info, planex,pagex, planey,pagey);
      Vdp2DrawPatternPos(info, texture, h, v, charx, chary);

    }

    lineindex++;
  }

}

static void Vdp2DrawMapTest(vdp2draw_struct *info, YglTexture *texture){

  int lineindex = 0;

  int sx; //, sy;
  int mapx, mapy;
  int planex, planey;
  int pagex, pagey;
  int charx, chary;
  int dot_on_planey;
  int dot_on_pagey;
  int dot_on_planex;
  int dot_on_pagex;
  int h, v;
  int cell_count = 0;

  const int planeh_shift = 9 + (info->planeh - 1);
  const int planew_shift = 9 + (info->planew - 1);
  const int plane_shift = 9;
  const int plane_mask = 0x1FF;
  const int page_shift = 9 - 7 + (64 / info->pagewh);
  const int page_mask = 0x0f >> ((info->pagewh / 32) - 1);

  info->patternpixelwh = 8 * info->patternwh;
  info->draww = (int)((float)vdp2width / info->coordincx);
  info->drawh = (int)((float)vdp2height / info->coordincy);
  info->lineinc = info->patternpixelwh;

  //info->coordincx = 1.0f;

  for (v = -info->patternpixelwh; v < info->drawh + info->patternpixelwh; v += info->patternpixelwh){
    int targetv = 0;
    sx = info->x;

    if (!info->isverticalscroll) {
      targetv = info->y + v;
      // determine which chara shoud be used.
      //mapy   = (v+sy) / (512 * info->planeh);
      mapy = (targetv) >> planeh_shift;
      //int dot_on_planey = (v + sy) - mapy*(512 * info->planeh);
      dot_on_planey = (targetv)-(mapy << planeh_shift);
      mapy = mapy & 0x01;
      //planey = dot_on_planey / 512;
      planey = dot_on_planey >> plane_shift;
      //int dot_on_pagey = dot_on_planey - planey * 512;
      dot_on_pagey = dot_on_planey & plane_mask;
      planey = planey & (info->planeh - 1);
      //pagey = dot_on_pagey / (512 / info->pagewh);
      pagey = dot_on_pagey >> page_shift;
      //chary = dot_on_pagey - pagey*(512 / info->pagewh);
      chary = dot_on_pagey & page_mask;
      if (pagey < 0) pagey = info->pagewh - 1 + pagey;
    }
    else{
      cell_count = 0;
    }

    for (h = -info->patternpixelwh; h < info->draww + info->patternpixelwh; h += info->patternpixelwh){

      if (info->isverticalscroll)	{
        targetv = info->y + v + (T1ReadLong(Vdp2Ram, info->verticalscrolltbl + cell_count) >> 16);
        cell_count += info->verticalscrollinc;
        // determine which chara shoud be used.
        //mapy   = (v+sy) / (512 * info->planeh);
        mapy = (targetv) >> planeh_shift;
        //int dot_on_planey = (v + sy) - mapy*(512 * info->planeh);
        dot_on_planey = (targetv)-(mapy << planeh_shift);
        mapy = mapy & 0x01;
        //planey = dot_on_planey / 512;
        planey = dot_on_planey >> plane_shift;
        //int dot_on_pagey = dot_on_planey - planey * 512;
        dot_on_pagey = dot_on_planey & plane_mask;
        planey = planey & (info->planeh - 1);
        //pagey = dot_on_pagey / (512 / info->pagewh);
        pagey = dot_on_pagey >> page_shift;
        //chary = dot_on_pagey - pagey*(512 / info->pagewh);
        chary = dot_on_pagey & page_mask;
        if (pagey < 0) pagey = info->pagewh - 1 + pagey;
      }

      //mapx = (h + sx) / (512 * info->planew);
      mapx = (h + sx) >> planew_shift;
      //int dot_on_planex = (h + sx) - mapx*(512 * info->planew);
      dot_on_planex = (h + sx) - (mapx << planew_shift);
      mapx = mapx & 0x01;
      //planex = dot_on_planex / 512;
      planex = dot_on_planex >> plane_shift;
      //int dot_on_pagex = dot_on_planex - planex * 512;
      dot_on_pagex = dot_on_planex & plane_mask;
      planex = planex & (info->planew - 1);
      //pagex = dot_on_pagex / (512 / info->pagewh);
      pagex = dot_on_pagex >> page_shift;
      //charx = dot_on_pagex - pagex*(512 / info->pagewh);
      charx = dot_on_pagex & page_mask;
      if (pagex < 0) pagex = info->pagewh - 1 + pagex;

      info->PlaneAddr(info, info->mapwh * mapy + mapx, fixVdp2Regs);
      Vdp2PatternAddrPos(info, planex, pagex, planey, pagey);
      Vdp2DrawPatternPos(info, texture, h - charx, v - chary, 0, 0);

    }

    lineindex++;
  }

}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL DoNothing(UNUSED void *info, u32 pixel)
{
   return pixel;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL DoColorOffset(void *info, u32 pixel)
{
    return pixel;
}

#if 0
static u32 FASTCALL DoColorOffset(void *info, u32 pixel)
{
    return COLOR_ADD(pixel, ((vdp2draw_struct *)info)->cor,
                     ((vdp2draw_struct *)info)->cog,
                     ((vdp2draw_struct *)info)->cob);
}
#endif



//////////////////////////////////////////////////////////////////////////////

static INLINE void ReadVdp2ColorOffset(Vdp2 * regs, vdp2draw_struct *info, int mask)
{
  if (regs->CLOFEN & mask)
   {
      // color offset enable
     if (regs->CLOFSL & mask)
      {
         // color offset B
      info->cor = regs->COBR & 0xFF;
      if (regs->COBR & 0x100)
            info->cor |= 0xFFFFFF00;

      info->cog = regs->COBG & 0xFF;
      if (regs->COBG & 0x100)
            info->cog |= 0xFFFFFF00;

      info->cob = regs->COBB & 0xFF;
      if (regs->COBB & 0x100)
            info->cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
      info->cor = regs->COAR & 0xFF;
      if (regs->COAR & 0x100)
            info->cor |= 0xFFFFFF00;

      info->cog = regs->COAG & 0xFF;
      if (regs->COAG & 0x100)
            info->cog |= 0xFFFFFF00;

      info->cob = regs->COAB & 0xFF;
      if (regs->COAB & 0x100)
            info->cob |= 0xFFFFFF00;
      }
      info->PostPixelFetchCalc = &DoColorOffset;
   }
   else{ // color offset disable

      info->PostPixelFetchCalc = &DoNothing;
      info->cor=0;
      info->cob=0;
      info->cog=0;

    }
}

//////////////////////////////////////////////////////////////////////////////

static INLINE u32 Vdp2RotationFetchPixel(vdp2draw_struct *info, int x, int y, int cellw)
{
   u32 dot;

   switch(info->colornumber)
   {
      case 0: // 4 BPP
         dot = T1ReadByte(Vdp2Ram, ((info->charaddr + ((y * cellw) + x) / 2) & 0x7FFFF));
         if (!(x & 0x1)) dot >>= 4;
         if (!(dot & 0xF) && info->transparencyenable) return 0x00000000;
         else return Vdp2ColorRamGetColor(info->coloroffset + ((info->paladdr << 4) | (dot & 0xF)), info->alpha);
      case 1: // 8 BPP
         dot = T1ReadByte(Vdp2Ram, ((info->charaddr + (y * cellw) + x) & 0x7FFFF));
         if (!(dot & 0xFF) && info->transparencyenable) return 0x00000000;
         else return Vdp2ColorRamGetColor(info->coloroffset + ((info->paladdr << 4) | (dot & 0xFF)), info->alpha);
      case 2: // 16 BPP(palette)
         dot = T1ReadWord(Vdp2Ram, ((info->charaddr + ((y * cellw) + x) * 2) & 0x7FFFF));
         if ((dot == 0) && info->transparencyenable) return 0x00000000;
         else return Vdp2ColorRamGetColor(info->coloroffset + dot, info->alpha);
      case 3: // 16 BPP(RGB)
         dot = T1ReadWord(Vdp2Ram, ((info->charaddr + ((y * cellw) + x) * 2) & 0x7FFFF));
         if (!(dot & 0x8000) && info->transparencyenable) return 0x00000000;
         else return SAT2YAB1(0xFF, dot);
      case 4: // 32 BPP
         dot = T1ReadLong(Vdp2Ram, ((info->charaddr + ((y * cellw) + x) * 4) & 0x7FFFF));
         if (!(dot & 0x80000000) && info->transparencyenable) return 0x00000000;
         else return SAT2YAB2(info->alpha, (dot >> 16), dot);
      default:
         return 0;
   }
}

#define RBG_IDLE -1
#define RBG_REQ_RENDER 0
#define RBG_FINIESED 1
#define RBG_TEXTURE_SYNCED 2
 
/*------------------------------------------------------------------------------
 Rotate Screen drawing
 ------------------------------------------------------------------------------*/
void Vdp2DrawRotationThread( void * p ){
  
  while (Vdp2DrawRotationThread_running){
    YabThreadSetCurrentThreadAffinityMask(0x02);
    YabThreadLock(g_rotate_mtx);
    if (Vdp2DrawRotationThread_running == 0){
      break;
    }
    FrameProfileAdd("Vdp2DrawRotationThread start");
    YGL_THREAD_DEBUG("Vdp2DrawRotationThread in %d,%08X\n", curret_rbg->vdp2_sync_flg, curret_rbg->texture.textdata );
    Vdp2DrawRotation_in(curret_rbg);
    FrameProfileAdd("Vdp2DrawRotation_in end"  );
    curret_rbg->vdp2_sync_flg = RBG_FINIESED;
    YGL_THREAD_DEBUG("Vdp2DrawRotationThread end %d,%08X\n", curret_rbg->vdp2_sync_flg, curret_rbg->texture.textdata );
    YabThreadUnLock(g_rotate_mtx);
    while (curret_rbg->vdp2_sync_flg == RBG_FINIESED && Vdp2DrawRotationThread_running) YabThreadYield();
    YGL_THREAD_DEBUG("Vdp2DrawRotationThread out %d\n", curret_rbg->vdp2_sync_flg);
    
  }
}

static void FASTCALL Vdp2DrawRotation( RBGDrawInfo * rbg )
{
  vdp2draw_struct *info = &rbg->info;
  int rgb_type = rbg->rgb_type;
  YglTexture *texture = &rbg->texture;
  YglTexture *line_texture = &rbg->line_texture;

   int x, y;
   int cellw, cellh;
   int oldcellx=-1, oldcelly=-1;
   int lineInc = fixVdp2Regs->LCTA.part.U & 0x8000 ? 2 : 0;
   int linecl = 0xFF;
   Vdp2 * regs;
   if ((fixVdp2Regs->CCCTL >> 5) & 0x01){
     linecl = ((~fixVdp2Regs->CCRLB & 0x1F) << 3) + 0x7;
   }


   if (vdp2height >= 448) lineInc <<= 1;
   if (vdp2height >= 448) rbg->vres = (vdp2height >> 1); else rbg->vres = vdp2height;
   if (vdp2width >= 640) rbg->hres = (vdp2width >> 1); else rbg->hres = vdp2width;
   info->vertices[0] = 0;
   info->vertices[1] = 0;
   info->vertices[2] = vdp2width;
   info->vertices[3] = 0;
   info->vertices[4] = vdp2width;
   info->vertices[5] = vdp2height;
   info->vertices[6] = 0;
   info->vertices[7] = vdp2height;
   cellw = info->cellw;
   cellh = info->cellh;   
   info->cellw = rbg->hres;
   info->cellh = rbg->vres;
   info->flipfunction = 0;
   info->linescreen = 0;
   info->cor = 0x00;
   info->cog = 0x00;
   info->cob = 0x00;
   
   if (fixVdp2Regs->RPMD != 0) rbg->useb = 1;
   if (rgb_type == 1) rbg->useb = 1;

   if (!info->isbitmap)
   {
      oldcellx=-1;
      oldcelly=-1;
    rbg->pagesize = info->pagewh*info->pagewh;
    rbg->patternshift = (2 + info->patternwh);
   }
   else
   {
      oldcellx=0;
      oldcelly=0;
    rbg->pagesize = 0;
    rbg->patternshift = 0;
   }
   
   regs = Vdp2RestoreRegs(3, Vdp2Lines);
   if (regs) ReadVdp2ColorOffset(regs, info, info->linecheck_mask);

   rbg->line_texture.textdata = NULL;
   if( info->LineColorBase !=0 )
   {
     u64 cacheaddr = 0x02;
     rbg->line_info.blendmode = 0;
     rbg->LineColorRamAdress = (T1ReadWord(Vdp2Ram, info->LineColorBase) & 0x7FF);// +info->coloroffset;
   }else{
     rbg->LineColorRamAdress = 0x00;
     rbg->cline.x = 0;
     rbg->cline.y = 0;
     rbg->line_texture.textdata = NULL;
     rbg->line_texture.w = 0;
   }

   if (info->lineTexture != 0){
     info->cor = 0;
     info->cog = 0;
     info->cob = 0;
     info->linescreen = 2;
   }

   if (rbg->async){

     u64 cacheaddr = 0x80000000BAD;
     YglTMAllocate(_Ygl->texture_manager, &rbg->texture, info->cellw, info->cellh, &x, &y);
     rbg->c.x = x;
     rbg->c.y = y;
     YglCacheAdd(_Ygl->texture_manager, cacheaddr, &rbg->c);
     info->cellw = cellw;
     info->cellh = cellh;
     curret_rbg = rbg;
     if (Vdp2DrawRotationThread_running == 0){
       Vdp2DrawRotationThread_running = 1;
       g_rotate_mtx = YabThreadCreateMutex();
       YabThreadLock(g_rotate_mtx);
       YabThreadStart(YAB_THREAD_VIDSOFT_LAYER_RBG0, Vdp2DrawRotationThread, NULL);
     }
     Vdp2RgbTextureSync();
     YGL_THREAD_DEBUG("Vdp2DrawRotation in %d\n", curret_rbg->vdp2_sync_flg);
     curret_rbg->vdp2_sync_flg = RBG_REQ_RENDER;
     YabThreadUnLock(g_rotate_mtx);
     YGL_THREAD_DEBUG("Vdp2DrawRotation out %d\n", curret_rbg->vdp2_sync_flg);
   }
   else{

     YglCache tmpc;
     rbg->vdp2_sync_flg = -1;
     YglQuad(&rbg->info, &rbg->texture, &tmpc);
     info->cellw = cellw;
     info->cellh = cellh;
     Vdp2DrawRotation_in(rbg);
   }
}

void Vdp2RgbTextureSync(){

  
  if (g_rotate_mtx && curret_rbg){

    if (curret_rbg->vdp2_sync_flg == RBG_REQ_RENDER){
      
      // Render Reqested But not finied
      YGL_THREAD_DEBUG("Vdp2RgbTextureSync in %d\n", curret_rbg->vdp2_sync_flg);
      while (curret_rbg->vdp2_sync_flg == RBG_REQ_RENDER) YabThreadYield();
      YabThreadLock(g_rotate_mtx);
      curret_rbg->vdp2_sync_flg = RBG_TEXTURE_SYNCED;
      YGL_THREAD_DEBUG("Vdp2RgbTextureSync out %d\n", curret_rbg->vdp2_sync_flg);
    }
    else if (curret_rbg->vdp2_sync_flg == RBG_FINIESED){
      YGL_THREAD_DEBUG("Vdp2RgbTextureSync in %d\n", curret_rbg->vdp2_sync_flg);
      YabThreadLock(g_rotate_mtx);
      curret_rbg->vdp2_sync_flg = RBG_TEXTURE_SYNCED;
      YGL_THREAD_DEBUG("Vdp2RgbTextureSync out %d\n", curret_rbg->vdp2_sync_flg);
    }
  }
  
}

static void Vdp2DrawRotationSync(){

  if (g_rotate_mtx && curret_rbg){
    
    Vdp2RgbTextureSync();

    if (curret_rbg->vdp2_sync_flg == RBG_TEXTURE_SYNCED){
      YGL_THREAD_DEBUG("Vdp2DrawRotationSync in %d\n", curret_rbg->vdp2_sync_flg);
      curret_rbg->info.cellw = curret_rbg->hres;
      curret_rbg->info.cellh = curret_rbg->vres;
      if (curret_rbg->LineColorRamAdress){
        curret_rbg->info.blendmode = VDP2_CC_NONE;
      }
      curret_rbg->info.flipfunction = 0;
      YglCachedQuad(&curret_rbg->info, &curret_rbg->c);
      curret_rbg->vdp2_sync_flg = RBG_IDLE;
      YGL_THREAD_DEBUG("Vdp2DrawRotationSync out %d\n", curret_rbg->vdp2_sync_flg);
    }
  }
}
   
void Vdp2PreFetchKtable( vdp2rotationparameter_struct * param, int hres, int vres ){

  int need_to_update = param->K_update;
  int maxdot = hres * param->deltaKAx;
  if (maxdot <= 0) maxdot = 1;
  int maxline = vres*param->deltaKAst * maxdot;
  if (maxline <= 0){
    maxline = 1;
  }

  if (param->ktablesize < maxline ){

    if (param->prefecth_k2w != NULL){
      free(param->prefecth_k2w);
      param->prefecth_k2w = NULL;
    }
    if (param->prefecth_k1w != NULL){
      free(param->prefecth_k1w);
      param->prefecth_k1w = NULL;
    }

    if (param->coefdatasize == 2) {
      param->prefecth_k1w = malloc(sizeof(u16)*maxline);
    }else{
      param->prefecth_k2w = malloc(sizeof(u32)*maxline);
    }
    param->ktablesize = maxline;
    need_to_update = 1;
  }

  if (need_to_update == 1) {
    int i = 0;
    for (i = 0; i < maxline; i++) {
      if (param->coefdatasize == 2){ // 1word size

        if (param->prefecth_k1w == NULL){
          param->prefecth_k1w = malloc(sizeof(u16)*maxline);
          param->ktablesize = maxline;
        }

        if (param->k_mem_type == 0) { // vram
          param->prefecth_k1w[i] = T1ReadWord(Vdp2Ram, (param->coeftbladdr + (i << 1)) & 0x7FFFF);
        }
        else{ // cram
          param->prefecth_k1w[i] = T2ReadWord((Vdp2ColorRam + 0x800), (param->coeftbladdr + (i << 1)) & 0xFFF);
        }
      }
      else { // 2word size

        if (param->prefecth_k2w == NULL){
          param->prefecth_k2w = malloc(sizeof(u32)*maxline);
          param->ktablesize = maxline;
        }
        if (param->k_mem_type == 0) { // vram
          param->prefecth_k2w[i] = T1ReadLong(Vdp2Ram, (param->coeftbladdr + (i << 2)) & 0x7FFFF);
        }
        else{ // cram
          param->prefecth_k2w[i] = T2ReadLong((Vdp2ColorRam + 0x800), (param->coeftbladdr + (i << 2)) & 0xFFF);
        }
      }
    }
  }
}

static void Vdp2DrawRotation_in(RBGDrawInfo * rbg){

  if (rbg == NULL) return;

  vdp2draw_struct *info = &rbg->info;
  int rgb_type = rbg->rgb_type;
  YglTexture *texture = &rbg->texture;
  YglTexture *line_texture = &rbg->line_texture;

  int i, j;
  int x, y;
  int cellw, cellh;
  int oldcellx = -1, oldcelly = -1;
  u32 color;
  int vres, hres;
  int h;
  int v;
  int lineInc = fixVdp2Regs->LCTA.part.U & 0x8000 ? 2 : 0;
  int linecl = 0xFF;
  vdp2rotationparameter_struct *parameter;
  Vdp2 * regs;
  if ((fixVdp2Regs->CCCTL >> 5) & 0x01){
    linecl = ((~fixVdp2Regs->CCRLB & 0x1F) << 3) + 0x7;
  }

  if (vdp2height >= 448) lineInc <<= 1;
  vres = rbg->vres;
  hres = rbg->hres;
  cellw = rbg->info.cellw;
  cellh = rbg->info.cellh;
  regs = Vdp2RestoreRegs(3, Vdp2Lines);

  x = 0;
  y = 0;
  if (rgb_type == 0)
  {
    paraA.dx = paraA.A * paraA.deltaX + paraA.B * paraA.deltaY;
    paraA.dy = paraA.D * paraA.deltaX + paraA.E * paraA.deltaY;
    paraA.Xp = paraA.A * (paraA.Px - paraA.Cx) +
      paraA.B * (paraA.Py - paraA.Cy) +
      paraA.C * (paraA.Pz - paraA.Cz) + paraA.Cx + paraA.Mx;
    paraA.Yp = paraA.D * (paraA.Px - paraA.Cx) +
      paraA.E * (paraA.Py - paraA.Cy) +
      paraA.F * (paraA.Pz - paraA.Cz) + paraA.Cy + paraA.My;

    if (paraA.coefenab) {
      Vdp2PreFetchKtable(&paraA, hres, vres);
    }
  }

  if (rbg->useb)
  {
    paraB.dx = paraB.A * paraB.deltaX + paraB.B * paraB.deltaY;
    paraB.dy = paraB.D * paraB.deltaX + paraB.E * paraB.deltaY;
    paraB.Xp = paraB.A * (paraB.Px - paraB.Cx) + paraB.B * (paraB.Py - paraB.Cy)
      + paraB.C * (paraB.Pz - paraB.Cz) + paraB.Cx + paraB.Mx;
    paraB.Yp = paraB.D * (paraB.Px - paraB.Cx) + paraB.E * (paraB.Py - paraB.Cy)
      + paraB.F * (paraB.Pz - paraB.Cz) + paraB.Cy + paraB.My;

    if (paraB.coefenab) {
      Vdp2PreFetchKtable(&paraB, hres, vres);
    }
  }

  for (j = 0; j < vres; j++)
  {
#if 0 // PERLINE
    Vdp2 * regs = Vdp2RestoreRegs(j, Vdp2Lines);
#endif

    if (rgb_type == 0){
#if 0 // PERLINE
      paraA.charaddr = (regs->MPOFR & 0x7) * 0x20000;
      ReadPlaneSizeR(&paraA, regs->PLSZ >> 8);
      for (i = 0; i<16; i++){
        paraA.PlaneAddr(info, i, regs);
        paraA.PlaneAddrv[i] = info->addr;
      }
#endif
      paraA.Xsp = paraA.A * ((paraA.Xst + paraA.deltaXst * j) - paraA.Px) +
        paraA.B * ((paraA.Yst + paraA.deltaYst * j) - paraA.Py) +
        paraA.C * (paraA.Zst - paraA.Pz);

      paraA.Ysp = paraA.D * ((paraA.Xst + paraA.deltaXst *j) - paraA.Px) +
        paraA.E * ((paraA.Yst + paraA.deltaYst * j) - paraA.Py) +
        paraA.F * (paraA.Zst - paraA.Pz);

      paraA.KtablV = paraA.deltaKAst* j;
    }
    if (rbg->useb)
    {
#if 0 // PERLINE
      Vdp2ReadRotationTable(1, &paraB, regs, Vdp2Ram);
      paraB.dx = paraB.A * paraB.deltaX + paraB.B * paraB.deltaY;
      paraB.dy = paraB.D * paraB.deltaX + paraB.E * paraB.deltaY;
      paraB.Xp = paraB.A * (paraB.Px - paraB.Cx) + paraB.B * (paraB.Py - paraB.Cy)
        + paraB.C * (paraB.Pz - paraB.Cz) + paraB.Cx + paraB.Mx;
      paraB.Yp = paraB.D * (paraB.Px - paraB.Cx) + paraB.E * (paraB.Py - paraB.Cy)
        + paraB.F * (paraB.Pz - paraB.Cz) + paraB.Cy + paraB.My;

      ReadPlaneSize(info, regs->PLSZ >> 12);
      ReadPatternData(info, regs->PNCN0, regs->CHCTLA & 0x1);

      paraB.charaddr = (regs->MPOFR & 0x70) * 0x2000;
      ReadPlaneSizeR(&paraB, regs->PLSZ >> 12);
      for (i = 0; i < 16; i++){
        paraB.PlaneAddr(info, i, regs);
        paraB.PlaneAddrv[i] = info->addr;
      }
#endif
      paraB.Xsp = paraB.A * ((paraB.Xst + paraB.deltaXst * j) - paraB.Px) +
        paraB.B * ((paraB.Yst + paraB.deltaYst * j) - paraB.Py) +
        paraB.C * (paraB.Zst - paraB.Pz);

      paraB.Ysp = paraB.D * ((paraB.Xst + paraB.deltaXst * j) - paraB.Px) +
        paraB.E * ((paraB.Yst + paraB.deltaYst * j) - paraB.Py) +
        paraB.F * (paraB.Zst - paraB.Pz);

      paraB.KtablV = paraB.deltaKAst * j;
    }

    if ((fixVdp2Regs->LCTA.part.U & 0x8000) != 0 && info->LineColorBase != 0)
    {
      rbg->LineColorRamAdress = T1ReadWord(Vdp2Ram, info->LineColorBase);
      info->LineColorBase += lineInc;
    }

    //	  if (regs) ReadVdp2ColorOffset(regs, info, info->linecheck_mask);

    for (i = 0; i < hres; i++)
    {

      parameter = info->GetRParam(info, i, j);
      if (parameter == NULL)
      {
        *(texture->textdata++) = 0x00000000;
        continue;
      }

      h = (parameter->ky * (parameter->Xsp + parameter->dx * i) + parameter->Xp);
      v = (parameter->ky * (parameter->Ysp + parameter->dy * i) + parameter->Yp);

      //if (i == 0){ // for Debug
      //  LOG("%d=%d", j, (int)v);
      //}
      if (info->isbitmap)
      {

        switch (parameter->screenover) {
        case OVERMODE_REPEAT:
          h &= cellw - 1;
          v &= cellh - 1;
          break;
        case OVERMODE_SELPATNAME:
          VDP2LOG("Screen-over mode 1 not implemented");
          h &= cellw - 1;
          v &= cellh - 1;
          break;
        case OVERMODE_TRANSE:
          if ((h < 0) || (h >= cellw) || (v < 0) || (v >= cellh)) {
            *(texture->textdata++) = 0x00;
            continue;
          }
          break;
        case OVERMODE_512:
          if ((h < 0) || (h > 512) || (v < 0) || (v > 512)){
            *(texture->textdata++) = 0x00;
            continue;
          }
        }
        // Fetch Pixel
        color = Vdp2RotationFetchPixel(info, h, v, cellw);
      }
      else
      {
        // Tile
        int planenum;
        switch (parameter->screenover) {
        case OVERMODE_REPEAT:
          h &= (parameter->MaxH - 1);
          v &= (parameter->MaxV - 1);
          break;
        case OVERMODE_SELPATNAME:
          VDP2LOG("Screen-over mode 1 not implemented");
          h &= (parameter->MaxH - 1);
          v &= (parameter->MaxV - 1);
          break;
        case OVERMODE_TRANSE:
          if ((h < 0) || (h >= parameter->MaxH) || (v < 0) || (v >= parameter->MaxV)) {
            *(texture->textdata++) = 0x00;
            continue;
          }
          break;
        case OVERMODE_512:
          if ((h < 0) || (h > 512) || (v < 0) || (v > 512)){
            *(texture->textdata++) = 0x00;
            continue;
          }
        }

        x = h;
        y = v;


        if ((x >> rbg->patternshift) != oldcellx || (y >> rbg->patternshift) != oldcelly)
        {
          oldcellx = x >> rbg->patternshift;
          oldcelly = y >> rbg->patternshift;

          // Calculate which plane we're dealing with
          planenum = (x >> parameter->ShiftPaneX) + ((y >> parameter->ShiftPaneY) << 2);
          x &= parameter->MskH;
          y &= parameter->MskV;
          info->addr = parameter->PlaneAddrv[planenum];

          // Figure out which page it's on(if plane size is not 1x1)
          info->addr += (((y >> 9) * rbg->pagesize * info->planew) +
            ((x >> 9) * rbg->pagesize) +
            (((y & 511) >> rbg->patternshift) * info->pagewh) +
            ((x & 511) >> rbg->patternshift)) << info->patterndatasize;

          Vdp2PatternAddr(info); // Heh, this could be optimized
        }

        // Figure out which pixel in the tile we want
        if (info->patternwh == 1)
        {
          x &= 8 - 1;
          y &= 8 - 1;

          // vertical flip
          if (info->flipfunction & 0x2)
            y = 8 - 1 - y;

          // horizontal flip	
          if (info->flipfunction & 0x1)
            x = 8 - 1 - x;
        }
        else
        {
          if (info->flipfunction)
          {
            y &= 16 - 1;
            if (info->flipfunction & 0x2)
            {
              if (!(y & 8))
                y = 8 - 1 - y + 16;
              else
                y = 16 - 1 - y;
            }
            else if (y & 8)
              y += 8;

            if (info->flipfunction & 0x1)
            {
              if (!(x & 8))
                y += 8;

              x &= 8 - 1;
              x = 8 - 1 - x;
            }
            else if (x & 8)
            {
              y += 8;
              x &= 8 - 1;
            }
            else
              x &= 8 - 1;
          }
          else
          {
            y &= 16 - 1;
            if (y & 8)
              y += 8;
            if (x & 8)
              y += 8;
            x &= 8 - 1;
          }
        }

        // Fetch pixel
        color = Vdp2RotationFetchPixel(info, x, y, 8);
      }
      if (info->LineColorBase != 0 && VDP2_CC_NONE != info->blendmode){
        u32 linecol;
        if ((color & 0xFF000000) == 0) {
          
        }
        else{
          if (parameter->lineaddr != 0xFFFFFFFF)
          {
            if (parameter->use_coef_for_linecolor)
              linecol = Vdp2ColorRamGetColor((rbg->LineColorRamAdress & 0x780) | parameter->lineaddr, linecl);
            else
              linecol = Vdp2ColorRamGetColor(rbg->LineColorRamAdress, linecl);

            if(info->blendmode == VDP2_CC_ADD ){
              color = COLOR_ADD(color, (linecol >> 16 & 0XFF), (linecol >> 8 & 0XFF), (linecol >> 0 & 0XFF));
            }else{
              int r,g,b,sr,sg,sb,dr,dg,db;
              int level = info->alpha;
              int invlevel = 256-info->alpha;
              sr = color & 0x00FF0000; dr = linecol & 0x00FF0000;
              r = (sr*level + dr*invlevel)>>8; r&= 0x00FF0000;
              sg = color & 0x0000FF00; dg = linecol & 0x0000FF00;
              g = (sg*level + dg*invlevel) >> 8; g &= 0x0000FF00;
              sb = color & 0x000000FF; db = linecol & 0x000000FF;
              b = (sb*level + db*invlevel) >> 8; b &= 0x000000FF;
              color = r|g|b|0xFF000000;
            }
          }
          else{
            linecol = 0x0 | (linecl << 24);
          }
          
        }
        
      }
      
      *(texture->textdata++) = color;
    }
    texture->textdata += texture->w;
  }

}

//////////////////////////////////////////////////////////////////////////////

static void SetSaturnResolution(int width, int height)
{
   YglChangeResolution(width, height);
   YglSetDensity((vdp2_interlace==0)?1:2);

   vdp2width=width;
   vdp2height=height;
}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLInit(void)
{

  if (YglInit(2048, 1024, 8) != 0)
      return -1;

   SetSaturnResolution(320, 224);

   g_rgb0.async = 1;
   g_rgb0.rgb_type = 0;
   g_rgb0.vdp2_sync_flg = -1;
   g_rgb1.async = 0;
   g_rgb1.rgb_type = 1;
   g_rgb1.vdp2_sync_flg = -1;
   vdp1wratio = 1;
   vdp1hratio = 1;


   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLDeInit(void)
{
  YglDeInit();

  if (Vdp2DrawRotationThread_running != 0){
    Vdp2DrawRotationThread_running = 0;
    YabThreadUnLock(g_rotate_mtx);
    YabThreadWait(YAB_THREAD_VIDSOFT_LAYER_RBG0);
  }
}

//////////////////////////////////////////////////////////////////////////////

int _VIDOGLIsFullscreen;

void VIDOGLResize(int originx, int originy , unsigned int w, unsigned int h, int on)
{

  if( originx == 0 && originy == 0 && w == 0 && h == 0 && on == 0 ){
    YglGLInit(8,8); // Just rebuild texture
  } 

   _VIDOGLIsFullscreen = on;
   
   GlWidth = w;
   GlHeight = h;

   if (_Ygl->resolution_mode == RES_NATIVE && (_Ygl->width != GlWidth || _Ygl->height != GlHeight) ) {
     _Ygl->width = GlWidth;
     _Ygl->height = GlHeight;
   }
   
   _Ygl->originx = originx;
   _Ygl->originy = originy;
   
   YglGLInit(2048, 1024);
   glViewport(originx, originy, GlWidth, GlHeight);
   YglNeedToUpdateWindow();

   SetSaturnResolution(vdp2width, vdp2height);

}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLIsFullscreen(void) {
   return _VIDOGLIsFullscreen;
}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLVdp1Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
void VIDOGLVdp1DrawStart(void)
{
   int i;
   int maxpri;
   int minpri;
   int line = 0;
   _Ygl->vpd1_running = 1;


 #ifdef PERFRAME_LOG
   if (ppfp == NULL){
     ppfp = fopen("ppfp0.txt","w");
   }
   else{
     char fname[64];
     sprintf(fname, "ppfp%d.txt", fount);
     fclose(ppfp);
     ppfp = fopen(fname, "w");
   }
   fount++;
#endif

   fixVdp2Regs = Vdp2RestoreRegs(0, Vdp2Lines);
   if (fixVdp2Regs == NULL) fixVdp2Regs = Vdp2Regs;
   memcpy(&baseVdp2Regs, fixVdp2Regs, sizeof(Vdp2));
   fixVdp2Regs = &baseVdp2Regs;

   u8 *sprprilist = (u8 *)&fixVdp2Regs->PRISA;

   FrameProfileAdd("Vdp1Command start");

   if (_Ygl->texture_manager == NULL){
     _Ygl->texture_manager = YglTM;
     YglTMReset(YglTM);
     YglCacheReset(YglTM);
   }
   YglTmPull(YglTM,0);
   
   maxpri = 0x00;
   minpri = 0x07;   
   for( i=0; i<8; i++ )
   {
      if( (sprprilist[i]&0x07) < minpri ) minpri = (sprprilist[i]&0x07);
      if( (sprprilist[i]&0x07) > maxpri ) maxpri = (sprprilist[i]&0x07);
   }
   _Ygl->vdp1_maxpri = maxpri;
   _Ygl->vdp1_minpri = minpri;

   if (*Vdp2External.perline_alpha_draw & 0x40){
    u32 * linebuf;
    int line_shift = 0;
    if (_Ygl->rheight > 256){
       line_shift = 1;
    }
    else{
       line_shift = 0;
    }

    linebuf = YglGetPerlineBuf(&_Ygl->bg[SPRITE]);
    for (line = 0; line < _Ygl->rheight; line++){
      linebuf[line] = 0xFF000000;
      Vdp2 * lVdp2Regs = &Vdp2Lines[line >> line_shift];
      if (lVdp2Regs->CLOFEN & 0x40){

        // color offset enable
        if (lVdp2Regs->CLOFSL & 0x40)
        {
          // color offset B
          vdp1cor = lVdp2Regs->COBR & 0xFF;
          if (lVdp2Regs->COBR & 0x100)
            vdp1cor |= 0xFFFFFF00;

          vdp1cog = lVdp2Regs->COBG & 0xFF;
          if (lVdp2Regs->COBG & 0x100)
            vdp1cog |= 0xFFFFFF00;

          vdp1cob = lVdp2Regs->COBB & 0xFF;
          if (lVdp2Regs->COBB & 0x100)
            vdp1cob |= 0xFFFFFF00;
        }
        else
        {
          // color offset A
          vdp1cor = lVdp2Regs->COAR & 0xFF;
          if (lVdp2Regs->COAR & 0x100)
            vdp1cor |= 0xFFFFFF00;

          vdp1cog = lVdp2Regs->COAG & 0xFF;
          if (lVdp2Regs->COAG & 0x100)
            vdp1cog |= 0xFFFFFF00;

          vdp1cob = lVdp2Regs->COAB & 0xFF;
          if (lVdp2Regs->COAB & 0x100)
            vdp1cob |= 0xFFFFFF00;
        }


        linebuf[line] |= ((int)(128.0f + (vdp1cor / 2.0)) & 0xFF) << 16;
        linebuf[line] |= ((int)(128.0f + (vdp1cog / 2.0)) & 0xFF) << 8;
        linebuf[line] |= ((int)(128.0f + (vdp1cob / 2.0)) & 0xFF) << 0;
      }
      else{
        linebuf[line] |= 0x00808080;
      }
    }

    YglSetPerlineBuf(&_Ygl->bg[SPRITE], linebuf, _Ygl->rheight);
    _Ygl->vdp1_lineTexture = _Ygl->bg[SPRITE].lincolor_tex;
  }
   else{
     _Ygl->vdp1_lineTexture = 0;
     if (fixVdp2Regs->CLOFEN & 0x40)
     {
       // color offset enable
       if (fixVdp2Regs->CLOFSL & 0x40)
       {
         // color offset B
         vdp1cor = fixVdp2Regs->COBR & 0xFF;
         if (fixVdp2Regs->COBR & 0x100)
           vdp1cor |= 0xFFFFFF00;

         vdp1cog = fixVdp2Regs->COBG & 0xFF;
         if (fixVdp2Regs->COBG & 0x100)
           vdp1cog |= 0xFFFFFF00;

         vdp1cob = fixVdp2Regs->COBB & 0xFF;
         if (fixVdp2Regs->COBB & 0x100)
           vdp1cob |= 0xFFFFFF00;
       }
       else
       {
         // color offset A
         vdp1cor = fixVdp2Regs->COAR & 0xFF;
         if (fixVdp2Regs->COAR & 0x100)
           vdp1cor |= 0xFFFFFF00;

         vdp1cog = fixVdp2Regs->COAG & 0xFF;
         if (fixVdp2Regs->COAG & 0x100)
           vdp1cog |= 0xFFFFFF00;

         vdp1cob = fixVdp2Regs->COAB & 0xFF;
         if (fixVdp2Regs->COAB & 0x100)
           vdp1cob |= 0xFFFFFF00;
       }
     }
     else // color offset disable
       vdp1cor = vdp1cog = vdp1cob = 0;
   }

   Vdp1DrawCommands(Vdp1Ram, Vdp1Regs, NULL);
   FrameProfileAdd("Vdp1Command end ");
   
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1DrawEnd(void)
{
  YglTmPush(YglTM);
  YglRenderVDP1();
  _Ygl->vpd1_running = 0;
}

#define IS_MESH(a) (a&0x100)
#define IS_GLOWSHADING(a) (a&0x04)
#define IS_REPLACE(a) ((a&0x03)==0x00)
#define IS_DONOT_DRAW_OR_SHADOW(a) ((a&0x03)==0x01)
#define IS_HALF_LUMINANCE(a)   ((a&0x03)==0x02)
#define IS_REPLACE_OR_HALF_TRANSPARENT(a) ((a&0x03)==0x03)

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1NormalSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
   vdp1cmd_struct cmd;
   YglSprite sprite;
   YglTexture texture;
   YglCache cash;
   u64 tmp;
   s16 x, y;
   u16 CMDPMOD;
   u16 color2;
   float col[4*4];
   int i;
   short CMDXA;
   short CMDYA;

   
   Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
   if ((cmd.CMDSIZE & 0x8000)){
     regs->EDSR |= 2;
     return; // BAD Command
   }

   sprite.dst=0;
   sprite.blendmode = VDP1_COLOR_CL_REPLACE;
   sprite.linescreen = 0;

   CMDXA = cmd.CMDXA;
   CMDYA = cmd.CMDYA;

   if ((CMDXA & 0x400)) CMDXA |= 0xFC00; else CMDXA &= ~(0xFC00);
   if ((CMDYA & 0x400)) CMDYA |= 0xFC00; else CMDYA &= ~(0xFC00);

   x = CMDXA + Vdp1Regs->localX;
   y = CMDYA + Vdp1Regs->localY;
   sprite.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
   sprite.h = cmd.CMDSIZE & 0xFF;

   sprite.flip = (cmd.CMDCTRL & 0x30) >> 4;

   sprite.vertices[0] = (int)((float)x * vdp1wratio);
   sprite.vertices[1] = (int)((float)y * vdp1hratio);
   sprite.vertices[2] = (int)((float)(x + sprite.w) * vdp1wratio);
   sprite.vertices[3] = (int)((float)y * vdp1hratio);
   sprite.vertices[4] = (int)((float)(x + sprite.w) * vdp1wratio);
   sprite.vertices[5] = (int)((float)(y + sprite.h) * vdp1hratio);
   sprite.vertices[6] = (int)((float)x * vdp1wratio);
   sprite.vertices[7] = (int)((float)(y + sprite.h) * vdp1hratio);

   tmp = cmd.CMDSRCA;
   tmp <<= 16;
   tmp |= cmd.CMDCOLR;
   tmp <<= 16;
   tmp |= cmd.CMDSIZE;

   sprite.priority = 8;

   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);
   
   sprite.uclipmode=(CMDPMOD>>9)&0x03;
   
   if((CMDPMOD & 0x8000) != 0)
   {
      tmp |= 0x00020000;
   }

   if (IS_REPLACE(CMDPMOD)){
     sprite.blendmode = VDP1_COLOR_CL_REPLACE;
   }
   else if (IS_DONOT_DRAW_OR_SHADOW(CMDPMOD)){
     sprite.blendmode = VDP1_COLOR_CL_SHADOW;
   }
   else if (IS_HALF_LUMINANCE(CMDPMOD)){
      sprite.blendmode = VDP1_COLOR_CL_HALF_LUMINANCE;
   }
   else if (IS_REPLACE_OR_HALF_TRANSPARENT(CMDPMOD)){
     tmp |= 0x00010000;
     sprite.blendmode = VDP1_COLOR_CL_GROW_HALF_TRANSPARENT;
   }
   if (IS_MESH(CMDPMOD)){
     tmp |= 0x00010000;
     sprite.blendmode = VDP1_COLOR_CL_MESH;
   }

   if( (CMDPMOD & 4)  )
   {
      for (i=0; i<4; i++)
      {
         color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
         col[(i << 2) + 0] = (float)((color2 & 0x001F))/(float)(0x1F)-0.5f;
         col[(i << 2) + 1] = (float)((color2 & 0x03E0)>>5)/(float)(0x1F)-0.5f;
         col[(i << 2) + 2] = (float)((color2 & 0x7C00)>>10)/(float)(0x1F)-0.5f;
         col[(i << 2) + 3] = 1.0f;
      }
     
      if (sprite.w > 0 && sprite.h > 0)
      {
      if (1 == YglIsCached(_Ygl->texture_manager, tmp, &cash))
         {
            YglCacheQuadGrowShading(&sprite, col,&cash);
            return;
         }

         YglQuadGrowShading(&sprite, &texture,col,&cash);
     YglCacheAdd(_Ygl->texture_manager, tmp, &cash);
         Vdp1ReadTexture(&cmd, &sprite, &texture);
         return;
      }
   
   }
   else // No Gouraud shading, use same color for all 4 vertices
   {
      if (sprite.w > 0 && sprite.h > 0)
      {
      if (1 == YglIsCached(_Ygl->texture_manager, tmp, &cash))
         {
            YglCacheQuadGrowShading(&sprite, NULL,&cash);
            return;
         }

         YglQuadGrowShading(&sprite, &texture,NULL,&cash);
     YglCacheAdd(_Ygl->texture_manager, tmp, &cash);

         Vdp1ReadTexture(&cmd, &sprite, &texture);
    }
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1ScaledSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
   vdp1cmd_struct cmd;
   YglSprite sprite;
   YglTexture texture;
   YglCache cash;
   u64 tmp;
   s16 rw=0, rh=0;
   s16 x, y;
   u16 CMDPMOD;
   u16 color2;
   float col[4*4];
   int i;

   Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
   if (cmd.CMDSIZE == 0){
     return; // BAD Command
   }

   sprite.dst=0;
   sprite.blendmode = VDP1_COLOR_CL_REPLACE;
   sprite.linescreen = 0;

   if ((cmd.CMDYA & 0x400)) cmd.CMDYA |= 0xFC00; else cmd.CMDYA &= ~(0xFC00);
   if ((cmd.CMDYC & 0x400)) cmd.CMDYC |= 0xFC00; else cmd.CMDYC &= ~(0xFC00);
   if ((cmd.CMDYB & 0x400)) cmd.CMDYB |= 0xFC00; else cmd.CMDYB &= ~(0xFC00);
   if ((cmd.CMDYD & 0x400)) cmd.CMDYD |= 0xFC00; else cmd.CMDYD &= ~(0xFC00);
   
   x = cmd.CMDXA + Vdp1Regs->localX;
   y = cmd.CMDYA + Vdp1Regs->localY;
   sprite.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
   sprite.h = cmd.CMDSIZE & 0xFF;
   sprite.flip = (cmd.CMDCTRL & 0x30) >> 4;

   // Setup Zoom Point
   switch ((cmd.CMDCTRL & 0xF00) >> 8)
   {
      case 0x0: // Only two coordinates
         rw = cmd.CMDXC - x + Vdp1Regs->localX;
         rh = cmd.CMDYC - y + Vdp1Regs->localY;
     if (cmd.CMDXA < cmd.CMDXC) rw += 1; else rw -= 1;
     if (cmd.CMDYA < cmd.CMDYC) rh += 1; else rh -= 1;
         break;
      case 0x5: // Upper-left
         rw = cmd.CMDXB + 1;
         rh = cmd.CMDYB + 1;
         break;
      case 0x6: // Upper-Center
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw/2;
         rw++;
         rh++;
         break;
      case 0x7: // Upper-Right
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw;
         rw++;
         rh++;
         break;
      case 0x9: // Center-left
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         y = y - rh/2;
         rw++;
         rh++;
         break;
      case 0xA: // Center-center
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw/2;
         y = y - rh/2;
         rw++;
         rh++;
         break;
      case 0xB: // Center-right
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw;
         y = y - rh/2;
         rw++;
         rh++;
         break;
      case 0xD: // Lower-left
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         y = y - rh;
         rw++;
         rh++;
         break;
      case 0xE: // Lower-center
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw/2;
         y = y - rh;
         rw++;
         rh++;
         break;
      case 0xF: // Lower-right
         rw = cmd.CMDXB;
         rh = cmd.CMDYB;
         x = x - rw;
         y = y - rh;
         rw++;
         rh++;
         break;
      default: break;
   }

   sprite.vertices[0] = (int)((float)x * vdp1wratio);
   sprite.vertices[1] = (int)((float)y * vdp1hratio);
   sprite.vertices[2] = (int)((float)(x + rw) * vdp1wratio);
   sprite.vertices[3] = (int)((float)y * vdp1hratio);
   sprite.vertices[4] = (int)((float)(x + rw) * vdp1wratio);
   sprite.vertices[5] = (int)((float)(y + rh) * vdp1hratio);
   sprite.vertices[6] = (int)((float)x * vdp1wratio);
   sprite.vertices[7] = (int)((float)(y + rh) * vdp1hratio);

   tmp = cmd.CMDSRCA;
   tmp <<= 16;
   tmp |= cmd.CMDCOLR;
   tmp <<= 16;
   tmp |= cmd.CMDSIZE;

   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);
   sprite.uclipmode=(CMDPMOD>>9)&0x03;
   
   sprite.priority = 8;
   
   // MSB
   if((CMDPMOD & 0x8000) != 0)
   {
      tmp |= 0x00020000;
   }

   if (IS_REPLACE(CMDPMOD)){
     if ( (CMDPMOD & 0x8000) )
        sprite.blendmode = VDP1_COLOR_CL_MESH;
     else {
     if ( (CMDPMOD & 0x40) != 0) {
        sprite.blendmode = VDP1_COLOR_SPD;
     } else{
       sprite.blendmode = VDP1_COLOR_CL_REPLACE;
     }
   }
   }
   else if (IS_DONOT_DRAW_OR_SHADOW(CMDPMOD)){
     sprite.blendmode = VDP1_COLOR_CL_SHADOW;
   }
   else if (IS_HALF_LUMINANCE(CMDPMOD)){
     sprite.blendmode = VDP1_COLOR_CL_HALF_LUMINANCE; 
   }
   else if (IS_REPLACE_OR_HALF_TRANSPARENT(CMDPMOD)){
     tmp |= 0x00010000;
     sprite.blendmode = VDP1_COLOR_CL_GROW_HALF_TRANSPARENT;
   }
   if (IS_MESH(CMDPMOD)){
     tmp |= 0x00010000;
     sprite.blendmode = VDP1_COLOR_CL_MESH;
   }

   


   if ( (CMDPMOD & 4) )
   {
      for (i=0; i<4; i++)
      {
         color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
         col[(i << 2) + 0] = (float)((color2 & 0x001F))/(float)(0x1F)-0.5f;
         col[(i << 2) + 1] = (float)((color2 & 0x03E0)>>5)/(float)(0x1F)-0.5f;
         col[(i << 2) + 2] = (float)((color2 & 0x7C00)>>10)/(float)(0x1F)-0.5f;
         col[(i << 2) + 3] = 1.0f;
      }
     
      if (sprite.w > 0 && sprite.h > 0)
      {
      if (1 == YglIsCached(_Ygl->texture_manager, tmp, &cash))
         {
            YglCacheQuadGrowShading(&sprite, col,&cash);
            return;
         }

         YglQuadGrowShading(&sprite, &texture,col,&cash);
     YglCacheAdd(_Ygl->texture_manager, tmp, &cash);
         Vdp1ReadTexture(&cmd, &sprite, &texture);
         return;
    }

   
   }
   else // No Gouraud shading, use same color for all 4 vertices
   {

      if (sprite.w > 0 && sprite.h > 0)
      {

      if (1 == YglIsCached(_Ygl->texture_manager, tmp, &cash))
         {
            YglCacheQuadGrowShading(&sprite, NULL,&cash);
            return;
         }

         YglQuadGrowShading(&sprite, &texture,NULL,&cash);
     YglCacheAdd(_Ygl->texture_manager, tmp, &cash);
         Vdp1ReadTexture(&cmd, &sprite, &texture);
    }

   }
  
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1DistortedSpriteDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
   vdp1cmd_struct cmd;
   YglSprite sprite;
   YglTexture texture;
   YglCache cash;
   u64 tmp;
   u16 CMDPMOD;
   u16 color2;
   int i;
   float col[4*4];
   int isSquare;
   

   Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
   if (cmd.CMDSIZE == 0){
     return; // BAD Command
   }

   sprite.blendmode = VDP1_COLOR_CL_REPLACE;
   sprite.linescreen = 0; 
   sprite.dst = 1;
   sprite.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
   sprite.h = cmd.CMDSIZE & 0xFF;
   sprite.cor = 0;
   sprite.cog = 0;
   sprite.cob = 0;

   // ??? sakatuku2 new scene bug ???
   if (sprite.h != 0 && sprite.w == 0){
     sprite.w = 8;
     sprite.h = 8;
   }

   sprite.flip = (cmd.CMDCTRL & 0x30) >> 4;

   if ((cmd.CMDYA & 0x400)) cmd.CMDYA |= 0xFC00; else cmd.CMDYA &= ~(0xFC00);
   if ((cmd.CMDYC & 0x400)) cmd.CMDYC |= 0xFC00; else cmd.CMDYC &= ~(0xFC00);
   if ((cmd.CMDYB & 0x400)) cmd.CMDYB |= 0xFC00; else cmd.CMDYB &= ~(0xFC00);
   if ((cmd.CMDYD & 0x400)) cmd.CMDYD |= 0xFC00; else cmd.CMDYD &= ~(0xFC00);
   
   if ((cmd.CMDXA & 0x400)) cmd.CMDXA |= 0xFC00; else cmd.CMDXA &= ~(0xFC00);
   if ((cmd.CMDXC & 0x400)) cmd.CMDXC |= 0xFC00; else cmd.CMDXC &= ~(0xFC00);
   if ((cmd.CMDXB & 0x400)) cmd.CMDXB |= 0xFC00; else cmd.CMDXB &= ~(0xFC00);
   if ((cmd.CMDXD & 0x400)) cmd.CMDXD |= 0xFC00; else cmd.CMDXD &= ~(0xFC00);

   sprite.vertices[0] = (s16)cmd.CMDXA;
   sprite.vertices[1] = (s16)cmd.CMDYA;
   sprite.vertices[2] = (s16)cmd.CMDXB;
   sprite.vertices[3] = (s16)cmd.CMDYB;
   sprite.vertices[4] = (s16)cmd.CMDXC;
   sprite.vertices[5] = (s16)cmd.CMDYC;
   sprite.vertices[6] = (s16)cmd.CMDXD;
   sprite.vertices[7] = (s16)cmd.CMDYD;
#if 0
   isSquare = 0;
#else
   isSquare = 1;

   for (i = 0; i < 3; i++){
     float dx = sprite.vertices[((i + 1) << 1) + 0] - sprite.vertices[((i + 0) << 1) + 0];
     float dy = sprite.vertices[((i + 1) << 1) + 1] - sprite.vertices[((i + 0) << 1) + 1];
     if ((dx <= 1.0f && dx >= -1.0f) && (dy <= 1.0f && dy >= -1.0f)){
       isSquare = 0;
       break;
     }

     float d2x = sprite.vertices[(((i + 2) & 0x3) << 1) + 0] - sprite.vertices[((i + 1) << 1) + 0];
     float d2y = sprite.vertices[(((i + 2)&0x3) << 1) + 1] - sprite.vertices[((i + 1) << 1) + 1];
     if ((d2x <= 1.0f && d2x >= -1.0f) && (d2y <= 1.0f && d2y >= -1.0f)){
       isSquare = 0;
       break;
     }

     float dot = dx*d2x + dy*d2y;
     if (dot > EPSILON || dot < -EPSILON){
       isSquare = 0;
       break;
     }
   }

   if (isSquare){
     float minx;
     float miny;
     int lt_index;
     
     sprite.dst = 0;

     // find upper left opsition
     minx = 65535.0f;
     miny = 65535.0f;
     lt_index = -1;
     for( i = 0; i < 4; i++){
       if (sprite.vertices[(i << 1) + 0] <= minx && sprite.vertices[(i << 1) + 1] <= miny){
         minx = sprite.vertices[(i << 1) + 0];
         miny = sprite.vertices[(i << 1) + 1];
         lt_index = i;
       }
     }

     for (i = 0; i < 4; i++){
       if (i != lt_index){
         float nx;
         float ny;
         // vectorize
         float dx = sprite.vertices[(i << 1) + 0] - sprite.vertices[((lt_index) << 1) + 0];
         float dy = sprite.vertices[(i << 1) + 1] - sprite.vertices[((lt_index) << 1) + 1];

         // normalize
         float len = fabsf(sqrtf(dx*dx + dy*dy));
         if (len <= EPSILON){
           continue;
         }
         nx = dx / len;
         ny = dy / len;
         if (nx >= EPSILON) nx = 1.0f; else nx = 0.0f;
         if (ny >= EPSILON) ny = 1.0f; else ny = 0.0f;

         // expand vertex
         sprite.vertices[(i << 1) + 0] += nx;
         sprite.vertices[(i << 1) + 1] += ny;
       }
     }
   }
#if 0
   // Line Polygon
   if ((sprite.vertices[1] == sprite.vertices[3]) &&
     (sprite.vertices[3] == sprite.vertices[5]) &&
     (sprite.vertices[5] == sprite.vertices[7])) {
     sprite.vertices[5] += 1;
     sprite.vertices[7] += 1;
     isSquare = 1;
   }
   // Line Polygon
   if ((sprite.vertices[0] == sprite.vertices[2]) &&
     (sprite.vertices[2] == sprite.vertices[4]) &&
     (sprite.vertices[4] == sprite.vertices[6])) {
     sprite.vertices[4] += 1;
     sprite.vertices[6] += 1;
     isSquare = 1;
   }
#endif
#endif

   sprite.vertices[0] = (sprite.vertices[0] + Vdp1Regs->localX) * vdp1wratio;
   sprite.vertices[1] = (sprite.vertices[1] + Vdp1Regs->localY) * vdp1hratio;
   sprite.vertices[2] = (sprite.vertices[2] + Vdp1Regs->localX) * vdp1wratio;
   sprite.vertices[3] = (sprite.vertices[3] + Vdp1Regs->localY) * vdp1hratio;
   sprite.vertices[4] = (sprite.vertices[4] + Vdp1Regs->localX) * vdp1wratio;
   sprite.vertices[5] = (sprite.vertices[5] + Vdp1Regs->localY) * vdp1hratio;
   sprite.vertices[6] = (sprite.vertices[6] + Vdp1Regs->localX) * vdp1wratio;
   sprite.vertices[7] = (sprite.vertices[7] + Vdp1Regs->localY) * vdp1hratio;


   tmp = cmd.CMDSRCA;
   tmp <<= 16;
   tmp |= cmd.CMDCOLR;
   tmp <<= 16;
   tmp |= cmd.CMDSIZE;


   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);
   
   sprite.priority = 8;
   
   sprite.uclipmode=(CMDPMOD>>9)&0x03;
   
   // MSB
   if((CMDPMOD & 0x8000) != 0)
   {
      tmp |= 0x00020000;
   }
   
   if (IS_REPLACE(CMDPMOD)){
     if ((CMDPMOD & 0x40) != 0) {
       sprite.blendmode = VDP1_COLOR_SPD;
     }
     else{
       sprite.blendmode = VDP1_COLOR_CL_REPLACE;
     }
   }
   else if (IS_DONOT_DRAW_OR_SHADOW(CMDPMOD)){
     sprite.blendmode = VDP1_COLOR_CL_SHADOW;
   }
   else if (IS_HALF_LUMINANCE(CMDPMOD)){
     sprite.blendmode = VDP1_COLOR_CL_HALF_LUMINANCE; 
   }
   else if (IS_REPLACE_OR_HALF_TRANSPARENT(CMDPMOD)){
     tmp |= 0x00010000;
     sprite.blendmode = VDP1_COLOR_CL_GROW_HALF_TRANSPARENT;
   }
   if (IS_MESH(CMDPMOD)){
     tmp |= 0x00010000;
     sprite.blendmode = VDP1_COLOR_CL_MESH;
   }


   // Check if the Gouraud shading bit is set and the color mode is RGB
   if ( (CMDPMOD & 4) )
   {
      for (i=0; i<4; i++)
      {
         color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
         col[(i << 2) + 0] = (float)((color2 & 0x001F))/(float)(0x1F)-0.5f;
         col[(i << 2) + 1] = (float)((color2 & 0x03E0)>>5)/(float)(0x1F)-0.5f;
         col[(i << 2) + 2] = (float)((color2 & 0x7C00)>>10)/(float)(0x1F)-0.5f;
         col[(i << 2) + 3] = 1.0f;
      }
     
    if (1 == YglIsCached(_Ygl->texture_manager, tmp, &cash))
      {
        YglCacheQuadGrowShading(&sprite, col,&cash);
          return;
      }

    YglQuadGrowShading(&sprite, &texture,col,&cash);
    YglCacheAdd(_Ygl->texture_manager, tmp, &cash);
    Vdp1ReadTexture(&cmd, &sprite, &texture);
    return;
   }
   else // No Gouraud shading, use same color for all 4 vertices
   {
     if (1 == YglIsCached(_Ygl->texture_manager, tmp, &cash))
        {
      YglCacheQuadGrowShading(&sprite, NULL,&cash);
      return;
    }
    YglQuadGrowShading(&sprite, &texture,NULL,&cash);
    YglCacheAdd(_Ygl->texture_manager, tmp, &cash);
    Vdp1ReadTexture(&cmd, &sprite, &texture);
   }
   
   return ;
}

//////////////////////////////////////////////////////////////////////////////



void VIDOGLVdp1PolygonDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
   u16 color;
   u16 CMDPMOD;
   u8 alpha;
   YglSprite sprite;
   YglTexture texture;
   u16 color2;
   int i;
   float col[4*4];
   int gouraud=0;
   int priority = 0;
   int isSquare = 0;
   int shadow = 0;
   int normalshadow = 0;
   int colorcalc = 0;
   vdp1cmd_struct cmd;

   sprite.linescreen = 0;

   Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
    
   if ((cmd.CMDYA & 0x400)) cmd.CMDYA |= 0xFC00; else cmd.CMDYA &= ~(0xFC00);
   if ((cmd.CMDYC & 0x400)) cmd.CMDYC |= 0xFC00; else cmd.CMDYC &= ~(0xFC00);
   if ((cmd.CMDYB & 0x400)) cmd.CMDYB |= 0xFC00; else cmd.CMDYB &= ~(0xFC00);
   if ((cmd.CMDYD & 0x400)) cmd.CMDYD |= 0xFC00; else cmd.CMDYD &= ~(0xFC00);

   sprite.blendmode = VDP1_COLOR_CL_REPLACE;
   sprite.dst = 0;


   sprite.vertices[0] = (s16)cmd.CMDXA;
   sprite.vertices[1] = (s16)cmd.CMDYA;
   sprite.vertices[2] = (s16)cmd.CMDXB;
   sprite.vertices[3] = (s16)cmd.CMDYB;
   sprite.vertices[4] = (s16)cmd.CMDXC;
   sprite.vertices[5] = (s16)cmd.CMDYC;
   sprite.vertices[6] = (s16)cmd.CMDXD;
   sprite.vertices[7] = (s16)cmd.CMDYD;

#ifdef PERFRAME_LOG
   if (ppfp != NULL) {
     fprintf(ppfp, "BEFORE %d,%d,%d,%d,%d,%d,%d,%d\n",
       (int)sprite.vertices[0], (int)sprite.vertices[1],
       (int)sprite.vertices[2], (int)sprite.vertices[3],
       (int)sprite.vertices[4], (int)sprite.vertices[5],
       (int)sprite.vertices[6],(int)sprite.vertices[7]
       );
   }
#endif
   isSquare = 1;

   for (i = 0; i < 3; i++){
     float dx = sprite.vertices[((i + 1) << 1) + 0] - sprite.vertices[((i + 0) << 1) + 0];
     float dy = sprite.vertices[((i + 1) << 1) + 1] - sprite.vertices[((i + 0) << 1) + 1];
     float d2x = sprite.vertices[(((i + 2) & 0x3) << 1) + 0] - sprite.vertices[((i + 1) << 1) + 0];
     float d2y = sprite.vertices[(((i + 2) & 0x3) << 1) + 1] - sprite.vertices[((i + 1) << 1) + 1];

     float dot = dx*d2x + dy*d2y;
     if (dot >= EPSILON || dot <= -EPSILON){
       isSquare = 0;
       break;
     }
   }

   // For gungiliffon big polygon
   if (sprite.vertices[2] - sprite.vertices[0] > 350){
     isSquare = 1;
   }

   if (isSquare){
     // find upper left opsition
     float minx = 65535.0f;
     float miny = 65535.0f;
     int lt_index = -1;

     sprite.dst = 0;

     for (i = 0; i < 4; i++){
       if (sprite.vertices[(i << 1) + 0] <= minx /*&& sprite.vertices[(i << 1) + 1] <= miny*/){

         if (minx == sprite.vertices[(i << 1) + 0]){
           if (sprite.vertices[(i << 1) + 1] < miny) {
             minx = sprite.vertices[(i << 1) + 0];
             miny = sprite.vertices[(i << 1) + 1];
             lt_index = i;
           }
         }
         else{
           minx = sprite.vertices[(i << 1) + 0];
           miny = sprite.vertices[(i << 1) + 1];
           lt_index = i;
         }
       }
     }

     float adx = sprite.vertices[(((lt_index + 1) & 0x03) << 1) + 0] - sprite.vertices[((lt_index) << 1) + 0];
     float ady = sprite.vertices[(((lt_index + 1) & 0x03) << 1) + 1] - sprite.vertices[((lt_index) << 1) + 1];
     float bdx = sprite.vertices[(((lt_index + 2) & 0x03) << 1) + 0] - sprite.vertices[((lt_index) << 1) + 0];
     float bdy = sprite.vertices[(((lt_index + 2) & 0x03) << 1) + 1] - sprite.vertices[((lt_index) << 1) + 1];
     float cross = (adx * bdy) - (bdx * ady);

     // clockwise
     if (cross >= 0) {
       sprite.vertices[(((lt_index + 1) & 0x03) << 1) + 0] += 1;
       sprite.vertices[(((lt_index + 1) & 0x03) << 1) + 1] += 0;
       sprite.vertices[(((lt_index + 2) & 0x03) << 1) + 0] += 1;
       sprite.vertices[(((lt_index + 2) & 0x03) << 1) + 1] += 1;
       sprite.vertices[(((lt_index + 3) & 0x03) << 1) + 0] += 0;
       sprite.vertices[(((lt_index + 3) & 0x03) << 1) + 1] += 1;
     }
     // counter-clockwise
     else{
       sprite.vertices[(((lt_index + 1) & 0x03) << 1) + 0] += 0;
       sprite.vertices[(((lt_index + 1) & 0x03) << 1) + 1] += 1;
       sprite.vertices[(((lt_index + 2) & 0x03) << 1) + 0] += 1;
       sprite.vertices[(((lt_index + 2) & 0x03) << 1) + 1] += 1;
       sprite.vertices[(((lt_index + 3) & 0x03) << 1) + 0] += 1;
       sprite.vertices[(((lt_index + 3) & 0x03) << 1) + 1] += 0;
     }

#if 0
     for (i = 0; i < 4; i++){
       if (i != lt_index){
         // vectorize
         float dx = sprite.vertices[(i << 1) + 0] - sprite.vertices[((lt_index) << 1) + 0];
         float dy = sprite.vertices[(i << 1) + 1] - sprite.vertices[((lt_index) << 1) + 1];
         float nx;
         float ny;

         if (sprite.vertices[(i << 1) + 1] == 78){
           int a = 0;
         }

         // normalize
         float len = fabsf(sqrtf(dx*dx + dy*dy));
         if (len <= EPSILON){
           continue;
         }
         nx = dx / len;
         ny = dy / len;
#if 0
         if (nx >= EPSILON){ nx = 5.0f; }
         else if (nx <= -EPSILON){ nx = -5.0f; }
         else nx = 0.0f;

         if (ny >= EPSILON){ ny = 5.0f; }
         else if (ny <= -EPSILON){ ny = -5.0f; }
         else ny = 0.0f;
#endif

         if (nx >= EPSILON &&  nx < 1.0 ){ nx = 1.0; }
         else if (nx <= -EPSILON && nx > -1.0){ nx = -1.0; }
         if (ny >= EPSILON &&  ny < 1.0){ ny = 1.0; }
         else if (ny <= -EPSILON && ny > -1.0){ ny = -1.0; }

         // expand vertex
         sprite.vertices[(i << 1) + 0] += nx;
         sprite.vertices[(i << 1) + 1] += ny;
       }
     }
#endif
   }

#if 0
   // Line Polygon
   if ( (sprite.vertices[1] == sprite.vertices[3]) && // Y1 == Y2
     (sprite.vertices[3]  == sprite.vertices[5]) && // Y2 == Y3
     (sprite.vertices[5]  == sprite.vertices[7]))   // Y3 == Y4
   {
       sprite.vertices[7] += 1;
       sprite.vertices[5] += 1;
   }

   // Line Polygon
   if ((sprite.vertices[0] == sprite.vertices[2]) &&
     (sprite.vertices[2] == sprite.vertices[4]) &&
     (sprite.vertices[4] == sprite.vertices[6])) {
     sprite.vertices[4] += 1;
     sprite.vertices[6] += 1;
   }
#endif

#ifdef PERFRAME_LOG
   if (ppfp != NULL) {
     fprintf(ppfp, "AFTER %d,%d,%d,%d,%d,%d,%d,%d\n",
       (int)sprite.vertices[0], (int)sprite.vertices[1],
       (int)sprite.vertices[2], (int)sprite.vertices[3],
       (int)sprite.vertices[4], (int)sprite.vertices[5],
       (int)sprite.vertices[6], (int)sprite.vertices[7]
       );
   }
#endif

   sprite.vertices[0] = (sprite.vertices[0] + Vdp1Regs->localX) * vdp1wratio;
   sprite.vertices[1] = (sprite.vertices[1] + Vdp1Regs->localY) * vdp1hratio;
   sprite.vertices[2] = (sprite.vertices[2] + Vdp1Regs->localX) * vdp1wratio;
   sprite.vertices[3] = (sprite.vertices[3] + Vdp1Regs->localY) * vdp1hratio;
   sprite.vertices[4] = (sprite.vertices[4] + Vdp1Regs->localX) * vdp1wratio;
   sprite.vertices[5] = (sprite.vertices[5] + Vdp1Regs->localY) * vdp1hratio;
   sprite.vertices[6] = (sprite.vertices[6] + Vdp1Regs->localX) * vdp1wratio;
   sprite.vertices[7] = (sprite.vertices[7] + Vdp1Regs->localY) * vdp1hratio;

   color = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x6);
   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);
   sprite.uclipmode = (CMDPMOD >> 9) & 0x03;

   // Check if the Gouraud shading bit is set and the color mode is RGB
   if( (CMDPMOD & 4) )
   {
      for (i=0; i<4; i++)
      {
         color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
         col[(i << 2) + 0] = (float)((color2 & 0x001F))/(float)(0x1F)-0.5f;
         col[(i << 2) + 1] = (float)((color2 & 0x03E0)>>5)/(float)(0x1F)-0.5f;
         col[(i << 2) + 2] = (float)((color2 & 0x7C00)>>10)/(float)(0x1F)-0.5f;
         col[(i << 2) + 3] = 1.0f;
      }
      gouraud = 1;
   }

   if (color & 0x8000)
      priority = fixVdp2Regs->PRISA & 0x7;
   else
   {
    Vdp1ProcessSpritePixel(fixVdp2Regs->SPCTL & 0xF, &color, &shadow, &normalshadow, &priority, &colorcalc);
#ifdef WORDS_BIGENDIAN
      priority = ((u8 *)&fixVdp2Regs->PRISA)[priority^1] & 0x7;
#else
      priority = ((u8 *)&fixVdp2Regs->PRISA)[priority] & 0x7;
#endif
   }

  
   sprite.priority = 8;
   sprite.w = 1;
   sprite.h = 1;
   sprite.flip = 0;
   sprite.cor = 0x00;
   sprite.cog = 0x00;
   sprite.cob = 0x00;
                  
   int spd = ((CMDPMOD & 0x40) != 0);

   // Pallet mode
   if (IS_REPLACE(CMDPMOD) && color == 0 ) 
   {
     if (spd){
       sprite.blendmode = VDP1_COLOR_SPD;
     }

     YglQuadGrowShading(&sprite, &texture, NULL, NULL);
     alpha = 0x0;
     alpha |= priority;
     *texture.textdata = SAT2YAB1(alpha, color);
     return;
   }

   // RGB mode
   if (color & 0x8000){

     if (spd){
       sprite.blendmode = VDP1_COLOR_SPD;
     }

     int vdp1spritetype = fixVdp2Regs->SPCTL & 0xF;
     if (color != 0x8000 || vdp1spritetype < 2 || (vdp1spritetype < 8 && !(fixVdp2Regs->SPCTL & 0x10)) ){
     }
     else{
       YglQuadGrowShading(&sprite, &texture, NULL, NULL);
       alpha = 0;
       alpha |= priority;
       *texture.textdata = SAT2YAB1(alpha, color);
       return;
     }
   }


   alpha = 0xF8;
   if (IS_REPLACE(CMDPMOD)){
     if ((CMDPMOD & 0x40) != 0) {
       sprite.blendmode = VDP1_COLOR_SPD;
     }
     else{
       alpha = 0xF8;
     }
   }
   else if (IS_DONOT_DRAW_OR_SHADOW(CMDPMOD)){
     alpha = 0xF8;
     sprite.blendmode = VDP1_COLOR_CL_SHADOW;
   }
   else if (IS_HALF_LUMINANCE(CMDPMOD)){
     alpha = 0xF8;
     sprite.blendmode = VDP1_COLOR_CL_HALF_LUMINANCE; 
   }
   else if (IS_REPLACE_OR_HALF_TRANSPARENT(CMDPMOD)){
     alpha = 0x80;
     sprite.blendmode = VDP1_COLOR_CL_GROW_HALF_TRANSPARENT;
   }

   if (IS_MESH(CMDPMOD)){
     alpha = 0x80;
     sprite.blendmode = VDP1_COLOR_CL_MESH; // zzzz
   }

   if (gouraud == 1)
   {
     YglQuadGrowShading(&sprite, &texture, col, NULL);
   }
   else{
     YglQuadGrowShading(&sprite, &texture, NULL, NULL);
   }

   if (fixVdp2Regs->SDCTL & 0x100 ){
   }
   
   /*
   if( (CMDPMOD & 0x100) || (CMDPMOD & 0x7) == 0x3)
   {
      alpha = 0x80;
   }
   */

   
   if ( (color&0x8000) && (fixVdp2Regs->SPCTL & 0x20) ){

     int colorcl;
     int nromal_shadow;
     Vdp1ReadPriority(&cmd, &priority, &colorcl, &nromal_shadow);
     int SPCCCS = (fixVdp2Regs->SPCTL >> 12) & 0x3;

     if (((fixVdp2Regs->CCCTL >> 6) & 0x01) == 0x01)
     {
       switch (SPCCCS)
       {
       case 0:
         if (priority <= ((fixVdp2Regs->SPCTL >> 8) & 0x07))
           alpha = 0xF8 - ((colorcl << 3) & 0xF8);
         break;
       case 1:
         if (priority == ((fixVdp2Regs->SPCTL >> 8) & 0x07))
           alpha = 0xF8 - ((colorcl << 3) & 0xF8);
         break;
       case 2:
         if (priority >= ((fixVdp2Regs->SPCTL >> 8) & 0x07))
           alpha = 0xF8 - ((colorcl << 3) & 0xF8);
         break;
       case 3:
         alpha = 0xF8 - ((colorcl << 3) & 0xF8);
         break;
       }
     }
     alpha |= priority;
     *texture.textdata = SAT2YAB1(alpha, color);

   }else{
     *texture.textdata = Vdp1ReadPolygonColor(&cmd);
   }
}

//////////////////////////////////////////////////////////////////////////////


static void  makeLinePolygon(s16 *v1, s16 *v2, float *outv){
  float dx;
  float dy;
  float len;
  float nx;
  float ny;
  float ex;
  float ey;
  float offset;

  if (v1[0] == v2[0] && v1[1] == v2[1]){
    outv[0] = v1[0];
    outv[1] = v1[1];
    outv[2] = v2[0];
    outv[3] = v2[1];
    outv[4] = v2[0];
    outv[5] = v2[1];
    outv[6] = v1[0];
    outv[7] = v1[1];
    return;
  }

  // vectorize;
  dx = v2[0] - v1[0];
  dy = v2[1] - v1[1];

  // normalize
  len = fabs( sqrtf((dx*dx) + (dy*dy)) );
  if (len < EPSILON ){
    // fail;
    outv[0] = v1[0];
    outv[1] = v1[1];
    outv[2] = v2[0];
    outv[3] = v2[1];
    outv[4] = v2[0];
    outv[5] = v2[1];
    outv[6] = v1[0];
    outv[7] = v1[1];
    return;
  }

  nx = dx / len;
  ny = dy / len;

  // turn
  dx = ny  * 0.5f;
  dy = -nx * 0.5f;

  // extend
  ex = nx * 0.5f;
  ey = ny * 0.5f;

  // offset
  offset = 0.5f;

  // triangle
  outv[0] = v1[0] - ex - dx + offset;
  outv[1] = v1[1] - ey - dy + offset;
  outv[2] = v1[0] - ex + dx + offset;
  outv[3] = v1[1] - ey + dy + offset;
  outv[4] = v2[0] + ex + dx + offset;
  outv[5] = v2[1] + ey + dy + offset;
  outv[6] = v2[0] + ex - dx + offset;
  outv[7] = v2[1] + ey - dy + offset;


}

void VIDOGLVdp1PolylineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
   s16 v[8];
   float line_poygon[8];
   u16 color;
   u16 CMDPMOD;
   u8 alpha;
   YglSprite polygon;
   YglTexture texture;
   YglCache c;
   int priority = 0;
   vdp1cmd_struct cmd;
   float col[4 * 4];
   float linecol[4 * 4];
   int gouraud = 0;
   u16 color2;
   int shadow = 0;
   int normalshadow = 0;
   int colorcalc = 0;

   polygon.blendmode = VDP1_COLOR_CL_REPLACE;
   polygon.linescreen = 0;
   polygon.dst = 0;
   v[0] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C) );
   v[1] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E) );
   v[2] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10) );
   v[3] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12) );
   v[4] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14) );
   v[5] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16) );
   v[6] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x18) );
   v[7] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1A) );

   color = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x6);
   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);
   polygon.uclipmode=(CMDPMOD>>9)&0x03;
   polygon.cor = 0x00;
   polygon.cog = 0x00;
   polygon.cob = 0x00;

   if (color & 0x8000)
      priority = fixVdp2Regs->PRISA & 0x7;
   else
   {
     Vdp1ProcessSpritePixel(fixVdp2Regs->SPCTL & 0xF, &color, &shadow, &normalshadow, &priority, &colorcalc);
#ifdef WORDS_BIGENDIAN
      priority = ((u8 *)&fixVdp2Regs->PRISA)[priority^1] & 0x7;
#else
      priority = ((u8 *)&fixVdp2Regs->PRISA)[priority] & 0x7;
#endif
   }
   
   polygon.priority = 8;
   polygon.w = 1;
   polygon.h = 1;
   polygon.flip = 0;

   if ((CMDPMOD & 4))
   {
     int i;
     for (i = 0; i<4; i++)
     {
       color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
       col[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
       col[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
       col[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
       col[(i << 2) + 3] = 1.0f;
     }
     gouraud = 1;
   }

   makeLinePolygon(&v[0], &v[2], line_poygon);
   polygon.vertices[0] = line_poygon[0] * vdp1wratio;
   polygon.vertices[1] = line_poygon[1] * vdp1hratio;
   polygon.vertices[2] = line_poygon[2] * vdp1wratio;
   polygon.vertices[3] = line_poygon[3] * vdp1hratio;
   polygon.vertices[4] = line_poygon[4] * vdp1wratio;
   polygon.vertices[5] = line_poygon[5] * vdp1hratio;
   polygon.vertices[6] = line_poygon[6] * vdp1wratio;
   polygon.vertices[7] = line_poygon[7] * vdp1hratio;

   if (IS_REPLACE(CMDPMOD)){
     polygon.blendmode = VDP1_COLOR_CL_REPLACE;
   }
   else if (IS_DONOT_DRAW_OR_SHADOW(CMDPMOD)){
     polygon.blendmode = VDP1_COLOR_CL_SHADOW;
   }
   else if (IS_HALF_LUMINANCE(CMDPMOD)){
     polygon.blendmode = VDP1_COLOR_CL_HALF_LUMINANCE; 
   }
   else if (IS_REPLACE_OR_HALF_TRANSPARENT(CMDPMOD)){
     polygon.blendmode = VDP1_COLOR_CL_GROW_HALF_TRANSPARENT;
   }
   if (IS_MESH(CMDPMOD)){
     polygon.blendmode = VDP1_COLOR_CL_MESH;
   }

   if (gouraud){
     linecol[0] = col[(0 << 2) + 0];
     linecol[1] = col[(0 << 2) + 1];
     linecol[2] = col[(0 << 2) + 2];
     linecol[3] = col[(0 << 2) + 3];
     linecol[4] = col[(0 << 2) + 0];
     linecol[5] = col[(0 << 2) + 1];
     linecol[6] = col[(0 << 2) + 2];
     linecol[7] = col[(0 << 2) + 3];
     linecol[8] = col[(1 << 2) + 0];
     linecol[9] = col[(1 << 2) + 1];
     linecol[10] = col[(1 << 2) + 2];
     linecol[11] = col[(1 << 2) + 3];
     linecol[12] = col[(1 << 2) + 0];
     linecol[13] = col[(1 << 2) + 1];
     linecol[14] = col[(1 << 2) + 2];
     linecol[15] = col[(1 << 2) + 3];
     YglQuadGrowShading(&polygon, &texture, linecol, &c);
   }
   else{
     YglQuadGrowShading(&polygon, &texture, NULL, &c);
   }

   if (color == 0)
   {
     alpha = 0;
     priority = 0;
   }
   else{
     alpha = 0xF8;
     if (CMDPMOD & 0x100)
     {
       alpha = 0x80;
     }
   }

   alpha |= priority;

   if (color & 0x8000 && (fixVdp2Regs->SPCTL & 0x20)){
     int SPCCCS = (fixVdp2Regs->SPCTL >> 12) & 0x3;
     if (SPCCCS == 0x03){
       int colorcl;
       int nromal_shadow;
       Vdp1ReadPriority(&cmd, &priority, &colorcl, &nromal_shadow);
       u32 talpha = 0xF8 - ((colorcl << 3) & 0xF8);
       talpha |= priority;
       *texture.textdata = SAT2YAB1(talpha, color);
     }
     else{
       alpha |= priority;
       *texture.textdata = SAT2YAB1(alpha, color);
     }
   }
   else{
      Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
     *texture.textdata = Vdp1ReadPolygonColor(&cmd);
   }

   makeLinePolygon(&v[2], &v[4], line_poygon);
   polygon.vertices[0] = line_poygon[0] * vdp1wratio;
   polygon.vertices[1] = line_poygon[1] * vdp1hratio;
   polygon.vertices[2] = line_poygon[2] * vdp1wratio;
   polygon.vertices[3] = line_poygon[3] * vdp1hratio;
   polygon.vertices[4] = line_poygon[4] * vdp1wratio;
   polygon.vertices[5] = line_poygon[5] * vdp1hratio;
   polygon.vertices[6] = line_poygon[6] * vdp1wratio;
   polygon.vertices[7] = line_poygon[7] * vdp1hratio;
   if (gouraud){
     linecol[0] = col[(1 << 2) + 0];
     linecol[1] = col[(1 << 2) + 1];
     linecol[2] = col[(1 << 2) + 2];
     linecol[3] = col[(1 << 2) + 3];

     linecol[4] = col[(1 << 2) + 0];
     linecol[5] = col[(1 << 2) + 1];
     linecol[6] = col[(1 << 2) + 2];
     linecol[7] = col[(1 << 2) + 3];
     
     linecol[8]  = col[(2 << 2) + 0];
     linecol[9]  = col[(2 << 2) + 1];
     linecol[10] = col[(2 << 2) + 2];
     linecol[11] = col[(2 << 2) + 3];
     
     linecol[12] = col[(2 << 2) + 0];
     linecol[13] = col[(2 << 2) + 1];
     linecol[14] = col[(2 << 2) + 2];
     linecol[15] = col[(2 << 2) + 3];
     
     YglCacheQuadGrowShading(&polygon, linecol, &c);
   }
   else{
     YglCacheQuadGrowShading(&polygon, NULL, &c);
   }

   makeLinePolygon(&v[4], &v[6], line_poygon);
   polygon.vertices[0] = line_poygon[0] * vdp1wratio;
   polygon.vertices[1] = line_poygon[1] * vdp1hratio;
   polygon.vertices[2] = line_poygon[2] * vdp1wratio;
   polygon.vertices[3] = line_poygon[3] * vdp1hratio;
   polygon.vertices[4] = line_poygon[4] * vdp1wratio;
   polygon.vertices[5] = line_poygon[5] * vdp1hratio;
   polygon.vertices[6] = line_poygon[6] * vdp1wratio;
   polygon.vertices[7] = line_poygon[7] * vdp1hratio;
   if (gouraud){
     linecol[0] = col[(2 << 2) + 0];
     linecol[1] = col[(2 << 2) + 1];
     linecol[2] = col[(2 << 2) + 2];
     linecol[3] = col[(2 << 2) + 3];
     linecol[4] = col[(2 << 2) + 0];
     linecol[5] = col[(2 << 2) + 1];
     linecol[6] = col[(2 << 2) + 2];
     linecol[7] = col[(2 << 2) + 3];
     linecol[8] = col[(3 << 2) + 0];
     linecol[9] = col[(3 << 2) + 1];
     linecol[10] = col[(3 << 2) + 2];
     linecol[11] = col[(3 << 2) + 3];
     linecol[12] = col[(3 << 2) + 0];
     linecol[13] = col[(3 << 2) + 1];
     linecol[14] = col[(3 << 2) + 2];
     linecol[15] = col[(3 << 2) + 3];
     YglCacheQuadGrowShading(&polygon, linecol, &c);
   }
   else{
     YglCacheQuadGrowShading(&polygon, NULL, &c);
   }


   if ( !(v[6] == v[0] && v[7] == v[1]) ){
     makeLinePolygon(&v[6], &v[0], line_poygon);
     polygon.vertices[0] = line_poygon[0] * vdp1wratio;
     polygon.vertices[1] = line_poygon[1] * vdp1hratio;
     polygon.vertices[2] = line_poygon[2] * vdp1wratio;
     polygon.vertices[3] = line_poygon[3] * vdp1hratio;
     polygon.vertices[4] = line_poygon[4] * vdp1wratio;
     polygon.vertices[5] = line_poygon[5] * vdp1hratio;
     polygon.vertices[6] = line_poygon[6] * vdp1wratio;
     polygon.vertices[7] = line_poygon[7] * vdp1hratio;
     if (gouraud){
       linecol[0] = col[(3 << 2) + 0];
       linecol[1] = col[(3 << 2) + 1];
       linecol[2] = col[(3 << 2) + 2];
       linecol[3] = col[(3 << 2) + 3];
       linecol[4] = col[(3 << 2) + 0];
       linecol[5] = col[(3 << 2) + 1];
       linecol[6] = col[(3 << 2) + 2];
       linecol[7] = col[(3 << 2) + 3];
       linecol[8] = col[(0 << 2) + 0];
       linecol[9] = col[(0 << 2) + 1];
       linecol[10] = col[(0 << 2) + 2];
       linecol[11] = col[(0 << 2) + 3];
       linecol[12] = col[(0 << 2) + 0];
       linecol[13] = col[(0 << 2) + 1];
       linecol[14] = col[(0 << 2) + 2];
       linecol[15] = col[(0 << 2) + 3];
       YglCacheQuadGrowShading(&polygon, linecol, &c);
     }
     else{
       YglCacheQuadGrowShading(&polygon, NULL, &c);
     }
   }


}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1LineDraw(u8 * ram, Vdp1 * regs, u8* back_framebuffer)
{
   s16 v[4];
   u16 color;
   u16 CMDPMOD;
   u8 alpha;
   YglSprite polygon;
   YglTexture texture;
   int priority = 0;
   float line_poygon[8];
   vdp1cmd_struct cmd;
   float col[4 * 2 * 2];
   int gouraud = 0;
   int shadow = 0;
   int normalshadow = 0;
   int colorcalc = 0;
   u16 color2;
   polygon.cor = 0x00;
   polygon.cog = 0x00;
   polygon.cob = 0x00;


   polygon.blendmode = VDP1_COLOR_CL_REPLACE;
   polygon.linescreen = 0;
   polygon.dst = 0;
   v[0] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C));
   v[1] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E));
   v[2] = Vdp1Regs->localX + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10));
   v[3] = Vdp1Regs->localY + (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12));

   color = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x6);
   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);
   polygon.uclipmode=(CMDPMOD>>9)&0x03;

   if (color & 0x8000)
      priority = fixVdp2Regs->PRISA & 0x7;
   else
   {
     Vdp1ProcessSpritePixel(fixVdp2Regs->SPCTL & 0xF, &color, &shadow, &normalshadow, &priority, &colorcalc);
#ifdef WORDS_BIGENDIAN
      priority = ((u8 *)&fixVdp2Regs->PRISA)[priority^1] & 0x7;
#else
      priority = ((u8 *)&fixVdp2Regs->PRISA)[priority] & 0x7;
#endif
   }
   
   polygon.priority = 8;

   // Check if the Gouraud shading bit is set and the color mode is RGB
   if ((CMDPMOD & 4))
   {
     int i;
     for (i = 0; i<4; i+=2)
     {
       color2 = T1ReadWord(Vdp1Ram, (T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1C) << 3) + (i << 1));
       col[(i << 2) + 0] = (float)((color2 & 0x001F)) / (float)(0x1F) - 0.5f;
       col[(i << 2) + 1] = (float)((color2 & 0x03E0) >> 5) / (float)(0x1F) - 0.5f;
       col[(i << 2) + 2] = (float)((color2 & 0x7C00) >> 10) / (float)(0x1F) - 0.5f;
       col[(i << 2) + 3] = 1.0f;
       col[((i + 1) << 2) + 0] = col[(i << 2) + 0];
       col[((i + 1) << 2) + 1] = col[(i << 2) + 1];
       col[((i + 1) << 2) + 2] = col[(i << 2) + 2];
       col[((i + 1) << 2) + 3] = 1.0f;
     }
     gouraud = 1;
   }


   makeLinePolygon(&v[0], &v[2], line_poygon);
   polygon.vertices[0] = line_poygon[0] * vdp1wratio;
   polygon.vertices[1] = line_poygon[1] * vdp1hratio;
   polygon.vertices[2] = line_poygon[2] * vdp1wratio;
   polygon.vertices[3] = line_poygon[3] * vdp1hratio;
   polygon.vertices[4] = line_poygon[4] * vdp1wratio;
   polygon.vertices[5] = line_poygon[5] * vdp1hratio;
   polygon.vertices[6] = line_poygon[6] * vdp1wratio;
   polygon.vertices[7] = line_poygon[7] * vdp1hratio;

   polygon.w = 1;
   polygon.h = 1;
   polygon.flip = 0;

   if (IS_REPLACE(CMDPMOD)){
     polygon.blendmode = VDP1_COLOR_CL_REPLACE;
   }
   else if (IS_DONOT_DRAW_OR_SHADOW(CMDPMOD)){
     polygon.blendmode = VDP1_COLOR_CL_SHADOW;
   }
   else if (IS_HALF_LUMINANCE(CMDPMOD)){
     polygon.blendmode = VDP1_COLOR_CL_HALF_LUMINANCE; 
   }
   else if (IS_REPLACE_OR_HALF_TRANSPARENT(CMDPMOD)){
     polygon.blendmode = VDP1_COLOR_CL_GROW_HALF_TRANSPARENT;
   }
   if (IS_MESH(CMDPMOD)){
     polygon.blendmode = VDP1_COLOR_CL_MESH;
   }

   if (gouraud == 1){
     YglQuadGrowShading(&polygon, &texture, col, NULL);
   }
   else{
     YglQuadGrowShading(&polygon, &texture, NULL, NULL);
   }
   
   if (color == 0)
   {
      alpha = 0;  
      priority = 0;
   }else{
      alpha = 0xF8;
      if (CMDPMOD & 0x100)
      {
         alpha = 0x80;
      }
   }
   alpha |= priority;
   


   if (color & 0x8000 && (fixVdp2Regs->SPCTL & 0x20)){
     int SPCCCS = (fixVdp2Regs->SPCTL >> 12) & 0x3;
     if (SPCCCS == 0x03){
       int colorcl;
       int nromal_shadow;
       Vdp1ReadPriority(&cmd, &priority, &colorcl, &nromal_shadow);
       u32 talpha = 0xF8 - ((colorcl << 3) & 0xF8);
       talpha |= priority;
       *texture.textdata = SAT2YAB1(talpha, color);
     }
     else{
       alpha |= priority;
       *texture.textdata = SAT2YAB1(alpha, color);
     }
   }
   else{
      Vdp1ReadCommand(&cmd, Vdp1Regs->addr, Vdp1Ram);
     *texture.textdata = Vdp1ReadPolygonColor(&cmd);
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1UserClipping(u8 * ram, Vdp1 * regs)
{
   Vdp1Regs->userclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->userclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
   Vdp1Regs->userclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->userclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1SystemClipping(u8 * ram, Vdp1 * regs)
{
   Vdp1Regs->systemclipX1 = 0;
   Vdp1Regs->systemclipY1 = 0;
   Vdp1Regs->systemclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->systemclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp1LocalCoordinate(u8 * ram, Vdp1 * regs)
{
   Vdp1Regs->localX = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->localY = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
}

//////////////////////////////////////////////////////////////////////////////

int VIDOGLVdp2Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2DrawStart(void)
{
  fixVdp2Regs = Vdp2RestoreRegs(0, Vdp2Lines);
  if (fixVdp2Regs == NULL) fixVdp2Regs = Vdp2Regs;
  VIDOGLVdp2SetResolution(fixVdp2Regs->TVMD);

  if (_Ygl->rwidth > YglTM->width){
    int new_width = _Ygl->rwidth;
    int new_height = YglTM->height;
    YglTMDeInit(YglTM);
    YglTM = YglTMInit(new_width, new_height);
  }
  YglReset();
  if (_Ygl->sync != 0){
    glClientWaitSync(_Ygl->sync, 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(_Ygl->sync);
    _Ygl->sync = 0;
  }
  YglTmPull(YglTM,0);
  YglTMReset(YglTM);
  YglCacheReset(YglTM);
  _Ygl->texture_manager = YglTM;


}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2DrawEnd(void)
{
  Vdp2DrawRotationSync();
  FrameProfileAdd("Vdp2DrawRotationSync end");

  YglTmPush(YglTM);
  
   YglRender();


   /* It would be better to reset manualchange in a Vdp1SwapFrameBuffer
   function that would be called here and during a manual change */
   //Vdp1External.manualchange = 0;
}

//////////////////////////////////////////////////////////////////////////////


   
static void Vdp2DrawBackScreen(void)
{
   u32 scrAddr;
   int dot;
  
   static unsigned char lineColors[512 * 3];
   static int line[512*4];

   if (fixVdp2Regs->VRSIZE & 0x8000)
     scrAddr = (((fixVdp2Regs->BKTAU & 0x7) << 16) | fixVdp2Regs->BKTAL) * 2;
   else
     scrAddr = (((fixVdp2Regs->BKTAU & 0x3) << 16) | fixVdp2Regs->BKTAL) * 2;

#if defined(__ANDROID__) || defined(_OGLES3_) || defined(_OGL3_)
   dot = T1ReadWord(Vdp2Ram, scrAddr);
   YglSetClearColor(
     (float)(dot & 0x1F) / (float)(0x1F), 
     (float)((dot & 0x3E0) >> 5) / (float)(0x1F), 
     (float)((dot & 0x7C00) >> 10)/ (float)(0x1F)
     );
#else
   if (fixVdp2Regs->BKTAU & 0x8000)
   {
    int y;

      for(y = 0; y < vdp2height; y++)
      {
         dot = T1ReadWord(Vdp2Ram, scrAddr);
         scrAddr += 2;
         
         lineColors[3*y+0] = (dot & 0x1F) << 3;
         lineColors[3*y+1] = (dot & 0x3E0) >> 2;
         lineColors[3*y+2] = (dot & 0x7C00) >> 7;
         line[4*y+0] = 0;
         line[4*y+1] = y;
         line[4*y+2] = vdp2width;
         line[4*y+3] = y;         
      }
      
      glColorPointer(3, GL_UNSIGNED_BYTE, 0, lineColors);
      glEnableClientState(GL_COLOR_ARRAY);
      glVertexPointer(2, GL_INT, 0, line);
      glEnableClientState(GL_VERTEX_ARRAY);
      glDrawArrays(GL_LINES,0,vdp2height*2);
      glDisableClientState(GL_COLOR_ARRAY);      
      glColor3ub(0xFF, 0xFF, 0xFF);
   }
   else
   {
      dot = T1ReadWord(Vdp2Ram, scrAddr);

      glColor3ub((dot & 0x1F) << 3, (dot & 0x3E0) >> 2, (dot & 0x7C00) >> 7);

      line[0] = 0;
      line[1] = 0;
      line[2] = vdp2width;
      line[3] = 0;    
      line[4] = vdp2width;
      line[5] = vdp2height;
      line[6] = 0;
      line[7] = vdp2height;    

      glVertexPointer(2, GL_INT, 0, line);
      glEnableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      glDrawArrays(GL_TRIANGLE_FAN,0,8);
      glColor3ub(0xFF, 0xFF, 0xFF);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   }
#endif
}

//////////////////////////////////////////////////////////////////////////////
// 11.3 Line Color insertion
//  7.1 Line Color Screen
static void Vdp2DrawLineColorScreen(void)
{

  u32 cacheaddr = 0xFFFFFFFF;
  int inc = 0;
  int line_cnt = vdp2height;
  int i;
  u32 * line_pixel_data;
  u32 addr;

  if ( fixVdp2Regs->LNCLEN == 0) return;

  line_pixel_data = YglGetLineColorPointer();
  if( line_pixel_data == NULL ){
      return;
  }

  if ((fixVdp2Regs->LCTA.part.U & 0x8000)){
    inc = 0x02; // single color
  } else{
    inc = 0x00; // color per line
  }

  u8 alpha = 0xFF;
  // Color calculation ratio mode Destination Alpha
  // Use blend value CRLB
  if ((fixVdp2Regs->CCCTL & 0x0300) == 0x0200){
    alpha = (float)(fixVdp2Regs->CCRLB & 0x1F) / 32.0f * 255.0f;
  }

  addr = (fixVdp2Regs->LCTA.all & 0x7FFFF) * 0x2;
  for (i = 0; i < line_cnt; i++){
    u16 LineColorRamAdress = T1ReadWord(Vdp2Ram, addr);
    *(line_pixel_data) = Vdp2ColorRamGetColor(LineColorRamAdress, alpha);
    line_pixel_data++;
    addr += inc;
  }

  YglSetLineColor( line_pixel_data, line_cnt );

}

void Vdp2GeneratePerLineColorCalcuration(vdp2draw_struct * info, int id){
  int bit = 1 << id;
  int line = 0;
  if (*Vdp2External.perline_alpha_draw & bit){
    u32 * linebuf;
    int line_shift = 0;
    if (_Ygl->rheight > 256 ){
      line_shift = 1;
    }
    else{
      line_shift = 0;
    }

    linebuf = YglGetPerlineBuf(&_Ygl->bg[id]);
    for (line = 0; line < _Ygl->rheight; line++){
      if ((Vdp2Lines[line >> line_shift].BGON & bit) == 0x00){
        linebuf[line] = 0x00;
      }
      else{
        info->enable = 1;
        if (Vdp2Lines[line >> line_shift].CCCTL & bit)
        {
          switch (id){
          case NBG0:
            linebuf[line] = (((~Vdp2Lines[line >> line_shift].CCRNA & 0x1F) << 3) + 0x7) << 24;
            break;
          case NBG1:
            linebuf[line] = (((~Vdp2Lines[line >> line_shift].CCRNA & 0x1F00) >> 5) + 0x7) << 24;
            break;
          case NBG2:
            linebuf[line] = (((~Vdp2Lines[line >> line_shift].CCRNB & 0x1F) << 3) + 0x7) << 24;
            break;
          case NBG3:
            linebuf[line] = (((~Vdp2Lines[line >> line_shift].CCRNB & 0x1F00) >> 5) + 0x7) << 24;
            break;
          case RBG0:
            linebuf[line] = (((~Vdp2Lines[line >> line_shift].CCRR & 0x1F) << 3) + 0x7) << 24;
            break;
          }

        }
        else{
          linebuf[line] = 0xFF000000;
        }

        if (Vdp2Lines[line >> line_shift].CLOFEN  & bit){
          ReadVdp2ColorOffset(&Vdp2Lines[line >> line_shift], info, bit);
          linebuf[line] |= ((int)(128.0f + (info->cor / 2.0)) & 0xFF) << 16;
          linebuf[line] |= ((int)(128.0f + (info->cog / 2.0)) & 0xFF) << 8;
          linebuf[line] |= ((int)(128.0f + (info->cob / 2.0)) & 0xFF) << 0;
        }
        else{
          linebuf[line] |= 0x00808080;
        }

      }
    }
    YglSetPerlineBuf(&_Ygl->bg[id], linebuf, _Ygl->rheight);
    info->lineTexture = _Ygl->bg[id].lincolor_tex;
  }
  else{
    info->lineTexture = 0;
  }

}


//////////////////////////////////////////////////////////////////////////////


static void Vdp2DrawNBG0(void)
{
   vdp2draw_struct info = { 0 };
   YglTexture texture;
   YglCache tmpc;
   info.dst=0;
   info.uclipmode=0;
   info.id = 0;
   info.coordincx = 1.0f;
   info.coordincy = 1.0f;

   info.cor = 0;
   info.cog = 0;
   info.cob = 0;
   int i;
   info.enable = 0;

   info.cellh = 256;
   info.specialcolorfunction = 0;


   Vdp2GeneratePerLineColorCalcuration(&info, NBG0);

   if (fixVdp2Regs->BGON & 0x20)
   {
      // RBG1 mode
      info.enable = fixVdp2Regs->BGON & 0x20;
      if (!info.enable) return;

      // Read in Parameter B
    Vdp2ReadRotationTable(1, &paraB, fixVdp2Regs, Vdp2Ram);

      if((info.isbitmap = fixVdp2Regs->CHCTLA & 0x2) != 0)
      {
         // Bitmap Mode

         ReadBitmapSize(&info, fixVdp2Regs->CHCTLA >> 2, 0x3);

         info.charaddr = (fixVdp2Regs->MPOFR & 0x70) * 0x2000;
         info.paladdr = (fixVdp2Regs->BMPNA & 0x7) << 4;
         info.flipfunction = 0;
         info.specialfunction = 0;
      }
      else
      {
         // Tile Mode
         info.mapwh = 4;
         ReadPlaneSize(&info, fixVdp2Regs->PLSZ >> 12);
         ReadPatternData(&info, fixVdp2Regs->PNCN0, fixVdp2Regs->CHCTLA & 0x1);

     paraB.ShiftPaneX = 8 + info.planew;
     paraB.ShiftPaneY = 8 + info.planeh;
     paraB.MskH = (8 * 64 * info.planew) - 1;
     paraB.MskV = (8 * 64 * info.planeh) - 1;
     paraB.MaxH = 8 * 64 * info.planew * 4;
     paraB.MaxV = 8 * 64 * info.planeh * 4;

      }

      info.rotatenum = 1;
    paraB.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterBPlaneAddr;
    paraB.coefenab = fixVdp2Regs->KTCTL & 0x100;
    paraB.charaddr = (fixVdp2Regs->MPOFR & 0x70) * 0x2000;
    ReadPlaneSizeR(&paraB, fixVdp2Regs->PLSZ >> 12);
    for (i = 0; i<16; i++)
    {
      paraB.PlaneAddr(&info, i, fixVdp2Regs);
      paraB.PlaneAddrv[i] = info.addr;
    }

      info.LineColorBase = 0x00;
      if (paraB.coefenab)
         info.GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode01WithK;
      else
         info.GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode01NoK;

    if (paraB.coefdatasize == 2)
    {
      if (paraB.coefmode < 3)
      {
        info.GetKValueB = vdp2rGetKValue1W;
      }
      else{
        info.GetKValueB = vdp2rGetKValue1Wm3;
      }

    }
    else{
      if (paraB.coefmode < 3)
      {
        info.GetKValueB = vdp2rGetKValue2W;
      }
      else{
        info.GetKValueB = vdp2rGetKValue2Wm3;
      }
    }
   }
   else if ( (fixVdp2Regs->BGON & 0x1) || info.enable )
   {
      // NBG0 mode
      info.enable = fixVdp2Regs->BGON & 0x1;
      if (!info.enable) return;

      if((info.isbitmap = fixVdp2Regs->CHCTLA & 0x2) != 0)
      {
         // Bitmap Mode

         ReadBitmapSize(&info, fixVdp2Regs->CHCTLA >> 2, 0x3);

         info.x = - ((fixVdp2Regs->SCXIN0 & 0x7FF) % info.cellw);
         info.y = - ((fixVdp2Regs->SCYIN0 & 0x7FF) % info.cellh);

         info.charaddr = (fixVdp2Regs->MPOFN & 0x7) * 0x20000;
         info.paladdr = (fixVdp2Regs->BMPNA & 0x7) << 4;
         info.flipfunction = 0;
	 info.specialcolorfunction = (fixVdp2Regs->BMPNA & 0x10) >> 4;
         info.specialfunction = 0;
      }
      else
      {
         // Tile Mode
         info.mapwh = 2;

         ReadPlaneSize(&info, fixVdp2Regs->PLSZ);

         info.x = - ((fixVdp2Regs->SCXIN0 & 0x7FF) % (512 * info.planew));
         info.y = - ((fixVdp2Regs->SCYIN0 & 0x7FF) % (512 * info.planeh));
         ReadPatternData(&info, fixVdp2Regs->PNCN0, fixVdp2Regs->CHCTLA & 0x1);
      }

      if( (fixVdp2Regs->ZMXN0.all & 0x7FF00) == 0 )
         info.coordincx = 1.0f; 
      else
         info.coordincx = (float) 65536 / (fixVdp2Regs->ZMXN0.all & 0x7FF00);
      
      switch(fixVdp2Regs->ZMCTL&0x03)
      {
      case 0:
      info.maxzoom = 1.0f;
         break;
      case 1:
      info.maxzoom = 0.5f;
         //if( info.coordincx < 0.5f )  info.coordincx = 0.5f;
         break;
      case 2:
      case 3:
      info.maxzoom = 0.25f;
         //if( info.coordincx < 0.25f )  info.coordincx = 0.25f;
         break;
      }

      if( (fixVdp2Regs->ZMYN0.all & 0x7FF00) == 0 )
         info.coordincy = 1.0f; 
      else
         info.coordincy = (float) 65536 / (fixVdp2Regs->ZMYN0.all & 0x7FF00);
      
      info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG0PlaneAddr;
   }
   else{
     return;
   }
 


   ReadMosaicData(&info, 0x1, fixVdp2Regs);

   info.transparencyenable = !(fixVdp2Regs->BGON & 0x100);
   info.specialprimode   = fixVdp2Regs->SFPRMD & 0x3;
   info.specialcolormode = fixVdp2Regs->SFCCMD & 0x3;

   info.colornumber = (fixVdp2Regs->CHCTLA & 0x70) >> 4;
   int dest_alpha = ((fixVdp2Regs->CCCTL >> 9) & 0x01);

   // 12.13 blur
   if ((fixVdp2Regs->CCCTL & 0xF000) == 0xA000){
     info.alpha = 0xFF;
     info.blendmode = VDP2_CC_BLUR;
   }
   else{

     // Enable Color Calculation
     if (fixVdp2Regs->CCCTL & 0x1)
     {
       // Color calculation ratio
       info.alpha = ((~fixVdp2Regs->CCRNA & 0x1F) << 3) + 0x7;

       // Color calculation mode bit
       if (fixVdp2Regs->CCCTL & 0x100){ // Add Color
         info.blendmode = VDP2_CC_ADD;
       }
       else{ // Use Color calculation ratio
         if (info.specialcolormode != 0 && dest_alpha){ // Just currently not supported
           info.blendmode = VDP2_CC_NONE;
         }
         else{
           info.blendmode = VDP2_CC_RATE;
         }
       }
       // Disable Color Calculation
     }
     else{

       // Use Destination Alpha 12.14 CCRTMD
       if (dest_alpha){

         // Color calculation will not be operated.
         // But need to write alpha value
         info.alpha = ((~fixVdp2Regs->CCRNA & 0x1F) << 3) + 0x7;
       }
       else{
         info.alpha = 0xFF;
       }
       info.blendmode = VDP2_CC_NONE;
     }
   }

   info.linescreen = 0;
   if (fixVdp2Regs->LNCLEN & 0x1)
     info.linescreen = 1;

   info.coloroffset = (fixVdp2Regs->CRAOFA & 0x7) << 8;
   ReadVdp2ColorOffset(fixVdp2Regs,&info, 0x1);
   if (info.lineTexture != 0){
     info.cor = 0;
     info.cog = 0;
     info.cob = 0;
     info.linescreen = 2;
   }
   info.linecheck_mask = 0x01;
   info.priority = fixVdp2Regs->PRINA & 0x7;

   if (!(info.enable & Vdp2External.disptoggle) || (info.priority == 0))
      return;
   
   // Window Mode
   info.bEnWin0 = (fixVdp2Regs->WCTLA >> 1) &0x01;
   info.WindowArea0 = (fixVdp2Regs->WCTLA >> 0) & 0x01;  
   info.bEnWin1 = (fixVdp2Regs->WCTLA >> 3) &0x01;
   info.WindowArea1 = (fixVdp2Regs->WCTLA >> 2) & 0x01; 
   info.LogicWin    = (fixVdp2Regs->WCTLA >> 7 ) & 0x01;
   

   ReadLineScrollData(&info, fixVdp2Regs->SCRCTL & 0xFF, fixVdp2Regs->LSTA0.all);
   info.lineinfo = lineNBG0;
   Vdp2GenLineinfo( &info );
   Vdp2SetGetColor( &info );

   if (fixVdp2Regs->SCRCTL & 1)
   {
     info.isverticalscroll = 1;
     info.verticalscrolltbl = (fixVdp2Regs->VCSTA.all & 0x7FFFE) << 1;
     if (fixVdp2Regs->SCRCTL & 0x100)
       info.verticalscrollinc = 8;
     else
       info.verticalscrollinc = 4;
   }
   else
     info.isverticalscroll = 0;

   if (info.enable == 1)
   {
      // NBG0 draw
      if (info.isbitmap)
      {
      if (info.coordincx != 1.0f || info.coordincy != 1.0f || VDPLINE_SZ(info.islinescroll) ){
        info.sh = (fixVdp2Regs->SCXIN0 & 0x7FF);
        info.sv = (fixVdp2Regs->SCYIN0 & 0x7FF);
        info.x = 0;
        info.y = 0;
        info.vertices[0] = 0;
        info.vertices[1] = 0;
        info.vertices[2] = vdp2width;
        info.vertices[3] = 0;
        info.vertices[4] = vdp2width;
        info.vertices[5] = vdp2height;
        info.vertices[6] = 0;
        info.vertices[7] = vdp2height;
        vdp2draw_struct infotmp = info;
        infotmp.cellw = vdp2width;
        if (vdp2height >= 448)
          infotmp.cellh = (vdp2height >> 1);
        else
          infotmp.cellh = vdp2height;
        YglQuad(&infotmp, &texture, &tmpc);
        Vdp2DrawBitmapCoordinateInc(&info, &texture);
      }
      else{

        int xx, yy;
        int isCached = 0;

        if (info.islinescroll) // Nights Movie
        {
          info.sh = (fixVdp2Regs->SCXIN0 & 0x7FF);
          info.sv = (fixVdp2Regs->SCYIN0 & 0x7FF);
          info.x = 0;
          info.y = 0;
          info.vertices[0] = 0;
          info.vertices[1] = 0;
          info.vertices[2] = vdp2width;
          info.vertices[3] = 0;
          info.vertices[4] = vdp2width;
          info.vertices[5] = vdp2height;
          info.vertices[6] = 0;
          info.vertices[7] = vdp2height;
          vdp2draw_struct infotmp = info;
          infotmp.cellw = vdp2width;
          infotmp.cellh = vdp2height;
          YglQuad(&infotmp, &texture, &tmpc);
          Vdp2DrawBitmapLineScroll(&info, &texture);

        }
        else{
          yy = info.y;
          while (yy + info.y < vdp2height)
          {
            xx = info.x;
            while (xx + info.x < vdp2width)
            {
              info.vertices[0] = xx;
              info.vertices[1] = yy;
              info.vertices[2] = (xx + info.cellw);
              info.vertices[3] = yy;
              info.vertices[4] = (xx + info.cellw);
              info.vertices[5] = (yy + info.cellh);
              info.vertices[6] = xx;
              info.vertices[7] = (yy + info.cellh);

              if (isCached == 0)
              {
                YglQuad(&info, &texture, &tmpc);
                if (info.islinescroll){
                  Vdp2DrawBitmapLineScroll(&info, &texture);
                }
                else{
                  Vdp2DrawCell(&info, &texture);
                }
                isCached = 1;
              }
              else{
                YglCachedQuad(&info, &tmpc);
              }
              xx += info.cellw;
            }
            yy += info.cellh;
          }
        }
      }
    }
    else
    {
      if (info.islinescroll){
        info.x = fixVdp2Regs->SCXIN0 & 0x7FF;
        info.y = fixVdp2Regs->SCYIN0 & 0x7FF;
        Vdp2DrawMapPerLine(&info, &texture);
      }
      else{
        info.x = fixVdp2Regs->SCXIN0 & 0x7FF;
        info.y = fixVdp2Regs->SCYIN0 & 0x7FF;
        Vdp2DrawMapTest(&info, &texture);
      }
    }
   }
   else
   {
     // RBG1 draw
     memcpy(&g_rgb1.info, &info, sizeof(info));
     Vdp2DrawRotation(&g_rgb1);
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG1(void)
{
  vdp2draw_struct info;
  YglTexture texture;
  YglCache tmpc;
  info.dst = 0;
  info.id = 1;
  info.uclipmode = 0;
  info.cor = 0;
  info.cog = 0;
  info.cob = 0;
  info.specialcolorfunction = 0;

  info.enable = fixVdp2Regs->BGON & 0x2;
  if (!info.enable) return;
  info.transparencyenable = !(fixVdp2Regs->BGON & 0x200);
  info.specialprimode = (fixVdp2Regs->SFPRMD >> 2) & 0x3;

  info.colornumber = (fixVdp2Regs->CHCTLA & 0x3000) >> 12;

  Vdp2GeneratePerLineColorCalcuration(&info, NBG1);

  if ((info.isbitmap = fixVdp2Regs->CHCTLA & 0x200) != 0)
  {
    ReadBitmapSize(&info, fixVdp2Regs->CHCTLA >> 10, 0x3);

    info.x = -((fixVdp2Regs->SCXIN1 & 0x7FF) % info.cellw);
    info.y = -((fixVdp2Regs->SCYIN1 & 0x7FF) % info.cellh);
    info.charaddr = ((fixVdp2Regs->MPOFN & 0x70) >> 4) * 0x20000;
    info.paladdr = (fixVdp2Regs->BMPNA & 0x700) >> 4;
    info.flipfunction = 0;
    info.specialfunction = 0;
    info.specialcolorfunction = (fixVdp2Regs->BMPNA & 0x1000) >> 4;
  }
  else
  {
    info.mapwh = 2;

    ReadPlaneSize(&info, fixVdp2Regs->PLSZ >> 2);

    info.x = -((fixVdp2Regs->SCXIN1 & 0x7FF) % (512 * info.planew));
    info.y = -((fixVdp2Regs->SCYIN1 & 0x7FF) % (512 * info.planeh));

    ReadPatternData(&info, fixVdp2Regs->PNCN1, fixVdp2Regs->CHCTLA & 0x100);
  }

  info.specialcolormode = (fixVdp2Regs->SFCCMD >> 2) & 0x3;

  ReadMosaicData(&info, 0x2, fixVdp2Regs);

  // 12.13 blur
  if ((fixVdp2Regs->CCCTL & 0xF000) == 0xC000 ){
    info.alpha = 0xFF;
    info.blendmode = VDP2_CC_BLUR;
  }
  else{
    if (fixVdp2Regs->CCCTL & 0x2)
    {
      info.alpha = ((~fixVdp2Regs->CCRNA & 0x1F00) >> 5) + 0x7;
      if (fixVdp2Regs->CCCTL & 0x100 && info.specialcolormode == 0)
      {
        info.blendmode = VDP2_CC_ADD;
      }
      else{
        info.blendmode = VDP2_CC_RATE;
      }
    }
    else{
      // 12.14 CCRTMD
      if (((fixVdp2Regs->CCCTL >> 9) & 0x01) == 0x01){
        info.alpha = ((~fixVdp2Regs->CCRNA & 0x1F00) >> 5) + 0x7;
      }
      else{
        info.alpha = 0xFF;
      }
      info.blendmode = VDP2_CC_NONE;
    }
  }

   info.linescreen = 0;
   if (fixVdp2Regs->LNCLEN & 0x2)
     info.linescreen = 1;

   info.coloroffset = (fixVdp2Regs->CRAOFA & 0x70) << 4;
   ReadVdp2ColorOffset(fixVdp2Regs,&info, 0x2);
   if (info.lineTexture != 0){
     info.cor = 0;
     info.cog = 0;
     info.cob = 0;
     info.linescreen = 2;
   }

   info.linecheck_mask = 0x02;

   if( (fixVdp2Regs->ZMXN1.all & 0x7FF00) == 0 )
      info.coordincx = 1.0f; 
   else
      info.coordincx = (float) 65536 / (fixVdp2Regs->ZMXN1.all & 0x7FF00);
      
   switch((fixVdp2Regs->ZMCTL>>8)&0x03)
   {
   case 0:
    info.maxzoom = 1.0f;
      break;
   case 1:
     info.maxzoom = 0.5f;
//      if( info.coordincx < 0.5f )  info.coordincx = 0.5f;
      break;
   case 2:
   case 3:
     info.maxzoom = 0.25f;
//      if( info.coordincx < 0.25f )  info.coordincx = 0.25f;
      break;
   }

   if( (fixVdp2Regs->ZMYN1.all & 0x7FF00) == 0 )
      info.coordincy = 1.0f; 
   else
      info.coordincy = (float) 65536 / (fixVdp2Regs->ZMYN1.all & 0x7FF00);
      
      
   info.priority = (fixVdp2Regs->PRINA >> 8) & 0x7;;
   info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG1PlaneAddr;

   if (!(info.enable & Vdp2External.disptoggle) || (info.priority == 0) ||
      (fixVdp2Regs->BGON & 0x1 && (fixVdp2Regs->CHCTLA & 0x70) >> 4 == 4)) // If NBG0 16M mode is enabled, don't draw
      return;
   
   // Window Mode
   info.bEnWin0 = (fixVdp2Regs->WCTLA >> 9) &0x01;
   info.WindowArea0 = (fixVdp2Regs->WCTLA >> 8) & 0x01;  
   info.bEnWin1 = (fixVdp2Regs->WCTLA >> 11) &0x01;
   info.WindowArea1 = (fixVdp2Regs->WCTLA >> 10) & 0x01; 
   info.LogicWin    = (fixVdp2Regs->WCTLA >> 15 ) & 0x01;
   
  
   ReadLineScrollData(&info, fixVdp2Regs->SCRCTL >> 8, fixVdp2Regs->LSTA1.all);
   info.lineinfo = lineNBG1;
   Vdp2GenLineinfo( &info );
   Vdp2SetGetColor( &info );
   
   if (fixVdp2Regs->SCRCTL & 0x100)
   {
     info.isverticalscroll = 1;
     if (fixVdp2Regs->SCRCTL & 0x1)
     {
       info.verticalscrolltbl = 4 + ((fixVdp2Regs->VCSTA.all & 0x7FFFE) << 1);
       info.verticalscrollinc = 8;
     }
     else
     {
       info.verticalscrolltbl = (fixVdp2Regs->VCSTA.all & 0x7FFFE) << 1;
       info.verticalscrollinc = 4;
     }
   }
   else
     info.isverticalscroll = 0;

   
   if (info.isbitmap)
   {

     if (info.coordincx != 1.0f || info.coordincy != 1.0f){
       info.sh = (fixVdp2Regs->SCXIN1 & 0x7FF);
       info.sv = (fixVdp2Regs->SCYIN1 & 0x7FF);
       info.x = 0;
       info.y = 0;
       info.vertices[0] = 0;
       info.vertices[1] = 0;
       info.vertices[2] = vdp2width;
       info.vertices[3] = 0;
       info.vertices[4] = vdp2width;
       info.vertices[5] = vdp2height;
       info.vertices[6] = 0;
       info.vertices[7] = vdp2height;
       vdp2draw_struct infotmp = info;
       infotmp.cellw = vdp2width;
       if (vdp2height >= 448 )
         infotmp.cellh = (vdp2height>>1);
       else	
        infotmp.cellh = vdp2height;

       YglQuad(&infotmp, &texture, &tmpc);
       Vdp2DrawBitmapCoordinateInc(&info, &texture);
     }
     else{

       int xx, yy;
       int isCached = 0;

       if (info.islinescroll) // Nights Movie
       {
         info.sh = (fixVdp2Regs->SCXIN1 & 0x7FF);
         info.sv = (fixVdp2Regs->SCYIN1 & 0x7FF);
         info.x = 0;
         info.y = 0;
       }

       yy = info.y;
       while (yy + info.y < vdp2height)
       {
         xx = info.x;
         while (xx + info.x < vdp2width)
         {
           info.vertices[0] = xx;
           info.vertices[1] = yy;
           info.vertices[2] = (xx + info.cellw);
           info.vertices[3] = yy;
           info.vertices[4] = (xx + info.cellw);
           info.vertices[5] = (yy + info.cellh);
           info.vertices[6] = xx;
           info.vertices[7] = (yy + info.cellh);

           if (isCached == 0)
           {
             YglQuad(&info, &texture, &tmpc);
             if (info.islinescroll){
               Vdp2DrawBitmapLineScroll(&info, &texture);
             }
             else{
               Vdp2DrawCell(&info, &texture);
             }
             isCached = 1;
           }
           else{
             YglCachedQuad(&info, &tmpc);
           }
           xx += info.cellw;
         }
         yy += info.cellh;
     }
   }
   }
   else{
     if (info.islinescroll){
       info.x = (fixVdp2Regs->SCXIN1 & 0x7FF);
       info.y = (fixVdp2Regs->SCYIN1 & 0x7FF);
       Vdp2DrawMapPerLine(&info, &texture);
     }
     else{
       //Vdp2DrawMap(&info, &texture);
       info.x = fixVdp2Regs->SCXIN1 & 0x7FF;
       info.y = fixVdp2Regs->SCYIN1 & 0x7FF;
       Vdp2DrawMapTest(&info, &texture);
     }
   }
  
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG2(void)
{
   vdp2draw_struct info;
   YglTexture texture;
   info.dst=0;
   info.id = 2;
   info.uclipmode=0;
   info.cor = 0;
   info.cog = 0;
   info.cob = 0;
info.specialcolorfunction = 0;

   info.enable = fixVdp2Regs->BGON & 0x4;
   if (!info.enable) return;
   info.transparencyenable = !(fixVdp2Regs->BGON & 0x400);
   info.specialprimode = (fixVdp2Regs->SFPRMD >> 4) & 0x3;

   info.colornumber = (fixVdp2Regs->CHCTLB & 0x2) >> 1;
   info.mapwh = 2;

   Vdp2GeneratePerLineColorCalcuration(&info, NBG2);

   ReadPlaneSize(&info, fixVdp2Regs->PLSZ >> 4);
   info.x = - ((fixVdp2Regs->SCXN2 & 0x7FF) % (512 * info.planew));
   info.y = - ((fixVdp2Regs->SCYN2 & 0x7FF) % (512 * info.planeh));
   ReadPatternData(&info, fixVdp2Regs->PNCN2, fixVdp2Regs->CHCTLB & 0x1);

   ReadMosaicData(&info, 0x4, fixVdp2Regs);

   info.specialcolormode = (fixVdp2Regs->SFCCMD>>4) & 0x3;

   // 12.13 blur
   if ((fixVdp2Regs->CCCTL & 0xF000) == 0xD000){
     info.alpha = 0xFF;
     info.blendmode = VDP2_CC_BLUR;
   }
   else{

     if (fixVdp2Regs->CCCTL & 0x4)
     {
       info.alpha = ((~fixVdp2Regs->CCRNB & 0x1F) << 3) + 0x7;
       if (fixVdp2Regs->CCCTL & 0x100 && info.specialcolormode == 0)
       {
         info.blendmode = VDP2_CC_ADD;
       }
       else{
         info.blendmode = VDP2_CC_RATE;
       }
     }
     else{
       // 12.14 CCRTMD
       if (((fixVdp2Regs->CCCTL >> 9) & 0x01) == 0x01){
         info.alpha = ((~fixVdp2Regs->CCRNB & 0x1F) << 3) + 0x7;
       }
       else{
         info.alpha = 0xFF;
       }
       info.blendmode = VDP2_CC_NONE;
     }
   }

   info.linescreen = 0;
   if (fixVdp2Regs->LNCLEN & 0x4)
     info.linescreen = 1;

   info.coloroffset = fixVdp2Regs->CRAOFA & 0x700;
   ReadVdp2ColorOffset(fixVdp2Regs,&info, 0x4);

   if (info.lineTexture != 0){
     info.linescreen = 2;
     info.cor = 0;
     info.cog = 0;
     info.cob = 0;
   }
     

   info.linecheck_mask = 0x04;
   info.coordincx = info.coordincy = 1;
   info.priority = fixVdp2Regs->PRINB & 0x7;;
   info.PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2NBG2PlaneAddr;

   if (!(info.enable & Vdp2External.disptoggle) || (info.priority == 0) ||
      (fixVdp2Regs->BGON & 0x1 && (fixVdp2Regs->CHCTLA & 0x70) >> 4 >= 2)) // If NBG0 2048/32786/16M mode is enabled, don't draw
      return;
   
   // Window Mode
   info.bEnWin0 = (fixVdp2Regs->WCTLB >> 1) &0x01;
   info.WindowArea0 = (fixVdp2Regs->WCTLB >> 0) & 0x01;  
   info.bEnWin1 = (fixVdp2Regs->WCTLB >> 3) &0x01;
   info.WindowArea1 = (fixVdp2Regs->WCTLB >> 2) & 0x01;    
   info.LogicWin    = (fixVdp2Regs->WCTLB >> 7 ) & 0x01;

   Vdp2SetGetColor( &info );   
   
   info.islinescroll = 0;
   info.linescrolltbl = 0;
   info.lineinc = 0;   
   info.isverticalscroll = 0;
   info.x = fixVdp2Regs->SCXN2 & 0x7FF;
   info.y = fixVdp2Regs->SCYN2 & 0x7FF;
   Vdp2DrawMapTest(&info, &texture);
   
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG3(void)
{
   vdp2draw_struct info;
   YglTexture texture;
   info.id = 3;
   info.dst=0;
   info.uclipmode=0;
   info.cor = 0;
   info.cog = 0;
   info.cob = 0;
   info.specialcolorfunction = 0;


   info.enable = fixVdp2Regs->BGON & 0x8;
   if (!info.enable) return;
   info.transparencyenable = !(fixVdp2Regs->BGON & 0x800);
   info.specialprimode = (fixVdp2Regs->SFPRMD >> 6) & 0x3;

   info.colornumber = (fixVdp2Regs->CHCTLB & 0x20) >> 5;

   Vdp2GeneratePerLineColorCalcuration(&info, NBG3);
  

   info.mapwh = 2;

   ReadPlaneSize(&info, fixVdp2Regs->PLSZ >> 6);
   info.x = - ((fixVdp2Regs->SCXN3 & 0x7FF) % (512 * info.planew));
   info.y = - ((fixVdp2Regs->SCYN3 & 0x7FF) % (512 * info.planeh));
   ReadPatternData(&info, fixVdp2Regs->PNCN3, fixVdp2Regs->CHCTLB & 0x10);

   ReadMosaicData(&info, 0x8, fixVdp2Regs);

   info.specialcolormode = (fixVdp2Regs->SFCCMD>>6) & 0x03;
   
   // 12.13 blur
   if ((fixVdp2Regs->CCCTL & 0xF000) == 0xE000){
     info.alpha = 0xFF;
     info.blendmode = VDP2_CC_BLUR;
   }
   else{
     if (fixVdp2Regs->CCCTL & 0x8)
     {
       info.alpha = ((~fixVdp2Regs->CCRNB & 0x1F00) >> 5) + 0x7;
       if (fixVdp2Regs->CCCTL & 0x100 && info.specialcolormode == 0)
       {
         info.blendmode = VDP2_CC_ADD;
       }
       else{
         info.blendmode = VDP2_CC_RATE;
       }
     }
     else{
       // 12.14 CCRTMD
       if (((fixVdp2Regs->CCCTL >> 9) & 0x01) == 0x01){
         info.alpha = ((~fixVdp2Regs->CCRNB & 0x1F00) >> 5) + 0x7;
       }
       else{
         info.alpha = 0xFF;
       }

       info.blendmode = VDP2_CC_NONE;
     }
   }
   
   info.linescreen = 0;
   if (fixVdp2Regs->LNCLEN & 0x8)
     info.linescreen = 1;


   info.coloroffset = (fixVdp2Regs->CRAOFA & 0x7000) >> 4;
   ReadVdp2ColorOffset(fixVdp2Regs,&info, 0x8);
   if (info.lineTexture != 0){
     info.cor = 0;
     info.cog = 0;
     info.cob = 0;
     info.linescreen = 2;
   }

   info.linecheck_mask = 0x08;
   info.coordincx = info.coordincy = 1;

   info.priority = (fixVdp2Regs->PRINB >> 8) & 0x7;
   info.PlaneAddr = (void FASTCALL (*)(void *, int, Vdp2*))&Vdp2NBG3PlaneAddr;

   if (!(info.enable & Vdp2External.disptoggle) || (info.priority == 0) ||
      (fixVdp2Regs->BGON & 0x1 && (fixVdp2Regs->CHCTLA & 0x70) >> 4 == 4) || // If NBG0 16M mode is enabled, don't draw
      (fixVdp2Regs->BGON & 0x2 && (fixVdp2Regs->CHCTLA & 0x3000) >> 12 >= 2)) // If NBG1 2048/32786 is enabled, don't draw
      return;
 
   // Window Mode
   info.bEnWin0 = (fixVdp2Regs->WCTLB >> 9) &0x01;
   info.WindowArea0 = (fixVdp2Regs->WCTLB >> 8) & 0x01;
   info.bEnWin1 = (fixVdp2Regs->WCTLB >> 11) &0x01;
   info.WindowArea1 = (fixVdp2Regs->WCTLB >> 10) & 0x01;
   info.LogicWin    = (fixVdp2Regs->WCTLB >> 15 ) & 0x01;
   
   Vdp2SetGetColor( &info );   
  
   info.islinescroll = 0;
   info.linescrolltbl = 0;
   info.lineinc = 0;   
   info.isverticalscroll = 0;
   info.x = fixVdp2Regs->SCXN3 & 0x7FF;
   info.y = fixVdp2Regs->SCYN3 & 0x7FF;
   Vdp2DrawMapTest(&info, &texture);
   
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawRBG0(void)
{
  vdp2draw_struct  * info = &g_rgb0.info;
  g_rgb0.rgb_type = 0;

  info->dst = 0;
  info->id = 4;
  info->uclipmode = 0;
  info->cor = 0;
  info->cog = 0;
  info->cob = 0;
  info->specialcolorfunction = 0;
  info->enable = fixVdp2Regs->BGON & 0x10;
  if (!info->enable) return;
  info->priority = fixVdp2Regs->PRIR & 0x7;
  if (!(info->enable & Vdp2External.disptoggle) || (info->priority == 0)){

    if (Vdp1Regs->TVMR & 0x02){
      Vdp2ReadRotationTable(0, &paraA, fixVdp2Regs, Vdp2Ram);
    }
    return;
  }

  Vdp2GeneratePerLineColorCalcuration(info, RBG0);

   info->transparencyenable = !(fixVdp2Regs->BGON & 0x1000);
   info->specialprimode = (fixVdp2Regs->SFPRMD >> 8) & 0x3;

   info->colornumber = (fixVdp2Regs->CHCTLB & 0x7000) >> 12;

   info->bEnWin0     = (fixVdp2Regs->WCTLC >> 1) & 0x01;
   info->WindowArea0 = (fixVdp2Regs->WCTLC >> 0) & 0x01;

   info->bEnWin1     = (fixVdp2Regs->WCTLC >> 3) & 0x01;
   info->WindowArea1 = (fixVdp2Regs->WCTLC >> 2) & 0x01;

   info->LogicWin    = (fixVdp2Regs->WCTLC >> 7 ) & 0x01;

   info->islinescroll = 0;
   info->linescrolltbl = 0;
   info->lineinc = 0;   
   
   Vdp2ReadRotationTable(0, &paraA, fixVdp2Regs, Vdp2Ram);
   Vdp2ReadRotationTable(1, &paraB, fixVdp2Regs, Vdp2Ram);
   Vdp2ColorRamUpdated = 0;
   A0_Updated = 0;
   A1_Updated = 0;
   B0_Updated = 0;
   B1_Updated = 0;

   paraA.PlaneAddr = (void FASTCALL (*)(void *, int, Vdp2*))&Vdp2ParameterAPlaneAddr;
   paraB.PlaneAddr = (void FASTCALL (*)(void *, int, Vdp2*))&Vdp2ParameterBPlaneAddr;
   paraA.charaddr = (fixVdp2Regs->MPOFR & 0x7) * 0x20000;
   paraB.charaddr = (fixVdp2Regs->MPOFR & 0x70) * 0x2000;
   ReadPlaneSizeR(&paraA,fixVdp2Regs->PLSZ >> 8);
   ReadPlaneSizeR(&paraB,fixVdp2Regs->PLSZ >> 12);
   
    
    
    if( paraA.coefdatasize == 2)
    {
        if(paraA.coefmode < 3 )
        {
            info->GetKValueA = vdp2rGetKValue1W;
        }else{
            info->GetKValueA = vdp2rGetKValue1Wm3;
        }

    }else{
        if(paraA.coefmode < 3 )
        {
            info->GetKValueA = vdp2rGetKValue2W;
        }else{
            info->GetKValueA = vdp2rGetKValue2Wm3;
        }
    }

    if( paraB.coefdatasize == 2)
    {
        if(paraB.coefmode < 3 )
        {
            info->GetKValueB = vdp2rGetKValue1W;
        }else{
            info->GetKValueB = vdp2rGetKValue1Wm3;
        }

    }else{
        if(paraB.coefmode < 3 )
        {
            info->GetKValueB = vdp2rGetKValue2W;
        }else{
            info->GetKValueB = vdp2rGetKValue2Wm3;
        }
    }

   if( fixVdp2Regs->RPMD == 0x00 )
   {
      if(!(paraA.coefenab))
      {
         info->GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode00NoK;
      }else{
         info->GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode00WithK;
      }

   }else if( fixVdp2Regs->RPMD == 0x01 )
   {
      if(!(paraB.coefenab))
      {
         info->GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode01NoK;
      }else{
         info->GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode01WithK;
      }

   }else if( fixVdp2Regs->RPMD == 0x02 )
   {
      if(!(paraA.coefenab))
      {
         info->GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode02NoK;
      }else{
      if( paraB.coefenab )
        info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode02WithKAWithKB;
      else
      info->GetRParam = (Vdp2GetRParam_func)vdp2RGetParamMode02WithKA;
      }

   }else if( fixVdp2Regs->RPMD == 0x03 )
   {
    // Enable Window0(RPW0E)?
    if( ((fixVdp2Regs->WCTLD >> 1) & 0x01) == 0x01 )
    {
      info->pWinInfo = m_vWindinfo0;
      // RPW0A( inside = 0, outside = 1 )
      info->WindwAreaMode = (fixVdp2Regs->WCTLD & 0x01) ;
    // Enable Window1(RPW1E)?
    }else if(  ((fixVdp2Regs->WCTLD >> 3) & 0x01) == 0x01 )
    {
      info->pWinInfo = m_vWindinfo1;
      // RPW1A( inside = 0, outside = 1 )
      info->WindwAreaMode = ((fixVdp2Regs->WCTLD >>2)& 0x01) ;
    // Bad Setting Both Window is disabled
    }else{
      info->pWinInfo = m_vWindinfo0;
      info->WindwAreaMode = (fixVdp2Regs->WCTLD & 0x01) ;
    }

      if( paraA.coefenab == 0 && paraB.coefenab == 0 )
      {
         info->GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode03NoK;
      }else if( paraA.coefenab && paraB.coefenab == 0 )
      {
         info->GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode03WithKA;
      }else if( paraA.coefenab == 0 && paraB.coefenab )
      {
         info->GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode03WithKB;
      }else if( paraA.coefenab && paraB.coefenab )
      {
         info->GetRParam = (Vdp2GetRParam_func) vdp2RGetParamMode03WithK;
      }
   }
   
   
   paraA.screenover = (fixVdp2Regs->PLSZ >> 10)  & 0x03;
   paraB.screenover = (fixVdp2Regs->PLSZ >> 14)  & 0x03;

   
   
   // Figure out which Rotation Parameter we're uqrt
   switch (fixVdp2Regs->RPMD & 0x3)
   {
      case 0:
         // Parameter A
         info->rotatenum = 0;
     info->rotatemode = 0;
       info->PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterAPlaneAddr;
         break;
      case 1:
         // Parameter B
         info->rotatenum = 1;
     info->rotatemode = 0;
       info->PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterBPlaneAddr;
         break;
      case 2:
         // Parameter A+B switched via coefficients
         // FIX ME(need to figure out which Parameter is being used)
      case 3:
      default:
         // Parameter A+B switched via rotation parameter window
         // FIX ME(need to figure out which Parameter is being used)
         info->rotatenum = 0;
     info->rotatemode = 1 + (fixVdp2Regs->RPMD & 0x1);
       info->PlaneAddr = (void FASTCALL(*)(void *, int, Vdp2*))&Vdp2ParameterAPlaneAddr;
         break;
   }

   

   if((info->isbitmap = fixVdp2Regs->CHCTLB & 0x200) != 0)
   {
      // Bitmap Mode
      ReadBitmapSize(info, fixVdp2Regs->CHCTLB >> 10, 0x1);

      if (info->rotatenum == 0)
         // Parameter A
         info->charaddr = (fixVdp2Regs->MPOFR & 0x7) * 0x20000;
      else
         // Parameter B
         info->charaddr = (fixVdp2Regs->MPOFR & 0x70) * 0x2000;

      info->paladdr = (fixVdp2Regs->BMPNB & 0x7) << 4;
      info->flipfunction = 0;
      info->specialfunction = 0;
   }
   else
   {
      int i;
      // Tile Mode
      info->mapwh = 4;

      if (info->rotatenum == 0)
         // Parameter A
         ReadPlaneSize(info, fixVdp2Regs->PLSZ >> 8);
      else
         // Parameter B
         ReadPlaneSize(info, fixVdp2Regs->PLSZ >> 12);

      ReadPatternData(info, fixVdp2Regs->PNCR, fixVdp2Regs->CHCTLB & 0x100);
  
      paraA.ShiftPaneX = 8 + paraA.planew;
      paraA.ShiftPaneY = 8 + paraA.planeh;
      paraB.ShiftPaneX = 8 + paraB.planew;
      paraB.ShiftPaneY = 8 + paraB.planeh;      

      paraA.MskH = (8 * 64 * paraA.planew)-1;
      paraA.MskV = (8 * 64 * paraA.planeh)-1;
      paraB.MskH = (8 * 64 * paraB.planew)-1;
      paraB.MskV = (8 * 64 * paraB.planeh)-1;
    
     paraA.MaxH = 8 * 64 * paraA.planew * 4;
     paraA.MaxV = 8 * 64 * paraA.planeh * 4;
     paraB.MaxH = 8 * 64 * paraB.planew * 4;
     paraB.MaxV = 8 * 64 * paraB.planeh * 4;

     if( paraA.screenover == OVERMODE_512 )
     {
       paraA.MaxH = 512;
       paraA.MaxV = 512;
     }

     if( paraB.screenover == OVERMODE_512 )
     {
       paraB.MaxH = 512;
       paraB.MaxV = 512;
     }
     
     for( i=0; i<16; i++ )
       {
         paraA.PlaneAddr(info,i, fixVdp2Regs);
         paraA.PlaneAddrv[i] = info->addr;
         paraB.PlaneAddr(info, i, fixVdp2Regs);
         paraB.PlaneAddrv[i] = info->addr;
       }
   }

   ReadMosaicData(info, 0x10, fixVdp2Regs);

   info->specialcolormode = (fixVdp2Regs->SFCCMD>>8) & 0x03;
   info->blendmode = VDP2_CC_NONE;
   if( (fixVdp2Regs->LNCLEN & 0x10) == 0x00 )
   {
      info->LineColorBase = 0x00;
      paraA.lineaddr = 0xFFFFFFFF;
      paraB.lineaddr = 0xFFFFFFFF;
   }else{
//      info->alpha = 0xFF;
      info->LineColorBase = ((fixVdp2Regs->LCTA.all)&0x7FFFF) << 1;
      if( info->LineColorBase >= 0x80000 ) info->LineColorBase = 0x00;
      paraA.lineaddr = 0xFFFFFFFF;
      paraB.lineaddr = 0xFFFFFFFF;
   }

   // 12.13 blur
   if ((fixVdp2Regs->CCCTL & 0xF000) == 0x9000){
     info->alpha = 0xFF;
     info->blendmode = VDP2_CC_BLUR;
   }
   else{
     if ((fixVdp2Regs->CCCTL & 0x410) == 0x10)
     {
       if ((*Vdp2External.perline_alpha_draw & 0x10) != 0) {
            info->alpha = 0xFF;
       } else {
            info->alpha = ((~fixVdp2Regs->CCRR & 0x1F) << 3) + 0x7;
       }
       if (fixVdp2Regs->CCCTL & 0x100 && info->specialcolormode == 0)
       {
         info->blendmode = VDP2_CC_ADD;
       }
       else{
         info->blendmode = VDP2_CC_RATE;
       }
     }
     else{
       // 12.14 CCRTMD
       if (((fixVdp2Regs->CCCTL >> 9) & 0x01) == 0x01){
         info->alpha = ((~fixVdp2Regs->CCRR & 0x1F) << 3) + 0x7;
       }
       else{
         info->alpha = 0xFF;
       }
       info->blendmode = VDP2_CC_NONE;
     }
   }

   info->coloroffset = (fixVdp2Regs->CRAOFB & 0x7) << 8;

   ReadVdp2ColorOffset(fixVdp2Regs,info, 0x10);
   info->linecheck_mask = 0x10;
   info->coordincx = info->coordincy = 1;
   
   // Window Mode
   info->bEnWin0 = (fixVdp2Regs->WCTLC >> 1) &0x01;
   info->WindowArea0 = (fixVdp2Regs->WCTLC >> 0) & 0x01;  
   info->bEnWin1 = (fixVdp2Regs->WCTLC >> 3) &0x01;
   info->WindowArea1 = (fixVdp2Regs->WCTLC >> 2) & 0x01;    
   info->LogicWin    = (fixVdp2Regs->WCTLC >> 7 ) & 0x01;
   
   Vdp2SetGetColor( info );   
   Vdp2DrawRotation(&g_rgb0);

}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2DrawScreens(void)
{
  fixVdp2Regs = Vdp2RestoreRegs(0, Vdp2Lines);
  if (fixVdp2Regs == NULL) fixVdp2Regs = Vdp2Regs;
  memcpy(&baseVdp2Regs, fixVdp2Regs, sizeof(Vdp2));
  fixVdp2Regs = &baseVdp2Regs;

  Vdp2GenerateWindowInfo();

  if (g_rgb0.async){
    Vdp2DrawRBG0();
    FrameProfileAdd("RBG0 end");
  }

   Vdp2DrawBackScreen();
   Vdp2DrawLineColorScreen();

   Vdp2DrawNBG3();
   FrameProfileAdd("NBG3 end");
   Vdp2DrawNBG2();
   FrameProfileAdd("NBG2 end");
   Vdp2DrawNBG1();
   FrameProfileAdd("NBG1 end");
   Vdp2DrawNBG0();
   FrameProfileAdd("NBG0 end");
   if (!g_rgb0.async){
    Vdp2DrawRBG0();
    FrameProfileAdd("RBG0 end");
   }
   Vdp2DrawRotationSync();
}

//////////////////////////////////////////////////////////////////////////////

void VIDOGLVdp2SetResolution(u16 TVMD)
{
   int width=0, height=0;
   int wratio=1, hratio=1;

   // Horizontal Resolution
   switch (TVMD & 0x7)
   {
      case 0:
         width = 320;
         wratio = 1;
         break;
      case 1:
         width = 352;
         wratio = 1;
         break;
      case 2:
         width = 640;
         wratio = 2;
         break;
      case 3:
         width = 704;
         wratio = 2;
         break;
      case 4:
         width = 320;
         wratio = 1;
         break;
      case 5:
         width = 352;
         wratio = 1;
         break;
      case 6:
         width = 640;
         wratio = 2;
         break;
      case 7:
         width = 704;
         wratio = 2;
         break;
   }

   // Vertical Resolution
   switch ((TVMD >> 4) & 0x3)
   {
      case 0:
         height = 224;
         break;
      case 1: height = 240;
                 break;
      case 2: height = 256;
                 break;
      default: break;
   }

   hratio = 1;

   // Check for interlace
   switch ((TVMD >> 6) & 0x3)
   {
      case 3: // Double-density Interlace
         height *= 2;
         hratio = 2;
         vdp2_interlace = 1;
         break;
      case 2: // Single-density Interlace
      case 0: // Non-interlace
      default:
          vdp2_interlace = 0; 
          break;
   }

   SetSaturnResolution(width, height);
   Vdp1SetTextureRatio(wratio, hratio);
}

//////////////////////////////////////////////////////////////////////////////

void YglGetGlSize(int *width, int *height)
{
   *width = GlWidth;
   *height = GlHeight;
}

void VIDOGLGetNativeResolution(int *width, int *height, int*interlace)
{
   *width = vdp2width;
   *height = vdp2height;
   *interlace = vdp2_interlace;
}

void VIDOGLVdp2DispOff()
{
}

vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue2W( vdp2rotationparameter_struct * param, int index )
{
   float kval;
   int   kdata;

   if (index < 0) index = 0;
   if (index >= param->ktablesize) index = param->ktablesize-1;
      
   kdata = param->prefecth_k2w[index];
   if( kdata & 0x80000000 ) return NULL;

   kval = (float) (int) ((kdata & 0x00FFFFFF) | (kdata & 0x00800000 ? 0xFF800000 : 0x00000000)) / 65536.0f;
    
   switch( param->coefmode )
   {
   case 0: 
     param->kx = kval;
     param->ky = kval;
       break;
   case 1: 
       param->kx = kval;
       break;
   case 2: 
       param->ky = kval;
       break;
   }

   param->lineaddr = (kdata >> 24)&0x7F;
   return param;
}

vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue1W( vdp2rotationparameter_struct * param, int index )
{
   float kval;
   u16   kdata;
  
   if (index < 0) index = 0;
   if (index >= param->ktablesize) index = param->ktablesize - 1;

   kdata = param->prefecth_k1w[index];
   if( kdata & 0x8000 ) return NULL;

   kval = (float) (signed) ((kdata & 0x7FFF) | (kdata & 0x4000 ? 0x8000 : 0x0000)) / 1024.0f;
   
   switch( param->coefmode )
   {
   case 0: 
       param->kx = kval;
       param->ky = kval;
       break;
   case 1: 
       param->kx = kval;
       break;
   case 2: 
       param->ky = kval;
       break;
   }
   return param;
   
}

vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue2Wm3( vdp2rotationparameter_struct * param, int index )
{
   return param; // ToDo:
}

vdp2rotationparameter_struct * FASTCALL vdp2rGetKValue1Wm3( vdp2rotationparameter_struct * param, int index )
{
   return param; // ToDo:   
}


vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode00NoK( vdp2draw_struct * info, int h, int v )
{
   return &paraA;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode00WithK( vdp2draw_struct * info,int h, int v )
{
   h = paraA.KtablV+(paraA.deltaKAx * h);
   return info->GetKValueA( &paraA, h );
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode01NoK( vdp2draw_struct * info,int h, int v )
{
    return &paraB;   
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode01WithK( vdp2draw_struct * info,int h, int v )
{
   h = (paraB.KtablV+(paraB.deltaKAx * h));
   return info->GetKValueB( &paraB, h );   
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02NoK( vdp2draw_struct * info,int h, int v )
{
   return &paraA;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKA( vdp2draw_struct * info,int h, int v )
{
  if (info->GetKValueA(&paraA, (paraA.KtablV + (paraA.deltaKAx * h))) == NULL)
  {
        return &paraB;
    }
    return &paraA;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKAWithKB(vdp2draw_struct * info, int h, int v)
{
  if (info->GetKValueA(&paraA, (paraA.KtablV + (paraA.deltaKAx * h))) == NULL)
  {
    info->GetKValueB(&paraB, (paraB.KtablV + (paraB.deltaKAx * h)));
    return &paraB;
  }
  return &paraA;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode02WithKB( vdp2draw_struct * info,int h, int v )
{
   return &paraA;   
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03NoK( vdp2draw_struct * info,int h, int v )
{
   if( info->WindwAreaMode == 0 )
   {
      if( info->pWinInfo[v].WinShowLine == 0  )
      {
         return (&paraA);
      }else{
         if( h < info->pWinInfo[v].WinHStart || h >= info->pWinInfo[v].WinHEnd )
         {
            return (&paraB);
         }else{
            return (&paraA);
         }
      }
   }
   else
   {
      if( info->pWinInfo[v].WinShowLine == 0 )
      {
         return (&paraB);
      }else{
         if( h < info->pWinInfo[v].WinHStart || h >= info->pWinInfo[v].WinHEnd )
         {
            return (&paraA);
         }else{
            return (&paraB);
         }
      }
   }
   return NULL;   
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithKA( vdp2draw_struct * info,int h, int v )
{
  // Virtua Fighter2
    if( info->WindwAreaMode == 0 )
   {
      if( info->pWinInfo[v].WinShowLine == 0 )
      {
      h = (paraA.KtablV + (paraA.deltaKAx * h));
      return info->GetKValueA(&paraA, h);
      }else{
         if( h < info->pWinInfo[v].WinHStart || h >= info->pWinInfo[v].WinHEnd )
         {
            return (&paraB);
         }else{
            h = (paraA.KtablV+(paraA.deltaKAx * h));
            return info->GetKValueA( &paraA, h );
         }
      }
   }else{
        if( info->pWinInfo[v].WinShowLine == 0 )
        {
            h = (paraA.KtablV+(paraA.deltaKAx * h));
            return info->GetKValueA( &paraA, h );
        }else{
         if( h < info->pWinInfo[v].WinHStart || h >= info->pWinInfo[v].WinHEnd )
         {
            h = (paraA.KtablV+(paraA.deltaKAx * h));
            return info->GetKValueA( &paraA, h );
         }else{
            return (&paraB);
         }
      }
   }
   return NULL;

}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithKB( vdp2draw_struct * info,int h, int v )
{
   if( info->WindwAreaMode == 0 )
   {
      if( info->pWinInfo[v].WinShowLine == 0 )
      {
     return &paraA;
      }else{
         if( h < info->pWinInfo[v].WinHStart || h >= info->pWinInfo[v].WinHEnd )
         {
            h = (paraB.KtablV+(paraB.deltaKAx * h));
            return info->GetKValueB( &paraB, h );
         }else{
            return &paraA;
         }
      }
   }else{
      {
        if( info->pWinInfo[v].WinShowLine == 0 )
        {
      h = (paraB.KtablV + (paraB.deltaKAx * h));
      return info->GetKValueB(&paraB, h);
        }else{
         if( h < info->pWinInfo[v].WinHStart || h >= info->pWinInfo[v].WinHEnd )
         {
       return &paraA;
         }else{
            h = (paraB.KtablV+(paraB.deltaKAx * h));
            return info->GetKValueB( &paraB, h );
         }
         }
      }
   }
   return NULL;
}

vdp2rotationparameter_struct * FASTCALL vdp2RGetParamMode03WithK( vdp2draw_struct * info,int h, int v )
{
  vdp2rotationparameter_struct * p;
  
  // Final Fight Revenge
  if (info->WindwAreaMode == WA_INSIDE ) {
    if( info->pWinInfo[v].WinShowLine == 0 ) {
      h = (paraA.KtablV + (paraA.deltaKAx * h));
      p = info->GetKValueA(&paraA, h);
      if (p) return p;
      h = (paraB.KtablV + (paraB.deltaKAx * h));
      return info->GetKValueB(&paraB, h);
    }else{
      if( h < info->pWinInfo[v].WinHStart || h >= info->pWinInfo[v].WinHEnd ) {
        h = (paraA.KtablV + (paraA.deltaKAx * h));
        p = info->GetKValueA(&paraA, h);
        if (p) return p;
        h = (paraB.KtablV + (paraB.deltaKAx * h));
        return info->GetKValueB(&paraB, h);
      }else{
        h = (paraB.KtablV + (paraB.deltaKAx * h));
        p = info->GetKValueB(&paraB, h);
        if (p) return p;
        h = (paraA.KtablV + (paraA.deltaKAx * h));
        return info->GetKValueA(&paraA, h);
      }
    }
  }else{
    if( info->pWinInfo[v].WinShowLine == 0 ) {
      h = (paraB.KtablV + (paraB.deltaKAx * h));
      p = info->GetKValueB(&paraB, h);
      if (p) return p;
      h = (paraA.KtablV + (paraA.deltaKAx * h));
      return info->GetKValueA(&paraA, h);
    }else{
      if( h < info->pWinInfo[v].WinHStart || h >= info->pWinInfo[v].WinHEnd ) {
        h = (paraB.KtablV + (paraB.deltaKAx * h));
        p = info->GetKValueB(&paraB, h);
        if (p) return p;
        h = (paraA.KtablV + (paraA.deltaKAx * h));
        return info->GetKValueA(&paraA, h);
      }else{
        h = (paraA.KtablV + (paraA.deltaKAx * h));
        p = info->GetKValueA(&paraA, h);
        if (p) return p;
        h = (paraB.KtablV + (paraB.deltaKAx * h));
        return info->GetKValueB(&paraB, h);
      }
    }
  }
  return NULL;
}

void VIDOGLSetFilterMode(int type){
    _Ygl->aamode = type;
    return;
}

#endif

void VIDOGLSetSettingValueMode(int type, int value){
  
  switch (type) {
  case VDP_SETTING_FILTERMODE:
    _Ygl->aamode = value;
    break;
  case VDP_SETTING_RESOLUTION_MODE:
    _Ygl->resolution_mode = value;
    break;		
  case VDP_SETTING_POLYGON_MODE:
    if (value == GPU_TESSERATION && _Ygl->polygonmode != GPU_TESSERATION){
      YglTesserationProgramInit();
    }
    _Ygl->polygonmode = value;
  }
  
  return;
}

