#ifndef UIPORTMANAGER_H
#define UIPORTMANAGER_H

#include "ui_UIPortManager.h"
#include "../QtYabause.h"

class UIPortManager : public QGroupBox, public Ui::UIPortManager
{
	Q_OBJECT

public:
	static const QString mSettingsKey;
	static const QString mSettingsType;
	
	UIPortManager( QWidget* parent = 0 );
	virtual ~UIPortManager();
	
	void setPort( uint portId );
	void setCore( PerInterface_struct* core );
	void loadSettings();

protected:
	uint mPort;
	PerInterface_struct* mCore;

protected slots:
	void cbTypeController_currentIndexChanged( int id );
	void tbSetJoystick_clicked();
	void tbClearJoystick_clicked();
};

#endif // UIPORTMANAGER_H
