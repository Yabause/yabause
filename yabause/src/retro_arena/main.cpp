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
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem ;

#if defined(ARCH_IS_LINUX)
#include <unistd.h>
#include <sys/resource.h>
#include <errno.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
#elif defined(_WINDOWS)
#include <windows.h>
#include <commdlg.h>

#include <direct.h>
#include <SDL.h>
#include <SDL_syswm.h>

#endif

#include "common.h"


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
#include "libpng16/png.h"
}

#include "InputManager.h"
#include "MenuScreen.h"
#include "Preference.h"
#include "EventManager.h"

#define YUI_LOG printf

static char last_state_filename[256] = "\0";
char s_savepath[256] ="\0";

extern "C" {
  static char biospath[256] = "/home/pigaming/RetroPie/BIOS/saturn/bios.bin";
  static char cdpath[256] = ""; ///home/pigaming/RetroPie/roms/saturn/nights.cue";
  //static char cdpath[256] = "/home/pigaming/RetroPie/roms/saturn/gd.cue";
  //static char cdpath[256] = "/home/pigaming/RetroPie/roms/saturn/Virtua Fighter Kids (1996)(Sega)(JP).ccd";
  static char buppath[256] = "./back.bin";
  static char mpegpath[256] = "\0";
  static char cartpath[256] = "\0";
  static bool menu_show = false;

#define LOG printf

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
  static SDL_Window* subwnd;
  static SDL_GLContext glc;
  static SDL_GLContext subglc;
  int g_EnagleFPS = 0;
  int g_resolution_mode = 0;
  int g_keep_aspect_rate = 0;
  int g_scsp_sync = 32;
  int g_frame_skip = 1;
  int g_emulated_bios = 1;
  bool g_full_screen = false;
  InputManager* inputmng;
  MenuScreen * menu;

  using std::string;
  string g_keymap_filename;

  void hideMenuScreen();

  //----------------------------------------------------------------------------------------------
  NVGcontext * getGlobalNanoVGContext() {
    return menu->nvgContext();
  }

  void DrawDebugInfo()
  {
  }

  void YuiErrorMsg(const char *string)
  {
    LOG("%s", string);
  }

  void YuiSwapBuffers(void)
  {
    SDL_GL_SwapWindow(wnd);
    SetOSDToggle(g_EnagleFPS);
  }

  int YuiRevokeOGLOnThisThread() {
#if defined(YAB_ASYNC_RENDERING)
    LOG("revoke thread\n");
#if defined(_JETSON_) || defined(PC) || defined(_WINDOWS)
    int rc = -1;
    int retry = 0;
    while (rc != 0) {
      YabThreadUSleep(16666);
      retry++;
      rc = SDL_GL_MakeCurrent(subwnd, subglc);
      if (rc != 0) { LOG("fail revoke thread\n"); }
      if (retry > 100) {
        LOG("out of retry cont\n");
        abort();
      }
    }
#else
    SDL_GL_MakeCurrent(wnd, nullptr);
#endif  
#endif
    return 0;
  }

  int YuiUseOGLOnThisThread() {
#if defined(YAB_ASYNC_RENDERING)
    LOG("use thread\n");
#if defined(_JETSON_) || defined(PC) || defined(_WINDOWS)
    int rc = -1;
    int retry = 0;
    while (rc != 0) {
      YabThreadUSleep(16666);
      retry++;
      rc = SDL_GL_MakeCurrent(wnd, glc);
      if (rc != 0) { LOG("fail revoke thread\n"); }
      if (retry > 100) {
        LOG("out of retry cont\n");
        abort();
      }
    }
#else
    SDL_GL_MakeCurrent(wnd, glc);
#endif
#endif
    return 0;
  }

}

int saveScreenshot( const char * filename );

//int padmode = 0;

