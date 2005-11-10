#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include "core.h"

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
   return (mem[addr] << 8) | mem[addr + 1];
#endif
}

static INLINE u32 T1ReadLong(u8 * mem, u32 addr)
{
#ifdef WORDS_BIGENDIAN
   return *((u32 *) (mem + addr));
#else
   return (mem[addr] << 24 | mem[addr + 1] << 16 |
           mem[addr + 2] << 8 | mem[addr + 3]);
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
   mem[addr] = val >> 8;
   mem[addr + 1] = val & 0xFF;
#endif
}

static INLINE void T1WriteLong(u8 * mem, u32 addr, u32 val)
{
#ifdef WORDS_BIGENDIAN
   *((u32 *) (mem + addr)) = val;
#else
   mem[addr] = val >> 24;
   mem[addr + 1] = (val >> 16) & 0xFF;
   mem[addr + 2] = (val >> 8) & 0xFF;
   mem[addr + 3] = val & 0xFF;
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
   return *((u16 *) (mem + addr)) << 16 | *((u16 *) (mem + addr + 2));
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
   *((u16 *) (mem + addr)) = val >> 16;
   *((u16 *) (mem + addr + 2)) = val & 0xFFFF;
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
   u32 filesize;
   u8 *buffer;
   u32 i;

   if ((fp = fopen(filename, "rb")) == NULL)
      return -1;

   // Calculate file size
   fseek(fp, 0, SEEK_END);
   filesize = ftell(fp);
   fseek(fp, 0, SEEK_SET);

   if (filesize > size)
      return -1;

   if ((buffer = (u8 *)malloc(filesize)) == NULL)
   {
      fclose(fp);
      return -1;
   }

   fread((void *)buffer, 1, filesize, fp);
   fclose(fp);

   switch (type)
   {
      case 1:
      {
         for (i = 0; i < filesize; i++)
            T1WriteByte(mem, i, buffer[i]);
         break;
      }
      case 2:
      {
         for (i = 0; i < filesize; i++)
            T2WriteByte(mem, i, buffer[i]);
         break;
      }
      case 3:
      {
         for (i = 0; i < filesize; i++)
            T3WriteByte(mem, i, buffer[i]);
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

   if (filename == NULL)
      return 0;

   if ((buffer = (u8 *)malloc(size)) == NULL)
      return -1;

   switch (type)
   {
      case 1:
      {
         for (i = 0; i < size; i++)
            buffer[i] = T1ReadByte(mem, i);
         break;
      }
      case 2:
      {
         for (i = 0; i < size; i++)
            buffer[i] = T2ReadByte(mem, i);
         break;
      }
      case 3:
      {
         for (i = 0; i < size; i++)
            buffer[i] = T3ReadByte(mem, i);
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

   fwrite((void *)buffer, 1, size, fp);
   fclose(fp);
   free(buffer);

   return 0;
}

/* Dummy memory, always returns 0 */

typedef void Dummy;

Dummy * DummyInit(u32);
void DummyDeInit(Dummy *);

static INLINE u8 DummyReadByte(Dummy * d, u32 a) { return 0; }
static INLINE u16 DummyReadWord(Dummy * d, u32 a) { return 0; }
static INLINE u32 DummyReadLong(Dummy * d, u32 a) { return 0; }

static INLINE void DummyWriteByte(Dummy * d, u32 a, u8 v) {}
static INLINE void DummyWriteWord(Dummy * d, u32 a, u16 v) {}
static INLINE void DummyWriteLong(Dummy * d, u32 a, u32 v) {}

void MappedMemoryInit();
u8 FASTCALL MappedMemoryReadByte(u32 addr);
u16 FASTCALL MappedMemoryReadWord(u32 addr);
u32 FASTCALL MappedMemoryReadLong(u32 addr);
void FASTCALL MappedMemoryWriteByte(u32 addr, u8 val);
void FASTCALL MappedMemoryWriteWord(u32 addr, u16 val);
void FASTCALL MappedMemoryWriteLong(u32 addr, u32 val);

extern u8 *HighWram;
extern u8 *LowWram;
extern u8 *BiosRom;
extern u8 *BupRam;

int MappedMemoryLoad(const char *filename, u32 addr);
int MappedMemorySave(const char *filename, u32 addr, u32 size);
void MappedMemoryLoadExec(const char *filename, u32 pc);

int LoadBios(const char *filename);
int LoadBackupRam(const char *filename);
void FormatBackupRam(void *mem, u32 size);

#endif
