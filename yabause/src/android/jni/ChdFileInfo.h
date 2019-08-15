#include <string>
#include <cstdint>
#include <jni.h>

using std::string;

extern "C" {
#include "chd.h"
}

class ChdFileInfo {
  chd_file *chd;
  char * hunk_buffer;
  int current_hunk_id;
public:  
  ChdFileInfo();
  ~ChdFileInfo();
  int getHeader( std::string filepath, std::string & header );
};

extern "C"{
JNIEXPORT jstring JNICALL Java_org_uoyabause_android_YabauseRunnable_getGameinfoFromChd( JNIEnv* env, jobject obj, jstring  jpath );
}
