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

class SaturnMemory;

extern unsigned short buttonbits;

class Smpc : public Memory {
private:
  bool dotsel; // 0 -> 320 | 1 -> 352
  bool mshnmi;
  bool sndres;
  bool cdres;
  bool sysres;
  bool resb;
  bool ste;
  bool resd;

  bool intback;
  unsigned char intbackIreg0;
  bool firstPeri;

  unsigned char regionid;

  unsigned char SMEM[4];

  SaturnMemory *sm;
  long timing;
public:
  Smpc(SaturnMemory *);
  void reset(void);

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

  static void execute(Smpc *);
  static void intcont(Smpc *);
  void execute2(unsigned long);
  void setTiming(void);

  void INTBACK(void);
  void INTBACKStatus(void);
  void INTBACKPeripheral(void);
  void INTBACKEnd(void);
  void RESENAB(void);
  void RESDISA(void);
  void SNDON(void);
  void SNDOFF(void);
  void SETSMEM(void);
  void SSHOFF(void);
  void SSHON(void);
  void CKCHG352(void);
  void CKCHG320(void);

  int saveState(FILE *fp);
  int loadState(FILE *fp, int version, int size);
};

#endif
