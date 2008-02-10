#ifndef UICHEATRAW_H
#define UICHEATRAW_H

#include "ui_UICheatRaw.h"

class QButtonGroup;

class UICheatRaw : public QDialog, public Ui::UICheatRaw
{
	Q_OBJECT

public:
	UICheatRaw( QWidget* parent = 0 );

	inline int type() const { return mButtonGroup->checkedId(); }

protected:
	QButtonGroup* mButtonGroup;
};

#endif // UICHEATRAW_H
