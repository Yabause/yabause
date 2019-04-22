#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include <sys/stat.h>

#include <libretro.h>

#include <file/file_path.h>

#include "vdp1.h"
#include "vdp2.h"
#include "peripheral.h"
#include "cdbase.h"
#include "stv.h"
#include "yabause.h"
#include "yui.h"
#include "cheat.h"

#include "cs0.h"
#include "cs2.h"

#include "m68kcore.h"
#include "vidogl.h"
#include "vidsoft.h"
#include "ygl.h"
#include "sh2int_kronos.h"

yabauseinit_struct yinit;

static char slash = path_default_slash_c();

static char g_save_dir[PATH_MAX];
static char g_system_dir[PATH_MAX];
static char full_path[PATH_MAX];
static char bios_path[PATH_MAX];
static char stv_bios_path[PATH_MAX];
static char bup_path[PATH_MAX];
static char addon_cart_path[PATH_MAX];

static char game_basename[128];

static int game_width  = 320;
static int game_height = 240;
static int game_interlace;

static int current_width;
static int current_height;

static bool hle_bios_force = false;
static int addon_cart_type = CART_NONE;
static int filter_mode = AA_NONE;
static int upscale_mode = UP_NONE;
static int scanlines = 0;

static int g_sh2coretype = 8;
static int g_videoformattype = VIDEOFORMATTYPE_NTSC;
static int resolution_mode = 1;
static int polygon_mode = PERSPECTIVE_CORRECTION;
static int initial_resolution_mode = 0;
static int numthreads = 4;
static int use_beetle_saves = 0;
static int auto_select_cart = 1;
static bool service_enabled = false;
static bool stv_mode = false;
static bool all_devices_ready = false;
static int pad_type[12] = {1,1,1,1,1,1,1,1,1,1,1,1};
static int multitap[2] = {0,0};
static unsigned players = 7;

struct retro_perf_callback perf_cb;
retro_get_cpu_features_t perf_get_cpu_features_cb = NULL;

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_batch_t audio_batch_cb;

#if defined(_USEGLEW_)
static struct retro_hw_render_callback hw_render;
#else
extern struct retro_hw_render_callback hw_render;
#endif

void retro_set_environment(retro_environment_t cb)
{
   static const struct retro_variable vars[] = {
      { "kronos_force_hle_bios", "Force HLE BIOS (restart, deprecated, debug only); disabled|enabled" },
      { "kronos_videoformattype", "Video format; NTSC|PAL" },
      { "kronos_sh2coretype", "SH2 Core (restart); kronos|interpreter" },
      { "kronos_use_beetle_saves", "Share saves with beetle (restart); disabled|enabled" },
      { "kronos_addon_cart", "Addon Cartridge (restart); none|1M_extended_ram|4M_extended_ram|512K_backup_ram|1M_backup_ram|2M_backup_ram|4M_backup_ram" },
      { "kronos_auto_select_cart", "Automatic Addon Cartridge (restart); enabled|disabled" },
      { "kronos_multitap_port1", "6Player Adaptor on Port 1; disabled|enabled" },
      { "kronos_multitap_port2", "6Player Adaptor on Port 2; disabled|enabled" },
      { "kronos_filter_mode", "Filter Mode; none|bilinear|bicubic" },
      { "kronos_upscale_mode", "Upscale Mode; none|hq4x|4xbrz|2xbrz" },
      { "kronos_resolution_mode", "Resolution Mode; original|2x|4x|8x|16x" },
      { "kronos_polygon_mode", "Polygon Mode; perspective_correction|gpu_tesselation|cpu_tesselation" },
      { "kronos_scanlines", "Scanlines; disabled|enabled" },
      { "kronos_service_enabled", "ST-V Service/Test Buttons; disabled|enabled" },
      { NULL, NULL },
   };

   static const struct retro_controller_description peripherals[] = {
       { "Saturn Pad", RETRO_DEVICE_JOYPAD },
       { "Saturn 3D Pad", RETRO_DEVICE_ANALOG },
       { "None", RETRO_DEVICE_NONE },
   };

   static const struct retro_controller_info ports[] = {
      { peripherals, 3 },
      { peripherals, 3 },
      { peripherals, 3 },
      { peripherals, 3 },
      { peripherals, 3 },
      { peripherals, 3 },
      { peripherals, 3 },
      { peripherals, 3 },
      { peripherals, 3 },
      { peripherals, 3 },
      { peripherals, 3 },
      { peripherals, 3 },
      { NULL, 0 },
   };

   environ_cb = cb;

   cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);
   environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

// PERLIBRETRO
#define PERCORE_LIBRETRO 2

