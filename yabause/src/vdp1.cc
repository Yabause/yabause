/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004 Lawrence Sebald
    Copyright 2004-2005 Theo Berkau

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
#include "vdp2.hh"

Vdp1::Vdp1(SaturnMemory *mem) : Memory(0xFF, 0x18) {
	satmem = mem;
	vram = new Memory(0xFFFFF, 0xC0000);
        disptoggle = true;
        reset();
}

void Vdp1::stop(void) {
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

  cache.reset();

  returnAddr = 0xFFFFFFFF;
  commandCounter = 0;

  if (!getWord(0x4)) return;
  // If TVMD's DISP isn't set, don't render

  if (!(vdp2reg->getWord(0) & 0x8000)) return;
  if (!disptoggle) return;

  // beginning of a frame (ST-013-R3-061694 page 53)
  // BEF <- CEF
  // CEF <- 0
  setWord(0x10, (getWord(0x10) & 2) >> 1);

  while (!(command & 0x8000) && commandCounter < 2000) { // fix me

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
      if (returnAddr == 0xFFFFFFFF)
         returnAddr = addr+0x20;

      addr = vram->getWord(addr + 2) * 8;
      break;
    case 3: // RETURN, return from subroutine
      if (returnAddr != 0xFFFFFFFF) {
         addr = returnAddr;
         returnAddr = 0xFFFFFFFF;
      }
      else
         addr += 0x20;
      break;
    }
#ifndef _arch_dreamcast
    try { command = vram->getWord(addr); }
    catch (BadMemoryAccess ma) { return; }
#else
    command = vram->getWord(addr);
#endif
    commandCounter++;    
  }
  // we set two bits to 1
  setWord(0x10, getWord(0x10) | 2);
  satmem->scu->sendDrawEnd();
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
void Vdp1::readTexture(unsigned int * textdata, unsigned int z) {
#else
#endif
        unsigned long charAddr = CMDSRCA * 8;
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

        unsigned long ca1 = charAddr;

	switch((CMDPMOD & 0x38) >> 3) {
	case 0:
        {
                // 4 bpp Bank mode
                unsigned long colorBank = CMDCOLR & 0xFFF0;
                int colorOffset = (vdp2reg->getWord(0xE6) >> 4) & 0x7;

		for(unsigned short i = 0;i < h;i++) {
			unsigned short j;
			j = 0;
			while(j < w) {
				dot = vram->getByte(charAddr);

                                // Pixel 1
				if (((dot >> 4) == 0) && !SPD) *textdata++ = 0;
                                else *textdata++ = cram->getColor((dot >> 4) + colorBank, alpha, colorOffset);
				j += 1;

                                // Pixel 2
				if (((dot & 0xF) == 0) && !SPD) *textdata++ = 0;
                                else *textdata++ = cram->getColor((dot & 0xF) + colorBank, alpha, colorOffset);
				j += 1;
				charAddr += 1;
                        }
			textdata += z;
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

				if (((dot >> 4) == 0) && !SPD) *textdata++ = 0;
                                else {
                                   temp = vram->getWord((dot >> 4) * 2 + colorLut);
                                   *textdata++ = SAT2YAB1(alpha, temp);
                                }

				j += 1;

				if (((dot & 0xF) == 0) && !SPD) *textdata++ = 0;
                                else {
                                   temp = vram->getWord((dot & 0xF) * 2 + colorLut);
                                   *textdata++ = SAT2YAB1(alpha, temp);
                                }

				j += 1;
				charAddr += 1;
			}
			textdata += z;
		}
		break;
        }
	case 2:
        {
                // 8 bpp(64 color) Bank mode
                unsigned long colorBank = CMDCOLR & 0xFFC0;
                int colorOffset = (vdp2reg->getWord(0xE6) >> 4) & 0x7;

		for(unsigned short i = 0;i < h;i++) {
                        for(unsigned short j = 0;j < w;j++) {
                                dot = vram->getByte(charAddr) & 0x3F;
                                charAddr++;

                                if ((dot == 0) && !SPD) *textdata++ = 0;
                                else *textdata++ = cram->getColor(dot + colorBank, alpha, colorOffset);
                        }
			textdata += z;
                }

		break;
        }
	case 3:
        {
                // 8 bpp(128 color) Bank mode
                unsigned long colorBank = CMDCOLR & 0xFF80;
                int colorOffset = (vdp2reg->getWord(0xE6) >> 4) & 0x7;

		for(unsigned short i = 0;i < h;i++) {
                        for(unsigned short j = 0;j < w;j++) {
                                dot = vram->getByte(charAddr) & 0x7F;
                                charAddr++;

                                if ((dot == 0) && !SPD) *textdata++ = 0;
                                else *textdata++ = cram->getColor(dot + colorBank, alpha, colorOffset);
                        }
			textdata += z;
                }
		break;
        }
	case 4:
        {
                // 8 bpp(256 color) Bank mode
                unsigned long colorBank = CMDCOLR & 0xFF00;
                int colorOffset = (vdp2reg->getWord(0xE6) >> 4) & 0x7;

		for(unsigned short i = 0;i < h;i++) {
                        for(unsigned short j = 0;j < w;j++) {
                                dot = vram->getByte(charAddr);
                                charAddr++;

                                if ((dot == 0) && !SPD) *textdata++ = 0;
                                else *textdata++ = cram->getColor(dot + colorBank, alpha, colorOffset);
                        }
			textdata += z;
                }

		break;
        }
	case 5:
                // 16 bpp Bank mode
		for(unsigned short i = 0;i < h;i++) {
			for(unsigned short j = 0;j < w;j++) {
				dot = vram->getWord(charAddr);
				charAddr += 2;

				if ((dot == 0) && !SPD) *textdata++ = 0;
                                else *textdata++ = SAT2YAB1(alpha, dot);
			}
			textdata += z;
		}
                break;
	}
}

