
#include <string>
#include <cstdint>
#include "bios.h"
#include <jni.h>

using std::string;


class BackupManager {

protected:
  BackupManager();
public:  
  static BackupManager * instance_;
  static BackupManager * getInstance(){
    if( instance_==NULL ) instance_ = new BackupManager();
    return instance_; 
  }

  virtual ~BackupManager();
  int getDevicelist( string & jsonstr );
  int getFilelist( int deviceid, string & jsonstr );
  int deletefile( int index );
  int getFile( int index, string & jsonstr );
  int putFile( const string & jsonstr );

protected:  
  uint32_t currentbupdevice_ = 0;
  deviceinfo_struct* devices_ = NULL;
  int32_t numbupdevices_ = 0;
  int32_t current_device_ = -1;
  saveinfo_struct* saves_ = NULL;
  int32_t numsaves_ = 0;

};

extern "C"{

JNIEXPORT jstring JNICALL Java_org_uoyabause_android_YabauseRunnable_getDevicelist(JNIEnv* env) {
  BackupManager * i = BackupManager::getInstance();
  string jsonstr;
  i->getDevicelist(jsonstr);
  return env->NewStringUTF(jsonstr.c_str());
}

JNIEXPORT jstring JNICALL Java_org_uoyabause_android_YabauseRunnable_getFilelist(JNIEnv* env, jint deviceid ) {
  BackupManager * i = BackupManager::getInstance();
  string jsonstr;
  i->getFilelist(deviceid,jsonstr);
  return env->NewStringUTF(jsonstr.c_str());
}

JNIEXPORT jint JNICALL Java_org_uoyabause_android_YabauseRunnable_deletefile(JNIEnv* env, jint index ) {
  BackupManager * i = BackupManager::getInstance();
  string jsonstr;
  return i->deletefile(index);
}

JNIEXPORT jstring JNICALL Java_org_uoyabause_android_YabauseRunnable_getFile(JNIEnv* env, jint index  ) {
  BackupManager * i = BackupManager::getInstance();
  string jsonstr;
  i->getFile(index,jsonstr);
  return env->NewStringUTF(jsonstr.c_str());
}

JNIEXPORT jint JNICALL Java_org_uoyabause_android_YabauseRunnable_putFile(JNIEnv* env, jstring jsonstr  ) {
  BackupManager * i = BackupManager::getInstance();
  jboolean dummy;
  const char *cpath = env->GetStringUTFChars( jsonstr, &dummy);
  int rtn = i->putFile(string(cpath));
  env->ReleaseStringUTFChars( jsonstr, cpath);
  return rtn;
}


}