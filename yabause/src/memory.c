/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau

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

/*! \file memory.c
    \brief Memory access functions.
*/

#ifdef PSP  // see FIXME in T1MemoryInit()
# include <stdint.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>

#include "memory.h"
#include "coffelf.h"
#include "cs0.h"
#include "cs1.h"
#include "cs2.h"
#include "debug.h"
#include "error.h"
#include "sh2core.h"
#include "scsp.h"
#include "scu.h"
#include "smpc.h"
#include "vdp1.h"
#include "vdp2.h"
#include "yabause.h"
#include "yui.h"
#include "movie.h"
#include "bios.h"
#include "peripheral.h"

//#ifdef HAVE_LIBGL
//#define USE_OPENGL
//#endif

#ifdef USE_OPENGL
#include "ygl.h"
#endif

#include "vidsoft.h"
#include "vidogl.h"

//////////////////////////////////////////////////////////////////////////////

u8** MemoryBuffer[0x1000];

writebytefunc WriteByteList[0x1000];
writewordfunc WriteWordList[0x1000];
writelongfunc WriteLongList[0x1000];

readbytefunc ReadByteList[0x1000];
readwordfunc ReadWordList[0x1000];
readlongfunc ReadLongList[0x1000];

readbytefunc CacheReadByteList[0x1000];
readwordfunc CacheReadWordList[0x1000];
readlongfunc CacheReadLongList[0x1000];
writebytefunc CacheWriteByteList[0x1000];
writewordfunc CacheWriteWordList[0x1000];
writelongfunc CacheWriteLongList[0x1000];

#define EXTENDED_BACKUP_SIZE 0x00800000
#define EXTENDED_BACKUP_ADDR 0x08000000

u32 backup_file_addr = EXTENDED_BACKUP_ADDR;
u32 backup_file_size = EXTENDED_BACKUP_SIZE;
static const char *bupfilename = NULL;

u8 *HighWram;
u8 *LowWram;
u8 *BiosRom;
u8 *BupRam;

u8* VoidMem = NULL;

/* This flag is set to 1 on every write to backup RAM.  Ports can freely
 * check or clear this flag to determine when backup RAM has been written,
 * e.g. for implementing autosave of backup RAM. */
u8 BupRamWritten;

#if defined(__GNUC__)

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static u32 mmapsize = 0;

void * YabMemMap(char * filename, u32 size ) {

  struct stat sb;
  off_t len;
  char *p;
  int fd;

  fd = open(filename, O_RDONLY);
  if (fd ==-1) {
    perror("open");
    return NULL;
   }

  if (fstat(fd, &sb) ==-1) {
    perror("fstat");
    return NULL;
  }

  if (!S_ISREG(sb.st_mode)) {
    fprintf(stderr, "%s is not a file\n", filename);
    return NULL;
  }

  p = mmap(0, sb.st_size, PROT_READ| PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }

  if (close(fd) == -1) {
    perror("close");
    return NULL;
  }

  mmapsize = sb.st_size;
  return p;
}

void YabFreeMap(void * p) {
  munmap(p, mmapsize);
}


#elif defined(_WINDOWS)

#include <windows.h>
HANDLE hFMWrite = INVALID_HANDLE_VALUE;
HANDLE hFile = INVALID_HANDLE_VALUE;
void * YabMemMap(char * filename, u32 size ) {

  struct stat sb;
  off_t len;
  char *p;
  int fd;

  hFile = CreateFileA(
    filename, 
    GENERIC_READ|GENERIC_WRITE, 
    0, 
    0, 
    OPEN_EXISTING, 
    FILE_ATTRIBUTE_NORMAL,
    0);
  if (INVALID_HANDLE_VALUE == hFile) {
    DWORD errorMessageID = GetLastError();
    LPSTR messageBuffer = NULL;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    LOG(messageBuffer);
    return NULL;
  }

  hFMWrite = CreateFileMapping(
    hFile,
    NULL,
    PAGE_READWRITE,
    0,
    size,
    "BACKUP");
  if (hFMWrite == INVALID_HANDLE_VALUE)
    return NULL;

  return MapViewOfFile(hFMWrite, FILE_MAP_ALL_ACCESS, 0, 0, size);
}

void YabFreeMap(void * p) {
  UnmapViewOfFile(p);
  CloseHandle(hFMWrite);
  CloseHandle(hFile);
  hFMWrite = NULL;
}

#else
void * YabMemMap(char * filename) {
  return NULL;
}
#endif


//////////////////////////////////////////////////////////////////////////////

u8 * T1MemoryInit(u32 size)
{
#ifdef PSP  // FIXME: could be ported to all arches, but requires stdint.h
            //        for uintptr_t
   u8 * base;
   u8 * mem;

   if ((base = calloc((size * sizeof(u8)) + sizeof(u8 *) + 64, 1)) == NULL)
      return NULL;

   mem = base + sizeof(u8 *);
   mem = mem + (64 - ((uintptr_t) mem & 63));
   *(u8 **)(mem - sizeof(u8 *)) = base; // Save base pointer below memory block

   return mem;
#else
   return calloc(size, sizeof(u8));
#endif
}

//////////////////////////////////////////////////////////////////////////////

void T1MemoryDeInit(u8 * mem)
{
#ifdef PSP
   if (mem)
      free(*(u8 **)(mem - sizeof(u8 *)));
#else
   free(mem);
#endif
}

//////////////////////////////////////////////////////////////////////////////

T3Memory * T3MemoryInit(u32 size)
{
   T3Memory * mem;

   if ((mem = (T3Memory *) calloc(1, sizeof(T3Memory))) == NULL)
      return NULL;

   if ((mem->base_mem = (u8 *) calloc(size, sizeof(u8))) == NULL)
   {
      free(mem);
      return NULL;
   }

   mem->mem = mem->base_mem + size;

   return mem;
}

//////////////////////////////////////////////////////////////////////////////

