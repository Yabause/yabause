/*  Copyright 2005 Guillaume Duhamel
Copyright 2016 Shinya Miyamoto

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

/*! \file sh2cache.c
\brief SH2 internal cache operations FIL0016332.PDF section 8
*/

#ifdef PSP 
# include <stdint.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>

#include "memory.h"
#include "yabause.h"
#include "sh2cache.h"
#include "sh2core.h"
#include "vdp2.h"
#include "vdp1.h"
#include "assert.h"
#include "scsp.h"
#include "scu.h"
#include "ygr.h"
#include "cs1.h"
#include "cs0.h"
#include "smpc.h"
#include "cs2.h"


#define AREA_MASK   (0xE0000000)
#define TAG_MASK   (0x1FFFFC00)
#define ENTRY_MASK (0x000003F0)
#define ENTRY_SHIFT (4)
#define LINE_MASK  (0x0000000F)

#define CACHE_USE ((0x00)<<29)
#define CACHE_THROUGH ((0x01)<<29)
#define CACHE_PURGE ((0x02)<<29)
#define CACHE_ADDRES_ARRAY ((0x03)<<29)
#define CACHE_DATA_ARRAY ((0x06)<<29)
#define CACHE_IO ((0x07)<<29)

void cache_clear(cache_enty * ca){
   int entry = 0;
	ca->enable = 0;

	for (entry = 0; entry < 64; entry++){
      int way = 0;
      ca->lru[entry] = 0;

      for (way = 0; way < 4; way++)
      {
         int i = 0;
         ca->way[way][entry].tag = 0;
         
         for (i = 0; i < 16; i++)
            ca->way[way][entry].data[i] = 0;
         ca->way[way][entry].v = 0;
      }
	}
	return;
}

void cache_enable(cache_enty * ca){
   //cache enable does not clear the cache
	ca->enable = 1;
}

void cache_disable(cache_enty * ca){
	ca->enable = 0;
}

//lru is updated
//when cache hit occurs during a read
//when cache hit occurs during a write
//when replacement occurs after a cache miss

static INLINE void update_lru(int way, u32*lru)
{
   if (way == 3)
   {
      *lru = *lru | 0xb;//set bits 3, 1, 0
      return;
   }
   else if (way == 2)
   {
      *lru = *lru & 0x3E;//set bit 0 to 0
      *lru = *lru | 0x14;//set bits 4 and 2
      return;
   }
   else if (way == 1)
   {
      *lru = *lru | (1 << 5);//set bit 5
      *lru = *lru & 0x39;//unset bits 2 and 1
      return;
   }
   else
   {
      *lru = *lru & 0x7;//unset bits 5,4,3
      return;
   }

   //should not happen
}

static INLINE int select_way_to_replace(SH2_struct *sh, u32 lru)
{
   if (sh->onchip.CCR & (1 << 3))//2-way mode
   {
      if ((lru & 1) == 1)
         return 2;
      else
         return 3;
   }
   else
   {
      if ((lru & 0x38) == 0x38)//bits 5, 4, 3 must be 1
         return 0;
      else if ((lru & 0x26) == 0x6)//bit 5 must be zero. bits 2 and 1 must be 1
         return 1;
      else if ((lru & 0x15) == 1)//bits 4, 2 must be zero. bit 0 must be 1
         return 2;
      else if ((lru & 0xB) == 0)//bits 3, 1, 0 must be zero
         return 3;
   }

   //should not happen
   return 0;
}

//values are from console measurements and have extra delays included
//delay 0 if the measured cycles are 7 or less, otherwise subtract 7
#define ADJUST_CYCLES(n) (n <= 7 ? 0 : (n - 7))

