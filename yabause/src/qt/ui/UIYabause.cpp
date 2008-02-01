#include "UIYabause.h"
#include "UISettings.h"
#include "../YabauseGL.h"
#include "../YabauseThread.h"

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

//#ifndef QT_NO_DEBUG
	LogStart();
	//LogChangeOutput( DEBUG_CALLBACK, (char*)appendLog );
//#endif
	
	// create thread
	mYabauseThread = new YabauseThread( this );
	mYabauseThread->startEmulation();
}

UIYabause::~UIYabause()
{
	mYabauseThread->stopEmulation();

//#ifndef QT_NO_DEBUG
	LogStop();
//#endif
}

void UIYabause::appendLog( const char* s )
{ mLog += s; }

void UIYabause::swapBuffers()
{ mYabauseGL->swapBuffers(); }

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
