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

#ifndef _SH2_CACHE_H_
#define _SH2_CACHE_H_

extern FILE * cache_f;
#if defined(ANDROID)
#define CACHE_LOG(...)
#else
//#define CACHE_LOG(...) if( cache_f == NULL ){ cache_f = fopen("cache.txt","w"); }  fprintf( cache_f , __VA_ARGS__);
#define CACHE_LOG(...)
#endif


//#define CACHE_STATICS

#define CCR_CE (0x01)
#define CCR_ID (0x02)
#define CCR_OD (0x04)
#define CCR_TW (0x08)
#define CCR_CP (0x10)
#define CCR_W0 (0x40)
#define CCR_W1 (0x80)

typedef struct _cache_line{
	u32 tag[4];
	u8 data[4][16];
} cache_line;

typedef struct _cache_enty{
	u32 enable;
	u32 lru[64];
	cache_line way[64];
#ifdef CACHE_STATICS	
	u32 read_hit_count;
	u32 read_miss_count;
	u32 write_count;
#endif	
} cache_enty;

#ifdef __cplusplus
extern "C"{
#endif

void cache_clear(cache_enty * ca);
void cache_enable(cache_enty * ca);
void cache_disable(cache_enty * ca);
void cache_memory_write_b(cache_enty * ca, u32 addr, u8 val,u32 * cycle );
void cache_memory_write_w(cache_enty * ca, u32 addr, u16 val,u32 * cycle);
void cache_memory_write_l(cache_enty * ca, u32 addr, u32 val,u32 * cycle);
u8 cache_memory_read_b(cache_enty * ca, u32 addr,u32 * cycle);
u16 cache_memory_read_w(cache_enty * ca, u32 addr,u32 * cycle, u32 isInst);
u32 cache_memory_read_l(cache_enty * ca, u32 addr,u32 * cycle);


#ifdef __cplusplus
}
#endif

#endif