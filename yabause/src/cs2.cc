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

#include "cs2.hh"
#include "timer.hh"

Cs2::Cs2(void) : Memory(0x100000) {
  cd = new YCD(this);
  cdThread = NULL;
  cdMutex = SDL_CreateMutex();
}

Cs2::~Cs2(void) {
  SDL_DestroyMutex(cdMutex);
  delete cd;
}

unsigned short Cs2::getHIRQ(void) {return Memory::getWord(0x90008);}
unsigned short Cs2::getHIRQMask(void) {return Memory::getWord(0x9000C);}
unsigned short Cs2::getCR1(void) {return Memory::getWord(0x90018);}
unsigned short Cs2::getCR2(void) {return Memory::getWord(0x9001C);}
unsigned short Cs2::getCR3(void) {return Memory::getWord(0x90020);}
unsigned short Cs2::getCR4(void) {return Memory::getWord(0x90024);}
void Cs2::setHIRQ(unsigned short val) {Memory::setWord(0x90008, val);}
void Cs2::setHIRQMask(unsigned short val) {Memory::setWord(0x9000C, val);}
void Cs2::setCR1(unsigned short val) {Memory::setWord(0x90018, val);}
void Cs2::setCR2(unsigned short val) {Memory::setWord(0x9001C, val);}
void Cs2::setCR3(unsigned short val) {Memory::setWord(0x90020, val);}
void Cs2::setCR4(unsigned short val) {Memory::setWord(0x90024, val);}

void Cs2::setWord(unsigned long addr, unsigned short val) {
  if (addr >= size) { throw BadMemoryAccess(addr); }
  switch(addr) {
    case 0x90008:
	    	  Memory::setWord(addr, (getHIRQ() & val) | 0x402);
	          break;
    case 0x9000C:
    case 0x90018:
    case 0x9001C:
    case 0x90020: Memory::setWord(addr, val);
		  break;
    case 0x90024: Memory::setWord(addr, val);
		  /*SDL_mutexP(cdMutex);
		  SDL_WaitThread(cdThread, NULL);
		  cdThread = SDL_CreateThread((int (*)(void*)) &YCD::lancer, cd);
		  SDL_mutexV(cdMutex);*/
		  YCD::lancer(cd);
		  break;
    default: Memory::setWord(addr, val);
  }
}

YCD::YCD(Cs2 *cs2) {
  _stop = false;
  memoire = cs2;
  FAD = 150;
  statut = 0x77; //statut = 0x26;
  options = 0;
  repcnt = 0;
  ctrladdr = 0x41;
  piste = 1;
  index = 1;

  memoire->setCR1(( 0 <<8) | 'C');
  memoire->setCR2(('D'<<8) | 'B');
  memoire->setCR3(('L'<<8) | 'O');
  memoire->setCR4(('C'<<8) | 'K');
  memoire->setHIRQ(0x07D3);
  memoire->setHIRQMask(0x7FF);
}

#if 0
void YCD::lancer(YCD *cd) {
  Timer t;
  while(!cd->_stop) {
    if (cd->_command) {
      cd->_command = false;
      cd->executer();
    }
    else {
      //t.waitVBlankOUT();
      //t.wait(1);
      //cd->periodicUpdate();
    }
  }
}

void YCD::command(void) {
  _command = true;
}

void YCD::periodicUpdate(void) {
  if ((memoire->getCR1() >> 8) != 0 ) return;
  statut |= 0x20;
}
#endif

void YCD::lancer(YCD *cd) {
  cd->executer();
}

