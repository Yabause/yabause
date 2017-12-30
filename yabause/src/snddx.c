/*  Copyright (C) 2005-2007 Theo Berkau

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file snddx.c
    \brief Direct X sound interface.
*/


#include <math.h>
#include <dsound.h>
#include <mmsystem.h>
#include "debug.h"
#include "dx.h"
#include "scsp.h"
#include "snddx.h"
#include "error.h"

int SNDDXInit(void);
void SNDDXDeInit(void);
int SNDDXReset(void);
int SNDDXChangeVideoFormat(int vertfreq);
void SNDDXUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples);
u32 SNDDXGetAudioSpace(void);
void SNDDXMuteAudio(void);
void SNDDXUnMuteAudio(void);
void SNDDXSetVolume(int volume);
#ifdef USE_SCSPMIDI
int SNDDXMidiInit(int inport, int outport);
void SNDDXMidiDeInit();
int SNDDXMidiChangePorts(int inport, int outport);
u8 SNDDXMidiIn(int *isdata);
int SNDDXMidiOut(u8 data);
#endif

SoundInterface_struct SNDDIRECTX = {
SNDCORE_DIRECTX,
"DirectX Sound Interface",
SNDDXInit,
SNDDXDeInit,
SNDDXReset,
SNDDXChangeVideoFormat,
SNDDXUpdateAudio,
SNDDXGetAudioSpace,
SNDDXMuteAudio,
SNDDXUnMuteAudio,
SNDDXSetVolume,
#ifdef USE_SCSPMIDI
SNDDXMidiChangePorts,
SNDDXMidiIn,
SNDDXMidiOut
#endif
};

LPDIRECTSOUND8 lpDS8;
LPDIRECTSOUNDBUFFER lpDSB, lpDSB2;

#define NUMSOUNDBLOCKS  4

static u16 *stereodata16;
static u32 soundlen;
static u32 soundoffset=0;
static u32 soundbufsize;
static LONG soundvolume;
static int issoundmuted;

HWND DXGetWindow ();

HMIDIIN indevice=NULL;
HMIDIOUT outdevice=NULL;
static CRITICAL_SECTION dxmidi_cs;

//////////////////////////////////////////////////////////////////////////////

