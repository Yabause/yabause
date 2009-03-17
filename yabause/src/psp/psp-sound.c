/*  src/psp/psp-sound.c: PSP sound output module
    Copyright 2009 Andrew Church

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

#include "../yabause.h"
#include "../scsp.h"

#include "common.h"
#include "sys.h"
#include "psp-sound.h"

/*************************************************************************/

/* Interface function declarations (must come before interface definition) */

static int psp_sound_init(void);
static void psp_sound_deinit(void);
static int psp_sound_reset(void);
static int psp_sound_change_video_format(int vertfreq);
static void psp_sound_update_audio(u32 *leftchanbuffer, u32 *rightchanbuffer,
                                   u32 num_samples);
static u32 psp_sound_get_audio_space(void);
static void psp_sound_mute_audio(void);
static void psp_sound_unmute_audio(void);
static void psp_sound_set_volume(int volume);

/*-----------------------------------------------------------------------*/

/* Module interface definition */

SoundInterface_struct SNDPSP = {
    .id                = SNDCORE_PSP,
    .Name              = "PSP Sound Interface",
    .Init              = psp_sound_init,
    .DeInit            = psp_sound_deinit,
    .Reset             = psp_sound_reset,
    .ChangeVideoFormat = psp_sound_change_video_format,
    .UpdateAudio       = psp_sound_update_audio,
    .GetAudioSpace     = psp_sound_get_audio_space,
    .MuteAudio         = psp_sound_mute_audio,
    .UnMuteAudio       = psp_sound_unmute_audio,
    .SetVolume         = psp_sound_set_volume,
};

/*************************************************************************/

/* Playback rate in Hz (unchangeable) */
#define PLAYBACK_RATE  44100

/* Playback buffer size in samples (larger values = less chance of skipping
 * but greater lag) */
#define BUFFER_SIZE  512

/* Buffer descriptors for left and right channels, implementing a lockless
 * ring buffer with the following semantics:
 *     WRITER (main program) waits for .write_ready to become nonzero, then
 *         writes BUFFER_SIZE samples of data into .buffer[.next_free] and
 *         sets .write_ready to zero.
 *     READER (playback thread) waits for .write_ready to become zero,
 *         submits .buffer[.next_free] to the OS (which blocks until the
 *         previous buffer finishes playing), updates .next_free to point
 *         to the other playback buffer (0 or 1), and sets .write_ready to
 *         nonzero.
 */
typedef struct PSPSoundBufferDesc_ {
    int started;      // Nonzero if channel is playing
    int channel;      // Channel number allocated for this buffer
    int16_t buffer[2][BUFFER_SIZE*2];
    int next_free;    // Index of next buffer to store data into
    int write_ready;  // When nonzero, data can be written to buffer[next_free]
    /* Internal use: */
    SceUID thread;    // Playback thread handle
    int stop;         // Flag to tell thread to terminate
} PSPSoundBufferDesc;
static PSPSoundBufferDesc stereo_buffer;

/* Number of samples stored in current buffer (for partial buffer fills) */
static int saved_samples = 0;

/* Mute flag (used by Mute and UnMute methods) */
static int muted = 1;

/*-----------------------------------------------------------------------*/

/* Local function declarations */

static int start_channel(PSPSoundBufferDesc *buffer_desc);
void stop_channel(PSPSoundBufferDesc *buffer_desc);
static int playback_thread(SceSize args, void *argp);

/*************************************************************************/
/************************** Interface functions **************************/
/*************************************************************************/

/**
 * psp_sound_init:  Initialize the sound interface.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, negative on error
 */
