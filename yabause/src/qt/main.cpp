/*  Copyright 2005 Guillaume Duhamel
	Copyright 2005-2006 Theo Berkau
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
#include <QApplication>

#include "QtYabause.h"
#include "Settings.h"
#include "ui/UIYabause.h"

int main( int argc, char** argv )
{
	// create application
	QApplication app( argc, argv );
	// init application
	app.setApplicationName( QString( "Qt Yabause v%1 - A Beautiful And Under-rated Saturn Emulator" ).arg( VERSION ) );
	// init settings
	Settings::setIniInformations();
	// set translation file
	if ( QtYabause::setTranslationFile() == -1 )
		qWarning( "Can't set translation file" );
	// show main window
	QtYabause::mainWindow()->setWindowTitle( app.applicationName() );
	QtYabause::mainWindow()->show();
	// connection
	QObject::connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );
	// exec application
	int i = app.exec();
	QtYabause::closeTranslation();
	return i;
}
