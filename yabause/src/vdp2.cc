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

#include "vdp2.hh"
#include "saturn.hh"
#include "scu.hh"
#include "timer.hh"

#if HAVE_LIBSDL_IMAGE
#include <SDL/SDL_image.h>
#endif
/*
#include "SDL_gfxPrimitives.h"
#include "SDL_imageFilter.h"
*/

/****************************************/
/*					*/
/*		VDP2 Registers		*/
/*					*/
/****************************************/

void Vdp2::setWord(unsigned long addr, unsigned short val) {
  switch(addr) {
    case 0:
      Memory::setWord(addr, val);
      break;
    case 0xE:
      Memory::setWord(addr, val);
      updateRam();
      break;
    case 0xF8:
      Memory::setWord(addr, val);
      sortScreens();
      break;
    case 0xFA:
      Memory::setWord(addr, val);
      sortScreens();
      break;
    case 0xFC:
      Memory::setWord(addr, val);
      sortScreens();
      break;
    default:
      Memory::setWord(addr, val);
      break;
  }
}

/****************************************/
/*					*/
/*		VDP2 Color RAM		*/
/*					*/
/****************************************/

void Vdp2ColorRam::setMode(int v) {
	mode = v;
}

unsigned long Vdp2ColorRam::getColor(unsigned long addr, Vdp2Screen *screen) {
  int alpha, colorOffset;
  alpha = screen->getAlpha();
  colorOffset = screen->getColorOffset();

  switch(mode) {
  case 0: {
    addr *= 2; // merci Runik !
    addr += colorOffset * 0x200;
    unsigned long tmp = getWord(addr);
    return ((tmp & 0x1F) << 27) | ((tmp & 0x3E0) << 14) | (tmp & 0x7C00) << 1 | alpha;
    //return SDL_MapRGBA(screen->getSurface()->format, (tmp & 0x1F) << 27, (tmp & 0x3E0) << 14, (tmp & 0x7C00) << 1, alpha);
  }
  case 1: {
    addr *= 2; // merci Runik !
    addr += colorOffset * 0x400;
    unsigned long tmp = getWord(addr);
    return ((tmp & 0x1F) << 27) | ((tmp & 0x3E0) << 14) | (tmp & 0x7C00) << 1 | alpha;
    //return SDL_MapRGBA(screen->getSurface()->format, (tmp & 0x1F) << 27, (tmp & 0x3E0) << 14, (tmp & 0x7C00) << 1, alpha);
  }
  case 2: {
    addr *= 2; // merci Runik !
    addr += colorOffset * 0x200;
    unsigned long tmp1 = getWord(addr);
    unsigned long tmp2 = getWord(addr + 2);
    return  ((tmp2 & 0xFF) << 24) | ((tmp1 & 0xFF00) << 8) | ((tmp1 & 0xFF) << 8) | alpha;
    //return SDL_MapRGBA(screen->getSurface()->format, (tmp2 & 0xFF) << 24, (tmp1 & 0xFF00) << 8, (tmp2 & 0xFF) << 8, alpha);
  }
  }
}

/****************************************/
/*					*/
/*		VDP2 Screen		*/
/*					*/
/****************************************/

Vdp2Screen::Vdp2Screen(Vdp2 *r, Vdp2Ram *v, Vdp2ColorRam *c, SDL_Surface *s) {
  reg = r;
  vram = v;
  cram = c;
  surface = s;
}

int Vdp2Screen::comparePriority(const void *arg1, const void *arg2) {
  Vdp2Screen **screen1 = (Vdp2Screen **) arg1;
  Vdp2Screen **screen2 = (Vdp2Screen **) arg2;
  int s1 = (*screen1)->getPriority();
  int is1 = (*screen1)->getInnerPriority();
  int s2 = (*screen2)->getPriority();
  int is2 = (*screen2)->getInnerPriority();

  if (s1 == s2) return is1 - is2;
  else return s1 - s2;
}

void Vdp2Screen::draw(void) {
	init();
	if (!enable || (getPriority() == 0)) return;

	if (bitmap) {
		drawCell();
	}
	else {
		drawMap();
	}
}

void Vdp2Screen::drawMap(void) {
	int X, Y;
	X = x;
	//for(int i = 0;i < mapWH;i++) {
		Y = y;
		x = X;
		//for(int j = 0;j < mapWH;j++) {
			y = Y;
			planeAddr(0); //planeAddr(mapWH * i + j);
			drawPlane();
		//}
	//}
}

void Vdp2Screen::drawPlane(void) {
	int X, Y;

	X = x;
	for(int i = 0;i < planeH;i++) {
		Y = y;
		x = X;
		for(int j = 0;j < planeW;j++) {
			y = Y;
			drawPage();
		}
	}
}

