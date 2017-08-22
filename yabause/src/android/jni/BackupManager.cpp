#include "BackupManager.h"
#include "bios.h"
#include "json/json.h"
#include <android/log.h>
#include <memory>
#include <iostream>
#include <sstream>

#define LOGTAG "BackupManager"
using namespace Json;
using std::unique_ptr;
using std::ostringstream;

BackupManager::BackupManager() {
  devices_ = BupGetDeviceList(&numbupdevices_);
}

BackupManager::~BackupManager() {
  
}

int convJsontoString( const Json::Value in, string & out ){

  StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "   ";  // or whatever you like
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

  ostringstream stream;
  writer->write(in,&stream);
  out = stream.str();

  return 0;
}

int BackupManager::getDevicelist( string & jsonstr ) {

  Json::Value root; 
  Json::Value item;

  root["devices"] = Json::Value(Json::arrayValue);
	for ( int i = 0; i < numbupdevices_ ; i++ ){
    item["name"] = devices_[i].name;
    item["id"] = devices_[i].id;
    root["devices"].append(item);
  }

  convJsontoString(root,jsonstr);
  __android_log_print(ANDROID_LOG_INFO,LOGTAG,jsonstr.c_str());

  return 0;
}

int BackupManager::getFilelist( int deviceid, string & jsonstr ) {
  
  Json::Value root; 
  Json::Value item;

  saves_ = BupGetSaveList( deviceid, &numsaves_);
  root["saves"] = Json::Value(Json::arrayValue);
	for ( int i = 0; i < numbupdevices_ ; i++ ){
    item["filename"] = saves_[i].filename;
    item["comment"] = saves_[i].comment;
    item["language"] = saves_[i].language;
    item["year"] = saves_[i].year;
    item["month"] = saves_[i].month;
    item["day"] = saves_[i].day;
    item["hour"] = saves_[i].hour;
    item["minute"] = saves_[i].minute;
    item["week"] = saves_[i].week;
    item["datasize"] = saves_[i].datasize;
    item["blocksize"] = saves_[i].blocksize;
    root["saves"].append(item);
  }

  convJsontoString(root,jsonstr);
  __android_log_print(ANDROID_LOG_INFO,LOGTAG,jsonstr.c_str());

  return 0;
}

int BackupManager::deletefile( int device, const string & filename ) {
  
  return BupDeleteSave( device, filename.c_str() );

}


int BackupManager::getFile( int index, string & jsonstr ) {
  
  return 0;
}


int BackupManager::putFile( const string & jsonstr ) {
  
  return 0;
}
