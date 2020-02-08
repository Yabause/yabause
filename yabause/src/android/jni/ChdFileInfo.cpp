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

#include "ChdFileInfo.h"
#include <malloc.h>

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


ChdFileInfo::ChdFileInfo(){
  chd = NULL;
  hunk_buffer = NULL;
  current_hunk_id = 0;
}

ChdFileInfo::~ChdFileInfo(){
  if(chd) chd_close(chd);
}

int ChdFileInfo::getHeader( std::string filepath, char * buf, int len  ){

  chd_error error = chd_open(filepath.c_str(), CHD_OPEN_READ, NULL, &chd);
  if (error != CHDERR_NONE) {
    return -1;
  }
  const chd_header * header = chd_get_header(chd);
  if( header == NULL ){
    return -1;
  }

  char * hunk_buffer = (char*)malloc(header->hunkbytes);
  chd_read(chd, 0, hunk_buffer);
  
  //char header_buff[256];
  memcpy(buf,&hunk_buffer[16],len);
  buf[len-1] = 0;

  free(hunk_buffer);
  return 0;
}

extern "C"{

int yprintf( const char * fmt, ... );

JNIEXPORT jbyteArray JNICALL Java_org_uoyabause_android_YabauseRunnable_getGameinfoFromChd( JNIEnv* env, jobject obj, jstring  jpath ){
  
  
  struct mallinfo pre = mallinfo();
  ChdFileInfo * i = new ChdFileInfo();
    jboolean dummy;
  char * rtnstr = (char*)malloc( sizeof(char)*256);
  const char *cpath = env->GetStringUTFChars( jpath, &dummy);
  int rtn = i->getHeader( std::string(cpath), rtnstr, 256);
  env->ReleaseStringUTFChars( jpath, cpath);
  struct mallinfo mid = mallinfo();
  delete i;
  struct mallinfo after = mallinfo();
  yprintf("leaks %d to %d", (mid.uordblks - pre.uordblks) , (after.uordblks - pre.uordblks) );

  //std::string log = GetUnreachableMemoryString();
  //yprintf("leaks %s", log.c_str() );

  jbyteArray jrtn;
  jbyteArray ret = env->NewByteArray(256);
  env->SetByteArrayRegion (ret, 0, 256, (const jbyte*)rtnstr);
  free(rtnstr);
  return ret;

}

}