/*  Copyright 2005 Guillaume Duhamel
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

#ifdef HAVE_LIBSDL
#include "persdl.h"
#include "SDL.h"
#include "yabause.h"
#include "vdp2.h"
#include "yui.h"

int PERSDLInit(void);
void PERSDLDeInit(void);
int PERSDLHandleEvents(void);
void PERSDLNothing(void);
#ifdef USENEWPERINTERFACE
PortData_struct *PERSDLGetPerDataP1(void);
PortData_struct *PERSDLGetPerDataP2(void);

static PortData_struct port1;
static PortData_struct port2;
#endif

PerInterface_struct PERSDL = {
PERCORE_SDL,
"SDL Input Interface",
PERSDLInit,
PERSDLDeInit,
#ifndef USENEWPERINTERFACE
PERSDLHandleEvents
#else
PERSDLHandleEvents,
PERSDLGetPerDataP1,
PERSDLGetPerDataP2,
PERSDLNothing
#endif
};

void (*PERSDLKeyPressed[SDLK_LAST])(void);
void (*PERSDLKeyReleased[SDLK_LAST])(void);

//////////////////////////////////////////////////////////////////////////////

static INLINE void PERSDLSetBoth(SDLKey key, void (*fct_ptr1)(void), void (*fct_ptr2)(void)) {
	PERSDLKeyPressed[key] = fct_ptr1;
	PERSDLKeyReleased[key] = fct_ptr2;
}

//////////////////////////////////////////////////////////////////////////////

int PERSDLInit(void) {
	int i;

        if (SDL_WasInit(SDL_INIT_VIDEO) == 0)
           SDL_InitSubSystem(SDL_INIT_VIDEO);

	for(i = 0;i < SDLK_LAST;i++) {
		PERSDLKeyPressed[i] = PERSDLNothing;
		PERSDLKeyReleased[i] = PERSDLNothing;
	}

	PERSDLSetBoth(SDLK_RIGHT, PerRightPressed, PerRightReleased);
	PERSDLSetBoth(SDLK_LEFT, PerLeftPressed, PerLeftReleased);
	PERSDLSetBoth(SDLK_DOWN, PerDownPressed, PerDownReleased);
	PERSDLSetBoth(SDLK_UP, PerUpPressed, PerUpReleased);
	PERSDLSetBoth(SDLK_j, PerStartPressed, PerStartReleased);
	PERSDLSetBoth(SDLK_k, PerAPressed, PerAReleased);
	PERSDLSetBoth(SDLK_l, PerBPressed, PerBReleased);
	PERSDLSetBoth(SDLK_m, PerCPressed, PerCReleased);
	PERSDLSetBoth(SDLK_u, PerXPressed, PerXReleased);
	PERSDLSetBoth(SDLK_i, PerYPressed, PerYReleased);
	PERSDLSetBoth(SDLK_o, PerZPressed, PerZReleased);
	PERSDLSetBoth(SDLK_z, PerRTriggerPressed, PerRTriggerReleased);
	PERSDLSetBoth(SDLK_x, PerLTriggerPressed, PerLTriggerReleased);
        PERSDLSetBoth(SDLK_BACKQUOTE, SpeedThrottleEnable, SpeedThrottleDisable);

	PERSDLKeyPressed[SDLK_q] = YuiQuit;

        PERSDLKeyPressed[SDLK_1] = ToggleNBG0;
        PERSDLKeyPressed[SDLK_2] = ToggleNBG1;
        PERSDLKeyPressed[SDLK_3] = ToggleNBG2;
        PERSDLKeyPressed[SDLK_4] = ToggleNBG3;
        PERSDLKeyPressed[SDLK_5] = ToggleRBG0;
        PERSDLKeyPressed[SDLK_6] = ToggleVDP1;

	/* for french keyboards */
        PERSDLKeyPressed[SDLK_AMPERSAND] = ToggleNBG0;
        PERSDLKeyPressed[SDLK_WORLD_73]  = ToggleNBG1;
        PERSDLKeyPressed[SDLK_QUOTEDBL]  = ToggleNBG2;
        PERSDLKeyPressed[SDLK_QUOTE]     = ToggleNBG3;
        PERSDLKeyPressed[SDLK_LEFTPAREN] = ToggleRBG0;
        PERSDLKeyPressed[SDLK_MINUS]     = ToggleVDP1;

        PERSDLKeyPressed[SDLK_F1] = ToggleFPS;

	PERSDLKeyPressed[SDLK_f] = ToggleFullScreen;

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERSDLDeInit(void) {
}

//////////////////////////////////////////////////////////////////////////////

void PERSDLNothing(void) {
}

//////////////////////////////////////////////////////////////////////////////

int PERSDLHandleEvents(void) {
   SDL_Event event;

   while (SDL_PollEvent(&event))
   {
      switch(event.type)
      {
         case SDL_KEYDOWN:
            PERSDLKeyPressed[event.key.keysym.sym]();
            break;
         case SDL_KEYUP:
            PERSDLKeyReleased[event.key.keysym.sym]();
            break;
         case SDL_QUIT:
            YuiQuit();
            break;
         case SDL_VIDEORESIZE:
            VIDCore->Resize(event.resize.w, event.resize.h, 0);
            break;
         default:
            // Since we're not handling it, might as well tell SDL to stop
            // sending the message
            SDL_EventState(event.type, SDL_IGNORE);
            break;
      }
   }

   // I may end up changing this depending on people's results
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
PortData_struct *PERSDLGetPerDataP1(void)
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

//////////////////////////////////////////////////////////////////////////////

PortData_struct *PERSDLGetPerDataP2(void)
{
   port2.data[0] = 0xF0;
   port2.size = 1;
   return &port2;
}

//////////////////////////////////////////////////////////////////////////////

PerInfo_struct *PERSDLGetList(void)
{
   // Returns a list of peripherals available along with information on each
   // peripheral
}

//////////////////////////////////////////////////////////////////////////////

#endif

#endif
