#include <iostream>
#include <map>
#include "SDL.h"
#ifndef _arch_dreamcast
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#endif
#include <stdarg.h>
#include <string.h>

#ifndef YGL_HH
#define YGL_HH

using namespace std;

template<unsigned int W, unsigned int H>
class TextureManager {
private:
	unsigned int currentX;
	unsigned int currentY;
public:
	unsigned int yMax;
	unsigned int texture[ W * H ];

	void reset(void);
	void allocate(unsigned int ** textdata, unsigned int w, unsigned int h, unsigned int * x, unsigned int * y, unsigned int * z);
};

template<unsigned int W, unsigned int H>
void TextureManager<W,H>::reset(void) {
	currentX = 0;
	currentY = 0;
	yMax = 0;
}

template<unsigned int W, unsigned int H>
void TextureManager<W,H>::allocate(unsigned int ** textdata, unsigned int w, unsigned int h, unsigned int * x, unsigned int * y, unsigned int * z) {
	if (h > (H - currentY)) {
		cerr << "texture overflow" << endl;
		*x = 0;
		*y = 0;
		*z = 0;
		*textdata = texture;
		return;
	}
	if ((W - currentX) >= w) {
		*x = currentX;
		*y = currentY;
		*z = W - w;
		*textdata = texture + currentY * W + currentX;
		currentX += w;
		if ((currentY + h) > yMax)
			yMax = currentY + h;
	}
	else {
		currentX = 0;
		currentY = yMax;
		allocate(textdata, w, h, x, y, z);
	}
}

template<unsigned int W, unsigned int H, unsigned int V>
class YglLevel {
private:
	TextureManager<W, H> * textdata;
	int quads [ V * 2 ];
	int textcoords [ V * 2 ];
	int currentQuad;
public:
	void init(TextureManager<W, H> *);
	int * quad(int *, unsigned int **, unsigned int, unsigned int, unsigned int *, int);
	void cachedQuad(int *, int *, unsigned int, unsigned int, int);
	void render(void);
	void reset(void);
	void status(void);
};

template<unsigned int W, unsigned int H, unsigned int V>
void YglLevel<W,H,V>::init(TextureManager<W, H> * td) {
	textdata = td;
	currentQuad = 0;
}

template<unsigned int W, unsigned int H, unsigned int V>
int * YglLevel<W,H,V>::quad(int * vert, unsigned int ** text, unsigned int w, unsigned int h, unsigned int * z, int flip) {
	if (currentQuad >= (V * 2)) {
		cerr << "vertex overflow" << endl;
	}

	unsigned int x, y;
	int * tmp = textcoords + currentQuad;

	memcpy(quads + currentQuad, vert, 8 * sizeof(int));
	currentQuad += 8;
	textdata->allocate(text, w, h, &x, &y, z);
	if (flip & 0x1) {
		*tmp = *(tmp + 6) = x + w;
		*(tmp + 2) = *(tmp + 4) = x;
	} else {
		*tmp = *(tmp + 6) = x;
		*(tmp + 2) = *(tmp + 4) = x + w;
	}
	if (flip & 0x2) {
		*(tmp + 1) = *(tmp + 3) = y + h;
		*(tmp + 5) = *(tmp + 7) = y;
	} else {
		*(tmp + 1) = *(tmp + 3) = y;
		*(tmp + 5) = *(tmp + 7) = y + h;
	}
	switch(flip) {
		case 0:
			return textcoords + currentQuad - 8;
		case 1:
			return textcoords + currentQuad - 6;
		case 2:
			return textcoords + currentQuad - 2;
		case 3:
			return textcoords + currentQuad - 4;
	}
}

template<unsigned int W, unsigned int H, unsigned int V>
void YglLevel<W,H,V>::cachedQuad(int * vert, int * text, unsigned int w, unsigned int h, int flip) {
	if (currentQuad >= (V * 2)) {
		cerr << "vertex overflow" << endl;
	}

	unsigned int x,y;
	int * tmp = textcoords + currentQuad;

	memcpy(quads + currentQuad, vert, 8 * sizeof(int));
	currentQuad += 8;

	x = *text;
	y = *(text + 1);
	if (flip & 0x1) {
		*tmp = *(tmp + 6) = x + w;
		*(tmp + 2) = *(tmp + 4) = x;
	} else {
		*tmp = *(tmp + 6) = x;
		*(tmp + 2) = *(tmp + 4) = x + w;
	}
	if (flip & 0x2) {
		*(tmp + 1) = *(tmp + 3) = y + h;
		*(tmp + 5) = *(tmp + 7) = y;
	} else {
		*(tmp + 1) = *(tmp + 3) = y;
		*(tmp + 5) = *(tmp + 7) = y + h;
	}
}