void Vdp2Screen::drawPage(void) {
	int X, Y;

	X = x;
	for(int i = 0;i < pageWH;i++) {
		Y = y;
		x = X;
		for(int j = 0;j < pageWH;j++) {
			y = Y;
			patternAddr();
			drawPattern();
		}
	}
}

void Vdp2Screen::patternAddr(void) {
	switch(patternDataSize) {
	case 1: {
		unsigned short tmp = vram->getWord(addr);
		addr += 2;
		specialFunction = supplementData & 0x300 >> 8;
    		switch(colorNumber) {
      		case 0: // in 16 colors
			palAddr = ((tmp & 0xF000) >> 12) | ((supplementData & 0xE0) >> 1);
        		break;
      		default: // not in 16 colors
			palAddr = (tmp & 0x7000) >> 8;
			break;
    		}
    		switch(auxMode) {
    		case 0:
      			flipFunction = (tmp & 0xC00) >> 10;
      			switch(patternWH) {
      			case 1:
				charAddr = (tmp & 0x3FF) | ((supplementData & 0x1F) << 10);
				break;
      			case 2:
				charAddr = ((tmp & 0x3FF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x1C) << 10);
				break;
      			}
      			break;
    		case 1:
      			flipFunction = 0;
      			switch(patternWH) {
      			case 1:
				charAddr = (tmp & 0xFFF) | ((supplementData & 0x1C) << 10);
				break;
      			case 4:
				charAddr = ((tmp & 0xFFF) << 2) |  (supplementData & 0x3) | ((supplementData & 0x10) << 10);
				break;
      			}
      			break;
    		}
    		break;
	}
  	case 2: {
    		unsigned short tmp1 = vram->getWord(addr);
    		addr += 2;
    		unsigned short tmp2 = vram->getWord(addr);
    		addr += 2;
    		charAddr = tmp2 & 0x7FFF;
    		flipFunction = (tmp1 & 0xC000) >> 14;
    		palAddr = (tmp1 & 0x7F) << 4;
    		specialFunction = (tmp1 & 0x3000) >> 12;
    		break;
	}
	}
	if (!(reg->getWord(0x6) & 0x8000)) charAddr &= 0x3FFF;
	charAddr *= 0x20; // selon Runik
}

void Vdp2Screen::drawPattern(void) {
	int X, Y;
	int xEnd, yEnd;

	if(flipFunction & 0x1) { // vertical flip
		x += patternWH * 8 - 1;
    		xEnd = patternWH * 8 + 1;
  	}
 	else {
    		xEnd = 0;
  	}

  	if(flipFunction & 0x2) { // horizontal flip
    		y += patternWH * 8 - 1;
    		yEnd = patternWH * 8 + 1;
  	}
  	else {
    		yEnd = 0;
  	}

	X = x;
	for(int i = 0;i < patternWH;i++) {
		Y = y;
		x = X;
		for(int j = 0;j < patternWH;j++) {
			y = Y;
			drawCell();
		}
	}
	x += xEnd;
	y += yEnd;
}

