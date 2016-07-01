/*  Copyright 2005-2006 Guillaume Duhamel
    Copyright 2005 Theo Berkau

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

#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include "core.h"

typedef struct SH2_struct SH2_struct;

/* Type 1 Memory, faster for byte (8 bits) accesses */

u8 * T1MemoryInit(u32);
void T1MemoryDeInit(u8 *);

static INLINE u8 T1ReadByte(u8 * mem, u32 addr)
{
   return mem[addr];
}

static INLINE u16 T1ReadWord(u8 * mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
   return *((u16 *) (mem + addr));
#else
   return BSWAP16L(*((u16 *) (mem + addr)));
#endif
}

static INLINE u32 T1ReadLong(u8 * mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
   return *((u32 *) (mem + addr));
#else
   return BSWAP32(*((u32 *) (mem + addr)));
#endif
}

static INLINE void T1WriteByte(u8 * mem, u32 addr, u8 val)
{
   mem[addr] = val;
}

static INLINE void T1WriteWord(u8 * mem, u32 addr, u16 val)
{
#ifdef WORDS_BIGENDIAN
   *((u16 *) (mem + addr)) = val;
#else
   *((u16 *) (mem + addr)) = BSWAP16L(val);
#endif
}

static INLINE void T1WriteLong(u8 * mem, u32 addr, u32 val)
{
#ifdef WORDS_BIGENDIAN
   *((u32 *) (mem + addr)) = val;
#else
   *((u32 *) (mem + addr)) = BSWAP32(val);
#endif
}

/* Type 2 Memory, faster for word (16 bits) accesses */

#define T2MemoryInit(x) (T1MemoryInit(x))
#define T2MemoryDeInit(x) (T1MemoryDeInit(x))

static INLINE u8 T2ReadByte(u8 * mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
   return mem[addr];
#else
   return mem[addr ^ 1];
#endif
}

static INLINE u16 T2ReadWord(u8 * mem, u32 addr)
{
   return *((u16 *) (mem + addr));
}

static INLINE u32 T2ReadLong(u8 * mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
   return *((u32 *) (mem + addr));
#else
   return WSWAP32(*((u32 *) (mem + addr)));
#endif
}

static INLINE void T2WriteByte(u8 * mem, u32 addr, u8 val)
{
#ifdef WORDS_BIGENDIAN
   mem[addr] = val;
#else
   mem[addr ^ 1] = val;
#endif
}

static INLINE void T2WriteWord(u8 * mem, u32 addr, u16 val)
{
   *((u16 *) (mem + addr)) = val;
}

static INLINE void T2WriteLong(u8 * mem, u32 addr, u32 val)
{
#ifdef WORDS_BIGENDIAN
   *((u32 *) (mem + addr)) = val;
#else
   *((u32 *) (mem + addr)) = WSWAP32(val);
#endif
}

/* Type 3 Memory, faster for long (32 bits) accesses */

typedef struct
{
   u8 * base_mem;
   u8 * mem;
} T3Memory;

T3Memory * T3MemoryInit(u32);
void T3MemoryDeInit(T3Memory *);

static INLINE u8 T3ReadByte(T3Memory * mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
	return mem->mem[addr];
#else
	return (mem->mem - addr - 1)[0];
#endif
}

static INLINE u16 T3ReadWord(T3Memory * mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
        return *((u16 *) (mem->mem + addr));
#else
	return ((u16 *) (mem->mem - addr - 2))[0];
#endif
}

static INLINE u32 T3ReadLong(T3Memory * mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
	return *((u32 *) (mem->mem + addr));
#else
	return ((u32 *) (mem->mem - addr - 4))[0];
#endif
}

static INLINE void T3WriteByte(T3Memory * mem, u32 addr, u8 val)
{
#ifdef WORDS_BIGENDIAN
	mem->mem[addr] = val;
#else
	(mem->mem - addr - 1)[0] = val;
#endif
}

static INLINE void T3WriteWord(T3Memory * mem, u32 addr, u16 val)
{
#ifdef WORDS_BIGENDIAN
	*((u16 *) (mem->mem + addr)) = val;
#else
	((u16 *) (mem->mem - addr - 2))[0] = val;
#endif
}

static INLINE void T3WriteLong(T3Memory * mem, u32 addr, u32 val)
{
#ifdef WORDS_BIGENDIAN
	*((u32 *) (mem->mem + addr)) = val;
#else
	((u32 *) (mem->mem - addr - 4))[0] = val;
#endif
}

