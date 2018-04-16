/*  Copyright 2006 Guillaume Duhamel
    Copyright 2006 Fabien Coulon
    Copyright 2005 Joost Peters

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

#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#include "platform.h"

#include "../yabause.h"
#include "../gameinfo.h"
#include "../yui.h"
#include "../peripheral.h"
#include "../sh2core.h"
#include "../sh2int.h"
#ifdef HAVE_LIBGL
#include "../vidogl.h"
#include "../ygl.h"
#endif
#include "../vidsoft.h"
#include "../cs0.h"
#include "../cs2.h"
#include "../cdbase.h"
#include "../scsp.h"
#include "../sndsdl.h"
#include "../sndal.h"
#include "../persdljoy.h"
#ifdef ARCH_IS_LINUX
#include "../perlinuxjoy.h"
#endif
#include "../debug.h"
#include "../m68kcore.h"
#include "../m68kc68k.h"
#include "../vdp1.h"
#include "../vdp2.h"
#include "../cdbase.h"
#include "../peripheral.h"
#ifdef DYNAREC_KRONOS
#include "../sh2_kronos/sh2int_kronos.h"
#endif

#define AR (4.0f/3.0f)
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT ((int)((float)WINDOW_WIDTH/AR))

#define  WINDOW_WIDTH_LOW 600
#define WINDOW_HEIGHT_LOW ((int)((float)WINDOW_WIDTH_LOW/AR))

static int Wwidth;
static int Wheight;

M68K_struct * M68KCoreList[] = {
&M68KDummy,
#ifdef HAVE_MUSASHI
&M68KMusashi,
#endif
#ifdef HAVE_C68K
&M68KC68K,
#endif
#ifdef HAVE_Q68
&M68KQ68,
#endif
NULL
};

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
#ifdef TEST_PSP_SH2
&SH2PSP,
#endif
#ifdef DYNAREC_DEVMIYAX
&SH2Dyn,
#endif
#ifdef DYNAREC_KRONOS
&SH2KronosInterpreter,
#endif
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
#ifdef ARCH_IS_LINUX
&PERLinuxJoy,
#endif
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
#ifndef UNKNOWN_ARCH
&ArchCD,
#endif
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
#ifdef HAVE_LIBSDL
&SNDSDL,
#endif
#ifdef HAVE_LIBAL
&SNDAL,
#endif
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
#ifdef HAVE_LIBGL
&VIDOGL,
#endif
&VIDSoft,
NULL
};

#ifdef YAB_PORT_OSD
#include "nanovg/nanovg_osdcore.h"
OSD_struct *OSDCoreList[] = {
&OSDNnovg,
NULL
};
#endif

static int fullscreen = 0;
static int lowres_mode = 0;

static char biospath[256] = "\0";
static char cdpath[256] = "\0";

yabauseinit_struct yinit;

void YuiErrorMsg(const char * string) {
    fprintf(stderr, "%s\n\r", string);
}

int YuiRevokeOGLOnThisThread(){
  platform_YuiRevokeOGLOnThisThread();
  return 0;
}

int YuiUseOGLOnThisThread(){
  platform_YuiUseOGLOnThisThread();
  return 0;
}

static unsigned long nextFrameTime = 0;
static unsigned long delayUs_NTSC = 1000000/60;
static unsigned long delayUs_PAL = 1000000/50;

#define delayUs ((yabsys.IsPal)?delayUs_PAL:delayUs_NTSC);

static int frameskip = 1;

static unsigned long getCurrentTimeUs(unsigned long offset) {
    struct timeval s;

    gettimeofday(&s, NULL);

    return (s.tv_sec * 1000000 + s.tv_usec) - offset;
}

static unsigned long time_left(void)
{
    unsigned long now;

    now = getCurrentTimeUs(0);
    if(nextFrameTime <= now)
        return 0;
    else
        return nextFrameTime - now;
}

void YuiSwapBuffers(void) {

   platform_swapBuffers();
   SetOSDToggle(1);

  if (frameskip == 1)
    usleep(time_left());

  nextFrameTime += delayUs;
}

void YuiInit() {
	yinit.m68kcoretype = M68KCORE_MUSASHI;
	yinit.percoretype = PERCORE_LINUXJOY;
        #if defined(DYNAREC_KRONOS)
          printf("Use kronos specific core emulation\n");
          yinit.sh2coretype = 8;
        #else
	  yinit.sh2coretype = 0;
        #endif
#ifdef FORCE_CORE_SOFT
  yinit.vidcoretype = VIDCORE_SOFT;
#else
	yinit.vidcoretype = VIDCORE_OGL; //VIDCORE_SOFT  
#endif
#ifdef HAVE_LIBSDL
	yinit.sndcoretype = SNDCORE_SDL;
#else
	yinit.sndcoretype = 0;
#endif
	yinit.cdcoretype = CDCORE_DEFAULT;
	yinit.carttype = CART_DRAM32MBIT;
	yinit.regionid = REGION_EUROPE;
	yinit.biospath = NULL;
	yinit.cdpath = NULL;
	yinit.buppath = "./bup.ram";
        yinit.extend_backup = 1;
	yinit.mpegpath = NULL;
	yinit.cartpath = "./backup32Mb.ram";
	yinit.osdcoretype = OSDCORE_DEFAULT;
	yinit.skip_load = 0;

	yinit.usethreads = 1;
	yinit.numthreads = 4;
        yinit.usecache = 0;
#ifdef SPRITE_CACHE
        yinit.useVdp1cache = 0;
#endif
}

static int SetupOpenGL() {
  int w = (lowres_mode == 0)?WINDOW_WIDTH:WINDOW_WIDTH_LOW;
  int h = (lowres_mode == 0)?WINDOW_HEIGHT:WINDOW_HEIGHT_LOW;
  Wwidth = w;
  Wheight = h;
  if (!platform_SetupOpenGL(w,h,fullscreen))
    exit(EXIT_FAILURE);
}

void displayGameInfo(char *filename) {
  GameInfo info;
  if (! GameInfoFromPath(filename, &info))
  {
    return;
  }

  printf("Game Info:\n\tSystem: %s\n\tCompany: %s\n\tItemNum:%s\n\tVersion:%s\n\tDate:%s\n\tCDInfo:%s\n\tRegion:%s\n\tPeripheral:%s\n\tGamename:%s\n", info.system, info.company, info.itemnum, info.version, info.date, info.cdinfo, info.region, info.peripheral, info.gamename);
}

void initEmulation() {
   YuiInit();
   SetupOpenGL();
   if (YabauseSh2Init(&yinit) != 0) {
    printf("YabauseSh2Init error \n\r");
    return;
  }
}
#ifndef TEST_MODE
int main(int argc, char *argv[]) {
	int i;

	LogStart();
	LogChangeOutput( DEBUG_STDERR, NULL );

	YuiInit();

//handle command line arguments
  for (i = 1; i < argc; ++i) {
    if (argv[i]) {
      //show usage
      if (0 == strcmp(argv[i], "-h") || 0 == strcmp(argv[i], "-?") || 0 == strcmp(argv[i], "--help")) {
        print_usage(argv[0]);
        return 0;
      }
			
      //set bios
      if (0 == strcmp(argv[i], "-b") && argv[i + 1]) {
        strncpy(biospath, argv[i + 1], 256);
        yinit.biospath = biospath;
      } else if (strstr(argv[i], "--bios=")) {
        strncpy(biospath, argv[i] + strlen("--bios="), 256);
        yinit.biospath = biospath;
      }
      //set iso
      else if (0 == strcmp(argv[i], "-i") && argv[i + 1]) {
        strncpy(cdpath, argv[i + 1], 256);
        yinit.cdcoretype = 1;
        yinit.cdpath = cdpath;
        displayGameInfo(cdpath);
      } else if (strstr(argv[i], "--iso=")) {
        strncpy(cdpath, argv[i] + strlen("--iso="), 256);
        yinit.cdcoretype = 1;
        yinit.cdpath = cdpath;
      }
      //set cdrom
      else if (0 == strcmp(argv[i], "-c") && argv[i + 1]) {
        strncpy(cdpath, argv[i + 1], 256);
        yinit.cdcoretype = 2;
        yinit.cdpath = cdpath;
      } else if (0 == strcmp(argv[i], "-c") && argv[i + 1]) {
        strncpy(cdpath, argv[i + 1], 256);
        yinit.cdcoretype = 2;
        yinit.cdpath = cdpath;
      } else if (strstr(argv[i], "--cache")) {
        yinit.usecache = 1;
      }
      // Set sound
      else if (strcmp(argv[i], "-ns") == 0 || strcmp(argv[i], "--nosound") == 0) {
        yinit.sndcoretype = 0;
      }
      // Set sound
      else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fullscreen") == 0) {
        fullscreen = 1;
      }
      // Low resolution mode
      else if (strcmp(argv[i], "-lr") == 0 || strcmp(argv[i], "--lowres") == 0) {
        lowres_mode = 1;
      }
      else if (strcmp(argv[i], "-sc") == 0 || strcmp(argv[i], "--softcore") == 0) {
        yinit.vidcoretype = VIDCORE_SOFT;
      }
#ifdef SPRITE_CACHE
      else if (strcmp(argv[i], "-vc") == 0 || strcmp(argv[i], "--vdp1cache") == 0) {
        yinit.useVdp1cache = 1;;
      }
#endif
      else if (strcmp(argv[i], "-ci") == 0 ) {
        yinit.sh2coretype = 1;
      }
      else if (strcmp(argv[i], "-cd") == 0 ) {
      #if defined(DYNAREC_DEVMIYAX)
          printf("Use new dynarec core emulation\n");
          yinit.sh2coretype = 3;
      #else
        #if defined(DYNAREC_KRONOS)
          printf("Use kronos specific core emulation\n");
          yinit.sh2coretype = 8;
        #else
          printf("No dynarec core emulation: fallback on SW core emulation\n");
          yinit.sh2coretype = 0;
        #endif
      #endif
      }

      // Auto frame skip
      else if (strstr(argv[i], "--vsyncoff")) {
        frameskip = 0;
        EnableAutoFrameSkip();
      }
      // Binary
      else if (strstr(argv[i], "--binary=")) {
        char binname[1024];
        unsigned int binaddress;
        int bincount;

        bincount = sscanf(argv[i] + strlen("--binary="), "%[^:]:%x", binname, &binaddress);
        if (bincount > 0) {
          if (bincount < 2) binaddress = 0x06004000;
          MappedMemoryLoadExec(MSH2, binname, binaddress);
        }
      }
    }
  }
  SetupOpenGL();

	YabauseDeInit();

  if (YabauseInit(&yinit) != 0) {
    printf("YabauseInit error \n\r");
    return 1;
  }

  if (yinit.vidcoretype == VIDCORE_SOFT) {
    OSDChangeCore(OSDCORE_SOFT);
    VIDSoftSetBilinear(1);
  }

  if (lowres_mode == 0){
    if (yinit.vidcoretype == VIDCORE_OGL) {
      VIDCore->SetSettingValue(VDP_SETTING_RESOLUTION_MODE, RES_2x);
      VIDCore->SetSettingValue(VDP_SETTING_FILTERMODE, AA_BILINEAR_FILTER);
      VIDCore->SetSettingValue(VDP_SETTING_UPSCALMODE, UP_4XBRZ);
    }
    VIDCore->Resize(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1);
  } else {
    if (yinit.vidcoretype == VIDCORE_OGL) {
      VIDCore->SetSettingValue(VDP_SETTING_RESOLUTION_MODE, RES_2x);
      VIDCore->SetSettingValue(VDP_SETTING_FILTERMODE, AA_BILINEAR_FILTER);
      VIDCore->SetSettingValue(VDP_SETTING_UPSCALMODE, UP_HQ4X);
    }
    VIDCore->Resize(0, 0, WINDOW_WIDTH_LOW, WINDOW_HEIGHT_LOW, 1);
  }

  platform_SetKeyCallback(PERCore->onKeyEvent);

  nextFrameTime = getCurrentTimeUs(0) + delayUs;

  while (!platform_shouldClose())
  {
        int height;
        int width;
        platform_getFBSize(&width, &height);
        if ((Wwidth != width) || (Wheight != height)) {
          Wwidth = width;
          Wheight = height;
          VIDCore->Resize(0, 0, Wwidth, Wheight, 1);
        }
        if (PERCore->HandleEvents() == -1) platform_Close();
        platform_HandleEvent();
  }

	YabauseDeInit();
	LogStop();
  platform_Deinit();

	return 0;
}
#endif
