typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed int s32;
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
 
typedef struct
{
   u32 R[16];
   uSR SRR; 
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


uSR SH2div1(s32 m,s32 n, uSR SR )
{
   u32 tmp0;
   u8 old_q, tmp1;
  
   old_q = SR.part.Q;
   SR.part.Q = (u8)((0x80000000 & regs.R[n])!=0);
   regs.R[n] <<= 1;
   regs.R[n]|=(u32)SR.part.T;

   switch(old_q)
   {
      case 0:
         switch(SR.part.M)
         {
            case 0:
               tmp0 = regs.R[n];
               regs.R[n] -= regs.R[m];
               tmp1 = (regs.R[n] > tmp0);
               switch(SR.part.Q)
               {
                  case 0:
                     SR.part.Q = tmp1;
                     break;
                  case 1:
                     SR.part.Q = (u8) (tmp1 == 0);
                     break;
               }
               break;
            case 1:
               tmp0 = regs.R[n];
               regs.R[n] += regs.R[m];
               tmp1 = (regs.R[n] < tmp0);
               switch(SR.part.Q)
               {
                  case 0:
                     SR.part.Q = (u8) (tmp1 == 0);
                     break;
                  case 1:
                     SR.part.Q = tmp1;
                     break;
               }
               break;
         }
         break;
      case 1:
         switch(SR.part.M)
         {
            case 0:
               tmp0 = regs.R[n];
               regs.R[n] += regs.R[m];
               tmp1 = (regs.R[n] < tmp0);
               switch(SR.part.Q)
               {
                  case 0:
                     SR.part.Q = tmp1;
                     break;
                  case 1:
                     SR.part.Q = (u8) (tmp1 == 0);
                     break;
               }
               break;
            case 1:
               tmp0 = regs.R[n];
               regs.R[n] -= regs.R[m];
               tmp1 = (regs.R[n] > tmp0);
               switch(SR.part.Q)
               {
                  case 0:
                     SR.part.Q = (u8) (tmp1 == 0);
                     break;
                  case 1:
                     SR.part.Q = tmp1;
                     break;
               }
               break;
         }
         break;
   }
   SR.part.T = (SR.part.Q == SR.part.M);
   return SR;
}



