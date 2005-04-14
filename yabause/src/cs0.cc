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
   switch (addr >> 20) {
      case 0x00:  
                 if ((addr & 0x80000) == 0) // EEPROM
                 {
                    biosarea->setByte(addr, val);
//                 fprintf(stderr, "EEPROM write to %08X = %02X\n", addr, val);
                 }
                 else // Outport
                 {   
//                 Memory::setByte(addr, val);
//                 fprintf(stderr, "Commlink Outport byte write\n");
                 }
                 break;
      case 0x01:
                 if ((addr & 0x80000) == 0) // Commlink Status flag
                 {
//                 Memory::setByte(addr, val);
//                 fprintf(stderr, "Commlink Status Flag write\n");
                 }
                 else // Inport for Commlink
                 {   
//                 Memory::setByte(addr, val);
//                 fprintf(stderr, "Commlink Inport Byte write\n");
                 }
                 break;
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
                 dramarea->setByte(addr, val);
                 break;
      default:   // The rest doesn't matter
                 break;
   }
}

unsigned char Cs0::getByte(unsigned long addr) {
   unsigned char val=0xFF;

   switch (addr >> 20) {
      case 0x00:  
                 if ((addr & 0x80000) == 0) // EEPROM
                 {
                    val = biosarea->getByte(addr);
//                    fprintf(stderr, "EEPROM read from %08X = %02X\n", addr, val);
                 }
                 else // Outport
                 {   
//                    val = Memory::getByte(addr);
//                    fprintf(stderr, "Commlink Outport Byte read\n");
                 }
                 break;
      case 0x01:
                 if ((addr & 0x80000) == 0) // Commlink Status flag
                 {
//                    val = Memory::getByte(addr);
//                    fprintf(stderr, "Commlink Status Flag read\n");
                 }
                 else // Inport for Commlink
                 {   
//                    val = Memory::getByte(addr);
//                    fprintf(stderr, "Commlink Inport Byte read\n");
                 }
                 break;
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
                 dramarea->setByte(addr, val);
                 val = biosarea->getByte(addr);
                 break;
//      case 0x12:
//      case 0x1E:
//                 break;
//      case 0x13:
//      case 0x16:
//      case 0x17:
//      case 0x1A:
//      case 0x1B:
//      case 0x1F:
//                 break;
      default:   // The rest doesn't matter
                 break;
   }

   return val;
}


void Cs0::setWord(unsigned long addr, unsigned short val) {
   switch (addr >> 20) {
      case 0x00:  
                 if ((addr & 0x80000) == 0) // EEPROM
                 {
                    biosarea->setWord(addr, val);
//                 fprintf(stderr, "EEPROM write to %08X = %04X\n", addr, val);
                 }
                 else // Outport
                 {   
//                 Memory::setWord(addr, val);
//                 fprintf(stderr, "Commlink Outport Word write\n");
                 }
                 break;
      case 0x01:
                 if ((addr & 0x80000) == 0) // Commlink Status flag
                 {
//                 Memory::setWord(addr, val);
//                 fprintf(stderr, "Commlink Status Flag write\n");
                 }
                 else // Inport for Commlink
                 {   
//                 Memory::setWord(addr, val);
//                 fprintf(stderr, "Commlink Inport Word write\n");
                 }
                 break;
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
                 dramarea->setWord(addr, val);
                 break;
      default:   // The rest doesn't matter
                 break;
   }

}

