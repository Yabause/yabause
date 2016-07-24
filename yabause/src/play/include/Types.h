#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdexcept>

typedef char int8;
typedef unsigned char uint8;

typedef short int16;
typedef unsigned short uint16;

typedef int int32;

//Fixes compilation problem under xcode
#ifndef _UINT32
#define _UINT32
typedef unsigned int uint32;
#endif

typedef long long int64;
typedef unsigned long long uint64;

static_assert(sizeof(int8) == 1, "int8 size must be 1 byte.");
static_assert(sizeof(uint8) == 1, "uint8 size must be 1 byte.");

static_assert(sizeof(int16) == 2, "int16 size must be 2 bytes.");
static_assert(sizeof(uint16) == 2, "uint16 size must be 2 bytes.");

static_assert(sizeof(int32) == 4, "int32 size must be 4 bytes.");
static_assert(sizeof(uint32) == 4, "uint32 size must be 4 bytes.");

static_assert(sizeof(int64) == 8, "int64 size must be 8 bytes.");
static_assert(sizeof(uint64) == 8, "uint64 size must be 8 bytes.");

#endif
