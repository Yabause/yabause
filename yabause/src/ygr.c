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
#include <stdarg.h>
#include "tsunami/yab_tsunami.h"
#include "mpeg_card.h"
#ifdef WIN32
#include "windows.h"
#endif

void Cs2Exec(u32 timing);

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


#define FIFO_SIZE 4096
#define FIFO_MASK (FIFO_SIZE-1)

struct Ygr
{
   struct Regs
   {
      u32 DTR;
      u16 UNKNOWN;
      u16 HIRQ;
      u16 HIRQMASK; // Masks bits from HIRQ -only- when generating A-bus interrupts

      //command regs
      //a-bus reads RR, writes to CR
      u16 CR1;
      u16 CR2;
      u16 CR3;
      u16 CR4;

      //response regs
      //sh1-bus writes to RR, reads CR
      u16 RR1;
      u16 RR2;
      u16 RR3;
      u16 RR4;

      u16 MPEGRGB;
   }regs;

   int fifo_ptr;
   u16 fifo[FIFO_SIZE];
   u16 transfer_ctrl;

   int fifo_read_ptr;
   int fifo_write_ptr;
   int fifo_num_stored;

   u16 cdirq_flags;

   int mbx_status;
   u16 fake_fifo;
   u16 reg_0c;
   u16 reg_1a;
   u16 reg_1c;
}ygr_cxt = { 0 };



u16 read_fifo()
{
   int ptr = ygr_cxt.fifo_read_ptr;

   if(ygr_cxt.transfer_ctrl & 4)
   {
      assert(ygr_cxt.fifo_num_stored > 0);

      if (ygr_cxt.fifo_num_stored <= 0)
      {
#ifdef WIN32
         DebugBreak();
#endif
      }
   }

   ygr_cxt.fifo_read_ptr++;
   ygr_cxt.fifo_read_ptr &= FIFO_MASK;

   ygr_cxt.fifo_num_stored--;

   if (ygr_cxt.fifo_num_stored < 0)
      ygr_cxt.fifo_num_stored = 0;

   return ygr_cxt.fifo[ptr];
}

void write_fifo(u16 data)
{

   if (ygr_cxt.fifo_num_stored == FIFO_SIZE)
      assert(0);

   ygr_cxt.fifo[ygr_cxt.fifo_write_ptr++] = data;
   ygr_cxt.fifo_write_ptr &= FIFO_MASK;
   ygr_cxt.fifo_num_stored++;
}

int fifo_full()
{
   if (ygr_cxt.fifo_num_stored == FIFO_SIZE)
      tsunami_log_value("FIFO_FULL", 1, 1);
   else
      tsunami_log_value("FIFO_FULL", 0, 1);

   return ygr_cxt.fifo_num_stored == FIFO_SIZE;
}


int fifo_empty()
{
   return ygr_cxt.fifo_num_stored == 0;
}

//#define WRITE_FIFO_LOG
//#define VERIFY_FIFO_LOG

void make_fifo_log(u32 data)
{
#if defined WRITE_FIFO_LOG && !defined(VERIFY_FIFO_LOG)
   static FILE * fp;
   static int started = 0;
   static int count = 0;

   if (!started)
   {
      fp = fopen("C:/yabause/fifo_log.txt", "w");
      started = 1;
   }

   fprintf(fp, "%08X, %d\n", data, count);
   count++;
#endif
}
static int ygr_count = 0;
void verify_fifo_log(u32 data_in)
{
#if defined VERIFY_FIFO_LOG && !defined(WRITE_FIFO_LOG)
   static FILE * fp;
   static int started = 0;
   u32 correct_data = 0;
   int correct_count = 0;
   int retval = 0;
   if (!started)
   {
      fp = fopen("C:/yabause/fifo_log.txt", "r");
      started = 1;
   }

   retval = fscanf(fp, "%08X, %d\n", &correct_data, &correct_count);

   if (retval == 2)
   {
      assert(data_in == correct_data);
      assert(ygr_count == correct_count);
   }
   ygr_count++;
#endif

}

