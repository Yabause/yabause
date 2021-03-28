#include <array>
#include <chrono>
#include <iostream>

#include "Renderer.h"
#include "Window.h"

#include <map>
#include <string>

using std::map;
using std::string;
map< int, int > g_Keymap;

#include "VIDVulkan.h"
#include "VIDVulkanCInterface.h"
#include "PlayRecorder.h"


#include <SDL.h>
#undef main

#include "libpng16/png.h"

extern "C" {
#include "../config.h"
#include "yabause.h"
#include "vdp2.h"
#include "scsp.h"
#include "vidsoft.h"
#include "vidogl.h"
#include "peripheral.h"
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
//#include "../sh2_dynarec_devmiyax/DynarecSh2.h"

#ifdef _WINDOWS
//static char biospath[256] = "C:/ext/osusume/bios.bin";
static char biospath[256] = "";
//static char cdpath[256] = "";
//static char cdpath[256] ="K:/Saturn/Shining Force III - Scenario 2 - Nerawareta Miko (Japan) (Rev A).chd";

//static char * biospath = NULL;
//static char * cdpath = NULL;
static char cdpath[256] = "C:/ext/osusume/SonycR.cue";
//static char cdpath[256] = "C:/ext/osusume/AfterBuner2.cue";
//static char cdpath[256] = "C:/ext/osusume/nights.cue";
//static char cdpath[] = "K:/Saturn/Shining Force III - Scenario 1 - Outo no Kyoshin (Japan).chd"; //UserClip
//static char cdpath[] = "K:/Saturn/Assault Suit Leynos 2 (Japan).chd"; // special priority
//static char cdpath[] = "K:/Saturn/Sakura Taisen (Japan) (Disc 1) (7M, 9M).chd";
//static char cdpath[] = "K:/Saturn/Dragon Force (Japan) (Rev A) (10M).chd";
//static char cdpath[] = "K:/Saturn/Bio Hazard (Japan).chd";
//static char cdpath[] = "C:/ext/osusume/Chaos Seed (J)(Saturn) (1)/Chaos Seed.cue";
//static char cdpath[] = "K:/Saturn/Gradius Deluxe Pack (Japan).chd";
//static char cdpath[256] = "K:/Saturn/Akumajou Dracula X - Gekka no Yasoukyoku (Japan) (2M).chd";
//static char cdpath[256] = "K:/Saturn/Burning Rangers (Japan).chd";
//static char cdpath[256] = "K:/Saturn/Magic Knight Rayearth (Japan) (Genteiban) (3M).chd";
//static char cdpath[256] = "K:/Saturn/Sega Ages - After Burner II (Japan).chd";
//static char cdpath[256] = "K:/Saturn/Sakura Taisen (Japan) (Disc 1) (7M, 9M).chd";
//static char cdpath[256] = "K:/Saturn/Sega Ages - Space Harrier (Japan) (1M).chd";
//static char cdpath[256] = "K:/Saturn/Sega Ages - OutRun (Japan) (Rev A).chd";
//static char cdpath[256] = "K:/Saturn/Grandia (Japan) (Disc 1) (2M).chd";
//static char cdpath[256] = "C:/ext/osusume/thunder_force_v[www.segasoluce.net]/thunder_force.iso";
//static char cdpath[256] = "C:/ext/osusume/Slam & Jam '96 featuring Magic & Kareem (U)(Saturn)/Slam & Jam '96 featuring Magic & Kareem (U)(Saturn).mds";
//static char cdpath[256] = "C:/ext/osusume/Black Matrix (J)(Saturn)/black_matrix.bin";
//static char cdpath[256] = "C:/ext/osusume/SegaRally.cue";
//static char cdpath[256] = "E:/gameiso/Sonic 3D Blast (U)(Saturn)/125 Sonic 3D Blast (U).bin";
//static char cdpath[256] = "E:/gameiso/sonicjam.iso";
//static char cdpath[256] = "K:/Saturn/Radiant Silvergun (Japan).chd";
//static char cdpath[256] = "K:/Saturn/Albert Odyssey Gaiden - Legend of Eldean (Japan) (3M).chd";
//static char cdpath[256] = "K:/Saturn/Street Fighter Zero 3 (Japan).chd";
//static char cdpath[256] = "K:/Saturn/Guardian Heroes (Japan) (3M).chd";
//static char cdpath[256] = "K:/Saturn/House of the Dead, The (Japan) (Rev A).chd";
//static char cdpath[256] = "K:/Saturn/Shining the Holy Ark (Japan) (1M).chd";
//static char cdpath[256] = "K:/Saturn/Virtua Cop 2 (Japan) (3M).chd";
//static char cdpath[256] = "K:/Saturn/Virtua Fighter 2 (Japan) (Rev B).chd";
#else
//static char biospath[256] = "/dat2/project/src/bios.bin";
static char * biospath = NULL;
//static char cdpath[256] = "/dat2/iso/nights.img";
static char cdpath[256] = "/dat2/iso/dytona/Daytona USA.iso";
//static char cdpath[256] = "/media/shinya/d-main1/gameiso/brtrck.bin";
#endif


int g_EnagleFPS = 1;
int g_resolution_mode = 0;
int g_keep_aspect_rate = 0;
int g_scsp_sync = 1;
int g_frame_skip = 1;
int g_emulated_bios = 1;
char * playdataPath = NULL;

const char * s_buppath = "./backup.bin";
char * s_playrecord_path = NULL;

//static char buppath[256] = "./back.bin";
static char mpegpath[256] = "\0";
static char cartpath[256] = "\0";

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
  &CVIDVulkan,
  &VIDOGL,
  NULL
};