int PERLIBRETROInit(void)
{
   void *controller;

   // ST-V
   if(stv_mode) {
      PerPortReset();
      controller = (void*)PerCabAdd(NULL);
      PerSetKey(PERPAD_UP, PERPAD_UP, controller);
      PerSetKey(PERPAD_RIGHT, PERPAD_RIGHT, controller);
      PerSetKey(PERPAD_DOWN, PERPAD_DOWN, controller);
      PerSetKey(PERPAD_LEFT, PERPAD_LEFT, controller);
      PerSetKey(PERPAD_A, PERPAD_A, controller);
      PerSetKey(PERPAD_B, PERPAD_B, controller);
      PerSetKey(PERPAD_C, PERPAD_C, controller);
      PerSetKey(PERPAD_X, PERPAD_X, controller);
      PerSetKey(PERPAD_Y, PERPAD_Y, controller);
      PerSetKey(PERPAD_Z, PERPAD_Z, controller);
      PerSetKey(PERJAMMA_COIN1, PERJAMMA_COIN1, controller );
      PerSetKey(PERJAMMA_COIN2, PERJAMMA_COIN2, controller );
      PerSetKey(PERJAMMA_TEST, PERJAMMA_TEST, controller);
      PerSetKey(PERJAMMA_SERVICE, PERJAMMA_SERVICE, controller);
      PerSetKey(PERJAMMA_START1, PERJAMMA_START1, controller);
      PerSetKey(PERJAMMA_START2, PERJAMMA_START2, controller);
      PerSetKey(PERJAMMA_MULTICART, PERJAMMA_MULTICART, controller);
      PerSetKey(PERJAMMA_PAUSE, PERJAMMA_PAUSE, controller);
      PerSetKey(PERJAMMA_P2_UP, PERJAMMA_P2_UP, controller );
      PerSetKey(PERJAMMA_P2_RIGHT, PERJAMMA_P2_RIGHT, controller );
      PerSetKey(PERJAMMA_P2_DOWN, PERJAMMA_P2_DOWN, controller );
      PerSetKey(PERJAMMA_P2_LEFT, PERJAMMA_P2_LEFT, controller );
      PerSetKey(PERJAMMA_P2_BUTTON1, PERJAMMA_P2_BUTTON1, controller );
      PerSetKey(PERJAMMA_P2_BUTTON2, PERJAMMA_P2_BUTTON2, controller );
      PerSetKey(PERJAMMA_P2_BUTTON3, PERJAMMA_P2_BUTTON3, controller );
      PerSetKey(PERJAMMA_P2_BUTTON4, PERJAMMA_P2_BUTTON4, controller );
      return 0;
   }

   uint32_t i, j;
   PortData_struct* portdata = NULL;

   //1 multitap + 1 peripherial
   if(multitap[0] == 0 && multitap[1] == 0)
      players = 2;
   else if(multitap[0] == 1 && multitap[1] == 1)
      players = 12;

   PerPortReset();

   for(i = 0; i < players; i++)
   {
      //Ports can handle 6 peripherals, fill port 1 first.
      if((players > 2 && i < 6) || i == 0)
         portdata = &PORTDATA1;
      else
         portdata = &PORTDATA2;

      switch(pad_type[i])
      {
         case RETRO_DEVICE_NONE:
            controller = NULL;
            break;
         case RETRO_DEVICE_ANALOG:
            controller = (void*)Per3DPadAdd(portdata);
            for(j = PERPAD_UP; j <= PERPAD_Z; j++)
               PerSetKey((i << 8) + j, j, controller);
            for(j = PERANALOG_AXIS1; j <= PERANALOG_AXIS7; j++)
               PerSetKey((i << 8) + j, j, controller);
            break;
         case RETRO_DEVICE_JOYPAD:
         default:
            controller = (void*)PerPadAdd(portdata);
            for(j = PERPAD_UP; j <= PERPAD_Z; j++)
               PerSetKey((i << 8) + j, j, controller);
            break;
      }
   }

   return 0;
}