static int psp_sound_init(void)
{
    if (stereo_buffer.started) {
        /* Already initialized! */
        return 0;
    }

    if (!start_channel(&stereo_buffer)) {
        DMSG("Failed to start playback");
        return -1;
    }

    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sound_deinit:  Shut down the sound interface.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_sound_deinit(void)
{
    stop_channel(&stereo_buffer);
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sound_reset:  Reset the sound interface.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, negative on error
 */
static int psp_sound_reset(void)
{
    /* Nothing to do */
    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sound_change_video_format:  Handle a change in the video refresh
 * frequency.
 *
 * [Parameters]
 *     vertfreq: New refresh frequency (Hz)
 * [Return value]
 *     Zero on success, negative on error
 */
static int psp_sound_change_video_format(int vertfreq)
{
    /* Nothing to do */
    return 0;
}

/*************************************************************************/

/**
 * psp_sound_update_audio:  Output audio data.
 *
 * [Parameters]
 *      leftchanbuffer: Left channel sample array, as _signed_ 16-bit samples
 *     rightchanbuffer: Right channel sample array, as _signed_ 16-bit samples
 *         num_samples: Number of samples in sample arrays
 * [Return value]
 *     None
 */
static void psp_sound_update_audio(u32 *leftchanbuffer, u32 *rightchanbuffer,
                                   u32 num_samples)
{
    if (!leftchanbuffer || !rightchanbuffer
     || !stereo_buffer.write_ready
     || num_samples == 0 || num_samples > BUFFER_SIZE - saved_samples
    ) {
        DMSG("invalid parameters: %p %p %u (status: wr=%d ss=%d)",
             leftchanbuffer, rightchanbuffer, (unsigned int)num_samples,
             stereo_buffer.write_ready, saved_samples);
        return;
    }

    int32_t *in_l = (int32_t *)leftchanbuffer;
    int32_t *in_r = (int32_t *)rightchanbuffer;
    int16_t *out =
        &stereo_buffer.buffer[stereo_buffer.next_free][saved_samples*2];
    int i;
    for (i = 0; i < num_samples; i++) {
        int32_t lval, rval;
        lval = *in_l++;
        rval = *in_r++;
        *out++ = bound(lval, -0x8000, 0x7FFF);
        *out++ = bound(rval, -0x8000, 0x7FFF);
    }
    saved_samples += num_samples;
    if (saved_samples >= BUFFER_SIZE) {
        stereo_buffer.write_ready = 0;
        saved_samples = 0;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sound_get_audio_space:  Return the number of samples immediately
 * available for outputting audio data.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Number of samples available
 */
static u32 psp_sound_get_audio_space(void)
{
    if (stereo_buffer.write_ready) {
        return BUFFER_SIZE - saved_samples;
    } else {
        return 0;
    }
}

/*************************************************************************/

/**
 * psp_sound_mute_audio:  Disable audio output.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_sound_mute_audio(void)
{
    muted = 1;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sound_mute_audio:  Enable audio output.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_sound_unmute_audio(void)
{
    muted = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sound_set_volume:  Set the audio output volume.
 *
 * [Parameters]
 *     volume: New volume (0-100, 100 = full volume)
 * [Return value]
 *     None
 */
static void psp_sound_set_volume(int volume)
{
    const int pspvol = (PSP_AUDIO_VOLUME_MAX * volume + 50) / 100;
    if (stereo_buffer.started) {
        sceAudioChangeChannelVolume(stereo_buffer.channel, pspvol, pspvol);
    }
}

/*************************************************************************/
/********************* PSP-local interface functions *********************/
/*************************************************************************/

/**
 * psp_sound_pause:  Stop audio output.  Called when the system is being
 * suspended.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void psp_sound_pause(void)
{
    if (stereo_buffer.started) {
        sceKernelSuspendThread(stereo_buffer.thread);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sound_unpause:  Resume audio output.  Called when the system is
 * resuming from a suspend.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void psp_sound_unpause(void)
{
    if (stereo_buffer.started) {
        sceKernelResumeThread(stereo_buffer.thread);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sound_exit:  Terminate all playback in preparation for exiting.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void psp_sound_exit(void)
{
    if (stereo_buffer.started) {
        stop_channel(&stereo_buffer);
    }
}

/*************************************************************************/
/****************** Low-level audio channel management *******************/
/*************************************************************************/

/**
 * start_channel:  Allocate a new channel and starts playback.
 *
 * [Parameters]
 *     buffer_desc: Playback buffer descriptor
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int start_channel(PSPSoundBufferDesc *buffer_desc)
{
    if (!buffer_desc) {
        DMSG("buffer_desc == NULL");
        return 0;
    }
    if (buffer_desc->started) {
        DMSG("Buffer is already started!");
        return 0;
    }

    /* Allocate a hardware channel */
    buffer_desc->channel = sceAudioChReserve(
        PSP_AUDIO_NEXT_CHANNEL, BUFFER_SIZE, PSP_AUDIO_FORMAT_STEREO
    );
    if (buffer_desc->channel < 0) {
        DMSG("Failed to allocate channel: %s",
             psp_strerror(buffer_desc->channel));
        return 0;
    }

    /* Initialize the ring buffer and other fields */
    memset(buffer_desc->buffer, 0, sizeof(buffer_desc->buffer));
    buffer_desc->next_free = 0;
    buffer_desc->write_ready = 1;
    buffer_desc->stop = 0;

    /* Start the playback thread */
    char thname[100];
    snprintf(thname, sizeof(thname), "YabauseSoundCh%d", buffer_desc->channel);
    SceUID handle = sys_start_thread(thname, playback_thread,
                                     THREADPRI_SOUND, 0x1000,
                                     sizeof(buffer_desc), &buffer_desc);
    if (handle < 0) {
        DMSG("Failed to create thread: %s", psp_strerror(handle));
        sceAudioChRelease(buffer_desc->channel);
        return 0;
    }
    buffer_desc->thread = handle;

    /* Success */
    buffer_desc->started = 1;
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * stop_channel:  Stop playback from the given playback buffer.
 *
 * [Parameters]
 *     buffer_desc: Playback buffer descriptor
 * [Return value]
 *     None
 */
void stop_channel(PSPSoundBufferDesc *buffer_desc)
{
    if (!buffer_desc) {
        DMSG("buffer_desc == NULL");
        return;
    }
    if (!buffer_desc->started) {
        DMSG("Buffer has not been started!");
        return;
    }

    /* Signal the thread to stop, then wait for it (if we try to stop the
     * thread in the middle of an audio write, we won't be able to free
     * the hardware channel) */
    buffer_desc->stop = 1;
    int tries;
    for (tries = (1000 * (2*BUFFER_SIZE)/PLAYBACK_RATE); tries > 0; tries--) {
        if (sys_delete_thread_if_stopped(buffer_desc->thread, NULL)) {
            break;
        }
        sceKernelDelayThread(1000);  // Wait for 1ms before trying again
    }

    if (!tries) {
        /* The thread didn't stop on its own, so terminate it with
         * extreme prejudice */
        sceKernelTerminateDeleteThread(buffer_desc->thread);
        sceAudioChRelease(buffer_desc->channel);
        memset(buffer_desc, 0, sizeof(*buffer_desc));
    }
}

/*************************************************************************/

/**
 * playback_thread:  Sound playback thread.  Continually sends the ring
 * buffer data to the OS until signaled to stop.
 *
 * [Parameters]
 *     args: Thread argument size
 *     argp: Thread argument pointer
 * [Return value]
 *     Always zero
 */
static int playback_thread(SceSize args, void *argp)
{
    PSPSoundBufferDesc * const buffer_desc = *(PSPSoundBufferDesc **)argp;

    while (!buffer_desc->stop) {
        while (buffer_desc->write_ready) {
            sceKernelDelayThread(10000);  // 0.01 sec
        }
        sceAudioOutputBlocking(buffer_desc->channel, muted ? 0 : 0x8000,
                               buffer_desc->buffer[buffer_desc->next_free]);
        buffer_desc->next_free = (buffer_desc->next_free + 1) % 2;
        buffer_desc->write_ready = 1;
    }

    sceAudioChRelease(buffer_desc->channel);
    memset(buffer_desc, 0, sizeof(*buffer_desc));
    return 0;
}

/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
