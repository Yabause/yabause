/*  Copyright 2003-2004 Stephane Dallongeville

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

#ifndef _C68KTYPES_H_
#define _C68KTYPES_H_

#ifdef _arch_dreamcast
#include <arch/types.h>
#else
#if defined(__LP64__)
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
#else
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;

typedef signed char int8;
typedef signed short int16;
typedef signed long int32;
#endif
#endif


// target specific
///////////////////

#ifdef _arch_dreamcast
#ifndef __sh__
#define __sh__
#endif
#else
#ifndef WORDS_BIGENDIAN
#ifndef __x86__
#define __x86__
#endif
#endif
#endif

// compiler specific
/////////////////////

#ifndef INLINE
#ifdef __x86__
#define INLINE      __inline
#else
#define INLINE      __inline
#endif
#endif

#ifndef FASTCALL
#ifdef __x86__
#ifdef __linux__
#define FASTCALL    __attribute__((__stdcall__))
#else
#define FASTCALL    __fastcall
#endif
#else
#define FASTCALL
#endif
#endif

// types definitions
/////////////////////

#ifndef NULL
#define NULL 0
#endif

#if defined(__LP64__)
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long pointer;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef signed char S8;
typedef signed short S16;
typedef signed int S32;
#else
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned int pointer;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;
typedef signed char S8;
typedef signed short S16;
typedef signed long S32;
#endif

#endif /* _TYPES_H_ */