int SNDDXInit(void)
{
   DSBUFFERDESC dsbdesc;
   WAVEFORMATEX wfx;
   HRESULT ret;
   char tempstr[512];

   if ((ret = DirectSoundCreate8(NULL, &lpDS8, NULL)) != DS_OK)
   {
      sprintf(tempstr, "Sound. DirectSound8Create error: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
      YabSetError(YAB_ERR_CANNOTINIT, tempstr);
      return -1;
   }

   if ((ret = IDirectSound8_SetCooperativeLevel(lpDS8, DXGetWindow(), DSSCL_PRIORITY)) != DS_OK)
   {
      sprintf(tempstr, "Sound. IDirectSound8_SetCooperativeLevel error: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
      YabSetError(YAB_ERR_CANNOTINIT, tempstr);
      return -1;
   }

   memset(&dsbdesc, 0, sizeof(dsbdesc));
   dsbdesc.dwSize = sizeof(DSBUFFERDESC);
   dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
   dsbdesc.dwBufferBytes = 0;
   dsbdesc.lpwfxFormat = NULL;

   if ((ret = IDirectSound8_CreateSoundBuffer(lpDS8, &dsbdesc, &lpDSB, NULL)) != DS_OK)
   {
      sprintf(tempstr, "Sound. Error when creating primary sound buffer: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
      YabSetError(YAB_ERR_CANNOTINIT, tempstr);
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

   if ((ret = IDirectSoundBuffer8_SetFormat(lpDSB, &wfx)) != DS_OK)
   {
      sprintf(tempstr, "Sound. IDirectSoundBuffer8_SetFormat error: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
      YabSetError(YAB_ERR_CANNOTINIT, tempstr);
      return -1;
   }

   memset(&dsbdesc, 0, sizeof(dsbdesc));
   dsbdesc.dwSize = sizeof(DSBUFFERDESC);
   dsbdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_STICKYFOCUS |
                     DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2 |
                     DSBCAPS_LOCHARDWARE;
   dsbdesc.dwBufferBytes = soundbufsize;
   dsbdesc.lpwfxFormat = &wfx;

   if ((ret = IDirectSound8_CreateSoundBuffer(lpDS8, &dsbdesc, &lpDSB2, NULL)) != DS_OK)
   {
      if (ret == DSERR_CONTROLUNAVAIL ||
          ret == DSERR_INVALIDCALL ||
          ret == E_FAIL || 
          ret == E_NOTIMPL)
      {
         // Try using a software buffer instead
         dsbdesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_STICKYFOCUS |
                           DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2 |
                           DSBCAPS_LOCSOFTWARE;

         if ((ret = IDirectSound8_CreateSoundBuffer(lpDS8, &dsbdesc, &lpDSB2, NULL)) != DS_OK)
         {
            sprintf(tempstr, "Sound. Error when creating secondary sound buffer: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
            YabSetError(YAB_ERR_CANNOTINIT, tempstr);
            return -1;
         }
      }
      else
      {
         sprintf(tempstr, "Sound. Error when creating secondary sound buffer: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
         YabSetError(YAB_ERR_CANNOTINIT, tempstr);
         return -1;
      }
   }

   IDirectSoundBuffer8_Play(lpDSB2, 0, 0, DSBPLAY_LOOPING);

   if ((stereodata16 = (u16 *)malloc(soundbufsize)) == NULL)
      return -1;

   memset(stereodata16, 0, soundbufsize);

   soundvolume = DSBVOLUME_MAX;
   issoundmuted = 0;

#ifdef USE_SCSPMIDI
   SNDDXMidiInit (-1, -1);
#endif

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDDXDeInit(void)
{
   DWORD status=0;

#ifdef USE_SCSPMIDI
   SNDDXMidiDeInit();
#endif

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

int SNDDXReset(void)
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
      memcpy(buffer2, ((u8 *)stereodata16)+buffer1_size, buffer2_size);

   soundoffset += buffer1_size + buffer2_size;
   soundoffset %= soundbufsize;

   IDirectSoundBuffer8_Unlock(lpDSB2, buffer1, buffer1_size, buffer2, buffer2_size);
}

//////////////////////////////////////////////////////////////////////////////

u32 SNDDXGetAudioSpace(void)
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

void SNDDXMuteAudio(void)
{
   issoundmuted = 1;
   IDirectSoundBuffer8_SetVolume (lpDSB2, DSBVOLUME_MIN);
}

//////////////////////////////////////////////////////////////////////////////

void SNDDXUnMuteAudio(void)
{
   issoundmuted = 0;
   IDirectSoundBuffer8_SetVolume (lpDSB2, soundvolume);
}

//////////////////////////////////////////////////////////////////////////////

void SNDDXSetVolume(int volume)
{
   // Convert linear volume to logarithmic volume
   if (volume == 0)
      soundvolume = DSBVOLUME_MIN;
   else
      soundvolume = (LONG)(log10((double)volume / 100.0) * 2000.0);

   if (!issoundmuted)
      IDirectSoundBuffer8_SetVolume (lpDSB2, soundvolume);
}

//////////////////////////////////////////////////////////////////////////////

volatile u8 midi_in_buf[512];
volatile int midi_in_cnt=0;

void CALLBACK midiincallback(
   HMIDIIN   hMidiIn,
   UINT      wMsg,
   DWORD_PTR dwInstance,
   DWORD_PTR dwParam1,
   DWORD_PTR dwParam2)
{
   switch (wMsg)
   {
   case MIM_OPEN: // Open/Close device
   case MIM_CLOSE:
      break;
   case MIM_DATA:
   {
      u8 status=dwParam1;
      // status byte
      if ((status & 0xF0) == 0xF0)
         break;
      EnterCriticalSection (&dxmidi_cs);
      midi_in_buf[midi_in_cnt++] = status;
      switch((status & 0xF0) >> 4)
      {
         case 0x8: // Note Off/On(2 bytes)
         case 0x9:
         case 0xA: // Polyphonic Aftertouch(2 bytes)
         case 0xB: // Control/Mode Change(2 bytes)
         case 0xE: // Pitch Bend(2 bytes)
            midi_in_buf[midi_in_cnt++] = dwParam1 >> 8;
            midi_in_buf[midi_in_cnt++] = dwParam1 >> 16;
            break;
         case 0xC: // Program Change(1 byte)
         case 0xD: // Channel Aftertouch(1 byte)
            midi_in_buf[midi_in_cnt++] = dwParam1 >> 8;
            break;
         default: 
            break;
      }
      LeaveCriticalSection(&dxmidi_cs);
      break;
   }
   default:
      //LOG("wMsg = Unsupport/unknown\n");
      break;
   }
}

//////////////////////////////////////////////////////////////////////////////
#ifdef USE_SCSPMIDI
int SNDDXMidiInit (int inport, int outport)
{
   MMRESULT flag;

   if (inport < 0)
      inport = 0;

   if (outport < 0)
      outport = 0;

   InitializeCriticalSection(&dxmidi_cs);

   // open midi in port
   flag = midiInOpen(&indevice, inport, (DWORD_PTR)midiincallback, 0, CALLBACK_FUNCTION);
   if (flag != MMSYSERR_NOERROR) {
		YabSetError (YAB_ERR_CANNOTINIT, (void *)"MIDI Input");
      return -1;
   }

   midiInStart(indevice);

   // open midi out port
   flag = midiOutOpen(&outdevice, outport, 0, 0, CALLBACK_NULL);
   if (flag != MMSYSERR_NOERROR) {
		YabSetError (YAB_ERR_CANNOTINIT, (void *)"MIDI Output");
      return -1;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDDXMidiDeInit ()
{
   // Remove any data in MIDI device and close the MIDI ports
   if (indevice)
   {
      midiInStop(indevice);
      midiInClose(indevice);
      indevice = NULL;
      DeleteCriticalSection(&dxmidi_cs);
   }
   if (outdevice)
   {
      midiOutReset(outdevice);
      midiOutClose(outdevice);
      outdevice = NULL;
   }
}

//////////////////////////////////////////////////////////////////////////////

int SNDDXMidiChangePorts(int inport, int outport)
{
   SNDDXMidiDeInit();
   return SNDDXMidiInit(inport, outport);
}

//////////////////////////////////////////////////////////////////////////////

u8 SNDDXMidiIn(int *isdata)
{
   if (midi_in_cnt)
   {
      int i;
      u8 data;
      *isdata = 1;
      EnterCriticalSection (&dxmidi_cs);
      data = midi_in_buf[0];
      midi_in_cnt--;
      for (i = 0; i < midi_in_cnt; i++)
         midi_in_buf[i] = midi_in_buf[i+1];
      LeaveCriticalSection (&dxmidi_cs);
      return data;
   }
   else
   {
      *isdata = 0;
      return 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

int SNDDXMidiOut(u8 data)
{
   static u8 buf[32];
   static int cnt=0;

   if (outdevice)
   {
      buf[cnt] = data;
      cnt++;

      switch((buf[0] & 0xF0) >> 4)
      {
         case 0x8: // Note Off/On(2 bytes)
         case 0x9:
         case 0xA: // Polyphonic Aftertouch(2 bytes)
         case 0xB: // Control/Mode Change(2 bytes)
         case 0xE: // Pitch Bend(2 bytes)
            if (cnt == 3)
            {
               midiOutShortMsg(outdevice, buf[0] | (buf[1] << 8) | (buf[2] << 16));
               cnt = 0;
            }
            break;
         case 0xC: // Program Change(1 byte)
         case 0xD: // Channel Aftertouch(1 byte)
            if (cnt == 2)
            {
               midiOutShortMsg(outdevice, buf[0] | (buf[1] << 8));
               cnt = 0;
            }
            break;
         default: 
            //LOG("Unsupported midi command %02X\n", buf[0]);
            break;
      }
   }
   return 1;
}
#endif
//////////////////////////////////////////////////////////////////////////////
