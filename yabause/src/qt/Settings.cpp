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
#include "Settings.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMainWindow>

QString Settings::mProgramName;
QString Settings::mProgramVersion;

QString getIniFile( const QString& s )
{
#if defined Q_OS_MAC
	return QString( "%1/../%2.ini" ).arg( QApplication::applicationDirPath() ).arg( s );
#elif defined Q_OS_WIN
	return QString( "%1/%2.ini" ).arg( QApplication::applicationDirPath() ).arg( s );
#else
	return QString( "%1/.%2/%2.ini" ).arg( QDir::homePath() ).arg( s );
#endif
}

Settings::Settings( QObject* o )
	: QSettings( QDir::convertSeparators( getIniFile( mProgramName ) ), QSettings::IniFormat, o )
{ beginGroup( mProgramVersion ); }

Settings::~Settings()
{ endGroup(); }

void Settings::setIniInformations( const QString& pName, const QString& pVersion )
{
	mProgramName = pName;
	mProgramVersion = pVersion;
}

QString Settings::programName()
{ return mProgramName; }

QString Settings::programVersion()
{ return mProgramVersion; }

void Settings::restoreState( QMainWindow* w )
{
	if ( !w )
		return;
	w->restoreState( value( "MainWindow/State" ).toByteArray() );
	QPoint p = value( "MainWindow/Position" ).toPoint();
	QSize s = value( "MainWindow/Size" ).toSize();
	if ( !p.isNull() && !s.isNull() )
	{
		w->resize( s );
		w->move( p );
	}
	if ( value( "MainWindow/Maximized", true ).toBool() )
		w->showMaximized();
}

void Settings::saveState( QMainWindow* w )
{
	if ( !w )
		return;
	setValue( "MainWindow/Maximized", w->isMaximized() );
	setValue( "MainWindow/Position", w->pos() );
	setValue( "MainWindow/Size", w->size() );
	setValue( "MainWindow/State", w->saveState() );
}

void Settings::setDefaultSettings()
{
}