int get_cache_through_timing_read_byte_word(u32 addr)
{
   addr = (addr >> 16) & 0xFFF;

   if (addr <= 0x00f)//bios
      return ADJUST_CYCLES(15);
   else if (addr >= 0x010 && addr <= 0x017)//smpc
      return ADJUST_CYCLES(15);
   else if (addr >= 0x018 && addr <= 0x01f)//bup
      return ADJUST_CYCLES(1);
   else if (addr >= 0x020 && addr <= 0x02f)//lwram
      return ADJUST_CYCLES(14);
   //ignore input capture
   else if (addr >= 0x200 && addr <= 0x3ff)//cs0
      return ADJUST_CYCLES(1);
   else if (addr >= 0x400 && addr <= 0x4ff)//cs1
      return ADJUST_CYCLES(1);
   else if (addr >= 0x580 && addr <= 0x58f)//cs2
      return ADJUST_CYCLES(24);
   else if (addr >= 0x5a0 && addr <= 0x5af)//sound ram
      return ADJUST_CYCLES(53);
   else if (addr >= 0x5b0 && addr <= 0x5bf)//scsp regs
      return ADJUST_CYCLES(52);
   else if (addr >= 0x5c0 && addr <= 0x5c7)//vdp1 ram
      return ADJUST_CYCLES(51);
   else if (addr >= 0x5c8 && addr <= 0x5cf)//vdp1 fb
      return ADJUST_CYCLES(51);
   else if (addr >= 0x5d0 && addr <= 0x5d7)//vdp1 regs
      return ADJUST_CYCLES(35);
   else if (addr >= 0x5e0 && addr <= 0x5ef)//vdp2 ram
      return ADJUST_CYCLES(44);
   else if (addr >= 0x5f0 && addr <= 0x5f7)//vdp2 color
      return ADJUST_CYCLES(44);
   else if (addr >= 0x5f8 && addr <= 0x5fb)//vdp2 regs
      return ADJUST_CYCLES(44);
   else if (addr >= 0x5fe && addr <= 0x5fe)//scu
      return ADJUST_CYCLES(14);
   else if (addr >= 0x600 && addr <= 0x7ff)//hwram
      return ADJUST_CYCLES(14);

   return 0;
}

int get_cache_through_timing_read_long(u32 addr)
{
   addr = (addr >> 16) & 0xFFF;

   if (addr <= 0x00f)//bios
      return ADJUST_CYCLES(23);
   else if (addr >= 0x010 && addr <= 0x017)//smpc
      return ADJUST_CYCLES(23);
   else if (addr >= 0x018 && addr <= 0x01f)//bup
      return ADJUST_CYCLES(1);
   else if (addr >= 0x020 && addr <= 0x02f)//lwram
      return ADJUST_CYCLES(21);
   //ignore input capture
   else if (addr >= 0x200 && addr <= 0x3ff)//cs0
      return ADJUST_CYCLES(1);
   else if (addr >= 0x400 && addr <= 0x4ff)//cs1
      return ADJUST_CYCLES(1);
   else if (addr >= 0x580 && addr <= 0x58f)//cs2
      return ADJUST_CYCLES(24);
   else if (addr >= 0x5a0 && addr <= 0x5af)//sound ram
      return ADJUST_CYCLES(53);
   else if (addr >= 0x5b0 && addr <= 0x5bf)//scsp regs
      return ADJUST_CYCLES(52);
   else if (addr >= 0x5c0 && addr <= 0x5c7)//vdp1 ram
      return ADJUST_CYCLES(51);
   else if (addr >= 0x5c8 && addr <= 0x5cf)//vdp1 fb
      return ADJUST_CYCLES(51);
   else if (addr >= 0x5d0 && addr <= 0x5d7)//vdp1 regs
      return ADJUST_CYCLES(35);
   else if (addr >= 0x5e0 && addr <= 0x5ef)//vdp2 ram
      return ADJUST_CYCLES(44);
   else if (addr >= 0x5f0 && addr <= 0x5f7)//vdp2 color
      return ADJUST_CYCLES(44);
   else if (addr >= 0x5f8 && addr <= 0x5fb)//vdp2 regs
      return ADJUST_CYCLES(44);
   else if (addr >= 0x5fe && addr <= 0x5fe)//scu
      return ADJUST_CYCLES(14);
   else if (addr >= 0x600 && addr <= 0x7ff)//hwram
      return ADJUST_CYCLES(14);

   return 0;
}

