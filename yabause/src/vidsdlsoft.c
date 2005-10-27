/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004-2005 Theo Berkau

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

#include "vidsdlsoft.h"
#include "SDL.h"
#include "debug.h"
#include "vdp2.h"

int VIDSDLSoftInit(void);
void VIDSDLSoftDeInit(void);
void VIDSDLSoftResize(unsigned int, unsigned int, int);
int VIDSDLSoftVdp1Reset(void);
void VIDSDLSoftVdp1DrawStart(void);
void VIDSDLSoftVdp1DrawEnd(void);
void VIDSDLSoftVdp1NormalSpriteDraw(void);
void VIDSDLSoftVdp1ScaledSpriteDraw(void);
void VIDSDLSoftVdp1DistortedSpriteDraw(void);
void VIDSDLSoftVdp1PolygonDraw(void);
void VIDSDLSoftVdp1PolylineDraw(void);
void VIDSDLSoftVdp1LineDraw(void);
void VIDSDLSoftVdp1UserClipping(void);
void VIDSDLSoftVdp1SystemClipping(void);
void VIDSDLSoftVdp1LocalCoordinate(void);
int VIDSDLSoftVdp2Reset(void);
void VIDSDLSoftVdp2DrawStart(void);
void VIDSDLSoftVdp2DrawEnd(void);
void VIDSDLSoftVdp2DrawScreens(void);
void VIDSDLSoftVdp2SetResolution(u16 TVMD);
void FASTCALL VIDSDLSoftVdp2SetPriorityNBG0(int priority);
void FASTCALL VIDSDLSoftVdp2SetPriorityNBG1(int priority);
void FASTCALL VIDSDLSoftVdp2SetPriorityNBG2(int priority);
void FASTCALL VIDSDLSoftVdp2SetPriorityNBG3(int priority);
void FASTCALL VIDSDLSoftVdp2SetPriorityRBG0(int priority);
void VIDSDLSoftVdp2ToggleDisplayNBG0(void);
void VIDSDLSoftVdp2ToggleDisplayNBG1(void);
void VIDSDLSoftVdp2ToggleDisplayNBG2(void);
void VIDSDLSoftVdp2ToggleDisplayNBG3(void);
void VIDSDLSoftVdp2ToggleDisplayRBG0(void);

VideoInterface_struct VIDSDLSoft = {
VIDCORE_SDLSOFT,
"SDL Software Video Interface",
VIDSDLSoftInit,
VIDSDLSoftDeInit,
VIDSDLSoftResize,
VIDSDLSoftVdp1Reset,
VIDSDLSoftVdp1DrawStart,
VIDSDLSoftVdp1DrawEnd,
VIDSDLSoftVdp1NormalSpriteDraw,
VIDSDLSoftVdp1ScaledSpriteDraw,
VIDSDLSoftVdp1DistortedSpriteDraw,
VIDSDLSoftVdp1PolygonDraw,
VIDSDLSoftVdp1PolylineDraw,
VIDSDLSoftVdp1LineDraw,
VIDSDLSoftVdp1UserClipping,
VIDSDLSoftVdp1SystemClipping,
VIDSDLSoftVdp1LocalCoordinate,
VIDSDLSoftVdp2Reset,
VIDSDLSoftVdp2DrawStart,
VIDSDLSoftVdp2DrawEnd,
VIDSDLSoftVdp2DrawScreens,
VIDSDLSoftVdp2SetResolution,
VIDSDLSoftVdp2SetPriorityNBG0,
VIDSDLSoftVdp2SetPriorityNBG1,
VIDSDLSoftVdp2SetPriorityNBG2,
VIDSDLSoftVdp2SetPriorityNBG3,
VIDSDLSoftVdp2SetPriorityRBG0,
VIDSDLSoftVdp2ToggleDisplayNBG0,
VIDSDLSoftVdp2ToggleDisplayNBG1,
VIDSDLSoftVdp2ToggleDisplayNBG2,
VIDSDLSoftVdp2ToggleDisplayNBG3,
VIDSDLSoftVdp2ToggleDisplayRBG0
};

static SDL_Surface *SDLframebuffer;
static int fps;
static int framecount;
static u32 ticks;

