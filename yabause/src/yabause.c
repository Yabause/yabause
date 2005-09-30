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
#ifdef __APPLE__
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
   printf("   -i STRING  --iso=STRING           iso file\n");
   printf("   -c STRING  --cdrom=STRING         cdrom path\n");
}
#endif

//////////////////////////////////////////////////////////////////////////////

void YabauseChangeTiming(int freqtype) {
   // Setup all the variables related to timing
   yabsys.DecilineCount = 0;
   yabsys.LineCount = 0;

   switch (freqtype)
   {
      case CLKTYPE_26MHZNTSC:
         yabsys.DecilineStop = 26846587 / 263 / 10 / 60; // I'm guessing this is wrong
         yabsys.Duf = 26846587 / 100000;
         break;
      case CLKTYPE_28MHZNTSC:
         yabsys.DecilineStop = 28636360 / 263 / 10 / 60; // I'm guessing this is wrong
         yabsys.Duf = 28636360 / 100000;                         
         break;
      case CLKTYPE_26MHZPAL:
         yabsys.DecilineStop = 26846587 / 312 / 10 / 50; // I'm guessing this is wrong
         yabsys.Duf = 26846587 / 100000;
         break;
      case CLKTYPE_28MHZPAL:
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

   if (Vdp1Init(init->vidcoretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, "VDP1");
      return -1;
   }

   if (Vdp2Init(init->vidcoretype) != 0)
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

   YabauseChangeTiming(CLKTYPE_26MHZNTSC);

   if (LoadBios(init->biospath) != 0)
   {
      YabSetError(YAB_ERR_FILENOTFOUND, (void *)init->biospath);
      return -2;
   }

   YabauseReset();

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

int main(int argc, char *argv[]) {
   int i;
   LogStart();

#ifndef NO_CLI
   //handle command line arguments
   for (i = 1; i < argc; ++i) {
      if (argv[i]) {
         //show usage
         if (0 == strcmp(argv[i], "-h") || 0 == strcmp(argv[i], "-?") || 0 == strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
         }
			
         //set bios
         if (0 == strcmp(argv[i], "-b") && argv[i + 1])
            YuiSetBiosFilename(argv[i + 1]);
         else if (strstr(argv[i], "--bios="))
            YuiSetBiosFilename(argv[i] + strlen("--bios="));
	
         //set iso
         else if (0 == strcmp(argv[i], "-i") && argv[i + 1])
            YuiSetIsoFilename(argv[i + 1]);
         else if (strstr(argv[i], "--iso="))
            YuiSetIsoFilename(argv[i] + strlen("--iso="));

         //set cdrom
         else if (0 == strcmp(argv[i], "-c") && argv[i + 1])
            YuiSetCdromFilename(argv[i + 1]);
         else if (strstr(argv[i], "--cdrom="))
            YuiSetCdromFilename(argv[i] + strlen("--cdrom="));
      }
   }
#endif

   if (YuiInit() != 0)
      fprintf(stderr, "Error running Yabause\n");

   YabauseDeInit();
   PROFILE_PRINT();
   LogStop();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
