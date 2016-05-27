/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2006, 2013 Theo Berkau
    Copyright 2016 James Laird-Wah

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef SH7034_H
#define SH7034_H

#include "core.h"
#include "sh2core.h"

void cd_trace_log(const char * format, ...);
void sh1_dreq_asserted(int which);

//#define WANT_CDTRACE
#ifdef WANT_CDTRACE
#define CDTRACE(...) cd_trace_log(__VA_ARGS__)
#else
#define CDTRACE(...)
#endif

u8 FASTCALL Sh1MemoryReadByte(SH2_struct *sh, u32 addr);
u16 FASTCALL Sh1MemoryReadWord(SH2_struct *sh, u32 addr);
u32 FASTCALL Sh1MemoryReadLong(SH2_struct *sh, u32 addr);
void FASTCALL Sh1MemoryWriteByte(SH2_struct *sh, u32 addr, u8 val);
void FASTCALL Sh1MemoryWriteWord(SH2_struct *sh, u32 addr, u16 val);
void FASTCALL Sh1MemoryWriteLong(SH2_struct *sh, u32 addr, u32 val);

void sh1_init_func();
void sh1_serial_recieve_bit(int bit, int channel);
void sh1_set_start(int state);
void sh1_serial_transmit_bit(int channel, int* output_bit);
void sh1_assert_tioca(int which);
void sh1_assert_tiocb(int which);

struct Onchip
{
   //sci0/1
   struct Sci
   {
      u8 smr;
      u8 brr;
      u8 scr;
      u8 tdr;
      u8 ssr;
      u8 rdr;

      //not visible to cpu
      u8 rsr;
      u8 rsr_counter;
      u8 rsr_written;

      //not visible to cpu
      u8 tsr;
      u8 tsr_counter;
      u8 tdr_written;

      //implementation specific
      int serial_clock_counter;
   }sci[2];

   //a/d
   u16 addra;
   u16 addrb;
   u16 addrc;
   u16 addrd;
   u8 adcsr;
   u8 adcr;

   //itu shared
   u8 tstr;
   u8 tsnc;
   u8 tmdr;
   u8 tfcr;

   //itu channels
   struct Itu01
   {
      u8 tcr;
      u8 tiorl;
      u8 tier;
      u8 tsr;
      u16 tcnt;
      u16 gra;
      u16 grb;
   }itu01[2];

   //2/3 have bra and brb
   struct Itu23
   {
      u8 tcr;
      u8 tiorl;
      u8 tier;
      u8 tsr;
      u16 tcnt;
      u16 gra;
      u16 grb;
      u16 bra;
      u16 brb;
   }itu23[2];

   struct Itu
   {
      //shared regs
      u8 tstr;
      u8 tsnc;
      u8 tmdr;
      u8 tfcr;
      u8 tocr;

      struct Channel
      {
         u8 tcr;
         u8 tior;
         u8 tier;
         u8 tsr;
         u16 tcnt;
         u16 gra;
         u16 grb;
         //buffer regs unused for channels 0-2
         u16 bra;
         u16 brb;

         //not registers, implementation specific
         u8 tcnt_fraction;

      }channel[5];
   }itu;

   u8 tocr;

   //dmac
   struct Dmac
   {
      struct channel
      {
         u32 sar;
         u32 dar;
         u16 tcr;
         u16 chcr;

         //implementation
         int is_active;
      }channel[4];

      u16 dmaor;
   }dmac;

   //intc
   struct Intc {
      u16 ipra;
      u16 iprb;
      u16 iprc;
      u16 iprd;
      u16 ipre;
      u16 icr;
   }intc;

   //ubc
   struct Ubc {
      u32 bar;
      u32 bamr;
      u16 bbr;
   }ubc;

   //bsc
   struct Bsc
   {
      u16 bcr;
      u16 wcr1;
      u16 wcr2;
      u16 wcr3;
      u16 dcr;
      u16 pcr;
      u16 rcr;
      u16 rtcsr;
      u16 rtcnt;
      u16 rtcor;
      u8 tcsr;
      u8 tcnt;
      u8 rstcsr;
   }bsc;

   struct Wdt
   {
      u8 tcsr;
      u8 tcnt;
      u8 rstcsr;
   }wdt;

   u8 sbycr;

   //address space

   //port a
   u16 padr;

   //port b
   u16 pbdr;

   //pfc
   struct Pfc
   {
      u16 paior;
      u16 pbior;
      u16 pacr1;
      u16 pacr2;
      u16 pbcr1;
      u16 pbcr2;
   }pfc;

   //port c
   u16 pcdr;

   //pfc
   u16 cascr;

   //tpc
   struct Tpc
   {
      u8 tpmr;
      u8 tpcr;
      u8 nderb;
      u8 ndera;
      u8 ndrb;
      u8 ndra;
   }tpc;
};

struct Sh1
{
   //u16 rom[0x8000];//64kb
   u8 ram[0x2000];
   struct Onchip onchip;
   s32 cycles_remainder;
};

extern struct Sh1 sh1_cxt;

void sh1_exec(struct Sh1 * sh1, s32 cycles);
void sh1_onchip_run_cycles(s32 cycles);
void sh1_set_output_enable_rising_edge();
void sh1_set_output_enable_falling_edge();
void sh1_dma_exec(s32 cycles);

// SCI SCR bits
#define SCI_TIE               0x80        /* Transmit interrupt enable */
#define SCI_RIE               0x40        /* Receive interrupt enable */
#define SCI_TE                0x20        /* Transmit enable */
#define SCI_RE                0x10        /* Receive enable */
#define SCI_MPIE              0x08        /* Multiprocessor interrupt enable */
#define SCI_TEIE              0x04        /* Transmit end interrupt enable */
#define SCI_CKE1              0x02        /* Clock enable 1 */
#define SCI_CKE0              0x01        /* Clock enable 0 */

// SCI SSR bits
#define SCI_TDRE              0x80        /* Transmit data register empty */
#define SCI_RDRF              0x40        /* Receive data register full */
#define SCI_ORER              0x20        /* Overrun error */
#define SCI_FER               0x10        /* Framing error */
#define SCI_PER               0x08        /* Parity error */
#define SCI_TEND              0x04        /* Transmit end */
#define SCI_MPB               0x02        /* Multiprocessor bit */
#define SCI_MPBT              0x01        /* Multiprocessor bit transfer */

#endif
