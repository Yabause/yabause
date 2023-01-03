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
#include <retro_miscellaneous.h>

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
#include "libretro_core_options.h"
#ifdef HAVE_PLAY_JIT
#include "sh2_jit.h"
#endif

yabauseinit_struct yinit;

static char slash = PATH_DEFAULT_SLASH_C();

static char g_roms_dir[PATH_MAX];
static char g_save_dir[PATH_MAX_LENGTH];
static char g_system_dir[PATH_MAX_LENGTH];
static char bios_path[PATH_MAX_LENGTH];
static char bup_path[PATH_MAX_LENGTH];

static char game_basename[PATH_MAX_LENGTH];

int game_width;
int game_height;

static bool one_frame_rendered = false;
static bool hle_bios_force = false;
static bool frameskip_enable = false;
static int addon_cart_type = CART_NONE;
static int numthreads = 1;

static bool libretro_supports_bitmasks = false;
static int16_t libretro_input_bitmask[12] = {-1,};
static int pad_type[12] = {RETRO_DEVICE_NONE,};
static int multitap[2] = {0,0};
static unsigned players = 7;
static bool all_devices_ready = false;

// disk swapping
#define M3U_MAX_FILE 5
static char disk_paths[M3U_MAX_FILE][PATH_MAX];
static char disk_labels[M3U_MAX_FILE][PATH_MAX];
static struct retro_disk_control_callback retro_disk_control_cb;
static struct retro_disk_control_ext_callback retro_disk_control_ext_cb;
static unsigned disk_initial_index = 0;
static char disk_initial_path[PATH_MAX];
static unsigned disk_index = 0;
static unsigned disk_total = 0;
static bool disk_tray_open = false;

static bool libretro_supports_option_categories = false;

struct retro_perf_callback perf_cb;
retro_get_cpu_features_t perf_get_cpu_features_cb = NULL;

retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_batch_t audio_batch_cb;

