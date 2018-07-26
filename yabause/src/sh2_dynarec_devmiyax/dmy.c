#include <stdio.h>
#include <string.h>
#include <malloc.h>

#ifdef _WINDOWS
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "threads.h"

int BiosBUPInit( int a ){
 return 0;
}

int BiosHandleFunc( int a ) {
}

void YabThreadLock(YabMutex * mtx) {
}

void YabThreadUnLock(YabMutex * mtx) {
}

YabMutex * YabThreadCreateMutex() {
  return (YabMutex *)0x0001;
}

void YabThreadFreeMutex(YabMutex * mtx) {
}


