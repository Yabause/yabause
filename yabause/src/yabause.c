/*  Copyright 2003 Guillaume Duhamel
    Copyright 2004-2005 Theo Berkau

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

#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#endif
#include "yabause.h"
#include "cs0.h"
#include "cs2.h"
#include "debug.h"
#include "error.h"
#include "memory.h"
#include "peripheral.h"
#include "scsp.h"
#include "scu.h"
#include "sh2core.h"
#include "smpc.h"
#include "vdp2.h"
#include "yui.h"
#ifdef HAVE_LIBSDL
#include "SDL.h"
#endif

#define DONT_PROFILE
#include "profile.h"

//////////////////////////////////////////////////////////////////////////////

yabsys_struct yabsys;
const char *bupfilename = NULL;

//////////////////////////////////////////////////////////////////////////////

#ifndef NO_CLI
void print_usage(const char *program_name) {
   printf("Yabause v" VERSION "\n");
   printf("\n"
          "Purpose:\n"
          "  This program is intended to be a Sega Saturn emulator\n"
          "\n"
          "Usage: %s [OPTIONS]...\n", program_name);
   printf("   -h         --help                 Print help and exit\n");
   printf("   -b STRING  --bios=STRING          bios file\n");
   printf("   -i STRING  --iso=STRING           iso/cue file\n");
   printf("   -c STRING  --cdrom=STRING         cdrom path\n");
}
#endif

//////////////////////////////////////////////////////////////////////////////

void YabauseChangeTiming(int freqtype) {
   // Setup all the variables related to timing
   yabsys.DecilineCount = 0;
   yabsys.LineCount = 0;
   yabsys.CurSH2FreqType = freqtype;

   switch (freqtype)
   {
      case CLKTYPE_26MHZ:
         if (!yabsys.IsPal)
            yabsys.DecilineStop = 26846587 / 263 / 10 / 60; // I'm guessing this is wrong
         else
            yabsys.DecilineStop = 26846587 / 312 / 10 / 50; // I'm guessing this is wrong

         yabsys.Duf = 26846587 / 100000;

         break;
      case CLKTYPE_28MHZ:
         if (!yabsys.IsPal)
            yabsys.DecilineStop = 28636360 / 263 / 10 / 60; // I'm guessing this is wrong
         else
            yabsys.DecilineStop = 28636360 / 312 / 10 / 50; // I'm guessing this is wrong

         yabsys.Duf = 28636360 / 100000;
         break;
      default: break;
   }

   yabsys.CycleCountII = 0;
}

//////////////////////////////////////////////////////////////////////////////

int YabauseInit(yabauseinit_struct *init)
{     
   // Initialize both cpu's
   if (SH2Init(init->sh2coretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "SH2");
      return -1;
   }

   if ((BiosRom = T2MemoryInit(0x80000)) == NULL)
      return -1;

   if ((HighWram = T2MemoryInit(0x100000)) == NULL)
      return -1;

   if ((LowWram = T2MemoryInit(0x100000)) == NULL)
      return -1;

   if ((BupRam = T1MemoryInit(0x10000)) == NULL)
      return -1;

   if (LoadBackupRam(init->buppath) != 0)
      FormatBackupRam(BupRam, 0x10000);

   bupfilename = init->buppath;

   if (VideoInit(init->vidcoretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "Video");
      return -1;
   }

   // Initialize input core
   if (PerInit(init->percoretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "Peripheral");
      return -1;
   }

   if (CartInit(init->cartpath, init->carttype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "Cartridge");
      return -1;
   }

   if (Cs2Init(init->carttype, init->cdcoretype, init->cdpath, init->mpegpath) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "CS2");
      return -1;
   }

   if (ScuInit() != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "SCU");
      return -1;
   }

   if (ScspInit(init->sndcoretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "SCSP/M68K");
      return -1;
   }

   if (Vdp1Init() != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "VDP1");
      return -1;
   }

   if (Vdp2Init() != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "VDP2");
      return -1;
   }

   if (SmpcInit(init->regionid) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "SMPC");
      return -1;
   }

   MappedMemoryInit();

   YabauseSetVideoFormat(VIDEOFORMATTYPE_NTSC);
   YabauseChangeTiming(CLKTYPE_26MHZ);

   if (init->biospath != NULL)
   {
      if (LoadBios(init->biospath) != 0)
      {
         YabSetError(YAB_ERR_FILENOTFOUND, (void *)init->biospath);
         return -2;
      }
      yabsys.emulatebios = 0;
   }
   else
      yabsys.emulatebios = 1;

   YabauseReset();

   yabsys.usequickload = 0;

   if (yabsys.usequickload)
   {
      if (YabauseQuickLoadGame() != 0)
         YabauseReset();
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabauseDeInit() {
   SH2DeInit();

   if (BiosRom)
      T2MemoryDeInit(BiosRom);

   if (HighWram)
      T2MemoryDeInit(HighWram);

   if (LowWram)
      T2MemoryDeInit(LowWram);

   if (BupRam)
   {
      if (T123Save(BupRam, 0x10000, 1, bupfilename) != 0)
         YabSetError(YAB_ERR_FILEWRITE, (void *)bupfilename);

      T1MemoryDeInit(BupRam);
   }

   CartDeInit();
   Cs2DeInit();
   ScuDeInit();
   ScspDeInit();
   Vdp1DeInit();
   Vdp2DeInit();
   SmpcDeInit();
   PerDeInit();
   VideoDeInit();
}

//////////////////////////////////////////////////////////////////////////////

void YabauseReset() {
   SH2Reset(MSH2);
   YabStopSlave();
   memset(HighWram, 0, 0x100000);
   memset(LowWram, 0, 0x100000);

   // Reset CS0 area here
   // Reset CS1 area here
   Cs2Reset();
   ScuReset();
   ScspReset();
   yabsys.IsM68KRunning = 0;
   Vdp1Reset();
   Vdp2Reset();
   SmpcReset();

   SH2PowerOn(MSH2);
}

//////////////////////////////////////////////////////////////////////////////

int YabauseExec() {
   PROFILE_START("Total Emulation");
   PROFILE_START("MSH2");
   SH2Exec(MSH2, yabsys.DecilineStop);
   PROFILE_STOP("MSH2");

   PROFILE_START("SSH2");
   if (yabsys.IsSSH2Running)
      SH2Exec(SSH2, yabsys.DecilineStop);
   PROFILE_STOP("SSH2");

   yabsys.DecilineCount++;
   if(yabsys.DecilineCount == 9)
   {
      // HBlankIN
      PROFILE_START("hblankin");
      Vdp2HBlankIN();
      PROFILE_STOP("hblankin");
   }
   else if (yabsys.DecilineCount == 10)
   {
      // HBlankOUT
      PROFILE_START("hblankout");
      Vdp2HBlankOUT();
      PROFILE_STOP("hblankout");
      PROFILE_START("SCSP");
      ScspExec();
      PROFILE_STOP("SCSP");
      yabsys.DecilineCount = 0;
      yabsys.LineCount++;
      if (yabsys.LineCount == 224)
      {
         PROFILE_START("vblankin");
         // VBlankIN
         SmpcINTBACKEnd();
         Vdp2VBlankIN();
         PROFILE_STOP("vblankin");
      }
      else if (yabsys.LineCount == 263)
      {
         // VBlankOUT
         PROFILE_START("VDP1/VDP2");
         Vdp2VBlankOUT();
         yabsys.LineCount = 0;
         PROFILE_STOP("VDP1/VDP2");
      }
   }

   yabsys.CycleCountII += MSH2->cycles;

   while (yabsys.CycleCountII > yabsys.Duf)
   {
      PROFILE_START("SMPC");
      SmpcExec(10);
      PROFILE_STOP("SMPC");
      PROFILE_START("CDB");
      Cs2Exec(10);
      PROFILE_STOP("CDB");
      PROFILE_START("SCU");
      ScuExec(10);
      PROFILE_STOP("SCU");
      yabsys.CycleCountII %= yabsys.Duf;
   }

   PROFILE_START("68K");
   M68KExec(170);
   PROFILE_STOP("68K");

   MSH2->cycles %= yabsys.DecilineStop;
   if (yabsys.IsSSH2Running) 
      SSH2->cycles %= yabsys.DecilineStop;
   PROFILE_STOP("Total Emulation");

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabStartSlave(void) {
   SH2PowerOn(SSH2);
   yabsys.IsSSH2Running = 1;
}

//////////////////////////////////////////////////////////////////////////////

void YabStopSlave(void) {
   SH2Reset(SSH2);
   yabsys.IsSSH2Running = 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 YabauseGetTicks(void) {
#ifdef WIN32
   return timeGetTime();
#else
#ifdef HAVE_LIBSDL
   return SDL_GetTicks();
#endif
#endif
}

//////////////////////////////////////////////////////////////////////////////

void YabauseSetVideoFormat(int type) {
   yabsys.IsPal = type;
   yabsys.OneFrameTime = 1000 / (type ? 50 : 60);
   Vdp2Regs->TVSTAT = Vdp2Regs->TVSTAT | (type & 0x1);
   ScspChangeVideoFormat(type);
   YabauseChangeTiming(yabsys.CurSH2FreqType);
   lastticks = YabauseGetTicks();
}

//////////////////////////////////////////////////////////////////////////////

void YabauseSpeedySetup()
{
   u32 data;
   int i;

   // Setup the vector table area, etc.(all bioses have it at 0x00000600-0x00000810)
   for (i = 0; i < 0x210; i+=4)
   {
      data = MappedMemoryReadLong(0x00000600+i);
      MappedMemoryWriteLong(0x06000000+i, data);
   }

   // Setup the bios function pointers, etc.(all bioses have it at 0x00000820-0x00001100)
   for (i = 0; i < 0x8E0; i+=4)
   {
      data = MappedMemoryReadLong(0x00000820+i);
      MappedMemoryWriteLong(0x06000220+i, data);
   }

   // I'm not sure this is really needed
   for (i = 0; i < 0x700; i+=4)
   {
      data = MappedMemoryReadLong(0x00001100+i);
      MappedMemoryWriteLong(0x06001100+i, data);
   }

   // Fix some spots in 0x06000210-0x0600032C area
   MappedMemoryWriteLong(0x06000234, 0x000002AC);
   MappedMemoryWriteLong(0x06000238, 0x000002BC);
   MappedMemoryWriteLong(0x0600023C, 0x00000350);
   MappedMemoryWriteLong(0x06000240, 0x32524459);
   MappedMemoryWriteLong(0x0600024C, 0x00000000);
   MappedMemoryWriteLong(0x06000268, MappedMemoryReadLong(0x00001344));
   MappedMemoryWriteLong(0x0600026C, MappedMemoryReadLong(0x00001348));
   MappedMemoryWriteLong(0x060002C4, MappedMemoryReadLong(0x00001104));
   MappedMemoryWriteLong(0x060002C8, MappedMemoryReadLong(0x00001108));
   MappedMemoryWriteLong(0x060002CC, MappedMemoryReadLong(0x0000110C));
   MappedMemoryWriteLong(0x060002D0, MappedMemoryReadLong(0x00001110));
   MappedMemoryWriteLong(0x060002D4, MappedMemoryReadLong(0x00001114));
   MappedMemoryWriteLong(0x060002D8, MappedMemoryReadLong(0x00001118));
   MappedMemoryWriteLong(0x06000328, 0x000004C8);
   MappedMemoryWriteLong(0x0600032C, 0x00001800);

   // Fix SCU interrupts
   for (i = 0; i < 0x80; i+=4)
      MappedMemoryWriteLong(0x06000A00+i, 0x0600083C);

   // Set the cpu's, etc. to sane states

   // Set CD block to a sane state
   Cs2Area->reg.HIRQ = 0xFC1;
   Cs2Area->isdiskchanged = 0;
   Cs2Area->reg.CR1 = (Cs2Area->status << 8) | ((Cs2Area->options & 0xF) << 4) | (Cs2Area->repcnt & 0xF);
   Cs2Area->reg.CR2 = (Cs2Area->ctrladdr << 8) | Cs2Area->track;
   Cs2Area->reg.CR3 = (Cs2Area->index << 8) | ((Cs2Area->FAD >> 16) & 0xFF);
   Cs2Area->reg.CR4 = (u16) Cs2Area->FAD; 
   Cs2Area->satauth = 4;
}

//////////////////////////////////////////////////////////////////////////////

int YabauseQuickLoadGame()
{
   partition_struct * lgpartition;
   u8 *buffer;
   u32 fad;
   u32 addr;
   u32 size;
   u32 blocks;
   int i, i2;
   dirrec_struct dirrec;

   Cs2Area->outconcddev = Cs2Area->filter + 0;
   Cs2Area->outconcddevnum = 0;

   // read in lba 0/FAD 150
   if ((lgpartition = Cs2ReadUnFilteredSector(150)) == NULL)
      return -1;

   // Make sure we're dealing with a saturn game
   buffer = lgpartition->block[lgpartition->numblocks - 1]->data;

   YabauseSpeedySetup();

   if (memcmp(buffer, "SEGA SEGASATURN", 15) == 0)
   {
      // figure out how many more sectors we need to read
      size = (buffer[0xE0] << 24) |
             (buffer[0xE1] << 16) |
             (buffer[0xE2] << 8) |
              buffer[0xE3];
      blocks = size >> 11;
      if ((size % 2048) != 0) 
         blocks++;


      // Figure out where to load the first program
      addr = (buffer[0xF0] << 24) |
             (buffer[0xF1] << 16) |
             (buffer[0xF2] << 8) |
              buffer[0xF3];

      // Free Block
      lgpartition->size = 0;
      Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
      lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
      lgpartition->numblocks = 0;

      // Copy over ip to 0x06002000
      for (i = 0; i < blocks; i++)
      {
         if ((lgpartition = Cs2ReadUnFilteredSector(150+i)) == NULL)
            return -1;

         buffer = lgpartition->block[lgpartition->numblocks - 1]->data;

         if (size >= 2048)
         {
            for (i2 = 0; i2 < 2048; i2++)
               MappedMemoryWriteByte(0x06002000 + (i * 0x800) + i2, buffer[i2]);
         }
         else
         {
            for (i2 = 0; i2 < size; i2++)
               MappedMemoryWriteByte(0x06002000 + (i * 0x800) + i2, buffer[i2]);
         }

         size -= 2048;

         // Free Block
         lgpartition->size = 0;
         Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
         lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
         lgpartition->numblocks = 0;
      }

      // Ok, now that we've loaded the ip, now it's time to load the
      // First Program

      // Figure out where the first program is located
      if ((lgpartition = Cs2ReadUnFilteredSector(166)) == NULL)
         return -1;

      // Figure out root directory's location

      // Retrieve directory record's lba
      Cs2CopyDirRecord(lgpartition->block[lgpartition->numblocks - 1]->data + 0x9C, &dirrec);

      // Free Block
      lgpartition->size = 0;
      Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
      lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
      lgpartition->numblocks = 0;

      // Now then, fetch the root directory's records
      if ((lgpartition = Cs2ReadUnFilteredSector(dirrec.lba+150)) == NULL)
         return -1;

      buffer = lgpartition->block[lgpartition->numblocks - 1]->data;

      // Skip the first two records, read in the last one
      for (i = 0; i < 3; i++)
      {
         Cs2CopyDirRecord(buffer, &dirrec);
         buffer += dirrec.recordsize;
      }

      size = dirrec.size;
      blocks = size >> 11;
      if ((dirrec.size % 2048) != 0)
         blocks++;

      // Free Block
      lgpartition->size = 0;
      Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
      lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
      lgpartition->numblocks = 0;

      // Copy over First Program to addr
      for (i = 0; i < blocks; i++)
      {
         if ((lgpartition = Cs2ReadUnFilteredSector(150+dirrec.lba+i)) == NULL)
            return -1;

         buffer = lgpartition->block[lgpartition->numblocks - 1]->data;

         if (size >= 2048)
         {
            for (i2 = 0; i2 < 2048; i2++)
               MappedMemoryWriteByte(addr + (i * 0x800) + i2, buffer[i2]);
         }
         else
         {
            for (i2 = 0; i2 < size; i2++)
               MappedMemoryWriteByte(addr + (i * 0x800) + i2, buffer[i2]);
         }

         size -= 2048;

         // Free Block
         lgpartition->size = 0;
         Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
         lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
         lgpartition->numblocks = 0;
      }

      // Now setup SH2 registers to start executing at ip code
      for (i = 0; i < 15; i++)
         MSH2->regs.R[i] = 0x00000000;
      MSH2->regs.R[15] = 0x06002000;
      MSH2->regs.SR.all = 0x00000000;
      MSH2->regs.GBR = 0x00000000;
      MSH2->regs.VBR = 0x06000000;
      MSH2->regs.MACH = 0x00000000;
      MSH2->regs.MACL = 0x00000000;
      MSH2->regs.PR = 0x00000000;
      MSH2->regs.PC = 0x06002E00;
   }
   else
   {
      // Ok, we're not. Time to bail!

      // Free Block
      lgpartition->size = 0;
      Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
      lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
      lgpartition->numblocks = 0;

      return -1;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////
