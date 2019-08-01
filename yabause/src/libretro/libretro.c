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
#include "yabause.h"
#include "yui.h"
#include "cheat.h"

#include "cs0.h"
#include "cs2.h"

#include "m68kcore.h"
#include "vidogl.h"
#include "vidsoft.h"
#include "ygl.h"

yabauseinit_struct yinit;

static char slash = path_default_slash_c();

static char g_save_dir[PATH_MAX];
static char g_system_dir[PATH_MAX];
static char full_path[PATH_MAX];
static char bios_path[PATH_MAX];
static char bup_path[PATH_MAX];

static int game_width  = 320;
static int game_height = 240;
static int game_interlace;

static int current_width;
static int current_height;

static bool renderer_running = false;
static bool hle_bios_force = false;
static bool one_frame_rendered = false;

static bool libretro_supports_bitmasks = false;
static int16_t libretro_input_bitmask[12] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
 
#ifdef DYNAREC_DEVMIYAX
static int g_sh2coretype = 3;
#else
static int g_sh2coretype = SH2CORE_INTERPRETER;
#endif

static int g_frame_skip = 1;
static int g_rbg_resolution_mode = 0;
static int g_rbg_use_compute_shader = 1;
static int addon_cart_type = CART_DRAM32MBIT;
static int resolution_mode = 1;
static int initial_resolution_mode = 0;
static int g_resolution_mode = 0;
#ifdef LOW_END
static int max_resolution_mode = 2;
#else
static int max_resolution_mode = 4;
#endif
static int polygon_mode = PERSPECTIVE_CORRECTION;
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
      { "yabasanshiro_force_hle_bios", "Force HLE BIOS (restart, deprecated, debug only); disabled|enabled" },
      { "yabasanshiro_frameskip", "Auto-frameskip (prevent fast-forwarding); enabled|disabled" },
      { "yabasanshiro_addon_cart", "Addon Cartridge (restart); 4M_extended_ram|1M_extended_ram" },
      { "yabasanshiro_multitap_port1", "6Player Adaptor on Port 1; disabled|enabled" },
      { "yabasanshiro_multitap_port2", "6Player Adaptor on Port 2; disabled|enabled" },
#ifdef DYNAREC_DEVMIYAX
      { "yabasanshiro_sh2coretype", "SH2 Core (restart); dynarec|interpreter" },
#endif
#ifdef ALLOW_POLYGON_MODE
      { "yabasanshiro_polygon_mode", "Polygon Mode; perspective_correction|gpu_tesselation|cpu_tesselation" },
#endif
#ifndef LOW_END
      { "yabasanshiro_resolution_mode", "Resolution Mode; original|2x|4x" },
#else
      { "yabasanshiro_resolution_mode", "Resolution Mode; original|2x" },
#endif
      { "yabasanshiro_rbg_resolution_mode", "RGB resolution mode; original|2x|720p|1080p" },
      { "yabasanshiro_rbg_use_compute_shader", "RGB use compute shader for RGB; enabled|disabled" },
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

static int input_state_cb_wrapper(unsigned port, unsigned device, unsigned index, unsigned id)
{
   if (libretro_supports_bitmasks && device == RETRO_DEVICE_JOYPAD)
   {
      if (libretro_input_bitmask[port] == -1)
         libretro_input_bitmask[port] = input_state_cb(port, RETRO_DEVICE_JOYPAD, index, RETRO_DEVICE_ID_JOYPAD_MASK);
      return (libretro_input_bitmask[port] & (1 << id));
   }
   else
      return input_state_cb(port, device, index, id);
}

