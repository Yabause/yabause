/*  Copyright 2020 Francois Caron

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
#include "vdp1_interface.h"
#include <stdlib.h>


YabEventQueue *vdp1_q;

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1FrameBufferWriteByte(SH2_struct *context, u8* mem, u32 addr, u8 val) {
   addr &= 0x7FFFF;
   vdp1Command_struct *p = (vdp1Command_struct*)malloc(sizeof(vdp1Command_struct));
   p->cmd = VDP1FB_WRITE;
   p->msg = malloc(3*sizeof(int));
   ((int*)(p->msg))[0]=0; //Byte
   ((int*)(p->msg))[1]=addr; //Addr
   ((int*)(p->msg))[2]=val; //Val
   YabAddEventQueue(vdp1_q,p);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1FrameBufferWriteWord(SH2_struct *context, u8* mem, u32 addr, u16 val) {
  addr &= 0x7FFFF;
  vdp1Command_struct *p = (vdp1Command_struct*)malloc(sizeof(vdp1Command_struct));
  p->cmd = VDP1FB_WRITE;
  p->msg = malloc(3*sizeof(int));
  ((int*)(p->msg))[0]=1; //Byte
  ((int*)(p->msg))[1]=addr; //Addr
  ((int*)(p->msg))[2]=val; //Val
  YabAddEventQueue(vdp1_q,p);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp1FrameBufferWriteLong(SH2_struct *context, u8* mem, u32 addr, u32 val) {
  addr &= 0x7FFFF;
  vdp1Command_struct *p = (vdp1Command_struct*)malloc(sizeof(vdp1Command_struct));
  p->cmd = VDP1FB_WRITE;
  p->msg = malloc(3*sizeof(int));
  ((int*)(p->msg))[0]=2; //Byte
  ((int*)(p->msg))[1]=addr; //Addr
  ((int*)(p->msg))[2]=val; //Val
  YabAddEventQueue(vdp1_q,p);
}

/////////////////////////////////////////////////////////////////////////////

void Vdp1HBlankIN(void)
{
  vdp1Command_struct *p = (vdp1Command_struct*)malloc(sizeof(vdp1Command_struct));
  p->cmd = VDP1_HBLANKIN;
  p->msg = NULL;
  YabAddEventQueue(vdp1_q,p);
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1HBlankOUT(void)
{
  vdp1Command_struct *p = (vdp1Command_struct*)malloc(sizeof(vdp1Command_struct));
  p->cmd = VDP1_HBLANKOUT;
  p->msg = NULL;
  YabAddEventQueue(vdp1_q,p);
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1VBlankIN(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void Vdp1VBlankOUT(void)
{
  vdp1Command_struct *p = (vdp1Command_struct*)malloc(sizeof(vdp1Command_struct));
  p->cmd = VDP1_VBLANKOUT;
  p->msg = NULL;
  YabAddEventQueue(vdp1_q,p);
}
