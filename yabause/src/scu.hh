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

#ifndef SCU_HH
#define SCU_HH

#include "memory.hh"
#include "intc.hh"
#include "superh.hh"

class Scu : public Memory {
private:
  template<unsigned char V, unsigned char L, unsigned short M>
	  void sendInterrupt(void);
  SaturnMemory *satmem;
  unsigned long timer0;
  unsigned long timer1;
public:
  Scu(SaturnMemory *);
  void reset(void);

  unsigned long getLong(unsigned long);
  void setLong(unsigned long, unsigned long);

  void DMA(int);
					//source|vector | level | mask
					//------------------------------
  void sendVBlankIN(void);	// VDP2	| 40	| F	| 0x0001
  void sendVBlankOUT(void);	// VDP2	| 41	| E	| 0x0002
  void sendHBlankIN(void);	// VDP2	| 42	| D	| 0x0004
  void sendTimer0(void);	// SCU	| 43	| C	| 0x0008
  void sendTimer1(void);	// SCU	| 44	| B	| 0x0010
  void sendDSPEnd(void);	// SCU	| 45	| A	| 0x0020
  void sendSoundRequest(void);	// SCSP	| 46	| 9	| 0x0040
  void sendSystemManager(void);	// SM	| 47	| 8	| 0x0080
  void sendPadInterrupt(void);	// PAD	| 48	| 8	| 0x0100
  void sendLevel2DMAEnd(void);	// ABus	| 49	| 6	| 0x0200
  void sendLevel1DMAEnd(void);	// ABus	| 4A	| 6	| 0x0400
  void sendLevel0DMAEnd(void);	// ABus	| 4B	| 5	| 0x0800
  void sendDMAIllegal(void);	// SCU	| 4C	| 3	| 0x1000
  void sendDrawEnd(void);	// VDP1	| 4D	| 2	| 0x2000
  //template<unsigned char E> static void sendExternalInterrupt(void);
};

template<unsigned char V, unsigned char L, unsigned short M>
void Scu::sendInterrupt(void) {
    Memory::setLong(0xA4, M);
  if (!(Memory::getWord(0xA2) & M)) {
    ((SuperH *) satmem->getMasterSH())->send(Interrupt(L, V));
#if 0
    cerr << "interrupt send " << (int) V << endl;
#endif
  }
  else {
    if (V == 0x47) cerr << "sm interrupt masked " << endl;
  }
}

/*
template<unsigned char E>
void Scu::sendExternalInterrupt(void) {
  sendInterrupt<0x50 + E, 
*/
  
#endif
