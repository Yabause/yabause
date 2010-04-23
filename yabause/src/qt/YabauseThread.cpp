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
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#include "YabauseThread.h"
#include "Settings.h"
#include "ui/UIPortManager.h"
#include "ui/UIYabause.h"

#include "../peripheral.h"

#include <QStringList>
#include <QDebug>

YabauseThread::YabauseThread( QObject* o )
	: QObject( o )
{
	mRunning = false;
	mPause = true;
	mTimerId = -1;
	mInit = -1;
}

yabauseinit_struct* YabauseThread::yabauseConf()
{ return &mYabauseConf; }

void YabauseThread::startEmulation()
{
	mRunning = false;
	mPause = true;
	reloadSettings();
	initEmulation();
}

void YabauseThread::stopEmulation()
{
	mPause = true;
	mRunning = false;
	deInitEmulation();
}

void YabauseThread::initEmulation()
{ mInit = YabauseInit( &mYabauseConf ); }

void YabauseThread::deInitEmulation()
{ YabauseDeInit(); }

bool YabauseThread::runEmulation()
{
	if ( mInit == -1 )
		initEmulation();
	if ( mInit == -1 )
	{
		emit error( "Can't init yabause" );
		return false;
	}
	mRunning = true;
	mPause = false;
	mTimerId = startTimer( 0 );
	ScspUnMuteAudio();
	return true;
}

void YabauseThread::pauseEmulation()
{
	mPause = true;
	mRunning = true;
	killTimer( mTimerId );
	ScspMuteAudio();
}

bool YabauseThread::resetEmulation( bool fullreset )
{
	if ( mInit == -1 )
		initEmulation();
	if ( mInit == -1 )
	{
		emit error( "Can't init yabause" );
		return false;
	}
	reloadSettings();
	if ( Cs2ChangeCDCore( mYabauseConf.cdcoretype, mYabauseConf.cdpath ) != 0 )
		return false;
	if ( VideoChangeCore( mYabauseConf.vidcoretype ) != 0 )
		return false;
	if ( ScspChangeVideoFormat( mYabauseConf.videoformattype ) != 0 )
		return false;
	if ( ScspChangeSoundCore( mYabauseConf.sndcoretype ) != 0 )
		return false;
	if ( fullreset )
		YabauseReset();
	return true;
}

void YabauseThread::reloadControllers()
{
	PerPortReset();
	QtYabause::clearPadsBits();
	
	Settings* settings = QtYabause::settings();
	
	for ( uint port = 1; port < 3; port++ )
	{
		settings->beginGroup( QString( "Input/Port/%1/Id" ).arg( port ) );
		QStringList ids = settings->childGroups();
		settings->endGroup();
		
		ids.sort();
		foreach ( const QString& id, ids )
		{
			uint type = settings->value( QString( UIPortManager::mSettingsType ).arg( port ).arg( id ) ).toUInt();
			
			switch ( type )
			{
				case PERPAD:
				{
					PerPad_struct* padbits = PerPadAdd( port == 1 ? &PORTDATA1 : &PORTDATA2 );
					
					settings->beginGroup( QString( "Input/Port/%1/Id/%2/Controller/%3/Key" ).arg( port ).arg( id ).arg( type ) );
					QStringList padKeys = settings->childKeys();
					settings->endGroup();
					
					padKeys.sort();
					foreach ( const QString& padKey, padKeys )
					{
						const QString key = settings->value( QString( UIPortManager::mSettingsKey ).arg( port ).arg( id ).arg( type ).arg( padKey ) ).toString();
						
						PerSetKey( key.toUInt(), padKey.toUInt(), padbits );
					}
					break;
				}
				case PERMOUSE:
					QtYabause::mainWindow()->appendLog( "Mouse controller type is not yet supported" );
					break;
				default:
					QtYabause::mainWindow()->appendLog( "Invalid controller type" );
					break;
			}
		}
	}
}

