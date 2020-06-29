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

#include "UIYabause.h"
#include "UIBackupRam.h"
#include "../CommonDialogs.h"
#include "../QtYabause.h"
#include "BackupManager.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QDebug>
#include <QTextCodec>

#include <firebase/app.h>
#include <firebase/auth.h>
#include <firebase/database.h>
#include <firebase/storage/metadata.h>

using firebase::Future;
using firebase::database::DataSnapshot;
using firebase::storage::Metadata;
using firebase::storage::Storage;

#include <iostream>
#include <string>
#include <map>
using std::cout;
using std::endl;

u32 currentbupdevice = 0;
deviceinfo_struct *devices = NULL;
int numbupdevices = 0;
saveinfo_struct *saves = NULL;
int numsaves = 0;

void UIBackupRam::OnValueChanged(
		const firebase::database::DataSnapshot &snapshot)
{

	std::vector<DataSnapshot> cd = snapshot.children();
	cloud_items.clear();
	for (int i = 0; i < cd.size(); i++)
	{
		BackupItem tmp;
		if (cd[i].Child("filename").is_valid() && cd[i].Child("filename").value().is_string())
		{
			tmp.filename = cd[i].Child("filename").value().string_value();
			std::cout << "filename: " << tmp.filename << endl;
		}

		if (cd[i].Child("comment").is_valid() && cd[i].Child("comment").value().is_string())
		{
			tmp.comment = cd[i].Child("comment").value().string_value();
			std::cout << "comment: " << tmp.comment << endl;
		}

		if (cd[i].Child("savedate").is_valid() && cd[i].Child("savedate").value().is_string())
		{
			tmp.savedate = cd[i].Child("savedate").value().string_value();
			std::cout << "savedate: " << tmp.savedate << endl;
		}

		if (cd[i].Child("datasize").is_valid() && cd[i].Child("datasize").value().is_int64())
		{
			tmp.datasize = cd[i].Child("datasize").value().int64_value();
			std::cout << "datasize: " << tmp.datasize << endl;
		}

		if (cd[i].Child("blocksize").is_valid() && cd[i].Child("blocksize").value().is_int64())
		{
			tmp.blocksize = cd[i].Child("blocksize").value().int64_value();
			std::cout << "blocksize: " << tmp.blocksize << endl;
		}

		if (cd[i].Child("url").is_valid() && cd[i].Child("url").value().is_string())
		{
			tmp.url = cd[i].Child("url").value().string_value();
			std::cout << "url: " << tmp.url << endl;
		}

		if (cd[i].Child("language").is_valid() && cd[i].Child("language").value().is_int64())
		{
			tmp.language = cd[i].Child("language").value().int64_value();
			std::cout << "language: " << tmp.blocksize << endl;
		}

		if (cd[i].Child("key").is_valid() && cd[i].Child("key").value().is_string())
		{
			tmp.key = cd[i].Child("key").value().string_value();
			std::cout << "key: " << tmp.key << endl;
		}

#if 0
          if( cd[i].Child("year").is_valid() && cd[i].Child("year").value().is_int64() ){
            tmp.year = cd[i].Child("year").value().int64_value();
            std::cout << "year: " << tmp.year << endl;
          }
          if( cd[i].Child("month").is_valid() && cd[i].Child("month").value().is_int64() ){
            tmp.month = cd[i].Child("month").value().int64_value();
            std::cout << "month: " << tmp.month << endl;
          }
          if( cd[i].Child("day").is_valid() && cd[i].Child("day").value().is_int64() ){
            tmp.day = cd[i].Child("day").value().int64_value();
            std::cout << "day: " << tmp.day << endl;
          }

          if( cd[i].Child("hour").is_valid() && cd[i].Child("hour").value().is_int64() ){
            tmp.hour = cd[i].Child("hour").value().int64_value();
            std::cout << "hour: " << tmp.hour << endl;
          }

          if( cd[i].Child("minute").is_valid() && cd[i].Child("minute").value().is_int64() ){
            tmp.hour = cd[i].Child("minute").value().int64_value();
            std::cout << "minute: " << tmp.minute << endl;
          }

		char datestring[256];
		sprintf(datestring, "%04d/%02d/%02d %02d:%02d:00",tmp.year+1980,tmp.month,tmp.day,tmp.hour,tmp.minute);
		tmp.savedate = datestring;
#endif
		cloud_items.push_back(tmp);
	}

	// clear listwidget
	lwCloudSaveList->clear();

	// add item to listwidget
	for (int i = 0; i < cloud_items.size(); i++)
		lwCloudSaveList->addItem(cloud_items[i].filename.c_str());

	// set infos about blocks
	//BupGetStats( id, &fs, &ms );
	//lBlocks->setText( QtYabause::translate( "%1/%2 blocks free" ).arg( fs ).arg( ms ) );

	// enable/disable button delete according to available item
	//pbDelete->setEnabled( lwSaveList->count() );

	// select first item in the item list
	if (lwCloudSaveList->count())
		lwCloudSaveList->setCurrentRow(0);
	on_lwCloudSaveList_itemSelectionChanged();
}