int yabauseinit()
{
  int res;
  yabauseinit_struct yinit = {};

  Preference pre("default");

  yinit.m68kcoretype = M68KCORE_MUSASHI;
  yinit.percoretype = PERCORE_DUMMY;
#if defined(__PC__)
  yinit.sh2coretype = 0;
#else
  yinit.sh2coretype = 3;
#endif  
  //yinit.vidcoretype = VIDCORE_SOFT;
  yinit.vidcoretype = 1;
  yinit.sndcoretype = SNDCORE_SDL;
  //yinit.sndcoretype = SNDCORE_DUMMY;
  //yinit.cdcoretype = CDCORE_DEFAULT;
  yinit.cdcoretype = CDCORE_ISO;
  yinit.carttype = CART_DRAM32MBIT;
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
#if defined(__JETSON__) || defined(_WINDOWS)   
  yinit.polygon_generation_mode = GPU_TESSERATION;
#else
  yinit.polygon_generation_mode = PERSPECTIVE_CORRECTION;
#endif
  yinit.use_new_scsp = 1;
  yinit.resolution_mode = pre.getInt( "Resolution" ,g_resolution_mode);
  yinit.rotate_screen = pre.getBool( "Rotate screen" , false );
  yinit.scsp_sync_count_per_frame = g_scsp_sync;
  yinit.extend_backup = 1;
#if defined(__JETSON__) || defined(__SWITCH__) || defined(_WINDOWS)
  yinit.scsp_main_mode = 1;
#else
  yinit.scsp_main_mode = 1;
#endif
  yinit.rbg_resolution_mode = pre.getInt( "Rotate screen resolution" ,0);
#if defined(__JETSON__) || defined(_WINDOWS)
  yinit.rbg_use_compute_shader = pre.getBool( "Use compute shader" , true);
#else
  yinit.rbg_use_compute_shader = pre.getBool( "Use compute shader" , false);
#endif

  yinit.use_sh2_cache = 1;

  res = YabauseInit(&yinit);
  if( res == -1) {
    return -1;
  }

  //padmode = inputmng->getCurrentPadMode( 0 );
#if !defined(__PC__)  
  OSDInit(0);
  OSDChangeCore(OSDCORE_NANOVG);
#endif
  LogStart();
  LogChangeOutput(DEBUG_CALLBACK, NULL);
  return 0;
}

#include <Shlobj.h>

void narrow(const std::wstring &src, std::string &dest) {
  char *mbs = new char[src.length() * MB_CUR_MAX + 1];
  wcstombs(mbs, src.c_str(), src.length() * MB_CUR_MAX + 1);
  dest = mbs;
  delete[] mbs;
}

void getHomeDir( std::string & homedir ) {
#if defined(ARCH_IS_LINUX)
  homedir = getenv("HOME");
  homedir += ".yabasanshiro";
#elif defined(_WINDOWS)
  
  WCHAR * path;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, NULL, &path))) {
    std::wstring tmp;
    tmp = path;
    narrow(tmp, homedir);
    homedir += "/YabaSanshiro/";
    CoTaskMemFree(path);
  }
  else {
    exit(-1);
  }
  
  //homedir = "./";
#endif
}

void ToggleFullscreen(SDL_Window* Window) {
  const Uint32 FullscreenFlag = SDL_WINDOW_FULLSCREEN;
  g_full_screen = !g_full_screen;
  SDL_SetWindowFullscreen(Window, g_full_screen ? 0 : FullscreenFlag);
  SDL_ShowCursor(g_full_screen);
}


int __stdcall BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
  TCHAR dir[MAX_PATH];
  ITEMIDLIST *lpid;
  HWND hEdit;

  switch (uMsg) {
  case BFFM_INITIALIZED:  //      ダイアログボックス初期化時
    SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);     //      コモンダイアログの初期ディレクトリ
    break;
  case BFFM_VALIDATEFAILED:       //      無効なフォルダー名が入力された
    //MessageBox(hWnd, (TCHAR*)lParam, _TEXT("無効なフォルダー名が入力されました"), MB_OK);
    //hEdit = FindWindowEx(hWnd, NULL, _TEXT("EDIT"), NULL);     //      エディットボックスのハンドルを取得する
    //SetWindowText(hEdit, _TEXT(""));
    return 1;       //      ダイアログボックスを閉じない
    break;
  case BFFM_IUNKNOWN:
    break;
  case BFFM_SELCHANGED:   //      選択フォルダーが変化した場合
    //lpid = (ITEMIDLIST *)lParam;
    //SHGetPathFromIDList(lpid, dir);
    //hEdit = FindWindowEx(hWnd, NULL, _TEXT("EDIT"), NULL);     //      エディットボックスのハンドルを取得する
    //SetWindowText(hEdit, dir);
    break;
  }
  return 0;
}