static int PERLIBRETROHandleEvents(void)
{
   unsigned i = 0;

   input_poll_cb();

   for(i = 0; i < players; i++)
   {
         int analog_left_x = 0;
         int analog_left_y = 0;
         int analog_right_x = 0;
         int analog_right_y = 0;
         uint16_t l_trigger, r_trigger;
         libretro_input_bitmask[i] = -1;

         switch(pad_type[i])
         {
            case RETRO_DEVICE_ANALOG:
               analog_left_x = input_state_cb_wrapper(i, RETRO_DEVICE_ANALOG,
                     RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);

               PerAxisValue((i << 8) + PERANALOG_AXIS1, (u8)((analog_left_x + 0x8000) >> 8));

               analog_left_y = input_state_cb_wrapper(i, RETRO_DEVICE_ANALOG,
                     RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);

               PerAxisValue((i << 8) + PERANALOG_AXIS2, (u8)((analog_left_y + 0x8000) >> 8));

               // analog triggers
               l_trigger = input_state_cb_wrapper( i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_BUTTON, RETRO_DEVICE_ID_JOYPAD_L2 );
               r_trigger = input_state_cb_wrapper( i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_BUTTON, RETRO_DEVICE_ID_JOYPAD_R2 );

               // if no analog trigger support, use digital
               if (l_trigger == 0)
                  l_trigger = input_state_cb_wrapper( i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2 ) ? 0x7FFF : 0;
               if (r_trigger == 0)
                  r_trigger = input_state_cb_wrapper( i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2 ) ? 0x7FFF : 0;

               PerAxisValue((i << 8) + PERANALOG_AXIS3, (u8)((r_trigger > 0 ? r_trigger + 0x8000 : 0) >> 8));
               PerAxisValue((i << 8) + PERANALOG_AXIS4, (u8)((l_trigger > 0 ? l_trigger + 0x8000 : 0) >> 8));

            case RETRO_DEVICE_JOYPAD:

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
                  PerKeyDown((i << 8) + PERPAD_UP);
               else
                  PerKeyUp((i << 8) + PERPAD_UP);
               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
                  PerKeyDown((i << 8) + PERPAD_DOWN);
               else
                  PerKeyUp((i << 8) + PERPAD_DOWN);
               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
                  PerKeyDown((i << 8) + PERPAD_LEFT);
               else
                  PerKeyUp((i << 8) + PERPAD_LEFT);
               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
                  PerKeyDown((i << 8) + PERPAD_RIGHT);
               else
                  PerKeyUp((i << 8) + PERPAD_RIGHT);

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y))
                  PerKeyDown((i << 8) + PERPAD_X);
               else
                  PerKeyUp((i << 8) + PERPAD_X);

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B))
                  PerKeyDown((i << 8) + PERPAD_A);
               else
                  PerKeyUp((i << 8) + PERPAD_A);

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A))
                  PerKeyDown((i << 8) + PERPAD_B);
               else
                  PerKeyUp((i << 8) + PERPAD_B);

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X))
                  PerKeyDown((i << 8) + PERPAD_Y);
               else
                  PerKeyUp((i << 8) + PERPAD_Y);

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L))
                  PerKeyDown((i << 8) + PERPAD_C);
               else
                  PerKeyUp((i << 8) + PERPAD_C);

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R))
                  PerKeyDown((i << 8) + PERPAD_Z);
               else
                  PerKeyUp((i << 8) + PERPAD_Z);

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
                  PerKeyDown((i << 8) + PERPAD_START);
               else
                  PerKeyUp((i << 8) + PERPAD_START);

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2))
                  PerKeyDown((i << 8) + PERPAD_LEFT_TRIGGER);
               else
                  PerKeyUp((i << 8) + PERPAD_LEFT_TRIGGER);

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2))
                  PerKeyDown((i << 8) + PERPAD_RIGHT_TRIGGER);
               else
                  PerKeyUp((i << 8) + PERPAD_RIGHT_TRIGGER);
               break;

            default:
               break;
         }
   }

   if ( YabauseExec() != 0 )
      return -1;
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
#ifdef DYNAREC_DEVMIYAX
    &SH2Dyn,
#endif
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
   // Downscale resolution_mode for Hi-Res games
   if (game_height > 256 && resolution_mode > max_resolution_mode/2)
   {
      log_cb(RETRO_LOG_INFO, "Halving Hi-Res games resolution mode\n", resolution_mode);
      resolution_mode = max_resolution_mode/2;
   }
   switch(resolution_mode)
   {
      case 1:
         g_resolution_mode = 3;
         break;
      case 2:
         g_resolution_mode = 2;
         break;
      case 4:
         g_resolution_mode = 1;
         break;
   }
   current_width = game_width * resolution_mode;
   current_height = game_height * resolution_mode;
   VIDCore->Resize(0, 0, current_width, current_height, 0);
   retro_reinit_av_info();
   VIDCore->SetSettingValue(VDP_SETTING_RESOLUTION_MODE, g_resolution_mode);
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
   one_frame_rendered = true;
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
      renderer_running = true;
      retro_set_resolution();
      OSDChangeCore(OSDCORE_DUMMY);
   }
   else
   {
      if (!renderer_running)
         VIDCore->Init();
      renderer_running = true;
      retro_set_resolution();
   }
}

static void context_destroy(void)
{
   if (renderer_running)
      VIDCore->DeInit();
   renderer_running = false;
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
#ifdef _OGLES3_
   hw_render.context_type = RETRO_HW_CONTEXT_OPENGLES3;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render))
      return false;
#else
   hw_render.context_type = RETRO_HW_CONTEXT_OPENGL;
   if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render))
       return false;
#endif
#else
   glsm_ctx_params_t params = {0};
   params.context_reset = context_reset;
   params.context_destroy = context_destroy;
   params.environ_cb = environ_cb;
   params.stencil = true;
