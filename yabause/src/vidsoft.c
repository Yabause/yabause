/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004-2006 Theo Berkau
    Copyright 2006 Fabien Coulon

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "vidsoft.h"
#include "debug.h"
#include "vdp2.h"

#ifdef HAVE_LIBGL
#define USE_OPENGL
#endif

#ifdef USE_OPENGL
#include "ygl.h"
#include "yui.h"
#endif

#include <stdlib.h>

#if defined WORDS_BIGENDIAN
#define COLSAT2YAB16(priority,temp)            (priority | (temp & 0x7C00) << 1 | (temp & 0x3E0) << 14 | (temp & 0x1F) << 27)
#define COLSAT2YAB32(priority,temp)            ((temp & 0xFF << 24) | ((temp & 0xFF00) << 8) | ((temp & 0xFF0000) >> 8) | priority)
#define COLSATSTRIPPRIORITY(pixel)              (pixel | 0xFF)
#else
#define COLSAT2YAB16(priority,temp)            (priority << 24 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9)
#define COLSAT2YAB32(priority,temp)            (priority << 24 | (temp & 0xFF0000) | (temp & 0xFF00) | (temp & 0xFF))
#define COLSATSTRIPPRIORITY(pixel)             (0xFF000000 | pixel)
#endif

#define COLOR_ADDt(b)		(b>0xFF?0xFF:(b<0?0:b))
#define COLOR_ADDb(b1,b2)	COLOR_ADDt((signed) (b1) + (b2))
#ifdef WORDS_BIGENDIAN
#define COLOR_ADD(l,r,g,b)	(COLOR_ADDb((l >> 24) & 0xFF, r) << 24) | \
				(COLOR_ADDb((l >> 16) & 0xFF, g) << 16) | \
				(COLOR_ADDb((l >> 8) & 0xFF, b) << 8) | \
				(l & 0xFF)
#else
#define COLOR_ADD(l,r,g,b)	COLOR_ADDb((l & 0xFF), r) | \
				(COLOR_ADDb((l >> 8 ) & 0xFF, g) << 8) | \
				(COLOR_ADDb((l >> 16 ) & 0xFF, b) << 16) | \
				(l & 0xFF000000)
#endif

int VIDSoftInit(void);
void VIDSoftDeInit(void);
void VIDSoftResize(unsigned int, unsigned int, int);
int VIDSoftIsFullscreen(void);
int VIDSoftVdp1Reset(void);
void VIDSoftVdp1DrawStart(void);
void VIDSoftVdp1DrawEnd(void);
void VIDSoftVdp1NormalSpriteDraw(void);
void VIDSoftVdp1ScaledSpriteDraw(void);
void VIDSoftVdp1DistortedSpriteDraw(void);
void VIDSoftVdp1PolygonDraw(void);
void VIDSoftVdp1PolylineDraw(void);
void VIDSoftVdp1LineDraw(void);
void VIDSoftVdp1UserClipping(void);
void VIDSoftVdp1SystemClipping(void);
void VIDSoftVdp1LocalCoordinate(void);
int VIDSoftVdp2Reset(void);
void VIDSoftVdp2DrawStart(void);
void VIDSoftVdp2DrawEnd(void);
void VIDSoftVdp2DrawScreens(void);
void VIDSoftVdp2SetResolution(u16 TVMD);
void FASTCALL VIDSoftVdp2SetPriorityNBG0(int priority);
void FASTCALL VIDSoftVdp2SetPriorityNBG1(int priority);
void FASTCALL VIDSoftVdp2SetPriorityNBG2(int priority);
void FASTCALL VIDSoftVdp2SetPriorityNBG3(int priority);
void FASTCALL VIDSoftVdp2SetPriorityRBG0(int priority);
void VIDSoftVdp2ToggleDisplayNBG0(void);
void VIDSoftVdp2ToggleDisplayNBG1(void);
void VIDSoftVdp2ToggleDisplayNBG2(void);
void VIDSoftVdp2ToggleDisplayNBG3(void);
void VIDSoftVdp2ToggleDisplayRBG0(void);
void VIDSoftOnScreenDebugMessage(char *string, ...);

VideoInterface_struct VIDSoft = {
VIDCORE_SOFT,
"Software Video Interface",
VIDSoftInit,
VIDSoftDeInit,
VIDSoftResize,
VIDSoftIsFullscreen,
VIDSoftVdp1Reset,
VIDSoftVdp1DrawStart,
VIDSoftVdp1DrawEnd,
VIDSoftVdp1NormalSpriteDraw,
VIDSoftVdp1ScaledSpriteDraw,
VIDSoftVdp1DistortedSpriteDraw,
VIDSoftVdp1PolygonDraw,
VIDSoftVdp1PolylineDraw,
VIDSoftVdp1LineDraw,
VIDSoftVdp1UserClipping,
VIDSoftVdp1SystemClipping,
VIDSoftVdp1LocalCoordinate,
VIDSoftVdp2Reset,
VIDSoftVdp2DrawStart,
VIDSoftVdp2DrawEnd,
VIDSoftVdp2DrawScreens,
VIDSoftVdp2SetResolution,
VIDSoftVdp2SetPriorityNBG0,
VIDSoftVdp2SetPriorityNBG1,
VIDSoftVdp2SetPriorityNBG2,
VIDSoftVdp2SetPriorityNBG3,
VIDSoftVdp2SetPriorityRBG0,
VIDSoftVdp2ToggleDisplayNBG0,
VIDSoftVdp2ToggleDisplayNBG1,
VIDSoftVdp2ToggleDisplayNBG2,
VIDSoftVdp2ToggleDisplayNBG3,
VIDSoftVdp2ToggleDisplayRBG0,
VIDSoftOnScreenDebugMessage,
};

u32 *dispbuffer=NULL;
u8 *vdp1framebuffer=NULL;
u32 *vdp2framebuffer=NULL;

static int vdp1width;
static int vdp1height;
static int vdp1clipxstart;
static int vdp1clipxend;
static int vdp1clipystart;
static int vdp1clipyend;
static int vdp1pixelsize;
static int vdp1spritetype;
static int vdp2width;
static int vdp2height;
static int vdp2disptoggle=0xFF;
static int nbg0priority=0;
static int nbg1priority=0;
static int nbg2priority=0;
static int nbg3priority=0;
static int rbg0priority=0;
static int outputwidth;
static int outputheight;

typedef struct
{
   int cellw, cellh;
   int flipfunction;
   int priority;

   int mapwh;
   int planew, planeh;
   int pagewh;
   int patternwh;
   int patterndatasize;
   int specialfunction;
   u32 addr, charaddr, paladdr;
   int colornumber;
   int isbitmap;
   u16 supplementdata;
   int auxmode;
   int enable;
   int x, y;
   int alpha;
   int coloroffset;
   int transparencyenable;
   int specialprimode;

   s32 cor;
   s32 cog;
   s32 cob;

   float coordincx, coordincy;
   void FASTCALL (* PlaneAddr)(void *, int);
   u32 FASTCALL (* PostPixelFetchCalc)(void *, u32);
   int patternpixelwh;
   int draww;
   int drawh;
} vdp2draw_struct;

typedef struct
{
   float Xst;
   float Yst;
   float Zst;
   float deltaXst;
   float deltaYst;
   float deltaX;
   float deltaY;
   float A;
   float B;
   float C;
   float D;
   float E;
   float F;
   s32 Px;
   s32 Py;
   s32 Pz;
   s32 Cx;
   s32 Cy;
   s32 Cz;
   float Mx;
   float My;
   float kx;
   float ky;
   float KAst;
   float deltaKAst;
   float deltaKAx;
   int coefenab;
} vdp2rotationparameter_struct;

typedef struct { s16 x; s16 y; } vdp1vertex;

//////////////////////////////////////////////////////////////////////////////

static INLINE void vdp2putpixel32(s32 x, s32 y, u32 color, int priority)
{
   vdp2framebuffer[(y * vdp2width) + x] = COLSAT2YAB32(priority, color);
}

//////////////////////////////////////////////////////////////////////////////

