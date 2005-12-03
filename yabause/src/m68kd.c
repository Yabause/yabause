/*  Copyright 2005 Theo Berkau

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

#include "core.h"

typedef struct
{
   u16 mask;
   u16 inst;
   const char *name;
   int size;
} m68kdis_struct;

static m68kdis_struct instruction[] = {
   { 0xF1F0, 0xC100, "abcd", 2 },
   { 0xF000, 0xD000, "adda", 2 },
   { 0xFF00, 0x0600, "addi", 2 },
   { 0xF100, 0x5000, "addq", 2 },
   { 0xF137, 0xD100, "addx", 2 },
   { 0xF000, 0xC000, "and", 2 },
   { 0xFF00, 0x0200, "andi", 2 },
   { 0xFFFF, 0x023C, "andi #??, CCR", 2 },
   { 0xFFC0, 0xE1C0, "asl", 2 },
   { 0xFFC0, 0xE0C0, "asr", 2 },
   { 0xF000, 0x6000, "bcc", 2 },
   { 0xF1C0, 0x0140, "bchg", 2 },
   { 0xF1C0, 0x0180, "bclr", 2 },
   { 0xFFF8, 0x4848, "bkpt", 2 },
   { 0xFF00, 0x6000, "bra", 2 },
   { 0xF1C0, 0x01C0, "bset", 2 },
   { 0xFF00, 0x6100, "bsr", 2 },
   { 0xF1C0, 0x0100, "btst", 2 },
   { 0xF040, 0x4000, "chk", 2 },
   { 0xFF00, 0x4200, "clr", 2 },
   { 0xF000, 0xB000, "cmp", 2 }, // fix me
   { 0xF000, 0xB000, "cmpa", 2 }, // fix me
   { 0xFF00, 0x0C00, "cmpi", 2 },
   { 0xF138, 0xB108, "cmpm", 2 },
   { 0xF0F8, 0x50C8, "dbcc", 2 },
   { 0xF1C0, 0x81C0, "divs.w", 2 },
   { 0xF1C0, 0x80C0, "divu.w", 2 },
   { 0xF000, 0xB000, "eor", 2 }, // fix me
   { 0xFF00, 0x0A00, "eori", 2 },
   { 0xFFFF, 0x0A3C, "eori #??, CCR", 2 },
   { 0xF100, 0xC100, "exg", 2 },
   { 0xFE38, 0x4800, "ext", 2 },
   { 0xFFFF, 0x4AFC, "illegal", 2 },
   { 0xFFC0, 0x4EC0, "jmp", 2 },
   { 0xFFC0, 0x4E80, "jsr", 2 },
   { 0xF1C0, 0x41C0, "lea", 2 },
   { 0xFFF8, 0x4E50, "link", 2 },
   { 0xF118, 0xE108, "lsl", 2 },
   { 0xF118, 0xE008, "lsr", 2 },
   { 0xC000, 0x0000, "move", 2 },
   { 0xC1C0, 0x0040, "movea", 2 },
   { 0xFFC0, 0x44C0, "move ??, CCR", 2 },
   { 0xFFC0, 0x40C0, "move SR, ??", 2 },
   { 0xFB80, 0x4880, "movem", 2 },
   { 0xF038, 0x0008, "movep", 2 },
   { 0xF100, 0x7000, "moveq", 2 },
   { 0xF1C0, 0xC1C0, "muls", 2 },
   { 0xF1C0, 0xC0C0, "mulu", 2 },
   { 0xFFC0, 0x4800, "nbcd", 2 },
   { 0xFF00, 0x4400, "neg", 2 },
   { 0xFF00, 0x4000, "negx", 2 },
   { 0xFFFF, 0x4E71, "nop", 2 },
   { 0xFF00, 0x4600, "not", 2 },
   { 0xF000, 0x8000, "or", 2 },
   { 0xFF00, 0x0000, "ori", 2 },
   { 0xFFFF, 0x003C, "ori #??, CCR", 2 },
   { 0xFFC0, 0x4840, "pea", 2 },
   { 0xF118, 0xE118, "rol", 2 },
   { 0xF118, 0xE018, "ror", 2 },
   { 0xF118, 0xE110, "roxl", 2 },
   { 0xF118, 0xE010, "roxr", 2 },
   { 0xFFFF, 0x4E77, "rtr", 2 },
   { 0xFFFF, 0x4E75, "rts", 2 },
   { 0xF1F0, 0x8100, "sbcd", 2 },
   { 0xF0C0, 0x50C0, "scc", 2 },
   { 0xF000, 0x9000, "sub", 2 }, // fix me
   { 0xF000, 0x9000, "suba", 2 }, // fix me
   { 0xFF00, 0x0400, "subi", 2 },
   { 0xF100, 0x5100, "subq", 2 },
   { 0xF130, 0x9100, "subx", 2 },
   { 0xFFF8, 0x4840, "swap", 2 },
   { 0xFFC0, 0x4AC0, "tas", 2 },
   { 0xFFF0, 0x4E40, "trap", 2 },
   { 0xFFFF, 0x4E76, "trapv", 2 },
   { 0xFF00, 0x4A00, "tst", 2 },
   { 0xFFF8, 0x4E58, "unlk", 2 },
   { 0x0000, 0x0000, NULL, 0 }
};

u32 FASTCALL c68k_word_read(const u32 adr);

//////////////////////////////////////////////////////////////////////////////

u32 M68KDisasm(u32 addr, char *outstring)
{
   int i;

   sprintf(outstring, "%05X: ", (unsigned int)addr);

   for (i = 0; instruction[i].name != NULL; i++)
   {
      u16 op = c68k_word_read(addr);

      // fix me
      if ((op & instruction[i].mask) == instruction[i].inst)
      {
         strcat(outstring, instruction[i].name);
         addr += instruction[i].size;
         return addr;
      }
   }

   strcat(outstring, "unknown");

   return addr;
}

//////////////////////////////////////////////////////////////////////////////
