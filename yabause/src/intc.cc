/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004 Theo Berkau

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
#include "timer.hh"

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

#define TIER    0x10
#define FTCSR   0x11
#define FRCH    0x12
#define FRCL    0x13
#define ORCH    0x14
#define ORCL    0x15
#define TCR     0x16
#define TOCR    0x17
#define ICRH    0x18
#define ICRL    0x19

#define IPRB	0x60
#define VCRA    0x62
#define VCRB    0x64
#define VCRC    0x66
#define VCRD    0x68
#define ICR	0xE0
#define IPRA	0xE2
#define VCRWDT  0xE4
#define VCRDIV  0x10C
#define SAR0	0x180
#define DAR0	0x184
#define TCR0	0x188
#define CHCR0	0x18C
#define CHCR1	0x19C
#define VCRDMA0 0x1A0
#define VCRDMA1 0x1A8
#define DMAOR   0x1B0
#define BCR1    0x1E0
#define BCR2    0x1E4

Onchip::Onchip(bool slave, SaturnMemory *sm, SuperH *sh) : Memory(0x1FF, 0x1FF) {
	memory = sm;
        shparent = sh;
        isslave = slave;

        reset();

#ifdef _arch_dreamcast
	__init_tree();
#endif
}

void Onchip::reset(void) {
        Memory::setByte(4, 0x84);

        // Initialize Bus Control registers(needed for Slave SH2 emulation)
        Memory::setLong(BCR1, 0x03F0 | (isslave << 15));
        Memory::setLong(BCR2, 0x00FC);

        // Initialize the Free-running Timer registers
        Memory::setByte(TIER, 0x01);
        Memory::setByte(FTCSR, 0x00);
        Memory::setByte(FRCH, 0x00);
        Memory::setByte(FRCL, 0x00);
        Memory::setByte(ORCH, 0xFF);
        Memory::setByte(ORCL, 0xFF);
        Memory::setByte(TCR, 0x00);
        Memory::setByte(TOCR, 0xE0);
        Memory::setByte(ICRH, 0x00);
        Memory::setByte(ICRL, 0x00);

	timing = 0;
        ccleftover = 0;
        frcdiv = 8;
}

void Onchip::setByte(unsigned long addr, unsigned char val) {
  switch(addr) {
  case TCR: {
    Memory::setByte(addr, val);

    switch (val & 3) {
       case 0:
               frcdiv = 8;
               break;
       case 1:
               frcdiv = 32;
               break;
       case 2:
               frcdiv = 128;
               break;
       case 3:
#if DEBUG
               cerr << "onchip\t: FRT external input clock not implemented" << endl;
#endif
               break;
    }

    break;
  }
  case FTCSR: {
    // the only thing you can do to bits 1-7 is clear them
    Memory::setByte(addr, (Memory::getByte(addr) & (val & 0xFE)) | (val & 0x1));
    break;
  }
  case ICRH:
  case ICRL:
  {
    // don't allow data to be written to them
    break;
  }
#if DEBUG
  case TIER:
    if (val & 0x80)
      cerr << "onchip\t: WARNING: Input Capture Flag Interrupts enabled" << endl;
    else if (val & 0x8)
      cerr << "onchip\t: WARNING: Output Compare A Interrupts enabled" << endl;
    else if (val & 0x4)
      cerr << "onchip\t: WARNING: Output Compare B Interrupts enabled" << endl;
    else if (val & 0x2)
      cerr << "onchip\t: WARNING: Timer overflow Interrupts enabled" << endl;
       
    Memory::setByte(addr, val);
    break;
#endif
  default:
    Memory::setByte(addr, val);
  }
}

void Onchip::setWord(unsigned long addr, unsigned short val) {
	switch(addr) {
		case ICR:
			if (val & 0x8000) {
				//SDL_CreateThread(&Timer::call<Onchip, &Onchip::sendNMI, 100>, this); // random value
				memory->getMasterSH()->timing = 100;
			}
	}
	Memory::setWord(addr, val);
}

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
  case CHCR0:
  case CHCR1:
    Memory::setLong(addr, val);

    // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
    // and CHCR's DE bit is set and TE bit is cleared,
    // do a dma transfer
    if ((Memory::getLong(DMAOR) & 7) == 1 && (val & 0x3) == 1)
       runDMA();
    break;
  case DMAOR:
    Memory::setLong(addr, val);

    // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
    // and CHCR's DE bit is set and TE bit is cleared,
    // do a dma transfer
    if ((val & 7) == 1)
       runDMA();
    break;
  default:
    Memory::setLong(addr, val);
  }
}