SDL_Rect vdp2rect = {
0, 0, 320, 224
};


typedef struct
{
   int vertices[8];
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

   int cor;
   int cog;
   int cob;

   float coordincx, coordincy;
   void FASTCALL (* PlaneAddr)(void *, int);
} vdp2draw_struct;

#if defined WORDS_BIGENDIAN
#define COLSAT2YAB16(alpha,temp)            (alpha | (temp & 0x7C00) << 1 | (temp & 0x3E0) << 14 | (temp & 0x1F) << 27)
#else
#define COLSAT2YAB16(alpha,temp)            (alpha << 24 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9)
#endif

#if defined WORDS_BIGENDIAN
#define COLSAT2YAB32(alpha,temp)            ((temp & 0xFF << 24) | ((temp & 0xFF00) << 8) | ((temp & 0xFF0000) >> 8) | alpha)
#else
#define COLSAT2YAB32(alpha,temp)            (alpha << 24 | (temp & 0xFF0000) | (temp & 0xFF00) | (temp & 0xFF))
#endif

//////////////////////////////////////////////////////////////////////////////

static inline void putpixel16(s32 x, s32 y, u16 color, int alpha)
{
   *((u32 *)(((u8 *)SDLframebuffer->pixels) + (y * SDLframebuffer->pitch) + (x * SDLframebuffer->format->BytesPerPixel))) = COLSAT2YAB16(alpha, color);
}

//////////////////////////////////////////////////////////////////////////////

static inline void putpixel32(s32 x, s32 y, u32 color, int alpha)
{
   *((u32 *)(((u8 *)SDLframebuffer->pixels) + (y * SDLframebuffer->pitch) + (x * SDLframebuffer->format->BytesPerPixel))) = COLSAT2YAB32(alpha, color);
}

//////////////////////////////////////////////////////////////////////////////

