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
#ifdef __APPLE__
 #include <SDL/SDL.h>
#else
 #include "SDL.h"
#endif

#include "../debug.h"
#include "PerQtSDL.h"

u32 mSDLoystickLastAxisP1[] = { 0, 0, 0, 0, 0, 0 }; // increase this if your joystick can handle more than 6 axis
#ifdef __APPLE__
Sint16 mSDLCenter = 128;
#else
Sint16 mSDLCenter = 0;
#endif

int PERQtSDLInit(void);
void PERQtSDLDeInit(void);
int PERQtSDLHandleEvents(void);
void PERQtSDLNothing(void);
#ifdef USENEWPERINTERFACE
PortData_struct *PERQtSDLGetPerDataP1(void);
PortData_struct *PERQtSDLGetPerDataP2(void);

static PortData_struct port1;
static PortData_struct port2;

u32 PERQtSDLScan(const char *);
void PERQtSDLFlush(void);
#endif

PerInterface_struct PERQTSDL = {
PERCORE_QTSDL,
"Qt SDL Interface",
PERQtSDLInit,
PERQtSDLDeInit,
#ifndef USENEWPERINTERFACE
PERQtSDLHandleEvents
#else
PERQtSDLHandleEvents,
PERQtSDLGetPerDataP1,
PERQtSDLGetPerDataP2,
PERQtSDLNothing,
PERQtSDLScan,
1,
PERQtSDLFlush
#endif
};

//////////////////////////////////////////////////////////////////////////////

int PERQtSDLInit(void) {
   putenv( "SDL_VIDEODRIVER=dummy" );
   SDL_InitSubSystem( SDL_INIT_JOYSTICK | SDL_INIT_VIDEO );
   SDL_JoystickOpen( 0 );
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERQtSDLDeInit(void) {
}

//////////////////////////////////////////////////////////////////////////////

void PERQtSDLNothing(void) {
}

//////////////////////////////////////////////////////////////////////////////

u32 hashAxisValue( Uint8 a, Sint16 v )
{
	u32 r = v < 0 ? -v : v;
	a == 0 ? r-- : r++;
	return r;
}

int PERQtSDLHandleEvents(void) {
	SDL_Event e;
	u32 k = 0;
	SDL_PollEvent( &e );
	//while ( SDL_PollEvent( &e ) )
	{
		switch ( e.type )
		{
			case SDL_JOYAXISMOTION:
			{
				Sint16 cur = e.jaxis.value;
				if ( cur == mSDLCenter )
					PerKeyUp( mSDLoystickLastAxisP1[e.jaxis.axis] );
				else
				{
					k = hashAxisValue( e.jaxis.axis, e.jaxis.value );
					PerKeyDown( k );
					mSDLoystickLastAxisP1[e.jaxis.axis] = k;
				}
				break;
			}
			case SDL_JOYBUTTONDOWN:
				PerKeyDown( e.jbutton.button +1 );
				break;
			case SDL_JOYBUTTONUP:
				PerKeyUp( e.jbutton.button +1 );
				break;
			default:
				break;
		}
	}
	
	PERQtSDLFlush();
	
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
PortData_struct *PERQtSDLGetPerDataP1(void)
{
   port1.data[0] = 0xF1;
   port1.data[1] = 0x02;
   port1.data[2] = buttonbits >> 8;
   port1.data[3] = buttonbits & 0xFF;
   port1.size = 4;

   return &port1;
}

//////////////////////////////////////////////////////////////////////////////

PortData_struct *PERQtSDLGetPerDataP2(void)
{
   port2.data[0] = 0xF0;
   port2.size = 1;

   return &port2;
}

//////////////////////////////////////////////////////////////////////////////

PerInfo_struct *PERQtSDLGetList(void)
{
   // Returns a list of peripherals available along with information on each
   // peripheral
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 PERQtSDLScan( const char* n ) {
	
	SDL_Event e;
	u32 k = 0;
	SDL_PollEvent( &e );
	switch ( e.type )
	{
		case SDL_JOYAXISMOTION:
		{
			Sint16 cur = e.jaxis.value;
			if ( cur != mSDLCenter )
				k = hashAxisValue( e.jaxis.axis, e.jaxis.value );
			break;
		}
		case SDL_JOYBUTTONDOWN:
			k = e.jbutton.button +1;
			break;
		default:
			k = 0;
	}
	if ( k != 0 )
		PerSetKey( k, n );
	return k;
}

void PERQtSDLFlush(void) {
	SDL_Event e;
	while ( SDL_PollEvent( &e ) );
}

#endif

#endif
