#include <stdio.h>
#include <string.h>
#include <malloc.h> 

#ifdef _WINDOWS
#include <io.h>  
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <iostream>

#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "DynarecSh2.h"
#include "memory_for_test.h"

SH2_struct *MSH2 = NULL;
SH2_struct *SSH2 = NULL;
extern SH2_struct *CurrentSH2;
extern yabsys_struct yabsys;
SH2Interface_struct *SH2Core = NULL;
SH2Interface_struct *SH2CoreList[] = {
  &SH2Dyn,
  &SH2DynDebug,
  NULL
};

int init();
int readProgram(const char * addr, const char * filename);
int reset(SH2_struct *context);




int main(int argv, char * argcv[]) {


  if (argv < 3) {
    printf("Usage: dynalib_test [load address] [file name]");
    return -1;
  }

  if (init() != 0) {
    return -1;
  }

  reset(MSH2);

  if (readProgram(argcv[1], argcv[2]) != 0) {
    return -1;
  }


  return 0;
}

int init() {

  initMemory();
  
  if ((MSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
    return -1;

  MSH2->onchip.BCR1 = 0x0000;
  MSH2->isslave = 0;

  // SSH2
  if ((SSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
    return -1;

  SSH2->onchip.BCR1 = 0x8000;
  SSH2->isslave = 1;

  SH2Core = SH2CoreList[1];  // debug


  // Reset Onchip modules
  CurrentSH2 = MSH2;


  return 0;
}

int reset(SH2_struct *context) {
  int i;

  SH2Core->Reset(context);

  // Reset general registers
  for (i = 0; i < 15; i++)
    SH2Core->SetGPR(context, i, 0x00000000);

  SH2Core->SetSR(context, 0x000000F0);
  SH2Core->SetGBR(context, 0x00000000);
  SH2Core->SetVBR(context, 0x00000000);
  SH2Core->SetMACH(context, 0x00000000);
  SH2Core->SetMACL(context, 0x00000000);
  SH2Core->SetPR(context, 0x00000000);

  // Internal variables
  context->delay = 0x00000000;
  context->cycles = 0;
  context->isIdle = 0;

  context->frc.leftover = 0;
  context->frc.shift = 3;

  context->wdt.isenable = 0;
  context->wdt.isinterval = 1;
  context->wdt.shift = 1;
  context->wdt.leftover = 0;

  // Reset Interrupts
  memset((void *)context->interrupts, 0, sizeof(interrupt_struct) * MAX_INTERRUPTS);
  SH2Core->SetInterrupts(context, 0, context->interrupts);

  // Reset Onchip modules
  CurrentSH2 = context;

  // Reset backtrace
  context->bt.numbacktrace = 0;

  return 0;

}

int readProgram(const char * addr, const char * filename) {

  long file_size;
  char *buffer;
  struct stat stbuf;
  int fd;
  FILE * fp;
  u32 startaddr;

  std::stringstream ss;
  ss << std::hex << addr;
  ss >> startaddr;

  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    printf("Fail to open %s", filename);
    return -1;
  }

  fp = fdopen(fd, "rb");
  if (fp == NULL) {
    printf("Fail to open %s", filename);
    return -1;
  }

  if (fstat(fd, &stbuf) == -1) {
    printf("Fail to fstat %s", filename);
    return -1;
  }

  file_size = stbuf.st_size;

  setromlock(false);
  u32 copyaddr = startaddr;
  for (int i = 0; i < file_size; i++) {
    MappedMemoryWriteByte(copyaddr, fgetc(fp));
    copyaddr++;
  }
  setromlock(true);
  fclose(fp);

	u32 VBR = SH2Core->GetVBR(CurrentSH2);
  SH2Core->SetPC(CurrentSH2, MappedMemoryReadLong(VBR));
  SH2Core->SetGPR(CurrentSH2, 15, MappedMemoryReadLong(VBR+4));


  printf("Starting %08X, %s\n",SH2Core->GetPC(CurrentSH2),filename );

  while(1)
    SH2Core->Exec(CurrentSH2,32);
  
  return 0;
}


