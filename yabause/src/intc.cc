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

Onchip::Onchip(SaturnMemory *sm) : Memory(0x1FF) {
	memory = sm;
	setByte(4, 0x84);

	intcThread = SDL_CreateThread((int (*)(void*)) &Onchip::startINTC, this);
#ifdef _arch_dreamcast
	__init_tree();
#endif
	mutex = SDL_CreateMutex();
	mutex_cond = SDL_CreateMutex();
	cond = SDL_CreateCond();
	_stop = false;
}

Onchip::~Onchip(void) {
#if DEBUG
  cerr << "stopping intc\n";
#endif
  send(Interrupt(255, 0));
  SDL_WaitThread(intcThread, NULL);
#if DEBUG
  cerr << "intc stopped\n";
#endif
  SDL_DestroyCond(cond);
  SDL_DestroyMutex(mutex_cond);
  SDL_DestroyMutex(mutex);
}

#define IPRB	0x60
#define ICR	0xE0
#define IPRA	0xE2
#define SAR0	0x180
#define DAR0	0x184
#define TCR0	0x188
#define CHCR0	0x18C
#define CHCR1	0x19C
#define DMAOR	0x1B0

void Onchip::setLong(unsigned long addr, unsigned long val) {
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
    SDL_CreateThread((int (*)(void*)) &Onchip::startDMA, this);
    break;
  default:
    Memory::setLong(addr, val);
  }
}

void Onchip::startDMA(Onchip *onchip) {
	onchip->runDMA();
}

void Onchip::runDMA(void) {
	unsigned long dmaor = getLong(DMAOR);

	if ((dmaor & 0x1) && !((dmaor & 0x4) || (dmaor & 0x2))) {
		unsigned long chcr0 = getLong(CHCR0);
		unsigned long chcr1 = getLong(CHCR1);
		if ((chcr0 & 0x1) && (chcr1 & 0x1)) { // both channel wants DMA
			if (dmaor & 0x8) { // round robin priority
			}
			else { // channel 0 > channel 1 priority
			}
		}
		else { // only one channel wants DMA
			if (chcr0 & 0x1) { // DMA for channel 0
				if (!(chcr0 & 0x2)) { // TE is not set
					int srcInc;
					switch(chcr0 & 0x3000) {
						case 0x0000: srcInc = 0; break;
	  					case 0x1000: srcInc = 1; break;
						case 0x2000: srcInc = -1; break;
					}
					int destInc;
					switch(chcr0 & 0xC000) {
						case 0x0000: destInc = 0; break;
						case 0x4000: destInc = 1; break;
						case 0x8000: destInc = -1; break;
					}
					unsigned long source = getLong(SAR0);
					unsigned long destination = getLong(DAR0);
					int size;
					switch (size = ((chcr0 & 0x0C00) >> 10)) {
						case 0:
							while(getLong(TCR0) & 0x00FFFFFF) {
								memory->setByte(getLong(DAR0), memory->getByte(getLong(SAR0)));
								setLong(DAR0, getLong(DAR0) + destInc);
								setLong(SAR0, getLong(SAR0) + srcInc);
								setLong(TCR0, getLong(TCR0) - 1);
							}
							break;
						case 1:
							while(getLong(TCR0) & 0x00FFFFFF) {
								memory->setWord(getLong(DAR0), memory->getWord(getLong(SAR0)));
								setLong(DAR0, getLong(DAR0) + 2 * destInc);
								setLong(SAR0, getLong(SAR0) + 2 * srcInc);
								setLong(TCR0, getLong(TCR0) - 1);
							}
							break;
						case 2:
							while(getLong(TCR0) & 0x00FFFFFF) {
								memory->setLong(getLong(DAR0), memory->getLong(getLong(SAR0)));
								setLong(DAR0, getLong(DAR0) + 4 * destInc);
								setLong(SAR0, getLong(SAR0) + 4 * srcInc);
								setLong(TCR0, getLong(TCR0) - 1);
							}
							break;
						case 3:
							while(getLong(TCR0) & 0x00FFFFFF) {
								for(int i = 0;i < 4;i++) {
									memory->setLong(getLong(DAR0), memory->getLong(getLong(SAR0)));
									setLong(DAR0, getLong(DAR0) + 4 * destInc);
									setLong(SAR0, getLong(SAR0) + 4 * srcInc);
									setLong(TCR0, getLong(TCR0) - 1);
								}
							}
							break;
					}
				}
				if (chcr0 & 0x4)
#if DEBUG
					cerr << "FIXME should launch an interrupt\n";
#endif
				setLong(chcr0, chcr0 | 0x2);
			}
			if (chcr1 & 0x1) { // DMA for channel 1
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

void Onchip::send(const Interrupt& i) {
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

void Onchip::startINTC(Onchip *onchip) {
	onchip->runINTC();
}

void Onchip::runINTC(void) {
  SuperH * proc = memory->getMasterSH();
  while(!_stop) {
    while(proc->processingInterrupt()) proc->waitInterruptEnd();

    SDL_mutexP(mutex_cond);
#ifndef _arch_dreamcast
    while(interrupts.empty()) SDL_CondWait(cond, mutex_cond);
#else
    while(int_tree.root == NULL) SDL_CondWait(cond, mutex_cond);
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

void Onchip::stopINTC(void) {
  _stop = true;
}

void Onchip::sendNMI(void) {
  send(Interrupt(16, 11));
}

void Onchip::sendUserBreak(void) {
  send(Interrupt(15, 12));
} 
