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
#include "UIYabause.h"
#include "../Settings.h"
#include "UISettings.h"
#include "UIBackupRam.h"
#include "UICheats.h"
#include "UIAbout.h"
#include "../YabauseGL.h"
#include "../QtYabause.h"
#include "../CommonDialogs.h"

#include <QKeyEvent>
#include <QTimer>
#include <QTextEdit>
#include <QDockWidget>
#include <QImageWriter>

//#define USE_UNIFIED_TITLE_TOOLBAR

void qAppendLog( const char* s )
{ QtYabause::mainWindow()->appendLog( s ); }

UIYabause::UIYabause( QWidget* parent )
	: QMainWindow( parent )
{
	// setup dialog
	setupUi( this );
	toolBar->insertAction( aYabauseSettings, mYabauseSaveState->menuAction() );
	toolBar->insertAction( aYabauseSettings, mYabauseLoadState->menuAction() );
	toolBar->insertSeparator( aYabauseSettings );
	setAttribute( Qt::WA_DeleteOnClose );
#ifdef USE_UNIFIED_TITLE_TOOLBAR
	setUnifiedTitleAndToolBarOnMac( true );
#endif
	
	// create glcontext
	mYabauseGL = new YabauseGL;
	// and set it as central application widget
	setCentralWidget( mYabauseGL );
	
	// create log widget
	teLog = new QTextEdit( this );
	teLog->setReadOnly( true );
	teLog->setWordWrapMode( QTextOption::NoWrap );
	teLog->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	teLog->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
	mLogDock = new QDockWidget( this );
	mLogDock->setWindowTitle( "Log" );
	mLogDock->setWidget( teLog );
	addDockWidget( Qt::BottomDockWidgetArea, mLogDock );
	mLogDock->setVisible( false );

	// start log
	LogStart();
	LogChangeOutput( DEBUG_CALLBACK, (char*)qAppendLog );
	
	// create emulator thread
	mYabauseThread = new YabauseThread( this );
	
	// connectionsdd
	connect( mYabauseThread, SIGNAL( requestSize( const QSize& ) ), this, SLOT( sizeRequested( const QSize& ) ) );
	connect( mYabauseThread, SIGNAL( requestFullscreen( bool ) ), this, SLOT( fullscreenRequested( bool ) ) );
	connect( aViewLog, SIGNAL( toggled( bool ) ), mLogDock, SLOT( setVisible( bool ) ) );
	connect( mLogDock->toggleViewAction(), SIGNAL( toggled( bool ) ), aViewLog, SLOT( setChecked( bool ) ) );
	
	// start emulation
	mYabauseThread->startEmulation();
	
	// show settings dialog
	QTimer::singleShot( 25, aYabauseSettings, SLOT( trigger() ) );
}

UIYabause::~UIYabause()
{
	// stop emulation
	mYabauseThread->stopEmulation();

	// stop log
	LogStop();
}

void UIYabause::closeEvent( QCloseEvent* e )
{
	// pause emulation
	aYabausePause->trigger();

	// close dialog
	QMainWindow::closeEvent( e );
}

void UIYabause::keyPressEvent( QKeyEvent* e )
{ PerKeyDown( e->key() ); }

void UIYabause::keyReleaseEvent( QKeyEvent* e )
{ PerKeyUp( e->key() ); }

void UIYabause::swapBuffers()
{ mYabauseGL->swapBuffers(); }

void UIYabause::appendLog( const char* s )
{
	teLog->moveCursor( QTextCursor::End );
	teLog->append( s );
}

void UIYabause::sizeRequested( const QSize& s )
{ resize( s.isNull() ? QSize( 640, 480 ) : s ); }

void UIYabause::fullscreenRequested( bool f )
{
	if ( isFullScreen() && !f )
	{
		showNormal();
#ifdef USE_UNIFIED_TITLE_TOOLBAR
		setUnifiedTitleAndToolBarOnMac( true );
#endif
	}
	else if ( !isFullScreen() && f )
	{
#ifdef USE_UNIFIED_TITLE_TOOLBAR
		setUnifiedTitleAndToolBarOnMac( false );
#endif
		showFullScreen();
	}
	if ( aViewFullscreen->isChecked() != f )
		aViewFullscreen->setChecked( f );
	aViewFullscreen->setIcon( QIcon( f ? ":/actions/no_fullscreen.png" : ":/actions/fullscreen.png" ) );
}

void UIYabause::on_aYabauseSettings_triggered()
{
	YabauseLocker locker( mYabauseThread );
	if ( UISettings( window() ).exec() )
		mYabauseThread->resetEmulation();
}

void UIYabause::on_aYabauseRun_triggered()
{
	if ( mYabauseThread->emulationPaused() )
	{
		aYabauseRun->setEnabled( false );
		aYabausePause->setEnabled( true );
		aYabauseReset->setEnabled( true );
		mYabauseThread->runEmulation();
	}
}