void UIBackupRam::OnCancelled(const firebase::database::Error &error_code,
															const char *error_message)
{
}

void UIBackupRam::OnAuthStateChanged(firebase::auth::Auth *auth)
{
	firebase::auth::User *user = auth->current_user();
	if (user != nullptr)
	{
		// User is signed in
		printf("OnAuthStateChanged: signed_in %s\n", user->uid().c_str());
		const std::string displayName = user->display_name();
		const std::string emailAddress = user->email();
		const std::string photoUrl = user->photo_url();

		firebase::database::Database *database = ::firebase::database::Database::GetInstance(UIYabause::getFirebaseApp());
		firebase::database::DatabaseReference dbref = database->GetReference();
		std::string key = "/user-posts/" + user->uid() + "/backup/";
		dbref.Child(key).AddValueListener(this);
	}
	else
	{
		// User is signed out
		printf("OnAuthStateChanged: signed_out\n");
	}
	// ...
}

UIBackupRam::UIBackupRam(QWidget *p)
		: QDialog(p)
{
	backupman_ = BackupManager::getInstance();

	new_metadata = nullptr;

	firebase::auth::Auth *auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
	firebase::auth::User *user = auth->current_user();
	if (user != nullptr)
	{

		printf("signed_in %s\n", user->uid().c_str());

		firebase::database::Database *database = ::firebase::database::Database::GetInstance(UIYabause::getFirebaseApp());
		firebase::database::DatabaseReference dbref = database->GetReference();
		std::string key = "/user-posts/" + user->uid() + "/backup/";
		dbref.Child(key).AddValueListener(this);
	}
	else
	{
		auth->AddAuthStateListener(this);
	}

	//setup dialog
	setupUi(this);
	if (p && !p->isFullScreen())
		setWindowFlags(Qt::Sheet);

	//lwCloudSaveList->setDragDropMode(QAbstractItemView::DragDrop);
	//lwCloudSaveList->setAcceptDrops(true);
	//lwCloudSaveList->setDragDropOverwriteMode(false);
	//lwSaveList->setDragDropMode(QAbstractItemView::DragDrop);
	//lwSaveList->setAcceptDrops(true);
	//lwSaveList->setDragDropOverwriteMode(false);

	// get available devices
	if ((devices = BupGetDeviceList(&numbupdevices)) == NULL)
		return;

	// add to combobox
	for (int i = 0; i < numbupdevices; i++)
		cbDeviceList->addItem(devices[i].name, devices[i].id);

	// get save list for current devices
	refreshSaveList();

	// retranslate widgets
	QtYabause::retranslateWidget(this);
}

UIBackupRam::~UIBackupRam()
{
	firebase::auth::Auth *auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
	firebase::auth::User *user = auth->current_user();
	if (user != nullptr)
	{
		firebase::database::Database *database = ::firebase::database::Database::GetInstance(UIYabause::getFirebaseApp());
		firebase::database::DatabaseReference dbref = database->GetReference();
		std::string key = "/user-posts/" + user->uid() + "/backup/";
		dbref.Child(key).RemoveValueListener(this);
	}
	auth->RemoveAuthStateListener(this);

	if (new_metadata != nullptr)
		delete new_metadata;
}