unsigned short Cs0::getWord(unsigned long addr) {
   unsigned short val=0xFFFF;

   switch (addr >> 20) {
      case 0x00:  
                 if ((addr & 0x80000) == 0) // EEPROM
                 {
                    val = biosarea->getWord(addr);
//                    fprintf(stderr, "EEPROM read from %08X = %04X\n", addr, val);
                 }
                 else // Outport
                 {   
//                    val = Memory::getWord(addr);
//                    fprintf(stderr, "Commlink Outport Word read\n");
                 }
                 break;
      case 0x01:
                 if ((addr & 0x80000) == 0) // Commlink Status flag
                 {
//                    val = Memory::getWord(addr);
//                    fprintf(stderr, "Commlink Status Flag read\n");
                 }
                 else // Inport for Commlink
                 {   
//                    val = Memory::getWord(addr);
//                    fprintf(stderr, "Commlink Inport Word read\n");
                 }
                 break;
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
                 val = dramarea->getWord(addr);
                 break;
      case 0x12:
      case 0x1E:
                 if (0x80000)
                    val = 0xFFFD;
                 break;
      case 0x13:
      case 0x16:
      case 0x17:
      case 0x1A:
      case 0x1B:
      case 0x1F:
                 val = 0xFFFD;
                 break;
      default:   // The rest doesn't matter
                 break;
   }

   return val;

}

void Cs0::setLong(unsigned long addr, unsigned long val) {
   switch (addr >> 20) {
      case 0x00:  
                 if ((addr & 0x80000) == 0) // EEPROM
                 {
                    biosarea->setLong(addr, val);
//                 fprintf(stderr, "EEPROM write to %08X = %08X\n", addr, val);
                 }
                 else // Outport
                 {   
//                 Memory::setLong(addr, val);
//                 fprintf(stderr, "Commlink Outport Long write\n");
                 }
                 break;
      case 0x01:
                 if ((addr & 0x80000) == 0) // Commlink Status flag
                 {
//                 Memory::setLong(addr, val);
//                 fprintf(stderr, "Commlink Status Flag write\n");
                 }
                 else // Inport for Commlink
                 {   
//                 Memory::setLong(addr, val);
//                 fprintf(stderr, "Commlink Inport Long write\n");
                 }
                 break;
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
                 dramarea->setLong(addr, val);
                 break;
      default:   // The rest doesn't matter
                 break;
   }
}

unsigned long Cs0::getLong(unsigned long addr) {
   unsigned long val=0xFFFFFFFF;

   switch (addr >> 20) {
      case 0x00:  
                 if ((addr & 0x80000) == 0) // EEPROM
                 {
                    val = biosarea->getLong(addr);
//                    fprintf(stderr, "EEPROM read from %08X = %08X\n", addr, val);
                 }
                 else // Outport
                 {   
//                    val = Memory::getLong(addr);
//                    fprintf(stderr, "Commlink Outport Long read\n");
                 }
                 break;
      case 0x01:
                 if ((addr & 0x80000) == 0) // Commlink Status flag
                 {
//                    val = Memory::getLong(addr);
//                    fprintf(stderr, "Commlink Status Flag read\n");
                 }
                 else // Inport for Commlink
                 {   
//                    val = Memory::getLong(addr);
//                    fprintf(stderr, "Commlink Inport Long read\n");
                 }
                 break;
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07: // Ram cart area
                 val = dramarea->getLong(addr);
                 break;
      case 0x12:
      case 0x1E:
                 if (0x80000)
                    val = 0xFFFDFFFD;
                 break;
      case 0x13:
      case 0x16:
      case 0x17:
      case 0x1A:
      case 0x1B:
      case 0x1F:
                 val = 0xFFFDFFFD;
                 break;
      default:   // The rest doesn't matter
                 break;
   }

   return val;
}

int Cs0::saveState(FILE *fp) {
   int offset;

   offset = stateWriteHeader(fp, "CS0 ", 1);

   // Write cart type
   fwrite((void *)&carttype, 4, 1, fp);

   // Write the areas associated with the cart type here

   return stateFinishHeader(fp, offset);
}

int Cs0::loadState(FILE *fp, int version, int size) {
   int oldtype = carttype;
   // Read cart type
   fread((void *)&carttype, 4, 1, fp);

   // Check to see if old cart type and new cart type match, if they don't,
   // reallocate memory areas

   // Read the areas associated with the cart type here

   return size;
}
