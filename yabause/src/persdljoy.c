/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau
    Copyright 2008 Filipe Azevedo

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

#include "debug.h"
#include "persdljoy.h"

SDL_Joystick* mSDLJoystick1 = 0;
#define SDL_MAX_AXIS_VALUE 32767
#define SDL_MIN_AXIS_VALUE -32768
#define SDL_BUTTON_PRESSED 1
#define SDL_BUTTON_RELEASED 0

int PERSDLJoyInit(void);
void PERSDLJoyDeInit(void);
int PERSDLJoyHandleEvents(void);
void PERSDLJoyNothing(void);

static PortData_struct port1;
static PortData_struct port2;

u32 PERSDLJoyScan(const char *);
void PERSDLJoyFlush(void);

PerInterface_struct PERSDLJoy = {
PERCORE_SDLJOY,
"SDL Joystick Interface",
PERSDLJoyInit,
PERSDLJoyDeInit,
PERSDLJoyHandleEvents,
PERSDLJoyNothing,
PERSDLJoyScan,
1,
PERSDLJoyFlush
};

//////////////////////////////////////////////////////////////////////////////

int PERSDLJoyInit(void) {
	// does not need init if already done
	if ( mSDLJoystick1 )
		return 0;
	
	// init joysticks
	if ( SDL_InitSubSystem( SDL_INIT_JOYSTICK ) == -1 )
		return -1;
	
	// ignore joysticks event in sdl event loop
	SDL_JoystickEventState( SDL_IGNORE );
	
	// open first joystick
	mSDLJoystick1 = SDL_JoystickOpen( 0 );
	
	// is it open ?
	if ( !mSDLJoystick1 )
	{
		PERSDLJoyDeInit();
		return -1;
	}
	
	// success
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERSDLJoyDeInit(void) {
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

void PERSDLJoyNothing(void) {
}

//////////////////////////////////////////////////////////////////////////////

u32 hashAxisSDL( u8 a, s16 v )
{
	u32 r;
	if ( !( v == -1 || v == 1 ) )
		v = v < 0 ? -1 : 1;
	r = v < 0 ? -v : v +1;
	a % 2 ? r-- : r++;
	r += 20 *( a +1 );
	//r += 10 *( playerid +1 );
	return r;
}

int PERSDLJoyHandleEvents(void) {
	// if available joy
	if ( mSDLJoystick1 )
	{
		// update joysticks states
		SDL_JoystickUpdate();
		
		// check axis
		int i;
		for ( i = 0; i < SDL_JoystickNumAxes( mSDLJoystick1 ); i++ )
		{
			Sint16 cur = SDL_JoystickGetAxis( mSDLJoystick1, i );
			
			if ( cur == SDL_MIN_AXIS_VALUE )
			{
				PerKeyUp( hashAxisSDL( i, SDL_MAX_AXIS_VALUE ) );
				PerKeyDown( hashAxisSDL( i, cur ) );
			}
			else if ( cur == SDL_MAX_AXIS_VALUE )
			{
				PerKeyUp( hashAxisSDL( i, SDL_MIN_AXIS_VALUE ) );
				PerKeyDown( hashAxisSDL( i, cur ) );
			}
			else
			{
				PerKeyUp( hashAxisSDL( i, SDL_MIN_AXIS_VALUE ) );
				PerKeyUp( hashAxisSDL( i, SDL_MAX_AXIS_VALUE ) );
			}
		}
		
		// check buttons
		for ( i = 0; i < SDL_JoystickNumButtons( mSDLJoystick1 ); i++ )
		{
			Uint8 v = SDL_JoystickGetButton( mSDLJoystick1, i );
			if ( v == SDL_BUTTON_PRESSED )
				PerKeyDown( i +1 );
			else if ( v == SDL_BUTTON_RELEASED )
				PerKeyUp( i +1 );
		}
	}
	
	// execute yabause
	if ( YabauseExec() != 0 )
		return -1;
	
	// return success
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 PERSDLJoyScan( const char* n ) {
	// if no available joy
	if ( !mSDLJoystick1 )
		return 0;
	
	// init vars
	int i;
	u32 k = 0;
	
	// update joysticks states
	SDL_JoystickUpdate();
	
	// check axis
	for ( i = 0; i < SDL_JoystickNumAxes( mSDLJoystick1 ); i++ )
	{
		Sint16 cur = SDL_JoystickGetAxis( mSDLJoystick1, i );
		if ( cur == SDL_MIN_AXIS_VALUE || cur == SDL_MAX_AXIS_VALUE )
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
			if ( SDL_JoystickGetButton( mSDLJoystick1, i ) == SDL_BUTTON_PRESSED )
			{
				k = i +1;
				break;
			}
		}
	}

	return k;
}

void PERSDLJoyFlush(void) {
}

#endif
