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

/*! \file yabause.c
    \brief Yabause main emulation functions and interface for the ports
*/


#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <string.h>
#include "yabause.h"
#include "cheat.h"
#include "cs0.h"
#include "cs2.h"
#include "debug.h"
#include "error.h"
#include "memory.h"
#include "m68kcore.h"
#include "peripheral.h"
#include "scsp.h"
#include "scspdsp.h"
#include "scu.h"
#include "sh2core.h"
#include "smpc.h"
#include "ygl.h"
#include "vidsoft.h"
#include "vdp2.h"
#include "yui.h"
#include "bios.h"
#include "movie.h"
#include "osdcore.h"
#include "stv.h"

#ifdef HAVE_LIBSDL
#if defined(__APPLE__) || defined(GEKKO)
 #ifdef HAVE_LIBSDL2
  #include <SDL2/SDL.h>
 #else
  #include <SDL/SDL.h>
 #endif
#else
 #include "SDL.h"
#endif
#endif
#if defined(_MSC_VER) || !defined(HAVE_SYS_TIME_H)
#include <time.h>
#else
#include <sys/time.h>
#endif
#ifdef _arch_dreamcast
#include <arch/timer.h>
#endif
#ifdef GEKKO
#include <ogc/lwp_watchdog.h>
#endif
#ifdef PSP
#include "psp/common.h"
#endif


#ifdef SYS_PROFILE_H
 #include SYS_PROFILE_H
#else
 #define DONT_PROFILE
 #include "profile.h"
#endif

#if HAVE_GDBSTUB
    #include "gdb/stub.h"
#endif

#ifdef YAB_WANT_SSF
#include "aosdk/ssf.h"
#endif

#include <inttypes.h>

#define DECILINE_STEP (10.0)

//#define DEBUG_ACCURACY

#define THREAD_LOG //printf

//////////////////////////////////////////////////////////////////////////////

yabsys_struct yabsys;
u64 tickfreq;
//todo this ought to be in scspdsp.c
ScspDsp scsp_dsp = { 0 };
char ssf_track_name[256] = { 0 };
char ssf_artist[256] = { 0 };

u32 saved_scsp_cycles = 0;//fixed point
volatile u64 saved_m68k_cycles = 0;//fixed point

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
   printf("   -ns        --nosound              turn sound off\n");
   printf("   -a         --autostart            autostart emulation\n");
   printf("   -f         --fullscreen           start in fullscreen mode\n");
}
#endif

//////////////////////////////////////////////////////////////////////////////


static unsigned long nextFrameTime = 0;
static unsigned long delayUs_NTSC = 1000000/60;
static unsigned long delayUs_PAL = 1000000/50;

#define delayUs ((yabsys.IsPal)?delayUs_PAL:delayUs_NTSC);

static unsigned long time_left(void)
{
    unsigned long now;
    now = YabauseGetTicks();
    if(nextFrameTime <= now)
        return 0;
    else
        return nextFrameTime - now;
}

static void syncVideoMode(void) {
  unsigned long sleep = time_left();
  usleep(time_left());
  nextFrameTime += delayUs;
}

void YabauseChangeTiming(int freqtype) {
   // Setup all the variables related to timing

   const double freq_base = yabsys.IsPal ? 28437500.0
      : (39375000.0 / 11.0) * 8.0;  // i.e. 8 * 3.579545... = 28.636363... MHz
   const double freq_mult = (freqtype == CLKTYPE_26MHZ) ? 15.0/16.0 : 1.0;
   const double freq_shifted = (freq_base * freq_mult) * (1 << YABSYS_TIMING_BITS);
   const double usec_shifted = 1.0e6 * (1 << YABSYS_TIMING_BITS);
   const double deciline_time = yabsys.IsPal ? 1.0 /  50        / 313 / DECILINE_STEP
                                             : 1.0 / (60/1.001) / 263 / DECILINE_STEP;

   yabsys.DecilineCount = 0;
   yabsys.LineCount = 0;
   yabsys.CurSH2FreqType = freqtype;
   yabsys.DecilineStop = (u32) (freq_shifted * deciline_time + 0.5);
   MSH2->cycleFrac = 0;
   SSH2->cycleFrac = 0;
   yabsys.DecilineUsec = (u32) (usec_shifted * deciline_time + 0.5);
   yabsys.UsecFrac = 0;
}

//////////////////////////////////////////////////////////////////////////////
extern int tweak_backup_file_size;
YabEventQueue * q_scsp_frame_start;
YabEventQueue * q_scsp_finish;


static void sh2ExecuteSync( SH2_struct* sh, int req ) {
    if (req != 0) {
//printf("%s Request %d cycles\n", (sh == MSH2)?"MSH2":"SSH2", sh->cycles_request);
         u32 sh2cycles;
         sh->cycleFrac = req+sh->cycleLost;
         sh->cycleLost = sh->cycleFrac - ((sh->cycleFrac >> YABSYS_TIMING_BITS)<<YABSYS_TIMING_BITS);
         if ((sh->cycleFrac + (sh->cdiff<<YABSYS_TIMING_BITS)) < 0) {
           req = 0;
	   sh->cycles += sh->cycleFrac>>YABSYS_TIMING_BITS;
         } else {
           req = ((sh->cycleFrac + (sh->cdiff<<YABSYS_TIMING_BITS)) >> (YABSYS_TIMING_BITS + 1)) << 1;
         }
         if (!yabsys.playing_ssf)
         {
           int i;
	   int sh2start = sh->cycles;
           SH2Exec(sh, req);
	   sh->cdiff = req - (sh->cycles-sh2start);
         }
         req = 0;
    }
} 