void Vdp2Screen::drawCell(void) {
  unsigned long color;
  int X;
  int xInc, yInc;

  if(flipFunction & 0x1) { // vertical flip
    xInc = -1;
  }
  else {
    xInc = 1;
  }

  if(flipFunction & 0x2) { // horizontal flip
    yInc = -1;
  }
  else {
    yInc = 1;
  }

  switch(colorNumber) {
    case 0:
      X = x;
      for(int i = 0;i < cellH;i++) {
	x = X;
	for(int j = 0;j < cellW;j+=4) {
	  unsigned short dot = vram->getWord(charAddr);
	  charAddr += 2;
	  if (!(dot & 0xF000) && transparencyEnable) color = 0x00000000;
	  else color = cram->getColor((palAddr << 4) | ((dot & 0xF000) >> 12), this);
	  //pixelColor(surface, x, y, color);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	  if (!(dot & 0xF00) && transparencyEnable) color = 0x00000000;
	  else color = cram->getColor((palAddr << 4) | ((dot & 0xF00) >> 8), this);
	  //pixelColor(surface, x, y, color);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	  if (!(dot & 0xF0) && transparencyEnable) color = 0x00000000;
	  else color = cram->getColor((palAddr << 4) | ((dot & 0xF0) >> 4), this);
	  //pixelColor(surface, x, y, color);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	  if (!(dot & 0xF) && transparencyEnable) color = 0x00000000;
	  else color = cram->getColor((palAddr << 4) | (dot & 0xF), this);
	  //pixelColor(surface, x, y, color);
	  drawPixel(surface, x, y, color);
	  x += xInc;
	}
	y += yInc;
      }
      break;
    case 1:
      X = x;
      for(int i = 0;i < cellH;i++) {
	x = X;
	for(int j = 0;j < cellW;j+=2) {
	  unsigned short dot = vram->getWord(charAddr);
	  charAddr += 2;
	  if (!(dot & 0xFF00) && transparencyEnable) color = 0x00000000;
	  else color = cram->getColor((palAddr << 4) | ((dot & 0xFF00) >> 8), this);
	  drawPixel(surface, x, y, color);
	  //pixelColor(surface, x, y, color);
	  x += xInc;
	  if (!(dot & 0xFF) && transparencyEnable) color = 0x00000000;
	  else color = cram->getColor((palAddr << 4) | (dot & 0xFF), this);
	  drawPixel(surface, x, y, color);
	  //pixelColor(surface, x, y, color);
	  x += xInc;
	}
	y += yInc;
      }
      break;
    case 2:
      X = x;
      for(int i = 0;i < cellH;i++) {
	x = X;
	for(int j = 0;j < cellW;j++) {
	  unsigned short dot = vram->getWord(charAddr);
	  if ((dot == 0) && transparencyEnable) color = 0x00000000;
	  else color = cram->getColor(dot, this);
	  charAddr += 2;
	  drawPixel(surface, x, y, color);
	  //pixelColor(surface, x, y, color);
	  x += xInc;
	}
	y += yInc;
      }
      break;
    case 3:
      X = x;
      for(int i = 0;i < cellH;i++) {
	x = X;
	for(int j = 0;j < cellW;j++) {
	  unsigned short dot = vram->getWord(charAddr);
	  charAddr += 2;
	  if ((dot & 0x8000) && transparencyEnable) color = 0x00000000; //pixelRGBA(surface, x, y, 0, 0, 0, 0);
	  else color = (dot & 0x1F) << 27 | (dot & 0xCE0) << 14 | (dot & 0x7C00) << 1 | 0xFF;
		  //pixelRGBA(surface, x, y, (dot & 0x1F) << 3, (dot & 0xCE0) >> 2, (dot & 0x7C00) >> 7, 0xFF);
	  drawPixel(surface, x, y, color);
	  //pixelColor(surface, x, y, color);
	  x += xInc;
	}
	y += yInc;
      }
      break;
    case 4:
      X = x;
      for(int i = 0;i < cellH;i++) {
	x = X;
	for(int j = 0;j < cellW;j++) {
	  unsigned short dot1 = vram->getWord(charAddr);
	  charAddr += 2;
	  unsigned short dot2 = vram->getWord(charAddr);
	  charAddr += 2;
	  if ((dot1 & 0x8000) && transparencyEnable) color = 0x00000000; //pixelRGBA(surface, x, y, 0, 0, 0, 0);
	  else color = ((dot2 & 0xFF) << 24) | ((dot2 & 0xFF00) << 8) | ((dot1 & 0xFF) << 8) | 0xFF;
		  //pixelRGBA(surface, x, y, dot2 & 0xFF, (dot2 & 0xFF00) >> 8, dot1 & 0xFF, 0xFF);
	  drawPixel(surface, x, y, color);
	  //pixelColor(surface, x, y, color);
	  x += xInc;
	}
	y += yInc;
      }
      break;
  }
}

// The drawPixel function comes from SDL_gfx

#define clip_xmin(surface) surface->clip_rect.x
#define clip_xmax(surface) surface->clip_rect.x+surface->clip_rect.w-1
#define clip_ymin(surface) surface->clip_rect.y
#define clip_ymax(surface) surface->clip_rect.y+surface->clip_rect.h-1

