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
	QMutexLocker l( &mMutex );
	return mYabauseConf;
}

void YabauseThread::startEmulation()
{
	//if ( isRunning() )
		//return;
	initEmulation();
	//start();
	mPause = true;
	mTimerId = startTimer( 0 );
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
	qWarning( "YabauseInit: %i", YabauseInit( mYabauseConf ) );
}

void YabauseThread::deInitEmulation()
{
	YabauseDeInit();
}

void YabauseThread::runEmulation()
{
	QMutexLocker l( &mMutex );
	mPause = false;
}

void YabauseThread::pauseEmulation()
{
	QMutexLocker l( &mMutex );
	mPause = true;
}

void YabauseThread::resetEmulation()
{
	QMutexLocker l( &mMutex );
	pauseEmulation();
	reloadSettings();
	runEmulation();
}

void YabauseThread::reloadSettings()
{
	QMutexLocker l( &mMutex );
	resetYabauseConf();
	// need read ini file
	mYabauseConf->percoretype = PERCORE_SDLJOY;
	mYabauseConf->sndcoretype = SNDCORE_SDL;
	mYabauseConf->biospath = "./SEGA_101.BIN";
}

bool YabauseThread::emulationRunning()
{
	QMutexLocker l( &mMutex );
	return mRunning;
}

bool YabauseThread::emulationPaused()
{
	QMutexLocker l( &mMutex );
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
