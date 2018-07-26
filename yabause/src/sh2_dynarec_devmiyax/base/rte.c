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
u32 MappedMemoryReadLong(u32 addr);



//s32 m = INSTRUCTION_C(sh->instruction);
//s32 n = INSTRUCTION_B(sh->instruction);

void SH2rte()
{
   u32 temp;
   temp=regs.PC;
   regs.PC = MappedMemoryReadLong(regs.R[15]);
   regs.R[15] += 4;
   regs.SR = MappedMemoryReadLong(regs.R[15]) & 0x000003F3;
   regs.R[15] += 4;
}