void UIBackupRam::refreshSaveList()
{
	// blocks
	u32 fs = 0, ms = 0;
	u32 id = cbDeviceList->itemData(cbDeviceList->currentIndex()).toInt();

	// clear listwidget
	lwSaveList->clear();

	// get save list
	saves = BupGetSaveList(id, &numsaves);

	// add item to listwidget
	for (int i = 0; i < numsaves; i++)
		lwSaveList->addItem(saves[i].filename);

	// set infos about blocks
	BupGetStats(id, &fs, &ms);
	lBlocks->setText(QtYabause::translate("%1/%2 blocks free").arg(fs).arg(ms));

	// enable/disable button delete according to available item
	pbDelete->setEnabled(lwSaveList->count());

	// select first item in the item list
	if (lwSaveList->count())
		lwSaveList->setCurrentRow(0);
	on_lwSaveList_itemSelectionChanged();
}

void UIBackupRam::on_cbDeviceList_currentIndexChanged(int)
{
	refreshSaveList();
}

void UIBackupRam::on_lwCloudSaveList_itemSelectionChanged()
{
	// get current save id
	int id = lwCloudSaveList->currentRow();

	// update gui
	if (id != -1)
	{
		leFileName->setText(cloud_items[id].filename.c_str());
		leComment->setText(cloud_items[id].comment.c_str());
		switch (cloud_items[id].language)
		{
		case 0:
			leLanguage->setText(QtYabause::translate("Japanese"));
			break;
		case 1:
			leLanguage->setText(QtYabause::translate("English"));
			break;
		case 2:
			leLanguage->setText(QtYabause::translate("French"));
			break;
		case 3:
			leLanguage->setText(QtYabause::translate("German"));
			break;
		case 4:
			leLanguage->setText(QtYabause::translate("Spanish"));
			break;
		case 5:
			leLanguage->setText(QtYabause::translate("Italian"));
			break;
		default:
			leLanguage->setText(QtYabause::translate("Unknown (%1)").arg(cloud_items[id].language));
			break;
		}
		leComment->setText(cloud_items[id].comment.c_str());
		leDataSize->setText(QString::number(cloud_items[id].datasize));
		leBlockSize->setText(QString::number(cloud_items[id].blocksize));

		leDate->setText(cloud_items[id].savedate.c_str());
	}
	else
	{
		// clear gui
		leFileName->clear();
		leComment->clear();
		leLanguage->clear();
		leDataSize->clear();
		leBlockSize->clear();
		leDate->clear();
	}
}

void UIBackupRam::on_lwSaveList_itemSelectionChanged()
{
	// get current save id
	int id = lwSaveList->currentRow();

	// update gui
	if (id != -1)
	{
		leFileName->setText(saves[id].filename);

		QTextCodec *codec = QTextCodec::codecForName("Shift-JIS");
		leComment->setText(codec->toUnicode(saves[id].comment));
		switch (saves[id].language)
		{
		case 0:
			leLanguage->setText(QtYabause::translate("Japanese"));
			break;
		case 1:
			leLanguage->setText(QtYabause::translate("English"));
			break;
		case 2:
			leLanguage->setText(QtYabause::translate("French"));
			break;
		case 3:
			leLanguage->setText(QtYabause::translate("German"));
			break;
		case 4:
			leLanguage->setText(QtYabause::translate("Spanish"));
			break;
		case 5:
			leLanguage->setText(QtYabause::translate("Italian"));
			break;
		default:
			leLanguage->setText(QtYabause::translate("Unknown (%1)").arg(saves[id].language));
			break;
		}
		leDataSize->setText(QString::number(saves[id].datasize));
		leBlockSize->setText(QString::number(saves[id].blocksize));

		char datestring[256];
		sprintf(datestring, "%04d/%02d/%02d %02d:%02d:00", saves[id].year + 1980, saves[id].month, saves[id].day, saves[id].hour, saves[id].minute);

		leDate->setText(datestring);
	}
	else
	{
		// clear gui
		leFileName->clear();
		leComment->clear();
		leLanguage->clear();
		leDataSize->clear();
		leBlockSize->clear();
		leDate->clear();
	}
}

