#include "YabauseThread.h"

YabauseThread::YabauseThread( QObject* o )
	: QObject( o )
{
	mYabauseConf = new YabauseConf;
	mTimerId = -1;
}

YabauseThread::~YabauseThread()
{ delete mYabauseConf; }

YabauseConf* YabauseThread::yabauseConf()
{
	//QMutexLocker l( &mMutex );
	return mYabauseConf;
}

void YabauseThread::startEmulation()
{
	//if ( isRunning() )
		//return;
	initEmulation();
	//start();
	mPause = true;
}

void YabauseThread::stopEmulation()
{
	//if ( isRunning() )
		mRunning = false;
	//terminate();
	//wait( 2000 );
	deInitEmulation();
}

void YabauseThread::initEmulation()
{
	reloadSettings();
	qWarning( "1/ bp: %s", mYabauseConf->biospath );
	qWarning( "YabauseInit: %i", YabauseInit( mYabauseConf ) );
	qWarning( "2/ bp: %s", mYabauseConf->biospath );
}

void YabauseThread::deInitEmulation()
{
	YabauseDeInit();
}

void YabauseThread::runEmulation()
{
	//QMutexLocker l( &mMutex );
	mPause = false;
	mTimerId = startTimer( 0 );
}

void YabauseThread::pauseEmulation()
{
	//QMutexLocker l( &mMutex );
	mPause = true;
	killTimer( mTimerId );
}

void YabauseThread::resetEmulation()
{
	//QMutexLocker l( &mMutex );
	pauseEmulation();
	reloadSettings();
	runEmulation();
}

void YabauseThread::reloadSettings()
{
	//QMutexLocker l( &mMutex );
	// get settings pointer
	Settings* s = QtYabause::settings();
	
	// reset yabause conf
	resetYabauseConf();

	// read & apply settings
	mYabauseConf->m68kcoretype = s->value( "Advanced/M68KCore", M68KCORE_C68K ).toInt();
	mYabauseConf->percoretype = s->value( "Input/PERCore", PERCORE_DUMMY ).toInt();
	mYabauseConf->sh2coretype = s->value( "Advanced/SH2Interpreter", SH2CORE_DEFAULT ).toInt();
	mYabauseConf->vidcoretype = s->value( "Video/VideoCore", VIDCORE_OGL ).toInt();
	mYabauseConf->sndcoretype = s->value( "Sound/SoundCore", SNDCORE_DUMMY ).toInt();
	mYabauseConf->cdcoretype = s->value( "General/CdRom", CDCORE_DEFAULT ).toInt();
	mYabauseConf->carttype = CART_NONE;
	mYabauseConf->regionid = 0;
	mYabauseConf->biospath = s->value( "General/Bios", "./SEGA_101.BIN" ).toString().toAscii().constData();
	mYabauseConf->cdpath = s->value( "General/CdRomISO", "" ).toString().toAscii().constData();
	mYabauseConf->buppath = 0;
	mYabauseConf->mpegpath = 0;
	mYabauseConf->cartpath = 0;
	mYabauseConf->flags = VIDEOFORMATTYPE_NTSC;
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
	mYabauseConf->percoretype = PERCORE_DUMMY;
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
		{
			YabauseExec();
			YabauseExec();
			YabauseExec();
			YabauseExec();
			YabauseExec();
		}
		//else
			//msleep( 25 );
		//sleep( 0 );
	}
}
