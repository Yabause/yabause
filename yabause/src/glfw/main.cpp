#ifdef _WINDOWS
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#if defined(_USEGLEW_)
#include <GL/glew.h>
#else
#include <GLES3/gl3.h>
#endif
#include <GLFW/glfw3.h>
#include <map>
#include <string>

using std::map;
using std::string;

#include "test_framework.h"

map< int , int > g_Keymap;

extern "C" {
#include "../config.h"
#include "yabause.h"
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


#ifdef _WINDOWS
  //static char biospath[256] = "G:/wkcvs/Emulation/Saturn/satourne v1.0.2p/roms/saturn_bios.bin";
  static char * biospath = NULL;
  //static char cdpath[256] = "C:/ext/osusume/SonycR.cue";
  static char cdpath[256] = "C:/ext/osusume/079 Albert Odyssey (U).cue";
  //static char cdpath[256] = "C:/ext/osusume/akumazyou/ws-dracula_x.bin";
  //static char cdpath[256] = "C:/ext/osusume/thunder_force_v[www.segasoluce.net]/thunder_force.iso";
  //static char cdpath[256] = "C:/ext/osusume/Slam & Jam '96 featuring Magic & Kareem (U)(Saturn)/Slam & Jam '96 featuring Magic & Kareem (U)(Saturn).mds";
  //static char cdpath[256] = "C:/ext/osusume/Black Matrix (J)(Saturn)/black_matrix.bin";
  //static char cdpath[256] = "E:/gameiso/brtrck.bin";
  //static char cdpath[256] = "E:/gameiso/Sonic 3D Blast (U)(Saturn)/125 Sonic 3D Blast (U).bin";
  //static char cdpath[256] = "E:/gameiso/sonicjam.iso";
#else
//static char biospath[256] = "/dat2/project/src/bios.bin";
static char * biospath = NULL;
//static char cdpath[256] = "/dat2/iso/nights.img";
static char cdpath[256] = "/dat2/iso/dytona/Daytona USA.iso";
//static char cdpath[256] = "/media/shinya/d-main1/gameiso/brtrck.bin";
#endif


static char buppath[256] = "./back.bin";
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
  &VIDOGL,
  NULL
};

#ifdef YAB_PORT_OSD
#include "nanovg/nanovg_osdcore.h"
OSD_struct *OSDCoreList[] = {
  &OSDNnovg,
  NULL
};
#endif

GLFWwindow* g_window = NULL;
TestFramework * test_fw_ = nullptr;

void DrawDebugInfo()
{
  float pxRatio;
  int winWidth, winHeight;
  int fbWidth, fbHeight;
  glfwGetFramebufferSize(g_window, &winWidth, &winHeight);
  pxRatio = (float)winWidth / (float)winHeight;
  glfwGetFramebufferSize(g_window, &fbWidth, &fbHeight);
}

void YuiErrorMsg(const char *string)
{
  printf("%s",string);
}

void YuiSwapBuffers(void)
{
  double t, dt;
  static double prevt = 0;
  t = glfwGetTime();
  dt = t - prevt;
  prevt = t;
  if (test_fw_ != nullptr) {
    if (test_fw_->step_in_draw_thread() == TestFramework::FINISHED) {
      printf("TestFramework::FINISHED\n");
      exit(0);
    }
    glfwSwapBuffers(g_window);
    test_fw_->onStartFrame();
  }
  else {
    glfwSwapBuffers(g_window);
  }
  SetOSDToggle(0);
}

void error_callback(int error, const char* description)
{
  fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);


  if( action == GLFW_PRESS )
  {
    //int yabaky = g_Keymap[key];
    PerKeyDown(key);
  }else if( action == GLFW_RELEASE  ){
    //int yabaky = g_Keymap[key];
    PerKeyUp(key);

    if (key == GLFW_KEY_F1){
      YabSaveStateSlot(".\\", 1);
      LOG("SAVE SLOT 1");
    }

    if (key == GLFW_KEY_F2){
      YabLoadStateSlot(".\\", 1);
      LOG("LOAD SLOT 1");
    }

  }
}

int yabauseinit()
{
  int res;
  yabauseinit_struct yinit;
  void * padbits;

  yinit.m68kcoretype = M68KCORE_MUSASHI;
  yinit.percoretype = PERCORE_DUMMY;
#ifdef SH2_DYNAREC
    yinit.sh2coretype = 2;
#else
  //yinit.sh2coretype = 0;
#endif
  yinit.sh2coretype = 3;
  //yinit.vidcoretype = VIDCORE_SOFT;
  yinit.vidcoretype = 1;
  yinit.sndcoretype = SNDCORE_SDL;
  //yinit.sndcoretype = SNDCORE_DUMMY;
  //yinit.cdcoretype = CDCORE_DEFAULT;
  yinit.cdcoretype = CDCORE_ISO;
  yinit.carttype = CART_NONE;
  yinit.regionid = 0;
  yinit.biospath = biospath;
  yinit.cdpath = cdpath;
  yinit.buppath = buppath;
  yinit.mpegpath = mpegpath;
  yinit.cartpath = cartpath;
  yinit.videoformattype = VIDEOFORMATTYPE_NTSC;
  yinit.frameskip = 0;
  yinit.usethreads = 0;
  yinit.skip_load = 0;    
  yinit.video_filter_type = 0;
  yinit.polygon_generation_mode = GPU_TESSERATION;
  yinit.use_new_scsp = 1;
  yinit.resolution_mode = 0;

    res = YabauseInit(&yinit);
    if( res == -1)
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
  OSDChangeCore(OSDCORE_NANOVG);
  
  LogStart();
  LogChangeOutput(DEBUG_CALLBACK, NULL);

  return 0;
}


