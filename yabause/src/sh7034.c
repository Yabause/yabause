/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2005, 2013 Theo Berkau

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

/*! \file sh7034.c
    \brief SH7034 cpu implementation (CD block controller)
*/

#include "core.h"
#include "sh7034.h"
#include "assert.h"

struct Sh1 sh1_cxt;

void onchip_write_timer_byte(struct Onchip * regs, u32 addr, int which_timer, u8 data)
{
   switch (addr)
   {
   case 0:
      regs->itu.channel[which_timer].tcr = data;
      return;
      break;
   case 1:
      regs->itu.channel[which_timer].tior = data;
      return;
      break;
   case 2:
      regs->itu.channel[which_timer].tier = data;
      return;
      break;
   case 3:
      regs->itu.channel[which_timer].tsr = data;
      return;
      break;
   case 4:
      regs->itu.channel[which_timer].tcnt = (regs->itu.channel[which_timer].tsr & 0xff) | (data << 8);
      return;
      break;
   case 5:
      regs->itu.channel[which_timer].tcnt = (regs->itu.channel[which_timer].tsr & 0xff00) | data;
      return;
      break;
   case 6:
      regs->itu.channel[which_timer].gra = (regs->itu.channel[which_timer].gra & 0xff) | (data << 8);
      return;
      break;
   case 7:
      regs->itu.channel[which_timer].gra = (regs->itu.channel[which_timer].gra & 0xff00) | data;
      return;
      break;
   case 8:
      regs->itu.channel[which_timer].grb = (regs->itu.channel[which_timer].grb & 0xff) | (data << 8);
      return;
      break;
   case 9:
      regs->itu.channel[which_timer].grb = (regs->itu.channel[which_timer].grb & 0xff00) | data;
      return;
      break;
   case 0xa:
      regs->itu.channel[which_timer].bra = (regs->itu.channel[which_timer].bra & 0xff) | (data << 8);
      return;
      break;
   case 0xb:
      regs->itu.channel[which_timer].bra = (regs->itu.channel[which_timer].bra & 0xff00) | data;
      return;
      break;
   case 0xc:
      regs->itu.channel[which_timer].brb = (regs->itu.channel[which_timer].brb & 0xff) | (data << 8);
      return;
      break;
   case 0xd:
      regs->itu.channel[which_timer].brb = (regs->itu.channel[which_timer].brb & 0xff00) | data;
      return;
      break;
   }

   assert(0);
}

u8 onchip_read_timer_byte(struct Onchip * regs, u32 addr, int which_timer)
{
   switch (addr)
   {
   case 0:
      return regs->itu.channel[which_timer].tcr;
      break;
   case 1:
      return regs->itu.channel[which_timer].tior;
      break;
   case 2:
      return regs->itu.channel[which_timer].tier;
      break;
   case 3:
      return regs->itu.channel[which_timer].tsr;
      break;
   case 4:
      return regs->itu.channel[which_timer].tcnt >> 8;
      break;
   case 5:
      return regs->itu.channel[which_timer].tcnt & 0xff;
      break;
   case 6:
      return regs->itu.channel[which_timer].gra >> 8;
      break;
   case 7:
      return regs->itu.channel[which_timer].gra & 0xff;
      break;
   case 8:
      return regs->itu.channel[which_timer].grb >> 8;
      break;
   case 9:
      return regs->itu.channel[which_timer].grb & 0xff;
      break;
   case 0xa:
      return regs->itu.channel[which_timer].bra >> 8;
      break;
   case 0xb:
      return regs->itu.channel[which_timer].bra & 0xff;
      break;
   case 0xc:
      return regs->itu.channel[which_timer].brb >> 8;
      break;
   case 0xd:
      return regs->itu.channel[which_timer].brb & 0xff;
      break;
   }

   assert(0);
   return 0;
}

void onchip_write_timer_word(struct Onchip * regs, u32 addr, int which_timer, u16 data)
{
   switch (addr)
   {
   case 0:
   case 1:
   case 2:
   case 3:
      //byte access only
      return;
      break;
   case 4:
      regs->itu.channel[which_timer].tcnt = data;
      return;
      break;
   case 6:
      regs->itu.channel[which_timer].gra = data;
      return;
      break;
   case 8:
      regs->itu.channel[which_timer].grb = data;
      return;
      break;
   case 0xa:
      regs->itu.channel[which_timer].bra = data;
      return;
      break;
   case 0xc:
      regs->itu.channel[which_timer].brb = data;
      return;
      break;
   }

   assert(0);
}

u16 onchip_read_timer_word(struct Onchip * regs, u32 addr, int which_timer)
{
   switch (addr)
   {
   case 0:
   case 1:
   case 2:
   case 3:
      //byte access only
      break;
   case 4:
      return regs->itu.channel[which_timer].tcnt;
      break;
   case 6:
      return regs->itu.channel[which_timer].gra;
      break;
   case 8:
      return regs->itu.channel[which_timer].grb;
      break;
   case 0xa:
      return regs->itu.channel[which_timer].bra;
      break;
   case 0xc:
      return regs->itu.channel[which_timer].brb;
      break;
   }

   assert(0);
   return 0;
}

