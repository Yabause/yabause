/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004 Theo Berkau

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

#if 0
#include "vdp1.hh"
#endif
#include "memory.hh"
#include "ygl.hh"

class Scu;
class Vdp2;
class SaturnMemory;

class Vdp2Ram : public Memory {
public:
  Vdp2Ram(void) : Memory(0x7FFFF, 0x80000) {}
};

class Vdp2ColorRam : public Memory {
private:
  int mode;
public:
  Vdp2ColorRam(void) : Memory(0xFFF, 0x1000) {}

  unsigned short getWord(unsigned long);
  void setWord(unsigned long, unsigned short);

  void setMode(int);
  unsigned long getColor(unsigned long addr, int alpha, int colorOffset);
};

class VdpScreen {
protected:
	int cor;
	int cog;
	int cob;
public:
	virtual void draw(void) = 0;
};

class Vdp2Screen : public VdpScreen {
protected:
  unsigned int *surface;
  Vdp2 *reg;
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
  bool disptoggle;
  int x, y;
  int alpha;
  int colorOffset;
  bool transparencyEnable;

  int priority;
public:
  float Xst, Yst, Zst;
  float deltaXst, deltaYst;
  float deltaX, deltaY;
  float A, B, C, D, E, F;
  short Px, Py, Pz;
  short Cx, Cy, Cz;
  float Mx, My;
  float kx, ky;
  /*
  float KAst;
  float deltaKAst;
  float deltaKAx;
  */
  float coordIncX, coordIncY;

  //float widthRatio, heightRatio;
  long width, height;
  unsigned int twidth;

  Vdp2Screen(Vdp2 *, Vdp2Ram *, Vdp2ColorRam *, unsigned long *);

  void draw(void);
  void drawMap(void);
  void drawPlane(void);
  void drawPage(void);
  void drawPattern(void);
  void drawCell(void);
  //static void drawPixel(unsigned long *, Sint16, Sint16, Uint32);
  void toggleDisplay(void);
  void setTextureRatio(unsigned long, unsigned long);

  void readRotationTable(unsigned long addr);

  void setPriority(int);
};

class RBG0 : public Vdp2Screen {
private:
  void init(void);
  void planeAddr(int);
public:
  RBG0(Vdp2 *reg, Vdp2Ram *vram, Vdp2ColorRam *cram, unsigned long *s);
  void debugStats(char *, bool *);
};

class NBG0 : public Vdp2Screen {
private:
  void init(void);
  void planeAddr(int);
public:
  NBG0(Vdp2 *reg, Vdp2Ram *vram, Vdp2ColorRam *cram, unsigned long *s) : Vdp2Screen(reg, vram, cram, s) {}
  void debugStats(char *, bool *);
};

class NBG1 : public Vdp2Screen {
private:
  void init(void);
  void planeAddr(int);
public:
  NBG1(Vdp2 *reg, Vdp2Ram *vram, Vdp2ColorRam *cram, unsigned long *s) : Vdp2Screen(reg, vram, cram, s) {}
  void debugStats(char *, bool *);
};

class NBG2 : public Vdp2Screen {
private:
  void init(void);
  void planeAddr(int);
public:
  NBG2(Vdp2 *reg, Vdp2Ram *vram, Vdp2ColorRam *cram, unsigned long *s) : Vdp2Screen(reg, vram, cram, s) {}
  void debugStats(char *, bool *);
};

class NBG3 : public Vdp2Screen {
private:
  void init(void);
  void planeAddr(int);
public:
  NBG3(Vdp2 *reg, Vdp2Ram *vram, Vdp2ColorRam *cram, unsigned long *s) : Vdp2Screen(reg, vram, cram, s) {}
  void debugStats(char *, bool *);
};

class Vdp2 : public Memory {
private:
  Vdp2Ram *vram;
  Vdp2ColorRam *cram;
  unsigned long *surface;

  Vdp2Screen *rbg0;
  Vdp2Screen *nbg0;
  Vdp2Screen *nbg1;
  Vdp2Screen *nbg2;
  Vdp2Screen *nbg3;
  SaturnMemory *satmem;

  int fps;
  int frameCount;
  unsigned long ticks;
  bool fpstoggle;

public:
  Ygl<1024,1024,8,40000> ygl;
  YglCache cache;

  Vdp2(SaturnMemory *);
  ~Vdp2(void);

  void reset(void);

  Vdp2ColorRam *getCRam(void);
  Vdp2Ram *getVRam(void);

  void setWord(unsigned long, unsigned short);

  void updateRam(void);

  void drawBackScreen(void);
  void priorityFunction(void);
  //void colorOffset(void);
  void VBlankIN(void);
  void VBlankOUT(void);
  void HBlankIN(void);
  void HBlankOUT(void);

  VdpScreen *getRBG0(void);
  VdpScreen *getNBG0(void);
  VdpScreen *getNBG1(void);
  VdpScreen *getNBG2(void);
  VdpScreen *getNBG3(void);
  void setSaturnResolution(int width, int height);
  void setActualResolution(int width, int height);
  void onScreenDebugMessage(float x, float y, char *string, ...);
  void toggleFPS(void);
};

#endif