static int PERLIBRETROHandleEvents(void)
{
   unsigned i = 0;

   input_poll_cb();

   if (stv_mode && service_enabled)
   {
      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
         PerKeyDown(PERJAMMA_TEST);
      else
         PerKeyUp(PERJAMMA_TEST);

      if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
         PerKeyDown(PERJAMMA_SERVICE);
      else
         PerKeyUp(PERJAMMA_SERVICE);
   }

   for(i = 0; i < players; i++)
   {
      if(stv_mode) {
         if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT)) {
            if(i == 0)
               PerKeyDown(PERJAMMA_COIN1);
            else
               PerKeyDown(PERJAMMA_COIN2);
         }
         else {
            if(i == 0)
               PerKeyUp(PERJAMMA_COIN1);
            else
               PerKeyUp(PERJAMMA_COIN2);
         }

         if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START)) {
            if(i == 0)
               PerKeyDown(PERJAMMA_START1);
            else
               PerKeyDown(PERJAMMA_START2);
         }
         else {
            if(i == 0)
               PerKeyUp(PERJAMMA_START1);
            else
               PerKeyUp(PERJAMMA_START2);
         }

         if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP)) {
            if(i == 0)
               PerKeyDown(PERPAD_UP);
            else
               PerKeyDown(PERJAMMA_P2_UP);
         }
         else {
            if(i == 0)
               PerKeyUp(PERPAD_UP);
            else
               PerKeyUp(PERJAMMA_P2_UP);
         }

         if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT)) {
            if(i == 0)
               PerKeyDown(PERPAD_RIGHT);
            else
               PerKeyDown(PERJAMMA_P2_RIGHT);
         }
         else {
            if(i == 0)
               PerKeyUp(PERPAD_RIGHT);
            else
               PerKeyUp(PERJAMMA_P2_RIGHT);
         }

         if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN)) {
            if(i == 0)
               PerKeyDown(PERPAD_DOWN);
            else
               PerKeyDown(PERJAMMA_P2_DOWN);
         }
         else {
            if(i == 0)
               PerKeyUp(PERPAD_DOWN);
            else
               PerKeyUp(PERJAMMA_P2_DOWN);
         }

         if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT)) {
            if(i == 0)
               PerKeyDown(PERPAD_LEFT);
            else
               PerKeyDown(PERJAMMA_P2_LEFT);
         }
         else {
            if(i == 0)
               PerKeyUp(PERPAD_LEFT);
            else
               PerKeyUp(PERJAMMA_P2_LEFT);
         }

         if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B)) {
            if(i == 0)
               PerKeyDown(PERPAD_A);
            else
               PerKeyDown(PERJAMMA_P2_BUTTON1);
         }
         else {
            if(i == 0)
               PerKeyUp(PERPAD_A);
            else
               PerKeyUp(PERJAMMA_P2_BUTTON1);
         }

         if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A)) {
            if(i == 0)
               PerKeyDown(PERPAD_B);
            else
               PerKeyDown(PERJAMMA_P2_BUTTON2);
         }
         else {
            if(i == 0)
               PerKeyUp(PERPAD_B);
            else
               PerKeyUp(PERJAMMA_P2_BUTTON2);
         }

         if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y)) {
            if(i == 0)
               PerKeyDown(PERPAD_C);
            else
               PerKeyDown(PERJAMMA_P2_BUTTON3);
         }
         else {
            if(i == 0)
               PerKeyUp(PERPAD_C);
            else
               PerKeyUp(PERJAMMA_P2_BUTTON3);
         }

         if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X)) {
            if(i == 0)
               PerKeyDown(PERPAD_X);
            else
               PerKeyDown(PERJAMMA_P2_BUTTON4);
         }
         else {
            if(i == 0)
               PerKeyUp(PERPAD_X);
            else
               PerKeyUp(PERJAMMA_P2_BUTTON4);
         }
      } else {

         int analog_left_x = 0;
         int analog_left_y = 0;
         int analog_right_x = 0;
         int analog_right_y = 0;
         uint16_t l_trigger, r_trigger;

         switch(pad_type[i])
         {
            case RETRO_DEVICE_ANALOG:
               analog_left_x = input_state_cb(i, RETRO_DEVICE_ANALOG,
                     RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);

               PerAxisValue((i << 8) + PERANALOG_AXIS1, (u8)((analog_left_x + 0x8000) >> 8));

               analog_left_y = input_state_cb(i, RETRO_DEVICE_ANALOG,
                     RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);

               PerAxisValue((i << 8) + PERANALOG_AXIS2, (u8)((analog_left_y + 0x8000) >> 8));

               // analog triggers
               l_trigger = input_state_cb( i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_BUTTON, RETRO_DEVICE_ID_JOYPAD_L2 );
               r_trigger = input_state_cb( i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_BUTTON, RETRO_DEVICE_ID_JOYPAD_R2 );

               // if no analog trigger support, use digital
               if (l_trigger == 0)
                  l_trigger = input_state_cb( i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2 ) ? 0x7FFF : 0;
               if (r_trigger == 0)
                  r_trigger = input_state_cb( i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2 ) ? 0x7FFF : 0;

               PerAxisValue((i << 8) + PERANALOG_AXIS3, (u8)((r_trigger > 0 ? r_trigger + 0x8000 : 0) >> 8));
               PerAxisValue((i << 8) + PERANALOG_AXIS4, (u8)((l_trigger > 0 ? l_trigger + 0x8000 : 0) >> 8));

            case RETRO_DEVICE_JOYPAD:

               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
                  PerKeyDown((i << 8) + PERPAD_UP);
               else
                  PerKeyUp((i << 8) + PERPAD_UP);
               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
                  PerKeyDown((i << 8) + PERPAD_DOWN);
               else
                  PerKeyUp((i << 8) + PERPAD_DOWN);
               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
                  PerKeyDown((i << 8) + PERPAD_LEFT);
               else
                  PerKeyUp((i << 8) + PERPAD_LEFT);
               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
                  PerKeyDown((i << 8) + PERPAD_RIGHT);
               else
                  PerKeyUp((i << 8) + PERPAD_RIGHT);

               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y))
                  PerKeyDown((i << 8) + PERPAD_X);
               else
                  PerKeyUp((i << 8) + PERPAD_X);

               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B))
                  PerKeyDown((i << 8) + PERPAD_A);
               else
                  PerKeyUp((i << 8) + PERPAD_A);

               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A))
                  PerKeyDown((i << 8) + PERPAD_B);
               else
                  PerKeyUp((i << 8) + PERPAD_B);

               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X))
                  PerKeyDown((i << 8) + PERPAD_Y);
               else
                  PerKeyUp((i << 8) + PERPAD_Y);

               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L))
                  PerKeyDown((i << 8) + PERPAD_C);
               else
                  PerKeyUp((i << 8) + PERPAD_C);

               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R))
                  PerKeyDown((i << 8) + PERPAD_Z);
               else
                  PerKeyUp((i << 8) + PERPAD_Z);

               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
                  PerKeyDown((i << 8) + PERPAD_START);
               else
                  PerKeyUp((i << 8) + PERPAD_START);

               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
                  PerKeyDown((i << 8) + PERPAD_LEFT_TRIGGER);
               else
                  PerKeyUp((i << 8) + PERPAD_LEFT_TRIGGER);

               if (input_state_cb(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
                  PerKeyDown((i << 8) + PERPAD_RIGHT_TRIGGER);
               else
                  PerKeyUp((i << 8) + PERPAD_RIGHT_TRIGGER);
               break;

            default:
               break;
         }
      }
   }

   return 0;
}

void PERLIBRETRODeInit(void) {}

void PERLIBRETRONothing(void) {}

u32 PERLIBRETROScan(u32 flags) { return 0;}

void PERLIBRETROKeyName(u32 key, char *name, int size) {}

PerInterface_struct PERLIBRETROJoy = {
    PERCORE_LIBRETRO,
    "Libretro Input Interface",
    PERLIBRETROInit,
    PERLIBRETRODeInit,
    PERLIBRETROHandleEvents,
    PERLIBRETROScan,
    0,
    PERLIBRETRONothing,
    PERLIBRETROKeyName
};