int YuiSetVideoMode(int width, int height, int bpp, int fullscreen) {
  return 0;
}

GLFWwindow* g_offscreen_context;
int saveScreenshot(const char * filename);

int main( int argc, char * argcv[] )
{
#ifdef _WINDOWS
  ::AllocConsole();
#endif
  int width = 800;
  int height = 600;

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwSetErrorCallback(error_callback);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
  glfwWindowHint(GLFW_RED_BITS,8);
  glfwWindowHint(GLFW_GREEN_BITS,8);
  glfwWindowHint(GLFW_BLUE_BITS,8);
  glfwWindowHint(GLFW_STENCIL_BITS,8);

  //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  //glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);

  g_window = glfwCreateWindow(width, height, "My Title", NULL, NULL);
  if (!g_window)
  {
      glfwTerminate();
      exit(EXIT_FAILURE);
  }

    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    g_offscreen_context = glfwCreateWindow(width,height, "", NULL, g_window);

  glfwMakeContextCurrent(g_window);
  glfwSwapInterval(1);

#if defined(_USEGLEW_)
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
      /* Problem: glewInit failed, something is seriously wrong. */
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
 #endif

  printf("context renderer string: \"%s\"\n", glGetString(GL_RENDERER));
  printf("context vendor string: \"%s\"\n", glGetString(GL_VENDOR));
  printf("version string: \"%s\"\n", glGetString(GL_VERSION));
  printf("Extentions: %s\n",glGetString(GL_EXTENSIONS));

  glfwSetKeyCallback(g_window, key_callback);

  if( yabauseinit() == -1 )
  {
      glfwTerminate();
      exit(EXIT_FAILURE);
  }
  VIDCore->Resize(0,0,width,height,0);

  for (int i = 0; i < argc; i++) {
    string sarg(argcv[i]);
    string::size_type pos;
    if ((pos = sarg.find("test=")) != string::npos) {
      std::string test_path = sarg.substr(pos + 5);
      test_fw_ = new TestFramework();
      if (test_fw_->load("Yaba Sanshiro", test_path) != 0) {
        printf("Fail to read %s\n", test_path.c_str());
        exit(-1);
      }
      test_fw_->setSaveScreenShotCallback(saveScreenshot);
    }
  }

  glfwSetTime(0);
  double prevt = glfwGetTime();

  while (!glfwWindowShouldClose(g_window)) {
    if (test_fw_ != nullptr) {
      if (test_fw_->step_in_main_thread() == TestFramework::FINISHED) {
        printf("TestFramework::FINISHED\n");
        exit(0);
      }
    }
    YabauseExec(); // exec one frame
    glfwPollEvents();
  }


  YabauseDeInit();
  glfwDestroyWindow(g_window);
  glfwTerminate();
  return 0;
}

int YuiRevokeOGLOnThisThread(){
    glfwMakeContextCurrent(g_offscreen_context);
    return 0;
}

int YuiUseOGLOnThisThread(){
    glfwMakeContextCurrent(g_window);
    return 0;
}

int xprintf( const char * fmt, ... )
{
  //va_list ap;
  //va_start(ap, fmt);
  //int result = __android_log_vprint(ANDROID_LOG_INFO, LOG_TAG, fmt, ap);
  //va_end(ap);
  return 0;
}
}

#include "png.h"
#include "ygl.h"

int saveScreenshot(const char * filename) {

  int width;
  int height;
  unsigned char * buf = NULL;
  unsigned char * bufRGB = NULL;
  png_bytep * row_pointers = NULL;
  int quality = 100; // best
  FILE * outfile = NULL;
  int glerror;
  int u, v;
  int pmode;
  png_byte color_type;
  png_byte bit_depth;
  png_structp png_ptr;
  png_infop info_ptr;
  int rtn = -1;

  glfwGetFramebufferSize(g_window, &width, &height);

  buf = (unsigned char *)malloc(width*height * 4);
  if (buf == NULL) {
    printf("not enough memory\n");
    goto FINISH;
  }

  glReadBuffer(GL_BACK);
  pmode = GL_RGBA;
  glGetError();
  glReadPixels(0, 0, width, height, pmode, GL_UNSIGNED_BYTE, buf);
  if ((glerror = glGetError()) != GL_NO_ERROR) {
    printf("glReadPixels %04X\n", glerror);
    goto FINISH;
  }

  for (u = 3; u <width*height * 4; u += 4) {
    buf[u] = 0xFF;
  }
  row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
  for (v = 0; v<height; v++)
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
    text[txt_fields].text = "test";
    text[txt_fields].compression = PNG_TEXT_COMPRESSION_NONE;
    txt_fields++;

    text[txt_fields].key = "Description";
    text[txt_fields].text = "test";
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
  if (buf) free(buf);
  if (bufRGB) free(bufRGB);
  if (row_pointers) free(row_pointers);

  return rtn;
}

