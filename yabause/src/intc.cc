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

#include "intc.hh"
#include "superh.hh"

#ifdef _arch_dreamcast
/* No STL for the dreamcast, use my tree implementation instead of a priority queue */
#include "tree.h"
tree_t int_tree;

static int __int_cmp(void *d1, void *d2)	{
	Interrupt *i1 = (Interrupt *) d1;
	Interrupt *i2 = (Interrupt *) d2;
	
	if(i1->level() < i2->level())	{
		return 1;
	}
	if(i1->level() > i2->level())	{
		return -1;
	}
	return 0;
}

void __init_tree()	{
	int_tree = create_tree(__int_cmp);
}

void __add_interrupt(const Interrupt *i)	{
	add_to_tree(&int_tree, (void *)i);
}

Interrupt *__get_highest_int()	{
	return (Interrupt *) get_highest_data(&int_tree);
}

void __del_highest_int()	{
	remove_highest_node(&int_tree);
}
#endif

OnchipRegisters::OnchipRegisters(SuperH *sh) : Memory(0x1FF) {
  setByte(4, 0x84);

  intc = new Intc(sh);
  dmac = new Dmac(this, sh);
  intcThread = SDL_CreateThread((int (*)(void*)) &Intc::lancer, intc);
#ifdef _arch_dreamcast
  __init_tree();
#endif
}

OnchipRegisters::~OnchipRegisters(void) {
#if DEBUG
  cerr << "stopping intc\n";
#endif
  intc->send(Interrupt(255, 0));
  SDL_WaitThread(intcThread, NULL);
#if DEBUG
  cerr << "intc stopped\n";
#endif
  delete intc;
  delete dmac;
}

Intc *OnchipRegisters::getIntc(void) {
  return intc;
}

unsigned short OnchipRegisters::getIPRA(void) {
  return Memory::getWord(0xE2);
}

unsigned short OnchipRegisters::getIPRB(void) {
  return Memory::getWord(0x60);
}

unsigned short OnchipRegisters::getICR(void) {
  return Memory::getWord(0xE0);
}

unsigned long OnchipRegisters::getDMAOR(void) {
  return Memory::getLong(0x1B0);
}

unsigned long OnchipRegisters::getCHCR0(void) {
  return Memory::getLong(0x18C);
}

unsigned long OnchipRegisters::getCHCR1(void) {
  return Memory::getLong(0x19C);
}

unsigned long OnchipRegisters::getSAR0(void) {
  return Memory::getLong(0x180);
}

unsigned long OnchipRegisters::getDAR0(void) {
  return Memory::getLong(0x184);
}

unsigned long OnchipRegisters::getTCR0(void) {
  return Memory::getLong(0x188);
}

void OnchipRegisters::setCHCR0(unsigned long val) {
  Memory::setLong(0x18C, val);
}

void OnchipRegisters::setSAR0(unsigned long val) {
  Memory::setLong(0x180, val);
}

void OnchipRegisters::setDAR0(unsigned long val) {
  Memory::setLong(0x184, val);
}

void OnchipRegisters::setTCR0(unsigned long val) {
  Memory::setLong(0x188, val);
}

void OnchipRegisters::setLong(unsigned long addr, unsigned long val) {
  switch(addr) {
  case 0x104: {
    long divisor = (long) Memory::getLong(0x100);
    if (divisor == 0) {
      Memory::setLong(0x104, val);
      Memory::setLong(0x114, val);
      Memory::setLong(0x108, 1);
    }
    else {
      long quotient = ((long) val) / divisor;
      long remainder = ((long) val) % divisor;
      Memory::setLong(0x104, quotient);
      Memory::setLong(0x114, quotient);
      Memory::setLong(0x110, remainder);
    }
    break;
  }
  case 0x114: {
    unsigned long divi[2];
    
#ifdef WORDS_BIGENDIAN
    divi[0] = Memory::getLong(0x110);
    divi[1] = val;
#else
    divi[0] = val;
    divi[1] = Memory::getLong(0x110);
#endif
    long long dividend = *((long long *) divi);
    long divisor = (long) Memory::getLong(0x100);
    if (divisor == 0) {
      Memory::setLong(0x104, val);
      Memory::setLong(0x114, val);
      Memory::setLong(0x108, 1);
    }
    else {
      long quotient = dividend / divisor;
      long remainder = dividend % divisor;
      Memory::setLong(0x104, quotient);
      Memory::setLong(0x114, quotient);
      Memory::setLong(0x110, remainder);
    }
    break;
  }
  case 0x1B0:
    Memory::setLong(addr, val);
    SDL_CreateThread((int (*)(void*)) &Dmac::lancer, dmac);
    break;
  default:
    Memory::setLong(addr, val);
  }
}

#ifndef _arch_dreamcast
ostream& operator<<(ostream& os, OnchipRegisters& ri) {
  os << hex << "IPRA\t: " << setw(10) << ri.getIPRA() << endl
            << "IPRB\t: " << setw(10) << ri.getIPRB() << endl
            << "ICR\t: "  << setw(10) << ri.getICR() << endl;
  return os;
}
#endif

Dmac::Dmac(OnchipRegisters *reg, SuperH *sh) {
  registers = reg;
  proc = sh;
}

void Dmac::lancer(Dmac *dmac) {
  dmac->executer();
}

