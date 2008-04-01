#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include "../scsp.h"
#include "sndwii.h"

#define NUMSOUNDBLOCKS  4
#define SOUNDLEN (44100 / 50)
#define SOUNDBUFSIZE (SOUNDLEN * NUMSOUNDBLOCKS * 2 * 2)

static u32 soundlen=SOUNDLEN;
static u32 soundbufsize=SOUNDBUFSIZE;
static u16 stereodata16[SOUNDBUFSIZE] ATTRIBUTE_ALIGN(32);
static u32 soundoffset=0;
static volatile u32 soundpos;
static int issoundmuted;

//////////////////////////////////////////////////////////////////////////////

static void StartDMA(void)
{
   AUDIO_InitDMA(((u32)stereodata16)+soundpos, soundlen * 4);
   DCFlushRange(((void *)stereodata16)+soundpos, soundlen * 4);
   AUDIO_StartDMA();
   soundpos += (soundlen * 4);
   if (soundpos >= soundbufsize)
      soundpos = 0;
}

//////////////////////////////////////////////////////////////////////////////

int SNDWiiInit()
{
   AUDIO_Init(NULL);
   AUDIO_SetDSPSampleRate(AI_SAMPLERATE_48KHZ);

   soundpos = 0;
   soundlen = 44100 / 60; // 60 for NTSC or 50 for PAL. Initially assume it's going to be NTSC.
   soundbufsize = soundlen * NUMSOUNDBLOCKS * 2 * 2;

   memset(stereodata16, 0, soundbufsize);

   issoundmuted = 0;

   AUDIO_RegisterDMACallback(StartDMA);
   StartDMA();

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDWiiDeInit()
{
}

//////////////////////////////////////////////////////////////////////////////

int SNDWiiReset()
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int SNDWiiChangeVideoFormat(int vertfreq)
{
   soundlen = 44100 / vertfreq;
   soundbufsize = soundlen * NUMSOUNDBLOCKS * 2 * 2;
   memset(stereodata16, 0, soundbufsize);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDWiiUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples)
{
   u32 copy1size=0, copy2size=0;

   if ((soundbufsize - soundoffset) < (num_samples * sizeof(s16) * 2))
   {
      copy1size = (soundbufsize - soundoffset);
      copy2size = (num_samples * sizeof(s16) * 2) - copy1size;
   }
   else
   {
      copy1size = (num_samples * sizeof(s16) * 2);
      copy2size = 0;
   }

   ScspConvert32uto16s((s32 *)leftchanbuffer, (s32 *)rightchanbuffer, (s16 *)(((u8 *)stereodata16)+soundoffset), copy1size / sizeof(s16) / 2);

   if (copy2size)
      ScspConvert32uto16s((s32 *)leftchanbuffer + (copy1size / sizeof(s16) / 2), (s32 *)rightchanbuffer + (copy1size / sizeof(s16) / 2), (s16 *)stereodata16, copy2size / sizeof(s16) / 2);

   soundoffset += copy1size + copy2size;   
   soundoffset %= soundbufsize;
}

//////////////////////////////////////////////////////////////////////////////

u32 SNDWiiGetAudioSpace()
{
   u32 freespace=0;

   if (soundoffset > soundpos)
      freespace = soundbufsize - soundoffset + soundpos;
   else
      freespace = soundpos - soundoffset;

   return (freespace / sizeof(s16) / 2);
}

//////////////////////////////////////////////////////////////////////////////

void SNDWiiMuteAudio()
{
   issoundmuted = 1;
}

//////////////////////////////////////////////////////////////////////////////

void SNDWiiUnMuteAudio()
{
   issoundmuted = 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDWiiSetVolume(int volume)
{
}

//////////////////////////////////////////////////////////////////////////////

SoundInterface_struct SNDWII = {
SNDCORE_WII,
"Wii Sound Interface",
SNDWiiInit,
SNDWiiDeInit,
SNDWiiReset,
SNDWiiChangeVideoFormat,
SNDWiiUpdateAudio,
SNDWiiGetAudioSpace,
SNDWiiMuteAudio,
SNDWiiUnMuteAudio,
SNDWiiSetVolume
};
