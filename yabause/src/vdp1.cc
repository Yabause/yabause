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
  //vdp1Thread = SDL_CreateThread((int (*)(void*)) &Vdp1::lancer, vdp1);
}

Vdp1Registers::~Vdp1Registers(void) {
#if DEBUG
  cerr << "stopping vdp1\n";
#endif
  vdp1->stop();
  //SDL_WaitThread(vdp1Thread, NULL);
#if DEBUG
  cerr << "vdp1 stopped\n";
#endif
  delete vdp1;
}

Vdp1 *Vdp1Registers::getVdp1(void) {
  return vdp1;
}

unsigned short Vdp1Registers::getEDSR(void) {
  return Memory::getWord(0x10);
} 

void Vdp1Registers::setEDSR(unsigned short val) {
  Memory::setWord(0x10, val);
}

unsigned short Vdp1Registers::getPTMR(void) {
  return Memory::getWord(0x4);
}

void Vdp1Registers::setPTMR(unsigned short val) {
  Memory::setWord(0x4, val);
}

Vdp1::Vdp1(Vdp1Registers *reg, Memory *mem, Scu *s) {
  memoire = mem;
  registres = reg;
  scu = s;

  _stop = false;
  registres->setPTMR(0);
}

void Vdp1::stop(void) {
  _stop = true;
}

void Vdp1::lancer(Vdp1 *vdp1) {
  Timer t;
  while(!vdp1->_stop) {
    t.waitVBlankOUT();
    vdp1->executer(0);
  }
}

void Vdp1::setSurface(SDL_Surface *s) {
	surface = s;
}

void Vdp1::executer(unsigned long adr) {
  unsigned short commande = memoire->getWord(adr);
  unsigned long suivante;
  int nbcom = 0;

  if (!registres->getPTMR()) return;

  // beginning of a frame (ST-013-R3-061694 page 53)
  // BEF <- CEF
  // CEF <- 0
  registres->setEDSR((registres->getEDSR() & 2) >> 1);

  SDL_Rect cleanRect;
  cleanRect.x = registres->getWord(0x8) >> 8;
  cleanRect.y = registres->getWord(0x8) & 0xFF;
  cleanRect.w = (registres->getWord(0xA) >> 8) - cleanRect.x;
  cleanRect.h = (registres->getWord(0xA) & 0xFF) - cleanRect.y;

  //SDL_FillRect(surface, /*&cleanRect*/ NULL, 0);
  
  while ((!(commande & 0x8000)) /*&& (nbcom++ < 10000)*/) { // FIXME

    //cerr << "vdp1 : commande = " << commande << '\n';
    //cerr << "vdp1 :   | mode de saut = " << ((commande & 0x7000) >> 12) << '\n';

    // First, process the command
    if (!(commande & 0x7000)) { // if (!skip)
      switch (commande & 0x001F) {
	case 0: //cerr << "vdp1\t: normal sprite draw" << endl;
	  normalSpriteDraw(adr);
	  break;
	case 1: //cerr << "vdp1\t:  scaled sprite draw" << endl;
	  scaledSpriteDraw();
	  break;
	case 2: // distorted sprite draw
	  distortedSpriteDraw(adr);
	  break;
	case 4: // polygon draw
	  polygonDraw(adr);
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
	  systemClipping(adr);
	  break;
	case 10: // local coordinate
	  localCoordinate(adr);
	  break;
	case 0x10: // draw end
	  drawEnd();
	  break;
	default:
	  //cerr << "vdp1\t: Mauvaise commande : "
	  //     << hex << setw(10) << commande << endl;
	  break;
      }
    }

    // Next, determine where to go next
    switch ((commande & 0x3000) >> 12) {
    case 0: // NEXT, jump to following table
      adr += 0x20;
      break;
    case 1: // ASSIGN, jump to CMDLINK
      adr = memoire->getWord(adr + 2) * 8;
      break;
    case 2: // CALL, call a subroutine
      executer(adr + 0x20);
      adr = memoire->getWord(adr + 2) * 8;
      break;
    case 3: // RETURN, return from subroutine
      return;
    }
    //cerr << "vdp1 :   | table suivante = " << adr << '\n';
#ifndef _arch_dreamcast
    try { commande = memoire->getWord(adr); }
    catch (BadMemoryAccess ma) { return; }
#else
    commande = memoire->getWord(adr);
#endif
  }
  // FIXME : on a fini, on place deux bits à 1 dans un registre
  registres->setEDSR(registres->getEDSR() | 2);
#if DEBUG
  //cerr << "vdp1 : draw end\n";
#endif
  scu->sendDrawEnd();
}

