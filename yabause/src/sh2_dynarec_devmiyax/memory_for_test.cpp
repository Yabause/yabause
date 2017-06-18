#include <core.h>
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "memory_for_test.h"
#include "DynarecSh2.h"

u8 *HighWram;
u8 *LowWram;
u8 *BiosRom;
bool romlock = false;

SH2_struct *CurrentSH2;
yabsys_struct yabsys;


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

int initMemory() {

  if ((BiosRom = (u8*)calloc(0x80000, sizeof(u8))) == NULL) {
    return -1;
  }

  if ((HighWram = (u8*)calloc(0x100000, sizeof(u8))) == NULL) {
    return -1;
  }

  if ((LowWram = (u8*)calloc(0x100000, sizeof(u8))) == NULL) {
    return -1;
  }
  return 0;
}

void setromlock( bool lock ){
  romlock = lock;
}

extern "C" {

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
    if(!romlock) BiosRom[addr & 0xFFFFF] = val;
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
    if(!romlock) *((u16 *)(BiosRom + (addr& 0xFFFFF) )) = BSWAP16L(val);
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
    if(!romlock) *((u32 *)(BiosRom + (addr & 0xFFFFF))) = BSWAP32(val);
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
}