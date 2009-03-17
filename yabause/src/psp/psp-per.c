/*  src/psp/psp-per.c: PSP peripheral interface module
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
#include "../peripheral.h"

#include "common.h"
#include "psp-per.h"

#ifdef SYS_PROFILE_H
# include "profile.h"  // Can only be ours
#endif

/*************************************************************************/

/* Interface function declarations (must come before interface definition) */

static int psp_per_init(void);
static void psp_per_deinit(void);
static int psp_per_handle_events(void);
#ifdef PERKEYNAME
static void psp_per_key_name(u32 key, char *name, int size);
#endif

/*-----------------------------------------------------------------------*/

/* Module interface definition */

PerInterface_struct PERPSP = {
    .id           = PERCORE_PSP,
    .Name         = "PSP Peripheral Interface",
    .Init         = psp_per_init,
    .DeInit       = psp_per_deinit,
    .HandleEvents = psp_per_handle_events,
    .canScan      = 0,
#ifdef PERKEYNAME
    .KeyName      = psp_per_key_name,
#endif
};

/*************************************************************************/

/* Local routine declarations */

static void check_pad(void);

/*************************************************************************/
/************************** Interface functions **************************/
/*************************************************************************/

/**
 * psp_per_init:  Initialize the peripheral interface.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, negative on error
 */
static int psp_per_init(void)
{
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_per_deinit:  Shut down the peripheral interface.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_per_deinit(void)
{
    /* We don't implement shutting down, so nothing to do */
}

/*************************************************************************/

/**
 * psp_per_handle_events:  Process pending peripheral events, and run one
 * iteration of the emulation.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, negative on error
 */
static int psp_per_handle_events(void)
{
#if defined(PSP_DEBUG) || defined(SYS_PROFILE_H)  // Unused otherwise
    static int frame = 0;
    DMSG("frame %d", frame);
    frame++;
#endif

    check_pad();

    if (YabauseExec() != 0) {
        return -1;
    }

#ifdef SYS_PROFILE_H  // Print out profiling info every 100 frames
    if (frame % 100 == 0) {
        PROFILE_PRINT();
        PROFILE_RESET();
    }
#endif

    return 0;
}

/*************************************************************************/

#ifdef PERKEYNAME

/**
 * psp_per_key_name:  Return the name corresponding to a system-dependent
 * key value.
 *
 * [Parameters]
 *      key: Key value to return name for
 *     name: Buffer into which name is to be stored
 *     size: Size of buffer in bytes
 * [Return value]
 *     None
 */
static void psp_per_key_name(u32 key, char *name, int size)
{
    /* Not supported on PSP */
    *name = 0;
}

#endif  // PERKEYNAME

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * check_pad:  Check the PSP control pad for input and map it to Saturn
 * controller input.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void check_pad(void)
{
    static uint32_t last_buttons;  // Previous value of pad_data.Buttons

    SceCtrlData pad_data;
    sceCtrlPeekBufferPositive(&pad_data, 1);

    /* If the directional pad isn't being used, check the analog pad instead */
    if (!(pad_data.Buttons & 0x00F0)) {
        if (pad_data.Lx < 32) {
            pad_data.Buttons |= PSP_CTRL_LEFT;
        } else if (pad_data.Lx >= 224) {
            pad_data.Buttons |= PSP_CTRL_RIGHT;
        }
        if (pad_data.Ly < 32) {
            pad_data.Buttons |= PSP_CTRL_UP;
        } else if (pad_data.Ly >= 224) {
            pad_data.Buttons |= PSP_CTRL_DOWN;
        }
    }

    /* Check for changed buttons and send them to the core */
    uint32_t changed_buttons = pad_data.Buttons ^ last_buttons;
    int i;
    for (i = 0; i < 16; i++) {
        const uint32_t button = 1 << i;
        if (changed_buttons & button) {
            if (pad_data.Buttons & button) {
                PerKeyDown(button);
            } else {
                PerKeyUp(button);
            }
        }
    }

    /* Save the current input state for next time */
    last_buttons = pad_data.Buttons;
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
