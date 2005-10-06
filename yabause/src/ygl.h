#ifdef MSVC
#include <windows.h>
#endif

#include "SDL.h"
#ifndef _arch_dreamcast
#if HAVE_LIBGLUT
    #ifdef __APPLE__
        #include <GLUT/glut.h>
    #else
        #include <GL/glut.h>
    #endif
#else
    #ifdef __APPLE__
        #include <OpenGL/gl.h>
    #else
        #include <GL/gl.h>
    #endif
#endif
#endif
#include <stdarg.h>
#include <string.h>

#ifndef YGL_H
#define YGL_H

#include "core.h"

typedef struct {
	int vertices[8];
	unsigned int w;
	unsigned int h;
	int flip;
	int priority;
} YglSprite;

typedef struct {
	unsigned int * textdata;
	unsigned int w;
} YglTexture;

typedef struct {
	unsigned int currentX;
	unsigned int currentY;
	unsigned int yMax;
	unsigned int * texture;
	unsigned int width;
	unsigned int height;
} YglTextureManager;

extern YglTextureManager * YglTM;

void YglTMInit(unsigned int, unsigned int);
void YglTMDeInit(void);
void YglTMReset(void);
 void YglTMAllocate(YglTexture *, unsigned int, unsigned int, unsigned int *, unsigned int *);

typedef struct {
	int * quads;
	int * textcoords;
	int currentQuad;
	int maxQuad;
} YglLevel;

typedef struct {
	GLuint texture;
	int st;
	char message[512];
	int msglength;
	unsigned int width;
	unsigned int height;
	unsigned int depth;
	YglLevel * levels;
}  Ygl;

extern Ygl * _Ygl;

int YglInit(int, int, unsigned int);
void YglDeInit(void);
int * YglQuad(YglSprite *, YglTexture *);
void YglCachedQuad(YglSprite *, int *);
void YglRender(void);
void YglReset(void);
void YglShowTexture(void);
void YglChangeResolution(int, int);
void YglOnScreenDebugMessage(char *, ...);


int * YglIsCached(u32);
void YglCache(u32, int *);
void YglCacheReset(void);

#endif
