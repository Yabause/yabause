/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004 Lawrence Sebald
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

#include "saturn.hh"
#include "vdp1.hh"
#include "intc.hh"
#include "exception.hh"
#include "scu.hh"
#include "timer.hh"
#include "vdp2.hh"

Vdp1::Vdp1(SaturnMemory *mem) : Memory(0xFF, 0x18) {
	satmem = mem;
	_stop = false;
	vram = new Vdp1VRAM(0xFFFFF, 0xC0000);
        disptoggle = true;
        reset();
}

void Vdp1::stop(void) {
	_stop = true;
	delete vram;
}

void Vdp1::reset(void) {
  setWord(0x0, 0);
  setWord(0x2, 0);
  setWord(0x4, 0);

  // Clear Vram here
}

void Vdp1::execute(unsigned long addr) {
  unsigned short command = vram->getWord(addr);

  if (!getWord(0x4)) return;
  // If TVMD's DISP isn't set, don't render
  if (!(((Vdp2 *)satmem->getVdp2())->getWord(0) & 0x8000)) return;
  if (!disptoggle) return;

  // beginning of a frame (ST-013-R3-061694 page 53)
  // BEF <- CEF
  // CEF <- 0
  setWord(0x10, (getWord(0x10) & 2) >> 1);

  while (!(command & 0x8000)) {

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
	default:
#ifdef VDP1_DEBUG
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
#ifdef VDP1_DEBUG
      cerr << "vdp1\t: CALL" << endl;
#endif
      returnAddr = addr;
      addr = vram->getWord(addr + 2) * 8;
      break;
    case 3: // RETURN, return from subroutine
#ifdef VDP1_DEBUG
      cerr << "vdp1\t: RETURN" << endl;
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

void Vdp1::readCommand(unsigned long addr) {
	CMDCTRL = vram->getWord(addr);
	CMDLINK = vram->getWord(addr + 0x2);
	CMDPMOD = vram->getWord(addr + 0x4);
	CMDCOLR = vram->getWord(addr + 0x6);
	CMDSRCA = vram->getWord(addr + 0x8);
	CMDSIZE = vram->getWord(addr + 0xA);
	CMDXA = vram->getWord(addr + 0xC);
	CMDYA = vram->getWord(addr + 0xE);
	CMDXB = vram->getWord(addr + 0x10);
	CMDYB = vram->getWord(addr + 0x12);
	CMDXC = vram->getWord(addr + 0x14);
	CMDYC = vram->getWord(addr + 0x16);
	CMDXD = vram->getWord(addr + 0x18);
	CMDYD = vram->getWord(addr + 0x1A);
	CMDGRDA = vram->getWord(addr + 0x1B);
}

#ifndef _arch_dreamcast
void Vdp1::readTexture(vdp1Sprite *sp) {
#else
#endif
        unsigned long charAddr = CMDSRCA * 8;
        *sp = vram->getSprite(charAddr);
        if(sp->vdp1_loc == 0) {
#ifdef VDP1_DEBUG
                cerr << "Making new sprite " << hex << charAddr << endl;
#endif
	unsigned long dot;
	bool SPD = ((CMDPMOD & 0x40) != 0);
	unsigned long alpha = 0xFF;
	switch(CMDPMOD & 0x7) {
		case 0:
			alpha = 0xFF;
			break;
		case 3:
		        alpha = 0x80;
			break;
		default:
#ifdef VDP1_DEBUG
			cerr << "unimplemented color calculation: " << (CMDPMOD & 0x7) << endl;
#endif
			break;
	}
#ifndef _arch_dreamcast
        unsigned long textdata[hh * ww];
#else
        unsigned char *_textdata = (unsigned char *)memalign(32, (ww * hh) << 1);
        unsigned short *textdata = (unsigned short *)_textdata;
#endif



        unsigned long ca1 = charAddr;


	switch((CMDPMOD & 0x38) >> 3) {
	case 0:
        {
                // 4 bpp Bank mode
                unsigned long colorBank = CMDCOLR & 0xFFF0;
                // fix me
                Vdp2ColorRam *cram = (Vdp2ColorRam *)((Vdp2 *)satmem->getVdp2())->getCRam();
                // fix me
                int colorOffset = (((Vdp2 *)satmem->getVdp2())->getWord(0xE6) >> 4) & 0x7;

		for(unsigned short i = 0;i < h;i++) {
			unsigned short j;
			j = 0;
			while(j < w) {
				dot = vram->getByte(charAddr);

                                // Pixel 1
				if (((dot >> 4) == 0) && !SPD) textdata[i * ww + j] = 0;
                                else textdata[i * ww + j] = cram->getColor((dot >> 4) + colorBank, alpha, colorOffset);
				j += 1;

                                // Pixel 2
				if (((dot & 0xF) == 0) && !SPD) textdata[i * ww + j] = 0;
                                else textdata[i * ww + j] = cram->getColor((dot & 0xF) + colorBank, alpha, colorOffset);
				j += 1;
				charAddr += 1;
                        }
                }
		break;
        }
	case 1:
        {
                // 4 bpp LUT mode
		unsigned long temp;
                unsigned long colorLut = CMDCOLR * 8;
		for(unsigned short i = 0;i < h;i++) {
			unsigned short j;
			j = 0;
			while(j < w) {
				dot = vram->getByte(charAddr);

				if (((dot >> 4) == 0) && !SPD) textdata[i * ww + j] = 0;
                                else {
                                   temp = vram->getWord((dot >> 4) * 2 + colorLut);
                                   textdata[i * ww + j] = SAT2YAB1(alpha, temp);
                                }

				j += 1;

				if (((dot & 0xF) == 0) && !SPD) textdata[i * ww + j] = 0;
                                else {
                                   temp = vram->getWord((dot & 0xF) * 2 + colorLut);
                                   textdata[i * ww + j] = SAT2YAB1(alpha, temp);
                                }

				j += 1;
				charAddr += 1;
			}
		}
		break;
        }
	case 2:
        {
                // 8 bpp(64 color) Bank mode
                unsigned long colorBank = CMDCOLR & 0xFFC0;
                // fix me
                Vdp2ColorRam *cram = (Vdp2ColorRam *)((Vdp2 *)satmem->getVdp2())->getCRam();
                // fix me
                int colorOffset = (((Vdp2 *)satmem->getVdp2())->getWord(0xE6) >> 4) & 0x7;

		for(unsigned short i = 0;i < h;i++) {
                        for(unsigned short j = 0;j < w;j++) {
                                dot = vram->getByte(charAddr) & 0x3F;
                                charAddr++;

                                if ((dot == 0) && !SPD) textdata[i * ww + j] = 0;
                                else textdata[i * ww + j] = cram->getColor(dot + colorBank, alpha, colorOffset);
                        }
                }

		break;
        }
	case 3:
        {
                // 8 bpp(128 color) Bank mode
                unsigned long colorBank = CMDCOLR & 0xFF80;
                // fix me
                Vdp2ColorRam *cram = (Vdp2ColorRam *)((Vdp2 *)satmem->getVdp2())->getCRam();
                // fix me
                int colorOffset = (((Vdp2 *)satmem->getVdp2())->getWord(0xE6) >> 4) & 0x7;

		for(unsigned short i = 0;i < h;i++) {
                        for(unsigned short j = 0;j < w;j++) {
                                dot = vram->getByte(charAddr) & 0x7F;
                                charAddr++;

                                if ((dot == 0) && !SPD) textdata[i * ww + j] = 0;
                                else textdata[i * ww + j] = cram->getColor(dot + colorBank, alpha, colorOffset);
                        }
                }
		break;
        }
	case 4:
        {
                // 8 bpp(256 color) Bank mode
                unsigned long colorBank = CMDCOLR & 0xFF00;
                // fix me
                Vdp2ColorRam *cram = (Vdp2ColorRam *)((Vdp2 *)satmem->getVdp2())->getCRam();
                // fix me
                int colorOffset = (((Vdp2 *)satmem->getVdp2())->getWord(0xE6) >> 4) & 0x7;

		for(unsigned short i = 0;i < h;i++) {
                        for(unsigned short j = 0;j < w;j++) {
                                dot = vram->getByte(charAddr);
                                charAddr++;

                                if ((dot == 0) && !SPD) textdata[i * ww + j] = 0;
                                else textdata[i * ww + j] = cram->getColor(dot + colorBank, alpha, colorOffset);
                        }
                }

		break;
        }
	case 5:
                // 16 bpp Bank mode
		for(unsigned short i = 0;i < h;i++) {
			for(unsigned short j = 0;j < w;j++) {
				dot = vram->getWord(charAddr);
				charAddr += 2;

				if ((dot == 0) && !SPD) textdata[i * ww + j] = 0;
                                else textdata[i * ww + j] = SAT2YAB1(alpha, dot);
			}
		}
                break;
	}
                if (sp->txr == 0) glGenTextures(1, &sp->txr);

                glBindTexture(GL_TEXTURE_2D, sp->txr);

#ifndef _arch_dreamcast
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ww, hh, 0, GL_RGBA, GL_UNSIGNED_BYTE, textdata);

                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
#else
                glTexImage2D(GL_TEXTURE_2D, 0, GL_ARGB1555, ww, hh, 0, GL_ARGB1555, GL_UNSIGNED_BYTE, textdata);
#endif

                sp->vdp1_loc = ca1;
                vram->addSprite(*sp);

#ifdef VDP1_DEBUG
                cerr << "Created new sprite at " << hex << ca1 << endl;
#endif
        }

}

Memory *Vdp1::getVRam(void) {
	return vram;
}

static int power_of_two(int input) {
	int value = 1;

	while ( value < input ) {
		value <<= 1;
	}
	return value;
}

void Vdp1::normalSpriteDraw(unsigned long addr) {
	readCommand(addr);

	short x = CMDXA + localX;
	short y = CMDYA + localY;
	w = ((CMDSIZE >> 8) & 0x3F) * 8;
	h = CMDSIZE & 0xFF;
	ww = power_of_two(w);
	hh = power_of_two(h);

	GLfloat u1 = 0.0f;
	GLfloat u2 = (GLfloat)w / ww;
	GLfloat v1 = 0.0f;
	GLfloat v2 = (GLfloat)h / hh;
	unsigned char dir = (CMDCTRL & 0x30) >> 4;
	if (dir & 0x1) {
		u1 = (GLfloat)w / ww;
		u2 = 0.0f;
	}
	if (dir & 0x2) {
		v1 = (GLfloat)h / hh;
		v2 = 0.0f;
	}

	vdp1Sprite sp;
	readTexture(&sp);

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
	readCommand(addr);

	short x = CMDXA + localX;
	short y = CMDYA + localY;
	w = ((CMDSIZE >> 8) & 0x3F) * 8;
	h = CMDSIZE & 0xFF;
	ww = power_of_two(w);
	hh = power_of_two(h);
	short rw = vram->getWord(addr + 0x10);
	short rh = vram->getWord(addr + 0x12);

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
	unsigned short ZP = (vram->getWord(addr) & 0xF00) >> 8;

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
			
	vdp1Sprite sp;
	readTexture(&sp);

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, sp.txr );
	glBegin(GL_QUADS);
	glTexCoord2f(u1, v1); glVertex2f((float) x/160 - 1, 1 - (float) y/112);
	glTexCoord2f(u2, v1); glVertex2f((float) (x + rw)/160 - 1, 1 - (float) y/112);
	glTexCoord2f(u2, v2); glVertex2f((float) (x + rw)/160 - 1, 1 - (float) (y + rh)/112);
	glTexCoord2f(u1, v2); glVertex2f((float) x/160 - 1, 1 - (float) (y + rh)/112);
	glEnd();
	glDisable( GL_TEXTURE_2D );
}

