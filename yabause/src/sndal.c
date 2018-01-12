/*  Copyright 2009 Lawrence Sebald
    Copyright 2005-2006 Theo Berkau

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifdef HAVE_LIBAL
  
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "threads.h"
#include <stdlib.h>
#include <string.h>


#include "error.h"
#include "scsp.h"
#include "sndal.h"
#include "debug.h"

int SNDALInit(void);
void SNDALDeInit(void);
int SNDALReset(void);
int SNDALChangeVideoFormat(int vertfreq);
void SNDALUpdateAudio(u32 *left, u32 *right, u32 samples);
u32 SNDALGetAudioSpace(void);
void SNDALMuteAudio(void);
void SNDALUnMuteAudio(void);
void SNDALSetVolume(int vol);

SoundInterface_struct SNDAL =   {
    SNDCORE_AL,
    "OpenAL Sound Interface",
    SNDALInit,
    SNDALDeInit,
    SNDALReset,
    SNDALChangeVideoFormat,
    SNDALUpdateAudio,
    SNDALGetAudioSpace,
    SNDALMuteAudio,
    SNDALUnMuteAudio,
    SNDALSetVolume
};

#define SOUND_BUFFERS   12
#define SOUND_FREQ      44100

static ALCdevice *device = NULL;
static ALCcontext *context = NULL;
static ALuint source;
static ALuint bufs[SOUND_BUFFERS];

static u16 *buffer;
static u32 soundbufsize = 0;
static u32 soundoffset;
static volatile u32 soundpos;

static int thd_done = 0;

static int soundvolume = 1;
static int soundlen;

//#define AL_DEBUG
#ifdef AL_DEBUG
#define LOG printf
#else
#define LOG
#endif

static void sound_update_thd(void *ptr __attribute__((unused)))    {
    ALint proc;
    ALuint buf;
	
    u8 data[2048];
	
    u8 *soundbuf = (u8 *)buffer;
    int i;
    u32 alerror;

    if( alcMakeContextCurrent(context) != AL_TRUE ){
        printf("fail to makecurrent\n");
        return;
    }
#if 0
    {
        int i=0;
        u16 * dst = (u16*)data;
        int len = 1024;
        int total_len=0;
        for (i = 0; i < (len>>1); i++)   {

            s16 wavedata;
            if( (total_len % 600) < 300 ){
                wavedata = 0x7FFF;
            }else{
                wavedata = -0x8000;
            }
            total_len++;
            *dst = wavedata;
            dst++;
            *dst = wavedata;
            dst++;
        }
    }    
#endif
    while(!thd_done)    {
        /* See if the stream needs updating yet. */
        LOG("AL_BUFFERS_PROCESSED in\n");
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &proc);
        LOG("AL_BUFFERS_PROCESSED out %d\n",proc);

        if( (alerror=alGetError()) != AL_NO_ERROR) {
            LOG("AL_BUFFERS_PROCESSED alGetError %d\n", alerror);
            continue;
        }
		
        /* Go through each buffer that needs more data. */
        while(proc--)   {
            /* Unqueue the old buffer, so that it can be filled again. */
            LOG("alSourceUnqueueBuffers in\n");
            alSourceUnqueueBuffers(source, 1, &buf);
            LOG("alSourceUnqueueBuffers out\n");
            
            if( (alerror=alGetError()) != AL_NO_ERROR) {
                LOG("alGetError %d\n", alerror);
                continue;
            }
#if 1
            for(i = 0; i < 2048; ++i)   {
                if(soundpos >= soundbufsize)
                    soundpos = 0;

                data[i] = soundbuf[soundpos];
                //if( data[i]!= 0 ){ printf("wav %d\n",data[i]); };
                ++soundpos;
            }
#endif            
            LOG("sound_update_thd soundpos = %d\n",soundpos);

            alBufferData(buf, AL_FORMAT_STEREO16, data, 2048, SOUND_FREQ);
            
            LOG("alSourceQueueBuffers in\n");
            alSourceQueueBuffers(source, 1, &buf);
            LOG("alSourceQueueBuffers out\n");
        }

		YabThreadYield();
    }

    //return NULL;
}

int total_len = 0;

static void sdlConvert32uto16s(s32 *srcL, s32 *srcR, s16 *dst, u32 len)    {
    u32 i;

    for (i = 0; i < len; i++)   {
#if 0
        s16 wavedata;
        if( (total_len % 2400) < 1200 ){
            wavedata = 0x7FFF;
        }else{
            wavedata = -0x8000;
        }
        total_len++;
        *dst = wavedata;
        dst++;
        *dst = wavedata;
        dst++;
#else        
        // Left Channel
        *srcL = ( *srcL *soundvolume ) / 100;
        if (*srcL > 0x7FFF) *dst = 0x7FFF;
        else if (*srcL < -0x8000) *dst = -0x8000;
        else *dst = *srcL;
        srcL++;
        dst++;
        // Right Channel
        *srcR = ( *srcR *soundvolume ) / 100;
        if (*srcR > 0x7FFF) *dst = 0x7FFF;
        else if (*srcR < -0x8000) *dst = -0x8000;
        else *dst = *srcR;
        srcR++;
        dst++;
#endif        
    } 
}

