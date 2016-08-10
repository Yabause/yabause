/*  Copyright 2016 d356

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

#ifndef SH2JIT_H
#define SH2JIT_H

#define SH2CORE_JIT             5

struct Sh2JitContext
{
   u32 r[16];
   u32 sr;
   u32 gbr;
   u32 vbr;
   u32 mach;
   u32 macl;
   u32 pr;
   u32 pc;
   s32 cycles;

   u32 tmp0, tmp1;

   s32 HH, HL, LH, LL;

   s32 dest, src, ans;
};

int SH2JitInit(enum SHMODELTYPE model, SH2_struct *msh, SH2_struct *ssh);
void SH2JitDeInit(void);
void SH2JitReset(SH2_struct *context);
void FASTCALL SH2JitExec(SH2_struct *context, u32 cycles);
void SH2JitGetRegisters(SH2_struct *context, sh2regs_struct *regs);
u32 SH2JitGetGPR(SH2_struct *context, int num);
u32 SH2JitGetSR(SH2_struct *context);
u32 SH2JitGetGBR(SH2_struct *context);
u32 SH2JitGetVBR(SH2_struct *context);
u32 SH2JitGetMACH(SH2_struct *context);
u32 SH2JitGetMACL(SH2_struct *context);
u32 SH2JitGetPR(SH2_struct *context);
u32 SH2JitGetPC(SH2_struct *context);
void SH2JitSetRegisters(SH2_struct *context, const sh2regs_struct *regs);
void SH2JitSetGPR(SH2_struct *context, int num, u32 value);
void SH2JitSetSR(SH2_struct *context, u32 value);
void SH2JitSetGBR(SH2_struct *context, u32 value);
void SH2JitSetVBR(SH2_struct *context, u32 value);
void SH2JitSetMACH(SH2_struct *context, u32 value);
void SH2JitSetMACL(SH2_struct *context, u32 value);
void SH2JitSetPR(SH2_struct *context, u32 value);
void SH2JitSetPC(SH2_struct *context, u32 value);
void SH2JitSendInterrupt(SH2_struct *context, u8 level, u8 vector);
int SH2JitGetInterrupts(SH2_struct *context,
                                interrupt_struct interrupts[MAX_INTERRUPTS]);
void SH2JitSetInterrupts(SH2_struct *context, int num_interrupts,
                                 const interrupt_struct interrupts[MAX_INTERRUPTS]);

extern SH2Interface_struct SH2Jit;

#endif