void Vdp1::distortedSpriteDraw(unsigned long addr) {
	readCommand(addr);
	
	w = ((CMDSIZE >> 8) & 0x3F) * 8;
	h = CMDSIZE & 0xFF;
	ww = power_of_two(w);
	hh = power_of_two(h);
	
	GLfloat u1 = 0.0f;
	GLfloat u2 = (GLfloat)w / ww;
	GLfloat v1 = 0.0f;
	GLfloat v2 = (GLfloat)h / hh;
	unsigned char dir = (CMDCTRL & 0x30) >> 4;
	if (dir & 0x1) {
		u1 = (GLfloat)w / ww;
		u2 = 0.0f;
	}
	if (dir & 0x2) {
		v1 = (GLfloat)h / hh;
		v2 = 0.0f;
	}

	vdp1Sprite sp;
	readTexture(&sp);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, sp.txr);
	glBegin(GL_QUADS);
	glTexCoord2f(u1, v1); glVertex2f((float) (CMDXA + localX)/160 - 1, 1 - (float) (CMDYA + localY)/112);
	glTexCoord2f(u2, v1); glVertex2f((float) (CMDXB + localX)/160 - 1, 1 - (float) (CMDYB + localY)/112);
	glTexCoord2f(u2, v2); glVertex2f((float) (CMDXC + localX)/160 - 1, 1 - (float) (CMDYC + localY)/112);
	glTexCoord2f(u1, v2); glVertex2f((float) (CMDXD + localX)/160 - 1, 1 - (float) (CMDYD + localY)/112);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void Vdp1::polygonDraw(unsigned long addr) {
	short X[4];
	short Y[4];
	
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

        if ((color & 0x8000) == 0) alpha = 0;

	int priority = ((Vdp2*) satmem->getVdp2())->getWord(0xF0) & 0x7;

	glColor4f((float) ((color & 0x1F) << 3) / 0xFF, (float) ((color & 0x3E0) >> 2) / 0xFF, (float) ((color & 0x7C00) >> 7) / 0xFF, alpha);
	glBegin(GL_QUADS);
	glVertex3f((float) X[0]/160 - 1, 1 - (float) Y[0]/112, priority);
	glVertex3f((float) X[1]/160 - 1, 1 - (float) Y[1]/112, priority);
	glVertex3f((float) X[2]/160 - 1, 1 - (float) Y[2]/112, priority);
	glVertex3f((float) X[3]/160 - 1, 1 - (float) Y[3]/112, priority);
	glEnd();
	glColor4f(1, 1, 1, 1);
}

