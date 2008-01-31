#include <QApplication>

#include "qt_yabause.h"

int main( int argc, char** argv )
{
	// create application
	QApplication app( argc, argv );

	// init application
	app.setApplicationName( "yabause" );
	
#ifndef QT_NO_DEBUG
	LogStart();
	LogChangeOutput( DEBUG_CALLBACK, (char*)Yabause::log );
#endif
	
	// init settings
	Settings::setIniInformations();
	
	// init yabause
	Yabause::init();
	
	// show main window
	Yabause::mainWindow()->show();

	// connection
	QObject::connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );

	// start application
	int i = app.exec();
	
	// de init yabause
	Yabause::deInit();
	
#ifndef QT_NO_DEBUG
	LogStop();
#endif
	
	return i;
}
