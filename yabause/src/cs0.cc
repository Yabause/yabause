/*  Copyright 2004-2005 Theo Berkau

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

#include "cs0.hh"


Cs0::Cs0(char *file, int type) : Dummy(0x1FFFFFF) {
   switch (type) {
      case CART_PAR: // Action Replay, Rom carts?(may need to change this)
         // allocate memory for Cart
         biosarea = new Memory(0x3FFFF,0x40000);
         biosarea->load(file, 0);
         dramarea = new Dummy(0x400000);
         break;
      case CART_DRAM32MBIT: // 32 Mbit Dram Cart
         // Allocate memory for dram(fix me)
         biosarea = new Dummy(0x40000); // may need to change this
         dramarea = new Memory(0x3FFFFF,0x400000);
         break;
      case CART_DRAM8MBIT: // 8 Mbit Dram Cart
         // Allocate memory for dram
         biosarea = new Dummy(0x40000); // may need to change this
         dramarea = new Memory(0xFFFFF,0x100000);
         break;
      default: // No Cart
         biosarea = new Dummy(0x40000); // may need to change this
         dramarea = new Dummy(0x400000);
         break;
   }

   carttype = type;
}

Cs0::~Cs0() {
   delete biosarea;
   delete dramarea;
}

void Cs0::setByte(unsigned long addr, unsigned char val) {

   switch (addr >> 16) {
      case 0x000:
      case 0x001:
      case 0x002:
      case 0x003:
      case 0x004:
      case 0x005:
      case 0x006:
      case 0x007: // EEPROM
//                 Memory::setByte(addr, val);                 
                 biosarea->setByte(addr, val);
//                 fprintf(stderr, "EEPROM write to %08X = %02X\n", addr, val);
                 break;
      case 0x008:
      case 0x009:
      case 0x00A:
      case 0x00B:
      case 0x00C:
      case 0x00D:
      case 0x00E:
      case 0x00F: // Outport for Commlink
//                 Memory::setByte(addr, val);
//                 fprintf(stderr, "Commlink Outport byte write\n");
                 break;
      case 0x010:
      case 0x011:
      case 0x012:
      case 0x013:
      case 0x014:
      case 0x015:
      case 0x016:
      case 0x017: // Commlink Status flag
//                 Memory::setByte(addr, val);
//                 fprintf(stderr, "Commlink Status Flag write\n");
                 break;
      case 0x018:
      case 0x019:
      case 0x01A:
      case 0x01B:
      case 0x01C:
      case 0x01D:
      case 0x01E:
      case 0x01F: // Inport for Commlink
//                 Memory::setByte(addr, val);
//                 fprintf(stderr, "Commlink Inport Byte write\n");
                 break;
      default:
//                 Memory::setByte(addr, val);
#if DEBUG
                 fprintf(stderr, "CS0 Unsupported Byte write to %08X\n", addr);
#endif
                 break;
   }
}

unsigned char Cs0::getByte(unsigned long addr) {
   unsigned char val=0xFF;

   switch (addr >> 16) {
      case 0x00:
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // EEPROM
                 val = biosarea->getByte(addr);
//                 fprintf(stderr, "EEPROM read from %08X = %02X\n", addr, val);
                 break;
      case 0x08:
      case 0x09:
      case 0x0A:
      case 0x0B:
      case 0x0C:
      case 0x0D:
      case 0x0E:
      case 0x0F: // Outport for Commlink
//                 val = Memory::getByte(addr);
//                 fprintf(stderr, "Commlink Outport Byte read\n");
                 break;
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17: // Commlink Status flag
//                 val = Memory::getByte(addr);
//                 fprintf(stderr, "Commlink Status Flag read\n");
                 break;
      case 0x18:
      case 0x19:
      case 0x1A:
      case 0x1B:
      case 0x1C:
      case 0x1D:
      case 0x1E:
      case 0x1F: // Inport for Commlink
//                 val = Memory::getByte(addr);
//                 fprintf(stderr, "Commlink Inport Byte read\n");
                 break;
      default:
//                 val = Memory::getByte(addr);
#if DEBUG
                 fprintf(stderr, "CS0 Unsupported Byte read from %08X\n", addr);
#endif
                 break;
   }

   return val;
}


void Cs0::setWord(unsigned long addr, unsigned short val) {
#if DEBUG
   fprintf(stderr, "CS0 Unsupported Word Write to %08X\n", addr);
#endif
}

unsigned short Cs0::getWord(unsigned long addr) {
   unsigned short val=0xFFFF;

   switch (addr >> 16) {
      case 0x00:
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // EEPROM
                 val = biosarea->getWord(addr);                 
//                 fprintf(stderr, "EEPROM read from %08X = %04X\n", addr, val);
                 break;
      case 0x08:
      case 0x09:
      case 0x0A:
      case 0x0B:
      case 0x0C:
      case 0x0D:
      case 0x0E:
      case 0x0F: // Outport for Commlink
//                 val = Memory::getWord(addr);
//                 fprintf(stderr, "Commlink Outport Word read\n");
                 break;
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17: // Commlink Status flag
//                 val = Memory::getWord(addr);
//                 fprintf(stderr, "Commlink Status Flag read\n");
                 break;
      case 0x18:
      case 0x19:
      case 0x1A:
      case 0x1B:
      case 0x1C:
      case 0x1D:
      case 0x1E:
      case 0x1F: // Inport for Commlink
//                 val = Memory::getWord(addr);
//                 fprintf(stderr, "Commlink Inport Word read\n");
                 break;
      default:
//                 val = Memory::getWord(addr);
#if DEBUG
                 fprintf(stderr, "CS0 Unsupported Word read from %08X\n", addr);
#endif
                 break;
   }

   return val;

}

void Cs0::setLong(unsigned long addr, unsigned long val) {
#if DEBUG
   fprintf(stderr, "CS0 Unsupported Long Write to %08X\n", addr);
#endif
}

unsigned long Cs0::getLong(unsigned long addr) {
   unsigned long val=0xFFFFFFFF;

   switch (addr >> 16) {
      case 0x00:
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // EEPROM
                 val = biosarea->getLong(addr);
                 break;
      case 0x08:
      case 0x09:
      case 0x0A:
      case 0x0B:
      case 0x0C:
      case 0x0D:
      case 0x0E:
      case 0x0F: // Outport for Commlink
//                 val = Memory::getLong(addr);
//                 fprintf(stderr, "Commlink Outport Long read\n");
                 break;
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17: // Commlink Status flag
//                 val = Memory::getLong(addr);
//                 fprintf(stderr, "Commlink Status Flag read\n");
                 break;
      case 0x18:
      case 0x19:
      case 0x1A:
      case 0x1B:
      case 0x1C:
      case 0x1D:
      case 0x1E:
      case 0x1F: // Inport for Commlink
//                 val = Memory::getLong(addr);
//                 fprintf(stderr, "Commlink Inport Long read\n");
                 break;
      default:
//                 val = Memory::getLong(addr);
#if DEBUG
                 fprintf(stderr, "CS0 Unsupported Long read from %08X\n", addr);
#endif
//                 if (addr == 0x01380000)// wrong
//                    val = 0xFFFDFFFD;
                 break;
   }

   return val;
}

