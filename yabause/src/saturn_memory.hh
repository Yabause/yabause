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

#ifndef SATURN_MEMORY_HH
#define SATURN_MEMORY_HH

#include "registres.hh"

class SuperH;

class SaturnMemory : public Memory {
public:
  Memory *rom;		//        0 -    80000
  Smpc *smpc;		//   100000 -   100080
  Memory *ram;		//   180000 -   190000
  Memory *ramLow;	//   200000 -   300000
  InputCaptureSignal * minit;	//  1000000 -  1000004
  InputCaptureSignal * sinit;	//  1800000 -  1800004
  Cs0 *cs0;           //  2000000 -  4000000
  Cs1 *cs1;		//  4000000 -  5000000
  Dummy *dummy;	//  5000000 -  5800000
  Cs2 *cs2;		//  5800000 -  5900000
  ScspRam *sound;	//  5A00000 -  5A80000
  Scsp *soundr;	//  5B00000 -  5B00EE4
  Memory *vdp1_1;	//  5C00000 -  5CC0000
  Vdp1 *vdp1_2;	//  5D00000 -  5D00018
  Vdp2Ram *vdp2_1;	//  5E00000 -  5E80000
  Vdp2ColorRam *vdp2_2;	//  5F00000 -  5F01000
  Vdp2 *vdp2_3;	//  5F80000 -  5F80120
  Scu *scu;		//  5FE0000 -  5FE00D0
  Memory *ramHigh;	//  6000000 -  6100000
  Memory *unhandled;    //  Anything Not specified

  Memory *mapMem;
  unsigned long mapAddr;
  void mapping(unsigned long);
  void initMemoryMap(void);
  void initMemoryHandler(int, int, Memory *);
  Memory * memoryMap[0x800];

  SuperH *msh;
  SuperH *ssh;
  SuperH *cursh;

  bool _stop;

  char *cdrom;
	int decilineCount;
	int lineCount;
	int decilineStop;
	int duf;
	unsigned long cycleCountII;
//public:
  SaturnMemory(void);
  ~SaturnMemory(void);

  unsigned char  getByte (unsigned long);
  void           setByte (unsigned long, unsigned char);
  unsigned short getWord (unsigned long);
  void           setWord (unsigned long, unsigned short);
  unsigned long  getLong (unsigned long);
  void           setLong (unsigned long, unsigned long);

  void           load    (const char *,unsigned long);

  void loadBios(const char *);
  void loadExec(const char *file, unsigned long PC);
  void FormatBackupRam();

  SuperH *getMasterSH(void);
  SuperH *getCurrentSH(void);
  SuperH *getSlaveSH(void);
  bool sshRunning;

/*
  Memory *getCS2(void);
  Memory *getVdp1Ram(void);
  Memory *getVdp1(void);
  Memory *getScu(void);
  Memory *getVdp2(void);
  Memory *getSmpc(void);
  Memory *getScsp(void);
*/

  void changeTiming(unsigned long, bool);
  void synchroStart(void);

  void startSlave(void);
  void stopSlave(void);

  int saveState(const char *filename);
  int loadState(const char *filename);
};

unsigned char inline readByte(SaturnMemory * mem , unsigned long addr) {
	mem->mapping(addr);
	return mem->mapMem->getByte(mem->mapAddr);
}

unsigned short inline readWord(SaturnMemory * mem, unsigned long addr) {
	mem->mapping(addr);
	return mem->mapMem->getWord(mem->mapAddr);
}

unsigned long inline readLong(SaturnMemory * mem, unsigned long addr) {
	mem->mapping(addr);
	return mem->mapMem->getLong(mem->mapAddr);
}

#endif
