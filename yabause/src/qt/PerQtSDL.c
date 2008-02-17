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
 #include "SDL/SDL.h"
#endif

#include "../debug.h"
#include "PerQtSDL.h"

SDL_Joystick* mSDLJoystick1 = 0;
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
	// init joysticks
	if ( SDL_InitSubSystem( SDL_INIT_JOYSTICK ) == -1 )
		return -1;
	// sdl will not add joystick event in sdlevent loop, we will request ourselves when needed
	SDL_JoystickEventState( SDL_IGNORE );
	// open first joystick
	mSDLJoystick1 = SDL_JoystickOpen( 0 );
	// is it open ?
	if ( !mSDLJoystick1 )
		return -1;
	// success
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERQtSDLDeInit(void) {
	// close joystick
	if ( mSDLJoystick1 )
	{
		 if ( SDL_JoystickOpened( 0 ) )
			SDL_JoystickClose( mSDLJoystick1 );
		mSDLJoystick1 = 0;
	}
	// close sdl joysticks
	SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
}

//////////////////////////////////////////////////////////////////////////////

void PERQtSDLNothing(void) {
}

//////////////////////////////////////////////////////////////////////////////

// this may need be moved in the PerCore interface so all core can call it
u32 hashAxisSDL( Uint8 a, Sint16 v )
{
	u32 r;
	if ( v == -1 || v == 1 )
		r = v < 0 ? -v : v +1;
	else
		r = v < 0 ? -v : v;
	a % 2 ? r-- : r++;
	r += 100; // to avoid conflict with buttons
	return r;
}

int isSDLJoysticksSame( Sint16* oav, Uint8* obv, Sint16* nav, Uint8* nbv, int na, int nb )
{
	int i;
	for ( i = 0; i < na; i++ )
		if ( oav[i] != nav[i] )
			return -1;
	for ( i = 0; i < nb; i++ )
		if ( obv[i] != nbv[i] )
			return -1;
	return 0;
}

int PERQtSDLHandleEvents(void) {
	// if available joy
	if ( mSDLJoystick1 )
	{
		// check joystick
		int i;
		int na = SDL_JoystickNumAxes( mSDLJoystick1 );
		int nb = SDL_JoystickNumButtons( mSDLJoystick1 );
	
		// remember last values before update joysticks values
		Sint16* oav = malloc( na *sizeof( Sint16 ) );
		for ( i = 0; i < na; i++ )
			oav[i] = SDL_JoystickGetAxis( mSDLJoystick1, i );
		Uint8* obv = malloc( nb *sizeof( Uint8 ) );
		for ( i = 0; i < nb; i++ )
			obv[i] = SDL_JoystickGetButton( mSDLJoystick1, i );
		
		// update joysticks states
		SDL_JoystickUpdate();
		
		// get new values
		Sint16* nav = malloc( na *sizeof( Sint16 ) );
		for ( i = 0; i < na; i++ )
			nav[i] = SDL_JoystickGetAxis( mSDLJoystick1, i );
		Uint8* nbv = malloc( nb *sizeof( Uint8 ) );
		for ( i = 0; i < nb; i++ )
			nbv[i] = SDL_JoystickGetButton( mSDLJoystick1, i );
		
		// if differents update states
		if ( isSDLJoysticksSame( oav, obv, nav, nbv, na, nb ) == -1 )
		{
			// check axis
			for ( i = 0; i < na; i++ )
			{
				Sint16 cur = nav[i];
				Sint16 cen = mSDLCenter; // need find a way to get neutral value for each axis
				if ( cur == cen )
					PerKeyUp( hashAxisSDL( i, oav[i] ) );
				else
					PerKeyDown( hashAxisSDL( i, cur ) );
			}
			// check buttons
			for ( i = 0; i < nb; i++ )
			{
				if ( nbv[i] )
					PerKeyDown( i +1 );
				else
					PerKeyUp( i +1 );
			}
		}
		// free values
		free( oav );
		free( obv );
		free( nav );
		free( nbv );
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
	// if no available joy
	if ( !mSDLJoystick1 )
		return 0;
	// check joystick
	int i;
	u32 k = 0;
	// update joysticks states
	SDL_JoystickUpdate();
	// check axis
	for ( i = 0; i < SDL_JoystickNumAxes( mSDLJoystick1 ); i++ )
	{
		Sint16 cur = SDL_JoystickGetAxis( mSDLJoystick1, i );
		Sint16 cen = mSDLCenter;
		LOG( "cur: %i, cen: %i", cur, cen );
		if ( cur != cen )
		{
			k = hashAxisSDL( i, cur );
			break;
		}
	}
	// check buttons
	if ( k == 0 )
	{
		for ( i = 0; i < SDL_JoystickNumButtons( mSDLJoystick1 ); i++ )
		{
			if ( SDL_JoystickGetButton( mSDLJoystick1, i ) )
			{
				k = i +1;
				break;
			}
		}
	}
	if ( k != 0 )
		PerSetKey( k, n );
	return k;
}

void PERQtSDLFlush(void) {
}

#endif

#endif