void Vdp2Screen::drawPixel(SDL_Surface *surface, Sint16 x, Sint16 y, Uint32 tmpcolor) {
	Uint8 alpha;
	Uint32 color;
	int result = 0;
	
	alpha = tmpcolor & 0x000000ff;
	color = SDL_MapRGBA(surface->format, (tmpcolor & 0xff000000) >> 24, (tmpcolor & 0x00ff0000) >> 16, (tmpcolor & 0x0000ff00) >> 8, alpha);
	
	Uint32 Rmask = surface->format->Rmask,
	       Gmask = surface->format->Gmask,
	       Bmask = surface->format->Bmask,
	       Amask = surface->format->Amask;
	Uint32 R, G, B, A = 0;
	                                                                                                                                                  
	if (x >= clip_xmin(surface) && x <= clip_xmax(surface) && y >= clip_ymin(surface) && y <= clip_ymax(surface)) {
		switch (surface->format->BytesPerPixel) {
			case 1:{
				if (alpha == 255) {
					*((Uint8 *) surface->pixels + y * surface->pitch + x) = color;
				} else {
				        Uint8 *pixel = (Uint8 *) surface->pixels + y * surface->pitch + x;

				        Uint8 dR = surface->format->palette->colors[*pixel].r;
				        Uint8 dG = surface->format->palette->colors[*pixel].g;
				        Uint8 dB = surface->format->palette->colors[*pixel].b;
				        Uint8 sR = surface->format->palette->colors[color].r;
				        Uint8 sG = surface->format->palette->colors[color].g;
				        Uint8 sB = surface->format->palette->colors[color].b;
				                         
				        dR = dR + ((sR - dR) * alpha >> 8);
				        dG = dG + ((sG - dG) * alpha >> 8);
				        dB = dB + ((sB - dB) * alpha >> 8);
														                                                                                                                                                 
				        *pixel = SDL_MapRGB(surface->format, dR, dG, dB);
				}
			}
			break;
			case 2:{
				if (alpha == 255) {
					*((Uint16 *) surface->pixels + y * surface->pitch / 2 + x) = color;
				 } else {
				        Uint16 *pixel = (Uint16 *) surface->pixels + y * surface->pitch / 2 + x;
				        Uint32 dc = *pixel;
									                                                                                                                                                 
				        R = ((dc & Rmask) + (((color & Rmask) - (dc & Rmask)) * alpha >> 8)) & Rmask;
				        G = ((dc & Gmask) + (((color & Gmask) - (dc & Gmask)) * alpha >> 8)) & Gmask;
				        B = ((dc & Bmask) + (((color & Bmask) - (dc & Bmask)) * alpha >> 8)) & Bmask;
				        if (Amask)
						A = ((dc & Amask) + (((color & Amask) - (dc & Amask)) * alpha >> 8)) & Amask;
				                                                                                   
					*pixel = R | G | B | A;
				}
			}
			break;
			case 3:{
				Uint8 *pix = (Uint8 *) surface->pixels + y * surface->pitch + x * 3;
				Uint8 rshift8 = surface->format->Rshift / 8;
				Uint8 gshift8 = surface->format->Gshift / 8;
				Uint8 bshift8 = surface->format->Bshift / 8;
				Uint8 ashift8 = surface->format->Ashift / 8;


				if (alpha == 255) {
		    			*(pix + rshift8) = color >> surface->format->Rshift;
		    			*(pix + gshift8) = color >> surface->format->Gshift;
		    			*(pix + bshift8) = color >> surface->format->Bshift;
		    			*(pix + ashift8) = color >> surface->format->Ashift;
				} else {
		    			Uint8 dR, dG, dB, dA = 0;
		    			Uint8 sR, sG, sB, sA = 0;

		    			pix = (Uint8 *) surface->pixels + y * surface->pitch + x * 3;

		    			dR = *((pix) + rshift8);
		    			dG = *((pix) + gshift8);
		    			dB = *((pix) + bshift8);
		    			dA = *((pix) + ashift8);

		    			sR = (color >> surface->format->Rshift) & 0xff;
		    			sG = (color >> surface->format->Gshift) & 0xff;
		    			sB = (color >> surface->format->Bshift) & 0xff;
		    			sA = (color >> surface->format->Ashift) & 0xff;

		    			dR = dR + ((sR - dR) * alpha >> 8);
		    			dG = dG + ((sG - dG) * alpha >> 8);
		    			dB = dB + ((sB - dB) * alpha >> 8);
		    			dA = dA + ((sA - dA) * alpha >> 8);

		    			*((pix) + rshift8) = dR;
		    			*((pix) + gshift8) = dG;
		    			*((pix) + bshift8) = dB;
		    			*((pix) + ashift8) = dA;
				}
	    		}
	    		break;

			case 4:{
				if (alpha == 255) {
		    			*((Uint32 *) surface->pixels + y * surface->pitch / 4 + x) = color;
				} else {
		    			Uint32 *pixel = (Uint32 *) surface->pixels + y * surface->pitch / 4 + x;
		    			Uint32 dc = *pixel;

		    			R = ((dc & Rmask) + (((color & Rmask) - (dc & Rmask)) * alpha >> 8)) & Rmask;
		    			G = ((dc & Gmask) + (((color & Gmask) - (dc & Gmask)) * alpha >> 8)) & Gmask;
		    			B = ((dc & Bmask) + (((color & Bmask) - (dc & Bmask)) * alpha >> 8)) & Bmask;
		    			if (Amask)
						A = ((dc & Amask) + (((color & Amask) - (dc & Amask)) * alpha >> 8)) & Amask;

		    			*pixel = R | G | B | A;
				}
	    		}
	   	 	break;
		}
    	}
}

int Vdp2Screen::getAlpha(void) {
	return alpha;
}

int Vdp2Screen::getColorOffset(void) {
	return colorOffset;
}

SDL_Surface *Vdp2Screen::getSurface(void) {
	return surface;
}

void RBG0::init(void) {
	enable = false;
}

void RBG0::planeAddr(int i) {
	addr = 0;
}

int RBG0::getPriority(void) {
	return (reg->getWord(0xFC) & 0x7);
}

int RBG0::getInnerPriority(void) {
	return 4;
}