int ygr_dreq_asserted()
{
   if (fifo_empty() && ((ygr_cxt.transfer_ctrl & 5) == 5))//put / get sector
   {
      return 0;
   }
   else if (fifo_full())
   {
      tsunami_log_value("DREQ", 0, 1);
      return 0;
   }
   else
   {
      int dreq = (ygr_cxt.transfer_ctrl >> 2) & 1;
      tsunami_log_value("DREQ", dreq, 1);
      return dreq;
   }
}


int sh2_a_bus_check_wait(u32 addr, int size)
{
   if (((addr & 0x7FFF) <= 0xFFF) && ((addr &= 0x3F) <= 2))
   {
      //if something is in the fifo, no wait
      if (size == 1 && ygr_cxt.fifo_num_stored)//word size
         return 0;
      else if (size == 2 && ygr_cxt.fifo_num_stored >= 2)//long size
         return 0;
      else
      {
         if ((ygr_cxt.transfer_ctrl >> 2) & 1)
            return 1;//more data expected
         else
            return 0;
      }
   }

   return 0;
}


u8 ygr_sh1_read_byte(u32 addr)
{
   assert(0);
   CDTRACE("rblsi: %08X\n", addr);
   YGR_SH1_RW_LOG("ygr_sh1_read_byte 0x%08x", addr );
   return 0;
}

u16 ygr_sh1_read_word(u32 addr)
{
   if ((addr & 0xf00000) == 0x100000)
   {
      return mpeg_card_read_word(addr);
   }
   CDTRACE("rwlsi: %08X\n", addr);
   switch (addr & 0xffff) {
   case 0:
   {
      u16 val = read_fifo();
      tsunami_log_value("SH1_R_FIFO", val, 16);
      return val;
   }
   case 2:
      return ygr_cxt.transfer_ctrl;
   case 4:
      return ygr_cxt.mbx_status;
   case 0x6:
      return ygr_cxt.cdirq_flags;
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
   case 0x1a:
      return ygr_cxt.reg_1a;
   case 0x1c:
      return ygr_cxt.reg_1c;
   }
   YGR_SH1_RW_LOG("ygr_sh1_read_word 0x%08x", addr);
   assert(0);
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
   assert(0);
   CDTRACE("wblsi: %08X %02X\n", addr, data);
   YGR_SH1_RW_LOG("ygr_sh1_write_byte 0x%08x 0x%02x", addr, data);
}

void ygr_sh1_write_word(u32 addr, u16 data)
{
   if ((addr & 0xf00000) == 0x100000)
   {
      mpeg_card_write_word(addr, data);
      return;
   }
   CDTRACE("wwlsi: %08X %04X\n", addr, data);
   switch (addr & 0xffff) {
   case 0:
      tsunami_log_value("SH1_W_FIFO", data, 16);
      write_fifo(data);
      return;
   case 2:
      if (data & 2)
      {
         ygr_cxt.fifo_read_ptr = 0;
         ygr_cxt.fifo_write_ptr = 0;
         ygr_cxt.fifo_num_stored = 0;
      }
      ygr_cxt.transfer_ctrl = data;
      return;
   case 4:
      ygr_cxt.mbx_status = data;
      return;
   case 0x6:
      ygr_cxt.cdirq_flags = data;
      return;
   case 8:
      ygr_cxt.regs.UNKNOWN = data & 3;
      return;
   case 0xa:
      ygr_cxt.regs.HIRQMASK = data & 0x70;
      return;
   case 0xc:
      ygr_cxt.reg_0c = data;
      return;
   case 0x10: // CR1
      ygr_cxt.regs.RR1 = data;
      return;
   case 0x12: // CR2
      ygr_cxt.regs.RR2 = data;
      return;
   case 0x14: // CR3
      ygr_cxt.regs.RR3 = data;
      return;
   case 0x16: // CR4
      ygr_cxt.regs.RR4 = data;
      return;
   case 0x1e:
      ygr_cxt.regs.HIRQ |= data;
      return;
   case 0x1a:
      ygr_cxt.reg_1a = data;
      return;
   }
   assert(0);
   YGR_SH1_RW_LOG("ygr_sh1_write_word 0x%08x 0x%04x", addr, data);
}

