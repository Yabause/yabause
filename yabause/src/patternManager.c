// C implementation of Heap Sort
#include <stdio.h>
#include <stdlib.h>

#include "patternManager.h"

Pattern* patternCache[0xFFFFFF];

void deleteCachePattern(Pattern* pat) {
	if (pat == NULL) return;
	if (pat->inUse != 0) return;
	pat->managed = 0;
	if (pat->pix != NULL) free(pat->pix);
        pat->pix = NULL;
	free(pat);
}

static u16 getHash(int param0, int param1) {
	u8 c[6];
	int i;
	u32 hash = 0xAAA555;
	c[0] = (param0 >> 0) & 0xFFF;
	c[1] = (param0 >> 12) & 0xFFF;
	c[2] = (param0 >> 24) & 0xFFF;
	c[3] = (param1 >> 0) & 0xFFF;
	c[4] = (param1 >> 12) & 0xFFF;
	c[5] = (param1 >> 24) & 0xFFF;

	for (i = 0; i<6; i++) {
		hash = hash ^ c[i];
	}
	for (i = 1; i<6; i++) {
		hash = hash ^ (c[i] << 12);
	}
//printf("Hash 0x%x\n", hash);
	return hash;
}

static Pattern* popCachePattern(int param0, int param1, int param2, int w, int h) {
  Pattern *pat = patternCache[getHash(param0, param1)];
  if ((pat!= NULL) && (pat->param[0]==param0) && (pat->param[1]==param1) && (pat->param[2]==param2) && (pat->width == w) && (pat->height == h)) {
//printf("Hit cache\n");
	pat->inUse++;
  	return pat;
  } else {
//printf("Miss cache\n");
	return NULL;
  }
}

static void pushCachePattern(Pattern *pat) {
	if (pat == NULL) return;
	pat->inUse = pat->inUse-1;
}

static void addCachePattern(Pattern* pat) {
	Pattern *collider = patternCache[getHash(pat->param[0], pat->param[1])];
	if ((collider != NULL) && (collider->inUse > 0)) {
//printf("Damned collider in use\n");
            return;
        }
	if (collider != NULL) {
                //printf("Delete pattern\n");
		deleteCachePattern(collider);
	}
	pat->managed = 1;
	patternCache[getHash(pat->param[0], pat->param[1])] = pat;
}

static Pattern* createCachePattern(int param0, int param1, int param2, int w, int h, int offset) {
	Pattern* new = malloc(sizeof(Pattern));
	new->param[0] = param0;
	new->param[1] = param1;
	new->param[2] = param2;
        new->width = w;
	new->height = h;
        new->offset = offset;
	new->managed = 0;
	new->inUse = 1;
	new->pix = (u32*) malloc(h*(offset+w)*sizeof(u32));
	return new;
}

Pattern* getPattern(vdp1cmd_struct *cmd, u8* ram) {
    int characterWidth = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
    int characterHeight = cmd->CMDSIZE & 0xFF;
    int characterAddress = cmd->CMDSRCA << 3;

    if ((characterWidth == 0) || (characterHeight == 0)) return NULL;

    int param0 = cmd->CMDSRCA << 16 | cmd->CMDCOLR;
    int param1 = cmd->CMDPMOD << 16 | cmd->CMDCTRL;
    int param2 = 0;

    param2 = T1ReadByte(ram, (characterAddress + characterHeight*characterWidth/3 + (characterWidth/3 >> 1)) & 0x7FFFF) << 16 | T1ReadByte(ram, (characterAddress + characterHeight*characterWidth*2/3 + (characterWidth*2/3 >> 1)) & 0x7FFFF);

    Pattern* curPattern = popCachePattern(param0, param1, param2, characterWidth, characterHeight);
    return curPattern;
}

void addPattern(vdp1cmd_struct *cmd, u8* ram, u32 *pix, int offset) {
    Pattern* curPattern;
    int characterWidth = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
    int characterHeight = cmd->CMDSIZE & 0xFF;
    int characterAddress = cmd->CMDSRCA << 3;

    if ((characterWidth == 0) || (characterHeight == 0)) return;

    int param0 = cmd->CMDSRCA << 16 | cmd->CMDCOLR;
    int param1 = cmd->CMDPMOD << 16 | cmd->CMDCTRL;
    int param2 = 0;

    param2 = T1ReadByte(ram, (characterAddress + characterHeight*characterWidth/3 + (characterWidth/3 >> 1)) & 0x7FFFF) << 16 | T1ReadByte(ram, (characterAddress + characterHeight*characterWidth*2/3 + (characterWidth*2/3 >> 1)) & 0x7FFFF);

    curPattern = createCachePattern(param0, param1, param2, characterWidth, characterHeight, offset);
    for (int i=0; i<characterHeight; i++) {
    	memcpy(&curPattern->pix[i*characterWidth], &pix[i*(characterWidth+offset)], characterWidth*sizeof(u32));
    }
    addCachePattern(curPattern);
}

void releasePattern() {
    for (int i = 0; i<0xFFFF; i++) {
	if ((patternCache[i] != NULL) && (patternCache[i]->inUse != 0))
            pushCachePattern(patternCache[i]);
    }
}


