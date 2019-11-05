/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
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

static INLINE int select_way_to_replace(u32 lru)
{
   if (CurrentSH2->onchip.CCR & (1 << 3))//2-way mode
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

void cache_memory_write_b(cache_enty * ca, u32 addr, u8 val){

	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
		if (ca->enable == 0){
			MappedMemoryWriteByteNocache(addr, val);
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
		MappedMemoryWriteByteNocache(addr, val);
	}
	break;
	case CACHE_THROUGH:
		MappedMemoryWriteByteNocache(addr, val);
		break;
	default:
		MappedMemoryWriteByteNocache(addr, val);
		break;
	}
}

void cache_memory_write_w(cache_enty * ca, u32 addr, u16 val){

	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
		if (ca->enable == 0){
			MappedMemoryWriteWordNocache(addr, val);
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
		MappedMemoryWriteWordNocache(addr, val);
	}
	break;
	case CACHE_THROUGH:
		MappedMemoryWriteWordNocache(addr, val);
		break;
	default:
		MappedMemoryWriteWordNocache(addr, val);
		break;
	}
}

void cache_memory_write_l(cache_enty * ca, u32 addr, u32 val){

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
			MappedMemoryWriteLongNocache(addr, val);
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
		MappedMemoryWriteLongNocache(addr, val);
	}
	break;
	case CACHE_THROUGH:
		MappedMemoryWriteLongNocache(addr, val);
		break;
	default:
		MappedMemoryWriteLongNocache(addr, val);
		break;
	}
}


u8 cache_memory_read_b(cache_enty * ca, u32 addr){
	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
      int i = 0;
      int lruway = 0;
		if (ca->enable == 0){
			return MappedMemoryReadByteNocache(addr);
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
      lruway = select_way_to_replace(ca->lru[entry]);
      update_lru(lruway, &ca->lru[entry]);
		ca->way[lruway][entry].tag = tagaddr;
		for (i = 0; i < 16; i++){
			ca->way[lruway][entry].data[i] = ReadByteList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + i);
		}
     
      ca->way[lruway][entry].v = 1; //becomes valid
		return ca->way[lruway][entry].data[addr&LINE_MASK];
	}
	break;
	case CACHE_THROUGH:
		return MappedMemoryReadByteNocache(addr);
		break;
	default:
		return MappedMemoryReadByteNocache(addr);
		break;
	}
	return 0;
}

u16 cache_memory_read_w(cache_enty * ca, u32 addr){

	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
      int i = 0;
      int lruway = 0;
		if (ca->enable == 0){
			return MappedMemoryReadWordNocache(addr);
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
		lruway = select_way_to_replace(ca->lru[entry]);
      update_lru(lruway, &ca->lru[entry]);
		ca->way[lruway][entry].tag = tagaddr;
		for (i = 0; i < 16; i++){
			ca->way[lruway][entry].data[i] = ReadByteList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + i);
		}
      ca->way[lruway][entry].v = 1; //becomes valid
		return ((u16)(ca->way[lruway][entry].data[addr&LINE_MASK]) << 8) | ca->way[lruway][entry].data[(addr&LINE_MASK) + 1];
	}
	break;
	case CACHE_THROUGH:
		return MappedMemoryReadWordNocache(addr);
		break;
	default:
		return MappedMemoryReadWordNocache(addr);
		break;
	}
	return 0;
}

u32 cache_memory_read_l(cache_enty * ca, u32 addr){
	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
      int i = 0;
      int lruway = 0;
		if (ca->enable == 0){
			return MappedMemoryReadLongNocache(addr);
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
		lruway = select_way_to_replace(ca->lru[entry]);
      update_lru(lruway, &ca->lru[entry]);
		ca->way[lruway][entry].tag = tagaddr;
		for (i = 0; i < 16; i++){
			ca->way[lruway][entry].data[i] = ReadByteList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + i);
		}
      ca->way[lruway][entry].v = 1; //becomes valid
		return ((u32)(ca->way[lruway][entry].data[addr&LINE_MASK]) << 24) |
			((u32)(ca->way[lruway][entry].data[(addr&LINE_MASK) + 1]) << 16) |
			((u32)(ca->way[lruway][entry].data[(addr&LINE_MASK) + 2]) << 8) |
			((u32)(ca->way[lruway][entry].data[(addr&LINE_MASK) + 3]) << 0);
	}
	break;
	case CACHE_THROUGH:
		return MappedMemoryReadLongNocache(addr);
		break;
	default:
		return MappedMemoryReadLongNocache(addr);
		break;
	}
	return 0;
}

