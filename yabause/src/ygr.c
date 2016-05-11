/*  Copyright 2014-2016 James Laird-Wah
    Copyright 2004-2006, 2013 Theo Berkau

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

/*! \file ygr.c
    \brief YGR implementation (CD block LSI)
*/

#include "core.h"
#include "sh7034.h"
#include "assert.h"
#include "memory.h"
#include "debug.h"

struct Ygr
{
   struct Regs
   {
      u32 DTR;
      u16 UNKNOWN;
      u16 HIRQ;
      u16 HIRQMASK; // Masks bits from HIRQ -only- when generating A-bus interrupts
      u16 CR1;
      u16 CR2;
      u16 CR3;
      u16 CR4;
      u16 MPEGRGB;
   }regs;
}ygr_cxt;

u8 ygr_sh1_read_byte(u32 addr)
{
   return 0;
}

u16 ygr_sh1_read_word(u32 addr)
{
   return 0;
}

u32 ygr_sh1_read_long(u32 addr)
{
   return 0;
}

void ygr_sh1_write_byte(u32 addr,u8 data)
{

}

void ygr_sh1_write_word(u32 addr, u16 data)
{

}

void ygr_sh1_write_long(u32 addr, u32 data)
{

}

//////////////////////////////////////////////////////////////////////////////

//replacements for Cs2ReadWord etc
u16 FASTCALL ygr_a_bus_read_word(u32 addr) {
   u16 val = 0;
   addr &= 0xFFFFF; // fix me(I should really have proper mapping)

   switch (addr) {
   case 0x90008:
   case 0x9000A:
      return ygr_cxt.regs.HIRQ;
   case 0x9000C:
   case 0x9000E: 
      return ygr_cxt.regs.HIRQMASK;
   case 0x90018:
   case 0x9001A: 
      return ygr_cxt.regs.CR1;
   case 0x9001C:
   case 0x9001E: 
      return ygr_cxt.regs.CR2;
   case 0x90020:
   case 0x90022: 
      return ygr_cxt.regs.CR3;
   case 0x90024:
   case 0x90026: 
      return ygr_cxt.regs.CR4;
   case 0x90028:
   case 0x9002A: 
      return ygr_cxt.regs.MPEGRGB;
   case 0x98000:
      // transfer info
      break;
   default:
      LOG("ygr\t: Undocumented register read %08X\n", addr);
      break;
   }

   return val;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ygr_a_bus_write_word(u32 addr, u16 val) {
   addr &= 0xFFFFF; // fix me(I should really have proper mapping)

   switch (addr) {
   case 0x90008:
   case 0x9000A:
      ygr_cxt.regs.HIRQ &= val;
      return;
   case 0x9000C:
   case 0x9000E: 
      ygr_cxt.regs.HIRQMASK = val;
      return;
   case 0x90018:
   case 0x9001A: 
      ygr_cxt.regs.CR1 = val;
      return;
   case 0x9001C:
   case 0x9001E: 
      ygr_cxt.regs.CR2 = val;
      return;
   case 0x90020:
   case 0x90022: 
      ygr_cxt.regs.CR3 = val;
      return;
   case 0x90024:
   case 0x90026: 
      ygr_cxt.regs.CR4 = val;
      return;
   case 0x90028:
   case 0x9002A: 
      ygr_cxt.regs.MPEGRGB = val;
      return;
   default:
      LOG("ygr\t:Undocumented register write %08X\n", addr);
      break;
   }
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL ygr_a_bus_read_long(u32 addr) {
   u32 val = 0;
   addr &= 0xFFFFF; // fix me(I should really have proper mapping)

   switch (addr) {
   case 0x90008:
      //2 copies?
      return ((ygr_cxt.regs.HIRQ << 16) | ygr_cxt.regs.HIRQ);
   case 0x9000C: 
      return ((ygr_cxt.regs.HIRQMASK << 16) | ygr_cxt.regs.HIRQMASK);
   case 0x90018: 
      return ((ygr_cxt.regs.CR1 << 16) | ygr_cxt.regs.CR1);
   case 0x9001C: 
      return ((ygr_cxt.regs.CR2 << 16) | ygr_cxt.regs.CR2);
   case 0x90020: 
      return ((ygr_cxt.regs.CR3 << 16) | ygr_cxt.regs.CR3);
   case 0x90024: 
      return ((ygr_cxt.regs.CR4 << 16) | ygr_cxt.regs.CR4);
   case 0x90028:
      return ((ygr_cxt.regs.MPEGRGB << 16) | ygr_cxt.regs.MPEGRGB);
   case 0x18000:
      // transfer data
      break;
   default:
      LOG("ygr\t: Undocumented register read %08X\n", addr);
      break;
   }

   return val;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ygr_a_bus_write_long(UNUSED u32 addr, UNUSED u32 val) {
   addr &= 0xFFFFF; // fix me(I should really have proper mapping)

   switch (addr)
   {
   case 0x18000:
      // transfer data
      break;
   default:
      LOG("ygr\t: Undocumented register write %08X\n", addr);
      //         T3WriteLong(Cs2Area->mem, addr, val);
      break;
   }
}
