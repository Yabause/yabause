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

#include "persdl.h"
#include "SDL.h"
#include "yabause.h"
#include "yui.h"

int PERSDLInit(void);
void PERSDLDeInit(void);
int PERSDLHandleEvents(void);
void PERSDLNothing(void);

PerInterface_struct PERSDL = {
PERCORE_SDL,
"SDL Input Interface",
PERSDLInit,
PERSDLDeInit,
PERSDLHandleEvents
};

void (*PERSDLKeyPressed[SDLK_LAST])(void);
void (*PERSDLKeyReleased[SDLK_LAST])(void);

void ToggleFPS (); // This should probably be changed

//////////////////////////////////////////////////////////////////////////////

static inline void PERSDLSetBoth(SDLKey key, void (*fct_ptr1)(void), void (*fct_ptr2)(void)) {
	PERSDLKeyPressed[key] = fct_ptr1;
	PERSDLKeyReleased[key] = fct_ptr2;
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG0(void)
{
   VIDCore->Vdp2ToggleDisplayNBG0();
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG1(void)
{
   VIDCore->Vdp2ToggleDisplayNBG1();
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG2(void)
{
   VIDCore->Vdp2ToggleDisplayNBG2();
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG3(void)
{
   VIDCore->Vdp2ToggleDisplayNBG3();
}

//////////////////////////////////////////////////////////////////////////////

void ToggleRBG0(void)
{
   VIDCore->Vdp2ToggleDisplayRBG0();
}

//////////////////////////////////////////////////////////////////////////////

void ToggleVDP1(void)
{
   Vdp1Regs->disptoggle ^= 1;
}

//////////////////////////////////////////////////////////////////////////////

int PERSDLInit(void) {
	int i;

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
   int i;

   while (SDL_PollEvent(&event))
   {
      switch(event.type)
      {
         case SDL_QUIT:
            YuiQuit();
            break;
         case SDL_KEYDOWN:
            PERSDLKeyPressed[event.key.keysym.sym]();
            break;
         case SDL_KEYUP:
            PERSDLKeyReleased[event.key.keysym.sym]();
            break;
         case SDL_VIDEORESIZE:
            VIDCore->Resize(event.resize.w, event.resize.h);
            break;
         default:
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

