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

class Intc;
class Dmac;

class OnchipRegisters : public Memory {
private:
  Intc *intc;
  Dmac *dmac;
  SDL_Thread *intcThread;
public:
  OnchipRegisters(SuperH *);
  ~OnchipRegisters(void);

  Intc *getIntc(void);
  
  unsigned short	getIPRA(void);
  unsigned short	getIPRB(void);
  unsigned short	getVCRA(void);
  unsigned short	getVCRB(void);
  unsigned short	getVCRC(void);
  unsigned short	getVCRD(void);
  unsigned short	getVCRWDT(void);
  unsigned long		getVCRDIV(void);
  unsigned long		getVCRDMA(int);
  unsigned short	getICR(void);
  unsigned long		getDMAOR(void);
  unsigned long		getCHCR0(void);
  unsigned long		getCHCR1(void);
  unsigned long		getSAR0(void);
  unsigned long		getDAR0(void);
  unsigned long		getTCR0(void);
  void			setCHCR0(unsigned long);
  void			setSAR0(unsigned long);
  void			setDAR0(unsigned long);
  void			setTCR0(unsigned long);
  void			setSSR(unsigned char);
  void setLong(unsigned long, unsigned long);
#ifndef _arch_dreamcast
  friend ostream& operator<<(ostream&, OnchipRegisters&);
#endif
};

class Dmac : public Cpu {
private:
  OnchipRegisters *registers;
  SuperH *proc;
public:
  Dmac(OnchipRegisters *, SuperH *);
  static void lancer(Dmac *);
  void executer(void);
};

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

class SuperH;

class Intc {
private:
  bool _stop;
#ifndef _arch_dreamcast
  priority_queue<Interrupt> interrupts;
#endif
  SuperH *proc;
  SDL_mutex *mutex, *mutex_cond;
  SDL_cond *cond;

public:
  Intc(SuperH *);
  ~Intc(void);

  SuperH *getSH(void);

  void sendNMI(void);
  void sendUserBreak(void);
  void send(const Interrupt&);
  //void sendOnChip(...);
  void run(void);
  void stop(void);

  static void lancer(Intc *);
};

#endif