void NBG0::init(void) {
	unsigned short patternNameReg = reg->getWord(0x30);
	unsigned short patternReg = reg->getWord(0x28);
	/* FIXME should start by checking if it's a normal
	 * or rotate scroll screen
	*/
	enable = reg->getWord(0x20) & 0x1;
	transparencyEnable = !(reg->getWord(0x20) & 0x100);
	x = - reg->getWord(0x70);
	y = - reg->getWord(0x74);

  	colorNumber = (patternReg & 0x70) >> 4;
	if(bitmap = patternReg & 0x2) {
		switch((patternReg & 0xC) >> 2) {
			case 0: cellW = 512;
				cellH = 256;
				break;
			case 1: cellW = 512;
				cellH = 512;
				break;
			case 2: cellW = 1024;
				cellH = 256;
				break;
			case 3: cellW = 1024;
				cellH = 512;
				break;
		}
		charAddr = (reg->getWord(0x3C) & 0x7) * 0x20000;
		palAddr = (reg->getWord(0x2C) & 0x7) << 4;
		flipFunction = 0;
		specialFunction = 0;
  	}
  	else {
		mapWH = 2;
  		switch(reg->getWord(0x3A) & 0x3) {
  			case 0: planeW = planeH = 1; break;
  			case 1: planeW = 2; planeH = 1; break;
  			case 2: planeW = planeH = 2; break;
  		}
  		if(patternNameReg & 0x8000) patternDataSize = 1;
  		else patternDataSize = 2;
  		if(patternReg & 0x1) patternWH = 2;
  		else patternWH = 1;
  		pageWH = 64/patternWH;
  		cellW = cellH = 8;
  		supplementData = patternNameReg & 0x3FF;
  		auxMode = (patternNameReg & 0x4000) >> 14;
  	}
	unsigned short colorCalc = reg->getWord(0xEC);
	if (colorCalc & 0x1) {
		alpha = ((~reg->getWord(0x108) & 0x1F) << 3) + 0x7;
	}
	else {
		alpha = 0xFF;
	}

	colorOffset = reg->getWord(0xE4) & 0x7;
}

void NBG0::planeAddr(int i) {
	unsigned long offset = (reg->getWord(0x3C) & 0x7) << 6;
	unsigned long tmp;
  	switch(i) {
    		case 0: tmp = offset | reg->getByte(0x41); break;
    		case 1: tmp = offset | reg->getByte(0x40); break;
    		case 2: tmp = offset | reg->getByte(0x43); break;
   		case 3: tmp = offset | reg->getByte(0x42); break;
  	}
  	int deca = planeH + planeW - 2;
  	int multi = planeH * planeW;
	//if (reg->getWord(0x6) & 0x8000) {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
	  		else addr = (tmp >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
  		}
	/*}
	else {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
  		}
	}*/
}

int NBG0::getPriority(void) {
  return reg->getByte(0xF9) & 0x7;
}

int NBG0::getInnerPriority(void) {
  return 3;
}

void NBG1::init(void) {
  	unsigned short patternNameReg = reg->getWord(0x32);
  	unsigned short patternReg = reg->getWord(0x28);

  	enable = reg->getWord(0x20) & 0x2;
	transparencyEnable = !(reg->getWord(0x20) & 0x200);
	x = - reg->getWord(0x80);
	y = - reg->getWord(0x84);
	
  	colorNumber = (patternReg & 0x3000) >> 12;
  	if(bitmap = patternReg & 0x200) {
		switch((patternReg & 0xC00) >> 10) {
			case 0: cellW = 512;
				cellH = 256;
				break;
			case 1: cellW = 512;
				cellH = 512;
				break;
			case 2: cellW = 1024;
				cellH = 256;
				break;
			case 3: cellW = 1024;
				cellH = 512;
				break;
		}
		charAddr = ((reg->getWord(0x3C) & 0x70) >> 4) * 0x20000;
		palAddr = (reg->getWord(0x2C) & 0x700) >> 4;
		flipFunction = 0;
		specialFunction = 0;
  	}
  	else {
  		mapWH = 2;
  		switch((reg->getWord(0x3A) & 0xC) >> 2) {
  			case 0: planeW = planeH = 1; break;
  			case 1: planeW = 2; planeH = 1; break;
  			case 2: planeW = planeH = 2; break;
  		}
  		if(patternNameReg & 0x8000) patternDataSize = 1;
  		else patternDataSize = 2;
  		if(patternReg & 0x100) patternWH = 2;
  		else patternWH = 1;
  		pageWH = 64/patternWH;
  		cellW = cellH = 8;
  		supplementData = patternNameReg & 0x3FF;
  		auxMode = (patternNameReg & 0x4000) >> 14;
	}

	unsigned short colorCalc = reg->getWord(0xEC);
	if (colorCalc & 0x2) {
		alpha = ((~reg->getWord(0x108) & 0x1F00) >> 5) + 0x7;
	}
	else {
		alpha = 0xFF;
	}

	colorOffset = (reg->getWord(0xE4) & 0x70) >> 4;
}

