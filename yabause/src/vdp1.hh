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

class Vdp1;
class Scu;

class Vdp1Registers : public Memory {
private:
  Vdp1 *vdp1;
  SDL_Thread *vdp1Thread;
public:
  Vdp1Registers(Memory *, Scu *);
  ~Vdp1Registers(void);
  Vdp1 *getVdp1(void);
};

class Vdp1 : public Cpu {
private:
  Memory *memory;
  Vdp1Registers *registers;
  Scu *scu;
  SDL_Surface *vdp1Surface;
  SDL_Surface *vdp2Surface;

  unsigned short localX;
  unsigned short localY;

  unsigned short returnAddr;
public:
  Vdp1(Vdp1Registers *, Memory *, Scu *);
  void execute(unsigned long = 0);
  void stop(void);

  void setSurface(SDL_Surface *);
  
  void normalSpriteDraw(unsigned long);
  void scaledSpriteDraw();
  void distortedSpriteDraw(unsigned long);
  void polygonDraw(unsigned short);
  void polylineDraw();
  void lineDraw();
  void userClipping();
  void systemClipping(unsigned short);
  void localCoordinate(unsigned short);
  void drawEnd();
};

#endif
