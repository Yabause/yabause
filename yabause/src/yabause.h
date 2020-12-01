/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2006 Theo Berkau
    Copyright 2006      Anders Montonen

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

#ifndef YABAUSE_H
#define YABAUSE_H

#include "core.h"
#include "sh2core.h"

typedef struct
{
   int percoretype;
   int sh1coretype;
   int sh2coretype;
   int vidcoretype;
   int sndcoretype;
   int m68kcoretype;
   int cdcoretype;
   int carttype;
   u8 regionid;
   int syslanguageid;
   const char *biospath;
   const char *cdpath;
   const char *ssfpath;
   const char *buppath;
   const char *mpegpath;
   const char *cartpath;
   const char *modemip;
   const char *modemport;
   const char *sh1rompath;
   int videoformattype;
   int frameskip;
   int clocksync;  // 1 = sync internal clock to emulation, 0 = realtime clock
   u32 basetime;   // Initial time in clocksync mode (0 = start w/ system time)
   int usethreads;
   int numthreads;
   int osdcoretype;
   int skip_load;//skip loading in YabauseInit so tests can be run without a bios
   int play_ssf;
   int use_new_scsp;
   int use_cd_block_lle;
   int use_sh2_dma_timing;
   int use_scu_dma_timing;
   int sh2_cache_enabled;
   int use_scsp_dsp_dynarec;
   int use_scu_dsp_jit;
} yabauseinit_struct;

#define CLKTYPE_26MHZ           0
#define CLKTYPE_28MHZ           1

#define VIDEOFORMATTYPE_NTSC    0
#define VIDEOFORMATTYPE_PAL     1

#ifndef NO_CLI
void print_usage(const char *program_name);
#endif

void YabauseChangeTiming(int freqtype);
int YabauseInit(yabauseinit_struct *init);
void YabFlushBackups(void);
void YabauseDeInit(void);
void YabauseSetDecilineMode(int on);
void YabauseResetNoLoad(void);
void YabauseReset(void);
void YabauseResetButton(void);
int YabauseExec(void);
void YabauseStartSlave(void);
void YabauseStopSlave(void);
u64 YabauseGetTicks(void);
void YabauseSetVideoFormat(int type);
void YabauseSpeedySetup(void);
int YabauseQuickLoadGame(void);

#define YABSYS_TIMING_BITS  20
#define YABSYS_TIMING_MASK  ((1 << YABSYS_TIMING_BITS) - 1)

typedef struct
{
   int DecilineMode;
   int DecilineCount;
   int LineCount;
   int VBlankLineCount;
   int MaxLineCount;
   u32 DecilineStop;  // Fixed point
   u32 SH2CycleFrac;  // Fixed point
   u32 DecilineUsec;  // Fixed point
   u32 UsecFrac;      // Fixed point
   int CurSH2FreqType;
   int IsPal;
   u8 UseThreads;
   int NumThreads;
   u8 IsSSH2Running;
   u64 OneFrameTime;
   u64 tickfreq;
   int emulatebios;
   int usequickload;
   int playing_ssf;
   int use_cd_block_lle;
   int use_sh2_dma_timing;
   int use_scu_dma_timing;
   int sh2_cache_enabled;
   int use_scsp_dsp_jit;
   int use_scu_dsp_jit;
} yabsys_struct;

extern yabsys_struct yabsys;

int YabauseEmulate(void);

#endif
