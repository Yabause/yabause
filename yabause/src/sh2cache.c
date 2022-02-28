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

FILE * cache_f = NULL;

void cache_clear(cache_enty * ca){
   int entry = 0;
	ca->enable = 0;

	for (entry = 0; entry < 64; entry++){
      int way = 0;
      ca->lru[entry] = 0;

      for (way = 0; way < 4; way++)
      {
         int i = 0;
         ca->way[entry].tag[way] = 0;
         
         for (i = 0; i < 16; i++)
            ca->way[entry].data[way][i] = 0;

         ca->way[entry].v[way] = 0;
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

typedef struct
{
 u8 and;
 u8 or;
} LRU_TABLE;

const LRU_TABLE lru_upd[4] ={  {0x07,0x00}, {0x19, 0x20}, {0x2A, 0x14}, {0x34, 0x0B} };

static const s8 lru_replace[0x40] =
{
  0x03, 0x02,   -1, 0x02, 0x03,   -1, 0x01, 0x01,   -1, 0x02,   -1, 0x02,   -1,   -1, 0x01, 0x01,
  0x03,   -1,   -1,   -1, 0x03,   -1, 0x01, 0x01,   -1,   -1,   -1,   -1,   -1,   -1, 0x01, 0x01,
  0x03, 0x02,   -1, 0x02, 0x03,   -1,   -1,   -1,   -1, 0x02,   -1, 0x02,   -1,   -1,   -1,   -1,
  0x03,   -1,   -1,   -1, 0x03,   -1,   -1,   -1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


static INLINE void update_lru(int way, u32*lru){
	*lru = ( (*lru) & lru_upd[way].and) | lru_upd[way].or;
	return;
}

static INLINE int select_way_to_replace(u32 lru){
		return lru_replace[lru & CurrentSH2->onchip.ccr_replace_and ]  | CurrentSH2->onchip.ccr_replace_or[0];  // i\IsInstr
}


#if HAVE_BUILTIN_BSWAP16
#define SWAP16(v) (__builtin_bswap16(v))
#else
static INLINE u16 SWAP16( u16 v ){
	return ((v >> 8)&0x00FF)|((v<<8) & 0xFF00); 
}
#endif  

#if HAVE_BUILTIN_BSWAP32
#define SWAP32( v ) (__builtin_bswap32(v))		
#else
static INLINE u32 SWAP32( u32 v ){
	return ((v >> 24)&0x000000FF) | ((v>>8)&0x0000FF00) | ((v << 8)&0x00FF0000) | ((v<<24) & 0xFF000000);
}  
#endif

void cache_memory_write_b(cache_enty * ca, u32 addr, u8 val, u32 * cycle){

	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
    u32 tagaddr = 0;
    u32 entry = 0;
		if (ca->enable == 0){
			MappedMemoryWriteByteNocache(addr, val, cycle);
			return;
		}
		tagaddr = (addr & TAG_MASK);
		entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		int way = -1;
		if (ca->way[entry].v[0] && ca->way[entry].tag[0] == tagaddr){
			way = 0;
		}
		else if (ca->way[entry].v[1] && ca->way[entry].tag[1] == tagaddr){
			way = 1;
		}
		else if (ca->way[entry].v[2] && ca->way[entry].tag[2] == tagaddr){
			way = 2;
		}
		else if (ca->way[entry].v[3] && ca->way[entry].tag[3] == tagaddr){
			way = 3;
		}
#ifdef CACHE_STATICS		
		ca->write_count++;
#endif		
		if( way > -1 ){
			ca->way[entry].data[way][(addr&LINE_MASK)] = val;
			update_lru(way, &ca->lru[entry]);
			CACHE_LOG("[%s] %d Cache Write 4 %08X %d:%d:%d %08X\n", CurrentSH2->isslave ? "SH2-S" : "SH2-M", CurrentSH2->cycles, addr, entry,way, (addr&LINE_MASK) , val);
		}
    MappedMemoryWriteByteNocache(addr, val, NULL);
    break;
	} // THROUGH TO CACHE_THROUGH
	case CACHE_THROUGH:
		MappedMemoryWriteByteNocache(addr, val, cycle);
		break;
	case CACHE_ADDRES_ARRAY:
	  CACHE_LOG("[%s] %zu-byte write to cache address array area; address=0x%08x value=0x%x\n", CurrentSH2->isslave?"SH2-S":"SH2-M", 1, addr, val);
		MappedMemoryWriteWordNocache(addr, val,cycle);
		break;		
	default:
		MappedMemoryWriteByteNocache(addr, val, cycle);
		break;
	}
}

void cache_memory_write_w(cache_enty * ca, u32 addr, u16 val, u32 * cycle){

	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
    u32 tagaddr = 0;
    u32 entry = 0;
		if (ca->enable == 0){
			MappedMemoryWriteWordNocache(addr, val,cycle);
			return;
		}

		if ( (addr&0x01) ){
			CACHE_LOG("[%s] data alignment error for 16bit %08X\n", CurrentSH2->isslave ? "SH2-S" : "SH2-M", addr);
			addr &= ~(0x01);
		}

		tagaddr = (addr & TAG_MASK);
		entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		int way = -1;
		if (ca->way[entry].v[0] && ca->way[entry].tag[0] == tagaddr){
			way = 0;
		}
		else if (ca->way[entry].v[1] && ca->way[entry].tag[1] == tagaddr){
			way = 1;
		}
		else if (ca->way[entry].v[2] && ca->way[entry].tag[2] == tagaddr){
			way = 2;
		}
		else if (ca->way[entry].v[3] && ca->way[entry].tag[3] == tagaddr){
			way = 3;
		}

		if( way > -1 ){
			*(u16*)(&ca->way[entry].data[way][(addr&LINE_MASK)]) = SWAP16(val);
			update_lru(way, &ca->lru[entry]);
			CACHE_LOG("[%s] %d Cache Write 4 %08X %d:%d:%d %08X\n", CurrentSH2->isslave ? "SH2-S" : "SH2-M", CurrentSH2->cycles, addr, entry,way, (addr&LINE_MASK) , val);
		}

#ifdef CACHE_STATICS
		ca->write_count++;
#endif
    MappedMemoryWriteWordNocache(addr, val, NULL);
    break;
	} // THROUGH TO CACHE_THROUGH
	case CACHE_THROUGH:{
		MappedMemoryWriteWordNocache(addr, val,cycle);
	}
		break;
	case CACHE_ADDRES_ARRAY:
	  CACHE_LOG("[%s] %zu-byte write to cache address array area; address=0x%08x value=0x%x\n", CurrentSH2->isslave?"SH2-S":"SH2-M", 2, addr, val);
		MappedMemoryWriteWordNocache(addr, val,cycle);
		break;		
	default:
		MappedMemoryWriteWordNocache(addr, val,cycle);
		break;
	}
}

