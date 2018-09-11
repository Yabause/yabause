/*
 * Copyright 2004 Stephane Dallongeville
 * Copyright 2004-2007 Theo Berkau
 * Copyright 2006 Guillaume Duhamel
 * Copyright 2012 Chris Lord
 *
 * This file is part of Yabause.
 *
 * Yabause is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Yabause is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Yabause; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

/*! \file scsp.c
    \brief SCSP emulation functions.
*/

////////////////////////////////////////////////////////////////
// Custom Sound Processor

// note: model2 scsp is mapped to 0x100000~0x100ee4 of the space, but seems to
//       have additional hw ports ($40a~$410)
// note: it seems that the user interrupt is used by the sound driver to reset
//       the subsystem

//--------------------------------------------------------------
//
// Common Control Register (CCR)
//
//      $+00      $+01
// $400 ---- --12 3333 4444 1:MEM4MB memory size 2:DAC18B dac for digital output 3:VER version number 4:MVOL
// $402 ---- ---1 1222 2222 1:RBL ring buffer length 2:RBP lead address
// $404 ---1 2345 6666 6666 1:MOFULL out fifo full 2:MOEMP empty 3:MIOVF overflow 4:MIFULL in 5:MIEMP 6:MIBUF
// $406 ---- ---- 1111 1111 1:MOBUF midi output data buffer
// $408 1111 1222 2334 4444 1:MSLC monitor slot 2:CA call address 3:SGC Slot phase 4:EG Slot envelope
// $40a ---- ---- ---- ----
// $40c ---- ---- ---- ----
// $40e ---- ---- ---- ----
// $410 ---- ---- ---- ----
// $412 1111 1111 1111 111- 1:DMEAL transfer start address (sound)
// $414 1111 2222 2222 222- 1:DMEAH transfer start address hi 2:DRGA start register address (dsp)
// $416 -123 4444 4444 444- 1:DGATE transfer gate 0 clear 2:DDIR direction 3:DEXE start 4:DTLG data count
// $418 ---- -111 2222 2222 1:TACTL timer a prescalar control 2:TIMA timer a count data
// $41a ---- -111 2222 2222 1:TBCTL timer b prescalar control 2:TIMB timer b count data
// $41c ---- -111 2222 2222 2:TCCTL timer c prescalar control 2:TIMC timer c count data
// $41e ---- -111 1111 1111 1:SCIEB allow sound cpu interrupt
// $420 ---- -111 1111 1111 1:SCIPD request sound cpu interrupt
// $422 ---- -111 1111 1111 1:SCIRE reset sound cpu interrupt
// $424 ---- ---- 1111 1111 1:SCILV0 sound cpu interrupt level bit0
// $426 ---- ---- 1111 1111 1:SCILV1 sound cpu interrupt level bit1
// $428 ---- ---- 1111 1111 1:SCILV2 sound cpu interrupt level bit2
// $42a ---- -111 1111 1111 1:MCIEB allow main cpu interrupt
// $42c ---- -111 1111 1111 1:MCIPD request main cpu interrupt
// $42e ---- -111 1111 1111 1:MCIRE reset main cpu interrupt
//
//--------------------------------------------------------------
//
// Individual Slot Register (ISR)
//
//     $+00      $+01
// $00 ---1 2334 4556 7777 1:KYONEX 2:KYONB 3:SBCTL 4:SSCTL 5:LPCTL 6:PCM8B 7:SA start address
// $02 1111 1111 1111 1111 1:SA start address
// $04 1111 1111 1111 1111 1:LSA loop start address
// $06 1111 1111 1111 1111 1:LEA loop end address
// $08 1111 1222 2234 4444 1:D2R decay 2 rate 2:D1R decay 1 rate 3:EGHOLD eg hold mode 4:AR attack rate
// $0a -122 2233 3334 4444 1:LPSLNK loop start link 2:KRS key rate scaling 3:DL decay level 4:RR release rate
// $0c ---- --12 3333 3333 1:STWINH stack write inhibit 2:SDIR sound direct 3:TL total level
// $0e 1111 2222 2233 3333 1:MDL modulation level 2:MDXSL modulation input x 3:MDYSL modulation input y
// $10 -111 1-22 2222 2222 1:OCT octave 2:FNS frequency number switch
// $12 1222 2233 4445 5666 1:LFORE 2:LFOF 3:PLFOWS 4:PLFOS 5:ALFOWS 6:ALFOS
// $14 ---- ---- -111 1222 1:ISEL input select 2:OMXL input mix level
// $16 1112 2222 3334 4444 1:DISDL 2:DIPAN 3:EFSDL 4:EFPAN
//
//--------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "c68k/c68k.h"
#include "cs2.h"
#include "debug.h"
#include "error.h"
#include "memory.h"
#include "m68kcore.h"
#include "scu.h"
#include "yabause.h"
#include "scsp.h"
#include "scspdsp.h"
#include "scsp_dsp_jit.h"
#if 0
#include "windows/aviout.h"
#endif

#ifdef PSP
# include "psp/common.h"

/* Macro to write a variable's value through the cache to main memory */
# define WRITE_THROUGH(var)  (*(u32 *)((u32)&(var) | 0x40000000) = (var))

/* Macro to flush SCSP state so it can be read from the ME */
# define FLUSH_SCSP()  sceKernelDcacheWritebackRange(&scsp, sizeof(scsp))

#else  // !PSP
# define WRITE_THROUGH(var)  /*nothing*/
# define FLUSH_SCSP()  /*nothing*/
#endif

int use_new_scsp = 0;
int new_scsp_outbuf_pos = 0;
s32 new_scsp_outbuf_l[900] = { 0 };
s32 new_scsp_outbuf_r[900] = { 0 };
int new_scsp_cycles = 0;

enum EnvelopeStates
{
   ATTACK,
   DECAY1,
   DECAY2,
   RELEASE
};

//0x30 to 0x3f only
const u8 attack_rate_table[][4] =
{
   { 4,4,4,4 },//0x30
   { 3,4,4,4 },
   { 3,4,3,4 },
   { 3,3,3,4 },

   { 3,3,3,3 },
   { 2,3,3,3 },
   { 2,3,2,3 },
   { 2,2,2,3 },

   { 2,2,2,2 },
   { 1,2,2,2 },
   { 1,2,1,2 },
   { 1,1,1,2 },

   { 1,1,1,1 },//0x3c
   { 1,1,1,1 },//0x3d
   { 1,1,1,1 },//0x3e
   { 1,1,1,1 },//0x3f
};

const u8 decay_rate_table[][4] =
{
   { 1,1,1,1 },//0x30
   { 2,1,1,1 },
   { 2,1,2,1 },
   { 2,2,2,1 },

   { 2,2,2,2 },
   { 4,2,2,2 },
   { 4,2,4,2 },
   { 4,4,4,2 },

   { 4,4,4,4 },
   { 8,4,4,4 },
   { 8,4,8,4 },
   { 8,8,8,4 },

   { 8,8,8,8 },//0x3c
   { 8,8,8,8 },//0x3d
   { 8,8,8,8 },//0x3e
   { 8,8,8,8 },//0x3f
};

#define EFFECTIVE_RATE_END 0xffff

//effective rate step table
//settings 0x2 to 0x5, then repeats with bigger shifts
//EFFECTIVE_RATE_END represents going back to the beginning of a row since
//the number of steps is not the same for each setting
#define MAKE_TABLE(SHIFT) \
   { 8192 >> SHIFT, 4096 >> SHIFT, 4096 >> SHIFT, EFFECTIVE_RATE_END,EFFECTIVE_RATE_END,EFFECTIVE_RATE_END,EFFECTIVE_RATE_END, EFFECTIVE_RATE_END }, \
   { 8192 >> SHIFT, 4096 >> SHIFT, 4096 >> SHIFT, 4096 >> SHIFT, 4096 >> SHIFT, 4096 >> SHIFT, 4096 >> SHIFT, EFFECTIVE_RATE_END }, \
   { 4096 >> SHIFT, EFFECTIVE_RATE_END,EFFECTIVE_RATE_END,EFFECTIVE_RATE_END,EFFECTIVE_RATE_END, EFFECTIVE_RATE_END,EFFECTIVE_RATE_END , EFFECTIVE_RATE_END }, \
   { 4096 >> SHIFT, 4096 >> SHIFT, 4096 >> SHIFT, 2048 >> SHIFT, 2048 >> SHIFT, EFFECTIVE_RATE_END, EFFECTIVE_RATE_END, EFFECTIVE_RATE_END },

const u16 envelope_table[][8] =
{
   MAKE_TABLE(0)
   MAKE_TABLE(1)
   MAKE_TABLE(2)
   MAKE_TABLE(3)
   MAKE_TABLE(4)
   MAKE_TABLE(5)
   MAKE_TABLE(6)
   MAKE_TABLE(7)
   MAKE_TABLE(8)
   MAKE_TABLE(9)
   MAKE_TABLE(10)
   MAKE_TABLE(11)
   MAKE_TABLE(12)
};

//unknown stored bits are unknown1-5
struct SlotRegs
{
   u8 kx;
   u8 kb;
   u8 sbctl;
   u8 ssctl;
   u8 lpctl;
   u8 pcm8b;
   u32 sa;
   u16 lsa;
   u16 lea;
   u8 d2r;
   u8 d1r;
   u8 hold;
   u8 ar;
   u8 unknown1;
   u8 ls;
   u8 krs;
   u8 dl;
   u8 rr;
   u8 unknown2;
   u8 si;
   u8 sd;
   u16 tl;
   u8 mdl;
   u8 mdxsl;
   u8 mdysl;
   u8 unknown3;
   u8 oct;
   u8 unknown4;
   u16 fns;
   u8 re;
   u8 lfof;
   u8 plfows;
   u8 plfos;
   u8 alfows;
   u8 alfos;
   u8 unknown5;
   u8 isel;
   u8 imxl;
   u8 disdl;
   u8 dipan;
   u8 efsdl;
   u8 efpan;
};

struct SlotState
{
   u16 wave;
   int backwards;
   enum EnvelopeStates envelope;
   s16 output;
   u16 attenuation;
   int step_count;
   u32 sample_counter;
   u32 envelope_steps_taken;
   s32 waveform_phase_value;
   s32 sample_offset;
   u32 address_pointer;
   u32 lfo_counter;
   u32 lfo_pos;

   int num;
   int is_muted;
};

struct Slot
{
   //registers
   struct SlotRegs regs;

   //internal state
   struct SlotState state;
};

struct Scsp
{
   u16 sound_stack[64];
   struct Slot slots[32];

   int debug_mode;
}new_scsp;

//samples per step through a 256 entry lfo table
const int lfo_step_table[0x20] = {
   0x3fc,//0
   0x37c,//1
   0x2fc,//2
   0x27c,//3
   0x1fc,//4
   0x1bc,//5
   0x17c,//6
   0x13c,//7
   0x0fc,//8
   0x0bc,//9
   0x0dc,//0xa
   0x08c,//0xb
   0x07c,//0xc
   0x06c,//0xd
   0x05c,//0xe
   0x04c,//0xf

   0x03c,//0x10
   0x034,//0x11
   0x02c,//0x12
   0x024,//0x13
   0x01c,//0x14
   0x018,//0x15
   0x014,//0x16
   0x010,//0x17
   0x00c,//0x18
   0x00a,//0x19
   0x008,//0x1a
   0x006,//0x1b
   0x004,//0x1c
   0x003,//0x1d
   0x002,//0x1e
   0x001,//0x1f
};

struct PlfoTables
{
   s8 saw_table[256];
   s8 square_table[256];
   s8 tri_table[256];
   s8 noise_table[256];
};

struct PlfoTables plfo;

struct AlfoTables
{
   u8 saw_table[256];
   u8 square_table[256];
   u8 tri_table[256];
   u8 noise_table[256];
};

struct AlfoTables alfo;

void fill_plfo_tables()
{
   int i;

   //saw
   for (i = 0; i < 256; i++)
   {
      if (i < 128)
         plfo.saw_table[i] = i;
      else
         plfo.saw_table[i] = -256 + i;
   }

   //square
   for (i = 0; i < 256; i++)
   {
      if (i < 128)
         plfo.square_table[i] = 127;
      else
         plfo.square_table[i] = -128;
   }

   //triangular
   for (i = 0; i < 256; i++)
   {
      if (i < 64)
         plfo.tri_table[i] = i * 2;
      else if (i < 192)
         plfo.tri_table[i] = 255 - (i * 2);
      else
         plfo.tri_table[i] = (i * 2) - 512;
   }

   //noise
   for (i = 0; i < 256; i++)
   {
      plfo.noise_table[i] = rand() & 0xff;
   }
}

void fill_alfo_tables()
{
   int i;

   //saw
   for (i = 0; i < 256; i++)
   {
      alfo.saw_table[i] = i;
   }

   //square
   for (i = 0; i < 256; i++)
   {
      if (i < 128)
         alfo.square_table[i] = 0;
      else
         alfo.square_table[i] = 0xff;
   }

   //triangular
   for (i = 0; i < 256; i++)
   {
      if (i < 128)
         alfo.tri_table[i] = i * 2;
      else
         alfo.tri_table[i] = 255 - (i * 2);
   }

   //noise
   for (i = 0; i < 256; i++)
   {
      alfo.noise_table[i] = rand() & 0xff;
   }
}

//pg, plfo
void op1(struct Slot * slot)
{
   u32 oct = slot->regs.oct ^ 8;
   u32 fns = slot->regs.fns ^ 0x400;
   u32 phase_increment = fns << oct;
   int plfo_val = 0;
   int plfo_shifted = 0;

   if (slot->state.attenuation > 0x3bf)
      return;

   if (slot->state.lfo_counter % lfo_step_table[slot->regs.lfof] == 0)
   {
      slot->state.lfo_counter = 0;
      slot->state.lfo_pos++;

      if (slot->state.lfo_pos > 0xff)
         slot->state.lfo_pos = 0;
   }

   if (slot->regs.plfows == 0)
      plfo_val = plfo.saw_table[slot->state.lfo_pos];
   else if (slot->regs.plfows == 1)
      plfo_val = plfo.square_table[slot->state.lfo_pos];
   else if (slot->regs.plfows == 2)
      plfo_val = plfo.tri_table[slot->state.lfo_pos];
   else if (slot->regs.plfows == 3)
      plfo_val = plfo.noise_table[slot->state.lfo_pos];

   plfo_shifted = (plfo_val << slot->regs.plfos) >> 2;

   slot->state.waveform_phase_value &= (1 << 18) - 1;//18 fractional bits
   slot->state.waveform_phase_value += (phase_increment + plfo_shifted);
}

int get_slot(struct Slot * slot, int mdsl)
{
   return (mdsl + slot->state.num) & 0x1f;
}

//address pointer calculation
//modulation data read
void op2(struct Slot * slot, struct Scsp * s)
{
   s32 md_out = 0;
   s32 sample_delta = slot->state.waveform_phase_value >> 18;

   if (slot->state.attenuation > 0x3bf)
      return;

   if (slot->regs.mdl)
   {
      //averaging operation
      u32 x_sel = get_slot(slot, slot->regs.mdxsl);
      u32 y_sel = get_slot(slot, slot->regs.mdysl);
      s16 xd = s->sound_stack[x_sel];
      s16 yd = s->sound_stack[y_sel];

      s32 zd = (xd + yd) / 2;

      //modulation operation
      u16 shift = 0xf - (slot->regs.mdl);
      zd >>= shift;

      md_out = zd;
   }

   //address pointer

   if (slot->regs.lpctl == 0)//no loop
   {
      slot->state.sample_offset += sample_delta;

      if (slot->state.sample_offset >= slot->regs.lea)
         slot->state.attenuation = 0x3ff;
   }
   else if (slot->regs.lpctl == 1)//normal loop
   {
      slot->state.sample_offset += sample_delta;

      if (slot->state.sample_offset >= slot->regs.lea)
         slot->state.sample_offset = slot->regs.lsa;
   }
   else if (slot->regs.lpctl == 2)//reverse
   {
      if (!slot->state.backwards)
         slot->state.sample_offset += sample_delta;
      else
         slot->state.sample_offset -= sample_delta;

      if (!slot->state.backwards)
      {
         if (slot->state.sample_offset >= slot->regs.lea)
         {
            slot->state.sample_offset = slot->regs.lea;
            slot->state.backwards = 1;
         }
      }
      else
      {
         //backwards
         if (slot->state.sample_offset <= slot->regs.lsa)
            slot->state.sample_offset = slot->regs.lea;
      }
   }
   else if (slot->regs.lpctl == 3)//ping pong
   {
      if(!slot->state.backwards)
         slot->state.sample_offset += sample_delta;
      else
         slot->state.sample_offset -= sample_delta;

      if (!slot->state.backwards)
      {
         if (slot->state.sample_offset >= slot->regs.lea)
         {
            slot->state.sample_offset = slot->regs.lea;
            slot->state.backwards = 1;
         }
      }
      else
      {
         if (slot->state.sample_offset <= slot->regs.lsa)
         {
            slot->state.sample_offset = slot->regs.lsa;
            slot->state.backwards = 0;
         }
      }
   }

   if (!slot->regs.pcm8b)
      slot->state.address_pointer = (s32)slot->regs.sa + (slot->state.sample_offset + md_out) * 2;
   else
      slot->state.address_pointer = (s32)slot->regs.sa + (slot->state.sample_offset + md_out);
}


void change_envelope_state(struct Slot * slot, enum EnvelopeStates new_state)
{
   slot->state.envelope = new_state;
   slot->state.step_count = 0;
}

int need_envelope_step(int effective_rate, u32 sample_counter, struct Slot* slot)
{
   if (sample_counter == 0)
      return 0;

   if (effective_rate == 0 || effective_rate == 1)
   {
      return 0;//never step
   }
   else if (effective_rate >= 0x30)
   {
      if ((sample_counter & 1) == 0)
      {
         slot->state.envelope_steps_taken++;
         return 1;
      }
      else
         return 0;
   }
   else
   {
      int pos = effective_rate - 2;

      int result = 0;

      int value = envelope_table[pos][slot->state.step_count];

      if (sample_counter % value == 0)
      {
         result = 1;

         slot->state.envelope_steps_taken++;
         slot->state.step_count++;

         if (envelope_table[pos][slot->state.step_count] == EFFECTIVE_RATE_END)
            slot->state.step_count = 0;//reached the end of the array
      }

      return result;
   }
   return 0;
}

s32 get_rate(struct Slot * slot, int rate)
{
   s32 result = 0;

   if (slot->regs.krs == 0xf)
      result = rate * 2;
   else
   {
      result = (slot->regs.krs * 2) + (rate * 2) + ((slot->regs.fns >> 9) & 1);
      result = (8 ^ slot->regs.oct) + (result - 8);
   }

   if (result <= 0)
      return 0;

   if (result >= 0x3c)
      return 0x3c;

   return result;
}

void do_decay(struct Slot * slot, int rate_in)
{
   int rate = get_rate(slot, rate_in);
   int sample_mod_4 = slot->state.envelope_steps_taken & 3;
   int decay_rate;

   if (rate <= 0x30)
      decay_rate = decay_rate_table[0][sample_mod_4];
   else
      decay_rate = decay_rate_table[rate - 0x30][sample_mod_4];

   if (need_envelope_step(rate, slot->state.sample_counter, slot))
   {
      if (slot->state.attenuation < 0x3bf)
         slot->state.attenuation += decay_rate;
   }
}

//interpolation
//eg
void op4(struct Slot * slot)
{
   int sample_mod_4 = slot->state.envelope_steps_taken & 3;

   if (slot->state.attenuation >= 0x3bf)
      return;

   if (slot->state.envelope == ATTACK)
   {
      int rate = get_rate(slot, slot->regs.ar);
      int need_step = need_envelope_step(rate, slot->state.sample_counter, slot);

      if (need_step)
      {
         int attack_rate = 0;

         if (rate <= 0x30)
            attack_rate = attack_rate_table[0][sample_mod_4];
         else
            attack_rate = attack_rate_table[rate - 0x30][sample_mod_4];

         slot->state.attenuation -= ((slot->state.attenuation >> attack_rate)) + 1;

         if (slot->state.attenuation == 0)
            change_envelope_state(slot, DECAY1);
      }
   }
   else if (slot->state.envelope == DECAY1)
   {
      do_decay(slot,slot->regs.d1r);

      if ((slot->state.attenuation >> 5) >= slot->regs.dl)
         change_envelope_state(slot, DECAY2);
   }
   else if (slot->state.envelope == DECAY2)
      do_decay(slot, slot->regs.d2r);
   else if (slot->state.envelope == RELEASE)
      do_decay(slot, slot->regs.rr);
}

s16 apply_volume(u16 tl, u16 slot_att, const s16 s)
{
   u32 tl_att = tl * 4;
   u32 att_clipped = 0;
   s32 sample_att = 0;
   u32 shift = 0;

   tl_att += slot_att;

   att_clipped = tl_att;
   att_clipped &= 0x3f;
   att_clipped ^= 0x7f;
   att_clipped += 1;

   sample_att = s * att_clipped;
   shift = tl_att >> 6;

   sample_att = sample_att >> (shift + 7);

   return sample_att;
}

//level 1
void op5(struct Slot * slot)
{
   if (slot->state.attenuation > 0x3bf)
   {
      slot->state.output = 0;
      return;
   }
   else
   {
      int alfo_val = 0;
      int lfo_add = 0;
      s16 sample = 0;

      if (slot->regs.alfows == 0)
         alfo_val = alfo.saw_table[slot->state.lfo_pos];
      else if (slot->regs.alfows == 1)
         alfo_val = alfo.square_table[slot->state.lfo_pos];
      else if (slot->regs.alfows == 2)
         alfo_val = alfo.tri_table[slot->state.lfo_pos];
      else if (slot->regs.alfows == 3)
         alfo_val = alfo.noise_table[slot->state.lfo_pos];

      lfo_add = (((alfo_val + 1)) >> (7 - slot->regs.alfos)) << 1;

      sample = apply_volume(slot->regs.tl, slot->state.attenuation + lfo_add, slot->state.output);
      slot->state.output = sample;
   }
}

//level 2
void op6(struct Slot * slot)
{

}

//sound stack write
void op7(struct Slot * slot, struct Scsp*s)
{
   u32 previous = s->sound_stack[slot->state.num + 32];
   s->sound_stack[slot->state.num + 32] = slot->state.output;
   s->sound_stack[slot->state.num] = previous;

   slot->state.sample_counter++;
   slot->state.lfo_counter++;
}

struct DebugInstrument
{
   u32 sa;
   int is_muted;
};

#define NUM_DEBUG_INSTRUMENTS 24

struct DebugInstrument debug_instruments[NUM_DEBUG_INSTRUMENTS] = { 0 };
int debug_instrument_pos = 0;

void scsp_debug_search_instruments(const u32 sa, int* found, int * offset)
{
   int i = 0;
   *found = 0;
   for (i = 0; i < NUM_DEBUG_INSTRUMENTS; i++)
   {
      if (debug_instruments[i].sa == sa)
      {
         *found = 1;
         break;
      }
   }

   *offset = i;
}

