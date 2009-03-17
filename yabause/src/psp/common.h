/*  src/psp/common.h: Common header for PSP source files
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

#ifndef PSP_COMMON_H
#define PSP_COMMON_H

/**************************************************************************/

/* Various system headers */

#define u8 pspsdk_u8  // Avoid type collisions with ../core.h
#define s8 pspsdk_s8
#define u16 pspsdk_u16
#define s16 pspsdk_s16
#define u32 pspsdk_u32
#define s32 pspsdk_s32
#define u64 pspsdk_u64
#define s64 pspsdk_s64

#ifdef PSP
#include <pspuser.h>
#include <pspaudio.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <psppower.h>
#endif

#undef u8
#undef s8
#undef u16
#undef s16
#undef u32
#undef s32
#undef u64
#undef s64

/* Helpful hints for GCC */
#ifdef __GNUC__
extern void sceKernelExitGame(void) __attribute__((noreturn));
extern int sceKernelExitThread(int status) __attribute__((noreturn));
extern int sceKernelExitDeleteThread(int status) __attribute__((noreturn));
#endif

/**************************************************************************/

/* Thread priority constants */
enum {
    THREADPRI_MAIN      = 32,
    THREADPRI_SOUND     = 20,
    THREADPRI_SYSTEM_CB = 15,
};

/**************************************************************************/

/* Convenience macros (not PSP-related) */

/* Get length of an array */
#define lenof(a)  (sizeof((a)) / sizeof((a)[0]))
/* Bound a value between two limits (inclusive) */
#define bound(x,low,high)  __extension__({  \
    typeof(x) __x = (x);                    \
    typeof(low) __low = (low);              \
    typeof(high) __high = (high);           \
    __x < __low ? __low : __x > __high ? __high : __x; \
})

#ifdef PSP_DEBUG

/* Debug/error message macro.  DMSG("message",...) prints to stderr a line
 * in the form:
 *     func_name(file:line): message
 * printf()-style format tokens and arguments are allowed, and no newline
 * is required at the end.  The format string must be a literal string
 * constant. */
#define DMSG(msg,...) \
    fprintf(stderr, "%s(%s:%d): " msg "\n", __FUNCTION__, __FILE__, __LINE__ \
            , ## __VA_ARGS__)

/* Timing macro.  Start timing with DSTART(); DEND() will then print the
 * elapsed time in microseconds.  Both must occur at the same level of
 * block nesting. */
#define DSTART() { const uint32_t __start = sceKernelGetSystemTimeLow()
#define DEND()     DMSG("time=%u", sceKernelGetSystemTimeLow() - __start); }

#else  // !PSP_DEBUG

/* Disable debug output */
#define DMSG(msg,...)  /*nothing*/
#define DSTART()       /*nothing*/
#define DEND()         /*nothing*/

#endif

/**************************************************************************/

#endif  // PSP_COMMON_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
