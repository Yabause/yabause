#ifndef UICHEATAR_H
#define UICHEATAR_H

#include "ui_UICheatAR.h"

class UICheatAR : public QDialog, public Ui::UICheatAR
{
	Q_OBJECT

public:
	UICheatAR( QWidget* parent = 0 );
};

#endif // UICHEATAR_H