template<unsigned int W, unsigned int H, unsigned int V>
void YglLevel<W,H,V>::render(void) {
	glVertexPointer(2, GL_INT, 0, quads);
	glTexCoordPointer(2, GL_INT, 0, textcoords);
	glDrawArrays(GL_QUADS, 0, currentQuad / 2);
}

template<unsigned int W, unsigned int H, unsigned int V>
void YglLevel<W,H,V>::reset(void) {
	currentQuad = 0;
}

template<unsigned int W, unsigned int H, unsigned int V>
void YglLevel<W,H,V>::status(void) {
	cerr << "currentQuad=" << currentQuad;
	cerr << " ( ";
	for(int i = 0;i < currentQuad;i++) {
		cerr << textcoords[i] << " ";
	}
	cerr << ")" << endl;
}

template<int W, int H, int L, int V>
class Ygl {
private:
	TextureManager<W, H> textdata;
	GLuint texture;
	bool st;
	char message[512];
	int msglength;
public:
	YglLevel<W,H,V> level [ L ];

	void init(void);
	int * quad(int, int *, unsigned int **, unsigned int, unsigned int, unsigned int *, int);
	void cachedQuad(int, int *, int *, unsigned int, unsigned int, int);
	void render(void);
	void reset(void);
	void status(void);
	void showTexture(void);
	void changeResolution(int, int);
	void onScreenDebugMessage(char *, ...);
};

template<int W, int H, int L, int V>
void Ygl<W,H,L,V>::init(void) {
	SDL_InitSubSystem( SDL_INIT_VIDEO );

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 4);
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	if ( SDL_SetVideoMode( 320, 224, 32, SDL_OPENGL ) == NULL ) {
		fprintf(stderr, "Couldn't set GL mode: %s\n", SDL_GetError());
		SDL_Quit();
		return;
	}

	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 320, 224, 0, 1, 0);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glOrtho(-W, W, -H, H, 1, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	textdata.reset();

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, textdata.texture);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	for(int i = 0;i < L;i++)
		level[i].init(&textdata);

	st = false;
	msglength = 0;
}

template<int W, int H, int L, int V>
int * Ygl<W,H,L,V>::quad(int l, int * v, unsigned int ** t, unsigned int w, unsigned int h, unsigned int * z, int f) {
	return level[l].quad(v,t,w,h,z,f);
}

template<int W, int H, int L, int V>
void Ygl<W,H,L,V>::cachedQuad(int l, int * v, int * t, unsigned int w, unsigned int h, int f) {
	level[l].cachedQuad(v,t,w,h,f);
}

template<int W, int H, int L, int V>
void Ygl<W,H,L,V>::render(void) {
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, W, textdata.yMax, GL_RGBA, GL_UNSIGNED_BYTE, textdata.texture);

	if(st) {
		int vertices [] = { 0, 0, 320, 0, 320, 224, 0, 224 };
		int text [] = { 0, 0, W, 0, W, H, 0, H };
		glVertexPointer(2, GL_INT, 0, vertices);
		glTexCoordPointer(2, GL_INT, 0, text);
		glDrawArrays(GL_QUADS, 0, 4);
	} else {
		for(int i = 0;i < L;i++)
			level[i].render();
	}

	glDisable(GL_TEXTURE_2D);

#ifndef _arch_dreamcast
	if (msglength > 0) {
		glColor3f(1.0f, 0.0f, 0.0f);
		glRasterPos2i(10, 22);
		for (int i = 0; i < msglength; i++) {
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, message[i]);
		}
		glColor3f(1, 1, 1);
	}
#endif

	SDL_GL_SwapBuffers();
}

template<int W, int H, int L, int V>
void Ygl<W,H,L,V>::reset(void) {
	glClear(GL_COLOR_BUFFER_BIT);

	textdata.reset();

	for(int i = 0;i < L;i++)
		level[i].reset();
	msglength = 0;
}

template<int W, int H, int L, int V>
void Ygl<W,H,L,V>::status(void) {
	cerr << "----" << endl;
	for(int i = 0;i < L;i++) {
		cerr << "priority=" << i << ", ";
		level[i].status();
	}
}

template<int W, int H, int L, int V>
void Ygl<W,H,L,V>::showTexture(void) {
	st = !st;
}

template<int W, int H, int L, int V>
void Ygl<W,H,L,V>::changeResolution(int w, int h) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, 1, 0);
}

template<int W, int H, int L, int V>
void Ygl<W,H,L,V>::onScreenDebugMessage(char *string, ...) {
	va_list arglist;

	va_start(arglist, string);
	vsprintf(message, string, arglist);
	va_end(arglist);
	msglength = strlen(message);
}

class YglCache {
private:
	map<unsigned long, int *, greater<unsigned long> > _cache;
public:
	int * isCached(unsigned long);
	void cache(unsigned long, int *);
	void reset(void);
};

#endif