int get_cache_through_timing_write_byte_word(u32 addr)
{
   addr = (addr >> 16) & 0xFFF;

   if (addr <= 0x00f)//bios
      return ADJUST_CYCLES(8);
   else if (addr >= 0x010 && addr <= 0x017)//smpc
      return ADJUST_CYCLES(8);
   else if (addr >= 0x018 && addr <= 0x01f)//bup
      return ADJUST_CYCLES(1);
   else if (addr >= 0x020 && addr <= 0x02f)//lwram
      return ADJUST_CYCLES(7);
   //ignore input capture
   else if (addr >= 0x200 && addr <= 0x3ff)//cs0
      return ADJUST_CYCLES(1);
   else if (addr >= 0x400 && addr <= 0x4ff)//cs1
      return ADJUST_CYCLES(1);
   else if (addr >= 0x580 && addr <= 0x58f)//cs2
      return ADJUST_CYCLES(7);
   else if (addr >= 0x5a0 && addr <= 0x5af)//sound ram
      return ADJUST_CYCLES(19);
   else if (addr >= 0x5b0 && addr <= 0x5bf)//scsp regs
      return ADJUST_CYCLES(19);
   else if (addr >= 0x5c0 && addr <= 0x5c7)//vdp1 ram
      return ADJUST_CYCLES(11);
   else if (addr >= 0x5c8 && addr <= 0x5cf)//vdp1 fb
      return ADJUST_CYCLES(11);
   else if (addr >= 0x5d0 && addr <= 0x5d7)//vdp1 regs
      return ADJUST_CYCLES(11);
   else if (addr >= 0x5e0 && addr <= 0x5ef)//vdp2 ram
      return ADJUST_CYCLES(7);
   else if (addr >= 0x5f0 && addr <= 0x5f7)//vdp2 color
      return ADJUST_CYCLES(8);
   else if (addr >= 0x5f8 && addr <= 0x5fb)//vdp2 regs
      return ADJUST_CYCLES(7);
   else if (addr >= 0x5fe && addr <= 0x5fe)//scu
      return ADJUST_CYCLES(7);
   else if (addr >= 0x600 && addr <= 0x7ff)//hwram
      return ADJUST_CYCLES(7);

   return 0;
}