#define RETRO_DEVICE_3DPAD RETRO_DEVICE_ANALOG
#define RETRO_DEVICE_WHEEL RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 0)

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   libretro_set_core_options(environ_cb,
         &libretro_supports_option_categories);

   static const struct retro_controller_description peripherals[] = {
       { "Saturn Pad", RETRO_DEVICE_JOYPAD },
       { "Saturn 3D Pad", RETRO_DEVICE_3DPAD },
       { "Saturn Wheel", RETRO_DEVICE_WHEEL },
       { "Saturn Mouse", RETRO_DEVICE_MOUSE },
       { "None", RETRO_DEVICE_NONE },
   };

   static const struct retro_controller_info ports[] = {
      { peripherals, 5 },
      { peripherals, 5 },
      { peripherals, 5 },
      { peripherals, 5 },
      { peripherals, 5 },
      { peripherals, 5 },
      { peripherals, 5 },
      { peripherals, 5 },
      { peripherals, 5 },
      { peripherals, 5 },
      { peripherals, 5 },
      { peripherals, 5 },
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

static PerMouse_struct* mousebits = NULL;

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
      portdata = ((players > 2 && i < 6) || i == 0 ? &PORTDATA1 : &PORTDATA2);

      switch(pad_type[i])
      {
         case RETRO_DEVICE_NONE:
            controller = NULL;
            break;
         case RETRO_DEVICE_3DPAD:
            controller = (void*)Per3DPadAdd(portdata);
            for(j = PERPAD_UP; j <= PERPAD_Z; j++)
               PerSetKey((i << 8) + j, j, controller);
            for(j = PERANALOG_AXIS1; j <= PERANALOG_AXIS7; j++)
               PerSetKey((i << 8) + j, j, controller);
            break;
         case RETRO_DEVICE_WHEEL:
            controller = (void*)PerWheelAdd(portdata);
            for(j = PERPAD_UP; j <= PERPAD_Z; j++)
               PerSetKey((i << 8) + j, j, controller);
            PerSetKey((i << 8) + PERANALOG_AXIS1, PERANALOG_AXIS1, controller);
            break;
         case RETRO_DEVICE_MOUSE:
            mousebits = PerMouseAdd(portdata);
            for(j = PERMOUSE_LEFT; j <= PERMOUSE_START; j++)
               PerSetKey((i << 8) + j, j, mousebits);
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
            case RETRO_DEVICE_3DPAD:

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

            case RETRO_DEVICE_WHEEL:

               analog_left_x = input_state_cb_wrapper(i, RETRO_DEVICE_ANALOG,
                     RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);

               PerAxisValue((i << 8) + PERANALOG_AXIS1, (u8)((analog_left_x + 0x8000) >> 8));

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

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R))
                  PerKeyDown((i << 8) + PERPAD_C);
               else
                  PerKeyUp((i << 8) + PERPAD_C);

               if (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L))
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

            case RETRO_DEVICE_MOUSE:

               (input_state_cb_wrapper(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT) ? PerKeyDown((i << 8) + PERMOUSE_LEFT) : PerKeyUp((i << 8) + PERMOUSE_LEFT));
               (input_state_cb_wrapper(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE) ? PerKeyDown((i << 8) + PERMOUSE_MIDDLE) : PerKeyUp((i << 8) + PERMOUSE_MIDDLE));
               (input_state_cb_wrapper(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT) ? PerKeyDown((i << 8) + PERMOUSE_RIGHT) : PerKeyUp((i << 8) + PERMOUSE_RIGHT));
               // there are some issues with libretro's mouse button 4 & 5 ?
               // let's also use joypad's start for safety
               (input_state_cb_wrapper(i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START) || input_state_cb_wrapper(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_BUTTON_4) || input_state_cb_wrapper(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_BUTTON_5) ? PerKeyDown((i << 8) + PERMOUSE_START) : PerKeyUp((i << 8) + PERMOUSE_START));

               // is saturn supposed to be able to use several mouse ?
               // because i don't think this code is right in that case
               s32 dispx = input_state_cb_wrapper(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
               s32 dispy = input_state_cb_wrapper(i, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
               PerMouseMove(mousebits, dispx, -dispy);
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
   info->valid_extensions = "cue|iso|mds|ccd|zip|chd|m3u";
}

static void set_descriptors(void)
{
   struct retro_input_descriptor *input_descriptors = (struct retro_input_descriptor*)calloc(((17*players)+1), sizeof(struct retro_input_descriptor));

   unsigned j = 0;
   for (unsigned i = 0; i < players; i++)
   {
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "C" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "X" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Y" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Z" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "L" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "R" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,  "Analog X" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,  "Analog Y" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Analog X (Right)" };
      input_descriptors[j++] = (struct retro_input_descriptor){ i, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Analog Y (Right)" };
   }
   input_descriptors[j].description = NULL;

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, input_descriptors);
   free(input_descriptors);
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
      // When all devices are set, we can send input descriptors
      if (all_devices_ready)
         set_descriptors();
   }
}

