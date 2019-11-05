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

typedef struct _cache_line{
	u32 tag;
   int v;
	u8 data[16];
} cache_line;

typedef struct _cache_enty{
	u32 enable;
	u32 lru[64];
	cache_line way[4][64];
} cache_enty;

#ifdef __cplusplus
extern "C"{
#endif

void cache_clear(cache_enty * ca);
void cache_enable(cache_enty * ca);
void cache_disable(cache_enty * ca);
void cache_memory_write_b(cache_enty * ca, u32 addr, u8 val);
void cache_memory_write_w(cache_enty * ca, u32 addr, u16 val);
void cache_memory_write_l(cache_enty * ca, u32 addr, u32 val);
u8 cache_memory_read_b(cache_enty * ca, u32 addr);
u16 cache_memory_read_w(cache_enty * ca, u32 addr);
u32 cache_memory_read_l(cache_enty * ca, u32 addr);


#ifdef __cplusplus
}
#endif

#endif