void ygr_sh1_write_long(u32 addr, u32 data)
{
   CDTRACE("wblsi: %08X %08X\n", addr, data);
   YGR_SH1_RW_LOG("ygr_sh1_write_long 0x%08x 0x%08x", addr, data);
}

//////////////////////////////////////////////////////////////////////////////

void lle_trace_log(const char * format, ...)
{
   static int started = 0;
   static FILE* fp = NULL;
   va_list l;

   if (!started)
   {
      fp = fopen("C:/yabause/lle_log.txt", "w");

      if (!fp)
      {
         return;
      }
      started = 1;
   }

   va_start(l, format);
   vfprintf(fp, format, l);
   va_end(l);
}

//#define WANT_LLE_TRACE

#ifdef WANT_LLE_TRACE
#define LLECDLOG(...) lle_trace_log(__VA_ARGS__)
#else
#define LLECDLOG(...)
#endif

void ygr_a_bus_cd_cmd_log(void);


char* get_status(u16 cr1)
{
   switch ((cr1 >> 8) & 0xf)
   {
   case 0:
      return "busy";
   case 1:
      return "paused";
   case 2:
      return "standby";
   case 3:
      return "playing";
   case 4:
      return "seeking";
   case 5:
      return "scanning";
   case 7:
      return "tray open";
   case 8:
      return "retrying";
   case 9:
      return "read data error";
   case 0xa:
      return "fatal error";
   default:
      return "unknown";
   }

   return "get status error";
}

//replacements for Cs2ReadWord etc
u16 FASTCALL ygr_a_bus_read_word(u32 addr) {
   u16 val = 0;

   if ((addr & 0x7FFF) <= 0xFFF)
   {
      addr &= 0x3F;

      switch (addr) {
      case 0x00:
      case 0x02:
         // transfer info
      {
         u16 val = read_fifo();
         verify_fifo_log(val);
         tsunami_log_value("SH2_R_FIFO", val, 16);
         return val;
      }
      break;
      case 0x08:
      case 0x0A:
         LLECDLOG("Cs2ReadWord %08X %04X\n", addr, ygr_cxt.regs.HIRQ);
         return ygr_cxt.regs.HIRQ;
      case 0x0C:
      case 0x0E:
         LLECDLOG("Cs2ReadWord %08X %04X\n", addr, ygr_cxt.regs.HIRQMASK);
         return ygr_cxt.regs.HIRQMASK;
      case 0x18:
      case 0x1A:
         LLECDLOG("Cs2ReadWord %08X %04X\n", addr, ygr_cxt.regs.RR1);
         return ygr_cxt.regs.RR1;
      case 0x1C:
      case 0x1E:
         LLECDLOG("Cs2ReadWord %08X %04X\n", addr, ygr_cxt.regs.RR2);
         return ygr_cxt.regs.RR2;
      case 0x20:
      case 0x22:
         LLECDLOG("Cs2ReadWord %08X %04X\n", addr, ygr_cxt.regs.RR3);
         return ygr_cxt.regs.RR3;
      case 0x24:
      case 0x26:
         LLECDLOG("Cs2ReadWord %08X %04X\n", addr, ygr_cxt.regs.RR4);
         ygr_cxt.mbx_status |= 2;
         CDLOG("abus cdb response: CR1: %04x CR2: %04x CR3: %04x CR4: %04x HIRQ: %04x Status: %s\n", ygr_cxt.regs.RR1, ygr_cxt.regs.RR2, ygr_cxt.regs.RR3, ygr_cxt.regs.RR4, ygr_cxt.regs.HIRQ, get_status(ygr_cxt.regs.RR1));
         return ygr_cxt.regs.RR4;
      case 0x28:
      case 0x2A:
         return ygr_cxt.regs.MPEGRGB;
      default:
         LOG("ygr\t: Undocumented register read %08X\n", addr);
         break;
      }
   }
   assert(0);
   return val;
}