#ifdef YAB_PORT_OSD
#include "nanovg/nanovg_osdcore.h"
OSD_struct *OSDCoreList[] = {
  &OSDDummy,
  &OSDNnovg,
  &OSDNnovgVulkan,
  NULL
};
#endif

int saveScreenshot(const char * filename);

void YuiSwapBuffers(void) {
  VIDVulkan::getInstance()->present();
}

void YuiErrorMsg(const char *string)
{
  printf("%s", string);
}

/* need to call before glXXXXX call in a thread */
int YuiUseOGLOnThisThread() {
  return 0;
}

/* Bfore rendering in a thread, it needs to revoke current rendering thread */
int YuiRevokeOGLOnThisThread() {
  return 0;
}

} 

int yabauseinit()
{
  int res;
  yabauseinit_struct yinit = {};
  void * padbits;

  

  yinit.m68kcoretype = M68KCORE_MUSASHI;
  yinit.percoretype = PERCORE_DUMMY;
#ifdef SH2_DYNAREC
  yinit.sh2coretype = VIDCORE_VULKAN;
#else
  //yinit.sh2coretype = 0;
#endif
  yinit.sh2coretype = 3;
  //yinit.vidcoretype = VIDCORE_SOFT;
  yinit.vidcoretype = VIDCORE_VULKAN;
  yinit.sndcoretype = SNDCORE_SDL;
  //yinit.sndcoretype = SNDCORE_DUMMY;
  //yinit.cdcoretype = CDCORE_DEFAULT;
  yinit.cdcoretype = CDCORE_ISO;
  yinit.carttype = CART_DRAM32MBIT;
  yinit.regionid = 0;
  yinit.biospath = biospath;
  yinit.cdpath = cdpath;
  yinit.buppath = s_buppath;
  yinit.mpegpath = mpegpath;
  yinit.cartpath = cartpath;
  yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
  yinit.frameskip = g_frame_skip;
  yinit.framelimit = 0;
  yinit.usethreads = 0;
  yinit.skip_load = 0;
  yinit.video_filter_type = 0;
  yinit.polygon_generation_mode = GPU_TESSERATION;
  yinit.use_new_scsp = 1;
  yinit.rotate_screen = 0;
  yinit.playRecordPath = playdataPath;
  yinit.rbg_resolution_mode = RBG_RES_1080P;
  yinit.resolution_mode = RES_NATIVE;
  yinit.extend_backup = 1;

  res = YabauseInit(&yinit);
  if (res == -1)
  {
    return -1;
  }
  PerPortReset();
  padbits = PerPadAdd(&PORTDATA1);

  PerSetKey(GLFW_KEY_UP, PERPAD_UP, padbits);
  PerSetKey(GLFW_KEY_RIGHT, PERPAD_RIGHT, padbits);
  PerSetKey(GLFW_KEY_DOWN, PERPAD_DOWN, padbits);
  PerSetKey(GLFW_KEY_LEFT, PERPAD_LEFT, padbits);
  PerSetKey(GLFW_KEY_Q, PERPAD_RIGHT_TRIGGER, padbits);
  PerSetKey(GLFW_KEY_E, PERPAD_LEFT_TRIGGER, padbits);
  PerSetKey(GLFW_KEY_ENTER, PERPAD_START, padbits);
  PerSetKey(GLFW_KEY_Z, PERPAD_A, padbits);
  PerSetKey(GLFW_KEY_X, PERPAD_B, padbits);
  PerSetKey(GLFW_KEY_C, PERPAD_C, padbits);
  PerSetKey(GLFW_KEY_A, PERPAD_X, padbits);
  PerSetKey(GLFW_KEY_S, PERPAD_Y, padbits);
  PerSetKey(GLFW_KEY_D, PERPAD_Z, padbits);

  g_Keymap[PERPAD_UP] = GLFW_KEY_UP;
  g_Keymap[PERPAD_RIGHT] = GLFW_KEY_RIGHT;
  g_Keymap[PERPAD_DOWN] = GLFW_KEY_DOWN;
  g_Keymap[PERPAD_LEFT] = GLFW_KEY_LEFT;
  g_Keymap[PERPAD_START] = GLFW_KEY_ENTER;

  OSDInit(0);
  OSDChangeCore(OSDCORE_NANOVG_VULKAN);
  SetOSDToggle(g_EnagleFPS);

  LogStart();
  LogChangeOutput(DEBUG_CALLBACK, NULL);

  return 0;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);


  if (action == GLFW_PRESS)
  {
    //int yabaky = g_Keymap[key];
    PerKeyDown(key);
  }
  else if (action == GLFW_RELEASE) {
    //int yabaky = g_Keymap[key];
    PerKeyUp(key);

    if (key == GLFW_KEY_F1) {
      YabSaveStateSlot(".\\", 1);
      //LOG("SAVE SLOT 1");
    }

    if (key == GLFW_KEY_F2) {
      YabLoadStateSlot(".\\", 1);
      //LOG("LOAD SLOT 1");
    }

    if (key == GLFW_KEY_F3) {
      vdp2ReqDump();
      //LOG("Req dump");
    }

    if (key == GLFW_KEY_F4) {
      vdp2ReqRestore();
      //LOG("ReqRestore");
    }
  }
}