void UIBackupRam::on_pbDelete_clicked()
{
	if (QListWidgetItem *it = lwSaveList->selectedItems().value(0))
	{
		u32 id = cbDeviceList->itemData(cbDeviceList->currentIndex()).toInt();
		if (CommonDialogs::question(QtYabause::translate("Are you sure you want to delete '%1' ?").arg(it->text())))
		{
			BupDeleteSave(id, it->text().toLatin1().constData());
			refreshSaveList();
		}
	}
}

void UIBackupRam::on_pbFormat_clicked()
{
	u32 id = cbDeviceList->itemData(cbDeviceList->currentIndex()).toInt();
	if (CommonDialogs::question(QtYabause::translate("Are you sure you want to format '%1' ?").arg(devices[id].name)))
	{
		BupFormat(id);
		refreshSaveList();
	}
}

void UIBackupRam::on_pbExport_clicked()
{
	int index = -1;
	if ((index = lwSaveList->currentRow()) >= 0)
	{
		u32 id = cbDeviceList->itemData(cbDeviceList->currentIndex()).toInt();
		string filejson;
		string json;
		backupman_->getFilelist(id, filejson);
		backupman_->getFile(index, json);

		QString fileName = QFileDialog::getSaveFileName(this, tr("Export filen name"), "export.json", tr("JSON(*.json)"));
		FILE *fp = fopen(fileName.toStdString().c_str(), "w");
		if (fp != NULL)
		{
			fwrite(json.c_str(), 1, json.length(), fp);
			fclose(fp);
		}
	}
}

#include <fstream>

void UIBackupRam::on_pbImport_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Import backup file"), "export.json", tr("JSON(*.json)"));
	if (fileName != "")
	{

		//try {
		std::ifstream t(fileName.toStdString());
		std::string str((std::istreambuf_iterator<char>(t)),
										std::istreambuf_iterator<char>());

		u32 id = cbDeviceList->itemData(cbDeviceList->currentIndex()).toInt();
		string filejson;
		backupman_->getFilelist(id, filejson);

		if (backupman_->putFile(str) != 0)
		{
			CommonDialogs::information(QtYabause::translate("Fail to import backup data"));
			return;
		}

		refreshSaveList();
		//}
		//catch (std::exception e) {

		//}
	}
}

void UIBackupRam::finishedGetBackupData()
{
	u32 id = cbDeviceList->itemData(cbDeviceList->currentIndex()).toInt();
	string filejson;

	backupman_->getFilelist(id, filejson);

	std::string str(tmpbuf);
	if (backupman_->putFile(str) != 0)
	{
		CommonDialogs::information(QtYabause::translate("Fail to import backup data"));
		return;
	}

	refreshSaveList();
}

void UIBackupRam::on_pbDeleteCloudItem_clicked()
{
	del_key = "";
	int id = lwCloudSaveList->currentRow();
	if (id == -1)
		return;

	firebase::auth::Auth *auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
	firebase::auth::User *user = auth->current_user();
	if (user == nullptr)
	{
		return;
	}

	// Delte Storage
	Storage *storage = Storage::GetInstance(UIYabause::getFirebaseApp(), "gs://uoyabause.appspot.com");
	StorageReference storage_ref = storage->GetReference();
	cout << "Bucket: " << storage_ref.bucket() << std::endl;
	StorageReference base = storage_ref.Child(user->uid());
	StorageReference backup = base.Child("backup");
	StorageReference fileref = backup.Child(cloud_items[id].key);

	del_key = cloud_items[id].key;

	Future<void> future = fileref.Delete();
	future.OnCompletion([](const firebase::Future<void> &result, void *user_data) {
			UIBackupRam *self = (UIBackupRam *)user_data;
			if (result.status() == firebase::kFutureStatusComplete) {
				if (result.error() == firebase::storage::kErrorNone) {
					firebase::auth::Auth *auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
					firebase::auth::User *user = auth->current_user();
					if (user == nullptr) {
						return;
					}

					firebase::database::Database *database = ::firebase::database::Database::GetInstance(UIYabause::getFirebaseApp());
					firebase::database::DatabaseReference dbref = database->GetReference();
					std::string delete_file = "/user-posts/" + user->uid() + "/backup/" + self->del_key;
					firebase::database::DatabaseReference ref_delete_file = dbref.Child(delete_file);					
					ref_delete_file.RemoveValue();
					self->del_key = "";

				}else{
					std::cout << "Failed: " << result.error() << " " << result.error_message() << std::endl;
					self->del_key = "";
				}
			}else{
				std::cout << "Failed: " << result.error() << " " << result.error_message() << std::endl;
				self->del_key = "";
			}
		}
		,this
	);
}