#ifdef SSH2_ASYNC
static void sh2Execute( void * p ){
  SH2_struct* sh = (SH2_struct*)p;
  sh->thread_running = 1;
  while(sh->thread_running) {
    sem_wait(&sh->start);
    sh2ExecuteSync(sh, sh->cycles_request);
    sem_post(&sh->end);
  }
  sh->thread_running = 0;
}
#endif


int YabauseSh2Init(yabauseinit_struct *init)
{
   // Need to set this first, so init routines see it
   yabsys.UseThreads = init->usethreads;
   yabsys.NumThreads = init->numthreads;
   yabsys.usecache = init->usecache;
   yabsys.isRotated = 0;

#ifdef SPRITE_CACHE
   yabsys.useVdp1cache = init->useVdp1cache;
#endif

   // Initialize both cpu's
   if (SH2Init(init->sh2coretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("SH2"));
      return -1;
   }

   if ((BiosRom = T2MemoryInit(0x80000)) == NULL)
      return -1;

   if ((HighWram = T2MemoryInit(0x100000)) == NULL)
      return -1;

   if ((LowWram = T2MemoryInit(0x100000)) == NULL)
      return -1;

   BackupInit(init->buppath, init->extend_backup);

   if (CartInit(init->cartpath, init->carttype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("Cartridge"));
      return -1;
   }
   if (STVSingleInit(init->stvgamepath, init->stvbiospath) != 0) {
     if (STVInit(init->stvgame, init->cartpath) != 0)
     {
       YabSetError(YAB_ERR_CANNOTINIT, _("STV emulation"));
       return -1;
     }
   }

   MappedMemoryInit();

#ifdef SSH2_ASYNC
   MSH2->thread_running = 0;
   sem_init(&MSH2->start, 0, 0);
   sem_init(&MSH2->lock, 0, 1);
   sem_init(&MSH2->end, 0, 1);
   MSH2->cycles_request = 0;
   MSH2->thread_id = YAB_THREAD_MSH2;
#endif
   MSH2->cycleFrac = 0;
   MSH2->cycleLost = 0;
   MSH2->cdiff = 0;
#ifdef SSH2_ASYNC
   SSH2->thread_running = 0;
   sem_init(&SSH2->start, 0, 0);
   sem_init(&SSH2->lock, 0, 1);
   sem_init(&SSH2->end, 0, 1);
   SSH2->cycles_request = 0;
   SSH2->thread_id = YAB_THREAD_SSH2;
#endif
   SSH2->cycleFrac = 0;
   SSH2->cycleLost = 0;
   SSH2->cdiff = 0;
#ifdef SSH2_ASYNC
   YabThreadStart(YAB_THREAD_SSH2, sh2Execute, SSH2);
#endif
   return 0;
}

int YabauseInit(yabauseinit_struct *init)
{
   // Need to set this first, so init routines see it
   yabsys.UseThreads = init->usethreads;
   yabsys.NumThreads = init->numthreads;
   yabsys.usecache = init->usecache;
   yabsys.isRotated = 0;

  setM68kCounter(0);

  q_scsp_frame_start = YabThreadCreateQueue(1);
  q_scsp_finish = YabThreadCreateQueue(1);

#ifdef SPRITE_CACHE
   yabsys.useVdp1cache = init->useVdp1cache;
#endif

   // Initialize both cpu's
   if (SH2Init(init->sh2coretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("SH2"));
      return -1;
   }

   if ((BiosRom = T2MemoryInit(0x80000)) == NULL)
      return -1;

   if ((HighWram = T2MemoryInit(0x100000)) == NULL)
      return -1;

   if ((LowWram = T2MemoryInit(0x100000)) == NULL)
      return -1;

   if (BackupInit(init->buppath, init->extend_backup) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("Backup Ram"));
      return -1;
   }
   // check if format is needed?

   if (CartInit(init->cartpath, init->carttype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("Cartridge"));
      return -1;
   }

   if (STVSingleInit(init->stvgamepath, init->stvbiospath) != 0) {
     if (STVInit(init->stvgame, init->cartpath) != 0)
     {
       YabSetError(YAB_ERR_CANNOTINIT, _("STV emulation"));
       return -1;
     }
   }

   MappedMemoryInit();
#ifdef SSH2_ASYNC
   MSH2->thread_running = 0;
   sem_init(&MSH2->start, 0, 0);
   sem_init(&MSH2->lock, 0, 1);
   sem_init(&MSH2->end, 0, 1);
   MSH2->cycles_request = 0;
   MSH2->thread_id = YAB_THREAD_MSH2;
#endif
   MSH2->cycleFrac = 0;
   MSH2->cycleLost = 0;
   MSH2->cdiff = 0;
#ifdef SSH2_ASYNC
   SSH2->thread_running = 0;
   sem_init(&SSH2->start, 0, 0);
   sem_init(&SSH2->lock, 0, 1);
   sem_init(&SSH2->end, 0, 1);
   SSH2->cycles_request = 0;
   SSH2->thread_id = YAB_THREAD_SSH2;