int get_cache_through_timing_write_long(u32 addr)
{
   addr = (addr >> 16) & 0xFFF;

   if (addr <= 0x00f)//bios
      return ADJUST_CYCLES(16);
   else if (addr >= 0x010 && addr <= 0x017)//smpc
      return ADJUST_CYCLES(16);
   else if (addr >= 0x018 && addr <= 0x01f)//bup
      return ADJUST_CYCLES(1);
   else if (addr >= 0x020 && addr <= 0x02f)//lwram
      return ADJUST_CYCLES(14);
   //ignore input capture
   else if (addr >= 0x200 && addr <= 0x3ff)//cs0
      return ADJUST_CYCLES(1);
   else if (addr >= 0x400 && addr <= 0x4ff)//cs1
      return ADJUST_CYCLES(1);
   else if (addr >= 0x580 && addr <= 0x58f)//cs2
      return ADJUST_CYCLES(14);
   else if (addr >= 0x5a0 && addr <= 0x5af)//sound ram
      return ADJUST_CYCLES(33);
   else if (addr >= 0x5b0 && addr <= 0x5bf)//scsp regs
      return ADJUST_CYCLES(32);
   else if (addr >= 0x5c0 && addr <= 0x5c7)//vdp1 ram
      return ADJUST_CYCLES(12);
   else if (addr >= 0x5c8 && addr <= 0x5cf)//vdp1 fb
      return ADJUST_CYCLES(12);
   else if (addr >= 0x5d0 && addr <= 0x5d7)//vdp1 regs
      return ADJUST_CYCLES(11);
   else if (addr >= 0x5e0 && addr <= 0x5ef)//vdp2 ram
      return ADJUST_CYCLES(7);
   else if (addr >= 0x5f0 && addr <= 0x5f7)//vdp2 color
      return ADJUST_CYCLES(8);
   else if (addr >= 0x5f8 && addr <= 0x5fb)//vdp2 regs
      return ADJUST_CYCLES(7);
   else if (addr >= 0x5fe && addr <= 0x5fe)//scu
      return ADJUST_CYCLES(7);
   else if (addr >= 0x600 && addr <= 0x7ff)//hwram
      return ADJUST_CYCLES(7);

   return 0;
}
void cache_memory_write_b(SH2_struct *sh, cache_enty * ca, u32 addr, u8 val){

	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
		if (ca->enable == 0){
			MappedMemoryWriteByteNocache(sh, addr, val);
			return;
		}
		tagaddr = (addr & TAG_MASK);
		entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].v && ca->way[0][entry].tag == tagaddr){
			ca->way[0][entry].data[addr&LINE_MASK] = val;
         update_lru(0, &ca->lru[entry]);
		}
		else if (ca->way[1][entry].v && ca->way[1][entry].tag == tagaddr){
			ca->way[1][entry].data[addr&LINE_MASK] = val;
         update_lru(1, &ca->lru[entry]);
		}
		else if (ca->way[2][entry].v && ca->way[2][entry].tag == tagaddr){
			ca->way[2][entry].data[addr&LINE_MASK] = val;
         update_lru(2, &ca->lru[entry]);
		}
		else if (ca->way[3][entry].v && ca->way[3][entry].tag == tagaddr){
			ca->way[3][entry].data[addr&LINE_MASK] = val;
         update_lru(3, &ca->lru[entry]);
		}
		MappedMemoryWriteByteNocache(sh, addr, val);
	}
	break;
	case CACHE_THROUGH:
      sh->cycles += get_cache_through_timing_write_byte_word(addr);
		MappedMemoryWriteByteNocache(sh, addr, val);
		break;
	default:
		MappedMemoryWriteByteNocache(sh, addr, val);
		break;
	}
}

void cache_memory_write_w(SH2_struct *sh, cache_enty * ca, u32 addr, u16 val){

	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
		if (ca->enable == 0){
			MappedMemoryWriteWordNocache(sh, addr, val);
			return;
		}

		tagaddr = (addr & TAG_MASK);
		entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].v && ca->way[0][entry].tag == tagaddr){
			ca->way[0][entry].data[addr&LINE_MASK] = val >> 8;
			ca->way[0][entry].data[(addr&LINE_MASK) + 1] = val;
         update_lru(0, &ca->lru[entry]);
		}
		else if (ca->way[1][entry].v && ca->way[1][entry].tag == tagaddr){
			ca->way[1][entry].data[addr&LINE_MASK] = val >> 8;
			ca->way[1][entry].data[(addr&LINE_MASK) + 1] = val;
         update_lru(1, &ca->lru[entry]);
		}
		else if (ca->way[2][entry].v && ca->way[2][entry].tag == tagaddr){
			ca->way[2][entry].data[addr&LINE_MASK] = val >> 8;
			ca->way[2][entry].data[(addr&LINE_MASK) + 1] = val;
         update_lru(2, &ca->lru[entry]);
		}
		else if (ca->way[3][entry].v && ca->way[3][entry].tag == tagaddr){
			ca->way[3][entry].data[addr&LINE_MASK] = val >> 8;
			ca->way[3][entry].data[(addr&LINE_MASK) + 1] = val;
         update_lru(3, &ca->lru[entry]);
		}

		// write through
		MappedMemoryWriteWordNocache(sh, addr, val);
	}
	break;
	case CACHE_THROUGH:
      sh->cycles += get_cache_through_timing_write_byte_word(addr);
		MappedMemoryWriteWordNocache(sh, addr, val);
		break;
	default:
		MappedMemoryWriteWordNocache(sh, addr, val);
		break;
	}
}

