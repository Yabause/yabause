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
#include <QTextEdit>
#include <QDockWidget>
#include <QImageWriter>
#include <QUrl>
#include <QDesktopServices>
#include <QDateTime>

#include <QDebug>

extern VideoInterface_struct *VIDCoreList[];

//#define USE_UNIFIED_TITLE_TOOLBAR

void qAppendLog( const char* s )
{ QtYabause::mainWindow()->appendLog( s ); }

UIYabause::UIYabause( QWidget* parent )
	: QMainWindow( parent )
{
	mInit = false;
	// setup dialog
	setupUi( this );
	toolBar->insertAction( aFileSettings, mFileSaveState->menuAction() );
	toolBar->insertAction( aFileSettings, mFileLoadState->menuAction() );
	toolBar->insertSeparator( aFileSettings );
	setAttribute( Qt::WA_DeleteOnClose );
#ifdef USE_UNIFIED_TITLE_TOOLBAR
	setUnifiedTitleAndToolBarOnMac( true );
#endif
	fSound->setParent( 0, Qt::Popup );
	fVideoDriver->setParent( 0, Qt::Popup );
	fSound->installEventFilter( this );
	fVideoDriver->installEventFilter( this );
	// fill combo driver
	cbVideoDriver->blockSignals( true );
	for ( int i = 0; VIDCoreList[i] != NULL; i++ )
		cbVideoDriver->addItem( VIDCoreList[i]->Name, VIDCoreList[i]->id );
	cbVideoDriver->blockSignals( false );
	// create glcontext
	mYabauseGL = new YabauseGL;
	// and set it as central application widget
	setCentralWidget( mYabauseGL );
	// set focus proxy to the mainwindow
	mYabauseGL->setFocusProxy( this );
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
	// create emulator thread
	mYabauseThread = new YabauseThread( this );
	// connectionsdd
	connect( mYabauseThread, SIGNAL( requestSize( const QSize& ) ), this, SLOT( sizeRequested( const QSize& ) ) );
	connect( mYabauseThread, SIGNAL( requestFullscreen( bool ) ), this, SLOT( fullscreenRequested( bool ) ) );
	connect( aViewLog, SIGNAL( toggled( bool ) ), mLogDock, SLOT( setVisible( bool ) ) );
	connect( mLogDock->toggleViewAction(), SIGNAL( toggled( bool ) ), aViewLog, SLOT( setChecked( bool ) ) );
	connect( mYabauseThread, SIGNAL( error( const char* ) ), this, SLOT( appendLog( const char* ) ) );
	// retranslate widgets
	QtYabause::retranslateWidget( this );
}

void UIYabause::showEvent( QShowEvent* e )
{
	if ( !mInit )
	{
		// start log
		LogStart();
		LogChangeOutput( DEBUG_CALLBACK, (char*)qAppendLog );
		// start emulation
		mYabauseThread->startEmulation();
		// refresh states actions
		refreshStatesActions();

		mInit = true;
	}
}