// SNDLIBRETRO
#define SNDCORE_LIBRETRO   11
#define SAMPLERATE         44100

static u32 audio_size;
static u32 soundlen;
static u32 soundbufsize;
static s16 *sound_buf;

static int SNDLIBRETROInit(void) {
    int vertfreq = (yabsys.IsPal == 1 ? 50 : 60);
    soundlen = (SAMPLERATE * 100 + (vertfreq >> 1)) / vertfreq;
    soundbufsize = (soundlen<<2 * sizeof(s16));
    if ((sound_buf = (s16 *)malloc(soundbufsize)) == NULL)
        return -1;
    memset(sound_buf, 0, soundbufsize);
    return 0;
}

static void SNDLIBRETRODeInit(void) {
   if (sound_buf)
      free(sound_buf);
}

static int SNDLIBRETROReset(void) { return 0; }

static int SNDLIBRETROChangeVideoFormat(int vertfreq)
{
    soundlen = (SAMPLERATE * 100 + (vertfreq >> 1)) / vertfreq;
    soundbufsize = (soundlen<<2 * sizeof(s16));
    if (sound_buf)
        free(sound_buf);
    if ((sound_buf = (s16 *)malloc(soundbufsize)) == NULL)
        return -1;
    memset(sound_buf, 0, soundbufsize);
    return 0;
}

static void sdlConvert32uto16s(int32_t *srcL, int32_t *srcR, int16_t *dst, size_t len)
{
   u32 i;

   for (i = 0; i < len; i++)
   {
      // Left Channel
      if (*srcL > 0x7FFF)
         *dst = 0x7FFF;
      else if (*srcL < -0x8000)
         *dst = -0x8000;
      else
         *dst = *srcL;
      srcL++;
      dst++;

      // Right Channel
      if (*srcR > 0x7FFF)
         *dst = 0x7FFF;
      else if (*srcR < -0x8000)
         *dst = -0x8000;
      else
         *dst = *srcR;
      srcR++;
      dst++;
   }
}

static void SNDLIBRETROUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples)
{
   sdlConvert32uto16s((int32_t*)leftchanbuffer, (int32_t*)rightchanbuffer, sound_buf, num_samples);
   audio_batch_cb(sound_buf, num_samples);

   audio_size -= num_samples;
}

static u32 SNDLIBRETROGetAudioSpace(void) { return audio_size; }

void SNDLIBRETROMuteAudio(void) {}

void SNDLIBRETROUnMuteAudio(void) {}

void SNDLIBRETROSetVolume(int volume) {}

SoundInterface_struct SNDLIBRETRO = {
    SNDCORE_LIBRETRO,
    "Libretro Sound Interface",
    SNDLIBRETROInit,
    SNDLIBRETRODeInit,
    SNDLIBRETROReset,
    SNDLIBRETROChangeVideoFormat,
    SNDLIBRETROUpdateAudio,
    SNDLIBRETROGetAudioSpace,
    SNDLIBRETROMuteAudio,
    SNDLIBRETROUnMuteAudio,
    SNDLIBRETROSetVolume
};

M68K_struct *M68KCoreList[] = {
    &M68KDummy,
#ifdef HAVE_MUSASHI
    &M68KMusashi,
#else
    &M68KC68K,
#endif
    NULL
};

SH2Interface_struct *SH2CoreList[] = {
    &SH2Interpreter,
    &SH2DebugInterpreter,
    &SH2KronosInterpreter,
    NULL
};

PerInterface_struct *PERCoreList[] = {
    &PERDummy,
    &PERLIBRETROJoy,
    NULL
};

CDInterface *CDCoreList[] = {
    &DummyCD,
    &ISOCD,
    NULL
};

SoundInterface_struct *SNDCoreList[] = {
    &SNDDummy,
    &SNDLIBRETRO,
    NULL
};

VideoInterface_struct *VIDCoreList[] = {
    //&VIDDummy,
    &VIDOGL,
    &VIDSoft,
    NULL
};

#pragma mark Yabause Callbacks

void YuiMsg(const char *format, ...)
{
  char buf[512]; 
  va_list arglist;
  va_start( arglist, format );
  int rc = vsnprintf(buf, 512, format, arglist);
  va_end( arglist );
  log_cb(RETRO_LOG_INFO, buf);
}

void YuiErrorMsg(const char *string)
{
   if (log_cb)
      log_cb(RETRO_LOG_ERROR, "Yabause: %s\n", string);
}

static int first_ctx_reset = 1;

int YuiUseOGLOnThisThread()
{
#if !defined(_USEGLEW_)
  return glsm_ctl(GLSM_CTL_STATE_BIND, NULL);
#endif
}

int YuiRevokeOGLOnThisThread()
{
#if !defined(_USEGLEW_)
  return glsm_ctl(GLSM_CTL_STATE_UNBIND, NULL);
#endif
}

int YuiGetFB(void)
{
  return hw_render.get_current_framebuffer();
}

void retro_reinit_av_info(void)
{
    struct retro_system_av_info av_info;
    retro_get_system_av_info(&av_info);
    environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &av_info);
}

void retro_set_resolution()
{
   // If resolution_mode > initial_resolution_mode, we'll need a restart to reallocate the max size for buffer
   if (resolution_mode > initial_resolution_mode)
   {
      log_cb(RETRO_LOG_INFO, "Restart the core for x%d resolution\n", resolution_mode);
      resolution_mode = initial_resolution_mode;
   }
   // Downscale x16 resolution_mode for Hi-Res games
   if (game_height > 256 && resolution_mode > 8)
   {
      log_cb(RETRO_LOG_INFO, "Hi-Res games limited to x8\n", resolution_mode);
      resolution_mode = 8;
   }
   current_width = game_width * resolution_mode;
   current_height = game_height * resolution_mode;
   VIDCore->Resize(0, 0, current_width, current_height, 0);
   retro_reinit_av_info();
   VIDCore->SetSettingValue(VDP_SETTING_RESOLUTION_MODE, resolution_mode);
}

