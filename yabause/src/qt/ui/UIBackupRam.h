/*	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>

	This file is part of Yabause.

	Yabause is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Yabause is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Yabause; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#ifndef UIBACKUPRAM_H
#define UIBACKUPRAM_H

#include "ui_UIBackupRam.h"

class BackupManager;

#include <firebase/database.h>
#include <firebase/auth.h>

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <firebase/database.h>
using firebase::database::DatabaseReference;

#include <firebase/storage.h>
using firebase::storage::StorageReference;

#include <firebase/storage/metadata.h>
using firebase::storage::Metadata;

#include <firebase/variant.h>
using firebase::Variant;



class BackupItem {
public:	
	string filename;
	string comment;
	int64_t language;
	string savedate;
	int64_t datasize;
	int64_t blocksize;

	int year;
	int month;
	int day;
	int hour;
	int minute;

	string url;
	string key;
};

class UIBackupRam : public QDialog, 
 public Ui::UIBackupRam,  
 public firebase::database::ValueListener,
 public firebase::auth::AuthStateListener 
{
	Q_OBJECT

public:
	UIBackupRam( QWidget* parent = 0 );
	virtual ~UIBackupRam();

	void OnValueChanged( const firebase::database::DataSnapshot& snapshot) override;
    void OnCancelled(const firebase::database::Error& error_code, const char* error_message) override;
 	void OnAuthStateChanged(firebase::auth::Auth* auth) override ;

	char *tmpbuf;

  static UIBackupRam * instance;

protected:
	void refreshSaveList();
  BackupManager * backupman_;
  vector<BackupItem> cloud_items;

  // upload data
  DatabaseReference new_post;
  StorageReference fileref;
  std::map<string, Variant> new_data;
  std::string new_key;
	std::string del_key;
  Metadata *  new_metadata = nullptr;
  std::string new_backup_data;


  void finishedGetBackupData();

signals:
  void errorMessage(QString info);

protected slots:
	void on_cbDeviceList_currentIndexChanged( int id );
	void on_lwSaveList_itemSelectionChanged();
	void on_lwCloudSaveList_itemSelectionChanged();
	void on_pbDelete_clicked();
	void on_pbFormat_clicked();
  void on_pbExport_clicked();
  void on_pbImport_clicked();

  void on_pbCopyFromCloud_clicked();
  void on_pbCopyFromLocal_clicked();
	void on_pbDeleteCloudItem_clicked();

  void onErrorMessage(QString info);

};

#endif // UIBACKUPRAM_H
