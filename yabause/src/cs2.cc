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

#define CDB_HIRQ_CMOK      0x0001
#define CDB_HIRQ_DRDY      0x0002
#define CDB_HIRQ_CSCT      0x0004
#define CDB_HIRQ_BFUL      0x0008
#define CDB_HIRQ_PEND      0x0010
#define CDB_HIRQ_DCHG      0x0020
#define CDB_HIRQ_ESEL      0x0040
#define CDB_HIRQ_EHST      0x0080
#define CDB_HIRQ_ECPY      0x0100
#define CDB_HIRQ_EFLS      0x0200
#define CDB_HIRQ_SCDQ      0x0400
#define CDB_HIRQ_MPED      0x0800
#define CDB_HIRQ_MPCM      0x1000
#define CDB_HIRQ_MPST      0x2000

#define CDB_STAT_BUSY      0x00
#define CDB_STAT_PAUSE     0x01
#define CDB_STAT_STANDBY   0x02
#define CDB_STAT_PLAY      0x03
#define CDB_STAT_SEEK      0x04
#define CDB_STAT_SCAN      0x05
#define CDB_STAT_OPEN      0x06
#define CDB_STAT_NODISC    0x07
#define CDB_STAT_RETRY     0x08
#define CDB_STAT_ERROR     0x09
#define CDB_STAT_FATAL     0x0A
#define CDB_STAT_PERI      0x20
#define CDB_STAT_TRNS      0x40
#define CDB_STAT_WAIT      0x80
#define CDB_STAT_REJECT    0xFF

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
                  cdThread = SDL_CreateThread((int (*)(void*)) &YCD::run, cd);
		  SDL_mutexV(cdMutex);*/
                  YCD::run(cd);
		  break;
    default: Memory::setWord(addr, val);
  }
}

YCD::YCD(Cs2 *cs2) {
  _stop = false;
  memory = cs2;
  FAD = 150;
  status = CDB_STAT_PERI | CDB_STAT_PAUSE; // Still not 100% correct, but better

  options = 0;
  repcnt = 0;
  ctrladdr = 0x41;
  track = 1;
  index = 1;

  memory->setCR1(( 0 <<8) | 'C');
  memory->setCR2(('D'<<8) | 'B');
  memory->setCR3(('L'<<8) | 'O');
  memory->setCR4(('C'<<8) | 'K');
  memory->setHIRQ(0x07D3);
  memory->setHIRQMask(0x7FF);
}

#if 0
void YCD::run(YCD *cd) {
  Timer t;
  while(!cd->_stop) {
    if (cd->_command) {
      cd->_command = false;
      cd->execute();
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
  if ((memory->getCR1() >> 8) != 0 ) return;
  status |= CDB_STAT_PERI;
}
#endif

void YCD::run(YCD *cd) {
  cd->execute();
}

void YCD::execute(void) {
  memory->setHIRQ(memory->getHIRQ() & 0XFFFE);
  unsigned short instruction = memory->getCR1() >> 8;

  switch (instruction) {
    case 0x00:
#if DEBUG
      cerr << "cs2\t: getStatus\n";
#endif
      getStatus();
      break;
    case 0x01:
#if DEBUG
      cerr << "cs2\t: getHardwareInfo\n";
#endif
      getHardwareInfo();
      break;
    case 0x02:
#if DEBUG
      cerr << "cs2\t: getToc\n";
#endif
      getToc();
      break;
    case 0x03:
    {
#if DEBUG
      cerr << "cs2\t: getSessionInfo\n";
#endif
       getSessionInfo();
       break;
    }
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
    case 0x93: {
#if DEBUG
      cerr << "cs2\t: mpegInit\n";
#endif
      mpegInit();
      break;
    }
    default:
#if DEBUG
      cerr << "cs2\t: " << hex << setw(10) << instruction
                  << " command not implemented" << endl;
#endif
      break;
  }
}

void YCD::getStatus(void) {
  memory->setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  memory->setCR2((ctrladdr << 8) | track);
  memory->setCR3((index << 8) | ((FAD >> 16) & 0xFF));
  memory->setCR4((unsigned short) FAD);
  memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK);
}

void YCD::getHardwareInfo(void) {
  memory->setCR1(status << 8);
  // hardware flags/CD Version
  memory->setCR2(0x0201); // mpeg card exists
  // mpeg version, doesn't have to be used
  memory->setCR3(0x0);
  // drive info/revision
  memory->setCR4(0x0400); 
  memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK);
}

void YCD::getToc(void) {
  // Read in Toc to 0x25898000 here

  cdwnum = 0xCC;

  memory->setCR1(status << 8);
  memory->setCR2(0xCC);
  memory->setCR3(0x0);
  memory->setCR4(0x0); 
  memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_DRDY);
}

void YCD::getSessionInfo(void) {
  memory->setCR1(status << 8);
  memory->setCR2(0);
  memory->setCR3(0); // session info
  memory->setCR4(0); // session info
  memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_DRDY);
}

void YCD::initializeCDSystem(void) {
  FAD = 150;
  memory->setCR1((status << 8) | (repcnt & 0xF));
  memory->setCR2((ctrladdr << 8) | (track & 0xFF));
  memory->setCR3((index << 8) | ((FAD >> 16) &0xFF));
  memory->setCR4((unsigned short) FAD);
  // FIXME Something missing here...
  memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK);
}

void YCD::endDataTransfer(void) {
  memory->setCR1((status << 8) | ((cdwnum >> 16) & 0xFF)); // FIXME
  memory->setCR2((unsigned short) cdwnum);
  memory->setCR3(0);
  memory->setCR4(0);
  memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_DRDY);
}

void YCD::resetSelector(void) {
  memory->setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  memory->setCR2((ctrladdr << 8) | (track & 0xFF));
  memory->setCR3((index << 8) | ((FAD >> 16) &0xFF));
  memory->setCR4((unsigned short) FAD);
  memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL);
}

void YCD::setSectorLength(void) {
  memory->setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  memory->setCR2((ctrladdr << 8) | (track & 0xFF));
  memory->setCR3((index << 8) | ((FAD >> 16) &0xFF));
  memory->setCR4((unsigned short) FAD);
  memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL);
}

void YCD::getCopyError(void) {
  memory->setCR1(status << 8);
  memory->setCR2(0);
  memory->setCR3(0);
  memory->setCR4(0);
  memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK);
}

void YCD::abortFile(void) {
  //status = 0x26;
  cdwnum = 0xFFFFFFFF;
  memory->setCR1((status << 8) | ((options & 0xF) << 4) | (repcnt & 0xF));
  memory->setCR2((ctrladdr << 8) | (track & 0xFF));
  memory->setCR3((index << 8) | ((FAD >> 16) &0xFF));
  memory->setCR4((unsigned short) FAD);
  memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_ESEL | CDB_HIRQ_EFLS);
}

void YCD::mpegInit(void) {
  memory->setCR1(status << 8);

  if (memory->getCR2() == 0x0001) // software timer?
     memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_MPCM | CDB_HIRQ_MPED | CDB_HIRQ_MPST); 
  else
     memory->setHIRQ(memory->getHIRQ() | CDB_HIRQ_CMOK | CDB_HIRQ_MPED | CDB_HIRQ_MPST); 

  memory->setCR2(0);
  memory->setCR3(0);
  memory->setCR4(0);

  // future mpeg-related variables should be initialized here
}

