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

#ifndef MEMORY_HH
#define MEMORY_HH

#include "exception.hh"

class Memory {
private:
  unsigned char *base_mem;
  unsigned char *memory;
protected:
  unsigned long size;
public:
  Memory(unsigned long);
  virtual ~Memory(void);

  virtual unsigned char  getByte (unsigned long);
  virtual void           setByte (unsigned long, unsigned char);
  virtual unsigned short getWord (unsigned long);
  virtual void           setWord (unsigned long, unsigned short);
  virtual unsigned long  getLong (unsigned long);
  virtual void           setLong (unsigned long, unsigned long);

  unsigned long getSize() const;
  virtual void load(const char *, unsigned long);

#ifndef _arch_dreamcast
  friend ostream& operator<<(ostream&, const Memory&);
#endif
};

class LoggedMemory : public Memory {
protected:
  Memory *mem;
  char nom[11];
  bool destroy;
public:
  LoggedMemory(const char *, Memory *, bool);
  virtual ~LoggedMemory(void);
  Memory *getMemory(void);
  unsigned char  getByte (unsigned long);
  void           setByte (unsigned long, unsigned char);
  unsigned short getWord (unsigned long);
  void           setWord (unsigned long, unsigned short);
  unsigned long  getLong (unsigned long);
  void           setLong (unsigned long, unsigned long);
};

class Dummy : public Memory {
public:
  Dummy(void) : Memory(0) {}
  ~Dummy(void) {}
  unsigned char getByte(unsigned long) { return 0; }
  void setByte(unsigned long, unsigned char) {}
  unsigned short getWord(unsigned long) { return 0; }
  void setWord(unsigned long, unsigned short) {}
  unsigned long getLong(unsigned long) { return 0; }
  void setLong(unsigned long, unsigned long) {}
};

class SuperH;

class SaturnMemory : public Memory {
private:
  Memory *rom;		//        0 -    80000
  Memory *smpc;		//   100000 -   100080
  Memory *ram;		//   180000 -   190000
  Memory *ramLow;	//   200000 -   300000
  Memory *minit;	//  1000000 -  1000004
  Memory *sinit;	//  1800000 -  1800004
  Memory *cs0;		//  2000000 -  4000000
  Memory *cs1;		//  4000000 -  5000000
  Memory *dummy;	//  5000000 -  5800000
  Memory *cs2;		//  5800000 -  5900000
  Memory *sound;	//  5A00000 -  5A80000
  Memory *soundr;	//  5B00000 -  5B00EE4
  Memory *vdp1_1;	//  5C00000 -  5CC0000
  Memory *vdp1_2;	//  5D00000 -  5D00018
  Memory *vdp2_1;	//  5E00000 -  5E80000
  Memory *vdp2_2;	//  5F00000 -  5F01000
  Memory *vdp2_3;	//  5F80000 -  5F80120
  Memory *scu;		//  5FE0000 -  5FE00D0
  Memory *ramHigh;	//  6000000 -  6100000
  Memory *purgeArea;	// 40000000 - 50000000
  Memory *adressArray;	// 60000000 - 600003FF
  Memory *modeSdram;	// FFFF8000 - FFFFBFFF
  Memory *onchip;	// FFFFFE00 - FFFFFFFF

  Memory *mapMem;
  unsigned long mapAdr;
  void mappage(unsigned long);

  SuperH *msh;
  SDL_Thread *mshThread;

public:
  SaturnMemory(const char * = NULL, const char * = NULL);
  ~SaturnMemory(void);

  unsigned char  getByte (unsigned long);
  void           setByte (unsigned long, unsigned char);
  unsigned short getWord (unsigned long);
  void           setWord (unsigned long, unsigned short);
  unsigned long  getLong (unsigned long);
  void           setLong (unsigned long, unsigned long);

  void           load    (const char *,unsigned long);

  void loadBios(const char *);

  SuperH *getMasterSH(void);

  Uint8 *keys;

  Memory *getOnchip(void);
  Memory *getVdp1Ram(void);
  Memory *getVdp1(void);
  Memory *getScu(void);
};

#endif
