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

#ifndef INTC_HH
#define INTC_HH

#include "memory.hh"
#include "cpu.hh"

#ifndef _arch_dreamcast
#include <queue>
#endif

class Interrupt {
private:
  unsigned char _level;
  unsigned char _vect;
public:
  Interrupt(unsigned char, unsigned char);
  unsigned char level(void) const;
  unsigned char vector(void) const;
};

#ifndef _arch_dreamcast
namespace std {
template<> struct less<Interrupt> {
public:
  bool operator()(const Interrupt& a, const Interrupt& b) {
    return a.level() < b.level();
  }
};
}
#endif

class Onchip : public Memory {
private:
	SaturnMemory *memory;
	unsigned long timing;

        /* DMAC */
        inline void DMATransfer(unsigned long chcr, unsigned long reg_offset);

	/* INTC */

public:
#ifndef _arch_dreamcast
	//priority_queue<Interrupt> interrupts;
#endif

	Onchip(SaturnMemory *);

        void setByte(unsigned long addr, unsigned char val);
	void setWord(unsigned long, unsigned short);
	void setLong(unsigned long, unsigned long);

	void run(unsigned long);

	/* DMAC */
	void runDMA(void);

	/* INTC */
/*
	void sendNMI(void);
	void sendUserBreak(void);
	void send(const Interrupt&);
*/
	//void sendOnChip(...);
};

#endif