void cache_memory_write_l(cache_enty * ca, u32 addr, u32 val, u32 *cycle){

	switch (addr & AREA_MASK){
   case CACHE_PURGE://associative purge
   {
      int i;
      u32 tagaddr = (addr & TAG_MASK);
      u32 entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;

			CACHE_LOG("Cache purge %08X %d\n",  addr, entry); 

      for (i = 0; i < 4; i++)
      {
         if (ca->way[entry].tag[i] == tagaddr)
         {
            //only v bit is changed, the rest of the data remains
            ca->way[entry].v[i] = 0;
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
			MappedMemoryWriteLongNocache(addr, val,cycle);
			return;
		}

		if ( (addr&0x03) ){
			addr &= ~(0x03);
			CACHE_LOG("[%s] data alignment error for 32bit %08X\n", CurrentSH2->isslave ? "SH2-S" : "SH2-M", addr);
		}


		tagaddr = (addr & TAG_MASK);
		entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;
		int way = -1;
		if (ca->way[entry].v[0] && ca->way[entry].tag[0] == tagaddr){
			way = 0;
		}
		else if (ca->way[entry].v[1] && ca->way[entry].tag[1] == tagaddr){
			way = 1;
		}
		else if (ca->way[entry].v[2] && ca->way[entry].tag[2] == tagaddr){
			way = 2;
		}
		else if (ca->way[entry].v[3] && ca->way[entry].tag[3] == tagaddr){
			way = 3;
		}
		if( way > -1 ){
			*(u32*)(&ca->way[entry].data[way][(addr&LINE_MASK)]) = SWAP32(val);
			update_lru(way, &ca->lru[entry]);
			CACHE_LOG("[%s] %d Cache Write 4 %08X %d:%d:%d %08X\n", CurrentSH2->isslave ? "SH2-S" : "SH2-M", CurrentSH2->cycles, addr, entry,way, (addr&LINE_MASK) , val);
		}

#ifdef CACHE_STATICS
		ca->write_count++;
#endif
    MappedMemoryWriteLongNocache(addr, val, NULL);
    break;
	} // THROUGH TO CACHE_THROUGH
	case CACHE_THROUGH:
		MappedMemoryWriteLongNocache(addr, val,cycle);
		break;
	default:
		MappedMemoryWriteLongNocache(addr, val,cycle);
		break;
	}
}

u8 cache_memory_read_b(cache_enty * ca, u32 addr, u32 * cycle){
	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
      u32 tagaddr = 0;
      u32 entry = 0;
      int i = 0;
      int lruway = 0;
		if (ca->enable == 0){
			return MappedMemoryReadByteNocache(addr,cycle);
		}
		tagaddr = (addr & TAG_MASK);
		entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;

		int way = -1;
		if (ca->way[entry].v[0] && ca->way[entry].tag[0] == tagaddr){
			way = 0;
		}
		else if (ca->way[entry].v[1] && ca->way[entry].tag[1] == tagaddr){
			way = 1;
		}
		else if (ca->way[entry].v[2] && ca->way[entry].tag[2] == tagaddr){
			way = 2;
		}
		else if (ca->way[entry].v[3] && ca->way[entry].tag[3] == tagaddr){
			way = 3;
		}

		if( way > -1){
#ifdef CACHE_STATICS				
			ca->read_hit_count++;
#endif			
      update_lru(way, &ca->lru[entry]);
			u8 rtn = ca->way[entry].data[way][(addr&LINE_MASK)];
			return rtn;
		}		
#ifdef CACHE_STATICS			
		ca->read_miss_count++;
#endif
		lruway = select_way_to_replace(ca->lru[entry]);
		if( lruway >= 0 ){
			update_lru(lruway, &ca->lru[entry]);
			ca->way[entry].tag[lruway] = tagaddr;
			for (i = 0; i < 16; i += 4){
				u32 odi = (addr + 4 +i) & 0xC;
				ca->way[entry].data[lruway][odi]   = ReadByteList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + odi);
        ca->way[entry].data[lruway][odi+1] = ReadByteList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + odi+1);
        ca->way[entry].data[lruway][odi+2] = ReadByteList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + odi+2);
        ca->way[entry].data[lruway][odi+3] = ReadByteList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + odi+3);
				CACHE_LOG("[SH2-%s] %d Cache miss read %08X %d:%d:%d %08X\n", CurrentSH2->isslave?"S":"M", CurrentSH2->cycles, addr, entry,lruway, odi, data);
			}
			ca->way[entry].v[lruway] = 1; //becomes valid
			return ca->way[entry].data[lruway][addr&LINE_MASK];
		}else{
			return MappedMemoryReadLongNocache(addr,cycle);	
		}


	}
	break;
	case CACHE_THROUGH:
		return MappedMemoryReadByteNocache(addr,cycle);
		break;
	default:
		return MappedMemoryReadByteNocache(addr,cycle);
		break;
	}
	return 0;
}

