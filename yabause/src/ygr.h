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

#ifndef YGR_H
#define YGR_H

#include "core.h"

u8 ygr_sh1_read_byte(u32 addr);
u16 ygr_sh1_read_word(u32 addr);
u32 ygr_sh1_read_long(u32 addr);

void ygr_sh1_write_byte(u32 addr, u8 data);
void ygr_sh1_write_word(u32 addr, u16 data);
void ygr_sh1_write_long(u32 addr, u32 data);

u16 FASTCALL ygr_a_bus_read_word(u32 addr);
u32 FASTCALL  ygr_a_bus_read_long(u32 addr);

void FASTCALL ygr_a_bus_write_word(u32 addr, u16 data);
void FASTCALL ygr_a_bus_write_long(u32 addr, u32 data);

u16 FASTCALL sh2_ygr_a_bus_read_word(SH2_struct * sh, u32 addr);
u32 FASTCALL  sh2_ygr_a_bus_read_long(SH2_struct * sh, u32 addr);

void FASTCALL sh2_ygr_a_bus_write_word(SH2_struct * sh, u32 addr, u16 data);
void FASTCALL sh2_ygr_a_bus_write_long(SH2_struct * sh, u32 addr, u32 data);

void ygr_cd_irq(u8 flags);
int sh2_a_bus_check_wait(u32 addr, int size);
int ygr_dreq_asserted();
int fifo_empty();
#endif
