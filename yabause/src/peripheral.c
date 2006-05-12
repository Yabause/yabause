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

//////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////

void PerDeInit(void) {
   if (PERCore)
      PERCore->DeInit();
}

//////////////////////////////////////////////////////////////////////////////

void PerUpPressed(void) {
   buttonbits &= 0xEFFF;
   LOG("Up\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerUpReleased(void) {
   buttonbits |= ~0xEFFF;
}

//////////////////////////////////////////////////////////////////////////////

void PerDownPressed(void) {
   buttonbits &= 0xDFFF;
   LOG("Down\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerDownReleased(void) {
   buttonbits |= ~0xDFFF;
}

//////////////////////////////////////////////////////////////////////////////

void PerRightPressed(void) {
   buttonbits &= 0x7FFF;
   LOG("Right\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerRightReleased(void) {
   buttonbits |= ~0x7FFF;
}

//////////////////////////////////////////////////////////////////////////////

void PerLeftPressed(void) {
   buttonbits &= 0xBFFF;
   LOG("Left\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerLeftReleased(void) {
   buttonbits |= ~0xBFFF;
}

//////////////////////////////////////////////////////////////////////////////

void PerStartPressed(void) {
   buttonbits &= 0xF7FF;
   LOG("Start\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerStartReleased(void) {
   buttonbits |= ~0xF7FF;
}

//////////////////////////////////////////////////////////////////////////////

void PerAPressed(void) {
   buttonbits &= 0xFBFF;
   LOG("A\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerAReleased(void) {
   buttonbits |= ~0xFBFF;
}

//////////////////////////////////////////////////////////////////////////////

void PerBPressed(void) {
   buttonbits &= 0xFEFF;
   LOG("B\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerBReleased(void) {
   buttonbits |= ~0xFEFF;
}

//////////////////////////////////////////////////////////////////////////////

void PerCPressed(void) {
   buttonbits &= 0xFDFF;
   LOG("C\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerCReleased(void) {
   buttonbits |= ~0xFDFF;
}

//////////////////////////////////////////////////////////////////////////////

void PerXPressed(void) {
   buttonbits &= 0xFFBF;
   LOG("X\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerXReleased(void) {
   buttonbits |= ~0xFFBF;
}

//////////////////////////////////////////////////////////////////////////////

void PerYPressed(void) {
   buttonbits &= 0xFFDF;
   LOG("Y\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerYReleased(void) {
   buttonbits |= ~0xFFDF;
}

//////////////////////////////////////////////////////////////////////////////

void PerZPressed(void) {
   buttonbits &= 0xFFEF;
   LOG("Z\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerZReleased(void) {
   buttonbits |= ~0xFFEF;
}

//////////////////////////////////////////////////////////////////////////////

void PerRTriggerPressed(void) {
   buttonbits &= 0xFF7F;
   LOG("Right Trigger\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerRTriggerReleased(void) {
   buttonbits |= ~0xFF7F;
}

//////////////////////////////////////////////////////////////////////////////

void PerLTriggerPressed(void) {
   buttonbits &= 0xFFF7;
   LOG("Left Trigger\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerLTriggerReleased(void) {
   buttonbits |= ~0xFFF7;
}

#ifdef USENEWPERINTERFACE
//////////////////////////////////////////////////////////////////////////////

void PerAddPeripheral(PortData_struct *port, int perid, int addoffset)
{
   int pernum = port->data[0] & 0xF;
   int i;
   int peroffset=1;

   if (pernum == 0xF)
     return;

   // if only one peripheral is connected use 0xF0, otherwise use 0x00 or 0x10
   if (pernum == 0)
      port->data[0] = 0xF0 | (pernum + 1);
   else
      port->data[0] = 0x10 | (pernum + 1);

   // figure out where we're at, then add peripheral id
   for (i = 0; i < pernum; i++)
      peroffset += (port->data[peroffset] & 0xF);

   port->data[peroffset] = perid;
   peroffset++;

   // set peripheral data for peripheral to default values and adjust size
   // of port data
   switch (perid)
   {
      case 0x02:
         port->data[peroffset] = 0xFF;
         port->data[peroffset+1] = 0xFF;
         port->size = peroffset+2;
         break;
      default: break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void PerRemovePeripheral(PortData_struct *port, int removeoffset)
{
   // stub
}

//////////////////////////////////////////////////////////////////////////////

#endif

//////////////////////////////////////////////////////////////////////////////
// Dummy Interface
//////////////////////////////////////////////////////////////////////////////

int PERDummyInit(void);
void PERDummyDeInit(void);
int PERDummyHandleEvents(void);
void PERDummyNothing(void);
#ifdef USENEWPERINTERFACE
PortData_struct *PERDummyGetPerDataP1(void);
PortData_struct *PERDummyGetPerDataP2(void);

static PortData_struct port1;
static PortData_struct port2;
#endif

PerInterface_struct PERDummy = {
PERCORE_DUMMY,
"Dummy Interface",
PERDummyInit,
PERDummyDeInit,
#ifndef USENEWPERINTERFACE
PERDummyHandleEvents
#else
PERDummyHandleEvents,
PERDummyGetPerDataP1,
PERDummyGetPerDataP2,
PERDummyNothing
#endif
};

//////////////////////////////////////////////////////////////////////////////

int PERDummyInit(void) {
   PerUpReleased();
   PerDownReleased();
   PerRightReleased();
   PerLeftReleased();
   PerStartReleased();
   PerAReleased();
   PerBReleased();
   PerCReleased();
   PerXReleased();
   PerYReleased();
   PerZReleased();
   PerRTriggerReleased();
   PerLTriggerReleased();

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERDummyDeInit(void) {
}

//////////////////////////////////////////////////////////////////////////////

void PERDummyNothing(void) {
}

//////////////////////////////////////////////////////////////////////////////

int PERDummyHandleEvents(void) {
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

#ifdef USENEWPERINTERFACE
PortData_struct *PERDummyGetPerDataP1(void)
{
   return &port1;
}

//////////////////////////////////////////////////////////////////////////////

PortData_struct *PERDummyGetPerDataP2(void)
{
   return &port2;
}

//////////////////////////////////////////////////////////////////////////////

PerInfo_struct *PERDummyGetList(void)
{
   // Returns a list of peripherals available along with information on each
   // peripheral
}

//////////////////////////////////////////////////////////////////////////////

#endif
