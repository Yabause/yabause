/*  Copyright 2004 Guillaume Duhamel
    Copyright 2004 Stephane Dallongeville
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

#ifndef SCSP_HH
#define SCSP_HH

#include "memory.hh"
#include "c68k/types.h"

class SaturnMemory;

typedef struct
{
   unsigned long D[8];
   unsigned long A[8];
   unsigned long SR;
   unsigned long PC;
} m68kregs_struct;

class ScspRam : public Memory {
private:
  unsigned char *base_mem;
public: 
  ScspRam(void) : Memory(0x7FFFF, 0x80000) {
  base_mem = Memory::getBuffer();
  }
  ~ScspRam(void) {}

  unsigned char getByte(unsigned long addr);
  void setByte(unsigned long addr, unsigned char val);
  unsigned short getWord(unsigned long addr);
  void setWord(unsigned long addr, unsigned short val);
  unsigned long getLong(unsigned long addr);
  void setLong(unsigned long addr, unsigned long val);
};

class Scsp : public Dummy {
private:
  ScspRam *sram;
  SaturnMemory *satmem;
  unsigned long scsptiming1;
  float scsptiming2;
  FILE *debugfp;
  void Scsp::Convert32to16s(long *, long *, short *, unsigned long);
public:
  Scsp(SaturnMemory *);
  ~Scsp(void);

  ScspRam *getSRam(void);

  void reset68k(void);
  void reset(void);
  void run68k(unsigned long);
  void step68k(void);
  void run(void);

  bool is68kOn;

  unsigned char getByte(unsigned long addr);
  void setByte(unsigned long addr, unsigned char val);
  unsigned short getWord(unsigned long addr);
  void setWord(unsigned long addr, unsigned short val);
  unsigned long getLong(unsigned long addr);
  void setLong(unsigned long addr, unsigned long val);

  void Get68kRegisters(m68kregs_struct *regs);
  void Set68kRegisters(m68kregs_struct *regs);
};

#ifdef __GNUC__

extern void		scsp_w_b(u32, u8);
extern void		scsp_w_w(u32, u16);
extern void		scsp_w_d(u32, u32);
extern u8		scsp_r_b(u32);
extern u16		scsp_r_w(u32);
extern u32		scsp_r_d(u32);

#else

extern DECL_FASTCALL(u8,	scsp_r_b(u32));
extern DECL_FASTCALL(u16,	scsp_r_w(u32));
extern DECL_FASTCALL(u32,	scsp_r_d(u32));
extern DECL_FASTCALL(void,	scsp_w_b(u32, u8));
extern DECL_FASTCALL(void,	scsp_w_w(u32, u16));
extern DECL_FASTCALL(void,	scsp_w_d(u32, u32));

#endif

extern void		scsp_init(u8 *scsp_ram, void *sint_hand, void *mint_hand);
extern void		scsp_shutdown(void);
extern void		scsp_reset(void);

extern char		* scsp_debug_ilist(int, char *);
extern char		* scsp_debug_slist_on(void);

extern void		scsp_midi_in_send(u8 data);
extern void		scsp_midi_out_send(u8 data);
extern u8		scsp_midi_in_read(void);
extern u8		scsp_midi_out_read(void);
extern void		scsp_update(s32 *bufL, s32 *bufR, u32 len);
extern void		scsp_update_timer(u32 len);

////////////////////////////////////////////////////////////////
// Sound Interface

extern void		sound_init(void);
extern void		sound_reset(void);
extern void		sound_shutdown(void);

#endif