static inline void puthline16(s32 x, s32 y, s32 width, u16 color, int alpha)
{
   u32 *buffer = ((u32 *) (((u8 *)SDLframebuffer->pixels) + (y * SDLframebuffer->pitch)));
   u32 dot=COLSAT2YAB16(alpha, color);
   int i;

   for (i = x; i < x+width; i++)
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

void Vdp2DrawScrollBitmap(vdp2draw_struct *info)
{
   int i, i2;

   int xstart, ystart;
   int xend, yend;
   int pixeloffset=0, lineincrement=0;

   // Handle clipping(both window and screen clipping)
   if (info->x < 0)
   {
      xstart = 0;
      pixeloffset = 0 - info->x;
   }
   else
      xstart = info->x;

   if ((info->x+info->cellw) > vdp2rect.w)
   {
      xend = vdp2rect.w;
      lineincrement = (info->x+info->cellw) - vdp2rect.w;
   }
   else
      xend = (info->x+info->cellw);

   if (info->y < 0)
   {
      ystart = 0;
      pixeloffset =  (info->cellw * (0 - info->y)) + pixeloffset;
   }
   else
      ystart = info->y;

   if ((info->y+info->cellh) >= vdp2rect.h)
      yend = vdp2rect.h;
   else
      yend = (info->y+info->cellh);

   switch (info->colornumber)
   {
      case 0: // 4 BPP(16 colors)
         // fix me
         LOG("vdp2 bitmap 4 bpp draw\n");
         return;
      case 1: // 8 BPP(256 colors)
//         LOG("vdp2 bitmap 8 bpp draw\n");

         info->charaddr += pixeloffset;
         
         for (i = ystart; i < yend; i++)
         {
            for (i2 = xstart; i2 < xend; i2++)
            {
               u32 color = T1ReadByte(Vdp2Ram, info->charaddr);
               info->charaddr++;

               if (color == 0 && !info->transparencyenable) // fix me
                  continue;

               color = Vdp2ColorRamGetColor(info->paladdr | color);
               putpixel32(i2, i, color, info->alpha);
            }

            info->charaddr += lineincrement;
         }

         return;
      case 3: // 15 BPP
         LOG("vdp2 bitmap 15 bpp draw\n");

         pixeloffset *= 2;
         lineincrement *= 2;

         info->charaddr += pixeloffset;

         for (i = ystart; i < yend; i++)
         {
            for (i2 = xstart; i2 < xend; i2++)
            {
               u32 color = T1ReadWord(Vdp2Ram, info->charaddr);
               info->charaddr += 2;

               if ((color & 0x8000) == 0 && !info->transparencyenable) // fix me
                  continue;

               putpixel32(i2, i, SDL_MapRGB(SDLframebuffer->format, (color & 0x1F) << 3, (color & 0x3E0) >> 2, (color & 0x7C00) >> 7), info->alpha);
            }

            info->charaddr += lineincrement;
         }

         return;
      case 4: // 24 BPP
         LOG("vdp2 bitmap 24 bpp draw\n");

         pixeloffset *= 4;
         lineincrement *= 4;

         info->charaddr += pixeloffset;

         for (i = ystart; i < yend; i++)
         {
            for (i2 = xstart; i2 < xend; i2++)
            {
               u32 color = T1ReadLong(Vdp2Ram, info->charaddr);
               info->charaddr += 4;

               if ((color & 0x80000000) == 0 && !info->transparencyenable) // fix me
                  continue;

               putpixel32(i2, i, SDL_MapRGB(SDLframebuffer->format, color, color >> 8, color >> 16), info->alpha);
            }
         }
         return;
      default: break;
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawBackScreen(void)
{
   int i;

   if ((Vdp2Regs->TVMD & 0x100) == 0)
   {
      // Draw Black
      SDL_FillRect(SDLframebuffer, &vdp2rect, SDL_MapRGB(SDLframebuffer->format, 0, 0, 0));
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
         for (i = 0; i < vdp2rect.h; i++)
         {
            dot = T1ReadWord(Vdp2Ram, scrAddr);
            scrAddr += 2;

            puthline16(vdp2rect.x, vdp2rect.y+i, vdp2rect.w, dot, SDL_ALPHA_OPAQUE);
         }
      }
      else
      {
         // Single Color
         dot = T1ReadWord(Vdp2Ram, scrAddr);

         SDL_FillRect(SDLframebuffer, &vdp2rect, SDL_MapRGB(SDLframebuffer->format, (dot & 0x1F) << 3, (dot & 0x3E0) >> 2, (dot & 0x7C00) >> 7));
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG0(void)
{
   vdp2draw_struct info;

   /* FIXME should start by checking if it's a normal
    * or rotate scroll screen
    */
   info.enable = Vdp2Regs->BGON & 0x1;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x100);

   info.x = - ((Vdp2Regs->SCXIN0 & 0x7FF) % 512);
   info.y = - ((Vdp2Regs->SCYIN0 & 0x7FF) % 512);

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
         case 2:
            info.planew = info.planeh = 2;
            break;
      }

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
      info.alpha = SDL_ALPHA_OPAQUE;

   info.coloroffset = Vdp2Regs->CRAOFA & 0x7;

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
   }
   else // color offset disable
      info.cor = info.cog = info.cob = 0;

   info.coordincx = (float) 65536 / (Vdp2Regs->ZMXN0.all & 0x7FF00);
   info.coordincy = (float) 65536 / (Vdp2Regs->ZMYN0.all & 0x7FF00);

//   info.priority = nbg0priority;
//   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG0PlaneAddr;

//   if (!(info.enable & vdp2disptoggle) || (info.priority == 0))
//      return;

   if (info.isbitmap)
   {
      info.vertices[0] = info.x * info.coordincx;
      info.vertices[1] = info.y * info.coordincy;
      info.vertices[2] = (info.x + info.cellw) * info.coordincx;
      info.vertices[3] = info.y * info.coordincy;
      info.vertices[4] = (info.x + info.cellw) * info.coordincx;
      info.vertices[5] = (info.y + info.cellh) * info.coordincy;
      info.vertices[6] = info.x * info.coordincx;
      info.vertices[7] = (info.y + info.cellh) * info.coordincy;

      Vdp2DrawScrollBitmap(&info);
   }
   else
   {
      // Draw Tile Map here
   }
}

//////////////////////////////////////////////////////////////////////////////

static void Vdp2DrawNBG1(void)
{
   vdp2draw_struct info;

   info.enable = Vdp2Regs->BGON & 0x2;
   info.transparencyenable = !(Vdp2Regs->BGON & 0x200);
   info.x = - ((Vdp2Regs->SCXIN1 & 0x7FF) % 512);
   info.y = - ((Vdp2Regs->SCYIN1 & 0x7FF) % 512);

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
         case 2:
            info.planew = info.planeh = 2;
            break;
      }

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

   info.coloroffset = (Vdp2Regs->CRAOFA & 0x70) >> 4;

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
   }
   else // color offset disable
      info.cor = info.cog = info.cob = 0;

   info.coordincx = (float) 65536 / (Vdp2Regs->ZMXN1.all & 0x7FF00);
   info.coordincy = (float) 65536 / (Vdp2Regs->ZMXN1.all & 0x7FF00);

//   info.priority = nbg1priority;
//   info.PlaneAddr = (void FASTCALL (*)(void *, int))&Vdp2NBG1PlaneAddr;

//   if (!(info.enable & vdp2disptoggle) || (info.priority == 0))
//      return;

   if (info.isbitmap)
   {
      info.vertices[0] = info.x * info.coordincx;
      info.vertices[1] = info.y * info.coordincy;
      info.vertices[2] = (info.x + info.cellw) * info.coordincx;
      info.vertices[3] = info.y * info.coordincy;
      info.vertices[4] = (info.x + info.cellw) * info.coordincx;
      info.vertices[5] = (info.y + info.cellh) * info.coordincy;
      info.vertices[6] = info.x * info.coordincx;
      info.vertices[7] = (info.y + info.cellh) * info.coordincy;

      Vdp2DrawScrollBitmap(&info);
   }
   else
   {
      fprintf(stderr, "nbg1 tile draw\n");
      // Draw Tile Map here
   }
}

