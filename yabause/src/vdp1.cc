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

#include "saturn.hh"
#include "vdp1.hh"
#include "intc.hh"
#include "exception.hh"
#include "scu.hh"
#include "timer.hh"
#include "SDL_gfxPrimitives.h"

Vdp1Registers::Vdp1Registers(Memory *mem, Scu *scu) : Memory(0x18) {
  vdp1 = new Vdp1(this, mem, scu);
}

Vdp1Registers::~Vdp1Registers(void) {
#if DEBUG
  cerr << "stopping vdp1\n";
#endif
  vdp1->stop();
#if DEBUG
  cerr << "vdp1 stopped\n";
#endif
  delete vdp1;
}

Vdp1 *Vdp1Registers::getVdp1(void) {
  return vdp1;
}

Vdp1::Vdp1(Vdp1Registers *reg, Memory *mem, Scu *s) {
  memory = mem;
  registers = reg;
  scu = s;

  _stop = false;
  registers->setWord(0x4, 0);
}

void Vdp1::stop(void) {
  _stop = true;
}

void Vdp1::setSurface(SDL_Surface *s) {
	vdp2Surface = s;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	vdp1Surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 400, 400, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
#else
	vdp1Surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 400, 400, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
#endif

}

void Vdp1::execute(unsigned long addr) {
  unsigned short command = memory->getWord(addr);
  unsigned long next;

  if (!registers->getWord(0x4)) return;

  // beginning of a frame (ST-013-R3-061694 page 53)
  // BEF <- CEF
  // CEF <- 0
  registers->setWord(0x10, (registers->getWord(0x10) & 2) >> 1);

  SDL_Rect cleanRect;
  cleanRect.x = registers->getWord(0x8) >> 8;
  cleanRect.y = registers->getWord(0x8) & 0xFF;
  cleanRect.w = (registers->getWord(0xA) >> 5) - cleanRect.x;
  cleanRect.h = (registers->getWord(0xA) & 0xFF) - cleanRect.y;

  SDL_FillRect(vdp1Surface, &cleanRect, SDL_MapRGBA(vdp1Surface->format, 0xFF, 0xFF, 0xFF, 0));
  
  while ((!(command & 0x8000))) { // FIXME

    // First, process the command
    if (!(command & 0x4000)) { // if (!skip)
      switch (command & 0x001F) {
	case 0: //cerr << "vdp1\t: normal sprite draw" << endl;
	  normalSpriteDraw(addr);
	  break;
	case 1: //cerr << "vdp1\t:  scaled sprite draw" << endl;
	  scaledSpriteDraw();
	  break;
	case 2: // distorted sprite draw
	  distortedSpriteDraw(addr);
	  break;
	case 4: // polygon draw
	  polygonDraw(addr);
	  break;
	case 5: // polyline draw
	  polylineDraw();
	  break;
	case 6: // line draw
	  lineDraw();
	  break;
	case 8: // user clipping coordinates
	  userClipping();
	  break;
	case 9: // system clipping coordinates
	  systemClipping(addr);
	  break;
	case 10: // local coordinate
	  localCoordinate(addr);
	  break;
	case 0x10: // draw end
	  drawEnd();
	  break;
	default:
#ifdef DEBUG
	  cerr << "vdp1\t: Bad command : " << hex << setw(10) << command << endl;
#endif
	  break;
      }
    }

    // Next, determine where to go next
    switch ((command & 0x3000) >> 12) {
    case 0: // NEXT, jump to following table
      addr += 0x20;
      break;
    case 1: // ASSIGN, jump to CMDLINK
      addr = memory->getWord(addr + 2) * 8;
      break;
    case 2: // CALL, call a subroutine
      returnAddr = addr;
      addr = memory->getWord(addr + 2) * 8;
#ifdef DEBUG
	  cerr << "CALL " << addr << endl;
#endif
      break;
    case 3: // RETURN, return from subroutine
      addr = returnAddr;
#ifdef DEBUG
      cerr << "RET" << endl;
#endif
      break;
    }
#ifndef _arch_dreamcast
    try { command = memory->getWord(addr); }
    catch (BadMemoryAccess ma) { return; }
#else
    command = memory->getWord(addr);
#endif
  }
  // drawing is finished, we set two bits at 1
  registers->setWord(0x10, registers->getWord(0x10) | 2);
#if DEBUG
  //cerr << "vdp1 : draw end\n";
#endif
  SDL_BlitSurface(vdp1Surface, NULL, vdp2Surface, NULL);
  scu->sendDrawEnd();
}

