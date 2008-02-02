#include "UIYabause.h"
#include "UISettings.h"
#include "../YabauseGL.h"
#include "../YabauseThread.h"

#include <QTimer>

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
	// and set it as central application widget
	setCentralWidget( mYabauseGL );
	// set strong focus on glwidget
	//mYabauseGL->setFocusPolicy( Qt::StrongFocus );
	
	// init actions
	aYabausePause->setEnabled( false );
	aYabauseReset->setEnabled( false );

	// start log
	LogStart();
	LogChangeOutput( DEBUG_CALLBACK, (char*)qAppendLog );
	
	// create emulator thread
	mYabauseThread = new YabauseThread( this );
	
	// start emulation
	mYabauseThread->startEmulation();
}

UIYabause::~UIYabause()
{
	// stop emulation
	mYabauseThread->stopEmulation();

	// stop log
	LogStop();
}

void UIYabause::swapBuffers()
{ mYabauseGL->swapBuffers(); }

void UIYabause::appendLog( const char* s )
{
	mLog += s;
	//qWarning( QString( "appendLog: %1" ).arg( s ).toAscii() );
}

void UIYabause::on_aYabauseSettings_triggered()
{ UISettings( window() ).exec(); }

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
{
	mYabauseThread->resetEmulation();
}