void scsp_debug_add_instrument(u32 sa)
{
   int i = 0, found = 0, offset = 0;

   if (debug_instrument_pos >= NUM_DEBUG_INSTRUMENTS)
      return;

   scsp_debug_search_instruments(sa, &found, &offset);

   //new instrument discovered
   if (!found)
      debug_instruments[debug_instrument_pos++].sa = sa;
}

void scsp_debug_instrument_set_mute(u32 sa, int mute)
{
   int found = 0, offset = 0;
   scsp_debug_search_instruments(sa, &found, &offset);

   if (offset >= NUM_DEBUG_INSTRUMENTS)
      return;

   if (found)
      debug_instruments[offset].is_muted = mute;
}

int scsp_debug_instrument_check_is_muted(u32 sa)
{
   int found = 0, offset = 0;
   scsp_debug_search_instruments(sa, &found, &offset);

   if (offset >= NUM_DEBUG_INSTRUMENTS)
      return 0;

   if (found && debug_instruments[offset].is_muted)
      return 1;

   return 0;
}

void scsp_debug_instrument_get_data(int i, u32 * sa, int * is_muted)
{
   if(i >= NUM_DEBUG_INSTRUMENTS)
      return;

   *sa = debug_instruments[i].sa;
   *is_muted = debug_instruments[i].is_muted;
}

void scsp_debug_set_mode(int mode)
{
   new_scsp.debug_mode = mode;
}

void scsp_debug_instrument_clear()
{
   debug_instrument_pos = 0;
   memset(debug_instruments, 0, sizeof(struct DebugInstrument) * NUM_DEBUG_INSTRUMENTS);
}

void scsp_debug_get_envelope(int chan, int * env, int * state)
{
   *env = new_scsp.slots[chan].state.attenuation;
   *state = new_scsp.slots[chan].state.envelope;
}

void keyon(struct Slot * slot)
{
   if (slot->state.envelope == RELEASE)
   {
      slot->state.envelope = ATTACK;
      slot->state.attenuation = 0x280;
      slot->state.sample_counter = 0;
      slot->state.step_count = 0;
      slot->state.sample_offset = 0;
      slot->state.envelope_steps_taken = 0;

      if (new_scsp.debug_mode)
         scsp_debug_add_instrument(slot->regs.sa);
   }
   //otherwise ignore
}

void keyoff(struct Slot * slot)
{
   change_envelope_state(slot, RELEASE);
}

void keyonex(struct Scsp *s)
{
   int channel;
   for (channel = 0; channel < 32; channel++)
   {
      if (s->slots[channel].regs.kb)
         keyon(&s->slots[channel]);
      else
         keyoff(&s->slots[channel]);
   }
}

void scsp_slot_write_byte(struct Scsp *s, u32 addr, u8 data)
{
   int slot_num = (addr >> 5) & 0x1f;
   struct Slot * slot = &s->slots[slot_num];
   u32 offset = (addr - (0x20 * slot_num));

   switch (offset)
   {
   case 0:
      slot->regs.kb = (data >> 3) & 1;//has to be done first

      if ((data >> 4) & 1)//keyonex
         keyonex(s);

      slot->regs.sbctl = (data >> 1) & 3;
      slot->regs.ssctl = (slot->regs.ssctl & 1) | ((data & 1) << 1);
      break;
   case 1:
      slot->regs.ssctl = (slot->regs.ssctl & 2) | ((data >> 7) & 1);
      slot->regs.lpctl = (data >> 5) & 3;
      slot->regs.pcm8b = (data >> 4) & 1;
      slot->regs.sa = (slot->regs.sa & 0xffff) | ((data & 0xf) << 16);
      break;
   case 2:
      slot->regs.sa = (slot->regs.sa & 0xf00ff) | (data << 8);
      break;
   case 3:
      slot->regs.sa = (slot->regs.sa & 0xfff00) | data;
      break;
   case 4:
      slot->regs.lsa = (slot->regs.lsa & 0x00ff) | (data << 8);
      break;
   case 5:
      slot->regs.lsa = (slot->regs.lsa & 0xff00) | data;
      break;
   case 6:
      slot->regs.lea = (slot->regs.lea & 0x00ff) | (data << 8);
      break;
   case 7:
      slot->regs.lea = (slot->regs.lea & 0xff00) | data;
      break;
   case 8:
      slot->regs.d2r = (data >> 3) & 0x1f;
      slot->regs.d1r = (slot->regs.d1r & 3) | ((data & 0x7) << 2);
      break;
   case 9:
      slot->regs.d1r = (slot->regs.d1r & 0x1c) | ((data >> 6) & 3);
      slot->regs.hold = (data >> 5) & 1;
      slot->regs.ar = data & 0x1f;
      break;
   case 10:
      slot->regs.unknown1 = (data >> 7) & 1;
      slot->regs.ls = (data >> 6) & 1;
      slot->regs.krs = (data >> 2) & 0xf;
      slot->regs.dl = (slot->regs.dl & 0x7) | ((data & 3) << 3);
      break;
   case 11:
      slot->regs.dl = (slot->regs.dl & 0x18) | ((data >> 5) & 7);
      slot->regs.rr = data & 0x1f;
      break;
   case 12:
      slot->regs.unknown2 = (data >> 2) & 3;
      slot->regs.si = (data >> 1) & 1;
      slot->regs.sd = data & 1;
      break;
   case 13:
      slot->regs.tl = data;
      break;
   case 14:
      slot->regs.mdl = (data >> 4) & 0xf;
      slot->regs.mdxsl = (slot->regs.mdxsl & 0x3) | ((data & 0xf) << 2);
      break;
   case 15:
      slot->regs.mdxsl = (slot->regs.mdxsl & 0x3C) | ((data >> 6) & 3);
      slot->regs.mdysl = data & 0x3f;
      break;
   case 16:
      slot->regs.unknown3 = (data >> 7) & 1;
      slot->regs.oct = (data >> 3) & 0xf;
      slot->regs.unknown4 = (data >> 2) & 1;
      slot->regs.fns = (slot->regs.fns & 0xff) | ((data & 3) << 8);
      break;
   case 17:
      slot->regs.fns = (slot->regs.fns & 0x300) | data;
      break;
   case 18:
      slot->regs.re = (data >> 7) & 1;
      slot->regs.lfof = (data >> 2) & 0x1f;
      slot->regs.plfows = data & 3;
      break;
   case 19:
      slot->regs.plfos = (data >> 5) & 7;
      slot->regs.alfows = (data >> 3) & 3;
      slot->regs.alfos = data & 7;
      break;
   case 20:
      //nothing here
      break;
   case 21:
      slot->regs.unknown5 = (data >> 7) & 1;
      slot->regs.isel = (data >> 3) & 0xf;
      slot->regs.imxl = data & 7;
      break;
   case 22:
      slot->regs.disdl = (data >> 5) & 7;
      slot->regs.dipan = data & 0x1f;
      break;
   case 23:
      slot->regs.efsdl = (data >> 5) & 7;
      slot->regs.efpan = data & 0x1f;
      break;
   default:
      break;
   }
}

u8 scsp_slot_read_byte(struct Scsp *s, u32 addr)
{
   int slot_num = (addr >> 5) & 0x1f;
   struct Slot * slot = &s->slots[slot_num];
   u32 offset = (addr - (0x20 * slot_num));
   u8 data = 0;

   switch (offset)
   {
   case 0:
      data |= slot->regs.kb << 3;
      data |= slot->regs.sbctl << 1;
      data |= (slot->regs.ssctl >> 1) & 1;
      break;
   case 1:
      data |= (slot->regs.ssctl & 1) << 7;
      data |= slot->regs.lpctl << 5;
      data |= slot->regs.pcm8b << 4;
      data |= (slot->regs.sa & 0xf0000) >> 16;
      break;
   case 2:
      data |= (slot->regs.sa & 0xff00) >> 8;
      break;
   case 3:
      data |= slot->regs.sa & 0xff;
      break;
   case 4:
      data |= (slot->regs.lsa & 0xff00) >> 8;
      break;
   case 5:
      data |= slot->regs.lsa & 0xff;
      break;
   case 6:
      data |= (slot->regs.lea & 0xff00) >> 8;
      break;
   case 7:
      data |= slot->regs.lea & 0xff;
      break;
   case 8:
      data |= slot->regs.d2r << 3;
      data |= slot->regs.d1r >> 2;
      break;
   case 9:
      data |= slot->regs.d1r << 6;
      data |= slot->regs.hold << 5;
      data |= slot->regs.ar;
      break;
   case 10:
      data |= slot->regs.unknown1 << 7;
      data |= slot->regs.ls << 6;
      data |= slot->regs.krs << 2;
      data |= slot->regs.dl >> 3;
      break;
   case 11:
      data |= slot->regs.dl << 5;
      data |= slot->regs.rr;
      break;
   case 12:
      data |= slot->regs.unknown2 << 2;
      data |= slot->regs.si << 1;
      data |= slot->regs.sd;
      break;
   case 13:
      data |= slot->regs.tl;
      break;
   case 14:
      data |= slot->regs.mdl << 4;
      data |= slot->regs.mdxsl >> 2;
      break;
   case 15:
      data |= slot->regs.mdxsl << 6;
      data |= slot->regs.mdysl;
      break;
   case 16:
      data |= slot->regs.unknown3 << 7;
      data |= slot->regs.oct << 3;
      data |= slot->regs.unknown4 << 2;
      data |= slot->regs.fns >> 8;
      break;
   case 17:
      data |= slot->regs.fns;
      break;
   case 18:
      data |= slot->regs.re << 7;
      data |= slot->regs.lfof << 2;
      data |= slot->regs.plfows;
      break;
   case 19:
      data |= slot->regs.plfos << 5;
      data |= slot->regs.alfows << 3;
      data |= slot->regs.alfos;
      break;
   case 20:
      //nothing
      break;
   case 21:
      data |= slot->regs.unknown5 << 7;
      data |= slot->regs.isel << 3;
      data |= slot->regs.imxl;
      break;
   case 22:
      data |= slot->regs.disdl << 5;
      data |= slot->regs.dipan;
      break;
   case 23:
      data |= slot->regs.efsdl << 5;
      data |= slot->regs.efpan;
      break;
   }

   return data;
}

void scsp_slot_write_word(struct Scsp *s, u32 addr, u16 data)
{
   int slot_num = (addr >> 5) & 0x1f;
   struct Slot * slot = &s->slots[slot_num];
   u32 offset = (addr - (0x20 * slot_num));

   switch (offset >> 1)
   {
   case 0:
      slot->regs.kb = (data >> 11) & 1;//has to be done before keyonex

      if (data & (1 << 12))
         keyonex(s);

      slot->regs.sbctl = (data >> 9) & 3;
      slot->regs.ssctl = (data >> 7) & 3;
      slot->regs.lpctl = (data >> 5) & 3;
      slot->regs.pcm8b = (data >> 4) & 1;
      slot->regs.sa = (slot->regs.sa & 0xffff) | ((data & 0xf) << 16);
      break;
   case 1:
      slot->regs.sa = (slot->regs.sa & 0xf0000) | data;
      break;
   case 2:
      slot->regs.lsa = data;
      break;
   case 3:
      slot->regs.lea = data;
      break;
   case 4:
      slot->regs.d2r = data >> 11;
      slot->regs.d1r = (data >> 6) & 0x1f;
      slot->regs.hold = (data >> 5) & 1;
      slot->regs.ar = data & 0x1f;
      break;
   case 5:
      slot->regs.unknown1 = (data >> 15) & 1;
      slot->regs.ls = (data >> 14) & 1;
      slot->regs.krs = (data >> 10) & 0xf;
      slot->regs.dl = (data >> 5) & 0x1f;
      slot->regs.rr = data & 0x1f;
      break;
   case 6:
      slot->regs.unknown2 = (data >> 10) & 3;
      slot->regs.si = (data >> 9) & 1;
      slot->regs.sd = (data >> 8) & 1;
      slot->regs.tl = data & 0xff;
      break;
   case 7:
      slot->regs.mdl = (data >> 12) & 0xf;
      slot->regs.mdxsl = (data >> 6) & 0x3f;
      slot->regs.mdysl = data & 0x3f;
      break;
   case 8:
      slot->regs.unknown3 = (data >> 15) & 1;
      slot->regs.unknown4 = (data >> 10) & 1;
      slot->regs.oct = (data >> 11) & 0xf;
      slot->regs.fns = data & 0x3ff;
      break;
   case 9:
      slot->regs.re = (data >> 15) & 1;
      slot->regs.lfof = (data >> 10) & 0x1f;
      slot->regs.plfows = (data >> 8) & 3;
      slot->regs.plfos = (data >> 5) & 7;
      slot->regs.alfows = (data >> 3) & 3;
      slot->regs.alfos = data & 7;
      break;
   case 10:
      slot->regs.unknown5 = (data >> 7) & 1;
      slot->regs.isel = (data >> 3) & 0xf;
      slot->regs.imxl = data & 7;
      break;
   case 11:
      slot->regs.disdl = (data >> 13) & 7;
      slot->regs.dipan = (data >> 8) & 0x1f;
      slot->regs.efsdl = (data >> 5) & 7;
      slot->regs.efpan = data & 0x1f;
      break;
   default:
      break;
   }
}

u16 scsp_slot_read_word(struct Scsp *s, u32 addr)
{
   int slot_num = (addr >> 5) & 0x1f;
   struct Slot * slot = &s->slots[slot_num];
   u32 offset = (addr - (0x20 * slot_num));
   u16 data = 0;

   switch (offset >> 1)
   {
   case 0:
      //keyonex not stored
      data |= slot->regs.kb << 11;
      data |= slot->regs.sbctl << 9;
      data |= slot->regs.ssctl << 7;
      data |= slot->regs.lpctl << 5;
      data |= slot->regs.pcm8b << 4;
      data |= (slot->regs.sa >> 16) & 0xf;
      break;
   case 1:
      data = slot->regs.sa & 0xffff;
      break;
   case 2:
      data = slot->regs.lsa;
      break;
   case 3:
      data = slot->regs.lea;
      break;
   case 4:
      data |= slot->regs.d2r << 11;
      data |= slot->regs.d1r << 6;
      data |= slot->regs.hold << 5;
      data |= slot->regs.ar;
      break;
   case 5:
      data |= slot->regs.unknown1 << 15;
      data |= slot->regs.ls << 14;
      data |= slot->regs.krs << 10;
      data |= slot->regs.dl << 5;
      data |= slot->regs.rr;
      break;
   case 6:
      data |= slot->regs.unknown2 << 10;
      data |= slot->regs.si << 9;
      data |= slot->regs.sd << 8;
      data |= slot->regs.tl;
      break;
   case 7:
      data |= slot->regs.mdl << 12;
      data |= slot->regs.mdxsl << 6;
      data |= slot->regs.mdysl;
      break;
   case 8:
      data |= slot->regs.unknown3 << 15;
      data |= slot->regs.oct << 11;
      data |= slot->regs.unknown4 << 10;
      data |= slot->regs.fns;
      break;
   case 9:
      data |= slot->regs.re << 15;
      data |= slot->regs.lfof << 10;
      data |= slot->regs.plfows << 8;
      data |= slot->regs.plfos << 5;
      data |= slot->regs.alfows << 3;
      data |= slot->regs.alfos;
      break;
   case 10:
      data |= slot->regs.unknown5 << 7;
      data |= slot->regs.isel << 3;
      data |= slot->regs.imxl;
      break;
   case 11:
      data |= slot->regs.disdl << 13;
      data |= slot->regs.dipan << 8;
      data |= slot->regs.efsdl << 5;
      data |= slot->regs.efpan;
      break;
   }
   return data;
}

void get_panning(int pan, int * pan_val_l, int * pan_val_r)
{
   if (pan & 0x10)
   {
      //negative values
      *pan_val_l = 0;
      *pan_val_r = pan & 0xf;
   }
   else
   {
      *pan_val_l = pan & 0xf;
      *pan_val_r = 0;
   }
}

int get_sdl_shift(int sdl)
{
   if (sdl == 0)
      return 16;//-infinity
   else return (7 - sdl);
}

void new_scsp_reset(struct Scsp* s)
{
   int slot_num;
   memset(s, 0, sizeof(struct Scsp));

   for (slot_num = 0; slot_num < 32; slot_num++)
   {
      s->slots[slot_num].state.attenuation = 0x3FF;
      s->slots[slot_num].state.envelope = RELEASE;
      s->slots[slot_num].state.num = slot_num;
   }

   fill_plfo_tables();
   fill_alfo_tables();

#ifdef HAVE_PLAY_JIT
   if (yabsys.use_scsp_dsp_jit)
      scsp_dsp_jit_init();
   else
      scsp_dsp_int_init();
#else
   scsp_dsp_int_init();
#endif

   new_scsp_outbuf_pos = 0;
   new_scsp_cycles = 0;
}

void scsp_set_use_new(int which)
{
   if (which && !use_new_scsp)
      new_scsp_reset(&new_scsp);

   use_new_scsp = which;
}

////////////////////////////////////////////////////////////////

#ifndef PI
#define PI 3.14159265358979323846
#endif

#define SCSP_FREQ         44100                                     // SCSP frequency

#define SCSP_RAM_SIZE     0x080000                                  // SCSP RAM size
#define SCSP_RAM_MASK     (SCSP_RAM_SIZE - 1)

#define SCSP_MIDI_IN_EMP  0x01                                      // MIDI flags
#define SCSP_MIDI_IN_FUL  0x02
#define SCSP_MIDI_IN_OVF  0x04
#define SCSP_MIDI_OUT_EMP 0x08
#define SCSP_MIDI_OUT_FUL 0x10

#define SCSP_ENV_RELEASE  3                                         // Envelope phase
#define SCSP_ENV_SUSTAIN  2
#define SCSP_ENV_DECAY    1
#define SCSP_ENV_ATTACK   0

#define SCSP_FREQ_HB      19                                        // Freq counter int part
#define SCSP_FREQ_LB      10                                        // Freq counter float part

#define SCSP_ENV_HB       10                                        // Env counter int part
#define SCSP_ENV_LB       10                                        // Env counter float part

#define SCSP_LFO_HB       10                                        // LFO counter int part
#define SCSP_LFO_LB       10                                        // LFO counter float part

#define SCSP_ENV_LEN      (1 << SCSP_ENV_HB)                        // Env table len
#define SCSP_ENV_MASK     (SCSP_ENV_LEN - 1)                        // Env table mask

#define SCSP_FREQ_LEN     (1 << SCSP_FREQ_HB)                       // Freq table len
#define SCSP_FREQ_MASK    (SCSP_FREQ_LEN - 1)                       // Freq table mask

#define SCSP_LFO_LEN      (1 << SCSP_LFO_HB)                        // LFO table len
#define SCSP_LFO_MASK     (SCSP_LFO_LEN - 1)                        // LFO table mask

#define SCSP_ENV_AS       0                                         // Env Attack Start
#define SCSP_ENV_DS       (SCSP_ENV_LEN << SCSP_ENV_LB)             // Env Decay Start
#define SCSP_ENV_AE       (SCSP_ENV_DS - 1)                         // Env Attack End
#define SCSP_ENV_DE       (((2 * SCSP_ENV_LEN) << SCSP_ENV_LB) - 1) // Env Decay End

#define SCSP_ATTACK_R     (u32) (8 * 44100)
#define SCSP_DECAY_R      (u32) (12 * SCSP_ATTACK_R)

////////////////////////////////////////////////////////////////

typedef struct slot_t
{
  u8 swe;      // stack write enable
  u8 sdir;     // sound direct
  u8 pcm8b;    // PCM sound format

  u8 sbctl;    // source bit control
  u8 ssctl;    // sound source control
  u8 lpctl;    // loop control

  u8 key;      // KEY_ state
  u8 keyx;     // still playing regardless the KEY_ state (hold, decay)

  s8 *buf8;    // sample buffer 8 bits
  s16 *buf16;  // sample buffer 16 bits

  u32 fcnt;    // phase counter
  u32 finc;    // phase step adder
  u32 finct;   // non adjusted phase step

  s32 ecnt;    // envelope counter
  s32 *einc;    // envelope current step adder
  s32 einca;   // envelope step adder for attack
  s32 eincd;   // envelope step adder for decay 1
  s32 eincs;   // envelope step adder for decay 2
  s32 eincr;   // envelope step adder for release
  s32 ecmp;    // envelope compare to raise next phase
  u32 ecurp;   // envelope current phase (attack / decay / release ...)
  s32 env;     // envelope multiplier (at time of last update)

  void (*enxt)(struct slot_t *);  // envelope function pointer for next phase event

  u32 lfocnt;   // lfo counter
  s32 lfoinc;   // lfo step adder

  u32 sa;       // start address
  u32 lsa;      // loop start address
  u32 lea;      // loop end address

  s32 tl;       // total level
  s32 sl;       // sustain level

  s32 ar;       // attack rate
  s32 dr;       // decay rate
  s32 sr;       // sustain rate
  s32 rr;       // release rate

  s32 *arp;     // attack rate table pointer
  s32 *drp;     // decay rate table pointer
  s32 *srp;     // sustain rate table pointer
  s32 *rrp;     // release rate table pointer

  u32 krs;      // key rate scale

  s32 *lfofmw;  // lfo frequency modulation waveform pointer
  s32 *lfoemw;  // lfo envelope modulation waveform pointer
  u8 lfofms;    // lfo frequency modulation sensitivity
  u8 lfoems;    // lfo envelope modulation sensitivity
  u8 fsft;      // frequency shift (used for freq lfo)

  u8 mdl;       // modulation level
  u8 mdx;       // modulation source X
  u8 mdy;       // modulation source Y

  u8 imxl;      // input sound level
  u8 disll;     // direct sound level left
  u8 dislr;     // direct sound level right
  u8 efsll;     // effect sound level left
  u8 efslr;     // effect sound level right

  u8 eghold;    // eg type envelope hold
  u8 lslnk;     // loop start link (start D1R when start loop adr is reached)

  // NOTE: Previously there were u8 pads here to maintain 4-byte alignment.
  //       There are current 22 u8's in this struct and 1 u16. This makes 24
  //       bytes, so there are no pads at the moment.
  //
  //       I'm not sure this is at all necessary either, but keeping this note
  //       in case.
} slot_t;