void NBG1::planeAddr(int i) {
  	unsigned long offset = (reg->getWord(0x3C) & 0x70) << 2;
  	unsigned long tmp;
  	switch(i) {
    		case 0: tmp = offset | reg->getByte(0x45); break;
    		case 1: tmp = offset | reg->getByte(0x44); break;
    		case 2: tmp = offset | reg->getByte(0x47); break;
    		case 3: tmp = offset | reg->getByte(0x46); break;
  	}
  	int deca = planeH + planeW - 2;
  	int multi = planeH * planeW;
	//if (reg->getWord(0x6) & 0x8000) {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
	  		else addr = (tmp >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
  		}
	/*}
	else {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
  		}
	}*/
}

int NBG1::getPriority(void) {
  return reg->getByte(0xF8) & 0x7;
}

int NBG1::getInnerPriority(void) {
  return 2;
}

void NBG2::init(void) {
	unsigned short patternNameReg = reg->getWord(0x34);
	unsigned short patternReg = reg->getWord(0x2A);

	enable = reg->getWord(0x20) & 0x4;
	transparencyEnable = !(reg->getWord(0x20) & 0x400);
	x = - reg->getWord(0x90);
	y = - reg->getWord(0x92);

  	colorNumber = (patternReg & 0x2) >> 1;
	bitmap = false; // NBG2 can only use cell mode
	
	mapWH = 2;
  	switch((reg->getWord(0x3A) & 0x30) >> 4) {
  		case 0: planeW = planeH = 1; break;
  		case 1: planeW = 2; planeH = 1; break;
  		case 2: planeW = planeH = 2; break;
  	}
  	if(patternNameReg & 0x8000) patternDataSize = 1;
  	else patternDataSize = 2;
  	if(patternReg & 0x1) patternWH = 2;
  	else patternWH = 1;
  	pageWH = 64/patternWH;
  	cellW = cellH = 8;
  	supplementData = patternNameReg & 0x3FF;
  	auxMode = (patternNameReg & 0x4000) >> 14;
	unsigned short colorCalc = reg->getWord(0xEC);
	if (colorCalc & 0x4) {
		alpha = ((~reg->getWord(0x10A) & 0x1F) << 3) + 0x7;
	}
	else {
		alpha = 0xFF;
	}

	colorOffset = (reg->getWord(0xE4) & 0x700) >> 8;
}

void NBG2::planeAddr(int i) {
	unsigned long offset = (reg->getWord(0x3C) & 0x700) >> 2;
  	unsigned long tmp;
  	switch(i) {
    		case 0: tmp = offset | reg->getByte(0x49); break;
    		case 1: tmp = offset | reg->getByte(0x48); break;
    		case 2: tmp = offset | reg->getByte(0x4B); break;
    		case 3: tmp = offset | reg->getByte(0x4A); break;
  	}
  	int deca = planeH + planeW - 2;
  	int multi = planeH * planeW;

	//if (reg->getWord(0x6) & 0x8000) {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
	  		else addr = (tmp >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
  		}
	/*}
	else {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) {
				cerr << "ici 1" << endl;
				addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
			}
	  		else {
				cerr << "ici 2" << endl;
				addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
			}
  		}
  		else {
	  		if (patternWH == 1) {
				cerr << "ici 3" << endl;
				addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
			}
	  		else {
				cerr << "ici 4" << endl;
				addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
			}
  		}
	}*/
}

int NBG2::getPriority(void) {
  return reg->getByte(0xFB) & 0x7;
}

int NBG2::getInnerPriority(void) {
  return 1;
}

void NBG3::init(void) {
	unsigned short patternNameReg = reg->getWord(0x36);
	unsigned short patternReg = reg->getWord(0x2A);

	enable = reg->getWord(0x20) & 0x8;
	transparencyEnable = !(reg->getWord(0x20) & 0x800);
	x = - reg->getWord(0x94);
	y = - reg->getWord(0x96);

  	colorNumber = (patternReg & 0x20) >> 5;
	bitmap = false; // NBG2 can only use cell mode
	
	mapWH = 2;
  	switch((reg->getWord(0x3A) & 0xC0) >> 6) {
  		case 0: planeW = planeH = 1; break;
  		case 1: planeW = 2; planeH = 1; break;
  		case 2: planeW = planeH = 2; break;
  	}
  	if(patternNameReg & 0x8000) patternDataSize = 1;
  	else patternDataSize = 2;
  	if(patternReg & 0x10) patternWH = 2;
  	else patternWH = 1;
  	pageWH = 64/patternWH;
  	cellW = cellH = 8;
  	supplementData = patternNameReg & 0x3FF;
  	auxMode = (patternNameReg & 0x4000) >> 14;
	unsigned short colorCalc = reg->getWord(0xEC);
	if (colorCalc & 0x8) {
		alpha = ((~reg->getWord(0x10A) & 0x1F00) >> 5) + 0x7;
	}
	else {
		alpha = 0xFF;
	}

	colorOffset = (reg->getWord(0xE4) & 0x7000) >> 12;
}

