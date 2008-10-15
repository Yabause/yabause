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

#include "debug.h"
#include "peripheral.h"

PortData_struct PORTDATA1;
PortData_struct PORTDATA2;

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
   PERCore = NULL;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadUpPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xEF;
   LOG("Up\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadUpReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xEF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadDownPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xDF;
   LOG("Down\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadDownReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xDF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadRightPressed(PerPad_struct * pad) {
   *pad->padbits &= 0x7F;
   LOG("Right\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadRightReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0x7F;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadLeftPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xBF;
   LOG("Left\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadLeftReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xBF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadStartPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xF7;
   LOG("Start\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadStartReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xF7;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadAPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xFB;
   LOG("A\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadAReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xFB;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadBPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xFE;
   LOG("B\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadBReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xFE;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadCPressed(PerPad_struct * pad) {
   *pad->padbits &= 0xFD;
   LOG("C\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadCReleased(PerPad_struct * pad) {
   *pad->padbits |= ~0xFD;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadXPressed(PerPad_struct * pad) {
   *(pad->padbits + 1) &= 0xBF;
   LOG("X\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadXReleased(PerPad_struct * pad) {
   *(pad->padbits + 1) |= ~0xBF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadYPressed(PerPad_struct * pad) {
   *(pad->padbits + 1) &= 0xDF;
   LOG("Y\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadYReleased(PerPad_struct * pad) {
   *(pad->padbits + 1) |= ~0xDF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadZPressed(PerPad_struct * pad) {
   *(pad->padbits + 1) &= 0xEF;
   LOG("Z\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadZReleased(PerPad_struct * pad) {
   *(pad->padbits + 1) |= ~0xEF;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadRTriggerPressed(PerPad_struct * pad) {
   *(pad->padbits + 1) &= 0x7F;
   LOG("Right Trigger\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadRTriggerReleased(PerPad_struct * pad) {
   *(pad->padbits + 1) |= ~0x7F;
}

//////////////////////////////////////////////////////////////////////////////

void PerPadLTriggerPressed(PerPad_struct * pad) {
   *(pad->padbits + 1) &= 0xF7;
   LOG("Left Trigger\n");
}

//////////////////////////////////////////////////////////////////////////////

void PerPadLTriggerReleased(PerPad_struct * pad) {
   *(pad->padbits + 1) |= ~0xF7;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseLeftPressed(PerMouse_struct * mouse) {
   *(mouse->mousebits) |= 1;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseLeftReleased(PerMouse_struct * mouse) {
   *(mouse->mousebits) &= 0xFFFE;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseMiddlePressed(PerMouse_struct * mouse) {
   *(mouse->mousebits) |= 4;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseMiddleReleased(PerMouse_struct * mouse) {
   *(mouse->mousebits) &= 0xFFFB;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseRightPressed(PerMouse_struct * mouse) {
   *(mouse->mousebits) |= 2;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseRightReleased(PerMouse_struct * mouse) {
   *(mouse->mousebits) &= 0xFFFD;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseStartPressed(PerMouse_struct * mouse) {
   *(mouse->mousebits) |= 8;
}

//////////////////////////////////////////////////////////////////////////////

void PerMouseStartReleased(PerMouse_struct * mouse) {
   *(mouse->mousebits) &= 0xFFF7;
}

//////////////////////////////////////////////////////////////////////////////

void * PerAddPeripheral(PortData_struct *port, int perid)
{
   int pernum = port->data[0] & 0xF;
   int i;
   int peroffset=1;
   u8 size;
   int current = 1;

   if (pernum == 0xF)
     return NULL;

   // if only one peripheral is connected use 0xF0, otherwise use 0x00 or 0x10
   if (pernum == 0)
   {
      pernum = 1;
      port->data[0] = 0xF1;
   }
   else
   {
      if (pernum == 1)
      {
         u8 tmp = peroffset;
         tmp += (port->data[peroffset] & 0xF) + 1;

         for(i = 0;i < 5;i++)
            port->data[tmp + i] = 0xFF;
      }
      pernum = 6;
      port->data[0] = 0x16;

      // figure out where we're at, then add peripheral id + 1
      current = 0;
      size = port->data[peroffset] & 0xF;
      while ((current < pernum) && (size != 0xF))
      {
         peroffset += size + 1;
         current++;
         size = port->data[peroffset] & 0xF;
      }

      if (current == pernum)
      {
         return NULL;
      }
      current++;
   }

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
      case 0xE3:
         port->data[peroffset] = 0;
         port->data[peroffset + 1] = 0;
         port->data[peroffset + 2] = 0;
         port->size = peroffset + 3;
         break;
      default: break;
   }

   {
      u8 tmp = peroffset;
      tmp += (perid & 0xF);
      for(i = 0;i < (pernum - current);i++)
      {
         port->data[tmp + i] = 0xFF;
      }
   }

   return (port->data + (peroffset - 1));
}

//////////////////////////////////////////////////////////////////////////////

void PerRemovePeripheral(PortData_struct *port, int removeoffset)
{
   // stub
}

//////////////////////////////////////////////////////////////////////////////

typedef struct {
	u8 name;
	void (*Press)(void *);
	void (*Release)(void *);
} PerBaseConfig_struct;

typedef struct {
	u32 key;
	PerBaseConfig_struct * base;
	void * controller;
} PerConfig_struct;

#define PERCALLBACK(func) ((void (*) (void *)) func)

PerBaseConfig_struct perkeybaseconfig[] = {
	{ PERPAD_UP, PERCALLBACK(PerPadUpPressed), PERCALLBACK(PerPadUpReleased) },
	{ PERPAD_RIGHT, PERCALLBACK(PerPadRightPressed), PERCALLBACK(PerPadRightReleased) },
	{ PERPAD_DOWN, PERCALLBACK(PerPadDownPressed), PERCALLBACK(PerPadDownReleased) },
	{ PERPAD_LEFT, PERCALLBACK(PerPadLeftPressed), PERCALLBACK(PerPadLeftReleased) },
	{ PERPAD_RIGHT_TRIGGER, PERCALLBACK(PerPadRTriggerPressed), PERCALLBACK(PerPadRTriggerReleased) },
	{ PERPAD_LEFT_TRIGGER, PERCALLBACK(PerPadLTriggerPressed), PERCALLBACK(PerPadLTriggerReleased) },
	{ PERPAD_START, PERCALLBACK(PerPadStartPressed), PERCALLBACK(PerPadStartReleased) },
	{ PERPAD_A, PERCALLBACK(PerPadAPressed), PERCALLBACK(PerPadAReleased) },
	{ PERPAD_B, PERCALLBACK(PerPadBPressed), PERCALLBACK(PerPadBReleased) },
        { PERPAD_C, PERCALLBACK(PerPadCPressed), PERCALLBACK(PerPadCReleased) },
	{ PERPAD_X, PERCALLBACK(PerPadXPressed), PERCALLBACK(PerPadXReleased) },
	{ PERPAD_Y, PERCALLBACK(PerPadYPressed), PERCALLBACK(PerPadYReleased) },
	{ PERPAD_Z, PERCALLBACK(PerPadZPressed), PERCALLBACK(PerPadZReleased) },
};

PerBaseConfig_struct permousebaseconfig[] = {
	{ PERMOUSE_LEFT, PERCALLBACK(PerMouseLeftPressed), PERCALLBACK(PerMouseLeftReleased) },
	{ PERMOUSE_MIDDLE, PERCALLBACK(PerMouseMiddlePressed), PERCALLBACK(PerMouseMiddleReleased) },
	{ PERMOUSE_RIGHT, PERCALLBACK(PerMouseRightPressed), PERCALLBACK(PerMouseRightReleased) },
	{ PERMOUSE_START, PERCALLBACK(PerMouseStartPressed), PERCALLBACK(PerMouseStartReleased) },
};

static u32 perkeyconfigsize = 0;
static PerConfig_struct * perkeyconfig = NULL;

void PerKeyDown(u32 key)
{
	int i = 0;

	while(i < perkeyconfigsize)
	{
		if (key == perkeyconfig[i].key)
		{
			perkeyconfig[i].base->Press(perkeyconfig[i].controller);
		}
		i++;
	}
}

void PerKeyUp(u32 key)
{
	int i = 0;

	while(i < perkeyconfigsize)
	{
		if (key == perkeyconfig[i].key)
		{
			perkeyconfig[i].base->Release(perkeyconfig[i].controller);
		}
		i++;
	}
}

void PerSetKey(u32 key, u8 name, PerPad_struct * pad)
{
	int i = 0;

	while(i < perkeyconfigsize)
	{
		if ((name == perkeyconfig[i].base->name) && (pad == perkeyconfig[i].controller))
		{
			perkeyconfig[i].key = key;
		}
		i++;
	}
}

void PerPortReset(void)
{
        PORTDATA1.data[0] = 0xF0;
        PORTDATA1.size = 1;
        PORTDATA2.data[0] = 0xF0;
        PORTDATA2.size = 1;

	perkeyconfigsize = 0;
        if (perkeyconfig)
           free(perkeyconfig);
	perkeyconfig = NULL;
}

PerPad_struct * PerPadAdd(PortData_struct * port)
{
	u32 oldsize = perkeyconfigsize;
	u32 i, j;
	PerPad_struct * pad = PerAddPeripheral(port, 0x02);

	if (!pad) return NULL;

	perkeyconfigsize += 13;
	perkeyconfig = realloc(perkeyconfig, perkeyconfigsize * sizeof(PerConfig_struct));
	j = 0;
	for(i = oldsize;i < perkeyconfigsize;i++)
	{
		perkeyconfig[i].base = perkeybaseconfig + j;
		perkeyconfig[i].controller = pad;
		j++;
	}

	return pad;
}

PerMouse_struct * PerMouseAdd(PortData_struct * port)
{
   u32 oldsize = perkeyconfigsize;
   u32 i, j;
   PerMouse_struct * mouse = PerAddPeripheral(port, 0xE3);

   if (!mouse) return NULL;

   perkeyconfigsize += 4;
   perkeyconfig = realloc(perkeyconfig, perkeyconfigsize * sizeof(PerConfig_struct));
   j = 0;
   for(i = oldsize;i < perkeyconfigsize;i++)
   {
      perkeyconfig[i].base = permousebaseconfig + j;
      perkeyconfig[i].controller = mouse;
      j++;
   }

   return mouse;
}

//////////////////////////////////////////////////////////////////////////////
// Dummy Interface
//////////////////////////////////////////////////////////////////////////////

int PERDummyInit(void);
void PERDummyDeInit(void);
int PERDummyHandleEvents(void);
void PERDummyNothing(void);

//static PortData_struct port1;
//static PortData_struct port2;

u32 PERDummyScan(const char *);
void PERDummyFlush(void);

PerInterface_struct PERDummy = {
PERCORE_DUMMY,
"Dummy Interface",
PERDummyInit,
PERDummyDeInit,
PERDummyHandleEvents,
PERDummyNothing,
PERDummyScan,
0,
PERDummyFlush
};

//////////////////////////////////////////////////////////////////////////////

int PERDummyInit(void) {

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

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 PERDummyScan(const char * name) {
   return 0;
}

void PERDummyFlush(void) {
}