void YuiSwapBuffers(void)
{
   int prev_game_width = game_width;
   int prev_game_height = game_height;
   VIDCore->GetNativeResolution(&game_width, &game_height, &game_interlace);
   if ((prev_game_width != game_width) || (prev_game_height != game_height))
      retro_set_resolution();
   audio_size = soundlen;
   video_cb(RETRO_HW_FRAME_BUFFER_VALID, current_width, current_height, 0);
}

static void context_reset(void)
{
#if !defined(_USEGLEW_)
   glsm_ctl(GLSM_CTL_STATE_CONTEXT_RESET, NULL);
   glsm_ctl(GLSM_CTL_STATE_SETUP, NULL);
#endif
   if (first_ctx_reset == 1)
   {
      first_ctx_reset = 0;
      YabauseInit(&yinit);
      YabauseSetVideoFormat(g_videoformattype);
      retro_set_resolution();
      OSDChangeCore(OSDCORE_DUMMY);
   }
   else
   {
      VIDCore->Init();
      retro_set_resolution();
   }
}

static void context_destroy(void)
{
   VIDCore->DeInit();
#if !defined(_USEGLEW_)
   glsm_ctl(GLSM_CTL_STATE_CONTEXT_DESTROY, NULL);
#endif
}

static bool retro_init_hw_context(void)
{
#if defined(_USEGLEW_)
   hw_render.context_reset = context_reset;
   hw_render.context_destroy = context_destroy;
   hw_render.depth = true;
   hw_render.bottom_left_origin = true;
#ifdef HAVE_GLES
   hw_render.context_type = RETRO_HW_CONTEXT_OPENGLES_VERSION;
   hw_render.version_major = 3;
   hw_render.version_minor = 1;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render))
      return false;
#else
   hw_render.context_type = RETRO_HW_CONTEXT_OPENGL_CORE;
   if (polygon_mode == GPU_TESSERATION)
   {
      hw_render.version_major = 4;
      hw_render.version_minor = 2;
   }
   else
   {
      hw_render.version_major = 3;
      hw_render.version_minor = 3;
   }
   if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render))
       return false;
#endif
#else
   glsm_ctx_params_t params = {0};
   params.context_reset = context_reset;
   params.context_destroy = context_destroy;
   params.environ_cb = environ_cb;
   params.stencil = true;
#ifdef HAVE_GLES
   params.context_type = RETRO_HW_CONTEXT_OPENGLES_VERSION;
   params.major = 3;
   params.minor = 1;
   if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params))
      return false;
#else
   params.context_type = RETRO_HW_CONTEXT_OPENGL_CORE;
   if (polygon_mode == GPU_TESSERATION)
   {
      params.major = 4;
      params.minor = 2;
   }
   else
   {
      params.major = 3;
      params.minor = 3;
   }
   if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params))
      return false;
#endif
#endif
   return true;
}

/************************************
 * libretro implementation
 ************************************/

static struct retro_system_av_info g_av_info;

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "Kronos";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "v" VERSION GIT_VERSION;
   info->need_fullpath    = true;
   info->block_extract    = true;
   info->valid_extensions = "cue|iso|mds|ccd|zip";
}

