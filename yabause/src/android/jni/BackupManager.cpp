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

jstring NewStringMS932(JNIEnv *env, const char *sjis)
{
	if (sjis==NULL) return NULL;

	jthrowable ej = env->ExceptionOccurred();
	if (ej!=NULL) env->ExceptionClear();

	int len = 0;
	jbyteArray arrj = NULL;
	jstring encj    = NULL;
	jclass clsj     = NULL;
	jmethodID mj    = NULL;
	jstring strj    = NULL;

	len = strlen(sjis);
	arrj = env->NewByteArray(len);
	if (arrj==NULL) goto END;
	env->SetByteArrayRegion(arrj, 0, len, (jbyte*)sjis);

	encj = env->NewStringUTF("MS932");
	if (encj==NULL) goto END;

	clsj = env->FindClass("java/lang/String");
  if (clsj==NULL) goto END;
  
  //getBiosPath = (*env)->GetMethodID(env, yclass, "getBiosPath", "()Ljava/lang/String;");

	mj = env->GetMethodID(clsj, "<init>", "([BLjava/lang/String;)V");
	if (mj==NULL) goto END;

	strj = (jstring)env->NewObject(clsj, mj, arrj, encj);
	if (strj==NULL) goto END;

	if (ej!=NULL) env->Throw(ej); 
END:
	env->DeleteLocalRef(ej);
	env->DeleteLocalRef(encj);
	env->DeleteLocalRef(clsj);
	env->DeleteLocalRef(arrj);

	return strj;
}

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
    item["name"] = string(devices_[i].name);
    item["id"] = devices_[i].id;
    root["devices"].append(item);
  }

  convJsontoString(root,jsonstr);
  __android_log_print(ANDROID_LOG_INFO,LOGTAG,jsonstr.c_str());

  return 0;
}

int BackupManager::getFilelist( int deviceid, string & jsonstr ) {
  return 0;
/*  
  Json::Value root; 
  Json::Value item;
  Json::Value status;
  devicestatus_struct devstatus;

   __android_log_print(ANDROID_LOG_INFO,LOGTAG,"getFilelist deviceid = %d",deviceid );

  if(saves_!=NULL){
    free(saves_);
  }    

  if( BiosBUPStatusMem(deviceid,&devstatus) != 0 ){
    return -1;
  }
  status["totalsize"] = devstatus.totalsize;
  status["freesize"] = devstatus.freesize;
  root["status"] = status;  
  
  saves_ = BupGetSaveList( deviceid, &numsaves_);
  __android_log_print(ANDROID_LOG_INFO,LOGTAG,"BupGetSaveList numsaves_ = %d",numsaves_ );
  root["saves"] = Json::Value(Json::arrayValue);
  for ( int i = 0; i < numsaves_ ; i++ ){
    item["index"] = i;

    string base64str;
    base64str = base64_encode((const unsigned char*)saves_[i].filename,12);
    item["filename"] = base64str;
    base64str = base64_encode((const unsigned char*)saves_[i].comment,11);
    item["comment"] = base64str;

    item["language"] = saves_[i].language;
    item["year"] = saves_[i].year;
    item["month"] = saves_[i].month;
    item["day"] = saves_[i].day;
    item["hour"] = saves_[i].hour;
    item["minute"] = saves_[i].minute;
    item["week"] = saves_[i].week;
    item["date"] = saves_[i].date;
    item["datasize"] = saves_[i].datasize;
    item["blocksize"] = saves_[i].blocksize;
    root["saves"].append(item);
  }

  current_device_ = deviceid;
  convJsontoString(root,jsonstr);
  __android_log_print(ANDROID_LOG_INFO,LOGTAG,jsonstr.c_str());

  return 0;
*/  
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
 return 0;
 /*
  Json::Value root; 
  Json::Value header;
  Json::Value data;

  string base64str;
  base64str = base64_encode((const unsigned char*)saves_[index].filename,12);
  header["filename"] = base64str;
  base64str = base64_encode((const unsigned char*)saves_[index].comment,11);
  header["comment"] = base64str;
  header["language"] = saves_[index].language;
  header["date"] = saves_[index].date;
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
  base64str = base64_encode((const unsigned char*)buf,bufsize);
  data["content"] = base64str;
  free(buf);
  root["data"] = data;

  convJsontoString(root,jsonstr);
  __android_log_print(ANDROID_LOG_INFO,LOGTAG,jsonstr.c_str());

  return 0;
*/  
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
  return 0;
/*  
  header = root["header"];

  string base64;
  string bin;

  base64 = header["filename"].asString();
  bin = base64_decode(base64);
  strncpy(save.filename,bin.c_str(),12);

  base64 = header["comment"].asString();
  bin = base64_decode(base64);
  strncpy(save.comment,bin.c_str(),11);

  save.language = header["language"].asInt();
  save.date = header["date"].asInt();
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
  base64 = data["content"].asString();
  bin = base64_decode(base64);
  
  int rtn = BiosBUPImport( current_device_, &save, bin.c_str(), bin.size() );
  if( rtn != 0 ) {
    return rtn;
  }

  return 0;
*/  
}


int BackupManager::copy( int target_device, int file_index ) {

  if( current_device_ == target_device ) return -1;

  string tmpjson;
  getFile( file_index, tmpjson );

  int current_device_back = current_device_;
  current_device_ = target_device;

  int rtn = putFile(tmpjson);
  
  current_device_ = current_device_back;

  return rtn;
}