void YCD::executer(void) {
  memoire->setHIRQ(memoire->getHIRQ() & 0XFFFE);
  unsigned short instruction = memoire->getCR1() >> 8;

  switch (instruction) {
    case 0x00:
#if DEBUG
      //cerr << "cs2\t: getStatus\n";
#endif
      getStatus();
      break;
    case 0x01:
#if DEBUG
      cerr << "cs2\t: getHardwareInfo\n";
#endif
      getHardwareInfo();
      break;
    case 0x04:
#if DEBUG
      cerr << "cs2\t: initializeCDSystem\n";
#endif
      initializeCDSystem();
      break;
    case 0x06:
#if DEBUG
      cerr << "cs2\t: endDataTransfer\n";
#endif
      endDataTransfer();
      break;
    case 0x48:
#if DEBUG
      cerr << "cs2\t: resetSelector\n";
#endif
      resetSelector();
      break;
    case 0x60:
#if DEBUG
      cerr << "cs2\t: setSectorLength\n";
#endif
      setSectorLength();
      break;
    case 0x67:
#if DEBUG
      cerr << "cs2\t: getCopyError\n";
#endif
      getCopyError();
      break;
    case 0x75:
#if DEBUG
      cerr << "cs2\t: abortFile\n";
#endif
      abortFile();
      break;
    default:
#if DEBUG
      cerr << "cs2\t: " << hex << setw(10) << instruction
                  << "commande non implantée" << endl;
#endif
      break;
  }
}

void YCD::getStatus(void) {
  memoire->setCR1((statut << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  memoire->setCR2((ctrladdr << 8) | piste);
  memoire->setCR3((index << 8) | ((FAD >> 16) & 0xFF));
  memoire->setCR4((unsigned short) FAD);
  memoire->setHIRQ(memoire->getHIRQ() | 0x0001);
}

void YCD::getHardwareInfo(void) {
  memoire->setCR1(statut << 8);
  memoire->setCR2(0x01);
  memoire->setCR3(0x0);
  memoire->setCR4(0x0400);
  memoire->setHIRQ(memoire->getHIRQ() | 0x0001);
}

void YCD::initializeCDSystem(void) {
  FAD = 150;
  memoire->setCR1((statut << 8) | (repcnt & 0xF));
  memoire->setCR2((ctrladdr << 8) | (piste & 0xFF));
  memoire->setCR3((index << 8) | ((FAD >> 16) &0xFF));
  memoire->setCR4((unsigned short) FAD);
  // FIXME manque une partie...
  memoire->setHIRQ(memoire->getHIRQ() | 0x0001);
}

void YCD::endDataTransfer(void) {
  memoire->setCR1((statut << 8) | (cdwnum >> 16)); // FIXME
  memoire->setCR2((unsigned short) cdwnum);
  memoire->setCR3(0);
  memoire->setCR4(0);
  memoire->setHIRQ(memoire->getHIRQ() | 0x0003);
}

void YCD::resetSelector(void) {
  memoire->setCR1((statut << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  memoire->setCR2((ctrladdr << 8) | (piste & 0xFF));
  memoire->setCR3((index << 8) | ((FAD >> 16) &0xFF));
  memoire->setCR4((unsigned short) FAD);
  memoire->setHIRQ(memoire->getHIRQ() | 0x0041);
}

void YCD::setSectorLength(void) {
  memoire->setCR1((statut << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  memoire->setCR2((ctrladdr << 8) | (piste & 0xFF));
  memoire->setCR3((index << 8) | ((FAD >> 16) &0xFF));
  memoire->setCR4((unsigned short) FAD);
  memoire->setHIRQ(memoire->getHIRQ() | 0x0041);
}

void YCD::getCopyError(void) {
  memoire->setCR1(statut << 8);
  memoire->setCR2(0);
  memoire->setCR3(0);
  memoire->setCR4(0);
  memoire->setHIRQ(memoire->getHIRQ() | 0x0001);
}

void YCD::abortFile(void) {
  //statut = 0x26;
  cdwnum = 0xFFFFFFFF;
  memoire->setCR1((statut << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  memoire->setCR2((ctrladdr << 8) | (piste & 0xFF));
  memoire->setCR3((index << 8) | ((FAD >> 16) &0xFF));
  memoire->setCR4((unsigned short) FAD);
  memoire->setHIRQ(memoire->getHIRQ() | 0x0203);
}