static INLINE int T123Load(void * mem, u32 size, int type, const char *filename)
{
   FILE *fp;
   u32 filesize, filesizecheck;
   u8 *buffer;
   u32 i;

   if (!filename)
      return -1;

   if ((fp = fopen(filename, "rb")) == NULL)
      return -1;

   // Calculate file size
   fseek(fp, 0, SEEK_END);
   filesize = ftell(fp);
   fseek(fp, 0, SEEK_SET);

   if (filesize > size)
   {
      fclose(fp);
      return -1;
   }

   if ((buffer = (u8 *)malloc(filesize)) == NULL)
   {
      fclose(fp);
      return -1;
   }

   filesizecheck = (u32)fread((void *)buffer, 1, filesize, fp);
   fclose(fp);

   if (filesizecheck != filesize)
   {
      free(buffer);
      return -1;
   }

   switch (type)
   {
      case 1:
      {
         for (i = 0; i < filesize; i++)
            T1WriteByte((u8 *) mem, i, buffer[i]);
         break;
      }
      case 2:
      {
         for (i = 0; i < filesize; i++)
            T2WriteByte((u8 *) mem, i, buffer[i]);
         break;
      }
      case 3:
      {
         for (i = 0; i < filesize; i++)
            T3WriteByte((T3Memory *) mem, i, buffer[i]);
         break;
      }
      default:
      {
         free(buffer);
         return -1;
      }
   }

   free(buffer);

   return 0;
}

static INLINE int T123Save(void * mem, u32 size, int type, const char *filename)
{
   FILE *fp;
   u8 *buffer;
   u32 i;
   u32 sizecheck;

   if (filename == NULL)
      return 0;

   if (filename[0] == 0x00)
      return 0;

   if ((buffer = (u8 *)malloc(size)) == NULL)
      return -1;

   switch (type)
   {
      case 1:
      {
         for (i = 0; i < size; i++)
            buffer[i] = T1ReadByte((u8 *) mem, i);
         break;
      }
      case 2:
      {
         for (i = 0; i < size; i++)
            buffer[i] = T2ReadByte((u8 *) mem, i);
         break;
      }
      case 3:
      {
         for (i = 0; i < size; i++)
            buffer[i] = T3ReadByte((T3Memory *) mem, i);
         break;
      }
      default:
      {
         free(buffer);
         return -1;
      }
   }

   if ((fp = fopen(filename, "wb")) == NULL)
   {
      free(buffer);
      return -1;
   }

   sizecheck = (u32)fwrite((void *)buffer, 1, size, fp);
   fclose(fp);
   free(buffer);

   if (sizecheck != size) return -1;

   return 0;
}

/* Dummy memory, always returns 0 */

typedef void Dummy;

Dummy * DummyInit(u32);
void DummyDeInit(Dummy *);

static INLINE u8 DummyReadByte(Dummy UNUSED * d, u32 UNUSED a) { return 0; }
static INLINE u16 DummyReadWord(Dummy UNUSED * d, u32 UNUSED a) { return 0; }
static INLINE u32 DummyReadLong(Dummy UNUSED * d, u32 UNUSED a) { return 0; }

static INLINE void DummyWriteByte(Dummy UNUSED * d, u32 UNUSED a, u8 UNUSED v) {}
static INLINE void DummyWriteWord(Dummy UNUSED * d, u32 UNUSED a, u16 UNUSED v) {}
static INLINE void DummyWriteLong(Dummy UNUSED * d, u32 UNUSED a, u32 UNUSED v) {}

#ifdef __cplusplus
extern "C" {
#endif
void MappedMemoryInit(SH2_struct *msh1, SH2_struct *ssh2, SH2_struct *sh1);

u8 FASTCALL MappedMemoryReadByteCacheEnabled(SH2_struct *sh, u32 addr);
u16 FASTCALL MappedMemoryReadWordCacheEnabled(SH2_struct *sh, u32 addr);
u32 FASTCALL MappedMemoryReadLongCacheEnabled(SH2_struct *sh, u32 addr);
void FASTCALL MappedMemoryWriteByteCacheEnabled(SH2_struct *sh, u32 addr, u8 val);
void FASTCALL MappedMemoryWriteWordCacheEnabled(SH2_struct *sh, u32 addr, u16 val);
void FASTCALL MappedMemoryWriteLongCacheEnabled(SH2_struct *sh, u32 addr, u32 val);

u8 FASTCALL MappedMemoryReadByteNocache(SH2_struct *sh, u32 addr);
u16 FASTCALL MappedMemoryReadWordNocache(SH2_struct *sh, u32 addr);
u32 FASTCALL MappedMemoryReadLongNocache(SH2_struct *sh, u32 addr);
void FASTCALL MappedMemoryWriteByteNocache(SH2_struct *sh, u32 addr, u8 val);
void FASTCALL MappedMemoryWriteWordNocache(SH2_struct *sh, u32 addr, u16 val);
void FASTCALL MappedMemoryWriteLongNocache(SH2_struct *sh, u32 addr, u32 val);
#ifdef __cplusplus
}
#endif

