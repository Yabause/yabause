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

#include <QApplication>
#include <QLabel>
#include <QGroupBox>
#include <QTreeWidget>

// cores

#ifdef Q_OS_WIN
extern CDInterface SPTICD;
//extern CDInterface ASPICD;
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
&PERQT,
#ifdef HAVE_LIBJSW
&PERJSW,
#endif
#ifdef HAVE_LIBSDL
&PERSDLJoy,
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
// current translation file
char* mTranslationFile = 0;

extern "C" 
{
	void YuiErrorMsg(const char *string)
	{ QtYabause::mainWindow()->appendLog( string ); }

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

void QtYabause::setTranslationFile( const char* filePath )
{
#ifdef HAVE_LIBMINI18N
	mini18n_set_locale( filePath );
	delete mTranslationFile;
	mTranslationFile = strdup( filePath );
	QtYabause::retranslateApplication();
#endif
}

const char* QtYabause::translationFile()
{ return mTranslationFile; }

void QtYabause::retranslateWidget( QWidget* widget )
{
	if ( !widget )
		return;
	// translate all widget based members
	widget->setAccessibleDescription( _( qPrintable( widget->accessibleDescription() ) ) );
	widget->setAccessibleName( _( qPrintable( widget->accessibleName() ) ) );
	widget->setStatusTip( _( qPrintable( widget->statusTip() ) ) );
	widget->setStyleSheet( _( qPrintable( widget->styleSheet() ) ) );
	widget->setToolTip( _( qPrintable( widget->toolTip() ) ) );
	widget->setWhatsThis( _( qPrintable( widget->whatsThis() ) ) );
	widget->setWindowIconText( _( qPrintable( widget->windowIconText() ) ) );
	widget->setWindowTitle( _( qPrintable( widget->windowTitle() ) ) );
	// get class name
	const QString className = widget->metaObject()->className();
	if ( className == "QWidget" )
		return;
	else if ( className == "QLabel" )
	{
		QLabel* l = qobject_cast<QLabel*>( widget );
		l->setText( _( qPrintable( l->text() ) ) );
	}
	else if ( className == "QAbstractButton" )
	{
		QAbstractButton* ab = qobject_cast<QAbstractButton*>( widget );
		ab->setText( _( qPrintable( ab->text() ) ) );
	}
	else if ( className == "QGroupBox" )
	{
		QGroupBox* gb = qobject_cast<QGroupBox*>( widget );
		gb->setTitle( _( qPrintable( gb->title() ) ) );
	}
	else if ( className == "QMenu" || className == "QMenuBar" )
	{
		QList<QMenu*> menus;
		if ( className == "QMenuBar" )
			menus = qobject_cast<QMenuBar*>( widget )->findChildren<QMenu*>();
		else
			menus << qobject_cast<QMenu*>( widget );
		foreach ( QMenu* m, menus )
		{
			m->setTitle( _( qPrintable( m->title() ) ) );
			// retranslate menu actions
			foreach ( QAction* a, m->actions() )
			{
				a->setIconText( _( qPrintable( a->iconText() ) ) );
				a->setStatusTip( _( qPrintable( a->statusTip() ) ) );
				a->setText( _( qPrintable( a->text() ) ) );
				a->setToolTip( _( qPrintable( a->toolTip() ) ) );
				a->setWhatsThis( _( qPrintable( a->whatsThis() ) ) );
			}
		}
	}
	else if ( className == "QTreeWidget" )
	{
		QTreeWidget* tw = qobject_cast<QTreeWidget*>( widget );
		QTreeWidgetItem* twi = tw->headerItem();
		for ( int i = 0; i < twi->columnCount(); i++ )
		{
			twi->setStatusTip( i, _( qPrintable( twi->statusTip( i ) ) ) );
			twi->setText( i, _( qPrintable( twi->text( i ) ) ) );
			twi->setToolTip( i, _( qPrintable( twi->toolTip( i ) ) ) );
			twi->setWhatsThis( i, _( qPrintable( twi->whatsThis( i ) ) ) );
		}
	}
}

void QtYabause::retranslateApplication()
{
	foreach ( QWidget* widget, QApplication::allWidgets() )
		retranslateWidget( widget );
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
#if defined HAVE_LIBGL
	return VIDOGL;
#elif defined HAVE_LIBSDL
	return VIDSoft;
#else
	return VIDDummy;
#endif
}

PerInterface_struct QtYabause::defaultPERCore()
{
	return PERQT;
}

SH2Interface_struct QtYabause::defaultSH2Core()
{
	return SH2Interpreter;
}

