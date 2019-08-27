#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include <sys/stat.h>

#include <libretro.h>

#include <file/file_path.h>

#include "vdp1.h"
#include "vdp2.h"
//#include "scsp.h"
#include "peripheral.h"
#include "cdbase.h"
#include "yabause.h"
#include "yui.h"
#include "cheat.h"

//#include "m68kc68k.h"
#include "cs0.h"
#include "cs2.h"

#include "m68kcore.h"
#include "vidogl.h"
#include "vidsoft.h"
#include "libretro_core_options.h"
#ifdef HAVE_PLAY_JIT
#include "sh2_jit.h"
#endif

yabauseinit_struct yinit;

static char slash = path_default_slash_c();

static char g_save_dir[PATH_MAX];
static char g_system_dir[PATH_MAX];
static char full_path[PATH_MAX];
static char bios_path[PATH_MAX];
static char bup_path[PATH_MAX];

static char game_basename[PATH_MAX];

int game_width;
int game_height;

static bool one_frame_rendered = false;
static bool hle_bios_force = false;
static bool frameskip_enable = false;
static int addon_cart_type = CART_NONE;
static int numthreads = 1;

static bool libretro_supports_bitmasks = false;
static int16_t libretro_input_bitmask[12] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static int pad_type[12] = {1,1,1,1,1,1,1,1,1,1,1,1};
static int multitap[2] = {0,0};
static unsigned players = 7;

struct retro_perf_callback perf_cb;
retro_get_cpu_features_t perf_get_cpu_features_cb = NULL;

retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_batch_t audio_batch_cb;

#define RETRO_DEVICE_MTAP_PAD RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)
#define RETRO_DEVICE_MTAP_3D  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 0)

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   libretro_set_core_options(environ_cb);

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
#ifdef HAVE_PLAY_JIT
    &SH2Jit,
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
    &VIDSoft,
    NULL
};

#pragma mark Yabause Callbacks

void YuiErrorMsg(const char *string)
{
   if (log_cb)
      log_cb(RETRO_LOG_ERROR, "Yabause: %s\n", string);
}

void YuiSetVideoAttribute(int type, int val)
{
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "Yabause called back to YuSetVideoAttribute.\n");
}

int YuiSetVideoMode(int width, int height, int bpp, int fullscreen)
{
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "Yabause called, it wants to set width of %d and height of %d.\n", width, height);
    return 0;
}

void YuiSwapBuffers(void)
{
   int current_width  = 320;
   int current_height = 240;

   /* Test if VIDCore valid AND NOT the
    * Dummy Interface (or at least VIDCore->id != 0).
    * Avoid calling GetGlSize if Dummy/ID = 0 is selected
    */
   if (VIDCore && VIDCore->id)
      VIDCore->GetGlSize(&current_width, &current_height);

   game_width  = current_width;
   game_height = current_height;

   audio_size = soundlen;
   video_cb(dispbuffer, game_width, game_height, game_width * 2);
   one_frame_rendered = true;
}

/************************************
 * libretro implementation
 ************************************/

static struct retro_system_av_info g_av_info;

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "Yabause";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "v0.9.15" GIT_VERSION;
   info->need_fullpath    = true;
   info->block_extract    = true;
   info->valid_extensions = "cue|iso|mds|ccd|zip|chd";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));

   info->timing.fps            = (retro_get_region() == RETRO_REGION_NTSC) ? 60 : 50;
   info->timing.sample_rate    = 44100;
   info->geometry.base_width   = game_width;
   info->geometry.base_height  = game_height;
   info->geometry.max_width    = 704;
   info->geometry.max_height   = 512;
   info->geometry.aspect_ratio = 4.0 / 3.0;
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

   ScspMuteAudio(SCSP_MUTE_SYSTEM);
   int error = YabSaveStateBuffer (&buffer, &out_size);
   ScspUnMuteAudio(SCSP_MUTE_SYSTEM);

   memcpy(data, buffer, size);

   free(buffer);

   return !error;
}

