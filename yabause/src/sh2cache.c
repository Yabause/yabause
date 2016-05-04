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


#define AREA_MASK   (0xE0000000)
#define TAG_MASK   (0x1FFFFC00)
#define ENTRY_MASK (0x000003F0)
#define ENTRY_SHIFT (4)
#define LINE_MASK  (0x0000000F)

#define CACHE_USE ((0x00)<<29)
#define CACHE_THROUGH ((0x01)<<29)
#define CACHE_PAGE ((0x02)<<29)
#define CACHE_ADDRES_ARRAY ((0x03)<<29)
#define CACHE_DATA_ARRAY ((0x06)<<29)
#define CACHE_IO ((0x07)<<29)

void cache_clear(cache_enty * ca){
	ca->enable = 0;
	ca->lru = 0x00;
	int entry = 0;
	for ( entry = 0; entry < 64; entry++){
		ca->way[0][entry].tag = 0xFFFFFFFF;
		ca->way[1][entry].tag = 0xFFFFFFFF;
		ca->way[2][entry].tag = 0xFFFFFFFF;
		ca->way[3][entry].tag = 0xFFFFFFFF;
	}
	return;
}

void cache_enable(cache_enty * ca){
	if (ca->enable == 0){
		cache_clear(ca);
	}
	ca->enable = 1;
}

