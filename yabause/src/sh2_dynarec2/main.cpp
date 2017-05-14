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

SH2_struct *MSH2 = NULL;
SH2_struct *SSH2 = NULL;
SH2_struct *CurrentSH2;
yabsys_struct yabsys;
SH2Interface_struct *SH2Core = NULL;
SH2Interface_struct *SH2CoreList[] = {
  &SH2Dyn,
  &SH2DynDebug,
  NULL
};

int init();
int readProgram(const char * addr, const char * filename);
int reset(SH2_struct *context);

u8 *HighWram;
u8 *LowWram;
u8 *BiosRom;


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
  
  if ((BiosRom = (u8*)calloc(0x80000, sizeof(u8))) == NULL) {
    return -1;
  }

  if ((HighWram = (u8*)calloc(0x100000, sizeof(u8))) == NULL) {
    return -1;
  }

  if ((LowWram = (u8*)calloc(0x100000, sizeof(u8))) == NULL) {
    return -1;
  }

  if ((MSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
    return -1;

  MSH2->onchip.BCR1 = 0x0000;
  MSH2->isslave = 0;

  // SSH2
  if ((SSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
    return -1;

  SSH2->onchip.BCR1 = 0x8000;
  SSH2->isslave = 1;

  SH2Core = SH2CoreList[0];


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

  u32 copyaddr = startaddr;
  for (int i = 0; i < file_size; i++) {
    MappedMemoryWriteByte(copyaddr, fgetc(fp));
    copyaddr++;
  }
  fclose(fp);

	u32 VBR = SH2Core->GetVBR(CurrentSH2);
  SH2Core->SetPC(CurrentSH2, MappedMemoryReadLong(VBR));
  SH2Core->SetGPR(CurrentSH2, 15, MappedMemoryReadLong(VBR+4));


  printf("Starting %08X, %s\n",SH2Core->GetPC(CurrentSH2),filename );

  SH2Core->Exec(CurrentSH2, 32);
  
  return 0;
}


#ifdef __GNUC__
#ifdef HAVE_BUILTIN_BSWAP16
# define BSWAP16(x)  ((__builtin_bswap16((x) >> 16) << 16) | __builtin_bswap16((x)))
# define BSWAP16L(x) (__builtin_bswap16((x)))
#endif
#ifdef HAVE_BUILTIN_BSWAP32
# define BSWAP32(x)  (__builtin_bswap32((x)))
#endif
#endif

#ifdef _MSC_VER
# define BSWAP16(x)  ((_byteswap_ushort((x) >> 16) << 16) | _byteswap_ushort((x)))
# define BSWAP16L(x) (_byteswap_ushort((x)))
# define BSWAP32(x)  (_byteswap_ulong((x)))
# define WSWAP32(x)  (_lrotr((x), 16))
#endif

u8 FASTCALL MappedMemoryReadByte(u32 addr) {

  switch (addr & 0x0FF00000)
  {
  // ROM
  case 0x00000000:
    return BiosRom[addr & 0xFFFFF];
    break;

  // Low Memory
  case 0x00200000:
    return LowWram[ addr & 0xFFFFF];
    break;

  // High Memory
  case 0x06000000:
    return HighWram[addr & 0xFFFFF];
    break;
  // Cache
  default:
    break;
  }

  return 0;
}

u16 FASTCALL MappedMemoryReadWord(u32 addr) {
  switch (addr & 0x0FF00000)
  {
    // ROM
  case 0x00000000:
    return BSWAP16L(*((u16 *)(BiosRom+(addr & 0xFFFFF))));
    break;

    // Low Memory
  case 0x00200000:
    return BSWAP16L(*((u16 *)(LowWram + (addr & 0xFFFFF))));
    break;

    // High Memory
  case 0x06000000:
    return BSWAP16L(*((u16 *)(HighWram + (addr & 0xFFFFF))));
    break;

    // Cache
  default:
    break;
  }

  return 0;
}


u32 FASTCALL MappedMemoryReadLong(u32 addr) {
  switch (addr & 0x0FF00000)
  {
    // ROM
  case 0x00000000:
    return BSWAP32(*((u32 *)(BiosRom + (addr & 0xFFFFF))));
    break;

    // Low Memory
  case 0x00200000:
    return BSWAP32(*((u32 *)(LowWram + (addr & 0xFFFFF))));
    break;

    // High Memory
  case 0x06000000:
    return BSWAP32(*((u32 *)(HighWram + (addr & 0xFFFFF))));
    break;

    // Cache
  default:
    break;
  }

  return 0;
}

void FASTCALL MappedMemoryWriteByte(u32 addr, u8 val) {
  switch (addr & 0x0FF00000)
  {
    // ROM
  case 0x00000000:
    BiosRom[addr & 0xFFFFF] = val;
    break;

    // Low Memory
  case 0x00200000:
    LowWram[addr & 0xFFFFF] = val;
    break;

    // High Memory
  case 0x06000000:
    HighWram[addr & 0xFFFFF] = val;
    break;

    // Cache
  default:
    break;
  }

}

void FASTCALL MappedMemoryWriteWord(u32 addr, u16 val) {
  switch (addr & 0x0FF00000)
  {
    // ROM
  case 0x00000000:
    *((u16 *)(BiosRom + (addr& 0xFFFFF) )) = BSWAP16L(val);
    break;

    // Low Memory
  case 0x00200000:
    *((u16 *)(LowWram + (addr & 0xFFFFF))) = BSWAP16L(val);
    break;

    // High Memory
  case 0x06000000:
    *((u16 *)(HighWram + (addr & 0xFFFFF))) = BSWAP16L(val);
    break;

    // Cache
  default:
    break;
  }

}

void FASTCALL MappedMemoryWriteLong(u32 addr, u32 val) {
  switch (addr & 0x0FF00000)
  {
    // ROM
  case 0x00000000:
    *((u32 *)(BiosRom + (addr & 0xFFFFF))) = BSWAP32(val);
    break;

    // Low Memory
  case 0x00200000:
    *((u32 *)(LowWram + (addr & 0xFFFFF))) = BSWAP32(val);
    break;

    // High Memory
  case 0x06000000:
    *((u32 *)(HighWram + (addr & 0xFFFFF))) = BSWAP32(val);
    break;

    // Cache
  default:
    break;
  }

}

void SH2HandleBreakpoints(SH2_struct *context) {

}

Debug * MainLog;
void DebugPrintf(Debug *, const char *, u32, const char *, ...) {

}