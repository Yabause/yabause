#ifndef UIABOUT_H
#define UIABOUT_H

#include "ui_UIAbout.h"

class UIAbout : public QDialog, public Ui::UIAbout
{
	Q_OBJECT

public:
	UIAbout( QWidget* owner = 0 );
};

#endif // UIABOUT_H
