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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file vidshared.c
    \brief Functions shared between opengl and software video renderers.
*/

#include "ygl.h"
#include "vidshared.h"
#include "vdp2.h"
#include "vdp1.h"
#include "debug.h"
#include "cs2.h"

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2NBG0PlaneAddr(vdp2draw_struct *info, int i, Vdp2* regs)
{
   u32 offset = (regs->MPOFN & 0x7) << 6;
   u32 tmp=0;

   switch(i)
   {
      case 0:
         tmp = offset | (regs->MPABN0 & 0xFF);
         break;
      case 1:
         tmp = offset | (regs->MPABN0 >> 8);
         break;
      case 2:
         tmp = offset | (regs->MPCDN0 & 0xFF);
         break;
      case 3:
         tmp = offset | (regs->MPCDN0 >> 8);
         break;
   }

   CalcPlaneAddr(info, tmp);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2NBG1PlaneAddr(vdp2draw_struct *info, int i, Vdp2* regs)
{
   u32 offset = (regs->MPOFN & 0x70) << 2;
   u32 tmp=0;

   switch(i)
   {
      case 0:
         tmp = offset | (regs->MPABN1 & 0xFF);
         break;
      case 1:
         tmp = offset | (regs->MPABN1 >> 8);
         break;
      case 2:
         tmp = offset | (regs->MPCDN1 & 0xFF);
         break;
      case 3:
         tmp = offset | (regs->MPCDN1 >> 8);
         break;
   }

   CalcPlaneAddr(info, tmp);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2NBG2PlaneAddr(vdp2draw_struct *info, int i, Vdp2* regs)
{
   u32 offset = (regs->MPOFN & 0x700) >> 2;
   u32 tmp=0;

   switch(i)
   {
      case 0:
         tmp = offset | (regs->MPABN2 & 0xFF);
         break;
      case 1:
         tmp = offset | (regs->MPABN2 >> 8);
         break;
      case 2:
         tmp = offset | (regs->MPCDN2 & 0xFF);
         break;
      case 3:
         tmp = offset | (regs->MPCDN2 >> 8);
         break;
   }

   CalcPlaneAddr(info, tmp);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2NBG3PlaneAddr(vdp2draw_struct *info, int i, Vdp2* regs)
{
   u32 offset = (regs->MPOFN & 0x7000) >> 6;
   u32 tmp=0;

   switch(i)
   {
      case 0:
         tmp = offset | (regs->MPABN3 & 0xFF);
         break;
      case 1:
         tmp = offset | (regs->MPABN3 >> 8);
         break;
      case 2:
         tmp = offset | (regs->MPCDN3 & 0xFF);
         break;
      case 3:
         tmp = offset | (regs->MPCDN3 >> 8);
         break;
   }

   CalcPlaneAddr(info, tmp);
}

//////////////////////////////////////////////////////////////////////////////
int Vdp2GetBank(Vdp2* regs, u32 addr){
	
	// 4Mbit mode
	if ( (regs->VRSIZE&0x8000) == 0){

		if (addr >= 0 && addr < 0x20000){
			return VDP2_VRAM_A0;
		}
		else if (addr >= 0x20000 && addr < 0x40000){
			if (regs->RAMCTL & 0x10){
				return VDP2_VRAM_A1;
			}
			else{
				return VDP2_VRAM_A0;
			}
			
		}
		else if (addr >= 0x40000 && addr < 0x60000){
			return VDP2_VRAM_B0;
		}
		else if (addr >= 0x60000 && addr < 0x80000){
			if (regs->RAMCTL & 0x20){
				return VDP2_VRAM_B1;
			}
			else{
				return VDP2_VRAM_B0;
			}
		}

	}
	// 8mbit mode
	else{
		if (addr >= 0 && addr < 0x40000){
			return VDP2_VRAM_A0;
		}
		else if (addr >= 0x40000 && addr < 0x80000){
			if (regs->RAMCTL & 0x10){
				return VDP2_VRAM_A1;
			}
			else{
				return VDP2_VRAM_A0;
			}
		}
		else if (addr >= 0x80000 && addr < 0xc0000){
			return VDP2_VRAM_B0;
		}
		else if (addr >= 0xc0000 && addr < 0x100000){
			if (regs->RAMCTL & 0x20){
				return VDP2_VRAM_B1;
			}
			else{
				return VDP2_VRAM_B0;
			}
		}
	}

	return 0;
}

void Vdp2ReadRotationTable(int which, vdp2rotationparameter_struct *parameter, Vdp2* regs, u8* ram)
{
   s32 i;
   u32 addr;
   int bank;

   addr = regs->RPTA.all << 1;
   bank = Vdp2GetBank(regs, addr);

   if (which == 0)
   {
      // Rotation Parameter A
      addr &= 0x000FFF7C;
      parameter->coefenab = regs->KTCTL & 0x1;
      parameter->screenover = (regs->PLSZ >> 10) & 0x3;
   }
   else
   {
      // Rotation Parameter B
      addr = (addr & 0x000FFFFC) | 0x00000080;
      parameter->coefenab = regs->KTCTL & 0x100;
      parameter->screenover = (regs->PLSZ >> 14) & 0x3;
   }

   i = T1ReadLong(ram, addr);
   parameter->Xst = (float) (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->Yst = (float) (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->Zst = (float) (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->deltaXst = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->deltaYst = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->deltaX = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->deltaY = (float) (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->A = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->B = (float) (signed) ((i & 0x000FFFC0) | ((i & 0x00080000) ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->C = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->D = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->E = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->F = (float) (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadWord(ram, addr);
   parameter->Px = (float) (signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000));
   addr += 2;

   i = T1ReadWord(ram, addr);
   parameter->Py = (float) (signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000));
   addr += 2;

   i = T1ReadWord(ram, addr);
   parameter->Pz = (float) (signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000));
   addr += 4;

   i = T1ReadWord(ram, addr);
   parameter->Cx = (float) (signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000));
   addr += 2;

   i = T1ReadWord(ram, addr);
   parameter->Cy = (float) (signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000));
   addr += 2;

   i = T1ReadWord(ram, addr);
   parameter->Cz = (float) (signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->Mx = (float) (signed) ((i & 0x3FFFFFC0) | (i & 0x20000000 ? 0xE0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->My = (float) (signed) ((i & 0x3FFFFFC0) | (i & 0x20000000 ? 0xE0000000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->kx = (float) (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000)) / 65536;
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->ky = (float) (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000)) / 65536;
   addr += 4;

   if (parameter->coefenab)
   {
	    int perdot = 0;
      float ftmp;
      u32 tmp;

      parameter->K_update = 0;

      // Read in coefficient values
      i = T1ReadLong(ram, addr);
      ftmp = (float)(unsigned)(i & 0xFFFFFFC0) / 65536;
      if (ftmp != parameter->KAst){
        parameter->K_update = 1;
        parameter->KAst = ftmp;
      }

      addr += 4;

      i = T1ReadLong(ram, addr);
      ftmp = (float)(signed)((i & 0x03FFFFC0) | (i & 0x02000000 ? 0xFE000000 : 0x00000000)) / 65536;
      if (ftmp != parameter->deltaKAst){
        parameter->K_update = 1;
        parameter->deltaKAst = ftmp;
      }
      addr += 4;     

      if (regs->RAMCTL & 0x8000){
        if (parameter->k_mem_type == 0){
          parameter->ktablesize = 0;
        }
        parameter->k_mem_type = 1; // use cram
        i = T1ReadLong(ram, addr);
        ftmp = (float)(signed)((i & 0x03FFFFC0) | (i & 0x02000000 ? 0xFE000000 : 0x00000000)) / 65536;
        if (ftmp != parameter->deltaKAx){
          parameter->K_update = 1;
          parameter->deltaKAx = ftmp;
        }
        
        if (Vdp2ColorRamUpdated){
          parameter->K_update = 1;
        }

      }
      else{
        if (parameter->k_mem_type == 1) {
          parameter->ktablesize = 0;
        }
        parameter->k_mem_type = 0; // use vram
        // hard/vdp2/hon/p06_20.htm#no6_21
        switch (bank)
        {
        case VDP2_VRAM_A0:
          perdot = (regs->RAMCTL & 0x03);
          break;
        case VDP2_VRAM_A1:
          perdot = ((regs->RAMCTL >> 2) & 0x03);
          break;
        case VDP2_VRAM_B0:
          perdot = ((regs->RAMCTL >> 4) & 0x03);
          break;
        case VDP2_VRAM_B1:
          perdot = ((regs->RAMCTL >> 6) & 0x03);
          break;
        }
        
        if (perdot != 1){
          if (parameter->deltaKAx != 0.0){
            parameter->K_update = 1;
          }
          parameter->deltaKAx = 0.0f;
        }
        else{
          i = T1ReadLong(ram, addr);
          ftmp = (float)(signed)((i & 0x03FFFFC0) | (i & 0x02000000 ? 0xFE000000 : 0x00000000)) / 65536;
          if (ftmp != parameter->deltaKAx){
            parameter->K_update = 1;
            parameter->deltaKAx = ftmp;
          }
         
        }
      }


	  addr += 4;

    if (which == 0) {

      tmp = (regs->KTCTL & 0x2 ? 2 : 4);
      if (tmp != parameter->coefdatasize){
        parameter->K_update = 1;
        parameter->coefdatasize = tmp;
      }

      tmp = ((regs->KTAOF & 0x7) * 0x10000 + (int)(parameter->KAst)) * parameter->coefdatasize;
      if (tmp != parameter->coeftbladdr){
        parameter->K_update = 1;
        parameter->coeftbladdr = tmp;
      }

      tmp = (regs->KTCTL >> 2) & 0x3;
      if (tmp != parameter->coefmode){
        parameter->K_update = 1;
        parameter->coefmode = tmp;
      }

      tmp = (regs->KTCTL >> 4) & 0x01;
      if (tmp != parameter->use_coef_for_linecolor){
        parameter->K_update = 1;
        parameter->use_coef_for_linecolor = tmp;
      }

    }else{

      tmp = (regs->KTCTL & 0x200 ? 2 : 4);
      if (tmp != parameter->coefdatasize){
        parameter->K_update = 1;
        parameter->coefdatasize = tmp;
      }
      

      tmp = (((regs->KTAOF >> 8) & 0x7) * 0x10000 + (int)(parameter->KAst)) * parameter->coefdatasize;
      if (tmp != parameter->coeftbladdr){
        parameter->K_update = 1;
        parameter->coeftbladdr = tmp;
      }
      
      tmp = (regs->KTCTL >> 10) & 0x3;
      if (tmp != parameter->coefmode){
        parameter->K_update = 1;
        parameter->coefmode = tmp;
      }
      

      tmp = (regs->KTCTL >> 12) & 0x01;
      if (tmp != parameter->use_coef_for_linecolor){
        parameter->K_update = 1;
        parameter->use_coef_for_linecolor = tmp;
      }

		  if (regs->RPMD == 0x02){
        if (parameter->deltaKAx != 0.0f ) parameter->K_update = 1;
		  	parameter->deltaKAx = 0.0f; // hard/vdp2/hon/p06_35.htm#RPMD_
		  }
    }
      

    if (parameter->k_mem_type == 0) {

      u32 kaddr = (parameter->coeftbladdr+0x01);
      if (A0_Updated == 1 && kaddr >= 0 && kaddr < 0x20000){
        parameter->K_update = 1;
      }
      else if (A1_Updated == 1 && kaddr >= 0x20000 && kaddr < 0x40000){
        parameter->K_update = 1;
      }
      else if (B0_Updated == 1 && kaddr >= 0x40000 && kaddr < 0x60000){
        parameter->K_update = 1;
      }
      else if (B1_Updated == 1 && kaddr >= 0x60000 && kaddr < 0x80000){
        parameter->K_update = 1;
      }

    }
  }
  else{
    parameter->use_coef_for_linecolor = 0;
  }
   
  VDP2LOG("Xst: %f, Yst: %f, Zst: %f, deltaXst: %f deltaYst: %f deltaX: %f\n", parameter->Xst, parameter->Yst, parameter->Zst, parameter->deltaXst,
    parameter->deltaYst, parameter->deltaX);
  VDP2LOG("deltaY: %f A: %f B: %f C: %f D: %f E: %f F: %f Px: %f Py: %f Pz: %f\n", parameter->deltaY,
    parameter->A, parameter->B, parameter->C, parameter->D, parameter->E,
    parameter->F, parameter->Px, parameter->Py, parameter->Pz);
  VDP2LOG("Cx: %f Cy: %f Cz: %f Mx: %f My: %f kx: %f ky: %f KAst: %f\n", parameter->Cx, parameter->Cy, parameter->Cz, parameter->Mx,
    parameter->My, parameter->kx, parameter->ky, parameter->KAst);
  VDP2LOG("deltaKAst: %f deltaKAx: %f kadr %08X\n",parameter->deltaKAst, parameter->deltaKAx,parameter->coeftbladdr);
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2ReadRotationTableFP(int which, vdp2rotationparameterfp_struct *parameter, Vdp2* regs, u8 * ram)
{
   s32 i;
   u32 addr;

   addr = regs->RPTA.all << 1;

   if (which == 0)
   {
      // Rotation Parameter A
      addr &= 0x000FFF7C;
      parameter->coefenab = regs->KTCTL & 0x1;
      parameter->screenover = (regs->PLSZ >> 10) & 0x3;
   }
   else
   {
      // Rotation Parameter B
      addr = (addr & 0x000FFFFC) | 0x00000080;
      parameter->coefenab = regs->KTCTL & 0x100;
      parameter->screenover = (regs->PLSZ >> 14) & 0x3;
   }

   i = T1ReadLong(ram, addr);
   parameter->Xst = (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->Yst = (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->Zst = (signed) ((i & 0x1FFFFFC0) | (i & 0x10000000 ? 0xF0000000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->deltaXst = (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->deltaYst = (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->deltaX = (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->deltaY = (signed) ((i & 0x0007FFC0) | (i & 0x00040000 ? 0xFFFC0000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->A = (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->B = (signed) ((i & 0x000FFFC0) | ((i & 0x00080000) ? 0xFFF80000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->C = (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->D = (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->E = (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->F = (signed) ((i & 0x000FFFC0) | (i & 0x00080000 ? 0xFFF80000 : 0x00000000));
   addr += 4;

   i = T1ReadWord(ram, addr);
   parameter->Px = tofixed((signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000)));
   addr += 2;

   i = T1ReadWord(ram, addr);
   parameter->Py = tofixed((signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000)));
   addr += 2;

   i = T1ReadWord(ram, addr);
   parameter->Pz = tofixed((signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000)));
   addr += 4;

   i = T1ReadWord(ram, addr);
   parameter->Cx = tofixed((signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000)));
   addr += 2;

   i = T1ReadWord(ram, addr);
   parameter->Cy = tofixed((signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000)));
   addr += 2;

   i = T1ReadWord(ram, addr);
   parameter->Cz = tofixed((signed) ((i & 0x3FFF) | (i & 0x2000 ? 0xFFF80000 : 0x00000000)));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->Mx = (signed) ((i & 0x3FFFFFC0) | (i & 0x20000000 ? 0xE0000000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->My = (signed) ((i & 0x3FFFFFC0) | (i & 0x20000000 ? 0xE0000000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->kx = (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000));
   addr += 4;

   i = T1ReadLong(ram, addr);
   parameter->ky = (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000));
   addr += 4;

   if (parameter->coefenab)
   {
      // Read in coefficient values
      i = T1ReadLong(ram, addr);
      parameter->KAst = (unsigned)(i & 0xFFFFFFC0);
      addr += 4;

      i = T1ReadLong(ram, addr);
      parameter->deltaKAst = (signed) ((i & 0x03FFFFC0) | (i & 0x02000000 ? 0xFE000000 : 0x00000000));
      addr += 4;     

      i = T1ReadLong(ram, addr);
      parameter->deltaKAx = (signed) ((i & 0x03FFFFC0) | (i & 0x02000000 ? 0xFE000000 : 0x00000000));
      addr += 4;

      if (which == 0)
      {
         parameter->coefdatasize = (regs->KTCTL & 0x2 ? 2 : 4);
         parameter->coeftbladdr = ((regs->KTAOF & 0x7) * 0x10000 + touint(parameter->KAst)) * parameter->coefdatasize;
         parameter->coefmode = (regs->KTCTL >> 2) & 0x3;
      }
      else
      {
         parameter->coefdatasize = (regs->KTCTL & 0x200 ? 2 : 4);
         parameter->coeftbladdr = (((regs->KTAOF >> 8) & 0x7) * 0x10000 + touint(parameter->KAst)) * parameter->coefdatasize;
         parameter->coefmode = (regs->KTCTL >> 10) & 0x3;
      }
   }

   VDP2LOG("Xst: %f, Yst: %f, Zst: %f, deltaXst: %f deltaYst: %f deltaX: %f\n"
       "deltaY: %f A: %f B: %f C: %f D: %f E: %f F: %f Px: %f Py: %f Pz: %f\n"
       "Cx: %f Cy: %f Cz: %f Mx: %f My: %f kx: %f ky: %f KAst: %f\n"
       "deltaKAst: %f deltaKAx: %f\n", 
       tofloat(parameter->Xst), tofloat(parameter->Yst),
       tofloat(parameter->Zst), tofloat(parameter->deltaXst),
       tofloat(parameter->deltaYst), tofloat(parameter->deltaX),
       tofloat(parameter->deltaY), tofloat(parameter->A),
       tofloat(parameter->B), tofloat(parameter->C), tofloat(parameter->D),
       tofloat(parameter->E), tofloat(parameter->F), tofloat(parameter->Px),
       tofloat(parameter->Py), tofloat(parameter->Pz), tofloat(parameter->Cx),
       tofloat(parameter->Cy), tofloat(parameter->Cz), tofloat(parameter->Mx),
       tofloat(parameter->My), tofloat(parameter->kx),
       tofloat(parameter->ky), tofloat(parameter->KAst),
       tofloat(parameter->deltaKAst), tofloat(parameter->deltaKAx));
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ParameterAPlaneAddr(vdp2draw_struct *info, int i, Vdp2* regs)
{
   u32 offset = (regs->MPOFR & 0x7) << 6;
   u32 tmp=0;

   switch(i)
   {
      case 0:
         tmp = offset | (regs->MPABRA & 0xFF);
         break;
      case 1:
         tmp = offset | (regs->MPABRA >> 8);
         break;
      case 2:
         tmp = offset | (regs->MPCDRA & 0xFF);
         break;
      case 3:
         tmp = offset | (regs->MPCDRA >> 8);
         break;
      case 4:
         tmp = offset | (regs->MPEFRA & 0xFF);
         break;
      case 5:
         tmp = offset | (regs->MPEFRA >> 8);
         break;
      case 6:
         tmp = offset | (regs->MPGHRA & 0xFF);
         break;
      case 7:
         tmp = offset | (regs->MPGHRA >> 8);
         break;
      case 8:
         tmp = offset | (regs->MPIJRA & 0xFF);
         break;
      case 9:
         tmp = offset | (regs->MPIJRA >> 8);
         break;
      case 10:
         tmp = offset | (regs->MPKLRA & 0xFF);
         break;
      case 11:
         tmp = offset | (regs->MPKLRA >> 8);
         break;
      case 12:
         tmp = offset | (regs->MPMNRA & 0xFF);
         break;
      case 13:
         tmp = offset | (regs->MPMNRA >> 8);
         break;
      case 14:
         tmp = offset | (regs->MPOPRA & 0xFF);
         break;
      case 15:
         tmp = offset | (regs->MPOPRA >> 8);
         break;
   }

   CalcPlaneAddr(info, tmp);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ParameterBPlaneAddr(vdp2draw_struct *info, int i, Vdp2* regs)
{
   u32 offset = (regs->MPOFR & 0x70) << 2;
   u32 tmp=0;

   // Parameter B
   switch(i)
   {
      case 0:
         tmp = offset | (regs->MPABRB & 0xFF);
         break;
      case 1:
         tmp = offset | (regs->MPABRB >> 8);
         break;
      case 2:
         tmp = offset | (regs->MPCDRB & 0xFF);
         break;
      case 3:
         tmp = offset | (regs->MPCDRB >> 8);
         break;
      case 4:
         tmp = offset | (regs->MPEFRB & 0xFF);
         break;
      case 5:
         tmp = offset | (regs->MPEFRB >> 8);
         break;
      case 6:
         tmp = offset | (regs->MPGHRB & 0xFF);
         break;
      case 7:
         tmp = offset | (regs->MPGHRB >> 8);
         break;
      case 8:
         tmp = offset | (regs->MPIJRB & 0xFF);
         break;
      case 9:
         tmp = offset | (regs->MPIJRB >> 8);
         break;
      case 10:
         tmp = offset | (regs->MPKLRB & 0xFF);
         break;
      case 11:
         tmp = offset | (regs->MPKLRB >> 8);
         break;
      case 12:
         tmp = offset | (regs->MPMNRB & 0xFF);
         break;
      case 13:
         tmp = offset | (regs->MPMNRB >> 8);
         break;
      case 14:
         tmp = offset | (regs->MPOPRB & 0xFF);
         break;
      case 15:
         tmp = offset | (regs->MPOPRB >> 8);
         break;
   }

   CalcPlaneAddr(info, tmp);
}

//////////////////////////////////////////////////////////////////////////////

float Vdp2ReadCoefficientMode0_2(vdp2rotationparameter_struct *parameter, u32 addr, u8 * ram)
{
   s32 i;

   if (parameter->coefdatasize == 2)
   {
      addr &= 0x7FFFE;
      i = T1ReadWord(ram, addr);
      parameter->msb = (i >> 15) & 0x1;
      return (float) (signed) ((i & 0x7FFF) | (i & 0x4000 ? 0xFFFFC000 : 0x00000000)) / 1024;
   }
   else
   {
      addr &= 0x7FFFC;
      i = T1ReadLong(ram, addr);
      parameter->msb = (i >> 31) & 0x1;
      return (float) (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000)) / 65536;
   }
}

//////////////////////////////////////////////////////////////////////////////

fixed32 Vdp2ReadCoefficientMode0_2FP(vdp2rotationparameterfp_struct *parameter, u32 addr, u8* ram)
{
   s32 i;

   if (parameter->coefdatasize == 2)
   {
      addr &= 0x7FFFE;
      i = T1ReadWord(ram, addr);
      parameter->msb = (i >> 15) & 0x1;
      return (signed) ((i & 0x7FFF) | (i & 0x4000 ? 0xFFFFC000 : 0x00000000)) * 64;
   }
   else
   {
      addr &= 0x7FFFC;
      i = T1ReadLong(ram, addr);
      parameter->linescreen = (i >> 24) & 0x7F;
      parameter->msb = (i >> 31) & 0x1;
      return (signed) ((i & 0x00FFFFFF) | (i & 0x00800000 ? 0xFF800000 : 0x00000000));
   }
}

//////////////////////////////////////////////////////////////////////////////