size_t retro_serialize_size(void)
{
   size_t size;

   ScspMuteAudio(SCSP_MUTE_SYSTEM);
   YabSaveStateBuffer(NULL, &size);
   ScspUnMuteAudio(SCSP_MUTE_SYSTEM);

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

   var.key = "yabause_addon_cartridge";
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

static void extract_directory(char *buf, const char *path, size_t size)
{
   strncpy(buf, path, size - 1);
   buf[size - 1] = '\0';

   char *base = strrchr(buf, slash);

   if (base)
      *base = '\0';
   else
   {
      buf[0] = '.';
      buf[1] = '\0';
   }
}

static bool disk_set_eject_state(bool ejected)
{
   disk_tray_open = ejected;
   if (ejected)
   {
      Cs2ForceOpenTray();
      return true;
   }
   else
   {
      return (Cs2ForceCloseTray(CDCORE_ISO, disk_paths[disk_index]) == 0);
   }
}

static bool disk_get_eject_state()
{
   return disk_tray_open;
}

static bool disk_set_image_index(unsigned index)
{
   if ( disk_tray_open == true )
   {
      if ( index < disk_total ) {
         disk_index = index;
         return true;
      }
   }

   return false;
}

static unsigned disk_get_image_index()
{
   return disk_index;
}

static unsigned disk_get_num_images(void)
{
   return disk_total;
}

static bool disk_add_image_index(void)
{
   if (disk_total >= M3U_MAX_FILE)
      return false;
   disk_total++;
   disk_paths[disk_total-1][0] = '\0';
   disk_labels[disk_total-1][0] = '\0';
   return true;
}

static bool disk_replace_image_index(unsigned index, const struct retro_game_info *info)
{
   if ((index >= disk_total))
      return false;

   if (!info)
   {
      disk_paths[index][0] = '\0';
      disk_labels[index][0] = '\0';
      disk_total--;

      if ((disk_index >= index) && (disk_index > 0))
         disk_index--;
   }
   else
   {
      snprintf(disk_paths[index], sizeof(disk_paths[index]), "%s", info->path);
      fill_pathname(disk_labels[index], path_basename(disk_paths[index]), "", sizeof(disk_labels[index]));
   }

   return true;
}

static bool disk_set_initial_image(unsigned index, const char *path)
{
   if (!path || (*path == '\0'))
      return false;

   disk_initial_index = index;
   snprintf(disk_initial_path, sizeof(disk_initial_path), "%s", path);

   return true;
}

static bool disk_get_image_path(unsigned index, char *path, size_t len)
{
   if (len < 1)
      return false;

   if (index >= disk_total)
      return false;

   if (disk_paths[index] == NULL || disk_paths[index][0] == '\0')
      return false;

   strncpy(path, disk_paths[index], len - 1);
   path[len - 1] = '\0';

   return true;
}

static bool disk_get_image_label(unsigned index, char *label, size_t len)
{
   if (len < 1)
      return false;

   if (index >= disk_total)
      return false;

   if (disk_labels[index] == NULL || disk_labels[index][0] == '\0')
      return false;

   strncpy(label, disk_labels[index], len - 1);
   label[len - 1] = '\0';

   return true;
}

static void init_disk_control_interface(void)
{
   unsigned dci_version = 0;

   retro_disk_control_cb.set_eject_state     = disk_set_eject_state;
   retro_disk_control_cb.get_eject_state     = disk_get_eject_state;
   retro_disk_control_cb.set_image_index     = disk_set_image_index;
   retro_disk_control_cb.get_image_index     = disk_get_image_index;
   retro_disk_control_cb.get_num_images      = disk_get_num_images;
   retro_disk_control_cb.add_image_index     = disk_add_image_index;
   retro_disk_control_cb.replace_image_index = disk_replace_image_index;

   retro_disk_control_ext_cb.set_eject_state     = disk_set_eject_state;
   retro_disk_control_ext_cb.get_eject_state     = disk_get_eject_state;
   retro_disk_control_ext_cb.set_image_index     = disk_set_image_index;
   retro_disk_control_ext_cb.get_image_index     = disk_get_image_index;
   retro_disk_control_ext_cb.get_num_images      = disk_get_num_images;
   retro_disk_control_ext_cb.add_image_index     = disk_add_image_index;
   retro_disk_control_ext_cb.replace_image_index = disk_replace_image_index;
   retro_disk_control_ext_cb.set_initial_image   = disk_set_initial_image;
   retro_disk_control_ext_cb.get_image_path      = disk_get_image_path;
   retro_disk_control_ext_cb.get_image_label     = disk_get_image_label;

   disk_initial_index = 0;
   disk_initial_path[0] = '\0';
   if (environ_cb(RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION, &dci_version) && (dci_version >= 1))
      environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE, &retro_disk_control_ext_cb);
   else
      environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, &retro_disk_control_cb);
}