void cache_memory_write_l(SH2_struct *sh, cache_enty * ca, u32 addr, u32 val){

	switch (addr & AREA_MASK){
   case CACHE_PURGE://associative purge
   {
      int i;
      u32 tagaddr = (addr & TAG_MASK);
      u32 entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
      for (i = 0; i < 3; i++)
      {
         if (ca->way[i][entry].tag == tagaddr)
         {
            //only v bit is changed, the rest of the data remains
            ca->way[i][entry].v = 0;
            break;
         }
      }
   }
   break;
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
		if (ca->enable == 0){
			MappedMemoryWriteLongNocache(sh, addr, val);
			return;
		}

		tagaddr = (addr & TAG_MASK);
		entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].v && ca->way[0][entry].tag == tagaddr){
			ca->way[0][entry].data[(addr&LINE_MASK)] = ((val >> 24) & 0xFF);
			ca->way[0][entry].data[(addr&LINE_MASK) + 1] = ((val >> 16) & 0xFF);
			ca->way[0][entry].data[(addr&LINE_MASK) + 2] = ((val >> 8) & 0xFF);
			ca->way[0][entry].data[(addr&LINE_MASK) + 3] = ((val >> 0) & 0xFF);
         update_lru(0, &ca->lru[entry]);
		}
		else if (ca->way[1][entry].v && ca->way[1][entry].tag == tagaddr){
			ca->way[1][entry].data[(addr&LINE_MASK)] = ((val >> 24) & 0xFF);
			ca->way[1][entry].data[(addr&LINE_MASK) + 1] = ((val >> 16) & 0xFF);
			ca->way[1][entry].data[(addr&LINE_MASK) + 2] = ((val >> 8) & 0xFF);
			ca->way[1][entry].data[(addr&LINE_MASK) + 3] = ((val >> 0) & 0xFF);
         update_lru(1, &ca->lru[entry]);
		}
		else if (ca->way[2][entry].v && ca->way[2][entry].tag == tagaddr){
			ca->way[2][entry].data[(addr&LINE_MASK)] = ((val >> 24) & 0xFF);
			ca->way[2][entry].data[(addr&LINE_MASK) + 1] = ((val >> 16) & 0xFF);
			ca->way[2][entry].data[(addr&LINE_MASK) + 2] = ((val >> 8) & 0xFF);
			ca->way[2][entry].data[(addr&LINE_MASK) + 3] = ((val >> 0) & 0xFF);
         update_lru(2, &ca->lru[entry]);

		}
		else if (ca->way[3][entry].v && ca->way[3][entry].tag == tagaddr){
			ca->way[3][entry].data[(addr&LINE_MASK)] = ((val >> 24) & 0xFF);
			ca->way[3][entry].data[(addr&LINE_MASK) + 1] = ((val >> 16) & 0xFF);
			ca->way[3][entry].data[(addr&LINE_MASK) + 2] = ((val >> 8) & 0xFF);
			ca->way[3][entry].data[(addr&LINE_MASK) + 3] = ((val >> 0) & 0xFF);
         update_lru(3, &ca->lru[entry]);
		}

		// write through
		MappedMemoryWriteLongNocache(sh, addr, val);
	}
	break;
	case CACHE_THROUGH:
      sh->cycles += get_cache_through_timing_write_long(addr);
		MappedMemoryWriteLongNocache(sh, addr, val);
		break;
	default:
		MappedMemoryWriteLongNocache(sh, addr, val);
		break;
	}
}