void Vdp1::polylineDraw(unsigned long addr) {
#if VDP1_DEBUG
  cerr << "vdp1\t: polyline draw" << endl;
#endif
}

void Vdp1::lineDraw(unsigned long addr) {
#if VDP1_DEBUG
  cerr << "vdp1\t: line draw" << endl;
#endif
}

void Vdp1::userClipping(unsigned long addr) {
#if VDP1_DEBUG
  cerr << "vdp1\t: user clipping (unimplemented)" << endl;
#endif
}

void Vdp1::systemClipping(unsigned long addr) {
#if VDP1_DEBUG
  //cerr << "vdp1\t: system clipping (unimplemented)" << endl;
#endif
}

void Vdp1::localCoordinate(unsigned long addr) {
  localX = vram->getWord(addr + 0xC);
  localY = vram->getWord(addr + 0xE);
#if VDP1_DEBUG
  //cerr << "vdp1\t: local coordinate x=" << CMDXA << " y=" << CMDYA << endl;
#endif
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
#ifdef VDP1_DEBUG
			cerr << "Vdp1VRAM: Sprite at " << hex << l << " is modified." << endl;
#endif
                        //glDeleteTextures(1, &i->txr);
                        sprites.erase(i);
			break;
		}
	}
	
	Memory::setByte(l, d);
}

