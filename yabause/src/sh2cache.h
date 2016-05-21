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
void cache_memory_write_b(SH2_struct *sh, cache_enty * ca, u32 addr, u8 val);
void cache_memory_write_w(SH2_struct *sh, cache_enty * ca, u32 addr, u16 val);
void cache_memory_write_l(SH2_struct *sh, cache_enty * ca, u32 addr, u32 val);
u8 cache_memory_read_b(SH2_struct *sh, cache_enty * ca, u32 addr);
u16 cache_memory_read_w(SH2_struct *sh, cache_enty * ca, u32 addr);
u32 cache_memory_read_l(SH2_struct *sh, cache_enty * ca, u32 addr);


#ifdef __cplusplus
}
#endif

#endif