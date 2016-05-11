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

//#define YGR_SH1_RW_DEBUG
#ifdef YGR_SH1_RW_DEBUG
#define YGR_SH1_RW_LOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define YGR_SH1_RW_LOG(...)
#endif

// ygr connections
// ygr <=> cd signal processor (on cd drive board)
// ygr <=> sh1 (aka sh7034)
// ygr <=> a-bus (sh2 interface)
// ygr <=> vdp2 (mpeg video)
// ygr <=> scsp (cd audio)

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

   int fifo_ptr;
   u16 fifo[4];
   u16 transfer_ctrl;
}ygr_cxt;

u8 ygr_sh1_read_byte(u32 addr)
{
   CDTRACE("rblsi: %08X\n", addr);
   YGR_SH1_RW_LOG("ygr_sh1_read_byte 0x%08x", addr );
   return 0;
}

u16 ygr_sh1_read_word(u32 addr)
{
   CDTRACE("rwlsi: %08X\n", addr);
   switch (addr & 0xffff) {
   case 0:
      ygr_cxt.fifo_ptr++;
      ygr_cxt.fifo_ptr &= 3;
      return ygr_cxt.fifo[ygr_cxt.fifo_ptr];
   case 2:
      return ygr_cxt.transfer_ctrl;
   case 0x6:
      return ygr_cxt.regs.HIRQ;
   case 8:
      return ygr_cxt.regs.UNKNOWN;
   case 0xa:
      return ygr_cxt.regs.HIRQMASK;
   case 0x10: // CR1
      return ygr_cxt.regs.CR1;
   case 0x12: // CR2
      return ygr_cxt.regs.CR2;
   case 0x14: // CR3
      return ygr_cxt.regs.CR3;
   case 0x16: // CR4
      return ygr_cxt.regs.CR4;
   }
   YGR_SH1_RW_LOG("ygr_sh1_read_word 0x%08x", addr);
   return 0;
}

u32 ygr_sh1_read_long(u32 addr)
{
   CDTRACE("rllsi: %08X\n", addr);
   YGR_SH1_RW_LOG("ygr_sh1_read_long 0x%08x", addr);
   return 0;
}

void ygr_sh1_write_byte(u32 addr,u8 data)
{
   CDTRACE("wblsi: %08X %02X\n", addr, data);
   YGR_SH1_RW_LOG("ygr_sh1_write_byte 0x%08x 0x%02x", addr, data);
}

void ygr_sh1_write_word(u32 addr, u16 data)
{
   CDTRACE("wwlsi: %08X %04X\n", addr, data);
   switch (addr & 0xffff) {
   case 0x6:
      ygr_cxt.regs.HIRQ = data;
      return;
   case 8:
      ygr_cxt.regs.UNKNOWN = data & 3;
      return;
   case 0xa:
      ygr_cxt.regs.HIRQMASK = data & 0x70;
      return;
   case 0x10: // CR1
      ygr_cxt.regs.CR1 = data;
      return;
   case 0x12: // CR2
      ygr_cxt.regs.CR2 = data;
      return;
   case 0x14: // CR3
      ygr_cxt.regs.CR3 = data;
      return;
   case 0x16: // CR4
      ygr_cxt.regs.CR4 = data;
      return;
   }
   YGR_SH1_RW_LOG("ygr_sh1_write_word 0x%08x 0x%04x", addr, data);
}

void ygr_sh1_write_long(u32 addr, u32 data)
{
   CDTRACE("wblsi: %08X %08X\n", addr, data);
   YGR_SH1_RW_LOG("ygr_sh1_write_long 0x%08x 0x%08x", addr, data);
}

//////////////////////////////////////////////////////////////////////////////

//replacements for Cs2ReadWord etc
u16 FASTCALL ygr_a_bus_read_word(SH2_struct * sh, u32 addr) {
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

void FASTCALL ygr_a_bus_write_word(SH2_struct * sh, u32 addr, u16 val) {
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

u32 FASTCALL ygr_a_bus_read_long(SH2_struct * sh, u32 addr) {
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

void FASTCALL ygr_a_bus_write_long(SH2_struct * sh, UNUSED u32 addr, UNUSED u32 val) {
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
