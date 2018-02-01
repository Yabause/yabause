// C implementation of Heap Sort
#include <stdio.h>
#include <stdlib.h>

#include "patternManager.h"

//#define VDP1_CACHE_STAT

#ifdef VDP1_CACHE_STAT
static long long nbReuse = 0;
static long long nbElem = 0;
static long long nbColid = 0;
#endif

static Pattern** patternCache;
static u8* patternUse;

void deleteCachePattern(Pattern** pat) {
	if ((*pat) == NULL) return;
	(*pat)->managed = 0;
	if ((*pat)->pix != NULL) free((*pat)->pix);
        (*pat)->pix = NULL;
	free(*pat);
        *pat = NULL;
}

static u16 djb2(unsigned char *str, int len)
{
  u16 hash = 5381;
  u16 c;
  int i;
  unsigned char *tmp = str;

  for (i = 0; i<len; i++) {
    c = *tmp++;
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

static u16 getHash(u8 *p, int len) {
  u16 hash;     
  hash = djb2(p, len);
  return hash;
}

static Pattern* popCachePattern(u8 *pixSample, int w, int h) {
  int id = getHash(pixSample, 32);
  Pattern *pat = patternCache[id];
  if ((pat!= NULL) && (memcmp(pat->pixSample, pixSample, 32) == 0) && (pat->width == w) && (pat->height == h)) {
#ifdef VDP1_CACHE_STAT
        nbReuse++;
#endif
	patternUse[id]++;
  	return pat;
  } else {
	return NULL;
  }
}

static int addCachePattern(Pattern* pat) {
        int id = getHash(pat->pixSample, 32);
	Pattern *collider = patternCache[id];
#ifdef VDP1_CACHE_STAT
        nbElem++;
#endif
	if ((collider != NULL) && (patternUse[id] > 0)) {
#ifdef VDP1_CACHE_STAT
            nbColid++;
#endif
            return -1;
        }
	if (collider != NULL) {
#ifdef VDP1_CACHE_STAT
          nbColid++;
#endif
	  deleteCachePattern(&collider);
	}
        patternUse[id] = 1;
	pat->managed = 1;
	patternCache[id] = pat;
        return 0;
}

static Pattern* createCachePattern(u8* pixSample, int w, int h, int offset) {
	Pattern* new = malloc(sizeof(Pattern));
        memcpy(new->pixSample, pixSample, 32);
        new->width = w;
	new->height = h;
        new->offset = offset;
	new->managed = 0;
	new->pix = (u32*) malloc(h*(offset+w)*sizeof(u32));
	return new;
}

Pattern* getPattern(vdp1cmd_struct *cmd, u8* ram) {
    int characterWidth = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
    int characterHeight = cmd->CMDSIZE & 0xFF;
    int characterAddress = cmd->CMDSRCA << 3;

    if(characterWidth*characterHeight <= 256) return NULL; //Cache impact is negligible here

    if ((characterWidth == 0) || (characterHeight == 0)) return NULL;

    int param0 = cmd->CMDSRCA << 16 | cmd->CMDCOLR;
    int param1 = cmd->CMDPMOD << 16 | cmd->CMDCTRL;
    u8 pixSample[32];

    for (int i =0; i<24; i++) {
      pixSample[i] =  T1ReadByte(ram, (characterAddress + characterHeight*characterWidth*i/24));
    }
    memcpy(&pixSample[24], &param0, 4);
    memcpy(&pixSample[28], &param1, 4);

    Pattern* curPattern = popCachePattern(pixSample, characterWidth, characterHeight);
    return curPattern;
}

void addPattern(vdp1cmd_struct *cmd, u8* ram, u32 *pix, int offset) {
    Pattern* curPattern;
    int characterWidth = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
    int characterHeight = cmd->CMDSIZE & 0xFF;
    int characterAddress = cmd->CMDSRCA << 3;

    if(characterWidth*characterHeight <= 256) return; //Cache impact is negligible here

    int param0 = cmd->CMDSRCA << 16 | cmd->CMDCOLR;
    int param1 = cmd->CMDPMOD << 16 | cmd->CMDCTRL;
    u8 pixSample[32];

    for (int i =0; i<24; i++) {
      pixSample[i] =  T1ReadByte(ram, (characterAddress + characterHeight*characterWidth*i/24));
    }
    memcpy(&pixSample[24], &param0, 4);
    memcpy(&pixSample[28], &param1, 4);

    curPattern = createCachePattern(pixSample, characterWidth, characterHeight, offset);
    if (addCachePattern(curPattern) != 0) deleteCachePattern(&curPattern);
    else {
      for (int i=0; i<characterHeight; i++) {
    	memcpy(&curPattern->pix[i*characterWidth], &pix[i*(characterWidth+offset)], characterWidth*sizeof(u32));
      }
    }
}

void releasePattern() {
    memset(patternUse, 0, 0xFFFF);
}

void resetPatternCache(){
    for (int i = 0; i<0x10000; i++) {
      deleteCachePattern(&patternCache[i]);
    }
#ifdef VDP1_CACHE_STAT
  nbReuse = 0;
  nbElem = 0;
  nbColid = 0;
#endif
}

void initPatternCache(){
    patternCache = malloc(0x10000 * sizeof(Pattern*));
    patternUse = malloc(0x10000 * sizeof(u8));
    for (int i = 0; i<0x10000; i++) {
      patternCache[i] = NULL;
      patternUse[i] = 0;
    }
}

void deinitPatternCache(){
    free(patternCache);
    free(patternUse);
#ifdef VDP1_CACHE_STAT
    printf("VDP1 Cache Stat Elem(%lld), Collision(%lld), Reuse(%lld)\n", nbElem, nbColid, nbReuse);
#endif
    resetPatternCache();
}


