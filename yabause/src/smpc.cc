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

#include "smpc.hh"
#include "cs2.hh"
#include "scu.hh"
#include "timer.hh"
#include <time.h>

SmpcRegisters::SmpcRegisters(Scu *scu, SaturnMemory *sm) : Memory(0x80) {
  smpc = new Smpc(this, scu, sm);
}

SmpcRegisters::~SmpcRegisters(void) {
  delete smpc;
}

unsigned char SmpcRegisters::getIREG(int i) {
  return getByte(1 + 2 * i);
}
unsigned char SmpcRegisters::getCOMREG(void) {
  return getByte(0x1F);
}
unsigned char SmpcRegisters::getOREG(int i) {
  return getByte(0x21 + 2 * i);
}
unsigned char SmpcRegisters::getSR(void) {
  return getByte(0x61);
}
unsigned char SmpcRegisters::getSF(void) {
  return getByte(0x63);
}
void SmpcRegisters::setIREG(int i, unsigned char val) {
  setByte(i + 2 * i, val);
}
void SmpcRegisters::setCOMREG(unsigned char val) {
  setByte(0x1F, val);
}
void SmpcRegisters::setOREG(int i, unsigned char val) {
  setByte(0x21 + 2 * i, val);
}
void SmpcRegisters::setSR(unsigned char val) {
  setByte(0x61, val);
}
void SmpcRegisters::setSF(unsigned char val) {
  setByte(0x63, val);
}

void SmpcRegisters::setByte(unsigned long adr, unsigned char valeur) {
  Memory::setByte(adr, valeur);

  if (adr == 1) {  // Maybe an INTBACK continue/break request
    SDL_CreateThread((int (*)(void*)) &Smpc::intcont, smpc);
  }

  if (adr == 0x1F) {
    SDL_CreateThread((int (*)(void*)) &Smpc::execute, smpc);
  }
}

Smpc::Smpc(SmpcRegisters *reg, Scu *s, SaturnMemory *sm) {
  registers = reg;
  dotsel = false;
  mshnmi = false;
  sndres = false;
  cdres = false;
  sysres = false;
  resb = false;
  ste = false;
  resd = false;
  intback = false;
  firstPeri = false;

  scu = s;
  this->sm = sm;
}

void Smpc::execute(Smpc *smpc) {
  switch(smpc->registers->getCOMREG()) {
  case 0x6:
#if DEBUG
    cerr << "smpc\t: CDON\n";
#endif
    smpc->CDON();
    break;
  case 0x7:
#if DEBUG
    cerr << "smpc\t: CDOFF\n";
#endif
    smpc->CDOFF();
    break;
  case 0x10:
#if DEBUG
    //cerr << "smpc\t: INTBACK\n";
#endif
    smpc->INTBACK();
    break;
  case 0x19:
#if DEBUG
    cerr << "smpc\t: RESENAB\n";
#endif
    smpc->RESENAB();
    break;
  case 0x1A:
#if DEBUG
    cerr << "smpc\t: RESDISA\n";
#endif
    smpc->RESDISA();
    break;
  default:
#if DEBUG
    cerr << "smpc\t: " << ((int) smpc->registers->getCOMREG())
	 << " commande non implantée\n";
#endif
    break;
  }
  
  smpc->registers->setSF(0);
}

void Smpc::intcont(Smpc *smpc) {
  if (smpc->intback) {
    if ((smpc->intbackIreg0 & 0x40) != (smpc->registers->getIREG(0) & 0x40)) {
#if DEBUG
      //cerr << "BREAK\n";
#endif
      smpc->intback = false;
      smpc->registers->setSR(smpc->registers->getSR() & 0x0F);
      smpc->scu->sendSystemManager();
      return;
    }
    if ((smpc->intbackIreg0 & 0x80) != (smpc->registers->getIREG(0) & 0x80)) {
#if DEBUG
      //cerr << "CONTINUE\n";
#endif
      smpc->INTBACKPeripheral();
      //smpc->intback = false;
      smpc->scu->sendSystemManager();
    }
  }
}

void Smpc::INTBACK(void) {
  registers->setSF(1);
  if (intback) {
#ifdef DEBUG
    cerr << "don't really know how I can get here..." << endl;
#endif
    INTBACKPeripheral();
    //intback = false;
    scu->sendSystemManager();
    return;
  }
  if (intbackIreg0 = registers->getIREG(0)) {
    firstPeri = true;
    intback = registers->getIREG(1) & 0x8;
    registers->setSR(0x40 | (intback << 5));
    INTBACKStatus();
    scu->sendSystemManager();
    return;
  }
  if (registers->getIREG(1) & 0x8) {
    firstPeri = true;
    intback = true;
    registers->setSR(0x40);
    INTBACKPeripheral();
    scu->sendSystemManager();
    return;
  }
}