void UIBackupRam::on_pbCopyFromCloud_clicked()
{
  int id = -1;

  id = lwCloudSaveList->currentRow();
  
  // update gui
	if (id != -1)
	{
    QNetworkRequest request;
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

    request.setUrl(QUrl(cloud_items[id].url.c_str()));

		QNetworkAccessManager dlManager;
		dlManager.setNetworkAccessible(QNetworkAccessManager::Accessible);
		QNetworkReply *reply = dlManager.get(request);

		QEventLoop Loop;
		QObject::connect(reply, SIGNAL(finished()), &Loop, SLOT(quit()));
		Loop.exec();
		QObject::disconnect(reply, SIGNAL(finished()), &Loop, SLOT(quit()));

		qDebug("Result: %d", reply->error());
		if (reply->error() != QNetworkReply::NoError)
		{
			qDebug() << "Error: " << reply->errorString();
			return;
		}

		std::string str = reply->readAll().toStdString();


		u32 id = cbDeviceList->itemData(cbDeviceList->currentIndex()).toInt();
		string filejson;
		backupman_->getFilelist(id, filejson);

		if (backupman_->putFile(str) != 0)
		{
			CommonDialogs::information(QtYabause::translate("Fail to import backup data"));
			return;
		}

		refreshSaveList();

#if 0
		Storage* storage = Storage::GetInstance(UIYabause::getFirebaseApp());
		StorageReference storage_ref = storage->GetReferenceFromUrl( cloud_items[id].url.c_str() );

		const size_t kMaxAllowedSize = 1 * 1024 * 1024;
		tmpbuf = (char*)malloc(kMaxAllowedSize);
		firebase::Future<size_t> result = storage_ref.GetBytes(tmpbuf, kMaxAllowedSize);

		result.OnCompletion(
		[](const firebase::Future<size_t>& result,
			void* user_data) {
  
			if (result.status() == firebase::kFutureStatusComplete) {
				if (result.error() == firebase::storage::kErrorNone) {
					size_t read_size = *result.result();
					UIBackupRam* self = (UIBackupRam*)user_data;
					self->finishedGetBackupData();
				}else{
					std::cout << "Failed: " << result.error_message() << endl;
				}
			}
		}
		,this
		);
#endif
	}
}