void check_variables(void)
{
   struct retro_variable var;

   var.key = "kronos_force_hle_bios";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0 && hle_bios_force)
         hle_bios_force = false;
      else if (strcmp(var.value, "enabled") == 0 && !hle_bios_force)
         hle_bios_force = true;
   }

   var.key = "kronos_videoformattype";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "NTSC") == 0)
         g_videoformattype = VIDEOFORMATTYPE_NTSC;
      else if (strcmp(var.value, "PAL") == 0)
         g_videoformattype = VIDEOFORMATTYPE_PAL;
   }

   var.key = "kronos_sh2coretype";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "kronos") == 0)
         g_sh2coretype = 8;
      else if (strcmp(var.value, "interpreter") == 0)
         g_sh2coretype = SH2CORE_INTERPRETER;
   }

   var.key = "kronos_use_beetle_saves";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         use_beetle_saves = 0;
      else if (strcmp(var.value, "enabled") == 0)
         use_beetle_saves = 1;
   }

   var.key = "kronos_addon_cart";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "auto") == 0 && addon_cart_type != -1)
         addon_cart_type = -1;
      else if (strcmp(var.value, "none") == 0 && addon_cart_type != CART_NONE)
         addon_cart_type = CART_NONE;
      else if (strcmp(var.value, "1M_extended_ram") == 0 && addon_cart_type != CART_DRAM8MBIT)
         addon_cart_type = CART_DRAM8MBIT;
      else if (strcmp(var.value, "4M_extended_ram") == 0 && addon_cart_type != CART_DRAM32MBIT)
         addon_cart_type = CART_DRAM32MBIT;
      else if (strcmp(var.value, "512K_backup_ram") == 0 && addon_cart_type != CART_BACKUPRAM4MBIT)
         addon_cart_type = CART_BACKUPRAM4MBIT;
      else if (strcmp(var.value, "1M_backup_ram") == 0 && addon_cart_type != CART_BACKUPRAM8MBIT)
         addon_cart_type = CART_BACKUPRAM8MBIT;
      else if (strcmp(var.value, "2M_backup_ram") == 0 && addon_cart_type != CART_BACKUPRAM16MBIT)
         addon_cart_type = CART_BACKUPRAM16MBIT;
      else if (strcmp(var.value, "4M_backup_ram") == 0 && addon_cart_type != CART_BACKUPRAM32MBIT)
         addon_cart_type = CART_BACKUPRAM32MBIT;
   }

   var.key = "kronos_auto_select_cart";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         auto_select_cart = 1;
      else if (strcmp(var.value, "disabled") == 0)
         auto_select_cart = 0;
   }

   var.key = "kronos_multitap_port1";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         multitap[0] = 0;
      else if (strcmp(var.value, "enabled") == 0)
         multitap[0] = 1;
   }

   var.key = "kronos_multitap_port2";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         multitap[1] = 0;
      else if (strcmp(var.value, "enabled") == 0)
         multitap[1] = 1;
   }

   var.key = "kronos_filter_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "none") == 0 && filter_mode != AA_NONE)
         filter_mode = AA_NONE;
      else if (strcmp(var.value, "bilinear") == 0 && filter_mode != AA_BILINEAR_FILTER)
         filter_mode = AA_BILINEAR_FILTER;
      else if (strcmp(var.value, "bicubic") == 0 && filter_mode != AA_BICUBIC_FILTER)
         filter_mode = AA_BICUBIC_FILTER;
   }

   var.key = "kronos_upscale_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "none") == 0 && upscale_mode != UP_NONE)
         upscale_mode = UP_NONE;
      else if (strcmp(var.value, "hq4x") == 0 && upscale_mode != UP_HQ4X)
         upscale_mode = UP_HQ4X;
      else if (strcmp(var.value, "4xbrz") == 0 && upscale_mode != UP_4XBRZ)
         upscale_mode = UP_4XBRZ;
      else if (strcmp(var.value, "2xbrz") == 0 && upscale_mode != UP_2XBRZ)
         upscale_mode = UP_2XBRZ;
   }

   var.key = "kronos_resolution_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "original") == 0)
         resolution_mode = 1;
      else if (strcmp(var.value, "2x") == 0)
         resolution_mode = 2;
      else if (strcmp(var.value, "4x") == 0)
         resolution_mode = 4;
      else if (strcmp(var.value, "8x") == 0)
         resolution_mode = 8;
      else if (strcmp(var.value, "16x") == 0)
         resolution_mode = 16;
   }

   var.key = "kronos_polygon_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "perspective_correction") == 0)
         polygon_mode = PERSPECTIVE_CORRECTION;
      else if (strcmp(var.value, "gpu_tesselation") == 0)
         polygon_mode = GPU_TESSERATION;
      else if (strcmp(var.value, "cpu_tesselation") == 0)
         polygon_mode = CPU_TESSERATION;
   }

   var.key = "kronos_scanlines";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0 && scanlines != 0)
         scanlines = 0;
      else if (strcmp(var.value, "enabled") == 0 && scanlines != 1)
         scanlines = 1;
   }

   var.key = "kronos_service_enabled";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         service_enabled = true;
      else
         service_enabled = false;
   }
}

static void set_descriptors(void)
{
   struct retro_input_descriptor *input_descriptors = (struct retro_input_descriptor*)calloc(((stv_mode?((2*10)+2):(17*players))+1), sizeof(struct retro_input_descriptor));

   if(stv_mode)
   {
      unsigned j = 0;
      for (unsigned i = 0; i < 2; i++)
      {
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Left" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Up" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Down" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Right" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Button 1" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Button 2" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Button 3" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Button 4" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Coin" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" };
         if (i == 0)
         {
            input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,  "Test" };
            input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,  "Service" };
         }
      }
      input_descriptors[j].description = NULL;
   }
   else
   {
      unsigned j = 0;
      for (unsigned i = 0; i < players; i++)
      {
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,  "Analog X" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,  "Analog Y" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" };
         input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" };
      }
      input_descriptors[j].description = NULL;
   }
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, input_descriptors);
   free(input_descriptors);
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));

   if(initial_resolution_mode == 0)
   {
      // Get the initial resolution mode at start
      // It will be the resolution_mode limit until the core is restarted
      check_variables();
      initial_resolution_mode = resolution_mode;
   }

   info->timing.fps            = (retro_get_region() == RETRO_REGION_NTSC) ? 60.0f : 50.0f;
   info->timing.sample_rate    = SAMPLERATE;
   info->geometry.base_width   = game_width;
   info->geometry.base_height  = game_height;
   // No need to go above 8x what is needed by Hi-Res games, we disallow 16x for Hi-Res games
   info->geometry.max_width    = 704 * (initial_resolution_mode == 16 ? 8 : initial_resolution_mode);
   info->geometry.max_height   = 512 * (initial_resolution_mode == 16 ? 8 : initial_resolution_mode);
   info->geometry.aspect_ratio = (retro_get_region() == RETRO_REGION_NTSC) ? 4.0 / 3.0 : 5.0 / 4.0;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   if(pad_type[port] != device)
   {
      pad_type[port] = device;
      if(PERCore)
         PERCore->Init();
      // When all devices are set, we can send input descriptors
      if (all_devices_ready)
         set_descriptors();
   }
}

size_t retro_serialize_size(void)
{
   void *buffer;
   size_t size;

   ScspMuteAudio(SCSP_MUTE_SYSTEM);
   YabSaveStateBuffer (&buffer, &size);
   ScspUnMuteAudio(SCSP_MUTE_SYSTEM);

   free(buffer);

   return size;
}

