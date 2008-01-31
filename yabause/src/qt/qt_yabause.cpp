#include "qt_yabause.h"
#include "YabauseGL.h"

// cores

M68K_struct * M68KCoreList[] = {
&M68KDummy,
#ifdef HAVE_C68K
&M68KC68K,
#endif
NULL
};

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
#ifdef HAVE_LIBSDL
&PERSDLJoy,
#endif
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
&ArchCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
#ifdef HAVE_LIBSDL
&SNDSDL,
#endif
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
#ifdef HAVE_LIBGL
&VIDOGL,
#endif
&VIDSoft,
NULL
};

// yabause init structure
yabauseinit_struct yinit;

// main window
UIYabause* mUIYabause = 0;
Settings* mSettings = 0;

extern "C" 
{
	void YuiErrorMsg(const char *string)
	{ qWarning( string ); }

	void YuiVideoResize(unsigned int /*w*/, unsigned int /*h*/, int /*isfullscreen*/)
	{ YuiErrorMsg( "YuiVideoResize" ); }

	int YuiSetVideoMode(int /*width*/, int /*height*/, int /*bpp*/, int /*fullscreen*/)
	{ return 0; }

	void YuiSetVideoAttribute(int /*type*/, int /*val*/)
	{ YuiErrorMsg( "YuiSetVideoAttribute" ); }

	void YuiSwapBuffers()
	{ Yabause::mainWindow()->mYabauseGL->updateGL(); }
}

void Yabause::init()
{
	// free structure
	memset( &yinit, 0, sizeof( yinit ) );
	
	// fill default structure
	yinit.m68kcoretype = M68KCORE_C68K;
	yinit.percoretype = PERCORE_DUMMY; // PERCORE_SDLJOY
	yinit.sh2coretype = SH2CORE_DEFAULT;
#ifdef HAVE_LIBGL
	yinit.vidcoretype = VIDCORE_OGL;
#else
	yinit.vidcoretype = VIDCORE_SOFT;
#endif
	yinit.sndcoretype = SNDCORE_SDL; // SNDCORE_DUMMY
	yinit.cdcoretype = CDCORE_DEFAULT;
	yinit.carttype = CART_NONE;
	yinit.regionid = 0;
	yinit.biospath = "./SEGA_101.BIN";
	yinit.cdpath = 0;
	yinit.buppath = 0;
	yinit.mpegpath = 0;
	yinit.cartpath = 0;
	yinit.flags = VIDEOFORMATTYPE_NTSC;
	
	// init yabause
	YabauseInit( &yinit );
}

void Yabause::deInit()
{
	// free structure
	memset( &yinit, 0, sizeof( yinit ) );
	
	// free yabayse
	YabauseDeInit();
}

void Yabause::exec()
{
	YabauseExec();
	YabauseExec();
	YabauseExec();
	YabauseExec();
	YabauseExec();
}

void Yabause::log( const char* string )
{ YuiErrorMsg( string ); }

UIYabause* Yabause::mainWindow()
{
	if ( !mUIYabause )
		mUIYabause = new UIYabause;
	return mUIYabause;
}

Settings* Yabause::settings()
{
	if ( !mSettings )
		mSettings = new Settings();
	return mSettings;
}
