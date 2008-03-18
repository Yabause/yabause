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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef YABAUSE_H
#define YABAUSE_H

#include "core.h"

typedef struct
{
   int percoretype;
   int sh2coretype;
   int vidcoretype;
   int sndcoretype;
   int m68kcoretype;
   int cdcoretype;
   int carttype;
   u8 regionid;
   const char *biospath;
   const char *cdpath;
   const char *buppath;
   const char *mpegpath;
   const char *cartpath;
   const char *netlinksetting;
   int flags;
   int frameskip;
} yabauseinit_struct;

#define CLKTYPE_26MHZ           0
#define CLKTYPE_28MHZ           1

#define VIDEOFORMATTYPE_NTSC    0
#define VIDEOFORMATTYPE_PAL     1

void YabauseChangeTiming(int freqtype);
int YabauseInit(yabauseinit_struct *init);
void YabauseDeInit(void);
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

typedef struct
{
   int DecilineCount;
   int LineCount;
   int VBlankLineCount;
   int MaxLineCount;
   int DecilineStop;
   u32 Duf;
   u32 CycleCountII;
   int CurSH2FreqType;
   int IsPal;
   u8 IsSSH2Running;
   u8 IsM68KRunning;
   u64 OneFrameTime;
   u64 tickfreq;
   int emulatebios;
   int usequickload;
} yabsys_struct;

extern yabsys_struct yabsys;

#endif
