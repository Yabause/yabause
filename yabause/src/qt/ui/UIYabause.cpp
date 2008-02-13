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
#include "UICheats.h"
#include "UIAbout.h"
#include "../YabauseGL.h"
#include "../QtYabause.h"
#include "../CommonDialogs.h"

#include <QKeyEvent>
#include <QTimer>
#include <QTextEdit>

void qAppendLog( const char* s )
{ QtYabause::mainWindow()->appendLog( s ); }

UIYabause::UIYabause( QWidget* parent )
	: QMainWindow( parent )
{
	mInit = false;
	
	// setup dialog
	setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose );
	
	// create glcontext
	mYabauseGL = new YabauseGL;
	// and set it as central application widget
	setCentralWidget( mYabauseGL );
	
	// create log widget
	teLog = new QTextEdit( this );
	teLog->setWindowFlags( Qt::Window );
	teLog->setReadOnly( true );
	
	// init actions
	aYabausePause->setEnabled( false );
	aYabauseReset->setEnabled( false );

	// start log
	LogStart();
	LogChangeOutput( DEBUG_CALLBACK, (char*)qAppendLog );
	
	// create emulator thread
	mYabauseThread = new YabauseThread( this );
	
	// connections
	connect( mYabauseThread, SIGNAL( requestSize( const QSize& ) ), this, SLOT( sizeRequested( const QSize& ) ) );
	connect( mYabauseThread, SIGNAL( requestFullscreen( bool ) ), this, SLOT( fullscreenRequested( bool ) ) );
}

UIYabause::~UIYabause()
{
	// stop emulation
	mYabauseThread->stopEmulation();

	// stop log
	LogStop();
}

void UIYabause::closeEvent( QCloseEvent* )
{
	teLog->close();
	QApplication::quit();
}

void UIYabause::showEvent( QShowEvent* )
{
	if ( !mInit )
	{
		mInit = true;
		// start emulation
		mYabauseThread->startEmulation();
		// show settings dialog
		aYabauseSettings->trigger();
	}
}

void UIYabause::keyPressEvent( QKeyEvent* e )
{
	switch ( e->key() )
	{
		case Qt::Key_Up:
			/*
			mPlayer1.UpPressed = true;
			qWarning( "Pressed: Up" );
			break;
			*/
		case Qt::Key_Down:
			/*
			mPlayer1.DownPressed = true;
			qWarning( "Pressed: down" );
			break;
			*/
		case Qt::Key_Left:
			/*
			mPlayer1.LeftPressed = true;
			qWarning( "Pressed: Left" );
			break;
			*/
		case Qt::Key_Right:
			/*
			mPlayer1.RightPressed = true;
			qWarning( "Pressed: Right" );
			break;
			*/
		case Qt::Key_Q:
		case Qt::Key_S:
		case Qt::Key_D:
		case Qt::Key_W:
		case Qt::Key_X:
		case Qt::Key_C:
		case Qt::Key_A:
		case Qt::Key_Z:
		case Qt::Key_Return:
			PerKeyDown( e->key() );
		default:
			break;
	}
}

void UIYabause::keyReleaseEvent( QKeyEvent* e )
{
	switch ( e->key() )
	{
		case Qt::Key_Up:
			/*
			mPlayer1.UpPressed = false;
			qWarning( "Released: Up" );
			break;
			*/
		case Qt::Key_Down:
			/*
			mPlayer1.DownPressed = false;
			qWarning( "Released: down" );
			break;
			*/
		case Qt::Key_Left:
			/*
			mPlayer1.LeftPressed = false;
			qWarning( "Released: Left" );
			break;
			*/
		case Qt::Key_Right:
			/*
			mPlayer1.RightPressed = false;
			qWarning( "Released: Right" );
			break;
			*/
		case Qt::Key_Q:
		case Qt::Key_S:
		case Qt::Key_D:
		case Qt::Key_W:
		case Qt::Key_X:
		case Qt::Key_C:
		case Qt::Key_A:
		case Qt::Key_Z:
		case Qt::Key_Return:
			PerKeyUp( e->key() );
		default:
			break;
	}
}

void UIYabause::swapBuffers()
{ mYabauseGL->swapBuffers(); }

void UIYabause::appendLog( const char* s )
{
	teLog->moveCursor( QTextCursor::End );
	teLog->append( s );
}

void UIYabause::sizeRequested( const QSize& s )
{ resize( s.isNull() ? QSize( 320, 240 ) : s ); }

void UIYabause::fullscreenRequested( bool f )
{
	if ( isFullScreen() && !f )
		showNormal();
	else if ( !isFullScreen() && f )
		showFullScreen();
	if ( aViewFullscreen->isChecked() != f )
		aViewFullscreen->setChecked( f );
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
	// images filter that can write qt
	QStringList filters = QStringList()
		<< tr( "PNG Images (*.png)" )
		<< tr( "JPG Images (*.jpg)" )
		<< tr( "JPEG Images (*.jpeg)" )
		<< tr( "BMP Images (*.bmp)" )
		<< tr( "PPM Images (*.ppm)" )
		<< tr( "XBM Images (*.xbm)" )
		<< tr( "XPM Images (*.xpm)" );
	
	// take screenshot of gl view
	QImage screenshot = mYabauseGL->grabFrameBuffer();
	
	// request a file to save to to user
	const QString s = CommonDialogs::getSaveFileName( QString(), filters.join( ";;" ), tr( "Choose a location for your screenshot" ) );
	
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
{
	aYabausePause->trigger();
	close();
}

void UIYabause::on_aCheatsList_triggered()
{ UICheats( this ).exec(); }

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

void UIYabause::on_aViewLog_triggered()
{
	if ( !teLog->isVisible() )
		teLog->show();
	teLog->raise();
}

void UIYabause::on_aHelpAbout_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIAbout( window() ).exec();
}