bool retro_unserialize(const void *data, size_t size)
{
   ScspMuteAudio(SCSP_MUTE_SYSTEM);
   int error = YabLoadStateBuffer(data, size);
   ScspUnMuteAudio(SCSP_MUTE_SYSTEM);

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

static void check_variables(void)
{
   struct retro_variable var;
   var.key = "yabause_frameskip";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
      {
         DisableAutoFrameSkip();
         frameskip_enable = false;
      }
      else if (strcmp(var.value, "enabled") == 0)
      {
         EnableAutoFrameSkip();
         frameskip_enable = true;
      }
   }

   var.key = "yabause_force_hle_bios";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         hle_bios_force = false;
      else if (strcmp(var.value, "enabled") == 0)
         hle_bios_force = true;
   }

   var.key = "yabause_addon_cart";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "none") == 0)
         addon_cart_type = CART_NONE;
      else if (strcmp(var.value, "1M_ram") == 0)
         addon_cart_type = CART_DRAM8MBIT;
      else if (strcmp(var.value, "4M_ram") == 0)
         addon_cart_type = CART_DRAM32MBIT;
   }

   var.key = "yabause_multitap_port1";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         multitap[0] = 0;
      else if (strcmp(var.value, "enabled") == 0)
         multitap[0] = 1;
   }

   var.key = "yabause_multitap_port2";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "disabled") == 0)
         multitap[1] = 0;
      else if (strcmp(var.value, "enabled") == 0)
         multitap[1] = 1;
   }

   var.key = "yabause_numthreads";
   var.value = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "1") == 0)
         numthreads = 1;
      else if (strcmp(var.value, "2") == 0)
         numthreads = 2;
      else if (strcmp(var.value, "4") == 0)
         numthreads = 4;
      else if (strcmp(var.value, "8") == 0)
         numthreads = 8;
      else if (strcmp(var.value, "16") == 0)
         numthreads = 16;
      else if (strcmp(var.value, "32") == 0)
         numthreads = 32;
   }

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
   game_width               = 320;
   game_height              = 240;
   uint64_t serialization_quirks = RETRO_SERIALIZATION_QUIRK_SINGLE_SESSION;
   /* Performance level for interpreter CPU core is 16 */
   unsigned level           = 16;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;

   if (environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb))
      perf_get_cpu_features_cb = perf_cb.get_cpu_features;

#if 1
   enum retro_pixel_format rgb565 = RETRO_PIXEL_FORMAT_RGB565;
   if(environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565))
      log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 - will use that instead of XRGB1555.\n");
#endif

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
   {
      strncpy(g_system_dir, dir, sizeof(g_system_dir));
   }

   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
   {
      strncpy(g_save_dir, dir, sizeof(g_save_dir));
   }

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);

   environ_cb(RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS, &serialization_quirks);
}

bool retro_load_game(const struct retro_game_info *info)
{
   int ret;
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

      { 0 },
   };

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

   if (bios_path[0] == '\0' || does_file_exist(bios_path) != 1 || hle_bios_force)
   {
      log_cb(RETRO_LOG_WARN, "HLE bios is enabled, you should use a real bios, expect lots of issues otherwise\n");
   }

   extract_basename(game_basename, info->path, sizeof(game_basename));

   snprintf(bup_path, sizeof(bup_path), "%s%c%s.srm", g_save_dir, slash, game_basename);

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   yinit.cdcoretype           = CDCORE_ISO;
   yinit.cdpath               = full_path;
   /* Emulate BIOS */
   yinit.biospath             = (bios_path[0] != '\0' && does_file_exist(bios_path) && !hle_bios_force) ? bios_path : NULL;
   yinit.percoretype          = PERCORE_LIBRETRO;
#ifdef HAVE_PLAY_JIT
   yinit.sh2coretype          = SH2CORE_JIT;
   yinit.use_scu_dsp_jit      = 1;
   yinit.use_scsp_dsp_dynarec = 1;
#else
   yinit.sh2coretype          = SH2CORE_INTERPRETER;
#endif
   yinit.vidcoretype          = VIDCORE_SOFT;
   yinit.sndcoretype          = SNDCORE_LIBRETRO;
#ifdef HAVE_MUSASHI
   yinit.m68kcoretype         = M68KCORE_MUSASHI;
#else
   yinit.m68kcoretype         = M68KCORE_C68K;
#endif
   yinit.carttype             = addon_cart_type;
   yinit.regionid             = REGION_AUTODETECT;
   yinit.buppath              = bup_path;
   yinit.use_new_scsp         = 0;
   yinit.mpegpath             = NULL;
   yinit.videoformattype      = VIDEOFORMATTYPE_NTSC;
   yinit.frameskip            = frameskip_enable;
   yinit.clocksync            = 0;
   yinit.basetime             = 0;
#ifdef HAVE_THREADS
   yinit.usethreads           = 1;
   yinit.numthreads           = numthreads;
#else
   yinit.usethreads           = 0;
#endif

   ret = YabauseInit(&yinit);
   YabauseSetDecilineMode(1);

   return !ret;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   (void)game_type;
   (void)info;
   (void)num_info;
   return false;
}

void retro_unload_game(void)
{
	YabauseDeInit();
}

unsigned retro_get_region(void)
{
   return Cs2GetRegionID() > 6 ? RETRO_REGION_PAL : RETRO_REGION_NTSC;
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
   YabauseResetButton();
   YabauseInit(&yinit);
   YabauseSetDecilineMode(1);
}

void retro_run(void)
{
   unsigned i;
   bool fastforward = false;
   bool updated     = false;
   one_frame_rendered = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_FASTFORWARDING, &fastforward) && fastforward)
   {
      if (frameskip_enable && !fastforward)
         EnableAutoFrameSkip();
      else
         DisableAutoFrameSkip();
   }

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
   {
      int prev_multitap[2] = {multitap[0],multitap[1]};
      check_variables();
      if(PERCore && (prev_multitap[0] != multitap[0] || prev_multitap[1] != multitap[1]))
         PERCore->Init();
   }

   //YabauseExec(); runs from handle events
   if(PERCore)
      PERCore->HandleEvents();

   // If no frame rendered, dupe
   if(!one_frame_rendered)
      video_cb(NULL, game_width, game_height, 0);
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