u32 sh2_cache_refill_read(SH2_struct *sh, u32 addr)
{
   addr &= 0xfffffff;

   if (addr <= 0x00fffff)
   {
      //bios
      return BiosRomMemoryReadLong(addr);
   }
   else if (addr >= 0x0100000 && addr <= 0x017ffff)
   {
      //smpc
      return SmpcReadLong(MSH2, addr);
   }
   else if (addr >= 0x0180000 && addr <= 0x01fffff)
   {
      //backup ram
      return BupRamMemoryReadLong(addr);
   }
   else if (addr >= 0x0200000 && addr <= 0x02fffff)
   {
      //low wram
      return LowWramMemoryReadLong(addr);
   }
   else if (addr >= 0x1000000 && addr <= 0x17fffff)
   {
      //ssh2 input capture
      return UnhandledMemoryReadLong(addr);
   }
   else if (addr >= 0x1800000 && addr <= 0x1ffffff)
   {
      //msh2 input capture
      return UnhandledMemoryReadLong(addr);
   }
   else if (addr >= 0x2000000 && addr <= 0x3ffffff)
   {
      //cs0
      return CartridgeArea->Cs0ReadLong(MSH2, addr);
   }
   else if (addr >= 0x4000000 && addr <= 0x4ffffff)
   {
      return Cs1ReadLong(MSH2, addr);
   }
   else if (addr >= 0x5000000 && addr <= 0x57fffff)
   {
      //dummy
   }
   else if (addr >= 0x5800000 && addr <= 0x58fffff)
   {
      //cs2
      if (yabsys.use_cd_block_lle)
      {
         return ygr_a_bus_read_long(addr);
      }
      else
      {
         return Cs2ReadLong(MSH2, addr);
      }
   }
   else if (addr >= 0x5a00000 && addr <= 0x5afffff)
   {
      //sound ram
      return SoundRamReadLong(addr);
   }
   else if (addr >= 0x5b00000 && addr <= 0x5bfffff)
   {
      //scsp regs
      return ScspReadLong(addr);
   }
   else if (addr >= 0x5c00000 && addr <= 0x5c7ffff)
   {
      //vdp1 ram
      return Vdp1RamReadLong(addr);
   }
   else if (addr >= 0x5c80000 && addr <= 0x5cfffff)
   {
      //vdp1 framebuffer
      return Vdp1FrameBufferReadLong(addr);
   }
   else if (addr >= 0x5d00000 && addr <= 0x5d7ffff)
   {
      //vdp1 registers
      return Vdp1ReadLong(addr);
   }
   else if (addr >= 0x5e00000 && addr <= 0x5efffff)
   {
      //vdp2 ram
      return Vdp2RamReadLong(addr);
   }
   else if (addr >= 0x5f00000 && addr <= 0x5f7ffff)
   {
      //vdp2 color ram
      return Vdp2ColorRamReadLong(addr);
   }
   else if (addr >= 0x5f80000 && addr <= 0x5fbffff)
   {
      //vdp2 registers
      return Vdp2ReadLong(addr);
   }
   else if (addr >= 0x5fe0000 && addr <= 0x5feffff)
   {
      //scu registers
      return ScuReadLong(addr);
   }
   else if (addr >= 0x6000000 && addr <= 0x7ffffff)
   {
      //high wram
      return HighWramMemoryReadLong(addr);
   }

   return 0;
}

void sh2_refill_cache(SH2_struct *sh, cache_enty * ca, int lruway, u32 entry, u32 addr)
{
   int i;

   sh->cycles += 4;

   for (i = 0; i < 16; i += 4) {
      u32 val = sh2_cache_refill_read(sh, (addr & 0xFFFFFFF0) + i);
      ca->way[lruway][entry].data[i + 0] = (val >> 24) & 0xff;
      ca->way[lruway][entry].data[i + 1] = (val >> 16) & 0xff;
      ca->way[lruway][entry].data[i + 2] = (val >> 8) & 0xff;
      ca->way[lruway][entry].data[i + 3] = (val >> 0) & 0xff;
   }
}