void cache_disable(cache_enty * ca){
	ca->enable = 0;
}
void cache_memory_write_b(cache_enty * ca, u32 addr, u8 val){

	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
		if (ca->enable == 0){
			MappedMemoryWriteByteNocache(addr, val);
			return;
		}
		u32 tagaddr = (addr & TAG_MASK);
		u32 entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x000000;
			ca->way[0][entry].data[addr&LINE_MASK] = val;
		}
		else if (ca->way[1][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x100000;
			ca->way[1][entry].data[addr&LINE_MASK] = val;
		}
		else if (ca->way[2][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x200000;
			ca->way[2][entry].data[addr&LINE_MASK] = val;
		}
		else if (ca->way[3][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x300000;
			ca->way[3][entry].data[addr&LINE_MASK] = val;
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
		if (ca->enable == 0){
			MappedMemoryWriteWordNocache(addr, val);
			return;
		}

		u32 tagaddr = (addr & TAG_MASK);
		u32 entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x000000;
			ca->way[0][entry].data[addr&LINE_MASK] = val >> 8;
			ca->way[0][entry].data[(addr&LINE_MASK) + 1] = val;
		}
		else if (ca->way[1][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x100000;
			ca->way[1][entry].data[addr&LINE_MASK] = val >> 8;
			ca->way[1][entry].data[(addr&LINE_MASK) + 1] = val;
		}
		else if (ca->way[2][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x200000;
			ca->way[2][entry].data[addr&LINE_MASK] = val >> 8;
			ca->way[2][entry].data[(addr&LINE_MASK) + 1] = val;
		}
		else if (ca->way[3][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x300000;
			ca->way[3][entry].data[addr&LINE_MASK] = val >> 8;
			ca->way[3][entry].data[(addr&LINE_MASK) + 1] = val;
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
	case CACHE_USE:
	{
		if (ca->enable == 0){
			MappedMemoryWriteLongNocache(addr, val);
			return;
		}

		u32 tagaddr = (addr & TAG_MASK);
		u32 entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x000000;
			ca->way[0][entry].data[(addr&LINE_MASK)] = ((val >> 24) & 0xFF);
			ca->way[0][entry].data[(addr&LINE_MASK) + 1] = ((val >> 16) & 0xFF);
			ca->way[0][entry].data[(addr&LINE_MASK) + 2] = ((val >> 8) & 0xFF);
			ca->way[0][entry].data[(addr&LINE_MASK) + 3] = ((val >> 0) & 0xFF);
		}
		else if (ca->way[1][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x100000;
			ca->way[1][entry].data[(addr&LINE_MASK)] = ((val >> 24) & 0xFF);
			ca->way[1][entry].data[(addr&LINE_MASK) + 1] = ((val >> 16) & 0xFF);
			ca->way[1][entry].data[(addr&LINE_MASK) + 2] = ((val >> 8) & 0xFF);
			ca->way[1][entry].data[(addr&LINE_MASK) + 3] = ((val >> 0) & 0xFF);
		}
		else if (ca->way[2][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x200000;
			ca->way[2][entry].data[(addr&LINE_MASK)] = ((val >> 24) & 0xFF);
			ca->way[2][entry].data[(addr&LINE_MASK) + 1] = ((val >> 16) & 0xFF);
			ca->way[2][entry].data[(addr&LINE_MASK) + 2] = ((val >> 8) & 0xFF);
			ca->way[2][entry].data[(addr&LINE_MASK) + 3] = ((val >> 0) & 0xFF);

		}
		else if (ca->way[3][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x300000;
			ca->way[3][entry].data[(addr&LINE_MASK)] = ((val >> 24) & 0xFF);
			ca->way[3][entry].data[(addr&LINE_MASK) + 1] = ((val >> 16) & 0xFF);
			ca->way[3][entry].data[(addr&LINE_MASK) + 2] = ((val >> 8) & 0xFF);
			ca->way[3][entry].data[(addr&LINE_MASK) + 3] = ((val >> 0) & 0xFF);
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
		if (ca->enable == 0){
			return MappedMemoryReadByteNocache(addr);
		}
		u32 tagaddr = (addr & TAG_MASK);
		u32 entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x000000;
			return ca->way[0][entry].data[addr&LINE_MASK];
		}
		else if (ca->way[1][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x100000;
			return ca->way[1][entry].data[addr&LINE_MASK];
		}
		else if (ca->way[2][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x200000;
			return ca->way[2][entry].data[addr&LINE_MASK];
		}
		else if (ca->way[3][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x300000;
			return ca->way[3][entry].data[addr&LINE_MASK];
		}
		// cache miss
		int lruway = (ca->lru & 0x3);
		int i;
		ca->way[lruway][entry].tag = tagaddr;
		for (i = 0; i < 16; i++){
			ca->way[lruway][entry].data[i] = ReadByteList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + i);
		}
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
		if (ca->enable == 0){
			return MappedMemoryReadWordNocache(addr);
		}
		u32 tagaddr = (addr & TAG_MASK);
		u32 entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x000000;
			return ((u16)(ca->way[0][entry].data[addr&LINE_MASK]) << 8) | ca->way[0][entry].data[(addr&LINE_MASK) + 1];
		}
		else if (ca->way[1][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x100000;
			return ((u16)(ca->way[1][entry].data[addr&LINE_MASK]) << 8) | ca->way[1][entry].data[(addr&LINE_MASK) + 1];
		}
		else if (ca->way[2][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x200000;
			return ((u16)(ca->way[2][entry].data[addr&LINE_MASK]) << 8) | ca->way[2][entry].data[(addr&LINE_MASK) + 1];
		}
		else if (ca->way[3][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x300000;
			return ((u16)(ca->way[3][entry].data[addr&LINE_MASK]) << 8) | ca->way[3][entry].data[(addr&LINE_MASK) + 1];
		}

		// cache miss
		int lruway = (ca->lru & 0x3);
		int i;
		ca->way[lruway][entry].tag = tagaddr;
		for (i = 0; i < 16; i++){
			ca->way[lruway][entry].data[i] = ReadByteList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + i);
		}
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
		if (ca->enable == 0){
			return MappedMemoryReadLongNocache(addr);
		}
		u32 tagaddr = (addr & TAG_MASK);
		u32 entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		if (ca->way[0][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x000000;
			return ((u32)(ca->way[0][entry].data[addr&LINE_MASK]) << 24) |
				((u32)(ca->way[0][entry].data[(addr&LINE_MASK) + 1]) << 16) |
				((u32)(ca->way[0][entry].data[(addr&LINE_MASK) + 2]) << 8) |
				((u32)(ca->way[0][entry].data[(addr&LINE_MASK) + 3]) << 0);
		}
		else if (ca->way[1][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x100000;
			return ((u32)(ca->way[1][entry].data[addr&LINE_MASK]) << 24) |
				((u32)(ca->way[1][entry].data[(addr&LINE_MASK) + 1]) << 16) |
				((u32)(ca->way[1][entry].data[(addr&LINE_MASK) + 2]) << 8) |
				((u32)(ca->way[1][entry].data[(addr&LINE_MASK) + 3]) << 0);
		}
		else if (ca->way[2][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x200000;
			return ((u32)(ca->way[2][entry].data[addr&LINE_MASK]) << 24) |
				((u32)(ca->way[2][entry].data[(addr&LINE_MASK) + 1]) << 16) |
				((u32)(ca->way[2][entry].data[(addr&LINE_MASK) + 2]) << 8) |
				((u32)(ca->way[2][entry].data[(addr&LINE_MASK) + 3]) << 0);
		}
		else if (ca->way[3][entry].tag == tagaddr){
			ca->lru >>= 4;
			ca->lru |= 0x300000;
			return ((u32)(ca->way[3][entry].data[addr&LINE_MASK]) << 24) |
				((u32)(ca->way[3][entry].data[(addr&LINE_MASK) + 1]) << 16) |
				((u32)(ca->way[3][entry].data[(addr&LINE_MASK) + 2]) << 8) |
				((u32)(ca->way[3][entry].data[(addr&LINE_MASK) + 3]) << 0);
			}
		// cache miss
		int lruway = (ca->lru & 0x3);
		int i;
		ca->way[lruway][entry].tag = tagaddr;
		for (i = 0; i < 16; i++){
			ca->way[lruway][entry].data[i] = ReadByteList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + i);
		}
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

