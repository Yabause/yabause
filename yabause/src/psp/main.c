/*  src/psp/main.c: PSP entry point and core interface routines
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
#include "../cdbase.h"
#include "../cs0.h"
#include "../debug.h"
#include "../m68kcore.h"
#include "../peripheral.h"
#include "../scsp.h"
#include "../sh2core.h"
#include "../sh2int.h"
#include "../vidsoft.h"

#include "common.h"
#include "display.h"
#include "sys.h"
#include "psp-per.h"
#include "psp-sh2.h"
#include "psp-sound.h"
#include "psp-video.h"

/*************************************************************************/

/* PSP module information */
PSP_MODULE_INFO("Yabause", 0, 0, 2);
const PSP_MAIN_THREAD_PRIORITY(THREADPRI_MAIN);
const PSP_MAIN_THREAD_STACK_SIZE_KB(128);
const PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
const PSP_HEAP_SIZE_KB(-64);  // Leave 64k for thread stacks

/* Main thread handle */
static SceUID main_thread;

/* Program directory (determined from argv[0]) */
static char progpath[256];

/* Various data file paths (read from command-line options or .ini file) */
//static char inipath[256] = "yabause.ini";
static char biospath[256] = "bios.bin";
static char cdpath[256] = "cd.iso";
static char buppath[256] = "backup.bin";
//static char mpegpath[256] = "";
//static char cartpath[256] = "";

/*-----------------------------------------------------------------------*/

/* Interface data */

M68K_struct *M68KCoreList[] = {
    &M68KDummy,
#ifdef HAVE_C68K
    &M68KC68K,
#endif
#ifdef HAVE_Q68
    &M68KQ68,
#endif
    NULL
};

SH2Interface_struct *SH2CoreList[] = {
    &SH2Interpreter,
    &SH2DebugInterpreter,
    &SH2PSP,
    NULL
};

PerInterface_struct *PERCoreList[] = {
    &PERDummy,
    &PERPSP,
    NULL
};

CDInterface *CDCoreList[] = {
    &DummyCD,
    &ISOCD,
    NULL
};

SoundInterface_struct *SNDCoreList[] = {
    &SNDDummy,
    &SNDPSP,
    NULL
};

VideoInterface_struct *VIDCoreList[] = {
    &VIDDummy,
    &VIDPSP,
    &VIDSoft,
    NULL
};

/*-----------------------------------------------------------------------*/

/* Local routine declarations */

static void init_psp(int argc, char **argv);
static void load_settings(void);
static void init_yabause(void);

static void get_base_directory(const char *argv0, char *buffer, int bufsize);
static int callback_thread(void);
static int exit_callback(int arg1, int arg2, void *common);
static int power_callback(int unknown, int power_info, void *common);

/*************************************************************************/
/****************** Entry point and initialization code ******************/
/*************************************************************************/

/**
 * main:  Program entry point.  Performs initialization and then loops
 * indefinitely, running the emulator.
 *
 * [Parameters]
 *     argc: Command line argument count
 *     argv: Command line argument vector
 * [Return value]
 *     Does not return
 */
int main(int argc, char **argv)
{
    init_psp(argc, argv);
    load_settings();
    init_yabause();
    for (;;) {
        if (PERCore->HandleEvents() != 0) {
            sceKernelExitGame();
        }
    }
}

/*************************************************************************/

/**
 * init_psp:  Perform PSP-related initialization and command-line option
 * parsing.  Aborts the program if an error occurs.
 *
 * [Parameters]
 *     argc: Command line argument count
 *     argv: Command line argument vector
 * [Return value]
 *     None
 */
static void init_psp(int argc, char **argv)
{
    /* Set the CPU to maximum speed, because boy, do we need it */
    scePowerSetClockFrequency(333, 333, 166);

    /* Determine the program's base directory and change to it */
    get_base_directory(argv[0], progpath, sizeof(progpath));
    if (*progpath) {
        sceIoChdir(progpath);
    }

    /* Retrieve the main thread's handle */
    main_thread = sceKernelGetThreadId();

    /* Start the system callback monitoring thread */
    SceUID thid = sys_start_thread(
        "YabauseCallbackThread", callback_thread, THREADPRI_SYSTEM_CB,
        0x1000, 0, NULL
    );
    if (thid < 0) {
        DMSG("sys_start_thread(callback_thread) failed: %s",
             psp_strerror(thid));
        sceKernelExitGame();
    }

    /* Initialize the display hardware */
    if (!display_init()) {
        DMSG("Failed to initialize display");
        sceKernelExitGame();
    }
}