bool selectDirectory(std::string & out) {

  char path[_MAX_PATH];

  BROWSEINFOA bInfo;
  LPITEMIDLIST pIDList;

  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(wnd, &wmInfo);
  HWND hWnd = wmInfo.info.win.window;

  memset(&bInfo, 0, sizeof(bInfo));
  bInfo.hwndOwner = hWnd; // ダイアログの親ウインドウのハンドル 
  bInfo.pidlRoot = NULL; // ルートフォルダをデスクトップフォルダとする 
  bInfo.pszDisplayName = path; //フォルダ名を受け取るバッファへのポインタ 
  bInfo.lpszTitle = "Choose game directory"; // ツリービューの上部に表示される文字列 
  bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_VALIDATE | BIF_NEWDIALOGSTYLE; // 表示されるフォルダの種類を示すフラグ 
  bInfo.lpfn = BrowseCallbackProc; // BrowseCallbackProc関数のポインタ 
  //bInfo.lParam = (LPARAM)def_dir;
  pIDList = SHBrowseForFolderA(&bInfo);
  if (pIDList == NULL) {
    path[0] = '\0';
    return false; //何も選択されなかった場合 
  }

   if (!SHGetPathFromIDListA(pIDList, path))
      return false;//変換に失敗 

   CoTaskMemFree(pIDList);// pIDListのメモリを開放 
   out = path;
   return true;
}


