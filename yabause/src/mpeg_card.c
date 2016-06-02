/*  Copyright 2014-2016 James Laird-Wah
    Copyright 2004-2006, 2013 Theo Berkau

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

/*! \file mpeg_card.c
    \brief Mpeg card implementation
*/

#include "core.h"
#include "sh7034.h"
#include "debug.h"
#include "assert.h"

void mpeg_reg_debug_print();

//a100000
//write
//FEDC BA98 7654 3210
//.ccc .yyy iiii h.d.
//d : 0 == display on, 1 == display off
//h : 0 == HD, 1 == standard def (mpeg set mode)
//i : interpolation (set to opposite of command, 0xf - command_value)
//y : y gain
//c : c gain

//read
//FEDC BA98 7654 3210
//.... ...o .... fpdc
// c == video decoding
// d == display
// p == pause
// f == freeze
// o == output prep complete

//a100002
//FEDC BA98 7654 3210
//.... .... .... .ooo
//.... .... .ppp ppp.
//o : display window offset x (not sure how many bits)
//p: display window pos x (seems to occupy the same bits, modal?) (not sure how many bits)

//a100004
//FEDC BA98 7654 3210
//.... .... .... .ooo
//.... .... .ppp ppp.
//o : display window offset y
//p: display window pos y

//a100006

//when writing
//FEDC BA98 7654 3210
//.... ..xx xxxx xxx.
//x : frame buffer window pos x (not sure how many bits total)

//a100008
//FEDC BA98 7654 3210
//.... ..yy yyyy yyy.
//y : frame buffer window pos y

//a10000e
//FEDC BA98 7654 3210
//.... .... .... xxxx
//x : zoom rate x (not sure how many bits total)

//a100010
//FEDC BA98 7654 3210
//.... .... .... yyyy
//y : zoom rate y (not sure how many bits total)

//a100012
//FEDC BA98 7654 3210
//cccc cccc cccc cccc
//c : border color (rgb555?)

//a100014
//FEDC BA98 7654 3210
//.... .... .s.. fpot
//t : 0 == decode timing mode is host, 1 == decode timing mode is vsync
//o : 0 == output to host, 1 == output to vdp2
//p : seems pause related. writing 0 in cr2 of set decoding method sets it to 0
//f : seems freeze related. write 0 in cr4 of set decoding method to set it to 0
//s : 0 == still picture, 1 == video (mpeg set mode)

//a10001c
//FEDC BA98 7654 3210
//hhhh wwww ..b. ....
//b : blur on/off (todo recheck)
//w : mosaic width 0xf == off, 0-9 settings
//h : mosaic height 0xf == off, 0-9 settings

//a100022
//FEDC BA98 7654 3210
//fff. .... .... ....
//f : frame type (sent in status info)
//    1 : i frame
//    2 : p frame
//    3 : b frame
//    4 : d frame

//a10003e
//FEDC BA98 7654 3210
//..i. .... .... ....
// i : 0 == not interlaced 1 == interlaced

struct MpegCard
{
   u16 reg_00;
   u16 reg_02;
   u16 window_x;//0x06
   u16 window_y;//0x08
   u16 border_color;//0x12
   u16 reg_14;
   u16 reg_1a;
   u16 mosaic_blur;//0x1c
   u16 reg_1e;
   u16 reg_20;
   u16 reg_32;
   u16 reg_34;
   u16 reg_80000;
   u16 reg_80008;
   
   u16 ram[0x40000];
   u32 ram_ptr;
}mpeg_card;

#define RAM_MASK 0x3ffff

void mpeg_card_write_word(u32 addr, u16 data)
{
   if ((addr & 0xfffff) != 0x34 && (addr & 0xfffff) != 0x36)
   {
      CDLOG("mpeg lsi ww %08x, %04x\n", addr, data);
      //mpeg_reg_debug_print();
   }
   switch (addr & 0xfffff)
   {
   case 0:
      mpeg_card.reg_00 = data;
      return;
   case 2:
      mpeg_card.reg_02 = data;
      return;
   case 6:
      mpeg_card.window_x = data;
      return;
   case 8:
      mpeg_card.window_y = data;
      return;
   case 0x12:
      mpeg_card.border_color = data;
      return;
   case 0x14:
      mpeg_card.reg_14 = data;
      return;
   case 0x1a:
      mpeg_card.reg_1a = data;
      return;
   case 0x1c:
      mpeg_card.mosaic_blur = data;
      return;
   case 0x1e:
      mpeg_card.reg_1e = data;
      return;
   case 0x20:
      mpeg_card.reg_20 = data;
      return;
   case 0x30:
      mpeg_card.ram_ptr = data << 2;
      return;
   case 0x32:
      mpeg_card.reg_32 = data;
      return;
   case 0x34:
      mpeg_card.reg_34 = data;
      return;
   case 0x36:
      mpeg_card.ram_ptr &= RAM_MASK;
      mpeg_card.ram[mpeg_card.ram_ptr++] = data;
      return;
   case 0x80000:
      mpeg_card.reg_80000 = data;
      return;
   case 0x80008:
      mpeg_card.reg_80008 = data;
      return;
   }

   assert(0);
}

u16 mpeg_card_read_word(u32 addr)
{
   if ((addr & 0xfffff) != 0x34 && (addr & 0xfffff) != 0x36)
      CDLOG("mpeg lsi rw %08x %08x\n", addr, SH1->regs.PC);
   switch (addr & 0xfffff)
   {
   case 0:
      return mpeg_card.reg_00;
   case 2:
      return mpeg_card.reg_02;
   case 0x34:
      return 1;//return mpeg_card.reg_34 | 5;//first bit is always set?
   case 0x36:
      mpeg_card.ram_ptr &= RAM_MASK;
      return mpeg_card.ram[mpeg_card.ram_ptr++];
   case 0x80000:
      return mpeg_card.reg_80000;
   case 0x80008:
      return mpeg_card.reg_80008;
   }
   assert(0);

   return 0;
}


void set_mpeg_audio_data_transfer_irq()
{
   sh1_assert_tioca(0);
}

void set_mpeg_video_data_transfer_irq()
{
   sh1_assert_tioca(1);
}

void set_mpeg_audio_irq()
{
   sh1_assert_tioca(2);
}

void set_mpeg_video_irq()
{
   sh1_assert_tiocb(2);
}

void mpeg_card_set_all_irqs()
{
   set_mpeg_audio_data_transfer_irq();
   set_mpeg_video_data_transfer_irq();

   //triggers jump to null pointer, get hardware info shows mpeg present without it
   //set_mpeg_audio_irq();
   set_mpeg_video_irq();
}

void mpeg_card_init()
{
   memset(&mpeg_card, 0, sizeof(struct MpegCard));
   mpeg_card.reg_34 = 1;
}

void mpeg_reg_debug_print()
{
   if((mpeg_card.reg_00 >> 1) & 1)
      CDLOG("Display disabled\n");
   else
      CDLOG("Display enabled\n");

   CDLOG("Interpolation %01X\n", 0xf - ((mpeg_card.reg_00 >> 4) & 0xf));

   CDLOG("Window x %d\n", mpeg_card.window_x >> 1);

   CDLOG("Window y %d\n", mpeg_card.window_y >> 1);

   CDLOG("Mosaic x %d\n", (mpeg_card.mosaic_blur >> 8) & 0xf);

   CDLOG("Mosaic y %d\n", (mpeg_card.mosaic_blur >> 12) & 0xf);
   
   if ((mpeg_card.mosaic_blur >> 5) & 1)
      CDLOG("Blur disabled\n");
   else
      CDLOG("Blur enabled\n");
}