#endif
   SSH2->cycleFrac = 0;
   SSH2->cycleLost = 0;
   SSH2->cdiff = 0;
#ifdef SSH2_ASYNC
   YabThreadStart(YAB_THREAD_SSH2, sh2Execute, SSH2);
#endif

   if (VideoInit(init->vidcoretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("Video"));
      return -1;
   }

   // Settings
   VideoSetSetting(VDP_SETTING_FILTERMODE,init->video_filter_type);
   VideoSetSetting(VDP_SETTING_UPSCALMODE,init->video_upscale_type);
   VideoSetSetting(VDP_SETTING_RESOLUTION_MODE, init->resolution_mode);
   VideoSetSetting(VDP_SETTING_ASPECT_RATIO, init->stretch);
   VideoSetSetting(VDP_SETTING_SCANLINE, init->scanline);

   // Initialize input core
   if (PerInit(init->percoretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("Peripheral"));
      return -1;
   }

   if (ScuInit() != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("SCU"));
      return -1;
   }

   if (M68KInit(init->m68kcoretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("M68K"));
      return -1;
   }

   if (ScspInit(init->sndcoretype) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("SCSP/M68K"));
      return -1;
   }

   if (Vdp1Init() != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("VDP1"));
      return -1;
   }

   if (Vdp2Init() != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("VDP2"));
      return -1;
   }

   if (Cs2Init(init->carttype, init->cdcoretype, init->cdpath, init->mpegpath, init->modemip, init->modemport) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("CS2"));
      return -1;
   }

   if (SmpcInit(init->regionid, init->clocksync, init->basetime) != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("SMPC"));
      return -1;
   }

   if (CheatInit() != 0)
   {
      YabSetError(YAB_ERR_CANNOTINIT, _("Cheat System"));
      return -1;
   }

   YabauseChangeTiming(CLKTYPE_26MHZ);

   if (init->frameskip)
      EnableAutoFrameSkip();

#ifdef YAB_PORT_OSD
   OSDChangeCore(init->osdcoretype);
#else
   OSDChangeCore(OSDCORE_DEFAULT);
#endif
   if (yabsys.isSTV == 0) {
	   if (init->biospath != NULL && strlen(init->biospath))
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
   } else yabsys.emulatebios = 0;

   yabsys.usequickload = 0;

   YabauseResetNoLoad();

#ifdef YAB_WANT_SSF

   if (init->play_ssf && init->ssfpath != NULL && strlen(init->ssfpath))
   {
      if (!load_ssf((char*)init->ssfpath, init->m68kcoretype, init->sndcoretype))
      {
         YabSetError(YAB_ERR_FILENOTFOUND, (void *)init->ssfpath);

         yabsys.playing_ssf = 0;

         return -2;
      }

      yabsys.playing_ssf = 1;

      get_ssf_info(1, ssf_track_name);
      get_ssf_info(3, ssf_artist);

      return 0;
   }
   else
      yabsys.playing_ssf = 0;

#endif

   if (init->skip_load)
   {
	   return 0;
   }

   if (yabsys.usequickload || yabsys.emulatebios)
   {
      if (YabauseQuickLoadGame() != 0)
      {
         if (yabsys.emulatebios)
         {
            YabSetError(YAB_ERR_CANNOTINIT, _("Game"));
            return -2;
         }
         else
            YabauseResetNoLoad();
      }
   }

   if (Cs2GetRegionID() == 0xC) YabauseSetVideoFormat(VIDEOFORMATTYPE_PAL);
   else YabauseSetVideoFormat(VIDEOFORMATTYPE_NTSC);

#ifdef HAVE_GDBSTUB
   GdbStubInit(MSH2, 43434);
#endif

   if (yabsys.UseThreads)
   {
      int num = yabsys.NumThreads < 1 ? 1 : yabsys.NumThreads;
      VIDSoftSetVdp1ThreadEnable(num == 1 ? 0 : 1);
      VIDSoftSetNumLayerThreads(num);
      VIDSoftSetNumPriorityThreads(num);
   }
   else
   {
      VIDSoftSetVdp1ThreadEnable(0);
      VIDSoftSetNumLayerThreads(0);
      VIDSoftSetNumPriorityThreads(0);
   }

   scsp_set_use_new(init->use_new_scsp);

   nextFrameTime = YabauseGetTicks();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabFlushBackups(void)
{
  BackupFlush();
  CartFlush();
}

//////////////////////////////////////////////////////////////////////////////

void YabauseDeInit(void) {
   
   Vdp2DeInit();
   Vdp1DeInit();
   
   SH2DeInit();

   if (BiosRom)
      T2MemoryDeInit(BiosRom);
   BiosRom = NULL;

   if (HighWram)
      T2MemoryDeInit(HighWram);
   HighWram = NULL;

   if (LowWram)
      T2MemoryDeInit(LowWram);
   LowWram = NULL;

   BackupDeinit();
 
   CartDeInit();
   Cs2DeInit();
   ScuDeInit();
   ScspDeInit();
   SmpcDeInit();
   PerDeInit();
   VideoDeInit();
   CheatDeInit();
}

//////////////////////////////////////////////////////////////////////////////

