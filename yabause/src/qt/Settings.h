#ifndef SETTINGS_H
#define SETTINGS_H
#include <QSettings>

#ifndef PROGRAM_NAME
#define PROGRAM_NAME PACKAGE
#endif

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION VERSION
#endif

class QMainWindow;

class Settings : public QSettings
{
	Q_OBJECT

public:
	Settings( QObject* = 0 );
	~Settings();
	static void setIniInformations( const QString& = PROGRAM_NAME, const QString& = PROGRAM_VERSION );
	static QString programName();
	static QString programVersion();

	virtual void restoreState( QMainWindow* );
	virtual void saveState( QMainWindow* );
	virtual void setDefaultSettings();

protected:
	static QString mProgramName;
	static QString mProgramVersion;
};

#endif // PSETTINGS_H
