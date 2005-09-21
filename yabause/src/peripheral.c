/*  Copyright 2003 Guillaume Duhamel
    Copyright 2004-2005 Theo Berkau

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

#include "debug.h"
#include "peripheral.h"

u16 buttonbits = 0xFFFF;

PerInterface_struct * PERCore = NULL;
extern PerInterface_struct * PERCoreList[];

int PerInit(int coreid) {
   int i;

   // So which core do we want?
   if (coreid == PERCORE_DEFAULT)
      coreid = 0; // Assume we want the first one

   // Go through core list and find the id
   for (i = 0; PERCoreList[i] != NULL; i++)
   {
      if (PERCoreList[i]->id == coreid)
      {
         // Set to current core
         PERCore = PERCoreList[i];
         break;
      }
   }

   if (PERCore == NULL)
      return -1;

   if (PERCore->Init() != 0)
      return -1;

   return 0;
}

void PerDeInit(void) {
   if (PERCore)
      PERCore->DeInit();
}

void PerUpPressed(void) {
   buttonbits &= 0xEFFF;
   LOG("Up\n");
}

void PerUpReleased(void) {
   buttonbits |= ~0xEFFF;
}

void PerDownPressed(void) {
   buttonbits &= 0xDFFF;
   LOG("Down\n");
}

void PerDownReleased(void) {
   buttonbits |= ~0xDFFF;
}

void PerRightPressed(void) {
   buttonbits &= 0x7FFF;
   LOG("Right\n");
}

void PerRightReleased(void) {
   buttonbits |= ~0x7FFF;
}

void PerLeftPressed(void) {
   buttonbits &= 0xBFFF;
   LOG("Left\n");
}

void PerLeftReleased(void) {
   buttonbits |= ~0xBFFF;
}

void PerStartPressed(void) {
   buttonbits &= 0xF7FF;
   LOG("Start\n");
}

void PerStartReleased(void) {
   buttonbits |= ~0xF7FF;
}

void PerAPressed(void) {
   buttonbits &= 0xFBFF;
   LOG("A\n");
}

void PerAReleased(void) {
   buttonbits |= ~0xFBFF;
}

void PerBPressed(void) {
   buttonbits &= 0xFEFF;
   LOG("B\n");
}

void PerBReleased(void) {
   buttonbits |= ~0xFEFF;
}

void PerCPressed(void) {
   buttonbits &= 0xFDFF;
   LOG("C\n");
}

void PerCReleased(void) {
   buttonbits |= ~0xFDFF;
}

void PerXPressed(void) {
   buttonbits &= 0xFFBF;
   LOG("X\n");
}

void PerXReleased(void) {
   buttonbits |= ~0xFFBF;
}

void PerYPressed(void) {
   buttonbits &= 0xFFDF;
   LOG("Y\n");
}

void PerYReleased(void) {
   buttonbits |= ~0xFFDF;
}

void PerZPressed(void) {
   buttonbits &= 0xFFEF;
   LOG("Z\n");
}

void PerZReleased(void) {
   buttonbits |= ~0xFFEF;
}

void PerRTriggerPressed(void) {
   buttonbits &= 0xFF7F;
   LOG("Right Trigger\n");
}

void PerRTriggerReleased(void) {
   buttonbits |= ~0xFF7F;
}

void PerLTriggerPressed(void) {
   buttonbits &= 0xFFF7;
   LOG("Left Trigger\n");
}

void PerLTriggerReleased(void) {
   buttonbits |= ~0xFFF7;
}
