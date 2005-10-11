/*  Copyright 2005 Theo Berkau

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdlib.h>

#include "SDL.h"
#include "scsp.h"
#include "sndsdl.h"

int SNDSDLInit();
void SNDSDLDeInit();
int SNDSDLReset();
int SNDSDLChangeVerticalFrequency(int vertfreq);
void SNDSDLUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples);
void SNDSDLMuteAudio();
void SNDSDLUnMuteAudio();

SoundInterface_struct SNDSDL = {
SNDCORE_SDL,
"SDL Sound Interface",
SNDSDLInit,
SNDSDLDeInit,
SNDSDLReset,
SNDSDLChangeVerticalFrequency,
SNDSDLUpdateAudio,
SNDSDLMuteAudio,
SNDSDLUnMuteAudio
};

#define NUMSOUNDBLOCKS  2

static u16 *stereodata16;
static u32 soundpos;
static u32 soundblknum;
static u32 soundlen;
static u32 soundbufsize;
static SDL_AudioSpec audiofmt;

//////////////////////////////////////////////////////////////////////////////

void MixAudio(void *userdata, Uint8 *stream, int len) {
   int i;
   Uint8 *soundbuf=(Uint8 *)stereodata16;

   for (i = 0; i < len; i++)
   {
      if (soundpos >= soundbufsize)
         soundpos = 0;

      stream[i] = soundbuf[soundpos];
      soundpos++;
   }
}

//////////////////////////////////////////////////////////////////////////////

int SNDSDLInit()
{
   SDL_InitSubSystem(SDL_INIT_AUDIO);
//   if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0);
//      return -1;

   audiofmt.freq = 44100;
   audiofmt.format = AUDIO_S16SYS;
   audiofmt.channels = 2;
   audiofmt.samples = (audiofmt.freq / 60) * 2;
   audiofmt.callback = MixAudio;
   audiofmt.userdata = NULL;

   soundlen = audiofmt.freq / 60; // 60 for NTSC or 50 for PAL. Initially assume it's going to be NTSC.
   soundbufsize = soundlen * NUMSOUNDBLOCKS * 2 * 2;

   if (SDL_OpenAudio(&audiofmt, NULL) != 0)
      return -1;

   if ((stereodata16 = (u16 *)malloc(soundbufsize)) == NULL)
      return -1;

   memset(stereodata16, 0, soundbufsize);

   soundpos = 0;
   soundblknum = 0;

   SDL_PauseAudio(0);

//  debugfp = fopen("scsp.raw", "wb");
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDSDLDeInit()
{
   SDL_CloseAudio();

   if (stereodata16)
      free(stereodata16);
}

//////////////////////////////////////////////////////////////////////////////

int SNDSDLReset()
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int SNDSDLChangeVerticalFrequency(int vertfreq)
{
   soundlen = audiofmt.freq / vertfreq;
   soundbufsize = soundlen * NUMSOUNDBLOCKS * 2 * 2;

   if ((stereodata16 = (u16 *)realloc(stereodata16, soundbufsize)) == NULL)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDSDLUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples)
{
   SDL_LockAudio();
   // check to see if we're falling behind/getting ahead

   ScspConvert32uto16s((s32 *)leftchanbuffer, (s32 *)rightchanbuffer, ((s16 *)stereodata16 + (soundblknum * soundlen * 2)), num_samples);

   soundblknum++;
   soundblknum &= (NUMSOUNDBLOCKS - 1);

//   if (debugfp) fwrite((void *)stereodata16, 1, SOUNDBLOCKSIZE * 2 * NUMSOUNDBLOCKS, debugfp);

   SDL_UnlockAudio();
}

//////////////////////////////////////////////////////////////////////////////

void SNDSDLMuteAudio()
{
   SDL_PauseAudio(1);
}

//////////////////////////////////////////////////////////////////////////////

void SNDSDLUnMuteAudio()
{
   SDL_PauseAudio(0);
}

//////////////////////////////////////////////////////////////////////////////