typedef struct scsp_t
{
  u32 mem4b;            // 4mbit memory
  u32 mvol;             // master volume

  u32 rbl;              // ring buffer lenght
  u32 rbp;              // ring buffer address (pointer)

  u32 mslc;             // monitor slot
  u32 ca;               // call address
  u32 sgc;              // phase
  u32 eg;               // envelope

  u32 dmea;             // dma memory address start
  u32 drga;             // dma register address start
  u32 dmfl;             // dma flags (direction / gate 0 ...)
  u32 dmlen;            // dma transfer len

  u8 midinbuf[4];       // midi in buffer
  u8 midoutbuf[4];      // midi out buffer
  u8 midincnt;          // midi in buffer size
  u8 midoutcnt;         // midi out buffer size
  u8 midflag;           // midi flag (empty, full, overflow ...)
  u8 midflag2;          // midi flag 2 (here only for alignement)

  s32 timacnt;          // timer A counter
  u32 timasd;           // timer A step diviser
  s32 timbcnt;          // timer B counter
  u32 timbsd;           // timer B step diviser
  s32 timccnt;          // timer C counter
  u32 timcsd;           // timer C step diviser

  u32 scieb;            // allow sound cpu interrupt
  u32 scipd;            // pending sound cpu interrupt

  u32 scilv0;           // IL0 M68000 interrupt pin state
  u32 scilv1;           // IL1 M68000 interrupt pin state
  u32 scilv2;           // IL2 M68000 interrupt pin state

  u32 mcieb;            // allow main cpu interrupt
  u32 mcipd;            // pending main cpu interrupt

  u8 *scsp_ram;         // scsp ram pointer
  void (*mintf)(void);  // main cpu interupt function pointer
  void (*sintf)(u32);   // sound cpu interrupt function pointer

  s32 stack[32 * 2];    // two last generation slot output (SCSP STACK)
  slot_t slot[32];      // 32 slots
} scsp_t;

////////////////////////////////////////////////////////////////

static s32 scsp_env_table[SCSP_ENV_LEN * 2];  // envelope curve table (attack & decay)

static s32 scsp_lfo_sawt_e[SCSP_LFO_LEN];     // lfo sawtooth waveform for envelope
static s32 scsp_lfo_squa_e[SCSP_LFO_LEN];     // lfo square waveform for envelope
static s32 scsp_lfo_tri_e[SCSP_LFO_LEN];      // lfo triangle waveform for envelope
static s32 scsp_lfo_noi_e[SCSP_LFO_LEN];      // lfo noise waveform for envelope

static s32 scsp_lfo_sawt_f[SCSP_LFO_LEN];     // lfo sawtooth waveform for frequency
static s32 scsp_lfo_squa_f[SCSP_LFO_LEN];     // lfo square waveform for frequency
static s32 scsp_lfo_tri_f[SCSP_LFO_LEN];      // lfo triangle waveform for frequency
static s32 scsp_lfo_noi_f[SCSP_LFO_LEN];      // lfo noise waveform frequency

static s32 scsp_attack_rate[0x40 + 0x20];     // envelope step for attack
static s32 scsp_decay_rate[0x40 + 0x20];      // envelope step for decay
static s32 scsp_null_rate[0x20];              // null envelope step

static s32 scsp_lfo_step[32];                 // directly give the lfo counter step

static s32 scsp_tl_table[256];                // table of values for total level attentuation

static u8 scsp_reg[0x1000];

static u8 *scsp_isr;
static u8 *scsp_ccr;
static u8 *scsp_dcr;

static s32 *scsp_bufL;
static s32 *scsp_bufR;
static u32 scsp_buf_len;
static u32 scsp_buf_pos;

static scsp_t   scsp;                         // SCSP structure

#define CDDA_NUM_BUFFERS	2*75

static union {
   u8 data[CDDA_NUM_BUFFERS*2352];
} cddabuf;
static unsigned int cdda_next_in=0;               // Next sector buffer offset to receive into
static u32 cdda_out_left;                       // Bytes of CDDA left to output

////////////////////////////////////////////////////////////////

static void scsp_env_null_next(slot_t *slot);
static void scsp_release_next(slot_t *slot);
static void scsp_sustain_next(slot_t *slot);
static void scsp_decay_next(slot_t *slot);
static void scsp_attack_next(slot_t *slot);
static void scsp_slot_update_keyon(slot_t *slot);

//////////////////////////////////////////////////////////////////////////////

static int scsp_mute_flags = 0;
static int scsp_volume = 100;
////////////////////////////////////////////////////////////////
// Slot Access

static void
scsp_slot_refresh_einc (slot_t *slot, u32 adsr_bitmask)
{
    if (slot->arp && (adsr_bitmask & 0x1))
        slot->einca = slot->arp[(14 - slot->fsft) >> slot->krs];
    if (slot->drp && (adsr_bitmask & 0x2))
        slot->eincd = slot->drp[(14 - slot->fsft) >> slot->krs];
    if (slot->srp && (adsr_bitmask & 0x4))
        slot->eincs = slot->srp[(14 - slot->fsft) >> slot->krs];
    if (slot->rrp && (adsr_bitmask & 0x8))
        slot->eincr = slot->rrp[(14 - slot->fsft) >> slot->krs];
}
////////////////////////////////////////////////////////////////
// Key ON/OFF event handler

static void
scsp_slot_keyon (slot_t *slot)
{
    // key need to be released before being pressed ;)
    if (slot->ecurp == SCSP_ENV_RELEASE)
    {
        SCSPLOG ("key on slot %d. 68K PC = %08X slot->sa = %08X slot->lsa = %08X "
                 "slot->lea = %08X\n", slot - &(scsp.slot[0]), M68K->GetPC(),
                 slot->sa, slot->lsa, slot->lea >> SCSP_FREQ_LB);

        // set buffer, loop start/end address of the slot
        if (slot->pcm8b)
        {
            slot->buf8 = (s8*) &(scsp.scsp_ram[slot->sa]);
            if ((slot->sa + (slot->lea >> SCSP_FREQ_LB)) > SCSP_RAM_MASK)
                slot->lea = (SCSP_RAM_MASK - slot->sa) << SCSP_FREQ_LB;
        }
        else
        {
            slot->buf16 = (s16*) &(scsp.scsp_ram[slot->sa & ~1]);
            if ((slot->sa + (slot->lea >> (SCSP_FREQ_LB - 1))) > SCSP_RAM_MASK)
                slot->lea = (SCSP_RAM_MASK - slot->sa) << (SCSP_FREQ_LB - 1);
        }

        slot->fcnt = 0;                 // reset frequency counter
        slot->ecnt = SCSP_ENV_AS;       // reset envelope counter (probably wrong,
        // should convert decay to attack?)
        slot->env = 0;                  // reset envelope

        slot->einc = &slot->einca;      // envelope counter step is attack step
        slot->ecurp = SCSP_ENV_ATTACK;  // current envelope phase is attack
        slot->ecmp = SCSP_ENV_AE;       // limit reach to next event (Attack End)
        slot->enxt = scsp_attack_next;  // function pointer to next event
    }
}

static void
scsp_slot_keyoff (slot_t *slot)
{
    // key need to be pressed before being released ;)
    if (slot->ecurp != SCSP_ENV_RELEASE)
    {
        SCSPLOG ("key off slot %d\n", slot - &(scsp.slot[0]));

        // if we still are in attack phase at release time, convert attack to decay
        if (slot->ecurp == SCSP_ENV_ATTACK)
            slot->ecnt = SCSP_ENV_DE - slot->ecnt;
        slot->einc = &slot->eincr;
        slot->ecmp = SCSP_ENV_DE;
        slot->ecurp = SCSP_ENV_RELEASE;
        slot->enxt = scsp_release_next;
    }
}

static void
scsp_slot_keyonoff (void)
{
    slot_t *slot;

    for(slot = &(scsp.slot[0]); slot < &(scsp.slot[32]); slot++)
    {
        if (slot->key)
            scsp_slot_keyon (slot);
        else
            scsp_slot_keyoff (slot);
    }
}

static void
scsp_slot_set_b (u32 s, u32 a, u8 d)
{

    slot_t *slot = &(scsp.slot[s]);

    SCSPLOG("slot %d : reg %.2X = %.2X\n", s, a & 0x1F, d);

    scsp_isr[a ^ 3] = d;

    switch (a & 0x1F)
    {
        case 0x00: // KX/KB/SBCTL/SSCTL(high bit)
            slot->key = (d >> 3) & 1;
            slot->sbctl = (d >> 1) & 3;
            slot->ssctl = (slot->ssctl & 1) + ((d & 1) << 1);

            if (d & 0x10) scsp_slot_keyonoff ();
            return;

        case 0x01: // SSCTL(low bit)/LPCTL/8B/SA(highest 4 bits)
            slot->ssctl = (slot->ssctl & 2) + ((d >> 7) & 1);
            slot->lpctl = (d >> 5) & 3;

            slot->pcm8b = d & 0x10;
            slot->sa = (slot->sa & 0x0FFFF) + ((d & 0xF) << 16);
            slot->sa &= SCSP_RAM_MASK;

            if (slot->ecnt < SCSP_ENV_DE) scsp_slot_update_keyon (slot);

            return;

        case 0x02: // SA(next highest byte)
            slot->sa = (slot->sa & 0xF00FF) + (d << 8);
            slot->sa &= SCSP_RAM_MASK;

            if (slot->ecnt < SCSP_ENV_DE) scsp_slot_update_keyon (slot);
            return;

        case 0x03: // SA(low byte)
            slot->sa = (slot->sa & 0xFFF00) + d;
            slot->sa &= SCSP_RAM_MASK;

            if (slot->ecnt < SCSP_ENV_DE) scsp_slot_update_keyon (slot);
            return;

        case 0x04: // LSA(high byte)
            slot->lsa = (slot->lsa & (0x00FF << SCSP_FREQ_LB)) +
            (d << (8 + SCSP_FREQ_LB));
            return;

        case 0x05: // LSA(low byte)
            slot->lsa = (slot->lsa & (0xFF00 << SCSP_FREQ_LB)) +
            (d << SCSP_FREQ_LB);
            return;

        case 0x06: // LEA(high byte)
            slot->lea = (slot->lea & (0x00FF << SCSP_FREQ_LB)) +
            (d << (8 + SCSP_FREQ_LB));
            return;

        case 0x07: // LEA(low byte)
            slot->lea = (slot->lea & (0xFF00 << SCSP_FREQ_LB)) +
            (d << SCSP_FREQ_LB);
            return;

        case 0x08: // D2R/D1R(highest 3 bits)
            slot->sr = (d >> 3) & 0x1F;
            slot->dr = (slot->dr & 0x03) + ((d & 7) << 2);

            if (slot->sr)
                slot->srp = &scsp_decay_rate[slot->sr << 1];
            else
                slot->srp = &scsp_null_rate[0];

            if (slot->dr)
                slot->drp = &scsp_decay_rate[slot->dr << 1];
            else
                slot->drp = &scsp_null_rate[0];

            scsp_slot_refresh_einc (slot, 0x2 | 0x4);
            return;

        case 0x09: // D1R(lowest 2 bits)/EGHOLD/AR
            slot->dr = (slot->dr & 0x1C) + ((d >> 6) & 3);
            slot->eghold = d & 0x20;
            slot->ar = d & 0x1F;

            if (slot->dr)
                slot->drp = &scsp_decay_rate[slot->dr << 1];
            else
                slot->drp = &scsp_null_rate[0];

            if (slot->ar)
                slot->arp = &scsp_attack_rate[slot->ar << 1];
            else
                slot->arp = &scsp_null_rate[0];

            scsp_slot_refresh_einc (slot, 0x1 | 0x2);
            return;

        case 0x0A: // LPSLNK/KRS/DL(highest 2 bits)
            slot->lslnk = d & 0x40;
            slot->krs = (d >> 2) & 0xF;

            if (slot->krs == 0xF)
                slot->krs = 4;
            else
                slot->krs >>= 2;

            slot->sl &= 0xE0 << SCSP_ENV_LB;
            slot->sl += (d & 3) << (8 + SCSP_ENV_LB);
            slot->sl += SCSP_ENV_DS; // adjusted for envelope compare (ecmp)

            scsp_slot_refresh_einc (slot, 0xF);
            return;

        case 0x0B: // DL(lowest 3 bits)/RR
            slot->sl &= 0x300 << SCSP_ENV_LB;
            slot->sl += (d & 0xE0) << SCSP_ENV_LB;
            slot->sl += SCSP_ENV_DS; // adjusted for envelope compare (ecmp)
            slot->rr = d & 0x1F;

            if (slot->rr)
                slot->rrp = &scsp_decay_rate[slot->rr << 1];
            else
                slot->rrp = &scsp_null_rate[0];

            scsp_slot_refresh_einc (slot, 0x8);
            return;

        case 0x0C: // STWINH/SDIR
            slot->sdir = d & 2;
            slot->swe = d & 1;
            return;

        case 0x0D: // TL
            slot->tl = scsp_tl_table[(d & 0xFF)];
            return;

        case 0x0E: // MDL/MDXSL(highest 4 bits)
            slot->mdl = (d >> 4) & 0xF; // need to adjust for correct shift
            slot->mdx = (slot->mdx & 3) + ((d & 0xF) << 2);
            return;

        case 0x0F: // MDXSL(lowest 2 bits)/MDYSL
            slot->mdx = (slot->mdx & 0x3C) + ((d >> 6) & 3);
            slot->mdy = d & 0x3F;
            return;

        case 0x10: // OCT/FNS(highest 2 bits)
            if (d & 0x40)
                slot->fsft = 23 - ((d >> 3) & 0xF);
            else
                slot->fsft = ((d >> 3) & 7) ^ 7;

            slot->finct = (slot->finct & 0x7F80) + ((d & 3) << (8 + 7));
            slot->finc = (0x20000 + slot->finct) >> slot->fsft;

            scsp_slot_refresh_einc (slot, 0xF);
            return;

        case 0x11: // FNS(low byte)
            slot->finct = (slot->finct & 0x18000) + (d << 7);
            slot->finc = (0x20000 + slot->finct) >> slot->fsft;
            return;

        case 0x12: // LFORE/LFOF/PLFOWS
            if (d & 0x80)
            {
                slot->lfoinc = -1;
                return;
            }
            else if (slot->lfoinc == -1)
            {
                slot->lfocnt = 0;
            }

            slot->lfoinc = scsp_lfo_step[(d >> 2) & 0x1F];

            switch (d & 3)
        {
            case 0:
                slot->lfofmw = scsp_lfo_sawt_f;
                return;

            case 1:
                slot->lfofmw = scsp_lfo_squa_f;
                return;

            case 2:
                slot->lfofmw = scsp_lfo_tri_f;
                return;

            case 3:
                slot->lfofmw = scsp_lfo_noi_f;
                return;
        }

        case 0x13: // PLFOS/ALFOWS/ALFOS
            if ((d >> 5) & 7)
                slot->lfofms = ((d >> 5) & 7) + 7;
            else
                slot->lfofms = 31;

            if (d & 7)
                slot->lfoems = ((d & 7) ^ 7) + 4;
            else
                slot->lfoems = 31;

            switch ((d >> 3) & 3)
        {
            case 0:
                slot->lfoemw = scsp_lfo_sawt_e;
                return;

            case 1:
                slot->lfoemw = scsp_lfo_squa_e;
                return;

            case 2:
                slot->lfoemw = scsp_lfo_tri_e;
                return;

            case 3:
                slot->lfoemw = scsp_lfo_noi_e;
        }
            return;

        case 0x15: // ISEL/OMXL
            if (d & 7)
                slot->imxl = ((d & 7) ^ 7) + SCSP_ENV_HB;
            else
                slot->imxl = 31;
            return;

        case 0x16: // DISDL/DIPAN
            if (d & 0xE0)
            {
                // adjusted for envelope calculation
                // some inaccuracy in panning though...
                slot->dislr = slot->disll = (((d >> 5) & 7) ^ 7) + SCSP_ENV_HB;
                if (d & 0x10)
                {
                    // Panning Left
                    if ((d & 0xF) == 0xF)
                        slot->dislr = 31;
                    else
                        slot->dislr += (d >> 1) & 7;
                }
                else
                {
                    // Panning Right
                    if ((d & 0xF) == 0xF)
                        slot->disll = 31;
                    else
                        slot->disll += (d >> 1) & 7;
                }
            }
            else
            {
                slot->dislr = slot->disll = 31; // muted
            }
            return;

        case 0x17: // EFSDL/EFPAN
            if (d & 0xE0)
            {
                slot->efslr = slot->efsll = (((d >> 5) & 7) ^ 7) + SCSP_ENV_HB;
                if (d & 0x10)
                {
                    // Panning Left
                    if ((d & 0xF) == 0xF)
                        slot->efslr = 31;
                    else
                        slot->efslr += (d >> 1) & 7;
                }
                else
                {
                    // Panning Right
                    if ((d & 0xF) == 0xF)
                        slot->efsll = 31;
                    else
                        slot->efsll += (d >> 1) & 7;
                }
            }
            else
            {
                slot->efslr = slot->efsll = 31; // muted
            }
            return;
    }
}

static void
scsp_slot_set_w (u32 s, s32 a, u16 d)
{
    slot_t *slot = &(scsp.slot[s]);

    SCSPLOG ("slot %d : reg %.2X = %.4X\n", s, a & 0x1E, d);

    *(u16 *)&scsp_isr[a ^ 2] = d;

    switch (a & 0x1E)
    {
        case 0x00: // KYONEX/KYONB/SBCTL/SSCTL/LPCTL/PCM8B/SA(highest 4 bits)
            slot->key = (d >> 11) & 1;
            slot->sbctl = (d >> 9) & 3;
            slot->ssctl = (d >> 7) & 3;
            slot->lpctl = (d >> 5) & 3;

            slot->pcm8b = d & 0x10;
            slot->sa = (slot->sa & 0x0FFFF) | ((d & 0xF) << 16);
            slot->sa &= SCSP_RAM_MASK;

            if (slot->ecnt < SCSP_ENV_DE)
                scsp_slot_update_keyon(slot);

            if (d & 0x1000)
                scsp_slot_keyonoff();
            return;

        case 0x02: // SA(low word)
            slot->sa = (slot->sa & 0xF0000) | d;
            slot->sa &= SCSP_RAM_MASK;

            if (slot->ecnt < SCSP_ENV_DE)
                scsp_slot_update_keyon(slot);
            return;

        case 0x04: // LSA
            slot->lsa = d << SCSP_FREQ_LB;
            return;

        case 0x06: // LEA
            slot->lea = d << SCSP_FREQ_LB;
            return;

        case 0x08: // D2R/D1R/EGHOLD/AR
            slot->sr = (d >> 11) & 0x1F;
            slot->dr = (d >> 6) & 0x1F;
            slot->eghold = d & 0x20;
            slot->ar = d & 0x1F;

            if (slot->sr)
                slot->srp = &scsp_decay_rate[slot->sr << 1];
            else
                slot->srp = &scsp_null_rate[0];

            if (slot->dr)
                slot->drp = &scsp_decay_rate[slot->dr << 1];
            else
                slot->drp = &scsp_null_rate[0];

            if (slot->ar)
                slot->arp = &scsp_attack_rate[slot->ar << 1];
            else
                slot->arp = &scsp_null_rate[0];

            scsp_slot_refresh_einc (slot, 0x1 | 0x2 | 0x4);
            return;

        case 0x0A: // LPSLNK/KRS/DL/RR
            slot->lslnk = (d >> 8) & 0x40;
            slot->krs = (d >> 10) & 0xF;

            if (slot->krs == 0xF)
                slot->krs = 4;
            else
                slot->krs >>= 2;

            slot->sl = ((d & 0x3E0) << SCSP_ENV_LB) + SCSP_ENV_DS; // adjusted for envelope compare (ecmp)
            slot->rr = d & 0x1F;

            if (slot->rr)
                slot->rrp = &scsp_decay_rate[slot->rr << 1];
            else
                slot->rrp = &scsp_null_rate[0];

            scsp_slot_refresh_einc (slot, 0xF);
            return;

        case 0x0C: // STWINH/SDIR
            slot->sdir = (d >> 8) & 2;
            slot->swe = (d >> 8) & 1;
            slot->tl = scsp_tl_table[(d & 0xFF)];
            return;

        case 0x0E: // MDL/MDXSL/MDYSL
            slot->mdl = (d >> 12) & 0xF; // need to adjust for correct shift
            slot->mdx = (d >> 6) & 0x3F;
            slot->mdy = d & 0x3F;
            return;

        case 0x10: // OCT/FNS
            if (d & 0x4000)
                slot->fsft = 23 - ((d >> 11) & 0xF);
            else
                slot->fsft = (((d >> 11) & 7) ^ 7);

            slot->finc = ((0x400 + (d & 0x3FF)) << 7) >> slot->fsft;

            scsp_slot_refresh_einc (slot, 0xF);
            return;

        case 0x12: // LFORE/LFOF/PLFOWS/PLFOS/ALFOWS/ALFOS
            if (d & 0x8000)
            {
                slot->lfoinc = -1;
                return;
            }
            else if (slot->lfoinc == -1)
            {
                slot->lfocnt = 0;
            }

            slot->lfoinc = scsp_lfo_step[(d >> 10) & 0x1F];
            if ((d >> 5) & 7)
                slot->lfofms = ((d >> 5) & 7) + 7;
            else
                slot->lfofms = 31;

            if (d & 7)
                slot->lfoems = ((d & 7) ^ 7) + 4;
            else
                slot->lfoems = 31;

            switch ((d >> 8) & 3)
        {
            case 0:
                slot->lfofmw = scsp_lfo_sawt_f;
                break;

            case 1:
                slot->lfofmw = scsp_lfo_squa_f;
                break;

            case 2:
                slot->lfofmw = scsp_lfo_tri_f;
                break;

            case 3:
                slot->lfofmw = scsp_lfo_noi_f;
                break;
        }

            switch ((d >> 3) & 3)
        {
            case 0:
                slot->lfoemw = scsp_lfo_sawt_e;
                return;

            case 1:
                slot->lfoemw = scsp_lfo_squa_e;
                return;

            case 2:
                slot->lfoemw = scsp_lfo_tri_e;
                return;

            case 3:
                slot->lfoemw = scsp_lfo_noi_e;
        }
            return;

        case 0x14: // ISEL/OMXL
            if (d & 7)
                slot->imxl = ((d & 7) ^ 7) + SCSP_ENV_HB;
            else
                slot->imxl = 31;
            return;

        case 0x16: // DISDL/DIPAN/EFSDL/EFPAN
            if (d & 0xE000)
            {
                // adjusted fr enveloppe calculation
                // some accuracy lose for panning here...
                slot->dislr = slot->disll = (((d >> 13) & 7) ^ 7) + SCSP_ENV_HB;
                if (d & 0x1000)
                {
                    // Panning Left
                    if ((d & 0xF00) == 0xF00)
                        slot->dislr = 31;
                    else
                        slot->dislr += (d >> 9) & 7;
                }
                else
                {
                    // Panning Right
                    if ((d & 0xF00) == 0xF00)
                        slot->disll = 31;
                    else
                        slot->disll += (d >> 9) & 7;
                }
            }
            else
            {
                slot->dislr = slot->disll = 31; // muted
            }

            if (d & 0xE0)
            {
                slot->efslr = slot->efsll = (((d >> 5) & 7) ^ 7) + SCSP_ENV_HB;
                if (d & 0x10)
                {
                    // Panning Left
                    if ((d & 0xF) == 0xF)
                        slot->efslr = 31;
                    else
                        slot->efslr += (d >> 1) & 7;
                }
                else
                {
                    // Panning Right
                    if ((d & 0xF) == 0xF)
                        slot->efsll = 31;
                    else
                        slot->efsll += (d >> 1) & 7;
                }
            }
            else
            {
                slot->efslr = slot->efsll = 31; // muted
            }
            return;
    }
}

