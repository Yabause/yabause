#include "QtYabause.h"

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

// main window
UIYabause* mUIYabause = 0;
// settings object
Settings* mSettings = 0;

extern "C" 
{
	void YuiErrorMsg(const char *string)
	{ qWarning( string ); }

	void YuiVideoResize(unsigned int /*w*/, unsigned int /*h*/, int /*isfullscreen*/)
	{}

	int YuiSetVideoMode(int /*width*/, int /*height*/, int /*bpp*/, int /*fullscreen*/)
	{ return 0; }

	void YuiSetVideoAttribute(int /*type*/, int /*val*/)
	{}

	void YuiSwapBuffers()
	{ QtYabause::mainWindow()->swapBuffers(); }
}

UIYabause* QtYabause::mainWindow()
{
	if ( !mUIYabause )
		mUIYabause = new UIYabause;
	return mUIYabause;
}

Settings* QtYabause::settings()
{
	if ( !mSettings )
		mSettings = new Settings();
	return mSettings;
}