void T3MemoryDeInit(T3Memory * mem)
{
   free(mem->base_mem);
   free(mem);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL UnhandledMemoryReadByte(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
   LOG("Unhandled byte read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL UnhandledMemoryReadWord(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
   LOG("Unhandled word read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL UnhandledMemoryReadLong(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
   LOG("Unhandled long read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteByte(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u8 val)
{
   LOG("Unhandled byte write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteWord(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u16 val)
{
   LOG("Unhandled word write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteLong(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u32 val)
{
   LOG("Unhandled long write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL HighWramMemoryReadByte(SH2_struct *context, u8* mem, u32 addr)
{
   return T2ReadByte(mem, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL HighWramMemoryReadWord(SH2_struct *context, u8* mem, u32 addr)
{
//printf("hi rw %x\n", addr);
   return T2ReadWord(mem, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL HighWramMemoryReadLong(SH2_struct *context, u8* mem, u32 addr)
{
   return T2ReadLong(mem, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL HighWramMemoryWriteByte(SH2_struct *context, u8* mem, u32 addr, u8 val)
{
   T2WriteByte(mem, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL HighWramMemoryWriteWord(SH2_struct *context, u8* mem, u32 addr, u16 val)
{
   T2WriteWord(mem, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL HighWramMemoryWriteLong(SH2_struct *context, u8* mem, u32 addr, u32 val)
{
   T2WriteLong(mem, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL LowWramMemoryReadByte(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
   return T2ReadByte(memory, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL LowWramMemoryReadWord(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
   return T2ReadWord(memory, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL LowWramMemoryReadLong(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
   return T2ReadLong(memory, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL LowWramMemoryWriteByte(SH2_struct *context, UNUSED u8* memory, u32 addr, u8 val)
{
   T2WriteByte(memory, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL LowWramMemoryWriteWord(SH2_struct *context, UNUSED u8* memory, u32 addr, u16 val)
{
   T2WriteWord(memory, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL LowWramMemoryWriteLong(SH2_struct *context, UNUSED u8* memory, u32 addr, u32 val)
{
   T2WriteLong(memory, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL BiosRomMemoryReadByte(SH2_struct *context, UNUSED u8* memory,  u32 addr)
{
   return T2ReadByte(memory, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL BiosRomMemoryReadWord(SH2_struct *context, u8* memory, u32 addr)
{
   return T2ReadWord(memory, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL BiosRomMemoryReadLong(SH2_struct *context, u8* memory, u32 addr)
{
   return T2ReadLong(memory, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BiosRomMemoryWriteByte(SH2_struct *context, UNUSED u8* memory,UNUSED u32 addr, UNUSED u8 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BiosRomMemoryWriteWord(SH2_struct *context, UNUSED u8* memory,UNUSED u32 addr, UNUSED u16 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BiosRomMemoryWriteLong(SH2_struct *context, UNUSED u8* memory,UNUSED u32 addr, UNUSED u32 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL BupRamMemoryReadByte(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
#if 0
  // maped memory is better
  if (BupRam == NULL) {
    if ((addr&0x0FFFFFFF) >= tweak_backup_file_addr) {
      addr = (addr & 0x0FFFFFFF) - tweak_backup_file_addr;
      if (addr >= tweak_backup_file_size) {
        return 0;
      }
    }
    else {
      addr = (addr & 0xFFFF);
    }
    fseek(pbackup, addr, 0);
    return fgetc(pbackup);
  }
#endif
  addr = addr & (backup_file_size - 1);
  return T1ReadByte(memory, addr);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL BupRamMemoryReadWord(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
   LOG("bup\t: BackupRam read word - %08X\n", addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL BupRamMemoryReadLong(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
   LOG("bup\t: BackupRam read long - %08X\n", addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
static void FASTCALL BupRamMemoryWriteByte(SH2_struct *context, UNUSED u8* memory, u32 addr, u8 val)
{
#if 0
  // maped memory is better
  if (BupRam == NULL) {
    if ((addr & 0x0FFFFFFF) >= tweak_backup_file_addr) {
      addr = ((addr & 0x0FFFFFFF) - tweak_backup_file_addr)|0x01;
    }else {
      addr = (addr & 0xFFFF) | 0x1;
    }
    fseek(pbackup, addr, 0);
    fputc(val, pbackup);
    fflush(pbackup);
  }
  else {
    T1WriteByte(BupRam, (addr & 0xFFFF) | 0x1, val);
    BupRamWritten = 1;  
  }
#endif

  addr = addr & (backup_file_size - 1);
  T1WriteByte(memory, addr|0x1, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BupRamMemoryWriteWord(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u16 val)
{
   LOG("bup\t: BackupRam write word - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BupRamMemoryWriteLong(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u32 val)
{
   LOG("bup\t: BackupRam write long - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FillMemoryArea(unsigned short start, unsigned short end,
                           readbytefunc r8func, readwordfunc r16func,
                           readlongfunc r32func, writebytefunc w8func,
                           writewordfunc w16func, writelongfunc w32func, u8** memory)
{
   int i;

   for (i=start; i < (end+1); i++)
   {
      MemoryBuffer[i] = memory;
      ReadByteList[i] = r8func;
      ReadWordList[i] = r16func;
      ReadLongList[i] = r32func;
      WriteByteList[i] = w8func;
      WriteWordList[i] = w16func;
      WriteLongList[i] = w32func;

      CacheReadByteList[i] = r8func;
      CacheReadWordList[i] = r16func;
      CacheReadLongList[i] = r32func;
      CacheWriteByteList[i] = w8func;
      CacheWriteWordList[i] = w16func;
      CacheWriteLongList[i] = w32func;
   }
}

//////////////////////////////////////////////////////////////////////////////

void MappedMemoryInit()
{
   // Initialize everyting to unhandled to begin with
   FillMemoryArea(0x000, 0xFFF, &UnhandledMemoryReadByte,
                                &UnhandledMemoryReadWord,
                                &UnhandledMemoryReadLong,
                                &UnhandledMemoryWriteByte,
                                &UnhandledMemoryWriteWord,
                                &UnhandledMemoryWriteLong,
                                &VoidMem);

   // Fill the rest
   FillMemoryArea(0x000, 0x00F, &BiosRomMemoryReadByte,
                                &BiosRomMemoryReadWord,
                                &BiosRomMemoryReadLong,
                                &BiosRomMemoryWriteByte,
                                &BiosRomMemoryWriteWord,
                                &BiosRomMemoryWriteLong,
                                &BiosRom);
   FillMemoryArea(0x010, 0x017, &SmpcReadByte,
                                &SmpcReadWord,
                                &SmpcReadLong,
                                &SmpcWriteByte,
                                &SmpcWriteWord,
                                &SmpcWriteLong,
                                &VoidMem);
   FillMemoryArea(0x020, 0x02F, &LowWramMemoryReadByte,
                                &LowWramMemoryReadWord,
                                &LowWramMemoryReadLong,
                                &LowWramMemoryWriteByte,
                                &LowWramMemoryWriteWord,
                                &LowWramMemoryWriteLong,
                                &LowWram);
   FillMemoryArea(0x040, 0x041, &IOPortReadByte, 
                                &UnhandledMemoryReadWord,
                                &UnhandledMemoryReadLong,
                                &IOPortWriteByte,
                                &UnhandledMemoryWriteWord,
                                &UnhandledMemoryWriteLong,
                                &VoidMem);
   FillMemoryArea(0x100, 0x17F, &UnhandledMemoryReadByte,
                                &UnhandledMemoryReadWord,
                                &UnhandledMemoryReadLong,
                                &UnhandledMemoryWriteByte,
                                &SSH2InputCaptureWriteWord,
                                &UnhandledMemoryWriteLong,
                                &VoidMem);
   FillMemoryArea(0x180, 0x1FF, &UnhandledMemoryReadByte,
                                &UnhandledMemoryReadWord,
                                &UnhandledMemoryReadLong,
                                &UnhandledMemoryWriteByte,
                                &MSH2InputCaptureWriteWord,
                                &UnhandledMemoryWriteLong,
                                &VoidMem);
   FillMemoryArea(0x200, 0x3FF, CartridgeArea->Cs0ReadByte,
                                CartridgeArea->Cs0ReadWord,
                                CartridgeArea->Cs0ReadLong,
                                CartridgeArea->Cs0WriteByte,
                                CartridgeArea->Cs0WriteWord,
                                CartridgeArea->Cs0WriteLong,
                                &CartridgeArea->rom); //A voir ici
   FillMemoryArea(0x400, 0x4FF, &Cs1ReadByte,
                                &Cs1ReadWord,
                                &Cs1ReadLong,
                                &Cs1WriteByte,
                                &Cs1WriteWord,
                                &Cs1WriteLong,
                                &VoidMem); //A voir ici
   FillMemoryArea(0x580, 0x58F, &Cs2ReadByte,
                                &Cs2ReadWord,
                                &Cs2ReadLong,
                                &Cs2WriteByte,
                                &Cs2WriteWord,
                                &Cs2WriteLong,
                                &VoidMem); //A voir ici
   FillMemoryArea(0x5A0, 0x5AF, &SoundRamReadByte,
                                &SoundRamReadWord,
                                &SoundRamReadLong,
                                &SoundRamWriteByte,
                                &SoundRamWriteWord,
                                &SoundRamWriteLong,
                                &SoundRam);
   FillMemoryArea(0x5B0, 0x5BF, &scsp_r_b,
                                &scsp_r_w,
                                &scsp_r_d,
                                &scsp_w_b,
                                &scsp_w_w,
                                &scsp_w_d,
                                &VoidMem);
   FillMemoryArea(0x5C0, 0x5C7, &Vdp1RamReadByte,
                                &Vdp1RamReadWord,
                                &Vdp1RamReadLong,
                                &Vdp1RamWriteByte,
                                &Vdp1RamWriteWord,
                                &Vdp1RamWriteLong,
                                &Vdp1Ram);
   FillMemoryArea(0x5C8, 0x5CF, &Vdp1FrameBufferReadByte,
                                &Vdp1FrameBufferReadWord,
                                &Vdp1FrameBufferReadLong,
                                &Vdp1FrameBufferWriteByte,
                                &Vdp1FrameBufferWriteWord,
                                &Vdp1FrameBufferWriteLong,
                                &VoidMem);
   FillMemoryArea(0x5D0, 0x5D7, &Vdp1ReadByte,
                                &Vdp1ReadWord,
                                &Vdp1ReadLong,
                                &Vdp1WriteByte,
                                &Vdp1WriteWord,
                                &Vdp1WriteLong,
                                &VoidMem);
   FillMemoryArea(0x5E0, 0x5EF, &Vdp2RamReadByte,
                                &Vdp2RamReadWord,
                                &Vdp2RamReadLong,
                                &Vdp2RamWriteByte,
                                &Vdp2RamWriteWord,
                                &Vdp2RamWriteLong,
                                &Vdp2Ram);
   FillMemoryArea(0x5F0, 0x5F7, &Vdp2ColorRamReadByte,
                                &Vdp2ColorRamReadWord,
                                &Vdp2ColorRamReadLong,
                                &Vdp2ColorRamWriteByte,
                                &Vdp2ColorRamWriteWord,
                                &Vdp2ColorRamWriteLong,
                                &Vdp2ColorRam);
   FillMemoryArea(0x5F8, 0x5FB, &Vdp2ReadByte,
                                &Vdp2ReadWord,
                                &Vdp2ReadLong,
                                &Vdp2WriteByte,
                                &Vdp2WriteWord,
                                &Vdp2WriteLong,
                                &VoidMem);
   FillMemoryArea(0x5FE, 0x5FE, &ScuReadByte,
                                &ScuReadWord,
                                &ScuReadLong,
                                &ScuWriteByte,
                                &ScuWriteWord,
                                &ScuWriteLong,
                                &VoidMem);
   FillMemoryArea(0x600,  0x7FF, &HighWramMemoryReadByte,
                                &HighWramMemoryReadWord,
                                &HighWramMemoryReadLong,
                                &HighWramMemoryWriteByte,
                                &HighWramMemoryWriteWord,
                                &HighWramMemoryWriteLong,
                                &HighWram);

     FillMemoryArea( ((backup_file_addr >> 16) & 0xFFF) , (((backup_file_addr + backup_file_size) >> 16) & 0xFFF)+1, &BupRamMemoryReadByte,
     &BupRamMemoryReadWord,
     &BupRamMemoryReadLong,
     &BupRamMemoryWriteByte,
     &BupRamMemoryWriteWord,
     &BupRamMemoryWriteLong,
     &BupRam);
}

u8 FASTCALL DMAMappedMemoryReadByte(SH2_struct *context, u32 addr) {
  u8 ret;
  ret = MappedMemoryReadByte(context, addr);
return ret;
}
//////////////////////////////////////////////////////////////////////////////
u8 FASTCALL MappedMemoryReadByte(SH2_struct *context, u32 addr)
{
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      {
        return ReadByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadByte(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled Byte R %x\n", addr);
         return UnhandledMemoryReadByte(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }

   return 0;
}
u8 FASTCALL SH2MappedMemoryReadByte(SH2_struct *context, u32 addr) {
CACHE_LOG("rb %x %x\n", addr, addr >> 29);
   switch (addr >> 29)
   {
      case 0x1:
      {
        return ReadByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x0:
      case 0x4:
      {
         return CacheReadByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x2:
         return UnhandledMemoryReadLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      //case 0x4:
      case 0x6:
         // Data Array
       LOG("DAta Byte R %x\n", addr);
         return DataArrayReadByte(context, addr);
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadByte(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled Byte R %x\n", addr);
         return UnhandledMemoryReadByte(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }

   return 0;
}


u16 FASTCALL DMAMappedMemoryReadWord(SH2_struct *context, u32 addr) {
  return MappedMemoryReadWord(context, addr);
}

//////////////////////////////////////////////////////////////////////////////
u16 FASTCALL MappedMemoryReadWord(SH2_struct *context, u32 addr)
{
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      {
        return ReadWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x2:
         return UnhandledMemoryReadLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadWord(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled Word R %x\n", addr);
         return UnhandledMemoryReadWord(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }

   return 0;
}


u16 FASTCALL SH2MappedMemoryReadWord(SH2_struct *context, u32 addr)
{
   switch (addr >> 29)
   {
      case 0x1:
      {
        return ReadWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x0: //0x0 cache
           return CacheReadWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      case 0x2:
         return UnhandledMemoryReadLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      //case 0x4:
      case 0x6:
         // Data Array
       CACHE_LOG("DAta Word R %x\n", addr);
         return DataArrayReadWord(context, addr);
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadWord(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled Word R %x\n", addr);
         return UnhandledMemoryReadWord(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }

   return 0;
}

u32 FASTCALL DMAMappedMemoryReadLong(SH2_struct *context, u32 addr)
{
  return MappedMemoryReadLong(context, addr);
}
//////////////////////////////////////////////////////////////////////////////
u32 FASTCALL MappedMemoryReadLong(SH2_struct *context, u32 addr)
{
  switch (addr >> 29)
   {
      case 0x0:
      case 0x1: //0x0 no cache
      {
        return ReadLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadLong(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled Long R %x %d 0x%x\n", addr, (addr >> 29), (addr >> 16) & 0xFFF);
         return UnhandledMemoryReadLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }
   return 0;
}

u32 FASTCALL SH2MappedMemoryReadLong(SH2_struct *context, u32 addr)
{
   switch (addr >> 29)
   {
      case 0x1: //0x0 no cache
      {
        return ReadLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x0:
      {
         return CacheReadLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x2:
LOG("Unhandled SH2 Memory Long %d\n", (addr >> 29));
         return UnhandledMemoryReadLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      case 0x3:
      {
         // Address Array
         return AddressArrayReadLong(context, addr);
      }
      //case 0x4:
      case 0x6:
         // Data Array
       CACHE_LOG("Data Long R %x\n", addr);
         return DataArrayReadLong(context, addr);
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadLong(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled SH2 Long R %x %d\n", addr,(addr >> 29));
         return UnhandledMemoryReadLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }
   return 0;
}

void FASTCALL DMAMappedMemoryWriteByte(SH2_struct *context, u32 addr, u8 val)
{
   MappedMemoryWriteByte(context, addr, val);
}

//////////////////////////////////////////////////////////////////////////////
void FASTCALL MappedMemoryWriteByte(SH2_struct *context, u32 addr, u8 val)
{
   SH2WriteNotify(context, addr, 1);
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      {
        WriteByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteByte(context, addr, val);
            return; 
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Byte W %x\n", addr);
         UnhandledMemoryWriteByte(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

void FASTCALL SH2MappedMemoryWriteByte(SH2_struct *context, u32 addr, u8 val)
{
   SH2WriteNotify(context, addr, 1);
   switch (addr >> 29)
   {
      case 0x1:
      {
        WriteByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x0:
      {
CACHE_LOG("wb %x %x\n", addr, addr >> 29);
         CacheWriteByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }

      case 0x2:
      {
         // Purge Area
         CacheInvalidate(context, addr);
         return;
      }

      //case 0x4:
      case 0x6:
         // Data Array
         DataArrayWriteByte(context, addr, val);
         return;
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteByte(context, addr, val);
            return; 
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Byte W %x\n", addr);
         UnhandledMemoryWriteByte(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

void FASTCALL DMAMappedMemoryWriteWord(SH2_struct *context, u32 addr, u16 val)
{
   MappedMemoryWriteWord(context, addr, val);
}

//////////////////////////////////////////////////////////////////////////////
void FASTCALL MappedMemoryWriteWord(SH2_struct *context, u32 addr, u16 val)
{
   SH2WriteNotify(context, addr, 2);
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      {
        WriteWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteWord(context, addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM setup 
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Word W %x\n", addr);
         UnhandledMemoryWriteWord(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

void FASTCALL SH2MappedMemoryWriteWord(SH2_struct *context, u32 addr, u16 val)
{
   SH2WriteNotify(context, addr, 2);
   switch (addr >> 29)
   {
      case 0x1:
      {
        WriteWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x0:
      {
CACHE_LOG("ww %x %x\n", addr, addr >> 29);
         // Cache/Non-Cached
         CacheWriteWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }

      case 0x2:
      {
         // Purge Area
         CacheInvalidate(context, addr);
         return;
      }

      //case 0x4:
      case 0x6:
         // Data Array

         DataArrayWriteWord(context, addr, val);
         return;
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteWord(context, addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM setup 
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Word W %x\n", addr);
         UnhandledMemoryWriteWord(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

void FASTCALL DMAMappedMemoryWriteLong(SH2_struct *context, u32 addr, u32 val)
{
   MappedMemoryWriteLong(context, addr, val);
}

//////////////////////////////////////////////////////////////////////////////
void FASTCALL MappedMemoryWriteLong(SH2_struct *context, u32 addr, u32 val)
{
   SH2WriteNotify(context, addr, 4);
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      {
        WriteLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteLong(context, addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Long W %x\n", addr);
         UnhandledMemoryWriteLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

void FASTCALL SH2MappedMemoryWriteLong(SH2_struct *context, u32 addr, u32 val)
{
   SH2WriteNotify(context, addr, 4);
   switch (addr >> 29)
   {
      case 0x1:
      {
        WriteLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x0:
      {
CACHE_LOG("wl %x %x\n", addr, addr >> 29);
         // Cache/Non-Cached
         CacheWriteLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
      case 0x2:
      {
         // Purge Area
         CacheInvalidate(context, addr);
         return;
      }
      case 0x3:
      {
         // Address Array
         AddressArrayWriteLong(context, addr, val);
         return;
      }
      //case 0x4:
      case 0x6:

         // Data Array
         DataArrayWriteLong(context, addr, val);
         return;
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteLong(context, addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Long W %x\n", addr);
         UnhandledMemoryWriteLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

int MappedMemoryLoad(SH2_struct *sh, const char *filename, u32 addr)
{
   FILE *fp;
   long filesize;
   u8 *buffer;
   u32 i;
   size_t num_read = 0;

   if (!filename)
      return -1;

   if ((fp = fopen(filename, "rb")) == NULL)
      return -1;

   // Calculate file size
   fseek(fp, 0, SEEK_END);
   filesize = ftell(fp);

   if (filesize <= 0)
   {
      YabSetError(YAB_ERR_FILEREAD, filename);
      fclose(fp);
      return -1;//error
   }

   fseek(fp, 0, SEEK_SET);

   if ((buffer = (u8 *)malloc(filesize)) == NULL)
   {
      fclose(fp);
      return -2;
   }

   num_read = fread((void *)buffer, 1, filesize, fp);
   fclose(fp);

   for (i = 0; i < filesize; i++)
      MappedMemoryWriteByte(sh, addr+i, buffer[i]);

   free(buffer);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int MappedMemorySave(SH2_struct *sh, const char *filename, u32 addr, u32 size)
{
   FILE *fp;
   u8 *buffer;
   u32 i;

   if (!filename)
      return -1;

   if ((fp = fopen(filename, "wb")) == NULL)
      return -1;

   if ((buffer = (u8 *)malloc(size)) == NULL)
   {
      fclose(fp);
      return -2;
   }

   for (i = 0; i < size; i++)
      buffer[i] = MappedMemoryReadByte(sh, addr+i);

   fwrite((void *)buffer, 1, size, fp);
   fclose(fp);
   free(buffer);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void MappedMemoryLoadExec(SH2_struct *sh, const char *filename, u32 pc)
{
   char *p;
   size_t i;

   if ((p = strrchr(filename, '.')))
   {
      p = strdup(p);
      for (i = 0; i < strlen(p); i++)
         p[i] = toupper(p[i]);
      if (strcmp(p, ".COF") == 0 || strcmp(p, ".COFF") == 0)
      {
         MappedMemoryLoadCoff(sh, filename);
         free(p);
         return;
      }
      else if(strcmp(p, ".ELF") == 0)
      {
         MappedMemoryLoadElf(sh, filename);
         free(p);
         return;
      }

      free(p);
   }

   YabauseResetNoLoad();

   // Setup the vector table area, etc.
   YabauseSpeedySetup();

   MappedMemoryLoad(sh, filename, pc);
   SH2GetRegisters(MSH2, &MSH2->regs);
   MSH2->regs.PC = pc;
   SH2SetRegisters(MSH2, &MSH2->regs);
}

int BackupHandled(SH2_struct * sh, u32 addr) {
   if ((addr & 0xFFFFF) == 0x7d600) {
     if (sh == NULL) return 1; 
     BiosBUPInit(sh);
     return 1;
   }
   if (((addr & 0xFFFFF) >= 0x0384 && (addr & 0xFFFFF) <= 0x03A8) || ((addr & 0xFFFFF) == 0x358) ) {
     if (sh == NULL) return 1;
     return BiosHandleFunc(sh); //replace by NOP
   }
   return 0;
}

int isBackupHandled(u32 addr) {
   return( ((addr & 0xFFFFF) == 0x7d600) || (((addr & 0xFFFFF) >= 0x0384 && (addr & 0xFFFFF) <= 0x03A8) || ((addr & 0xFFFFF) == 0x358) ));
}

int BackupInit(char* path, int extended) {
  int currentSaveSize = TSize(path);
  int forceFormat = 0;
  if (currentSaveSize != -1) {
    int isNormalSize = (currentSaveSize == 0x8000);
    int isExtendedSize = (currentSaveSize == EXTENDED_BACKUP_SIZE);
    if (extended && isNormalSize) {
      extended = 0; //Force to use small save format
      printf("Internal backup file format is detected as standard save format - Force to use standard save format\n");
    }
    if (!extended && isExtendedSize) {
      extended = 1; //Force to use large save format
      printf("Internal backup file format is detected as extended save format - Force to use extended save format\n");
    }
    if (!((!extended && isNormalSize)||(extended && isExtendedSize))) {
      printf("Internal backup file format is bad - Force format to %s save format\n", (extended?"extended":"standard") );
      forceFormat =1; //Size is not the good one - Format
    }
  }
  if (!extended) {
    backup_file_addr = 0x00180000;
    backup_file_size = 0x8000;
  } else {
    backup_file_addr = EXTENDED_BACKUP_ADDR;
    backup_file_size = EXTENDED_BACKUP_SIZE;
  }

  if ((BupRam = T1MemoryInit(backup_file_size)) == NULL)
       return -1;

  if ((LoadBackupRam(path) != 0) || (forceFormat))
       FormatBackupRam(BupRam, backup_file_size);
  BupRamWritten = 0;
  bupfilename = path;
  return 0;
}

void BackupFlush() {
  if (BupRam)
  {
      if (T123Save(BupRam, backup_file_size, 1, bupfilename) != 0)
        YabSetError(YAB_ERR_FILEWRITE, (void *)bupfilename);
  }
}

void BackupExtended() {
}

void BackupDeinit() {
   if (BupRam)
   {
       if (T123Save(BupRam, backup_file_size, 1, bupfilename) != 0)
        YabSetError(YAB_ERR_FILEWRITE, (void *)bupfilename);
       T1MemoryDeInit(BupRam);
   }
   BupRam = NULL;
}


//////////////////////////////////////////////////////////////////////////////

int LoadBios(const char *filename)
{
  const char STVBios[4] = "STVB";
  const char STVBios2[16] = "SEGA ST-V(TITAN)";
  int ret = T123Load(BiosRom, 0x80000, 2, filename); //Saturn
  if (strncmp(&BiosRom[0x800], "TB_R", 4) != 0) {
    //Not a Saturn bios? Try ST-V one
    if (strncmp(&BiosRom[0x800], STVBios, 4) != 0) {
      if (strncmp(&BiosRom[0x500], STVBios2, 16) != 0) {
        printf("Unknown Bios\n");
        return -1; //Incompatible bios
      }
    }
    ret = T123Load(BiosRom, 0x80000, 1, filename); //STV
    yabsys.isSTV = 1;
  } else {
    yabsys.isSTV= 0;
  }
  if (yabsys.isSTV) printf("ST-V Emulation mode\n");
  else printf("Saturn Emulation mode\n");
  return ret;
}

//////////////////////////////////////////////////////////////////////////////

int LoadBackupRam(const char *filename)
{
   return T123Load(BupRam, backup_file_size, 1, filename);
}

static u8 header[32] = {
  0xFF, 'B', 0xFF, 'a', 0xFF, 'c', 0xFF, 'k',
  0xFF, 'U', 0xFF, 'p', 0xFF, 'R', 0xFF, 'a',
  0xFF, 'm', 0xFF, ' ', 0xFF, 'F', 0xFF, 'o',
  0xFF, 'r', 0xFF, 'm', 0xFF, 'a', 0xFF, 't'
};

int CheckBackupFile(FILE *fp) {
  int i, i2;
  u32 i3;

  // Fill in header
  for (i2 = 0; i2 < 4; i2++) {
    for (i = 0; i < 32; i++) {
      u8 val = fgetc(fp);
      if ( val != header[i]) {
        return -1;
      }
    }
  }
  return 0;
}

int ExtendBackupFile(FILE *fp, u32 size ) {

  fseek(fp, 0, SEEK_END);
  u32 acsize = ftell(fp);
  if (acsize < size) {
    // Clear the rest
    for ( u32 i = (acsize&0xFFFFFFFE) ; i < size; i += 2)
    {
      fputc(0xFF, fp);
      fputc(0x00, fp);
    }
    fflush(fp);
  }
  fseek(fp, 0, SEEK_SET);

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
void FormatBackupRamFile(FILE *fp, u32 size) {

  int i, i2;
  u32 i3;

  // Fill in header
  for (i2 = 0; i2 < 4; i2++)
    for (i = 0; i < 32; i++)
      fputc(header[i],fp);

  // Clear the rest
  for (i3 = 0x80; i3 < size; i3 += 2)
  {
    fputc(0xFF,fp);
    fputc(0x00,fp);
  }
  fflush(fp);
}

void FormatBackupRam(void *mem, u32 size)
{
   int i, i2;
   u32 i3;

   // Fill in header
   for(i2 = 0; i2 < 4; i2++)
      for(i = 0; i < 32; i++)
         T1WriteByte(mem, (i2 * 32) + i, header[i]);

   // Clear the rest
   for(i3 = 0x80; i3 < size; i3+=2)
   {
      T1WriteByte(mem, i3, 0xFF);
      T1WriteByte(mem, i3+1, 0x00);
   }
}

//////////////////////////////////////////////////////////////////////////////

int YabSaveStateBuffer(void ** buffer, size_t * size)
{
   FILE * fp;
   int status;
   size_t num_read = 0;

   if (buffer != NULL) *buffer = NULL;
   *size = 0;

   fp = tmpfile();

   ScspLockThread();
   status = YabSaveStateStream(fp);
   ScspUnLockThread();

   if (status != 0)
   {
      fclose(fp);
      return status;
   }

   fseek(fp, 0, SEEK_END);
   *size = ftell(fp);
   fseek(fp, 0, SEEK_SET);

   if (buffer != NULL)
   {
      *buffer = malloc(*size);
      num_read = fread(*buffer, 1, *size, fp);
   }

   fclose(fp);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YabSaveState(const char *filename)
{
   FILE *fp;
   int status;

   //use a second set of savestates for movies
   filename = MakeMovieStateName(filename);
   if (!filename)
      return -1;

   if ((fp = fopen(filename, "wb")) == NULL)
      return -1;
   ScspLockThread();
   status = YabSaveStateStream(fp);
   ScspUnLockThread();
   fclose(fp);

   return status;
}

//////////////////////////////////////////////////////////////////////////////

// FIXME: Here's a (possibly incomplete) list of data that should be added
// to the next version of the save state file:
//    yabsys.DecilineStop (new format)
//    yabsys.SH2CycleFrac (new field)
//    yabsys.DecilineUSed (new field)
//    yabsys.UsecFrac (new field)
//    [scsp2.c] It would be nice to redo the format entirely because so
//              many fields have changed format/size from the old scsp.c
//    [scsp2.c] scsp_clock, scsp_clock_frac, ScspState.sample_timer (timing)
//    [scsp2.c] cdda_buf, cdda_next_in, cdda_next_out (CDDA buffer)
//    [sh2core.c] frc.div changed to frc.shift
//    [sh2core.c] wdt probably needs to be written as well

int YabSaveStateStream(FILE *fp)
{
   u32 i;
   int offset;
   IOCheck_struct check;
   u8 *buf;
   int totalsize;
   int outputwidth;
   int outputheight;
   int movieposition;
   int temp;
   u32 temp32;

   check.done = 0;
   check.size = 0;

   // Write signature
   fprintf(fp, "YSS");

   // Write endianness byte
#ifdef WORDS_BIGENDIAN
   fputc(0x00, fp);
#else
   fputc(0x01, fp);
#endif

   // Write version(fix me)
   i = 2;
   ywrite(&check, (void *)&i, sizeof(i), 1, fp);

   // Skip the next 4 bytes for now
   i = 0;
   ywrite(&check, (void *)&i, sizeof(i), 1, fp);

   //write frame number
   ywrite(&check, (void *)&framecounter, 4, 1, fp);

   //this will be updated with the movie position later
   ywrite(&check, (void *)&framecounter, 4, 1, fp);

   // Go through each area and write each state
   i += CartSaveState(fp);
   i += Cs2SaveState(fp);
   i += SH2SaveState(MSH2, fp);
   i += SH2SaveState(SSH2, fp);
   i += SoundSaveState(fp);
   i += ScuSaveState(fp);
   i += SmpcSaveState(fp);
   i += Vdp1SaveState(fp);
   i += Vdp2SaveState(fp);

   offset = StateWriteHeader(fp, "OTHR", 1);

   // Other data
   //ywrite(&check, (void *)BupRam, 0x8000, 1, fp); // do we really want to save this?
   ywrite(&check, (void *)HighWram, 0x100000, 1, fp);
   ywrite(&check, (void *)LowWram, 0x100000, 1, fp);

   ywrite(&check, (void *)&yabsys.DecilineCount, sizeof(int), 1, fp);
   ywrite(&check, (void *)&yabsys.LineCount, sizeof(int), 1, fp);
   ywrite(&check, (void *)&yabsys.VBlankLineCount, sizeof(int), 1, fp);
   ywrite(&check, (void *)&yabsys.MaxLineCount, sizeof(int), 1, fp);
   temp = yabsys.DecilineStop >> YABSYS_TIMING_BITS;
   ywrite(&check, (void *)&temp, sizeof(int), 1, fp);
   temp = (yabsys.CurSH2FreqType == CLKTYPE_26MHZ) ? 268 : 286;
   ywrite(&check, (void *)&temp, sizeof(int), 1, fp);
   temp32 = (yabsys.UsecFrac * temp / 10) >> YABSYS_TIMING_BITS;
   ywrite(&check, (void *)&temp32, sizeof(u32), 1, fp);
   ywrite(&check, (void *)&yabsys.CurSH2FreqType, sizeof(int), 1, fp);
   ywrite(&check, (void *)&yabsys.IsPal, sizeof(int), 1, fp);

   VIDCore->GetGlSize(&outputwidth, &outputheight);

   totalsize=outputwidth * outputheight * sizeof(u32);

   if ((buf = (u8 *)malloc(totalsize)) == NULL)
   {
      return -2;
   }

   //YuiSwapBuffers();
   #ifdef USE_OPENGL
   glPixelZoom(1,1);
   glReadBuffer(GL_BACK);
   glReadPixels(0, 0, outputwidth, outputheight, GL_RGBA, GL_UNSIGNED_BYTE, buf);
   #else
   //memcpy(buf, dispbuffer, totalsize);
   #endif
   //YuiSwapBuffers();

   ywrite(&check, (void *)&outputwidth, sizeof(outputwidth), 1, fp);
   ywrite(&check, (void *)&outputheight, sizeof(outputheight), 1, fp);

   ywrite(&check, (void *)buf, totalsize, 1, fp);

   movieposition=ftell(fp);
   //write the movie to the end of the savestate
   SaveMovieInState(fp, check);

   i += StateFinishHeader(fp, offset);

   // Go back and update size
   fseek(fp, 8, SEEK_SET);
   ywrite(&check, (void *)&i, sizeof(i), 1, fp);
   fseek(fp, 16, SEEK_SET);
   ywrite(&check, (void *)&movieposition, sizeof(movieposition), 1, fp);

   free(buf);

   OSDPushMessage(OSDMSG_STATUS, 150, "STATE SAVED");
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YabLoadStateBuffer(const void * buffer, size_t size)
{
   FILE * fp;
   int status;

   fp = tmpfile();
   fwrite(buffer, 1, size, fp);

   fseek(fp, 0, SEEK_SET);

   ScspLockThread();
   status = YabLoadStateStream(fp);
   ScspUnLockThread();

   fclose(fp);

   return status;
}

//////////////////////////////////////////////////////////////////////////////

int YabLoadState(const char *filename)
{
   FILE *fp;
   int status;

   filename = MakeMovieStateName(filename);
   if (!filename)
      return -1;

   if ((fp = fopen(filename, "rb")) == NULL)
      return -1;

   ScspLockThread();
   status = YabLoadStateStream(fp);
   ScspUnLockThread();

   fclose(fp);

   return status;
}

//////////////////////////////////////////////////////////////////////////////

int YabLoadStateStream(FILE *fp)
{
   char id[3];
   u8 endian;
   int headerversion, version, size, chunksize, headersize;
   IOCheck_struct check;
   u8* buf;
   int totalsize;
   int outputwidth;
   int outputheight;
   int curroutputwidth;
   int curroutputheight;
   int movieposition;
   int temp;
   u32 temp32;
	int test_endian;

   headersize = 0xC;
   check.done = 0;
   check.size = 0;


   // Read signature
   yread(&check, (void *)id, 1, 3, fp);

   if (strncmp(id, "YSS", 3) != 0)
   {
      return -2;
   }

   // Read header
   yread(&check, (void *)&endian, 1, 1, fp);
   yread(&check, (void *)&headerversion, 4, 1, fp);
   yread(&check, (void *)&size, 4, 1, fp);
   switch(headerversion)
   {
      case 1:
         /* This is the "original" version of the format */
         break;
      case 2:
         /* version 2 adds video recording */
         yread(&check, (void *)&framecounter, 4, 1, fp);
		 movieposition=ftell(fp);
		 yread(&check, (void *)&movieposition, 4, 1, fp);
         headersize = 0x14;
         break;
      default:
         /* we're trying to open a save state using a future version
          * of the YSS format, that won't work, sorry :) */
         return -3;
         break;
   }

#ifdef WORDS_BIGENDIAN
   test_endian = endian == 1;
#else
   test_endian = endian == 0;
#endif
   if (test_endian)
   {
      // should setup reading so it's byte-swapped
      YabSetError(YAB_ERR_OTHER, (void *)"Load State byteswapping not supported");
      return -3;
   }

   // Make sure size variable matches actual size minus header
   fseek(fp, 0, SEEK_END);

   if (size != (ftell(fp) - headersize))
   {
      return -2;
   }
   fseek(fp, headersize, SEEK_SET);

   // Verify version here
 
   ScspMuteAudio(SCSP_MUTE_SYSTEM);
   
   if (StateCheckRetrieveHeader(fp, "CART", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -3;
   }
   CartLoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "CS2 ", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -3;
   }
   Cs2LoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "MSH2", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -3;
   }
   SH2LoadState(MSH2, fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "SSH2", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -3;
   }
   SH2LoadState(SSH2, fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "SCSP", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -3;
   }
   SoundLoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "SCU ", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -3;
   }
   ScuLoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "SMPC", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -3;
   }
   SmpcLoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "VDP1", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -3;
   }
   Vdp1LoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "VDP2", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -3;
   }
   Vdp2LoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "OTHR", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -3;
   }
   // Other data
   //yread(&check, (void *)BupRam, 0x8000, 1, fp);
   yread(&check, (void *)HighWram, 0x100000, 1, fp);
   yread(&check, (void *)LowWram, 0x100000, 1, fp);

   yread(&check, (void *)&yabsys.DecilineCount, sizeof(int), 1, fp);
   yread(&check, (void *)&yabsys.LineCount, sizeof(int), 1, fp);
   yread(&check, (void *)&yabsys.VBlankLineCount, sizeof(int), 1, fp);
   yread(&check, (void *)&yabsys.MaxLineCount, sizeof(int), 1, fp);
   yread(&check, (void *)&temp, sizeof(int), 1, fp);
   yread(&check, (void *)&temp, sizeof(int), 1, fp);
   yread(&check, (void *)&temp32, sizeof(u32), 1, fp);
   yread(&check, (void *)&yabsys.CurSH2FreqType, sizeof(int), 1, fp);
   yread(&check, (void *)&yabsys.IsPal, sizeof(int), 1, fp);
   YabauseChangeTiming(yabsys.CurSH2FreqType);
   yabsys.UsecFrac = (temp32 << YABSYS_TIMING_BITS) * temp / 10;

   if (headerversion > 1) {

   yread(&check, (void *)&outputwidth, sizeof(outputwidth), 1, fp);
   yread(&check, (void *)&outputheight, sizeof(outputheight), 1, fp);

   totalsize=outputwidth * outputheight * sizeof(u32);

   if ((buf = (u8 *)malloc(totalsize)) == NULL)
   {
      return -2;
   }

   yread(&check, (void *)buf, totalsize, 1, fp);

   YuiSwapBuffers();

   #ifdef USE_OPENGL
   if(VIDCore->id == VIDCORE_SOFT)
     glRasterPos2i(0, outputheight);
   if(VIDCore->id == VIDCORE_OGL)
	 glRasterPos2i(0, outputheight/2);
   #endif

   VIDCore->GetGlSize(&curroutputwidth, &curroutputheight);
   #ifdef USE_OPENGL
   glPixelZoom((float)curroutputwidth / (float)outputwidth, ((float)curroutputheight / (float)outputheight));
   glDrawPixels(outputwidth, outputheight, GL_RGBA, GL_UNSIGNED_BYTE, buf);
   #endif
   YuiSwapBuffers();
   free(buf);

   fseek(fp, movieposition, SEEK_SET);
   MovieReadState(fp);
   }

   ScspUnMuteAudio(SCSP_MUTE_SYSTEM);

   OSDPushMessage(OSDMSG_STATUS, 150, "STATE LOADED");

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YabSaveStateSlot(const char *dirpath, u8 slot)
{
  int rtn;
   char filename[512];

   if (cdip == NULL)
      return -1;

#ifdef WIN32
   sprintf(filename, "%s\\%s_%03d.yss", dirpath, cdip->itemnum, slot);
#else
   sprintf(filename, "%s/%s_%03d.yss", dirpath, cdip->itemnum, slot);
#endif
   ScspMuteAudio(1);
   rtn = YabSaveState(filename);
   ScspUnMuteAudio(1);
   return rtn;
}

//////////////////////////////////////////////////////////////////////////////

int YabLoadStateSlot(const char *dirpath, u8 slot)
{
   int rtn;
   char filename[512];

   if (cdip == NULL)
      return -1;

#ifdef WIN32
   sprintf(filename, "%s\\%s_%03d.yss", dirpath, cdip->itemnum, slot);
#else
   sprintf(filename, "%s/%s_%03d.yss", dirpath, cdip->itemnum, slot);
#endif
   ScspMuteAudio(1);
   rtn = YabLoadState(filename);
   ScspUnMuteAudio(1);
   return rtn;
}

//////////////////////////////////////////////////////////////////////////////

static int MappedMemoryAddMatch(SH2_struct *sh, u32 addr, u32 val, int searchtype, result_struct *result, u32 *numresults)
{
   result[numresults[0]].addr = addr;
   result[numresults[0]].val = val;
   numresults[0]++;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE int SearchIncrementAndCheckBounds(result_struct *prevresults,
                                                u32 *maxresults,
                                                u32 numresults, u32 *i,
                                                u32 inc, u32 *newaddr,
                                                u32 endaddr)
{
   if (prevresults)
   {
      if (i[0] >= maxresults[0])
      {
         maxresults[0] = numresults;
         return 1;
      }
      newaddr[0] = prevresults[i[0]].addr;
      i[0]++;
   }
   else
   {
      newaddr[0] = inc;

      if (newaddr[0] > endaddr || numresults >= maxresults[0])
      {
         maxresults[0] = numresults;
         return 1;
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static int SearchString(SH2_struct *sh, u32 startaddr, u32 endaddr, int searchtype,
                        const char *searchstr, result_struct *results,
                        u32 *maxresults)
{
   u8 *buf=NULL;
   u32 *buf32=NULL;
   u32 buflen=0;
   u32 counter;
   u32 addr;
   u32 numresults=0;

   buflen=(u32)strlen(searchstr);

   if ((buf32=(u32 *)malloc(buflen*sizeof(u32))) == NULL)
      return 0;

   buf = (u8 *)buf32;

   // Copy string to buffer
   switch (searchtype & 0x70)
   {
      case SEARCHSTRING:
         strcpy((char *)buf, searchstr);
         break;
      case SEARCHREL8BIT:
      case SEARCHREL16BIT:
      {
         char *text;
         char *searchtext=strdup(searchstr);

         // Calculate buffer length and read values into table
         buflen = 0;
         for (text=strtok((char *)searchtext, " ,"); text != NULL; text=strtok(NULL, " ,"))
         {            
            buf32[buflen] = strtoul(text, NULL, 0);
            buflen++;
         }
         free(searchtext);

         break;
      }
   }    

   addr = startaddr;
   counter = 0;

   for (;;)
   {
      // Fetch byte/word/etc.
      switch (searchtype & 0x70)
      {
         case SEARCHSTRING:
         {
            u8 val = MappedMemoryReadByte(sh, addr);
            addr++;

            if (val == buf[counter])
            {
               counter++;
               if (counter == buflen)
                  MappedMemoryAddMatch(sh, addr-buflen, val, searchtype, results, &numresults);
            }
            else
               counter = 0;
            break;
         }
         case SEARCHREL8BIT:
         {
            int diff;
            u32 j;
            u8 val2;
            u8 val = MappedMemoryReadByte(sh, addr);

            for (j = 1; j < buflen; j++)
            {
               // grab the next value
               val2 = MappedMemoryReadByte(sh, addr+j);

               // figure out the diff
               diff = (int)val2 - (int)val;

               // see if there's a match             
               if (((int)buf32[j] - (int)buf32[j-1]) != diff)
                  break;

               if (j == (buflen - 1))
                  MappedMemoryAddMatch(sh, addr, val, searchtype, results, &numresults);

               val = val2;
            }

            addr++;

            break;
         }
         case SEARCHREL16BIT:
         {
            int diff;
            u32 j;
            u16 val2;
            u16 val = MappedMemoryReadWord(sh, addr);

            for (j = 1; j < buflen; j++)
            {
               // grab the next value
               val2 = MappedMemoryReadWord(sh, addr+(j*2));

               // figure out the diff
               diff = (int)val2 - (int)val;

               // see if there's a match             
               if (((int)buf32[j] - (int)buf32[j-1]) != diff)
                  break;

               if (j == (buflen - 1))
                  MappedMemoryAddMatch(sh, addr, val, searchtype, results, &numresults);

               val = val2;
            }

            addr+=2;
            break;
         }
      }    

      if (addr > endaddr || numresults >= maxresults[0])
         break;
   }

   free(buf);
   maxresults[0] = numresults;
   return 1;
}

//////////////////////////////////////////////////////////////////////////////

result_struct *MappedMemorySearch(SH2_struct *sh, u32 startaddr, u32 endaddr, int searchtype,
                                  const char *searchstr,
                                  result_struct *prevresults, u32 *maxresults)
{
   u32 i=0;
   result_struct *results;
   u32 numresults=0;
   unsigned long searchval = 0;
   int issigned=0;
   u32 addr;

   if ((results = (result_struct *)malloc(sizeof(result_struct) * maxresults[0])) == NULL)
      return NULL;

   switch (searchtype & 0x70)
   {
      case SEARCHSTRING:
      case SEARCHREL8BIT:
      case SEARCHREL16BIT:
      {
         // String/8-bit relative/16-bit relative search(not supported, yet)
         if (SearchString(sh, startaddr, endaddr,  searchtype, searchstr,
                          results, maxresults) == 0)
         {
            maxresults[0] = 0;
            free(results);
            return NULL;
         }

         return results;
      }
      case SEARCHHEX:         
         sscanf(searchstr, "%08lx", &searchval);
         break;
      case SEARCHUNSIGNED:
         searchval = (unsigned long)strtoul(searchstr, NULL, 10);
         issigned = 0;
         break;
      case SEARCHSIGNED:
         searchval = (unsigned long)strtol(searchstr, NULL, 10);
         issigned = 1;
         break;
   }   

   if (prevresults)
   {
      addr = prevresults[i].addr;
      i++;
   }
   else
      addr = startaddr;

   // Regular value search
   for (;;)
   {
       u32 val=0;
       u32 newaddr;

       // Fetch byte/word/etc.
       switch (searchtype & 0x3)
       {
          case SEARCHBYTE:
             val = MappedMemoryReadByte(sh, addr);
             // sign extend if neccessary
             if (issigned)
                val = (s8)val;

             if (SearchIncrementAndCheckBounds(prevresults, maxresults, numresults, &i, addr+1, &newaddr, endaddr))
                return results;
             break;
          case SEARCHWORD:
             val = MappedMemoryReadWord(sh, addr);
             // sign extend if neccessary
             if (issigned)
                val = (s16)val;

             if (SearchIncrementAndCheckBounds(prevresults, maxresults, numresults, &i, addr+2, &newaddr, endaddr))
                return results;
             break;
          case SEARCHLONG:
             val = MappedMemoryReadLong(sh, addr);

             if (SearchIncrementAndCheckBounds(prevresults, maxresults, numresults, &i, addr+4, &newaddr, endaddr))
                return results;
             break;
          default:
             maxresults[0] = 0; 
             if (results)
                free(results);
             return NULL;
       }

       // Do a comparison
       switch (searchtype & 0xC)
       {
          case SEARCHEXACT:
             if (val == searchval)
                MappedMemoryAddMatch(sh, addr, val, searchtype, results, &numresults);
             break;
          case SEARCHLESSTHAN:
             if ((!issigned && val < searchval) || (issigned && (signed)val < (signed)searchval))
                MappedMemoryAddMatch(sh, addr, val, searchtype, results, &numresults);
             break;
          case SEARCHGREATERTHAN:
             if ((!issigned && val > searchval) || (issigned && (signed)val > (signed)searchval))
                MappedMemoryAddMatch(sh, addr, val, searchtype, results, &numresults);
             break;
          default:
             maxresults[0] = 0;
             if (results)
                free(results);
             return NULL;
       }

       addr = newaddr;
   }

   maxresults[0] = numresults;
   return results;
}
