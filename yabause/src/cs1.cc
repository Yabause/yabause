/*  Copyright 2003 Guillaume Duhamel
    Copyright 2005 Theo Berkau

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
#include "cs1.hh"
                                                
Cs1::Cs1(char *file, int type) : Dummy(0xFFFFFF) {
   switch (type) {
      case CART_PAR: // Action Replay, Rom carts?(may need to change this)
         cartid = 0x5C; // fix me
         sramarea = new Dummy(0xFFFFFE);
         break;
      case CART_BACKUPRAM4MBIT: // 4 Mbit Backup Ram
         cartid = 0x21;
         sramarea = new Memory(0xFFFFF,0x100000);
         try {
            sramarea->load(file, 0);
         }
         catch (Exception e)
         {
            // Should Probably format Ram here
         }
         break;
      case CART_BACKUPRAM8MBIT: // 8 Mbit Backup Ram
         cartid = 0x22;
         sramarea = new Memory(0x1FFFFF,0x200000);
         try {
            sramarea->load(file, 0);
         }
         catch (Exception e)
         {
            // Should Probably format Ram here
         }
         break;
      case CART_BACKUPRAM16MBIT: // 16 Mbit Backup Ram
         cartid = 0x23;
         sramarea = new Memory(0x3FFFFF,0x400000);
         try {
            sramarea->load(file, 0);
         }
         catch (Exception e)
         {
            // Should Probably format Ram here
         }
         break;
      case CART_BACKUPRAM32MBIT: // 32 Mbit Backup Ram
         cartid = 0x24;
         sramarea = new Memory(0x7FFFFF,0x800000);
         try {
            sramarea->load(file, 0);
         }
         catch (Exception e)
         {
            // Should Probably format Ram here
         }
         break;
      case CART_DRAM8MBIT: // 8 Mbit Dram Cart
         cartid = 0x5A;
         sramarea = new Dummy(0xFFFFFE);
         break;
      case CART_DRAM32MBIT: // 32 Mbit Dram Cart
         cartid = 0x5C;
         sramarea = new Dummy(0xFFFFFE);
         break;
      default: // No Cart
         cartid = 0xFF;
         sramarea = new Dummy(0xFFFFFE);
         break;
   }

   carttype = type;
   cartfile = file;
}

Cs1::~Cs1(void) {
   switch (carttype) {
      case CART_BACKUPRAM4MBIT: // 4 Mbit Backup Ram
         sramarea->save(cartfile, 0, 0x100000);
         break;
      case CART_BACKUPRAM8MBIT: // 8 Mbit Backup Ram
         sramarea->save(cartfile, 0, 0x200000);
         break;
      case CART_BACKUPRAM16MBIT: // 16 Mbit Backup Ram
         sramarea->save(cartfile, 0, 0x400000);
         break;
      case CART_BACKUPRAM32MBIT: // 32 Mbit Backup Ram
         sramarea->save(cartfile, 0, 0x800000);
         break;
      default:
         break;
   }

   delete sramarea;
}

unsigned char Cs1::getByte(unsigned long addr) {
   if (addr == 0xFFFFFF)
       return cartid;

   return sramarea->getByte(addr);
}

unsigned short Cs1::getWord(unsigned long addr) {
   if (addr == 0xFFFFFE)
      return (0xFF00 | cartid);

   return sramarea->getWord(addr);
}

unsigned long Cs1::getLong(unsigned long addr) {
   if (addr == 0xFFFFFC)
      return (0xFF00FF00 | (cartid << 16) | cartid);

   return sramarea->getLong(addr);
}

void Cs1::setByte(unsigned long addr, unsigned char val) {
   if (addr == 0xFFFFFF)
      return;
   sramarea->setByte(addr, val);
}

void Cs1::setWord(unsigned long addr, unsigned short val) {
   if (addr == 0xFFFFFF)
      return;
   sramarea->setWord(addr, val);
}

void Cs1::getLong(unsigned long addr, unsigned long val) {
   if (addr == 0xFFFFFF)
      return;
   sramarea->setLong(addr, val);
}