//////////////////////////////////////////////////////////////////////////////
#ifdef CDDEBUG
void ygr_a_bus_cd_cmd_log(void) 
{
	u16 instruction=ygr_cxt.regs.CR1 >> 8;

   switch (instruction) 
   {
      case 0x00:
         CDLOG("abus cdb command: Get Status %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x01:
         CDLOG("abus cdb command: Get Hardware Info %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x02:
         CDLOG("abus cdb command: Get TOC %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x03:
         CDLOG("abus cdb command: Get Session Info %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x04:
         CDLOG("abus cdb command: Initialize CD System %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x05:
         CDLOG("abus cdb command: Open Tray %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x06:
         CDLOG("abus cdb command: End Data Transfer %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x10:
         CDLOG("abus cdb command: Play Disc %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x11:
         CDLOG("abus cdb command: Seek Disc %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x12:
         CDLOG("abus cdb command: Scan Disc %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x20:
         CDLOG("abus cdb command: Get Subcode QRW %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x30:
         CDLOG("abus cdb command: Set CD Device Connection %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x31:
         CDLOG("abus cdb command: Get CD Device Connection %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x32:
         CDLOG("abus cdb command: Get Last Buffer Destination %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x40:
         CDLOG("abus cdb command: Set Filter Range %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x41:
         CDLOG("abus cdb command: Get Filter Range %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x42:
         CDLOG("abus cdb command: Set Filter Subheader Conditions %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x43:
         CDLOG("abus cdb command: Get Filter Subheader Conditions %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x44:
         CDLOG("abus cdb command: Set Filter Mode %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x45:
         CDLOG("abus cdb command: Get Filter Mode %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x46:
         CDLOG("abus cdb command: Set Filter Connection %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x47:
         CDLOG("abus cdb command: Get Filter Connection %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x48:
         CDLOG("abus cdb command: Reset Selector %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x50:
         CDLOG("abus cdb command: Get Buffer Size %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x51:
         CDLOG("abus cdb command: Get Sector Number %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x52:
         CDLOG("abus cdb command: Calculate Actual Size %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x53:
         CDLOG("abus cdb command: Get Actual Size %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x54:
         CDLOG("abus cdb command: Get Sector Info %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x55:
         CDLOG("abus cdb command: Execute FAD Search %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x56:
         CDLOG("abus cdb command: Get FAD Search Results %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x60:
         CDLOG("abus cdb command: Set Sector Length %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x61:
         CDLOG("abus cdb command: Get Sector Data %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x62:
         CDLOG("abus cdb command: Delete Sector Data %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x63:
         CDLOG("abus cdb command: Get Then Delete Sector Data %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x64:
         CDLOG("abus cdb command: Put Sector Data %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x65:
         CDLOG("abus cdb command: Copy Sector Data %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x66:
         CDLOG("abus cdb command: Move Sector Data %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x67:
         CDLOG("abus cdb command: Get Copy Error %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x70:
         CDLOG("abus cdb command: Change Directory %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x71:
         CDLOG("abus cdb command: Read Directory %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x72:
         CDLOG("abus cdb command: Get File System Scope %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x73:
         CDLOG("abus cdb command: Get File Info %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x74:
         CDLOG("abus cdb command: Read File %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x75:
         CDLOG("abus cdb command: Abort File %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x90:
         CDLOG("abus cdb command: MPEG Get Status %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x91:
         CDLOG("abus cdb command: MPEG Get Interrupt %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x92:
         CDLOG("abus cdb command: MPEG Set Interrupt Mask %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2);
         break;
      case 0x93: 
         CDLOG("abus cdb command: MPEG Init %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2);
         break;
      case 0x94:
         CDLOG("abus cdb command: MPEG Set Mode %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3);
         break;
      case 0x95:
         CDLOG("abus cdb command: MPEG Play %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR4);
         break;
      case 0x96:
         CDLOG("abus cdb command: MPEG Set Decoding Method %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR4);
         break;
		case 0x97:
			CDLOG("abus cdb command: MPEG Out Decoding Sync %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR4);
			break;
		case 0x98:
			CDLOG("abus cdb command: MPEG Get Timecode %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR4);
			break;
		case 0x99:
			CDLOG("abus cdb command: MPEG Get PTS %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR4);
			break;
      case 0x9A:
         CDLOG("abus cdb command: MPEG Set Connection %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x9B:
         CDLOG("abus cdb command: MPEG Get Connection %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
		case 0x9C:
			CDLOG("abus cdb command: MPEG Change Connection %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
			break;
      case 0x9D:
         CDLOG("abus cdb command: MPEG Set Stream %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0x9E:
         CDLOG("abus cdb command: MPEG Get Stream %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
		case 0x9F:
			CDLOG("abus cdb command: MPEG Get Picture Size %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
			break;
      case 0xA0:
         CDLOG("abus cdb command: MPEG Display %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xA1:
         CDLOG("abus cdb command: MPEG Set Window %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xA2:
         CDLOG("abus cdb command: MPEG Set Border Color %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xA3:
         CDLOG("abus cdb command: MPEG Set Fade %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xA4:
         CDLOG("abus cdb command: MPEG Set Video Effects %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xA5:
         CDLOG("abus cdb command: MPEG Get Image %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xA6:
         CDLOG("abus cdb command: MPEG Set Image %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xA7:
         CDLOG("abus cdb command: MPEG Read Image %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xA8:
         CDLOG("abus cdb command: MPEG Write Image %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xA9:
         CDLOG("abus cdb command: MPEG Read Sector %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xAA:
         CDLOG("abus cdb command: MPEG Write Sector %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xAE:
         CDLOG("abus cdb command: MPEG Get LSI %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xAF:
         CDLOG("abus cdb command: MPEG Set LSI %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xE0:
         CDLOG("abus cdb command: Authenticate Device %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xE1:
         CDLOG("abus cdb command: Is Device Authenticated %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      case 0xE2:
         CDLOG("abus cdb command: Get MPEG ROM %04x %04x %04x %04x %04x\n", ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
      default:
         CDLOG("abus cdb command: Unknown Command(0x%02X) %04x %04x %04x %04x %04x\n", instruction, ygr_cxt.regs.HIRQ, ygr_cxt.regs.CR1, ygr_cxt.regs.CR2, ygr_cxt.regs.CR3, ygr_cxt.regs.CR4);
         break;
	}
}
#else
#define ygr_a_bus_cd_cmd_log() 
#endif

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ygr_a_bus_write_word(u32 addr, u16 val) {

   if ((addr & 0x7FFF) <= 0xFFF)
   {
      addr &= 0x3F;

      switch (addr) {
      case 0x00:
         if (ygr_cxt.transfer_ctrl & 1)
            write_fifo(val);
         break;
      case 0x08:
      case 0x0A:
         ygr_cxt.regs.HIRQ &= val;
         return;
      case 0x0C:
      case 0x0E:
         ygr_cxt.regs.HIRQMASK = val;
         return;
      case 0x18:
      case 0x1A:
         ygr_cxt.regs.CR1 = val;
         return;
      case 0x1C:
      case 0x1E:
         ygr_cxt.regs.CR2 = val;
         return;
      case 0x20:
      case 0x22:
         ygr_cxt.regs.CR3 = val;
         return;
      case 0x24:
      case 0x26:
         ygr_cxt.regs.CR4 = val;
         SH2SendInterrupt(SH1, 70, (sh1_cxt.onchip.intc.iprb >> 4) & 0xf);
         ygr_a_bus_cd_cmd_log();
         return;
      case 0x28:
      case 0x2A:
         ygr_cxt.regs.MPEGRGB = val;
         return;
      default:
         LOG("ygr\t:Undocumented register write %08X\n", addr);
         break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL ygr_a_bus_read_long(u32 addr) {
   u32 val = 0;

   if ((addr & 0x7FFF) <= 0xFFF)
   {
      addr &= 0x3F;

      switch (addr) {
      case 0x00:
      {
         u32 top;
         top = read_fifo();
         tsunami_log_value("SH2_R_FIFO", top & 0xffff, 16);
         top <<= 16;
         top |= read_fifo();
         tsunami_log_value("SH2_R_FIFO", top & 0xffff, 16);
         make_fifo_log(top);
         verify_fifo_log(top);
         return top;
      }
      case 0x08:
         return ((ygr_cxt.regs.HIRQ << 16) | ygr_cxt.regs.HIRQ);
      case 0x0C:
         return ((ygr_cxt.regs.HIRQMASK << 16) | ygr_cxt.regs.HIRQMASK);
      case 0x18:
         return ((ygr_cxt.regs.RR1 << 16) | ygr_cxt.regs.RR1);
      case 0x1C:
         return ((ygr_cxt.regs.RR2 << 16) | ygr_cxt.regs.RR2);
      case 0x20:
         return ((ygr_cxt.regs.RR3 << 16) | ygr_cxt.regs.RR3);
      case 0x24:
         ygr_cxt.mbx_status |= 2;
         return ((ygr_cxt.regs.RR4 << 16) | ygr_cxt.regs.RR4);
      case 0x28:
         return ((ygr_cxt.regs.MPEGRGB << 16) | ygr_cxt.regs.MPEGRGB);
      default:
         LOG("ygr\t: Undocumented register read %08X\n", addr);
         break;
      }
   }
   return val;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL ygr_a_bus_write_long(UNUSED u32 addr, UNUSED u32 val) {

   if ((addr & 0x7FFF) <= 0xFFF)
   {
      addr &= 0x3F;

      switch (addr)
      {
      case 0x00:
         if (ygr_cxt.transfer_ctrl & 1)
         {
            write_fifo(val >> 16);
            write_fifo(val);
         }
         break;
      case 0x0c:
         ygr_cxt.regs.HIRQMASK = val;//gets truncated
         break;
      case 0x18000:
         // transfer data
         break;
      default:
         LOG("ygr\t: Undocumented register write %08X\n", addr);
         //         T3WriteLong(Cs2Area->mem, addr, val);
         break;
      }
   }
}

u16 FASTCALL sh2_ygr_a_bus_read_word(SH2_struct * sh, u32 addr) {
   return ygr_a_bus_read_word(addr);
}

void FASTCALL sh2_ygr_a_bus_write_word(SH2_struct * sh, u32 addr, u16 val) {
   ygr_a_bus_write_word(addr, val);
}

u32 FASTCALL sh2_ygr_a_bus_read_long(SH2_struct * sh, u32 addr) {
   return ygr_a_bus_read_long(addr);
}

void FASTCALL sh2_ygr_a_bus_write_long(SH2_struct * sh, UNUSED u32 addr, UNUSED u32 val) {
   ygr_a_bus_write_long(addr, val);
}

void ygr_cd_irq(u8 flags)
{
   ygr_cxt.cdirq_flags |= flags;
   if (ygr_cxt.cdirq_flags & ygr_cxt.regs.HIRQMASK)
      SH2SendInterrupt(SH1, 71, sh1_cxt.onchip.intc.iprb & 0xf);
}
