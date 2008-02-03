#ifndef UISETTINGS_H
#define UISETTINGS_H

#include "ui_UISettings.h"
#include "QtYabause.h"

class UISettings : public QDialog, public Ui::UISettings
{
	Q_OBJECT
	
public:
	UISettings( QWidget* parent = 0 );
	~UISettings();

protected:
	bool mScanningInput;
	void qSleep( int ms );
	void requestFile( const QString& caption, QLineEdit* edit );
	void requestFolder( const QString& caption, QLineEdit* edit );
	void requestDrive( const QString& caption, QLineEdit* edit );
	void loadCores();
	void loadSettings();
	void saveSettings();

protected slots:
	void inputScan_timeout();
	void tbBrowse_clicked();
	void accept();
};

#endif // UISETTINGS_H
