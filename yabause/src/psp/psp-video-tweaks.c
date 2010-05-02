/*  src/psp/psp-video-tweaks.c: Game-specific tweaks for PSP video module
    Copyright 2010 Andrew Church

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

#include "common.h"

#include "../memory.h"
#include "../vdp2.h"
#include "../vidshared.h"

#include "psp-video.h"
#include "psp-video-internal.h"

/*************************************************************************/
/************************** Interface functions **************************/
/*************************************************************************/

/**
 * psp_video_apply_tweaks:  Apply game-specific optimizations and tweaks
 * for faster/better PSP video output.  Called at the beginning of drawing
 * each frame.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
extern void psp_video_apply_tweaks(void)
{
    /**** Panzer Dragoon Saga: Display movies with transparency ****
     **** disabled to improve playback speed.                   ****/

    if (memcmp(HighWram+0x42F34, "APDNAR\0003", 8) == 0) {
        if ((Vdp2Regs->CHCTLA & 0x0070) == 0x0040) {
            Vdp2Regs->BGON |= 0x0100;
        }
    }
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
