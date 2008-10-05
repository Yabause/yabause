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

#ifdef HAVE_LIBGL
#ifdef _MSC_VER
#include <windows.h>
#endif

#ifdef HAVE_LIBSDL
 #ifdef __APPLE__
  #include <SDL/SDL.h>
 #else
  #include "SDL.h"
 #endif
#endif
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

#ifdef USEMICSHADERS
extern GLuint shaderProgram;
extern int useShaders;
extern const unsigned char noMeshGouraud[16];
#endif

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

#ifdef USEMICSHADERS
typedef struct {
	unsigned char rgba[4*4];
} YglColor;
#endif

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
#ifdef USEMICSHADERS
	unsigned char * colors;
	int currentColors;
	int maxColors;
#endif
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

int YglGLInit(int, int);
int YglScreenInit(int r, int g, int b, int d);
int YglInit(int, int, unsigned int);
void YglDeInit(void);
int * YglQuad(YglSprite *, YglTexture *);
void YglCachedQuad(YglSprite *, int *);
#ifdef USEMICSHADERS
int * YglQuad2(YglSprite *, YglTexture *, YglColor *);
void YglCachedQuad2(YglSprite *, int *, YglColor *);
#endif
void YglRender(void);
void YglReset(void);
void YglShowTexture(void);
void YglChangeResolution(int, int);
void YglOnScreenDebugMessage(char *, ...);


int * YglIsCached(u32);
void YglCache(u32, int *);
void YglCacheReset(void);

#endif
#endif