#ifdef _OGLES3_
   params.context_type = RETRO_HW_CONTEXT_OPENGLES_VERSION;
   params.major = 3;
   params.minor = 1;
   if (!glsm_ctl(GLSM_CTL_STATE_CONTEXT_INIT, &params))
      return false;
#else
   params.context_type = RETRO_HW_CONTEXT_OPENGL;
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
   info->library_name     = "YabaSanshiro";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "v" VERSION GIT_VERSION;
   info->need_fullpath    = true;
   info->block_extract    = false;
   info->valid_extensions = "cue|iso|mds|ccd";
}

void check_variables(void)
{
   struct retro_variable var;

   var.key = "yabasanshiro_force_hle_bios";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0 && hle_bios_force)
         hle_bios_force = false;
      else if (strcmp(var.value, "enabled") == 0 && !hle_bios_force)
         hle_bios_force = true;
   }

   var.key = "yabasanshiro_frameskip";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         g_frame_skip = 1;
      else if (strcmp(var.value, "disabled") == 0)
         g_frame_skip = 0;
   }

   var.key = "yabasanshiro_rbg_resolution_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "original") == 0)
      {
         g_rbg_resolution_mode = 0;
      }
      else if (strcmp(var.value, "2x") == 0)
      {
         g_rbg_resolution_mode = 1;
      }
      else if (strcmp(var.value, "720p") == 0)
      {
         g_rbg_resolution_mode = 2;
      }
      else if (strcmp(var.value, "1080p") == 0)
      {
         g_rbg_resolution_mode = 3;
      }
   }

   var.key = "yabasanshiro_rbg_use_compute_shader";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "enabled") == 0)
         g_rbg_use_compute_shader = 1;
      else if (strcmp(var.value, "disabled") == 0)
         g_rbg_use_compute_shader = 0;
   }

#ifdef DYNAREC_DEVMIYAX
   var.key = "yabasanshiro_sh2coretype";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "dynarec") == 0)
         g_sh2coretype = 3;
      else if (strcmp(var.value, "interpreter") == 0)
         g_sh2coretype = SH2CORE_INTERPRETER;
   }
#endif

   var.key = "yabasanshiro_addon_cart";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "none") == 0 && addon_cart_type != CART_NONE)
         addon_cart_type = CART_NONE;
      else if (strcmp(var.value, "1M_extended_ram") == 0 && addon_cart_type != CART_DRAM8MBIT)
         addon_cart_type = CART_DRAM8MBIT;
      else if (strcmp(var.value, "4M_extended_ram") == 0 && addon_cart_type != CART_DRAM32MBIT)
         addon_cart_type = CART_DRAM32MBIT;
   }

   var.key = "yabasanshiro_multitap_port1";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         multitap[0] = 0;
      else if (strcmp(var.value, "enabled") == 0)
         multitap[0] = 1;
   }

   var.key = "yabasanshiro_multitap_port2";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         multitap[1] = 0;
      else if (strcmp(var.value, "enabled") == 0)
         multitap[1] = 1;
   }

   var.key = "yabasanshiro_resolution_mode";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "original") == 0)
      {
         resolution_mode = 1;
         g_resolution_mode = 3;
      }
      else if (strcmp(var.value, "2x") == 0)
      {
         resolution_mode = 2;
         g_resolution_mode = 2;
      }
#ifndef LOW_END
      else if (strcmp(var.value, "4x") == 0)
      {
         resolution_mode = 4;
         g_resolution_mode = 1;
      }
#endif
   }

#ifdef ALLOW_POLYGON_MODE
   var.key = "yabasanshiro_polygon_mode";
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
#endif
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
   info->geometry.max_width    = 704 * (initial_resolution_mode == max_resolution_mode ? max_resolution_mode/2 : initial_resolution_mode);
   info->geometry.max_height   = 512 * (initial_resolution_mode == max_resolution_mode ? max_resolution_mode/2 : initial_resolution_mode);
   info->geometry.aspect_ratio = (retro_get_region() == RETRO_REGION_NTSC) ? 4.0 / 3.0 : 5.0 / 4.0;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   if(pad_type[port] != device)
   {
      pad_type[port] = device;
      if(PERCore)
         PERCore->Init();
   }
}

size_t retro_serialize_size(void)
{
   // Disabling savestates until they are safe
   return 0;
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
   // Disabling savestates until they are safe
   return true;
   void *buffer;
   size_t out_size;

   int error = YabSaveStateBuffer (&buffer, &out_size);

   memcpy(data, buffer, size);

   free(buffer);
   return !error;
}

