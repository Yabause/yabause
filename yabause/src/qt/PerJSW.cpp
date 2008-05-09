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
#ifdef HAVE_LIBJSW

#include "PerJSW.h"
#include "JSWHelper.h"

#include <QDebug>

extern "C"
{
	static JSWHelperAttributes* mJoysticks = 0;
	static JSWHelperJoystick* mJoystickP1 = 0;
	static int mCalibrationDone = 0; // use to check if autocalibration already done

	int PERJSWInit(void);
	void PERJSWDeInit(void);
	int PERJSWHandleEvents(void);
	void PERJSWNothing(void);

	u32 PERJSWScan(const char * name);
	void PERJSWFlush(void);

	PerInterface_struct PERJSW = {
	PERCORE_JSW,
	"JSW Joystick Interface",
	PERJSWInit,
	PERJSWDeInit,
	PERJSWHandleEvents,
	PERJSWNothing,
	PERJSWScan,
	1,
	PERJSWFlush
	};

	int PERJSWInit(void)
	{
		// get joysticks list
		if ( !mJoysticks )
			mJoysticks = JSWHelper::joysticks();
		// get joy 0
		if ( !mJoystickP1 )
			mJoystickP1 = mJoysticks->joystick( 0 );
		// is it ok ?
		if ( !mJoystickP1 )
			return 0;
		// check joy status
		if ( mJoystickP1->status() != JSSuccess )
			PERJSWDeInit();
		// perform auto calibration
		PERJSWScan( 0 );
		// return state
		return mJoystickP1 ? ( mJoystickP1->status() == JSSuccess ? 0 : -1 ) : -1;
	}

	void PERJSWDeInit(void)
	{
		delete mJoystickP1;
		mJoystickP1 = 0;
		delete mJoysticks;
		mJoysticks = 0;
		mCalibrationDone = 0;
	}

	void PERJSWNothing(void)
	{}

	// this should move to percore interface
	u32 hashAxis( u8 a, s16 v )
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

	int PERJSWHandleEvents(void)
	{
		if ( mJoystickP1 )
		{
			// handle joystick events
			js_data_struct jsd = *mJoystickP1->datas();
			if ( JSUpdate( &jsd ) == JSGotEvent )
			{
				// check axis
				for ( int i = 0; i < jsd.total_axises; i++ )
				{
					// double cur = JSGetAxisCoeffNZ( &jsd, i ); // should normally use this is joy is calibrate using jscalibrator
					s16 cur = jsd.axis[i]->cur;
					s16 cen = jsd.axis[i]->cen;
					if ( cur == cen )
						PerKeyUp( hashAxis( i, jsd.axis[i]->prev ) );
					else
						PerKeyDown( hashAxis( i, cur ) );
				}
				// check buttons
				for ( int i = 0; i < jsd.total_buttons; i++ )
				{
					if ( JSGetButtonState( &jsd, i ) )
						PerKeyDown( i +1 );
					else
						PerKeyUp( i +1 );
				}
			}
		}
	
	   if (YabauseExec() != 0)
		  return -1;
	   return 0;
	}

	PerInfo_struct* PERJSWGetList(void)
	{
	   // Returns a list of peripherals available along with information on each
	   // peripheral
		return 0;
	}

	u32 PERJSWScan( const char* n )
	{
		if ( !mJoystickP1 )
			return 0;
		u32 k = 0;
		js_data_struct jsd = *mJoystickP1->datas();
		if ( JSUpdate( &jsd ) == JSGotEvent )
		{
			// check axis
			for ( int i = 0; i < jsd.total_axises; i++ )
			{
				// if joy is calibrate
				if ( mCalibrationDone != 0 )
				{
					// double cur = JSGetAxisCoeffNZ( &jsd, i ); // should normally use this is joy is calibrate using jscalibrator
					s16 cur = jsd.axis[i]->cur;
					s16 cen = jsd.axis[i]->cen;
					qWarning( "cur: %i, cen: %i", cur, cen );
					if ( cur != cen )
					{
						k = hashAxis( i, cur );
						break;
					}
				}
				// calibrate axis
				else
					jsd.axis[i]->cen = jsd.axis[i]->cur;
			}
			// remember that axis are calibrate
			mCalibrationDone = -1;
			// check buttons
			if ( k == 0 )
			{
				for ( int i = 0; i < jsd.total_buttons; i++ )
				{
					if ( JSGetButtonState( &jsd, i ) )
					{
						k = i +1;
						break;
					}
				}
			}
		}
#if 0
		if ( k != 0 )
			PerSetKey( k, n );
#endif
		return k;
	}

	void PERJSWFlush(void)
	{}
}

#endif // HAVE_LIBJSW
