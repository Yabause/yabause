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

#ifndef VDP2_HH
#define VDP2_HH

#include "memory.hh"
#include "cpu.hh"
#include "vdp1.hh"

class Scu;
class Vdp2;

class Vdp2Ram : public Memory {
public:
  Vdp2Ram(void) : Memory(0x80000) {}
};

class Vdp2Screen;

class Vdp2ColorRam : public Memory {
private:
  int mode;
public:
  Vdp2ColorRam(void) : Memory(0x1000) {}

  void setMode(int);
  unsigned long getColor(unsigned long, Vdp2Screen *);
};

class Vdp2Registers : public Memory {
private:
  Vdp2 *vdp2;
  SDL_Thread *vdp2Thread;
public:
  Vdp2Registers(Vdp2Ram *, Vdp2ColorRam *, Scu *, Vdp1 *);
  ~Vdp2Registers(void);

  Vdp2 *getVdp2(void);

  void setWord(unsigned long, unsigned short);

  unsigned short getTVSTAT(void);
  void setTVSTAT(unsigned short);
  unsigned short getCOAR(void);
  unsigned short getCOAG(void);
  unsigned short getCOAB(void);

  unsigned short getPRINA(void);
  unsigned short getPRINB(void);
  unsigned short getPRIR(void);
};

class Vdp2Screen {
protected:
  SDL_Surface *surface;
  Vdp2Registers *reg;
  Vdp2Ram *vram;
  Vdp2ColorRam *cram;

  virtual void init(void) = 0;
  virtual void planeAddr(int) = 0;
  void patternAddr(void);

  int mapWH;
  int planeW, planeH;
  int pageWH;
  int patternWH;
  int cellW, cellH;
  int patternDataSize;
  int specialFunction, flipFunction;
  unsigned long addr, charAddr, palAddr;
  int colorNumber;
  bool bitmap;
  unsigned short supplementData;
  int auxMode;
  bool enable;
  int x, y;
  int alpha;
  int colorOffset;
public:
  Vdp2Screen(Vdp2Registers *, Vdp2Ram *, Vdp2ColorRam *, SDL_Surface *);

  virtual int getPriority(void) = 0;
  virtual int getInnerPriority(void) = 0;
  static int comparePriority(const void *, const void *);

  int getAlpha(void);
  int getColorOffset(void);
  SDL_Surface *getSurface(void);
  
  void draw(void);
  void drawMap(void);
  void drawPlane(void);
  void drawPage(void);
  void drawPattern(void);
  void drawCell(void);
  void drawPixel(SDL_Surface *, Sint16, Sint16, Uint32);
};

class RBG0 : public Vdp2Screen {
private:
  void init(void);
  void planeAddr(int);
public:
  RBG0(Vdp2Registers *reg, Vdp2Ram *vram, Vdp2ColorRam *cram, SDL_Surface *s) : Vdp2Screen(reg, vram, cram, s) {}
  int getPriority(void);
  int getInnerPriority(void);
};

class NBG0 : public Vdp2Screen {
private:
  void init(void);
  void planeAddr(int);
public:
  NBG0(Vdp2Registers *reg, Vdp2Ram *vram, Vdp2ColorRam *cram, SDL_Surface *s) : Vdp2Screen(reg, vram, cram, s) {}
  int getPriority(void);
  int getInnerPriority(void);
};

class NBG1 : public Vdp2Screen {
private:
  void init(void);
  void planeAddr(int);
public:
  NBG1(Vdp2Registers *reg, Vdp2Ram *vram, Vdp2ColorRam *cram, SDL_Surface *s) : Vdp2Screen(reg, vram, cram, s) {}
  int getPriority(void);
  int getInnerPriority(void);
};

class NBG2 : public Vdp2Screen {
private:
  void init(void);
  void planeAddr(int);
public:
  NBG2(Vdp2Registers *reg, Vdp2Ram *vram, Vdp2ColorRam *cram, SDL_Surface *s) : Vdp2Screen(reg, vram, cram, s) {}
  int getPriority(void);
  int getInnerPriority(void);
};

class NBG3 : public Vdp2Screen {
private:
  void init(void);
  void planeAddr(int);
public:
  NBG3(Vdp2Registers *reg, Vdp2Ram *vram, Vdp2ColorRam *cram, SDL_Surface *s) : Vdp2Screen(reg, vram, cram, s) {}
  int getPriority(void);
  int getInnerPriority(void);
};

class Vdp2 : public Cpu {
private:
  Vdp2Registers *reg;
  Vdp2Ram *vram;
  Vdp2ColorRam *cram;
  Scu *scu;
  SDL_Surface *surface;
  SDL_Surface *GLSurface;
  SDL_Surface *logo;

  Vdp2Screen *screens[5];
  Vdp1 *vdp1;
public:
  Vdp2(Vdp2Registers *, Vdp2Ram *, Vdp2ColorRam *, Scu *, Vdp1 *);
  ~Vdp2(void);
  static void lancer(Vdp2 *);
  void executer(void);

  Vdp2Screen *getScreen(int);
  void sortScreens(void);
  void updateRam(void);

  void drawBackScreen(void);
  void priorityFunction(void);
  //void colorOffset(void);
};

#endif
