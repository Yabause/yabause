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


typedef   union {
    struct {
      u32 T:1;
      u32 S:1;
      u32 reserved0:2;
      u32 I:4;
      u32 Q:1;
      u32 M:1;
      u32 reserved1:22;
    } part;
    u32 all;
  } uSR; 
 
uSR SH2subc(s32 m, s32 n, uSR SR )
{
   u32 tmp0,tmp1;
  
   tmp1 = regs.R[n] - regs.R[m];
   tmp0 = regs.R[n];
   regs.R[n] = tmp1 - SR.part.T;

   if (tmp0 < tmp1)
      SR.part.T = 1;
   else
      SR.part.T = 0;      

   if (tmp1 < regs.R[n])
      SR.part.T = 1;

   return SR;
}


uSR SH2addc(s32 source, s32 dest, uSR SR)
{
   u32 tmp0, tmp1;

   tmp1 = regs.R[source] + regs.R[dest];
   tmp0 = regs.R[dest];

   regs.R[dest] = tmp1 + SR.part.T;
   if (tmp0 > tmp1)
      SR.part.T = 1;
   else
      SR.part.T = 0;
   if (tmp1 > regs.R[dest])
      SR.part.T = 1;
  return SR;
}

uSR SH2addv(s32 m, s32 n, uSR SR)
{
   s32 dest,src,ans;

   if ((s32) regs.R[n] >= 0)
      dest = 0;
   else
      dest = 1;
  
   if ((s32) regs.R[m] >= 0)
      src = 0;
   else
      src = 1;
  
   src += dest;
   regs.R[n] += regs.R[m];

   if ((s32) regs.R[n] >= 0)
      ans = 0;
   else
      ans = 1;

   ans += dest;
  
   if (src == 0 || src == 2)
      if (ans == 1)
         SR.part.T = 1;
      else
         SR.part.T = 0;
   else
      SR.part.T = 0;

   return SR;
}

uSR SH2subv(s32 m, s32 n, uSR SR)
{
   s32 dest,src,ans;

   if ((s32)regs.R[n]>=0)
      dest=0;
   else
      dest=1;

   if ((s32)regs.R[m]>=0)
      src=0;
   else
      src=1;

   src+=dest;
   regs.R[n]-=regs.R[m];

   if ((s32)regs.R[n]>=0)
      ans=0;
   else
      ans=1;

   ans+=dest;

   if (src==1)
   {
     if (ans==1)
        SR.part.T=1;
     else
        SR.part.T=0;
   }
   else
      SR.part.T=0;

   return SR;
}