void UIBackupRam::on_pbCopyFromLocal_clicked()
{
	int index = -1;
	if ((index = lwSaveList->currentRow()) >= 0)
	{
		u32 id = cbDeviceList->itemData(cbDeviceList->currentIndex()).toInt();
		string filejson;
		backupman_->getFilelist(id, filejson);
    if (backupman_->getFile(index, new_backup_data) != 0) {
      CommonDialogs::information(QtYabause::translate("Fail to export backup data"));
    }

		firebase::auth::Auth *auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
		firebase::auth::User *user = auth->current_user();
		if (user == nullptr)
		{
			return;
		}

		firebase::database::Database *database = ::firebase::database::Database::GetInstance(UIYabause::getFirebaseApp());
		firebase::database::DatabaseReference dbref = database->GetReference();
		std::string baseurl = "/user-posts/" + user->uid() + "/backup/";
		firebase::database::DatabaseReference user_dir = dbref.Child(baseurl);
		new_post = user_dir.PushChild();

		char datestring[256];
		snprintf(datestring, 256, "%04d/%02d/%02d %02d:%02d:00",
						 saves[index].year + 1980,
						 saves[index].month,
						 saves[index].day,
						 saves[index].hour,
						 saves[index].minute);

		new_data["filename"] = saves[index].filename;
		new_data["comment"] = saves[index].comment;
		new_data["language"] = (int)saves[index].language;
		new_data["savedate"] = datestring;
		new_data["datasize"] = (int)saves[index].datasize;
		new_data["blocksize"] = (int)saves[index].blocksize;
		new_data["url"] = "";
		new_data["key"] = new_post.key();

		new_post.UpdateChildren(new_data);

		Storage *storage = Storage::GetInstance(UIYabause::getFirebaseApp(), "gs://uoyabause.appspot.com");
		StorageReference storage_ref = storage->GetReference();
		cout << "Bucket: " << storage_ref.bucket() << std::endl;
		StorageReference base = storage_ref.Child(user->uid());
		StorageReference backup = base.Child("backup");
		fileref = backup.Child(new_post.key());

		char bytesize[256];
		snprintf(bytesize, 256, "%dByte", saves[index].datasize);

		if (new_metadata != nullptr)
		{
			delete new_metadata;
			new_metadata = nullptr;
		}
		new_metadata = new Metadata();
		new_metadata->set_content_type("text/json");
		std::map<std::string, std::string> *custom_metadata = new_metadata->custom_metadata();
		custom_metadata->insert(std::make_pair("dbref", baseurl + new_post.key()));
		custom_metadata->insert(std::make_pair("uid", user->uid()));
		custom_metadata->insert(std::make_pair("filename", saves[index].filename));
		custom_metadata->insert(std::make_pair("comment", saves[index].comment));
		custom_metadata->insert(std::make_pair("size", bytesize));
		custom_metadata->insert(std::make_pair("date", datestring));

		this->new_key = new_post.key();
		Future<Metadata> future = fileref.PutBytes(
				new_backup_data.c_str(),
				new_backup_data.size(),
				*new_metadata);

		future.OnCompletion(
				[](const firebase::Future<Metadata> &result, void *user_data) {
					UIBackupRam *self = (UIBackupRam *)user_data;
					if (result.status() == firebase::kFutureStatusComplete)
					{
						if (result.error() == firebase::storage::kErrorNone)
						{

							firebase::auth::Auth *auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
							firebase::auth::User *user = auth->current_user();
							if (user == nullptr)
							{
								return;
							}

							Storage *storage = Storage::GetInstance(UIYabause::getFirebaseApp());
							StorageReference storage_ref = storage->GetReference().Child(user->uid()).Child("backup").Child(self->new_key);
							Future<std::string> future = storage_ref.GetDownloadUrl();

							future.OnCompletion(
									[](const firebase::Future<std::string> &result, void *user_data) {
										UIBackupRam *self = (UIBackupRam *)user_data;
										if (result.status() == firebase::kFutureStatusComplete)
										{
											if (result.error() == firebase::storage::kErrorNone)
											{
												firebase::auth::Auth *auth = firebase::auth::Auth::GetAuth(UIYabause::getFirebaseApp());
												firebase::auth::User *user = auth->current_user();
												if (user == nullptr)
												{
													return;
												}
												firebase::database::Database *database = ::firebase::database::Database::GetInstance(UIYabause::getFirebaseApp());
												firebase::database::DatabaseReference dbref = database->GetReference();
												std::string baseurl = "/user-posts/" + user->uid() + "/backup/" + self->new_key + "/url/";
												firebase::database::DatabaseReference url_dir = dbref.Child(baseurl);
												url_dir.SetValue(*result.result());
												self->new_key = "";
											}
											else
											{
												std::cout << "Failed: " << result.error() << " " << result.error_message() << std::endl;
												self->new_key = "";
											}
										}
										else
										{
											std::cout << "Failed: " << result.error() << " " << result.error_message() << std::endl;
											self->new_key = "";
										}
									},
									user_data);
						}
						else
						{
							std::cout << "Failed: " << result.error() << " " << result.error_message() << std::endl;
							self->new_key = "";
						}
					}
				},
				this);
		while (this->new_key != "")
		{
			QThread::sleep(1);
		}
	}
}