//#undef main
//int main(int argc, char** argv)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR lpCmdLine, int cmdShow)
{

  inputmng = InputManager::getInstance();

  char** argv = __argv;
  int argc = __argc;

  //printf("\033[2J");

  // Inisialize home directory
  std::string home_dir;
  getHomeDir(home_dir);
  struct stat st = { 0 };
  if (stat(home_dir.c_str(), &st) == -1) {
#if defined(_WINDOWS)
    mkdir(home_dir.c_str());
#else
    mkdir(home_dir.c_str(), 0700);
#endif
  }

#if 0
  AllocConsole();
  FILE * stdout_fp = freopen("CONOUT$", "wb", stdout);
  FILE * stderr_fp = freopen("CONOUT$", "wb", stderr);
#else
  FILE * stdout_fp = freopen(string(home_dir + "/stdout.txt").c_str(), "wb", stdout);
  FILE * stderr_fp = freopen(string(home_dir + "/stderr.txt").c_str(), "wb", stderr);
#endif

  std::string games_dir = home_dir + "games";
  if (stat(games_dir.c_str(), &st) == -1) {
#if defined(_WINDOWS)
    mkdir(games_dir.c_str());
#else
    mkdir(home_dir.c_str(), 0700);
#endif
  }

  std::string bckup_dir = home_dir + "/backup.bin";
  strcpy(buppath, bckup_dir.c_str());
  strcpy(s_savepath, home_dir.c_str());
  g_keymap_filename = home_dir + "/keymapv2.json";

  Preference * defpref = new Preference("default");

  std::string current_exec_name = argv[0]; // Name of the current exec program
  std::vector<std::string> all_args;
  if (argc > 1) {
    all_args.assign(argv + 1, argv + argc);
    if (all_args[0] == "-h" || all_args[0] == "--h") {
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

  auto biosFilename = defpref->getString("bios file", "");
  if (biosFilename != "") {
    g_emulated_bios = 0;
    strncpy(biospath, biosFilename.c_str(), 256);
    if (fs::exists(biosFilename.c_str()) == false) {
      g_emulated_bios = 1;
      biosFilename = "";
    }
  }

  auto lastFilePath = defpref->getString("last play game path", "");
  if (lastFilePath != "") {
    strncpy(cdpath, lastFilePath.c_str(), 256);
  }

  g_resolution_mode = defpref->getInt("Resolution", 0);
  g_keep_aspect_rate = defpref->getInt("Aspect rate", 0);
  g_scsp_sync = defpref->getInt("sound sync count per a frame", 32);
  g_frame_skip = defpref->getBool("frame skip", false);
  g_full_screen = defpref->getBool("Full screen", false);
  g_EnagleFPS = defpref->getBool("Show Fps", false);

  for (int i = 0; i < all_args.size(); i++) {
    string x = all_args[i];
    if ((x == "-b" || x == "--bios") && (i + 1 < all_args.size())) {
      g_emulated_bios = 0;
      strncpy(biospath, all_args[i + 1].c_str(), 256);
    }
    else if ((x == "-i" || x == "--iso") && (i + 1 < all_args.size())) {
      strncpy(cdpath, all_args[i + 1].c_str(), 256);
    }
    else if ((x == "-r" || x == "--resolution_mode") && (i + 1 < all_args.size())) {
      g_resolution_mode = std::stoi(all_args[i + 1]);
    }
    else if ((x == "-a" || x == "--keep_aspect_rate")) {
      g_keep_aspect_rate = 1;
    }
    else if ((x == "-s" || x == "--g_scsp_sync") && (i + 1 < all_args.size())) {
      g_scsp_sync = std::stoi(all_args[i + 1]);
    }
    else if ((x == "-nf" || x == "--no_frame_skip")) {
      g_frame_skip = 0;
    }
    else if ((x == "-w" || x == "--window")) {
      g_full_screen = false;
    }
    else if ((x == "-v" || x == "--version")) {
      printf("YabaSanshiro version %s(%s)\n", YAB_VERSION, GIT_SHA1);
      return 0;
    }
  }

  defpref->setString("last play game path", cdpath);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    printf("Fail to init SDL Bye! (%s)", SDL_GetError());
    return -1;
  }

  SDL_DisplayMode dsp;
  if (SDL_GetCurrentDisplayMode(0, &dsp) != 0) {
    printf("Fail to SDL_GetCurrentDisplayMode Bye! (%s)", SDL_GetError());
    return -1;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

  SDL_GL_SetSwapInterval(1);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

  int width = 1280;
  int height = 720;

  if (g_full_screen == false) {
    wnd = SDL_CreateWindow("Yaba Snashiro", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

    subwnd = SDL_CreateWindow("Yaba Snashiro sub", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

  }
  else {

    int targetDisplay = 0; // defpref->getInt("target display", 0);

    // enumerate displays
    int displays = SDL_GetNumVideoDisplays();
    assert(displays > 1);  // assume we have secondary monitor

    if (targetDisplay > displays || targetDisplay < 0 ) {
      LOG("Display number is ecxeeded. force to use 0");
      targetDisplay = 0;
    }

    // get display bounds for all displays
    vector< SDL_Rect > displayBounds;
    for (int i = 0; i < displays; i++) {
      displayBounds.push_back(SDL_Rect());
      SDL_GetDisplayBounds(i, &displayBounds.back());
    }

    int x = displayBounds[targetDisplay].x;
    int y = displayBounds[targetDisplay].y;
    int w = displayBounds[targetDisplay].w;
    int h = displayBounds[targetDisplay].h;

    width = dsp.w;
    height = dsp.h;
    wnd = SDL_CreateWindow("Yaba Snashiro", x, y,
      w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
    if (wnd == nullptr) {
      printf("Fail to SDL_CreateWindow Bye! (%s)", SDL_GetError());
      return -1;
    }
    SDL_ShowCursor(SDL_FALSE);
  }
  
#if defined(_JETSON_)  
  subwnd = SDL_CreateWindow("Yaba Snashiro sub", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN );
#endif


#if 0
  SDL_Cursor *cursor; /* Make this variable visible in the point
                       where you exit the program */
  int32_t cursorData[2] = {0, 0};
  cursor = SDL_CreateCursor((Uint8 *)cursorData, (Uint8 *)cursorData, 8, 8, 4, 4);
  SDL_SetCursor(cursor);
#endif

  //dsp.refresh_rate = 60;
  //SDL_SetWindowDisplayMode(wnd,&dsp);
  SDL_GetWindowSize(wnd,&width,&height);
//  SDL_SetWindowInputFocus(wnd);
#if defined(_JETSON_) || defined(__PC__)
  subglc = SDL_GL_CreateContext(wnd);
  if(subglc == nullptr ) {
    printf("Fail to SDL_GL_CreateContext Bye! (%s)", SDL_GetError() );
    return -1;
  }
#endif  
  glc = SDL_GL_CreateContext(wnd);
  if(glc == nullptr ) {
    printf("Fail to SDL_GL_CreateContext Bye! (%s)", SDL_GetError() );
    return -1;
  }

  printf("context renderer string: \"%s\"\n", glGetString(GL_RENDERER));
  printf("context vendor string: \"%s\"\n", glGetString(GL_VENDOR));
  printf("version string: \"%s\"\n", glGetString(GL_VERSION));
  printf("Extentions: %s\n",glGetString(GL_EXTENSIONS));

/*
  if (strlen(cdpath) <= 0 ) {
    strcpy(cdpath, home_dir.c_str());
    p = new Preference(cdpath);
  }
  else {
    p = new Preference("default");
  }
*/

  SDL_GL_MakeCurrent(wnd, glc);

  inputmng->init(g_keymap_filename);
  menu = new MenuScreen(wnd,width,height, g_keymap_filename, cdpath);
  menu->setConfigFile(g_keymap_filename);  
  menu->setCurrentGamePath(cdpath);
  

  if( yabauseinit() == -1 ) {
      printf("Fail to yabauseinit Bye! (%s)", SDL_GetError() );
      return -1;
  }

  
  VIDCore->Resize(0,0,width,height,1,defpref->getInt("Aspect rate",0));
  
#if defined(YAB_ASYNC_RENDERING)  
  SDL_GL_MakeCurrent(wnd,nullptr);
#endif

#if defined(__RP64__) || defined(__N2__)
  YabThreadSetCurrentThreadAffinityMask(0x4);
#else
  YabThreadSetCurrentThreadAffinityMask(0x0);
#endif
  Uint32 evToggleMenu = SDL_RegisterEvents(1);
  inputmng->setToggleMenuEventCode(evToggleMenu);

  Uint32  evRepeat = SDL_RegisterEvents(1);
  menu->setRepeatEventCode(evRepeat);

  EventManager * evm = EventManager::getInstance();


  evm->setEvent("reset", [](int code, void * data1, void * data2) {
     printf("hello");
     YabauseReset();
     hideMenuScreen(); 
  });

  evm->setEvent("toggle fps", [](int code, void * data1, void * data2) {
    if (g_EnagleFPS == 0) {
      g_EnagleFPS = 1;
    }
    else {
      g_EnagleFPS = 0;
    }
    hideMenuScreen();
  });

  evm->setEvent("toggle frame skip", [](int code, void * data1, void * data2) {
    if (g_frame_skip == 0) {
      g_frame_skip = 1;
      EnableAutoFrameSkip();
    }
    else {
      g_frame_skip = 0;
      DisableAutoFrameSkip();
    }
    hideMenuScreen();
  });

  evm->setEvent("open tray", [](int code, void * data1, void * data2) {
    menu->setCurrentGamePath(cdpath);
    Cs2ForceOpenTray();
    if (!g_emulated_bios) {
      hideMenuScreen();
    }
  });

  evm->setEvent("close tray", [](int code, void * data1, void * data2) {
    if (data1 != nullptr) {
      strcpy(cdpath, (const char*)data1);
      free(data1);
    }
    Preference defpref("default");
    defpref.setString("last play game path", cdpath);
    Cs2ForceCloseTray(CDCORE_ISO, cdpath);
    hideMenuScreen();
  });

  std::string tmpfilename = home_dir + "tmp.png";

  evm->setEvent("save state", [tmpfilename](int code, void * data1, void * data2) {
    int ret;
    time_t t = time(NULL);
    YUI_LOG("MSG_SAVE_STATE");

    snprintf(last_state_filename, 256, "%s/%s_%d.yss", s_savepath, cdip->itemnum, code);
    ret = YabSaveState(last_state_filename);
    if (ret == 0) {
      char pngname[256];
      snprintf(pngname, 256, "%s/%s_%d.png", s_savepath, cdip->itemnum, code);
      fs::copy(tmpfilename, pngname, fs::copy_options::overwrite_existing);
    }
    hideMenuScreen();
  });

  evm->setEvent("load state", [tmpfilename](int code, void * data1, void * data2) {
    int rtn;
    YUI_LOG("MSG_LOAD_STATE");

    // Find latest filename
    sprintf(last_state_filename, "%s/%s_%d.yss", s_savepath, cdip->itemnum, code);
    rtn = YabLoadState(last_state_filename);
    switch (rtn) {
    case 0:
      YUI_LOG("Load State: OK");
      break;
    case -1:
      YUI_LOG("Load State: File Not Found");
      break;
    case -2:
      YUI_LOG("Load State: Bad format");
      break;
    case -3:
      YUI_LOG("Load State: Bad format deep inside");
      break;
    default:
      YUI_LOG("Load State: Fail unkown");
      break;
    }
    hideMenuScreen();
  });

  evm->setEvent("update config", [](int code, void * data1, void * data2) {
      inputmng->updateConfig();
  });

  evm->setEvent("select bios", [home_dir](int code, void * data1, void * data2) {

    // use builtlin bios
    if (code == 0) {
      Preference * p = new Preference("default");
      p->setString("bios file", "");
      delete p;
    }
    else {

      SDL_SysWMinfo wmInfo;
      SDL_VERSION(&wmInfo.version);
      SDL_GetWindowWMInfo(wnd, &wmInfo);
      HWND hwnd = wmInfo.info.win.window;

      char * fname = new char[256];
      OPENFILENAMEA o;
      fname[0] = '\0';
      ZeroMemory(&o, sizeof(o));
      o.lStructSize = sizeof(o);              //      構造体サイズ
      o.hwndOwner = hwnd;                             //      親ウィンドウのハンドル
      o.lpstrInitialDir = home_dir.c_str();    //      初期フォルダー
      o.lpstrFile = fname;                    //      取得したファイル名を保存するバッファ
      o.nMaxFile = 256;                                //      取得したファイル名を保存するバッファサイズ
      //o.lpstrFilter = _TEXT("TXTファイル(*.TXT)\0*.TXT\0") _TEXT("全てのファイル(*.*)\0*.*\0");
      //o.lpstrDefExt = _TEXT("TXT");
      o.lpstrTitle = "Select a BIOS file";
      o.nFilterIndex = 1;
      if (GetOpenFileNameA(&o)) {
        Preference * p = new Preference("default");
        p->setString("bios file", fname);
        delete p;
      }
      delete fname;
    }
    menu->popActiveMenu();
    hideMenuScreen();
  });

  
  // 初期設定がされていない場合はメニューを表示する
  // BIOSなしの状態でゲームが選択されていない場合も
  if (defpref->getBool("is first time",true) || (g_emulated_bios == 1 && strlen(cdpath) == 0 ) ) {
    defpref->setBool("is first time", false);
    SDL_Event event = {};
    event.type = evToggleMenu;
    event.user.code = 0;
    event.user.data1 = 0;
    event.user.data2 = 0;
    SDL_PushEvent(&event);
  }
  delete defpref;

#if defined(ARCH_IS_LINUX)
  struct sched_param thread_param;
  thread_param.sched_priority = 15; //sched_get_priority_max(SCHED_FIFO);
  if ( pthread_setschedparam(pthread_self(), SCHED_FIFO, &thread_param) < -1 ) {
    LOG("sched_setscheduler");
  }
  setpriority( PRIO_PROCESS, 0, -8);
#endif
  int frame_cont = 0;
  int event_count = 0;
  while(true) {
    SDL_Event e;
    event_count = 0;
    while(SDL_PollEvent(&e)) {
      event_count++;
      if(e.type == SDL_QUIT){
        glClearColor(0.0,0.0,0.0,1.0);
        glClear(GL_COLOR_BUFFER_BIT);        
        SDL_GL_SwapWindow(wnd);
        YabauseDeInit();
        SDL_Quit();
        return 0;
      }

      else if (e.type == SDL_WINDOWEVENT) {

        switch (e.window.event) {
        case SDL_WINDOWEVENT_CLOSE:   // exit game
          glClearColor(0.0, 0.0, 0.0, 1.0);
          glClear(GL_COLOR_BUFFER_BIT);
          SDL_GL_SwapWindow(wnd);
          YabauseDeInit();
          SDL_Quit();
          return 0;
          break;
        default:
          break;
        }
        break;
      }
      else if(e.type == evToggleMenu){
        if( menu_show ){
            Preference * p = new Preference(cdpath);
            VIDCore->Resize(0,0,width,height,1,p->getInt("Aspect rate",0));
            delete p;
          hideMenuScreen();           
        }else{
          menu_show = true;
          SDL_ShowCursor(SDL_TRUE);
          ScspMuteAudio(1);
#if defined(YAB_ASYNC_RENDERING)  
#if defined(_JETSON_) || defined(PC) || defined(_WINDOWS)
          SDL_GL_MakeCurrent(subwnd,nullptr);
          VdpRevoke();
          YuiUseOGLOnThisThread();
#else
          VdpRevoke();
#endif
#endif
          fflush(stdout_fp);
          fflush(stderr_fp);
          
          char pngname_base[256];
          snprintf(pngname_base,256,"%s/%s_", s_savepath, cdip->itemnum);
          menu->setCurrentGameId(pngname_base);

          inputmng->setMenuLayer(menu);
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

      else if (e.type == evRepeat) {
       string keycode((char*)e.user.data1);
        menu->keyboardEvent(keycode,0,e.user.code,0);
        delete[] e.user.data1;
      }
      else {
        evm->procEvent(e.type, e.user.code, e.user.data1);
      }

      inputmng->parseEvent(e);
      if( menu_show ){
        menu->onEvent( e );
      }
    }
    inputmng->handleJoyEvents();

    if( menu_show ){

      if( event_count > 0 ){
        glClearColor(0.0f, 0.0f, 0.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        menu->drawAll();
        SDL_GL_SwapWindow(wnd);
      }else{
        YabThreadUSleep( 16*1000 );
      }
    }else{
      //printf("\033[%d;%dH Frmae = %d \n", 0, 0, frame_cont);
      //frame_cont++;
      YabauseExec(); // exec one frame
    }
  }

  fclose(stdout_fp);
  fclose(stderr_fp);

  YabauseDeInit();
//  SDL_FreeCursor(cursor);
  SDL_Quit();
  return 0;
}

void hideMenuScreen(){
  menu_show = false;
  SDL_ShowCursor(SDL_FALSE);
  inputmng->setMenuLayer(nullptr);
  glClearColor(0.0f, 0.0f, 0.0f, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapWindow(wnd);       
#if defined(YAB_ASYNC_RENDERING)
#if defined(_JETSON_)  || defined(PC) || defined(_WINDOWS)
  SDL_GL_MakeCurrent(wnd,nullptr);
  VdpResume();
  YuiRevokeOGLOnThisThread();
#else
  glClear(GL_COLOR_BUFFER_BIT);
  SDL_GL_SwapWindow(wnd);          
  SDL_GL_MakeCurrent(wnd,nullptr);
  VdpResume();
#endif
#endif
  ScspUnMuteAudio(1); 
}

int saveScreenshot( const char * filename ){
    
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
}



extern "C" {

  int YabauseThread_IsUseBios() {
    //if( s_biospath == NULL){
    //    return 1;
    //}
    return 0;

  }

  const char * YabauseThread_getBackupPath() {
    //return s_buppath;
    return NULL;
  }

  void YabauseThread_setUseBios(int use) {


  }

  char tmpbakcup[256];
  void YabauseThread_setBackupPath( const char * buf) {
      //strcpy(tmpbakcup,buf);
      //s_buppath = tmpbakcup;
  }

  void YabauseThread_resetPlaymode() {
      //if( s_playrecord_path != NULL ){
      //    free(s_playrecord_path);
      //    s_playrecord_path = NULL;
      //}
      //s_buppath = GetMemoryPath();
  }

  void YabauseThread_coldBoot() {
    //YabauseDeInit();
    //YabauseInit();
    //YabauseReset();
  }
}