void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  VIDCore->Resize(0, 0, width, height, 0, 0);
}

class ScreenRecorder {
public:
  void setScreenshotCallback(PlayRecorder * p) {
    using std::placeholders::_1;
    p->f_takeScreenshot = std::bind(&ScreenRecorder::takeScreenshot, this, _1);
  }
  void takeScreenshot(const char * fname) {
    ::saveScreenshot(fname);
  }
};

ScreenRecorder gsc;


int main( int argc, char *argv[] )
{
  Renderer r;


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
      printf("-p/--playrecord  <DIR>  Play play record.");
      exit(0);
    }
  }

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
    else if ((x == "-p" || x == "--p")) {
      int len = all_args[i + 1].length();
      playdataPath = (char*)malloc(len + 1);
      strcpy(playdataPath, all_args[i + 1].c_str());
    }
    else if ((x == "-v" || x == "--version")) {
      printf("YabaSanshiro version %s(%s)\n", YAB_VERSION, GIT_SHA1);
      return 0;
    }
  }

  gsc.setScreenshotCallback(PlayRecorder::getInstance());

    
  
  SDL_Init(SDL_INIT_AUDIO);

  const int width = 800;
  const int height = 600;
  auto w = r.OpenWindow(width, height, "Yaba sanshiro Vulkan",nullptr);
  VIDVulkan::getInstance()->setRenderer(&r);

  int rtn;
  rtn = yabauseinit();
  if (rtn != 0) {
    return -1;
  }

  VIDCore->Resize(0, 0, width, height, 0,0);

  float color_rotator = 0.0f;
  auto timer = std::chrono::steady_clock();
  auto last_time = timer.now();
  uint64_t frame_counter = 0;
  uint64_t fps = 0;

  glfwSetKeyCallback(w->getWindowHandle(), key_callback);
  glfwSetFramebufferSizeCallback(w->getWindowHandle(), framebufferResizeCallback);

  while (r.Run()) {
    // CPU logic calculations
    ++frame_counter;
    if (last_time + std::chrono::seconds(1) < timer.now()) {
      last_time = timer.now();
      fps = frame_counter;
      frame_counter = 0;
      std::cout << "FPS: " << fps << std::endl;
    }
    YabauseExec(); // exec one frame

  }
  
  vkQueueWaitIdle(r.GetVulkanQueue());

  YabauseDeInit();

  return 0;

}





