#ifndef __PATTERN_MANAGER_H__
#define __PATTERN_MANAGER_H__

#include "core.h"
#include "vdp1.h"

typedef struct sPattern {
	u32 *pix;
	int width;
	int height;
        int offset;
	int inUse;
	int managed;
	u32 param[3];
} Pattern;

Pattern* getPattern(vdp1cmd_struct *cmd, u8* ram);
void releasePattern();
void addPattern(vdp1cmd_struct *cmd, u8* ram, u32 *pix, int offset);


#endif