inline void Onchip::DMATransfer(unsigned long chcr, unsigned long reg_offset)
{
   if (!(chcr & 0x2)) { // TE is not set
      int srcInc;
      switch(chcr & 0x3000) {
         case 0x0000: srcInc = 0; break;
         case 0x1000: srcInc = 1; break;
         case 0x2000: srcInc = -1; break;
      }

      int destInc;
      switch(chcr & 0xC000) {
         case 0x0000: destInc = 0; break;
         case 0x4000: destInc = 1; break;
         case 0x8000: destInc = -1; break;
      }

      unsigned long source = getLong(SAR0+reg_offset);
      unsigned long destination = getLong(DAR0+reg_offset);
      int size;

      switch (size = ((chcr & 0x0C00) >> 10)) {
         case 0:
            while(getLong(TCR0+reg_offset) & 0x00FFFFFF) {
               memory->setByte(getLong(DAR0+reg_offset), memory->getByte(getLong(SAR0+reg_offset)));
               setLong(DAR0+reg_offset, getLong(DAR0+reg_offset) + destInc);
               setLong(SAR0+reg_offset, getLong(SAR0+reg_offset) + srcInc);
               setLong(TCR0+reg_offset, getLong(TCR0+reg_offset) - 1);
            }
            break;
         case 1:
            while(getLong(TCR0+reg_offset) & 0x00FFFFFF) {
               memory->setWord(getLong(DAR0+reg_offset), memory->getWord(getLong(SAR0+reg_offset)));
               setLong(DAR0+reg_offset, getLong(DAR0+reg_offset) + 2 * destInc);
               setLong(SAR0+reg_offset, getLong(SAR0+reg_offset) + 2 * srcInc);
               setLong(TCR0+reg_offset, getLong(TCR0+reg_offset) - 1);
            }
            break;
         case 2:
            while(getLong(TCR0+reg_offset) & 0x00FFFFFF) {
               memory->setLong(getLong(DAR0+reg_offset), memory->getLong(getLong(SAR0+reg_offset)));
               setLong(DAR0+reg_offset, getLong(DAR0+reg_offset) + 4 * destInc);
               setLong(SAR0+reg_offset, getLong(SAR0+reg_offset) + 4 * srcInc);
               setLong(TCR0+reg_offset, getLong(TCR0+reg_offset) - 1);
            }
            break;
         case 3:
            while(getLong(TCR0+reg_offset) & 0x00FFFFFF) {
               for(int i = 0;i < 4;i++) {
                  memory->setLong(getLong(DAR0+reg_offset), memory->getLong(getLong(SAR0+reg_offset)));
                  setLong(DAR0+reg_offset, getLong(DAR0+reg_offset) + 4 * destInc);
                  setLong(SAR0+reg_offset, getLong(SAR0+reg_offset) + 4 * srcInc);
                  setLong(TCR0+reg_offset, getLong(TCR0+reg_offset) - 1);
               }
            }
            break;
      }
   }

   if (chcr & 0x4)
   {
#if DEBUG
      cerr << "FIXME should launch an interrupt\n";
#endif
        shparent->send(Interrupt((Memory::getWord(IPRA) & 0xF00) >> 8, Memory::getLong(VCRDMA0+(reg_offset / 2))));
   }                                                                    

   // Set Transfer End bit
   Memory::setLong(CHCR0+reg_offset, chcr & 0xFFFFFFFE | 0x2);
}

void Onchip::runDMA(void) {
	unsigned long dmaor = getLong(DMAOR);

	if ((dmaor & 0x1) && !((dmaor & 0x4) || (dmaor & 0x2))) {
		unsigned long chcr0 = getLong(CHCR0);
		unsigned long chcr1 = getLong(CHCR1);
		if ((chcr0 & 0x1) && (chcr1 & 0x1)) { // both channel wants DMA
			if (dmaor & 0x8) { // round robin priority
#if DEBUG
                           cerr << "dma\t: FIXME: two channel dma - round robin priority not properly implemented\n";
#endif
                           DMATransfer(chcr0, 0);
                           DMATransfer(chcr1, 0x10);
			}
			else { // channel 0 > channel 1 priority
                           DMATransfer(chcr0, 0);
                           DMATransfer(chcr1, 0x10);
                        }
		}
		else { // only one channel wants DMA
			if (chcr0 & 0x1) { // DMA for channel 0
                           DMATransfer(chcr0, 0);
			}
			if (chcr1 & 0x1) { // DMA for channel 1
                           DMATransfer(chcr1, 0x10);
			}
		}
	}
}

void Onchip::runFRT(unsigned long cc) {
   unsigned long frctemp;

   frctemp = (unsigned long)Memory::getWord(FRCH);

   // Increment FRC
   frctemp += ((cc + ccleftover) / frcdiv);
   ccleftover = (cc + ccleftover) % frcdiv;

   // If FRC overflows, set overflow flag
   if (frctemp > 0xFFFF) {
      // Do we need to trigger an interrupt?

      Memory::setByte(FTCSR, Memory::getByte(FTCSR) | 2);
   }

   // Write new FRC value
   Memory::setWord(FRCH, (unsigned short)frctemp);

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

/*
void Onchip::send(const Interrupt& i) {
#ifndef _arch_dreamcast
  interrupts.push(i);
#else
  __add_interrupt(&i);
#endif
};

void Onchip::sendNMI(void) {
  send(Interrupt(16, 11));
}

void Onchip::sendUserBreak(void) {
  send(Interrupt(15, 12));
} 

void Onchip::run(unsigned long t) {
	if (timing > 0) {
		timing -= t;
		if (timing <= 0) {
			sendNMI();
		}
	}
}
*/

void Onchip::inputCaptureSignal(void) {
   // Set Input Capture Flag
   Memory::setByte(FTCSR, Memory::getByte(FTCSR) | 0x80);

   // Copy FRC registers to Input Capture Registers
   Memory::setWord(ICRH, Memory::getWord(FRCH));

   // Generate an Interrupt?
   if (Memory::getByte(TIER) & 0x80)  {
//#if DEBUG
//      fprintf(stderr, "onchip\t: ICI interrupt: level = %d vector = %d\n", (Memory::getWord(IPRB) & 0xF00) >> 8, (Memory::getWord(VCRC) & 0x7F00) >> 8);
//#endif
        shparent->send(Interrupt((Memory::getWord(IPRB) & 0xF00) >> 8, (Memory::getWord(VCRC) & 0x7F) >> 8));
   }
}

InputCaptureSignal::InputCaptureSignal(SuperH *icsh) : Memory(0, 4) {
   onchip = (Onchip *)icsh->GetOnchip();
}

void InputCaptureSignal::setWord(unsigned long addr, unsigned short val) {
   onchip->inputCaptureSignal();
}

