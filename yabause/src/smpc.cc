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
    SDL_CreateThread((int (*)(void*)) &Smpc::lancer, smpc);
  }

  if (adr == 0x1F) {
    //kill(Reveil<Smpc>::pid(), SIGUSR1);
    SDL_CreateThread((int (*)(void*)) &Smpc::executer, smpc);
  }
}

Smpc::Smpc(SmpcRegisters *mem, Scu *s, SaturnMemory *sm) {
  memoire = mem;
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

void Smpc::executer(Smpc *smpc) {
  switch(smpc->memoire->getCOMREG()) {
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
    cerr << "smpc\t: " << ((int) smpc->memoire->getCOMREG())
	 << " commande non implantée\n";
#endif
    break;
  }
  
  smpc->memoire->setSF(0);
}

void Smpc::lancer(Smpc *smpc) {
  if (smpc->intback) {
    if ((smpc->intbackIreg0 & 0x40) != (smpc->memoire->getIREG(0) & 0x40)) {
#if DEBUG
      //cerr << "BREAK\n";
#endif
      smpc->intback = false;
      smpc->memoire->setSR(smpc->memoire->getSR() & 0x0F);
      smpc->scu->sendSystemManager();
      return;
    }
    if ((smpc->intbackIreg0 & 0x80) != (smpc->memoire->getIREG(0) & 0x80)) {
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
  memoire->setSF(1);
  if (intback) {
#ifdef DEBUG
    cerr << "don't really know how I can get here..." << endl;
#endif
    INTBACKPeripheral();
    //intback = false;
    scu->sendSystemManager();
    return;
  }
  if (intbackIreg0 = memoire->getIREG(0)) {
    firstPeri = true;
    intback = memoire->getIREG(1) & 0x8;
    memoire->setSR(0x40 | (intback << 5));
    INTBACKStatus();
    scu->sendSystemManager();
    return;
  }
  if (memoire->getIREG(1) & 0x8) {
    firstPeri = true;
    intback = true;
    memoire->setSR(0x40);
    INTBACKPeripheral();
    scu->sendSystemManager();
    return;
  }
}

void Smpc::INTBACKStatus(void) {
  Timer t;
  t.wait(3000); //ne pas se fier a cette valeur !!!
#if DEBUG
  //cerr << "INTBACK status\n";
#endif
    // renvoyer les données d'heure, code cartouche, code de zone, ...
    // Ecrire les données dans SR
    
    // Ecrire les données dans OREG0
    memoire->setOREG(0, 0x80);
    //memoire->setOREG(0, 0x0);
    
    // Ecrire les données dans OREG1-7

    time_t tmp = time(NULL);
    struct tm times;
#ifdef WIN32
    memcpy(&times, localtime(&tmp), sizeof(times));
#else
    localtime_r(&tmp, &times);
#endif
    unsigned char annee[4] = {
      (1900 + times.tm_year) / 1000,
      ((1900 + times.tm_year) % 1000) / 100,
      (((1900 + times.tm_year) % 1000) % 100) / 10,
      (((1900 + times.tm_year) % 1000) % 100) % 10 };
    memoire->setOREG(1, (annee[0] << 4) | annee[1]);
    memoire->setOREG(2, (annee[2] << 4) | annee[3]);
    memoire->setOREG(3, (times.tm_wday << 4) | (times.tm_mon + 1));
    memoire->setOREG(4, ((times.tm_mday / 10) << 4) | (times.tm_mday % 10));
    memoire->setOREG(5, ((times.tm_hour / 10) << 4) | (times.tm_hour % 10));
    memoire->setOREG(6, ((times.tm_min / 10) << 4) | (times.tm_min % 10));
    memoire->setOREG(7, ((times.tm_sec / 10) << 4) | (times.tm_sec % 10));

    // Ecrire le code cartouche dans OREG8, bit 0 et 1
    
    memoire->setOREG(8, 0); // FIXME : du vrai hasard :)
    
    // Ecrire le code de zone dans OREG9, bit 0-7
    // 1 -> japon
    // 2 -> asie ntsc
    // 4 -> amérique du nord
    // 5 -> amérique du sud/centrale ntsc
    // 6 -> corée
    // A -> asie pal
    // C -> europe + divers pal
    // D -> amérique du sud/centrale pal
    
    //memoire->setOREG(9, 0xC);
    memoire->setOREG(9, 0);
    
    // Etat du système, première partie dans OREG10, bit 0-7
    // bit | valeur | commentaires
    // ---------------------------
    // 7   | 0      |
    // 6   | DOTSEL |
    // 5   | 1      |
    // 4   | 1      |
    // 3   | MSHNMI |
    // 2   | 1      |
    // 1   | SYSRES | 
    // 0   | SNDRES |
    
    // FIXME
    memoire->setOREG(10, 0x34|(dotsel<<6)|(mshnmi<<3)|(sysres<<1)|sndres);
    
    // Etat du système, deuxième partie dans OREG11, bit 6
    // bit 6 -> CDRES
    
    memoire->setOREG(11, cdres << 6); // FIXME
    
    // Les sauvegardes vont dans OREG12-15

    for(int i = 0;i < 4;i++) memoire->setOREG(12, 0);
    
    // Ecrire les données dans OREG31

    //t.wait(3000); //ne pas se fier a cette valeur !!!
    memoire->setOREG(31, 0);
    
  /*}
  else {
    // ne pas renvoyer les données d'heure, code cartouche, code de zone, ...
  }*/
  //Scu::sendSystemManager();
}

void Smpc::RESENAB(void) {
  resd = false;
  memoire->setOREG(31, 0x19);
}

void Smpc::RESDISA(void) {
  resd = true;
  memoire->setOREG(31, 0x1A);
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
    memoire->setSR(0xC0 | (memoire->getIREG(1) >> 4));
  else
    memoire->setSR(0x80 | (memoire->getIREG(1) >> 4));

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
  
  memoire->setOREG(0, 0xF1);  //PeripheralID
  memoire->setOREG(1, 0x02);
  memoire->setOREG(2, buttonbits >> 8);   //First Data
  memoire->setOREG(3, buttonbits & 0xFF);  //Second Data
  memoire->setOREG(4, 0xF1);
  memoire->setOREG(5, 0x02);
  memoire->setOREG(6, 0xFF);
  memoire->setOREG(7, 0xFF);
  for(int i = 8;i < 32;i++) memoire->setOREG(i, 0);
  //t.wait(20); //ne pas se fier a cette valeur !!!
}
