/*  Copyright 2003 Guillaume Duhamel
	Copyright 2004 Lawrence Sebald

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
#include "vdp2.hh"
#include "tree.h"

Vdp1::Vdp1(SaturnMemory *mem) : Memory(0xFF, 0x18) {
	satmem = mem;
	_stop = false;
	setWord(0x4, 0);
	vram = new Vdp1VRAM(0xFFFFF, 0xC0000);
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
    nbcom++;

    // First, process the command
    if (!(command & 0x4000)) { // if (!skip)
      switch (command & 0x000F) {
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
#ifdef DEBUG
	  cerr << "vdp1\t: Bad command: " << hex << setw(10) << command << endl;
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
      addr = vram->getWord(addr + 2) * 8;
      break;
    case 2: // CALL, call a subroutine
#ifdef DEBUG
      cerr << "CALL" << endl;
#endif
      returnAddr = addr;
      addr = vram->getWord(addr + 2) * 8;
      break;
    case 3: // RETURN, return from subroutine
#ifdef DEBUG
      cerr << "RETURN" << endl;
#endif
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

#ifndef _arch_dreamcast
	unsigned long textdata[hh * ww];
#else
	unsigned char *_textdata = (unsigned char *)memalign(32, (ww * hh) << 1);
	unsigned short *textdata = (unsigned short *)_textdata;
#endif

	unsigned short tx = 0;
	unsigned short ty = 0;
	unsigned short TX;
	short txinc = 1;
	short tyinc = 1;
	GLfloat u1 = 0.0f;
	GLfloat u2 = (GLfloat)w / ww;
	GLfloat v1 = 0.0f;
	GLfloat v2 = (GLfloat)h / hh;
	unsigned char dir = (vram->getWord(addr) & 0x30) >> 4;
	if (dir & 0x1) {
		//tx = w - 1;
		//txinc = -1;
		u1 = (GLfloat)w / ww;
		u2 = 0.0f;
	}
	if (dir & 0x2) {
		//ty = h - 1;
		//tyinc = -1;
		v1 = (GLfloat)h / hh;
		v2 = 0.0f;
	}

	unsigned long charAddr = vram->getWord(addr + 0x8) * 8;
	unsigned long dot, color;

	unsigned short colorMode = (vram->getWord(addr + 0x4) & 0x38) >> 3;
	bool SPD = ((vram->getWord(addr + 0x4) & 0x40) != 0);
	unsigned short colorBank = vram->getWord(addr + 0x6);
	vdp1Sprite sp = vram->getSprite(charAddr);
	unsigned long ca1 = charAddr;
	
	if(sp.vdp1_loc == 0)	{
#ifdef DEBUG
	cerr << "Making new sprite " << hex << charAddr << endl;
#endif
	switch(colorMode) {
	case 0:
#ifdef DEBUG
		cerr << "color mode 0 not implemented" << endl;
#endif
		break;
	case 1:
		colorBank *= 8;
		unsigned long temp;
		TX = tx;
		for(unsigned short i = 0;i < h;i++) {
			tx = TX;
			for(unsigned short j = 0;j < w;j += 2) {
				dot = vram->getByte(charAddr);
				temp = vram->getWord((dot >> 4) * 2 + colorBank);
#ifndef _arch_dreamcast
				color = 0xFF000000 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9;
#else
				color = (0x8000) | (temp & 0x1F) << 10 | (temp & 0x3E0) | (temp & 0x7C00) >> 10;
#endif
				if (((dot >> 4) == 0) && !SPD) textdata[ty * ww + tx] = 0;
				else textdata[ty * ww + tx] = color;
				tx += txinc;
				temp = vram->getWord((dot & 0xF) * 2 + colorBank);
#ifndef _arch_dreamcast
				color = 0xFF000000 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9;
#else
				color = (0x8000) | (temp & 0x1F) << 10 | (temp & 0x3E0) | (temp & 0x7C00) >> 10;
#endif
				if (((dot & 0xF) == 0) && !SPD) textdata[ty * ww + tx] = 0;
				else textdata[ty * ww + tx] = color;
				tx += txinc;
				charAddr += 1;
			}
			ty += tyinc;
		}
		break;
	case 2:
#ifdef DEBUG
		cerr << "color mode 2 not implemented" << endl;
#endif
		break;
	case 3:
#ifdef DEBUG
		cerr << "color mode 3 not implemented" << endl;
#endif
		break;
	case 4:
#ifdef DEBUG
		cerr << "color mode 4 not implemented" << endl;
#endif
		break;
	case 5:
		TX = tx;
		for(unsigned short i = 0;i < h;i++) {
			tx = TX;
			for(unsigned short j = 0;j < w;j++) {
				dot = vram->getWord(charAddr);
				charAddr += 2;
#ifndef _arch_dreamcast
				color = 0xFF000000 | (dot & 0x1F) << 3 | (dot & 0x3E0) << 6 | (dot & 0x7C00) << 9;
#else
				color = (0x8000) | (dot & 0x1F) << 10 | (dot & 0x3E0) | (dot & 0x7C00) >> 10;
#endif
				if ((dot == 0) && !SPD) textdata[ty * ww + tx] = 0;
				else textdata[ty * ww + tx] = color;
				tx += txinc;
			}
			ty += tyinc;
		}
	}
	
	if (sp.txr == 0) glGenTextures(1, &sp.txr);
	glBindTexture(GL_TEXTURE_2D, sp.txr);
	
#ifndef _arch_dreamcast
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ww, hh, 0, GL_RGBA, GL_UNSIGNED_BYTE, textdata);
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ARGB1555, ww, hh, 0, GL_ARGB1555, GL_UNSIGNED_BYTE, textdata);
#endif
	
	sp.vdp1_loc = ca1;
	vram->addSprite(sp);

#ifdef DEBUG
	cerr << "Created new sprite at " << hex << ca1 << endl;
#endif
	}		

	glEnable( GL_TEXTURE_2D );
	glBindTexture(GL_TEXTURE_2D, sp.txr);
	glBegin(GL_QUADS);
	glTexCoord2f(u1, v1); glVertex2f((float) x/160 - 1, 1 - (float) y/112);
	glTexCoord2f(u2, v1); glVertex2f((float) (x + w)/160 - 1, 1 - (float) y/112);
	glTexCoord2f(u2, v2); glVertex2f((float) (x + w)/160 - 1, 1 - (float) (y + h)/112);
	glTexCoord2f(u1, v2); glVertex2f((float) x/160 - 1, 1 - (float) (y + h)/112);
	glEnd();
	glDisable( GL_TEXTURE_2D );
}

void Vdp1::scaledSpriteDraw(unsigned long addr) {
#ifndef _arch_dreamcast
	unsigned short xy = vram->getWord(addr + 0xA);

	unsigned short x = localX + vram->getWord(addr + 0xC);
	unsigned short y = localY + vram->getWord(addr + 0xE);
	unsigned short w = ((xy >> 8) & 0x3F) * 8;
	unsigned short h = xy & 0xFF;
	unsigned short ww = power_of_two(w);
	unsigned short hh = power_of_two(h);
	unsigned short rw = vram->getWord(addr + 0x10);
	unsigned short rh = vram->getWord(addr + 0x12);

	unsigned short tx = 0;
	unsigned short ty = 0;
	unsigned short TX;
	short txinc = 1;
	short tyinc = 1;
	GLfloat u1 = 0.0f;
	GLfloat u2 = (GLfloat)w / ww;
	GLfloat v1 = 0.0f;
	GLfloat v2 = (GLfloat)h / hh;
	unsigned char dir = (vram->getWord(addr) & 0x30) >> 4;
	if (dir & 0x1) {
		//tx = w - 1;
		//txinc = -1;
		u1 = (GLfloat)w / ww;
		u2 = 0.0f;
	}
	if (dir & 0x2) {
		//ty = h - 1;
		//tyinc = -1;
		v1 = (GLfloat)h / hh;
		v2 = 0.0f;
	}
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
			
	unsigned long textdata[hh][ww];

	unsigned long charAddr = vram->getWord(addr + 0x8) * 8;
	unsigned long dot, color;

	unsigned short colorMode = (vram->getWord(addr + 0x4) & 0x38) >> 3;
	bool SPD = ((vram->getWord(addr + 0x4) & 0x40) != 0);
	unsigned short colorBank = vram->getWord(addr + 0x6);
	vdp1Sprite sp = vram->getSprite(charAddr);
	unsigned long ca1 = charAddr;
	
	if(sp.vdp1_loc == 0)	{
	switch(colorMode) {
	case 0:
#ifdef DEBUG
		cerr << "color mode 0 not implemented" << endl;
#endif
		break;
	case 1:
		colorBank *= 8;
		unsigned long temp;
		TX = tx;
		for(unsigned short i = 0;i < h;i++) {
			tx = TX;
			for(unsigned short j = 0;j < w;j += 2) {
				dot = vram->getByte(charAddr);
				temp = vram->getWord((dot >> 4) * 2 + colorBank);
				color = 0xFF000000 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9;
				if (((dot >> 4) == 0) && !SPD) textdata[ty][tx] = 0;
				else textdata[ty][tx] = color;
				tx += txinc;
				temp = vram->getWord((dot & 0xF) * 2 + colorBank);
				color = 0xFF000000 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9;
				if (((dot & 0xF) == 0) && !SPD) textdata[ty][tx] = 0;
				else textdata[ty][tx] = color;
				tx += txinc;
				charAddr += 1;
			}
			ty += tyinc;
		}
		break;
	case 2:
#ifdef DEBUG
		cerr << "color mode 2 not implemented" << endl;
#endif
		break;
	case 3:
#ifdef DEBUG
		cerr << "color mode 3 not implemented" << endl;
#endif
		break;
	case 4:
#ifdef DEBUG
		cerr << "color mode 4 not implemented" << endl;
#endif
		break;
	case 5:
		TX = tx;
		for(unsigned short i = 0;i < h;i++) {
			tx = TX;
			for(unsigned short j = 0;j < w;j++) {
				dot = vram->getWord(charAddr);
				charAddr += 2;
				color = 0xFF000000 | (dot & 0x1F) << 3 | (dot & 0x3E0) << 6 | (dot & 0x7C00) << 9;
				if ((dot == 0) && !SPD) textdata[ty][tx] = 0;
				else textdata[ty][tx] = color;
				tx += txinc;
			}
			ty += tyinc;
		}
	}
	
	if (sp.txr == 0) glGenTextures(1, &sp.txr);
	glBindTexture(GL_TEXTURE_2D, sp.txr);
	
#ifndef _arch_dreamcast
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ww, hh, 0, GL_RGBA, GL_UNSIGNED_BYTE, textdata);
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ARGB1555, ww, hh, 0, GL_ARGB1555, GL_UNSIGNED_BYTE, textdata);
#endif
	
	sp.vdp1_loc = ca1;
	vram->addSprite(sp);

#ifdef DEBUG
	cerr << "Created new scaled sprite at " << hex << ca1 << endl;
#endif
	}
	
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, sp.txr );
	glBegin(GL_QUADS);
	glTexCoord2f(u1, v1); glVertex2f((float) x/160 - 1, 1 - (float) y/112);
	glTexCoord2f(u2, v1); glVertex2f((float) (x + rw)/160 - 1, 1 - (float) y/112);
	glTexCoord2f(u2, v2); glVertex2f((float) (x + rw)/160 - 1, 1 - (float) (y + rh)/112);
	glTexCoord2f(u1, v2); glVertex2f((float) x/160 - 1, 1 - (float) (y + rh)/112);
	glEnd();
	glDisable( GL_TEXTURE_2D );
#endif
}

void Vdp1::distortedSpriteDraw(unsigned long addr) {
	unsigned short X[4];
	unsigned short Y[4];
	
	unsigned short xy = vram->getWord(addr + 0xA);

	unsigned short w = ((xy >> 8) & 0x3F) * 8;
	unsigned short h = xy & 0xFF;
	unsigned short ww = power_of_two(w);
	unsigned short hh = power_of_two(h);
	
#ifndef _arch_dreamcast
	unsigned long textdata[hh * ww];
#else
	unsigned char *_textdata = (unsigned char *)memalign(32, (ww * hh) << 1);
	unsigned short *textdata = (unsigned short *)_textdata;
#endif
	
	unsigned short tx = 0;
	unsigned short ty = 0;
	unsigned short TX;
	short txinc = 1;
	short tyinc = 1;
	GLfloat u1 = 0.0f;
	GLfloat u2 = (GLfloat)w / ww;
	GLfloat v1 = 0.0f;
	GLfloat v2 = (GLfloat)h / hh;
	unsigned char dir = (vram->getWord(addr) & 0x30) >> 4;
	if (dir & 0x1) {
		u1 = (GLfloat)w / ww;
		u2 = 0.0f;
	}
	if (dir & 0x2) {
		v1 = (GLfloat)h / hh;
		v2 = 0.0f;
	}
	
	unsigned long charAddr = vram->getWord(addr + 0x8) * 8;
	unsigned long dot, color;

	unsigned short colorMode = (vram->getWord(addr + 0x4) & 0x38) >> 3;
	bool SPD = ((vram->getWord(addr + 0x4) & 0x40) != 0);
	unsigned short colorBank = vram->getWord(addr + 0x6);
	vdp1Sprite sp = vram->getSprite(charAddr);
	unsigned long ca1 = charAddr;
	
	if(sp.vdp1_loc == 0)	{
#ifdef DEBUG
	cerr << "Making new sprite " << hex << charAddr << endl;
#endif
	switch(colorMode) {
	case 0:
#ifdef DEBUG
		cerr << "color mode 0 not implemented" << endl;
#endif
		break;
	case 1:
		colorBank *= 8;
		unsigned long temp;
		TX = tx;
		for(unsigned short i = 0;i < h;i++) {
			tx = TX;
			for(unsigned short j = 0;j < w;j += 2) {
				dot = vram->getByte(charAddr);
				temp = vram->getWord((dot >> 4) * 2 + colorBank);
#ifndef _arch_dreamcast
				color = 0xFF000000 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9;
#else
				color = (0x8000) | (temp & 0x1F) << 10 | (temp & 0x3E0) | (temp & 0x7C00) >> 10;
#endif
				if (((dot >> 4) == 0) && !SPD) textdata[ty * ww + tx] = 0;
				else textdata[ty * ww + tx] = color;
				tx += txinc;
				temp = vram->getWord((dot & 0xF) * 2 + colorBank);
#ifndef _arch_dreamcast
				color = 0xFF000000 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9;
#else
				color = (0x8000) | (temp & 0x1F) << 10 | (temp & 0x3E0) | (temp & 0x7C00) >> 10;
#endif
				if (((dot & 0xF) == 0) && !SPD) textdata[ty * ww + tx] = 0;
				else textdata[ty * ww + tx] = color;
				tx += txinc;
				charAddr += 1;
			}
			ty += tyinc;
		}
		break;
	case 2:
#ifdef DEBUG
		cerr << "color mode 2 not implemented" << endl;
#endif
		break;
	case 3:
#ifdef DEBUG
		cerr << "color mode 3 not implemented" << endl;
#endif
		break;
	case 4:
#ifdef DEBUG
		cerr << "color mode 4 not implemented" << endl;
#endif
		break;
	case 5:
		TX = tx;
		for(unsigned short i = 0;i < h;i++) {
			tx = TX;
			for(unsigned short j = 0;j < w;j++) {
				dot = vram->getWord(charAddr);
				charAddr += 2;
#ifndef _arch_dreamcast
				color = 0xFF000000 | (dot & 0x1F) << 3 | (dot & 0x3E0) << 6 | (dot & 0x7C00) << 9;
#else
				color = (0x8000) | (dot & 0x1F) << 10 | (dot & 0x3E0) | (dot & 0x7C00) >> 10;
#endif
				if ((dot == 0) && !SPD) textdata[ty * ww + tx] = 0;
				else textdata[ty * ww + tx] = color;
				tx += txinc;
			}
			ty += tyinc;
		}
	}
	
	if (sp.txr == 0) glGenTextures(1, &sp.txr);
	glBindTexture(GL_TEXTURE_2D, sp.txr);
	
#ifndef _arch_dreamcast
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ww, hh, 0, GL_RGBA, GL_UNSIGNED_BYTE, textdata);
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
#else
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ARGB1555, ww, hh, 0, GL_ARGB1555, GL_UNSIGNED_BYTE, textdata);
#endif
	
	sp.vdp1_loc = ca1;
	vram->addSprite(sp);

#ifdef DEBUG
	cerr << "Created new distorted sprite at " << hex << ca1 << endl;
#endif
	}
	
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
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, sp.txr);
		glBegin(GL_QUADS);
		glTexCoord2f(u1, v1); glVertex2f((float) X[0]/160 - 1, 1 - (float) Y[0]/112);
		glTexCoord2f(u2, v1); glVertex2f((float) X[1]/160 - 1, 1 - (float) Y[1]/112);
		glTexCoord2f(u2, v2); glVertex2f((float) X[2]/160 - 1, 1 - (float) Y[2]/112);
		glTexCoord2f(u1, v2); glVertex2f((float) X[3]/160 - 1, 1 - (float) Y[3]/112);
		glEnd();
		glDisable(GL_TEXTURE_2D);
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
  cerr << "vdp1\t: graaaaaaaaaaaaaa draw end" << endl;
#endif
  //Scu::sendDrawEnd();
}

Vdp1VRAM::Vdp1VRAM(unsigned long m, unsigned long size) : Memory(m, size)	{
}

Vdp1VRAM::~Vdp1VRAM()	{
	for(vector<vdp1Sprite>::iterator i = sprites.begin(); i != sprites.end(); i++)	{
		glDeleteTextures(1, &i->txr);
	}
	sprites.clear();
}

void Vdp1VRAM::setByte(unsigned long l, unsigned char d)	{
	for(vector<vdp1Sprite>::iterator i = sprites.begin(); i != sprites.end(); i++)	{
		if(i->vdp1_loc == l)	{
#ifdef DEBUG
			cerr << "Vdp1VRAM: Sprite at " << hex << l << " is modified." << endl;
#endif
			glDeleteTextures(1, &i->txr);
			sprites.erase(i);
			break;
		}
	}
	
	Memory::setByte(l, d);
}

void Vdp1VRAM::setWord(unsigned long l, unsigned short d)	{
	for(vector<vdp1Sprite>::iterator i = sprites.begin(); i != sprites.end(); i++)	{
		if(i->vdp1_loc == l)	{
#ifdef DEBUG
			cerr << "Vdp1VRAM: Sprite at " << hex << l << " is modified." << endl;
#endif
			glDeleteTextures(1, &i->txr);
			sprites.erase(i);
			break;
		}
	}
	
	Memory::setWord(l, d);
}


void Vdp1VRAM::setLong(unsigned long l, unsigned long d)	{
	for(vector<vdp1Sprite>::iterator i = sprites.begin(); i != sprites.end(); i++)	{
		if(i->vdp1_loc == l)	{
#ifdef DEBUG
			cerr << "Vdp1VRAM: Sprite at " << hex << l << " is modified." << endl;
#endif
			glDeleteTextures(1, &i->txr);
			sprites.erase(i);
			break;
		}
	}
	
	Memory::setLong(l, d);
}

vdp1Sprite Vdp1VRAM::getSprite(unsigned long l)	{
	vdp1Sprite blank = { 0, 0, 0 };
	for(vector<vdp1Sprite>::iterator i = sprites.begin(); i != sprites.end(); i++)	{
		if(i->vdp1_loc == l)	{
			return *i;
			break;
		}
	}
#ifdef DEBUG
	cerr << "Vdp1VRAM: getSprite: Didn't find sprite at " << hex << l << endl;
#endif
	return blank;
}

void Vdp1VRAM::addSprite(vdp1Sprite &spr)	{
	sprites.push_back(spr);
}
