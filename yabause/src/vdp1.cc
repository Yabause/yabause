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
#include <GL/gl.h>
#include "vdp2.hh"

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
	glGenTextures(1, texture );
}

void Vdp1::stop(void) {
  _stop = true;
}

void Vdp1::execute(unsigned long addr) {
  unsigned short command = memory->getWord(addr);
  int nbcom = 0;

  if (!registers->getWord(0x4)) return;

  // beginning of a frame (ST-013-R3-061694 page 53)
  // BEF <- CEF
  // CEF <- 0
  registers->setWord(0x10, (registers->getWord(0x10) & 2) >> 1);

  while (!(command & 0x8000)) {

    // First, process the command
    if (!(command & 0x4000)) { // if (!skip)
      switch (command & 0x001F) {
	case 0: // normal sprite draw
	  normalSpriteDraw(addr);
	  break;
	case 1: // scaled sprite draw
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
	  cerr << "vdp1\t: Bad command: " << hex << setw(10) << command << endl;
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
      break;
    case 3: // RETURN, return from subroutine
      addr = returnAddr;
    }
#ifndef _arch_dreamcast
    try { command = memory->getWord(addr); }
    catch (BadMemoryAccess ma) { return; }
#else
    command = memory->getWord(addr);
#endif
  }
  // we set two bits to 1
  registers->setWord(0x10, registers->getWord(0x10) | 2);
  scu->sendDrawEnd();
}

void Vdp1::setVdp2Ram(Vdp2Registers *r, Vdp2ColorRam *c) {
	vdp2regs = r;
	cram = c;
}

int Vdp1::getAlpha(void) {
	return 0xFF;
}

int Vdp1::getColorOffset(void) {
	return (vdp2regs->getWord(0xE6) & 0x70) >> 4;
}

SDL_Surface *Vdp1::getSurface(void) {
	return surface;
}

static int power_of_two(int input)
{
	int value = 1;

	while ( value < input ) {
		value <<= 1;
	}
	return value;
}

void Vdp1::normalSpriteDraw(unsigned long addr) {
	unsigned short xy = memory->getWord(addr + 0xA);

	unsigned short x = localX + memory->getWord(addr + 0xC);
	unsigned short y = localY + memory->getWord(addr + 0xE);
	unsigned short w = ((xy >> 8) & 0x3F) * 8;
	unsigned short h = xy & 0xFF;
	unsigned short ww = power_of_two(w);
	unsigned short hh = power_of_two(h);

        surface = SDL_CreateRGBSurface(SDL_SWSURFACE, ww, hh, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	//SDL_SetAlpha(surface,SDL_SRCALPHA,0xFF);

	unsigned long charAddr = memory->getWord(addr + 0x8) * 8;
	unsigned long dot, color;

	unsigned short colorMode = (memory->getWord(addr + 0x4) & 0x38) >> 3;
	unsigned short colorBank = memory->getWord(addr + 0x6);
	switch(colorMode) {
	case 0:
		for(unsigned short i = 0;i < h;i++) {
			for(unsigned short j = 0;j < w;j += 2) {
				dot = memory->getByte(charAddr);
				color = cram->getColor((colorBank & 0xF0) + (dot >> 4), (Vdp2Screen *) this);
				pixelColor(surface, j, i, color);
				color = cram->getColor((colorBank & 0xF0) + (dot & 0xF), (Vdp2Screen *) this);
				pixelColor(surface, j + 1, i, color);
				charAddr += 1;
			}
		}
		break;
	case 1:
		cerr << "color mode 1 not implemented" << endl; break;
	case 2:
		cerr << "color mode 2 not implemented" << endl; break;
	case 3:
		cerr << "color mode 3 not implemented" << endl; break;
	case 4:
		cerr << "color mode 4 not implemented" << endl; break;
	case 5:
		for(unsigned short i = 0;i < h;i++) {
			for(unsigned short j = 0;j < w;j++) {
				dot = memory->getWord(charAddr);
				charAddr += 2;
				color = (dot & 0x1F) << 27 | (dot & 0x7E0) << 14 | (dot & 0x7C00) << 1 | 0xFF;
				if (dot != 0) pixelColor(surface, j, i, color);
				else pixelColor(surface, j, i, 0);
			}
		}
	}

	glBindTexture(GL_TEXTURE_2D, texture[0] );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ww, hh, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture[0] );
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f((float) x/160 - 1, 1 - (float) y/112);
	glTexCoord2f((float) w / ww, 0); glVertex2f((float) (x + w)/160 - 1, 1 - (float) y/112);
	glTexCoord2f((float) w / ww, (float) h / hh); glVertex2f((float) (x + w)/160 - 1, 1 - (float) (y + h)/112);
	glTexCoord2f(0, (float) h /  hh); glVertex2f((float) x/160 - 1, 1 - (float) (y + h)/112);
	glEnd();
	glDisable( GL_TEXTURE_2D );

	SDL_FreeSurface(surface);
}

void Vdp1::scaledSpriteDraw(void) {
#if DEBUG
  cerr << "vdp1\t:  scaled sprite draw" << endl;
#endif
}

void Vdp1::distortedSpriteDraw(unsigned long addr) {
	unsigned short X[4];
	unsigned short Y[4];
	
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
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		glVertex2f((float) X[0]/160 - 1, 1 - (float) Y[0]/112);
		glVertex2f((float) X[1]/160 - 1, 1 - (float) Y[1]/112);
		glVertex2f((float) X[2]/160 - 1, 1 - (float) Y[2]/112);
		glVertex2f((float) X[3]/160 - 1, 1 - (float) Y[3]/112);
		glEnd();
	}
}

void Vdp1::polygonDraw(unsigned short addr) {
	unsigned short X[4];
	unsigned short Y[4];
	
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
		glColor3f((float) ((color & 0x1F) << 3) / 0xFF, (float) ((color & 0x3E0) >> 2) / 0xFF, (float) ((color & 0x7C00) >> 7) / 0xFF);
		glBegin(GL_QUADS);
		glVertex2f((float) X[0]/160 - 1, 1 - (float) Y[0]/112);
		glVertex2f((float) X[1]/160 - 1, 1 - (float) Y[1]/112);
		glVertex2f((float) X[2]/160 - 1, 1 - (float) Y[2]/112);
		glVertex2f((float) X[3]/160 - 1, 1 - (float) Y[3]/112);
		glEnd();
	}

	unsigned short cmdpmod = memory->getWord(addr + 0x4);
#if 0
	cerr << "vdp1\t: polygon draw (pmod=" << hex << cmdpmod << ")"
	  << " X0=" << X[0] << " Y0=" << Y[0]
	  << " X1=" << X[1] << " Y1=" << Y[1]
	  << " X2=" << X[2] << " Y2=" << Y[2]
	  << " X3=" << X[3] << " Y3=" << Y[3] << endl;
#endif
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