void NBG3::planeAddr(int i) {
	unsigned long offset = (reg->getWord(0x3C) & 0x7000) >> 6;
  	unsigned long tmp;
  	switch(i) {
    		case 0: tmp = offset | reg->getByte(0x4D); break;
    		case 1: tmp = offset | reg->getByte(0x4C); break;
    		case 2: tmp = offset | reg->getByte(0x4F); break;
    		case 3: tmp = offset | reg->getByte(0x4E); break;
  	}
  	int deca = planeH + planeW - 2;
  	int multi = planeH * planeW;

	//if (reg->getWord(0x6) & 0x8000) {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
	  		else addr = (tmp >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
  		}
	/*}
	else {
  		if (patternDataSize == 1) {
	  		if (patternWH == 1) addr = ((tmp & 0x1F) >> deca) * (multi * 0x2000);
	  		else addr = ((tmp & 0x7F) >> deca) * (multi * 0x800);
  		}
  		else {
	  		if (patternWH == 1) addr = ((tmp & 0xF) >> deca) * (multi * 0x4000);
	  		else addr = ((tmp & 0x3F) >> deca) * (multi * 0x1000);
  		}
	}*/
}

int NBG3::getPriority(void) {
	return (reg->getByte(0xFA) & 0x7);
}

int NBG3::getInnerPriority(void) {
	return 0;
}

/****************************************/
/*					*/
/*		VDP2 			*/
/*					*/
/****************************************/

Vdp2::Vdp2(SaturnMemory *v) : Memory(0x120) {
  satmem = v;
  _stop = false;
  vram = new Vdp2Ram;
  cram = new Vdp2ColorRam;
  setWord(0x4, 0); //setWord(0x4, 0x302);
  setWord(0x20, 0);

  SDL_Init(SDL_INIT_VIDEO);

#if HAVE_LIBSDL_IMAGE
  logo = IMG_Load("logo.png");
#else
  logo = SDL_LoadBMP("logo.bmp");
#endif
  SDL_WM_SetIcon(logo, NULL);
#ifndef _arch_dreamcast
  string title("Yabause ");
  title += VERSION;
  SDL_WM_SetCaption(title.c_str(), NULL);
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	GLSurface = SDL_SetVideoMode(320,224,32, SDL_OPENGL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glGenTextures(1, texture );
#endif
  //surface = SDL_SetVideoMode(320,224,16,SDL_DOUBLEBUF|SDL_HWSURFACE);
  surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 512, 256, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
  //vdp2Thread = SDL_CreateThread((int (*)(void*)) &Vdp2::lancer, this);
  	screens[4] = new RBG0(this, vram, cram, surface);
  	screens[3] = new NBG0(this, vram, cram, surface);
  	screens[2] = new NBG1(this, vram, cram, surface);
  	screens[1] = new NBG2(this, vram, cram, surface);
  	screens[0] = new NBG3(this, vram, cram, surface);
	((Vdp1 *) satmem->getVdp1())->setVdp2Ram(this, cram);
}

Vdp2::~Vdp2(void) {
#if DEBUG
  cerr << "stopping vdp2\n";
#endif
  //stop();
  //SDL_WaitThread(vdp2Thread, NULL);
#if DEBUG
  cerr << "vdp2 stopped\n";
#endif
  for(int i = 0;i < 5;i++) delete screens[i];
  SDL_FreeSurface(surface);
  SDL_FreeSurface(logo);
	delete vram;
	delete cram;
}

Memory *Vdp2::getCRam(void) {
	return cram;
}

Memory *Vdp2::getVRam(void) {
	return vram;
}

void Vdp2::lancer(Vdp2 *vdp2) {
	/*
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 4 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	vdp2->GLSurface = SDL_SetVideoMode(320,224,32, SDL_OPENGL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGenTextures(1, vdp2->texture );
	*/
	
	/*
  	vdp2->screens[4] = new RBG0(vdp2, vdp2->vram, vdp2->cram, vdp2->surface);
  	vdp2->screens[3] = new NBG0(vdp2, vdp2->vram, vdp2->cram, vdp2->surface);
  	vdp2->screens[2] = new NBG1(vdp2, vdp2->vram, vdp2->cram, vdp2->surface);
  	vdp2->screens[1] = new NBG2(vdp2, vdp2->vram, vdp2->cram, vdp2->surface);
  	vdp2->screens[0] = new NBG3(vdp2, vdp2->vram, vdp2->cram, vdp2->surface);
	((Vdp1 *) vdp2->satmem->getVdp1())->setVdp2Ram(vdp2, vdp2->cram);
	*/

	while(!vdp2->_stop) vdp2->executer();
}

void Vdp2::executer(void) {
  Timer t;
  for(int i = 0;i < 224;i++) { // FIXME depends on SuperH::synchroStart
    t.waitHBlankIN();
    setWord(0x4, getWord(0x4) | 0x0004);
    t.waitHBlankOUT();
    setWord(0x4, getWord(0x4) & 0xFFFB);
  }
  t.waitVBlankIN();
  setWord(0x4, getWord(0x4) | 0x0008);
  ((Scu *) satmem->getScu())->sendVBlankIN();
  for(int i = 0;i < 39;i++) { // FIXME depends on SuperH::synchroStart
    t.waitHBlankIN();
    setWord(0x4, getWord(0x4) | 0x0004);
    t.waitHBlankOUT();
    setWord(0x4, getWord(0x4) & 0xFFFB);
  }
  t.waitVBlankOUT();
  setWord(0x4, getWord(0x4) & 0xFFF7);

  glClear(GL_COLOR_BUFFER_BIT);
  //drawBackScreen();
  if (getWord(0) & 0x8000) {
    if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);
    screens[0]->draw();
    screens[1]->draw();
    screens[2]->draw();
    screens[3]->draw();
    screens[4]->draw();
    if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
  }

  if (*texture ==0) glGenTextures(1, texture );
  glBindTexture(GL_TEXTURE_2D, texture[0] );
  glTexImage2D(GL_TEXTURE_2D, 0, 4, 512, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  glEnable( GL_TEXTURE_2D );
  glBindTexture( GL_TEXTURE_2D, texture[0] );
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0); glVertex2f(-1, 1);
  glTexCoord2f(0.625, 0); glVertex2f(1, 1);
  glTexCoord2f(0.625, 0.875); glVertex2f(1, -1);
  glTexCoord2f(0, 0.875); glVertex2f(-1, -1);
  glEnd();
  glDisable( GL_TEXTURE_2D );

  ((Vdp1 *) satmem->getVdp1())->execute(0);
  glFlush();
  SDL_GL_SwapBuffers();
  //colorOffset();
  //SDL_Flip(surface);
  ((Scu *) satmem->getScu())->sendVBlankOUT();
}

