#pragma once

#ifdef _MSC_VER

#include <intrin.h>

static unsigned long bitmanip_clz(unsigned long value)
{
	unsigned long r = 0;
	_BitScanReverse(&r, value);
	return (31 - r);
}

static unsigned long bitmanip_ctz(unsigned long value)
{
	unsigned long r = 0;
	_BitScanForward(&r, value);
	return r;
}

#define __builtin_popcount __popcnt
#define __builtin_clz bitmanip_clz
#define __builtin_ctz bitmanip_ctz

#endif
