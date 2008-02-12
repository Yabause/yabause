#ifndef UIWAITINPUT_H
#define UIWAITINPUT_H

#include "ui_UIWaitInput.h"
#include "QtYabause.h"

class UIWaitInput : public QDialog, public Ui::UIWaitInput
{
	Q_OBJECT
public:
	UIWaitInput( PerInterface_struct* core, const QString& padKey, QWidget* parent = 0 );

	inline QString keyString() const { return mKeyString; }

protected:
	PerInterface_struct* mCore;
	QString mPadKey;
	QString mKeyString;
	bool mScanningInput;

	void keyPressEvent( QKeyEvent* event );

protected slots:
	void inputScan_timeout();
};

#endif // UIWAITINPUT_H
