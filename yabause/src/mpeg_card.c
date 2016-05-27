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

struct MpegCard
{
   u16 reg_02;
   u16 reg_1a;
   u16 reg_1e;
   u16 reg_20;
   u16 reg_80000;
}mpeg_card;

void mpeg_card_write_word(u32 addr, u16 data)
{
   switch (addr & 0xfffff)
   {
   case 2:
      mpeg_card.reg_02 = data;
      return;
   case 0x1a:
      mpeg_card.reg_1a = data;
      return;
   case 0x1e:
      mpeg_card.reg_1e = data;
      return;
   case 0x20:
      mpeg_card.reg_20 = data;
      return;
   case 0x80000:
      mpeg_card.reg_80000 = data;
      return;
   }
}

u16 mpeg_card_read_word(u32 addr)
{
   switch (addr & 0xfffff)
   {
   case 2:
      return mpeg_card.reg_02;
   }

   return 0;
}