void Vdp1VRAM::setWord(unsigned long l, unsigned short d)	{
	for(vector<vdp1Sprite>::iterator i = sprites.begin(); i != sprites.end(); i++)	{
		if(i->vdp1_loc == l)	{
#ifdef VDP1_DEBUG
			cerr << "Vdp1VRAM: Sprite at " << hex << l << " is modified." << endl;
#endif
			//glDeleteTextures(1, &i->txr);
			sprites.erase(i);
			break;
		}
	}
	
	Memory::setWord(l, d);
}


void Vdp1VRAM::setLong(unsigned long l, unsigned long d)	{
	for(vector<vdp1Sprite>::iterator i = sprites.begin(); i != sprites.end(); i++)	{
		if(i->vdp1_loc == l)	{
#ifdef VDP1_DEBUG
			cerr << "Vdp1VRAM: Sprite at " << hex << l << " is modified." << endl;
#endif
			//glDeleteTextures(1, &i->txr);
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
#ifdef VDP1_DEBUG
	cerr << "Vdp1VRAM: getSprite: Didn't find sprite at " << hex << l << endl;
#endif
	return blank;
}

void Vdp1VRAM::addSprite(vdp1Sprite &spr)	{
	sprites.push_back(spr);
}

void Vdp1::draw(void) {
	execute(0);
}

int Vdp1::getPriority(void) {
	return ((Vdp2*) satmem->getVdp2())->getWord(0xF0) & 0x7; //FIXME
}

int Vdp1::getInnerPriority(void) {
	return 5;
}

void Vdp1::toggleDisplay(void) {
   disptoggle ^= true;
}

