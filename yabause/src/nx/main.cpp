/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <exception>
#include <functional>
#include <string>  
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/resource.h>
#include <errno.h>
#include <pthread.h>

#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>

#include <switch.h>

extern "C" {
#include "../config.h"
#include "yabause.h"
#include "vdp2.h"
#include "scsp.h"
#include "vidsoft.h"
#include "vidogl.h"
#include "peripheral.h"
#include "persdljoy.h"
#include "m68kcore.h"
#include "sh2core.h"
#include "sh2int.h"
#include "cdbase.h"
#include "cs2.h"
#include "debug.h"
#include "sndal.h"
#include "sndsdl.h"
#include "osdcore.h"
#include "ygl.h"
//#include "libpng12/png.h"
}

static EGLDisplay s_display;
static EGLContext s_context;
static EGLSurface s_surface;

//#include "InputManager.h"
//#include "MenuScreen.h"
#define ENABLE_NXLINK
#ifndef ENABLE_NXLINK
#define TRACE(fmt,...) ((void)0)
#else
#include <unistd.h>
#define TRACE(fmt,...) printf("%s: " fmt "\n", __PRETTY_FUNCTION__, ## __VA_ARGS__)
static int s_nxlinkSock = -1;

static void initNxLink()
{
	if (R_FAILED(socketInitializeDefault()))
		return;

	s_nxlinkSock = nxlinkStdio();
	if (s_nxlinkSock >= 0)
		TRACE("printf output now goes to nxlink server");
	else
		socketExit();
}

static void deinitNxLink()
{
	if (s_nxlinkSock >= 0)
	{
		close(s_nxlinkSock);
		socketExit();
		s_nxlinkSock = -1;
	}
}

extern "C" {

void userAppInit()
{
	initNxLink();
}

void userAppExit()
{
	deinitNxLink();
}
}

#endif

extern "C" {
static char biospath[256] = "./yabasanshiro/bios.bin";
static char cdpath[256] = "./yabasanshiro/nights.cue";
//static char cdpath[256] = "/home/pigaming/RetroPie/roms/saturn/gd.cue";
//static char cdpath[256] = "/home/pigaming/RetroPie/roms/saturn/Virtua Fighter Kids (1996)(Sega)(JP).ccd";
static char buppath[256] = "./back.bin";
static char mpegpath[256] = "\0";
static char cartpath[256] = "\0";

#define LOG

M68K_struct * M68KCoreList[] = {
  &M68KDummy,
  #ifdef HAVE_C68K
  &M68KC68K,
  #endif
  #ifdef HAVE_Q68
  &M68KQ68,
  #endif
#ifdef HAVE_MUSASHI
  &M68KMusashi,
#endif
  NULL
};

SH2Interface_struct *SH2CoreList[] = {
  &SH2Interpreter,
  &SH2DebugInterpreter,
#ifdef SH2_DYNAREC
  &SH2Dynarec,
#endif
#if DYNAREC_DEVMIYAX
  &SH2Dyn,
  &SH2DynDebug,
#endif
  NULL
};

PerInterface_struct *PERCoreList[] = {
  &PERDummy,
  &PERSDLJoy,
  NULL
};

CDInterface *CDCoreList[] = {
  &DummyCD,
  &ISOCD,
  NULL
};

SoundInterface_struct *SNDCoreList[] = {
  &SNDDummy,
#ifdef HAVE_LIBSDL
  &SNDSDL,
#endif
  NULL
};

VideoInterface_struct *VIDCoreList[] = {
  &VIDDummy,
  &VIDOGL,
  NULL
};


#ifdef YAB_PORT_OSD
#include "nanovg/nanovg_osdcore.h"
OSD_struct *OSDCoreList[] = {
  &OSDNnovg,
  &OSDDummy,
  NULL
};
#endif


static SDL_Window* wnd;
static SDL_GLContext glc;
int g_EnagleFPS = 1;
int g_resolution_mode = 0;
int g_keep_aspect_rate = 0;
int g_scsp_sync = 1;
int g_frame_skip = 0;
int g_emulated_bios = 0;
//InputManager* inputmng;
//MenuScreen * menu;

using std::string;
string g_keymap_filename;

#include "nanovg.h"

//----------------------------------------------------------------------------------------------
NVGcontext * getGlobalNanoVGContext(){
  return NULL;
}

void DrawDebugInfo()
{
}

void YuiErrorMsg(const char *string)
{
  LOG("%s",string);
}

void YuiSwapBuffers(void)
{
  eglSwapBuffers(s_display, s_surface);
  SetOSDToggle(g_EnagleFPS);
}

//void YuiSwapBuffers(void)
//{
//  SDL_GL_SwapWindow(wnd);
//  SetOSDToggle(g_EnagleFPS);
//}

int YuiRevokeOGLOnThisThread(){
  LOG("revoke thread\n");
  //SDL_GL_MakeCurrent(wnd,nullptr);
  eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  return 0;
}

int YuiUseOGLOnThisThread(){
  LOG("use thread\n");
  //SDL_GL_MakeCurrent(wnd,glc);
  eglMakeCurrent(s_display, s_surface, s_surface, s_context);
  return 0;
}

}

