#include "UIYabause.h"
#include "UISettings.h"
#include "../YabauseGL.h"
#include "../qt_yabause.h"

using namespace Yabause;

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
	
	// no timer yet started
	mTimerId = -1;
}

UIYabause::~UIYabause()
{ on_aYabausePause_triggered(); }

void UIYabause::timerEvent( QTimerEvent* )
{ Yabause::exec(); }

void UIYabause::on_aYabauseSettings_triggered()
{ UISettings( window() ).exec(); }

void UIYabause::on_aYabauseRun_triggered()
{
	if ( mTimerId == -1 )
	{
		mTimerId = startTimer( 0 );
		aYabauseRun->setEnabled( false );
		aYabausePause->setEnabled( true );
		aYabauseReset->setEnabled( true );
	}
}

void UIYabause::on_aYabausePause_triggered()
{
	if ( mTimerId != -1 )
	{
		killTimer( mTimerId );
		mTimerId = -1;
		aYabauseRun->setEnabled( true );
		aYabausePause->setEnabled( false );
		aYabauseReset->setEnabled( true );
	}
}