void Vdp1::normalSpriteDraw(unsigned long addr) {
  unsigned short xy = memoire->getWord(addr + 0xA);

  SDL_Rect rect;
  rect.x = localX + memoire->getWord(addr + 0xC);
  rect.y = localY + memoire->getWord(addr + 0xE);
  rect.w = ((xy >> 8) & 0x3F) * 8;
  rect.h = xy & 0xFF;

  SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, 0xFF, 0, 0));
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
	
	/*
	X[0] = localX + (memoire->getWord(addr + 0x0C) & 0x7FF);
	Y[0] = localY + (memoire->getWord(addr + 0x0E) & 0x7FF);
	X[1] = localX + (memoire->getWord(addr + 0x10) & 0x7FF);
	Y[1] = localY + (memoire->getWord(addr + 0x12) & 0x7FF);
	X[2] = localX + (memoire->getWord(addr + 0x14) & 0x7FF);
	Y[2] = localY + (memoire->getWord(addr + 0x16) & 0x7FF);
	X[3] = localX + (memoire->getWord(addr + 0x18) & 0x7FF);
	Y[3] = localY + (memoire->getWord(addr + 0x1A) & 0x7FF);
	*/
	X[0] = localX + (memoire->getWord(addr + 0x0C) );
	Y[0] = localY + (memoire->getWord(addr + 0x0E) );
	X[1] = localX + (memoire->getWord(addr + 0x10) );
	Y[1] = localY + (memoire->getWord(addr + 0x12) );
	X[2] = localX + (memoire->getWord(addr + 0x14) );
	Y[2] = localY + (memoire->getWord(addr + 0x16) );
	X[3] = localX + (memoire->getWord(addr + 0x18) );
	Y[3] = localY + (memoire->getWord(addr + 0x1A) );

	if ((X[0] & 0x400) ||(Y[0] & 0x400) ||(X[1] & 0x400) ||(Y[1] & 0x400) ||(X[2] & 0x400) ||(Y[2] & 0x400) ||(X[3] & 0x400) ||(Y[3] & 0x400)) {
		//cerr << "don't know what to do" << endl;
	}
	else {
		filledPolygonColor(surface, X, Y, 4, 0xFFFFFFFF);
	}
#if 0
  cerr << "vdp1\t: distorted sprite draw"
	  << " X0=" << X[0] << " Y0=" << Y[0]
	  << " X1=" << X[1] << " Y1=" << Y[1]
	  << " X2=" << X[2] << " Y2=" << Y[2]
	  << " X3=" << X[3] << " Y3=" << Y[3] << endl;
#endif
}

void Vdp1::polygonDraw(unsigned short addr) {
	Sint16 X[4];
	Sint16 Y[4];
	
	X[0] = localX + (memoire->getWord(addr + 0x0C) );
	Y[0] = localY + (memoire->getWord(addr + 0x0E) );
	X[1] = localX + (memoire->getWord(addr + 0x10) );
	Y[1] = localY + (memoire->getWord(addr + 0x12) );
	X[2] = localX + (memoire->getWord(addr + 0x14) );
	Y[2] = localY + (memoire->getWord(addr + 0x16) );
	X[3] = localX + (memoire->getWord(addr + 0x18) );
	Y[3] = localY + (memoire->getWord(addr + 0x1A) );

	unsigned short color = memoire->getWord(addr + 0x6);

	if ((X[0] & 0x400) ||(Y[0] & 0x400) ||(X[1] & 0x400) ||(Y[1] & 0x400) ||(X[2] & 0x400) ||(Y[2] & 0x400) ||(X[3] & 0x400) ||(Y[3] & 0x400)) {
		//cerr << "don't know what to do" << endl;
	}
	else {
		filledPolygonRGBA(surface, X, Y, 4, (color & 0x1F) << 3, (color & 0x3E0) >> 2, (color & 0x7C00) >> 7, 0xFF);
		//filledPolygonColor(surface, X, Y, 4, 0xFFFFFFFF);
	}

	unsigned short cmdpmod = memoire->getWord(addr + 0x4);
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
  unsigned short CMDXC = memoire->getWord(addr + 0x14);
  unsigned short CMDYC = memoire->getWord(addr + 0x16);
#if DEBUG
  //cerr << "vdp1\t: system clipping x=" << CMDXC << " y=" << CMDYC << endl;
#endif
}

void Vdp1::localCoordinate(unsigned short addr) {
  localX = memoire->getWord(addr + 0xC);
  localY = memoire->getWord(addr + 0xE);
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
