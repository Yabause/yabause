/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau

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

#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include "core.h"
#include "smpc.h"
#include "yabause.h"

#define PERCORE_DEFAULT -1
#define PERCORE_DUMMY 0

#define PERPAD_UP	0
#define PERPAD_RIGHT	1
#define PERPAD_DOWN	2
#define PERPAD_LEFT	3
#define PERPAD_RIGHT_TRIGGER 4
#define PERPAD_LEFT_TRIGGER 5
#define PERPAD_START	6
#define PERPAD_A	7
#define PERPAD_B	8
#define PERPAD_C	9
#define PERPAD_X	10
#define PERPAD_Y	11
#define PERPAD_Z	12

extern PortData_struct PORTDATA1;
extern PortData_struct PORTDATA2;

typedef struct
{
   int id;
   const char * Name;
   int (*Init)(void);
   void (*DeInit)(void);
   int (*HandleEvents)(void);
   void (*PerSetButtonMapping)();
   u32 (*Scan)(const char *);
   int canScan;
   void (*Flush)(void);
} PerInterface_struct;

extern PerInterface_struct * PERCore;

typedef struct
{
   u8 perid;
   u8 padbits[2];
} PerPad_struct;

typedef struct
{
   u8 perid;
   u8 mousebits[3];
} PerMouse_struct;

extern PerInterface_struct PERDummy;

int PerInit(int coreid);
void PerDeInit(void);

/* port related functions */
void * PerAddPeripheral(PortData_struct *port, int perid);
void PerPortReset(void);

/* pad related functions */
void PerPadUpPressed(PerPad_struct * pad);
void PerPadUpReleased(PerPad_struct * pad);

void PerPadDownPressed(PerPad_struct * pad);
void PerPadDownReleased(PerPad_struct * pad);

void PerPadRightPressed(PerPad_struct * pad);
void PerPadRightReleased(PerPad_struct * pad);

void PerPadLeftPressed(PerPad_struct * pad);
void PerPadLeftReleased(PerPad_struct * pad);

void PerPadStartPressed(PerPad_struct * pad);
void PerPadStartReleased(PerPad_struct * pad);

void PerPadAPressed(PerPad_struct * pad);
void PerPadAReleased(PerPad_struct * pad);

void PerPadBPressed(PerPad_struct * pad);
void PerPadBReleased(PerPad_struct * pad);

void PerPadCPressed(PerPad_struct * pad);
void PerPadCReleased(PerPad_struct * pad);

void PerPadXPressed(PerPad_struct * pad);
void PerPadXReleased(PerPad_struct * pad);

void PerPadYPressed(PerPad_struct * pad);
void PerPadYReleased(PerPad_struct * pad);

void PerPadZPressed(PerPad_struct * pad);
void PerPadZReleased(PerPad_struct * pad);

void PerPadRTriggerPressed(PerPad_struct * pad);
void PerPadRTriggerReleased(PerPad_struct * pad);

void PerPadLTriggerPressed(PerPad_struct * pad);
void PerPadLTriggerReleased(PerPad_struct * pad);

void PerKeyDown(u32);
void PerKeyUp(u32);
void PerSetKey(u32, u8, PerPad_struct * pad);
PerPad_struct * PerPadAdd(PortData_struct * port);

PerMouse_struct * PerMouseAdd(PortData_struct * port);

#endif
