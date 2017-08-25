#include "BackupManager.h"
#include "bios.h"


#include "json/json.h"
#include <android/log.h>
#include <memory>
#include <iostream>
#include <sstream>
#include "base64.h"

#define LOGTAG "BackupManager"
using namespace Json;
using std::unique_ptr;
using std::ostringstream;

//BackupManager::instance_ = NULL;
BackupManager * BackupManager::instance_ = NULL;

BackupManager::BackupManager() {
  devices_ = BupGetDeviceList(&numbupdevices_);
}

BackupManager::~BackupManager() {
  if(saves_!=NULL){
    free(saves_);
  }  
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

  if(saves_!=NULL){
    free(saves_);
  }
  saves_ = BupGetSaveList( deviceid, &numsaves_);
  root["saves"] = Json::Value(Json::arrayValue);
	for ( int i = 0; i < numbupdevices_ ; i++ ){
    item["index"] = i;
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
  current_device_ = deviceid;
  convJsontoString(root,jsonstr);
  __android_log_print(ANDROID_LOG_INFO,LOGTAG,jsonstr.c_str());

  return 0;
}

int BackupManager::deletefile( int index ) {
  
  return BupDeleteSave( current_device_, saves_[index].filename );

}

int toBase64( const char * buf, string & out  ){

  return 0;
}

int Base64toBin( const string & base64, char ** out  ){
  
    return 0;
}
  

int BackupManager::getFile( int index, string & jsonstr ) {

  Json::Value root; 
  Json::Value header;
  Json::Value data;

  header["filename"] = saves_[index].filename;
  header["comment"] = saves_[index].comment;
  header["language"] = saves_[index].language;
  header["year"] = saves_[index].year;
  header["month"] = saves_[index].month;
  header["day"] = saves_[index].day;
  header["hour"] = saves_[index].hour;
  header["minute"] = saves_[index].minute;
  header["week"] = saves_[index].week;
  header["datasize"] = saves_[index].datasize;
  header["blocksize"] = saves_[index].blocksize;
  root["header"] = header;
  
  char * buf;
  int bufsize;
  int rtn = BiosBUPExport(current_device_, saves_[index].filename, &buf, &bufsize );
  if( rtn != 0 ){
    return rtn;
  }
  data["size"] = bufsize;
  string base64str;

  base64str = base64_encode((const unsigned char*)buf,bufsize);

  data["content"] = buf;
  free(buf);
  root["header"] = data;

  convJsontoString(root,jsonstr);
  __android_log_print(ANDROID_LOG_INFO,LOGTAG,jsonstr.c_str());

  return 0;
}


int BackupManager::putFile( const string & rootstr ) {

  Json::Value root;
  Json::Value header;
  Json::Value data;
  saveinfo_struct save;
  char * buf;
  int size;

  Json::Reader reader;
  bool parsingSuccessful = reader.parse( rootstr.c_str(), root );     //parse process
  if ( !parsingSuccessful ){
      return -1;
  }

  header = root["header"];
  strcpy(save.filename,header["filename"].asString().c_str());
  strcpy(save.comment,header["comment"].asString().c_str());
  save.language = header["language"].asInt();
  save.year = header["year"].asInt();
  save.month = header["month"].asInt();
  save.day = header["day"].asInt();
  save.hour = header["hour"].asInt();
  save.minute = header["minute"].asInt();
  save.week = header["week"].asInt();
  save.datasize = header["datasize"].asInt();
  save.blocksize = header["blocksize"].asInt();
  
  data = root["data"];
  size = data["size"].asInt();
  string base64 = data["content"].asString();
  string bin;
  bin = base64_decode(base64);
  
  int rtn = BiosBUPImport( current_device_, &save, bin.c_str(), bin.size() );
  if( rtn != 0 ) {
    return rtn;
  }

  return 0;
}
