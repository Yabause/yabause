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

#ifndef CS2_HH
#define CS2_HH

#include "memory.hh"
#include "cpu.hh"

class YCD;

class Cs2 : public Memory {
private:
  YCD *cd;
  SDL_Thread *cdThread;
  SDL_mutex *cdMutex;
public:
  Cs2(void);
  ~Cs2(void);
  unsigned short getHIRQ(void);
  unsigned short getHIRQMask(void);
  unsigned short getCR1(void);
  unsigned short getCR2(void);
  unsigned short getCR3(void);
  unsigned short getCR4(void);
  void setHIRQ(unsigned short val);
  void setHIRQMask(unsigned short val);
  void setCR1(unsigned short val);
  void setCR2(unsigned short val);
  void setCR3(unsigned short val);
  void setCR4(unsigned short val);

  void setWord(unsigned long, unsigned short);
};

class YCD : public Cpu {
private:
  unsigned long FAD;
  unsigned char statut;
  unsigned char options;
  unsigned char repcnt;
  unsigned char ctrladdr;
  unsigned char piste;
  unsigned char index;

  unsigned long cdwnum;

  bool _command;
  Cs2 *memoire;
public:
  YCD(Cs2 *);

  static void lancer(YCD *);
  void executer(void);
#if 0
  void command(void);
  void periodicUpdate(void);
#endif

  //   command name                command code
  void getStatus(void);		// 0x00
  void getHardwareInfo(void);	// 0x01
  void initializeCDSystem(void);// 0x04
  void endDataTransfer(void);	// 0x06
  void resetSelector(void);	// 0x48
  void setSectorLength(void);	// 0x60
  void getCopyError(void);	// 0x67
  void abortFile(void);		// 0x75
};

#endif
