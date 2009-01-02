/*  Copyright 2005 Guillaume Duhamel
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <string.h>

#ifndef STDCALL
#define STDCALL
#endif

#ifndef FASTCALL
#ifdef __MINGW32__
#define FASTCALL __attribute__((fastcall))
#elif defined (__i386__)
#define FASTCALL __attribute__((regparm(3)))
#else
#define FASTCALL
#endif
#endif

/* When building multiple arches on OS X you must use the compiler-
   provided endian flags instead of the one provided by autoconf */
#if defined(__BIG_ENDIAN__) || defined(__LITTLE_ENDIAN__)
 #undef WORDS_BIGENDIAN
 #ifdef __BIG_ENDIAN__
  #define WORDS_BIGENDIAN
 #endif
#endif


#ifndef INLINE
#ifdef _MSC_VER
#define INLINE _inline
#else
#define INLINE inline
#endif 
#endif

#ifndef GEKKO
typedef unsigned char u8;
typedef unsigned short u16;

typedef signed char s8;
typedef signed short s16;

#if defined(__LP64__)
// Generic 64-bit
typedef unsigned int u32;
typedef unsigned long u64;
typedef unsigned long pointer;

typedef signed int s32;
typedef signed long s64;

#elif defined(_MSC_VER)
typedef unsigned long u32;
typedef unsigned __int64 u64;
typedef unsigned long long u64;
#ifdef _WIN64
typedef __int64 pointer;
#else
typedef unsigned long pointer;
#endif

typedef signed long s32;
typedef __int64 s64;
typedef signed long long s64;

#else
// 32-bit Linux GCC/MINGW/etc.
typedef unsigned long u32;
typedef unsigned long long u64;
typedef unsigned long pointer;

typedef signed long s32;
typedef signed long long s64;
#endif
#else
// Gamecube/Wii
#include <gccore.h>
typedef unsigned long pointer;
#endif

typedef struct {
	unsigned int size;
	unsigned int done;
} IOCheck_struct;

static INLINE void ywrite(IOCheck_struct * check, void * ptr, size_t size, size_t nmemb, FILE * stream) {
   check->done += (unsigned int)fwrite(ptr, size, nmemb, stream);
   check->size += (unsigned int)size;
}

static INLINE void yread(IOCheck_struct * check, void * ptr, size_t size, size_t nmemb, FILE * stream) {
   check->done += (unsigned int)fread(ptr, size, nmemb, stream);
   check->size += (unsigned int)size;
}

static INLINE int StateWriteHeader(FILE *fp, const char *name, int version) {
   IOCheck_struct check;
   fprintf(fp, "%s", name);
   ywrite(&check, (void *)&version, sizeof(version), 1, fp);
   ywrite(&check, (void *)&version, sizeof(version), 1, fp); // place holder for size
   return (check.done == check.size) ? ftell(fp) : -1;
}

static INLINE int StateFinishHeader(FILE *fp, int offset) {
   IOCheck_struct check;
   int size = 0;
   size = ftell(fp) - offset;
   fseek(fp, offset - 4, SEEK_SET);
   ywrite(&check, (void *)&size, sizeof(size), 1, fp); // write true size
   fseek(fp, 0, SEEK_END);
   return (check.done == check.size) ? (size + 12) : -1;
}

static INLINE int StateCheckRetrieveHeader(FILE *fp, const char *name, int *version, int *size) {
   char id[4];
   size_t ret;

   if ((ret = fread((void *)id, 1, 4, fp)) != 4)
      return -1;

   if (strncmp(name, id, 4) != 0)
      return -2;

   if ((ret = fread((void *)version, 4, 1, fp)) != 1)
      return -1;

   if (fread((void *)size, 4, 1, fp) != 1)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

// Terrible, but I'm not sure how to do the equivalent in inline
#ifdef HAVE_C99_VARIADIC_MACROS
#define AddString(s, ...) \
   sprintf(s, __VA_ARGS__); \
   s += strlen(s)
#else
#define AddString(s, r...) \
   sprintf(s, ## r); \
   s += strlen(s)
#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_LIBMINI18N
#include "mini18n.h"
#else
#ifndef _
#define _(a) (a)
#endif
#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef __GNUC__

#define UNUSED __attribute ((unused))

#ifdef DEBUG
#define USED_IF_DEBUG
#else
#define USED_IF_DEBUG __attribute ((unused))
#endif

#ifdef SMPC_DEBUG
#define USED_IF_SMPC_DEBUG
#else
#define USED_IF_SMPC_DEBUG __attribute ((unused))
#endif

#else

#define UNUSED
#define USED_IF_DEBUG
#define USED_IF_SMPC_DEBUG
#endif

#endif