int saveScreenshot( const char * filename );

int padmode = 0;

int yabauseinit()
{
  int res;
  yabauseinit_struct yinit = {};

  yinit.m68kcoretype = M68KCORE_MUSASHI;
  yinit.percoretype = PERCORE_DUMMY;
#ifdef SH2_DYNAREC
    yinit.sh2coretype = 2;
#else
  //yinit.sh2coretype = 0;
#endif
  yinit.sh2coretype = SH2CORE_INTERPRETER;
  //yinit.vidcoretype = VIDCORE_SOFT;
  yinit.vidcoretype = VIDCORE_OGL;
  yinit.sndcoretype = SNDCORE_DUMMY;
  //yinit.sndcoretype = SNDCORE_DUMMY;
  //yinit.cdcoretype = CDCORE_DEFAULT;
  yinit.cdcoretype = CDCORE_ISO;
  yinit.carttype = CART_NONE;
  yinit.regionid = 0;
  if( g_emulated_bios ){
    yinit.biospath = NULL;
  }else{
    yinit.biospath = biospath;
  }
  yinit.cdpath = cdpath;
  yinit.buppath = buppath;
  yinit.mpegpath = mpegpath;
  yinit.cartpath = cartpath;
  yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
  yinit.frameskip = g_frame_skip;
  yinit.usethreads = 0;
  yinit.skip_load = 0;    
  yinit.video_filter_type = 0;
  yinit.polygon_generation_mode = PERSPECTIVE_CORRECTION; ////GPU_TESSERATION;
  yinit.use_new_scsp = 1;
  yinit.resolution_mode = g_resolution_mode;
  yinit.rotate_screen = 0;
  yinit.scsp_sync_count_per_frame = g_scsp_sync;
  yinit.extend_backup = 0;
  yinit.scsp_main_mode = 1;

  res = YabauseInit(&yinit);
  if( res == -1) {
    return -1;
  }

  //padmode = inputmng->getCurrentPadMode( 0 );
  OSDInit(0);
  OSDChangeCore(OSDCORE_NANOVG);
  LogStart();
  //LogChangeOutput(DEBUG_CALLBACK, NULL);
  return 0;
}

