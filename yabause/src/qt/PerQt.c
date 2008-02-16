/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau
	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>

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
#include "PerQt.h"

int PERQTInit(void);
void PERQTDeInit(void);
int PERQTHandleEvents(void);
void PERQTNothing(void);
#ifdef USENEWPERINTERFACE
PortData_struct *PERQTGetPerDataP1(void);
PortData_struct *PERQTGetPerDataP2(void);

static PortData_struct port1;
static PortData_struct port2;
u32 PERQTScan(const char* name);
void PERQTFlush(void);
#endif

PerInterface_struct PERQT = {
PERCORE_QT,
"Qt Keyboard Input Interface",
PERQTInit,
PERQTDeInit,
#ifndef USENEWPERINTERFACE
PERQTHandleEvents
#else
PERQTHandleEvents,
PERQTGetPerDataP1,
PERQTGetPerDataP2,
PERQTNothing,
PERQTScan,
0,
PERQTFlush
#endif
};

int PERQTInit(void)
{ return 0; }

void PERQTDeInit(void)
{}

void PERQTNothing(void)
{}

int PERQTHandleEvents(void)
{
	// need fix
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

#ifdef USENEWPERINTERFACE
PortData_struct* PERQTGetPerDataP1(void)
{
   // fix me, but this is the basic idea. Basically make sure the structure
   // is completely ready before you return
   port1.data[0] = 0xF1;
   port1.data[1] = 0x02;
   port1.data[2] = buttonbits >> 8;
   port1.data[3] = buttonbits & 0xFF;
   port1.size = 4;
   return &port1;
}

PortData_struct* PERQTGetPerDataP2(void)
{
   port2.data[0] = 0xF0;
   port2.size = 1;
   return &port2;
}

PerInfo_struct* PERQTGetList(void)
{
   // Returns a list of peripherals available along with information on each
   // peripheral
	return 0;
}

u32 PERQTScan(const char* name)
{ return 1; }

void PERQTFlush(void)
{}

#endif
