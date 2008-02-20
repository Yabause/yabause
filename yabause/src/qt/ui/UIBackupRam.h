#ifndef UIBACKUPRAM_H
#define UIBACKUPRAM_H

#include "ui_UIBackupRam.h"

class UIBackupRam : public QDialog, public Ui::UIBackupRam
{
	Q_OBJECT

public:
	UIBackupRam( QWidget* parent = 0 );

protected:
	void refreshSaveList();

protected slots:
	void on_cbDeviceList_currentIndexChanged( int id );
	void on_lwSaveList_itemSelectionChanged();
	void on_pbDelete_clicked();
	void on_pbFormat_clicked();
};

#endif // UIBACKUPRAM_H
