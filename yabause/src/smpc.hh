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

#ifndef SMPC_HH
#define SMPC_HH

#include "memory.hh"
#include "cpu.hh"

extern unsigned short buttonbits;

class Smpc;
class Scu;

class SmpcRegisters : public Memory {
private:
  Smpc *smpc;
public:
  SmpcRegisters(Scu *, SaturnMemory *);
  ~SmpcRegisters(void);
  unsigned char  getIREG    (int);
  unsigned char  getCOMREG  (void);
  unsigned char  getOREG    (int);
  unsigned char  getSR      (void);
  unsigned char  getSF      (void);
  void		 setIREG    (int, unsigned char);
  void		 setCOMREG  (unsigned char);
  void		 setOREG    (int, unsigned char);
  void		 setSR      (unsigned char);
  void		 setSF      (unsigned char);

  void           setByte   (unsigned long, unsigned char);
};

class Smpc : public Cpu {
private:
  bool dotsel; // 0 -> 320 | 1 -> 352
  bool mshnmi;
  bool sndres;
  bool cdres;
  bool sysres;
  bool resb;
  bool ste;
  bool resd;

  SmpcRegisters *memoire;
  bool intback;
  unsigned char intbackIreg0;
  bool firstPeri;

  Scu *scu;
  SaturnMemory *sm;
public:
  Smpc(SmpcRegisters *, Scu *, SaturnMemory *);

  static void executer(Smpc *);
  static void lancer(Smpc *);

  void INTBACK(void);
  void INTBACKStatus(void);
  void INTBACKPeripheral(void);
  void RESENAB(void);
  void RESDISA(void);
  void CDON(void);
  void CDOFF(void);
};

#endif