Vdp2Screen *Vdp2::getScreen(int i) {
  return screens[i];
}

void Vdp2::sortScreens(void) {
  qsort(screens, 5, sizeof(Vdp2Screen *), &Vdp2Screen::comparePriority);
}

void Vdp2::updateRam(void) {
  cram->setMode(getWord(0xE) & 0xC000);
}

/*
void Vdp2::drawBackScreen(void) {
	unsigned long BKTAU = getWord(0xAC);
	unsigned long BKTAL = getWord(0xAE);
	unsigned long scrAddr;

	if (getWord(0x6) & 0x8000)
		scrAddr = ((BKTAU & 0x7 << 16) | BKTAL) * 2;
	else
		scrAddr = ((BKTAU & 0x3 << 16) | BKTAL) * 2;

	unsigned short dot;
	if (BKTAU & 0x8000) {
		dot = vram->getWord(scrAddr);
		SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, (dot & 0x1F) << 3, (dot & 0xCE0) >> 2, (dot & 0x7C00) >> 7));
	}
	else {
		for(int y = 0;y < 240;y++) {
			dot = vram->getWord(scrAddr);
			scrAddr += 2;
			hlineRGBA(surface, 0, 320, y, (dot & 0x1F) << 3, (dot & 0xCE0) >> 2, (dot & 0x7C00) >> 7, 0xFF);
		}
	}
}
*/

void Vdp2::priorityFunction(void) {
}

/*
void Vdp2::colorOffset(void) {
  SDL_Surface *tmp1, *tmp2;

  tmp1 = SDL_CreateRGBSurface(SDL_HWSURFACE, 400, 400, 16, 0, 0, 0, 0);
  tmp2 = SDL_CreateRGBSurface(SDL_HWSURFACE, 400, 400, 16, 0, 0, 0, 0);

  SDL_FillRect(tmp2, NULL, SDL_MapRGB(tmp2->format, getWord(0x114),
			  			    getWord(0x116),
						    getWord(0x118)));
  SDL_BlitSurface(surface, NULL, tmp1, NULL);

  SDL_imageFilterAdd((unsigned char *) tmp1->pixels,
		     (unsigned char *) tmp2->pixels,
		     (unsigned char *) surface->pixels, surface->pitch);

  SDL_FreeSurface(tmp1);
  SDL_FreeSurface(tmp2);
}
*/
