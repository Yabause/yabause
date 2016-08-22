/*	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>
	Copyright 2013 Theo Berkau <cwx@cyberwarriorx.com>

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
        Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#include "UITwinStickSetting.h"
#include "UIPortManager.h"
#include "../Settings.h"

#include <QKeyEvent>
#include <QTimer>
#include <QStylePainter>
#include <QStyleOptionToolButton>

// Make a parent class for all controller setting classes


UITwinStickSetting::UITwinStickSetting( PerInterface_struct* core, uint port, uint pad, uint perType, QWidget* parent )
	: UIControllerSetting( core, port, pad, perType, parent )
{
   setupUi( this );
   setInfos(lInfos);
		
	mButtons[ tbLeftStickUp ] = PERPAD_UP;
	mButtons[ tbLeftStickRight ] = PERPAD_RIGHT;
	mButtons[ tbLeftStickDown ] = PERPAD_DOWN;
	mButtons[ tbLeftStickLeft ] = PERPAD_LEFT;
	mButtons[ tbLeftButton ] = PERPAD_RIGHT_TRIGGER;
	mButtons[ tbLeftTrigger ] = PERPAD_LEFT_TRIGGER;
	mButtons[ tbStart ] = PERPAD_START;
	mButtons[ tbRightTrigger ] = PERPAD_A;
	mButtons[ tbRightStickDown ] = PERPAD_B;
	mButtons[ tbRightButton ] = PERPAD_C;
	mButtons[ tbRightStickLeft ] = PERPAD_X;
	mButtons[ tbRightStickUp ] = PERPAD_Y;
	mButtons[ tbRightStickRight ] = PERPAD_Z;
	
	mNames[ PERPAD_UP ] = QtYabause::translate( "Left Stick Up" );
	mNames[ PERPAD_RIGHT ] = QtYabause::translate( "Left Stick Right" );
	mNames[ PERPAD_DOWN ] = QtYabause::translate( "Left Stick Down" );
	mNames[ PERPAD_LEFT ] = QtYabause::translate( "Left Stick Left" );
	mNames[ PERPAD_RIGHT_TRIGGER ] = QtYabause::translate( "Left Button" );
	mNames[ PERPAD_LEFT_TRIGGER ] = QtYabause::translate( "Left Trigger" );
	mNames[ PERPAD_START ] = "Start";
	mNames[ PERPAD_A ] = "Right Trigger";
	mNames[ PERPAD_B ] = "Right Stick Down";
	mNames[ PERPAD_C ] = "Right Button";
	mNames[ PERPAD_X ] = "Right Stick Left";
	mNames[ PERPAD_Y ] = "Right Stick Up";
	mNames[ PERPAD_Z ] = "Right Stick Right";

   //PERSF_AXIS so LT and RT can be used as normal buttons
   mScanMasks[ PERPAD_UP ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_RIGHT ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_DOWN ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_LEFT ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_RIGHT_TRIGGER ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_LEFT_TRIGGER ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_START ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_A ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_B ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_C ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_X ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_Y ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
   mScanMasks[ PERPAD_Z ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;

	loadPadSettings();
	
	foreach ( QToolButton* tb, findChildren<QToolButton*>() )
	{
		tb->installEventFilter( this );
		connect( tb, SIGNAL( clicked() ), this, SLOT( tbButton_clicked() ) );
	}
	
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( timer_timeout() ) );

	QtYabause::retranslateWidget( this );
}

UITwinStickSetting::~UITwinStickSetting()
{
}
