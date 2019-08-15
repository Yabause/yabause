#include "ChdFileInfo.h"

ChdFileInfo::ChdFileInfo(){
  chd = NULL;
  hunk_buffer = NULL;
  current_hunk_id = 0;
}

ChdFileInfo::~ChdFileInfo(){
  if(chd) chd_close(chd);
}

int ChdFileInfo::getHeader( std::string filepath, std::string & out ){

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
  
  char header_buff[256];
  memcpy(header_buff,&hunk_buffer[16],256);
  header_buff[255] = 0;

  out = string(header_buff);

  free(hunk_buffer);
  return 0;
}

extern "C"{

JNIEXPORT jstring JNICALL Java_org_uoyabause_android_YabauseRunnable_getGameinfoFromChd( JNIEnv* env, jobject obj, jstring  jpath ){
  ChdFileInfo * i = new ChdFileInfo();
  jboolean dummy;
  std::string rtnstr;
  const char *cpath = env->GetStringUTFChars( jpath, &dummy);
  int rtn = i->getHeader( std::string(cpath), rtnstr);
  env->ReleaseStringUTFChars( jpath, cpath);
  delete i;
  return env->NewStringUTF(rtnstr.c_str());
}

}