#include "Settings.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMainWindow>

QString Settings::mProgramName;
QString Settings::mProgramVersion;

QString getIniFile( const QString& s )
{
#ifdef Q_OS_MAC
	return QString( "%1/../%2.ini" ).arg( QApplication::applicationDirPath() ).arg( s );
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
