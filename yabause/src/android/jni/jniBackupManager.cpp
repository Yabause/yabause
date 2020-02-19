
/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <string>
#include <cstdint>
#include "bios.h"
#include <jni.h>
#include "BackupManager.h"
#include "jniBackupManager.h"

using std::string;

extern "C"{

JNIEXPORT jstring JNICALL Java_org_uoyabause_android_YabauseRunnable_getDevicelist(JNIEnv* env) {
  BackupManager * i = BackupManager::getInstance();
  string jsonstr;
  i->getDevicelist(jsonstr);
  return env->NewStringUTF(jsonstr.c_str());
}

JNIEXPORT jstring JNICALL Java_org_uoyabause_android_YabauseRunnable_getFilelist(JNIEnv* env, jobject obj, jint deviceid ) {
  BackupManager * i = BackupManager::getInstance();
  string jsonstr;
  i->getFilelist(deviceid,jsonstr);
  return env->NewStringUTF(jsonstr.c_str());
}

JNIEXPORT jint JNICALL Java_org_uoyabause_android_YabauseRunnable_deletefile(JNIEnv* env, jobject obj, jint index ) {
  BackupManager * i = BackupManager::getInstance();
  string jsonstr;
  return i->deletefile(index);
}

JNIEXPORT jstring JNICALL Java_org_uoyabause_android_YabauseRunnable_getFile(JNIEnv* env, jobject obj, jint index  ) {
  BackupManager * i = BackupManager::getInstance();
  string jsonstr;
  i->getFile(index,jsonstr);
  return env->NewStringUTF(jsonstr.c_str());
}

JNIEXPORT jint JNICALL Java_org_uoyabause_android_YabauseRunnable_putFile(JNIEnv* env, jobject obj, jstring jsonstr  ) {
  BackupManager * i = BackupManager::getInstance();
  jboolean dummy;
  const char *cpath = env->GetStringUTFChars( jsonstr, &dummy);
  int rtn = i->putFile(string(cpath));
  env->ReleaseStringUTFChars( jsonstr, cpath);
  return rtn;
}

JNIEXPORT jint JNICALL Java_org_uoyabause_android_YabauseRunnable_copy(JNIEnv* env, jobject obj, jint target, jint file  ) {
  BackupManager * i = BackupManager::getInstance();
  return i->copy(target,file);
}


}