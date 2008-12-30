#ifndef UIPADSETTING_H
#define UIPADSETTING_H

#include "ui_UIPadSetting.h"
#include "QtYabause.h"

#include <QMap>

class QTimer;

class UIPadSetting : public QDialog, public Ui::UIPadSetting
{
	Q_OBJECT

public:
	UIPadSetting( PerInterface_struct* core, PerPad_struct* padbits, uint port, uint pad, QWidget* parent = 0 );
	virtual ~UIPadSetting();

protected:
	PerInterface_struct* mCore;
	PerPad_struct* mPadBits;
	uint mPort;
	uint mPad;
	u8 mPadKey;
	QTimer* mTimer;
	QMap<QToolButton*, u8> mPadButtons;
	QMap<u8, QString> mPadNames;
	
	void keyPressEvent( QKeyEvent* event );
	void setPadKey( u32 key );
	void loadPadSettings();

protected slots:
	void tbButton_clicked();
	void timer_timeout();
};

#endif // UIPADSETTING_H
