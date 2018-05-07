/*  Copyright 2004-2005 Theo Berkau
    Copyright 2005 Guillaume Duhamel

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

#ifndef CS0_H
#define CS0_H

#include "memory.h"

#define CART_NONE               0
#define CART_PAR                1
#define CART_BACKUPRAM4MBIT     2
#define CART_BACKUPRAM8MBIT     3
#define CART_BACKUPRAM16MBIT    4
#define CART_BACKUPRAM32MBIT    5
#define CART_DRAM8MBIT          6
#define CART_DRAM32MBIT         7
#define CART_NETLINK            8
#define CART_ROM16MBIT          9
#define CART_JAPMODEM          10
#define CART_USBDEV            11
#define CART_ROMSTV            12

typedef struct
{
   int carttype;
   int cartid;
   const char *filename;

   u8 FASTCALL (*Cs0ReadByte)(SH2_struct *context, u8* memory, u32 addr);
   u16 FASTCALL (*Cs0ReadWord)(SH2_struct *context, u8* memory, u32 addr);
   u32 FASTCALL (*Cs0ReadLong)(SH2_struct *context, u8* memory, u32 addr);
   void FASTCALL (*Cs0WriteByte)(SH2_struct *context, u8* memory, u32 addr, u8 val);
   void FASTCALL (*Cs0WriteWord)(SH2_struct *context, u8* memory, u32 addr, u16 val);
   void FASTCALL (*Cs0WriteLong)(SH2_struct *context, u8* memory, u32 addr, u32 val);

   u8 FASTCALL (*Cs1ReadByte)(SH2_struct *context, u8* memory, u32 addr);
   u16 FASTCALL (*Cs1ReadWord)(SH2_struct *context, u8* memory, u32 addr);
   u32 FASTCALL (*Cs1ReadLong)(SH2_struct *context, u8* memory, u32 addr);
   void FASTCALL (*Cs1WriteByte)(SH2_struct *context, u8* memory, u32 addr, u8 val);
   void FASTCALL (*Cs1WriteWord)(SH2_struct *context, u8* memory, u32 addr, u16 val);
   void FASTCALL (*Cs1WriteLong)(SH2_struct *context, u8* memory, u32 addr, u32 val);

   u8 FASTCALL (*Cs2ReadByte)(SH2_struct *context, u8* memory, u32 addr);
   u16 FASTCALL (*Cs2ReadWord)(SH2_struct *context, u8* memory, u32 addr);
   u32 FASTCALL (*Cs2ReadLong)(SH2_struct *context, u8* memory, u32 addr);
   void FASTCALL (*Cs2WriteByte)(SH2_struct *context, u8* memory, u32 addr, u8 val);
   void FASTCALL (*Cs2WriteWord)(SH2_struct *context, u8* memory, u32 addr, u16 val);
   void FASTCALL (*Cs2WriteLong)(SH2_struct *context, u8* memory, u32 addr, u32 val);

   u8 *rom;
   u8 *bupram;
   u8 *dram;
} cartridge_struct;

extern cartridge_struct *CartridgeArea;

int CartInit(const char *filename, int);
void CartFlush(void);
void CartDeInit(void);

int CartSaveState(FILE *fp);
int CartLoadState(FILE *fp, int version, int size);

#endif
