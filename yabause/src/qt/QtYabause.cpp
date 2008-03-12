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
#include "QtYabause.h"
#include "ui/UIYabause.h"
#include "Settings.h"

// cores

#ifdef Q_OS_WIN
extern CDInterface SPTICD;
extern CDInterface ASPICD;
#endif

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
#ifdef USENEWPERINTERFACE
&PERQT,
#ifdef HAVE_LIBJSW
&PERJSW,
#endif
#ifdef HAVE_LIBSDL
&PERSDL,
&PERSDLJoy,
#endif
#endif
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
#ifndef Q_OS_WIN
&ArchCD,
#else
&SPTICD,
//&ASPICD,
#endif
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
	{ qWarning( QString( "YuiErrorMsg: %1" ).arg( string ).toAscii() ); }

	void YuiVideoResize(unsigned int /*w*/, unsigned int /*h*/, int /*isfullscreen*/)
	{}

	int YuiSetVideoMode(int /*width*/, int /*height*/, int /*bpp*/, int /*fullscreen*/)
	{ return 0; }

	void YuiSetVideoAttribute(int /*type*/, int /*val*/)
	{}
	
	void YuiQuit()
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

const char* QtYabause::getCurrentCdSerial()
{ return cdip ? cdip->itemnum : 0; }

M68K_struct* QtYabause::getM68KCore( int id )
{
	for ( int i = 0; M68KCoreList[i] != NULL; i++ )
		if ( M68KCoreList[i]->id == id )
			return M68KCoreList[i];
	return 0;
}

SH2Interface_struct* QtYabause::getSH2Core( int id )
{
	for ( int i = 0; SH2CoreList[i] != NULL; i++ )
		if ( SH2CoreList[i]->id == id )
			return SH2CoreList[i];
	return 0;
}

PerInterface_struct* QtYabause::getPERCore( int id )
{
	for ( int i = 0; PERCoreList[i] != NULL; i++ )
		if ( PERCoreList[i]->id == id )
			return PERCoreList[i];
	return 0;
}

CDInterface* QtYabause::getCDCore( int id )
{
	for ( int i = 0; CDCoreList[i] != NULL; i++ )
		if ( CDCoreList[i]->id == id )
			return CDCoreList[i];
	return 0;
}

SoundInterface_struct* QtYabause::getSNDCore( int id )
{
	for ( int i = 0; SNDCoreList[i] != NULL; i++ )
		if ( SNDCoreList[i]->id == id )
			return SNDCoreList[i];
	return 0;
}

VideoInterface_struct* QtYabause::getVDICore( int id )
{
	for ( int i = 0; VIDCoreList[i] != NULL; i++ )
		if ( VIDCoreList[i]->id == id )
			return VIDCoreList[i];
	return 0;
}

CDInterface QtYabause::defaultCDCore()
{
#ifdef Q_OS_WIN
	return SPTICD;
#else
	return ArchCD;
#endif
}

SoundInterface_struct QtYabause::defaultSNDCore()
{
#ifdef HAVE_LIBSDL
	return SNDSDL;
#else
	return SNDDummy;
#endif
}

VideoInterface_struct QtYabause::defaultVIDCore()
{
#ifdef HAVE_LIBGL
	return VIDOGL;
#else
	return VIDSoft;
#endif
}

PerInterface_struct QtYabause::defaultPERCore()
{
#ifdef HAVE_LIBSDL
	return PERSDLJoy;
#else
	return PERQT;
#endif
}

