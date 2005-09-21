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

#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include "core.h"

#define PERCORE_DEFAULT -1
#define PERCORE_DUMMY 0

extern u16 buttonbits;

typedef struct
{
   int id;
   const char * Name;
   int (*Init)(void);
   void (*DeInit)(void);
   int (*HandleEvents)(void);
} PerInterface_struct;

extern PerInterface_struct * PERCore;

int PerInit(int coreid);
void PerDeInit(void);

void PerUpPressed(void);
void PerUpReleased(void);

void PerDownPressed(void);
void PerDownReleased(void);

void PerRightPressed(void);
void PerRightReleased(void);

void PerLeftPressed(void);
void PerLeftReleased(void);

void PerStartPressed(void);
void PerStartReleased(void);

void PerAPressed(void);
void PerAReleased(void);

void PerBPressed(void);
void PerBReleased(void);

void PerCPressed(void);
void PerCReleased(void);

void PerXPressed(void);
void PerXReleased(void);

void PerYPressed(void);
void PerYReleased(void);

void PerZPressed(void);
void PerZReleased(void);

void PerRTriggerPressed(void);
void PerRTriggerReleased(void);

void PerLTriggerPressed(void);
void PerLTriggerReleased(void);

#endif
