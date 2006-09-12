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

#ifdef HAVE_LIBSDL
#include "SDL.h"

#include "debug.h"
#include "persdljoy.h"

int PERSDLJoyInit(void);
void PERSDLJoyDeInit(void);
int PERSDLJoyHandleEvents(void);
void PERSDLJoyNothing(void);
#ifdef USENEWPERINTERFACE
PortData_struct *PERSDLJoyGetPerDataP1(void);
PortData_struct *PERSDLJoyGetPerDataP2(void);

static PortData_struct port1;
static PortData_struct port2;

u32 PERSDLJoyScan(const char *);
void PERSDLJoyFlush(void);
#endif

PerInterface_struct PERSDLJoy = {
PERCORE_SDLJOY,
"SDL Joystick Interface",
PERSDLJoyInit,
PERSDLJoyDeInit,
#ifndef USENEWPERINTERFACE
PERSDLJoyHandleEvents
#else
PERSDLJoyHandleEvents,
PERSDLJoyGetPerDataP1,
PERSDLJoyGetPerDataP2,
PERSDLJoyNothing,
PERSDLJoyScan,
1,
PERSDLJoyFlush
#endif
};

typedef union {
	u32 pack;
	struct {
		u8 type;
		u8 key;
		u16 value;
	} data;
} persdljoykey_struct;

//////////////////////////////////////////////////////////////////////////////

int PERSDLJoyInit(void) {
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

   putenv("SDL_VIDEODRIVER=dummy");
   SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO);
   SDL_JoystickOpen(0);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERSDLJoyDeInit(void) {
}

//////////////////////////////////////////////////////////////////////////////

void PERSDLJoyNothing(void) {
}

//////////////////////////////////////////////////////////////////////////////

int PERSDLJoyHandleEvents(void) {
   SDL_Event event;
   persdljoykey_struct k;
   static u16 lastvalue;

   k.pack = 0;

   while(SDL_PollEvent(&event)) {
      k.data.type = event.type;
      switch(event.type) {
		case SDL_JOYAXISMOTION:
			k.data.key = event.jaxis.axis;
			if (event.jaxis.value == 0) {
				k.data.value = lastvalue;
				//PERSDLJoyKeyUp(k.pack);
				PerKeyUp(k.pack);
			} else {
				k.data.value = event.jaxis.value;
				lastvalue = k.data.value;
				//PERSDLJoyKeyDown(k.pack);
				PerKeyDown(k.pack);
			}
			break;
		case SDL_JOYBUTTONDOWN:
			k.data.key = event.jbutton.button;
			//PERSDLJoyKeyDown(k.pack);
			PerKeyDown(k.pack);
			break;
		case SDL_JOYBUTTONUP:
      			k.data.type = SDL_JOYBUTTONDOWN;
			k.data.key = event.jbutton.button;
			//PERSDLJoyKeyUp(k.pack);
			PerKeyUp(k.pack);
			break;
		default:
			return 0;
	}
   }
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
PortData_struct *PERSDLJoyGetPerDataP1(void)
{
   port1.data[0] = 0xF1;
   port1.data[1] = 0x02;
   port1.data[2] = buttonbits >> 8;
   port1.data[3] = buttonbits & 0xFF;
   port1.size = 4;

   return &port1;
}

//////////////////////////////////////////////////////////////////////////////

PortData_struct *PERSDLJoyGetPerDataP2(void)
{
   port2.data[0] = 0xF0;
   port2.size = 1;

   return &port2;
}

//////////////////////////////////////////////////////////////////////////////

PerInfo_struct *PERSDLJoyGetList(void)
{
   // Returns a list of peripherals available along with information on each
   // peripheral
}

//////////////////////////////////////////////////////////////////////////////

u32 PERSDLJoyScan(const char * name) {
	SDL_Event event;
	persdljoykey_struct k;

	k.pack = 0;

	SDL_PollEvent(&event);
	k.data.type = event.type;
	switch(event.type) {
		case SDL_JOYAXISMOTION:
			k.data.key = event.jaxis.axis;
			k.data.value = event.jaxis.value;
			break;
		case SDL_JOYBUTTONDOWN:
			k.data.key = event.jbutton.button;
			break;
		default:
			k.pack = 0;
	}

	//PERSDLJoySetKey(k.pack, name);
	PerSetKey(k.pack, name);
	return k.pack;
}

void PERSDLJoyFlush(void) {
	SDL_Event event;

	while(SDL_PollEvent(&event));
}

#endif

#endif