bool retro_unserialize(const void *data, size_t size)
{
   // Disabling savestates until they are safe
   return true;
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
   snprintf(save_dir, sizeof(save_dir), "%s%cyabasanshiro%c", g_save_dir, slash, slash);
   path_mkdir(save_dir);

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

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

   yinit.vidcoretype               = VIDCORE_OGL;
   yinit.percoretype               = PERCORE_LIBRETRO;
   yinit.sh2coretype               = g_sh2coretype;
   yinit.sndcoretype               = SNDCORE_LIBRETRO;
#ifdef HAVE_MUSASHI
   yinit.m68kcoretype              = M68KCORE_MUSASHI;
#else
   yinit.m68kcoretype              = M68KCORE_C68K;
#endif
   yinit.mpegpath                  = NULL;
   yinit.frameskip                 = g_frame_skip;
   yinit.rbg_resolution_mode       = g_rbg_resolution_mode;
   yinit.rbg_use_compute_shader    = g_rbg_use_compute_shader;
   yinit.usethreads                = 0;
   yinit.rotate_screen             = 0;
   yinit.skip_load                 = 0;
   yinit.polygon_generation_mode   = polygon_mode;
   yinit.extend_backup             = 0;
   yinit.buppath                   = bup_path;
   yinit.use_new_scsp              = 1;
   yinit.scsp_sync_count_per_frame = 1;
   yinit.extend_backup             = 1;
   yinit.scsp_main_mode            = 1;
   yinit.videoformattype           = VIDEOFORMATTYPE_NTSC;
   yinit.video_filter_type         = 0;

   return true;
}

bool retro_load_game(const struct retro_game_info *info)
{
   if (!info)
      return false;

   check_variables();

   snprintf(full_path, sizeof(full_path), "%s", info->path);
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

   // Real bios is REQUIRED, even if we support HLE bios
   // HLE bios is deprecated and causing more issues than it solves
   // No "autoselect HLE when bios is missing" ever again !
   if (does_file_exist(bios_path) != 1)
   {
      log_cb(RETRO_LOG_ERROR, "We are missing the bios, ABORTING\n");
      return false;
   }
   if (hle_bios_force)
   {
      log_cb(RETRO_LOG_WARN, "HLE bios is enabled, this is for debugging purpose only, expect lots of issues\n");
   }

   snprintf(bup_path, sizeof(bup_path), "%s%cyabasanshiro%cbackup.bin", g_save_dir, slash, slash);

   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 2, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 2, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 2, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 2, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 3, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 3, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 3, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 3, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 4, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 4, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 4, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 4, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 4, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 5, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 5, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 5, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 5, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 5, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 6, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 6, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 6, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 6, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 6, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 7, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 7, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 7, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 7, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 7, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 8, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 8, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 8, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 8, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 8, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 9, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 9, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 9, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 9, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 9, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 10, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 10, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 10, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 10, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 10, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "C" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Z" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" },
      { 11, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" },
      { 11, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Analog X" },
      { 11, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y" },
      { 11, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" },
      { 11, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" }, 

      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   yinit.cdcoretype       = CDCORE_ISO;
   yinit.cdpath           = full_path;
   yinit.biospath         = (hle_bios_force ? NULL : bios_path);
   yinit.carttype         = addon_cart_type;
   yinit.cartpath         = "\0";

   return retro_load_game_common();
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   return false;
}

void retro_unload_game(void)
{
   if (!renderer_running)
      VIDCore->Init();
   YabauseDeInit();
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
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
   libretro_supports_bitmasks = false;
}

void retro_reset(void)
{
   YabauseReset();
   VdpResume();
   // The following function crashes the core when you use "restart"
   //YabauseResetButton();
}

void reset_global_gl_state()
{
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
}

void retro_run(void)
{
   unsigned i;
   bool updated  = false;
   one_frame_rendered = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
   {
      int prev_resolution_mode = resolution_mode;
      int prev_multitap[2] = {multitap[0],multitap[1]};
      check_variables();
      if(prev_resolution_mode != resolution_mode)
         retro_set_resolution();
      // Unlike Kronos, this core dislike changing tesselation on the fly
      //VIDCore->SetSettingValue(VDP_SETTING_POLYGON_MODE, polygon_mode);
      VIDCore->SetSettingValue(VDP_SETTING_RBG_RESOLUTION_MODE, g_rbg_resolution_mode);
      VIDCore->SetSettingValue(VDP_SETTING_RBG_USE_COMPUTESHADER, g_rbg_use_compute_shader);
      if(PERCore && (prev_multitap[0] != multitap[0] || prev_multitap[1] != multitap[1]))
         PERCore->Init();
      if(g_frame_skip == 1)
         EnableAutoFrameSkip();
      else
         DisableAutoFrameSkip();
   }

   //YabauseExec(); runs from handle events
   if(PERCore)
      PERCore->HandleEvents();

   // If no frame rendered, dupe
   if(!one_frame_rendered)
      video_cb(NULL, current_width, current_height, 0);

   reset_global_gl_state();
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
