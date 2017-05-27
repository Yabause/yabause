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
#if defined(_USEGLEW_)
#include <GL/glew.h>
#endif
#include <GLFW/glfw3.h>

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

#define AR (4.0f/3.0f)
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT ((int)((float)WINDOW_WIDTH/AR))

#define  WINDOW_WIDTH_LOW 600
#define WINDOW_HEIGHT_LOW ((int)((float)WINDOW_WIDTH_LOW/AR))


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
#ifdef SH2_DYNAREC
&SH2Dynarec,
#endif
#ifdef DYNAREC_DEVMIYAX
&SH2Dyn,
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

#define FORCE_CORE_SOFT

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

GLFWwindow* g_window = NULL;
GLFWwindow* g_offscreen_context;

yabauseinit_struct yinit;

void YuiErrorMsg(const char * string) {
    fprintf(stderr, "%s\n\r", string);
}

int YuiRevokeOGLOnThisThread(){
#if defined(YAB_ASYNC_RENDERING)
  glfwMakeContextCurrent(g_offscreen_context);
#endif
  return 0;
}

int YuiUseOGLOnThisThread(){
#if defined(YAB_ASYNC_RENDERING)
  glfwMakeContextCurrent(g_window);
#endif
  return 0;
}

static unsigned long nextFrameTime = 0;
static unsigned long delayUs = 1000000/60;

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

   if( g_window == NULL ){
      return;
   }

   glfwSwapBuffers(g_window);
   SetOSDToggle(1);

  if (frameskip == 1)
    usleep(time_left());

  nextFrameTime += delayUs;
}

void YuiInit() {
	yinit.m68kcoretype = M68KCORE_MUSASHI;
	yinit.percoretype = PERCORE_LINUXJOY;
#if defined(DYNAREC_DEVMIYAX)
  yinit.sh2coretype = 3;
#elif defined(SH2_DYNAREC)
//	yinit.sh2coretype = 2;
#else
	yinit.sh2coretype = 0;
#endif
#ifdef FORCE_CORE_SOFT
  yinit.vidcoretype = 0; //VIDCORE_SOFT;
#else
	yinit.vidcoretype = VIDCORE_OGL; //VIDCORE_SOFT  
#endif
	yinit.sndcoretype = SNDCORE_SDL;
	yinit.cdcoretype = CDCORE_DEFAULT;
	yinit.carttype = CART_DRAM32MBIT;
	yinit.regionid = REGION_EUROPE;
	yinit.biospath = NULL;
	yinit.cdpath = NULL;
	yinit.buppath = NULL;
	yinit.mpegpath = NULL;
	yinit.cartpath = "./backup32Mb.ram";
  yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
	yinit.osdcoretype = OSDCORE_DEFAULT;
	yinit.skip_load = 0;

	yinit.usethreads = 1;
	yinit.numthreads = 4;
}

void error_callback(int error, const char* description)
{
  fputs(description, stderr);
}

static int SetupOpenGL() {
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwSetErrorCallback(error_callback);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API) ;
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RED_BITS,8);
  glfwWindowHint(GLFW_GREEN_BITS,8);
  glfwWindowHint(GLFW_BLUE_BITS,8);

  if (lowres_mode == 0){
    g_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Yabause", NULL, NULL);
  } else {
    g_window = glfwCreateWindow(WINDOW_WIDTH_LOW, WINDOW_HEIGHT_LOW, "Yabause", NULL, NULL);

  }
  if (!g_window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
  g_offscreen_context = glfwCreateWindow(WINDOW_WIDTH,WINDOW_HEIGHT, "", NULL, g_window);

  glfwMakeContextCurrent(g_window);
  glfwSwapInterval(0);
  glewExperimental=GL_TRUE;

}

void displayGameInfo(char *filename) {
  GameInfo info;
  if (! GameInfoFromPath(filename, &info))
  {
    return;
  }

  printf("Game Info:\n\tSystem: %s\n\tCompany: %s\n\tItemNum:%s\n\tVersion:%s\n\tDate:%s\n\tCDInfo:%s\n\tRegion:%s\n\tPeripheral:%s\n\tGamename:%s\n", info.system, info.company, info.itemnum, info.version, info.date, info.cdinfo, info.region, info.peripheral, info.gamename);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

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
      } else if (strstr(argv[i], "--cdrom=")) {
        strncpy(cdpath, argv[i] + strlen("--cdrom="), 256);
        yinit.cdcoretype = 2;
        yinit.cdpath = cdpath;
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
      else if (strcmp(argv[i], "-ci") == 0 ) {
        yinit.sh2coretype = 1;
      }

      // Auto frame skip
      else if (strstr(argv[i], "--vsyncoff")) {
        frameskip = 0;
      }
      // Binary
      else if (strstr(argv[i], "--binary=")) {
        char binname[1024];
        unsigned int binaddress;
        int bincount;

        bincount = sscanf(argv[i] + strlen("--binary="), "%[^:]:%x", binname, &binaddress);
        if (bincount > 0) {
          if (bincount < 2) binaddress = 0x06004000;
          MappedMemoryLoadExec(binname, binaddress);
        }
      }
    }
  }
  SetupOpenGL();

  glfwSetKeyCallback(g_window, key_callback);

	YabauseDeInit();

  if (YabauseInit(&yinit) != 0) printf("YabauseInit error \n\r");

  if (yinit.vidcoretype == VIDCORE_OGL) {
    if (lowres_mode == 0){
      VIDCore->SetSettingValue(VDP_SETTING_RESOLUTION_MODE, RES_NATIVE);
      VIDCore->SetSettingValue(VDP_SETTING_FILTERMODE, AA_FXAA);
      VIDCore->Resize(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1);
    } else {
      VIDCore->SetSettingValue(VDP_SETTING_RESOLUTION_MODE, RES_ORIGINAL);
      VIDCore->SetSettingValue(VDP_SETTING_FILTERMODE, AA_NONE);
      VIDCore->Resize(0, 0, WINDOW_WIDTH_LOW, WINDOW_HEIGHT_LOW, 1);
    }
  }

  nextFrameTime = getCurrentTimeUs(0) + delayUs;

  while (!glfwWindowShouldClose(g_window))
  {
        if (PERCore->HandleEvents() == -1) glfwSetWindowShouldClose(g_window, GL_TRUE);
        glfwPollEvents();
  }

	YabauseDeInit();
	LogStop();
  glfwDestroyWindow(g_window);
  glfwDestroyWindow(g_offscreen_context);
  glfwTerminate();

	return 0;
}
