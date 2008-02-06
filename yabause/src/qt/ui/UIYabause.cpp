#include "UIYabause.h"
#include "Settings.h"
#include "UISettings.h"
#include "UIAbout.h"
#include "../YabauseGL.h"
#include "QtYabause.h"

#include <QTimer>
#include <QTextEdit>
#include <QFileDialog>
#include <QMessageBox>

void qAppendLog( const char* s )
{ QtYabause::mainWindow()->appendLog( s ); }

UIYabause::UIYabause( QWidget* parent )
	: QMainWindow( parent )
{
	// setup dialog
	setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose );
	
	// create glcontext
	mYabauseGL = new YabauseGL;
	//mYabauseGL->setFocusPolicy( Qt::StrongFocus );
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
	
	// start emulation
	mYabauseThread->startEmulation();
	
	// show settings dialog
	QTimer::singleShot( 0, aYabauseSettings, SLOT( trigger() ) );
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

void UIYabause::swapBuffers()
{ mYabauseGL->swapBuffers(); }

void UIYabause::appendLog( const char* s )
{
	teLog->moveCursor( QTextCursor::End );
	teLog->append( s );
}

void UIYabause::sizeRequested( const QSize& s )
{ resize( s ); }

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
{ mYabauseThread->resetEmulation(); }

void UIYabause::on_aYabauseTransfer_triggered()
{
}

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
	QPixmap screenshot = mYabauseGL->renderPixmap();
	
	// request a file to save to to user
	const QString s = QFileDialog::getSaveFileName( window(), tr( "Choose a location for your screenshot" ), QString(), filters.join( ";;" ) );
	
	// write image if ok
	if ( !s.isEmpty() )
		if ( !screenshot.save( s ) )
			QMessageBox::information( window(), tr( "Informations..." ), tr( "An error occur while writing the screenshot." ) );
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
	if ( YabSaveStateSlot( QtYabause::settings()->value( "General/SaveStates", QDir::homePath() ).toString().toAscii().constData(), a->text().toInt() ) != 0 )
		QMessageBox::information( window(), tr( "Informations..." ), tr( "Couldn't save state file" ) );
}

void UIYabause::on_mYabauseLoadState_triggered( QAction* a )
{
	YabauseLocker locker( mYabauseThread );
	if ( YabLoadStateSlot( QtYabause::settings()->value( "General/SaveStates", QDir::homePath() ).toString().toAscii().constData(), a->text().toInt() ) != 0 )
		QMessageBox::information( window(), tr( "Informations..." ), tr( "Couldn't load state file" ) );
}

void UIYabause::on_aYabauseSaveStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = QFileDialog::getSaveFileName( window(), tr( "Choose a file to save your state" ), QtYabause::settings()->value( "General/SaveStates", QDir::homePath() ).toString(), tr( "Yabause Save State (*.yss )" ) );
	if ( fn.isNull() )
		return;
	if ( YabSaveState( fn.toAscii().constData() ) != 0 )
		QMessageBox::information( window(), tr( "Informations..." ), tr( "Couldn't save state file" ) );
}

void UIYabause::on_aYabauseLoadStateAs_triggered()
{
	YabauseLocker locker( mYabauseThread );
	const QString fn = QFileDialog::getOpenFileName( window(), tr( "Select a file to load your state" ), QtYabause::settings()->value( "General/SaveStates", QDir::homePath() ).toString(), tr( "Yabause Save State (*.yss )" ) );
	if ( fn.isNull() )
		return;
	if ( YabLoadState( fn.toAscii().constData() ) != 0 )
		QMessageBox::information( window(), tr( "Informations..." ), tr( "Couldn't load state file" ) );
	else
		aYabauseRun->trigger();
}

void UIYabause::on_aYabauseQuit_triggered()
{
	aYabausePause->trigger();
	close();
}

void UIYabause::on_aViewFPS_triggered()
{
	ToggleFPS();
}

void UIYabause::on_aViewLayerVdp1_triggered()
{
	ToggleVDP1();
}

void UIYabause::on_aViewLayerNBG0_triggered()
{
	ToggleNBG0();
}

void UIYabause::on_aViewLayerNBG1_triggered()
{
	ToggleNBG1();
}

void UIYabause::on_aViewLayerNBG2_triggered()
{
	ToggleNBG2();
}

void UIYabause::on_aViewLayerNBG3_triggered()
{
	ToggleNBG3();
}

void UIYabause::on_aViewLayerRBG0_triggered()
{
	ToggleRBG0();
}

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
{ UIAbout( window() ).exec(); }