static bool initEgl(NWindow* win)
{
    // Connect to the EGL default display
    s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!s_display)
    {
        TRACE("Could not connect to display! error: %d", eglGetError());
        goto _fail0;
    }

    // Initialize the EGL display connection
    eglInitialize(s_display, NULL, NULL);

    // Get an appropriate EGL framebuffer configuration
    EGLConfig config;
    EGLint numConfigs;
    static const EGLint framebufferAttributeList[] =
    {
        EGL_RED_SIZE,     8,
        EGL_GREEN_SIZE,   8,
        EGL_BLUE_SIZE,    8,
        EGL_ALPHA_SIZE,   8,
        EGL_DEPTH_SIZE,   24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };
    eglChooseConfig(s_display, framebufferAttributeList, &config, 1, &numConfigs);
    if (numConfigs == 0)
    {
        TRACE("No config found! error: %d", eglGetError());
        goto _fail1;
    }

    // Create an EGL window surface
    s_surface = eglCreateWindowSurface(s_display, config, win, NULL);
    if (!s_surface)
    {
        TRACE("Surface creation failed! error: %d", eglGetError());
        goto _fail1;
    }

    // Create an EGL rendering context
    static const EGLint contextAttributeList[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 3, // request OpenGL ES 2.x
        EGL_NONE
    };
    s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, contextAttributeList);
    if (!s_context)
    {
        TRACE("Context creation failed! error: %d", eglGetError());
        goto _fail2;
    }

    // Connect the context to the surface
    eglMakeCurrent(s_display, s_surface, s_surface, s_context);
    return true;

_fail2:
    eglDestroySurface(s_display, s_surface);
    s_surface = NULL;
_fail1:
    eglTerminate(s_display);
    s_display = NULL;
_fail0:
    return false;
}

static void deinitEgl()
{
    if (s_display)
    {
        eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (s_context)
        {
            eglDestroyContext(s_display, s_context);
            s_context = NULL;
        }
        if (s_surface)
        {
            eglDestroySurface(s_display, s_surface);
            s_surface = NULL;
        }
        eglTerminate(s_display);
        s_display = NULL;
    }
}