void UIYabause::on_aYabausePause_triggered()
{
	if ( !mYabauseThread->emulationPaused() )
	{
		aYabauseRun->setEnabled( true );
		aYabausePause->setEnabled( false );
		aYabauseReset->setEnabled( true );
		mYabauseThread->pauseEmulation();
	}
}

void UIYabause::on_aYabauseReset_triggered()
{ mYabauseThread->resetEmulation( true ); }

void UIYabause::on_aYabauseTransfer_triggered()
{}

void UIYabause::on_aYabauseScreenshot_triggered()
{
	YabauseLocker locker( mYabauseThread );
	// images filter that qt can write
	QStringList filters;
	foreach ( QByteArray ba, QImageWriter::supportedImageFormats() )
		if ( !filters.contains( ba, Qt::CaseInsensitive ) )
			filters << QString( ba ).toLower();
	for ( int i = 0; i < filters.count(); i++ )
		filters[i] = tr( "%1 Images (*.%2)" ).arg( filters[i].toUpper() ).arg( filters[i] );
	
	// take screenshot of gl view
	QImage screenshot = mYabauseGL->grabFrameBuffer();
	
	// request a file to save to to user
	const QString s = CommonDialogs::getSaveFileName( QString(), tr( "Choose a location for your screenshot" ), filters.join( ";;" ) );
	
	// write image if ok
	if ( !s.isEmpty() )
		if ( !screenshot.save( s ) )
			CommonDialogs::information( tr( "An error occur while writing the screenshot." ) );
}

void UIYabause::on_aYabauseFrameSkipLimiter_triggered( bool b )
{
	if ( b )
		EnableAutoFrameSkip();
	else
		DisableAutoFrameSkip();
}

void UIYabause::on_mYabauseSaveState_triggered( QAction* a )
{
	YabauseLocker locker( mYabauseThread );
	if ( YabSaveStateSlot( QtYabause::settings()->value( "General/SaveStates", QApplication::applicationDirPath() ).toString().toAscii().constData(), a->text().toInt() ) != 0 )
		CommonDialogs::information( tr( "Couldn't save state file" ) );
}

void UIYabause::on_mYabauseLoadState_triggered( QAction* a )
{
	YabauseLocker locker( mYabauseThread );
	if ( YabLoadStateSlot( QtYabause::settings()->value( "General/SaveStates", QApplication::applicationDirPath() ).toString().toAscii().constData(), a->text().toInt() ) != 0 )
		CommonDialogs::information( tr( "Couldn't load state file" ) );
}

void UIYabause::on_aYabauseSaveStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getSaveFileName( QtYabause::settings()->value( "General/SaveStates", QApplication::applicationDirPath() ).toString(), tr( "Yabause Save State (*.yss )" ), tr( "Choose a file to save your state" ) );
	if ( fn.isNull() )
		return;
	if ( YabSaveState( fn.toAscii().constData() ) != 0 )
		CommonDialogs::information( tr( "Couldn't save state file" ) );
}

void UIYabause::on_aYabauseLoadStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getOpenFileName( QtYabause::settings()->value( "General/SaveStates", QApplication::applicationDirPath() ).toString(), tr( "Select a file to load your state" ), tr( "Yabause Save State (*.yss )" ) );
	if ( fn.isNull() )
		return;
	if ( YabLoadState( fn.toAscii().constData() ) != 0 )
		CommonDialogs::information( tr( "Couldn't load state file" ) );
	else
		aYabauseRun->trigger();
}

void UIYabause::on_aYabauseQuit_triggered()
{ close(); }

void UIYabause::on_aToolsBackupManager_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIBackupRam( this ).exec();
}

void UIYabause::on_aToolsCheatsList_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UICheats( this ).exec();
}

void UIYabause::on_aViewFPS_triggered()
{ ToggleFPS(); }

void UIYabause::on_aViewLayerVdp1_triggered()
{ ToggleVDP1(); }

void UIYabause::on_aViewLayerNBG0_triggered()
{ ToggleNBG0(); }

void UIYabause::on_aViewLayerNBG1_triggered()
{ ToggleNBG1(); }

void UIYabause::on_aViewLayerNBG2_triggered()
{ ToggleNBG2(); }

void UIYabause::on_aViewLayerNBG3_triggered()
{ ToggleNBG3(); }

void UIYabause::on_aViewLayerRBG0_triggered()
{ ToggleRBG0(); }

void UIYabause::on_aViewFullscreen_triggered( bool b )
{
	fullscreenRequested( b );
	//ToggleFullScreen();
}

void UIYabause::on_aHelpAbout_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIAbout( window() ).exec();
}