void Dmac::executer(void) {
  unsigned long DMAOR = registers->getDMAOR();

  if ((DMAOR & 0x1) && !((DMAOR & 0x4) || (DMAOR & 0x2))) {
    unsigned long CHCR0 = registers->getCHCR0();
    unsigned long CHCR1 = registers->getCHCR1();
    if ((CHCR0 & 0x1) && (CHCR1 & 0x1)) { // both channel wants DMA
      if (DMAOR & 0x8) { // round robin priority
      }
      else { // channel 0 > channel 1 priority
      }
    }
    else { // only one channel wants DMA
      if (CHCR0 & 0x1) { // DMA for channel 0
        if (!(CHCR0 & 0x2)) { // TE is not set
	  int srcInc;
	  switch(CHCR0 & 0x3000) {
	  case 0x0000: srcInc = 0; break;
	  case 0x1000: srcInc = 1; break;
	  case 0x2000: srcInc = -1; break;
	  }
	  int destInc;
	  switch(CHCR0 & 0xC000) {
	  case 0x0000: destInc = 0; break;
	  case 0x4000: destInc = 1; break;
	  case 0x8000: destInc = -1; break;
	  }
	  unsigned long source = registers->getSAR0();
	  unsigned long destination = registers->getDAR0();
	  int size;
	  Memory *satmem = proc->getMemory();
	  switch (size = ((CHCR0 & 0x0C00) >> 10)) {
	  case 0:
	    while(registers->getTCR0() & 0x00FFFFFF) {
	      satmem->setByte(registers->getDAR0(),
		 	      satmem->getByte(registers->getSAR0()));
	      registers->setDAR0(registers->getDAR0() + destInc);
	      registers->setSAR0(registers->getSAR0() + srcInc);
	      registers->setTCR0(registers->getTCR0() - 1);
	    }
	    break;
	  case 1:
	    while(registers->getTCR0() & 0x00FFFFFF) {
	      satmem->setWord(registers->getDAR0(),
			    satmem->getWord(registers->getSAR0()));
	      registers->setDAR0(registers->getDAR0() + 2 * destInc);
	      registers->setSAR0(registers->getSAR0() + 2 * srcInc);
	      registers->setTCR0(registers->getTCR0() - 1);
	    }
	    break;
	  case 2:
	    while(registers->getTCR0() & 0x00FFFFFF) {
	      satmem->setLong(registers->getDAR0(),
				 satmem->getLong(registers->getSAR0()));
	      registers->setDAR0(registers->getDAR0() + 4 * destInc);
	      registers->setSAR0(registers->getSAR0() + 4 * srcInc);
	      registers->setTCR0(registers->getTCR0() - 1);
	    }
	    break;
	  case 3:
	    while(registers->getTCR0() & 0x00FFFFFF) {
	      for(int i = 0;i < 4;i++) {
	        satmem->setLong(registers->getDAR0(),
		  		  satmem->getLong(registers->getSAR0()));
	        registers->setDAR0(registers->getDAR0() + 4 * destInc);
	        registers->setSAR0(registers->getSAR0() + 4 * srcInc);
	        registers->setTCR0(registers->getTCR0() - 1);
	      }
	    }
	    break;
	  }
	}
	if (CHCR0 & 0x4)
#if DEBUG
	  cerr << "FIXME should launch an interrupt\n";
#endif
	registers->setCHCR0(CHCR0 | 0x2);
      }
      if (CHCR1 & 0x1) { // DMA for channel 1
      }
    }
  }
}

Interrupt::Interrupt(unsigned char l, unsigned char v) {
  _level = l;
  _vect = v;
}

unsigned char Interrupt::level(void) const {
  return _level;
}

unsigned char Interrupt::vector(void) const {
  return _vect;
}

Intc::Intc(SuperH *sh) {
  mutex = SDL_CreateMutex();
  mutex_cond = SDL_CreateMutex();
  cond = SDL_CreateCond();
  _stop = false;
  proc = sh;
}

Intc::~Intc(void) {
  SDL_DestroyCond(cond);
  SDL_DestroyMutex(mutex_cond);
  SDL_DestroyMutex(mutex);
}

SuperH *Intc::getSH(void) {
	return proc;
}

void Intc::send(const Interrupt& i) {
  bool empty = false;

  SDL_mutexP(mutex);
  
#ifndef _arch_dreamcast
  empty = interrupts.empty();
  interrupts.push(i);
#else
  empty = int_tree.root == NULL;
  __add_interrupt(&i);
#endif
  
  SDL_mutexV(mutex);

  if (empty) SDL_CondSignal(cond);
};

void Intc::lancer(Intc *intc) {
	intc->run();
}

void Intc::run(void) {
  while(!_stop) {
    while(proc->processingInterrupt()) proc->waitInterruptEnd();

    SDL_mutexP(mutex_cond);
#ifndef _arch_dreamcast
    while(interrupts.empty()) SDL_CondWait(cond, mutex);
#else
    while(int_tree.root == NULL) SDL_CondWait(cond, mutex);
#endif
    SDL_mutexV(mutex_cond);

    SDL_mutexP(mutex);
#ifndef _arch_dreamcast
    Interrupt interrupt = interrupts.top();
#else
    Interrupt interrupt = * __get_highest_int();
#endif
    SDL_mutexV(mutex);

    if (interrupt.level() == 255) return;

    if (interrupt.level() > proc->SR.partie.I) {
      proc->level() = interrupt.level();
      proc->vector() = interrupt.vector();
      proc->interrupt();

      SDL_mutexP(mutex);
#ifndef _arch_dreamcast
      interrupts.pop();
#else
      __del_highest_int();
#endif
      SDL_mutexV(mutex);
    }
  }
};

void Intc::stop(void) {
  _stop = true;
}

void Intc::sendNMI(void) {
  send(Interrupt(16, 11));
}

void Intc::sendUserBreak(void) {
  send(Interrupt(15, 12));
} 