u8 cache_memory_read_b(SH2_struct *sh, cache_enty * ca, u32 addr){
	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
      int i = 0;
      int lruway = 0;
		if (ca->enable == 0){
			return MappedMemoryReadByteNocache(sh, addr);
		}
		tagaddr = (addr & TAG_MASK);
		entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].v && ca->way[0][entry].tag == tagaddr){
         update_lru(0, &ca->lru[entry]);
			return ca->way[0][entry].data[addr&LINE_MASK];
		}
		else if (ca->way[1][entry].v && ca->way[1][entry].tag == tagaddr){
         update_lru(1, &ca->lru[entry]);
			return ca->way[1][entry].data[addr&LINE_MASK];
		}
		else if (ca->way[2][entry].v && ca->way[2][entry].tag == tagaddr){
         update_lru(2, &ca->lru[entry]);
			return ca->way[2][entry].data[addr&LINE_MASK];
		}
		else if (ca->way[3][entry].v && ca->way[3][entry].tag == tagaddr){
         update_lru(3, &ca->lru[entry]);
			return ca->way[3][entry].data[addr&LINE_MASK];
		}
		// cache miss
      lruway = select_way_to_replace(sh, ca->lru[entry]);
      update_lru(lruway, &ca->lru[entry]);
		ca->way[lruway][entry].tag = tagaddr;

      sh2_refill_cache(sh, ca, lruway, entry, addr);

      ca->way[lruway][entry].v = 1; //becomes valid
		return ca->way[lruway][entry].data[addr&LINE_MASK];
	}
	break;
	case CACHE_THROUGH:
      sh->cycles += get_cache_through_timing_read_byte_word(addr);
		return MappedMemoryReadByteNocache(sh, addr);
		break;
	default:
		return MappedMemoryReadByteNocache(sh, addr);
		break;
	}
	return 0;
}

u16 cache_memory_read_w(SH2_struct *sh, cache_enty * ca, u32 addr){

	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
      int i = 0;
      int lruway = 0;
		if (ca->enable == 0){
			return MappedMemoryReadWordNocache(sh, addr);
		}
	   tagaddr = (addr & TAG_MASK);
		entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].v && ca->way[0][entry].tag == tagaddr){
         update_lru(0, &ca->lru[entry]);
			return ((u16)(ca->way[0][entry].data[addr&LINE_MASK]) << 8) | ca->way[0][entry].data[(addr&LINE_MASK) + 1];
		}
		else if (ca->way[1][entry].v && ca->way[1][entry].tag == tagaddr){
         update_lru(1, &ca->lru[entry]);
			return ((u16)(ca->way[1][entry].data[addr&LINE_MASK]) << 8) | ca->way[1][entry].data[(addr&LINE_MASK) + 1];
		}
		else if (ca->way[2][entry].v && ca->way[2][entry].tag == tagaddr){
         update_lru(2, &ca->lru[entry]);
			return ((u16)(ca->way[2][entry].data[addr&LINE_MASK]) << 8) | ca->way[2][entry].data[(addr&LINE_MASK) + 1];
		}
		else if (ca->way[3][entry].v && ca->way[3][entry].tag == tagaddr){
         update_lru(3, &ca->lru[entry]);
			return ((u16)(ca->way[3][entry].data[addr&LINE_MASK]) << 8) | ca->way[3][entry].data[(addr&LINE_MASK) + 1];
		}

		// cache miss
		lruway = select_way_to_replace(sh, ca->lru[entry]);
      update_lru(lruway, &ca->lru[entry]);
		ca->way[lruway][entry].tag = tagaddr;

      sh2_refill_cache(sh, ca, lruway, entry, addr);

      ca->way[lruway][entry].v = 1; //becomes valid
		return ((u16)(ca->way[lruway][entry].data[addr&LINE_MASK]) << 8) | ca->way[lruway][entry].data[(addr&LINE_MASK) + 1];
	}
	break;
	case CACHE_THROUGH:
      sh->cycles += get_cache_through_timing_read_byte_word(addr);
		return MappedMemoryReadWordNocache(sh, addr);
		break;
	default:
		return MappedMemoryReadWordNocache(sh, addr);
		break;
	}
	return 0;
}

