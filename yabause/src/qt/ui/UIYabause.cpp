/*  Copyright 2005 Guillaume Duhamel
	Copyright 2005-2006, 2013 Theo Berkau
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
#include "../VolatileSettings.h"
#include "UISettings.h"
#include "UIBackupRam.h"
#include "UICheats.h"
#include "UICheatSearch.h"
#include "UIDebugSH2.h"
#include "UIDebugVDP1.h"
#include "UIDebugVDP2.h"
#include "UIDebugM68K.h"
#include "UIDebugSCUDSP.h"
#include "UIDebugSCSP.h"
#include "UIMemoryEditor.h"
#include "UIMemoryTransfer.h"
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

extern "C" {
extern VideoInterface_struct *VIDCoreList[];
}

//#define USE_UNIFIED_TITLE_TOOLBAR

void qAppendLog( const char* s )
{
	UIYabause* ui = QtYabause::mainWindow( false );
	
	if ( ui ) {
		ui->appendLog( s );
	}
	else {
		qWarning( "%s", s );
	}
}

UIYabause::UIYabause( QWidget* parent )
	: QMainWindow( parent )
{
	mInit = false;
	searchResults = NULL;
	numSearchResults = 0;
	searchType = 0;

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
	mYabauseGL = new YabauseGL( this );
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
	// create emulator thread
	mYabauseThread = new YabauseThread( this );
	// connectionsdd
	connect( mYabauseThread, SIGNAL( requestSize( const QSize& ) ), this, SLOT( sizeRequested( const QSize& ) ) );
	connect( mYabauseThread, SIGNAL( requestFullscreen( bool ) ), this, SLOT( fullscreenRequested( bool ) ) );
	connect( aViewLog, SIGNAL( toggled( bool ) ), mLogDock, SLOT( setVisible( bool ) ) );
	connect( mLogDock->toggleViewAction(), SIGNAL( toggled( bool ) ), aViewLog, SLOT( setChecked( bool ) ) );
	connect( mYabauseThread, SIGNAL( error( const QString&, bool ) ), this, SLOT( errorReceived( const QString&, bool ) ) );
	connect( mYabauseThread, SIGNAL( pause( bool ) ), this, SLOT( pause( bool ) ) );
	connect( mYabauseThread, SIGNAL( reset() ), this, SLOT( reset() ) );
	// retranslate widgets
	QtYabause::retranslateWidget( this );

	QList<QAction *> actionList = menubar->actions();
	for(int i = 0;i < actionList.size();i++) {
		addAction(actionList.at(i));
	}

	VolatileSettings* vs = QtYabause::volatileSettings();
	restoreGeometry( vs->value("General/Geometry" ).toByteArray() );
   mYabauseGL->setMouseTracking(true);
   setMouseTracking(true);
}

void UIYabause::showEvent( QShowEvent* e )
{
	QMainWindow::showEvent( e );
	
	if ( !mInit )
	{
		LogStart();
		LogChangeOutput( DEBUG_CALLBACK, (char*)qAppendLog );
		VolatileSettings* vs = QtYabause::volatileSettings();

		if ( vs->value( "View/Menubar" ).toInt() == 2 )
			menubar->hide();
		if ( vs->value( "View/Toolbar" ).toInt() == 2 )
			toolBar->hide();
		if ( vs->value( "autostart" ).toBool() )
			aEmulationRun->trigger();
		aEmulationFrameSkipLimiter->setChecked( vs->value( "General/EnableFrameSkipLimiter" ).toBool() );
		aViewFPS->setChecked( vs->value( "General/ShowFPS" ).toBool() );
		mInit = true;
	}
}

void UIYabause::closeEvent( QCloseEvent* e )
{
	aEmulationPause->trigger();
	LogStop();

	Settings* vs = QtYabause::settings();
	vs->setValue( "General/Geometry", saveGeometry() );
	vs->sync();

	QMainWindow::closeEvent( e );
}

void UIYabause::keyPressEvent( QKeyEvent* e )
{ PerKeyDown( e->key() ); }

void UIYabause::keyReleaseEvent( QKeyEvent* e )
{ PerKeyUp( e->key() ); }

void UIYabause::mousePressEvent( QMouseEvent* e )
{ 
   PerKeyDown( (1 << 31) | e->button() );
}

void UIYabause::mouseReleaseEvent( QMouseEvent* e )
{ 
   PerKeyUp( (1 << 31) | e->button() );
}

void UIYabause::mouseMoveEvent( QMouseEvent* e )
{ 
   PerAxisMove((1 << 30), e->x()-oldMouseX, oldMouseY-e->y());
   oldMouseX = e->x();
   oldMouseY = e->y();
}

void UIYabause::swapBuffers()
{ 
   mYabauseGL->swapBuffers(); 
   mYabauseGL->makeCurrent();
}

void UIYabause::appendLog( const char* s )
{
	teLog->moveCursor( QTextCursor::End );
	teLog->append( s );

	VolatileSettings* vs = QtYabause::volatileSettings();
	if (( !mLogDock->isVisible( )) && ( vs->value( "View/LogWindow" ).toInt() == 1 )) {
		mLogDock->setVisible( true );
	}
}

bool UIYabause::eventFilter( QObject* o, QEvent* e )
{
	if ( e->type() == QEvent::Hide )
		setFocus();
	return QMainWindow::eventFilter( o, e );
}

void UIYabause::errorReceived( const QString& error, bool internal )
{
	if ( internal ) {
		appendLog( error.toLocal8Bit().constData() );
	}
	else {
		CommonDialogs::information( error );
	}
}

void UIYabause::sizeRequested( const QSize& s )
{
	int heightOffset = toolBar->height()+menubar->height();
	int width, height;
	if (s.isNull())
	{
		width=640;
		height=480;
	}
	else
	{
		width=s.width();
		height=s.height();
	}
	// Compensate for menubar and toolbar
	VolatileSettings* vs = QtYabause::volatileSettings();
	if (vs->value( "View/Menubar" ).toInt() != 2)
		height += menubar->height();
	if (vs->value( "View/Toolbar" ).toInt() != 2)
		height += toolBar->height();
	resize( width, height ); 
}

void UIYabause::fullscreenRequested( bool f )
{
	if ( isFullScreen() && !f )
	{
		showNormal();
#ifdef USE_UNIFIED_TITLE_TOOLBAR
		setUnifiedTitleAndToolBarOnMac( true );
#endif
		VolatileSettings* vs = QtYabause::volatileSettings();
		if ( vs->value( "View/Menubar" ).toInt() == 1 )
			menubar->show();
		if ( vs->value( "View/Toolbar" ).toInt() == 1 )
			toolBar->show();
	}
	else if ( !isFullScreen() && f )
	{
#ifdef USE_UNIFIED_TITLE_TOOLBAR
		setUnifiedTitleAndToolBarOnMac( false );
#endif
		showFullScreen();
		VolatileSettings* vs = QtYabause::volatileSettings();
		if ( vs->value( "View/Menubar" ).toInt() == 1 )
			menubar->hide();
		if ( vs->value( "View/Toolbar" ).toInt() == 1 )
			toolBar->hide();
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
	const QString statesPath = QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString();
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
	Settings *s = (QtYabause::settings());
	QHash<QString, QVariant> hash;
	const QStringList keys = s->allKeys();
	Q_FOREACH(QString key, keys) {
		hash[key] = s->value(key);
	}

	YabauseLocker locker( mYabauseThread );
	if ( UISettings( window() ).exec() )
	{
		VolatileSettings* vs = QtYabause::volatileSettings();
		aEmulationFrameSkipLimiter->setChecked( vs->value( "General/EnableFrameSkipLimiter" ).toBool() );
		aViewFPS->setChecked( vs->value( "General/ShowFPS" ).toBool() );
		
		if(isFullScreen())
		{
			if ( vs->value( "View/Menubar" ).toInt() == 1 || vs->value( "View/Menubar" ).toInt() == 2)
				menubar->hide();
			else
				menubar->show();

			if ( vs->value( "View/Toolbar" ).toInt() == 1 || vs->value( "View/Toolbar" ).toInt() == 2 )
				toolBar->hide();
			else
				toolBar->show();
		}
		else
		{
			if ( vs->value( "View/Menubar" ).toInt() == 2 )
				menubar->hide();
			else
				menubar->show();

			if ( vs->value( "View/Toolbar" ).toInt() == 2 )
				toolBar->hide();
			else
				toolBar->show();
		}

		
		//only reset if bios, region, cart,  back up, mpeg, sh2, m68k are changed
		Settings *ss = (QtYabause::settings());
		QHash<QString, QVariant> newhash;
		const QStringList newkeys = ss->allKeys();
		Q_FOREACH(QString key, newkeys) {
			newhash[key] = ss->value(key);
		}
		if(newhash["General/Bios"]!=hash["General/Bios"] ||
			newhash["Advanced/Region"]!=hash["Advanced/Region"] ||
			newhash["Cartridge/Type"]!=hash["Cartridge/Type"] ||
			newhash["Memory/Path"]!=hash["Memory/Path"] ||
			newhash["MpegROM/Path" ]!=hash["MpegROM/Path" ] ||
			newhash["Advanced/SH2Interpreter" ]!=hash["Advanced/SH2Interpreter" ] ||
			newhash["General/CdRom"]!=hash["General/CdRom"] ||
			newhash["General/CdRomISO"]!=hash["General/CdRomISO"]
		)
		{
			if ( mYabauseThread->pauseEmulation( true, true ) )
				refreshStatesActions();
			return;
		}
		if(newhash["Video/VideoCore"] != hash["Video/VideoCore"])
			on_cbVideoDriver_currentIndexChanged(newhash["Video/VideoCore"].toInt());
		
		if(newhash["General/ShowFPS"] != hash["General/ShowFPS"])
			SetOSDToggle(newhash["General/ShowFPS"].toBool());
		
		if (newhash["Sound/SoundCore"] != hash["Sound/SoundCore"])
			ScspChangeSoundCore(newhash["Sound/SoundCore"].toInt());
		
		if (newhash["Video/Width"] != hash["Video/Width"] || newhash["Video/Height"] != hash["Video/Height"] ||
          newhash["View/Menubar"] != hash["View/Menubar"] || newhash["View/Toolbar"] != hash["View/Toolbar"])
			sizeRequested(QSize(newhash["Video/Width"].toInt(),newhash["Video/Height"].toInt()));
		
		if (newhash["Video/Fullscreen"] != hash["Video/Fullscreen"])
			fullscreenRequested( newhash["Video/Fullscreen"].toBool());
		
		if (newhash["Video/VideoFormat"] != hash["Video/VideoFormat"])
			YabauseSetVideoFormat(newhash["Video/VideoFormat"].toInt());

		mYabauseThread->reloadControllers();
		refreshStatesActions();
	}
}

void UIYabause::on_aFileOpenISO_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getOpenFileName( QtYabause::volatileSettings()->value( "Recents/ISOs" ).toString(), QtYabause::translate( "Select your iso/cue/bin file" ), QtYabause::translate( "CD Images (*.iso *.cue *.bin *.mds)" ) );
	if ( !fn.isEmpty() )
	{
		VolatileSettings* vs = QtYabause::volatileSettings();
		const int currentCDCore = vs->value( "General/CdRom" ).toInt();
		const QString currentCdRomISO = vs->value( "General/CdRomISO" ).toString();
		
		QtYabause::settings()->setValue( "Recents/ISOs", fn );
		
		vs->setValue( "autostart", false );
		vs->setValue( "General/CdRom", ISOCD.id );
		vs->setValue( "General/CdRomISO", fn );
		
		mYabauseThread->pauseEmulation( false, true );
		
		refreshStatesActions();
	}
}

void UIYabause::on_aFileOpenCDRom_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getExistingDirectory( QtYabause::volatileSettings()->value( "Recents/CDs" ).toString(), QtYabause::translate( "Choose a cdrom drive/mount point" ) );
	if ( !fn.isEmpty() )
	{
		VolatileSettings* vs = QtYabause::volatileSettings();
		const int currentCDCore = vs->value( "General/CdRom" ).toInt();
		const QString currentCdRomISO = vs->value( "General/CdRomISO" ).toString();
		
		QtYabause::settings()->setValue( "Recents/CDs", fn );
		
		vs->setValue( "autostart", false );
		vs->setValue( "General/CdRom", QtYabause::defaultCDCore().id );
		vs->setValue( "General/CdRomISO", fn );
		
		mYabauseThread->pauseEmulation( false, true );
		
		refreshStatesActions();
	}
}

void UIYabause::on_mFileSaveState_triggered( QAction* a )
{
	if ( a == aFileSaveStateAs )
		return;
	YabauseLocker locker( mYabauseThread );
	if ( YabSaveStateSlot( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString().toLatin1().constData(), a->data().toInt() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't save state file" ) );
	else
		refreshStatesActions();
}

void UIYabause::on_mFileLoadState_triggered( QAction* a )
{
	if ( a == aFileLoadStateAs )
		return;
	YabauseLocker locker( mYabauseThread );
	if ( YabLoadStateSlot( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString().toLatin1().constData(), a->data().toInt() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't load state file" ) );
}

void UIYabause::on_aFileSaveStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getSaveFileName( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString(), QtYabause::translate( "Choose a file to save your state" ), QtYabause::translate( "Yabause Save State (*.yss)" ) );
	if ( fn.isNull() )
		return;
	if ( YabSaveState( fn.toLatin1().constData() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't save state file" ) );
}

void UIYabause::on_aFileLoadStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getOpenFileName( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString(), QtYabause::translate( "Select a file to load your state" ), QtYabause::translate( "Yabause Save State (*.yss)" ) );
	if ( fn.isNull() )
		return;
	if ( YabLoadState( fn.toLatin1().constData() ) != 0 )
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

#ifdef HAVE_LIBGL
	glReadBuffer(GL_FRONT);
#endif

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
		mYabauseThread->pauseEmulation( false, false );
      refreshStatesActions();
   }
}

void UIYabause::on_aEmulationPause_triggered()
{
	if ( !mYabauseThread->emulationPaused() )
		mYabauseThread->pauseEmulation( true, false );
}

void UIYabause::on_aEmulationReset_triggered()
{ mYabauseThread->resetEmulation(); }

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
	if ( mYabauseThread->init() < 0 )
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

void UIYabause::on_aToolsCheatSearch_triggered()
{
   YabauseLocker locker( mYabauseThread );
   UICheatSearch cs(this, searchResults, numSearchResults, searchType);
      
   cs.exec();

   searchResults = cs.getSearchVariables(&numSearchResults, &searchType);
}

void UIYabause::on_aToolsTransfer_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIMemoryTransfer( mYabauseThread, this ).exec();
}

void UIYabause::on_aViewFPS_triggered( bool toggled )
{
	VolatileSettings* vs = QtYabause::volatileSettings();
	vs->setValue( "General/ShowFPS", toggled );
	SetOSDToggle(toggled ? 1 : 0);
}

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

void UIYabause::breakpointHandlerMSH2(bool displayMessage)
{
	YabauseLocker locker( mYabauseThread );
	if (displayMessage)
		CommonDialogs::information( QtYabause::translate( "Breakpoint Reached" ) );
	UIDebugSH2( true, mYabauseThread, this ).exec();
}

void UIYabause::breakpointHandlerSSH2(bool displayMessage)
{
	YabauseLocker locker( mYabauseThread );
	if (displayMessage)
		CommonDialogs::information( QtYabause::translate( "Breakpoint Reached" ) );
	UIDebugSH2( false, mYabauseThread, this ).exec();
}

void UIYabause::breakpointHandlerM68K()
{
	YabauseLocker locker( mYabauseThread );
	CommonDialogs::information( QtYabause::translate( "Breakpoint Reached" ) );
	UIDebugM68K( mYabauseThread, this ).exec();
}

void UIYabause::breakpointHandlerSCUDSP()
{
	YabauseLocker locker( mYabauseThread );
	CommonDialogs::information( QtYabause::translate( "Breakpoint Reached" ) );
	UIDebugSCUDSP( mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugMSH2_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugSH2( true, mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugSSH2_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugSH2( false, mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugVDP1_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugVDP1( this ).exec();
}

void UIYabause::on_aViewDebugVDP2_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugVDP2( this ).exec();
}

void UIYabause::on_aViewDebugM68K_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugM68K( mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugSCUDSP_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugSCUDSP( mYabauseThread, this ).exec();
}

void UIYabause::on_aViewDebugSCSP_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIDebugSCSP( this ).exec();
}

void UIYabause::on_aViewDebugMemoryEditor_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIMemoryEditor( mYabauseThread, this ).exec();
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
		ScspUnMuteAudio(SCSP_MUTE_USER);
	else
		ScspMuteAudio(SCSP_MUTE_USER);
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
			mYabauseGL->updateView();
	}
}

void UIYabause::pause( bool paused )
{
	mYabauseGL->updateView();
	
	aEmulationRun->setEnabled( paused );
	aEmulationPause->setEnabled( !paused );
	aEmulationReset->setEnabled( !paused );
}

void UIYabause::reset()
{
	mYabauseGL->updateView();
}
