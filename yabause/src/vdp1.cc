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
//#include "SDL_gfxPrimitives.h"
#include "vdp2.hh"

Vdp1::Vdp1(SaturnMemory *mem) : Memory(0x18) {
	satmem = mem;
	_stop = false;
	setWord(0x4, 0);
	//glGenTextures(1, &texture[0] );
	vram = new Memory(0xC0000);
}

void Vdp1::stop(void) {
	_stop = true;
	delete vram;
}

void Vdp1::execute(unsigned long addr) {
  unsigned short command = vram->getWord(addr);
  int nbcom = 0;

  if (!getWord(0x4)) return;

  // beginning of a frame (ST-013-R3-061694 page 53)
  // BEF <- CEF
  // CEF <- 0
  setWord(0x10, (getWord(0x10) & 2) >> 1);

  while (!(command & 0x8000)) {

    // First, process the command
    if (!(command & 0x4000)) { // if (!skip)
      switch (command & 0x001F) {
	case 0: // normal sprite draw
	  normalSpriteDraw(addr);
	  break;
	case 1: // scaled sprite draw
	  scaledSpriteDraw(addr);
	  break;
	case 2: // distorted sprite draw
	  distortedSpriteDraw(addr);
	  break;
	case 4: // polygon draw
	  polygonDraw(addr);
	  break;
	case 5: // polyline draw
	  polylineDraw(addr);
	  break;
	case 6: // line draw
	  lineDraw(addr);
	  break;
	case 8: // user clipping coordinates
	  userClipping(addr);
	  break;
	case 9: // system clipping coordinates
	  systemClipping(addr);
	  break;
	case 10: // local coordinate
	  localCoordinate(addr);
	  break;
	case 0x10: // draw end
	  drawEnd(addr);
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
      addr = vram->getWord(addr + 2) * 8;
      break;
    case 2: // CALL, call a subroutine
      returnAddr = addr;
      addr = vram->getWord(addr + 2) * 8;
      break;
    case 3: // RETURN, return from subroutine
      addr = returnAddr;
    }
#ifndef _arch_dreamcast
    try { command = vram->getWord(addr); }
    catch (BadMemoryAccess ma) { return; }
#else
    command = vram->getWord(addr);
#endif
  }
  // we set two bits to 1
  setWord(0x10, getWord(0x10) | 2);
  ((Scu *) satmem->getScu())->sendDrawEnd();
}

void Vdp1::setVdp2Ram(Vdp2 *r, Vdp2ColorRam *c) {
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

Memory *Vdp1::getVRam(void) {
	return vram;
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
	unsigned short xy = vram->getWord(addr + 0xA);

	unsigned short x = localX + vram->getWord(addr + 0xC);
	unsigned short y = localY + vram->getWord(addr + 0xE);
	unsigned short w = ((xy >> 8) & 0x3F) * 8;
	unsigned short h = xy & 0xFF;
	unsigned short ww = power_of_two(w);
	unsigned short hh = power_of_two(h);

        surface = SDL_CreateRGBSurface(SDL_SWSURFACE, ww, hh, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

	if ((vram->getWord(addr) & 0x30) != 0) cerr << "flip not implemented" << endl;

	unsigned long charAddr = vram->getWord(addr + 0x8) * 8;
	unsigned long dot, color;

	unsigned short colorMode = (vram->getWord(addr + 0x4) & 0x38) >> 3;
	unsigned short colorBank = vram->getWord(addr + 0x6);
	switch(colorMode) {
	case 0:
		for(unsigned short i = 0;i < h;i++) {
			for(unsigned short j = 0;j < w;j += 2) {
				dot = vram->getByte(charAddr);
				color = cram->getColor((colorBank & 0xF0) + (dot >> 4), (Vdp2Screen *) this);
				Vdp2Screen::drawPixel(surface, j, i, color);
				color = cram->getColor((colorBank & 0xF0) + (dot & 0xF), (Vdp2Screen *) this);
				Vdp2Screen::drawPixel(surface, j + 1, i, color);
				charAddr += 1;
			}
		}
		break;
	case 1:
		colorBank *= 8;
		unsigned long temp;
		for(unsigned short i = 0;i < h;i++) {
			for(unsigned short j = 0;j < w;j += 2) {
				dot = vram->getByte(charAddr);
				temp = vram->getWord((dot >> 4) * 2 + colorBank);
				color = (temp & 0x1F) << 27 | (temp & 0x7E0) << 14 | (temp & 0x7C00) << 1 | 0xFF;
				if ((dot >> 4) != 0) Vdp2Screen::drawPixel(surface, j, i, color);
				else Vdp2Screen::drawPixel(surface, j, i, 0);
				temp = vram->getWord((dot & 0xF) * 2 + colorBank);
				color = (temp & 0x1F) << 27 | (temp & 0x7E0) << 14 | (temp & 0x7C00) << 1 | 0xFF;
				if ((dot & 0xF) != 0) Vdp2Screen::drawPixel(surface, j + 1, i, color);
				else Vdp2Screen::drawPixel(surface, j, i, 0);
				charAddr += 1;
			}
		}
		break;
	case 2:
		cerr << "color mode 2 not implemented" << endl; break;
	case 3:
		cerr << "color mode 3 not implemented" << endl; break;
	case 4:
		cerr << "color mode 4 not implemented" << endl; break;
	case 5:
		for(unsigned short i = 0;i < h;i++) {
			for(unsigned short j = 0;j < w;j++) {
				dot = vram->getWord(charAddr);
				charAddr += 2;
				color = (dot & 0x1F) << 27 | (dot & 0x7E0) << 14 | (dot & 0x7C00) << 1 | 0xFF;
				if (dot != 0) Vdp2Screen::drawPixel(surface, j, i, color);
				else Vdp2Screen::drawPixel(surface, j, i, 0);
			}
		}
	}

	if (*texture == 0) glGenTextures(1, &texture[0] );
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

void Vdp1::scaledSpriteDraw(unsigned long addr) {
	unsigned short xy = vram->getWord(addr + 0xA);

	unsigned short x = localX + vram->getWord(addr + 0xC);
	unsigned short y = localY + vram->getWord(addr + 0xE);
	unsigned short w = ((xy >> 8) & 0x3F) * 8;
	unsigned short h = xy & 0xFF;
	unsigned short ww = power_of_two(w);
	unsigned short hh = power_of_two(h);
	unsigned short rw = vram->getWord(addr + 0x10);
	unsigned short rh = vram->getWord(addr + 0x12);

	if ((vram->getWord(addr) & 0x30) != 0) cerr << "flip not implemented" << endl;

	unsigned short ZP = (vram->getWord(addr) & 0xF00) >> 8;
	unsigned short tmp;

	switch(ZP & 0xC) {
		case 0x4: 
			  break;
		case 0x8: y = y - rh/2;
			  break;
		case 0xC: y = y - rh;
			  break;
	}
	switch(ZP & 0x3) {
		case 1: 
			break;
		case 2: x = x - rw/2;
			break;
		case 3: x = x - rw;
			break;
	}
			
        surface = SDL_CreateRGBSurface(SDL_SWSURFACE, ww, hh, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	//SDL_SetAlpha(surface,SDL_SRCALPHA,0xFF);

	unsigned long charAddr = vram->getWord(addr + 0x8) * 8;
	unsigned long dot, color;

	unsigned short colorMode = (vram->getWord(addr + 0x4) & 0x38) >> 3;
	unsigned short colorBank = vram->getWord(addr + 0x6);
	switch(colorMode) {
	case 0:
		for(unsigned short i = 0;i < h;i++) {
			for(unsigned short j = 0;j < w;j += 2) {
				dot = vram->getByte(charAddr);
				color = cram->getColor((colorBank & 0xF0) + (dot >> 4), (Vdp2Screen *) this);
				Vdp2Screen::drawPixel(surface, j, i, color);
				color = cram->getColor((colorBank & 0xF0) + (dot & 0xF), (Vdp2Screen *) this);
				Vdp2Screen::drawPixel(surface, j + 1, i, color);
				charAddr += 1;
			}
		}
		break;
	case 1:
		colorBank *= 8;
		unsigned long temp;
		for(unsigned short i = 0;i < h;i++) {
			for(unsigned short j = 0;j < w;j += 2) {
				dot = vram->getByte(charAddr);
				temp = vram->getWord((dot >> 4) * 2 + colorBank);
				color = (temp & 0x1F) << 27 | (temp & 0x7E0) << 14 | (temp & 0x7C00) << 1 | 0xFF;
				if ((dot >> 4) != 0) Vdp2Screen::drawPixel(surface, j, i, color);
				else Vdp2Screen::drawPixel(surface, j, i, 0);
				temp = vram->getWord((dot & 0xF) * 2 + colorBank);
				color = (temp & 0x1F) << 27 | (temp & 0x7E0) << 14 | (temp & 0x7C00) << 1 | 0xFF;
				if ((dot & 0xF) != 0) Vdp2Screen::drawPixel(surface, j + 1, i, color);
				else Vdp2Screen::drawPixel(surface, j, i, 0);
				charAddr += 1;
			}
		}
		break;
	case 2:
		cerr << "color mode 2 not implemented" << endl; break;
	case 3:
		cerr << "color mode 3 not implemented" << endl; break;
	case 4:
		cerr << "color mode 4 not implemented" << endl; break;
	case 5:
		for(unsigned short i = 0;i < h;i++) {
			for(unsigned short j = 0;j < w;j++) {
				dot = vram->getWord(charAddr);
				charAddr += 2;
				color = (dot & 0x1F) << 27 | (dot & 0x7E0) << 14 | (dot & 0x7C00) << 1 | 0xFF;
				if (dot != 0) Vdp2Screen::drawPixel(surface, j, i, color);
				else Vdp2Screen::drawPixel(surface, j, i, 0);
			}
		}
	}

	if (*texture == 0) glGenTextures(1, &texture[0] );
	glBindTexture(GL_TEXTURE_2D, texture[0] );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ww, hh, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, texture[0] );
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f((float) x/160 - 1, 1 - (float) y/112);
	glTexCoord2f((float) w / ww, 0); glVertex2f((float) (x + rw)/160 - 1, 1 - (float) y/112);
	glTexCoord2f((float) w / ww, (float) h / hh); glVertex2f((float) (x + rw)/160 - 1, 1 - (float) (y + rh)/112);
	glTexCoord2f(0, (float) h /  hh); glVertex2f((float) x/160 - 1, 1 - (float) (y + rh)/112);
	glEnd();
	glDisable( GL_TEXTURE_2D );

	SDL_FreeSurface(surface);
}

void Vdp1::distortedSpriteDraw(unsigned long addr) {
	unsigned short X[4];
	unsigned short Y[4];
	
	X[0] = localX + (vram->getWord(addr + 0x0C) );
	Y[0] = localY + (vram->getWord(addr + 0x0E) );
	X[1] = localX + (vram->getWord(addr + 0x10) );
	Y[1] = localY + (vram->getWord(addr + 0x12) );
	X[2] = localX + (vram->getWord(addr + 0x14) );
	Y[2] = localY + (vram->getWord(addr + 0x16) );
	X[3] = localX + (vram->getWord(addr + 0x18) );
	Y[3] = localY + (vram->getWord(addr + 0x1A) );

	if ((X[0] & 0x400) ||(Y[0] & 0x400) ||(X[1] & 0x400) ||(Y[1] & 0x400) ||(X[2] & 0x400) ||(Y[2] & 0x400) ||(X[3] & 0x400) ||(Y[3] & 0x400)) {
		//cerr << "don't know what to do" << endl;
	}
	else {
		glColor4f(1, 1, 1, 1);
		glBegin(GL_QUADS);
		glVertex2f((float) X[0]/160 - 1, 1 - (float) Y[0]/112);
		glVertex2f((float) X[1]/160 - 1, 1 - (float) Y[1]/112);
		glVertex2f((float) X[2]/160 - 1, 1 - (float) Y[2]/112);
		glVertex2f((float) X[3]/160 - 1, 1 - (float) Y[3]/112);
		glEnd();
		glColor4f(1, 1, 1, 1);
	}
}

void Vdp1::polygonDraw(unsigned long addr) {
	unsigned short X[4];
	unsigned short Y[4];
	
	X[0] = localX + (vram->getWord(addr + 0x0C) );
	Y[0] = localY + (vram->getWord(addr + 0x0E) );
	X[1] = localX + (vram->getWord(addr + 0x10) );
	Y[1] = localY + (vram->getWord(addr + 0x12) );
	X[2] = localX + (vram->getWord(addr + 0x14) );
	Y[2] = localY + (vram->getWord(addr + 0x16) );
	X[3] = localX + (vram->getWord(addr + 0x18) );
	Y[3] = localY + (vram->getWord(addr + 0x1A) );

	unsigned short color = vram->getWord(addr + 0x6);
	unsigned short CMDPMOD = vram->getWord(addr + 0x4);

	float alpha = 1;
	if ((CMDPMOD & 0x7) == 0x3) alpha = 0.5;

	if ((X[0] & 0x400) ||(Y[0] & 0x400) ||(X[1] & 0x400) ||(Y[1] & 0x400) ||(X[2] & 0x400) ||(Y[2] & 0x400) ||(X[3] & 0x400) ||(Y[3] & 0x400)) {
		//cerr << "don't know what to do" << endl;
	}
	else {
		/*
		if (((X[0] < X[1]) and (X[2] < X[3]) and (Y[0] < Y[2]) and (Y[1] < Y[3])) ||
				((X[0] > X[1]) and (X[2] > X[3]) and (Y[0] < Y[2]) and (Y[1] < Y[3])) ||
				((X[0] < X[1]) and (X[2] < X[3]) and (Y[0] > Y[2]) and (Y[1] > Y[3])) ||
				((X[0] > X[1]) and (X[2] > X[3]) and (Y[0] > Y[2]) and (Y[1] > Y[3])))
			cerr << "OK\n";
		else cerr << "NOK\n";
		*/

		glColor4f((float) ((color & 0x1F) << 3) / 0xFF, (float) ((color & 0x3E0) >> 2) / 0xFF, (float) ((color & 0x7C00) >> 7) / 0xFF, alpha);
		//glColor3f(.5, .5, .5);
		glBegin(GL_QUADS);
		glVertex2f((float) X[0]/160 - 1, 1 - (float) Y[0]/112);
		glVertex2f((float) X[1]/160 - 1, 1 - (float) Y[1]/112);
		glVertex2f((float) X[2]/160 - 1, 1 - (float) Y[2]/112);
		glVertex2f((float) X[3]/160 - 1, 1 - (float) Y[3]/112);
		glEnd();
		glColor4f(1, 1, 1, 1);
	}

	unsigned short cmdpmod = vram->getWord(addr + 0x4);
#if 0
	cerr << "vdp1\t: polygon draw (pmod=" << hex << cmdpmod << ")"
	  << " X0=" << X[0] << " Y0=" << Y[0]
	  << " X1=" << X[1] << " Y1=" << Y[1]
	  << " X2=" << X[2] << " Y2=" << Y[2]
	  << " X3=" << X[3] << " Y3=" << Y[3] << endl;
#endif
}

void Vdp1::polylineDraw(unsigned long addr) {
#if DEBUG
  cerr << "vdp1\t: polyline draw" << endl;
#endif
}

void Vdp1::lineDraw(unsigned long addr) {
#if DEBUG
  cerr << "vdp1\t: line draw" << endl;
#endif
}

void Vdp1::userClipping(unsigned long addr) {
#if DEBUG
  cerr << "vdp1\t: user clipping" << endl;
#endif
}

void Vdp1::systemClipping(unsigned long addr) {
  unsigned short CMDXC = vram->getWord(addr + 0x14);
  unsigned short CMDYC = vram->getWord(addr + 0x16);
#if DEBUG
  //cerr << "vdp1\t: system clipping x=" << CMDXC << " y=" << CMDYC << endl;
#endif
}

void Vdp1::localCoordinate(unsigned long addr) {
  localX = vram->getWord(addr + 0xC);
  localY = vram->getWord(addr + 0xE);
#if DEBUG
  //cerr << "vdp1\t: local coordinate x=" << CMDXA << " y=" << CMDYA << endl;
#endif
}

void Vdp1::drawEnd(unsigned long addr) {
#if DEBUG
  cerr << "vdp1\t: draw end" << endl;
#endif
  //Scu::sendDrawEnd();
}