void YabauseThread::reloadSettings()
{
	//QMutexLocker l( &mMutex );
	// get settings pointer
	Settings* s = QtYabause::settings();
	
	// reset yabause conf
	resetYabauseConf();

	// read & apply settings
	mYabauseConf.m68kcoretype = s->value( "Advanced/M68KCore", mYabauseConf.m68kcoretype ).toInt();
	mYabauseConf.percoretype = s->value( "Input/PerCore", mYabauseConf.percoretype ).toInt();
	mYabauseConf.sh2coretype = s->value( "Advanced/SH2Interpreter", mYabauseConf.sh2coretype ).toInt();
	mYabauseConf.vidcoretype = s->value( "Video/VideoCore", mYabauseConf.vidcoretype ).toInt();
	mYabauseConf.sndcoretype = s->value( "Sound/SoundCore", mYabauseConf.sndcoretype ).toInt();
	mYabauseConf.cdcoretype = s->value( "General/CdRom", mYabauseConf.cdcoretype ).toInt();
	mYabauseConf.carttype = s->value( "Cartridge/Type", mYabauseConf.carttype ).toInt();
	const QString r = s->value( "Advanced/Region", mYabauseConf.regionid ).toString();
	if ( r.isEmpty() || r == "Auto" )
		mYabauseConf.regionid = 0;
	else
	{
		switch ( r[0].toAscii() )
		{
			case 'J': mYabauseConf.regionid = 1; break;
			case 'T': mYabauseConf.regionid = 2; break;
			case 'U': mYabauseConf.regionid = 4; break;
			case 'B': mYabauseConf.regionid = 5; break;
			case 'K': mYabauseConf.regionid = 6; break;
			case 'A': mYabauseConf.regionid = 0xA; break;
			case 'E': mYabauseConf.regionid = 0xC; break;
			case 'L': mYabauseConf.regionid = 0xD; break;
		}
	}
	mYabauseConf.biospath = strdup( s->value( "General/Bios", mYabauseConf.biospath ).toString().toAscii().constData() );
	mYabauseConf.cdpath = strdup( s->value( "General/CdRomISO", mYabauseConf.cdpath ).toString().toAscii().constData() );
	mYabauseConf.buppath = strdup( s->value( "Memory/Path", mYabauseConf.buppath ).toString().toAscii().constData() );
	mYabauseConf.mpegpath = strdup( s->value( "MpegROM/Path", mYabauseConf.mpegpath ).toString().toAscii().constData() );
	mYabauseConf.cartpath = strdup( s->value( "Cartridge/Path", mYabauseConf.cartpath ).toString().toAscii().constData() );
	mYabauseConf.videoformattype = s->value( "Video/VideoFormat", mYabauseConf.videoformattype ).toInt();
	
	emit requestSize( QSize( s->value( "Video/Width", 0 ).toInt(), s->value( "Video/Height", 0 ).toInt() ) );
	emit requestFullscreen( s->value( "Video/Fullscreen", false ).toBool() );

	reloadControllers();
}

bool YabauseThread::emulationRunning()
{
	//QMutexLocker l( &mMutex );
	return mRunning;
}

bool YabauseThread::emulationPaused()
{
	//QMutexLocker l( &mMutex );
	return mPause;
}

void YabauseThread::resetYabauseConf()
{
	// free structure
	memset( &mYabauseConf, 0, sizeof( yabauseinit_struct ) );
	// fill default structure
	mYabauseConf.m68kcoretype = M68KCORE_C68K;
	mYabauseConf.percoretype = QtYabause::defaultPERCore().id;
	mYabauseConf.sh2coretype = SH2CORE_DEFAULT;
	mYabauseConf.vidcoretype = QtYabause::defaultVIDCore().id;
	mYabauseConf.sndcoretype = QtYabause::defaultSNDCore().id;
	mYabauseConf.cdcoretype = QtYabause::defaultCDCore().id;
	mYabauseConf.carttype = CART_NONE;
	mYabauseConf.regionid = 0;
	mYabauseConf.biospath = 0;
	mYabauseConf.cdpath = 0;
	mYabauseConf.buppath = 0;
	mYabauseConf.mpegpath = 0;
	mYabauseConf.cartpath = 0;
	mYabauseConf.videoformattype = VIDEOFORMATTYPE_NTSC;
}

void YabauseThread::timerEvent( QTimerEvent* )
{
	//mPause = true;
	//mRunning = true;
	//while ( mRunning )
	{
		if ( !mPause )
			PERCore->HandleEvents();
		//else
			//msleep( 25 );
		//sleep( 0 );
	}
}