//////////////////////////////////////////////////////////////////////////////

int VIDSDLSoftInit(void)
{
   char yab_version[64];

   SDL_InitSubSystem(SDL_INIT_VIDEO);

   // Set the window title
   sprintf(yab_version, "Yabause %s", VERSION);
   SDL_WM_SetCaption(yab_version, NULL);

   if ((SDLframebuffer = SDL_SetVideoMode(320, 224, 32, SDL_SWSURFACE | SDL_RESIZABLE)) == NULL)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftDeInit(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftResize(unsigned int w, unsigned int h, int on)
{
}

//////////////////////////////////////////////////////////////////////////////

int VIDSDLSoftVdp1Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1DrawStart(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1DrawEnd(void)
{
   fprintf(stderr, "%02d/60 FPS\n", fps);

   framecount++;
   if(SDL_GetTicks() >= ticks + 1000)
   {
      fps = framecount;
      framecount = 0;
      ticks = SDL_GetTicks();
   }
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1NormalSpriteDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1ScaledSpriteDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1DistortedSpriteDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1PolygonDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1PolylineDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1LineDraw(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1UserClipping(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1SystemClipping(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp1LocalCoordinate(void)
{
}

//////////////////////////////////////////////////////////////////////////////

int VIDSDLSoftVdp2Reset(void)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp2DrawStart(void)
{
   Vdp2DrawBackScreen();
   Vdp2DrawNBG1(); // fix me
//   Vdp2DrawNBG0(); // fix me
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp2DrawEnd(void)
{
   SDL_Flip(SDLframebuffer);
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp2DrawScreens(void)
{

}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp2SetResolution(u16 TVMD)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSDLSoftVdp2SetPriorityNBG0(int priority)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSDLSoftVdp2SetPriorityNBG1(int priority)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSDLSoftVdp2SetPriorityNBG2(int priority)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSDLSoftVdp2SetPriorityNBG3(int priority)
{
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL VIDSDLSoftVdp2SetPriorityRBG0(int priority)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp2ToggleDisplayNBG0(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp2ToggleDisplayNBG1(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp2ToggleDisplayNBG2(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp2ToggleDisplayNBG3(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void VIDSDLSoftVdp2ToggleDisplayRBG0(void)
{
}

//////////////////////////////////////////////////////////////////////////////
