/*  Copyright 2015 Theo Berkau

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

#include "scsp.h"
#include "scspdsp.h"

void ScspDspDisasm(u8 addr, char *outstring)
{
   union ScspDspInstruction instruction;

   instruction.all = scsp_dsp.mpro[addr];

   sprintf(outstring, "%02X: ", addr);
   outstring += strlen(outstring);

   if (instruction.all == 0)
   {
      sprintf(outstring, "nop ");
      outstring += strlen(outstring);
      return;
   }

   if (instruction.part.nofl)
   {
      sprintf(outstring, "nofl ");
      outstring += strlen(outstring);
   }

   if (instruction.part.coef)
   {
      sprintf(outstring, "coef %02X ", (unsigned int)(instruction.part.coef & 0x3F));
      outstring += strlen(outstring);
   }

   if (instruction.part.masa)
   {
      sprintf(outstring, "masa %02X ", (unsigned int)(instruction.part.masa & 0x1F));
      outstring += strlen(outstring);
   }

   if (instruction.part.adreb)
   {
      sprintf(outstring, "adreb ");
      outstring += strlen(outstring);
   }

   if (instruction.part.nxadr)
   {
      sprintf(outstring, "nxadr ");
      outstring += strlen(outstring);
   }

   if (instruction.part.table)
   {
      sprintf(outstring, "table ");
      outstring += strlen(outstring);
   }

   if (instruction.part.mwt)
   {
      sprintf(outstring, "mwt ");
      outstring += strlen(outstring);
   }

   if (instruction.part.mrd)
   {
      sprintf(outstring, "mrd ");
      outstring += strlen(outstring);
   }

   if (instruction.part.ewt)
   {
      sprintf(outstring, "ewt ");
      outstring += strlen(outstring);
   }

   if (instruction.part.ewa)
   {
      sprintf(outstring, "ewa %01X ", (unsigned int)(instruction.part.ewa & 0xf));
      outstring += strlen(outstring);
   }

   if (instruction.part.adrl)
   {
      sprintf(outstring, "adrl ");
      outstring += strlen(outstring);
   }

   if (instruction.part.frcl)
   {
      sprintf(outstring, "frcl ");
      outstring += strlen(outstring);
   }

   if (instruction.part.shift)
   {
      sprintf(outstring, "shift %d ", (int)(instruction.part.shift & 3));
      outstring += strlen(outstring);
   }

   if (instruction.part.yrl)
   {
      sprintf(outstring, "yrl ");
      outstring += strlen(outstring);
   }

   if (instruction.part.negb)
   {
      sprintf(outstring, "negb ");
      outstring += strlen(outstring);
   }

   if (instruction.part.zero)
   {
      sprintf(outstring, "zero ");
      outstring += strlen(outstring);
   }

   if (instruction.part.bsel)
   {
      sprintf(outstring, "bsel ");
      outstring += strlen(outstring);
   }

   if (instruction.part.xsel)
   {
      sprintf(outstring, "xsel ");
      outstring += strlen(outstring);
   }

   if (instruction.part.ysel)
   {
      sprintf(outstring, "ysel %d ", (int)(instruction.part.ysel & 3));
      outstring += strlen(outstring);
   }

   if (instruction.part.ira)
   {
      sprintf(outstring, "ira %02X ", (int)(instruction.part.ira & 0x3F));
      outstring += strlen(outstring);
   }

   if (instruction.part.iwt)
   {
      sprintf(outstring, "iwt ");
      outstring += strlen(outstring);
   }

   if (instruction.part.iwa)
   {
      sprintf(outstring, "iwa %02X ", (unsigned int)(instruction.part.iwa & 0x1F));
      outstring += strlen(outstring);
   }

   if (instruction.part.tra)
   {
      sprintf(outstring, "tra %02X ", (unsigned int)(instruction.part.tra & 0x7F));
      outstring += strlen(outstring);
   }

   if (instruction.part.twt)
   {
      sprintf(outstring, "twt ");
      outstring += strlen(outstring);
   }

   if (instruction.part.twa)
   {
      sprintf(outstring, "twa %02X ", (unsigned int)(instruction.part.twa & 0x7F));
      outstring += strlen(outstring);
   }

   if (instruction.part.unknown)
   {
      sprintf(outstring, "unknown ");
      outstring += strlen(outstring);
   }

   if (instruction.part.unknown2)
   {
      sprintf(outstring, "unknown2 ");
      outstring += strlen(outstring);
   }

   if (instruction.part.unknown3)
   {
      sprintf(outstring, "unknown3 %d", (int)(instruction.part.unknown3 & 3));
      outstring += strlen(outstring);
   }
}