static u8
scsp_slot_get_b (u32 s, u32 a)
{
    u8 val = scsp_isr[a ^ 3];

    // Mask out keyonx
    if ((a & 0x1F) == 0x00) val &= 0xEF;

    SCSPLOG ("r_b slot %d (%.2X) : reg %.2X = %.2X\n", s, a, a & 0x1F, val);

    return val;
}

static u16 scsp_slot_get_w(u32 s, u32 a)
{
    u16 val = *(u16 *)&scsp_isr[a ^ 2];

    if ((a & 0x1E) == 0x00) return val &= 0xEFFF;

    SCSPLOG ("r_w slot %d (%.2X) : reg %.2X = %.4X\n", s, a, a & 0x1E, val);

    return val;
}

////////////////////////////////////////////////////////////////
// Misc

static int
scsp_round (double val)
{
    return (int)(val + 0.5);
}


////////////////////////////////////////////////////////////////
// Interrupts

static INLINE void
scsp_trigger_main_interrupt (u32 id)
{
    SCSPLOG ("scsp main interrupt accepted %.4X\n", id);
    scsp.mintf ();
}

static INLINE void
scsp_trigger_sound_interrupt (u32 id)
{
    u32 level;
    level = 0;
    if (id > 0x80) id = 0x80;

    if (scsp.scilv0 & id) level |= 1;
    if (scsp.scilv1 & id) level |= 2;
    if (scsp.scilv2 & id) level |= 4;

#ifdef SCSP_DEBUG
    if (id == 0x8) SCSPLOG ("scsp sound interrupt accepted %.2X lev=%d\n", id, level);
#endif

    scsp.sintf (level);
}

static void
scsp_main_interrupt (u32 id)
{
    //  if (scsp.mcipd & id) return;
    //  if (id != 0x400) SCSPLOG("scsp main interrupt %.4X\n", id);

    scsp.mcipd |= id;
    WRITE_THROUGH (scsp.mcipd);

    if (scsp.mcieb & id)
        scsp_trigger_main_interrupt (id);
}

static void
scsp_sound_interrupt (u32 id)
{
    //  if (scsp.scipd & id) return;

    //  SCSPLOG ("scsp sound interrupt %.4X\n", id);

    scsp.scipd |= id;
    WRITE_THROUGH (scsp.scipd);

    if (scsp.scieb & id)
        scsp_trigger_sound_interrupt (id);
}

////////////////////////////////////////////////////////////////
// Direct Memory Access

static void
scsp_dma (void)
{
    if (scsp.dmfl & 0x20)
    {
        // dsp -> scsp_ram
        SCSPLOG ("scsp dma: scsp_ram(%08lx) <- reg(%08lx) * %08lx\n",
                 scsp.dmea, scsp.drga, scsp.dmlen);
    }
    else
    {
        // scsp_ram -> dsp
        SCSPLOG ("scsp dma: scsp_ram(%08lx) -> reg(%08lx) * %08lx\n",
                 scsp.dmea, scsp.drga, scsp.dmlen);
    }

    scsp_ccr[0x16 ^ 3] &= 0xE0;

    scsp_sound_interrupt (0x10);
    scsp_main_interrupt (0x10);
}


/*
 Envelope Events Handler

 Max EG level = 0x3FF      /|\
 / | \
 /  |  \_____
 Min EG level = 0x000 __/   |  |    |\___
 A   D1 D2   R
 */

static void
scsp_env_null_next (UNUSED slot_t *slot)
{
    // only to prevent null call pointer...
}

static void
scsp_release_next (slot_t *slot)
{
    // end of release happened, update to process the next phase...

    slot->ecnt = SCSP_ENV_DE;
    slot->einc = NULL;
    slot->ecmp = SCSP_ENV_DE + 1;
    slot->enxt = scsp_env_null_next;
}

static void
scsp_sustain_next (slot_t *slot)
{
    // end of sustain happened, update to process the next phase...

    slot->ecnt = SCSP_ENV_DE;
    slot->einc = NULL;
    slot->ecmp = SCSP_ENV_DE + 1;
    slot->enxt = scsp_env_null_next;
}

static void
scsp_decay_next (slot_t *slot)
{
    // end of decay happened, update to process the next phase...

    slot->ecnt = slot->sl;
    slot->einc = &slot->eincs;
    slot->ecmp = SCSP_ENV_DE;
    slot->ecurp = SCSP_ENV_SUSTAIN;
    slot->enxt = scsp_sustain_next;
}

static void
scsp_attack_next (slot_t *slot)
{
    // end of attack happened, update to process the next phase...

    slot->ecnt = SCSP_ENV_DS;
    slot->einc = &slot->eincd;
    slot->ecmp = slot->sl;
    slot->ecurp = SCSP_ENV_DECAY;
    slot->enxt = scsp_decay_next;
}


////////////////////////////////////////////////////////////////
// Synth Slot
//
//      SCSPLOG("outL=%.8X bufL=%.8X disll=%d\n", outL, scsp_bufL[scsp_buf_pos], slot->disll);

////////////////////////////////////////////////////////////////

#ifdef WORDS_BIGENDIAN
#define SCSP_GET_OUT_8B \
out = (s32) slot->buf8[(slot->fcnt >> SCSP_FREQ_LB)];
#else
#define SCSP_GET_OUT_8B \
out = (s32) slot->buf8[(slot->fcnt >> SCSP_FREQ_LB) ^ 1];
#endif

#define SCSP_GET_OUT_16B \
out = (s32) slot->buf16[slot->fcnt >> SCSP_FREQ_LB];

#define SCSP_GET_ENV \
slot->env = scsp_env_table[slot->ecnt >> SCSP_ENV_LB] * slot->tl / 1024;

#define SCSP_GET_ENV_LFO \
slot->env = (scsp_env_table[slot->ecnt >> SCSP_ENV_LB] * slot->tl / 1024) - \
(slot->lfoemw[(slot->lfocnt >> SCSP_LFO_LB) & SCSP_LFO_MASK] >> \
slot->lfoems);

#define SCSP_OUT_8B_L \
if ((out) && (slot->env > 0))                            \
{                                                      \
out *= slot->env;                                    \
scsp_bufL[scsp_buf_pos] += out >> (slot->disll - 8); \
}

#define SCSP_OUT_8B_R \
if ((out) && (slot->env > 0))                            \
{                                                      \
out *= slot->env;                                    \
scsp_bufR[scsp_buf_pos] += out >> (slot->dislr - 8); \
}

#define SCSP_OUT_8B_LR \
if ((out) && (slot->env > 0))                            \
{                                                      \
out *= slot->env;                                    \
scsp_bufL[scsp_buf_pos] += out >> (slot->disll - 8); \
scsp_bufR[scsp_buf_pos] += out >> (slot->dislr - 8); \
}

#define SCSP_OUT_16B_L \
if ((out) && (slot->env > 0))                      \
{                                                \
out *= slot->env;                              \
scsp_bufL[scsp_buf_pos] += out >> slot->disll; \
}

#define SCSP_OUT_16B_R \
if ((out) && (slot->env > 0))                      \
{                                                \
out *= slot->env;                              \
scsp_bufR[scsp_buf_pos] += out >> slot->dislr; \
}

#define SCSP_OUT_16B_LR \
if ((out) && (slot->env > 0))                    \
{                                                \
out *= slot->env;                              \
scsp_bufL[scsp_buf_pos] += out >> slot->disll; \
scsp_bufR[scsp_buf_pos] += out >> slot->dislr; \
}

#define SCSP_UPDATE_PHASE \
if ((slot->fcnt += slot->finc) > slot->lea) \
{                                           \
if (slot->lpctl)                          \
{                                       \
slot->fcnt = slot->lsa;               \
}                                       \
else                                      \
{                                       \
slot->ecnt = SCSP_ENV_DE;             \
return;                               \
}                                       \
}

#define SCSP_UPDATE_PHASE_LFO \
slot->fcnt +=                                                      \
((slot->lfofmw[(slot->lfocnt >> SCSP_LFO_LB) & SCSP_LFO_MASK] << \
(slot->lfofms-7)) >> (slot->fsft+1));                          \
if ((slot->fcnt += slot->finc) > slot->lea)                        \
{                                                                \
if (slot->lpctl)                                               \
{                                                            \
slot->fcnt = slot->lsa;                                    \
}                                                            \
else                                                           \
{                                                            \
slot->ecnt = SCSP_ENV_DE;                                  \
return;                                                    \
}                                                            \
}

#define SCSP_UPDATE_ENV \
if (slot->einc) slot->ecnt += *slot->einc; \
if (slot->ecnt >= slot->ecmp)              \
{                                        \
slot->enxt(slot);                      \
if (slot->ecnt >= SCSP_ENV_DE) return; \
}

#define SCSP_UPDATE_LFO \
slot->lfocnt += slot->lfoinc;

////////////////////////////////////////////////////////////////

static void
scsp_slot_update_keyon (slot_t *slot)
{
    // set buffer, loop start/end address of the slot
    if (slot->pcm8b)
    {
        slot->buf8 = (s8*)&(scsp.scsp_ram[slot->sa]);

        if ((slot->sa + (slot->lea >> SCSP_FREQ_LB)) > SCSP_RAM_MASK)
            slot->lea = (SCSP_RAM_MASK - slot->sa) << SCSP_FREQ_LB;
    }
    else
    {
        slot->buf16 = (s16*)&(scsp.scsp_ram[slot->sa & ~1]);

        if ((slot->sa + (slot->lea >> (SCSP_FREQ_LB - 1))) > SCSP_RAM_MASK)
            slot->lea = (SCSP_RAM_MASK - slot->sa) << (SCSP_FREQ_LB - 1);
    }

    SCSP_UPDATE_PHASE
}

////////////////////////////////////////////////////////////////

static void
scsp_slot_update_null (slot_t *slot)
{
    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_ENV

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
    }
}

////////////////////////////////////////////////////////////////
// Normal 8 bits

static void
scsp_slot_update_8B_L (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        // env = [0..0x3FF] - slot->tl
        SCSP_GET_OUT_8B
        SCSP_GET_ENV

        // don't waste time if no sound...
        SCSP_OUT_8B_L

        // calculate new frequency (phase) counter and enveloppe counter
        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
    }
}

static void
scsp_slot_update_8B_R (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV

        SCSP_OUT_8B_R

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
    }
}

static void
scsp_slot_update_8B_LR(slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV

        SCSP_OUT_8B_LR

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
    }
}

////////////////////////////////////////////////////////////////
// Envelope LFO modulation 8 bits

