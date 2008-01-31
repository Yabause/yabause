#ifndef UISETTINGS_H
#define UISETTINGS_H

#include "ui_UISettings.h"

class UISettings : public QDialog, public Ui::UISettings
{
	Q_OBJECT
	
public:
	UISettings( QWidget* parent = 0 );
	~UISettings();
};

#endif // UISETTINGS_H
