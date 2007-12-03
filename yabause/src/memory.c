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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

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

//////////////////////////////////////////////////////////////////////////////

writebytefunc WriteByteList[0x1000];
writewordfunc WriteWordList[0x1000];
writelongfunc WriteLongList[0x1000];

readbytefunc ReadByteList[0x1000];
readwordfunc ReadWordList[0x1000];
readlongfunc ReadLongList[0x1000];

u8 *HighWram;
u8 *LowWram;
u8 *BiosRom;
u8 *BupRam;

//////////////////////////////////////////////////////////////////////////////

u8 * T1MemoryInit(u32 size)
{
   return calloc(size, sizeof(u8));
}

//////////////////////////////////////////////////////////////////////////////

void T1MemoryDeInit(u8 * mem)
{
   free(mem);
}

//////////////////////////////////////////////////////////////////////////////

T3Memory * T3MemoryInit(u32 size)
{
   T3Memory * mem;

   if ((mem = (T3Memory *) calloc(1, sizeof(T3Memory))) == NULL)
      return NULL;

   if ((mem->base_mem = (u8 *) calloc(size, sizeof(u8))) == NULL)
      return NULL;

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

Dummy * DummyInit(u32 s)
{
   return NULL;
}

//////////////////////////////////////////////////////////////////////////////

void DummyDeInit(Dummy * d)
{
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL UnhandledMemoryReadByte(u32 addr)
{
   fprintf(stderr, "Unhandled byte read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL UnhandledMemoryReadWord(u32 addr)
{
   fprintf(stderr, "Unhandled word read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL UnhandledMemoryReadLong(u32 addr)
{
   fprintf(stderr, "Unhandled long read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL UnhandledMemoryWriteByte(u32 addr, u8 val)
{
   fprintf(stderr, "Unhandled byte write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL UnhandledMemoryWriteWord(u32 addr, u16 val)
{
   fprintf(stderr, "Unhandled word write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL UnhandledMemoryWriteLong(u32 addr, u32 val)
{
   fprintf(stderr, "Unhandled long write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL HighWramMemoryReadByte(u32 addr)
{
   return T2ReadByte(HighWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL HighWramMemoryReadWord(u32 addr)
{
   return T2ReadWord(HighWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL HighWramMemoryReadLong(u32 addr)
{
   return T2ReadLong(HighWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL HighWramMemoryWriteByte(u32 addr, u8 val)
{
   T2WriteByte(HighWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL HighWramMemoryWriteWord(u32 addr, u16 val)
{
   T2WriteWord(HighWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL HighWramMemoryWriteLong(u32 addr, u32 val)
{
   T2WriteLong(HighWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL LowWramMemoryReadByte(u32 addr)
{
   return T2ReadByte(LowWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL LowWramMemoryReadWord(u32 addr)
{
   return T2ReadWord(LowWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL LowWramMemoryReadLong(u32 addr)
{
   return T2ReadLong(LowWram, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL LowWramMemoryWriteByte(u32 addr, u8 val)
{
   T2WriteByte(LowWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL LowWramMemoryWriteWord(u32 addr, u16 val)
{
   T2WriteWord(LowWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL LowWramMemoryWriteLong(u32 addr, u32 val)
{
   T2WriteLong(LowWram, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL BiosRomMemoryReadByte(u32 addr)
{
   return T2ReadByte(BiosRom, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL BiosRomMemoryReadWord(u32 addr)
{
   return T2ReadWord(BiosRom, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL BiosRomMemoryReadLong(u32 addr)
{
   return T2ReadLong(BiosRom, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosRomMemoryWriteByte(u32 addr, u8 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BiosRomMemoryWriteWord(u32 addr, u16 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////


void FASTCALL BiosRomMemoryWriteLong(u32 addr, u32 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL BupRamMemoryReadByte(u32 addr)
{
   return T1ReadByte(BupRam, addr & 0xFFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL BupRamMemoryReadWord(u32 addr)
{
   LOG("bup\t: BackupRam read word - %08X\n", addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL BupRamMemoryReadLong(u32 addr)
{
   LOG("bup\t: BackupRam read long - %08X\n", addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BupRamMemoryWriteByte(u32 addr, u8 val)
{
   T1WriteByte(BupRam, (addr & 0xFFFF) | 0x1, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BupRamMemoryWriteWord(u32 addr, u16 val)
{
   LOG("bup\t: BackupRam write word - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL BupRamMemoryWriteLong(u32 addr, u32 val)
{
   LOG("bup\t: BackupRam write long - %08X\n", addr);
}

//////////////////////////////////////////////////////////////////////////////

void FillMemoryArea(unsigned short start, unsigned short end,
                    readbytefunc r8func, readwordfunc r16func,
                    readlongfunc r32func, writebytefunc w8func,
                    writewordfunc w16func, writelongfunc w32func)
{
   int i;

   for (i=start; i < (end+1); i++)
   {
      ReadByteList[i] = r8func;
      ReadWordList[i] = r16func;
      ReadLongList[i] = r32func;
      WriteByteList[i] = w8func;
      WriteWordList[i] = w16func;
      WriteLongList[i] = w32func;
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
                                &UnhandledMemoryWriteLong);

   // Fill the rest
   FillMemoryArea(0x000, 0x00F, &BiosRomMemoryReadByte,
                                &BiosRomMemoryReadWord,
                                &BiosRomMemoryReadLong,
                                &BiosRomMemoryWriteByte,
                                &BiosRomMemoryWriteWord,
                                &BiosRomMemoryWriteLong);
   FillMemoryArea(0x010, 0x017, &SmpcReadByte,
                                &SmpcReadWord,
                                &SmpcReadLong,
                                &SmpcWriteByte,
                                &SmpcWriteWord,
                                &SmpcWriteLong);
   FillMemoryArea(0x018, 0x01F, &BupRamMemoryReadByte,
                                &BupRamMemoryReadWord,
                                &BupRamMemoryReadLong,
                                &BupRamMemoryWriteByte,
                                &BupRamMemoryWriteWord,
                                &BupRamMemoryWriteLong);
   FillMemoryArea(0x020, 0x02F, &LowWramMemoryReadByte,
                                &LowWramMemoryReadWord,
                                &LowWramMemoryReadLong,
                                &LowWramMemoryWriteByte,
                                &LowWramMemoryWriteWord,
                                &LowWramMemoryWriteLong);
   FillMemoryArea(0x100, 0x17F, &UnhandledMemoryReadByte,
                                &UnhandledMemoryReadWord,
                                &UnhandledMemoryReadLong,
                                &UnhandledMemoryWriteByte,
                                &SSH2InputCaptureWriteWord,
                                &UnhandledMemoryWriteLong);
   FillMemoryArea(0x180, 0x1FF, &UnhandledMemoryReadByte,
                                &UnhandledMemoryReadWord,
                                &UnhandledMemoryReadLong,
                                &UnhandledMemoryWriteByte,
                                &MSH2InputCaptureWriteWord,
                                &UnhandledMemoryWriteLong);
   FillMemoryArea(0x200, 0x3FF, CartridgeArea->Cs0ReadByte,
                                CartridgeArea->Cs0ReadWord,
                                CartridgeArea->Cs0ReadLong,
                                CartridgeArea->Cs0WriteByte,
                                CartridgeArea->Cs0WriteWord,
                                CartridgeArea->Cs0WriteLong);
   FillMemoryArea(0x400, 0x4FF, &Cs1ReadByte,
                                &Cs1ReadWord,
                                &Cs1ReadLong,
                                &Cs1WriteByte,
                                &Cs1WriteWord,
                                &Cs1WriteLong);
   FillMemoryArea(0x580, 0x58F, &Cs2ReadByte,
                                &Cs2ReadWord,
                                &Cs2ReadLong,
                                &Cs2WriteByte,
                                &Cs2WriteWord,
                                &Cs2WriteLong);
   FillMemoryArea(0x5A0, 0x5AF, &SoundRamReadByte,
                                &SoundRamReadWord,
                                &SoundRamReadLong,
                                &SoundRamWriteByte,
                                &SoundRamWriteWord,
                                &SoundRamWriteLong);
   FillMemoryArea(0x5B0, 0x5BF, &scsp_r_b,
                                &scsp_r_w,
                                &scsp_r_d,
                                &scsp_w_b,
                                &scsp_w_w,
                                &scsp_w_d);
   FillMemoryArea(0x5C0, 0x5C7, &Vdp1RamReadByte,
                                &Vdp1RamReadWord,
                                &Vdp1RamReadLong,
                                &Vdp1RamWriteByte,
                                &Vdp1RamWriteWord,
                                &Vdp1RamWriteLong);
   FillMemoryArea(0x5C8, 0x5CF, &Vdp1FrameBufferReadByte,
                                &Vdp1FrameBufferReadWord,
                                &Vdp1FrameBufferReadLong,
                                &Vdp1FrameBufferWriteByte,
                                &Vdp1FrameBufferWriteWord,
                                &Vdp1FrameBufferWriteLong);
   FillMemoryArea(0x5D0, 0x5D7, &Vdp1ReadByte,
                                &Vdp1ReadWord,
                                &Vdp1ReadLong,
                                &Vdp1WriteByte,
                                &Vdp1WriteWord,
                                &Vdp1WriteLong);
   FillMemoryArea(0x5E0, 0x5EF, &Vdp2RamReadByte,
                                &Vdp2RamReadWord,
                                &Vdp2RamReadLong,
                                &Vdp2RamWriteByte,
                                &Vdp2RamWriteWord,
                                &Vdp2RamWriteLong);
   FillMemoryArea(0x5F0, 0x5F7, &Vdp2ColorRamReadByte,
                                &Vdp2ColorRamReadWord,
                                &Vdp2ColorRamReadLong,
                                &Vdp2ColorRamWriteByte,
                                &Vdp2ColorRamWriteWord,
                                &Vdp2ColorRamWriteLong);
   FillMemoryArea(0x5F8, 0x5FB, &Vdp2ReadByte,
                                &Vdp2ReadWord,
                                &Vdp2ReadLong,
                                &Vdp2WriteByte,
                                &Vdp2WriteWord,
                                &Vdp2WriteLong);
   FillMemoryArea(0x5FE, 0x5FE, &ScuReadByte,
                                &ScuReadWord,
                                &ScuReadLong,
                                &ScuWriteByte,
                                &ScuWriteWord,
                                &ScuWriteLong);
   FillMemoryArea(0x600, 0x7FF, &HighWramMemoryReadByte,
                                &HighWramMemoryReadWord,
                                &HighWramMemoryReadLong,
                                &HighWramMemoryWriteByte,
                                &HighWramMemoryWriteWord,
                                &HighWramMemoryWriteLong);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL MappedMemoryReadByte(u32 addr)
{
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         return ReadByteList[(addr >> 16) & 0xFFF](addr);
      }
/*
      case 0x2:
      {
         // Purge Area
         break;
      }
*/
      case 0x4:
      case 0x6:
         // Data Array
         return DataArrayReadByte(addr);
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadByte(addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
         return UnhandledMemoryReadByte(addr);
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL MappedMemoryReadWord(u32 addr)
{
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         return ReadWordList[(addr >> 16) & 0xFFF](addr);
      }
/*
      case 0x2:
      {
         // Purge Area
         break;
      }
*/
      case 0x4:
      case 0x6:
         // Data Array
         return DataArrayReadWord(addr);
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadWord(addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
         return UnhandledMemoryReadWord(addr);
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL MappedMemoryReadLong(u32 addr)
{
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         return ReadLongList[(addr >> 16) & 0xFFF](addr);
      }
/*
      case 0x2:
      {
         // Purge Area
         break;
      }
*/
      case 0x3:
      {
         // Address Array
         return AddressArrayReadLong(addr);
      }
      case 0x4:
      case 0x6:
         // Data Array
         return DataArrayReadLong(addr);
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadLong(addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
         return UnhandledMemoryReadLong(addr);
      }
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL MappedMemoryWriteByte(u32 addr, u8 val)
{
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         WriteByteList[(addr >> 16) & 0xFFF](addr, val);
         return;
      }
/*
      case 0x2:
      {
         // Purge Area
         return;
      }
*/
      case 0x4:
      case 0x6:
         // Data Array
         DataArrayWriteByte(addr, val);
         return;
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteByte(addr, val);
            return; 
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
         UnhandledMemoryWriteByte(addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL MappedMemoryWriteWord(u32 addr, u16 val)
{
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         WriteWordList[(addr >> 16) & 0xFFF](addr, val);
         return;
      }
/*
      case 0x2:
      {
         // Purge Area
         return;
      }
*/
      case 0x4:
      case 0x6:
         // Data Array
         DataArrayWriteWord(addr, val);
         return;
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteWord(addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
         UnhandledMemoryWriteWord(addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL MappedMemoryWriteLong(u32 addr, u32 val)
{
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      case 0x5:
      {
         // Cache/Non-Cached
         WriteLongList[(addr >> 16) & 0xFFF](addr, val);
         return;
      }
      case 0x2:
      {
         // Purge Area
         return;
      }
      case 0x3:
      {
         // Address Array
         AddressArrayWriteLong(addr, val);
         return;
      }
      case 0x4:
      case 0x6:
         // Data Array
         DataArrayWriteLong(addr, val);
         return;
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteLong(addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // ???
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
         UnhandledMemoryWriteLong(addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

int MappedMemoryLoad(const char *filename, u32 addr)
{
   FILE *fp;
   u32 filesize;
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

   if ((buffer = (u8 *)malloc(filesize)) == NULL)
   {
      fclose(fp);
      return -2;
   }

   fread((void *)buffer, 1, filesize, fp);
   fclose(fp);

   for (i = 0; i < filesize; i++)
      MappedMemoryWriteByte(addr+i, buffer[i]);

   free(buffer);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int MappedMemorySave(const char *filename, u32 addr, u32 size)
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
      buffer[i] = MappedMemoryReadByte(addr+i);

   fwrite((void *)buffer, 1, size, fp);
   fclose(fp);
   free(buffer);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void MappedMemoryLoadExec(const char *filename, u32 pc)
{
   char *p;
   int i;

   if ((p = strrchr(filename, '.')))
   {
      p = strdup(p);
      for (i = 0; i < strlen(p); i++)
         p[i] = toupper(p[i]);
      if (strcmp(p, ".COF") == 0 || strcmp(p, ".COFF") == 0)
      {
         MappedMemoryLoadCoff(filename);
         free(p);
         return;
      }

      free(p);
   }

   YabauseResetNoLoad();

   // Setup the vector table area, etc.
   YabauseSpeedySetup();

   MappedMemoryLoad(filename, pc);
   MSH2->regs.PC = pc;
}

//////////////////////////////////////////////////////////////////////////////

int LoadBios(const char *filename)
{
   return T123Load(BiosRom, 0x80000, 2, filename);
}

//////////////////////////////////////////////////////////////////////////////

int LoadBackupRam(const char *filename)
{
   return T123Load(BupRam, 0x10000, 1, filename);
}

//////////////////////////////////////////////////////////////////////////////

void FormatBackupRam(void *mem, u32 size)
{
   int i, i2;
   u8 header[32] = {
      0xFF, 'B', 0xFF, 'a', 0xFF, 'c', 0xFF, 'k',
      0xFF, 'U', 0xFF, 'p', 0xFF, 'R', 0xFF, 'a',
      0xFF, 'm', 0xFF, ' ', 0xFF, 'F', 0xFF, 'o',
      0xFF, 'r', 0xFF, 'm', 0xFF, 'a', 0xFF, 't'
   };

   // Fill in header
   for(i2 = 0; i2 < 4; i2++)
      for(i = 0; i < 32; i++)
         T1WriteByte(mem, (i2 * 32) + i, header[i]);

   // Clear the rest
   for(i = 0x80; i < size; i+=2)
   {
      T1WriteByte(mem, i, 0xFF);
      T1WriteByte(mem, i+1, 0x00);
   }
}

//////////////////////////////////////////////////////////////////////////////

int YabSaveState(const char *filename)
{
   u32 i;
   FILE *fp;
   int offset;

   if ((fp = fopen(filename, "wb")) == NULL)
      return -1;

   // Write signature
   fprintf(fp, "YSS");

   // Write endianness byte
#ifdef WORDS_BIGENDIAN
   fputc(0x00, fp);
#else
   fputc(0x01, fp);
#endif

   // Write version(fix me)
   i = 1;
   fwrite((void *)&i, sizeof(i), 1, fp);

   // Skip the next 4 bytes for now
   i = 0;
   fwrite((void *)&i, sizeof(i), 1, fp);

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
   fwrite((void *)BupRam, 0x10000, 1, fp); // do we really want to save this?
   fwrite((void *)HighWram, 0x100000, 1, fp);
   fwrite((void *)LowWram, 0x100000, 1, fp);

   fwrite((void *)&yabsys.DecilineCount, sizeof(int), 1, fp);
   fwrite((void *)&yabsys.LineCount, sizeof(int), 1, fp);
   fwrite((void *)&yabsys.VBlankLineCount, sizeof(int), 1, fp);
   fwrite((void *)&yabsys.MaxLineCount, sizeof(int), 1, fp);
   fwrite((void *)&yabsys.DecilineStop, sizeof(int), 1, fp);
   fwrite((void *)&yabsys.Duf, sizeof(int), 1, fp);
   fwrite((void *)&yabsys.CycleCountII, sizeof(u32), 1, fp);
   fwrite((void *)&yabsys.CurSH2FreqType, sizeof(int), 1, fp);
   fwrite((void *)&yabsys.IsPal, sizeof(int), 1, fp);

   i += StateFinishHeader(fp, offset);

   // Go back and update size
   fseek(fp, 8, SEEK_SET);
   fwrite((void *)&i, sizeof(i), 1, fp);

   fclose(fp);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YabLoadState(const char *filename)
{
   FILE *fp;
   char id[3];
   u8 endian;
   int version, size, chunksize;

   if ((fp = fopen(filename, "rb")) == NULL)
      return -1;

   // Read signature
   fread((void *)id, 1, 3, fp);

   if (strncmp(id, "YSS", 3) != 0)
   {
      fclose(fp);
      return -2;
   }

   // Read header
   fread((void *)&endian, 1, 1, fp);
   fread((void *)&version, 4, 1, fp);
   fread((void *)&size, 4, 1, fp);

#ifdef WORDS_BIGENDIAN
   if (endian == 1)
#else
   if (endian == 0)
#endif
   {
      // should setup reading so it's byte-swapped
      YabSetError(YAB_ERR_OTHER, (void *)"Load State byteswapping not supported");
      fclose(fp);
      return -3;
   }

   // Make sure size variable matches actual size minus header
   fseek(fp, 0, SEEK_END);
   if (size != (ftell(fp) - 0xC))
   {
      fclose(fp);
      return -2;
   }
   fseek(fp, 0xC, SEEK_SET);

   // Verify version here

   ScspMuteAudio();
   
   if (StateCheckRetrieveHeader(fp, "CART", &version, &chunksize) != 0)
   {
      fclose(fp);
      // Revert back to old state here
      ScspUnMuteAudio();
      return -3;
   }
   CartLoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "CS2 ", &version, &chunksize) != 0)
   {
      fclose(fp);
      // Revert back to old state here
      ScspUnMuteAudio();
      return -3;
   }
   Cs2LoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "MSH2", &version, &chunksize) != 0)
   {
      fclose(fp);
      // Revert back to old state here
      ScspUnMuteAudio();
      return -3;
   }
   SH2LoadState(MSH2, fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "SSH2", &version, &chunksize) != 0)
   {
      fclose(fp);
      // Revert back to old state here
      ScspUnMuteAudio();
      return -3;
   }
   SH2LoadState(SSH2, fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "SCSP", &version, &chunksize) != 0)
   {
      fclose(fp);
      // Revert back to old state here
      ScspUnMuteAudio();
      return -3;
   }
   SoundLoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "SCU ", &version, &chunksize) != 0)
   {
      fclose(fp);
      // Revert back to old state here
      ScspUnMuteAudio();
      return -3;
   }
   ScuLoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "SMPC", &version, &chunksize) != 0)
   {
      fclose(fp);
      // Revert back to old state here
      ScspUnMuteAudio();
      return -3;
   }
   SmpcLoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "VDP1", &version, &chunksize) != 0)
   {
      fclose(fp);
      // Revert back to old state here
      ScspUnMuteAudio();
      return -3;
   }
   Vdp1LoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "VDP2", &version, &chunksize) != 0)
   {
      fclose(fp);
      // Revert back to old state here
      ScspUnMuteAudio();
      return -3;
   }
   Vdp2LoadState(fp, version, chunksize);

   if (StateCheckRetrieveHeader(fp, "OTHR", &version, &chunksize) != 0)
   {
      fclose(fp);
      // Revert back to old state here
      ScspUnMuteAudio();
      return -3;
   }
   // Other data
   fread((void *)BupRam, 0x10000, 1, fp);
   fread((void *)HighWram, 0x100000, 1, fp);
   fread((void *)LowWram, 0x100000, 1, fp);

   fread((void *)&yabsys.DecilineCount, sizeof(int), 1, fp);
   fread((void *)&yabsys.LineCount, sizeof(int), 1, fp);
   fread((void *)&yabsys.VBlankLineCount, sizeof(int), 1, fp);
   fread((void *)&yabsys.MaxLineCount, sizeof(int), 1, fp);
   fread((void *)&yabsys.DecilineStop, sizeof(int), 1, fp);
   fread((void *)&yabsys.Duf, sizeof(int), 1, fp);
   fread((void *)&yabsys.CycleCountII, sizeof(u32), 1, fp);
   fread((void *)&yabsys.CurSH2FreqType, sizeof(int), 1, fp);
   fread((void *)&yabsys.IsPal, sizeof(int), 1, fp);

   fclose(fp);

   ScspUnMuteAudio();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YabSaveStateSlot(const char *dirpath, u8 slot)
{
   char filename[512];
#ifdef WIN32
   sprintf(filename, "%s\\%s_%03d.yss", dirpath, cdip->itemnum, slot);
#else
   sprintf(filename, "%s/%s_%03d.yss", dirpath, cdip->itemnum);
#endif
   return YabSaveState(filename);
}

//////////////////////////////////////////////////////////////////////////////

int YabLoadStateSlot(const char *dirpath, u8 slot)
{
   char filename[512];
#ifdef WIN32
   sprintf(filename, "%s\\%s_%03d.yss", dirpath, cdip->itemnum, slot);
#else
   sprintf(filename, "%s/%s_%03d.yss", dirpath, cdip->itemnum);
#endif
   return YabLoadState(filename);
}

//////////////////////////////////////////////////////////////////////////////

int MappedMemoryAddMatch(u32 addr, u32 val, int searchtype, result_struct *result, u32 *numresults)
{
   result[numresults[0]].addr = addr;
   result[numresults[0]].val = val;
   numresults[0]++;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE int SearchIncrementAndCheckBounds(result_struct *prevresults,
                                                u32 *maxresults,
                                                u32 numresults, u32 i,
                                                u32 inc, u32 *newaddr,
                                                u32 endaddr)
{
   if (prevresults)
   {
      if (i >= maxresults[0])
      {
         maxresults[0] = numresults;
         return 1;
      }
      newaddr[0] = prevresults[i].addr;
      i++;
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

static int SearchString(u32 startaddr, u32 endaddr, int searchtype,
                        const char *searchstr, result_struct *results,
                        u32 *maxresults)
{
   u8 *buf=NULL;
   u32 *buf32=NULL;
   u32 buflen=0;
   u32 counter;
   u16 lastval=0;
   u32 addr;
   u32 numresults=0;

   buflen=strlen(searchstr);

   if ((buf32=(u32 *)malloc(buflen*sizeof(u32))) == NULL)
      return 0;

   buf = (u8 *)buf32;

   // Copy string to buffer
   switch (searchtype & 0x70)
   {
      case SEARCHSTRING:
         strcpy(buf, searchstr);
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
            buf32[buflen] = atoi(text);
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
            u8 val = MappedMemoryReadByte(addr);
            addr++;

            if (val == buf[counter])
            {
               counter++;
               if (counter == buflen)
                  MappedMemoryAddMatch(addr-buflen, val, searchtype, results, &numresults);
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
            u8 val = MappedMemoryReadByte(addr);

            for (j = 1; j < buflen; j++)
            {
               // grab the next value
               val2 = MappedMemoryReadByte(addr+j);

               // figure out the diff
               diff = (int)val2 - (int)val;

               // see if there's a match             
               if (((int)buf32[j] - (int)buf32[j-1]) != diff)
                  break;

               if (j == (buflen - 1))
                  MappedMemoryAddMatch(addr, val, searchtype, results, &numresults);

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
            u16 val = MappedMemoryReadWord(addr);

            for (j = 1; j < buflen; j++)
            {
               // grab the next value
               val2 = MappedMemoryReadWord(addr+(j*2));

               // figure out the diff
               diff = (int)val2 - (int)val;

               // see if there's a match             
               if (((int)buf32[j] - (int)buf32[j-1]) != diff)
                  break;

               if (j == (buflen - 1))
                  MappedMemoryAddMatch(addr, val, searchtype, results, &numresults);

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

result_struct *MappedMemorySearch(u32 startaddr, u32 endaddr, int searchtype,
                                  const char *searchstr,
                                  result_struct *prevresults, u32 *maxresults)
{
   u32 i=0;
   result_struct *results;
   u32 numresults=0;
   u32 searchval;
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
         if (SearchString(startaddr, endaddr,  searchtype, searchstr,
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
         searchval = (u32)atoi(searchstr);
         issigned = 0;
         break;
      case SEARCHSIGNED:
         searchval = (u32)atoi(searchstr);
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
             val = MappedMemoryReadByte(addr);
             // sign extend if neccessary
             if (issigned)
                val = (s8)val;

             if (SearchIncrementAndCheckBounds(prevresults, maxresults, numresults, i, addr+1, &newaddr, endaddr))
                return results;
             break;
          case SEARCHWORD:
             val = MappedMemoryReadWord(addr);
             // sign extend if neccessary
             if (issigned)
                val = (s16)val;

             if (SearchIncrementAndCheckBounds(prevresults, maxresults, numresults, i, addr+2, &newaddr, endaddr))
                return results;
             break;
          case SEARCHLONG:
             val = MappedMemoryReadLong(addr);

             if (SearchIncrementAndCheckBounds(prevresults, maxresults, numresults, i, addr+4, &newaddr, endaddr))
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
                MappedMemoryAddMatch(addr, val, searchtype, results, &numresults);
             break;
          case SEARCHLESSTHAN:
             if ((!issigned && val < searchval) || (issigned && (signed)val < (signed)searchval))
                MappedMemoryAddMatch(addr, val, searchtype, results, &numresults);
             break;
          case SEARCHGREATERTHAN:
             if ((!issigned && val > searchval) || (issigned && (signed)val > (signed)searchval))
                MappedMemoryAddMatch(addr, val, searchtype, results, &numresults);
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
