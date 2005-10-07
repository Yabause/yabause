#ifndef CORE_H
#define CORE_H

#include <stdio.h>
#include <string.h>

#ifndef FASTCALL
#ifdef __i386__
#define FASTCALL __attribute__((regparm(3)))
#else
#define FASTCALL
#endif
#endif

#ifndef INLINE
#ifdef _MSC_VER
#define INLINE _inline
#else
#define INLINE inline
#endif 
#endif

#if defined(__LP64__)
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef unsigned long pointer;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long s64;
#else
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
typedef unsigned long pointer;

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef signed long long s64;
#endif

static inline int StateWriteHeader(FILE *fp, const char *name, int version) {
        fprintf(fp, name);
	fwrite((void *)&version, sizeof(version), 1, fp);
	fwrite((void *)&version, sizeof(version), 1, fp); // place holder for size
	return ftell(fp);
}

static inline int StateFinishHeader(FILE *fp, int offset) {
	int size = 0;
	size = ftell(fp) - offset;
	fseek(fp, offset - 4, SEEK_SET);
	fwrite((void *)&size, sizeof(size), 1, fp); // write true size
	fseek(fp, 0, SEEK_END);
	return (size + 12);
}

static inline int StateCheckRetrieveHeader(FILE *fp, const char *name, int *version, int *size) {
	char id[4];
	int ret;

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

#endif
