typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
typedef signed long s64;
typedef unsigned long u64;

typedef struct
{
   u32 R[16];
   u32 SR;
   u32 GBR;
   u32 VBR;
   u32 MACH;
   u32 MACL;
   u32 PR;
   u32 PC;
} sh2regs_struct;

sh2regs_struct regs;

u8 MappedMemoryReadByte(u32 addr);
u16 MappedMemoryReadWord(u32 addr);
u32 MappedMemoryReadLong(u32 addr);

void SH2macl(s32 m, s32 n)
{
  s32 m0, m1;
  m1 = (s32)MappedMemoryReadLong(regs.R[n]);
  regs.R[n] += 4;
  m0 = (s32)MappedMemoryReadLong(regs.R[m]);
  regs.R[m] += 4;

  u64 a, b, sum;
  a = regs.MACL | ((u64)regs.MACH << 32);
  b = (s64)m0 * m1;
  sum = a + b;
  if ( regs.SR & 0x02 ) {
    if (sum > 0x00007FFFFFFFFFFFULL && sum < 0xFFFF800000000000ULL)
    {
      if ((s64)b < 0)
        sum = 0xFFFF800000000000ULL;
      else
        sum = 0x00007FFFFFFFFFFFULL;
    }
  }
  regs.MACL = sum;
  regs.MACH = sum >> 32;
}


void SH2macw(s32 m, s32 n)
{
  s16 m0, m1;
  u32 templ;

  m0 = (s32)MappedMemoryReadWord(regs.R[m]);
  regs.R[m] += 2;
  m1 = (s32)MappedMemoryReadWord(regs.R[n]);
  regs.R[n] += 2;

  s32 b = (s32)m0 * m1;
  u64 sum = (s64)(s32)regs.MACL + b;

  if ( regs.SR & 0x02 ) {
    if (sum > 0x000000007FFFFFFFULL && sum < 0xFFFFFFFF80000000ULL)
    {
      regs.MACH |= 1;

      if (b < 0)
        sum = 0x80000000ULL;
      else
        sum = 0x7FFFFFFFULL;
    }
    regs.MACL = sum;
  }
  else {
    regs.MACL = sum;
    regs.MACH = sum >> 32;
  }
}

