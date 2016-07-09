/*	Copyright 2013 Theo Berkau <cwx@cyberwarriorx.com>

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
#include "UIDoubleMissionStickSetting.h"
#include "UIPortManager.h"
#include "../Settings.h"

#include <QKeyEvent>
#include <QTimer>
#include <QStylePainter>
#include <QStyleOptionToolButton>

UIDoubleMissionStickSetting::UIDoubleMissionStickSetting( PerInterface_struct* core, uint port, uint pad, uint perType, QWidget* parent )
	: UIControllerSetting( core, port, pad, perType, parent )
{
   setupUi( this );
	setInfos(lInfos);	

	mButtons[ tbRightTrigger ] = PERPAD_RIGHT_TRIGGER;
	mButtons[ tbLeftTrigger ] = PERPAD_LEFT_TRIGGER;
	mButtons[ tbStart ] = PERPAD_START;
	mButtons[ tbA ] = PERPAD_A;
	mButtons[ tbB ] = PERPAD_B;
	mButtons[ tbC ] = PERPAD_C;
	mButtons[ tbX ] = PERPAD_X;
	mButtons[ tbY ] = PERPAD_Y;
	mButtons[ tbZ ] = PERPAD_Z;

   //right stick
	mButtons[ tbAxis1Left ] = PERANALOG_AXIS1;
	mButtons[ tbAxis1Right ] = PERANALOG_AXIS1;
	mButtons[ tbAxis2Up ] = PERANALOG_AXIS2;
	mButtons[ tbAxis2Down ] = PERANALOG_AXIS2;
	mButtons[ tbAxis3Up] = PERANALOG_AXIS3;
   mButtons[ tbAxis3Down] = PERANALOG_AXIS3;

   //left stick
   mButtons[tbAxis5Left] = PERANALOG_AXIS5;
   mButtons[tbAxis5Right] = PERANALOG_AXIS5;
   mButtons[tbAxis6Up] = PERANALOG_AXIS6;
   mButtons[tbAxis6Down] = PERANALOG_AXIS6;
   mButtons[tbAxis7Up] = PERANALOG_AXIS7;
   mButtons[tbAxis7Down] = PERANALOG_AXIS7;

	mNames[ PERPAD_RIGHT_TRIGGER ] = QtYabause::translate( "Right trigger" );
	mNames[ PERPAD_LEFT_TRIGGER ] = QtYabause::translate( "Left trigger" );
	mNames[ PERPAD_START ] = "Start";
	mNames[ PERPAD_A ] = "A";
	mNames[ PERPAD_B ] = "B";
	mNames[ PERPAD_C ] = "C";
	mNames[ PERPAD_X ] = "X";
	mNames[ PERPAD_Y ] = "Y";
	mNames[ PERPAD_Z ] = "Z";

	mNames[ PERANALOG_AXIS1 ] = "Right Stick Axis X";
	mNames[ PERANALOG_AXIS2 ] = "Right Stick Axis Y";
   mNames[PERANALOG_AXIS3] = "Right Stick Axis Throttle";

   mNames[PERANALOG_AXIS5] = "Left Stick Axis X";
   mNames[PERANALOG_AXIS6] = "Left Stick Axis Y";
   mNames[PERANALOG_AXIS7] = "Left Stick Axis Throttle";

	mScanMasks[ PERPAD_RIGHT_TRIGGER ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
	mScanMasks[ PERPAD_LEFT_TRIGGER ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
	mScanMasks[ PERPAD_START ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
	mScanMasks[ PERPAD_A ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
	mScanMasks[ PERPAD_B ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
	mScanMasks[ PERPAD_C ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
	mScanMasks[ PERPAD_X ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
	mScanMasks[ PERPAD_Y ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;
	mScanMasks[ PERPAD_Z ] = PERSF_KEY | PERSF_BUTTON | PERSF_HAT | PERSF_AXIS;

	mScanMasks[ PERANALOG_AXIS1 ] = PERSF_AXIS;
	mScanMasks[ PERANALOG_AXIS2 ] = PERSF_AXIS;
	mScanMasks[ PERANALOG_AXIS3 ] = PERSF_AXIS;

   mScanMasks[PERANALOG_AXIS5] = PERSF_AXIS;
   mScanMasks[PERANALOG_AXIS6] = PERSF_AXIS;
   mScanMasks[PERANALOG_AXIS7] = PERSF_AXIS;

	loadPadSettings();
	
	foreach ( QToolButton* tb, findChildren<QToolButton*>() )
	{
		tb->installEventFilter( this );
		connect( tb, SIGNAL( clicked() ), this, SLOT( tbButton_clicked() ) );
	}
	
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( timer_timeout() ) );

	QtYabause::retranslateWidget( this );
}

UIDoubleMissionStickSetting::~UIDoubleMissionStickSetting()
{
}
