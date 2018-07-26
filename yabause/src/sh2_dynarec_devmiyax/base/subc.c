typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;

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



//s32 m = INSTRUCTION_C(sh->instruction);
//s32 n = INSTRUCTION_B(sh->instruction);

void SH2addi( s8 cd, s32 b )
{
   s32 cdi = (s32)cd;
   regs.R[b] += cdi;
}

void SH2subc(s32 m, s32 n)
{
   u32 tmp0,tmp1;
  
   tmp1 = regs.R[n] - regs.R[m];
   tmp0 = regs.R[n];
   regs.R[n] = tmp1 - (regs.SR&0x01);

   if (tmp0 < tmp1)
      regs.SR |= 1;
   else
      regs.SR &= ~0x01;

   if (tmp1 < regs.R[n])
      regs.SR |= 1;

}

