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
#include "yui.hh"
#include <time.h>

unsigned char Smpc::getIREG(int i) {
  return getByte(1 + 2 * i);
}
unsigned char Smpc::getCOMREG(void) {
  return getByte(0x1F);
}
unsigned char Smpc::getOREG(int i) {
  return getByte(0x21 + 2 * i);
}
unsigned char Smpc::getSR(void) {
  return getByte(0x61);
}
unsigned char Smpc::getSF(void) {
  return getByte(0x63);
}
void Smpc::setIREG(int i, unsigned char val) {
  setByte(i + 2 * i, val);
}
void Smpc::setCOMREG(unsigned char val) {
  setByte(0x1F, val);
}
void Smpc::setOREG(int i, unsigned char val) {
  setByte(0x21 + 2 * i, val);
}
void Smpc::setSR(unsigned char val) {
  setByte(0x61, val);
}
void Smpc::setSF(unsigned char val) {
  setByte(0x63, val);
}

void Smpc::setByte(unsigned long addr, unsigned char value) {
  Memory::setByte(addr, value);

  if (addr == 1) {  // Maybe an INTBACK continue/break request
    SDL_WaitThread(smpcThread, NULL);
    smpcThread = SDL_CreateThread((int (*)(void*)) &Smpc::intcont, this);
  }

  if (addr == 0x1F) {
    SDL_WaitThread(smpcThread, NULL);
    smpcThread = SDL_CreateThread((int (*)(void*)) &Smpc::execute, this);
  }
}

Smpc::Smpc(SaturnMemory *sm) : Memory(0xFF, 0x80) {
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

  // get region
  regionid = yui_region();

  if (regionid == 0)
  {
     // Time to autodetect the region using the cd block
     regionid = ((Cs2 *)sm->getCS2())->GetRegionID();          
  }

  this->sm = sm;

  smpcThread = NULL;
}

void Smpc::execute(Smpc *smpc) {
  switch(smpc->getCOMREG()) {
  case 0x0:
#if DEBUG
    cerr << "smpc\t: MSHON not implemented\n";
#endif
    break;
  case 0x2:
#if DEBUG
    cerr << "smpc\t: SSHON not implemented\n";
#endif
    break;
  case 0x3:
#if DEBUG
    cerr << "smpc\t: SSHOFF not implemented\n";
#endif
    break;
  case 0x6:
#if DEBUG
    cerr << "smpc\t: SNDON\n";
#endif
    smpc->SNDON();
    break;
  case 0x7:
#if DEBUG
    cerr << "smpc\t: SNDOFF\n";
#endif
    smpc->SNDOFF();
    break;
  case 0x8:
#if DEBUG
    cerr << "smpc\t: CDON not implemented\n";
#endif
    break;
  case 0x9:
#if DEBUG
    cerr << "smpc\t: CDOFF not implemented\n";
#endif
    break;
  case 0xD:
#if DEBUG
    cerr << "smpc\t: SYSRES not implemented\n";
#endif
    break;
  case 0xE:
#if DEBUG
    cerr << "smpc\t: CKCHG352 not implemented\n";
#endif
    break;
  case 0xF:
#if DEBUG
    cerr << "smpc\t: CKCHG320 not implemented\n";
#endif
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
    cerr << "smpc\t: command " << ((int) smpc->getCOMREG())
         << " not implemented\n";
#endif
    break;
  }
  
  smpc->setSF(0);
}

void Smpc::intcont(Smpc *smpc) {
  if (smpc->intback) {
    if ((smpc->intbackIreg0 & 0x40) != (smpc->getIREG(0) & 0x40)) {
#if DEBUG
      //cerr << "BREAK\n";
#endif
      smpc->intback = false;
      smpc->setSR(smpc->getSR() & 0x0F);
      ((Scu *) smpc->sm->getScu())->sendSystemManager();
      return;
    }
    if ((smpc->intbackIreg0 & 0x80) != (smpc->getIREG(0) & 0x80)) {
#if DEBUG
      //cerr << "CONTINUE\n";
#endif
      smpc->INTBACKPeripheral();
      //smpc->intback = false;
      ((Scu *) smpc->sm->getScu())->sendSystemManager();
    }
  }
}

void Smpc::INTBACK(void) {
  setSF(1);
  if (intback) {
#ifdef DEBUG
    cerr << "don't really know how I can get here..." << endl;
#endif
    INTBACKPeripheral();
    //intback = false;
    ((Scu *) sm->getScu())->sendSystemManager();
    return;
  }
  if (intbackIreg0 = getIREG(0)) {
    firstPeri = true;
    intback = getIREG(1) & 0x8;
    setSR(0x40 | (intback << 5));
    INTBACKStatus();
    ((Scu *) sm->getScu())->sendSystemManager();
    return;
  }
  if (getIREG(1) & 0x8) {
    firstPeri = true;
    intback = true;
    setSR(0x40);
    INTBACKPeripheral();
    ((Scu *) sm->getScu())->sendSystemManager();
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
    
    setOREG(0, 0x80);	// goto normal startup
    //setOREG(0, 0x0);	// goto setclock/setlanguage screen
    
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
    setOREG(1, (year[0] << 4) | year[1]);
    setOREG(2, (year[2] << 4) | year[3]);
    setOREG(3, (times.tm_wday << 4) | (times.tm_mon + 1));
    setOREG(4, ((times.tm_mday / 10) << 4) | (times.tm_mday % 10));
    setOREG(5, ((times.tm_hour / 10) << 4) | (times.tm_hour % 10));
    setOREG(6, ((times.tm_min / 10) << 4) | (times.tm_min % 10));
    setOREG(7, ((times.tm_sec / 10) << 4) | (times.tm_sec % 10));

    // write cartidge data in OREG8
    setOREG(8, 0); // FIXME : random value
    
    // write zone data in OREG9 bits 0-7
    // 1 -> japan
    // 2 -> asia/ntsc
    // 4 -> north america
    // 5 -> central/south america/ntsc
    // 6 -> corea
    // A -> asia/pal
    // C -> europe + others/pal
    // D -> central/south america/pal
    setOREG(9, regionid);

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
    setOREG(10, 0x34|(dotsel<<6)|(mshnmi<<3)|(sysres<<1)|sndres);
    
    // Etat du système, deuxième partie dans OREG11, bit 6
    // system state, second part in OREG11, bit 6
    // bit 6 -> CDRES
    setOREG(11, cdres << 6); // FIXME
    
    // backups in OREG12-15
    for(int i = 0;i < 4;i++) setOREG(12+i, 0);
    
    setOREG(31, 0);
}

void Smpc::RESENAB(void) {
  resd = false;
  setOREG(31, 0x19);
}

void Smpc::RESDISA(void) {
  resd = true;
  setOREG(31, 0x1A);
}

void Smpc::SNDON(void) {
}

void Smpc::SNDOFF(void) {
}

void Smpc::INTBACKPeripheral(void) {
#if DEBUG
  //cerr << "INTBACK peripheral\n";
#endif
  if (firstPeri)
    setSR(0xC0 | (getIREG(1) >> 4));
  else
    setSR(0x80 | (getIREG(1) >> 4));

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
  
  setOREG(0, 0xF1);  //PeripheralID
  setOREG(1, 0x02);
  setOREG(2, buttonbits >> 8);   //First Data
  setOREG(3, buttonbits & 0xFF);  //Second Data
  setOREG(4, 0xF1);
  setOREG(5, 0x02);
  setOREG(6, 0xFF);
  setOREG(7, 0xFF);
  for(int i = 8;i < 32;i++) setOREG(i, 0);
}
