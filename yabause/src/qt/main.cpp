#include <QApplication>

#include "QtYabause.h"

int main( int argc, char** argv )
{
	// create application
	QApplication app( argc, argv );
	// init application
	app.setApplicationName( "Yabause Qt Gui" );
	// init settings
	Settings::setIniInformations();
	// show main window
	QtYabause::mainWindow()->show();
	// connection
	QObject::connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );
	// exec application
	return app.exec();
}