void SNDALUpdateAudio(u32 *left, u32 *right, u32 num_samples)   {
    u32 copy1size = 0, copy2size = 0;

    if((soundbufsize - soundoffset) < (num_samples * sizeof(s16) * 2))  {
        copy1size = (soundbufsize - soundoffset);
        copy2size = (num_samples * sizeof(s16) * 2) - copy1size;
    }
    else    {
        copy1size = (num_samples * sizeof(s16) * 2);
        copy2size = 0;
    }

    sdlConvert32uto16s((s32 *)left, (s32 *)right,
                       (s16 *)(((u8 *)buffer)+soundoffset),
                       copy1size / sizeof(s16) / 2);

    if(copy2size)
        sdlConvert32uto16s((s32 *)left + (copy1size / sizeof(s16) / 2),
                           (s32 *)right + (copy1size / sizeof(s16) / 2),
                           (s16 *)buffer, copy2size / sizeof(s16) / 2);

    soundoffset += copy1size + copy2size;   
    soundoffset %= soundbufsize;

}

int SNDALInit() {
    int rv = 0, i;
    int alerror;

    /* Attempt to grab the preferred device from OpenAL. */
    device = alcOpenDevice(NULL);
    if(!device) {
        rv = -1;
        goto err1;
    }

    context = alcCreateContext(device, NULL);

    if(!context)    {
        rv = -2;
        goto err2;
    }

    if( alcMakeContextCurrent(context) != ALC_TRUE ){
        rv = -2;
        goto err2;       
    }

    /* Clear any error states. */
    alGetError();

    /* Create our sound buffers. */
    alGenBuffers(SOUND_BUFFERS, bufs);

    if(alGetError() != AL_NO_ERROR) {
        rv = -3;
        goto err3;
    }

    /* Create the source for the stream. */
    alGenSources(1, &source);

    if(alGetError() != AL_NO_ERROR) {
        rv = -4;
        goto err4;
    }

    /* Set up the source for basic playback. */
    alSource3f(source, AL_DIRECTION, 0.0f, 0.0f, 0.0f);
    alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(source, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    alSourcef(source, AL_ROLLOFF_FACTOR, 0.0f);
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    //alSourcef(source,AL_GAIN,1.0f);
    //alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);

	if((alerror = alGetError()) != AL_NO_ERROR) {
		exit(1);
	}

    soundlen = 2048; //SOUND_FREQ / 60;

    soundbufsize = soundlen * SOUND_BUFFERS * 2 * 2;
    soundvolume = 100;

    if((buffer = (u16 *)malloc(soundbufsize)) == NULL)  {
        rv = -5;
        goto err5;
    }

    memset(buffer, 0, soundbufsize);
#if 0
    {
        int i=0;
        u16 * dst = (u16*)buffer;
        int len = soundlen * SOUND_BUFFERS;
        total_len=0;
        for (i = 0; i < len; i++)   {

            s16 wavedata;
            if( (total_len % 2400) < 1200 ){
                wavedata = 0x7FFF;
            }else{
                wavedata = -0x8000;
            }
            total_len++;
            *dst = wavedata;
            dst++;
            *dst = wavedata;
            dst++;
        }
        total_len = 0;
    }
#endif
    for(i = 0; i < SOUND_BUFFERS; ++i)  {
        /* Fill the buffer with empty sound. */
        alBufferData(bufs[i], AL_FORMAT_STEREO16, buffer + i * 2048, 2048,
                     SOUND_FREQ);
        alSourceQueueBuffers(source, 1, bufs + i);
    }

    /* Start the sound playback. */
    alSourcePlay(source);
    if( (alerror=alGetError()) != AL_NO_ERROR) {
         LOG("alSourcePlay alGetError %d\n", alerror);
    
    }

    alcMakeContextCurrent(NULL);
    /* Start the update thread. */
	YabThreadStart(YAB_THREAD_OPENAL,sound_update_thd,(void *)buffer);
    return 0;

    /* Error conditions. Errors cause cascading deinitialization, so hence this
       chain of labels. */
err5:
    alDeleteSources(1, &source);

err4:
    alDeleteBuffers(SOUND_BUFFERS, bufs);

err3:
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);

err2:
    alcCloseDevice(device);

err1:
    context = NULL;
    device = NULL;
    return rv;
}

void SNDALDeInit()  {
    /* Stop our update thread. */
    thd_done = 1;
    //pthread_join(thd, NULL);
	YabThreadWait(YAB_THREAD_OPENAL);
    /* Stop playback. */
    alSourceStop(source);

    /* Clean up our buffers and such. */
    alDeleteSources(1, &source);
    alDeleteBuffers(SOUND_BUFFERS, bufs);

    /* Release our context and close the sound device. */
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);

    context = NULL;
    device = NULL;
    thd_done = 0;
}

int SNDALReset()    {
    return 0;
}

int SNDALChangeVideoFormat(int vertfreq)    {
    soundlen = SOUND_FREQ / vertfreq;
    soundbufsize = soundlen * SOUND_BUFFERS * 2 * 2;

    if(buffer)
        free(buffer);

    if((buffer = (u16 *)malloc(soundbufsize)) == NULL)
        return -1;

    memset(buffer, 0, soundbufsize);

    return 0;
}

u32 SNDALGetAudioSpace()    {
    u32 freespace=0;
    if(soundoffset > soundpos)
        freespace = soundbufsize - soundoffset + soundpos;
    else
        freespace = soundpos - soundoffset;

    LOG("%d,%d,%d,%d\n",freespace,soundbufsize,soundoffset,soundpos);
    
    return (freespace / sizeof(s16) / 2);
}

static int sound_pause = 0;

void SNDALMuteAudio()   {
    
    if( sound_pause == 0){
        alSourcePause(source);
        alSourcef(source, AL_GAIN, 0.0f);
        sound_pause = 1;
    }
}

void SNDALUnMuteAudio() {
    if( sound_pause == 1){
        alSourcePlay(source);
        alSourcef(source, AL_GAIN, 1.0f);
        sound_pause = 0;
    }
}

void SNDALSetVolume(int vol)    {
    soundvolume = 100; //(int)((128.0 / 100.0) * vol);
}
#endif /* HAVE_LIBAL */