void YabauseResetNoLoad(void) {
   SH2Reset(MSH2);
   YabauseStopSlave();
   memset(HighWram, 0, 0x100000);
   memset(LowWram, 0, 0x100000);

   // Reset CS0 area here
   // Reset CS1 area here
   Cs2Reset();
   ScuReset();
   ScspReset();
   Vdp1Reset();
   Vdp2Reset();
   SmpcReset();

   SH2PowerOn(MSH2);
}

//////////////////////////////////////////////////////////////////////////////

void YabauseReset(void) {

   if (yabsys.playing_ssf)
      yabsys.playing_ssf = 0;

   YabauseResetNoLoad();

   if (yabsys.usequickload || yabsys.emulatebios)
   {
      if (YabauseQuickLoadGame() != 0)
      {
         if (yabsys.emulatebios)
            YabSetError(YAB_ERR_CANNOTINIT, _("Game"));
         else
            YabauseResetNoLoad();
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void YabauseResetButton(void) {
   // This basically emulates the reset button behaviour of the saturn. This
   // is the better way of reseting the system since some operations (like
   // backup ram access) shouldn't be interrupted and this allows for that.

   SmpcResetButton();
}

//////////////////////////////////////////////////////////////////////////////

int YabauseExec(void) {

	//automatically advance lag frames, this should be optional later
	if (FrameAdvanceVariable > 0 && LagFrameFlag == 1){ 
		FrameAdvanceVariable = NeedAdvance; //advance a frame
		YabauseEmulate();
		FrameAdvanceVariable = Paused; //pause next time
		return(0);
	}

	if (FrameAdvanceVariable == Paused){
		ScspMuteAudio(SCSP_MUTE_SYSTEM);
		return(0);
	}
  
	if (FrameAdvanceVariable == NeedAdvance){  //advance a frame
		FrameAdvanceVariable = Paused; //pause next time
		ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
		YabauseEmulate();
	}
	
	if (FrameAdvanceVariable == RunNormal ) { //run normally
		ScspUnMuteAudio(SCSP_MUTE_SYSTEM);	
		YabauseEmulate();
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
int saved_centicycles;

u32 get_cycles_per_line_division(u32 clock, int frames, int lines, int divisions_per_line)
{
   return ((u64)(clock / frames) << SCSP_FRACTIONAL_BITS) / (lines * divisions_per_line);
}

u32 YabauseGetCpuTime(){

  return MSH2->cycles;
}

// cyclesinc

u32 YabauseGetFrameCount() {
  return yabsys.frame_count;
}

//#define YAB_STATICS
void SyncCPUtoSCSP();
u64 getM68KCounter();
u64 g_m68K_dec_cycle = 0;


int YabauseEmulate(void) {
   int oneframeexec = 0;
   yabsys.frame_count++;

   const u32 usecinc = yabsys.DecilineUsec;

   unsigned int m68kcycles;       // Integral M68k cycles per call
   unsigned int m68kcenticycles;  // 1/100 M68k cycles per call

   u32 m68k_cycles_per_deciline = 0;
   u32 scsp_cycles_per_deciline = 0;

   if(use_new_scsp)
   {
      int lines = 0;
      int frames = 0;

      if (yabsys.IsPal)
      {
         lines = 313;
         frames = 50;
      }
      else
      {
         lines = 263; 
         frames = 60;
      }
      scsp_cycles_per_deciline = get_cycles_per_line_division(44100 * 512, frames, lines, DECILINE_STEP);
      m68k_cycles_per_deciline = get_cycles_per_line_division(44100 * 256, frames, lines, DECILINE_STEP);
   }
   else
   {
      if (yabsys.IsPal)
      {
         /* 11.2896MHz / 50Hz / 313 lines / 10 calls/line = 72.20 cycles/call */
         m68kcycles = (int)(722/DECILINE_STEP);
         m68kcenticycles = (int)(((722.0/DECILINE_STEP) - m68kcycles)*100);
      }
      else
      {
         /* 11.2896MHz / 60Hz / 263 lines / 10 calls/line = 71.62 cycles/call */
         m68kcycles = (int)(716/DECILINE_STEP);
         m68kcenticycles = (int)(((716.2/DECILINE_STEP) - m68kcycles)*100);
      }
   }

   DoMovie();

   MSH2->cycles = 0;
   MSH2->depth = 0;
   SSH2->depth = 0;
   SSH2->cycles = 0;
//   SH2OnFrame(MSH2);
//   SH2OnFrame(SSH2);
   u64 cpu_emutime = 0;
   while (!oneframeexec)
   {
      PROFILE_START("Total Emulation");

#ifdef YAB_STATICS
		 u64 current_cpu_clock = YabauseGetTicks();
#endif
         if (!yabsys.playing_ssf)
         {
             THREAD_LOG("Unlock MSH2\n");
#ifdef SSH2_ASYNC
               if (yabsys.IsSSH2Running) {
                  if(yabsys.DecilineCount == 0) {
                    SSH2->cycles_request = (DECILINE_STEP)*yabsys.DecilineStop;
                    sem_post(&SSH2->start);
               }
             }
#endif
             sh2ExecuteSync(MSH2, yabsys.DecilineStop);
             if (yabsys.IsSSH2Running) {
#ifdef SSH2_ASYNC
             if(yabsys.DecilineCount == (DECILINE_STEP-1)) {
               sem_wait(&SSH2->end);
             }
#else
               sh2ExecuteSync(SSH2, yabsys.DecilineStop);
#endif
             }
         }
#ifdef YAB_STATICS
		 cpu_emutime += (YabauseGetTicks() - current_cpu_clock) * 1000000 / yabsys.tickfreq;
#endif

         yabsys.DecilineCount++;
         if(yabsys.DecilineCount == DECILINE_STEP-1)
         {
            // HBlankIN
            PROFILE_START("hblankin");
            Vdp1HBlankIN();
            Vdp2HBlankIN();
            PROFILE_STOP("hblankin");
         }

      if (yabsys.DecilineCount == DECILINE_STEP)
      {
         // HBlankOUT
         PROFILE_START("hblankout");
         Vdp2HBlankOUT();
         Vdp1HBlankOUT();
        // SyncScsp();
         PROFILE_STOP("hblankout");
         PROFILE_START("SCSP");
         ScspExec();
         PROFILE_STOP("SCSP");
         yabsys.DecilineCount = 0;
         yabsys.LineCount++;
         if (yabsys.LineCount == yabsys.VBlankLineCount)
         {
#if defined(ASYNC_SCSP)
            setM68kCounter((u64)(44100 * 256 / 60));
#endif
            PROFILE_START("vblankin");
            // VBlankIN
            SmpcINTBACKEnd();
            Vdp1VBlankIN();
            Vdp2VBlankIN();
#if defined(ASYNC_SCSP)
            SyncCPUtoSCSP();
#endif
            PROFILE_STOP("vblankin");
            CheatDoPatches(MSH2);
         }
         else if (yabsys.LineCount == yabsys.MaxLineCount)
         {
            // VBlankOUT
            PROFILE_START("VDP1/VDP2");
            Vdp1VBlankOUT();
            Vdp2VBlankOUT();
            yabsys.LineCount = 0;
            oneframeexec = 1;
            PROFILE_STOP("VDP1/VDP2");
         }
      }
      PROFILE_START("SCU");
      ScuExec((yabsys.DecilineStop>>YABSYS_TIMING_BITS) / 2);
      PROFILE_STOP("SCU");
      PROFILE_START("68K");
      M68KSync();  // Wait for the previous iteration to finish
      PROFILE_STOP("68K");

      yabsys.UsecFrac += usecinc;
      PROFILE_START("SMPC");
      SmpcExec(yabsys.UsecFrac >> YABSYS_TIMING_BITS);
      PROFILE_STOP("SMPC");
      PROFILE_START("CDB");
      Cs2Exec(yabsys.UsecFrac >> YABSYS_TIMING_BITS);
      PROFILE_STOP("CDB");
      yabsys.UsecFrac &= YABSYS_TIMING_MASK;
      
#if !defined(ASYNC_SCSP)
      if(!use_new_scsp)
      {
         int cycles;

         PROFILE_START("68K");
         cycles = m68kcycles;
         saved_centicycles += m68kcenticycles;
         if (saved_centicycles >= 100) {
            cycles++;
            saved_centicycles -= 100;
         }
         M68KExec(cycles);
         PROFILE_STOP("68K");
      }
      else
      {

         u32 m68k_integer_part = 0, scsp_integer_part = 0;
         saved_m68k_cycles += m68k_cycles_per_deciline;
         m68k_integer_part = saved_m68k_cycles >> SCSP_FRACTIONAL_BITS;
         M68KExec(m68k_integer_part);
         saved_m68k_cycles -= m68k_integer_part << SCSP_FRACTIONAL_BITS;

         saved_scsp_cycles += scsp_cycles_per_deciline;
         scsp_integer_part = saved_scsp_cycles >> SCSP_FRACTIONAL_BITS;
         new_scsp_exec(scsp_integer_part);
         saved_scsp_cycles -= scsp_integer_part << SCSP_FRACTIONAL_BITS;
#else
      {
        saved_m68k_cycles  += m68k_cycles_per_deciline;
        setM68kCounter(saved_m68k_cycles);
#endif
      }
      PROFILE_STOP("Total Emulation");
   }
   M68KSync();

   if (isAutoFrameSkip() == 0) syncVideoMode();

#ifdef YAB_WANT_SSF

   if (yabsys.playing_ssf)
   {
      OSDPushMessage(OSDMSG_FPS, 1, "NAME %s", ssf_track_name);
      OSDPushMessage(OSDMSG_STATUS, 1, "ARTIST %s", ssf_artist);
   }

#endif
   
#ifdef YAB_STATICS
   printf("CPUTIME = %" PRId64 " @ %d \n", cpu_emutime, yabsys.frame_count );
#if 1
   if (yabsys.frame_count >= 4000 ) {
     static FILE * pfm = NULL;
     if (pfm == NULL) {
#ifdef ANDROID
       pfm = fopen("/mnt/sdcard/cpu.txt", "w");
#else
       pfm = fopen("cpu.txt", "w");
#endif
     }
     if (pfm) {
       fprintf(pfm, "%d\t%" PRId64 "\n", yabsys.frame_count, cpu_emutime);
       fflush(pfm);
     }
     if( yabsys.frame_count >= 6100) {
       fclose(pfm);
       exit(0);
     }
   }
#endif
#endif
#if DYNAREC_DEVMIYAX
   if (SH2Core->id == 3) SH2DynShowSttaics(MSH2, SSH2);
#endif

   return 0;
}


void SyncCPUtoSCSP() {
  //LOG("[SH2] WAIT SCSP");
    YabWaitEventQueue(q_scsp_finish);
    saved_m68k_cycles = 0;
    setM68kCounter(saved_m68k_cycles);
    YabAddEventQueue(q_scsp_frame_start, 0);
  //LOG("[SH2] START SCSP");
}

//////////////////////////////////////////////////////////////////////////////

void YabauseStartSlave(void) {
   if (yabsys.IsSSH2Running == 1) return;
   if (yabsys.emulatebios)
   {
      MappedMemoryWriteLong(SSH2, 0xFFFFFFE0, 0xA55A03F1); // BCR1
      MappedMemoryWriteLong(SSH2, 0xFFFFFFE4, 0xA55A00FC); // BCR2
      MappedMemoryWriteLong(SSH2, 0xFFFFFFE8, 0xA55A5555); // WCR
      MappedMemoryWriteLong(SSH2, 0xFFFFFFEC, 0xA55A0070); // MCR

      MappedMemoryWriteWord(SSH2, 0xFFFFFEE0, 0x0000); // ICR
      MappedMemoryWriteWord(SSH2, 0xFFFFFEE2, 0x0000); // IPRA
      MappedMemoryWriteWord(SSH2, 0xFFFFFE60, 0x0F00); // VCRWDT
      MappedMemoryWriteWord(SSH2, 0xFFFFFE62, 0x6061); // VCRA
      MappedMemoryWriteWord(SSH2, 0xFFFFFE64, 0x6263); // VCRB
      MappedMemoryWriteWord(SSH2, 0xFFFFFE66, 0x6465); // VCRC
      MappedMemoryWriteWord(SSH2, 0xFFFFFE68, 0x6600); // VCRD
      MappedMemoryWriteWord(SSH2, 0xFFFFFEE4, 0x6869); // VCRWDT
      MappedMemoryWriteLong(SSH2, 0xFFFFFFA8, 0x0000006C); // VCRDMA1
      MappedMemoryWriteLong(SSH2, 0xFFFFFFA0, 0x0000006D); // VCRDMA0
      MappedMemoryWriteLong(SSH2, 0xFFFFFF0C, 0x0000006E); // VCRDIV
      MappedMemoryWriteLong(SSH2, 0xFFFFFE10, 0x00000081); // TIER

      SH2GetRegisters(SSH2, &SSH2->regs);
      SSH2->regs.R[15] = Cs2GetSlaveStackAdress();
      SSH2->regs.VBR = 0x06000400;
      SSH2->regs.PC = SH2MappedMemoryReadLong(SSH2, 0x06000250);
      if (SH2MappedMemoryReadLong(SSH2, 0x060002AC) != 0)
         SSH2->regs.R[15] = SH2MappedMemoryReadLong(SSH2, 0x060002AC);
      SH2SetRegisters(SSH2, &SSH2->regs);
   }
   else {
     SH2PowerOn(SSH2);
   }

   yabsys.IsSSH2Running = 1;
}

//////////////////////////////////////////////////////////////////////////////

void YabauseStopSlave(void) {
   if (yabsys.IsSSH2Running == 0) return;
   SH2Reset(SSH2);
   yabsys.IsSSH2Running = 0;
}

//////////////////////////////////////////////////////////////////////////////

u64 YabauseGetTicks(void) {
#ifdef WIN32
   u64 ticks;
   QueryPerformanceCounter((LARGE_INTEGER *)&ticks);
   return ticks;
#elif defined(_arch_dreamcast)
   return (u64) timer_ms_gettime64();
#elif defined(GEKKO)  
   return gettime();
#elif defined(PSP)
   return sceKernelGetSystemTimeWide();
#elif defined(ANDROID)
	struct timespec clock_time;
	clock_gettime(CLOCK_REALTIME , &clock_time);
	return (u64)clock_time.tv_sec * 1000000 + clock_time.tv_nsec/1000;
#elif defined(HAVE_GETTIMEOFDAY)
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return (u64)tv.tv_sec * 1000000 + tv.tv_usec;
#elif defined(HAVE_LIBSDL)
   return (u64)SDL_GetTicks();
#endif
}

//////////////////////////////////////////////////////////////////////////////

void YabauseSetVideoFormat(int type) {
   if (Vdp2Regs == NULL) return;
   yabsys.IsPal = (type == VIDEOFORMATTYPE_PAL);
   yabsys.MaxLineCount = type ? 313 : 263;
#ifdef WIN32
   QueryPerformanceFrequency((LARGE_INTEGER *)&yabsys.tickfreq);
#elif defined(_arch_dreamcast)
   yabsys.tickfreq = 1000;
#elif defined(GEKKO)
   yabsys.tickfreq = secs_to_ticks(1);
#elif defined(PSP)
   yabsys.tickfreq = 1000000;
#elif defined(ANDROID)
   yabsys.tickfreq = 1000000;
#elif defined(HAVE_GETTIMEOFDAY)
   yabsys.tickfreq = 1000000;
#elif defined(HAVE_LIBSDL)
   yabsys.tickfreq = 1000;
#endif
   yabsys.OneFrameTime =
      type ? (yabsys.tickfreq / 50) : (yabsys.tickfreq * 1001 / 60000);
   Vdp2Regs->TVSTAT = Vdp2Regs->TVSTAT | (type & 0x1);
   ScspChangeVideoFormat(type);
   YabauseChangeTiming(yabsys.CurSH2FreqType);
}

//////////////////////////////////////////////////////////////////////////////

void YabauseSpeedySetup(void)
{
   u32 data;
   int i;

   if (yabsys.emulatebios)
      BiosInit(MSH2);
   else
   {
      // Setup the vector table area, etc.(all bioses have it at 0x00000600-0x00000810)
      for (i = 0; i < 0x210; i+=4)
      {
         data = MappedMemoryReadLong(MSH2, 0x00000600+i);
         MappedMemoryWriteLong(MSH2, 0x06000000+i, data);
      }

      // Setup the bios function pointers, etc.(all bioses have it at 0x00000820-0x00001100)
      for (i = 0; i < 0x8E0; i+=4)
      {
         data = MappedMemoryReadLong(MSH2, 0x00000820+i);
         MappedMemoryWriteLong(MSH2, 0x06000220+i, data);
      }

      // I'm not sure this is really needed
      for (i = 0; i < 0x700; i+=4)
      {
         data = MappedMemoryReadLong(MSH2, 0x00001100+i);
         MappedMemoryWriteLong(MSH2, 0x06001100+i, data);
      }

      // Fix some spots in 0x06000210-0x0600032C area
      MappedMemoryWriteLong(MSH2, 0x06000234, 0x000002AC);
      MappedMemoryWriteLong(MSH2, 0x06000238, 0x000002BC);
      MappedMemoryWriteLong(MSH2, 0x0600023C, 0x00000350);
      MappedMemoryWriteLong(MSH2, 0x06000240, 0x32524459);
      MappedMemoryWriteLong(MSH2, 0x0600024C, 0x00000000);
      MappedMemoryWriteLong(MSH2, 0x06000268, MappedMemoryReadLong(MSH2, 0x00001344));
      MappedMemoryWriteLong(MSH2, 0x0600026C, MappedMemoryReadLong(MSH2, 0x00001348));
      MappedMemoryWriteLong(MSH2, 0x0600029C, MappedMemoryReadLong(MSH2, 0x00001354));
      MappedMemoryWriteLong(MSH2, 0x060002C4, MappedMemoryReadLong(MSH2, 0x00001104));
      MappedMemoryWriteLong(MSH2, 0x060002C8, MappedMemoryReadLong(MSH2, 0x00001108));
      MappedMemoryWriteLong(MSH2, 0x060002CC, MappedMemoryReadLong(MSH2, 0x0000110C));
      MappedMemoryWriteLong(MSH2, 0x060002D0, MappedMemoryReadLong(MSH2, 0x00001110));
      MappedMemoryWriteLong(MSH2, 0x060002D4, MappedMemoryReadLong(MSH2, 0x00001114));
      MappedMemoryWriteLong(MSH2, 0x060002D8, MappedMemoryReadLong(MSH2, 0x00001118));
      MappedMemoryWriteLong(MSH2, 0x060002DC, MappedMemoryReadLong(MSH2, 0x0000111C));
      MappedMemoryWriteLong(MSH2, 0x06000328, 0x000004C8);
      MappedMemoryWriteLong(MSH2, 0x0600032C, 0x00001800);

      // Fix SCU interrupts
      for (i = 0; i < 0x80; i+=4)
         MappedMemoryWriteLong(MSH2, 0x06000A00+i, 0x0600083C);
   }

   // Set the cpu's, etc. to sane states

   // Set CD block to a sane state
   Cs2Area->reg.HIRQ = 0xFC1;
   Cs2Area->isdiskchanged = 0;
   Cs2Area->reg.CR1 = (Cs2Area->status << 8) | ((Cs2Area->options & 0xF) << 4) | (Cs2Area->repcnt & 0xF);
   Cs2Area->reg.CR2 = (Cs2Area->ctrladdr << 8) | Cs2Area->track;
   Cs2Area->reg.CR3 = (Cs2Area->index << 8) | ((Cs2Area->FAD >> 16) & 0xFF);
   Cs2Area->reg.CR4 = (u16) Cs2Area->FAD; 
   Cs2Area->satauth = 4;

   // Set Master SH2 registers accordingly
   SH2GetRegisters(MSH2, &MSH2->regs);
   for (i = 0; i < 15; i++)
      MSH2->regs.R[i] = 0x00000000;
   MSH2->regs.R[15] = 0x06002000;
   MSH2->regs.SR.all = 0x00000000;
   MSH2->regs.GBR = 0x00000000;
   MSH2->regs.VBR = 0x06000000;
   MSH2->regs.MACH = 0x00000000;
   MSH2->regs.MACL = 0x00000000;
   MSH2->regs.PR = 0x00000000;
   SH2SetRegisters(MSH2, &MSH2->regs);

   // Set SCU registers to sane states
   ScuRegs->D1AD = ScuRegs->D2AD = 0;
   ScuRegs->D0EN = 0x101;
   ScuRegs->IST = 0x2006;
   ScuRegs->AIACK = 0x1;
   ScuRegs->ASR0 = ScuRegs->ASR1 = 0x1FF01FF0;
   ScuRegs->AREF = 0x1F;
   ScuRegs->RSEL = 0x1;

   // Set SMPC registers to sane states
   SmpcRegs->COMREG = 0x10;
   SmpcInternalVars->resd = 0;

   // Set VDP1 registers to sane states
   Vdp1Regs->EDSR = 3;
   Vdp1Regs->localX = 160;
   Vdp1Regs->localY = 112;
   Vdp1Regs->systemclipX2 = 319;
   Vdp1Regs->systemclipY2 = 223;

   // Set VDP2 registers to sane states
   memset(Vdp2Regs, 0, sizeof(Vdp2));
   Vdp2Regs->TVMD = 0x8000;
   Vdp2Regs->TVSTAT = 0x020A;
   Vdp2Regs->CYCA0L = 0x0F44;
   Vdp2Regs->CYCA0U = 0xFFFF;
   Vdp2Regs->CYCA1L = 0xFFFF;
   Vdp2Regs->CYCA1U = 0xFFFF;
   Vdp2Regs->CYCB0L = 0xFFFF;
   Vdp2Regs->CYCB0U = 0xFFFF;
   Vdp2Regs->CYCB1L = 0xFFFF;
   Vdp2Regs->CYCB1U = 0xFFFF;
   Vdp2Regs->BGON = 0x0001;
   Vdp2Regs->PNCN0 = 0x8000;
   Vdp2Regs->MPABN0 = 0x0303;
   Vdp2Regs->MPCDN0 = 0x0303;
   Vdp2Regs->ZMXN0.all = 0x00010000;
   Vdp2Regs->ZMYN0.all = 0x00010000;
   Vdp2Regs->ZMXN1.all = 0x00010000;
   Vdp2Regs->ZMYN1.all = 0x00010000;
   Vdp2Regs->BKTAL = 0x4000;
   Vdp2Regs->SPCTL = 0x0020;
   Vdp2Regs->PRINA = 0x0007;
   Vdp2Regs->CLOFEN = 0x0001;
   Vdp2Regs->COAR = 0x0200;
   Vdp2Regs->COAG = 0x0200;
   Vdp2Regs->COAB = 0x0200;
}

//////////////////////////////////////////////////////////////////////////////

int YabauseQuickLoadGame(void)
{
   partition_struct * lgpartition;
   u8 *buffer;
   u32 addr;
   u32 size;
   u32 blocks;
   unsigned int i, i2;
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
               MappedMemoryWriteByte(MSH2, 0x06002000 + (i * 0x800) + i2, buffer[i2]);
         }
         else
         {
            for (i2 = 0; i2 < size; i2++)
               MappedMemoryWriteByte(MSH2, 0x06002000 + (i * 0x800) + i2, buffer[i2]);
         }

         size -= 2048;

         // Free Block
         lgpartition->size = 0;
         Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
         lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
         lgpartition->numblocks = 0;
      }

      SH2WriteNotify(MSH2, 0x6002000, blocks<<11);

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
               MappedMemoryWriteByte(MSH2, addr + (i * 0x800) + i2, buffer[i2]);
         }
         else
         {
            for (i2 = 0; i2 < size; i2++)
               MappedMemoryWriteByte(MSH2, addr + (i * 0x800) + i2, buffer[i2]);
         }

         size -= 2048;

         // Free Block
         lgpartition->size = 0;
         Cs2FreeBlock(lgpartition->block[lgpartition->numblocks - 1]);
         lgpartition->blocknum[lgpartition->numblocks - 1] = 0xFF;
         lgpartition->numblocks = 0;
      }

      SH2WriteNotify(MSH2, addr, blocks<<11);

      // Now setup SH2 registers to start executing at ip code
      SH2GetRegisters(MSH2, &MSH2->regs);
      MSH2->onchip.VCRC = 0x64 << 8;
      MSH2->onchip.IPRB = 0x0F00;
      MSH2->regs.PC = Cs2GetMasterExecutionAdress();
      MSH2->regs.R[15] = Cs2GetMasterStackAdress();
      SH2SetRegisters(MSH2, &MSH2->regs);
      //OnchipWriteByte(0x92, 0X1); //Enable cache support

      Cs2InitializeCDSystem();
      Cs2Area->reg.CR1 = 0x48fc;
      Cs2Area->reg.CR2 = 0x0;
      Cs2Area->reg.CR3 = 0x0;
      Cs2Area->reg.CR4 = 0x0;
      Cs2ResetSelector();
      Cs2GetToc();

      // LANGRISSER Dramatic Edition #531
      // This game uses Color ram data written by BIOS
      // So it need to write them before start game on no bios mode.
      Vdp2WriteWord(MSH2, NULL, 0x0E,0); // set color mode to 0
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x0, 0x8000);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x2, 0x9908);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x4, 0xCA94);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x6, 0xF39C);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x8, 0xFBDE);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0xA, 0xFB16);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0xC, 0x9084);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0xE, 0xF20C);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x10, 0xF106);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x12, 0xF18A);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x14, 0xB9CE);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x16, 0xA14A);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x18, 0xE318);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x1A, 0xEB5A);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x1C, 0xF39C);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0x1E, 0xFBDE);
      Vdp2ColorRamWriteWord(MSH2, Vdp2ColorRam, 0xFF, 0x0000);
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