bool retro_serialize(void *data, size_t size)
{
   void *buffer;
   size_t out_size;

   int error = YabSaveStateBuffer (&buffer, &out_size);

   memcpy(data, buffer, size);

   free(buffer);
   return !error;
}

bool retro_unserialize(const void *data, size_t size)
{
   int error = YabLoadStateBuffer(data, size);
   retro_set_resolution();

   return !error;
}

void retro_cheat_reset(void)
{
   CheatClearCodes();
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;

   if (CheatAddARCode(code) == 0)
      return;
}

static int does_file_exist(const char *filename)
{
   struct stat st;
   int result = stat(filename, &st);
   return result == 0;
}

static void extract_basename(char *buf, const char *path, size_t size)
{
   strncpy(buf, path_basename(path), size - 1);
   buf[size - 1] = '\0';

   char *ext = strrchr(buf, '.');
   if (ext)
      *ext = '\0';
}

void configure_saturn_addon_cart()
{
   if (use_beetle_saves == 1 && (addon_cart_type == CART_BACKUPRAM8MBIT || addon_cart_type == CART_BACKUPRAM16MBIT || addon_cart_type == CART_BACKUPRAM32MBIT))
      addon_cart_type = CART_BACKUPRAM4MBIT;

   if (addon_cart_type == CART_BACKUPRAM4MBIT || auto_select_cart == 1)
   {
      if (use_beetle_saves == 1)
         snprintf(addon_cart_path, sizeof(addon_cart_path), "%s%c%s.bcr", g_save_dir, slash, game_basename);
      else
         snprintf(addon_cart_path, sizeof(addon_cart_path), "%s%ckronos%csaturn%c%s-ext512K.ram", g_save_dir, slash, slash, slash, game_basename);
   }

   if (addon_cart_type == CART_BACKUPRAM8MBIT)
   {
      snprintf(addon_cart_path, sizeof(addon_cart_path), "%s%ckronos%csaturn%c%s-ext1M.ram", g_save_dir, slash, slash, slash, game_basename);
   }

   if (addon_cart_type == CART_BACKUPRAM16MBIT)
   {
      snprintf(addon_cart_path, sizeof(addon_cart_path), "%s%ckronos%csaturn%c%s-ext2M.ram", g_save_dir, slash, slash, slash, game_basename);
   }

   if (addon_cart_type == CART_BACKUPRAM32MBIT)
   {
      snprintf(addon_cart_path, sizeof(addon_cart_path), "%s%ckronos%csaturn%c%s-ext4M.ram", g_save_dir, slash, slash, slash, game_basename);
   }
}

void retro_init(void)
{
   struct retro_log_callback log;
   const char *dir = NULL;

   log_cb                   = NULL;
   perf_get_cpu_features_cb = NULL;
   uint64_t serialization_quirks = RETRO_SERIALIZATION_QUIRK_SINGLE_SESSION;
   /* Performance level for interpreter CPU core is 16 */
   unsigned level           = 16;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;

   if (environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb))
      perf_get_cpu_features_cb = perf_cb.get_cpu_features;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
   {
      strncpy(g_system_dir, dir, sizeof(g_system_dir));
   }

   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
   {
      strncpy(g_save_dir, dir, sizeof(g_save_dir));
   }

   char save_dir[PATH_MAX];
   snprintf(save_dir, sizeof(save_dir), "%s%ckronos%cstv%c", g_save_dir, slash, slash, slash);
   path_mkdir(save_dir);
   snprintf(save_dir, sizeof(save_dir), "%s%ckronos%csaturn%c", g_save_dir, slash, slash, slash);
   path_mkdir(save_dir);

   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);

   environ_cb(RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS, &serialization_quirks);
}

bool retro_load_game_common()
{
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      return false;
   if (!retro_init_hw_context())
      return false;

   yinit.vidcoretype             = VIDCORE_OGL;
   yinit.percoretype             = PERCORE_LIBRETRO;
   yinit.sh2coretype             = g_sh2coretype;
   yinit.sndcoretype             = SNDCORE_LIBRETRO;
#ifdef HAVE_MUSASHI
   yinit.m68kcoretype            = M68KCORE_MUSASHI;
#else
   yinit.m68kcoretype            = M68KCORE_C68K;
#endif
   yinit.regionid                = REGION_AUTODETECT;
   yinit.mpegpath                = NULL;
   // There was some misunderstanding about frameskip, disabling frameskip is actually just enabling a limiter, which is bad in the libretro ecosystem
   yinit.frameskip               = 1;
   yinit.clocksync               = 0;
   yinit.basetime                = 0;
   yinit.usethreads              = 1;
   yinit.numthreads              = numthreads;
#ifdef SPRITE_CACHE
   yinit.useVdp1cache            = 0;
#endif
   yinit.usecache                = 0;
   yinit.skip_load               = 0;
   yinit.video_filter_type       = filter_mode;
   yinit.video_upscale_type      = upscale_mode;
   yinit.polygon_generation_mode = polygon_mode;
   yinit.scanline                = scanlines;
   yinit.stretch                 = 1;
   yinit.extend_backup           = 0;
   yinit.buppath                 = bup_path;

   return true;
}

