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

void qAppendLog( const char* s)
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
   search.clear();
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
	mYabauseGL = new YabauseGL( );
	// and set it as central application widget
        QWidget *container = QWidget::createWindowContainer(mYabauseGL);
        container->setFocusPolicy( Qt::StrongFocus );
        setFocusPolicy( Qt::StrongFocus );
        container->setFocusProxy( this );
	setCentralWidget( container );
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
	mCanLog = true;
	oldMouseX = oldMouseY = 0;
	mouseCaptured = false;

#ifndef SH2_TRACE
	aTraceLogging->setVisible(false);
#endif

	// create emulator thread
	mYabauseThread = new YabauseThread( this );
	// create hide mouse timer
	hideMouseTimer = new QTimer();
	// create mouse cursor timer
	mouseCursorTimer = new QTimer();
	// connections
	connect( mYabauseThread, SIGNAL( requestSize( const QSize& ) ), this, SLOT( sizeRequested( const QSize& ) ) );
	connect( mYabauseThread, SIGNAL( requestFullscreen( bool ) ), this, SLOT( fullscreenRequested( bool ) ) );
	connect( mYabauseThread, SIGNAL( requestVolumeChange( int ) ), this, SLOT( on_sVolume_valueChanged( int ) ) );
	connect( aViewLog, SIGNAL( toggled( bool ) ), mLogDock, SLOT( setVisible( bool ) ) );
	connect( mLogDock->toggleViewAction(), SIGNAL( toggled( bool ) ), aViewLog, SLOT( setChecked( bool ) ) );
	connect( mYabauseThread, SIGNAL( error( const QString&, bool ) ), this, SLOT( errorReceived( const QString&, bool ) ) );
	connect( mYabauseThread, SIGNAL( pause( bool ) ), this, SLOT( pause( bool ) ) );
	connect( mYabauseThread, SIGNAL( reset() ), this, SLOT( reset() ) );
	connect( hideMouseTimer, SIGNAL( timeout() ), this, SLOT( hideMouse() ));
	connect( mouseCursorTimer, SIGNAL( timeout() ), this, SLOT( cursorRestore() ));
	connect( mYabauseThread, SIGNAL( toggleEmulateMouse( bool ) ), this, SLOT( toggleEmulateMouse( bool ) ) );

	// Load shortcuts
	VolatileSettings* vs = QtYabause::volatileSettings();
	QList<QAction *> actions = findChildren<QAction *>();
	foreach ( QAction* action, actions )
	{
		if (action->text().isEmpty())
			continue;

		QString text = vs->value(QString("Shortcuts/") + action->text(), "").toString();
		if (text.isEmpty())
			continue;
		action->setShortcut(text);
	}

	// retranslate widgets
	QtYabause::retranslateWidget( this );

	QList<QAction *> actionList = menubar->actions();
	for(int i = 0;i < actionList.size();i++) {
		addAction(actionList.at(i));
	}

	restoreGeometry( vs->value("General/Geometry" ).toByteArray() );
	container->setMouseTracking(true);
	setMouseTracking(true);
	mouseXRatio = mouseYRatio = 1.0;
	emulateMouse = false;
	mouseSensitivity = vs->value( "Input/GunMouseSensitivity", 100 ).toInt();
	showMenuBarHeight = menubar->height();
	translations = QtYabause::getTranslationList();

	mIsCdIn = false;


}

UIYabause::~UIYabause()
{
	mCanLog = false;
}