void onchip_write_byte(struct Onchip * regs, u32 addr, u8 data)
{
   if (addr == 0x05ffff9a)
   {
      int q = 1;
   }
   if (addr >= 0x5FFFE00 && addr <= 0x5FFFEBF)
   {
      //unmapped
      return;
   }
   //sci
   else if (addr >= 0x5FFFEC0 && addr <= 0x5FFFECD)
   {
      switch (addr - 0x5FFFEC0)
      {
      case 0:
         regs->sci[0].smr = data;
         return;
         break;
      case 1:
         regs->sci[0].brr = data;
         return;
         break;
      case 2:
         regs->sci[0].scr = data;
         return;
         break;
      case 3:
         regs->sci[0].tdr = data;
         return;
         break;
      case 4:
         if (data == 0)//only 0 can be written
            regs->sci[0].ssr = data;
         return;
         break;
      case 5:
         //read only
         return;
         break;
      case 6:
         //nothing
         return;
         break;
      case 7:
         //nothing
         return;
         break;
      case 8:
         regs->sci[1].smr = data;
         return;
         break;
      case 9:
         regs->sci[1].brr = data;
         return;
         break;
      case 0xa:
         regs->sci[1].scr = data;
         return;
         break;
      case 0xb:
         regs->sci[1].tdr = data;
         return;
         break;
      case 0xc:
         if (data == 0)//only 0 can be written
            regs->sci[1].ssr = data;
         return;
         break;
      case 0xd:
         //read only
         return;
         break;
      case 0xe:
      case 0xf:
         //nothing
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFECE && addr <= 0x5FFFEDF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFEE0 && addr <= 0x5FFFEEF)
   {
      //a/d
      //read only
      return;
   }
   else if (addr >= 0x5FFFEF0 && addr <= 0x5FFFEFF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFF00 && addr <= 0x5FFFF03)
   {
      switch (addr - 0x5FFFF00)
      {
      case 0:
         regs->itu.tstr = data;
         return;
      case 1:
         regs->itu.tsnc = data;
         return;
      case 2:
         regs->itu.tmdr = data;
         return;
      case 3:
         regs->itu.tfcr = data;
         return;
      }
   }
   //timer writes
   else if (addr >= 0x5FFFF04 && addr <= 0x5FFFF3F)
   {
      if (addr <= 0x5FFFF0D)
         onchip_write_timer_byte(regs, addr - 0x5FFFF04, 0, data);
      else if (addr <= 0x5FFFF17)
         onchip_write_timer_byte(regs, addr - 0x5FFFF0E, 1, data);
      else if (addr <= 0x5FFFF21)
         onchip_write_timer_byte(regs, addr - 0x5FFFF18, 2, data);
      else if (addr <= 0x5FFFF2F)
         onchip_write_timer_byte(regs, addr - 0x5FFFF22, 3, data);
      else if (addr == 0x5FFFF30)
      {
         return;//unmapped
      }
      else if (addr == 0x5FFFF31)
      {
         regs->itu.tocr = data;
         return;
      }
      else
         onchip_write_timer_byte(regs, addr - 0x5FFFF32, 4, data);
      return;
   }
   else if (addr >= 0x5FFFF40 && addr <= 0x5FFFF7F)
   {
      if (addr == 0x5FFFF4F)
      {
         //clear bit 0
         if (!(data & 1))
            regs->dmac.channel[0].chcr &= 0xfffe;
         return;
      }
      else if (addr == 0x5FFFF48)
      {
         //can only clear bits 1 and 2
         //todo check this
         if (!(data & 1))
            regs->dmac.channel[0].chcr &= 0xfffe;
         if (!(data & 2))
            regs->dmac.channel[0].chcr &= 0xFFFD;

         return;
      }
      else if (addr == 0x5FFFF5F)
      {
         //clear bit 0
         if (!(data & 1))
            regs->dmac.channel[1].chcr &= 0xfffe;
         return;
      }
      else if (addr == 0x5FFFF6F)
      {
         //clear bit 0
         if (!(data & 1))
            regs->dmac.channel[2].chcr &= 0xfffe;
         return;
      }
      else if (addr == 0x5FFFF7F)
      {
         //clear bit 0
         if (!(data & 1))
            regs->dmac.channel[3].chcr &= 0xfffe;
         return;
      }
      //dmac

      //rest is inaccessible from byte writes
      return;
   }
   else if (addr >= 0x5FFFF80 && addr <= 0x5FFFF8F)
   {
      //intc

      switch (addr - 0x5FFFF80)
      {
      case 0:
      case 1:
      case 2:
      case 3:
         return;
         break;
      //ipra
      case 4:
         regs->intc.ipra = regs->intc.ipra & 0xff | data << 8;
         return;
         break;
      case 5:
         regs->intc.ipra = regs->intc.ipra & 0xff00 | data;
         return;
         break;
      //iprb
      case 6:
         regs->intc.iprb = regs->intc.iprb & 0xff | data << 8;
         return;
         break;
      case 7:
         regs->intc.iprb = regs->intc.iprb & 0xff00 | data;
         return;
         break;
      //iprc
      case 8:
         regs->intc.iprc = regs->intc.iprc & 0xff | data << 8;
         return;
         break;
      case 9:
         regs->intc.iprc = regs->intc.iprc & 0xff00 | data;
         return;
         break;
      //iprd
      case 0xa:
         regs->intc.iprd = regs->intc.iprd & 0xff | data << 8;
         return;
         break;
      case 0xb:
         regs->intc.iprd = regs->intc.iprd & 0xff00 | data;
         return;
         break;
      //ipre
      case 0xc:
         regs->intc.ipre = regs->intc.ipre & 0xff | data << 8;
         return;
         break;
      case 0xd:
         regs->intc.ipre = regs->intc.ipre & 0xff00 | data;
         return;
         break;
      //icr
      case 0xe:
         regs->intc.icr = regs->intc.icr & 0xff | data << 8;
         return;
         break;
      case 0xf:
         regs->intc.icr = regs->intc.icr & 0xff00 | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFF90 && addr <= 0x5FFFF99)
   {
      //ubc
      switch (addr - 0x5FFFF90)
      {
      //bar
      case 0:
         regs->ubc.bar = regs->ubc.bar & 0x00ffffff | data << 24;
         return;
         break;
      case 1:
         regs->ubc.bar = regs->ubc.bar & 0xff00ffff | data << 16;
         return;
         break;
      case 2:
         regs->ubc.bar = regs->ubc.bar & 0xffff00ff | data << 8;
         return;
         break;
      case 3:
         regs->ubc.bar = regs->ubc.bar & 0xffffff00 | data;
         return;
         break;
      //bamr
      case 4:
         regs->ubc.bamr = regs->ubc.bamr & 0x00ffffff | data << 24;
         return;
         break;
      case 5:
         regs->ubc.bamr = regs->ubc.bamr & 0xff00ffff | data << 16;
         return;
         break;
      case 6:
         regs->ubc.bamr = regs->ubc.bamr & 0xffff00ff | data << 8;
         return;
         break;
      case 7:
         regs->ubc.bamr = regs->ubc.bamr & 0xffffff00 | data;
         return;
         break;
      //bbr
      case 8:
         regs->ubc.bbr = regs->ubc.bar & 0x00ff | data << 8;
         return;
         break;
      case 9:
         regs->ubc.bbr = regs->ubc.bar & 0xff00 | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFF9A && addr <= 0x5FFFF9F)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFA0 && addr <= 0x5FFFFB3)
   {
      //bsc
      switch (addr - 0x5FFFFA0)
      {
      case 0:
         regs->bsc.bcr = regs->bsc.bcr & 0x00ff | data << 8;
         return;
         break;
      case 1:
         regs->bsc.bcr = regs->bsc.bcr & 0xff00 | data;
         return;
         break;
      case 2:
         regs->bsc.wcr1 = regs->bsc.wcr1 & 0x00ff | data << 8;
         return;
         break;
      case 3:
         regs->bsc.wcr1 = regs->bsc.wcr1 & 0xff00 | data;
         return;
         break;
      case 4:
         regs->bsc.wcr2 = regs->bsc.wcr2 & 0x00ff | data << 8;
         return;
         break;
      case 5:
         regs->bsc.wcr2 = regs->bsc.wcr2 & 0xff00 | data;
         return;
         break;
      case 6:
         regs->bsc.wcr3 = regs->bsc.wcr3 & 0x00ff | data << 8;
         return;
         break;
      case 7:
         regs->bsc.wcr3 = regs->bsc.wcr3 & 0xff00 | data;
         return;
         break;
      case 8:
         regs->bsc.dcr = regs->bsc.dcr & 0x00ff | data << 8;
         return;
         break;
      case 9:
         regs->bsc.dcr = regs->bsc.dcr & 0xff00 | data;
         return;
         break;
      case 0xa:
         regs->bsc.pcr = regs->bsc.pcr & 0x00ff | data << 8;
         return;
         break;
      case 0xb:
         regs->bsc.pcr = regs->bsc.pcr & 0xff00 | data;
         return;
         break;
      case 0xc:
         regs->bsc.rcr = regs->bsc.rcr & 0x00ff | data << 8;
         return;
         break;
      case 0xd:
         regs->bsc.rcr = regs->bsc.rcr & 0xff00 | data;
         return;
         break;
      case 0xe:
         regs->bsc.rtcsr = regs->bsc.rtcsr & 0x00ff | data << 8;
         return;
         break;
      case 0xf:
         regs->bsc.rtcsr = regs->bsc.rtcsr & 0xff00 | data;
         return;
         break;
      //rtcnt
      case 0x10:
         regs->bsc.rtcnt = regs->bsc.rtcnt & 0x00ff | data << 8;
         return;
         break;
      case 0x11:
         regs->bsc.rtcnt = regs->bsc.rtcnt & 0xff00 | data;
         return;
         break;
      //rtcor
      case 0x12:
         regs->bsc.rtcor = regs->bsc.rtcor & 0x00ff | data << 8;
         return;
         break;
      case 0x13:
         regs->bsc.rtcor = regs->bsc.rtcor & 0xff00 | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFFb4 && addr <= 0x5FFFFb7)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFB8 && addr <= 0x5FFFFBB)
   {
      //wdt
      //TODO
      return;
   }
   else if (addr == 0x5FFFFbc)
   {
      regs->sbycr = data;
      return;
   }
   else if (addr >= 0x5FFFFBD && addr <= 0x5FFFFBF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFC0 && addr <= 0x5FFFFC3)
   {
      //port a / b

      switch (addr - 0x5FFFFC0)
      {
      case 0:
         regs->padr = regs->padr & 0x00ff | data << 8;
         return;
         break;
      case 1:
         regs->padr = regs->padr & 0xff00 | data;
         return;
         break;
      case 2:
         regs->pbdr = regs->pbdr & 0x00ff | data << 8;
         return;
         break;
      case 3:
         regs->pbdr = regs->pbdr & 0xff00 | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFFC4 && addr <= 0x5FFFFCF)
   {
      //pfc

      switch (addr - 0x5FFFFC0)
      {
      //paior
      case 4:
         regs->pfc.paior = regs->pfc.paior & 0x00ff | data << 8;
         return;
         break;
      case 5:
         regs->pfc.paior = regs->pfc.paior & 0xff00 | data;
         return;
         break;
      //pbior
      case 6:
         regs->pfc.pbior = regs->pfc.pbior & 0x00ff | data << 8;
         return;
         break;
      case 7:
         regs->pfc.pbior = regs->pfc.pbior & 0xff00 | data;
         return;
         break;
      //pacr1
      case 8:
         regs->pfc.pacr1 = regs->pfc.pacr1 & 0x00ff | data << 8;
         return;
         break;
      case 9:
         regs->pfc.pacr1 = regs->pfc.pacr1 & 0xff00 | data;
         return;
         break;
      //pacr2
      case 0xa:
         regs->pfc.pacr2 = regs->pfc.pacr2 & 0x00ff | data << 8;
         return;
         break;
      case 0xb:
         regs->pfc.pacr2 = regs->pfc.pacr2 & 0xff00 | data;
         return;
         break;
      //pbcr1
      case 0xc:
         regs->pfc.pbcr1 = regs->pfc.pbcr1 & 0x00ff | data << 8;
         return;
         break;
      case 0xd:
         regs->pfc.pbcr1 = regs->pfc.pbcr1 & 0xff00 | data;
         return;
         break;
      //pbcr2
      case 0xe:
         regs->pfc.pbcr2 = regs->pfc.pbcr2 & 0x00ff | data << 8;
         return;
         break;
      case 0xf:
         regs->pfc.pbcr2 = regs->pfc.pbcr2 & 0xff00 | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFFD0 && addr <= 0x5FFFFD1)
   {
      //port a / b

      switch (addr - 0x5FFFFD0)
      {
      case 0:
         regs->pcdr = regs->pcdr & 0x00ff | data << 8;
         return;
         break;
      case 1:
         regs->pcdr = regs->pcdr & 0xff00 | data;
         return;
         break;
      }
   }
   else if (addr >= 0x5FFFFD2 && addr <= 0x5FFFFED)
   {
      //unmapped
      return;
   }
   else if (addr == 0x5FFFFEE)
   {
      //cascr
      regs->cascr = data;
      return;
   }
   else if (addr == 0x5FFFFEF)
   {
      //unmapped
      return;
   }
   else if (addr >= 0x5FFFFF0 && addr <= 0x5FFFFF7)
   {
      //tpc
      switch (addr - 0x5FFFFF0)
      {
      case 0:
         regs->tpc.tpmr = data;
         return;
      case 1:
         regs->tpc.tpcr = data;
         return;
      case 2:
         regs->tpc.nderb = data;
         return;
      case 3:
         regs->tpc.ndera = data;
         return;
      case 4:
         regs->tpc.ndrb = data;
         return;
      case 5:
         regs->tpc.ndra = data;
         return;
      case 6:
         regs->tpc.ndrb = data;
         return;
      case 7:
         regs->tpc.ndra = data;
         return;
      }
   }
   else if (addr >= 0x5FFFFF8 && addr <= 0x5FFFFFF)
   {
      //unmapped
      return;
   }

   assert(0);
}

u8 onchip_sci_read_byte(struct Onchip * regs, u32 addr, int which)
{
   switch (addr)
   {
   case 0:
      return regs->sci[which].smr;
   case 1:
      return regs->sci[which].brr;
   case 2:
      return regs->sci[which].scr;
   case 3:
      return regs->sci[which].tdr;
   case 4:
      return regs->sci[which].ssr;
   case 5:
      return regs->sci[which].rdr;
   }

   assert(0);

   return 0;
}

u8 onchip_read_byte(struct Onchip * regs, u32 addr)
{
   if (addr >= 0x5FFFE00 && addr <= 0x5FFFEBF)
   {
      //unmapped
      return 0;
   }
   //sci
   else if (addr >= 0x5FFFEC0 && addr <= 0x5FFFECD)
   {
      if (addr >= 0x5FFFEC0 && addr <= 0x5FFFEC5)
      {
         //channel 0
         return onchip_sci_read_byte(regs, addr - 0x5FFFEC0, 0);
      }
      else if (addr >= 0x5FFFEC6 && addr <= 0x5FFFEC7)
      {
         return 0;//unmapped
      }
      else if (addr >= 0x5FFFEC8 && addr <= 0x5FFFECD)
      {
         //channel 1
         return onchip_sci_read_byte(regs, addr - 0x5FFFEC8, 1);
      }
      else if (addr >= 0x5FFFECE && addr <= 0x5FFFECF)
      {
         return 0;//unmapped
      }
   }
   else if (addr >= 0x5FFFECE && addr <= 0x5FFFEDF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFEE0 && addr <= 0x5FFFEE9)
   {
      //a/d
      switch (addr - 0x5FFFEE0)
      {
      //addra
      case 0:
         return (regs->addra & 0xff00) >> 8;
         break;
      case 1:
         return regs->addra & 0xff;
         break;
      //addrb
      case 2:
         return (regs->addrb & 0xff00) >> 8;
         break;
      case 3:
         return regs->addrb & 0xff;
         break;
      //addrc
      case 4:
         return (regs->addrc & 0xff00) >> 8;
         break;
      case 5:
         return regs->addrc & 0xff;
         break;
      //addrd
      case 6:
         return (regs->addrd & 0xff00) >> 8;
         break;
      case 7:
         return regs->addrd & 0xff;
         break;
      case 8:
         return regs->adcsr;
         break;
      case 9:
         return regs->adcr;
         break;
      }
   }
   else if (addr >= 0x5FFFEEa && addr <= 0x5FFFEEf)
   {
      return 0;//unmapped
   }
   else if (addr >= 0x5FFFEF0 && addr <= 0x5FFFEFF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFF00 && addr <= 0x5FFFF03)
   {
      //itu shared
      switch (addr - 0x5FFFF00)
      {
      case 0:
         return regs->itu.tstr;
      case 1:
         return regs->itu.tsnc;
      case 2:
         return regs->itu.tmdr;
      case 3:
         return regs->itu.tfcr;
      }
   }
   //timer
   else if (addr >= 0x5FFFF04 && addr <= 0x5FFFF3F)
   {
      if (addr <= 0x5FFFF0D)
         return onchip_read_timer_byte(regs, addr - 0x5FFFF04, 0);
      else if (addr >= 0x5FFFF0e && addr <= 0x5FFFF17)
         return onchip_read_timer_byte(regs, addr - 0x5FFFF0E, 1);
      else if (addr >= 0x5FFFF18 && addr <= 0x5FFFF21)
         return onchip_read_timer_byte(regs, addr - 0x5FFFF18, 2);
      else if (addr >= 0x5FFFF22 && addr <= 0x5FFFF2F)
         return onchip_read_timer_byte(regs, addr - 0x5FFFF22, 3);
      else if (addr >= 0x5FFFF32 && addr <= 0x5FFFF3F)
         return onchip_read_timer_byte(regs, addr - 0x5FFFF32, 4);

      if (addr == 0x05ffff30)
         return 0;//unmapped

      if (addr == 0x05ffff31)
         return regs->itu.tocr;
   }
   else if (addr >= 0x5FFFF40 && addr <= 0x5FFFF7F)
   {
      //dmac
      if (addr == 0x5FFFF4E)
         return regs->dmac.channel[0].chcr >> 8;
      else if (addr == 0x5FFFF4F)
         return regs->dmac.channel[0].chcr & 0xff;

      if (addr == 0x5FFFF48)
         return regs->dmac.dmaor >> 8;
      else if (addr == 0x5FFFF49)
         return regs->dmac.dmaor & 0xff;

      if (addr == 0x5FFFF5E)
         return regs->dmac.channel[1].chcr >> 8;
      else if (addr == 0x5FFFF5F)
         return regs->dmac.channel[1].chcr & 0xff;

      if (addr == 0x5FFFF6E)
         return regs->dmac.channel[2].chcr >> 8;
      else if (addr == 0x5FFFF5F)
         return regs->dmac.channel[2].chcr & 0xff;

      if (addr == 0x5FFFF7E)
         return regs->dmac.channel[3].chcr >> 8;
      else if (addr == 0x5FFFF5F)
         return regs->dmac.channel[3].chcr & 0xff;

      //the rest is inacessible
      return 0;
   }
   else if (addr >= 0x5FFFF80 && addr <= 0x5FFFF83)
   {
      //unmapped?
      return 0;
   }
   else if (addr >= 0x5FFFF84 && addr <= 0x5FFFF8F)
   {
      //intc
      switch (addr - 0x5FFFF80)
      {
      case 0:
      case 1:
      case 2:
      case 3:
         assert(0);
         break;
         //ipra
      case 4:
         return regs->intc.ipra >> 8;
      case 5:
         return regs->intc.ipra & 0xff; 
         //iprb
      case 6:
         return regs->intc.iprb >> 8;
      case 7:
         return regs->intc.iprb & 0xff;
         //iprc
      case 8:
         return regs->intc.iprc >> 8;
      case 9:
         return regs->intc.iprc & 0xff;
         //iprd
      case 0xa:
         return regs->intc.iprd >> 8;
      case 0xb:
         return regs->intc.iprd & 0xff;
         //ipre
      case 0xc:
         return regs->intc.ipre >> 8;
      case 0xd:
         return regs->intc.ipre & 0xff;
         //icr
      case 0xe:
         return regs->intc.icr >> 8;
      case 0xf:
         return regs->intc.icr & 0xff;
      }
   }
   else if (addr >= 0x5FFFF90 && addr <= 0x5FFFF99)
   {
      //ubc
      switch (addr - 0x5FFFF90)
      {
         //bar
      case 0:
         return regs->ubc.bar & 0xff000000 >> 24;
      case 1:
         return regs->ubc.bar & 0x00ff0000 >> 16;
      case 2:
         return regs->ubc.bar & 0x0000ff00 >> 8;
      case 3:
         return regs->ubc.bar & 0x000000ff >> 0;
         //bamr
      case 4:
         return regs->ubc.bamr & 0xff000000 >> 24;
      case 5:
         return regs->ubc.bamr & 0x00ff0000 >> 16;
      case 6:
         return regs->ubc.bamr & 0x0000ff00 >> 8;
      case 7:
         return regs->ubc.bamr & 0x000000ff >> 0;
         //bbr
      case 8:
         return regs->ubc.bbr >> 8;
      case 9:
         return regs->ubc.bbr & 0xff;
      }
   }
   else if (addr >= 0x5FFFF9A && addr <= 0x5FFFF9F)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFA0 && addr <= 0x5FFFFB3)
   {
      //bsc
      switch (addr - 0x5FFFFA0)
      {
      case 0:
         return regs->bsc.bcr >> 8;
      case 1:
         return regs->bsc.bcr & 0xff;
      case 2:
         return regs->bsc.wcr1 >> 8;
      case 3:
         return regs->bsc.wcr1 & 0xff;
      case 4:
         return regs->bsc.wcr2 >> 8;
      case 5:
         return regs->bsc.wcr2 & 0xff;
      case 6:
         return regs->bsc.wcr3 >> 8;
      case 7:
         return regs->bsc.wcr3 & 0xff;
      case 8:
         return regs->bsc.dcr >> 8;
      case 9:
         return regs->bsc.dcr & 0xff;
      case 0xa:
         return regs->bsc.pcr >> 8;
      case 0xb:
         return regs->bsc.pcr & 0xff;
      case 0xc:
         return regs->bsc.rcr >> 8;
      case 0xd:
         return regs->bsc.rcr & 0xff;
      case 0xe:
         return regs->bsc.rtcsr >> 8;
      case 0xf:
         return regs->bsc.rtcsr & 0xff;
         //rtcnt
      case 0x10:
         return regs->bsc.rtcnt >> 8; 
      case 0x11:
         return regs->bsc.rtcnt & 0xff; 
         //rtcor
      case 0x12:
         return regs->bsc.rtcor >> 8;
      case 0x13:
         return regs->bsc.rtcor & 0xff;
      }
   }
   else if (addr >= 0x5FFFFb4 && addr <= 0x5FFFFb7)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFB8 && addr <= 0x5FFFFBB)
   {
      //wdt
      //TODO
      return 0;
   }
   else if (addr == 0x5FFFFbc)
   {
      //sbycr
      return regs->sbycr;
   }
   else if (addr >= 0x5FFFFBD && addr <= 0x5FFFFBF)
   {
      //unmapped
      return 0;
   }
   else if (addr >= 0x5FFFFC0 && addr <= 0x5FFFFC3)
   {
      //port a/b data reg
      switch (addr - 0x5FFFFC0)
      {
      case 0:
         return regs->padr >> 8;
      case 1:
         return regs->padr & 0xff;
      case 2:
         return regs->pbdr >> 8;
      case 3:
         return regs->pbdr & 0xff;
      }
   }
   else if (addr >= 0x5FFFFC4 && addr <= 0x5FFFFCF)
   {
      //pfc
      switch (addr - 0x5FFFFC0)
      {
      //paior
      case 4:
         return regs->pfc.paior >> 8;
      case 5:
         return regs->pfc.paior & 0xff;
         //pbior
      case 6:
         return regs->pfc.pbior >> 8;
      case 7:
         return regs->pfc.pbior & 0xff;
         //pacr1
      case 8:
         return regs->pfc.pacr1 >> 8;
      case 9:
         return regs->pfc.pacr1 & 0xff;
         //pacr2
      case 0xa:
         return regs->pfc.pacr2 >> 8;
      case 0xb:
         return regs->pfc.pacr2 & 0xff;
         //pbcr1
      case 0xc:
         return regs->pfc.pbcr1 >> 8;
      case 0xd:
         return regs->pfc.pbcr1 & 0xff;
         //pbcr2
      case 0xe:
         return regs->pfc.pbcr2 >> 8;
      case 0xf:
         return regs->pfc.pbcr2 & 0xff;
      }
   }
   else if (addr == 0x5FFFFD0)
   {
      return regs->pcdr >> 8;
   }
   else if (addr == 0x5FFFFD1)
   {
      return regs->pcdr & 0xff;
   }
   else if (addr >= 0x5FFFFD2 && addr <= 0x5FFFFED)
   {
      //unmapped
      return 0;
   }
   else if (addr == 0x5FFFFEE)
   {
      //cascr
      return regs->cascr >> 8;
   }
   else if (addr == 0x5FFFFEF)
   {
      return regs->cascr & 0xff;
   }
   else if (addr >= 0x5FFFFF0 && addr <= 0x5FFFFF7)
   {
      //tpc
      switch (addr - 0x5FFFFF0)
      {
      case 0:
         return regs->tpc.tpmr;
      case 1:
         return regs->tpc.tpcr;
      case 2:
         return regs->tpc.nderb;
      case 3:
         return regs->tpc.ndera;
      case 4:
         return regs->tpc.ndrb;
      case 5:
         return regs->tpc.ndra;
      case 6:
         return regs->tpc.ndrb;
      case 7:
         return regs->tpc.ndra;
      }
   }
   else if (addr >= 0x5FFFFF8 && addr <= 0x5FFFFFF)
   {
      //unmapped
      return 0;
   }

   assert(0);

   return 0;
}

void onchip_write_word(struct Onchip * regs, u32 addr, u16 data)
{
   //timer word writes
   if (addr >= 0x5FFFF04 && addr <= 0x5FFFF3F)
   {
      if (addr <= 0x5FFFF0D)
         onchip_write_timer_word(regs, addr - 0x5FFFF04, 0, data);
      else if (addr <= 0x5FFFF17)
         onchip_write_timer_word(regs, addr - 0x5FFFF0E, 1, data);
      else if (addr <= 0x5FFFF21)
         onchip_write_timer_word(regs, addr - 0x5FFFF18, 2, data);
      else if (addr <= 0x5FFFF2F)
         onchip_write_timer_word(regs, addr - 0x5FFFF22, 3, data);
      else
         onchip_write_timer_word(regs, addr - 0x5FFFF32, 4, data);
      return;
   }

   addr -= 0x5FFFEC0;

   addr /= 2;
   switch (addr)
   {
   case 0:
      regs->sci[0].smr = (data >> 8) & 0xff;
      regs->sci[0].brr = data & 0xff;
      return;
      break;
   case 1:
      regs->sci[0].scr = (data >> 8) & 0xff;
      regs->sci[0].tdr = data & 0xff;
      return;
      break;
   case 2:
   {
      u8 temp = (data >> 8) & 0xff;
      if(!temp)
         regs->sci[0].ssr = temp;
      //read only
   }
      return;
      break;
   case 3:
      return;
      break;
   case 4:
      regs->sci[1].smr = (data >> 8) & 0xff;
      regs->sci[1].brr = data & 0xff;
      return;
      break;
   case 5:
      regs->sci[1].scr = (data >> 8) & 0xff;
      regs->sci[1].tdr = data & 0xff;
      return;
      break;
   case 6:
   {
      u8 temp = (data >> 8) & 0xff;
      if (!temp)
         regs->sci[1].ssr = temp;
      //read only

      return;
   }
   break;
   case 7:
      return;
      break;
   }

   assert(0);
}

u16 onchip_read_word(struct Onchip * regs, u32 addr)
{
   if (addr >= 0x5FFFF04 && addr <= 0x5FFFF3F)
   {
      if (addr <= 0x5FFFF0D)
         return onchip_read_timer_word(regs, addr - 0x5FFFF04, 0);
      else if (addr <= 0x5FFFF17)
         return onchip_read_timer_word(regs, addr - 0x5FFFF0E, 1);
      else if (addr <= 0x5FFFF21)
         return onchip_read_timer_word(regs, addr - 0x5FFFF18, 2);
      else if (addr <= 0x5FFFF2F)
         return onchip_read_timer_word(regs, addr - 0x5FFFF22, 3);
      else
         return onchip_read_timer_word(regs, addr - 0x5FFFF32, 4);
   }

//   addr -= ;

//   addr /= 2;
   switch (addr - 0x5FFFEC0)
   {
   case 0:
      return (regs->sci[0].smr << 8) | regs->sci[0].brr;
      break;
   case 1:
      return (regs->sci[0].scr << 8) | regs->sci[0].tdr;
      break;
   case 2:
      return (regs->sci[0].ssr << 8) | regs->sci[0].rdr;
      break;
   case 3:
      break;
   case 4:
      return (regs->sci[1].smr << 8) | regs->sci[1].brr;
      break;
   case 5:
      return (regs->sci[1].scr << 8) | regs->sci[1].tdr;
      break;
   case 6:
      return (regs->sci[1].ssr << 8) | regs->sci[1].rdr;
      break;
   case 7:
      break;
   }
   assert(0);
   return 0;
}

void onchip_write_long(struct Onchip * regs, u32 addr, u32 data)
{
   switch (addr)
   {
   case 0x5FFFF08:
      break;
   case 0x5FFFF0a:
      break;
   }
   assert(0);
}

u32 onchip_read_long(struct Onchip * regs, u32 addr)
{
   assert(0);
   return 0;
}

void memory_map_write_byte(struct Sh1* sh1, u32 addr, u8 data)
{
   u8 area = (addr >> 24) & 7;
   u8 a27 = (addr >> 27) & 1;
   int mode_pins = 0;

   switch (area)
   {
   case 0:
      //ignore a27 in area 0

      if (mode_pins == 2)//010
      {
         //   return sh1->rom[addr & 0xffff];
      }
      else
      {
         //mode 000 or 001

         //external memory space

         addr &= 0x3FFFFF;
      }
      break;
   case 1:
      if (!sh1->onchip.bsc.bcr)
      {
         //extern memory space
      }
      else
      {

      }
      break;
   case 2:
   case 3:
   case 4:
      //external memory space
      break;
   case 5:
      //onchip area
      if (!a27)
      {
         onchip_write_byte(&sh1->onchip, addr, data);
         return;
      }
      else
      {
         //external memory space
      }
      break;
   case 6:
      if (a27)
      {
         //external memory space
      }
      else if (!sh1->onchip.bsc.bcr)
      {
         //external memory space
      }
      break;
   case 7:
      //onchip ram
      if (a27)
      {
         sh1->ram[addr & 0xFFF] = data;
      }
      else
      {
         //onchip peripherals
      }
      break;
   }
}

u8 memory_map_read_byte(struct Sh1* sh1, u32 addr)
{
   u8 area = (addr >> 24) & 7;
   u8 a27 = (addr >> 27) & 1;
   int mode_pins = 0;

   switch (area)
   {
   case 0:
      //ignore a27 in area 0

      if (mode_pins == 2)//010
         return sh1->rom[addr & 0xffff];
      else
      {
         //mode 000 or 001

         //external memory space

         addr &= 0x3FFFFF;
      }
      break;
   case 1:
      if (!sh1->onchip.bsc.bcr)
      {
         //extern memory space
      }
      else
      {

      }
      break;
   case 2:
   case 3:
   case 4:
      //external memory space
      break;
   case 5:
      //onchip area
      if (!a27)
         return onchip_read_byte(&sh1->onchip, addr);
      else
      {
         //external memory space
      }
      break;
   case 6:
      if (a27)
      {
         //external memory space
      }
      else if (!sh1->onchip.bsc.bcr)
      {
         //external memory space
      }
      break;
   case 7:
      //onchip ram
      if (a27)
         return sh1->ram[addr & 0xFFF];
      else
      {
         //onchip peripherals
      }
      break;
   }

   return 0;
}


u16 memory_map_read_word(struct Sh1* sh1, u32 addr)
{
   u8 area = (addr >> 24) & 7;
   u8 a27 = (addr >> 27) & 1;
   int mode_pins = 0;

   switch (area)
   {
   case 0:
      //ignore a27 in area 0

      if (mode_pins == 2)//010
         return sh1->rom[addr & 0xffff];
      else
      {
         //mode 000 or 001

         //external memory space

         addr &= 0x3FFFFF;
      }
      break;
   case 1:
      if (!sh1->onchip.bsc.bcr)
      {
         //extern memory space
      }
      else
      {

      }
      break;
   case 2:
   case 3:
   case 4:
      //external memory space
      break;
   case 5:
      //onchip area
      if (!a27)
         return onchip_read_word(&sh1->onchip, addr);
      else
      {
         //external memory space
      }
      break;
   case 6:
      if (a27)
      {
         //external memory space
      }
      else if (!sh1->onchip.bsc.bcr)
      {
         //external memory space
      }
      break;
   case 7:
      //onchip ram
      if(a27)
         return sh1->ram[addr & 0xFFF];
      else
      {
         //onchip peripherals
      }
      break;
   }

   return 0;
}

void onchip_sci_init(struct Onchip * regs)
{
   int i;
   for (i = 0; i < 2; i++)
   {
      regs->sci[i].smr = 0;
      regs->sci[i].brr = 0xff;
      regs->sci[i].scr = 0;
      regs->sci[i].tdr = 0xff;
      regs->sci[i].ssr = 0x84;
      regs->sci[i].rdr = 0;
   }
}

void onchip_ad_init(struct Onchip * regs)
{
   regs->addra = 0;
   regs->addrb = 0;
   regs->addrc = 0;
   regs->addrd = 0;
   regs->adcsr = 0;
   regs->adcr = 0x7f;
}

void onchip_itu_init(struct Onchip * regs)//init values depend on config
{
   int i;
   
   regs->itu.tstr = 0xe0;
   regs->itu.tsnc = 0xe0;
   regs->itu.tmdr = 0x80;
   regs->itu.tfcr = 0xc0;
   regs->itu.tocr = 0xff;

   for (i = 0; i < 5; i++)
   {
      regs->itu.channel[i].tcr = 0x80;
      regs->itu.channel[i].tior = 0x88;
      regs->itu.channel[i].tier = 0xf8;
      regs->itu.channel[i].tsr = 0xf8;
      regs->itu.channel[i].tcnt = 0;
      regs->itu.channel[i].gra = 0xff;
      regs->itu.channel[i].grb = 0xff;
      //not used for channels 0-2
      regs->itu.channel[i].bra = 0xff;
      regs->itu.channel[i].brb = 0xff;
   }
}

void onchip_dmac_init(struct Onchip * regs)
{
   int i;
   for (i = 0; i < 4; i++)
   {
      regs->dmac.channel[i].sar = 0;//undefined
      regs->dmac.channel[i].dar = 0;//undefined
      regs->dmac.channel[i].tcr = 0;//undefined
      regs->dmac.channel[i].chcr = 0;
   }

   regs->dmac.dmaor = 0;
}

void onchip_intc_init(struct Onchip * regs)
{
   regs->intc.ipra = 0;
   regs->intc.iprb = 0;
   regs->intc.iprc = 0;
   regs->intc.iprd = 0;
   regs->intc.ipre = 0;
}

void onchip_ubc_init(struct Onchip * regs)
{
   regs->ubc.bar = 0;
   regs->ubc.bamr = 0;
   regs->ubc.bbr = 0;
}

void onchip_bsc_init(struct Onchip * regs)
{
   regs->bsc.bcr = 0;
   regs->bsc.wcr1 = 0;
   regs->bsc.wcr2 = 0;
   regs->bsc.wcr3 = 0;
   regs->bsc.dcr = 0;
   regs->bsc.pcr = 0;
   regs->bsc.rcr = 0;
   regs->bsc.rtcsr = 0;
   regs->bsc.rtcnt = 0;
   regs->bsc.rtcor = 0;
}

void onchip_wdt_init(struct Onchip * regs)
{
   regs->wdt.tcsr = 0;
   regs->wdt.tcnt = 0;
   regs->wdt.rstcsr = 0;
}

void onchip_pfc_init(struct Onchip * regs)
{
   regs->pfc.paior = 0;
   regs->pfc.pacr1 = 0x3302;
   regs->pfc.pacr2 = 0xff95;
   regs->pfc.pbior = 0;
   regs->pfc.pbcr1 = 0;
   regs->pfc.pbcr2 = 0;
   regs->cascr = 0x5fff;
}

void onchip_tpc_init(struct Onchip * regs)
{
   regs->tpc.tpmr = 0xf0;
   regs->tpc.tpcr = 0xff;
   regs->tpc.nderb = 0;
   regs->tpc.ndera = 0;
   regs->tpc.ndrb = 0;
   regs->tpc.ndra = 0;
}

void onchip_init(struct Sh1 * sh1)
{
   onchip_sci_init(&sh1->onchip);
   onchip_ad_init(&sh1->onchip);
   onchip_itu_init(&sh1->onchip);
   onchip_dmac_init(&sh1->onchip);
   onchip_intc_init(&sh1->onchip);
   onchip_ubc_init(&sh1->onchip);
   onchip_bsc_init(&sh1->onchip);
   onchip_wdt_init(&sh1->onchip);

   sh1->onchip.sbycr = 0;

   sh1->onchip.padr = 0;
   sh1->onchip.pbdr = 0;

   onchip_pfc_init(&sh1->onchip);

   sh1->onchip.pcdr = 0;

   onchip_tpc_init(&sh1->onchip);
}

void sh1_init(struct Sh1* sh1)
{
   memset(sh1, 0, sizeof(struct Sh1));
   onchip_init(sh1);
}

u16 sh1_fetch(struct Sh1* sh1)
{
   u32 PC = 0;
   return sh1->rom[PC & 0xffff];
}

int sh1_execute_instruction(struct Sh1 * sh1)
{
   u16 instruction = sh1_fetch(sh1);
   int cycles_executed = 1;
   return cycles_executed;
}


void test_byte_access(struct Sh1* sh1, u32 addr)
{
   u8 test_val = 0xff;
   memory_map_write_byte(sh1, addr, test_val);
   u8 result = memory_map_read_byte(sh1, addr);
}

void sh1_exec(struct Sh1 * sh1, s32 cycles)
{
#if 0
   int i;

   for (i = 0x5FFFEC0; i < 0x5FFFFFF; i++)
   {
      test_byte_access(sh1, i);
   }

   s32 cycles_temp = sh1->cycles_remainder - cycles;
   while (cycles_temp < 0)
   {
      int cycles = sh1_execute_instruction(sh1);
      cycles_temp += cycles;
   }
   sh1->cycles_remainder = cycles_temp;
#endif
}

int sh1_load_rom(struct Sh1* sh1, const char* filename)
{
   size_t size = 0;
   FILE * fp = NULL;

   if (!sh1)
      return -1;

   if (!filename)
      return -1;

   fp = fopen(filename, "rb");

   if (!fp)
      return -1;

   if (!fseek(fp, 0, SEEK_END))
      return -1;

   size = ftell(fp);

   if (size != 0x8000)
      return -1;

   if (!fseek(fp, 0, SEEK_SET))
      return -1;

   size = fread(&sh1->rom, sizeof(u8), 0x8000, fp);

   if (size != 0x8000)
      return -1;

   return 1;
}