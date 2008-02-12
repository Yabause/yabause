#include "UIWaitInput.h"

#include <QTimer>
#include <QTime>

UIWaitInput::UIWaitInput( PerInterface_struct* c, const QString& pk, QWidget* p )
	: QDialog( p )
{
	// setup dialog
	setupUi( this );

	// get core
	mCore = c;
	Q_ASSERT( mCore );

	// remember key to scan
	mPadKey = pk;

	// init core
	if ( mCore->Init() != 0 )
		qWarning( "UIWaitInput: Can't Init Core" );
	
	// create timer for input scan
	QTimer* mTimerInputScan = new QTimer( this );
	mTimerInputScan->setInterval( 25 );
	
	// connect
	connect( mTimerInputScan, SIGNAL( timeout() ), this, SLOT( inputScan_timeout() ) );
	
	// start input scan
	mTimerInputScan->start();
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