void Vdp1::readPriority(void) {
	unsigned char SPCLMD = vdp2reg->getWord(0xE0);
	unsigned char sprite_register;

	priority = 7; /* if we don't know what to do with a sprite, we put it on top */

	if ((SPCLMD & 0x20) and (CMDCOLR & 0x8000)) { /* RGB data, use register 0 */
		priority = vdp2reg->getWord(0xF0) & 0x7;
	} else {
		unsigned char sprite_type = SPCLMD & 0xF;
		switch(sprite_type) {
			case 0:
				sprite_register = ((CMDCOLR & 0x8000) | (~CMDCOLR & 0x4000)) >> 14;
				priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
				break;
			case 1:
				sprite_register = ((CMDCOLR & 0xC000) | (~CMDCOLR & 0x2000)) >> 13;
				priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
				break;
			case 3:
				sprite_register = ((CMDCOLR & 0x4000) | (~CMDCOLR & 0x2000)) >> 13;
				priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
				break;
			case 5:
				sprite_register = ((CMDCOLR & 0x6000) | (~CMDCOLR & 0x1000)) >> 12;
				priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
				break;
                        case 6:
				sprite_register = ((CMDCOLR & 0x6000) | (~CMDCOLR & 0x1000)) >> 12;
				priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
                                break;
                        case 7:
				sprite_register = ((CMDCOLR & 0x6000) | (~CMDCOLR & 0x1000)) >> 12;
				priority = vdp2reg->getByte(0xF0 + sprite_register) & 0x7;
                                break;
			default:
                                cerr << "sprite type " << ((int) sprite_type) << " not implemented" << endl;
                                break;
		}
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

	unsigned char dir = (CMDCTRL & 0x30) >> 4;

	unsigned int * textdata;
	int vertices[8];
	unsigned int z;

        vertices[0] = (int)((float)x * vdp1wratio);
        vertices[1] = (int)((float)y * vdp1hratio);
        vertices[2] = (int)((float)(x + w) * vdp1wratio);
        vertices[3] = (int)((float)y * vdp1hratio);
        vertices[4] = (int)((float)(x + w) * vdp1wratio);
        vertices[5] = (int)((float)(y + h) * vdp1hratio);
        vertices[6] = (int)((float)x * vdp1wratio);
        vertices[7] = (int)((float)(y + h) * vdp1hratio);

	readPriority();
	int * c;
	unsigned long tmp = CMDSRCA;
	tmp <<= 16;
	tmp |= CMDCOLR;

        if (w > 0 && h > 1) {
		if ((c = cache.isCached(tmp)) != NULL) {
			satmem->vdp2_3->ygl.cachedQuad(priority, vertices, c, w, h, dir);
			return;
		} 
		c = satmem->vdp2_3->ygl.quad(priority, vertices, &textdata, w, h, &z, dir);
		cache.cache(tmp, c);

		readTexture(textdata, z);
        }
}

void Vdp1::scaledSpriteDraw(unsigned long addr) {
        short rw=0;
        short rh=0;

	readCommand(addr);

	short x = CMDXA + localX;
	short y = CMDYA + localY;
	w = ((CMDSIZE >> 8) & 0x3F) * 8;
	h = CMDSIZE & 0xFF;
	ww = power_of_two(w);
	hh = power_of_two(h);

        unsigned char dir = (CMDCTRL & 0x30) >> 4;

        // Setup Zoom Point
        switch ((CMDCTRL & 0xF00) >> 8) {
           case 0x0: // Only two coordinates
                     rw = CMDXC - x;
                     rh = CMDYC - y;
                     break;
           case 0x5: // Upper-left
                     rw = CMDXB;
                     rh = CMDYB;
                     break;
           case 0x6: // Upper-Center
                     rw = CMDXB;
                     rh = CMDYB;
                     x = x - rw/2;
                     break;
           case 0x7: // Upper-Right
                     rw = CMDXB;
                     rh = CMDYB;
                     x = x - rw;
                     break;
           case 0x9: // Center-left
                     rw = CMDXB;
                     rh = CMDYB;
                     y = y - rh/2;
                     break;
           case 0xA: // Center-center
                     rw = CMDXB;
                     rh = CMDYB;
                     x = x - rw/2;
                     y = y - rh/2;
                     break;
           case 0xB: // Center-right
                     rw = CMDXB;
                     rh = CMDYB;
                     x = x - rw;
                     y = y - rh/2;
                     break;
           case 0xC: // Lower-left
                     rw = CMDXB;
                     rh = CMDYB;
                     y = y - rh;
                     break;
           case 0xE: // Lower-center
                     rw = CMDXB;
                     rh = CMDYB;
                     x = x - rw/2;
                     y = y - rh;
                     break;
           case 0xF: // Lower-right
                     rw = CMDXB;
                     rh = CMDYB;
                     x = x - rw;
                     y = y - rh;
                     break;
           default: break;
        }

	unsigned int * textdata;
	int vertices[8];
	unsigned int z;

        vertices[0] = (int)((float)x * vdp1wratio);
        vertices[1] = (int)((float)y * vdp1hratio);
        vertices[2] = (int)((float)(x + rw) * vdp1wratio);
        vertices[3] = (int)((float)y * vdp1hratio);
        vertices[4] = (int)((float)(x + rw) * vdp1wratio);
        vertices[5] = (int)((float)(y + rh) * vdp1hratio);
        vertices[6] = (int)((float)x * vdp1wratio);
        vertices[7] = (int)((float)(y + rh) * vdp1hratio);

	readPriority();

	int * c;
	unsigned long tmp = CMDSRCA;
	tmp <<= 16;
	tmp |= CMDCOLR;

        if (w > 0 && h > 1) {
		if ((c = cache.isCached(tmp)) != NULL) {
			satmem->vdp2_3->ygl.cachedQuad(priority, vertices, c, w, h, dir);
			return;
		} 
		c = satmem->vdp2_3->ygl.quad(priority, vertices, &textdata, w, h, &z, dir);
		cache.cache(tmp, c);

		readTexture(textdata, z);
        }
}

void Vdp1::distortedSpriteDraw(unsigned long addr) {
	readCommand(addr);

	w = ((CMDSIZE >> 8) & 0x3F) * 8;
	h = CMDSIZE & 0xFF;
	ww = power_of_two(w);
	hh = power_of_two(h);
	
	unsigned char dir = (CMDCTRL & 0x30) >> 4;

	unsigned int * textdata;
	int vertices[8];
	unsigned int z;

        vertices[0] = (int)((float)(CMDXA + localX) * vdp1wratio);
        vertices[1] = (int)((float)(CMDYA + localY) * vdp1hratio);
        vertices[2] = (int)((float)(CMDXB + localX) * vdp1wratio);
        vertices[3] = (int)((float)(CMDYB + localY) * vdp1hratio);
        vertices[4] = (int)((float)(CMDXC + localX) * vdp1wratio);
        vertices[5] = (int)((float)(CMDYC + localY) * vdp1hratio);
        vertices[6] = (int)((float)(CMDXD + localX) * vdp1wratio);
        vertices[7] = (int)((float)(CMDYD + localY) * vdp1hratio);

	readPriority();

	int * c;
	unsigned long tmp = CMDSRCA;
	tmp <<= 16;
	tmp |= CMDCOLR;

        if (w > 0 && h > 1) {
		if ((c = cache.isCached(tmp)) != NULL) {
			satmem->vdp2_3->ygl.cachedQuad(priority, vertices, c, w, h, dir);
			return;
		} 
		c = satmem->vdp2_3->ygl.quad(priority, vertices, &textdata, w, h, &z, dir);
		cache.cache(tmp, c);

		readTexture(textdata, z);
        }
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

	unsigned char alpha = 0xFF;
	if ((CMDPMOD & 0x7) == 0x3) alpha = 0x80;

        if ((color & 0x8000) == 0) alpha = 0;

        int priority = vdp2reg->getWord(0xF0) & 0x7;

        unsigned int * textdata;
        int vertices[8];
        unsigned int z;

        vertices[0] = (int)((float)X[0] * vdp1wratio);
        vertices[1] = (int)((float)Y[0] * vdp1hratio);
        vertices[2] = (int)((float)X[1] * vdp1wratio);
        vertices[3] = (int)((float)Y[1] * vdp1hratio);
        vertices[4] = (int)((float)X[2] * vdp1wratio);
        vertices[5] = (int)((float)Y[2] * vdp1hratio);
        vertices[6] = (int)((float)X[3] * vdp1wratio);
        vertices[7] = (int)((float)Y[3] * vdp1hratio);

        satmem->vdp2_3->ygl.quad(priority, vertices, &textdata, 1, 1, &z, 0);

	*textdata = SAT2YAB1(alpha,color);

}

void Vdp1::polylineDraw(unsigned long addr) {
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

	unsigned char alpha = 0xFF;
	if ((CMDPMOD & 0x7) == 0x3) alpha = 0x80;

        if ((color & 0x8000) == 0) alpha = 0;

        int priority = vdp2reg->getWord(0xF0) & 0x7;

/*
	glColor4ub(((color & 0x1F) << 3), ((color & 0x3E0) >> 2), ((color & 0x7C00) >> 7), alpha);
        glBegin(GL_LINE_STRIP);
        glVertex2i(X[0], Y[0]);
        glVertex2i(X[1], Y[1]);
        glVertex2i(X[2], Y[2]);
        glVertex2i(X[3], Y[3]);
        glVertex2i(X[0], Y[0]);
	glEnd();
	glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
*/
}

void Vdp1::lineDraw(unsigned long addr) {
        short X[2];
        short Y[2];
	
	X[0] = localX + (vram->getWord(addr + 0x0C) );
	Y[0] = localY + (vram->getWord(addr + 0x0E) );
	X[1] = localX + (vram->getWord(addr + 0x10) );
	Y[1] = localY + (vram->getWord(addr + 0x12) );

	unsigned short color = vram->getWord(addr + 0x6);
	unsigned short CMDPMOD = vram->getWord(addr + 0x4);

	unsigned char alpha = 0xFF;
	if ((CMDPMOD & 0x7) == 0x3) alpha = 0x80;

        if ((color & 0x8000) == 0) alpha = 0;

        int priority = vdp2reg->getWord(0xF0) & 0x7;

/*
	glColor4ub(((color & 0x1F) << 3), ((color & 0x3E0) >> 2), ((color & 0x7C00) >> 7), alpha);
        glBegin(GL_LINES);
        glVertex2i(X[0], Y[0]);
        glVertex2i(X[1], Y[1]);
	glEnd();
	glColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
*/
}

void Vdp1::userClipping(unsigned long addr) {
#if VDP1_DEBUG
  cerr << "vdp1\t: user clipping (unimplemented)" << endl;
#endif
  userclipX1 = vram->getWord(addr + 0xC);
  userclipY1 = vram->getWord(addr + 0xE);
  userclipX2 = vram->getWord(addr + 0x14);
  userclipY2 = vram->getWord(addr + 0x16);
}

void Vdp1::systemClipping(unsigned long addr) {
#if VDP1_DEBUG
  //cerr << "vdp1\t: system clipping (unimplemented) " << endl;
#endif
  systemclipX1 = vram->getWord(addr + 0xC);
  systemclipY1 = vram->getWord(addr + 0xE);
  systemclipX2 = vram->getWord(addr + 0x14);
  systemclipY2 = vram->getWord(addr + 0x16);
}

void Vdp1::localCoordinate(unsigned long addr) {
  localX = vram->getWord(addr + 0xC);
  localY = vram->getWord(addr + 0xE);
#if VDP1_DEBUG
  //cerr << "vdp1\t: local coordinate x=" << CMDXA << " y=" << CMDYA << endl;
#endif
}

void Vdp1::draw(void) {
	execute(0);
}

int Vdp1::getPriority(void) {
        return vdp2reg->getWord(0xF0) & 0x7; //FIXME
}

int Vdp1::getInnerPriority(void) {
	return 5;
}

void Vdp1::toggleDisplay(void) {
   disptoggle ^= true;
}

void Vdp1::setTextureRatio(int vdp2widthratio, int vdp2heightratio) {
   float vdp1w;
   float vdp1h;

   // may need some tweaking

   // Figure out which vdp1 screen mode to use
   switch (Memory::getWord(0) & 7)
   {
      case 0:
      case 2:
      case 3:
          vdp1w=1;
          break;
      case 1:
          vdp1w=2;
          break;
      default:
          vdp1w=1;
          vdp1h=1;
          break;
   }

   // Is double-interlace enabled?
   if (Memory::getWord(2) & 0x8)
      vdp1h=2;

   vdp1wratio = (float)vdp2widthratio / vdp1w;
   vdp1hratio = (float)vdp2heightratio / vdp1w;
}

void Vdp1::setVdp2Ram(Vdp2 *v, Vdp2ColorRam *c) {
  vdp2reg = v;
  cram = c;
}