int saveScreenshot(const char * filename) {
  unsigned char * buf = NULL;
  int width = 0;
  int height = 0;
  
  unsigned char * bufRGB = NULL;
  png_bytep * row_pointers = NULL;
  int quality = 100; // best
  FILE * outfile = NULL;
  int row_stride;
  int glerror;
  int u, v;
  int pmode;
  png_byte color_type;
  png_byte bit_depth;
  png_structp png_ptr;
  png_infop info_ptr;
  int number_of_passes;
  int rtn = -1;
  

  VIDCore->GetScreenshot((void**)&buf, &width, &height);


  for (u = 3; u < width*height * 4; u += 4) {
    buf[u] = 0xFF;
  }


  row_pointers = (png_byte**)malloc(sizeof(png_bytep) * height);
  for (v = 0; v < height; v++)
    row_pointers[v] = (png_byte*)&buf[(height - 1 - v) * width * 4];

  // save as png
  if ((outfile = fopen(filename, "wb")) == NULL) {
    printf("can't open %s\n", filename);
    goto FINISH;
  }

  /* initialize stuff */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr) {
    printf("[write_png_file] png_create_write_struct failed");
    goto FINISH;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    printf("[write_png_file] png_create_info_struct failed");
    goto FINISH;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    printf("[write_png_file] Error during init_io");
    goto FINISH;
  }
  /* write header */
  png_init_io(png_ptr, outfile);

  if (setjmp(png_jmpbuf(png_ptr))) {
    printf("[write_png_file] Error during writing header");
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

    sprintf(desc, "Yaba Sanshiro Version %s\n VENDER: %s\n RENDERER: %s\n VERSION %s\n", YAB_VERSION, glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));
    text[txt_fields].key = "Description";
    text[txt_fields].text = desc;
    text[txt_fields].compression = PNG_TEXT_COMPRESSION_NONE;
    txt_fields++;

    png_set_text(png_ptr, info_ptr, text, txt_fields);
  }
  png_write_info(png_ptr, info_ptr);


  /* write bytes */
  if (setjmp(png_jmpbuf(png_ptr))) {
    printf("[write_png_file] Error during writing bytes");
    goto FINISH;
  }
  png_write_image(png_ptr, row_pointers);

  /* end write */
  if (setjmp(png_jmpbuf(png_ptr))) {
    printf("[write_png_file] Error during end of write");
    goto FINISH;
  }

  png_write_end(png_ptr, NULL);
  rtn = 0;

FINISH:
  if (outfile) fclose(outfile);
  //if (buf) free(buf);
  if (bufRGB) free(bufRGB);
  if (row_pointers) free(row_pointers);
  return rtn;

}

extern "C" {

  int YabauseThread_IsUseBios() {
    return 0;
  }

  const char * YabauseThread_getBackupPath() {
    return s_buppath;
  }

  void YabauseThread_setUseBios(int use) {


  }

  char tmpbakcup[256];
  void YabauseThread_setBackupPath(const char * buf) {
    strcpy(tmpbakcup, buf);
    s_buppath = tmpbakcup;
  }

  void YabauseThread_resetPlaymode() {
    if (s_playrecord_path != NULL) {
      free(s_playrecord_path);
      s_playrecord_path = NULL;
    }
    s_buppath = "./back.bin";
  }

  void YabauseThread_coldBoot() {
    YabauseDeInit();
    yabauseinit();
    YabauseReset();
  }


}