void Smpc::INTBACKStatus(void) {
  Timer t;
  t.wait(3000);
#if DEBUG
  //cerr << "INTBACK status\n";
#endif
    // return time, cartidge, zone, etc. data
    
    registers->setOREG(0, 0x80);	// goto normal startup
    //registers->setOREG(0, 0x0);	// goto setclock/setlanguage screen
    
    // write time data in OREG1-7
    time_t tmp = time(NULL);
    struct tm times;
#ifdef WIN32
    memcpy(&times, localtime(&tmp), sizeof(times));
#else
    localtime_r(&tmp, &times);
#endif
    unsigned char year[4] = {
      (1900 + times.tm_year) / 1000,
      ((1900 + times.tm_year) % 1000) / 100,
      (((1900 + times.tm_year) % 1000) % 100) / 10,
      (((1900 + times.tm_year) % 1000) % 100) % 10 };
    registers->setOREG(1, (year[0] << 4) | year[1]);
    registers->setOREG(2, (year[2] << 4) | year[3]);
    registers->setOREG(3, (times.tm_wday << 4) | (times.tm_mon + 1));
    registers->setOREG(4, ((times.tm_mday / 10) << 4) | (times.tm_mday % 10));
    registers->setOREG(5, ((times.tm_hour / 10) << 4) | (times.tm_hour % 10));
    registers->setOREG(6, ((times.tm_min / 10) << 4) | (times.tm_min % 10));
    registers->setOREG(7, ((times.tm_sec / 10) << 4) | (times.tm_sec % 10));

    // write cartidge data in OREG8
    registers->setOREG(8, 0); // FIXME : random value
    
    // write zone data in OREG9 bits 0-7
    // 1 -> japan
    // 2 -> asia/ntsc
    // 4 -> north america
    // 5 -> central/south america/ntsc
    // 6 -> corea
    // A -> asia/pal
    // C -> europe + others/pal
    // D -> central/south america/pal
    registers->setOREG(9, 0);
    
    // system state, first part in OREG10, bits 0-7
    // bit | value  | comment
    // ---------------------------
    // 7   | 0      |
    // 6   | DOTSEL |
    // 5   | 1      |
    // 4   | 1      |
    // 3   | MSHNMI |
    // 2   | 1      |
    // 1   | SYSRES | 
    // 0   | SNDRES |
    registers->setOREG(10, 0x34|(dotsel<<6)|(mshnmi<<3)|(sysres<<1)|sndres);
    
    // Etat du système, deuxième partie dans OREG11, bit 6
    // system state, second part in OREG11, bit 6
    // bit 6 -> CDRES
    registers->setOREG(11, cdres << 6); // FIXME
    
    // backups in OREG12-15
    for(int i = 0;i < 4;i++) registers->setOREG(12, 0);
    
    registers->setOREG(31, 0);
}

void Smpc::RESENAB(void) {
  resd = false;
  registers->setOREG(31, 0x19);
}

void Smpc::RESDISA(void) {
  resd = true;
  registers->setOREG(31, 0x1A);
}

void Smpc::CDON(void) {
}

void Smpc::CDOFF(void) {
}

void Smpc::INTBACKPeripheral(void) {
#if DEBUG
  //cerr << "INTBACK peripheral\n";
#endif
  if (firstPeri)
    registers->setSR(0xC0 | (registers->getIREG(1) >> 4));
  else
    registers->setSR(0x80 | (registers->getIREG(1) >> 4));

  Timer t;
  t.wait(20);

  /* PeripheralID:
  0xF0 - Digital Device Standard Format
  0xF1 - Analog Device Standard Format
  0xF2 - Pointing Device Standard Format
  0xF3 - Keyboard Device Standard Format
  0x1E - Mega Drive 3-Button Pad
  0x2E - Mega Drive 6-Button Pad
  0x3E - Saturn Mouse */
  
  registers->setOREG(0, 0xF1);  //PeripheralID
  registers->setOREG(1, 0x02);
  registers->setOREG(2, buttonbits >> 8);   //First Data
  registers->setOREG(3, buttonbits & 0xFF);  //Second Data
  registers->setOREG(4, 0xF1);
  registers->setOREG(5, 0x02);
  registers->setOREG(6, 0xFF);
  registers->setOREG(7, 0xFF);
  for(int i = 8;i < 32;i++) registers->setOREG(i, 0);
}