void UIYabause::showEvent( QShowEvent* e )
{
	QMainWindow::showEvent( e );
	
	if ( !mInit )
	{
		LogStart();
		LogChangeOutput( DEBUG_CALLBACK, (char*)qAppendLog );
		VolatileSettings* vs = QtYabause::volatileSettings();

		if ( vs->value( "View/Menubar" ).toInt() == BD_ALWAYSHIDE )
			menubar->hide();
		if ( vs->value( "View/Toolbar" ).toInt() == BD_ALWAYSHIDE )
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

	if (isFullScreen())
		// Need to switch out of full screen or the geometry settings get saved
		fullscreenRequested( false );
	Settings* vs = QtYabause::settings();
	vs->setValue( "General/Geometry", saveGeometry() );
	vs->sync();

	QMainWindow::closeEvent( e );
}

void UIYabause::keyPressEvent( QKeyEvent* e )
{ 
	if (emulateMouse && mouseCaptured && e->key() == Qt::Key_Escape)
		mouseCaptured = false;
	else
		PerKeyDown( e->key() ); 
}

void UIYabause::keyReleaseEvent( QKeyEvent* e )
{ PerKeyUp( e->key() ); }

void UIYabause::leaveEvent( QEvent* e )
{
	if (emulateMouse && mouseCaptured)
	{
		// lock cursor to center
		int midX = geometry().x()+(width()/2); // widget global x
		int midY = geometry().y()+menubar->height()+toolBar->height()+(height()/2); // widget global y

		QPoint newPos(midX, midY);
		this->cursor().setPos(newPos);
	}
}

void UIYabause::mousePressEvent( QMouseEvent* e )
{ 
	if (emulateMouse && !mouseCaptured)
	{
		this->setCursor(Qt::BlankCursor);
		mouseCaptured = true;
	}
	else
		PerKeyDown( (1 << 31) | e->button() );
}

void UIYabause::mouseReleaseEvent( QMouseEvent* e )
{ 
	PerKeyUp( (1 << 31) | e->button() );
}

void UIYabause::hideMouse()
{
	this->setCursor(Qt::BlankCursor);
	hideMouseTimer->stop();
}

void UIYabause::cursorRestore()
{
	this->setCursor(Qt::ArrowCursor);
	mouseCursorTimer->stop();
}

void UIYabause::mouseMoveEvent( QMouseEvent* e )
{ 
	int midX = geometry().x()+(width()/2); // widget global x
	int midY = geometry().y()+menubar->height()+toolBar->height()+(height()/2); // widget global y

	int x = (e->x()-(width()/2))*mouseXRatio;
	int y = ((menubar->height()+toolBar->height()+(height()/2))-e->y())*mouseYRatio;
	int minAdj = mouseSensitivity/100;

	// If minimum movement is less than x, wait until next pass to apply	
	if (abs(x) < minAdj) x = 0;
	if (abs(y) < minAdj) y = 0;

	if (mouseCaptured)
		PerAxisMove((1 << 30), x, y);

	VolatileSettings* vs = QtYabause::volatileSettings();

	if (!isFullScreen())
	{
		if (emulateMouse && mouseCaptured)
		{
			// lock cursor to center
			QPoint newPos(midX, midY);
			this->cursor().setPos(newPos);
			this->setCursor(Qt::BlankCursor);
			return;
		}
		else
			this->setCursor(Qt::ArrowCursor);
	}
	else
	{
		if (emulateMouse && mouseCaptured)
		{
			this->setCursor(Qt::BlankCursor);
			return;
		}
		else if (vs->value( "View/Menubar" ).toInt() == BD_SHOWONFSHOVER)
		{
			if (e->y() < showMenuBarHeight)
				menubar->show();
			else
				menubar->hide();
		}

		hideMouseTimer->start(3 * 1000);
		this->setCursor(Qt::ArrowCursor);
	}
}

void UIYabause::resizeEvent( QResizeEvent* event )
{

    //	if (event->oldSize().width() != event->size().width())
    //fixAspectRatio(event->size().width(), event->size().height());

	QMainWindow::resizeEvent( event );
}

void UIYabause::adjustHeight(int & height)
{
   // Compensate for menubar and toolbar
   VolatileSettings* vs = QtYabause::volatileSettings();
   if (vs->value("View/Menubar").toInt() != BD_ALWAYSHIDE)
      height += menubar->height();
   if (vs->value("View/Toolbar").toInt() != BD_ALWAYSHIDE)
      height += toolBar->height();
}

void UIYabause::swapBuffers()
{ 
	mYabauseGL->swapBuffers();
}

void UIYabause::appendLog( const char* s )
{

	if (! mCanLog)
	{
		qWarning( "%s", s );
		return;
	}

	teLog->moveCursor( QTextCursor::End );
	teLog->insertPlainText( s );
        teLog->moveCursor( QTextCursor::End );

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
		return;
	}
	else
	{
		width=s.width();
		height=s.height();
	}

	mouseXRatio = 320.0 / (float)width * 2.0 * (float)mouseSensitivity / 100.0;
	mouseYRatio = 240.0 / (float)height * 2.0 * (float)mouseSensitivity / 100.0;

	// Compensate for menubar and toolbar
	VolatileSettings* vs = QtYabause::volatileSettings();
	if (vs->value( "View/Menubar" ).toInt() != BD_ALWAYSHIDE)
		height += menubar->height();
	if (vs->value( "View/Toolbar" ).toInt() != BD_ALWAYSHIDE)
		height += toolBar->height();

	resize( width, height ); 
}