u16 cache_memory_read_w(cache_enty * ca, u32 addr, u32 * cycle){

	switch (addr & AREA_MASK){
	case CACHE_USE:
	{
    u32 tagaddr = 0;
    u32 entry = 0;
    int i = 0;
    int lruway = 0;
		if (ca->enable == 0){
			return MappedMemoryReadWordNocache(addr,cycle);
		}

		if ( (addr&0x01) ){
			CACHE_LOG("[%s] data alignment error for 16bit %08X\n", CurrentSH2->isslave ? "SH2-S" : "SH2-M", addr);
			addr &= ~(0x01);
		}

	  tagaddr = (addr & TAG_MASK);
		entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;

		int way = -1;
		if (ca->way[entry].v[0] && ca->way[entry].tag[0] == tagaddr){
			way = 0;
		}
		else if (ca->way[entry].v[1] && ca->way[entry].tag[1] == tagaddr){
			way = 1;
		}
		else if (ca->way[entry].v[2] && ca->way[entry].tag[2] == tagaddr){
			way = 2;
		}
		else if (ca->way[entry].v[3] && ca->way[entry].tag[3] == tagaddr){
			way = 3;
		}

		if( way > -1){
#ifdef CACHE_STATICS	
			ca->read_hit_count++;
#endif			
      update_lru(way, &ca->lru[entry]);
			u16 rtn = SWAP16( *(u16*)(&ca->way[entry].data[way][(addr&LINE_MASK)]) );
			return rtn;
		}

#ifdef CACHE_STATICS
		ca->read_miss_count++;
#endif		

		lruway = select_way_to_replace(ca->lru[entry]);
		if( lruway >= 0 ){
			update_lru(lruway, &ca->lru[entry]);
			ca->way[entry].tag[lruway] = tagaddr;
			for (i = 0; i < 16; i += 4){
				u32 odi = (addr + 4 +i) & 0xC;
				*(u16*)(&ca->way[entry].data[lruway][odi]) = SWAP16(ReadWordList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + odi));
        *(u16*)(&ca->way[entry].data[lruway][odi+2]) = SWAP16(ReadWordList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + odi+2));
				CACHE_LOG("[SH2-%s] %d Cache miss read %08X %d:%d:%d %08X\n", CurrentSH2->isslave?"S":"M", CurrentSH2->cycles, addr, entry,lruway, odi, data);
			}
			ca->way[entry].v[lruway] = 1; //becomes valid
			u16 rtn = SWAP16( *(u16*)(&ca->way[entry].data[lruway][(addr&LINE_MASK)]));
			return rtn;			
		}else{
			return MappedMemoryReadLongNocache(addr,cycle);	
		}
	
	}
	break;
	case CACHE_THROUGH:
		return MappedMemoryReadWordNocache(addr,cycle);
		break;
	default:
		return MappedMemoryReadWordNocache(addr,cycle);
		break;
	}
	return 0;
}

