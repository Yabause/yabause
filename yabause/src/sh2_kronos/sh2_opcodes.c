/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2007, 2013 Theo Berkau
    Copyright 2005 Fabien Coulon

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file sh2int.c
    \brief SH2 interpreter interface
*/

#include "sh2core.h"
#include "cs0.h"
#include "debug.h"
#include "error.h"
#include "memory.h"
#include "bios.h"
#include "yabause.h"
#include "sh2int_kronos.h"
#include "opcode_functions_define.h"

extern void SH2HandleInterrupts(SH2_struct *context);

//////////////////////////////////////////////////////////////////////////////

static void SH2delay(SH2_struct * sh, u32 addr)
{
   sh->instruction = fetchlist[(addr >> 20) & 0x0FF](sh, addr);
   sh->regs.PC -= 2;
   opcodeTable[sh->instruction](sh);
}

//////////////////////////////////////////////////////////////////////////////

void SH2undecoded(SH2_struct * sh)
{
   int vectnum;

   if (BackupHandled(sh, sh->regs.PC) != 0) {
     return;
   }

   if (yabsys.emulatebios )
   {
      if (BiosHandleFunc(sh))
         return;
   }

   YabSetError(YAB_ERR_SH2INVALIDOPCODE, sh);      

   // Save regs.SR on stack
   sh->regs.R[15]-=4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15],sh->regs.SR.all);

   // Save regs.PC on stack
   sh->regs.R[15]-=4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15],sh->regs.PC + 2);

   // What caused the exception? The delay slot or a general instruction?
   // 4 for General Instructions, 6 for delay slot
   vectnum = 4; //  Fix me

   // Jump to Exception service routine
   sh->regs.PC = SH2MappedMemoryReadLong(sh, sh->regs.VBR+(vectnum<<2));
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2add(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] += sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2addi(SH2_struct * sh, u32 n, u32 d)
{
   s32 cd = (s32)(s8)d;

   sh->regs.R[n] += cd;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2addc(SH2_struct * sh, u32 n, u32 m)
{
   u32 tmp0, tmp1;

   tmp1 = sh->regs.R[m] + sh->regs.R[n];
   tmp0 = sh->regs.R[n];

   sh->regs.R[n] = tmp1 + sh->regs.SR.part.T;
   if (tmp0 > tmp1)
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   if (tmp1 > sh->regs.R[n])
      sh->regs.SR.part.T = 1;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2addv(SH2_struct * sh, u32 n, u32 m)
{
   s32 dest,src,ans;

   if ((s32) sh->regs.R[n] >= 0)
      dest = 0;
   else
      dest = 1;
  
   if ((s32) sh->regs.R[m] >= 0)
      src = 0;
   else
      src = 1;
  
   src += dest;
   sh->regs.R[n] += sh->regs.R[m];

   if ((s32) sh->regs.R[n] >= 0)
      ans = 0;
   else
      ans = 1;

   ans += dest;
  
   if (src == 0 || src == 2)
      if (ans == 1)
         sh->regs.SR.part.T = 1;
      else
         sh->regs.SR.part.T = 0;
   else
      sh->regs.SR.part.T = 0;

   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2and(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] &= sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2andi(SH2_struct * sh, u32 d)
{
   sh->regs.R[0] &= d;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2andm(SH2_struct * sh, u32 d)
{
   s32 temp;
   s32 source = d;

   temp = (s32) SH2MappedMemoryReadByte(sh, sh->regs.GBR + sh->regs.R[0]);
   temp &= source;
   SH2MappedMemoryWriteByte(sh, (sh->regs.GBR + sh->regs.R[0]),temp);
   sh->regs.PC += 2;
   sh->cycles += 3;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2bf(SH2_struct * sh, u32 d)
{
   if (sh->regs.SR.part.T == 0)
   {
      s32 disp = (s32)(s8)d;

      sh->regs.PC = sh->regs.PC+(disp<<1)+4;
      sh->cycles += 3;
   }
   else
   {
      sh->regs.PC+=2;
      sh->cycles++;
   }
}

//////////////////////////////////////////////////////////////////////////////

static void SH2bfs(SH2_struct * sh, u32 d)
{
   u32 temp = sh->regs.PC;
   if (sh->regs.SR.part.T == 0)
   {
      s32 disp = (s32)(s8)d;

      sh->regs.PC = sh->regs.PC + (disp << 1) + 4;

      sh->cycles += 2;
      SH2delay(sh, temp + 2);
   }
   else
   {
      sh->regs.PC += 2;
      sh->cycles++;
   }

}



//////////////////////////////////////////////////////////////////////////////

static void SH2bra(SH2_struct * sh, u32 disp)
{
   u32 temp = sh->regs.PC;

   if ((disp&0x800) != 0)
      disp |= 0xFFFFF000;

   sh->regs.PC = sh->regs.PC + (disp<<1) + 4;

   sh->cycles += 2;
   SH2delay(sh, temp + 2);
}


//////////////////////////////////////////////////////////////////////////////

static void SH2braf(SH2_struct * sh, u32 m)
{
   u32 temp;

   temp = sh->regs.PC;
   sh->regs.PC += sh->regs.R[m] + 4; 

   sh->cycles += 2;
   SH2delay(sh, temp + 2);
}


//////////////////////////////////////////////////////////////////////////////

static void SH2bsr(SH2_struct * sh, u32 disp)
{
   u32 temp;

   temp = sh->regs.PC;
   if ((disp&0x800) != 0) disp |= 0xFFFFF000;
   sh->regs.PR = sh->regs.PC + 4;
   sh->regs.PC = sh->regs.PC+(disp<<1) + 4;

   sh->cycles += 2;
   SH2delay(sh, temp + 2);
}

//////////////////////////////////////////////////////////////////////////////

static void SH2bsrf(SH2_struct * sh, u32 n)
{
   u32 temp = sh->regs.PC;
   sh->regs.PR = sh->regs.PC + 4;
   sh->regs.PC += sh->regs.R[n] + 4;
   sh->cycles += 2;
   SH2delay(sh, temp + 2);
}


//////////////////////////////////////////////////////////////////////////////

static void SH2bt(SH2_struct * sh, u32 d)
{
   if (sh->regs.SR.part.T == 1)
   {
      s32 disp = (s32)(s8)d;

      sh->regs.PC = sh->regs.PC+(disp<<1)+4;
      sh->cycles += 3;
   }
   else
   {
      sh->regs.PC += 2;
      sh->cycles++;
   }
}


//////////////////////////////////////////////////////////////////////////////

static void SH2bts(SH2_struct * sh, u32 d)
{
   u32 temp = sh->regs.PC;
   if (sh->regs.SR.part.T)
   {
      s32 disp = (s32)(s8)d;

      sh->regs.PC += (disp << 1) + 4;
      sh->cycles += 2;
      SH2delay(sh, temp + 2);
   }
   else
   {
      sh->regs.PC+=2;
      sh->cycles++;
   }
}


//////////////////////////////////////////////////////////////////////////////

static void SH2clrmac(SH2_struct * sh)
{
   sh->regs.MACH = 0;
   sh->regs.MACL = 0;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2clrt(SH2_struct * sh)
{
   sh->regs.SR.part.T = 0;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2cmpeq(SH2_struct * sh, u32 n, u32 m)
{
   if (sh->regs.R[n] == sh->regs.R[m])
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   sh->regs.PC += 2;
   sh->cycles++;
}



//////////////////////////////////////////////////////////////////////////////

static void SH2cmpge(SH2_struct * sh, u32 n, u32 m)
{
   if ((s32)sh->regs.R[n] >=
       (s32)sh->regs.R[m])
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2cmpgt(SH2_struct * sh, u32 n, u32 m)
{
   if ((s32)sh->regs.R[n]>(s32)sh->regs.R[m])
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2cmphi(SH2_struct * sh, u32 n, u32 m)
{
   if ((u32)sh->regs.R[n] >
       (u32)sh->regs.R[m])
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2cmphs(SH2_struct * sh, u32 n, u32 m)
{
   if ((u32)sh->regs.R[n] >=
       (u32)sh->regs.R[m])
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2cmpim(SH2_struct * sh, u32 i)
{
   s32 imm = (s32)(s8)i;

   if (sh->regs.R[0] == (u32) imm) // FIXME: ouais � doit �re bon...
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2cmppl(SH2_struct * sh, u32 n)
{
   if ((s32)sh->regs.R[n]>0)
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2cmppz(SH2_struct * sh, u32 n)
{
   if ((s32)sh->regs.R[n]>=0)
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2cmpstr(SH2_struct * sh, u32 n, u32 m)
{
   u32 temp;
   s32 HH,HL,LH,LL;
   temp=sh->regs.R[n]^sh->regs.R[m];
   HH = (temp>>24) & 0x000000FF;
   HL = (temp>>16) & 0x000000FF;
   LH = (temp>>8) & 0x000000FF;
   LL = temp & 0x000000FF;
   HH = HH && HL && LH && LL;
   if (HH == 0)
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2div0s(SH2_struct * sh, u32 n, u32 m)
{
   if ((sh->regs.R[n]&0x80000000)==0)
     sh->regs.SR.part.Q = 0;
   else
     sh->regs.SR.part.Q = 1;
   if ((sh->regs.R[m]&0x80000000)==0)
     sh->regs.SR.part.M = 0;
   else
     sh->regs.SR.part.M = 1;
   sh->regs.SR.part.T = !(sh->regs.SR.part.M == sh->regs.SR.part.Q);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2div0u(SH2_struct * sh)
{
   sh->regs.SR.part.M = sh->regs.SR.part.Q = sh->regs.SR.part.T = 0;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2div1(SH2_struct * sh, u32 n, u32 m)
{
   u32 tmp0;
   u8 old_q, tmp1;
  
   old_q = sh->regs.SR.part.Q;
   sh->regs.SR.part.Q = (u8)((0x80000000 & sh->regs.R[n])!=0);
   sh->regs.R[n] <<= 1;
   sh->regs.R[n]|=(u32) sh->regs.SR.part.T;

   switch(old_q)
   {
      case 0:
         switch(sh->regs.SR.part.M)
         {
            case 0:
               tmp0 = sh->regs.R[n];
               sh->regs.R[n] -= sh->regs.R[m];
               tmp1 = (sh->regs.R[n] > tmp0);
               switch(sh->regs.SR.part.Q)
               {
                  case 0:
                     sh->regs.SR.part.Q = tmp1;
                     break;
                  case 1:
                     sh->regs.SR.part.Q = (u8) (tmp1 == 0);
                     break;
               }
               break;
            case 1:
               tmp0 = sh->regs.R[n];
               sh->regs.R[n] += sh->regs.R[m];
               tmp1 = (sh->regs.R[n] < tmp0);
               switch(sh->regs.SR.part.Q)
               {
                  case 0:
                     sh->regs.SR.part.Q = (u8) (tmp1 == 0);
                     break;
                  case 1:
                     sh->regs.SR.part.Q = tmp1;
                     break;
               }
               break;
         }
         break;
      case 1:
         switch(sh->regs.SR.part.M)
         {
            case 0:
               tmp0 = sh->regs.R[n];
               sh->regs.R[n] += sh->regs.R[m];
               tmp1 = (sh->regs.R[n] < tmp0);
               switch(sh->regs.SR.part.Q)
               {
                  case 0:
                     sh->regs.SR.part.Q = tmp1;
                     break;
                  case 1:
                     sh->regs.SR.part.Q = (u8) (tmp1 == 0);
                     break;
               }
               break;
            case 1:
               tmp0 = sh->regs.R[n];
               sh->regs.R[n] -= sh->regs.R[m];
               tmp1 = (sh->regs.R[n] > tmp0);
               switch(sh->regs.SR.part.Q)
               {
                  case 0:
                     sh->regs.SR.part.Q = (u8) (tmp1 == 0);
                     break;
                  case 1:
                     sh->regs.SR.part.Q = tmp1;
                     break;
               }
               break;
         }
         break;
   }
   sh->regs.SR.part.T = (sh->regs.SR.part.Q == sh->regs.SR.part.M);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2dmuls(SH2_struct * sh, u32 n, u32 m)
{
   u32 RnL,RnH,RmL,RmH,Res0,Res1,Res2;
   u32 temp0,temp1,temp2,temp3;
   s32 tempm,tempn,fnLmL;

   const u64 result = (s64)(s32)sh->regs.R[n] * (s32)sh->regs.R[m];

   sh->regs.MACL = result >> 0;
   sh->regs.MACH = result >> 32;
   sh->regs.PC += 2;
   sh->cycles += 2;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2dmulu(SH2_struct * sh, u32 n, u32 m)
{
   u32 RnL,RnH,RmL,RmH,Res0,Res1,Res2;
   u32 temp0,temp1,temp2,temp3;

   RnL = sh->regs.R[n] & 0x0000FFFF;
   RnH = (sh->regs.R[n] >> 16) & 0x0000FFFF;
   RmL = sh->regs.R[m] & 0x0000FFFF;
   RmH = (sh->regs.R[m] >> 16) & 0x0000FFFF;

   temp0 = RmL * RnL;
   temp1 = RmH * RnL;
   temp2 = RmL * RnH;
   temp3 = RmH * RnH;
  
   Res2 = 0;
   Res1 = temp1 + temp2;
   if (Res1 < temp1)
      Res2 += 0x00010000;
  
   temp1 = (Res1 << 16) & 0xFFFF0000;
   Res0 = temp0 + temp1;
   if (Res0 < temp0)
      Res2++;
  
   Res2 = Res2 + ((Res1 >> 16) & 0x0000FFFF) + temp3;
 
   sh->regs.MACH = Res2;
   sh->regs.MACL = Res0;
   sh->regs.PC += 2;
   sh->cycles += 2;
}



//////////////////////////////////////////////////////////////////////////////

static void SH2dt(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]--;
   if (sh->regs.R[n] == 0)
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2extsb(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = (u32)(s8)sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2extsw(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = (u32)(s16)sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2extub(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = (u32)(u8)sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2extuw(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = (u32)(u16)sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2jmp(SH2_struct * sh, u32 m)
{
   u32 temp;

   temp=sh->regs.PC;
   sh->regs.PC = sh->regs.R[m];
   sh->cycles += 2;
   SH2delay(sh, temp + 2);
}


//////////////////////////////////////////////////////////////////////////////

static void SH2jsr(SH2_struct * sh, u32 m)
{
   u32 temp;

   temp = sh->regs.PC;
   sh->regs.PR = sh->regs.PC + 4;
   sh->regs.PC = sh->regs.R[m];
   sh->cycles += 2;
   SH2delay(sh, temp + 2);
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldcgbr(SH2_struct * sh, u32 m)
{
   sh->regs.GBR = sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldcmgbr(SH2_struct * sh, u32 m)
{
   sh->regs.GBR = SH2MappedMemoryReadLong(sh, sh->regs.R[m]);
   sh->regs.R[m] += 4;
   sh->regs.PC += 2;
   sh->cycles += 3;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldcmsr(SH2_struct * sh, u32 m)
{
   sh->regs.SR.all = SH2MappedMemoryReadLong(sh, sh->regs.R[m]) & 0x000003F3;
   sh->regs.R[m] += 4;
   sh->regs.PC += 2;
   sh->cycles += 3;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldcmvbr(SH2_struct * sh, u32 m)
{
   sh->regs.VBR = SH2MappedMemoryReadLong(sh, sh->regs.R[m]);
   sh->regs.R[m] += 4;
   sh->regs.PC += 2;
   sh->cycles += 3;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldcsr(SH2_struct * sh, u32 m)
{
   sh->regs.SR.all = sh->regs.R[m]&0x000003F3;
   sh->regs.PC += 2;
   sh->cycles++;
   SH2HandleInterrupts(sh);
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldcvbr(SH2_struct * sh, u32 m)
{
   sh->regs.VBR = sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldsmach(SH2_struct * sh, u32 m)
{
   sh->regs.MACH = sh->regs.R[m];
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldsmacl(SH2_struct * sh, u32 m)
{
   sh->regs.MACL = sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldsmmach(SH2_struct * sh, u32 m)
{
   sh->regs.MACH = SH2MappedMemoryReadLong(sh, sh->regs.R[m]);
   sh->regs.R[m] += 4;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldsmmacl(SH2_struct * sh, u32 m)
{
   sh->regs.MACL = SH2MappedMemoryReadLong(sh, sh->regs.R[m]);
   sh->regs.R[m] += 4;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldsmpr(SH2_struct * sh, u32 m)
{
   sh->regs.PR = SH2MappedMemoryReadLong(sh, sh->regs.R[m]);
   sh->regs.R[m] += 4;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ldspr(SH2_struct * sh, u32 m)
{
   sh->regs.PR = sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2macl(SH2_struct * sh, u32 n, u32 m)
{
   s32 m0, m1;
   u64 a, b, sum;

   m1 = (s32) SH2MappedMemoryReadLong(sh, sh->regs.R[n]);
   sh->regs.R[n] += 4;
   m0 = (s32) SH2MappedMemoryReadLong(sh, sh->regs.R[m]);
   sh->regs.R[m] += 4;

#if 1 // fast and better
   a = sh->regs.MACL | ((u64)sh->regs.MACH << 32);
   b = (s64)m0 * m1;
   sum = a+b;
   if (sh->regs.SR.part.S == 1) {
     if (sum > 0x00007FFFFFFFFFFFULL && sum < 0xFFFF800000000000ULL)
     {
       if ((s64)b < 0)
         sum = 0xFFFF800000000000ULL;
       else
         sum = 0x00007FFFFFFFFFFFULL;
     }
   }
   sh->regs.MACL = sum; 
   sh->regs.MACH = sum >> 32;
#else
   if ((s32) (tempn^tempm) < 0)
      fnLmL = -1;
   else
      fnLmL = 0;
   if (tempn < 0)
      tempn = 0 - tempn;
   if (tempm < 0)
      tempm = 0 - tempm;

   temp1 = (u32) tempn;
   temp2 = (u32) tempm;

   RnL = temp1 & 0x0000FFFF;
   RnH = (temp1 >> 16) & 0x0000FFFF;
   RmL = temp2 & 0x0000FFFF;
   RmH = (temp2 >> 16) & 0x0000FFFF;

   temp0 = RmL * RnL;
   temp1 = RmH * RnL;
   temp2 = RmL * RnH;
   temp3 = RmH * RnH;

   Res2 = 0;
   Res1 = temp1 + temp2;
   if (Res1 < temp1)
      Res2 += 0x00010000;

   temp1 = (Res1 << 16) & 0xFFFF0000;
   Res0 = temp0 + temp1;
   if (Res0 < temp0)
      Res2++;

   Res2=Res2+((Res1>>16)&0x0000FFFF)+temp3;

   if(fnLmL < 0)
   {
      Res2=~Res2;
      if (Res0==0)
         Res2++;
      else
         Res0=(~Res0)+1;
   }
   if(sh->regs.SR.part.S == 1)
   {
      Res0=sh->regs.MACL+Res0;
      if (sh->regs.MACL>Res0)
         Res2++;
      //if (sh->regs.MACH & 0x00008000);
      //else Res2 += sh->regs.MACH | 0xFFFF0000;

      if (sh->regs.MACH & 0x00008000){
        Res2 -= (sh->regs.MACH & 0x0000FFFF);
      }
      else{
        Res2 += (sh->regs.MACH & 0x0000FFFF);
      }
      if(((s32)Res2<0)&&(Res2<0xFFFF8000))
      {
         Res2=0x00008000;
         Res0=0x00000000;
      }
      if(((s32)Res2>0)&&(Res2>0x00007FFF))
      {
         Res2=0x00007FFF;
         Res0=0xFFFFFFFF;
      };

      sh->regs.MACH=Res2;
      sh->regs.MACL=Res0;
   }
   else
   {
      Res0=sh->regs.MACL+Res0;
      if (sh->regs.MACL>Res0)
         Res2++;
      Res2+=sh->regs.MACH;

      sh->regs.MACH=Res2;
      sh->regs.MACL=Res0;
   }
#endif
   sh->regs.PC+=2;
   sh->cycles += 3;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2macw(SH2_struct * sh, u32 n, u32 m)
{
   s32 tempm,tempn,dest,src,ans;
   u32 templ;

   tempn=(s32) SH2MappedMemoryReadWord(sh, sh->regs.R[n]);
   sh->regs.R[n]+=2;
   tempm=(s32) SH2MappedMemoryReadWord(sh, sh->regs.R[m]);
   sh->regs.R[m]+=2;
   templ=sh->regs.MACL;
   tempm=((s32)(s16)tempn*(s32)(s16)tempm);

   if ((s32)sh->regs.MACL>=0)
      dest=0;
   else
      dest=1;
   if ((s32)tempm>=0)
   {
      src=0;
      tempn=0;
   }
   else
   {
      src=1;
      tempn=0xFFFFFFFF;
   }
   src+=dest;
   sh->regs.MACL+=tempm;
   if ((s32)sh->regs.MACL>=0)
      ans=0;
   else
      ans=1;
   ans+=dest;
   if (sh->regs.SR.part.S == 1)
   {
      if (ans==1)
      {
         if (src==0)
            sh->regs.MACL=0x7FFFFFFF;
         if (src==2)
            sh->regs.MACL=0x80000000;
      }
   }
   else
   {
      sh->regs.MACH+=tempn;
      if (templ>sh->regs.MACL)
         sh->regs.MACH+=1;
   }
   sh->regs.PC+=2;
   sh->cycles += 3;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2mov(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n]=sh->regs.R[m];
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2mova(SH2_struct * sh, u32 disp)
{
   sh->regs.R[0]=((sh->regs.PC+4)&0xFFFFFFFC)+(disp<<2);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movbl(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = (s32)(s8)SH2MappedMemoryReadByte(sh, sh->regs.R[m]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movbl0(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = (s32)(s8)SH2MappedMemoryReadByte(sh, sh->regs.R[m] + sh->regs.R[0]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movbl4(SH2_struct * sh, u32 m, u32 disp)
{
   sh->regs.R[0] = (s32)(s8)SH2MappedMemoryReadByte(sh, sh->regs.R[m] + disp);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movblg(SH2_struct * sh, u32 disp)
{
   sh->regs.R[0] = (s32)(s8)SH2MappedMemoryReadByte(sh, sh->regs.GBR + disp);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movbm(SH2_struct * sh, u32 n, u32 m)
{
   SH2MappedMemoryWriteByte(sh, (sh->regs.R[n] - 1),sh->regs.R[m]);
   sh->regs.R[n] -= 1;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movbp(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = (s32)(s8)SH2MappedMemoryReadByte(sh, sh->regs.R[m]);
   if (n != m)
     sh->regs.R[m] += 1;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movbs(SH2_struct * sh, u32 n, u32 m)
{
   SH2MappedMemoryWriteByte(sh, sh->regs.R[n], sh->regs.R[m]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movbs0(SH2_struct * sh, u32 n, u32 m)
{
   SH2MappedMemoryWriteByte(sh, sh->regs.R[n] + sh->regs.R[0],
                         sh->regs.R[m]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movbs4(SH2_struct * sh, u32 n, u32 disp)
{
   SH2MappedMemoryWriteByte(sh, sh->regs.R[n]+disp,sh->regs.R[0]);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movbsg(SH2_struct * sh, u32 disp)
{
   SH2MappedMemoryWriteByte(sh, sh->regs.GBR + disp,sh->regs.R[0]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movi(SH2_struct * sh, u32 n, u32 i)
{
   sh->regs.R[n] = (s32)(s8)i;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movli(SH2_struct * sh, u32 n, u32 disp)
{
   sh->regs.R[n] = SH2MappedMemoryReadLong(sh, ((sh->regs.PC + 4) & 0xFFFFFFFC) + (disp << 2));
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movll(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = SH2MappedMemoryReadLong(sh, sh->regs.R[m]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movll0(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = SH2MappedMemoryReadLong(sh, sh->regs.R[m] + sh->regs.R[0]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movll4(SH2_struct * sh, u32 n, u32 m, u32 disp)
{
   sh->regs.R[n] = SH2MappedMemoryReadLong(sh, sh->regs.R[m] + (disp << 2));
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movllg(SH2_struct * sh, u32 disp)
{
   sh->regs.R[0] = SH2MappedMemoryReadLong(sh, sh->regs.GBR + (disp << 2));
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movlm(SH2_struct * sh, u32 n, u32 m)
{
   SH2MappedMemoryWriteLong(sh, sh->regs.R[n] - 4,sh->regs.R[m]);
   sh->regs.R[n] -= 4;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movlp(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = SH2MappedMemoryReadLong(sh, sh->regs.R[m]);
   if (n != m) sh->regs.R[m] += 4;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movls(SH2_struct * sh, u32 n, u32 m)
{
   SH2MappedMemoryWriteLong(sh, sh->regs.R[n], sh->regs.R[m]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movls0(SH2_struct * sh, u32 n, u32 m)
{
   SH2MappedMemoryWriteLong(sh, sh->regs.R[n] + sh->regs.R[0],
                         sh->regs.R[m]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movls4(SH2_struct * sh, u32 n, u32 m, u32 disp)
{
   SH2MappedMemoryWriteLong(sh, sh->regs.R[n]+(disp<<2),sh->regs.R[m]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movlsg(SH2_struct * sh, u32 disp)
{
   SH2MappedMemoryWriteLong(sh, sh->regs.GBR+(disp<<2),sh->regs.R[0]);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movt(SH2_struct * sh, u32 n)
{
   sh->regs.R[n] = (0x00000001 & sh->regs.SR.all);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movwi(SH2_struct * sh, u32 n, u32 disp)
{
   sh->regs.R[n] = (s32)(s16)SH2MappedMemoryReadWord(sh, sh->regs.PC + (disp<<1) + 4);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movwl(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = (s32)(s16)SH2MappedMemoryReadWord(sh, sh->regs.R[m]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movwl0(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = (s32)(s16)SH2MappedMemoryReadWord(sh, sh->regs.R[m]+sh->regs.R[0]);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movwl4(SH2_struct * sh, u32 m, u32 disp)
{
   sh->regs.R[0] = (s32)(s16)SH2MappedMemoryReadWord(sh, sh->regs.R[m]+(disp<<1));
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movwlg(SH2_struct * sh, u32 disp)
{
   sh->regs.R[0] = (s32)(s16)SH2MappedMemoryReadWord(sh, sh->regs.GBR+(disp<<1));
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movwm(SH2_struct * sh, u32 n, u32 m)
{
   SH2MappedMemoryWriteWord(sh, sh->regs.R[n] - 2,sh->regs.R[m]);
   sh->regs.R[n] -= 2;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movwp(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = (s32)(s16)SH2MappedMemoryReadWord(sh, sh->regs.R[m]);
   if (n != m)
      sh->regs.R[m] += 2;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movws(SH2_struct * sh, u32 n, u32 m)
{
   SH2MappedMemoryWriteWord(sh, sh->regs.R[n],sh->regs.R[m]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movws0(SH2_struct * sh, u32 n, u32 m)
{
   SH2MappedMemoryWriteWord(sh, sh->regs.R[n] + sh->regs.R[0],
                         sh->regs.R[m]);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movws4(SH2_struct * sh, u32 n, u32 disp)
{
   SH2MappedMemoryWriteWord(sh, sh->regs.R[n]+(disp<<1),sh->regs.R[0]);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2movwsg(SH2_struct * sh, u32 disp)
{
   SH2MappedMemoryWriteWord(sh, sh->regs.GBR+(disp<<1),sh->regs.R[0]);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2mull(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.MACL = sh->regs.R[n] * sh->regs.R[m];
   sh->regs.PC+=2;
   sh->cycles += 2;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2muls(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.MACL = ((s32)(s16)sh->regs.R[n]*(s32)(s16)sh->regs.R[m]);
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2mulu(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.MACL = ((u32)(u16)sh->regs.R[n] * (u32)(u16)sh->regs.R[m]);
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2neg(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n]=0-sh->regs.R[m];
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2negc(SH2_struct * sh, u32 n, u32 m)
{
   u32 temp;
  
   temp=0-sh->regs.R[m];
   sh->regs.R[n] = temp - sh->regs.SR.part.T;
   if (0 < temp)
      sh->regs.SR.part.T=1;
   else
      sh->regs.SR.part.T=0;
   if (temp < sh->regs.R[n])
      sh->regs.SR.part.T=1;
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2nop(SH2_struct * sh)
{
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2not(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] = ~sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2or(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] |= sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2ori(SH2_struct * sh, u32 imm)
{
   sh->regs.R[0] |= imm;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2orm(SH2_struct * sh, u32 imm)
{
   s32 temp;
   s32 source = imm;

   temp = (s32) SH2MappedMemoryReadByte(sh, sh->regs.GBR + sh->regs.R[0]);
   temp |= source;
   SH2MappedMemoryWriteByte(sh, sh->regs.GBR + sh->regs.R[0],temp);
   sh->regs.PC += 2;
   sh->cycles += 3;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2rotcl(SH2_struct * sh, u32 n)
{
   s32 temp;

   if ((sh->regs.R[n]&0x80000000)==0)
      temp=0;
   else
      temp=1;

   sh->regs.R[n]<<=1;

   if (sh->regs.SR.part.T == 1)
      sh->regs.R[n]|=0x00000001;
   else
      sh->regs.R[n]&=0xFFFFFFFE;

   if (temp==1)
      sh->regs.SR.part.T=1;
   else
      sh->regs.SR.part.T=0;

   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2rotcr(SH2_struct * sh, u32 n)
{
   s32 temp;

   if ((sh->regs.R[n]&0x00000001)==0)
      temp=0;
   else
      temp=1;

   sh->regs.R[n]>>=1;

   if (sh->regs.SR.part.T == 1)
      sh->regs.R[n]|=0x80000000;
   else
      sh->regs.R[n]&=0x7FFFFFFF;

   if (temp==1)
      sh->regs.SR.part.T=1;
   else
      sh->regs.SR.part.T=0;

   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2rotl(SH2_struct * sh, u32 n)
{
   if ((sh->regs.R[n]&0x80000000)==0)
      sh->regs.SR.part.T=0;
   else
      sh->regs.SR.part.T=1;

   sh->regs.R[n]<<=1;

   if (sh->regs.SR.part.T==1)
      sh->regs.R[n]|=0x00000001;
   else
      sh->regs.R[n]&=0xFFFFFFFE;

   sh->regs.PC+=2;
   sh->cycles++;
}



//////////////////////////////////////////////////////////////////////////////

static void SH2rotr(SH2_struct * sh, u32 n)
{
   if ((sh->regs.R[n]&0x00000001)==0)
      sh->regs.SR.part.T = 0;
   else
      sh->regs.SR.part.T = 1;

   sh->regs.R[n]>>=1;

   if (sh->regs.SR.part.T == 1)
      sh->regs.R[n]|=0x80000000;
   else
      sh->regs.R[n]&=0x7FFFFFFF;

   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2rte(SH2_struct * sh)
{
   u32 temp;
   temp=sh->regs.PC;
   sh->regs.PC = SH2MappedMemoryReadLong(sh, sh->regs.R[15]);
   sh->regs.R[15] += 4;
   sh->regs.SR.all = SH2MappedMemoryReadLong(sh, sh->regs.R[15]) & 0x000003F3;
   sh->regs.R[15] += 4;
   sh->cycles += 4;
   SH2delay(sh, temp + 2);
}


//////////////////////////////////////////////////////////////////////////////

static void SH2rts(SH2_struct * sh)
{
   u32 temp;

   temp = sh->regs.PC;
   sh->regs.PC = sh->regs.PR;

   sh->cycles += 2;
   SH2delay(sh, temp + 2);
}


//////////////////////////////////////////////////////////////////////////////

static void SH2sett(SH2_struct * sh)
{
   sh->regs.SR.part.T = 1;
   sh->regs.PC += 2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2shal(SH2_struct * sh, u32 n)
{
   if ((sh->regs.R[n] & 0x80000000) == 0)
      sh->regs.SR.part.T = 0;
   else
      sh->regs.SR.part.T = 1;
   sh->regs.R[n] <<= 1;
   sh->regs.PC += 2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2shar(SH2_struct * sh, u32 n)
{
   s32 temp;

   if ((sh->regs.R[n]&0x00000001)==0)
      sh->regs.SR.part.T = 0;
   else
      sh->regs.SR.part.T = 1;

   if ((sh->regs.R[n]&0x80000000)==0)
      temp = 0;
   else
      temp = 1;

   sh->regs.R[n] >>= 1;

   if (temp == 1)
      sh->regs.R[n] |= 0x80000000;
   else
      sh->regs.R[n] &= 0x7FFFFFFF;

   sh->regs.PC += 2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2shll(SH2_struct * sh, u32 n)
{
   if ((sh->regs.R[n]&0x80000000)==0)
      sh->regs.SR.part.T=0;
   else
      sh->regs.SR.part.T=1;
 
   sh->regs.R[n]<<=1;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2shll2(SH2_struct * sh, u32 n)
{
   sh->regs.R[n] <<= 2;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2shll8(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]<<=8;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2shll16(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]<<=16;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2shlr(SH2_struct * sh, u32 n)
{
   if ((sh->regs.R[n]&0x00000001)==0)
      sh->regs.SR.part.T=0;
   else
      sh->regs.SR.part.T=1;

   sh->regs.R[n]>>=1;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2shlr2(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]>>=2;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2shlr8(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]>>=8;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2shlr16(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]>>=16;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stcgbr(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]=sh->regs.GBR;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stcmgbr(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]-=4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[n],sh->regs.GBR);
   sh->regs.PC+=2;
   sh->cycles += 2;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stcmsr(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]-=4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[n],sh->regs.SR.all);
   sh->regs.PC+=2;
   sh->cycles += 2;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stcmvbr(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]-=4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[n],sh->regs.VBR);
   sh->regs.PC+=2;
   sh->cycles += 2;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stcsr(SH2_struct * sh, u32 n)
{
   sh->regs.R[n] = sh->regs.SR.all & 0x3F3;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stcvbr(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]=sh->regs.VBR;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stsmach(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]=sh->regs.MACH;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stsmacl(SH2_struct * sh, u32 n)
{
   sh->regs.R[n]=sh->regs.MACL;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stsmmach(SH2_struct * sh, u32 n)
{
   sh->regs.R[n] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[n],sh->regs.MACH); 
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stsmmacl(SH2_struct * sh, u32 n)
{
   sh->regs.R[n] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[n],sh->regs.MACL);
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stsmpr(SH2_struct * sh, u32 n)
{
   sh->regs.R[n] -= 4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[n],sh->regs.PR);
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2stspr(SH2_struct * sh, u32 n)
{
   sh->regs.R[n] = sh->regs.PR;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2sub(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n]-=sh->regs.R[m];
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2subc(SH2_struct * sh, u32 n, u32 m)
{
   u32 tmp0,tmp1;
  
   tmp1 = sh->regs.R[n] - sh->regs.R[m];
   tmp0 = sh->regs.R[n];
   sh->regs.R[n] = tmp1 - sh->regs.SR.part.T;

   if (tmp0 < tmp1)
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;

   if (tmp1 < sh->regs.R[n])
      sh->regs.SR.part.T = 1;

   sh->regs.PC += 2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2subv(SH2_struct * sh, u32 n, u32 m)
{
   s32 dest,src,ans;

   if ((s32)sh->regs.R[n]>=0)
      dest=0;
   else
      dest=1;

   if ((s32)sh->regs.R[m]>=0)
      src=0;
   else
      src=1;

   src+=dest;
   sh->regs.R[n]-=sh->regs.R[m];

   if ((s32)sh->regs.R[n]>=0)
      ans=0;
   else
      ans=1;

   ans+=dest;

   if (src==1)
   {
     if (ans==1)
        sh->regs.SR.part.T=1;
     else
        sh->regs.SR.part.T=0;
   }
   else
      sh->regs.SR.part.T=0;

   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2swapb(SH2_struct * sh, u32 n, u32 m)
{
   u32 temp0,temp1;

   temp0=sh->regs.R[m]&0xffff0000;
   temp1=(sh->regs.R[m]&0x000000ff)<<8;
   sh->regs.R[n]=(sh->regs.R[m]>>8)&0x000000ff;
   sh->regs.R[n]=sh->regs.R[n]|temp1|temp0;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2swapw(SH2_struct * sh, u32 n, u32 m)
{
   u32 temp;
   temp=(sh->regs.R[m]>>16)&0x0000FFFF;
   sh->regs.R[n]=sh->regs.R[m]<<16;
   sh->regs.R[n]|=temp;
   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2tas(SH2_struct * sh, u32 n)
{
   s32 temp;

   temp=(s32) SH2MappedMemoryReadByte(sh, 0X20000000|sh->regs.R[n]);

   if (temp==0)
      sh->regs.SR.part.T=1;
   else
      sh->regs.SR.part.T=0;

   temp|=0x00000080;
   SH2MappedMemoryWriteByte(sh, sh->regs.R[n],temp);
   sh->regs.PC+=2;
   sh->cycles += 4;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2trapa(SH2_struct * sh, u32 imm)
{
   sh->regs.R[15]-=4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15],sh->regs.SR.all);
   sh->regs.R[15]-=4;
   SH2MappedMemoryWriteLong(sh, sh->regs.R[15],sh->regs.PC + 2);
   sh->regs.PC = SH2MappedMemoryReadLong(sh, sh->regs.VBR+(imm<<2));
   sh->cycles += 8;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2tst(SH2_struct * sh, u32 n, u32 m)
{
   if ((sh->regs.R[n]&sh->regs.R[m])==0)
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;

   sh->regs.PC += 2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2tsti(SH2_struct * sh, u32 imm)
{
   s32 temp;

   temp=sh->regs.R[0]&imm;

   if (temp==0)
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;

   sh->regs.PC+=2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2tstm(SH2_struct * sh, u32 imm)
{
   s32 temp;

   temp=(s32) SH2MappedMemoryReadByte(sh, sh->regs.GBR+sh->regs.R[0]);
   temp&=imm;

   if (temp==0)
      sh->regs.SR.part.T = 1;
   else
      sh->regs.SR.part.T = 0;

   sh->regs.PC+=2;
   sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2xor(SH2_struct * sh, u32 n, u32 m)
{
   sh->regs.R[n] ^= sh->regs.R[m];
   sh->regs.PC += 2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2xori(SH2_struct * sh, u32 imm)
{
   sh->regs.R[0] ^= imm;
   sh->regs.PC += 2;
   sh->cycles++;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2xorm(SH2_struct * sh, u32 imm)
{
   s32 temp;

   temp = (s32) SH2MappedMemoryReadByte(sh, sh->regs.GBR + sh->regs.R[0]);
   temp ^= imm;
   SH2MappedMemoryWriteByte(sh, sh->regs.GBR + sh->regs.R[0],temp);
   sh->regs.PC += 2;
   sh->cycles += 3;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2xtrct(SH2_struct * sh, u32 n, u32 m)
{
   u32 temp;

   temp=(sh->regs.R[m]<<16)&0xFFFF0000;
   sh->regs.R[n]=(sh->regs.R[n]>>16)&0x0000FFFF;
   sh->regs.R[n]|=temp;
   sh->regs.PC+=2;
   sh->cycles++;
}


//////////////////////////////////////////////////////////////////////////////

static void SH2sleep(SH2_struct * sh)
{
   sh->cycles += 3;
}

#include "sh2_functions.inc"