u32 cache_memory_read_l(SH2_struct *sh, cache_enty * ca, u32 addr){
	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
      int i = 0;
      int lruway = 0;
		if (ca->enable == 0){
			return MappedMemoryReadLongNocache(sh, addr);
		}
		tagaddr = (addr & TAG_MASK);
	   entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;

		if (ca->way[0][entry].v && ca->way[0][entry].tag == tagaddr){
         update_lru(0, &ca->lru[entry]);
			return ((u32)(ca->way[0][entry].data[addr&LINE_MASK]) << 24) |
				((u32)(ca->way[0][entry].data[(addr&LINE_MASK) + 1]) << 16) |
				((u32)(ca->way[0][entry].data[(addr&LINE_MASK) + 2]) << 8) |
				((u32)(ca->way[0][entry].data[(addr&LINE_MASK) + 3]) << 0);
		}
		else if (ca->way[1][entry].v && ca->way[1][entry].tag == tagaddr){
         update_lru(1, &ca->lru[entry]);
			return ((u32)(ca->way[1][entry].data[addr&LINE_MASK]) << 24) |
				((u32)(ca->way[1][entry].data[(addr&LINE_MASK) + 1]) << 16) |
				((u32)(ca->way[1][entry].data[(addr&LINE_MASK) + 2]) << 8) |
				((u32)(ca->way[1][entry].data[(addr&LINE_MASK) + 3]) << 0);
		}
		else if (ca->way[2][entry].v && ca->way[2][entry].tag == tagaddr){
         update_lru(2, &ca->lru[entry]);
			return ((u32)(ca->way[2][entry].data[addr&LINE_MASK]) << 24) |
				((u32)(ca->way[2][entry].data[(addr&LINE_MASK) + 1]) << 16) |
				((u32)(ca->way[2][entry].data[(addr&LINE_MASK) + 2]) << 8) |
				((u32)(ca->way[2][entry].data[(addr&LINE_MASK) + 3]) << 0);
		}
		else if (ca->way[3][entry].v && ca->way[3][entry].tag == tagaddr){
         update_lru(3, &ca->lru[entry]);
			return ((u32)(ca->way[3][entry].data[addr&LINE_MASK]) << 24) |
				((u32)(ca->way[3][entry].data[(addr&LINE_MASK) + 1]) << 16) |
				((u32)(ca->way[3][entry].data[(addr&LINE_MASK) + 2]) << 8) |
				((u32)(ca->way[3][entry].data[(addr&LINE_MASK) + 3]) << 0);
			}
		// cache miss
		lruway = select_way_to_replace(sh, ca->lru[entry]);
      update_lru(lruway, &ca->lru[entry]);
		ca->way[lruway][entry].tag = tagaddr;

      sh2_refill_cache(sh, ca, lruway, entry, addr);

      ca->way[lruway][entry].v = 1; //becomes valid
		return ((u32)(ca->way[lruway][entry].data[addr&LINE_MASK]) << 24) |
			((u32)(ca->way[lruway][entry].data[(addr&LINE_MASK) + 1]) << 16) |
			((u32)(ca->way[lruway][entry].data[(addr&LINE_MASK) + 2]) << 8) |
			((u32)(ca->way[lruway][entry].data[(addr&LINE_MASK) + 3]) << 0);
	}
	break;
	case CACHE_THROUGH:
      sh->cycles += get_cache_through_timing_read_long(addr);
		return MappedMemoryReadLongNocache(sh, addr);
		break;
	default:
		return MappedMemoryReadLongNocache(sh, addr);
		break;
	}
	return 0;
}

