#include "UIWaitInput.h"

#include <QTimer>
#include <QKeyEvent>

UIWaitInput::UIWaitInput( PerInterface_struct* c, const QString& pk, QWidget* p )
	: QDialog( p )
{
	// setup dialog
	setupUi( this );

	// set focus
	setFocusPolicy( Qt::StrongFocus );
	setFocus();

	// get core
	mCore = c;
	Q_ASSERT( mCore );

	// remember key to scan
	mPadKey = pk;

	// init core
	if ( mCore->Init() != 0 )
		qWarning( "UIWaitInput: Can't Init Core" );
	
	if ( mCore->canScan == 1 )
	{
		// create timer for input scan
		QTimer* mTimerInputScan = new QTimer( this );
		mTimerInputScan->setInterval( 25 );
		
		// connect
		connect( mTimerInputScan, SIGNAL( timeout() ), this, SLOT( inputScan_timeout() ) );
		
		// start input scan
		mTimerInputScan->start();
	}
}

void UIWaitInput::keyPressEvent( QKeyEvent* e )
{
	if ( mCore->id == PERCORE_QT && e->key() != Qt::Key_Escape )
	{
		mKeyString = QString::number( e->key() );
		PerSetKey( e->key(), mPadKey.toAscii().constData() );
		QDialog::accept();
	}
	QWidget::keyPressEvent( e );
}

void UIWaitInput::inputScan_timeout()
{
	const char* ki = mPadKey.toAscii().constData();
	u32 k = 0;
	mCore->Flush();
	k = mCore->Scan( ki );
	if ( k != 0 )
	{
		delete sender();
		mKeyString = QString::number( k );
		QDialog::accept();
	}
}
