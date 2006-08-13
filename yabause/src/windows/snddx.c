/*  Copyright 2005-2006 Theo Berkau

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

#include <dsound.h>
//#include "error.h"
#include "../scsp.h"
#include "snddx.h"

int SNDDXInit();
void SNDDXDeInit();
int SNDDXReset();
int SNDDXChangeVideoFormat(int vertfreq);
void SNDDXUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples);
u32 SNDDXGetAudioSpace();
void SNDDXMuteAudio();
void SNDDXUnMuteAudio();
void SNDDXSetVolume(int volume);

SoundInterface_struct SNDDIRECTX = {
SNDCORE_DIRECTX,
"Direct Sound Interface",
SNDDXInit,
SNDDXDeInit,
SNDDXReset,
SNDDXChangeVideoFormat,
SNDDXUpdateAudio,
SNDDXGetAudioSpace,
SNDDXMuteAudio,
SNDDXUnMuteAudio,
SNDDXSetVolume
};

LPDIRECTSOUND8 lpDS8;
LPDIRECTSOUNDBUFFER lpDSB, lpDSB2;
extern HWND YabWin;

#define NUMSOUNDBLOCKS  4

static u16 *stereodata16;
static u32 soundoffset=0;
static u32 soundlen;
static u32 soundbufsize;
static LONG soundvolume;
static int issoundmuted;

/*
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
*/
//////////////////////////////////////////////////////////////////////////////

int SNDDXInit()
{
   DSBUFFERDESC dsbdesc;
   WAVEFORMATEX wfx;

   if (DirectSoundCreate8(NULL, &lpDS8, NULL) != DS_OK)
   {
      MessageBox (NULL, "DirectSound8Create error", "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   if (IDirectSound8_SetCooperativeLevel(lpDS8, YabWin, DSSCL_PRIORITY) != DS_OK)
   {
      MessageBox (NULL, "IDirectSound8_SetCooperativeLevel error", "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   memset(&dsbdesc, 0, sizeof(dsbdesc));
   dsbdesc.dwSize = sizeof(DSBUFFERDESC);
   dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
   dsbdesc.dwBufferBytes = 0;
   dsbdesc.lpwfxFormat = NULL;

   if (IDirectSound8_CreateSoundBuffer(lpDS8, &dsbdesc, &lpDSB, NULL) != DS_OK)
   {
      MessageBox (NULL, "IDirectSound8_CreateSoundBuffer error", "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   soundlen = 44100 / 60; // 60 for NTSC or 50 for PAL. Initially assume it's going to be NTSC.
   soundbufsize = soundlen * NUMSOUNDBLOCKS * 2 * 2;

   memset(&wfx, 0, sizeof(wfx));
   wfx.wFormatTag = WAVE_FORMAT_PCM;
   wfx.nChannels = 2;
   wfx.nSamplesPerSec = 44100;
   wfx.wBitsPerSample = 16;
   wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
   wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

   if (IDirectSoundBuffer8_SetFormat(lpDSB, &wfx) != DS_OK)
   {
      MessageBox (NULL, "IDirectSoundBuffer8_SetFormat error", "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   memset(&dsbdesc, 0, sizeof(dsbdesc));
   dsbdesc.dwSize = sizeof(DSBUFFERDESC);
   dsbdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_STICKYFOCUS |
                     DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2 |
                     DSBCAPS_LOCHARDWARE;
//                     DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLPOSITIONNOTIFY;
   dsbdesc.dwBufferBytes = soundbufsize;
   dsbdesc.lpwfxFormat = &wfx;

   if (IDirectSound8_CreateSoundBuffer(lpDS8, &dsbdesc, &lpDSB2, NULL) != DS_OK)
   {
      MessageBox (NULL, "IDirectSound8_CreateSoundBuffer error", "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   IDirectSoundBuffer8_Play(lpDSB2, 0, 0, DSBPLAY_LOOPING);

   if ((stereodata16 = (u16 *)malloc(soundbufsize)) == NULL)
      return -1;

   memset(stereodata16, 0, soundbufsize);

   soundvolume = DSBVOLUME_MAX;
   issoundmuted = 0;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDDXDeInit()
{
   DWORD status=0;

   if (lpDSB2)
   {
      IDirectSoundBuffer8_GetStatus(lpDSB2, &status);

      if(status == DSBSTATUS_PLAYING)
         IDirectSoundBuffer8_Stop(lpDSB2);

      IDirectSoundBuffer8_Release(lpDSB2);
      lpDSB2 = NULL;
   }

   if (lpDSB)
   {
      IDirectSoundBuffer8_Release(lpDSB);
      lpDSB = NULL;
   }

   if (lpDS8)
   {
      IDirectSound8_Release(lpDS8);
      lpDS8 = NULL;
   }
}

//////////////////////////////////////////////////////////////////////////////

int SNDDXReset()
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int SNDDXChangeVideoFormat(int vertfreq)
{
   soundlen = 44100 / vertfreq;
   soundbufsize = soundlen * NUMSOUNDBLOCKS * 2 * 2;

   if (stereodata16)
      free(stereodata16);

   if ((stereodata16 = (u16 *)malloc(soundbufsize)) == NULL)
      return -1;

   memset(stereodata16, 0, soundbufsize);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDDXUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples)
{
   LPVOID buffer1;
   LPVOID buffer2;
   DWORD buffer1_size, buffer2_size;
   DWORD status;

   IDirectSoundBuffer8_GetStatus(lpDSB2, &status);

   if (status & DSBSTATUS_BUFFERLOST)
      return; // fix me

   IDirectSoundBuffer8_Lock(lpDSB2, soundoffset, num_samples * sizeof(s16) * 2, &buffer1, &buffer1_size, &buffer2, &buffer2_size, 0);

   ScspConvert32uto16s((s32 *)leftchanbuffer, (s32 *)rightchanbuffer, (s16 *)stereodata16, num_samples);
   memcpy(buffer1, stereodata16, buffer1_size);
   if (buffer2)
      memcpy(buffer2, stereodata16+buffer1_size, buffer2_size);

   soundoffset += buffer1_size + buffer2_size;
   soundoffset %= soundbufsize;

   IDirectSoundBuffer8_Unlock(lpDSB2, buffer1, buffer1_size, buffer2, buffer2_size);
}

//////////////////////////////////////////////////////////////////////////////

u32 SNDDXGetAudioSpace()
{
   DWORD playcursor, writecursor;
   u32 freespace=0;

   if (IDirectSoundBuffer8_GetCurrentPosition (lpDSB2, &playcursor, &writecursor) != DS_OK)
      return 0;

   if (soundoffset > playcursor)
      freespace = soundbufsize - soundoffset + playcursor;
   else
      freespace = playcursor - soundoffset;

//   if (freespace > 512)
      return (freespace / 2 / 2);
//   else
//      return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDDXMuteAudio()
{
   issoundmuted = 1;
   IDirectSoundBuffer8_SetVolume (lpDSB2, DSBVOLUME_MIN);
}

//////////////////////////////////////////////////////////////////////////////

void SNDDXUnMuteAudio()
{
   issoundmuted = 0;
   IDirectSoundBuffer8_SetVolume (lpDSB2, soundvolume);
}

//////////////////////////////////////////////////////////////////////////////

void SNDDXSetVolume(int volume)
{
   soundvolume = (((LONG)volume) - 100) * 100;
   if (!issoundmuted)
      IDirectSoundBuffer8_SetVolume (lpDSB2, soundvolume);
}