static INLINE u8 Vdp2GetPixelPriority(u32 pixel)
{
#if defined WORDS_BIGENDIAN
   return pixel;
#else
   return pixel >> 24;
#endif
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void puthline16(s32 x, s32 y, s32 width, u16 color, int priority)
{
   u32 *buffer = vdp2framebuffer + (y * vdp2width) + x;
   u32 dot=COLSAT2YAB16(priority, color);
   int i;

   for (i = 0; i < width; i++)
      buffer[i] = dot;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL Vdp2ColorRamGetColor(u32 addr)
{
   switch(Vdp2Internal.ColorMode)
   {
      case 0:
      {
         u32 tmp;
         addr <<= 1;
         tmp = T2ReadWord(Vdp2ColorRam, addr & 0xFFF);
         return (((tmp & 0x1F) << 3) | ((tmp & 0x03E0) << 6) | ((tmp & 0x7C00) << 9));
      }
      case 1:
      {
         u32 tmp;
         addr <<= 1;
         tmp = T2ReadWord(Vdp2ColorRam, addr & 0xFFF);
         return (((tmp & 0x1F) << 3) | ((tmp & 0x03E0) << 6) | ((tmp & 0x7C00) << 9));
      }
      case 2:
      {
         addr <<= 2;   
         return T2ReadLong(Vdp2ColorRam, addr & 0xFFF);
      }
      default: break;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

typedef struct
{
   int xstart, ystart;
   int xend, yend;
   int pixeloffset;
   int lineincrement;
} clipping_struct;

static void INLINE HandleClipping(vdp2draw_struct *info, clipping_struct *clip)
{
   clip->pixeloffset=0;
   clip->lineincrement=0;

   // Handle clipping(both window and screen clipping)
   if (info->x < 0)
   {
      clip->xstart = 0;
      clip->xend = (info->x+info->cellw);
      clip->pixeloffset = 0 - info->x;
      clip->lineincrement = 0 - info->x;
   }
   else
   {
      clip->xstart = info->x;

      if ((info->x+info->cellw) > vdp2width)
      {
         clip->xend = vdp2width;
         clip->lineincrement = (info->x+info->cellw) - vdp2width;
      }
      else
         clip->xend = (info->x+info->cellw);
   }

   if (info->y < 0)
   {
      clip->ystart = 0;
      clip->yend = (info->y+info->cellh);
      clip->pixeloffset =  (info->cellw * (0 - info->y)) + clip->pixeloffset;
   }
   else
   {
      clip->ystart = info->y;

      if ((info->y+info->cellh) >= vdp2height)
         clip->yend = vdp2height;
      else
         clip->yend = (info->y+info->cellh);
   }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DrawScrollBitmap(vdp2draw_struct *info)
{
   int i, i2;
   clipping_struct clip;

   HandleClipping(info, &clip);

   switch (info->colornumber)
   {
      case 0: // 4 BPP(16 colors)
         // fix me
         LOG("vdp2 bitmap 4 bpp draw\n");
         return;
      case 1: // 8 BPP(256 colors)
         info->charaddr += clip.pixeloffset;
         
         for (i = clip.ystart; i < clip.yend; i++)
         {
            for (i2 = clip.xstart; i2 < clip.xend; i2++)
            {
               u32 color = T1ReadByte(Vdp2Ram, info->charaddr);
               info->charaddr++;

               if (color == 0 && info->transparencyenable)
                  continue;               

               color = Vdp2ColorRamGetColor(info->coloroffset + (info->paladdr | color));
               vdp2putpixel32(i2, i, info->PostPixelFetchCalc(info, color), info->priority);
            }

            info->charaddr += clip.lineincrement;
         }

         return;
      case 3: // 15 BPP
         clip.pixeloffset *= 2;
         clip.lineincrement *= 2;

         info->charaddr += clip.pixeloffset;

         for (i = clip.ystart; i < clip.yend; i++)
         {
            for (i2 = clip.xstart; i2 < clip.xend; i2++)
            {
               u32 color = T1ReadWord(Vdp2Ram, info->charaddr);
               info->charaddr += 2;

               if ((color & 0x8000) == 0 && info->transparencyenable)
                  continue;

               vdp2framebuffer[(i * vdp2width) + i2] = info->PostPixelFetchCalc(info, COLSAT2YAB16(info->priority, color));
            }

            info->charaddr += clip.lineincrement;
         }

         return;
      case 4: // 24 BPP
         clip.pixeloffset *= 4;
         clip.lineincrement *= 4;

         info->charaddr += clip.pixeloffset;

         for (i = clip.ystart; i < clip.yend; i++)
         {
            for (i2 = clip.xstart; i2 < clip.xend; i2++)
            {
               u32 color = T1ReadLong(Vdp2Ram, info->charaddr);
               info->charaddr += 4;

               if ((color & 0x80000000) == 0 && info->transparencyenable)
                  continue;

               vdp2putpixel32(i2, i, info->PostPixelFetchCalc(info, color), info->priority);
            }
         }
         return;
      default: break;
   }
}

//////////////////////////////////////////////////////////////////////////////

#define Vdp2DrawCell4bpp(mask, shift) \
   if ((dot & mask) == 0 && info->transparencyenable) {} \
   else \
   { \
      color = Vdp2ColorRamGetColor(info->coloroffset + (info->paladdr | ((dot & mask) >> shift))); \
      vdp2putpixel32(i2, i, info->PostPixelFetchCalc(info, color), info->priority); \
   }

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2DrawCell(vdp2draw_struct *info)
{
   u32 color;
   int i, i2;
   clipping_struct clip;
   u32 newcharaddr;
   int addrincrement=1;

   HandleClipping(info, &clip);

   if (info->flipfunction & 0x1)
   {
      // Horizontal flip
   }

   if (info->flipfunction & 0x2)
   {
      // Vertical flip
//      clip.pixeloffset = (info.w * info.h) - clip.pixeloffset;
//      clip.lineincrement = 0 - clip.lineincrement;
   }

   switch(info->colornumber)
   {
      case 0: // 4 BPP
         if ((clip.lineincrement | clip.pixeloffset) == 0)
         {
            for (i = clip.ystart; i < clip.yend; i++)
            {
               u32 dot;
               u32 color;

               i2 = clip.xstart;

               // Fetch Pixel 1/2/3/4/5/6/7/8
               dot = T1ReadLong(Vdp2Ram, info->charaddr);
               info->charaddr+=4;

               // Draw 8 Pixels
               Vdp2DrawCell4bpp(0xF0000000, 28)
               i2++;
               Vdp2DrawCell4bpp(0x0F000000, 24)
               i2++;
               Vdp2DrawCell4bpp(0x00F00000, 20)
               i2++;
               Vdp2DrawCell4bpp(0x000F0000, 16)
               i2++;
               Vdp2DrawCell4bpp(0x0000F000, 12)
               i2++;
               Vdp2DrawCell4bpp(0x00000F00, 8)
               i2++;
               Vdp2DrawCell4bpp(0x000000F0, 4)
               i2++;
               Vdp2DrawCell4bpp(0x0000000F, 0)
               i2++;
            }
         }
         else
         {
            u8 dot;

            newcharaddr = info->charaddr + ((info->cellw * info->cellh) >> 1);

            info->charaddr <<= 1;
            info->charaddr += clip.pixeloffset;

            for (i = clip.ystart; i < clip.yend; i++)
            {
               dot = T1ReadByte(Vdp2Ram, info->charaddr >> 1);
               info->charaddr++;

               for (i2 = clip.xstart; i2 < clip.xend; i2++)
               {
                  u32 color;

                  // Draw two pixels
                  if(info->charaddr & 0x1)
                  {
                     Vdp2DrawCell4bpp(0xF0, 4)
                     info->charaddr++;
                  }
                  else
                  {
                     Vdp2DrawCell4bpp(0x0F, 0)
                     dot = T1ReadByte(Vdp2Ram, info->charaddr >> 1);
                     info->charaddr++;
                  }
               }
               info->charaddr += clip.lineincrement;
            }

            info->charaddr = newcharaddr;
         }
         break;
      case 1: // 8 BPP
         newcharaddr = info->charaddr + (info->cellw * info->cellh);   
         info->charaddr += clip.pixeloffset;
         
         for (i = clip.ystart; i < clip.yend; i++)
         {
            for (i2 = clip.xstart; i2 < clip.xend; i2++)
            {
               u32 color = T1ReadByte(Vdp2Ram, info->charaddr);
               info->charaddr++;

               if (color == 0 && info->transparencyenable)
                  continue;

               color = Vdp2ColorRamGetColor(info->coloroffset + (info->paladdr | color));
               vdp2putpixel32(i2, i, info->PostPixelFetchCalc(info, color), info->priority);
            }

            info->charaddr += clip.lineincrement;
         }

         info->charaddr = newcharaddr;

         break;
    case 2: // 16 BPP(palette)
/*
      for(i = 0;i < info->cellh;i++)
      {
        for(j = 0;j < info->cellw;j++)
        {
          u16 dot = T1ReadWord(Vdp2Ram, info->charaddr & 0x7FFFF);
          if ((dot == 0) && info->transparencyenable) color = 0x00000000;
          else color = Vdp2ColorRamGetColor(info->coloroffset + dot, info->alpha);
          info->charaddr += 2;
          *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
	}
        texture->textdata += texture->w;
      }
*/
      break;
    case 3: // 16 BPP(RGB)
/*
      for(i = 0;i < info->cellh;i++)
      {
        for(j = 0;j < info->cellw;j++)
        {
          u16 dot = T1ReadWord(Vdp2Ram, info->charaddr & 0x7FFFF);
          info->charaddr += 2;
          if (!(dot & 0x8000) && info->transparencyenable) color = 0x00000000;
	  else color = SAT2YAB1(0xFF, dot);
          *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
	}
        texture->textdata += texture->w;
      }
*/
      break;
    case 4: // 32 BPP
/*
      for(i = 0;i < info->cellh;i++)
      {
        for(j = 0;j < info->cellw;j++)
        {
          u16 dot1 = T1ReadWord(Vdp2Ram, info->charaddr & 0x7FFFF);
          info->charaddr += 2;
          u16 dot2 = T1ReadWord(Vdp2Ram, info->charaddr & 0x7FFFF);
          info->charaddr += 2;
          if (!(dot1 & 0x8000) && info->transparencyenable) color = 0x00000000;
          else color = SAT2YAB2(info->alpha, dot1, dot2);
          *texture->textdata++ = COLOR_ADD(color,info->cor,info->cog,info->cob);
	}
        texture->textdata += texture->w;
      }
*/
      break;
  }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawPattern(vdp2draw_struct *info)
{
   int * c;

//   if (info->specialprimode == 1)
//      tile.priority = (info->priority & 0xFFFFFFFE) | info->specialfunction;
//   else
//      tile.priority = info->priority;

   switch(info->patternwh)
   {
      case 1:
         Vdp2DrawCell(info);
         info->x += 8;
         info->y += 8;
         break;
      case 2:
         Vdp2DrawCell(info);
         info->x += 8;
         Vdp2DrawCell(info);
         info->x -= 8;
         info->y += 8;
         Vdp2DrawCell(info);
         info->x += 8;
         Vdp2DrawCell(info);
         info->x += 8;
         info->y += 8;
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

         switch(info->colornumber)
         {
            case 0: // in 16 colors
               info->paladdr = ((tmp & 0xF000) >> 8) | ((info->supplementdata & 0xE0) << 3);
               break;
            default: // not in 16 colors
               info->paladdr = (tmp & 0x7000) >> 4;
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
         info->paladdr = (tmp1 & 0x7F) << 4;
         info->specialfunction = (tmp1 & 0x2000) >> 13;
         break;
      }
   }

   if (!(Vdp2Regs->VRSIZE & 0x8000))
      info->charaddr &= 0x3FFF;

   info->charaddr *= 0x20; // selon Runik
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawPage(vdp2draw_struct *info)
{
   int X, Y;
   int i, j;

   X = info->x;
   for(i = 0;i < info->pagewh;i++)
   {
      Y = info->y;
      info->x = X;
      for(j = 0;j < info->pagewh;j++)
      {
         info->y = Y;
         if ((info->x >= -info->patternpixelwh) &&
             (info->y >= -info->patternpixelwh) &&
             (info->x <= info->draww) &&
             (info->y <= info->drawh))
         {
            Vdp2PatternAddr(info);
            Vdp2DrawPattern(info);
         }
         else
         {
            info->addr += info->patterndatasize * 2;
            info->x += info->patternpixelwh;
            info->y += info->patternpixelwh;
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawPlane(vdp2draw_struct *info)
{
   int X, Y;
   int i, j;

   X = info->x;
   for(i = 0;i < info->planeh;i++)
   {
      Y = info->y;
      info->x = X;
      for(j = 0;j < info->planew;j++)
      {
         info->y = Y;
         Vdp2DrawPage(info);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawMap(vdp2draw_struct *info)
{
   int i, j;
   int X, Y;
   u32 lastplane;

   X = info->x;
   lastplane=0xFFFFFFFF;

   info->patternpixelwh = 8 * info->patternwh;
   info->draww = (int)((float)vdp2width / info->coordincx);
   info->drawh = (int)((float)vdp2height / info->coordincy);

   for(i = 0;i < info->mapwh;i++)
   {
      Y = info->y;
      info->x = X;
      for(j = 0;j < info->mapwh;j++)
      {
         info->y = Y;
         info->PlaneAddr(info, info->mapwh * i + j);
         if (info->addr != lastplane)
         {
            Vdp2DrawPlane(info);
            lastplane = info->addr;
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2ReadRotationTable(int which, vdp2rotationparameter_struct *parameter)
{
   s32 i;
   u32 addr;

   addr = Vdp2Regs->RPTA.all << 1;

   if (which == 0)
   {
      // Rotation Parameter A
      addr &= 0x000FFF7C;
      parameter->coefenab = Vdp2Regs->KTCTL & 0x1;
   }
   else
   {
      // Rotation Parameter B
      addr = (addr & 0x000FFFFC) | 0x00000080;
      parameter->coefenab = Vdp2Regs->KTCTL & 0x100;
   }

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->Xst = (float) (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->Yst = (float) (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->Zst = (float) (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->deltaXst = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->deltaYst = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->deltaX = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->deltaY = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->A = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->B = (float) (signed) ((i & 0x000FFFC0) | ((i & 0x00080000) ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->C = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->D = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->E = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->F = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Px = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 2;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Py = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 2;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Pz = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 4;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Cx = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 2;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Cy = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 2;

   i = T1ReadWord(Vdp2Ram, addr);
   parameter->Cz = ((i & 0x3FFF) | (i & 0x2000 ? 0xE000 : 0x0000));
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->Mx = (float) (signed) ((i & 0x3FFFFFC0) | (i & 0x20000000 ? 0xE0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->My = (float) (signed) ((i & 0x3FFFFFC0) | (i & 0x20000000 ? 0xE0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->kx = (float) (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(Vdp2Ram, addr);
   parameter->ky = (float) (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000)) / 65536;
   addr += 4;

   if (parameter->coefenab)
   {
      // Handle coefficients here
   }
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL DoNothing(void *info, u32 pixel)
{
   return pixel;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL DoColorOffset(void *info, u32 pixel)
{
    return COLOR_ADD(pixel, ((vdp2draw_struct *)info)->cor,
                     ((vdp2draw_struct *)info)->cog,
                     ((vdp2draw_struct *)info)->cob);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL DoColorCalc(void *info, u32 pixel)
{
   // should be doing color calculation here
   return pixel;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL DoColorCalcWithColorOffset(void *info, u32 pixel)
{
   // should be doing color calculation here

   return COLOR_ADD(pixel, ((vdp2draw_struct *)info)->cor,
                    ((vdp2draw_struct *)info)->cog,
                    ((vdp2draw_struct *)info)->cob);
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawBackScreen(void)
{
   int i;

   // Only draw black if TVMD's DISP and BDCLMD bits are cleared
   if ((Vdp2Regs->TVMD & 0x8000) == 0 && (Vdp2Regs->TVMD & 0x100) == 0)
   {
      // Draw Black
      for (i = 0; i < (vdp2width * vdp2height); i++)
         vdp2framebuffer[i] = 0;
   }
   else
   {
      // Draw Back Screen
      u32 scrAddr;
      u16 dot;

      if (Vdp2Regs->VRSIZE & 0x8000)
         scrAddr = (((Vdp2Regs->BKTAU & 0x7) << 16) | Vdp2Regs->BKTAL) * 2;
      else
         scrAddr = (((Vdp2Regs->BKTAU & 0x3) << 16) | Vdp2Regs->BKTAL) * 2;

      if (Vdp2Regs->BKTAU & 0x8000)
      {
         // Per Line
         for (i = 0; i < vdp2height; i++)
         {
            dot = T1ReadWord(Vdp2Ram, scrAddr);
            scrAddr += 2;

            puthline16(0, i, vdp2width, dot, 0);
         }
      }
      else
      {
         // Single Color
         dot = T1ReadWord(Vdp2Ram, scrAddr);

         for (i = 0; i < (vdp2width * vdp2height); i++)
            vdp2framebuffer[i] = COLSAT2YAB16(0, dot);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2NBG0PlaneAddr(vdp2draw_struct *info, int i)
{
   u32 offset = (Vdp2Regs->MPOFN & 0x7) << 6;
   u32 tmp=0;
   int deca;
   int multi;

   switch(i)
   {
      case 0:
         tmp = offset | (Vdp2Regs->MPABN0 & 0xFF);
         break;
      case 1:
         tmp = offset | (Vdp2Regs->MPABN0 >> 8);
         break;
      case 2:
         tmp = offset | (Vdp2Regs->MPCDN0 & 0xFF);
         break;
      case 3:
         tmp = offset | (Vdp2Regs->MPCDN0 >> 8);
         break;
   }

   deca = info->planeh + info->planew - 2;
   multi = info->planeh * info->planew;

   //if (Vdp2Regs->VRSIZE & 0x8000)
   //{
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
         else
            info->addr = (tmp >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
      }
   /*}
   else
   {
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
      }
   }*/
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG0(void)
{
   vdp2draw_struct info;
   int i, i2;
   u32 linescrolladdr;

   /* FIXME should start by checking if it's a normal
    * or rotate scroll screen
    */
   info.enable = Vdp2Regs->BGON & 0x1;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x100);
   info.specialprimode = Vdp2Regs->SFPRMD & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLA & 0x70) >> 4;

   if((info.isbitmap = Vdp2Regs->CHCTLA & 0x2) != 0)
   {
      // Bitmap Mode

      switch((Vdp2Regs->CHCTLA & 0xC) >> 2)
      {
         case 0: info.cellw = 512;
                 info.cellh = 256;
                 break;
         case 1: info.cellw = 512;
                 info.cellh = 512;
                 break;
         case 2: info.cellw = 1024;
                 info.cellh = 256;
                 break;
         case 3: info.cellw = 1024;
                 info.cellh = 512;
                 break;
      }

      info.x = - ((Vdp2Regs->SCXIN0 & 0x7FF) % info.cellw);
      info.y = - ((Vdp2Regs->SCYIN0 & 0x7FF) % info.cellh);

      info.charaddr = (Vdp2Regs->MPOFN & 0x7) * 0x20000;
      info.paladdr = (Vdp2Regs->BMPNA & 0x7) << 8;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else
   {
      // Tile Mode
      info.mapwh = 2;

      switch(Vdp2Regs->PLSZ & 0x3)
      {
         case 0:
            info.planew = info.planeh = 1;
            break;
         case 1:
            info.planew = 2;
            info.planeh = 1;
            break;
         case 3:
            info.planew = info.planeh = 2;
            break;
         default: // Not sure what 0x2 does
            info.planew = info.planeh = 1;
            break;
      }

      info.x = - ((Vdp2Regs->SCXIN0 & 0x7FF) % (512 * info.planew));
      info.y = - ((Vdp2Regs->SCYIN0 & 0x7FF) % (512 * info.planeh));

      if(Vdp2Regs->PNCN0 & 0x8000)
         info.patterndatasize = 1;
      else
         info.patterndatasize = 2;

      if(Vdp2Regs->CHCTLA & 0x1)
         info.patternwh = 2;
      else
         info.patternwh = 1;

      info.pagewh = 64/info.patternwh;
      info.cellw = info.cellh = 8;
      info.supplementdata = Vdp2Regs->PNCN0 & 0x3FF;
      info.auxmode = (Vdp2Regs->PNCN0 & 0x4000) >> 14;
   }

   if (Vdp2Regs->CCCTL & 0x1)
      info.alpha = ((~Vdp2Regs->CCRNA & 0x1F) << 3) + 0x7;
   else
      info.alpha = 0xFF;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x7) << 8;

   if (Vdp2Regs->CLOFEN & 0x1)
   {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x1)
      {
         // color offset B
         info.cor = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            info.cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
         info.cor = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            info.cob |= 0xFFFFFF00;
      }

      if (Vdp2Regs->CCCTL & 0x1)
         info.PostPixelFetchCalc = &DoColorCalcWithColorOffset;
      else
         info.PostPixelFetchCalc = &DoColorOffset;
   }
   else // color offset disable
   {
      if (Vdp2Regs->CCCTL & 0x1)
         info.PostPixelFetchCalc = &DoColorCalc;
      else
         info.PostPixelFetchCalc = &DoNothing;
   }

   info.coordincx = (float) 65536 / (Vdp2Regs->ZMXN0.all & 0x7FF00);
   info.coordincy = (float) 65536 / (Vdp2Regs->ZMYN0.all & 0x7FF00);

   info.priority = nbg0priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG0PlaneAddr;

   if (!(info.enable & vdp2disptoggle))
      return;

   if (info.isbitmap)
      Vdp2DrawScrollBitmap(&info);
   else
      Vdp2DrawMap(&info);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2NBG1PlaneAddr(vdp2draw_struct *info, int i)
{
   u32 offset = (Vdp2Regs->MPOFN & 0x70) << 2;
   u32 tmp=0;
   int deca;
   int multi;

   switch(i)
   {
      case 0:
         tmp = offset | (Vdp2Regs->MPABN1 & 0xFF);
         break;
      case 1:
         tmp = offset | (Vdp2Regs->MPABN1 >> 8);
         break;
      case 2:
         tmp = offset | (Vdp2Regs->MPCDN1 & 0xFF);
         break;
      case 3:
         tmp = offset | (Vdp2Regs->MPCDN1 >> 8);
         break;
   }

   deca = info->planeh + info->planew - 2;
   multi = info->planeh * info->planew;

   //if (Vdp2Regs->VRSIZE & 0x8000)
   //{
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
         else
            info->addr = (tmp >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
      }
   /*}
   else
   {
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
      }
   }*/
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG1(void)
{
   vdp2draw_struct info;

   info.enable = Vdp2Regs->BGON & 0x2;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x200);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 2) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLA & 0x3000) >> 12;

   if((info.isbitmap = Vdp2Regs->CHCTLA & 0x200) != 0)
   {
      switch((Vdp2Regs->CHCTLA & 0xC00) >> 10)
      {
         case 0: info.cellw = 512;
                 info.cellh = 256;
                 break;
         case 1: info.cellw = 512;
                 info.cellh = 512;
                 break;
         case 2: info.cellw = 1024;
                 info.cellh = 256;
                 break;
         case 3: info.cellw = 1024;
                 info.cellh = 512;
                 break;
      }

      info.x = - ((Vdp2Regs->SCXIN1 & 0x7FF) % info.cellw);
      info.y = - ((Vdp2Regs->SCYIN1 & 0x7FF) % info.cellh);

      info.charaddr = ((Vdp2Regs->MPOFN & 0x70) >> 4) * 0x20000;
      info.paladdr = Vdp2Regs->BMPNA & 0x700;
      info.flipfunction = 0;
      info.specialfunction = 0;
   }
   else
   {
      info.mapwh = 2;

      switch((Vdp2Regs->PLSZ & 0xC) >> 2)
      {
         case 0:
            info.planew = info.planeh = 1;
            break;
         case 1:
            info.planew = 2;
            info.planeh = 1;
            break;
         case 3:
            info.planew = info.planeh = 2;
            break;
         default: // Not sure what 0x2 does
            info.planew = info.planeh = 1;
            break;
      }

      info.x = - ((Vdp2Regs->SCXIN1 & 0x7FF) % (512 * info.planew));
      info.y = - ((Vdp2Regs->SCYIN1 & 0x7FF) % (512 * info.planeh));

      if(Vdp2Regs->PNCN1 & 0x8000)
         info.patterndatasize = 1;
      else
         info.patterndatasize = 2;

      if(Vdp2Regs->CHCTLA & 0x100)
         info.patternwh = 2;
      else
         info.patternwh = 1;

      info.pagewh = 64/info.patternwh;
      info.cellw = info.cellh = 8;
      info.supplementdata = Vdp2Regs->PNCN1 & 0x3FF;
      info.auxmode = (Vdp2Regs->PNCN1 & 0x4000) >> 14;
   }

   if (Vdp2Regs->CCCTL & 0x2)
      info.alpha = ((~Vdp2Regs->CCRNA & 0x1F00) >> 5) + 0x7;
   else
      info.alpha = 0xFF;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x70) << 4;

   if (Vdp2Regs->CLOFEN & 0x2)
   {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x2)
      {
         // color offset B
         info.cor = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            info.cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
         info.cor = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            info.cob |= 0xFFFFFF00;
      }

      if (Vdp2Regs->CCCTL & 0x2)
         info.PostPixelFetchCalc = &DoColorCalcWithColorOffset;
      else
         info.PostPixelFetchCalc = &DoColorOffset;
   }
   else // color offset disable
   {
      if (Vdp2Regs->CCCTL & 0x2)
         info.PostPixelFetchCalc = &DoColorCalc;
      else
         info.PostPixelFetchCalc = &DoNothing;
   }

   info.coordincx = (float) 65536 / (Vdp2Regs->ZMXN1.all & 0x7FF00);
   info.coordincy = (float) 65536 / (Vdp2Regs->ZMXN1.all & 0x7FF00);

   info.priority = nbg1priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG1PlaneAddr;

   if (!(info.enable & vdp2disptoggle))
      return;

   if (info.isbitmap)
   {
      Vdp2DrawScrollBitmap(&info);
/*
      // Handle Scroll Wrapping(Let's see if we even need do to it to begin
      // with)
      if (info.x < (vdp2width - info.cellw))
      {
         info.vertices[0] = (info.x+info.cellw) * info.coordincx;
         info.vertices[2] = (info.x + (info.cellw<<1)) * info.coordincx;
         info.vertices[4] = (info.x + (info.cellw<<1)) * info.coordincx;
         info.vertices[6] = (info.x+info.cellw) * info.coordincx;

         YglCachedQuad((YglSprite *)&info, tmp);

         if (info.y < (vdp2height - info.cellh))
         {
            info.vertices[1] = (info.y+info.cellh) * info.coordincy;
            info.vertices[3] = (info.y + (info.cellh<<1)) * info.coordincy;
            info.vertices[5] = (info.y + (info.cellh<<1)) * info.coordincy;
            info.vertices[7] = (info.y+info.cellh) * info.coordincy;

            YglCachedQuad((YglSprite *)&info, tmp);
         }
      }
      else if (info.y < (vdp2height - info.cellh))
      {
         info.vertices[1] = (info.y+info.cellh) * info.coordincy;
         info.vertices[3] = (info.y + (info.cellh<<1)) * info.coordincy;
         info.vertices[5] = (info.y + (info.cellh<<1)) * info.coordincy;
         info.vertices[7] = (info.y+info.cellh) * info.coordincy;

         YglCachedQuad((YglSprite *)&info, tmp);
      }
*/
   }
   else
      Vdp2DrawMap(&info);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2NBG2PlaneAddr(vdp2draw_struct *info, int i)
{
   u32 offset = (Vdp2Regs->MPOFN & 0x700) >> 2;
   u32 tmp=0;
   int deca;
   int multi;

   switch(i)
   {
      case 0:
         tmp = offset | (Vdp2Regs->MPABN2 & 0xFF);
         break;
      case 1:
         tmp = offset | (Vdp2Regs->MPABN2 >> 8);
         break;
      case 2:
         tmp = offset | (Vdp2Regs->MPCDN2 & 0xFF);
         break;
      case 3:
         tmp = offset | (Vdp2Regs->MPCDN2 >> 8);
         break;
   }

   deca = info->planeh + info->planew - 2;
   multi = info->planeh * info->planew;

   //if (Vdp2Regs->VRSIZE & 0x8000)
   //{
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
         else
            info->addr = (tmp >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
      }
   /*}
   else
   {
      if (info->patterndatasize == 1)
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
      }
      else
      {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
      }
   }*/
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG2(void)
{
   vdp2draw_struct info;

   info.enable = Vdp2Regs->BGON & 0x4;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x400);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 4) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLB & 0x2) >> 1;	
   info.mapwh = 2;

   switch((Vdp2Regs->PLSZ & 0x30) >> 4)
   {
      case 0:
         info.planew = info.planeh = 1;
         break;
      case 1:
         info.planew = 2;
         info.planeh = 1;
         break;
      case 3:
         info.planew = info.planeh = 2;
         break;
      default: // Not sure what 0x2 does
         info.planew = info.planeh = 1;
         break;
   }
   info.x = - ((Vdp2Regs->SCXN2 & 0x7FF) % (512 * info.planew));
   info.y = - ((Vdp2Regs->SCYN2 & 0x7FF) % (512 * info.planeh));

   if(Vdp2Regs->PNCN2 & 0x8000)
      info.patterndatasize = 1;
   else
      info.patterndatasize = 2;

   if(Vdp2Regs->CHCTLB & 0x1)
      info.patternwh = 2;
   else
      info.patternwh = 1;

   info.pagewh = 64/info.patternwh;
   info.cellw = info.cellh = 8;
   info.supplementdata = Vdp2Regs->PNCN2 & 0x3FF;
   info.auxmode = (Vdp2Regs->PNCN2 & 0x4000) >> 14;
    
   if (Vdp2Regs->CCCTL & 0x4)
      info.alpha = ((~Vdp2Regs->CCRNB & 0x1F) << 3) + 0x7;
   else
      info.alpha = 0xFF;

   info.coloroffset = Vdp2Regs->CRAOFA & 0x700;

   if (Vdp2Regs->CLOFEN & 0x4)
   {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x4)
      {
         // color offset B
         info.cor = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            info.cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
         info.cor = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            info.cob |= 0xFFFFFF00;
      }

      if (Vdp2Regs->CCCTL & 0x4)
         info.PostPixelFetchCalc = &DoColorCalcWithColorOffset;
      else
         info.PostPixelFetchCalc = &DoColorOffset;
   }
   else // color offset disable
   {
      if (Vdp2Regs->CCCTL & 0x4)
         info.PostPixelFetchCalc = &DoColorCalc;
      else
         info.PostPixelFetchCalc = &DoNothing;
   }

   info.coordincx = info.coordincy = 1;

   info.priority = nbg2priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG2PlaneAddr;

   if (!(info.enable & vdp2disptoggle))
      return;

   Vdp2DrawMap(&info);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL Vdp2NBG3PlaneAddr(vdp2draw_struct *info, int i)
{
   u32 offset = (Vdp2Regs->MPOFN & 0x7000) >> 6;
   u32 tmp=0;
   int deca;
   int multi;

   switch(i)
   {
      case 0:
         tmp = offset | (Vdp2Regs->MPABN3 & 0xFF);
         break;
      case 1:
         tmp = offset | (Vdp2Regs->MPABN3 >> 8);
         break;
      case 2:
         tmp = offset | (Vdp2Regs->MPCDN3 & 0xFF);
         break;
      case 3:
         tmp = offset | (Vdp2Regs->MPCDN3 >> 8);
         break;
   }

   deca = info->planeh + info->planew - 2;
   multi = info->planeh * info->planew;

   //if (Vdp2Regs->VRSIZE & 0x8000) {
      if (info->patterndatasize == 1) {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
         else
            info->addr = (tmp >> deca) * (multi * 0x800);
      }
      else {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
      }
   /*}
   else {
      if (info->patterndatasize == 1) {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
         else
            info->addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
      }
      else {
         if (info->patternwh == 1)
            info->addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
         else
            info->addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
      }
   }*/
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG3(void)
{
   vdp2draw_struct info;

   info.enable = Vdp2Regs->BGON & 0x8;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x800);
   info.specialprimode = (Vdp2Regs->SFPRMD >> 6) & 0x3;

   info.colornumber = (Vdp2Regs->CHCTLB & 0x20) >> 5;
	
   info.mapwh = 2;

   switch((Vdp2Regs->PLSZ & 0xC0) >> 6)
   {
      case 0:
         info.planew = info.planeh = 1;
         break;
      case 1:
         info.planew = 2;
         info.planeh = 1;
         break;
      case 3:
         info.planew = info.planeh = 2;
         break;
      default: // Not sure what 0x2 does
         info.planew = info.planeh = 1;
         break;
   }
   info.x = - ((Vdp2Regs->SCXN3 & 0x7FF) % (512 * info.planew));
   info.y = - ((Vdp2Regs->SCYN3 & 0x7FF) % (512 * info.planeh));

   if(Vdp2Regs->PNCN3 & 0x8000)
      info.patterndatasize = 1;
   else
      info.patterndatasize = 2;

   if(Vdp2Regs->CHCTLB & 0x10)
      info.patternwh = 2;
   else
      info.patternwh = 1;

   info.pagewh = 64/info.patternwh;
   info.cellw = info.cellh = 8;
   info.supplementdata = Vdp2Regs->PNCN3 & 0x3FF;
   info.auxmode = (Vdp2Regs->PNCN3 & 0x4000) >> 14;

   if (Vdp2Regs->CCCTL & 0x8)
      info.alpha = ((~Vdp2Regs->CCRNB & 0x1F00) >> 5) + 0x7;
   else
      info.alpha = 0xFF;

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x7000) >> 4;

   if (Vdp2Regs->CLOFEN & 0x8)
   {
      // color offset enable
      if (Vdp2Regs->CLOFSL & 0x8)
      {
         // color offset B
         info.cor = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            info.cob |= 0xFFFFFF00;
      }
      else
      {
         // color offset A
         info.cor = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            info.cor |= 0xFFFFFF00;

         info.cog = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            info.cog |= 0xFFFFFF00;

         info.cob = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            info.cob |= 0xFFFFFF00;
      }

      if (Vdp2Regs->CCCTL & 0x8)
         info.PostPixelFetchCalc = &DoColorCalcWithColorOffset;
      else
         info.PostPixelFetchCalc = &DoColorOffset;
   }
   else // color offset disable
   {
      if (Vdp2Regs->CCCTL & 0x8)
         info.PostPixelFetchCalc = &DoColorCalc;
      else
         info.PostPixelFetchCalc = &DoNothing;
   }

   info.coordincx = info.coordincy = 1;

   info.priority = nbg3priority;
   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG3PlaneAddr;

   if (!(info.enable & vdp2disptoggle))
      return;

   Vdp2DrawMap(&info);
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftInit(void)
{
   // Initialize output buffer
   if ((dispbuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;

   // Initialize VDP1 framebuffer
   if ((vdp1framebuffer = (u8 *)calloc(sizeof(u8), 0x40000)) == NULL)
      return -1;

   // Initialize VDP2 framebuffer
   if ((vdp2framebuffer = (u32 *)calloc(sizeof(u32), 704 * 512)) == NULL)
      return -1;

   vdp2width = 320;
   vdp2height = 224;

#ifdef USE_OPENGL
   YuiSetVideoAttribute(DOUBLEBUFFER, 1);

   if (!YglScreenInit(8, 8, 8, 24))
   {
      if (!YglScreenInit(4, 4, 4, 24))
      {
         if (!YglScreenInit(5, 6, 5, 16))
         {
	    YuiErrorMsg("Couldn't set GL mode\n");
            return -1;
         }
      }
   }

   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, 320, 224, 0, 1, 0);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glOrtho(-320, 320, -224, 224, 1, 0);
   outputwidth = 320;
   outputheight = 224;
#endif
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftDeInit(void)
{
   if (dispbuffer)
      free(dispbuffer);

   if (vdp1framebuffer)
      free(vdp1framebuffer);

   if (vdp2framebuffer)
      free(vdp2framebuffer);
}

//////////////////////////////////////////////////////////////////////////////

static int IsFullscreen = 0;

void VIDSoftResize(unsigned int w, unsigned int h, int on)
{
#ifdef USE_OPENGL
   IsFullscreen = on;

   if (on)
      YuiSetVideoMode(w, h, 32, 1);
   else
      YuiSetVideoMode(w, h, 32, 0);

   glClear(GL_COLOR_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, w, h, 0, 1, 0);

   glMatrixMode(GL_TEXTURE);
   glLoadIdentity();
   glOrtho(-w, w, -h, h, 1, 0);

   glViewport(0, 0, w, h);
   outputwidth = w;
   outputheight = h;
#endif
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftIsFullscreen(void) {
   return IsFullscreen;
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftVdp1Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1DrawStart(void)
{
   int i,i2;
   int w,h;

   if (Vdp1Regs->TVMR & 0x1)
   {
      if (Vdp1Regs->TVMR & 0x2)
      {
         // Rotation 8-bit
         vdp1width = 512;
         vdp1height = 512;

      }
      else
      {
         // Normal 8-bit
         vdp1width = 1024;
         vdp1width = 256;
      }

      vdp1pixelsize = 1;
   }
   else
   {
      // Rotation/Normal 16-bit
      vdp1width = 512;
      vdp1height = 256;
      vdp1pixelsize = 2;
   }

   h = Vdp1Regs->EWRR & 0x1FF;
   if (h > vdp1height) h = vdp1height;
   w = (Vdp1Regs->EWRR >> 6) & 0x3F8;
   if (w > vdp1width) w = vdp1width;

   // This should be controlled by FBCR
   for (i2 = (Vdp1Regs->EWLR & 0x1FF); i2 < h; i2++)
   {
      for (i = ((Vdp1Regs->EWLR >> 6) & 0x1F8); i < w; i++)
      {
         ((u16 *)vdp1framebuffer)[(i2 * vdp1width) + i] = Vdp1Regs->EWDR;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1DrawEnd(void)
{
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp1DrawHorzLine(int x1, int y, int x2, int type, u16 colorbank, u8 SPD, u32 addr, int xInc)
{
   int i;
   u16 dot;

   switch (type)
   {
      case 0x10:
      {
         // 4 bpp Bank mode -> 16-bit FB Pixel
         int counter=0;

         for(i = x1; i < x2; i+=xInc)
         {
            // This sucks, but for now it will do
            dot = T1ReadByte(Vdp1Ram, addr & 0x7FFFF);

            if ((counter & 0x1) == 0)
            {
               // Pixel 1
               counter++;

               dot >>= 4;
               if (dot == 0 && !SPD)
                  continue;

               ((u16 *)vdp1framebuffer)[(y * vdp1width) + i] = colorbank | dot;
            }
            else
            {
               // Pixel 2
               counter++;
               addr++;
               dot &= 0xF;
               if (dot == 0 && !SPD)
                  continue;

               ((u16 *)vdp1framebuffer)[(y * vdp1width) + i] = colorbank | dot;
            }
         }
         break;
      }
      case 0x11:
      {
         // 4 bpp LUT mode -> 16-bit FB Pixel
         u32 colorlut = (u32)colorbank << 3;
         int counter=0;

         for(i = x1; i < x2; i+=xInc)
         {
            // This sucks, but for now it will do
            dot = T1ReadByte(Vdp1Ram, addr & 0x7FFFF);

            if ((counter & 0x1) == 0)
            {
               // Pixel 1
               counter++;

               dot >>= 4;
               if (dot == 0 && !SPD)
                  continue;

               ((u16 *)vdp1framebuffer)[(y * vdp1width) + i] = T1ReadWord(Vdp1Ram, (dot * 2 + colorlut) & 0x7FFFF);
            }
            else
            {
               // Pixel 2
               counter++;
               addr++;
               dot &= 0xF;
               if (dot == 0 && !SPD)
                  continue;

               ((u16 *)vdp1framebuffer)[(y * vdp1width) + i] = T1ReadWord(Vdp1Ram, (dot * 2 + colorlut) & 0x7FFFF);
            }
         }
         break;
      }
      case 0x12:
      {
         // 8 bpp(64 color) Bank mode -> 16-bit FB Pixel
         for(i = x1; i < x2; i+=xInc)
         {
            dot = T1ReadByte(Vdp1Ram, addr & 0x7FFFF) & 0x3F;
            addr++;

            if ((dot == 0) && !SPD)
               continue;
            ((u16 *)vdp1framebuffer)[(y * vdp1width) + i] = colorbank | dot;
         }
         break;
      }
      case 0x13:
      {
         // 8 bpp(128 color) Bank mode -> 16-bit FB Pixel
         for(i = x1; i < x2; i+=xInc)
         {
            dot = T1ReadByte(Vdp1Ram, addr & 0x7FFFF) & 0x7F;
            addr++;

            if ((dot == 0) && !SPD)
               continue;
            ((u16 *)vdp1framebuffer)[(y * vdp1width) + i] = colorbank | dot;
         }
         break;
      }
      case 0x14:
      {
         // 8 bpp(256 color) Bank mode -> 16-bit FB Pixel
         for(i = x1; i < x2; i+=xInc)
         {
            dot = T1ReadByte(Vdp1Ram, addr & 0x7FFFF);
            addr++;

            if ((dot == 0) && !SPD)
               continue;
            ((u16 *)vdp1framebuffer)[(y * vdp1width) + i] = colorbank | dot;
         }
         break;
      }
      case 0x15:
      {
         // 16 bpp Bank mode -> 16-bit FB Pixel
         for(i = x1; i < x2; i+=xInc)
         {
            dot = T1ReadWord(Vdp1Ram, addr & 0x7FFFF);
            addr+=2;

            //if (!(dot & 0x8000) && (Vdp2Regs->SPCTL & 0x20)) printf("mixed mode\n");
            if ((dot == 0) && !SPD)
               continue;

            ((u16 *)vdp1framebuffer)[(y * vdp1width) + i] = dot;
         }
         break;
      }
      default:
         VDP1LOG("Unimplemented sprite color mode: %X\n", type);
         break;
   }

   return addr;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1ProcessSpritePixel(u16 *pixel, int *shadow, int *priority, int *colorcalc)
{
   switch(vdp1spritetype)
   {
      case 0x0:
      {
         // Type 0(2-bit priority, 3-bit color calculation, 11-bit color data)
         *priority = *pixel >> 14;
         *colorcalc = (*pixel >> 11) & 0x7;
         *pixel = *pixel & 0x7FF;
         break;
      }
      case 0x1:
      {
         // Type 1(3-bit priority, 2-bit color calculation, 11-bit color data)
         *priority = *pixel >> 13;
         *colorcalc = (*pixel >> 11) & 0x3;
         *pixel &= 0x7FF;
         break;
      }
      case 0x2:
      {
         // Type 2(1-bit shadow, 1-bit priority, 3-bit color calculation, 11-bit color data)
         *shadow = *pixel >> 15;
         *priority = (*pixel >> 14) & 0x1;
         *colorcalc = (*pixel >> 11) & 0x7;
         *pixel &= 0x7FF;
         break;
      }
      case 0x3:
      {
         // Type 3(1-bit shadow, 2-bit priority, 2-bit color calculation, 11-bit color data)
         *shadow = *pixel >> 15;
         *priority = (*pixel >> 13) & 0x3;
         *colorcalc = (*pixel >> 11) & 0x3;
         *pixel &= 0x7FF;
         break;
      }
      case 0x4:
      {
         // Type 4(1-bit shadow, 2-bit priority, 3-bit color calculation, 10-bit color data)
         *shadow = *pixel >> 15;
         *priority = (*pixel >> 13) & 0x3;
         *colorcalc = (*pixel >> 10) & 0x7;
         *pixel &= 0x3FF;
         break;
      }
      case 0x5:
      {
         // Type 5(1-bit shadow, 3-bit priority, 1-bit color calculation, 11-bit color data)
         *shadow = *pixel >> 15;
         *priority = (*pixel >> 12) & 0x7;
         *colorcalc = (*pixel >> 11) & 0x1;
         *pixel &= 0x7FF;
         break;
      }
      case 0x6:
      {
         // Type 6(1-bit shadow, 3-bit priority, 2-bit color calculation, 10-bit color data)
         *shadow = *pixel >> 15;
         *priority = (*pixel >> 12) & 0x7;
         *colorcalc = (*pixel >> 10) & 0x3;
         *pixel &= 0x3FF;
         break;
      }
      case 0x7:
      {
         // Type 7(1-bit shadow, 3-bit priority, 3-bit color calculation, 9-bit color data)
         *shadow = *pixel >> 15;
         *priority = (*pixel >> 12) & 0x7;
         *colorcalc = (*pixel >> 9) & 0x7;
         *pixel &= 0x1FF;
         break;
      }
      case 0x8:
      {
         // Type 8(1-bit priority, 7-bit color data)
         *priority = *pixel >> 7;
         *pixel &= 0x7F;
         break;
      }
      case 0x9:
      {
         // Type 9(1-bit priority, 1-bit color calculation, 6-bit color data)
         *priority = *pixel >> 7;
         *colorcalc = (*pixel >> 6) & 0x1;
         *pixel &= 0x3F;
         break;
      }
      case 0xA:
      {
         // Type A(2-bit priority, 6-bit color data)
         *priority = *pixel >> 6;
         *pixel &= 0x3F;
         break;
      }
      case 0xB:
      {
         // Type B(2-bit color calculation, 6-bit color data)
         *colorcalc = *pixel >> 6;
         *pixel &= 0x3F;
         break;
      }
      default: break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DrawRegularSprite(vdp2draw_struct *info)
{
   int i, i2;
   clipping_struct clip;

   // Handle vdp1 clipping
   clip.pixeloffset=0;
   clip.lineincrement=0;

   // Handle clipping(both window and screen clipping)
   if (info->x < 0)
   {
      clip.xstart = 0;
      clip.xend = (info->x+info->cellw);
      clip.pixeloffset = 0 - info->x;
      clip.lineincrement = 0 - info->x;
   }
   else
   {
      clip.xstart = info->x;

      if ((info->x+info->cellw) > vdp1width)
      {
         clip.xend = vdp1width;
         clip.lineincrement = (info->x+info->cellw) - vdp1width;
      }
      else
         clip.xend = (info->x+info->cellw);
   }

   if (info->y < 0)
   {
      clip.ystart = 0;
      clip.yend = (info->y+info->cellh);
      clip.pixeloffset =  (info->cellw * (0 - info->y)) + clip.pixeloffset;
   }
   else
   {
      clip.ystart = info->y;

      if ((info->y+info->cellh) >= vdp1height)
         clip.yend = vdp1height;
      else
         clip.yend = (info->y+info->cellh);
   }

   for (i = clip.ystart; i < clip.yend; i++)
      info->addr = Vdp1DrawHorzLine(clip.xstart, i, clip.xend, info->colornumber, info->supplementdata, info->transparencyenable, info->addr, 1) + clip.lineincrement; // fix me

   
}

//////////////////////////////////////////////////////////////////////////////

void DrawDistortedSprite(vdp2draw_struct *info, s32* vertices) {

  #define max4(a,b,c,d) (a>b)?( (c>d)?( (a>c)?a:c ):( (a>d)?a:d ) ):( (c>d)?( (b>c)?b:c ):( (b>d)?b:d ) )
  #define max2(a,b) (a>b)?a:b

  static int flipVerticesAssign[4][8] = {
    {0,1,2,3,4,5,6,7}, // no flip
    {2,3,0,1,6,7,4,5}, // horz flip
    {6,7,4,5,2,3,0,1}, // vert flip
    {4,5,6,7,0,1,2,3}}; // horz & vert flip

  s32 x1 = vertices[flipVerticesAssign[info->flipfunction][0]];
  s32 y1 = vertices[flipVerticesAssign[info->flipfunction][1]];
  s32 x4 = vertices[flipVerticesAssign[info->flipfunction][2]];
  s32 y4 = vertices[flipVerticesAssign[info->flipfunction][3]];
  s32 x3 = vertices[flipVerticesAssign[info->flipfunction][4]];
  s32 y3 = vertices[flipVerticesAssign[info->flipfunction][5]];
  s32 x2 = vertices[flipVerticesAssign[info->flipfunction][6]];
  s32 y2 = vertices[flipVerticesAssign[info->flipfunction][7]];

  s32 lW = info->cellw;
  s32 lH = info->cellh;

  int type = info->colornumber;
  u16 colorbank = info->supplementdata;
  u32 colorlut = (u32)colorbank << 3;
  u8 SPD = info->transparencyenable;
    
  s32 xLead;
  s32 yLead;

  float xStepC;
  float yStepC;
  float xStepM;
  float yStepM;
  float xStepB;
  float yStepB;
  
  float xN = x1;
  float yN = y1;

  float H = 0;
  int y;

  float xStepStepM;
  float yStepStepM;

  float stepW;
  float stepH;

  {
    s32 xA = x4-x1;
    s32 yA = y4-y1;
    s32 xB = x3-x2;
    s32 yB = y3-y2;
    s32 xC = x2-x1;
    s32 yC = y2-y1;
    s32 xD = x3-x4;
    s32 yD = y3-y4;

    s32 xLeadA = abs(xA)+abs(yA);
    s32 xLeadB = abs(xB)+abs(yB);
    s32 yLeadC = abs(xC)+abs(yC);
    s32 yLeadD = abs(xD)+abs(yD);
    
    xLead = max2(xLeadA, xLeadB)+1;
    yLead = max2(yLeadC, yLeadD)+1;

    xStepC = 1.0*xC / yLead;
    yStepC = 1.0*yC / yLead;
    xStepM = 1.0*xA / xLead;
    yStepM = 1.0*yA / xLead;
    xStepB = 1.0*xB / xLead;
    yStepB = 1.0*yB / xLead;
  }
  
  xStepStepM = (xStepB - xStepM) / yLead;
  yStepStepM = (yStepB - yStepM) / yLead;
  
  stepW = (float)(lW-1) / xLead;
  stepH = (float)(lH-1) / yLead;

#define DRAW_DISTORTED_SPRITE_LOOP_BEGIN       for ( x = xLead ; x ; x-- ) { \
    if (( xM >= vdp1clipxstart )&&( yM >= vdp1clipystart )&&( xM < vdp1clipxend )&&( yM < vdp1clipyend )) {
      
#define DRAW_DISTORTED_SPRITE_LOOP_END    	} \
      W += stepW;  \
      xM += xStepM;\
      yM += yStepM;}
 
  for ( y = yLead ; y ; y-- ) {

    float xM = xN;
    float yM = yN;
    float W = 0;
    int iHaddr;
    int x;

    switch ( type ) {
      
    case 0x10:
      // 4 bpp Bank mode -> 16-bit FB Pixel
      iHaddr = info->addr + (int)H * (lW>>1);
      
      DRAW_DISTORTED_SPRITE_LOOP_BEGIN
	
	int iW = W;
        u16 dot = T1ReadByte(Vdp1Ram, ( iHaddr + (iW>>1)) & 0x7FFFF);
        if ((iW & 0x1) == 0) dot >>= 4; // Even pixel
        else dot &= 0xF; // Odd pixel
	if ( SPD | dot )
	  ((u16 *)vdp1framebuffer)[((int)yM * vdp1width) + (int)xM] = colorbank | dot;
      
      DRAW_DISTORTED_SPRITE_LOOP_END
      break;
    case 0x11:
      // 4 bpp LUT mode -> 16-bit FB Pixel
      iHaddr = info->addr + (int)H * (lW>>1);
      
      DRAW_DISTORTED_SPRITE_LOOP_BEGIN
      
        int iW = W;
        u16 dot = T1ReadByte(Vdp1Ram, ( iHaddr + (iW>>1)) & 0x7FFFF);
        if ((iW & 0x1) == 0)  dot >>= 4; // Even pixel
        else dot &= 0xF; // Odd pixel
	if ( SPD | dot )
	  ((u16 *)vdp1framebuffer)[((int)yM * vdp1width) + (int)xM] = T1ReadWord(Vdp1Ram, (dot * 2 + colorlut) & 0x7FFFF);
    
      DRAW_DISTORTED_SPRITE_LOOP_END
      break;
    case 0x12:
      // 8 bpp(64 color) Bank mode -> 16-bit FB Pixel
      iHaddr = info->addr + (int)H * lW;
      
      DRAW_DISTORTED_SPRITE_LOOP_BEGIN
      
      u16 dot = T1ReadByte(Vdp1Ram, ( iHaddr + (int)W) & 0x7FFFF) & 0x3F;
      if ( SPD | dot )
	((u16 *)vdp1framebuffer)[((int)yM * vdp1width) + (int)xM] = colorbank | dot;
      
      DRAW_DISTORTED_SPRITE_LOOP_END
      break;
    case 0x13:
      // 8 bpp(128 color) Bank mode -> 16-bit FB Pixel
      iHaddr = info->addr + (int)H * lW;
      
      DRAW_DISTORTED_SPRITE_LOOP_BEGIN
      
      u16 dot = T1ReadByte(Vdp1Ram, ( iHaddr + (int)W) & 0x7FFFF) & 0x7F;
      if ( SPD | dot )
	((u16 *)vdp1framebuffer)[((int)yM * vdp1width) + (int)xM] = colorbank | dot;
      
      DRAW_DISTORTED_SPRITE_LOOP_END
      break;
    case 0x14:
      // 8 bpp(256 color) Bank mode -> 16-bit FB Pixel
      iHaddr = info->addr + (int)H * lW;
      
      DRAW_DISTORTED_SPRITE_LOOP_BEGIN

      u16 dot = T1ReadByte(Vdp1Ram, ( iHaddr + (int)W) & 0x7FFFF);
      if ( SPD | dot )	  
	((u16 *)vdp1framebuffer)[((int)yM * vdp1width) + (int)xM] = colorbank | dot;

      DRAW_DISTORTED_SPRITE_LOOP_END
      break;
    case 0x15:
      // 16 bpp Bank mode -> 16-bit FB Pixel
      iHaddr = info->addr + 2*((int)H * lW);
      
      DRAW_DISTORTED_SPRITE_LOOP_BEGIN

      u16 dot = T1ReadWord(Vdp1Ram, ( iHaddr + 2*(int)W) & 0x7FFFF);
      if ( SPD | dot )	  
	((u16 *)vdp1framebuffer)[((int)yM * vdp1width) + (int)xM] = dot;

      DRAW_DISTORTED_SPRITE_LOOP_END
      break;      
    }

    xStepM += xStepStepM;
    yStepM += yStepStepM;
    xN += xStepC;
    yN += yStepC;
    H += stepH;
  }
}

/////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1NormalSpriteDraw(void)
{
   vdp1cmd_struct cmd;
   // vdp2draw_struct info;
   s32 x0, y0, x1, y1, W, H;
   u8 flip, SPD;
   u16 colorbank;
   int type;
   u32 addr;
   int h0, w0, stepH, stepW;
   s32   clipx1, clipx2, clipy1, clipy2;
   u16* iPix;
   int stepPix;
   u32 colorlut;
   
   Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

   /*   info.x = cmd.CMDXA + Vdp1Regs->localX;
   info.y = cmd.CMDYA + Vdp1Regs->localY;
   info.cellw = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
   info.cellh = cmd.CMDSIZE & 0xFF;
   info.transparencyenable = ((cmd.CMDPMOD & 0x40) != 0);
   info.colornumber = (vdp1pixelsize << 3) | ((cmd.CMDPMOD >> 3) & 0x7);
   info.flipfunction = (cmd.CMDCTRL & 0x30) >> 4;
   info.addr = cmd.CMDSRCA << 3;
   info.supplementdata = cmd.CMDCOLR;

   DrawRegularSprite(&info); */

   x0 = cmd.CMDXA + Vdp1Regs->localX;
   y0 = cmd.CMDYA + Vdp1Regs->localY;
   W = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
   H = cmd.CMDSIZE & 0xFF;
   type = (vdp1pixelsize << 3) | ((cmd.CMDPMOD >> 3) & 0x7);
   flip = (cmd.CMDCTRL & 0x30) >> 4;
   colorbank = cmd.CMDCOLR;
   colorlut = (u32)colorbank << 3;
   SPD = ((cmd.CMDPMOD & 0x40) != 0);
   addr = cmd.CMDSRCA << 3;

   if ( x0 < vdp1clipxstart ) {
     
     if ( x0+W < vdp1clipxstart ) return;
     clipx1 = vdp1clipxstart-x0;
   } else clipx1 = 0;
   
   if ( x0+W > vdp1clipxend ) {
     
     if ( x0 > vdp1clipxend ) return;
     clipx2 = x0+W-vdp1clipxend;
   } else clipx2 = 0;
   
   if ( y0 < vdp1clipystart ) {
     
     if ( y0+H < vdp1clipystart ) return;
     clipy1 = vdp1clipystart-y0;
   } else clipy1 = 0;
   
   if ( y0+H > vdp1clipyend ) {
     
     if ( y0 > vdp1clipyend ) return;
     clipy2 = y0+H-vdp1clipyend;
   } else clipy2 = 0;

   switch( flip ) {
     
   case 0: 
     stepH = 1;
     stepW = 1;
     h0 = clipy1;
     w0 = clipx1;
     break;
   case 1:
     stepH = 1;
     stepW = -1;
     h0 = clipy1;
     w0 = W-1-clipx2;
     break;
   case 2:
     stepH = -1;
     stepW = 1;
     h0 = H-1-clipy2;
     w0 = clipx1;
     break;
   case 3:
     stepH = -1;
     stepW = -1;
     h0 = H-1-clipy2;
     w0 = W-1-clipx2;
     break;
   }
   
   y1 = H - clipy1 + clipy2;
   x1 = W - clipx1 + clipx2;
   
   iPix = ((u16*)vdp1framebuffer) + (y0+clipy1) * vdp1width + x0+clipx1;
   stepPix = vdp1width - x1;
   
   for ( ; y1 ; y1-- ) {
     
     int w = w0;
     int iAddr;
     int x = x1;
     
     switch ( type ) {
       
     case 0x10:
       // 4 bpp Bank mode -> 16-bit FB Pixel
      
       iAddr = addr + h0*(W>>1);
      
       for ( ; x ; x-- ) {
	
	 u16 dot = T1ReadByte(Vdp1Ram, ( iAddr + (w>>1)) & 0x7FFFF);
	 if ((w & 0x1) == 0) dot >>= 4; // Even pixel
	 else dot &= 0xF; // Odd pixel
	 if (!(dot == 0 && !SPD)) *(iPix) = colorbank | dot;

	 iPix++;      
	 w += stepW;
       }
       break;
     case 0x11:
       // 4 bpp LUT mode -> 16-bit FB Pixel
       iAddr = addr + h0*(W>>1);
      
       for ( ; x ; x-- ) {

	 u16 dot = T1ReadByte(Vdp1Ram, ( iAddr + (w>>1)) & 0x7FFFF);
	 if ((w & 0x1) == 0) dot >>= 4; // Even pixel
	 else dot &= 0xF; // Odd pixel
	 if (!(dot == 0 && !SPD)) *(iPix) = T1ReadWord(Vdp1Ram, (dot * 2 + colorlut) & 0x7FFFF);

	 iPix++;      
	 w += stepW;
       }
       break;
     case 0x12:
       // 8 bpp(64 color) Bank mode -> 16-bit FB Pixel

       iAddr = addr + h0*W;
       for ( ; x ; x-- ) {
	
	 u16 dot = T1ReadByte(Vdp1Ram, ( iAddr + w) & 0x7FFFF) & 0x3F;
	 if (!((dot == 0) && !SPD)) *(iPix) = colorbank | dot;
	
	 iPix++;      
	 w += stepW;
       }
       break;
     case 0x13:
       // 8 bpp(128 color) Bank mode -> 16-bit FB Pixel

       iAddr = addr + h0*W;
       for ( ; x ; x-- ) {
	
	 u16 dot = T1ReadByte(Vdp1Ram, ( iAddr + w) & 0x7FFFF) & 0x7F;
	 if (!((dot == 0) && !SPD)) *(iPix) = colorbank | dot;
	
	 iPix++;      
	 w += stepW;
       }
       break;
     case 0x14:
       // 8 bpp(256 color) Bank mode -> 16-bit FB Pixel

       iAddr = addr + h0*W;
       for ( ; x ; x-- ) {
	
	 u16 dot = T1ReadByte(Vdp1Ram, ( iAddr + w) & 0x7FFFF);
	 if (!((dot == 0) && !SPD)) *(iPix) = colorbank | dot;
	
	 iPix++;      
	 w += stepW;
       }
       break;
     case 0x15:
       // 16 bpp Bank mode -> 16-bit FB Pixel
      
       iAddr = addr + 2*h0*W;
       for ( ; x ; x-- ) {
	
	 u16 dot = T1ReadWord(Vdp1Ram, ( iAddr + 2*w) & 0x7FFFF);
	 if (!((dot == 0) && !SPD)) *(iPix) = dot;
	
	 iPix++;      
	 w += stepW;
       }
       break;
     }
    
     iPix += stepPix;
     h0 += stepH;
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1ScaledSpriteDraw(void)
{
   vdp1cmd_struct cmd;

   s32 x0, y0, x1, y1, W, H;
   u8 flip, SPD;
   u16 colorbank;
   int type;
   u32 addr;
   float h0, w0, stepH, stepW;
   s32   clipx1, clipx2, clipy1, clipy2;
   u16* iPix;
   int stepPix;
   u32 colorlut;

   Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

   flip = (cmd.CMDCTRL & 0x30) >> 4;

   x0 = cmd.CMDXA + Vdp1Regs->localX;
   y0 = cmd.CMDYA + Vdp1Regs->localY;

   switch ((cmd.CMDCTRL >> 8) & 0xF)
   {
      case 0x0: // Only two coordinates
         x1 = ((int)cmd.CMDXC) - x0 + Vdp1Regs->localX + 1;
         y1 = ((int)cmd.CMDYC) - y0 + Vdp1Regs->localY + 1;
	 if (x1<0) { x1 = -x1; x0 -= x1; flip ^= 1; }
	 if (y1<0) { y1 = -y1; y0 -= y1; flip ^= 2; }
         break;
      case 0x5: // Upper-left
         x1 = ((int)cmd.CMDXB) + 1;
         y1 = ((int)cmd.CMDYB) + 1;
         break;
      case 0x6: // Upper-Center
         x1 = ((int)cmd.CMDXB);
         y1 = ((int)cmd.CMDYB);
         x0 = x0 - x1/2;
         x1++;
         y1++;
         break;
      case 0x7: // Upper-Right
         x1 = ((int)cmd.CMDXB);
         y1 = ((int)cmd.CMDYB);
         x0 = x0 - x1;
         x1++;
         y1++;
         break;
      case 0x9: // Center-left
         x1 = ((int)cmd.CMDXB);
         y1 = ((int)cmd.CMDYB);
         y0 = y0 - y1/2;
         x1++;
         y1++;
         break;
      case 0xA: // Center-center
         x1 = ((int)cmd.CMDXB);
         y1 = ((int)cmd.CMDYB);
         x0 = x0 - x1/2;
         y0 = y0 - y1/2;
         x1++;
         y1++;
         break;
      case 0xB: // Center-right
         x1 = ((int)cmd.CMDXB);
         y1 = ((int)cmd.CMDYB);
         x0 = x0 - x1;
         y0 = y0 - y1/2;
         x1++;
         y1++;
         break;
      case 0xD: // Lower-left
         x1 = ((int)cmd.CMDXB);
         y1 = ((int)cmd.CMDYB);
         y0 = y0 - y1;
         x1++;
         y1++;
         break;
      case 0xE: // Lower-center
         x1 = ((int)cmd.CMDXB);
         y1 = ((int)cmd.CMDYB);
         x0 = x0 - x1/2;
         y0 = y0 - y1;
         x1++;
         y1++;
         break;
      case 0xF: // Lower-right
         x1 = ((int)cmd.CMDXB);
         y1 = ((int)cmd.CMDYB);
         x0 = x0 - x1;
         y0 = y0 - y1;
         x1++;
         y1++;
         break;
      default: break;
   }

   W = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
   H = cmd.CMDSIZE & 0xFF;
   type = (vdp1pixelsize << 3) | ((cmd.CMDPMOD >> 3) & 0x7);
   colorbank = cmd.CMDCOLR;
   colorlut = (u32)colorbank << 3;
   SPD = ((cmd.CMDPMOD & 0x40) != 0);
   addr = cmd.CMDSRCA << 3;

   if ( x0 < vdp1clipxstart ) {
     
     if ( x0+x1 < vdp1clipxstart ) return;
     clipx1 = vdp1clipxstart-x0;
   } else clipx1 = 0;
   
   if ( x0+x1 > vdp1clipxend ) {
     
     if ( x0 > vdp1clipxend ) return;
     clipx2 = x0+x1-vdp1clipxend;
   } else clipx2 = 0;
   
   if ( y0 < vdp1clipystart ) {
     
     if ( y0+y1 < vdp1clipystart ) return;
     clipy1 = vdp1clipystart-y0;
   } else clipy1 = 0;
   
   if ( y0+y1 > vdp1clipyend ) {
     
     if ( y0 > vdp1clipyend ) return;
     clipy2 = y0+y1-vdp1clipyend;
   } else clipy2 = 0;
   
   switch( flip ) {
     
   case 0: 
     stepH = (float)H/y1;
     stepW = (float)W/x1;
     h0 = clipy1*stepH;
     w0 = clipx1*stepW;
     break;
   case 1:
     stepH = (float)H/y1;
     stepW = -(float)W/x1;
     h0 = clipy1*stepH;
     w0 = W+clipx2*stepW;
     break;
   case 2:
     stepH = -(float)H/y1;
     stepW = (float)W/x1;
     h0 = H+clipy2*stepH;
     w0 = clipx1*stepW;
     break;
   case 3:
     stepH = -(float)H/y1;
     stepW = -(float)W/x1;
     h0 = H+clipy2*stepH;
     w0 = W+clipx2*stepW;
     break;
   }
   
   y1 -= clipy1 + clipy2;
   x1 -= clipx1 + clipx2;
   
   iPix = ((u16*)vdp1framebuffer) + (y0+clipy1) * vdp1width + x0+clipx1;
   stepPix = vdp1width - x1;
   
   for ( ; y1 ; y1-- ) {
     
     float w = w0;
     int iAddr;
     int x = x1;
     
     switch ( type ) {
       
     case 0x10:
       // 4 bpp Bank mode -> 16-bit FB Pixel
      
       iAddr = addr + (int)h0*(W>>1);
      
       for ( ; x ; x-- ) {
	
	 int iw = w;
	 u16 dot = T1ReadByte(Vdp1Ram, ( iAddr + (iw>>1)) & 0x7FFFF);
	 if ((iw & 0x1) == 0) dot >>= 4; // Even pixel
	 else dot &= 0xF; // Odd pixel
	 if (!(dot == 0 && !SPD)) *(iPix) = colorbank | dot;

	 iPix++;      
	 w += stepW;
       }
       break;
     case 0x11:
       // 4 bpp LUT mode -> 16-bit FB Pixel
       iAddr = addr + (int)h0*(W>>1);
      
       for ( ; x ; x-- ) {

	 int iw = w;
	 u16 dot = T1ReadByte(Vdp1Ram, ( iAddr + (iw>>1)) & 0x7FFFF);
	 if ((iw & 0x1) == 0) dot >>= 4; // Even pixel
	 else dot &= 0xF; // Odd pixel
	 if (!(dot == 0 && !SPD)) *(iPix) = T1ReadWord(Vdp1Ram, (dot * 2 + colorlut) & 0x7FFFF);

	 iPix++;      
	 w += stepW;
       }
       break;
     case 0x12:
       // 8 bpp(64 color) Bank mode -> 16-bit FB Pixel

       iAddr = addr + (int)h0*W;
       for ( ; x ; x-- ) {
	
	 u16 dot = T1ReadByte(Vdp1Ram, ( iAddr + (int)w) & 0x7FFFF) & 0x3F;
	 if (!((dot == 0) && !SPD)) *(iPix) = colorbank | dot;
	
	 iPix++;      
	 w += stepW;
       }
       break;
     case 0x13:
       // 8 bpp(128 color) Bank mode -> 16-bit FB Pixel

       iAddr = addr + (int)h0*W;
       for ( ; x ; x-- ) {
	
	 u16 dot = T1ReadByte(Vdp1Ram, ( iAddr + (int)w) & 0x7FFFF) & 0x7F;
	 if (!((dot == 0) && !SPD)) *(iPix) = colorbank | dot;
	
	 iPix++;      
	 w += stepW;
       }
       break;
     case 0x14:
       // 8 bpp(256 color) Bank mode -> 16-bit FB Pixel

       iAddr = addr + (int)h0*W;
       for ( ; x ; x-- ) {
	
	 u16 dot = T1ReadByte(Vdp1Ram, ( iAddr + (int)w) & 0x7FFFF);
	 if (!((dot == 0) && !SPD)) *(iPix) = colorbank | dot;
	
	 iPix++;      
	 w += stepW;
       }
       break;
     case 0x15:
       // 16 bpp Bank mode -> 16-bit FB Pixel
      
       iAddr = addr + 2*(int)h0*W;
       for ( ; x ; x-- ) {
	
	 u16 dot = T1ReadWord(Vdp1Ram, ( iAddr + 2*(int)w) & 0x7FFFF);
	 if (!((dot == 0) && !SPD)) *(iPix) = dot;
	
	 iPix++;      
	 w += stepW;
       }
       break;
     }
    
     iPix += stepPix;
     h0 += stepH;
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1DistortedSpriteDraw(void)
{
   vdp1cmd_struct cmd;
   vdp2draw_struct info;
   int w, h;
   s32 vertices[8];

   Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

   w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
   h = cmd.CMDSIZE & 0xFF;
	
   info.flipfunction = (cmd.CMDCTRL & 0x30) >> 4;

   vertices[0] = (s32)(cmd.CMDXA + Vdp1Regs->localX);
   vertices[1] = (s32)(cmd.CMDYA + Vdp1Regs->localY);
   vertices[2] = (s32)((cmd.CMDXB + 1) + Vdp1Regs->localX);
   vertices[3] = (s32)(cmd.CMDYB + Vdp1Regs->localY);
   vertices[4] = (s32)((cmd.CMDXC + 1) + Vdp1Regs->localX);
   vertices[5] = (s32)((cmd.CMDYC + 1) + Vdp1Regs->localY);
   vertices[6] = (s32)(cmd.CMDXD + Vdp1Regs->localX);
   vertices[7] = (s32)((cmd.CMDYD + 1) + Vdp1Regs->localY);

   info.x = vertices[0];
   info.y = vertices[1];
   info.cellw = w;
   info.cellh = h;
   info.transparencyenable = ((cmd.CMDPMOD & 0x40) != 0);
   info.colornumber = (vdp1pixelsize << 3) | ((cmd.CMDPMOD >> 3) & 0x7);
   info.flipfunction = (cmd.CMDCTRL & 0x30) >> 4;
   info.addr = cmd.CMDSRCA << 3;
   info.supplementdata = cmd.CMDCOLR;
   DrawDistortedSprite(&info, vertices);
}

//////////////////////////////////////////////////////////////////////////////

int fcmpy_vdp1vertex( const void* v1, const void* v2 ) {

  return ( ((vdp1vertex*)v1)->y <= ((vdp1vertex*)v2)->y )? -1 : 1;
}

void VIDSoftVdp1PolygonDraw(void) {

  vdp1vertex v[4];
  float xleft, xwidth;
  int y, x;
  int zA, zB, zC, zD, zE, zF;
  int y01, y02, y03, y12, y13, y23;
  float stepA, stepB, stepC, stepD, stepE, stepF;
  int bInverted;
  u16 *fb;
  u16 color = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x6);

  v[0].x = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C));
  v[1].x = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10));
  v[2].x = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14));
  v[3].x = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x18));
  v[0].y = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E));
  v[1].y = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12));
  v[2].y = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16));
  v[3].y = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1A));

  /* points 0,1,2,3 are sorted by increasing y */
  qsort( v, 4, sizeof( vdp1vertex ), fcmpy_vdp1vertex );

  y01 = v[1].y - v[0].y;
  y02 = v[2].y - v[0].y;
  y03 = v[3].y - v[0].y;
  y12 = v[2].y - v[1].y;
  y13 = v[3].y - v[1].y;
  y23 = v[3].y - v[2].y;
  stepA = y01 ? (zA = 0, (float)( v[1].x - v[0].x ) / (y01)) : (zA = 1, v[1].x - v[0].x);  /* edge A = 01 */
  stepB = y02 ? (zB = 0, (float)( v[2].x - v[0].x ) / (y02)) : (zB = 1, v[2].x - v[0].x); /* edge B = 02 */
  stepC = y13 ? (zC = 0, (float)( v[3].x - v[1].x ) / (y13)) : (zC = 1, v[3].x - v[1].x); /* edge C = 13 */
  stepD = y23 ? (zD = 0, (float)( v[3].x - v[2].x ) / (y23)) : (zD = 1, v[3].x - v[2].x); /* edge D = 23 */
  stepE = y03 ? (zE = 0, (float)( v[3].x - v[0].x ) / (y03)) : (zE = 1, v[3].x - v[0].x); /* edge E = 03 */
  stepF = y12 ? (zF = 0, (float)( v[2].x - v[1].x ) / (y12)) : (zF = 1, v[2].x - v[1].x); /* edge F = 12 */

  xleft = v[0].x;
  xwidth = 1;
  y = v[0].y;

#define POLYGON_PAINT_PART( pBegin, pEnd, stepLeft, stepRight, zLeft, zRight ) \
    if ( v[pBegin].y <= vdp1clipystart ) { \
      if ( v[pEnd].y <= vdp1clipystart ) y = v[pEnd].y; else y = vdp1clipystart;\
      xleft += stepLeft * (y-v[pBegin].y);\
      xwidth += (stepRight - stepLeft) * (y-v[pBegin].y);\
      }\
    if ( y == v[pEnd].y ) { \
      if ( zLeft ) { xleft += stepLeft; xwidth -= stepLeft; zLeft = 0; } \
      if ( zRight ) { xwidth += stepRight; zRight = 0; }\
    if ( xleft < vdp1clipxstart ) { \
          xwidth -= (vdp1clipxstart - xleft); \
          xleft = vdp1clipxstart;} \
    if ( xleft + xwidth > vdp1clipxend ) xwidth = vdp1clipxend-xleft;\
    fb = (u16 *)vdp1framebuffer + y*vdp1width + (int)xleft;\
    for ( x = (int)xwidth ; x>=0 ; x-- ) *(fb++) = color; \
    }  \
    while (( y < v[pEnd].y ) && (y <= vdp1clipyend)) { \
      \
      xwidth += stepRight - stepLeft;\
      xleft += stepLeft;\
      \
      if ( xleft < vdp1clipxstart ) { \
            xwidth -= (vdp1clipxstart - xleft); \
            xleft = vdp1clipxstart;} \
      if ( xleft + xwidth > vdp1clipxend ) xwidth = vdp1clipxend-xleft;\
      fb = (u16 *)vdp1framebuffer + y*vdp1width + (int)xleft;\
      for ( x = (int)xwidth ; x>=0 ; x-- ) *(fb++) = color; \
      y++; \
    }

#define POLYGON_PAINT_PART_NOCLIP( pBegin, pEnd, stepLeft, stepRight, zLeft, zRight ) \
    if ( y == v[pEnd].y ) { \
      if ( zLeft ) { xleft += stepLeft; xwidth -= stepLeft; zLeft = 0; } \
      if ( zRight ) { xwidth += stepRight; zRight = 0; }\
    fb = (u16 *)vdp1framebuffer + y*vdp1width + (int)xleft;\
    for ( x = (int)xwidth ; x>=0 ; x-- ) *(fb++) = color; \
    }  \
    while ( y < v[pEnd].y ) { \
      \
      xwidth += stepRight - stepLeft;\
      xleft += stepLeft;\
      \
      fb = (u16 *)vdp1framebuffer + y*vdp1width + (int)xleft;\
      for ( x = (int)xwidth ; x>=0 ; x-- ) *(fb++) = color; \
      y++; \
    }

  if (( v[0].y < vdp1clipystart )||( v[3].y > vdp1clipyend )
      ||( v[0].x < vdp1clipxstart )||( v[0].x > vdp1clipxend )
      ||( v[1].x < vdp1clipxstart )||( v[1].x > vdp1clipxend )
      ||( v[2].x < vdp1clipxstart )||( v[2].x > vdp1clipxend )
      ||( v[3].x < vdp1clipxstart )||( v[3].x > vdp1clipxend )) {
    
    if ( stepE < stepB ) {
      if ( stepE < stepA ) {
	
	POLYGON_PAINT_PART( 0, 1, stepE, stepA, zE, zA );
	POLYGON_PAINT_PART( 1, 2, stepE, stepF, zE, zF );
	POLYGON_PAINT_PART( 2, 3, stepE, stepD, zE, zD );
      } else {
	
	POLYGON_PAINT_PART( 0, 1, stepA, stepB, zA, zB );
	POLYGON_PAINT_PART( 1, 2, stepC, stepB, zC, zB );
	POLYGON_PAINT_PART( 2, 3, stepC, stepD, zC, zD );
      }
    } else {
      
      if ( stepE < stepA ) {
	
	POLYGON_PAINT_PART( 0, 1, stepB, stepA, zB, zA );
	POLYGON_PAINT_PART( 1, 2, stepB, stepC, zB, zC );
	POLYGON_PAINT_PART( 2, 3, stepD, stepC, zD, zC );
      } else {
	
	POLYGON_PAINT_PART( 0, 1, stepA, stepE, zA, zE );
	POLYGON_PAINT_PART( 1, 2, stepF, stepE, zF, zE );
	POLYGON_PAINT_PART( 2, 3, stepD, stepE, zD, zE );
      }
    }
  } else {
    if ( stepE < stepB ) {
      if ( stepE < stepA ) {
	
	POLYGON_PAINT_PART_NOCLIP( 0, 1, stepE, stepA, zE, zA );
	POLYGON_PAINT_PART_NOCLIP( 1, 2, stepE, stepF, zE, zF );
	POLYGON_PAINT_PART_NOCLIP( 2, 3, stepE, stepD, zE, zD );
      } else {
	
	POLYGON_PAINT_PART_NOCLIP( 0, 1, stepA, stepB, zA, zB );
	POLYGON_PAINT_PART_NOCLIP( 1, 2, stepC, stepB, zC, zB );
	POLYGON_PAINT_PART_NOCLIP( 2, 3, stepC, stepD, zC, zD );
      }
    } else {
      
      if ( stepE < stepA ) {
	
	POLYGON_PAINT_PART_NOCLIP( 0, 1, stepB, stepA, zB, zA );
	POLYGON_PAINT_PART_NOCLIP( 1, 2, stepB, stepC, zB, zC );
	POLYGON_PAINT_PART_NOCLIP( 2, 3, stepD, stepC, zD, zC );
      } else {
	
	POLYGON_PAINT_PART_NOCLIP( 0, 1, stepA, stepE, zA, zE );
	POLYGON_PAINT_PART_NOCLIP( 1, 2, stepF, stepE, zF, zE );
	POLYGON_PAINT_PART_NOCLIP( 2, 3, stepD, stepE, zD, zE );
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////////

int INLINE ClipLine(int *x1, int *y1, int *x2, int *y2)
{
   // rewrite me
   int point1vis=0;
   int point2vis=0;

   // Let's see if point 1 is clipped
   if (*x1 >= vdp1clipxstart &&
       *x1 < vdp1clipxend &&
       *y1 >= vdp1clipystart &&
       *y1 < vdp1clipyend)
      point1vis = 1;

   // Let's see if point 1 is clipped
   if (*x2 >= vdp1clipxstart &&
       *x2 < vdp1clipxend &&
       *y2 >= vdp1clipystart &&
       *y2 < vdp1clipyend)
      point2vis = 1;

   // Both points are visible, so don't do any clipping
   if (point1vis && point2vis)
      return 1;

   // Both points are invisible, so don't draw
   if (point1vis == 0 && point2vis == 0)
      return 0;

   return 1;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DrawLine(int x1, int y1, int x2, int y2, u16 color)
{
   // Uses Bresenham's line algorithm(eventually this should be changed over
   // to Wu's symmetric double step
   u16 *fbstart = &((u16 *)vdp1framebuffer)[(y1 * vdp1width) + x1];
   int deltax, deltay, xinc, yinc, i;
   int error=0;

   deltax = x2-x1;
   deltay = y2-y1;

   if (deltax >= 0)
     // Line is going right
     xinc = 1;
   else
   {
      // Line is going left
      xinc = -1;
      deltax = 0 - deltax;
   }

   if (deltay >= 0)
      // Line is going down
      yinc = vdp1width;
   else
   {
      // Line is going up
      yinc = 0 - vdp1width;
      deltay = 0 - deltay;
   }

   // Draw the line
   if (deltax > deltay)
   {
      for (i = 0; i <= deltax; i++)
      {
         fbstart[0] = color;

         error += deltay;

         if (error > deltax)
         {
            // Handle error
            error -= deltax;
            fbstart += yinc;
         }

         fbstart += xinc;
      }
   }
   else
   {
      for (i = 0; i <= deltay; i++)
      {
         *fbstart = color;
         error += deltax;

         if (error > 0)
         {
            error -= deltay;
            fbstart += xinc;
         }

         fbstart += yinc;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1PolylineDraw(void)
{
   int X[4];
   int Y[4];
   u16 color;

   X[0] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C));
   Y[0] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E));
   X[1] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10));
   Y[1] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12));
   X[2] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14));
   Y[2] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16));
   X[3] = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x18));
   Y[3] = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1A));

   color = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x6);
//   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);

   if (ClipLine(&X[0], &Y[0], &X[1], &Y[1]))
      DrawLine(X[0], Y[0], X[1], Y[1], color);

   if (ClipLine(&X[1], &Y[1], &X[2], &Y[2]))
      DrawLine(X[1], Y[1], X[2], Y[2], color);

   if (ClipLine(&X[2], &Y[2], &X[3], &Y[3]))
      DrawLine(X[2], Y[2], X[3], Y[3], color);

   if (ClipLine(&X[3], &Y[3], &X[0], &Y[0]))
      DrawLine(X[3], Y[3], X[0], Y[0], color);
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1LineDraw(void)
{
   int x1, y1, x2, y2;

   x1 = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C));
   y1 = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E));
   x2 = (int)Vdp1Regs->localX + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10));
   y2 = (int)Vdp1Regs->localY + (int)((s16)T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12));

//   CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x4);

   if (ClipLine(&x1, &y1, &x2, &y2))
      DrawLine(x1, y1, x2, y2, T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x6));
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1UserClipping(void)
{
   Vdp1Regs->userclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->userclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
   Vdp1Regs->userclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->userclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);

   if (vdp1clipxstart > Vdp1Regs->systemclipX1)
      vdp1clipxstart = Vdp1Regs->userclipX1;
   else
      vdp1clipxstart = Vdp1Regs->systemclipX1;

   if (vdp1clipxend < Vdp1Regs->systemclipX2)
      vdp1clipxend = Vdp1Regs->userclipX2;
   else
      vdp1clipxend = Vdp1Regs->systemclipX2;

   if (vdp1clipystart > Vdp1Regs->systemclipY1)
      vdp1clipystart = Vdp1Regs->userclipY1;
   else
      vdp1clipystart = Vdp1Regs->systemclipY1;

   if (vdp1clipyend < Vdp1Regs->systemclipY2)
      vdp1clipyend = Vdp1Regs->userclipY2;
   else
      vdp1clipyend = Vdp1Regs->systemclipY2;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1SystemClipping(void)
{
   Vdp1Regs->systemclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->systemclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
   Vdp1Regs->systemclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
   Vdp1Regs->systemclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);

   vdp1clipxstart = Vdp1Regs->systemclipX1;
   vdp1clipxend = Vdp1Regs->systemclipX2;
   vdp1clipystart = Vdp1Regs->systemclipY1;
   vdp1clipyend = Vdp1Regs->systemclipY2;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp1LocalCoordinate(void)
{
   Vdp1Regs->localX = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
   Vdp1Regs->localY = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
}

//////////////////////////////////////////////////////////////////////////////

int VIDSoftVdp2Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2DrawStart(void)
{
   Vdp2DrawBackScreen();
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2DrawEnd(void)
{
   int i, i2;
   u16 pixel;
   u8 prioritytable[8];
   int priority;
   int shadow;
   int colorcalc;
   u32 vdp1coloroffset;
   vdp2draw_struct info;

   // Figure out whether to draw vdp1 framebuffer or vdp2 framebuffer pixels
   // based on priority
   if (Vdp1Regs->disptoggle)
   {
      prioritytable[0] = Vdp2Regs->PRISA & 0x7;
      prioritytable[1] = (Vdp2Regs->PRISA >> 8) & 0x7;
      prioritytable[2] = Vdp2Regs->PRISB & 0x7;
      prioritytable[3] = (Vdp2Regs->PRISB >> 8) & 0x7;
      prioritytable[4] = Vdp2Regs->PRISC & 0x7;
      prioritytable[5] = (Vdp2Regs->PRISC >> 8) & 0x7;
      prioritytable[6] = Vdp2Regs->PRISD & 0x7;
      prioritytable[7] = (Vdp2Regs->PRISD >> 8) & 0x7;

      vdp1coloroffset = (Vdp2Regs->CRAOFB & 0x70) << 4;
      vdp1spritetype = Vdp2Regs->SPCTL & 0xF;

      if (Vdp2Regs->CLOFEN & 0x40)
      {
         // color offset enable
         if (Vdp2Regs->CLOFSL & 0x40)
         {
            // color offset B
            info.cor = Vdp2Regs->COBR & 0xFF;
            if (Vdp2Regs->COBR & 0x100)
               info.cor |= 0xFFFFFF00;

            info.cog = Vdp2Regs->COBG & 0xFF;
            if (Vdp2Regs->COBG & 0x100)
               info.cog |= 0xFFFFFF00;

            info.cob = Vdp2Regs->COBB & 0xFF;
            if (Vdp2Regs->COBB & 0x100)
               info.cob |= 0xFFFFFF00;
         }
         else
         {
            // color offset A
            info.cor = Vdp2Regs->COAR & 0xFF;
            if (Vdp2Regs->COAR & 0x100)
               info.cor |= 0xFFFFFF00;

            info.cog = Vdp2Regs->COAG & 0xFF;
            if (Vdp2Regs->COAG & 0x100)
               info.cog |= 0xFFFFFF00;

            info.cob = Vdp2Regs->COAB & 0xFF;
            if (Vdp2Regs->COAB & 0x100)
               info.cob |= 0xFFFFFF00;
         }

         if (Vdp2Regs->CCCTL & 0x1)
            info.PostPixelFetchCalc = &DoColorCalcWithColorOffset;
         else
            info.PostPixelFetchCalc = &DoColorOffset;
      }
      else // color offset disable
      {
         if (Vdp2Regs->CCCTL & 0x1)
            info.PostPixelFetchCalc = &DoColorCalc;
         else
            info.PostPixelFetchCalc = &DoNothing;
      }

      for (i2 = 0; i2 < vdp2height; i2++)
      {
         for (i = 0; i < vdp2width; i++)
         {
            if (vdp1pixelsize == 2)
            {
               // 16-bit pixel size
               pixel = ((u16 *)vdp1framebuffer)[(i2 * vdp1width) + i];

               if (pixel == 0)
                  dispbuffer[(i2 * vdp2width) + i] = COLSATSTRIPPRIORITY(vdp2framebuffer[(i2 * vdp2width) + i]);
               else if (pixel & 0x8000)
               {
                  // 16 BPP               
                  if (prioritytable[0] >= Vdp2GetPixelPriority(vdp2framebuffer[(i2 * vdp2width) + i]))                    
                     dispbuffer[(i2 * vdp2width) + i] = info.PostPixelFetchCalc(&info, COLSAT2YAB16(0xFF, pixel));                     
                  else               
                     dispbuffer[(i2 * vdp2width) + i] = COLSATSTRIPPRIORITY(vdp2framebuffer[(i2 * vdp2width) + i]);
               }
               else
               {
                  // Color bank
                  Vdp1ProcessSpritePixel(&pixel, &shadow, &priority, &colorcalc);
                  if (prioritytable[priority] >= Vdp2GetPixelPriority(vdp2framebuffer[(i2 * vdp2width) + i]))
                     dispbuffer[(i2 * vdp2width) + i] = info.PostPixelFetchCalc(&info, COLSAT2YAB32(0xFF, Vdp2ColorRamGetColor(vdp1coloroffset | pixel)));
                  else               
                     dispbuffer[(i2 * vdp2width) + i] = COLSATSTRIPPRIORITY(vdp2framebuffer[(i2 * vdp2width) + i]);
               }
            }
            else
            {
               // 8-bit pixel size
               pixel = vdp1framebuffer[(i2 * vdp1width) + i];

               if (pixel == 0)
                  dispbuffer[(i2 * vdp2width) + i] = COLSATSTRIPPRIORITY(vdp2framebuffer[(i2 * vdp2width) + i]);
               else
               {
                  // Color bank(fix me)
                  LOG("8-bit Color Bank draw - %02X\n", pixel);
                  dispbuffer[(i2 * vdp2width) + i] = COLSATSTRIPPRIORITY(vdp2framebuffer[(i2 * vdp2width) + i]);
               }
            }
         }
      }
   }
   else
   {
      // Render VDP2 only
      for (i = 0; i < (vdp2width*vdp2height); i++)
         dispbuffer[i] = COLSATSTRIPPRIORITY(vdp2framebuffer[i]);
   }
#ifdef USE_OPENGL
   glRasterPos2i(0, 0);
   glPixelZoom((float)outputwidth / (float)vdp2width, 0 - ((float)outputheight / (float)vdp2height));
   glDrawPixels(vdp2width, vdp2height, GL_RGBA, GL_UNSIGNED_BYTE, dispbuffer);

#endif
   YuiSwapBuffers();
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2DrawScreens(void)
{
   int i;

   for (i = 1; i < 8; i++)
   {   
      if (nbg3priority == i)
         Vdp2DrawNBG3();
      if (nbg2priority == i)
         Vdp2DrawNBG2();
      if (nbg1priority == i)
         Vdp2DrawNBG1();
      if (nbg0priority == i)
         Vdp2DrawNBG0();
//      if (rbg0priority == i)
//         Vdp2DrawRBG0();
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2SetResolution(u16 TVMD)
{
   // This needs some work

   // Horizontal Resolution
   switch (TVMD & 0x7)
   {
      case 0:
         vdp2width = 320;
         break;
      case 1:
         vdp2width = 352;
         break;
      case 2:
//         vdp2width = 640;
         vdp2width = 320;
         break;
      case 3:
//         vdp2width = 704;
         vdp2width = 352;
         break;
      case 4:
         vdp2width = 320;
         break;
      case 5:
         vdp2width = 352;
         break;
      case 6:
//         vdp2width = 640;
         vdp2width = 320;
         break;
      case 7:
//         vdp2width = 704;
         vdp2width = 352;
         break;
   }

   // Vertical Resolution
   switch ((TVMD >> 4) & 0x3)
   {
      case 0:
         vdp2height = 224;
         break;
      case 1:
         vdp2height = 240;
         break;
      case 2:
         vdp2height = 256;
         break;
      default: break;
   }

   // Check for interlace
   switch ((TVMD >> 6) & 0x3)
   {
      case 2: // Single-density Interlace
      case 3: // Double-density Interlace
//         vdp2height *= 2;
         break;
      case 0: // Non-interlace
      default: break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG0(int priority)
{
   nbg0priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG1(int priority)
{
   nbg1priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG2(int priority)
{
   nbg2priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityNBG3(int priority)
{
   nbg3priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSoftVdp2SetPriorityRBG0(int priority)
{
   rbg0priority = priority;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2ToggleDisplayNBG0(void)
{
   vdp2disptoggle ^= 0x1;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2ToggleDisplayNBG1(void)
{
   vdp2disptoggle ^= 0x2;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2ToggleDisplayNBG2(void)
{
   vdp2disptoggle ^= 0x4;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2ToggleDisplayNBG3(void)
{
   vdp2disptoggle ^= 0x8;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftVdp2ToggleDisplayRBG0(void)
{
   vdp2disptoggle ^= 0x10;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftOnScreenDebugMessage(char *string, ...)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSoftGetScreenSize(int *width, int *height)
{
   *width = vdp2width;
   *height = vdp2height;
}

//////////////////////////////////////////////////////////////////////////////