/*-----------------------------------------------------------------------*/

/**
 * load_settings:  Load settings from the default settings file, if available.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void load_settings(void)
{
    // FIXME: not implemented
}

/*-----------------------------------------------------------------------*/

/**
 * init_yabause:  Initialize the emulator core.  Aborts the program if an
 * error occurs.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void init_yabause(void)
{
    yabauseinit_struct yinit;

    /* Set up general defaults */
    memset(&yinit, 0, sizeof(yinit));
    yinit.m68kcoretype = M68KCORE_Q68;
    yinit.percoretype = PERCORE_PSP;
    yinit.sh2coretype = SH2CORE_PSP;
    yinit.vidcoretype = VIDCORE_PSP;
    yinit.sndcoretype = SNDCORE_PSP;
    yinit.cdcoretype = CDCORE_ISO;
    yinit.carttype = CART_NONE;
    yinit.regionid = 0;
    yinit.biospath = biospath;
    yinit.cdpath = cdpath;
    yinit.buppath = buppath;
    yinit.mpegpath = NULL;
    yinit.cartpath = NULL;
    yinit.flags = VIDEOFORMATTYPE_NTSC;

    /* Initialize controller settings */
    PerInit(yinit.percoretype);
    PerPortReset();
    void *padbits = PerPadAdd(&PORTDATA1);
    static const struct {
        uint8_t key;      // Key index from peripheral.h
        uint16_t button;  // PSP button index (PSP_CTRL_*)
    } buttons[] = {
        {PERPAD_UP,             PSP_CTRL_UP},
        {PERPAD_RIGHT,          PSP_CTRL_RIGHT},
        {PERPAD_DOWN,           PSP_CTRL_DOWN},
        {PERPAD_LEFT,           PSP_CTRL_LEFT},
        {PERPAD_RIGHT_TRIGGER,  PSP_CTRL_RTRIGGER},
        {PERPAD_LEFT_TRIGGER,   PSP_CTRL_LTRIGGER},
        {PERPAD_START,          PSP_CTRL_START},
        {PERPAD_A,              PSP_CTRL_SQUARE},
        {PERPAD_B,              PSP_CTRL_CROSS},
        {PERPAD_C,              0},
        {PERPAD_X,              PSP_CTRL_TRIANGLE},
        {PERPAD_Y,              PSP_CTRL_CIRCLE},
        {PERPAD_Z,              0},
    };
    int i;
    for (i = 0; i < lenof(buttons); i++) {
        PerSetKey(buttons[i].button, buttons[i].key, padbits);
    }

    /* Initialize emulator state */
    if (YabauseInit(&yinit) != 0) {
        DMSG("YabauseInit() failed!");
        sceKernelExitGame();
    }
    ScspUnMuteAudio();
}

/*************************************************************************/

/**
 * get_base_directory:  Extract the program's base directory from argv[0].
 * Stores the empty string in the destination buffer if the base directory
 * cannot be determined.
 *
 * [Parameters]
 *       argv0: Value of argv[0]
 *      buffer: Buffer to store directory path into
 *     bufsize: Size of buffer
 * [Return value]
 *     None
 */
static void get_base_directory(const char *argv0, char *buffer, int bufsize)
{
    *buffer = 0;
    if (argv0) {
        const char *s = strrchr(argv0, '/');
        if (s != NULL) {
            int n = snprintf(buffer, bufsize, "%.*s", s - argv0, argv0);
            if (n >= bufsize) {
                DMSG("argv[0] too long: %s", argv0);
                *buffer = 0;
            }
        } else {
            DMSG("argv[0] has no directory: %s", argv0);
        }
    } else {
        DMSG("argv[0] is NULL!");
    }
}

/*-----------------------------------------------------------------------*/

/**
 * callback_thread:  Thread for monitoring HOME button and power status
 * notification callbacks.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Does not return
 */
