/*  Copyright 2003 Guillaume Duhamel

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

#ifndef VDP1_HH
#define VDP1_HH

#include "memory.hh"
#include "cpu.hh"
#ifdef _arch_dreamcast
#include "tree.h"
#endif
#ifdef WORDS_BIGENDIAN
#include <OpenGL/gl.h>
#else
#ifdef __WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

#include <vector>

class Scu;
class Vdp2;
class Vdp2ColorRam;

typedef struct {
	int vdp1_loc;
	bool dirty;
	GLuint txr;
} vdp1Sprite;

class Vdp1VRAM : public Memory	{
	private:
		/*tree_t sprTree;
		tree_node_t *__treeSearchFunc(unsigned long l, tree_node_t *p);*/
		vector<vdp1Sprite> sprites;		
		
	public:
		Vdp1VRAM(unsigned long m, unsigned long size);
		~Vdp1VRAM();
		
		void setByte(unsigned long l, unsigned char d);
		void setWord(unsigned long l, unsigned short d);
		void setLong(unsigned long l, unsigned long d);
		vdp1Sprite getSprite(unsigned long l);
		void addSprite(vdp1Sprite &sp);
};

class Vdp1 : public Cpu, public Memory {
private:
  SDL_Surface *surface;
  GLuint texture[1];
  SaturnMemory *satmem;
  Vdp1VRAM *vram;
  Vdp2 *vdp2regs;
  Vdp2ColorRam *cram;

  unsigned short localX;
  unsigned short localY;

  unsigned short returnAddr;
public:
  Vdp1(SaturnMemory *);
  void execute(unsigned long = 0);
  void stop(void);

  void setVdp2Ram(Vdp2 *, Vdp2ColorRam *);
  int getAlpha(void);
  int getColorOffset(void);
  SDL_Surface *getSurface(void);
  Memory *getVRam(void);

  void normalSpriteDraw(unsigned long);
  void scaledSpriteDraw(unsigned long);
  void distortedSpriteDraw(unsigned long);
  void polygonDraw(unsigned long);
  void polylineDraw(unsigned long);
  void lineDraw(unsigned long);
  void userClipping(unsigned long);
  void systemClipping(unsigned long);
  void localCoordinate(unsigned long);
  void drawEnd(unsigned long); // FIXME useless...
};

#endif