int main(int argc, char** argv)
{
  //inputmng = InputManager::getInstance();

  std::string bckup_dir = "./backup.bin";
  strcpy( buppath, bckup_dir.c_str() );

#if 0  
  printf("\033[2J");

  // Inisialize home directory
  std::string home_dir = getenv("HOME");
  home_dir += "/.yabasanshiro/";
  struct stat st = {0};
  if (stat(home_dir.c_str(), &st) == -1) {
    mkdir(home_dir.c_str(), 0700);
  }  
  std::string bckup_dir = home_dir + "backup.bin";
  strcpy( buppath, bckup_dir.c_str() );

  g_keymap_filename = home_dir + "keymapv2.json";

  std::string current_exec_name = argv[0]; // Name of the current exec program
  std::vector<std::string> all_args;
  if (argc > 1) {
    all_args.assign(argv + 1, argv + argc);
    if( all_args[0] == "-h" || all_args[0] == "--h" ){
      printf("Usage:\n");
      printf("  -b STRING  --bios STRING                 bios file\n");
      printf("  -i STRING  --iso STRING                  iso/cue file\n");
      printf("  -r NUMBER  --resolution_mode NUMBER      0 .. Native, 1 .. 4x, 2 .. 2x, 3 .. Original\n");
      printf("  -a         --keep_aspect_rate\n");
      printf("  -s NUMBER  --scps_sync_per_frame NUMBER\n");
      printf("  -nf         --no_frame_skip              disable frame skip\n");    
      printf("  -v         --version\n");    
      exit(0);
    }
  }

  for( int i=0; i<all_args.size(); i++ ){
    string x = all_args[i];
		if(( x == "-b" || x == "--bios") && (i+1<all_args.size() ) ) {
      g_emulated_bios = 0;
      strncpy(biospath, all_args[i+1].c_str(), 256);
    }
		else if(( x == "-i" || x == "--iso") && (i+1<all_args.size() ) ) {
      strncpy(cdpath, all_args[i+1].c_str(), 256);
    }
		else if(( x == "-r" || x == "--resolution_mode") && (i+1<all_args.size() ) ) {
      g_resolution_mode = std::stoi( all_args[i+1] );
    }
		else if(( x == "-a" || x == "--keep_aspect_rate") ) {
      g_keep_aspect_rate = 1;
    }
		else if(( x == "-s" || x == "--g_scsp_sync")&& (i+1<all_args.size() ) ) {
      g_scsp_sync = std::stoi( all_args[i+1] );
    }
		else if(( x == "-nf" || x == "--no_frame_skip") ) {
      g_frame_skip = 0;
    }
		else if(( x == "-v" || x == "--version") ) {
      printf("YabaSanshiro version %s(%s)\n",YAB_VERSION, GIT_SHA1 );
      return 0;
    }
	}
#endif
  if (!initEgl(nwindowGetDefault()))
      return EXIT_FAILURE;

  printf("context renderer string: \"%s\"\n", glGetString(GL_RENDERER));
  printf("context vendor string: \"%s\"\n", glGetString(GL_VENDOR));
  printf("version string: \"%s\"\n", glGetString(GL_VERSION));
  printf("Extentions: %s\n",glGetString(GL_EXTENSIONS));

  if( yabauseinit() == -1 ) {
      printf("Fail to yabauseinit Bye! (%s)", SDL_GetError() );
      return -1;
  }

  int width = 1280;
  int height = 720;
  VIDCore->Resize(0,0,width,height,0);
  glViewport(0,0,width,height);
  glClearColor( 0.0f, 0.0f,0.0f,1.0f);
  glClear( GL_COLOR_BUFFER_BIT );

#if 0
  inputmng->init(g_keymap_filename);
  menu = new MenuScreen(wnd,width,height, g_keymap_filename);
  menu->setConfigFile(g_keymap_filename);  


  if( g_keep_aspect_rate ){
    int originx = 0;
    int originy = 0;
    int specw = width;
    int spech = height;
    float specratio = (float)specw / (float)spech;
    int saturnw = 4;
    int saturnh = 3;
    float saturnraito = (float)saturnw/ (float)saturnh;
    float revraito = (float) saturnh/ (float)saturnw;
    if( specratio > saturnraito ){
            width = spech * saturnraito;
            height = spech;
            originx = (dsp.w - width)/2.0;
            originy = 0;
    }else{
        width = specw ;
        height = specw * revraito;
        originx = 0;
        originy = spech - height;
    }
    VIDCore->Resize(originx,originy,width,height,0);
  }else{
    VIDCore->Resize(0,0,width,height,0);
  }
  SDL_GL_MakeCurrent(wnd,nullptr);
  YabThreadSetCurrentThreadAffinityMask(0x00);

  Uint32 evToggleMenu = SDL_RegisterEvents(1);
  inputmng->setToggleMenuEventCode(evToggleMenu);

  Uint32  evResetMenu = SDL_RegisterEvents(1);
  menu->setResetMenuEventCode(evResetMenu);

  Uint32  evPadMenu = SDL_RegisterEvents(1);
  menu->setTogglePadModeMenuEventCode(evPadMenu);

  Uint32  evToggleFps = SDL_RegisterEvents(1);
  menu->setToggleFpsCode(evToggleFps);

  Uint32  evToggleFrameSkip = SDL_RegisterEvents(1);
  menu->setToggleFrameSkip(evToggleFrameSkip);

  Uint32  evUpdateConfig = SDL_RegisterEvents(1);
  menu->setUpdateConfig(evUpdateConfig);

  Uint32  evOpenTray = SDL_RegisterEvents(1);
  menu->setOpenTrayMenuEventCode(evOpenTray);

  Uint32  evCloseTray = SDL_RegisterEvents(1);
  menu->setCloseTrayMenuEventCode(evCloseTray);


  bool menu_show = false;
  std::string tmpfilename = home_dir + "tmp.png";

  struct sched_param thread_param;
  thread_param.sched_priority = 15; //sched_get_priority_max(SCHED_FIFO);
  if ( pthread_setschedparam(pthread_self(), SCHED_FIFO, &thread_param) < -1 ) {
    LOG("sched_setscheduler");
  }
  setpriority( PRIO_PROCESS, 0, -8);
  int frame_cont = 0;
  while(true) {
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
      if(e.type == SDL_QUIT){
        glClearColor(0.0,0.0,0.0,1.0);
        glClear(GL_COLOR_BUFFER_BIT);        
        SDL_GL_SwapWindow(wnd);
        YabauseDeInit();
        SDL_Quit();
        return 0;
      }
      else if( e.type == evUpdateConfig ){
          inputmng->updateConfig();
      }
      else if(e.type == evToggleMenu){
        if( menu_show ){
          menu_show = false;
          inputmng->setMenuLayer(nullptr);
          SDL_GL_MakeCurrent(wnd,nullptr);
          VdpResume();
          SNDSDL.UnMuteAudio();          
        }else{
          menu_show = true;
          SNDSDL.MuteAudio();
          VdpRevoke();
          inputmng->setMenuLayer(menu);
          SDL_GL_MakeCurrent(wnd,glc);
          saveScreenshot(tmpfilename.c_str());
          glUseProgram(0);
          glGetError();
          glBindBuffer(GL_ARRAY_BUFFER, 0);
          glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
          glDisableVertexAttribArray(0);
          glDisableVertexAttribArray(1);
          glDisableVertexAttribArray(2);
          glDisable(GL_DEPTH_TEST);
          glDisable(GL_SCISSOR_TEST);
          glDisable(GL_STENCIL_TEST);
          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   
          menu->setBackGroundImage( tmpfilename );
        }
      }

      else if(e.type == evResetMenu){
        YabauseReset();
        menu_show = false;
        inputmng->setMenuLayer(nullptr);
        SDL_GL_MakeCurrent(wnd,nullptr);
        VdpResume();
        SNDSDL.UnMuteAudio(); 
      }

      else if(e.type == evPadMenu ){
        if( padmode == 0 ){
          padmode = 1;
        }else{
          padmode = 0;
        }
        inputmng->setGamePadomode( 0, padmode );
        menu_show = false;
        inputmng->setMenuLayer(nullptr);
        SDL_GL_MakeCurrent(wnd,nullptr);
        VdpResume();
        SNDSDL.UnMuteAudio();         
      }

      else if(e.type == evToggleFps ){
        if( g_EnagleFPS == 0 ){
          g_EnagleFPS = 1;
        }else{
          g_EnagleFPS = 0;
        }
        menu_show = false;
        inputmng->setMenuLayer(nullptr);
        SDL_GL_MakeCurrent(wnd,nullptr);
        VdpResume();
        SNDSDL.UnMuteAudio();         
      }

      else if(e.type == evToggleFrameSkip ){
        if( g_frame_skip == 0 ){
          g_frame_skip = 1;
          EnableAutoFrameSkip();
        }else{
          g_frame_skip = 0;
          DisableAutoFrameSkip();
        }
        menu_show = false;
        inputmng->setMenuLayer(nullptr);
        SDL_GL_MakeCurrent(wnd,nullptr);
        VdpResume();
        SNDSDL.UnMuteAudio();         
      }
      else if(e.type == evOpenTray ){
        menu->setCurrentGamePath(cdpath);
        Cs2ForceOpenTray();
        if( !g_emulated_bios ) {
        menu_show = false;
        inputmng->setMenuLayer(nullptr);
        SDL_GL_MakeCurrent(wnd,nullptr);
        VdpResume();
        SNDSDL.UnMuteAudio();         
      }
      }
      else if(e.type == evCloseTray ){
        if( e.user.data1 != nullptr ){
          strcpy( cdpath, (const char*)e.user.data1 );
          free(e.user.data1);
        }
        Cs2ForceCloseTray(CDCORE_ISO, cdpath );
        menu_show = false;
        inputmng->setMenuLayer(nullptr);
        SDL_GL_MakeCurrent(wnd,nullptr);
        VdpResume();
        SNDSDL.UnMuteAudio();         
      }

      inputmng->parseEvent(e);
      if( menu_show ){
        menu->onEvent( e );
      }
    }
    inputmng->handleJoyEvents();

    if( menu_show ){
      glClearColor(0.0f, 0.0f, 0.0f, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      menu->drawAll();
      SDL_GL_SwapWindow(wnd);
    }else{
      //printf("\033[%d;%dH Frmae = %d \n", 0, 0, frame_cont);
      //frame_cont++;
      YabauseExec(); // exec one frame
    }
  }
#else

#define MAKE_PAD(a,b) ((a<<24)|(b))
  void * padbits;
  PerPortReset();
  padbits = PerPadAdd(&PORTDATA1);

  PerSetKey(MAKE_PAD(0,PERPAD_UP), PERPAD_UP, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_RIGHT), PERPAD_RIGHT, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_DOWN), PERPAD_DOWN, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_LEFT), PERPAD_LEFT, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_START), PERPAD_START, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_A), PERPAD_A, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_B), PERPAD_B, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_C), PERPAD_C, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_X), PERPAD_X, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_Y), PERPAD_Y, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_Z), PERPAD_Z, padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_RIGHT_TRIGGER),PERPAD_RIGHT_TRIGGER,padbits);
  PerSetKey(MAKE_PAD(0,PERPAD_LEFT_TRIGGER),PERPAD_LEFT_TRIGGER,padbits);

  while(appletMainLoop()) {
      // Get and process input
        hidScanInput();
        u32 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        if ( (kDown & KEY_MINUS) ) 
            break;

    int player = 0;
    if (kDown & KEY_B) PerKeyDown(MAKE_PAD(player,PERPAD_A)); else PerKeyUp(MAKE_PAD(player,PERPAD_A));
    if (kDown & KEY_A) PerKeyDown(MAKE_PAD(player,PERPAD_B)); else PerKeyUp(MAKE_PAD(player,PERPAD_B));
    if (kDown & KEY_R) PerKeyDown(MAKE_PAD(player,PERPAD_C)); else PerKeyUp(MAKE_PAD(player,PERPAD_C));

    if (kDown & KEY_Y) PerKeyDown(MAKE_PAD(player,PERPAD_X)); else PerKeyUp(MAKE_PAD(player,PERPAD_X));
    if (kDown & KEY_X) PerKeyDown(MAKE_PAD(player,PERPAD_Y)); else PerKeyUp(MAKE_PAD(player,PERPAD_Y));
    if (kDown & KEY_L) PerKeyDown(MAKE_PAD(player,PERPAD_Z)); else PerKeyUp(MAKE_PAD(player,PERPAD_Z));

    if (kDown & KEY_DLEFT) PerKeyDown(MAKE_PAD(player,PERPAD_LEFT)); else PerKeyUp(MAKE_PAD(player,PERPAD_LEFT));
    if (kDown & KEY_DRIGHT) PerKeyDown(MAKE_PAD(player,PERPAD_RIGHT)); else PerKeyUp(MAKE_PAD(player,PERPAD_RIGHT));
    if (kDown & KEY_DUP) PerKeyDown(MAKE_PAD(player,PERPAD_UP)); else PerKeyUp(MAKE_PAD(player,PERPAD_UP));
    if (kDown & KEY_DDOWN) PerKeyDown(MAKE_PAD(player,PERPAD_DOWN)); else PerKeyUp(MAKE_PAD(player,PERPAD_DOWN));

    if (kDown & KEY_PLUS) PerKeyDown(MAKE_PAD(player,PERPAD_START)); else PerKeyUp(MAKE_PAD(player,PERPAD_START));

    if (kDown & KEY_ZR) PerKeyDown(MAKE_PAD(player,PERPAD_RIGHT_TRIGGER)); else PerKeyUp(MAKE_PAD(player,PERPAD_RIGHT_TRIGGER));
    if (kDown & KEY_ZL) PerKeyDown(MAKE_PAD(player,PERPAD_LEFT_TRIGGER)); else PerKeyUp(MAKE_PAD(player,PERPAD_LEFT_TRIGGER));

    //glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT);
    //eglSwapBuffers(s_display, s_surface);
    YabauseExec(); // exec one frame
  }
