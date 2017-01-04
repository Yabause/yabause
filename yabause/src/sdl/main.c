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

#include <SDL2/SDL.h>

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
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
#ifdef HAVE_LIBSDL
&PERSDLJoy,
#endif
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

static GLint g_FrameBuffer = 0;
static GLint g_SWFrameBuffer = 0;
static GLint g_VertexBuffer = 0;
static GLint g_VertexDevBuffer = 0;
static GLint g_VertexSWBuffer = 0;
static GLint programObject  = 0;
static GLint positionLoc    = 0;
static GLint texCoordLoc    = 0;
static GLint samplerLoc     = 0;

int g_buf_width = -1;
int g_buf_height = -1;

int fbo_buf_width = -1;
int fbo_buf_height = -1;

static int resizeFilter = GL_NEAREST;
static int fullscreen = 0;

static char biospath[256] = "\0";
static char cdpath[256] = "\0";

SDL_Window *window;
SDL_Texture* fb;
SDL_Renderer* rdr;

yabauseinit_struct yinit;

static int error;

static float vertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 1.0f, 0,
   1.0f, -1.0f, 1.0f, 1.0f,
   -1.0f,-1.0f, 0, 1.0f
};

static float swVertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 1.0f, 0,
   1.0f, -1.0f, 1.0f, 1.0f,
   -1.0f,-1.0f, 0, 1.0f
};

static const float squareVertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 1.0f, 0,
   1.0f, -1.0f, 1.0f, 1.0f,
   -1.0f,-1.0f, 0, 1.0f
};


static float devVertices [] = {
   -1.0f, 1.0f, 0, 0,
   1.0f, 1.0f, 1.0f, 0,
   1.0f, -1.0f, 1.0f, 1.0f,
   -1.0f,-1.0f, 0, 1.0f
};

void YuiErrorMsg(const char * string) {
    fprintf(stderr, "%s\n\r", string);
}

void YuiDrawSoftwareBuffer() {

    int buf_width, buf_height;
    int error;

    VIDCore->GetGlSize(&buf_width, &buf_height);
   SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = buf_width;
  rect.h = buf_height;
  SDL_Rect dest;
  dest.x = 0;
  dest.y = 0;
  dest.w = WINDOW_WIDTH;
  dest.h = WINDOW_HEIGHT;
   SDL_UpdateTexture(fb,&rect,dispbuffer,buf_width*sizeof(pixel_t));
   SDL_RenderCopy(rdr,fb,&rect,&dest);
   SDL_RenderPresent(rdr);
}

int YuiRevokeOGLOnThisThread(){
  // Todo: needs to imp for async rendering
  return 0;
}

int YuiUseOGLOnThisThread(){
  // Todo: needs to imp for async rendering
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

   if( window == NULL ){
      return;
   }

   SDL_GL_SwapWindow(window);
  if (frameskip == 1)
    usleep(time_left());

  nextFrameTime += delayUs;

   //lastFrameTime = getCurrentTimeUs(0);
}

void YuiInit() {
	yinit.m68kcoretype = M68KCORE_MUSASHI;
	yinit.percoretype = PERCORE_SDLJOY;
	yinit.sh2coretype = SH2CORE_DEFAULT;
#ifdef FORCE_CORE_SOFT
        yinit.vidcoretype = VIDCORE_SOFT;
#else
	yinit.vidcoretype = VIDCORE_OGL; //VIDCORE_SOFT  
#endif
	yinit.sndcoretype = SNDCORE_SDL;
	yinit.cdcoretype = CDCORE_DEFAULT;
	yinit.carttype = CART_NONE;
	yinit.regionid = REGION_EUROPE;
	yinit.biospath = NULL;
	yinit.cdpath = NULL;
	yinit.buppath = NULL;
	yinit.mpegpath = NULL;
	yinit.cartpath = NULL;
        yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
	yinit.osdcoretype = OSDCORE_DEFAULT;
	yinit.skip_load = 0;

	yinit.usethreads = 1;
	yinit.numthreads = 4;
}

void SDLInit(void) {
	SDL_GLContext context;
	Uint32 flags = (fullscreen == 1)?SDL_WINDOW_FULLSCREEN|SDL_WINDOW_OPENGL:SDL_WINDOW_OPENGL;

	SDL_InitSubSystem(SDL_INIT_VIDEO);

#ifdef _OGL_
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	SDL_GL_SetSwapInterval(1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);

	
	window = SDL_CreateWindow("OpenGL Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, flags);
	if (!window) {
    		fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
    		return;
	}

	context = SDL_GL_CreateContext(window);
	if (!context) {
    		fprintf(stderr, "Couldn't create context: %s\n", SDL_GetError());
    		return;
	}

	rdr = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

#ifdef _OGL_
        glClearColor( 0.0f,0.0f,0.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
#else
        SDL_RenderClear(rdr);
#ifdef USE_16BPP
        fb = SDL_CreateTexture(rdr,SDL_PIXELFORMAT_ABGR1555,SDL_TEXTUREACCESS_STREAMING, 704, 512);
#else
        fb = SDL_CreateTexture(rdr,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STREAMING, 704, 512);
#endif
#endif

  nextFrameTime = getCurrentTimeUs(0) + delayUs;
}

void displayGameInfo(char *filename) {
    GameInfo info;
    if (! GameInfoFromPath(filename, &info))
    {
       return;
    }

    printf("Game Info:\n\tSystem: %s\n\tCompany: %s\n\tItemNum:%s\n\tVersion:%s\n\tDate:%s\n\tCDInfo:%s\n\tRegion:%s\n\tPeripheral:%s\n\tGamename:%s\n", info.system, info.company, info.itemnum, info.version, info.date, info.cdinfo, info.region, info.peripheral, info.gamename);
}

int main(int argc, char *argv[]) {
	int i;
	SDL_Event event;
	int quit = SDL_FALSE;
	SDL_GameController* ctrl;

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
         // Set sound
         else if (strcmp(argv[i], "-rb") == 0 || strcmp(argv[i], "--resizebilinear") == 0) {
	    resizeFilter = GL_LINEAR;
	 }
	 else if (strcmp(argv[i], "-sc") == 0 || strcmp(argv[i], "--softcore") == 0) {
	    yinit.vidcoretype = VIDCORE_SOFT;
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

               //yui_window_run(YUI_WINDOW(yui));
	       MappedMemoryLoadExec(binname, binaddress);
	    }
	 }
      }
   }
        SDLInit();

	YabauseDeInit();

        if (YabauseInit(&yinit) != 0) printf("YabauseInit error \n\r");

  if (yinit.vidcoretype == VIDCORE_OGL) {
    VIDCore->SetSettingValue(VDP_SETTING_RESOLUTION_MODE, RES_ORIGINAL);
    VIDCore->Resize(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1);
  }

	while (quit == SDL_FALSE)
	{
		if (SDL_PollEvent(&event))
		{
   			switch(event.type)
    			{
     				case SDL_QUIT:
      					quit = SDL_TRUE;
      				break;
				case SDL_KEYDOWN:
					printf("Key down!");
				break;
     			}
  		}

	        PERCore->HandleEvents();
	}

	YabauseDeInit();
	LogStop();
	SDL_Quit();

	return 0;
}
