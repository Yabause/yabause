/*  Copyright 2003-2006 Guillaume Duhamel
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef VIDSHARED_H
#define VIDSHARED_H

#include "core.h"
#include "vdp2.h"

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
   int rotatenum;
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
   float Px;
   float Py;
   float Pz;
   float Cx;
   float Cy;
   float Cz;
   float Mx;
   float My;
   float kx;
   float ky;
   float KAst;
   float deltaKAst;
   float deltaKAx;
   u32 coeftbladdr;
   int coefenab;
   int coefmode;
   int coefdatasize;
   float Xp;
   float Yp;
   float dX;
   float dY;
   int screenover;
   int msb;
} vdp2rotationparameter_struct;

void FASTCALL Vdp2NBG0PlaneAddr(vdp2draw_struct *info, int i);
void FASTCALL Vdp2NBG1PlaneAddr(vdp2draw_struct *info, int i);
void FASTCALL Vdp2NBG2PlaneAddr(vdp2draw_struct *info, int i);
void FASTCALL Vdp2NBG3PlaneAddr(vdp2draw_struct *info, int i);
void Vdp2ReadRotationTable(int which, vdp2rotationparameter_struct *parameter);
void FASTCALL Vdp2ParameterAPlaneAddr(vdp2draw_struct *info, int i);
void FASTCALL Vdp2ParameterBPlaneAddr(vdp2draw_struct *info, int i);
float Vdp2ReadCoefficientMode0_2(vdp2rotationparameter_struct *parameter, u32 addr);

//////////////////////////////////////////////////////////////////////////////

static INLINE int GenerateRotatedXPos(vdp2draw_struct *info, vdp2rotationparameter_struct *p, int x, int y)
{
   float Xsp = p->A * ((p->Xst + p->deltaXst * y) - p->Px) +
               p->B * ((p->Yst + p->deltaYst * y) - p->Py) +
               p->C * (p->Zst - p->Pz);

   return (int)(p->kx * (Xsp + p->dX * (float)x) + p->Xp);
}

//////////////////////////////////////////////////////////////////////////////

static INLINE int GenerateRotatedYPos(vdp2draw_struct *info, vdp2rotationparameter_struct *p, int x, int y)
{
   float Ysp = p->D * ((p->Xst + p->deltaXst * y) - p->Px) +
               p->E * ((p->Yst + p->deltaYst * y) - p->Py) +
               p->F * (p->Zst - p->Pz);

   return (int)(p->ky * (Ysp + p->dY * (float)x) + p->Yp);
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void CalculateRotationValues(vdp2rotationparameter_struct *p)
{
   p->Xp=p->A * (p->Px - p->Cx) +
         p->B * (p->Py - p->Cy) +
         p->C * (p->Pz - p->Cz) +
         p->Cx + p->Mx;
   p->Yp=p->D * (p->Px - p->Cx) +
         p->E * (p->Py - p->Cy) +
         p->F * (p->Pz - p->Cz) +
         p->Cy + p->My;
   p->dX=p->A * p->deltaX +
         p->B * p->deltaY;
   p->dY=p->D * p->deltaX +
         p->E * p->deltaY;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void CalcPlaneAddr(vdp2draw_struct *info, u32 tmp)
{
   int deca = info->planeh + info->planew - 2;
   int multi = info->planeh * info->planew;
     
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

static INLINE void ReadBitmapSize(vdp2draw_struct *info, u16 bm, int mask)
{
   switch(bm & mask)
   {
      case 0: info->cellw = 512;
              info->cellh = 256;
              break;
      case 1: info->cellw = 512;
              info->cellh = 512;
              break;
      case 2: info->cellw = 1024;
              info->cellh = 256;
              break;
      case 3: info->cellw = 1024;
              info->cellh = 512;
              break;
   }
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void ReadPlaneSize(vdp2draw_struct *info, u16 reg)
{
   switch(reg & 0x3)
   {
      case 0:
         info->planew = info->planeh = 1;
         break;
      case 1:
         info->planew = 2;
         info->planeh = 1;
         break;
      case 3:
         info->planew = info->planeh = 2;
         break;
      default: // Not sure what 0x2 does, though a few games seem to use it
         info->planew = info->planeh = 1;
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void ReadPatternData(vdp2draw_struct *info, u16 pnc, int chctlwh)
{
   if(pnc & 0x8000)
      info->patterndatasize = 1;
   else
      info->patterndatasize = 2;

   if(chctlwh)
      info->patternwh = 2;
   else
      info->patternwh = 1;

   info->pagewh = 64/info->patternwh;
   info->cellw = info->cellh = 8;
   info->supplementdata = pnc & 0x3FF;
   info->auxmode = (pnc & 0x4000) >> 14;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE int IsScreenRotated(vdp2rotationparameter_struct *parameter)
{
  return (parameter->deltaXst == 0.0 &&
          parameter->deltaYst == 1.0 &&
          parameter->deltaX == 1.0 &&
          parameter->deltaY == 0.0 &&
          parameter->A == 1.0 &&
          parameter->B == 0.0 &&
          parameter->C == 0.0 &&
          parameter->D == 0.0 &&
          parameter->E == 1.0 &&
          parameter->F == 0.0);
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void Vdp2ReadCoefficient(vdp2rotationparameter_struct *parameter, u32 addr)
{
   switch (parameter->coefmode)
   {
      case 0: // coefficient for kx and ky
         parameter->kx = parameter->ky = Vdp2ReadCoefficientMode0_2(parameter, addr);
         break;
      case 1: // coefficient for kx
         parameter->kx = Vdp2ReadCoefficientMode0_2(parameter, addr);
         break;
      case 2: // coefficient for ky
         parameter->ky = Vdp2ReadCoefficientMode0_2(parameter, addr);
         break;
      case 3: // coefficient for Xp
      {
         s32 i;

         if (parameter->coefdatasize == 2)
         {
            i = T1ReadWord(Vdp2Ram, addr);
            parameter->Xp = (float) (signed) ((i & 0x7FFF) | (i & 0x4000 ? 0xFFFFC000 : 0x00000000)) / 4;
         }
         else
         {
            i = T1ReadLong(Vdp2Ram, addr);
            parameter->Xp = (float) (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000)) / 256;
         }
         break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

#endif