void UIYabause::closeEvent( QCloseEvent* e )
{
	// pause emulation
	aEmulationPause->trigger();
	// stop emulation
	mYabauseThread->stopEmulation();
	// stop log
	LogStop();
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

bool UIYabause::eventFilter( QObject* o, QEvent* e )
{
	if ( e->type() == QEvent::Hide )
		setFocus();
	return QMainWindow::eventFilter( o, e );
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

void UIYabause::refreshStatesActions()
{
	// reset save actions
	foreach ( QAction* a, findChildren<QAction*>( QRegExp( "aFileSaveState*" ) ) )
	{
		if ( a == aFileSaveStateAs )
			continue;
		int i = a->objectName().remove( "aFileSaveState" ).toInt();
		a->setText( QString( "%1 ... " ).arg( i ) );
		a->setToolTip( a->text() );
		a->setStatusTip( a->text() );
		a->setData( i );
	}
	// reset load actions
	foreach ( QAction* a, findChildren<QAction*>( QRegExp( "aFileLoadState*" ) ) )
	{
		if ( a == aFileLoadStateAs )
			continue;
		int i = a->objectName().remove( "aFileLoadState" ).toInt();
		a->setText( QString( "%1 ... " ).arg( i ) );
		a->setToolTip( a->text() );
		a->setStatusTip( a->text() );
		a->setData( i );
		a->setEnabled( false );
	}
	// get states files of this game
	const QString serial = QtYabause::getCurrentCdSerial();
	const QString mask = QString( "%1_*.yss" ).arg( serial );
	const QString statesPath = QtYabause::settings()->value( "General/SaveStates", QApplication::applicationDirPath() ).toString();
	QRegExp rx( QString( mask ).replace( '*', "(\\d+)") );
	QDir d( statesPath );
	foreach ( const QFileInfo& fi, d.entryInfoList( QStringList( mask ), QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase ) )
	{
		if ( rx.exactMatch( fi.fileName() ) )
		{
			int slot = rx.capturedTexts().value( 1 ).toInt();
			const QString caption = QString( "%1 %2 " ).arg( slot ).arg( fi.lastModified().toString( Qt::SystemLocaleDate ) );
			// update save state action
			if ( QAction* a = findChild<QAction*>( QString( "aFileSaveState%1" ).arg( slot ) ) )
			{
				a->setText( caption );
				a->setToolTip( caption );
				a->setStatusTip( caption );
				// update load state action
				a = findChild<QAction*>( QString( "aFileLoadState%1" ).arg( slot ) );
				a->setText( caption );
				a->setToolTip( caption );
				a->setStatusTip( caption );
				a->setEnabled( true );
			}
		}
	}
}

void UIYabause::on_aFileSettings_triggered()
{
	YabauseLocker locker( mYabauseThread );
	if ( UISettings( window() ).exec() )
	{
		if ( mYabauseThread->resetEmulation() )
			refreshStatesActions();
	}
}

void UIYabause::on_aFileOpenISO_triggered()
{
	YabauseLocker locker( mYabauseThread );
	if ( mYabauseThread->init() == -1 )
	{
		CommonDialogs::information( QtYabause::translate( "Yabause is not initialized, can't open ISO." ) );
		return;
	}
	const QString fn = CommonDialogs::getOpenFileName( QtYabause::settings()->value( "Recents/ISOs" ).toString(), QtYabause::translate( "Select your iso/cue/bin file" ), QtYabause::translate( "CD Images (*.iso *.cue *.bin)" ) );
	if ( !fn.isEmpty() )
	{
		QtYabause::settings()->setValue( "Recents/ISOs", fn );
		if ( Cs2ChangeCDCore( ISOCD.id, strdup( fn.toAscii().constData() ) ) == 0 )
		{
			YabauseReset();
			if ( !aEmulationRun->isChecked() )
				aEmulationRun->trigger();
			refreshStatesActions();
		}
	}
}

void UIYabause::on_aFileOpenCDRom_triggered()
{
	YabauseLocker locker( mYabauseThread );
	if ( mYabauseThread->init() == -1 )
	{
		CommonDialogs::information( QtYabause::translate( "Yabause is not initialized, can't open CD Rom." ) );
		return;
	}
	const QString fn = CommonDialogs::getExistingDirectory( QtYabause::settings()->value( "Recents/CDs" ).toString(), QtYabause::translate( "Choose a cdrom drive/mount point" ) );
	if ( !fn.isEmpty() )
	{
		QtYabause::settings()->setValue( "Recents/CDs", fn );
		if ( Cs2ChangeCDCore( QtYabause::defaultCDCore().id, strdup( fn.toAscii().constData() ) ) == 0 )
		{
			YabauseReset();
			if ( !aEmulationRun->isChecked() )
				aEmulationRun->trigger();
			refreshStatesActions();
		}
	}
}

void UIYabause::on_mFileSaveState_triggered( QAction* a )
{
	if ( a == aFileSaveStateAs )
		return;
	YabauseLocker locker( mYabauseThread );
	if ( YabSaveStateSlot( QtYabause::settings()->value( "General/SaveStates", QApplication::applicationDirPath() ).toString().toAscii().constData(), a->data().toInt() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't save state file" ) );
	else
		refreshStatesActions();
}

void UIYabause::on_mFileLoadState_triggered( QAction* a )
{
	if ( a == aFileLoadStateAs )
		return;
	YabauseLocker locker( mYabauseThread );
	if ( YabLoadStateSlot( QtYabause::settings()->value( "General/SaveStates", QApplication::applicationDirPath() ).toString().toAscii().constData(), a->data().toInt() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't load state file" ) );
}

void UIYabause::on_aFileSaveStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getSaveFileName( QtYabause::settings()->value( "General/SaveStates", QApplication::applicationDirPath() ).toString(), QtYabause::translate( "Choose a file to save your state" ), QtYabause::translate( "Yabause Save State (*.yss)" ) );
	if ( fn.isNull() )
		return;
	if ( YabSaveState( fn.toAscii().constData() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't save state file" ) );
}

void UIYabause::on_aFileLoadStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getOpenFileName( QtYabause::settings()->value( "General/SaveStates", QApplication::applicationDirPath() ).toString(), QtYabause::translate( "Select a file to load your state" ), QtYabause::translate( "Yabause Save State (*.yss)" ) );
	if ( fn.isNull() )
		return;
	if ( YabLoadState( fn.toAscii().constData() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't load state file" ) );
	else
		aEmulationRun->trigger();
}

void UIYabause::on_aFileScreenshot_triggered()
{
	YabauseLocker locker( mYabauseThread );
	// images filter that qt can write
	QStringList filters;
	foreach ( QByteArray ba, QImageWriter::supportedImageFormats() )
		if ( !filters.contains( ba, Qt::CaseInsensitive ) )
			filters << QString( ba ).toLower();
	for ( int i = 0; i < filters.count(); i++ )
		filters[i] = QtYabause::translate( "%1 Images (*.%2)" ).arg( filters[i].toUpper() ).arg( filters[i] );
	
	// take screenshot of gl view
	QImage screenshot = mYabauseGL->grabFrameBuffer();
	
	// request a file to save to to user
	const QString s = CommonDialogs::getSaveFileName( QString(), QtYabause::translate( "Choose a location for your screenshot" ), filters.join( ";;" ) );
	
	// write image if ok
	if ( !s.isEmpty() )
		if ( !screenshot.save( s ) )
			CommonDialogs::information( QtYabause::translate( "An error occur while writing the screenshot." ) );
}

void UIYabause::on_aFileQuit_triggered()
{ close(); }

void UIYabause::on_aEmulationRun_triggered()
{
	if ( mYabauseThread->emulationPaused() )
	{
		if ( mYabauseThread->runEmulation() )
		{
			aEmulationRun->setEnabled( false );
			aEmulationPause->setEnabled( true );
			aEmulationReset->setEnabled( true );
		}
	}
}

void UIYabause::on_aEmulationPause_triggered()
{
	if ( !mYabauseThread->emulationPaused() )
	{
		aEmulationRun->setEnabled( true );
		aEmulationPause->setEnabled( false );
		aEmulationReset->setEnabled( true );
		mYabauseThread->pauseEmulation();
	}
}

void UIYabause::on_aEmulationReset_triggered()
{ mYabauseThread->resetEmulation( true ); }

void UIYabause::on_aEmulationFrameSkipLimiter_toggled( bool toggled )
{
	if ( toggled )
		EnableAutoFrameSkip();
	else
		DisableAutoFrameSkip();
}

void UIYabause::on_aToolsBackupManager_triggered()
{
	YabauseLocker locker( mYabauseThread );
	if ( mYabauseThread->init() == -1 )
	{
		CommonDialogs::information( QtYabause::translate( "Yabause is not initialized, can't manage backup ram." ) );
		return;
	}
	UIBackupRam( this ).exec();
}

void UIYabause::on_aToolsCheatsList_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UICheats( this ).exec();
}

void UIYabause::on_aToolsTransfer_triggered()
{}

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

void UIYabause::on_aHelpEmuCompatibility_triggered()
{ QDesktopServices::openUrl( QUrl( aHelpEmuCompatibility->statusTip() ) ); }

void UIYabause::on_aHelpAbout_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIAbout( window() ).exec();
}

void UIYabause::on_aSound_triggered()
{
	// show volume widget
	QWidget* ab = toolBar->widgetForAction( aSound );
	fSound->move( ab->mapToGlobal( ab->rect().bottomLeft() ) );
	fSound->show();
}

void UIYabause::on_aVideoDriver_triggered()
{
	// set current core the selected one in the combo list
	if ( VIDCore )
	{
		cbVideoDriver->blockSignals( true );
		for ( int i = 0; VIDCoreList[i] != NULL; i++ )
		{
			if ( VIDCoreList[i]->id == VIDCore->id )
			{
				cbVideoDriver->setCurrentIndex( cbVideoDriver->findData( VIDCore->id ) );
				break;
			}
		}
		cbVideoDriver->blockSignals( false );
	}
	//  show video core widget
	QWidget* ab = toolBar->widgetForAction( aVideoDriver );
	fVideoDriver->move( ab->mapToGlobal( ab->rect().bottomLeft() ) );
	fVideoDriver->show();
}

void UIYabause::on_cbSound_toggled( bool toggled )
{
	if ( toggled )
		ScspUnMuteAudio();
	else
		ScspMuteAudio();
	cbSound->setIcon( QIcon( toggled ? ":/actions/sound.png" : ":/actions/mute.png" ) );
}

void UIYabause::on_sVolume_valueChanged( int value )
{ ScspSetVolume( value ); }

void UIYabause::on_cbVideoDriver_currentIndexChanged( int id )
{
	VideoInterface_struct* core = QtYabause::getVDICore( cbVideoDriver->itemData( id ).toInt() );
	if ( core )
	{
		if ( VideoChangeCore( core->id ) == 0 )
			mYabauseGL->updateView( mYabauseGL->size() );
	}
}