static bool read_m3u(const char *file)
{
   char line[PATH_MAX];
   char name[PATH_MAX];
   FILE *f = fopen(file, "r");

   disk_total = 0;

   if (!f)
   {
      log_cb(RETRO_LOG_ERROR, "Could not read file\n");
      return false;
   }

   while (fgets(line, sizeof(line), f) && disk_total <= M3U_MAX_FILE)
   {
      if (line[0] == '#')
         continue;

      char *carriage_return = strchr(line, '\r');
      if (carriage_return)
         *carriage_return = '\0';

      char *newline = strchr(line, '\n');
      if (newline)
         *newline = '\0';

      if (line[0] == '"')
         memmove(line, line + 1, strlen(line));

      if (line[strlen(line) - 1] == '"')
         line[strlen(line) - 1]  = '\0';

      if (line[0] != '\0')
      {
         snprintf(disk_paths[disk_total], sizeof(disk_paths[disk_total]), "%s%c%s", g_roms_dir, slash, line);
         fill_pathname(disk_labels[disk_total], path_basename(disk_paths[disk_total]), "", sizeof(disk_labels[disk_total]));
         disk_total++;
      }
   }

   fclose(f);
   return (disk_total != 0);
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

   init_disk_control_interface();
}

bool retro_load_game(const struct retro_game_info *info)
{
   int ret;

   if (!info)
      return false;

#ifdef USE_RGB_565
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
#else
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_0RGB1555;
#endif
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
      return false;

   check_variables();

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
   extract_directory(g_roms_dir, info->path, sizeof(g_roms_dir));

   snprintf(bup_path, sizeof(bup_path), "%s%c%s.srm", g_save_dir, slash, game_basename);

   if (strcmp(path_get_extension(info->path), "m3u") == 0)
   {
      if (!read_m3u(info->path))
      {
         log_cb(RETRO_LOG_ERROR, "Aborting, this m3u file is invalid\n");
         return false;
      }
      else if (bios_path[0] == '\0' && does_file_exist(bios_path) != 1)
      {
         log_cb(RETRO_LOG_ERROR, "Aborting, you don't have a real bios, but it is required for m3u support\n");
         return false;
      }
      else
      {
         disk_index = 0;
         // saturn requires real bios to swap discs
         if (hle_bios_force)
         {
            log_cb(RETRO_LOG_WARN, "Forcing real bios, it is required for m3u support\n");
            hle_bios_force = false;
         }

         if ((disk_total > 1) && (disk_initial_index > 0) && (disk_initial_index < disk_total))
            if (strcmp(disk_paths[disk_initial_index], disk_initial_path) == 0)
               disk_index = disk_initial_index;
      }
   }
   else
   {
      snprintf(disk_paths[disk_total], sizeof(disk_paths[disk_total]), "%s", info->path);
      fill_pathname(disk_labels[disk_total], path_basename(disk_paths[disk_total]), "", sizeof(disk_labels[disk_total]));
      disk_total++;
   }

   yinit.cdcoretype           = CDCORE_ISO;
   yinit.cdpath               = disk_paths[disk_index];
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

   log_cb(RETRO_LOG_DEBUG, "Starting emulation\n");
   ret = YabauseInit(&yinit);
   OSDChangeCore(OSDCORE_DUMMY);
   YabauseSetDecilineMode(1);

   log_cb(RETRO_LOG_DEBUG, "Enabling cheevos\n");
   bool cheevos_supported                  = true;
   struct retro_memory_map mmaps           = {0};
   struct retro_memory_descriptor descs[2] =
   {
      { RETRO_MEMDESC_SYSTEM_RAM, LowWram,  0, 0x0200000, 0, 0, 0x100000, "LowWram"  },
      { RETRO_MEMDESC_SYSTEM_RAM, HighWram, 0, 0x6000000, 0, 0, 0x100000, "HighWram" },
   };

   mmaps.descriptors     = descs;
   mmaps.num_descriptors = sizeof(descs) / sizeof(*descs);

   environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &mmaps);
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &cheevos_supported);

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
   if (!all_devices_ready)
   {
      // Running first frame, so we can assume all devices id were set
      // Let's send input descriptors
      all_devices_ready = true;
      set_descriptors();
   }

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