bool retro_load_game(const struct retro_game_info *info)
{
   if (!info)
      return false;

   check_variables();

   snprintf(full_path, sizeof(full_path), "%s", info->path);

   snprintf(stv_bios_path, sizeof(stv_bios_path), "%s%ckronos%cstvbios.zip", g_system_dir, slash, slash);
   if (does_file_exist(stv_bios_path) != 1)
   {
      log_cb(RETRO_LOG_WARN, "%s NOT FOUND\n", stv_bios_path);
      snprintf(stv_bios_path, sizeof(stv_bios_path), "%s%cstvbios.zip", g_system_dir, slash);
      if (does_file_exist(stv_bios_path) != 1)
      {
         log_cb(RETRO_LOG_WARN, "%s NOT FOUND\n", stv_bios_path);
      }
   }

   snprintf(bios_path, sizeof(bios_path), "%s%ckronos%csaturn_bios.bin", g_system_dir, slash, slash);
   if (does_file_exist(bios_path) != 1)
   {
      log_cb(RETRO_LOG_WARN, "%s NOT FOUND\n", bios_path);
      snprintf(bios_path, sizeof(bios_path), "%s%csaturn_bios.bin", g_system_dir, slash);
      if (does_file_exist(bios_path) != 1)
      {
         log_cb(RETRO_LOG_WARN, "%s NOT FOUND\n", bios_path);
         snprintf(bios_path, sizeof(bios_path), "%s%csega_101.bin", g_system_dir, slash);
         if (does_file_exist(bios_path) != 1)
         {
            log_cb(RETRO_LOG_WARN, "%s NOT FOUND\n", bios_path);
            snprintf(bios_path, sizeof(bios_path), "%s%cmpr-17933.bin", g_system_dir, slash);
            if (does_file_exist(bios_path) != 1)
            {
               log_cb(RETRO_LOG_WARN, "%s NOT FOUND\n", bios_path);
            }
         }
      }
   }

   extract_basename(game_basename, info->path, sizeof(game_basename));

   // Check if the path lead to a ST-V game
   // Store the game "id", if no game id found then this is most likely not a ST-V game
   int stvgame = -1;
   STVGetSingle(full_path, stv_bios_path, &stvgame);

   if (stvgame != -1)
      stv_mode = true;

   if(stv_mode)
   {
      if (does_file_exist(stv_bios_path) != 1)
      {
         log_cb(RETRO_LOG_ERROR, "This is a ST-V game but we are missing the bios, ABORTING\n");
         return false;
      }

      snprintf(bup_path, sizeof(bup_path), "%s%ckronos%cstv%c%s.ram", g_save_dir, slash, slash, slash, game_basename);

      yinit.stvgamepath     = full_path;
      yinit.stvgame         = stvgame;
      yinit.cartpath        = NULL;
      yinit.carttype        = CART_ROMSTV;
      yinit.stvbiospath     = stv_bios_path;
   }
   else
   {
      // Real bios is REQUIRED, even if we support HLE bios
      // HLE bios is deprecated and causing more issues than it solves
      // No "autoselect HLE when bios is missing" ever again !
      if (does_file_exist(bios_path) != 1)
      {
         log_cb(RETRO_LOG_ERROR, "This is a Saturn game but we are missing the bios, ABORTING\n");
         return false;
      }
      if (hle_bios_force)
      {
         log_cb(RETRO_LOG_WARN, "HLE bios is enabled, this is for debugging purpose only, expect lots of issues\n");
      }

      if (use_beetle_saves == 1)
         snprintf(bup_path, sizeof(bup_path), "%s%c%s.bkr", g_save_dir, slash, game_basename);
      else
         snprintf(bup_path, sizeof(bup_path), "%s%ckronos%csaturn%c%s.ram", g_save_dir, slash, slash, slash, game_basename);

      // Configure addon cart settings
      configure_saturn_addon_cart();

      yinit.cdcoretype       = CDCORE_ISO;
      yinit.cdpath           = full_path;
      yinit.biospath         = (hle_bios_force ? NULL : bios_path);
      yinit.carttype         = addon_cart_type;
      yinit.cartpath         = addon_cart_path;
      yinit.supportdir       = g_system_dir;
      yinit.auto_cart_select = auto_select_cart;
   }

   return retro_load_game_common();
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   return false;
}

void retro_unload_game(void)
{
   YabauseDeInit();
}

unsigned retro_get_region(void)
{
   return yabsys.IsPal == 1 ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void *retro_get_memory_data(unsigned id)
{
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   return 0;
}

void retro_deinit(void)
{
}

void retro_reset(void)
{
   YabauseResetNoLoad();
}

void retro_run(void)
{
   unsigned i;
   bool updated  = false;
   if (!all_devices_ready)
   {
      // Running first frame, so we can assume all devices id were set
      // Let's send input descriptors
      all_devices_ready = true;
      set_descriptors();
   }

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
   {
      int prev_resolution_mode = resolution_mode;
      int prev_multitap[2] = {multitap[0],multitap[1]};
      check_variables();
      if(prev_resolution_mode != resolution_mode)
         retro_set_resolution();
      if(PERCore && (prev_multitap[0] != multitap[0] || prev_multitap[1] != multitap[1]))
         PERCore->Init();
      VIDCore->SetSettingValue(VDP_SETTING_FILTERMODE, filter_mode);
      VIDCore->SetSettingValue(VDP_SETTING_POLYGON_MODE, polygon_mode);
      VIDCore->SetSettingValue(VDP_SETTING_UPSCALMODE, upscale_mode);
      VIDCore->SetSettingValue(VDP_SETTING_SCANLINE, scanlines);
      YabauseSetVideoFormat(g_videoformattype);
   }

   YabauseExec();
}

#ifdef ANDROID
#include <wchar.h>

size_t mbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
   if (!pwcs)
      return strlen(s);
   return mbsrtowcs(pwcs, &s, n, NULL);
}

size_t wcstombs(char *s, const wchar_t *pwcs, size_t n)
{
   return wcsrtombs(s, &pwcs, n, NULL);
}

#endif
