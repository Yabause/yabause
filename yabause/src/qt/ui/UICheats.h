#ifndef UICHEATS_H
#define UICHEATS_H

#include "ui_UICheats.h"
#include "../QtYabause.h"

class UICheats : public QDialog, public Ui::UICheats
{
	Q_OBJECT

public:
	UICheats( QWidget* parent = 0 );

protected:
	cheatlist_struct* mCheats;

	void addCode( int id );
	void addARCode( const QString& code, const QString& description );
	void addRawCode( int type, const QString& address, const QString& value, const QString& description );

protected slots:
	void on_twCheats_itemSelectionChanged();
	void on_twCheats_itemDoubleClicked( QTreeWidgetItem* item, int column );
	void on_pbDelete_clicked();
	void on_pbClear_clicked();
	void on_pbAR_clicked();
	void on_pbRaw_clicked();
	void on_pbFile_clicked();
};

#endif // UICHEATS_H