extern u8 *SH1Rom;
extern u8 *SH1Dram;
extern u8 *SH1MpegRom;
#ifdef __cplusplus
extern "C" {
#endif
extern u8 *HighWram;
#ifdef __cplusplus
}
#endif
extern u8 *LowWram;
extern u8 *BiosRom;
extern u8 *BupRam;
extern u8 BupRamWritten;

typedef void (FASTCALL *writebytefunc)(SH2_struct *, u32, u8);
typedef void (FASTCALL *writewordfunc)(SH2_struct *, u32, u16);
typedef void (FASTCALL *writelongfunc)(SH2_struct *, u32, u32);

typedef u8 (FASTCALL *readbytefunc)(SH2_struct *, u32);
typedef u16 (FASTCALL *readwordfunc)(SH2_struct *, u32);
typedef u32 (FASTCALL *readlongfunc)(SH2_struct *, u32);

typedef struct {
u32 addr;
u32 val;
} result_struct;

#define SEARCHBYTE              0
#define SEARCHWORD              1
#define SEARCHLONG              2

#define SEARCHEXACT             (0 << 2)
#define SEARCHLESSTHAN          (1 << 2)
#define SEARCHGREATERTHAN       (2 << 2)

#define SEARCHUNSIGNED          (0 << 4)
#define SEARCHSIGNED            (1 << 4)
#define SEARCHHEX               (2 << 4)
#define SEARCHSTRING            (3 << 4)
#define SEARCHREL8BIT           (6 << 4)
#define SEARCHREL16BIT          (7 << 4)

result_struct *MappedMemorySearch(u32 startaddr, u32 endaddr, int searchtype,
                                  const char *searchstr,
                                  result_struct *prevresults, u32 *maxresults);

int MappedMemoryLoad(SH2_struct *sh, const char *filename, u32 addr);
int MappedMemorySave(SH2_struct *sh, const char *filename, u32 addr, u32 size);
void MappedMemoryLoadExec(const char *filename, u32 pc);

int LoadSH1Rom(const char *filename);
int LoadMpegRom(const char *filename);
int LoadBios(const char *filename);
int LoadBackupRam(const char *filename);
void FormatBackupRam(void *mem, u32 size);

int YabSaveState(const char *filename);
int YabLoadState(const char *filename);
int YabSaveStateSlot(const char *dirpath, u8 slot);
int YabLoadStateSlot(const char *dirpath, u8 slot);
int YabSaveStateStream(FILE *stream);
int YabLoadStateStream(FILE *stream);
int YabSaveStateBuffer(void **buffer, size_t *size);
int YabLoadStateBuffer(const void *buffer, size_t size);


