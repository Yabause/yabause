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


#ifndef VDP1_INTERFACEH
#define VDP1_INTERFACEH

#include "threads.h"
#include "sh2core.h"

typedef struct {
  int cmd;
  void* msg;
}vdp1Command_struct;

#define NB_VDP1_MSG 704*512

enum {
  VDP1FB_WRITE = 0,
  VDP1_VBLANKOUT,
  VDP1_HBLANKOUT,
  VDP1_HBLANKIN
};

extern YabEventQueue *vdp1_q;

extern void Vdp1FrameBufferWriteByte(SH2_struct *context, u8*, u32, u8);
extern void Vdp1FrameBufferWriteWord(SH2_struct *context, u8*, u32, u16);
extern void Vdp1FrameBufferWriteLong(SH2_struct *context, u8*, u32, u32);

extern void Vdp1HBlankIN(void);
extern void Vdp1HBlankOUT(void);
extern void Vdp1VBlankIN(void);
extern void Vdp1VBlankOUT(void);

#endif