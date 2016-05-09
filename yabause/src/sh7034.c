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

struct Sh1 sh1_cxt;

//module order / offsets in bytes from 0x5FFFEC0
//sci : 0
//a/d : 0x20
//itu : 0x40
//dmac : 0x80
//intc : 0xC0
//ubc : 0xD0
//bsc : 0xE0
//wdt : 0xF8
//sbycr : 0xFC
//pfc : 0x104
//tpc : 0x12F

void onchip_write_timer(struct Onchip * regs, u32 addr, u8 data)
{
   switch (addr)
   {
   case 0x44:
      regs->itu.channel[0].tcr = data;
      break;
   case 0x45:
      regs->itu.channel[0].tior = data;
      break;
   case 0x46:
      regs->itu.channel[0].tier = data;
      break;
   case 0x47:
      regs->itu.channel[0].tsr = data;
      break;
   case 0x48:
      regs->itu.channel[0].tcnt = (regs->itu.channel[0].tsr & 0xff) | (data << 8);
      break;
   case 0x49:
      regs->itu.channel[0].tcnt = (regs->itu.channel[0].tsr & 0xff00) | data;
      break;
   case 0x4a:
      regs->itu.channel[0].gra = (regs->itu.channel[0].gra & 0xff) | (data << 8);
      break;
   case 0x4b:
      regs->itu.channel[0].gra = (regs->itu.channel[0].gra & 0xff00) | data;
      break;
   case 0x4c:
      regs->itu.channel[0].grb = (regs->itu.channel[0].grb & 0xff) | (data << 8);
      break;
   case 0x4d:
      regs->itu.channel[0].grb = (regs->itu.channel[0].grb & 0xff00) | data;
      break;
   }
}
void onchip_write_byte(struct Onchip * regs, u32 addr, u8 data)
{
   addr -= 0x5FFFEC0;
   switch (addr)
   {
   case 0:
      regs->sci[0].smr = data;
      break;
   case 1:
      regs->sci[0].brr = data;
      break;
   case 2:
      regs->sci[0].scr = data;
      break;
   case 3:
      regs->sci[0].tdr = data;
      break;
   case 4:
      if(data == 0)//only 0 can be written
         regs->sci[0].ssr = data;
      break;
   case 5:
      //read only
      break;
   case 6:
      //nothing
      break;
   case 7:
      //nothing
      break;
   case 8:
      regs->sci[1].smr = data;
      break;
   case 9:
      regs->sci[1].brr = data;
      break;
   case 0xa:
      regs->sci[1].scr = data;
      break;
   case 0xb:
      regs->sci[1].tdr = data;
      break;
   case 0xc:
      if (data == 0)//only 0 can be written
         regs->sci[1].ssr = data;
      break;
   case 0xd:
      //read only
      break;
   case 0x40:
      regs->itu.tstr = data;
      break;
   case 0x41:
      regs->itu.tsnc = data;
      break;
   case 0x42:
      regs->itu.tmdr = data;
      break;
   case 0x43:
      regs->itu.tfcr = data;
      break;

   case 0x4e:
      regs->itu.channel[1].tcr = data;
      break;
   case 0x4f:
      regs->itu.channel[1].tior = data;
      break;
   case 0x50:
      regs->itu.channel[1].tcr = data;
      break;
   case 0xff://0x5FFFF31
      regs->itu.tocr = data;
      break;
   }
}

u8 onchip_read_byte(struct Onchip * regs, u32 addr)
{
   addr -= 0x5FFFEC0;
   switch (addr)
   {
   case 0:
      return regs->sci[0].smr;
      break;
   case 1:
      return regs->sci[0].brr;
      break;
   case 2:
      return regs->sci[0].scr;
      break;
   case 3:
      return regs->sci[0].tdr;
      break;
   case 4:
      return regs->sci[0].ssr;
      break;
   case 5:
      return regs->sci[0].rdr;
      break;
   case 6:
      //nothing
      break;
   case 7:
      //nothing
      break;
   case 8:
      return regs->sci[1].smr;
      break;
   case 9:
      return regs->sci[1].brr;
      break;
   case 0xa:
      return regs->sci[1].scr;
      break;
   case 0xb:
      return regs->sci[1].tdr;
      break;
   case 0xc:
      return regs->sci[1].ssr;
      break;
   case 0xd:
      return regs->sci[1].rdr;
      break;
   //a/d
   case 0x20:
      return (regs->addra & 0xff00) >> 8;
      break;
   case 0x21:
      return regs->addra & 0xff;
      break;
   case 0x22:
      return (regs->addrb & 0xff00) >> 8;
      break;
   case 0x23:
      return regs->addrb & 0xff;
      break;
   case 0x24:
      return (regs->addrc & 0xff00) >> 8;
      break;
   case 0x25:
      return regs->addrc & 0xff;
      break;
   case 0x26:
      return (regs->addrd & 0xff00) >> 8;
      break;
   case 0x27:
      return regs->addrd & 0xff;
      break;
   case 0x28:
      return regs->adcsr;
      break;
   case 0x29:
      return regs->adcr;
      break;
   }

   return 0;
}

void onchip_write_word(struct Onchip * regs, u32 addr, u16 data)
{
   addr -= 0x5FFFEC0;

   addr /= 2;
   switch (addr)
   {
   case 0:
      regs->sci[0].smr = (data >> 8) & 0xff;
      regs->sci[0].brr = data & 0xff;
      break;
   case 1:
      regs->sci[0].scr = (data >> 8) & 0xff;
      regs->sci[0].tdr = data & 0xff;
      break;
   case 2:
   {
      u8 temp = (data >> 8) & 0xff;
      if(!temp)
         regs->sci[0].ssr = temp;
      //read only
   }
      break;
   case 3:
      break;
   case 4:
      regs->sci[1].smr = (data >> 8) & 0xff;
      regs->sci[1].brr = data & 0xff;
      break;
   case 5:
      regs->sci[1].scr = (data >> 8) & 0xff;
      regs->sci[1].tdr = data & 0xff;
      break;
   case 6:
   {
      u8 temp = (data >> 8) & 0xff;
      if (!temp)
         regs->sci[1].ssr = temp;
      //read only
   }
   break;
   case 7:
      break;
   }
}

u16 onchip_read_word(struct Onchip * regs, u32 addr)
{
   addr -= 0x5FFFEC0;

   addr /= 2;
   switch (addr)
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

   return 0;
}

void onchip_write_long(struct Onchip * regs, u32 addr, u32 data)
{

}

u32 onchip_read_long(struct Onchip * regs, u32 addr)
{
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

void sh1_exec(struct Sh1 * sh1, s32 cycles)
{
   s32 cycles_temp = sh1->cycles_remainder - cycles;
   while (cycles_temp < 0)
   {
      int cycles = sh1_execute_instruction(sh1);
      cycles_temp += cycles;
   }
   sh1->cycles_remainder = cycles_temp;
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