#endif  
  YabauseDeInit();
  deinitEgl();
  return 0;
}


#define YUI_LOG printf
int saveScreenshot( const char * filename ){
    
#if 0    
    int width;
    int height;
    unsigned char * buf = NULL;
    unsigned char * bufRGB = NULL;
    png_bytep * row_pointers = NULL;
    int quality = 100; // best
    FILE * outfile = NULL;
    int row_stride;
    int glerror;
    int u,v;
    int pmode;
    png_byte color_type;
    png_byte bit_depth; 
    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    int rtn = -1;
  
    SDL_GetWindowSize( wnd, &width, &height);
    buf = (unsigned char *)malloc(width*height*4);
    if( buf == NULL ) {
        YUI_LOG("not enough memory\n");
        goto FINISH;
    }

    glReadBuffer(GL_BACK);
    pmode = GL_RGBA;
    glGetError();
    glReadPixels(0, 0, width, height, pmode, GL_UNSIGNED_BYTE, buf);
    if( (glerror = glGetError()) != GL_NO_ERROR ){
        YUI_LOG("glReadPixels %04X\n",glerror);
         goto FINISH;
    }
	
	for( u = 3; u <width*height*4; u+=4 ){
		buf[u]=0xFF;
	}
    row_pointers = (png_byte**)malloc(sizeof(png_bytep) * height);
    for (v=0; v<height; v++)
        row_pointers[v] = (png_byte*)&buf[ (height-1-v) * width * 4];

    // save as png
    if ((outfile = fopen(filename, "wb")) == NULL) {
        YUI_LOG("can't open %s\n", filename);
        goto FINISH;
    }

    /* initialize stuff */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr){
        YUI_LOG("[write_png_file] png_create_write_struct failed");
        goto FINISH;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr){
        YUI_LOG("[write_png_file] png_create_info_struct failed");
        goto FINISH;
    }

    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during init_io");
        goto FINISH;
    }
    /* write header */
    png_init_io(png_ptr, outfile);
    
    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during writing header");
        goto FINISH;
    }
    bit_depth = 8;
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    png_set_IHDR(png_ptr, info_ptr, width, height,
        bit_depth, color_type, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    //png_set_gAMA(png_ptr, info_ptr, 1.0);
    {
        png_text text[3];
        int txt_fields = 0;
        char desc[256];
        
        time_t      gmt;
        png_time    mod_time;
        
        time(&gmt);
        png_convert_from_time_t(&mod_time, gmt);
        png_set_tIME(png_ptr, info_ptr, &mod_time);
    
        text[txt_fields].key = "Title";
        text[txt_fields].text = Cs2GetCurrentGmaecode();
        text[txt_fields].compression = PNG_TEXT_COMPRESSION_NONE;
        txt_fields++;

        sprintf( desc, "Yaba Sanshiro Version %s\n VENDER: %s\n RENDERER: %s\n VERSION %s\n",YAB_VERSION,glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION));
        text[txt_fields].key ="Description";
        text[txt_fields].text=desc;
        text[txt_fields].compression = PNG_TEXT_COMPRESSION_NONE;
        txt_fields++;
        
        png_set_text(png_ptr, info_ptr, text,txt_fields);
    }       
    png_write_info(png_ptr, info_ptr);


    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during writing bytes");
        goto FINISH;
    }
    png_write_image(png_ptr, row_pointers);

    /* end write */
    if (setjmp(png_jmpbuf(png_ptr))){
        YUI_LOG("[write_png_file] Error during end of write");
        goto FINISH;
    }
    
    png_write_end(png_ptr, NULL);
    rtn = 0;
FINISH: 
    if(outfile) fclose(outfile);
    if(buf) free(buf);
    if(bufRGB) free(bufRGB);
    if(row_pointers) free(row_pointers);
    return rtn;
#endif
  return 0;    
}