void Vdp1::normalSpriteDraw(unsigned long addr) {
  unsigned short xy = memory->getWord(addr + 0xA);

  SDL_Rect rect;
  rect.x = localX + memory->getWord(addr + 0xC);
  rect.y = localY + memory->getWord(addr + 0xE);
  rect.w = ((xy >> 8) & 0x3F) * 8;
  rect.h = xy & 0xFF;

  SDL_FillRect(vdp1Surface, &rect, SDL_MapRGBA(vdp1Surface->format, 0xFF, 0, 0, 0xFF));
#if DEBUG
  //cerr << "vdp1\t: normal sprite draw" << endl;
#endif
}

void Vdp1::scaledSpriteDraw(void) {
#if DEBUG
  cerr << "vdp1\t:  scaled sprite draw" << endl;
#endif
}

void Vdp1::distortedSpriteDraw(unsigned long addr) {
	Sint16 X[4];
	Sint16 Y[4];
	
	X[0] = localX + (memory->getWord(addr + 0x0C) );
	Y[0] = localY + (memory->getWord(addr + 0x0E) );
	X[1] = localX + (memory->getWord(addr + 0x10) );
	Y[1] = localY + (memory->getWord(addr + 0x12) );
	X[2] = localX + (memory->getWord(addr + 0x14) );
	Y[2] = localY + (memory->getWord(addr + 0x16) );
	X[3] = localX + (memory->getWord(addr + 0x18) );
	Y[3] = localY + (memory->getWord(addr + 0x1A) );

	if ((X[0] & 0x400) ||(Y[0] & 0x400) ||(X[1] & 0x400) ||(Y[1] & 0x400) ||(X[2] & 0x400) ||(Y[2] & 0x400) ||(X[3] & 0x400) ||(Y[3] & 0x400)) {
		//cerr << "don't know what to do" << endl;
	}
	else {
		filledPolygonColor(vdp1Surface, X, Y, 4, 0xFFFFFFFF);
	}
}

void Vdp1::polygonDraw(unsigned short addr) {
	Sint16 X[4];
	Sint16 Y[4];
	
	X[0] = localX + (memory->getWord(addr + 0x0C) );
	Y[0] = localY + (memory->getWord(addr + 0x0E) );
	X[1] = localX + (memory->getWord(addr + 0x10) );
	Y[1] = localY + (memory->getWord(addr + 0x12) );
	X[2] = localX + (memory->getWord(addr + 0x14) );
	Y[2] = localY + (memory->getWord(addr + 0x16) );
	X[3] = localX + (memory->getWord(addr + 0x18) );
	Y[3] = localY + (memory->getWord(addr + 0x1A) );

	unsigned short color = memory->getWord(addr + 0x6);
	unsigned short CMDPMOD = memory->getWord(addr + 0x4);

	unsigned char alpha = 0xFF;
	if ((CMDPMOD & 0x7) == 0x3) alpha = 0x80;

	if ((X[0] & 0x400) ||(Y[0] & 0x400) ||(X[1] & 0x400) ||(Y[1] & 0x400) ||(X[2] & 0x400) ||(Y[2] & 0x400) ||(X[3] & 0x400) ||(Y[3] & 0x400)) {
		//cerr << "don't know what to do" << endl;
	}
	else {
		filledPolygonRGBA(vdp1Surface, X, Y, 4, (color & 0x1F) << 3, (color & 0x3E0) >> 2, (color & 0x7C00) >> 7, alpha);
	}

	unsigned short cmdpmod = memory->getWord(addr + 0x4);
}

void Vdp1::polylineDraw(void) {
#if DEBUG
  cerr << "vdp1\t: polyline draw" << endl;
#endif
}

void Vdp1::lineDraw(void) {
#if DEBUG
  cerr << "vdp1\t: line draw" << endl;
#endif
}

void Vdp1::userClipping(void) {
#if DEBUG
  cerr << "vdp1\t: user clipping" << endl;
#endif
}

void Vdp1::systemClipping(unsigned short addr) {
  unsigned short CMDXC = memory->getWord(addr + 0x14);
  unsigned short CMDYC = memory->getWord(addr + 0x16);
#if DEBUG
  //cerr << "vdp1\t: system clipping x=" << CMDXC << " y=" << CMDYC << endl;
#endif
}

void Vdp1::localCoordinate(unsigned short addr) {
  localX = memory->getWord(addr + 0xC);
  localY = memory->getWord(addr + 0xE);
#if DEBUG
  //cerr << "vdp1\t: local coordinate x=" << CMDXA << " y=" << CMDYA << endl;
#endif
}

void Vdp1::drawEnd(void) {
#if DEBUG
  cerr << "vdp1\t: draw end" << endl;
#endif
  //Scu::sendDrawEnd();
}