u32 cache_memory_read_l(cache_enty * ca, u32 addr, u32 * cycle){


  switch (addr & AREA_MASK){
	case CACHE_USE:
	{
    u32 tagaddr = 0;
    u32 entry = 0;
    int i = 0;
    int lruway = 0;
		if (ca->enable == 0){
			return MappedMemoryReadLongNocache(addr,cycle);
		}

		if ( (addr&0x03) ){
			CACHE_LOG("[%s] data alignment error for 32bit %08X\n", CurrentSH2->isslave ? "SH2-S" : "SH2-M", addr);
			addr &= ~(0x03);
		}

		tagaddr = (addr & TAG_MASK);
	  entry = (addr & ENTRY_MASK) >> ENTRY_SHIFT;

	  int way = -1;
		if (ca->way[entry].v[0] && ca->way[entry].tag[0] == tagaddr){
			way = 0;
		}
		else if (ca->way[entry].v[1] && ca->way[entry].tag[1] == tagaddr){
			way = 1;
		}
		else if (ca->way[entry].v[2] && ca->way[entry].tag[2] == tagaddr){
			way = 2;
		}
		else if (ca->way[entry].v[3] && ca->way[entry].tag[3] == tagaddr){
			way = 3;
		}

		if( way > -1){
#ifdef CACHE_STATICS			
			ca->read_hit_count++;
#endif			
      update_lru(way, &ca->lru[entry]);
			u32 rtn = SWAP32( *(u32*)(&ca->way[entry].data[way][(addr&LINE_MASK)]) );
			return rtn;
		}
#ifdef CACHE_STATICS		
		ca->read_miss_count++;
#endif
		// cache miss
		lruway = select_way_to_replace(ca->lru[entry]);
		if( lruway >= 0 ){

			update_lru(lruway, &ca->lru[entry]);
			ca->way[entry].tag[lruway] = tagaddr;
			for (i = 0; i < 16; i += 4){
				u32 odi = (addr + 4 +i) & 0xC;
				u32 data = ReadLongList[(addr >> 16) & 0xFFF]((addr & 0xFFFFFFF0) + odi);
				*(u32*)(&ca->way[entry].data[lruway][odi]) = SWAP32(data);
				CACHE_LOG("[SH2-%s] %d Cache miss read %08X %d:%d:%d %08X\n", CurrentSH2->isslave?"S":"M", CurrentSH2->cycles, addr, entry,lruway, odi, data);
			}
			ca->way[entry].v[lruway] = 1; //becomes valid

			u32 rtn = SWAP32( *(u32*)(&ca->way[entry].data[lruway][(addr&LINE_MASK)]) );
			return rtn;

		}else{
			u32 rtn = MappedMemoryReadLongNocache(addr,cycle);	
			return rtn;
		}
			

	}
	break;
	case CACHE_THROUGH:
		return MappedMemoryReadLongNocache(addr,cycle);
		break;
	default:
		return MappedMemoryReadLongNocache(addr,cycle);
		break;
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL AddressArrayReadLong(u32 addr) {
#ifdef CACHE_ENABLE
   int way = (CurrentSH2->onchip.CCR >> 6) & 3;
   int entry = (addr & 0x3FC) >> 4;
   u32 data = CurrentSH2->onchip.cache.way[entry].tag[way];
   data |= CurrentSH2->onchip.cache.lru[entry] << 4;
   data |= CurrentSH2->onchip.cache.way[entry].v[way] << 2;
	 CACHE_LOG( cache_f , "[SH2-%s] Address Read %08X %d:%d:%d %08X\n", CurrentSH2->isslave?"S":"M", addr,entry, way, addr & 0x0F , data);  
   return data;
#else
   return CurrentSH2->AddressArray[(addr & 0x3FC) >> 2];
#endif
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL AddressArrayWriteLong(u32 addr, u32 val)  {
#ifdef CACHE_ENABLE
   int way = (CurrentSH2->onchip.CCR >> 6) & 3;
   int entry = (addr & 0x3FC) >> 4;
   CurrentSH2->onchip.cache.way[entry].tag[way] = addr & 0x1FFFFC00;
   CurrentSH2->onchip.cache.way[entry].v[way] = (addr >> 2) & 1;
   CurrentSH2->onchip.cache.lru[entry] = (val >> 4) & 0x3f;
   CACHE_LOG( cache_f , "[SH2-%s] Address Write %08X %d:%d:%d %08X\n", CurrentSH2->isslave?"S":"M", addr,entry, way, addr & 0x0F , val);  
#else
   CurrentSH2->AddressArray[(addr & 0x3FC) >> 2] = val;
#endif
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL DataArrayReadByte(u32 addr) {
#ifdef CACHE_ENABLE
   int way = (addr >> 10) & 3;
   int entry = (addr >> 4) & 0x3f;
	 u8 data = CurrentSH2->onchip.cache.way[entry].data[way][addr&0xf];
	 CACHE_LOG( cache_f , "[SH2-%s] DataArrayReadByte %08X %d:%d:%d %08X\n", CurrentSH2->isslave?"S":"M", addr,entry, way, addr & 0x0F , data);  
   return data;
#else
   return T2ReadByte(CurrentSH2->DataArray, addr & 0xFFF);
#endif
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL DataArrayReadWord(u32 addr) {
#ifdef CACHE_ENABLE
   int way = (addr >> 10) & 3;
   int entry = (addr >> 4) & 0x3f;
   u16 data = ((u16)(CurrentSH2->onchip.cache.way[entry].data[way][addr&0xf]) << 8) | CurrentSH2->onchip.cache.way[entry].data[way][(addr&0xf) + 1];
	 CACHE_LOG( cache_f , "[SH2-%s] DataArrayReadWord %08X %d:%d:%d %08X\n", CurrentSH2->isslave?"S":"M", addr,entry, way, addr & 0x0F , data);  
   return data;
#else
   return T2ReadWord(CurrentSH2->DataArray, addr & 0xFFF);
#endif
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL DataArrayReadLong(u32 addr) {
#ifdef CACHE_ENABLE
   int way = (addr >> 10) & 3;
   int entry = (addr >> 4) & 0x3f;
   u32 data = ((u32)(CurrentSH2->onchip.cache.way[entry].data[way][addr&0xf]) << 24) |
      ((u32)(CurrentSH2->onchip.cache.way[entry].data[way][(addr& 0xf) + 1]) << 16) |
      ((u32)(CurrentSH2->onchip.cache.way[entry].data[way][(addr& 0xf) + 2]) << 8) |
      ((u32)(CurrentSH2->onchip.cache.way[entry].data[way][(addr& 0xf) + 3]) << 0);

	CACHE_LOG( cache_f , "[SH2-%s] DataArrayReadLong %08X %d:%d:%d %08X\n", CurrentSH2->isslave?"S":"M", addr,entry, way, addr & 0x0F , data);  
	return data;			
#else
   return T2ReadLong(CurrentSH2->DataArray, addr & 0xFFF);
#endif
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DataArrayWriteByte(u32 addr, u8 val)  {
#ifdef CACHE_ENABLE
   int way = (addr >> 10) & 3;
   int entry = (addr >> 4) & 0x3f;
   CurrentSH2->onchip.cache.way[entry].data[way][addr&0xf] = val;
#else
   T2WriteByte(CurrentSH2->DataArray, addr & 0xFFF, val);
#endif
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DataArrayWriteWord(u32 addr, u16 val)  {
#ifdef CACHE_ENABLE
   int way = (addr >> 10) & 3;
   int entry = (addr >> 4) & 0x3f;
   CurrentSH2->onchip.cache.way[entry].data[way][addr&0xf] = val >> 8;
   CurrentSH2->onchip.cache.way[entry].data[way][(addr&0xf) + 1] = val;
#else
   T2WriteWord(CurrentSH2->DataArray, addr & 0xFFF, val);
#endif
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DataArrayWriteLong(u32 addr, u32 val)  {
#ifdef CACHE_ENABLE
   int way = (addr >> 10) & 3;
   int entry = (addr >> 4) & 0x3f;
   CurrentSH2->onchip.cache.way[entry].data[way][(addr& 0xf)] = ((val >> 24) & 0xFF);
   CurrentSH2->onchip.cache.way[entry].data[way][(addr& 0xf) + 1] = ((val >> 16) & 0xFF);
   CurrentSH2->onchip.cache.way[entry].data[way][(addr& 0xf) + 2] = ((val >> 8) & 0xFF);
   CurrentSH2->onchip.cache.way[entry].data[way][(addr& 0xf) + 3] = ((val >> 0) & 0xFF);
#else
   T2WriteLong(CurrentSH2->DataArray, addr & 0xFFF, val);
#endif
}



