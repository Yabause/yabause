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
#include "YabauseThread.h"
#include "Settings.h"

#include <QStringList>

YabauseThread::YabauseThread( QObject* o )
	: QObject( o )
{
	mRunning = false;
	mPause = true;
	mYabauseConf = new YabauseConf;
	mTimerId = -1;
}

YabauseThread::~YabauseThread()
{ delete mYabauseConf; }

YabauseConf* YabauseThread::yabauseConf()
{ return mYabauseConf; }

void YabauseThread::startEmulation()
{
	mRunning = false;
	mPause = true;
	initEmulation();
}

void YabauseThread::stopEmulation()
{
	mPause = true;
	mRunning = false;
	deInitEmulation();
}

void YabauseThread::initEmulation()
{
	reloadSettings();
	qWarning( "YabauseInit: %i", YabauseInit( mYabauseConf ) );
}

void YabauseThread::deInitEmulation()
{
	YabauseDeInit();
}

void YabauseThread::runEmulation()
{
	mRunning = true;
	mPause = false;
	mTimerId = startTimer( 0 );
}

void YabauseThread::pauseEmulation()
{
	mPause = true;
	mRunning = true;
	killTimer( mTimerId );
}

void YabauseThread::resetEmulation( bool fullreset )
{
	// reload settings
	reloadSettings();
	// update cores...
	Cs2ChangeCDCore( mYabauseConf->cdcoretype, mYabauseConf->cdpath );
	// reset yabause
	if ( fullreset )
		YabauseReset();
}

void YabauseThread::reloadSettings()
{
	//QMutexLocker l( &mMutex );
	// get settings pointer
	Settings* s = QtYabause::settings();
	
	// reset yabause conf
	resetYabauseConf();

	// read & apply settings
	mYabauseConf->m68kcoretype = s->value( "Advanced/M68KCore", mYabauseConf->m68kcoretype ).toInt();
	mYabauseConf->percoretype = s->value( "Input/PERCore", mYabauseConf->percoretype ).toInt();
	mYabauseConf->sh2coretype = s->value( "Advanced/SH2Interpreter", mYabauseConf->sh2coretype ).toInt();
	mYabauseConf->vidcoretype = s->value( "Video/VideoCore", mYabauseConf->vidcoretype ).toInt();
	mYabauseConf->sndcoretype = s->value( "Sound/SoundCore", mYabauseConf->sndcoretype ).toInt();
	mYabauseConf->cdcoretype = s->value( "General/CdRom", mYabauseConf->cdcoretype ).toInt();
	mYabauseConf->carttype = s->value( "Cartridge/Type", mYabauseConf->carttype ).toInt();
	const QString r = s->value( "Advanced/Region", mYabauseConf->regionid ).toString();
	if ( r.isEmpty() || r == "Auto" )
		mYabauseConf->regionid = 0;
	else
	{
		switch ( r[0].toAscii() )
		{
			case 'J': mYabauseConf->regionid = 1; break;
			case 'T': mYabauseConf->regionid = 2; break;
			case 'U': mYabauseConf->regionid = 4; break;
			case 'B': mYabauseConf->regionid = 5; break;
			case 'K': mYabauseConf->regionid = 6; break;
			case 'A': mYabauseConf->regionid = 0xA; break;
			case 'E': mYabauseConf->regionid = 0xC; break;
			case 'L': mYabauseConf->regionid = 0xD; break;
		}
	}
	mYabauseConf->biospath = strdup( s->value( "General/Bios", mYabauseConf->biospath ).toString().toAscii().constData() );
	mYabauseConf->cdpath = strdup( s->value( "General/CdRomISO", mYabauseConf->cdpath ).toString().toAscii().constData() );
	mYabauseConf->buppath = strdup( s->value( "Memory/Path", mYabauseConf->buppath ).toString().toAscii().constData() );
	mYabauseConf->mpegpath = strdup( s->value( "MpegROM/Path", mYabauseConf->mpegpath ).toString().toAscii().constData() );
	mYabauseConf->cartpath = strdup( s->value( "Cartridge/Path", mYabauseConf->cartpath ).toString().toAscii().constData() );
	mYabauseConf->flags = s->value( "Video/VideoFormat", mYabauseConf->flags ).toInt();
	
	emit requestSize( QSize( s->value( "Video/Width", 320 ).toInt(), s->value( "Video/Height", 240 ).toInt() ) );
	emit requestFullscreen( s->value( "Video/Fullscreen", false ).toBool() );
	
	s->beginGroup( "Input" );
	foreach ( const QString& ki, s->childKeys() )
		PerSetKey( (u32)s->value( ki ).toString().toUInt(), ki.toAscii().constData() );
	s->endGroup();
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
	memset( mYabauseConf, 0, sizeof( yabauseinit_struct ) );
	// fill default structure
	mYabauseConf->m68kcoretype = M68KCORE_C68K;
#if defined( HAVE_LIBSDL ) && defined( USENEWPERINTERFACE )
	mYabauseConf->percoretype = PERCORE_SDLJOY;
#else
	mYabauseConf->percoretype = PERCORE_DUMMY;
#endif
	mYabauseConf->sh2coretype = SH2CORE_DEFAULT;
#ifdef HAVE_LIBGL
	mYabauseConf->vidcoretype = VIDCORE_OGL;
#else
	mYabauseConf->vidcoretype = VIDCORE_SOFT;
#endif
	mYabauseConf->sndcoretype = SNDCORE_DUMMY;
	mYabauseConf->cdcoretype = CDCORE_DEFAULT;
	mYabauseConf->carttype = CART_NONE;
	mYabauseConf->regionid = 0;
	mYabauseConf->biospath = 0;
	mYabauseConf->cdpath = 0;
	mYabauseConf->buppath = 0;
	mYabauseConf->mpegpath = 0;
	mYabauseConf->cartpath = 0;
	mYabauseConf->flags = VIDEOFORMATTYPE_NTSC;
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