static void
scsp_slot_update_E_8B_L (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV_LFO

        SCSP_OUT_8B_L

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_E_8B_R (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV_LFO

        SCSP_OUT_8B_R

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_E_8B_LR (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV_LFO

        SCSP_OUT_8B_LR

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

////////////////////////////////////////////////////////////////
// Frequency LFO modulation 8 bits

static void
scsp_slot_update_F_8B_L (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV

        SCSP_OUT_8B_L

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_F_8B_R (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV

        SCSP_OUT_8B_R

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_F_8B_LR (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV

        SCSP_OUT_8B_LR

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

////////////////////////////////////////////////////////////////
// Enveloppe & Frequency LFO modulation 8 bits

static void
scsp_slot_update_F_E_8B_L (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV_LFO

        SCSP_OUT_8B_L

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_F_E_8B_R (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV_LFO

        SCSP_OUT_8B_R

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void scsp_slot_update_F_E_8B_LR(slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_8B
        SCSP_GET_ENV_LFO

        SCSP_OUT_8B_LR

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

////////////////////////////////////////////////////////////////
// Normal 16 bits

static void
scsp_slot_update_16B_L (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV

        SCSP_OUT_16B_L

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
    }
}

static void
scsp_slot_update_16B_R (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV

        SCSP_OUT_16B_R

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
    }
}

static void
scsp_slot_update_16B_LR (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV

        SCSP_OUT_16B_LR

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
    }
}

////////////////////////////////////////////////////////////////
// Envelope LFO modulation 16 bits

static void
scsp_slot_update_E_16B_L (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV_LFO

        SCSP_OUT_16B_L

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_E_16B_R (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV_LFO

        SCSP_OUT_16B_R

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_E_16B_LR (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV_LFO

        SCSP_OUT_16B_LR

        SCSP_UPDATE_PHASE
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

////////////////////////////////////////////////////////////////
// Frequency LFO modulation 16 bits

static void
scsp_slot_update_F_16B_L (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV

        SCSP_OUT_16B_L

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_F_16B_R (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV

        SCSP_OUT_16B_R

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_F_16B_LR (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV

        SCSP_OUT_16B_LR

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

////////////////////////////////////////////////////////////////
// Envelope & Frequency LFO modulation 16 bits

static void
scsp_slot_update_F_E_16B_L (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV_LFO

        SCSP_OUT_16B_L

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_F_E_16B_R (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV_LFO

        SCSP_OUT_16B_R

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

static void
scsp_slot_update_F_E_16B_LR (slot_t *slot)
{
    s32 out;

    for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
    {
        SCSP_GET_OUT_16B
        SCSP_GET_ENV_LFO

        SCSP_OUT_16B_LR

        SCSP_UPDATE_PHASE_LFO
        SCSP_UPDATE_ENV
        SCSP_UPDATE_LFO
    }
}

////////////////////////////////////////////////////////////////
// MIDI

void
scsp_midi_in_send (u8 data)
{
    if (scsp.midflag & SCSP_MIDI_IN_EMP)
    {
        scsp_sound_interrupt(0x8);
        scsp_main_interrupt(0x8);
    }

    scsp.midflag &= ~SCSP_MIDI_IN_EMP;

    if (scsp.midincnt > 3)
    {
        scsp.midflag |= SCSP_MIDI_IN_OVF;
        return;
    }

    scsp.midinbuf[scsp.midincnt++] = data;

    if (scsp.midincnt > 3) scsp.midflag |= SCSP_MIDI_IN_FUL;
}

void
scsp_midi_out_send (u8 data)
{
    scsp.midflag &= ~SCSP_MIDI_OUT_EMP;

    if (scsp.midoutcnt > 3) return;

    scsp.midoutbuf[scsp.midoutcnt++] = data;

    if (scsp.midoutcnt > 3) scsp.midflag |= SCSP_MIDI_OUT_FUL;
}

u8
scsp_midi_in_read (void)
{
    u8 data;

    scsp.midflag &= ~(SCSP_MIDI_IN_OVF | SCSP_MIDI_IN_FUL);

    if (scsp.midincnt > 0)
    {
        if (scsp.midincnt > 1)
        {
            scsp_sound_interrupt(0x8);
            scsp_main_interrupt(0x8);
        }
        else
        {
            scsp.midflag |= SCSP_MIDI_IN_EMP;
        }

        data = scsp.midinbuf[0];

        switch ((--scsp.midincnt) & 3)
        {
            case 1:
                scsp.midinbuf[0] = scsp.midinbuf[1];
                break;

            case 2:
                scsp.midinbuf[0] = scsp.midinbuf[1];
                scsp.midinbuf[1] = scsp.midinbuf[2];
                break;

            case 3:
                scsp.midinbuf[0] = scsp.midinbuf[1];
                scsp.midinbuf[1] = scsp.midinbuf[2];
                scsp.midinbuf[2] = scsp.midinbuf[3];
                break;
        }

        return data;
    }

    return 0xFF;
}

u8
scsp_midi_out_read (void)
{
    u8 data;

    scsp.midflag &= ~SCSP_MIDI_OUT_FUL;

    if (scsp.midoutcnt > 0)
    {
        if (scsp.midoutcnt == 1)
        {
            scsp.midflag |= SCSP_MIDI_OUT_EMP;
            scsp_sound_interrupt(0x200);
            scsp_main_interrupt(0x200);
        }

        data = scsp.midoutbuf[0];

        switch (--scsp.midoutcnt & 3)
        {
            case 1:
                scsp.midoutbuf[0] = scsp.midoutbuf[1];
                break;

            case 2:
                scsp.midoutbuf[0] = scsp.midoutbuf[1];
                scsp.midoutbuf[1] = scsp.midoutbuf[2];
                break;

            case 3:
                scsp.midoutbuf[0] = scsp.midoutbuf[1];
                scsp.midoutbuf[1] = scsp.midoutbuf[2];
                scsp.midoutbuf[2] = scsp.midoutbuf[3];
                break;
        }

        return data;
    }

    return 0xFF;
}

////////////////////////////////////////////////////////////////
// Update functions

static void (*scsp_slot_update_p[2][2][2][2][2])(slot_t *slot) =
{
    // NO FMS
    {  // NO EMS
        {  // 8 BITS
            {  // NO LEFT
                {  // NO RIGHT
                    scsp_slot_update_null,
                    // RIGHT
                    scsp_slot_update_8B_R
                },
                // LEFT
                {  // NO RIGHT
                    scsp_slot_update_8B_L,
                    // RIGHT
                    scsp_slot_update_8B_LR
                },
            },
            // 16 BITS
            {  // NO LEFT
                {  // NO RIGHT
                    scsp_slot_update_null,
                    // RIGHT
                    scsp_slot_update_16B_R
                },
                // LEFT
                {  // NO RIGHT
                    scsp_slot_update_16B_L,
                    // RIGHT
                    scsp_slot_update_16B_LR
                },
            }
        },
        // EMS
        {  // 8 BITS
            {  // NO LEFT
                {  // NO RIGHT
                    scsp_slot_update_null,
                    // RIGHT
                    scsp_slot_update_E_8B_R
                },
                // LEFT
                {  // NO RIGHT
                    scsp_slot_update_E_8B_L,
                    // RIGHT
                    scsp_slot_update_E_8B_LR
                },
            },
            // 16 BITS
            {  // NO LEFT
                {  // NO RIGHT
                    scsp_slot_update_null,
                    // RIGHT
                    scsp_slot_update_E_16B_R
                },
                // LEFT
                {  // NO RIGHT
                    scsp_slot_update_E_16B_L,
                    // RIGHT
                    scsp_slot_update_E_16B_LR
                },
            }
        }
    },
    // FMS
    {  // NO EMS
        {  // 8 BITS
            {  // NO LEFT
                {  // NO RIGHT
                    scsp_slot_update_null,
                    // RIGHT
                    scsp_slot_update_F_8B_R
                },
                // LEFT
                {  // NO RIGHT
                    scsp_slot_update_F_8B_L,
                    // RIGHT
                    scsp_slot_update_F_8B_LR
                },
            },
            // 16 BITS
            {  // NO LEFT
                {  // NO RIGHT
                    scsp_slot_update_null,
                    // RIGHT
                    scsp_slot_update_F_16B_R
                },
                // LEFT
                {  // NO RIGHT
                    scsp_slot_update_F_16B_L,
                    // RIGHT
                    scsp_slot_update_F_16B_LR
                },
            }
        },
        // EMS
        {  // 8 BITS
            {  // NO LEFT
                {  // NO RIGHT
                    scsp_slot_update_null,
                    // RIGHT
                    scsp_slot_update_F_E_8B_R
                },
                // LEFT
                {  // NO RIGHT
                    scsp_slot_update_F_E_8B_L,
                    // RIGHT
                    scsp_slot_update_F_E_8B_LR
                },
            },
            // 16 BITS
            {  // NO LEFT
                {  // NO RIGHT
                    scsp_slot_update_null,
                    // RIGHT
                    scsp_slot_update_F_E_16B_R
                },
                // LEFT
                {  // NO RIGHT
                    scsp_slot_update_F_E_16B_L,
                    // RIGHT
                    scsp_slot_update_F_E_16B_LR
                },
            }
        }
    }
};

void
scsp_update (s32 *bufL, s32 *bufR, u32 len)
{
    slot_t *slot;

    scsp_bufL = bufL;
    scsp_bufR = bufR;

    for (slot = &(scsp.slot[0]); slot < &(scsp.slot[32]); slot++)
    {
        if (slot->ecnt >= SCSP_ENV_DE) continue; // enveloppe null...

        if (slot->ssctl)
        {
            // Still not correct, but at least this fixes games
            // that rely on Call Address information
            scsp_buf_len = len;
            scsp_buf_pos = 0;

            for (; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
            {
                if ((slot->fcnt += slot->finc) > slot->lea)
                {
                    if (slot->lpctl) slot->fcnt = slot->lsa;
                    else
                    {
                        slot->ecnt = SCSP_ENV_DE;
                        break;
                    }
                }
            }

            continue; // not yet supported!
        }

        scsp_buf_len = len;
        scsp_buf_pos = 0;

        // take effect sound volume if no direct sound volume...
        if ((slot->disll == 31) && (slot->dislr == 31))
        {
            slot->disll = slot->efsll;
            slot->dislr = slot->efslr;
        }

        // SCSPLOG("update : VL=%d  VR=%d CNT=%.8X STEP=%.8X\n", slot->disll, slot->dislr, slot->fcnt, slot->finc);

        scsp_slot_update_p[(slot->lfofms == 31) ? 0 : 1]
        [(slot->lfoems == 31) ? 0 : 1]
        [(slot->pcm8b == 0) ? 1 : 0]
        [(slot->disll == 31) ? 0 : 1]
        [(slot->dislr == 31) ? 0 : 1](slot);
    }

    if (cdda_out_left > 0)
    {
        if (len > cdda_out_left / 4)
            scsp_buf_len = cdda_out_left / 4;
        else
            scsp_buf_len = len;

        scsp_buf_pos = 0;

        /* May need to wrap around the buffer, so use nested loops */
        while (scsp_buf_pos < scsp_buf_len)
        {
            s32 temp = cdda_next_in - cdda_out_left;
            s32 outpos = (temp < 0) ? temp + sizeof(cddabuf.data) : temp;
            u8 *buf = &cddabuf.data[outpos];

            u32 scsp_buf_target;
            u32 this_len = scsp_buf_len - scsp_buf_pos;
            if (this_len > (sizeof(cddabuf.data) - outpos) / 4)
                this_len = (sizeof(cddabuf.data) - outpos) / 4;
            scsp_buf_target = scsp_buf_pos + this_len;

            for (; scsp_buf_pos < scsp_buf_target; scsp_buf_pos++, buf += 4)
            {
                s32 out;

                out = (s32)(s16)((buf[1] << 8) | buf[0]);

                if (out)
                    scsp_bufL[scsp_buf_pos] += out;

                out = (s32)(s16)((buf[3] << 8) | buf[2]);

                if (out)
                    scsp_bufR[scsp_buf_pos] += out;
            }

            cdda_out_left -= this_len * 4;
        }
    }
    else if (Cs2Area->isaudio)
    {
        SCSPLOG("WARNING: CDDA buffer underrun\n");
    }
}

void
scsp_update_monitor(void)
{
    if (use_new_scsp)
    {
        scsp.ca = new_scsp.slots[scsp.mslc].state.sample_offset >> 5;
        scsp.sgc = new_scsp.slots[scsp.mslc].state.envelope;
        scsp.eg = new_scsp.slots[scsp.mslc].state.attenuation >> 5;
    }
    else
    {
        slot_t *slot = &scsp.slot[scsp.mslc];
        scsp.ca = ((slot->fcnt >> (SCSP_FREQ_LB + 12)) & 0xF) << 7;
        scsp.sgc = slot->ecurp;
        scsp.eg = 0x1f - (slot->env >> (SCSP_ENV_HB - 5));
    }
#ifdef PSP
    WRITE_THROUGH(scsp.ca);
    WRITE_THROUGH(scsp.sgc);
    WRITE_THROUGH(scsp.eg);
#endif
}

void
scsp_update_timer (u32 len)
{
    scsp.timacnt += len << (8 - scsp.timasd);

    if (scsp.timacnt >= 0xFF00)
    {
        if (!(scsp.scipd & 0x40))
            scsp_sound_interrupt(0x40);
        if (!(scsp.mcipd & 0x40))
            scsp_main_interrupt(0x40);
        scsp.timacnt -= 0xFF00;
    }

    scsp.timbcnt += len << (8 - scsp.timbsd);

    if (scsp.timbcnt >= 0xFF00)
    {
        if (!(scsp.scipd & 0x80))
            scsp_sound_interrupt(0x80);
        if (!(scsp.mcipd & 0x80))
            scsp_main_interrupt(0x80);
        scsp.timbcnt -= 0xFF00;
    }

    scsp.timccnt += len << (8 - scsp.timcsd);

    if (scsp.timccnt >= 0xFF00)
    {
        if (!(scsp.scipd & 0x100))
            scsp_sound_interrupt(0x100);
        if (!(scsp.mcipd & 0x100))
            scsp_main_interrupt(0x100);
        scsp.timccnt -= 0xFF00;
    }

    // 1F interrupt can't be accurate here...
    if (len)
    {
        if (!(scsp.scipd & 0x400))
            scsp_sound_interrupt(0x400);
        if (!(scsp.mcipd & 0x400))
            scsp_main_interrupt(0x400);
    }
}

////////////////////////////////////////////////////////////////
// Interface

void
scsp_shutdown (void)
{

}

void
scsp_reset (void)
{
    slot_t *slot;

    memset(scsp_reg, 0, 0x1000);

    scsp.mem4b     = 0;
    scsp.mvol      = 0;
    scsp.rbl       = 0;
    scsp.rbp       = 0;
    scsp.mslc      = 0;
    scsp.ca        = 0;

    scsp.dmea      = 0;
    scsp.drga      = 0;
    scsp.dmfl      = 0;
    scsp.dmlen     = 0;

    scsp.midincnt  = 0;
    scsp.midoutcnt = 0;
    scsp.midflag   = SCSP_MIDI_IN_EMP | SCSP_MIDI_OUT_EMP;
    scsp.midflag2  = 0;

    scsp.timacnt   = 0xFF00;
    scsp.timbcnt   = 0xFF00;
    scsp.timccnt   = 0xFF00;
    scsp.timasd    = 0;
    scsp.timbsd    = 0;
    scsp.timcsd    = 0;

    scsp.mcieb     = 0;
    scsp.mcipd     = 0;
    scsp.scieb     = 0;
    scsp.scipd     = 0;
    scsp.scilv0    = 0;
    scsp.scilv1    = 0;
    scsp.scilv2    = 0;

    for(slot = &(scsp.slot[0]); slot < &(scsp.slot[32]); slot++)
    {
        memset(slot, 0, sizeof(slot_t));
        slot->ecnt = SCSP_ENV_DE;       // slot off
        slot->ecurp = SCSP_ENV_RELEASE;
        slot->dislr = slot->disll = 31; // direct level sound off
        slot->efslr = slot->efsll = 31; // effect level sound off

        // Make sure lfofmw/lfoemw have sane values
        slot->lfofmw = scsp_lfo_sawt_f;
        slot->lfoemw = scsp_lfo_sawt_e;
    }

    if (use_new_scsp)
        new_scsp_reset(&new_scsp);
    else
        scsp_dsp_int_init();
}

void
scsp_init (u8 *scsp_ram, void (*sint_hand)(u32), void (*mint_hand)(void))
{
    u32 i, j;
    double x;

    scsp_shutdown ();

    scsp_isr = &scsp_reg[0x0000];
    scsp_ccr = &scsp_reg[0x0400];
    scsp_dcr = &scsp_reg[0x0700];

    scsp.scsp_ram = scsp_ram;
    scsp.sintf = sint_hand;
    scsp.mintf = mint_hand;

    for (i = 0; i < SCSP_ENV_LEN; i++)
    {
        // Attack Curve (x^7 ?)
        x = pow (((double)(SCSP_ENV_MASK - i) / (double)SCSP_ENV_LEN), 7);
        x *= (double)SCSP_ENV_LEN;
        scsp_env_table[i] = SCSP_ENV_MASK - (s32)x;

        // Decay curve (x = linear)
        x = pow (((double)i / (double)SCSP_ENV_LEN), 1);
        x *= (double)SCSP_ENV_LEN;
        scsp_env_table[i + SCSP_ENV_LEN] = SCSP_ENV_MASK - (s32)x;
    }

    for (i = 0, j = 0; i < 32; i++)
    {
        j += 1 << (i >> 2);

        // lfo freq
        x = (SCSP_FREQ / 256.0) / (double)j;

        // converting lfo freq in lfo step
        scsp_lfo_step[31 - i] = scsp_round(x * ((double)SCSP_LFO_LEN /
                                                (double)SCSP_FREQ) *
                                           (double)(1 << SCSP_LFO_LB));
    }

    // Calculate LFO (modulation) values
    for (i = 0; i < SCSP_LFO_LEN; i++)
    {
        // Envelope modulation
        scsp_lfo_sawt_e[i] = SCSP_LFO_MASK - i;

        if (i < (SCSP_LFO_LEN / 2))
            scsp_lfo_squa_e[i] = SCSP_LFO_MASK;
        else
            scsp_lfo_squa_e[i] = 0;

        if (i < (SCSP_LFO_LEN / 2))
            scsp_lfo_tri_e[i] = SCSP_LFO_MASK - (i * 2);
        else
            scsp_lfo_tri_e[i] = (i - (SCSP_LFO_LEN / 2)) * 2;

        scsp_lfo_noi_e[i] = rand() & SCSP_LFO_MASK;

        // Frequency modulation
        scsp_lfo_sawt_f[(i + 512) & SCSP_LFO_MASK] = i - (SCSP_LFO_LEN / 2);

        if (i < (SCSP_LFO_LEN / 2))
            scsp_lfo_squa_f[i] = SCSP_LFO_MASK - (SCSP_LFO_LEN / 2) - 128;
        else
            scsp_lfo_squa_f[i] = 0 - (SCSP_LFO_LEN / 2) + 128;

        if (i < (SCSP_LFO_LEN / 2))
            scsp_lfo_tri_f[(i + 768) & SCSP_LFO_MASK] = (i * 2) -
            (SCSP_LFO_LEN / 2);
        else
            scsp_lfo_tri_f[(i + 768) & SCSP_LFO_MASK] =
            (SCSP_LFO_MASK - ((i - (SCSP_LFO_LEN / 2)) * 2)) -
            (SCSP_LFO_LEN / 2) + 1;

        scsp_lfo_noi_f[i] = scsp_lfo_noi_e[i] - (SCSP_LFO_LEN / 2);
    }

    for(i = 0; i < 4; i++)
    {
        scsp_attack_rate[i] = 0;
        scsp_decay_rate[i] = 0;
    }

    for(i = 0; i < 60; i++)
    {
        x = 1.0 + ((i & 3) * 0.25);                  // bits 0-1 : x1.00, x1.25, x1.50, x1.75
        x *= (double)(1 << ((i >> 2)));              // bits 2-5 : shift bits (x2^0 - x2^15)
        x *= (double)(SCSP_ENV_LEN << SCSP_ENV_LB);  // adjust for table scsp_env_table

        scsp_attack_rate[i + 4] = scsp_round(x / (double)SCSP_ATTACK_R);
        scsp_decay_rate[i + 4] = scsp_round(x / (double)SCSP_DECAY_R);

        if (scsp_attack_rate[i + 4] == 0) scsp_attack_rate[i + 4] = 1;
        if (scsp_decay_rate[i + 4] == 0) scsp_decay_rate[i + 4] = 1;
    }

    scsp_attack_rate[63] = SCSP_ENV_AE;
    scsp_decay_rate[61] = scsp_decay_rate[60];
    scsp_decay_rate[62] = scsp_decay_rate[60];
    scsp_decay_rate[63] = scsp_decay_rate[60];

    for(i = 64; i < 96; i++)
    {
        scsp_attack_rate[i] = scsp_attack_rate[63];
        scsp_decay_rate[i] = scsp_decay_rate[63];
        scsp_null_rate[i - 64] = 0;
    }

    for(i = 0; i < 96; i++)
    {
        SCSPLOG ("attack rate[%d] = %.8X -> %.8X\n", i, scsp_attack_rate[i],
                 scsp_attack_rate[i] >> SCSP_ENV_LB);
        SCSPLOG ("decay rate[%d] = %.8X -> %.8X\n", i, scsp_decay_rate[i],
                 scsp_decay_rate[i] >> SCSP_ENV_LB);
    }

    for(i = 0; i < 256; i++)
        scsp_tl_table[i] = scsp_round(pow(10, ((double)i * -0.3762) / 20) * 1024.0);

    scsp_reset();
}

////////////////////////////////////////////////////////////////
// SCSP Access

static void
scsp_set_b (u32 a, u8 d)
{
  if ((a != 0x408) && (a != 0x41D))
    {
      SCSPLOG("scsp : reg %.2X = %.2X\n", a & 0x3F, d);
    }

  scsp_ccr[a ^ 3] = d;

  switch (a & 0x3F)
    {
    case 0x00: // MEM4MB/DAC18B
      scsp.mem4b = (d >> 1) & 0x1;
      if (scsp.mem4b)
        {
          M68K->SetFetch(0x000000, 0x080000, (pointer)SoundRam);
        }
      else
        {
          M68K->SetFetch(0x000000, 0x040000, (pointer)SoundRam);
          M68K->SetFetch(0x040000, 0x080000, (pointer)SoundRam);
          M68K->SetFetch(0x080000, 0x0C0000, (pointer)SoundRam);
          M68K->SetFetch(0x0C0000, 0x100000, (pointer)SoundRam);
        }
      return;

    case 0x01: // VER/MVOL
      scsp.mvol = d & 0xF;
      return;

    case 0x02: // RBL(high bit)
      scsp.rbl = (scsp.rbl & 1) + ((d & 1) << 1);
      return;

    case 0x03: // RBL(low bit)/RBP
      scsp.rbl = (scsp.rbl & 2) + ((d >> 7) & 1);
      scsp.rbp = (d & 0x7F) * (4 * 1024 * 2);
      return;

    case 0x07: // MOBUF
      scsp_midi_out_send(d);
      return;

    case 0x08: // MSLC
      scsp.mslc = (d >> 3) & 0x1F;
      scsp_update_monitor ();
      return;

    case 0x12: // DMEAL(high byte)
      scsp.dmea = (scsp.dmea & 0x700FE) + (d << 8);
      return;

    case 0x13: // DMEAL(low byte)
      scsp.dmea = (scsp.dmea & 0x7FF00) + (d & 0xFE);
      return;

    case 0x14: // DMEAH(high byte)
      scsp.dmea = (scsp.dmea & 0xFFFE) + ((d & 0x70) << 12);
      scsp.drga = (scsp.drga & 0xFE) + ((d & 0xF) << 8);
      return;

    case 0x15: // DMEAH(low byte)
      scsp.drga = (scsp.drga & 0xF00) + (d & 0xFE);
      return;

    case 0x16: // DGATE/DDIR/DEXE/DTLG(upper 4 bits)
      scsp.dmlen = (scsp.dmlen & 0xFE) + ((d & 0xF) << 8);
      if ((scsp.dmfl = d & 0xF0) & 0x10) scsp_dma ();
      return;

    case 0x17: // DTLG(lower byte)
      scsp.dmlen = (scsp.dmlen & 0xF00) + (d & 0xFE);
      return;

    case 0x18: // TACTL
      scsp.timasd = d & 7;
      return;

    case 0x19: // TIMA
      scsp.timacnt = d << 8;
      return;

    case 0x1A: // TBCTL
      scsp.timbsd = d & 7;
      return;

    case 0x1B: // TIMB
      scsp.timbcnt = d << 8;
      return;

    case 0x1C: // TCCTL
      scsp.timcsd = d & 7;
      return;

    case 0x1D: // TIMC
      scsp.timccnt = d << 8;
      return;

    case 0x1E: // SCIEB(high byte)
    {
      int i;
      scsp.scieb = (scsp.scieb & 0xFF) + (d << 8);

      for (i = 0; i < 3; i++)
        {
          if (scsp.scieb & (1 << i) && scsp.scipd & (1 << i))
            scsp_trigger_sound_interrupt ((1 << (i+8)));
        }

      return;
    }
    case 0x1F: // SCIEB(low byte)
    {
      int i;
      scsp.scieb = (scsp.scieb & 0x700) + d;

      for (i = 0; i < 8; i++)
        {
          if (scsp.scieb & (1 << i) && scsp.scipd & (1 << i))
            scsp_trigger_sound_interrupt ((1 << i));
        }
      return;
    }
    case 0x21: // SCIPD(low byte)
      if (d & 0x20) scsp_sound_interrupt (0x20);
      return;

    case 0x22: // SCIRE(high byte)
      scsp.scipd &= ~(d << 8);
      return;

    case 0x23: // SCIRE(low byte)
      scsp.scipd &= ~(u32)d;
      return;

    case 0x25: // SCILV0
      scsp.scilv0 = d;
      return;

    case 0x27: // SCILV1
      scsp.scilv1 = d;
      return;

    case 0x29: // SCILV2
      scsp.scilv2 = d;
      return;

    case 0x2A: // MCIEB(high byte)
      scsp.mcieb = (scsp.mcieb & 0xFF) + (d << 8);
      return;

    case 0x2B: // MCIEB(low byte)
      scsp.mcieb = (scsp.mcieb & 0x700) + d;
      return;

    case 0x2D: // MCIPD(low byte)
      if (d & 0x20)
        scsp_main_interrupt(0x20);
      return;

    case 0x2E: // MCIRE(high byte)
      scsp.mcipd &= ~(d << 8);
      return;

    case 0x2F: // MCIRE(low byte)
      scsp.mcipd &= ~(u32)d;
      return;
    }
}

static void
scsp_set_w (u32 a, u16 d)
{
  if ((a != 0x418) && (a != 0x41A) && (a != 0x422))
    {
      SCSPLOG("scsp : reg %.2X = %.4X\n", a & 0x3E, d);
    }

  *(u16 *)&scsp_ccr[a ^ 2] = d;

  switch (a & 0x3E)
    {
    case 0x00: // MEM4MB/DAC18B/VER/MVOL
      scsp.mem4b = (d >> 9) & 0x1;
      scsp.mvol = d & 0xF;
      if (scsp.mem4b)
        {
          M68K->SetFetch(0x000000, 0x080000, (pointer)SoundRam);
        }
      else
        {
          M68K->SetFetch(0x000000, 0x040000, (pointer)SoundRam);
          M68K->SetFetch(0x040000, 0x080000, (pointer)SoundRam);
          M68K->SetFetch(0x080000, 0x0C0000, (pointer)SoundRam);
          M68K->SetFetch(0x0C0000, 0x100000, (pointer)SoundRam);
        }
      return;

    case 0x02: // RBL/RBP
      scsp.rbl = (d >> 7) & 3;
      scsp.rbp = (d & 0x7F) * (4 * 1024 * 2);
      return;

    case 0x06: // MOBUF
      scsp_midi_out_send(d & 0xFF);
      return;

    case 0x08: // MSLC
      scsp.mslc = (d >> 11) & 0x1F;
      scsp_update_monitor();
      return;

    case 0x12: // DMEAL
      scsp.dmea = (scsp.dmea & 0x70000) + (d & 0xFFFE);
      return;

    case 0x14: // DMEAH/DRGA
      scsp.dmea = (scsp.dmea & 0xFFFE) + ((d & 0x7000) << 4);
      scsp.drga = d & 0xFFE;
      return;

    case 0x16: // DGATE/DDIR/DEXE/DTLG
      scsp.dmlen = d & 0xFFE;
      if ((scsp.dmfl = ((d >> 8) & 0xF0)) & 0x10) scsp_dma ();
      return;

    case 0x18: // TACTL/TIMA
      scsp.timasd = (d >> 8) & 7;
      scsp.timacnt = (d & 0xFF) << 8;
      return;

    case 0x1A: // TBCTL/TIMB
      scsp.timbsd = (d >> 8) & 7;
      scsp.timbcnt = (d & 0xFF) << 8;
      return;

    case 0x1C: // TCCTL/TIMC
      scsp.timcsd = (d >> 8) & 7;
      scsp.timccnt = (d & 0xFF) << 8;
      return;

    case 0x1E: // SCIEB
    {
      int i;
      scsp.scieb = d;
      for (i = 0; i < 11; i++)
        {
          if (scsp.scieb & (1 << i) && scsp.scipd & (1 << i))
            scsp_trigger_sound_interrupt ((1 << i));
        }
      return;
    }
    case 0x20: // SCIPD
      if (d & 0x20) scsp_sound_interrupt (0x20);
      return;

    case 0x22: // SCIRE
      scsp.scipd &= ~d;
      return;

    case 0x24: // SCILV0
      scsp.scilv0 = d;
      return;

    case 0x26: // SCILV1
      scsp.scilv1 = d;
      return;

    case 0x28: // SCILV2
      scsp.scilv2 = d;
      return;

    case 0x2A: // MCIEB
    {
      int i;
      scsp.mcieb = d;
      for (i = 0; i < 11; i++)
        {
          if (scsp.mcieb & (1 << i) && scsp.mcipd & (1 << i))
            scsp_trigger_main_interrupt ((1 << i));
        }
      return;
    }

    case 0x2C: // MCIPD
      if (d & 0x20) scsp_main_interrupt (0x20);
      return;

    case 0x2E: // MCIRE
      scsp.mcipd &= ~d;
      return;
    }
}

static u8
scsp_get_b (u32 a)
{
  a &= 0x3F;

  if ((a != 0x09) && (a != 0x21))
    {
      SCSPLOG("r_b scsp : reg %.2X\n", a);
    }
//  if (a == 0x09) SCSPLOG("r_b scsp 09 = %.2X\n", ((scsp.slot[scsp.mslc].fcnt >> (SCSP_FREQ_LB + 12)) & 0x1) << 7);

  switch (a)
    {
    case 0x01: // VER/MVOL
      scsp_ccr[a ^ 3] &= 0x0F;
      break;

    case 0x04: // Midi flags register
      return scsp.midflag;

    case 0x05: // MIBUF
      return scsp_midi_in_read();

    case 0x07: // MOBUF
      return scsp_midi_out_read();

    case 0x08: // CA(highest 3 bits)
      return (scsp.ca >> 8);

    case 0x09: // CA(lowest bit)/SGC/EG
      return (scsp.ca & 0xE0) | (scsp.sgc << 5) | scsp.eg;

    case 0x1E: // SCIEB(high byte)
      return (scsp.scieb >> 8);

    case 0x1F: // SCIEB(low byte)
      return scsp.scieb;

    case 0x20: // SCIPD(high byte)
      return (scsp.scipd >> 8);

    case 0x21: // SCIPD(low byte)
      return scsp.scipd;

    case 0x2C: // MCIPD(high byte)
      return (scsp.mcipd >> 8);

    case 0x2D: // MCIPD(low byte)
      return scsp.mcipd;
    }

  return scsp_ccr[a ^ 3];
}

static u16
scsp_get_w (u32 a)
{
  a &= 0x3E;

  if ((a != 0x20) && (a != 0x08))
    {
      SCSPLOG("r_w scsp : reg %.2X\n", a * 2);
    }

  switch (a)
    {
    case 0x00: // MEM4MB/DAC18B/VER/MVOL
      *(u16 *)&scsp_ccr[a ^ 2] &= 0xFF0F;
      break;

    case 0x04: // Midi flags/MIBUF
      return (scsp.midflag << 8) | scsp_midi_in_read();

    case 0x06: // MOBUF
      return scsp_midi_out_read();

    case 0x08: // CA/SGC/EG
      return (scsp.ca & 0x780) | (scsp.sgc << 5) | scsp.eg;

    case 0x18: // TACTL
      return (scsp.timasd << 8);

    case 0x1A: // TBCTL
      return (scsp.timbsd << 8);

    case 0x1C: // TCCTL
      return (scsp.timcsd << 8);

    case 0x1E: // SCIEB
      return scsp.scieb;

    case 0x20: // SCIPD
      return scsp.scipd;

    case 0x2C: // MCIPD
      return scsp.mcipd;
    }

  return *(u16 *)&scsp_ccr[a ^ 2];
}

///////////////////////////////////////////////////////////////
// Access

void FASTCALL
scsp_w_b (u32 a, u8 d)
{
    a &= 0xFFF;

    if (a < 0x400)
    {
        if (use_new_scsp)
            scsp_slot_write_byte(&new_scsp, a, d);
        else
            scsp_slot_set_b(a >> 5, a, d);
        FLUSH_SCSP ();
        return;
    }
    else if (a < 0x600)
    {
        if (a < 0x440)
        {
            scsp_set_b (a, d);
            FLUSH_SCSP ();
            return;
        }
    }
    else if (a < 0x700)
    {

    }
    else if (a < 0xee4)
    {
        a &= 0x3ff;
        scsp_dcr[a ^ 3] = d;
        return;
    }

    SCSPLOG("WARNING: scsp w_b to %08lx w/ %02x\n", a, d);
}

//////////////////////////////////////////////////////////////////////////////

#ifndef scsp_w_b
void FASTCALL
ScspWriteByte (u32 addr, u8 val)
{
    scsp_w_b(addr, val);
}
#endif

////////////////////////////////////////////////////////////////

void FASTCALL
scsp_w_w (u32 a, u16 d)
{
    if (a & 1)
    {
        SCSPLOG ("ERROR: scsp w_w misaligned : %.8X\n", a);
    }

    a &= 0xFFE;

    if (a < 0x400)
    {
        if (use_new_scsp)
            scsp_slot_write_word(&new_scsp, a, d);
        else
            scsp_slot_set_w(a >> 5, a, d);
        FLUSH_SCSP ();
        return;
    }
    else if (a < 0x600)
    {
        if (a < 0x440)
        {
            scsp_set_w (a, d);
            FLUSH_SCSP ();
            return;
        }
    }
    else if (a < 0x700)
    {

    }
    else if (a >= 0x700 && a < 0x780)
    {
        u32 address = (a - 0x700) / 2;
        dsp_inf.set_coef(d >> 3, address);//lower 3 bits seem to be discarded
    }
    else if (a >= 0x780 && a < 0x7A0)
    {
        u32 address = (a - 0x780) / 2;
        dsp_inf.set_madrs(d, address);
    }
    else if (a >= 0x7A0 && a < 0x7C0)
    {
        //madrs mirror
        //seems to read only (todo test)
    }
    else if (a >= 0x800 && a < 0xC00)
    {
        u32 address = (a - 0x800) / 8;
        u64 current_val = dsp_inf.get_mpro(address);
#ifdef HAVE_PLAY_JIT
        scsp_dsp_jit_need_recompile();
#endif

        switch (a & 0xf)
        {
            case 0:
            case 8:
                current_val = (current_val & 0x0000ffffffffffff) | (u64)d << (u64)48;
                break;
            case 2:
            case 0xa:
                current_val = (current_val & 0xffff0000ffffffff) | (u64)d << (u64)32;
                break;
            case 4:
            case 0xc:
                current_val = (current_val  & 0xffffffff0000ffff) | (u64)d << (u64)16;
                break;
            case 6:
            case 0xe:
                current_val = (current_val & 0xffffffffffff0000) | d;
                break;
            default:
                break;
        }

        dsp_inf.set_mpro(current_val, address);
    }
    else if (a < 0xee4)
    {
        a &= 0x3ff;
        *(u16 *)&scsp_dcr[a ^ 2] = d;
        return;
    }

    SCSPLOG ("WARNING: scsp w_w to %08lx w/ %04x\n", a, d);
}

//////////////////////////////////////////////////////////////////////////////

#ifndef scsp_w_w
void FASTCALL
ScspWriteWord (u32 addr, u16 val)
{
    scsp_w_w(addr, val);
}
#endif

////////////////////////////////////////////////////////////////

void FASTCALL
scsp_w_d (u32 a, u32 d)
{
    if (a & 3)
    {
        SCSPLOG ("ERROR: scsp w_d misaligned : %.8X\n", a);
    }

    a &= 0xFFC;

    if (a < 0x400)
    {
        if (use_new_scsp)
        {
            scsp_slot_write_word(&new_scsp, a + 0, d >> 16);
            scsp_slot_write_word(&new_scsp, a + 2, d & 0xffff);
        }
        else
        {
            scsp_slot_set_w(a >> 5, a + 0, d >> 16);
            scsp_slot_set_w(a >> 5, a + 2, d & 0xFFFF);
        }
        FLUSH_SCSP ();
        return;
    }
    else if (a < 0x600)
    {
        if (a < 0x440)
        {
            scsp_set_w (a + 0, d >> 16);
            scsp_set_w (a + 2, d & 0xFFFF);
            FLUSH_SCSP ();
            return;
        }
    }
    else if (a < 0x700)
    {

    }
    else if (a < 0xee4)
    {
        a &= 0x3ff;
        *(u32 *)&scsp_dcr[a] = d;
        return;
    }

    SCSPLOG ("WARNING: scsp w_d to %08lx w/ %08lx\n", a, d);
}

//////////////////////////////////////////////////////////////////////////////

#ifndef scsp_w_d
void FASTCALL
ScspWriteLong (u32 addr, u32 val)
{
    scsp_w_d(addr, val);
}
#endif

////////////////////////////////////////////////////////////////

u8 FASTCALL
scsp_r_b (u32 a)
{
    a &= 0xFFF;

    if (a < 0x400)
    {
        if (use_new_scsp)
            return scsp_slot_read_byte(&new_scsp, a);
        else
            return scsp_slot_get_b(a >> 5, a);
    }
    else if (a < 0x600)
    {
        if (a < 0x440) return scsp_get_b(a);
    }
    else if (a < 0x700)
    {

    }
    else if (a < 0xee4)
    {

    }

    SCSPLOG("WARNING: scsp r_b to %08lx\n", a);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

#ifndef scsp_r_b
u8 FASTCALL
ScspReadByte (u32 addr)
{
    return scsp_r_b(addr);
}
#endif

////////////////////////////////////////////////////////////////

u16 FASTCALL
scsp_r_w (u32 a)
{
    if (a & 1)
    {
        SCSPLOG ("ERROR: scsp r_w misaligned : %.8X\n", a);
    }

    a &= 0xFFE;

    if (a < 0x400)
    {
        if (use_new_scsp)
            return scsp_slot_read_word(&new_scsp, a);
        else
            return scsp_slot_get_w(a >> 5, a);
    }
    else if (a < 0x600)
    {
        if (a < 0x440) return scsp_get_w (a);
    }
    else if (a < 0x700)
    {
        if (use_new_scsp)
        {
            u32 addr = a - 0x600;
            return new_scsp.sound_stack[(addr / 2) & 0x3f];
        }
    }
    else if (a >= 0x700 && a < 0x780)
    {
        u32 address = (a - 0x700) / 2;
        return dsp_inf.get_coef(address) << 3;
    }
    else if (a >= 0x780 && a < 0x7A0)
    {
        u32 address = (a - 0x780) / 2;
        return dsp_inf.get_madrs(address);
    }
    else if (a >= 0x7A0 && a < 0x7C0)
    {
        //madrs mirror
        u32 address = (a - 0x7A0) / 2;
        return dsp_inf.get_madrs(address);
    }
    else if (a >= 0x800 && a < 0xC00)
    {
        u32 address = (a - 0x800) / 8;
        u64 value = dsp_inf.get_mpro(address);
        switch (a & 0xf)
        {
            case 0:
            case 8:
                return (value >> (u64)48) & 0xffff;
                break;
            case 2:
            case 0xa:
                return (value >> (u64)32) & 0xffff;
                break;
            case 4:
            case 0xc:
                return (value >> (u64)16) & 0xffff;
                break;
            case 6:
            case 0xe:
                return value & 0xffff;
                break;
            default:
                break;
        }
    }
    else if (a >= 0xE00 && a <= 0xE7F)
    {
        u32 address = (a - 0xE00) / 2;
        return dsp_inf.get_mems(address);
    }
    else if (a >= 0xEE0 && a <= 0xEE3)
    {
        u32 address = (a - 0xEE0) / 2;
        return dsp_inf.get_exts(address);
    }
    else if (a < 0xee4)
    {

    }

    SCSPLOG ("WARNING: scsp r_w to %08lx\n", a);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

#ifndef scsp_r_w
u16 FASTCALL
ScspReadWord (u32 addr)
{
    return scsp_r_w(addr);
}
#endif

////////////////////////////////////////////////////////////////

u32 FASTCALL
scsp_r_d (u32 a)
{
    if (a & 3)
    {
        SCSPLOG ("ERROR: scsp r_d misaligned : %.8X\n", a);
    }

    a &= 0xFFC;

    if (a < 0x400)
    {
        if (use_new_scsp)
            return (scsp_slot_read_word(&new_scsp, a + 0) << 16) | scsp_slot_read_word(&new_scsp, a + 2);
        else
            return (scsp_slot_get_w(a >> 5, a + 0) << 16) | scsp_slot_get_w(a >> 5, a + 2);
    }
    else if (a < 0x600)
    {
        if (a < 0x440) return (scsp_get_w (a + 0) << 16) | scsp_get_w (a + 2);
    }
    else if (a < 0x700)
    {

    }
    else if (a < 0xee4)
    {

    }

    SCSPLOG("WARNING: scsp r_d to %08lx\n", a);

    return 0;
}

//////////////////////////////////////////////////////////////////////////////

#ifndef scsp_r_d
u32 FASTCALL
ScspReadLong (u32 addr)
{
    return scsp_r_d(addr);
}
#endif

/////////////////////////////////////////////////////////////////////////////

u32 FASTCALL
c68k_byte_read (const u32 adr)
{
    if (adr < 0x100000)
        return T2ReadByte(SoundRam, adr & 0x7FFFF);
    else
        return scsp_r_b(adr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL
c68k_byte_write (const u32 adr, u32 data)
{
    if (adr < 0x100000)
        T2WriteByte(SoundRam, adr & 0x7FFFF, data);
    else
        scsp_w_b(adr, data);
}

//////////////////////////////////////////////////////////////////////////////

/* exported to m68kd.c */
u32 FASTCALL
c68k_word_read (const u32 adr)
{
    if (adr < 0x100000)
        return T2ReadWord(SoundRam, adr & 0x7FFFF);
    else
        return scsp_r_w(adr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL
c68k_word_write (const u32 adr, u32 data)
{
    if (adr < 0x100000)
        T2WriteWord (SoundRam, adr & 0x7FFFF, data);
    else
        scsp_w_w (adr, data);
}

//////////////////////////////////////////////////////////////////////////////

static void
c68k_interrupt_handler (u32 level)
{
    // send interrupt to 68k
    M68K->SetIRQ ((s32)level);
}

//waveform dram read
void op3(struct Slot * slot)
{
   u32 addr = (slot->state.address_pointer) & 0x7FFFF;

   if (slot->state.attenuation > 0x3bf)
      return;

   if (!slot->regs.pcm8b)
      slot->state.wave = c68k_word_read(addr);
   else
      slot->state.wave = c68k_byte_read(addr) << 8;

   slot->state.output = slot->state.wave;
}

//////////////////////////////////////////////////////////////////////////////

static void
scu_interrupt_handler (void)
{
    // send interrupt to scu
    ScuSendSoundRequest ();
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL
SoundRamReadByte (u32 addr)
{
    addr &= 0xFFFFF;

    // If mem4b is set, mirror ram every 256k
    if (scsp.mem4b == 0)
        addr &= 0x3FFFF;
    else if (addr > 0x7FFFF)
        return 0xFF;

    return T2ReadByte (SoundRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL
SoundRamWriteByte (u32 addr, u8 val)
{
    addr &= 0xFFFFF;

    // If mem4b is set, mirror ram every 256k
    if (scsp.mem4b == 0)
        addr &= 0x3FFFF;
    else if (addr > 0x7FFFF)
        return;

    T2WriteByte (SoundRam, addr, val);
    M68K->WriteNotify (addr, 1);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL
SoundRamReadWord (u32 addr)
{
    addr &= 0xFFFFF;

    if (scsp.mem4b == 0)
        addr &= 0x3FFFF;
    else if (addr > 0x7FFFF)
        return 0xFFFF;

    return T2ReadWord (SoundRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL
SoundRamWriteWord (u32 addr, u16 val)
{
    addr &= 0xFFFFF;

    // If mem4b is set, mirror ram every 256k
    if (scsp.mem4b == 0)
        addr &= 0x3FFFF;
    else if (addr > 0x7FFFF)
        return;

    T2WriteWord (SoundRam, addr, val);
    M68K->WriteNotify (addr, 2);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL
SoundRamReadLong (u32 addr)
{
    addr &= 0xFFFFF;

    // If mem4b is set, mirror ram every 256k
    if (scsp.mem4b == 0)
        addr &= 0x3FFFF;
    else if (addr > 0x7FFFF)
        return 0xFFFFFFFF;

    return T2ReadLong (SoundRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL
SoundRamWriteLong (u32 addr, u32 val)
{
    addr &= 0xFFFFF;

    // If mem4b is set, mirror ram every 256k
    if (scsp.mem4b == 0)
        addr &= 0x3FFFF;
    else if (addr > 0x7FFFF)
        return;

    T2WriteLong (SoundRam, addr, val);
    M68K->WriteNotify (addr, 4);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL
Sh2ScspReadByte(SH2_struct *sh, u32 addr)
{
    return ScspReadByte(addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL
Sh2ScspWriteByte(SH2_struct *sh, u32 addr, u8 val)
{
    ScspWriteByte(addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL
Sh2ScspReadWord(SH2_struct *sh, u32 addr)
{
    return ScspReadWord(addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL
Sh2ScspWriteWord(SH2_struct *sh, u32 addr, u16 val)
{
    ScspWriteWord(addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL
Sh2ScspReadLong(SH2_struct *sh, u32 addr)
{
    return ScspReadLong(addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL
Sh2ScspWriteLong(SH2_struct *sh, u32 addr, u32 val)
{
    ScspWriteLong(addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL
Sh2SoundRamReadByte(SH2_struct *sh, u32 addr)
{
    return SoundRamReadByte(addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL
Sh2SoundRamWriteByte(SH2_struct *sh, u32 addr, u8 val)
{
    SoundRamWriteByte(addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL
Sh2SoundRamReadWord(SH2_struct *sh, u32 addr)
{
    return SoundRamReadWord(addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL
Sh2SoundRamWriteWord(SH2_struct *sh, u32 addr, u16 val)
{
    SoundRamWriteWord(addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL
Sh2SoundRamReadLong(SH2_struct *sh, u32 addr)
{
    return SoundRamReadLong(addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL
Sh2SoundRamWriteLong(SH2_struct *sh, u32 addr, u32 val)
{
    SoundRamWriteLong(addr, val);
}

//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// Yabause specific
//////////////////////////////////////////////////////////////////////////////

u8 *SoundRam = NULL;
ScspInternal *ScspInternalVars;
static SoundInterface_struct *SNDCore = NULL;
extern SoundInterface_struct *SNDCoreList[];

struct sounddata
{
  u32 *data32;
} scspchannel[2];

static u32 scspsoundlen;        // Samples to output per frame
static u32 scsplines;           // Lines per frame
static u32 scspsoundbufs;       // Number of "scspsoundlen"-sample buffers
static u32 scspsoundbufsize;    // scspsoundlen * scspsoundbufs
static u32 scspsoundgenpos;     // Offset of next byte to generate
static u32 scspsoundoutleft;    // Samples not yet sent to host driver

static int
scsp_alloc_bufs (void)
{
  if (scspchannel[0].data32)
    free(scspchannel[0].data32);
  scspchannel[0].data32 = NULL;
  if (scspchannel[1].data32)
    free(scspchannel[1].data32);
  scspchannel[1].data32 = NULL;

  scspchannel[0].data32 = (u32 *)calloc(scspsoundbufsize, sizeof(u32));
  if (scspchannel[0].data32 == NULL)
    return -1;
  scspchannel[1].data32 = (u32 *)calloc(scspsoundbufsize, sizeof(u32));
  if (scspchannel[1].data32 == NULL)
    return -1;

  return 0;
}

static u8 IsM68KRunning;
static s32 FASTCALL (*m68kexecptr)(s32 cycles);  // M68K->Exec or M68KExecBP
static s32 savedcycles;  // Cycles left over from the last M68KExec() call


#ifndef USE_SCSP2
int
ScspInit (int coreid)
{
  int i;

  if ((SoundRam = T2MemoryInit (0x80000)) == NULL)
    return -1;

  if ((ScspInternalVars = (ScspInternal *)calloc(1, sizeof(ScspInternal))) == NULL)
    return -1;

  if (M68K->Init () != 0)
    return -1;

  M68K->SetReadB ((C68K_READ *)c68k_byte_read);
  M68K->SetReadW ((C68K_READ *)c68k_word_read);
  M68K->SetWriteB ((C68K_WRITE *)c68k_byte_write);
  M68K->SetWriteW ((C68K_WRITE *)c68k_word_write);

  M68K->SetFetch (0x000000, 0x040000, (pointer)SoundRam);
  M68K->SetFetch (0x040000, 0x080000, (pointer)SoundRam);
  M68K->SetFetch (0x080000, 0x0C0000, (pointer)SoundRam);
  M68K->SetFetch (0x0C0000, 0x100000, (pointer)SoundRam);

  IsM68KRunning = 0;

  scsp_init (SoundRam, &c68k_interrupt_handler, &scu_interrupt_handler);
  ScspInternalVars->scsptiming1 = 0;
  ScspInternalVars->scsptiming2 = 0;

  for (i = 0; i < MAX_BREAKPOINTS; i++)
    ScspInternalVars->codebreakpoint[i].addr = 0xFFFFFFFF;
  ScspInternalVars->numcodebreakpoints = 0;
  ScspInternalVars->BreakpointCallBack = NULL;
  ScspInternalVars->inbreakpoint = 0;

  m68kexecptr = M68K->Exec;

  // Allocate enough memory for each channel buffer(may have to change)
  scspsoundlen = 44100 / 60; // assume it's NTSC timing
  scsplines = 263;
  scspsoundbufs = 10; // should be enough to prevent skipping
  scspsoundbufsize = scspsoundlen * scspsoundbufs;
  if (scsp_alloc_bufs () < 0)
    return -1;

  // Reset output pointers
  scspsoundgenpos = 0;
  scspsoundoutleft = 0;

  return ScspChangeSoundCore (coreid);
}
#endif

//////////////////////////////////////////////////////////////////////////////

int
ScspChangeSoundCore (int coreid)
{
  int i;

  // Make sure the old core is freed
  if (SNDCore)
    SNDCore->DeInit();

  // So which core do we want?
  if (coreid == SNDCORE_DEFAULT)
    coreid = 0; // Assume we want the first one

  // Go through core list and find the id
  for (i = 0; SNDCoreList[i] != NULL; i++)
    {
      if (SNDCoreList[i]->id == coreid)
        {
          // Set to current core
          SNDCore = SNDCoreList[i];
          break;
        }
    }

  if (SNDCore == NULL)
    {
      SNDCore = &SNDDummy;
      return -1;
    }

  if (SNDCore->Init () == -1)
    {
      // Since it failed, instead of it being fatal, we'll just use the dummy
      // core instead

      // This might be helpful though.
      YabSetError (YAB_ERR_CANNOTINIT, (void *)SNDCore->Name);

      SNDCore = &SNDDummy;
    }

  if (SNDCore)
    {
      if (scsp_mute_flags) SNDCore->MuteAudio();
      else SNDCore->UnMuteAudio();
      SNDCore->SetVolume(scsp_volume);
    }

  return 0;
}

//////////////////////////////////////////////////////////////////////////////

void
ScspDeInit (void)
{
  if (scspchannel[0].data32)
    free(scspchannel[0].data32);
  scspchannel[0].data32 = NULL;

  if (scspchannel[1].data32)
    free(scspchannel[1].data32);
  scspchannel[1].data32 = NULL;

  if (SNDCore)
    SNDCore->DeInit();
  SNDCore = NULL;

  scsp_shutdown();

  if (SoundRam)
    T2MemoryDeInit (SoundRam);
  SoundRam = NULL;
}

//////////////////////////////////////////////////////////////////////////////

void
M68KStart (void)
{
  M68K->Reset ();
  savedcycles = 0;
  IsM68KRunning = 1;
}

//////////////////////////////////////////////////////////////////////////////

void
M68KStop (void)
{
  IsM68KRunning = 0;
}

//////////////////////////////////////////////////////////////////////////////

void
ScspReset (void)
{
  scsp_reset();
}

//////////////////////////////////////////////////////////////////////////////

int
ScspChangeVideoFormat (int type)
{
  scspsoundlen = 44100 / (type ? 50 : 60);
  scsplines = type ? 313 : 263;
  scspsoundbufsize = scspsoundlen * scspsoundbufs;

  if (scsp_alloc_bufs () < 0)
    return -1;

  SNDCore->ChangeVideoFormat (type ? 50 : 60);

  return 0;
}

//////////////////////////////////////////////////////////////////////////////

/* Process breakpoints in a separate function to avoid unnecessary register
 * spillage on the fast path (and to avoid too much block nesting) */
#ifdef __GNUC__
__attribute__((noinline))
#endif
static s32 FASTCALL M68KExecBP (s32 cycles);

void
M68KExec (s32 cycles)
{
  s32 newcycles = savedcycles - cycles;
  if (LIKELY(IsM68KRunning))
    {
      if (LIKELY(newcycles < 0))
        {
          s32 cyclestoexec = -newcycles;
          newcycles += (*m68kexecptr)(cyclestoexec);
        }
      savedcycles = newcycles;
    }
}

void generate_sample(struct Scsp * s, int rbp, int rbl, s16 * out_l, s16* out_r, int mvol, s16 cd_in_l, s16 cd_in_r)
{
   int step_num = 0;
   int i = 0;
   int mvol_shift = 0;

   //run 32 steps to generate 1 full sample (512 clock cycles at 22579200hz)
   //7 operations happen simultaneously on different channels due to pipelining
   for (step_num = 0; step_num < 32; step_num++)
   {
      int last_step = (step_num - 6) & 0x1f;
      int debug_muted = 0;

      op1(&s->slots[step_num]);//phase, pitch lfo
      op2(&s->slots[(step_num - 1) & 0x1f],s);//address pointer, modulation data read
      op3(&s->slots[(step_num - 2) & 0x1f]);//waveform dram read
      op4(&s->slots[(step_num - 3) & 0x1f]);//interpolation, eg, amplitude lfo
      op5(&s->slots[(step_num - 4) & 0x1f]);//level calc 1
      op6(&s->slots[(step_num - 5) & 0x1f]);//level calc 2
      op7(&s->slots[(step_num - 6) & 0x1f],s);//sound stack write

      if (s->debug_mode)
      {
         if (scsp_debug_instrument_check_is_muted(s->slots[last_step].regs.sa))
            debug_muted = 1;
      }

      if (!debug_muted)
      {
         int disdl = get_sdl_shift(s->slots[last_step].regs.disdl);

         s16 disdl_applied = (s->slots[last_step].state.output >> disdl);

         s16 mixs_input = s->slots[last_step].state.output >>
            get_sdl_shift(s->slots[last_step].regs.imxl);

         int pan_val_l = 0, pan_val_r = 0;

         get_panning(s->slots[last_step].regs.dipan, &pan_val_l, &pan_val_r);

         *out_l = *out_l + ((disdl_applied >> pan_val_l) >> 2);
         *out_r = *out_r + ((disdl_applied >> pan_val_r) >> 2);

         dsp_inf.set_mixs(s->slots[last_step].regs.isel, mixs_input);
      }
   }

   dsp_inf.set_rbl_rbp(rbl, rbp);
   dsp_inf.set_exts(cd_in_l, cd_in_r);

   dsp_inf.exec();

   for (i = 0; i < 18; i++)//16,17 are exts0/1
   {
      int efsdl = get_sdl_shift(s->slots[i].regs.efsdl);
      s16 efsdl_applied = 0;

      int pan_val_l = 0, pan_val_r = 0;
      s16 panned_l = 0, panned_r = 0;

      efsdl_applied = dsp_inf.get_effect_out(i) >> efsdl;

      get_panning(s->slots[i].regs.efpan, &pan_val_l, &pan_val_r);

      panned_l = (efsdl_applied >> pan_val_l) >> 2;
      panned_r = (efsdl_applied >> pan_val_r) >> 2;

      *out_l = *out_l + panned_l;
      *out_r = *out_r + panned_r;
   }

   mvol_shift = 0xf - mvol;

   *out_l = *out_l >> mvol_shift;
   *out_r = *out_r >> mvol_shift;
}

void new_scsp_run_sample()
{
   s32 temp = cdda_next_in - cdda_out_left;
   s32 outpos = (temp < 0) ? temp + sizeof(cddabuf.data) : temp;
   u8 *buf = &cddabuf.data[outpos];

   s16 out_l = 0;
   s16 out_r = 0;

   s16 cd_in_l = 0;
   s16 cd_in_r = 0;

   if ((s32)cdda_out_left > 0)
   {
      cd_in_l = (s16)((buf[1] << 8) | buf[0]);
      cd_in_r = (s16)((buf[3] << 8) | buf[2]);

      cdda_out_left -= 4;
   }

   scsp_update_timer(1);
   generate_sample(&new_scsp, scsp.rbp, scsp.rbl, &out_l, &out_r, scsp.mvol, cd_in_l, cd_in_r);

   if (new_scsp_outbuf_pos < 900)
   {
      new_scsp_outbuf_l[new_scsp_outbuf_pos] = out_l;
      new_scsp_outbuf_r[new_scsp_outbuf_pos] = out_r;
   }
   else
   {
      //buffer overrun
   }

   scsp_update_monitor();
   new_scsp_outbuf_pos++;
}

void new_scsp_exec(s32 cycles)
{
   s32 cycles_temp = new_scsp_cycles - cycles;
   if (cycles_temp < 0)
   {
      new_scsp_run_sample();
      cycles_temp += 512;
   }
   new_scsp_cycles = cycles_temp;
}

//----------------------------------------------------------------------------

static s32 FASTCALL
M68KExecBP (s32 cycles)
{
  s32 cyclestoexec=cycles;
  s32 cyclesexecuted=0;
  int i;

  while (cyclesexecuted < cyclestoexec)
    {
      // Make sure it isn't one of our breakpoints
      for (i = 0; i < ScspInternalVars->numcodebreakpoints; i++)
        {
          if ((M68K->GetPC () == ScspInternalVars->codebreakpoint[i].addr) &&
              ScspInternalVars->inbreakpoint == 0)
            {
              ScspInternalVars->inbreakpoint = 1;
              if (ScspInternalVars->BreakpointCallBack)
                ScspInternalVars->BreakpointCallBack (ScspInternalVars->codebreakpoint[i].addr);
              ScspInternalVars->inbreakpoint = 0;
            }
        }

      // execute instructions individually
      cyclesexecuted += M68K->Exec(1);

    }
  return cyclesexecuted;
}

//////////////////////////////////////////////////////////////////////////////

void
M68KStep (void)
{
  M68K->Exec(1);
}

//////////////////////////////////////////////////////////////////////////////

// Wait for background execution to finish (used on PSP)
void
M68KSync (void)
{
  M68K->Sync();
}

//////////////////////////////////////////////////////////////////////////////

void
ScspConvert32uto16s (s32 *srcL, s32 *srcR, s16 *dst, u32 len)
{
  u32 i;

  for (i = 0; i < len; i++)
    {
      // Left Channel
      if (*srcL > 0x7FFF)
        *dst = 0x7FFF;
      else if (*srcL < -0x8000)
        *dst = -0x8000;
      else
        *dst = *srcL;

      srcL++;
      dst++;

      // Right Channel
      if (*srcR > 0x7FFF)
        *dst = 0x7FFF;
      else if (*srcR < -0x8000)
        *dst = -0x8000;
      else
        *dst = *srcR;

      srcR++;
      dst++;
    }
}

//////////////////////////////////////////////////////////////////////////////

void
ScspReceiveCDDA (const u8 *sector)
{
   // If buffer is half empty or less, boost timing for a bit until we've buffered a few sectors
   if (cdda_out_left < (sizeof(cddabuf.data) / 2))
   {
      Cs2Area->isaudio = 0;
      Cs2SetTiming(1);
		Cs2Area->isaudio = 1;
   }
	else if (cdda_out_left > (sizeof(cddabuf.data) * 3 / 4 ))
		Cs2SetTiming(0);
   else
   {
      Cs2Area->isaudio = 1;
      Cs2SetTiming(1);
   }

  memcpy(cddabuf.data+cdda_next_in, sector, 2352);
  if (sizeof(cddabuf.data)-cdda_next_in <= 2352)
     cdda_next_in = 0;
  else
     cdda_next_in += 2352;

  cdda_out_left += 2352;

  if (cdda_out_left > sizeof(cddabuf.data))
    {
      SCSPLOG ("WARNING: CDDA buffer overrun\n");
      cdda_out_left = sizeof(cddabuf.data);
    }
}

//////////////////////////////////////////////////////////////////////////////

void ScspReceiveMpeg (const u8 *samples, int len)
{
  memcpy(cddabuf.data+cdda_next_in, samples, len);

  if (sizeof(cddabuf.data)-cdda_next_in <= len)
     cdda_next_in = 0;
  else
     cdda_next_in += len;

  cdda_out_left += len;

  if (cdda_out_left > sizeof(cddabuf.data))
    {
      SCSPLOG ("WARNING: CDDA buffer overrun\n");
      cdda_out_left = sizeof(cddabuf.data);
    }
}

//////////////////////////////////////////////////////////////////////////////

void new_scsp_update_samples(s32 *bufL, s32 *bufR, int scspsoundlen)
{
   int i;
   for (i = 0; i < new_scsp_outbuf_pos; i++)
   {
      if (i >= scspsoundlen)
         break;

      bufL[i] = new_scsp_outbuf_l[i];
      bufR[i] = new_scsp_outbuf_r[i];
   }

   new_scsp_outbuf_pos = 0;
}

void
ScspExec ()
{
  u32 audiosize;

  ScspInternalVars->scsptiming2 +=
    ((scspsoundlen << 16) + scsplines / 2) / scsplines;
  if (!use_new_scsp)
     scsp_update_timer (ScspInternalVars->scsptiming2 >> 16); // Pass integer part
  ScspInternalVars->scsptiming2 &= 0xFFFF; // Keep fractional part
  ScspInternalVars->scsptiming1++;

  if (ScspInternalVars->scsptiming1 >= scsplines)
  {
     s32 *bufL, *bufR;

     ScspInternalVars->scsptiming1 -= scsplines;
     ScspInternalVars->scsptiming2 = 0;

     // Update sound buffers
     if (scspsoundgenpos + scspsoundlen > scspsoundbufsize)
        scspsoundgenpos = 0;

     if (scspsoundoutleft + scspsoundlen > scspsoundbufsize)
     {
        u32 overrun = (scspsoundoutleft + scspsoundlen) -
           scspsoundbufsize;
        SCSPLOG("WARNING: Sound buffer overrun, %lu samples\n",
           (long)overrun);
        scspsoundoutleft -= overrun;
     }

     bufL = (s32 *)&scspchannel[0].data32[scspsoundgenpos];
     bufR = (s32 *)&scspchannel[1].data32[scspsoundgenpos];
     memset(bufL, 0, sizeof(u32) * scspsoundlen);
     memset(bufR, 0, sizeof(u32) * scspsoundlen);
     if (use_new_scsp)
        new_scsp_update_samples(bufL, bufR, scspsoundlen);
     else
        scsp_update(bufL, bufR, scspsoundlen);
     scspsoundgenpos += scspsoundlen;
     scspsoundoutleft += scspsoundlen;
  }

  while (scspsoundoutleft > 0 &&
     (audiosize = SNDCore->GetAudioSpace()) > 0)
  {
     s32 outstart = (s32)scspsoundgenpos - (s32)scspsoundoutleft;

     if (outstart < 0)
        outstart += scspsoundbufsize;
     if (audiosize > scspsoundoutleft)
        audiosize = scspsoundoutleft;
     if (audiosize > scspsoundbufsize - outstart)
        audiosize = scspsoundbufsize - outstart;

     SNDCore->UpdateAudio(&scspchannel[0].data32[outstart],
        &scspchannel[1].data32[outstart], audiosize);
     scspsoundoutleft -= audiosize;

#if 0
     ScspConvert32uto16s(&scspchannel[0].data32[outstart],
        &scspchannel[1].data32[outstart],
        (s16 *)stereodata16, audiosize);
     DRV_AviSoundUpdate(stereodata16, audiosize);
#endif
  }

  if (!use_new_scsp)
     scsp_update_monitor();

#ifdef USE_SCSPMIDI
  // Process Midi ports
  while (scsp.midincnt < 4)
  {
     u8 data;
     int isdata;

     data = SNDCore->MidiIn(&isdata);
     if (!isdata)
        break;
     scsp_midi_in_send(data);
  }


  while (scsp.midoutcnt)
  {
     SNDCore->MidiOut(scsp_midi_out_read());
  }

#endif
}

//////////////////////////////////////////////////////////////////////////////

void
M68KWriteNotify (u32 address, u32 size)
{
  M68K->WriteNotify (address, size);
}

//////////////////////////////////////////////////////////////////////////////

void
M68KGetRegisters (m68kregs_struct *regs)
{
  int i;

  if (regs != NULL)
    {
      for (i = 0; i < 8; i++)
        {
          regs->D[i] = M68K->GetDReg (i);
          regs->A[i] = M68K->GetAReg (i);
        }

      regs->SR = M68K->GetSR ();
      regs->PC = M68K->GetPC ();
    }
}

//////////////////////////////////////////////////////////////////////////////
#ifndef USE_SCSP2
void
M68KSetRegisters (m68kregs_struct *regs)
{
  int i;

  if (regs != NULL)
    {
      for (i = 0; i < 8; i++)
        {
          M68K->SetDReg (i, regs->D[i]);
          M68K->SetAReg (i, regs->A[i]);
        }

      M68K->SetSR (regs->SR);
      M68K->SetPC (regs->PC);
    }
}
#endif
//////////////////////////////////////////////////////////////////////////////

void
ScspMuteAudio (int flags)
{
  scsp_mute_flags |= flags;
  if (SNDCore && scsp_mute_flags)
    SNDCore->MuteAudio ();
}

//////////////////////////////////////////////////////////////////////////////

void
ScspUnMuteAudio (int flags)
{
  scsp_mute_flags &= ~flags;
  if (SNDCore && (scsp_mute_flags == 0))
    SNDCore->UnMuteAudio ();
}

//////////////////////////////////////////////////////////////////////////////

void
ScspSetVolume (int volume)
{
  scsp_volume = volume;
  if (SNDCore)
    SNDCore->SetVolume (volume);
}

//////////////////////////////////////////////////////////////////////////////

void
M68KSetBreakpointCallBack (void (*func)(u32))
{
  ScspInternalVars->BreakpointCallBack = func;
}

//////////////////////////////////////////////////////////////////////////////

int
M68KAddCodeBreakpoint (u32 addr)
{
  int i;

  if (ScspInternalVars->numcodebreakpoints < MAX_BREAKPOINTS)
    {
      // Make sure it isn't already on the list
      for (i = 0; i < ScspInternalVars->numcodebreakpoints; i++)
        {
          if (addr == ScspInternalVars->codebreakpoint[i].addr)
            return -1;
        }

      ScspInternalVars->codebreakpoint[i].addr = addr;
      ScspInternalVars->numcodebreakpoints++;
      m68kexecptr = M68KExecBP;

      return 0;
    }

  return -1;
}

//////////////////////////////////////////////////////////////////////////////

void
M68KSortCodeBreakpoints (void)
{
  int i, i2;
  u32 tmp;

  for (i = 0; i < (MAX_BREAKPOINTS - 1); i++)
    {
      for (i2 = i+1; i2 < MAX_BREAKPOINTS; i2++)
        {
          if (ScspInternalVars->codebreakpoint[i].addr == 0xFFFFFFFF &&
              ScspInternalVars->codebreakpoint[i2].addr != 0xFFFFFFFF)
            {
              tmp = ScspInternalVars->codebreakpoint[i].addr;
              ScspInternalVars->codebreakpoint[i].addr =
                ScspInternalVars->codebreakpoint[i2].addr;
              ScspInternalVars->codebreakpoint[i2].addr = tmp;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

int
M68KDelCodeBreakpoint (u32 addr)
{
  int i;
  if (ScspInternalVars->numcodebreakpoints > 0)
    {
      for (i = 0; i < ScspInternalVars->numcodebreakpoints; i++)
        {
          if (ScspInternalVars->codebreakpoint[i].addr == addr)
            {
              ScspInternalVars->codebreakpoint[i].addr = 0xFFFFFFFF;
              M68KSortCodeBreakpoints ();
              ScspInternalVars->numcodebreakpoints--;
              if (ScspInternalVars->numcodebreakpoints == 0)
                m68kexecptr = M68K->Exec;
              return 0;
            }
        }
    }

  return -1;
}

//////////////////////////////////////////////////////////////////////////////

#ifndef USE_SCSP2
m68kcodebreakpoint_struct *
M68KGetBreakpointList ()
{
  return ScspInternalVars->codebreakpoint;
}
#endif
//////////////////////////////////////////////////////////////////////////////

void
M68KClearCodeBreakpoints ()
{
  int i;
  for (i = 0; i < MAX_BREAKPOINTS; i++)
    ScspInternalVars->codebreakpoint[i].addr = 0xFFFFFFFF;

  ScspInternalVars->numcodebreakpoints = 0;
}

//////////////////////////////////////////////////////////////////////////////

int
SoundSaveState (FILE *fp)
{
  int i;
  u32 temp;
  int offset;
  u8 nextphase;
  IOCheck_struct check = { 0, 0 };

  offset = StateWriteHeader (fp, "SCSP", 2);

  // Save 68k registers first
  ywrite (&check, (void *)&IsM68KRunning, 1, 1, fp);

#ifdef IMPROVED_SAVESTATES
  M68K->SaveState(fp);
#else
  for (i = 0; i < 8; i++)
    {
      temp = M68K->GetDReg (i);
      ywrite (&check, (void *)&temp, 4, 1, fp);
    }

  for (i = 0; i < 8; i++)
    {
      temp = M68K->GetAReg (i);
      ywrite (&check, (void *)&temp, 4, 1, fp);
    }

  temp = M68K->GetSR ();
  ywrite (&check, (void *)&temp, 4, 1, fp);
  temp = M68K->GetPC ();
  ywrite (&check, (void *)&temp, 4, 1, fp);
#endif

  // Now for the SCSP registers
  ywrite (&check, (void *)scsp_reg, 0x1000, 1, fp);

  // Sound RAM is important
  ywrite (&check, (void *)SoundRam, 0x80000, 1, fp);

  // Write slot internal variables
  for (i = 0; i < 32; i++)
    {
      s32 einc;

#ifdef IMPROVED_SAVESTATES
      ywrite(&check, (void *)&scsp.slot[i].swe, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].sdir, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].pcm8b, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].sbctl, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].ssctl, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].lpctl, sizeof(u8), 1, fp);
#endif
      ywrite (&check, (void *)&scsp.slot[i].key, 1, 1, fp);
#ifdef IMPROVED_SAVESTATES
      ywrite(&check, (void *)&scsp.slot[i].keyx, sizeof(u8), 1, fp);
#endif
      //buf8,16 get regenerated on state load

      ywrite (&check, (void *)&scsp.slot[i].fcnt, 4, 1, fp);
#ifdef IMPROVED_SAVESTATES
      ywrite(&check, (void *)&scsp.slot[i].finc, sizeof(u32), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].finct, sizeof(u32), 1, fp);
#endif
      ywrite (&check, (void *)&scsp.slot[i].ecnt, 4, 1, fp);

      if (scsp.slot[i].einc == &scsp.slot[i].einca)
        einc = 0;
      else if (scsp.slot[i].einc == &scsp.slot[i].eincd)
        einc = 1;
      else if (scsp.slot[i].einc == &scsp.slot[i].eincs)
        einc = 2;
      else if (scsp.slot[i].einc == &scsp.slot[i].eincr)
        einc = 3;
      else
        einc = 4;

      ywrite (&check, (void *)&einc, 4, 1, fp);

      //einca,eincd,eincs,eincr

      ywrite (&check, (void *)&scsp.slot[i].ecmp, 4, 1, fp);
      ywrite (&check, (void *)&scsp.slot[i].ecurp, 4, 1, fp);
#ifdef IMPROVED_SAVESTATES
      ywrite(&check, (void *)&scsp.slot[i].env, sizeof(s32), 1, fp);
#endif
      if (scsp.slot[i].enxt == scsp_env_null_next)
        nextphase = 0;
      else if (scsp.slot[i].enxt == scsp_release_next)
        nextphase = 1;
      else if (scsp.slot[i].enxt == scsp_sustain_next)
        nextphase = 2;
      else if (scsp.slot[i].enxt == scsp_decay_next)
        nextphase = 3;
      else if (scsp.slot[i].enxt == scsp_attack_next)
        nextphase = 4;
      ywrite (&check, (void *)&nextphase, 1, 1, fp);

      ywrite (&check, (void *)&scsp.slot[i].lfocnt, 4, 1, fp);
      ywrite (&check, (void *)&scsp.slot[i].lfoinc, 4, 1, fp);
#ifdef IMPROVED_SAVESTATES
      ywrite(&check, (void *)&scsp.slot[i].sa, sizeof(u32), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].lsa, sizeof(u32), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].lea , sizeof(u32), 1, fp);

      ywrite(&check, (void *)&scsp.slot[i].tl, sizeof(s32), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].sl, sizeof(s32), 1, fp);

      ywrite(&check, (void *)&scsp.slot[i].ar, sizeof(s32), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].dr, sizeof(s32), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].sr, sizeof(s32), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].rr, sizeof(s32), 1, fp);

      //arp
      //drp
      //srp
      //rrp

      ywrite(&check, (void *)&scsp.slot[i].krs, sizeof(u32), 1, fp);

      //lfofmw
      //lfoemw

      ywrite(&check, (void *)&scsp.slot[i].lfofms, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].lfoems, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].fsft, sizeof(u8), 1, fp);

      ywrite(&check, (void *)&scsp.slot[i].mdl, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].mdx, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].mdy, sizeof(u8), 1, fp);

      ywrite(&check, (void *)&scsp.slot[i].imxl, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].disll, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].dislr, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].efsll, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].efslr, sizeof(u8), 1, fp);

      ywrite(&check, (void *)&scsp.slot[i].eghold, sizeof(u8), 1, fp);
      ywrite(&check, (void *)&scsp.slot[i].lslnk, sizeof(u8), 1, fp);
#endif
    }

  // Write main internal variables
  ywrite (&check, (void *)&scsp.mem4b, 4, 1, fp);
  ywrite (&check, (void *)&scsp.mvol, 4, 1, fp);

  ywrite (&check, (void *)&scsp.rbl, 4, 1, fp);
  ywrite (&check, (void *)&scsp.rbp, 4, 1, fp);

  ywrite (&check, (void *)&scsp.mslc, 4, 1, fp);

  ywrite (&check, (void *)&scsp.dmea, 4, 1, fp);
  ywrite (&check, (void *)&scsp.drga, 4, 1, fp);
  ywrite (&check, (void *)&scsp.dmfl, 4, 1, fp);
  ywrite (&check, (void *)&scsp.dmlen, 4, 1, fp);

  ywrite (&check, (void *)scsp.midinbuf, 1, 4, fp);
  ywrite (&check, (void *)scsp.midoutbuf, 1, 4, fp);
  ywrite (&check, (void *)&scsp.midincnt, 1, 1, fp);
  ywrite (&check, (void *)&scsp.midoutcnt, 1, 1, fp);
  ywrite (&check, (void *)&scsp.midflag, 1, 1, fp);

  ywrite (&check, (void *)&scsp.timacnt, 4, 1, fp);
  ywrite (&check, (void *)&scsp.timasd, 4, 1, fp);
  ywrite (&check, (void *)&scsp.timbcnt, 4, 1, fp);
  ywrite (&check, (void *)&scsp.timbsd, 4, 1, fp);
  ywrite (&check, (void *)&scsp.timccnt, 4, 1, fp);
  ywrite (&check, (void *)&scsp.timcsd, 4, 1, fp);

  ywrite (&check, (void *)&scsp.scieb, 4, 1, fp);
  ywrite (&check, (void *)&scsp.scipd, 4, 1, fp);
  ywrite (&check, (void *)&scsp.scilv0, 4, 1, fp);
  ywrite (&check, (void *)&scsp.scilv1, 4, 1, fp);
  ywrite (&check, (void *)&scsp.scilv2, 4, 1, fp);
  ywrite (&check, (void *)&scsp.mcieb, 4, 1, fp);
  ywrite (&check, (void *)&scsp.mcipd, 4, 1, fp);

  ywrite (&check, (void *)scsp.stack, 4, 32 * 2, fp);

  return StateFinishHeader (fp, offset);
}

//////////////////////////////////////////////////////////////////////////////

int
SoundLoadState (FILE *fp, int version, int size)
{
  int i, i2;
  u32 temp;
  u8 nextphase;
  IOCheck_struct check = { 0, 0 };

  // Read 68k registers first
  yread (&check, (void *)&IsM68KRunning, 1, 1, fp);

#ifdef IMPROVED_SAVESTATES
  M68K->LoadState(fp);
#else
  for (i = 0; i < 8; i++)
    {
      yread (&check, (void *)&temp, 4, 1, fp);
      M68K->SetDReg (i, temp);
    }

  for (i = 0; i < 8; i++)
    {
      yread (&check, (void *)&temp, 4, 1, fp);
      M68K->SetAReg (i, temp);
    }

  yread (&check, (void *)&temp, 4, 1, fp);
  M68K->SetSR (temp);
  yread (&check, (void *)&temp, 4, 1, fp);
  M68K->SetPC (temp);
#endif

  // Now for the SCSP registers
  yread (&check, (void *)scsp_reg, 0x1000, 1, fp);

  // Lastly, sound ram
  yread (&check, (void *)SoundRam, 0x80000, 1, fp);

  if (version > 1)
    {
      // Internal variables need to be regenerated
      for(i = 0; i < 32; i++)
        {
          for (i2 = 0; i2 < 0x20; i2+=2)
            scsp_slot_set_w (i, 0x1E - i2, scsp_slot_get_w (i, 0x1E - i2));
        }

      scsp_set_w (0x402, scsp_get_w (0x402));

      // Read slot internal variables
      for (i = 0; i < 32; i++)
        {
          s32 einc;
#ifdef IMPROVED_SAVESTATES
          yread(&check, (void *)&scsp.slot[i].swe, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].sdir, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].pcm8b, sizeof(u8), 1, fp);

          yread(&check, (void *)&scsp.slot[i].sbctl, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].ssctl, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].lpctl, sizeof(u8), 1, fp);
#endif
          yread (&check, (void *)&scsp.slot[i].key, 1, 1, fp);
#ifdef IMPROVED_SAVESTATES
          yread(&check, (void *)&scsp.slot[i].keyx, sizeof(u8), 1, fp);
#endif
          //buf8,16 regenerated at end

          yread (&check, (void *)&scsp.slot[i].fcnt, 4, 1, fp);
#ifdef IMPROVED_SAVESTATES
          yread(&check, (void *)&scsp.slot[i].finc, sizeof(u32), 1, fp);
          yread(&check, (void *)&scsp.slot[i].finct, sizeof(u32), 1, fp);
#endif
          yread (&check, (void *)&scsp.slot[i].ecnt, 4, 1, fp);

          yread (&check, (void *)&einc, 4, 1, fp);
          switch (einc)
            {
            case 0:
              scsp.slot[i].einc = &scsp.slot[i].einca;
              break;
            case 1:
              scsp.slot[i].einc = &scsp.slot[i].eincd;
              break;
            case 2:
              scsp.slot[i].einc = &scsp.slot[i].eincs;
              break;
            case 3:
              scsp.slot[i].einc = &scsp.slot[i].eincr;
              break;
            default:
              scsp.slot[i].einc = NULL;
              break;
            }

          //einca,eincd,eincs,eincr

          yread (&check, (void *)&scsp.slot[i].ecmp, 4, 1, fp);
          yread (&check, (void *)&scsp.slot[i].ecurp, 4, 1, fp);
#ifdef IMPROVED_SAVESTATES
          yread(&check, (void *)&scsp.slot[i].env, sizeof(s32), 1, fp);
#endif
          yread (&check, (void *)&nextphase, 1, 1, fp);
          switch (nextphase)
            {
            case 0:
              scsp.slot[i].enxt = scsp_env_null_next;
              break;
            case 1:
              scsp.slot[i].enxt = scsp_release_next;
              break;
            case 2:
              scsp.slot[i].enxt = scsp_sustain_next;
              break;
            case 3:
              scsp.slot[i].enxt = scsp_decay_next;
              break;
            case 4:
              scsp.slot[i].enxt = scsp_attack_next;
              break;
            default: break;
            }

          yread (&check, (void *)&scsp.slot[i].lfocnt, 4, 1, fp);
          yread (&check, (void *)&scsp.slot[i].lfoinc, 4, 1, fp);

#ifdef IMPROVED_SAVESTATES
          yread(&check, (void *)&scsp.slot[i].sa, sizeof(u32), 1, fp);
          yread(&check, (void *)&scsp.slot[i].lsa, sizeof(u32), 1, fp);
          yread(&check, (void *)&scsp.slot[i].lea, sizeof(u32), 1, fp);

          yread(&check, (void *)&scsp.slot[i].tl, sizeof(s32), 1, fp);
          yread(&check, (void *)&scsp.slot[i].sl, sizeof(s32), 1, fp);

          yread(&check, (void *)&scsp.slot[i].ar, sizeof(s32), 1, fp);
          yread(&check, (void *)&scsp.slot[i].dr, sizeof(s32), 1, fp);
          yread(&check, (void *)&scsp.slot[i].sr, sizeof(s32), 1, fp);
          yread(&check, (void *)&scsp.slot[i].rr, sizeof(s32), 1, fp);

          //arp
          //drp
          //srp
          //rrp

          yread(&check, (void *)&scsp.slot[i].krs, sizeof(u32), 1, fp);

          //lfofmw
          //lfoemw

          yread(&check, (void *)&scsp.slot[i].lfofms, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].lfoems, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].fsft, sizeof(u8), 1, fp);

          yread(&check, (void *)&scsp.slot[i].mdl, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].mdx, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].mdy, sizeof(u8), 1, fp);

          yread(&check, (void *)&scsp.slot[i].imxl, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].disll, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].dislr, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].efsll, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].efslr, sizeof(u8), 1, fp);

          yread(&check, (void *)&scsp.slot[i].eghold, sizeof(u8), 1, fp);
          yread(&check, (void *)&scsp.slot[i].lslnk, sizeof(u8), 1, fp);
#endif

          // depends on pcm8b, sa, lea being loaded first
          // Rebuild the buf8/buf16 variables
          if (scsp.slot[i].pcm8b)
            {
              scsp.slot[i].buf8 = (s8*)&(scsp.scsp_ram[scsp.slot[i].sa]);
              if ((scsp.slot[i].sa + (scsp.slot[i].lea >> SCSP_FREQ_LB)) >
                  SCSP_RAM_MASK)
                scsp.slot[i].lea = (SCSP_RAM_MASK - scsp.slot[i].sa) <<
                                   SCSP_FREQ_LB;
            }
          else
            {
              scsp.slot[i].buf16 = (s16*)&(scsp.scsp_ram[scsp.slot[i].sa & ~1]);
              if ((scsp.slot[i].sa + (scsp.slot[i].lea >> (SCSP_FREQ_LB - 1))) >
                  SCSP_RAM_MASK)
                scsp.slot[i].lea = (SCSP_RAM_MASK - scsp.slot[i].sa) <<
                                   (SCSP_FREQ_LB - 1);
            }
        }

      // Read main internal variables
      yread (&check, (void *)&scsp.mem4b, 4, 1, fp);
      yread (&check, (void *)&scsp.mvol, 4, 1, fp);

      yread (&check, (void *)&scsp.rbl, 4, 1, fp);
      yread (&check, (void *)&scsp.rbp, 4, 1, fp);

      yread (&check, (void *)&scsp.mslc, 4, 1, fp);

      yread (&check, (void *)&scsp.dmea, 4, 1, fp);
      yread (&check, (void *)&scsp.drga, 4, 1, fp);
      yread (&check, (void *)&scsp.dmfl, 4, 1, fp);
      yread (&check, (void *)&scsp.dmlen, 4, 1, fp);

      yread (&check, (void *)scsp.midinbuf, 1, 4, fp);
      yread (&check, (void *)scsp.midoutbuf, 1, 4, fp);
      yread (&check, (void *)&scsp.midincnt, 1, 1, fp);
      yread (&check, (void *)&scsp.midoutcnt, 1, 1, fp);
      yread (&check, (void *)&scsp.midflag, 1, 1, fp);

      yread (&check, (void *)&scsp.timacnt, 4, 1, fp);
      yread (&check, (void *)&scsp.timasd, 4, 1, fp);
      yread (&check, (void *)&scsp.timbcnt, 4, 1, fp);
      yread (&check, (void *)&scsp.timbsd, 4, 1, fp);
      yread (&check, (void *)&scsp.timccnt, 4, 1, fp);
      yread (&check, (void *)&scsp.timcsd, 4, 1, fp);

      yread (&check, (void *)&scsp.scieb, 4, 1, fp);
      yread (&check, (void *)&scsp.scipd, 4, 1, fp);
      yread (&check, (void *)&scsp.scilv0, 4, 1, fp);
      yread (&check, (void *)&scsp.scilv1, 4, 1, fp);
      yread (&check, (void *)&scsp.scilv2, 4, 1, fp);
      yread (&check, (void *)&scsp.mcieb, 4, 1, fp);
      yread (&check, (void *)&scsp.mcipd, 4, 1, fp);

      yread (&check, (void *)scsp.stack, 4, 32 * 2, fp);
    }

  return size;
}

//////////////////////////////////////////////////////////////////////////////

static char *
AddSoundLFO (char *outstring, const char *string, u16 level, u16 waveform)
{
  if (level > 0)
    {
      switch (waveform)
        {
        case 0:
          AddString(outstring, "%s Sawtooth\r\n", string);
          break;
        case 1:
          AddString(outstring, "%s Square\r\n", string);
          break;
        case 2:
          AddString(outstring, "%s Triangle\r\n", string);
          break;
        case 3:
          AddString(outstring, "%s Noise\r\n", string);
          break;
        }
    }

  return outstring;
}

//////////////////////////////////////////////////////////////////////////////

static char *
AddSoundPan (char *outstring, u16 pan)
{
  if (pan == 0x0F)
    {
      AddString(outstring, "Left = -MAX dB, Right = -0 dB\r\n");
    }
  else if (pan == 0x1F)
    {
      AddString(outstring, "Left = -0 dB, Right = -MAX dB\r\n");
    }
  else
    {
      AddString(outstring, "Left = -%d dB, Right = -%d dB\r\n", (pan & 0xF) * 3, (pan >> 4) * 3);
    }

  return outstring;
}

//////////////////////////////////////////////////////////////////////////////

static char *
AddSoundLevel (char *outstring, u16 level)
{
  if (level == 0)
    {
      AddString(outstring, "-MAX dB\r\n");
    }
  else
    {
      AddString(outstring, "-%d dB\r\n", (7-level) *  6);
    }

  return outstring;
}

//////////////////////////////////////////////////////////////////////////////

void
ScspSlotDebugStats (u8 slotnum, char *outstring)
{
  u32 slotoffset = slotnum * 0x20;

  AddString (outstring, "Sound Source = ");
  switch (scsp.slot[slotnum].ssctl)
    {
    case 0:
      AddString (outstring, "External DRAM data\r\n");
      break;
    case 1:
      AddString (outstring, "Internal(Noise)\r\n");
      break;
    case 2:
      AddString (outstring, "Internal(0's)\r\n");
      break;
    default:
      AddString (outstring, "Invalid setting\r\n");
      break;
    }

  AddString (outstring, "Source bit = ");
  switch(scsp.slot[slotnum].sbctl)
    {
    case 0:
      AddString (outstring, "No bit reversal\r\n");
      break;
    case 1:
      AddString (outstring, "Reverse other bits\r\n");
      break;
    case 2:
      AddString (outstring, "Reverse sign bit\r\n");
      break;
    case 3:
      AddString (outstring, "Reverse sign and other bits\r\n");
      break;
    }

  // Loop Control
  AddString (outstring, "Loop Mode = ");
  switch (scsp.slot[slotnum].lpctl)
    {
    case 0:
      AddString (outstring, "Off\r\n");
      break;
    case 1:
      AddString (outstring, "Normal\r\n");
      break;
    case 2:
      AddString (outstring, "Reverse\r\n");
      break;
    case 3:
      AddString (outstring, "Alternating\r\n");
      break;
    }

  // PCM8B
  // NOTE: Need curly braces here, as AddString is a macro.
  if (scsp.slot[slotnum].pcm8b)
    {
      AddString (outstring, "8-bit samples\r\n");
    }
  else
    {
      AddString (outstring, "16-bit samples\r\n");
    }

  AddString (outstring, "Start Address = %05lX\r\n", (unsigned long)scsp.slot[slotnum].sa);
  AddString (outstring, "Loop Start Address = %04lX\r\n", (unsigned long)scsp.slot[slotnum].lsa >> SCSP_FREQ_LB);
  AddString (outstring, "Loop End Address = %04lX\r\n", (unsigned long)scsp.slot[slotnum].lea >> SCSP_FREQ_LB);
  AddString (outstring, "Decay 1 Rate = %ld\r\n", (unsigned long)scsp.slot[slotnum].dr);
  AddString (outstring, "Decay 2 Rate = %ld\r\n", (unsigned long)scsp.slot[slotnum].sr);
  if (scsp.slot[slotnum].eghold)
    AddString (outstring, "EG Hold Enabled\r\n");
  AddString (outstring, "Attack Rate = %ld\r\n", (unsigned long)scsp.slot[slotnum].ar);

  if (scsp.slot[slotnum].lslnk)
    AddString (outstring, "Loop Start Link Enabled\r\n");

  if (scsp.slot[slotnum].krs != 0)
    AddString (outstring, "Key rate scaling = %ld\r\n", (unsigned long)scsp.slot[slotnum].krs);

  AddString (outstring, "Decay Level = %d\r\n", (scsp_r_w(slotoffset + 0xA) >> 5) & 0x1F);
  AddString (outstring, "Release Rate = %ld\r\n", (unsigned long)scsp.slot[slotnum].rr);

  if (scsp.slot[slotnum].swe)
    AddString (outstring, "Stack Write Inhibited\r\n");

  if (scsp.slot[slotnum].sdir)
    AddString (outstring, "Sound Direct Enabled\r\n");

  AddString (outstring, "Total Level = %ld\r\n", (unsigned long)scsp.slot[slotnum].tl);

  AddString (outstring, "Modulation Level = %d\r\n", scsp.slot[slotnum].mdl);
  AddString (outstring, "Modulation Input X = %d\r\n", scsp.slot[slotnum].mdx);
  AddString (outstring, "Modulation Input Y = %d\r\n", scsp.slot[slotnum].mdy);

  AddString (outstring, "Octave = %d\r\n", (scsp_r_w(slotoffset + 0x10) >> 11) & 0xF);
  AddString (outstring, "Frequency Number Switch = %d\r\n", scsp_r_w(slotoffset + 0x10) & 0x3FF);

  AddString (outstring, "LFO Reset = %s\r\n", ((scsp_r_w(slotoffset + 0x12) >> 15) & 0x1) ? "TRUE" : "FALSE");
  AddString (outstring, "LFO Frequency = %d\r\n", (scsp_r_w(slotoffset + 0x12) >> 10) & 0x1F);
  outstring = AddSoundLFO (outstring, "LFO Frequency modulation waveform = ",
                           (scsp_r_w(slotoffset + 0x12) >> 5) & 0x7,
                           (scsp_r_w(slotoffset + 0x12) >> 8) & 0x3);
  AddString (outstring, "LFO Frequency modulation level = %d\r\n", (scsp_r_w(slotoffset + 0x12) >> 5) & 0x7);
  outstring = AddSoundLFO (outstring, "LFO Amplitude modulation waveform = ",
                           scsp_r_w(slotoffset + 0x12) & 0x7,
                           (scsp_r_w(slotoffset + 0x12) >> 3) & 0x3);
  AddString (outstring, "LFO Amplitude modulation level = %d\r\n", scsp_r_w(slotoffset + 0x12) & 0x7);

  AddString (outstring, "Input mix level = ");
  outstring = AddSoundLevel (outstring, scsp_r_w(slotoffset + 0x14) & 0x7);
  AddString (outstring, "Input Select = %d\r\n", (scsp_r_w(slotoffset + 0x14) >> 3) & 0x1F);

  AddString (outstring, "Direct data send level = ");
  outstring = AddSoundLevel (outstring, (scsp_r_w(slotoffset + 0x16) >> 13) & 0x7);
  AddString (outstring, "Direct data panpot = ");
  outstring = AddSoundPan (outstring, (scsp_r_w(slotoffset + 0x16) >> 8) & 0x1F);

  AddString (outstring, "Effect data send level = ");
  outstring = AddSoundLevel (outstring, (scsp_r_w(slotoffset + 0x16) >> 5) & 0x7);
  AddString (outstring, "Effect data panpot = ");
  outstring = AddSoundPan (outstring, scsp_r_w(slotoffset + 0x16) & 0x1F);
}

//////////////////////////////////////////////////////////////////////////////

void
ScspCommonControlRegisterDebugStats (char *outstring)
{
   AddString (outstring, "Memory: %s\r\n", scsp.mem4b ? "4 Mbit" : "2 Mbit");
   AddString (outstring, "Master volume: %ld\r\n", (unsigned long)scsp.mvol);
   AddString (outstring, "Ring buffer length: %ld\r\n", (unsigned long)scsp.rbl);
   AddString (outstring, "Ring buffer address: %08lX\r\n", (unsigned long)scsp.rbp);
   AddString (outstring, "\r\n");

   AddString (outstring, "Slot Status Registers\r\n");
   AddString (outstring, "-----------------\r\n");
   AddString (outstring, "Monitor slot: %ld\r\n", (unsigned long)scsp.mslc);
   AddString (outstring, "Call address: %ld\r\n", (unsigned long)scsp.ca);
   AddString (outstring, "\r\n");

   AddString (outstring, "DMA Registers\r\n");
   AddString (outstring, "-----------------\r\n");
   AddString (outstring, "DMA memory address start: %08lX\r\n", (unsigned long)scsp.dmea);
   AddString (outstring, "DMA register address start: %08lX\r\n", (unsigned long)scsp.drga);
   AddString (outstring, "DMA Flags: %lX\r\n", (unsigned long)scsp.dmlen);
   AddString (outstring, "\r\n");

   AddString (outstring, "Timer Registers\r\n");
   AddString (outstring, "-----------------\r\n");
   AddString (outstring, "Timer A counter: %02lX\r\n", (unsigned long)scsp.timacnt >> 8);
   AddString (outstring, "Timer A increment: Every %d sample(s)\r\n", (int)pow(2, (double)scsp.timasd));
   AddString (outstring, "Timer B counter: %02lX\r\n", (unsigned long)scsp.timbcnt >> 8);
   AddString (outstring, "Timer B increment: Every %d sample(s)\r\n", (int)pow(2, (double)scsp.timbsd));
   AddString (outstring, "Timer C counter: %02lX\r\n", (unsigned long)scsp.timccnt >> 8);
   AddString (outstring, "Timer C increment: Every %d sample(s)\r\n", (int)pow(2, (double)scsp.timcsd));
   AddString (outstring, "\r\n");

   AddString (outstring, "Interrupt Registers\r\n");
   AddString (outstring, "-----------------\r\n");
   AddString (outstring, "Sound cpu interrupt pending: %04lX\r\n", (unsigned long)scsp.scipd);
   AddString (outstring, "Sound cpu interrupt enable: %04lX\r\n", (unsigned long)scsp.scieb);
   AddString (outstring, "Sound cpu interrupt level 0: %04lX\r\n", (unsigned long)scsp.scilv0);
   AddString (outstring, "Sound cpu interrupt level 1: %04lX\r\n", (unsigned long)scsp.scilv1);
   AddString (outstring, "Sound cpu interrupt level 2: %04lX\r\n", (unsigned long)scsp.scilv2);
   AddString (outstring, "Main cpu interrupt pending: %04lX\r\n", (unsigned long)scsp.mcipd);
   AddString (outstring, "Main cpu interrupt enable: %04lX\r\n", (unsigned long)scsp.mcieb);
   AddString (outstring, "\r\n");
}

//////////////////////////////////////////////////////////////////////////////

int
ScspSlotDebugSaveRegisters (u8 slotnum, const char *filename)
{
  FILE *fp;
  int i;
  IOCheck_struct check = { 0, 0 };

  if ((fp = fopen (filename, "wb")) == NULL)
    return -1;

  for (i = (slotnum * 0x20); i < ((slotnum+1) * 0x20); i += 2)
    {
#ifdef WORDS_BIGENDIAN
      ywrite (&check, (void *)&scsp_isr[i ^ 2], 1, 2, fp);
#else
      ywrite (&check, (void *)&scsp_isr[(i + 1) ^ 2], 1, 1, fp);
      ywrite (&check, (void *)&scsp_isr[i ^ 2], 1, 1, fp);
#endif
    }

  fclose (fp);
  return 0;
}

//////////////////////////////////////////////////////////////////////////////

static slot_t debugslot;

u32
ScspSlotDebugAudio (u32 *workbuf, s16 *buf, u32 len)
{
  u32 *bufL, *bufR;

  bufL = workbuf;
  bufR = workbuf+len;
  scsp_bufL = (s32 *)bufL;
  scsp_bufR = (s32 *)bufR;

  if (debugslot.ecnt >= SCSP_ENV_DE)
    {
      // envelope null...
      memset (buf, 0, sizeof(s16) * 2 * len);
      return 0;
    }

  if (debugslot.ssctl)
    {
      memset (buf, 0, sizeof(s16) * 2 * len);
      return 0; // not yet supported!
    }

  scsp_buf_len = len;
  scsp_buf_pos = 0;

  // take effect sound volume if no direct sound volume...
  if ((debugslot.disll == 31) && (debugslot.dislr == 31))
    {
      debugslot.disll = debugslot.efsll;
      debugslot.dislr = debugslot.efslr;
    }

  memset (bufL, 0, sizeof(u32) * len);
  memset (bufR, 0, sizeof(u32) * len);
  scsp_slot_update_p[(debugslot.lfofms == 31)?0:1]
                    [(debugslot.lfoems == 31)?0:1]
                    [(debugslot.pcm8b == 0)?1:0]
                    [(debugslot.disll == 31)?0:1]
                    [(debugslot.dislr == 31)?0:1](&debugslot);
  ScspConvert32uto16s ((s32 *)bufL, (s32 *)bufR, (s16 *)buf, len);

  return len;
}

//////////////////////////////////////////////////////////////////////////////

typedef struct
{
  char id[4];
  u32 size;
} chunk_struct;

typedef struct
{
  chunk_struct riff;
  char rifftype[4];
} waveheader_struct;

typedef struct
{
  chunk_struct chunk;
  u16 compress;
  u16 numchan;
  u32 rate;
  u32 bytespersec;
  u16 blockalign;
  u16 bitspersample;
} fmt_struct;

//////////////////////////////////////////////////////////////////////////////

void
ScspSlotResetDebug(u8 slotnum)
{
  memcpy (&debugslot, &scsp.slot[slotnum], sizeof(slot_t));

  // Clear out the phase counter, etc.
  debugslot.fcnt = 0;
  debugslot.ecnt = SCSP_ENV_AS;
  debugslot.einc = &debugslot.einca;
  debugslot.ecmp = SCSP_ENV_AE;
  debugslot.ecurp = SCSP_ENV_ATTACK;
  debugslot.enxt = scsp_attack_next;
}

//////////////////////////////////////////////////////////////////////////////

int
ScspSlotDebugAudioSaveWav (u8 slotnum, const char *filename)
{
  u32 workbuf[512*2*2];
  s16 buf[512*2];
  FILE *fp;
  u32 counter = 0;
  waveheader_struct waveheader;
  fmt_struct fmt;
  chunk_struct data;
  long length;
  IOCheck_struct check = { 0, 0 };

  if (scsp.slot[slotnum].lea == 0)
    return 0;

  if ((fp = fopen (filename, "wb")) == NULL)
    return -1;

  // Do wave header
  memcpy (waveheader.riff.id, "RIFF", 4);
  waveheader.riff.size = 0; // we'll fix this after the file is closed
  memcpy (waveheader.rifftype, "WAVE", 4);
  ywrite (&check, (void *)&waveheader, 1, sizeof(waveheader_struct), fp);

  // fmt chunk
  memcpy (fmt.chunk.id, "fmt ", 4);
  fmt.chunk.size = 16; // we'll fix this at the end
  fmt.compress = 1; // PCM
  fmt.numchan = 2; // Stereo
  fmt.rate = 44100;
  fmt.bitspersample = 16;
  fmt.blockalign = fmt.bitspersample / 8 * fmt.numchan;
  fmt.bytespersec = fmt.rate * fmt.blockalign;
  ywrite (&check, (void *)&fmt, 1, sizeof(fmt_struct), fp);

  // data chunk
  memcpy (data.id, "data", 4);
  data.size = 0; // we'll fix this at the end
  ywrite (&check, (void *)&data, 1, sizeof(chunk_struct), fp);

  ScspSlotResetDebug(slotnum);

  // Mix the audio, and then write it to the file
  for (;;)
    {
      if (ScspSlotDebugAudio (workbuf, buf, 512) == 0)
        break;

      counter += 512;
      ywrite (&check, (void *)buf, 2, 512 * 2, fp);
      if (debugslot.lpctl != 0 && counter >= (44100 * 2 * 5))
        break;
    }

  length = ftell (fp);

  // Let's fix the riff chunk size and the data chunk size
  fseek (fp, sizeof(waveheader_struct)-0x8, SEEK_SET);
  length -= 0x4;
  ywrite (&check, (void *)&length, 1, 4, fp);

  fseek (fp, sizeof(waveheader_struct) + sizeof(fmt_struct) + 0x4, SEEK_SET);
  length -= sizeof(waveheader_struct) + sizeof(fmt_struct);
  ywrite (&check, (void *)&length, 1, 4, fp);
  fclose (fp);

  return 0;
}

//////////////////////////////////////////////////////////////////////////////