static int callback_thread(void)
{
    /* Note: We can turn off the HOME button menu with
     * sceImposeSetHomePopup(0) if we want to create our own */
    SceUID cbid;
    cbid = sceKernelCreateCallback("YabauseExitCallback",
                                   exit_callback, NULL);
    if (cbid < 0) {
        DMSG("sceKernelCreateCallback(exit_callback) failed: %s",
             psp_strerror(cbid));
    } else {
        sceKernelRegisterExitCallback(cbid);
    }

    cbid = sceKernelCreateCallback("YabausePowerCallback",
                                   power_callback, NULL);
    if (cbid < 0) {
        DMSG("sceKernelCreateCallback(exit_callback) failed: %s",
             psp_strerror(cbid));
    } else {
        scePowerRegisterCallback(0, cbid);
    }

    for (;;) {
        sceKernelSleepThreadCB();
    }

    return 0;  // Avoid compiler warning
}

/*-----------------------------------------------------------------------*/

/**
 * exit_callback:  HOME button callback.  Exits the program immediately.
 *
 * [Parameters]
 *     arg1, arg2, common: Unused
 * [Return value]
 *     Does not return
 */
static int exit_callback(int arg1, int arg2, void *common)
{
    psp_sound_exit();
    sceKernelExitGame();
}

/*-----------------------------------------------------------------------*/

/**
 * power_callback:  Power status notification callback.  Handles suspend
 * and resume events.
 *
 * [Parameters]
 *        unknown: Unused
 *     power_info: Power status
 *         common: Unused
 * [Return value]
 *     Always zero
 */
static int power_callback(int unknown, int power_info, void *common)
{
    if (power_info & PSP_POWER_CB_SUSPENDING) {
        sceKernelSuspendThread(main_thread);
        psp_sound_pause();
    } else if (power_info & PSP_POWER_CB_RESUMING) {
        /* Restore the current directory.  It takes time for the memory
         * stick to be recognized, so wait up to 3 seconds before giving up. */
        uint32_t start = sceKernelGetSystemTimeLow();
        int res;
        do {
            res = sceIoChdir(progpath);
            if (res < 0) {
                sceKernelDelayThread(1000000/10);  // 0.1 sec
            }
        } while (res < 0 && sceKernelGetSystemTimeLow()-start < 3*1000000);
        if (res < 0) {
            DMSG("Restore current directory (%s) failed: %s",
                 progpath, psp_strerror(res));
        }
        /* Resume program */
        psp_sound_unpause();
        sceKernelResumeThread(main_thread);
    }
    return 0;
}

/*************************************************************************/
/******************** Yabause core interface routines ********************/
/*************************************************************************/

/**
 * YuiErrorMsg:  Report an error to the user.
 *
 * [Parameters]
 *     string: Error message string
 * [Return value]
 *     None
 */
void YuiErrorMsg(const char *string)
{
    fprintf(stderr, "%s", string);
}

/*************************************************************************/

/**
 * YuiSwapBuffers:  Swap the front and back display buffers.  Called by the
 * software renderer.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void YuiSwapBuffers(void)
{
    if (!dispbuffer) {
        return;
    }

    /* Make sure all video buffer data is flushed to memory */
    sceKernelDcacheWritebackAll();

    /* Calculate display size (shrink interlaced/hi-res displays by half) */
    int width_in, height_in, width_out, height_out;
    VIDSoftGetScreenSize(&width_in, &height_in);
    if (width_in <= DISPLAY_WIDTH) {
        width_out = width_in;
    } else {
        width_out = width_in / 2;
    }
    if (height_in <= DISPLAY_HEIGHT) {
        height_out = height_in;
    } else {
        height_out = height_in / 2;
    }
    int x = (DISPLAY_WIDTH - width_out) / 2;
    int y = (DISPLAY_HEIGHT - height_out) / 2;

    display_begin_frame();
    if (width_out == width_in && height_out == height_in) {
        display_blit(dispbuffer, width_in, height_in, width_in, x, y);
    } else {
        /* The PSP can't draw textures larger than 512x512, so if we're
         * drawing a high-resolution buffer, split it in half.  The height
         * will never be greater than 512, so we don't need to check for a
         * vertical split. */
        if (width_in > 512) {
            const uint32_t *dispbuffer32 = (const uint32_t *)dispbuffer;
            display_blit_scaled(dispbuffer32, width_in/2, height_in,
                                width_in, x, y, width_out/2, height_out);
            dispbuffer32 += width_in/2;
            x += width_out/2;
            display_blit_scaled(dispbuffer32, width_in/2, height_in,
                                width_in, x, y, width_out/2, height_out);
        } else {
            display_blit_scaled(dispbuffer, width_in, height_in, width_in,
                                x, y, width_out, height_out);
        }
    }
    display_end_frame();
    sceDisplayWaitVblankStart();
}

/*************************************************************************/
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