void UIYabause::fixAspectRatio( int width , int height )
{
	int aspectRatio = QtYabause::volatileSettings()->value( "Video/AspectRatio").toInt();

      if (this->isFullScreen()) {
        mYabauseGL->resize(width, height);
      }
      else{
        int heightOffset = toolBar->height()+menubar->height();
        switch(aspectRatio) {
          case 0:
            height = 3 * ((float) width / 4);
            adjustHeight(height );
            setFixedHeight(height);
            break;
          case 2:
            height = 9 * ((float) width / 16);
            adjustHeight(height );
            setFixedHeight(height);
            break;
          default:
            break;
        }
        mouseYRatio = 240.0 / (float)height * 2.0 * (float)mouseSensitivity / 100.0;
      }
}

void UIYabause::toggleFullscreen( int width, int height, bool f, int videoFormat )
{
}

void UIYabause::fullscreenRequested( bool f )
{
	if ( isFullScreen() && !f )
	{
#ifdef USE_UNIFIED_TITLE_TOOLBAR
		setUnifiedTitleAndToolBarOnMac( true );
#endif
		showNormal();

		VolatileSettings* vs = QtYabause::volatileSettings();
		int menubarHide = vs->value( "View/Menubar" ).toInt();
		if ( menubarHide == BD_HIDEFS ||
			  menubarHide == BD_SHOWONFSHOVER)
			menubar->show();
		if ( vs->value( "View/Toolbar" ).toInt() == BD_HIDEFS )
			toolBar->show();

		setCursor(Qt::ArrowCursor);
		hideMouseTimer->stop();
	}
	else if ( !isFullScreen() && f )
	{
#ifdef USE_UNIFIED_TITLE_TOOLBAR
		setUnifiedTitleAndToolBarOnMac( false );
#endif
		VolatileSettings* vs = QtYabause::volatileSettings();

		setMaximumSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
		setMinimumSize( 0,0 );
		QPoint ps;
		ps.setX(0);
		ps.setY(0);
		this->move(ps);

		showFullScreen();

		if ( vs->value( "View/Menubar" ).toInt() == BD_HIDEFS )
			menubar->hide();
		if ( vs->value( "View/Toolbar" ).toInt() == BD_HIDEFS )
			toolBar->hide();

		hideMouseTimer->start(3 * 1000);
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
	if ( UISettings(&translations, window() ).exec() )
	{
		VolatileSettings* vs = QtYabause::volatileSettings();
		aEmulationFrameSkipLimiter->setChecked( vs->value( "General/EnableFrameSkipLimiter" ).toBool() );
		aViewFPS->setChecked( vs->value( "General/ShowFPS" ).toBool() );
		mouseSensitivity = vs->value( "Input/GunMouseSensitivity" ).toInt();
		
		if(isFullScreen())
		{
			if ( vs->value( "View/Menubar" ).toInt() == BD_HIDEFS || vs->value( "View/Menubar" ).toInt() == BD_ALWAYSHIDE )
				menubar->hide();
			else
				menubar->show();

			if ( vs->value( "View/Toolbar" ).toInt() == BD_HIDEFS || vs->value( "View/Toolbar" ).toInt() == BD_ALWAYSHIDE )
				toolBar->hide();
			else
				toolBar->show();
		}
		else
		{
			if ( vs->value( "View/Menubar" ).toInt() == BD_ALWAYSHIDE )
				menubar->hide();
			else
				menubar->show();

			if ( vs->value( "View/Toolbar" ).toInt() == BD_ALWAYSHIDE )
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
			newhash["General/EnableEmulatedBios"]!=hash["General/EnableEmulatedBios"] ||
			newhash["Advanced/Region"]!=hash["Advanced/Region"] ||
			newhash["Cartridge/Type"]!=hash["Cartridge/Type"] ||
			newhash["Memory/Path"]!=hash["Memory/Path"] ||
			newhash["MpegROM/Path" ]!=hash["MpegROM/Path" ] ||
			newhash["Advanced/SH2Interpreter" ]!=hash["Advanced/SH2Interpreter" ] ||
         newhash["Advanced/68kCore"] != hash["Advanced/68kCore"] ||
			newhash["General/CdRom"]!=hash["General/CdRom"] ||
			newhash["General/CdRomISO"]!=hash["General/CdRomISO"] ||
			newhash["General/ClockSync"]!=hash["General/ClockSync"] ||
			newhash["General/FixedBaseTime"]!=hash["General/FixedBaseTime"]
		)
		{
			if ( mYabauseThread->pauseEmulation( true, true ) )
				refreshStatesActions();
			return;
		}
#ifdef HAVE_LIBMINI18N
		if(newhash["General/Translation"] != hash["General/Translation"])
		{
			mini18n_close();
			retranslateUi(this);
			if ( QtYabause::setTranslationFile() == -1 )
				qWarning( "Can't set translation file" );
			QtYabause::retranslateApplication();
		}
#endif
		if(newhash["Video/VideoCore"] != hash["Video/VideoCore"])
			on_cbVideoDriver_currentIndexChanged(newhash["Video/VideoCore"].toInt());
		
		if(newhash["General/ShowFPS"] != hash["General/ShowFPS"])
			SetOSDToggle(newhash["General/ShowFPS"].toBool());

		if (newhash["General/EnableMultiThreading"] != hash["General/EnableMultiThreading"] ||
			 newhash["General/NumThreads"] != hash["General/NumThreads"])
		{
			if (newhash["General/EnableMultiThreading"].toBool())
			{
				int num = newhash["General/NumThreads"].toInt() < 1 ? 1 : newhash["General/NumThreads"].toInt();
				VIDSoftSetVdp1ThreadEnable(num == 1 ? 0 : 1);
				VIDSoftSetNumLayerThreads(num);
				VIDSoftSetNumPriorityThreads(num);
			}
			else
			{
				VIDSoftSetVdp1ThreadEnable(0);
				VIDSoftSetNumLayerThreads(1);
				VIDSoftSetNumPriorityThreads(1);
			}
		}

		
		if (newhash["Sound/SoundCore"] != hash["Sound/SoundCore"])
			ScspChangeSoundCore(newhash["Sound/SoundCore"].toInt());

		if (newhash["Video/WindowWidth"] != hash["Video/WindowWidth"] || newhash["Video/WindowHeight"] != hash["Video/WindowHeight"] ||
          newhash["View/Menubar"] != hash["View/Menubar"] || newhash["View/Toolbar"] != hash["View/Toolbar"] || 
			 newhash["Input/GunMouseSensitivity"] != hash["Input/GunMouseSensitivity"])
			sizeRequested(QSize(newhash["Video/WindowWidth"].toInt(),newhash["Video/WindowHeight"].toInt()));
		
		if (newhash["Video/FullscreenWidth"] != hash["Video/FullscreenWidth"] || 
			newhash["Video/FullscreenHeight"] != hash["Video/FullscreenHeight"] ||
			newhash["Video/Fullscreen"] != hash["Video/Fullscreen"])
		{
			bool f = isFullScreen();
			if (f)
				fullscreenRequested( false );
			fullscreenRequested( f );
		}

		mYabauseThread->reloadControllers();
		refreshStatesActions();
	}
}

void UIYabause::on_aFileOpenISO_triggered()
{
        YabauseLocker locker(mYabauseThread);

	if (mIsCdIn){
		mYabauseThread->OpenTray();
		mIsCdIn = false;
	}
	else{
		const QString fn = CommonDialogs::getOpenFileName( QtYabause::volatileSettings()->value( "Recents/ISOs" ).toString(), QtYabause::translate( "Select your iso/cue/bin/zip file" ), QtYabause::translate( "CD Images (*.iso *.cue *.bin *.mds *.ccd *.zip)" ) );
	        if ( !fn.isEmpty() )
	        {
		  VolatileSettings* vs = QtYabause::volatileSettings();
		  const int currentCDCore = vs->value( "General/CdRom" ).toInt();
		  const QString currentCdRomISO = vs->value( "General/CdRomISO" ).toString();
		
		  QtYabause::settings()->setValue( "Recents/ISOs", fn );
		
		  vs->setValue( "autostart", false );
		  vs->setValue( "General/CdRom", ISOCD.id );
		  vs->setValue( "General/CdRomISO", fn );
                  vs->setValue("General/PlaySSF", false);

		  if (mYabauseThread->CloseTray() != 0) {
                    mYabauseThread->pauseEmulation( false, true );
                  }
		  mIsCdIn = true;
		
		  refreshStatesActions();
	        }
	}
}

void UIYabause::on_aFileOpenSSF_triggered()
{
   YabauseLocker locker(mYabauseThread);

   const QString fn = CommonDialogs::getOpenFileName(
      QtYabause::volatileSettings()->value("Recents/SSFs").toString(), 
      QtYabause::translate("Select your ssf file"), 
      QtYabause::translate("Sega Saturn Sound Format files (*.ssf *.minissf)"));

   if (!fn.isEmpty())
   {
      VolatileSettings* vs = QtYabause::volatileSettings();

      QtYabause::settings()->setValue("Recents/SSFs", fn);

      vs->setValue("autostart", false);
      vs->setValue("General/SSFPath", fn);
      vs->setValue("General/PlaySSF", true);

      mYabauseThread->pauseEmulation(false, true);

      refreshStatesActions();
   }
}

void UIYabause::on_aFileOpenCDRom_triggered()
{
	YabauseLocker locker( mYabauseThread );
	QStringList list = getCdDriveList();
	int current = list.indexOf(QtYabause::volatileSettings()->value( "Recents/CDs").toString());
	QString fn = QInputDialog::getItem(this, QtYabause::translate("Open CD Rom"), 
													QtYabause::translate("Choose a cdrom drive/mount point") + ":",
													list, current, false);
	if (!fn.isEmpty())
	{
		VolatileSettings* vs = QtYabause::volatileSettings();
		const int currentCDCore = vs->value( "General/CdRom" ).toInt();
		const QString currentCdRomISO = vs->value( "General/CdRomISO" ).toString();

		QtYabause::settings()->setValue( "Recents/CDs", fn );

		vs->setValue( "autostart", false );
		vs->setValue( "General/CdRom", QtYabause::defaultCDCore().id );
		vs->setValue( "General/CdRomISO", fn );
      vs->setValue("General/PlaySSF", false);

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
	const QString fn = CommonDialogs::getSaveFileName( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString(), QtYabause::translate( "Choose a file to save your state" ), QtYabause::translate( "Kronos Save State (*.yss)" ) );
	if ( fn.isNull() )
		return;
	if ( YabSaveState( fn.toLatin1().constData() ) != 0 )
		CommonDialogs::information( QtYabause::translate( "Couldn't save state file" ) );
}

void UIYabause::on_aFileLoadStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = CommonDialogs::getOpenFileName( QtYabause::volatileSettings()->value( "General/SaveStates", getDataDirPath() ).toString(), QtYabause::translate( "Select a file to load your state" ), QtYabause::translate( "Kronos Save State (*.yss)" ) );
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

#if defined(HAVE_LIBGL) && !defined(QT_OPENGL_ES_1) && !defined(QT_OPENGL_ES_2)
	glReadBuffer(GL_FRONT);
#endif

	// take screenshot of gl view
	QImage screenshot = mYabauseGL->grabFramebuffer();
	
	// request a file to save to to user
	QString s = CommonDialogs::getSaveFileName( QString(), QtYabause::translate( "Choose a location for your screenshot" ), filters.join( ";;" ) );

	// if the user didn't provide a filename extension, we force it to png
	QFileInfo qfi( s );
	if ( qfi.suffix().isEmpty() )
		s += ".png";
	
	// write image if ok
	if ( !s.isEmpty() )
	{
		QImageWriter iw( s );
		if ( !iw.write( screenshot ))
		{
			CommonDialogs::information( QtYabause::translate( "An error occur while writing the screenshot: " + iw.errorString()) );
		}
	}
}

void UIYabause::on_aFileQuit_triggered()
{ close(); }

void UIYabause::on_aEmulationRun_triggered()
{
	if ( mYabauseThread->emulationPaused() )
	{
		mYabauseThread->pauseEmulation( false, false );
		refreshStatesActions();
		if (isFullScreen())
			hideMouseTimer->start(3 * 1000);
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
	Settings* vs = QtYabause::settings();
	vs->setValue( "General/EnableFrameSkipLimiter", toggled );
	vs->sync();

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
		CommonDialogs::information( QtYabause::translate( "Kronos is not initialized, can't manage backup ram." ) );
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
   UICheatSearch cs(this, &search, searchType);
      
   cs.exec();

   search = *cs.getSearchVariables( &searchType);
}

void UIYabause::on_aToolsTransfer_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIMemoryTransfer( mYabauseThread, this ).exec();
}

void UIYabause::on_aViewFPS_triggered( bool toggled )
{
	Settings* vs = QtYabause::settings();
	vs->setValue( "General/ShowFPS", toggled );
	vs->sync();
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

void UIYabause::on_aViewLayerRBG1_triggered()
{ ToggleRBG1(); }

void UIYabause::on_aViewFullscreen_triggered( bool b )
{
	fullscreenRequested( b );
}

void UIYabause::on_aTraceLogging_triggered( bool toggled )
{
#ifdef SH2_TRACE
	SH2SetInsTracing(toggled? 1 : 0);
#endif
	return;
}

void UIYabause::on_aHelpReport_triggered()
{
	QDesktopServices::openUrl(QUrl(aHelpReport->statusTip()));
}

void UIYabause::on_aHelpCompatibilityList_triggered()
{ QDesktopServices::openUrl( QUrl( aHelpCompatibilityList->statusTip() ) ); }

void UIYabause::on_aHelpAbout_triggered()
{
	YabauseLocker locker( mYabauseThread );
	UIAbout( window() ).exec();
}

void UIYabause::on_aSound_triggered()
{
	// show volume widget	
	sVolume->setValue(QtYabause::volatileSettings()->value( "Sound/Volume").toInt());
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
{ 
	ScspSetVolume( value ); 
	Settings* vs = QtYabause::settings();
	vs->setValue("Sound/Volume", value );
}

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

void UIYabause::toggleEmulateMouse( bool enable )
{
	emulateMouse = enable;
}