u8 FASTCALL UnhandledMemoryReadByte(USED_IF_DEBUG u32 addr);
u16 FASTCALL UnhandledMemoryReadWord(USED_IF_DEBUG u32 addr);
u32 FASTCALL UnhandledMemoryReadLong(USED_IF_DEBUG u32 addr);
void FASTCALL UnhandledMemoryWriteByte(USED_IF_DEBUG u32 addr, UNUSED u8 val);
void FASTCALL UnhandledMemoryWriteWord(USED_IF_DEBUG u32 addr, UNUSED u16 val);
void FASTCALL UnhandledMemoryWriteLong(USED_IF_DEBUG u32 addr, UNUSED u32 val);
u8 FASTCALL HighWramMemoryReadByte(u32 addr);
u16 FASTCALL HighWramMemoryReadWord(u32 addr);
u32 FASTCALL HighWramMemoryReadLong(u32 addr);
void FASTCALL HighWramMemoryWriteByte(u32 addr, u8 val);
void FASTCALL HighWramMemoryWriteWord(u32 addr, u16 val);
void FASTCALL HighWramMemoryWriteLong(u32 addr, u32 val);
u8 FASTCALL LowWramMemoryReadByte(u32 addr);
u16 FASTCALL LowWramMemoryReadWord(u32 addr);
u32 FASTCALL LowWramMemoryReadLong(u32 addr);
void FASTCALL LowWramMemoryWriteByte(u32 addr, u8 val);
void FASTCALL LowWramMemoryWriteWord(u32 addr, u16 val);
void FASTCALL LowWramMemoryWriteLong(u32 addr, u32 val);
u8 FASTCALL BiosRomMemoryReadByte(u32 addr);
u16 FASTCALL BiosRomMemoryReadWord(u32 addr);
u32 FASTCALL BiosRomMemoryReadLong(u32 addr);
void FASTCALL BiosRomMemoryWriteByte(UNUSED u32 addr, UNUSED u8 val);
void FASTCALL BiosRomMemoryWriteWord(UNUSED u32 addr, UNUSED u16 val);
void FASTCALL BiosRomMemoryWriteLong(UNUSED u32 addr, UNUSED u32 val);
u8 FASTCALL BupRamMemoryReadByte(u32 addr);
u16 FASTCALL BupRamMemoryReadWord(USED_IF_DEBUG u32 addr);
u32 FASTCALL BupRamMemoryReadLong(USED_IF_DEBUG u32 addr);
void FASTCALL BupRamMemoryWriteByte(u32 addr, u8 val);
void FASTCALL BupRamMemoryWriteWord(USED_IF_DEBUG u32 addr, UNUSED u16 val);
void FASTCALL BupRamMemoryWriteLong(USED_IF_DEBUG u32 addr, UNUSED u32 val);

#if 0
u8 FASTCALL UnhandledMemoryReadByte(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr);
u16 FASTCALL UnhandledMemoryReadWord(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr);
u32 FASTCALL UnhandledMemoryReadLong(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr);
void FASTCALL UnhandledMemoryWriteByte(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr, UNUSED u8 val);
void FASTCALL UnhandledMemoryWriteWord(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr, UNUSED u16 val);
void FASTCALL UnhandledMemoryWriteLong(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr, UNUSED u32 val);
u8 FASTCALL HighWramMemoryReadByte(SH2_struct *sh, u32 addr);
u16 FASTCALL HighWramMemoryReadWord(SH2_struct *sh, u32 addr);
u32 FASTCALL HighWramMemoryReadLong(SH2_struct *sh, u32 addr);
void FASTCALL HighWramMemoryWriteByte(SH2_struct *sh, u32 addr, u8 val);
void FASTCALL HighWramMemoryWriteWord(SH2_struct *sh, u32 addr, u16 val);
void FASTCALL HighWramMemoryWriteLong(SH2_struct *sh, u32 addr, u32 val);
u8 FASTCALL LowWramMemoryReadByte(SH2_struct *sh, u32 addr);
u16 FASTCALL LowWramMemoryReadWord(SH2_struct *sh, u32 addr);
u32 FASTCALL LowWramMemoryReadLong(SH2_struct *sh, u32 addr);
void FASTCALL LowWramMemoryWriteByte(SH2_struct *sh, u32 addr, u8 val);
void FASTCALL LowWramMemoryWriteWord(SH2_struct *sh, u32 addr, u16 val);
void FASTCALL LowWramMemoryWriteLong(SH2_struct *sh, u32 addr, u32 val);
u8 FASTCALL BiosRomMemoryReadByte(SH2_struct *sh, u32 addr);
u16 FASTCALL BiosRomMemoryReadWord(SH2_struct *sh, u32 addr);
u32 FASTCALL BiosRomMemoryReadLong(SH2_struct *sh, u32 addr);
static void FASTCALL BiosRomMemoryWriteByte(UNUSED SH2_struct *sh, UNUSED u32 addr, UNUSED u8 val);
static void FASTCALL BiosRomMemoryWriteWord(UNUSED SH2_struct *sh, UNUSED u32 addr, UNUSED u16 val);
void FASTCALL BiosRomMemoryWriteLong(UNUSED SH2_struct *sh, UNUSED u32 addr, UNUSED u32 val);
static u8 FASTCALL BupRamMemoryReadByte(SH2_struct *sh, u32 addr);
u16 FASTCALL BupRamMemoryReadWord(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr);
u32 FASTCALL BupRamMemoryReadLong(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr);
void FASTCALL BupRamMemoryWriteByte(SH2_struct *sh, u32 addr, u8 val);
void FASTCALL BupRamMemoryWriteWord(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr, UNUSED u16 val);
void FASTCALL BupRamMemoryWriteLong(UNUSED SH2_struct *sh, USED_IF_DEBUG u32 addr, UNUSED u32 val);
#endif


#endif
