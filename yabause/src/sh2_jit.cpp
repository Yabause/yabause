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

/*! \file sh2_jit.cpp
    \brief SH-1 / SH-2 dynamic recompiler
*/
extern "C"
{
#include "sh2core.h"
#include "sh2idle.h"
#include "cs0.h"
#include "debug.h"
#include "error.h"
#include "memory.h"
#include "bios.h"
#include "yabause.h"
#include "ygr.h"
#include "sh7034.h"
#include "sh2_jit.h"
#include "assert.h"
#include "sh2int.h"

#ifdef SH2_TRACE
#include "sh2trace.h"
#endif
   SH2Interface_struct SH2Jit = {
      SH2CORE_JIT,
      "SH Jit",

      SH2InterpreterInit,
      SH2InterpreterDeInit,
      SH2InterpreterReset,
      SH2JitExec,

      SH2JitGetRegisters,
      SH2JitGetGPR,
      SH2JitGetSR,
      SH2JitGetGBR,
      SH2JitGetVBR,
      SH2JitGetMACH,
      SH2JitGetMACL,
      SH2JitGetPR,
      SH2JitGetPC,

      SH2JitSetRegisters,
      SH2JitSetGPR,
      SH2JitSetSR,
      SH2JitSetGBR,
      SH2JitSetVBR,
      SH2JitSetMACH,
      SH2JitSetMACL,
      SH2JitSetPR,
      SH2JitSetPC,

      SH2InterpreterSendInterrupt,
      SH2InterpreterGetInterrupts,
      SH2InterpreterSetInterrupts,

      NULL  // SH2WriteNotify not used
   };
}

#define SR_T 0x00000001
#define SR_S 0x00000002
#define SR_Q 0x00000100
#define SR_M 0x00000200

#define SH1_ROM_SIZE 0x10000
#define MAX_SH1_BLOCKS (SH1_ROM_SIZE / 2)

#include "MemStream.h"
#include "MemoryFunction.h"
#include "Jitter_CodeGenFactory.h"
#include "Jitter.h"
#include "offsetof_def.h"

struct ShCodeBlock
{
   int dirty;
   CMemoryFunction function;
   u32 start_pc;
   u32 end_pc;
}code_blocks[MAX_SH1_BLOCKS];

static Jitter::CJitter jit(Jitter::CreateCodeGen());

u32 basic_block = 0;

typedef void (FASTCALL *jit_opcode_func)(u16 instruction, u32 recompile_addr);
static jit_opcode_func decode(enum SHMODELTYPE model, u16 instruction);

static void FASTCALL SH2undecoded(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void add_cycles(u32 cycles_to_add)
{
   jit.PushRel(offsetof(Sh2JitContext, cycles));
   jit.PushCst(cycles_to_add);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, cycles));

   jit.PushCst(cycles_to_add);
   jit.Call(reinterpret_cast<void*>(&sh1_dma_exec), 1, Jitter::CJitter::RETURN_VALUE_NONE);
}

static void increment_pc()
{
   jit.PushRel(offsetof(Sh2JitContext, pc));
   jit.PushCst(2);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, pc));
}

SH2_struct *current;

void mapped_memory_write_byte(u32 addr, u32 data)
{
   current->MappedMemoryWriteByte(current, addr, data);
}

void mapped_memory_write_word(u32 addr, u32 data)
{
   current->MappedMemoryWriteWord(current, addr, data);
}

void mapped_memory_write_long(u32 addr, u32 data)
{
   current->MappedMemoryWriteLong(current, addr, data);
}

u32 mapped_memory_read_byte(u32 addr)
{
   u8 val = current->MappedMemoryReadByte(current, addr);
   return val;
}

u32 mapped_memory_read_word(u32 addr)
{
   u16 val = current->MappedMemoryReadWord(current, addr);
   return val;
}

u32 mapped_memory_read_long(u32 addr)
{
   u32 val = current->MappedMemoryReadLong(current, addr);
   return val;
}

//////////////////////////////////////////////////////////////////////////////

//0 format
//div0u, rts, clrt, clrmac, nop, rte, sett, sleep
static void FASTCALL SH2div0u(u16 instruction, u32 recompile_addr)
{
   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.PushCst(~(SR_M | SR_Q | SR_T));
   jit.And();
   jit.PullRel(offsetof(Sh2JitContext, sr));
   increment_pc();
   add_cycles(1);
}

void recompile_sh2_instruction(u32 recompile_addr)
{
   u16 instruction = ((fetchfunc *)current->fetchlist)[(recompile_addr >> 20) & 0x0FF](current, recompile_addr);

   jit_opcode_func func = decode(SHMT_SH1, instruction);

   func(instruction, recompile_addr);
}

void SH2delay(u32 recompile_addr)
{
   recompile_sh2_instruction(recompile_addr);

   jit.PushRel(offsetof(Sh2JitContext, pc));
   jit.PushCst(2);
   jit.Sub();
   jit.PullRel(offsetof(Sh2JitContext, pc));
}

static void FASTCALL SH2rts(u16 instruction, u32 recompile_addr)
{
   jit.PushRel(offsetof(Sh2JitContext, pr));
   jit.PullRel(offsetof(Sh2JitContext, pc));

   add_cycles(2);

   SH2delay(recompile_addr + 2);

   basic_block = 1;
}

static void FASTCALL SH2clrt(u16 instruction, u32 recompile_addr)
{
   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.PushCst(~SR_T);
   jit.And();
   jit.PullRel(offsetof(Sh2JitContext, sr));
   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2clrmac(u16 instruction, u32 recompile_addr)
{
   jit.PushCst(0);
   jit.PullRel(offsetof(Sh2JitContext, mach));
   jit.PushCst(0);
   jit.PullRel(offsetof(Sh2JitContext, macl));
   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2nop(u16 instruction, u32 recompile_addr)
{
   increment_pc();
   add_cycles(1);
}

void r15_add_4()
{
   jit.PushRel(offsetof(Sh2JitContext, r[15]));
   jit.PushCst(4);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, r[15]));
}

static void FASTCALL SH2rte(u16 instruction, u32 recompile_addr)
{
   jit.PushRel(offsetof(Sh2JitContext, r[15]));
   jit.Call(reinterpret_cast<void*>(&mapped_memory_read_long), 1, Jitter::CJitter::RETURN_VALUE_32);
   jit.PullRel(offsetof(Sh2JitContext, pc));
   r15_add_4();

   jit.PushRel(offsetof(Sh2JitContext, r[15]));
   jit.Call(reinterpret_cast<void*>(&mapped_memory_read_long), 1, Jitter::CJitter::RETURN_VALUE_32);
   jit.PushCst(0x000003F3);
   jit.And();
   jit.PullRel(offsetof(Sh2JitContext, sr));
   r15_add_4();

   add_cycles(4);

   SH2delay(recompile_addr + 2);

   basic_block = 1;
}

void set_t_const(int value)
{
   if (value == 0)
   {
      jit.PushRel(offsetof(Sh2JitContext, sr));
      jit.PushCst(~SR_T);
      jit.And();
      jit.PullRel(offsetof(Sh2JitContext, sr));
   }
   else
   {
      jit.PushRel(offsetof(Sh2JitContext, sr));
      jit.PushCst(SR_T);
      jit.Or();
      jit.PullRel(offsetof(Sh2JitContext, sr));
   }
}

static void FASTCALL SH2sett(u16 instruction, u32 recompile_addr)
{
   set_t_const(1);
   increment_pc();
   add_cycles(1);
}

//needs special handling probably
static void FASTCALL SH2sleep(u16 instruction, u32 recompile_addr)
{
   add_cycles(3);
}

//////////////////////////////////////////////////////////////////////////////

//n format
//movt, cmp/pz, cmp/pl, dt, tas.b

//rotl, rotr, rotcl, rotcr, 
//shal, shar, shll, shlr
//shll2, shlr2, shll8, shlr8
//shll16, shlr16

//stc sr, stc gbr, stc vbr,
//stc.l sr, @-rn, STC.L GBR, @–Rn, STC.L VBR,@–Rn 
//sts mach, sts macl, sts pr
//sts.l mach, sts.l macl, sts.l pr

static void FASTCALL SH2movt(u16 instruction, u32 recompile_addr)
{
   u32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.PushCst(SR_T);
   jit.And();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

#define CMPEQ 0
#define CMPGE 1
#define CMPGT 2
#define CMPHI 3
#define CMPHS 4
#define CMPPL 5
#define CMPPZ 6

void set_t()//requires value on stack
{
   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.PushCst(~SR_T);
   jit.And();
   jit.Or();
   jit.PullRel(offsetof(Sh2JitContext, sr));
}

//support for unsigned conditionals needs to be added to codegen
u32 unsigned_gt(u32 a, u32 b)
{
   return a > b;
}

u32 unsigned_ge(u32 a, u32 b)
{
   return a >= b;
}

u32 signed_ge(s32 a, s32 b)
{
   return a >= b;
}

u32 signed_gt(s32 a, s32 b)
{
   return a > b;
}

//todo add missing comparisons to codegen
static void FASTCALL SH2cmpeq_template(u16 instruction, u32 cond)
{
   int skip = 0;
   switch (cond)
   {
   case CMPEQ:
      jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_B(instruction)]));
      jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_C(instruction)]));
      jit.Cmp(Jitter::CONDITION_EQ);
      break;
   case CMPGE:
      //ge not implemented, switch arguments and use le
      jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_C(instruction)]));
      jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_B(instruction)]));
      jit.Cmp(Jitter::CONDITION_LE);
      break;
   case CMPGT:
      jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_B(instruction)]));
      jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_C(instruction)]));
      jit.Cmp(Jitter::CONDITION_GT);
      break;
   case CMPHI:
   case CMPHS:
   case CMPPZ:
   case CMPPL:
      skip = 1;
      jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_B(instruction)]));

      if (cond == CMPPZ || cond == CMPPL)
         jit.PushCst(0);
      else
         jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_C(instruction)]));

      if (cond == CMPHI)
         jit.Call(reinterpret_cast<void*>(&unsigned_gt), 2, Jitter::CJitter::RETURN_VALUE_32);
      else if (cond == CMPHS)
         jit.Call(reinterpret_cast<void*>(&unsigned_ge), 2, Jitter::CJitter::RETURN_VALUE_32);
      else if (cond == CMPPL)
         jit.Call(reinterpret_cast<void*>(&signed_gt), 2, Jitter::CJitter::RETURN_VALUE_32);
      else
         jit.Call(reinterpret_cast<void*>(&signed_ge), 2, Jitter::CJitter::RETURN_VALUE_32);


      jit.PushCst(1);
      jit.BeginIf(Jitter::CONDITION_EQ);
      {
         jit.PushCst(1);
         set_t();
      }
      jit.Else();
      {
         jit.PushCst(0);
         set_t();
      }
      jit.EndIf();
      break;
   default:
      assert(0);
      break;
   }

   if (!skip)
   {
      jit.PushRel(offsetof(Sh2JitContext, sr));
      jit.PushCst(~SR_T);
      jit.And();
      jit.Or();
      jit.PullRel(offsetof(Sh2JitContext, sr));
   }

   increment_pc();
   add_cycles(1);

   assert(jit.IsStackEmpty());
}

static void FASTCALL SH2cmppz(u16 instruction, u32 recompile_addr)
{
   SH2cmpeq_template(instruction, CMPPZ);
}

static void FASTCALL SH2cmppl(u16 instruction, u32 recompile_addr)
{
   SH2cmpeq_template(instruction, CMPPL);
}

void set_t_if_zero()
{
   jit.PushCst(0);
   jit.BeginIf(Jitter::CONDITION_EQ);
   {
      jit.PushRel(offsetof(Sh2JitContext, sr));
      jit.PushCst(SR_T);
      jit.Or();
      jit.PullRel(offsetof(Sh2JitContext, sr));
   }
   jit.Else();
   {
      jit.PushRel(offsetof(Sh2JitContext, sr));
      jit.PushCst(~SR_T);
      jit.And();
      jit.PullRel(offsetof(Sh2JitContext, sr));
   }
   jit.EndIf();
}

static void FASTCALL SH2dt(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushCst(1);
   jit.Sub();
   jit.PushTop();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   set_t_if_zero();

   increment_pc();
   add_cycles(1);
}

void set_sr_t_after_cmp()
{
   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.PushCst(~SR_T);
   jit.And();
   jit.Or();
   jit.PullRel(offsetof(Sh2JitContext, sr));
}

static void FASTCALL SH2tas(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.Call(reinterpret_cast<void*>(&mapped_memory_read_byte), 1, Jitter::CJitter::RETURN_VALUE_32);
   jit.PushTop();
   jit.PullRel(offsetof(Sh2JitContext, tmp0));

   jit.PushCst(0);
   jit.Cmp(Jitter::CONDITION_EQ);
   set_sr_t_after_cmp();

   jit.PushRel(offsetof(Sh2JitContext, r[n]));

   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   jit.PushCst(0x00000080);
   jit.Or();

   jit.Call(reinterpret_cast<void*>(&mapped_memory_write_byte), 2, Jitter::CJitter::RETURN_VALUE_NONE);

   increment_pc();
   add_cycles(4);
}

//rotation/shift

void rotate_mask_by_stack(u32 n, u32 a, u32 b)
{
   jit.BeginIf(Jitter::CONDITION_EQ);
   {
      jit.PushRel(offsetof(Sh2JitContext, r[n]));
      jit.PushCst(a);
      jit.Or();
      jit.PullRel(offsetof(Sh2JitContext, r[n]));
   }
   jit.Else();
   {
      jit.PushRel(offsetof(Sh2JitContext, r[n]));
      jit.PushCst(b);
      jit.And();
      jit.PullRel(offsetof(Sh2JitContext, r[n]));
   }
   jit.EndIf();
}

void rotate_mask_by_t(u32 n, u32 a, u32 b)
{
   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.PushCst(SR_T);
   jit.And();
   jit.PushCst(SR_T);
   rotate_mask_by_stack(n, a, b);
}

void rot_set_t_or_temp(u32 n, u32 check_bit, int set_t)
{
   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushCst(check_bit);
   jit.And();

   jit.PushCst(check_bit);
   jit.Cmp(Jitter::CONDITION_EQ);
   if (set_t)
      set_sr_t_after_cmp();
   else
   {
      jit.PullRel(offsetof(Sh2JitContext, tmp0));
   }
}

static void FASTCALL SH2rotl(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);

   rot_set_t_or_temp(n, 0x80000000, 1);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.Shl(1);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   rotate_mask_by_t(n, 1, 0xFFFFFFFE);

   increment_pc();
   add_cycles(1);

   assert(jit.IsStackEmpty());
}

static void FASTCALL SH2rotr(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);

   rot_set_t_or_temp(n, 1, 1);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.Srl(1);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   rotate_mask_by_t(n, 0x80000000, 0x7FFFFFFF);

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2rotcl(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);

   rot_set_t_or_temp(n, 0x80000000, 0);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.Shl(1);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   rotate_mask_by_t(n, 0x00000001, 0xFFFFFFFE);

   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   set_t();

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2rotcr(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);

   rot_set_t_or_temp(n, 0x00000001, 0);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.Srl(1);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   rotate_mask_by_t(n, 0x80000000, 0x7FFFFFFF);

   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   set_t();

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2shal(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);

   rot_set_t_or_temp(n, 0x80000000, 1);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.Shl(1);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2shar(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);

   rot_set_t_or_temp(n, 1, 1);

   assert(jit.IsStackEmpty());

   rot_set_t_or_temp(n, 0x80000000, 0);
   assert(jit.IsStackEmpty());

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.Srl(1);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   assert(jit.IsStackEmpty());

   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   jit.PushCst(1);
   rotate_mask_by_stack(n, 0x80000000, 0x7FFFFFFF);

   assert(jit.IsStackEmpty());

   increment_pc();
   add_cycles(1);
}

static void FASTCALL shlr_template(u16 instruction, u32 shift, u32 left)
{
   s32 n = INSTRUCTION_B(instruction);
   jit.PushRel(offsetof(Sh2JitContext, r[n]));

   if (left)
      jit.Shl(shift);
   else
      jit.Srl(shift);

   jit.PullRel(offsetof(Sh2JitContext, r[n]));
   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2shll(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);

   rot_set_t_or_temp(n, 0x80000000, 1);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.Shl(1);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2shlr(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);

   rot_set_t_or_temp(n, 0x00000001, 1);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.Srl(1);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2shll2(u16 instruction, u32 recompile_addr)
{
   shlr_template(instruction, 2, 1);
}

static void FASTCALL SH2shlr2(u16 instruction, u32 recompile_addr)
{
   shlr_template(instruction, 2, 0);
}

static void FASTCALL SH2shll8(u16 instruction, u32 recompile_addr)
{
   shlr_template(instruction, 8, 1);
}

static void FASTCALL SH2shlr8(u16 instruction, u32 recompile_addr)
{
   shlr_template(instruction, 8, 0);
}

static void FASTCALL SH2shll16(u16 instruction, u32 recompile_addr)
{
   shlr_template(instruction, 16, 1);
}

static void FASTCALL SH2shlr16(u16 instruction, u32 recompile_addr)
{
   shlr_template(instruction, 16, 0);
}

//system control

static void FASTCALL SH2stc_sts_template(u16 instruction, size_t src)
{
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(src);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2stcsr(u16 instruction, u32 recompile_addr)
{
   SH2stc_sts_template(instruction, offsetof(Sh2JitContext, sr));
}

static void FASTCALL SH2stcgbr(u16 instruction, u32 recompile_addr)
{
   SH2stc_sts_template(instruction, offsetof(Sh2JitContext, gbr));
}

static void FASTCALL SH2stcvbr(u16 instruction, u32 recompile_addr)
{
   SH2stc_sts_template(instruction, offsetof(Sh2JitContext, vbr));
}

void SH2stsm_template(u16 instruction, u32 recompile_addr, size_t offset, u32 cycles)
{
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushCst(4);
   jit.Sub();
   jit.PushTop();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   jit.PushRel(offset);
   jit.Call(reinterpret_cast<void*>(&mapped_memory_write_long), 2, Jitter::CJitter::RETURN_VALUE_NONE);
   increment_pc();
   add_cycles(cycles);
}

static void FASTCALL SH2stcmsr(u16 instruction, u32 recompile_addr)
{
   SH2stsm_template(instruction, recompile_addr, offsetof(Sh2JitContext, sr), 2);
}

static void FASTCALL SH2stcmgbr(u16 instruction, u32 recompile_addr)
{
   SH2stsm_template(instruction, recompile_addr, offsetof(Sh2JitContext, gbr), 2);
}

static void FASTCALL SH2stcmvbr(u16 instruction, u32 recompile_addr)
{
   SH2stsm_template(instruction, recompile_addr, offsetof(Sh2JitContext, vbr), 2);
}

static void FASTCALL SH2stsmach(u16 instruction, u32 recompile_addr)
{
   SH2stc_sts_template(instruction, offsetof(Sh2JitContext, mach));
}

static void FASTCALL SH2stsmacl(u16 instruction, u32 recompile_addr)
{
   SH2stc_sts_template(instruction, offsetof(Sh2JitContext, macl));
}

static void FASTCALL SH2stspr(u16 instruction, u32 recompile_addr)
{
   SH2stc_sts_template(instruction, offsetof(Sh2JitContext, pr));
}

static void FASTCALL SH2stsmmach(u16 instruction, u32 recompile_addr)
{
   SH2stsm_template(instruction, recompile_addr, offsetof(Sh2JitContext, mach), 1);
}

static void FASTCALL SH2stsmmacl(u16 instruction, u32 recompile_addr)
{
   SH2stsm_template(instruction, recompile_addr, offsetof(Sh2JitContext, macl), 1);
}

static void FASTCALL SH2stsmpr(u16 instruction, u32 recompile_addr)
{
   SH2stsm_template(instruction, recompile_addr, offsetof(Sh2JitContext, pr), 1);
}

//////////////////////////////////////////////////////////////////////////////

//m format

//braf, bsrf, jmp, jsr
//ldc sr, ldc gbr, ldc vbr
//ldc.l sr, gbr, vbr
//lds mach macl pr
//lds.l mach macl pr

void pc_plus_rm_plus_4(u32 m)
{
   jit.PushRel(offsetof(Sh2JitContext, pc));
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.PushCst(4);
   jit.Add();
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, pc));
}

static void FASTCALL SH2braf(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_B(instruction);

   pc_plus_rm_plus_4(m);

   add_cycles(2);

   SH2delay(recompile_addr + 2);

   basic_block = 1;
}

static void FASTCALL SH2bsrf(u16 instruction, u32 recompile_addr)
{
   u32 m = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, pc));
   jit.PushCst(4);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, pr));

   pc_plus_rm_plus_4(m);

   add_cycles(2);

   SH2delay(recompile_addr + 2);

   basic_block = 1;
}

static void FASTCALL SH2jmp(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.PullRel(offsetof(Sh2JitContext, pc));

   add_cycles(2);
   SH2delay(recompile_addr + 2);

   basic_block = 1;
}

static void FASTCALL SH2jsr(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, pc));
   jit.PushCst(4);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, pr));

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.PullRel(offsetof(Sh2JitContext, pc));

   add_cycles(2);
   SH2delay(recompile_addr + 2);

   basic_block = 1;
}

static void FASTCALL SH2ldc_lds_template(u16 instruction, size_t dest, int is_sr)
{
   jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_B(instruction)]));
   if (is_sr)
   {
      jit.PushCst(0x000003F3);
      jit.And();
   }
   jit.PullRel(dest);
   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2ldcsr(u16 instruction, u32 recompile_addr)
{
   SH2ldc_lds_template(instruction, offsetof(Sh2JitContext, sr), 1);
}

static void FASTCALL SH2ldcgbr(u16 instruction, u32 recompile_addr)
{
   SH2ldc_lds_template(instruction, offsetof(Sh2JitContext, gbr), 0);
}

static void FASTCALL SH2ldcvbr(u16 instruction, u32 recompile_addr)
{
   SH2ldc_lds_template(instruction, offsetof(Sh2JitContext, vbr), 0);
}

void rm_add_4(s32 m)
{
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.PushCst(4);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, r[m]));
}

static void FASTCALL SH2ldcmgbr_template(u32 instruction, int is_gbr)
{
   s32 m = INSTRUCTION_B(instruction);
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Call(reinterpret_cast<void*>(mapped_memory_read_long), 1, Jitter::CJitter::RETURN_VALUE_32);

   if (is_gbr)
      jit.PullRel(offsetof(Sh2JitContext, gbr));
   else
      jit.PullRel(offsetof(Sh2JitContext, vbr));

   rm_add_4(m);

   increment_pc();
   add_cycles(3);
}

static void FASTCALL SH2ldcmsr(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_B(instruction);
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Call(reinterpret_cast<void*>(mapped_memory_read_long), 1, Jitter::CJitter::RETURN_VALUE_32);
   jit.PushCst(0x000003F3);
   jit.And();
   jit.PullRel(offsetof(Sh2JitContext, sr));
   rm_add_4(m);
   increment_pc();
   add_cycles(3);
}

static void FASTCALL SH2ldcmgbr(u16 instruction, u32 recompile_addr)
{
   SH2ldcmgbr_template(instruction, 1);
}

static void FASTCALL SH2ldcmvbr(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void FASTCALL SH2ldsmach(u16 instruction, u32 recompile_addr)
{
   SH2ldc_lds_template(instruction, offsetof(Sh2JitContext, mach), 0);
}

static void FASTCALL SH2ldsmacl(u16 instruction, u32 recompile_addr)
{
   SH2ldc_lds_template(instruction, offsetof(Sh2JitContext, macl), 0);
}

static void FASTCALL SH2ldspr(u16 instruction, u32 recompile_addr)
{
   SH2ldc_lds_template(instruction, offsetof(Sh2JitContext, pr), 0);
}

static void FASTCALL SH2ldsm_template(u16 instruction, u32 recompile_addr, size_t offset)
{
   s32 m = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Call(reinterpret_cast<void*>(&mapped_memory_read_long), 1, Jitter::CJitter::RETURN_VALUE_32);
   jit.PullRel(offset);

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.PushCst(4);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, r[m]));
   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2ldsmmach(u16 instruction, u32 recompile_addr)
{
   SH2ldsm_template(instruction, recompile_addr, offsetof(Sh2JitContext, mach));
}

static void FASTCALL SH2ldsmmacl(u16 instruction, u32 recompile_addr)
{
   SH2ldsm_template(instruction, recompile_addr, offsetof(Sh2JitContext, macl));
}

static void FASTCALL SH2ldsmpr(u16 instruction, u32 recompile_addr)
{
   SH2ldsm_template(instruction, recompile_addr, offsetof(Sh2JitContext, pr));
}

//////////////////////////////////////////////////////////////////////////////

//n m instructions

//mov rm, rn
//mov (memory access)
//swap.b, swap.w
//xtrct
//add, addc, addv 
//cmp/eq, cmp/hs, cmp/ge, cmp/hi, cmp/gt, cmp/st (cmpstr)
//div1, div0s
//dmuls.l, dmulu.l
//exts.b, exts.w, extu.b, extu.w
//mac.l, mac
//mul.l, muls.w, mulu.w
//neg, negc
//sub, subc, subv
//and, not, or
//tst
//xor

static void FASTCALL SH2mov(u16 instruction, u32 recompile_addr)
{
   u32 n = INSTRUCTION_B(instruction);
   u32 m = INSTRUCTION_C(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

//memory access movs

//mov.b/w/l rm, @rn       SH2movbs  SH2movws  SH2movls
//mob.b/w/l @rm, rn       SH2movbl  SH2movwl  SH2movll
//mov.b/w/l rm, @-rn      SH2movbm  SH2movwm  SH2movlm
//mov.b/w/l @rm+, rn      SH2movbp  SH2movwp  SH2movlp
//mov.b/w/l rm, @(r0, rn) SH2movbs0 SH2movws0 SH2movls0
//mov.b/w/l @(r0, rm), rn SH2movbl0 SH2movwl0 SH2movll0

void read_with_extend(int size)
{
   if (size == 0)
   {
      jit.Call(reinterpret_cast<void*>(&mapped_memory_read_byte), 1, Jitter::CJitter::RETURN_VALUE_32);
      jit.SignExt8();
   }
   else if (size == 1)
   {
      jit.Call(reinterpret_cast<void*>(&mapped_memory_read_word), 1, Jitter::CJitter::RETURN_VALUE_32);
      jit.SignExt16();
   }
   else
      jit.Call(reinterpret_cast<void*>(&mapped_memory_read_long), 1, Jitter::CJitter::RETURN_VALUE_32);
}

static void FASTCALL SH2movb_template(u16 instruction, u32 is_bl0, int size)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   if (is_bl0)
   {
      jit.PushRel(offsetof(Sh2JitContext, r[0]));
      jit.Add();
   }

   read_with_extend(size);

   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

void write_size(int size)
{
   if (size == 0)
      jit.Call(reinterpret_cast<void*>(&mapped_memory_write_byte), 2, Jitter::CJitter::RETURN_VALUE_NONE);
   else if (size == 1)
      jit.Call(reinterpret_cast<void*>(&mapped_memory_write_word), 2, Jitter::CJitter::RETURN_VALUE_NONE);
   else
      jit.Call(reinterpret_cast<void*>(&mapped_memory_write_long), 2, Jitter::CJitter::RETURN_VALUE_NONE);
}

static void FASTCALL SH2movbs_template(u16 instruction, int is_bs0, int size)
{
   int b = INSTRUCTION_B(instruction);
   int c = INSTRUCTION_C(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[b]));

   if (is_bs0)
   {
      jit.PushRel(offsetof(Sh2JitContext, r[0]));
      jit.Add();
   }
   jit.PushRel(offsetof(Sh2JitContext, r[c]));

   write_size(size);

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2movbs(u16 instruction, u32 recompile_addr)
{
   SH2movbs_template(instruction, 0, 0);
}

static void FASTCALL SH2movws(u16 instruction, u32 recompile_addr)
{
   SH2movbs_template(instruction, 0, 1);
}

static void FASTCALL SH2movls(u16 instruction, u32 recompile_addr)
{
   SH2movbs_template(instruction, 0, 2);
}

static void FASTCALL SH2movbl(u16 instruction, u32 recompile_addr)
{
   SH2movb_template(instruction, 0, 0);
}

static void FASTCALL SH2movwl(u16 instruction, u32 recompile_addr)
{
   SH2movb_template(instruction, 0, 1);
}

static void FASTCALL SH2movll(u16 instruction, u32 recompile_addr)
{
   SH2movb_template(instruction, 0, 2);
}

static void SH2movbm_template(u16 instruction, int size)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   int vals[] = { 1, 2, 4 };

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushCst(vals[size]);
   jit.Sub();
   jit.PushRel(offsetof(Sh2JitContext, r[m]));

   write_size(size);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushCst(vals[size]);
   jit.Sub();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2movbm(u16 instruction, u32 recompile_addr)
{
   SH2movbm_template(instruction, 0);
}

static void FASTCALL SH2movwm(u16 instruction, u32 recompile_addr)
{
   SH2movbm_template(instruction, 1);
}

static void FASTCALL SH2movlm(u16 instruction, u32 recompile_addr)
{
   SH2movbm_template(instruction, 2);
}

void SH2movp_template(u16 instruction, int size)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   int values[] = { 1,2,4 };

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   read_with_extend(size);

   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   if (n != m)
   {
      jit.PushRel(offsetof(Sh2JitContext, r[m]));
      jit.PushCst(values[size]);
      jit.Add();
      jit.PullRel(offsetof(Sh2JitContext, r[m]));
   }

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2movbp(u16 instruction, u32 recompile_addr)
{
   SH2movp_template(instruction, 0);
}

static void FASTCALL SH2movwp(u16 instruction, u32 recompile_addr)
{
   SH2movp_template(instruction, 1);
}

static void FASTCALL SH2movlp(u16 instruction, u32 recompile_addr)
{
   SH2movp_template(instruction, 2);
}

static void FASTCALL SH2movbs0(u16 instruction, u32 recompile_addr)
{
   SH2movbs_template(instruction, 1, 0);
}

static void FASTCALL SH2movws0(u16 instruction, u32 recompile_addr)
{
   SH2movbs_template(instruction, 1, 1);
}

static void FASTCALL SH2movls0(u16 instruction, u32 recompile_addr)
{
   SH2movbs_template(instruction, 1, 2);
}

static void FASTCALL SH2movbl0(u16 instruction, u32 recompile_addr)
{
   SH2movb_template(instruction, 1, 0);
}

static void FASTCALL SH2movwl0(u16 instruction, u32 recompile_addr)
{
   SH2movb_template(instruction, 1, 1);
}

static void FASTCALL SH2movll0(u16 instruction, u32 recompile_addr)
{
   SH2movb_template(instruction, 1, 2);
}

//others

static void FASTCALL SH2swapb(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.PushCst(0xffff0000);
   jit.And();
   jit.PullRel(offsetof(Sh2JitContext, tmp0));

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.PushCst(0xff);
   jit.And();
   jit.Shl(8);
   jit.PullRel(offsetof(Sh2JitContext, tmp1));

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Srl(8);
   jit.PushCst(0xff);
   jit.And();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   jit.PushRel(offsetof(Sh2JitContext, tmp1));
   jit.Or();
   jit.Or();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2swapw(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Srl(16);

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Shl(16);

   jit.Or();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2xtrct(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.Srl(16);
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Shl(16);
   jit.Or();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2add(u16 instruction, u32 recompile_addr)
{
   jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_B(instruction)]));
   jit.PushRel(offsetof(Sh2JitContext, r[INSTRUCTION_C(instruction)]));
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, r[INSTRUCTION_B(instruction)]));

   increment_pc();

   add_cycles(1);
}

static void FASTCALL SH2addc(u16 instruction, u32 recompile_addr)
{
   s32 source = INSTRUCTION_C(instruction);
   s32 dest = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[source]));
   jit.PushRel(offsetof(Sh2JitContext, r[dest]));

   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, tmp1));

   jit.PushRel(offsetof(Sh2JitContext, r[dest]));
   jit.PullRel(offsetof(Sh2JitContext, tmp0));

   jit.PushRel(offsetof(Sh2JitContext, tmp1));
   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.PushCst(SR_T);
   jit.And();
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, r[dest]));

   assert(jit.IsStackEmpty());

   jit.PushRel(offsetof(Sh2JitContext, tmp1));
   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   jit.Cmp(Jitter::CONDITION_BL);
   set_sr_t_after_cmp();

   assert(jit.IsStackEmpty());

   //if (tmp1 > r[dest])
   jit.PushRel(offsetof(Sh2JitContext, tmp1));
   jit.PushRel(offsetof(Sh2JitContext, r[dest]));
   jit.Call(reinterpret_cast<void*>(&unsigned_gt), 2, Jitter::CJitter::RETURN_VALUE_32);
   jit.PushCst(1);
   jit.BeginIf(Jitter::CONDITION_EQ);
   {
      jit.PushRel(offsetof(Sh2JitContext, sr));
      jit.PushCst(SR_T);
      jit.Or();
      jit.PullRel(offsetof(Sh2JitContext, sr));
   }
   jit.EndIf();

   assert(jit.IsStackEmpty());

   increment_pc();
   add_cycles(1);
}

void ans_set_t()
{
   jit.PushCst(1);
   jit.PushRel(offsetof(Sh2JitContext, ans));
   jit.BeginIf(Jitter::CONDITION_EQ);
   {
      set_t_const(1);
   }
   jit.Else();
   {
      set_t_const(0);
   }
   jit.EndIf();
}

u32 signed_ge_inv(s32 a, s32 b)
{
   return !(a >= b);
}

void do_src_dest_ans(u32 nm, size_t srcdest)
{
   jit.PushRel(offsetof(Sh2JitContext, r[nm]));
   jit.PushCst(0);
   jit.Call(reinterpret_cast<void*>(&signed_ge_inv), 2, Jitter::CJitter::RETURN_VALUE_32);
   jit.PullRel(srcdest);
}

static void FASTCALL SH2addv(u16 instruction, u32 recompile_addr)
{
   s32 n = INSTRUCTION_B(instruction);
   s32 m = INSTRUCTION_C(instruction);

   do_src_dest_ans(n, offsetof(Sh2JitContext, dest));

   do_src_dest_ans(m, offsetof(Sh2JitContext, src));

   jit.PushRel(offsetof(Sh2JitContext, src));
   jit.PushRel(offsetof(Sh2JitContext, dest));
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, src));

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   do_src_dest_ans(n, offsetof(Sh2JitContext, ans));

   jit.PushRel(offsetof(Sh2JitContext, ans));
   jit.PushRel(offsetof(Sh2JitContext, dest));
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, ans));

   jit.PushCst(0);
   jit.PushRel(offsetof(Sh2JitContext, src));
   jit.BeginIf(Jitter::CONDITION_EQ);
   {
      ans_set_t();
   }
   jit.Else();
   {
      jit.PushCst(2);
      jit.PushRel(offsetof(Sh2JitContext, src));
      jit.BeginIf(Jitter::CONDITION_EQ);
      {
         ans_set_t();
      }
      jit.Else();
      {
         set_t_const(0);
      }
      jit.EndIf();
   }
   jit.EndIf();

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2cmpeq(u16 instruction, u32 recompile_addr)
{
   SH2cmpeq_template(instruction, CMPEQ);
}

static void FASTCALL SH2cmphs(u16 instruction, u32 recompile_addr)
{
   SH2cmpeq_template(instruction, CMPHS);
}

static void FASTCALL SH2cmpge(u16 instruction, u32 recompile_addr)
{
   SH2cmpeq_template(instruction, CMPGE);
}

static void FASTCALL SH2cmphi(u16 instruction, u32 recompile_addr)
{
   SH2cmpeq_template(instruction, CMPHI);
}

static void FASTCALL SH2cmpgt(u16 instruction, u32 recompile_addr)
{
   SH2cmpeq_template(instruction, CMPGT);
}

//needs hardware test
static void FASTCALL SH2cmpstr(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Xor();
   jit.PullRel(offsetof(Sh2JitContext, tmp0));

   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   jit.Srl(24);
   jit.PullRel(offsetof(Sh2JitContext, HH));

   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   jit.Srl(16);
   jit.PushCst(0xff);
   jit.And();
   jit.PullRel(offsetof(Sh2JitContext, HL));

   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   jit.Srl(8);
   jit.PushCst(0xff);
   jit.And();
   jit.PullRel(offsetof(Sh2JitContext, LH));

   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   jit.PushCst(0xff);
   jit.And();
   jit.PullRel(offsetof(Sh2JitContext, LL));

   jit.PushRel(offsetof(Sh2JitContext, HH));
   jit.PushRel(offsetof(Sh2JitContext, HL));
   jit.PushRel(offsetof(Sh2JitContext, LH));
   jit.PushRel(offsetof(Sh2JitContext, LL));

   jit.And(); //todo & is not the same as &&
   jit.And();
   jit.And();

   jit.PullRel(offsetof(Sh2JitContext, HH));

   assert(jit.IsStackEmpty());

   jit.PushRel(offsetof(Sh2JitContext, HH));
   jit.PushCst(0);
   jit.BeginIf(Jitter::CONDITION_EQ);
   {
      set_t_const(1);
   }
   jit.Else();
   {
      set_t_const(0);
   }
   jit.EndIf();

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2div1(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void FASTCALL SH2div0s(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void FASTCALL SH2dmuls(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void FASTCALL SH2dmulu(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void FASTCALL SH2ext(u16 instruction, u32 is_unsigned, u32 is_word)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[m]));

   if (is_unsigned)
   {
      if (is_word)
         jit.PushCst(0xFFFF);
      else
         jit.PushCst(0xFF);

      jit.And();
   }
   else
   {
      if (is_word)
         jit.SignExt16();
      else
         jit.SignExt8();
   }

   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2extsb(u16 instruction, u32 recompile_addr)
{
   SH2ext(instruction, 0, 0);
}

static void FASTCALL SH2extsw(u16 instruction, u32 recompile_addr)
{
   SH2ext(instruction, 0, 1);
}

static void FASTCALL SH2extub(u16 instruction, u32 recompile_addr)
{
   SH2ext(instruction, 1, 0);
}

static void FASTCALL SH2extuw(u16 instruction, u32 recompile_addr)
{
   SH2ext(instruction, 1, 1);
}

static void FASTCALL SH2macw(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void FASTCALL SH2macl(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void FASTCALL SH2mull(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void FASTCALL SH2muls(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void FASTCALL SH2mulu(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Mult();
   jit.ExtLow64();
   jit.PullRel(offsetof(Sh2JitContext, macl));
   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2neg(u16 instruction, u32 recompile_addr)
{
   int n = INSTRUCTION_B(instruction);
   int m = INSTRUCTION_C(instruction);

   jit.PushCst(0);
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Sub();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));
   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2negc(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

static void FASTCALL SH2sub(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.Sub();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));
   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2subc(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushRel(offsetof(Sh2JitContext, r[m]));

   jit.Sub();
   jit.PullRel(offsetof(Sh2JitContext, tmp1));

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PullRel(offsetof(Sh2JitContext, tmp0));

   jit.PushRel(offsetof(Sh2JitContext, tmp1));
   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.PushCst(SR_T);
   jit.And();
   jit.Sub();
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   assert(jit.IsStackEmpty());

   jit.PushRel(offsetof(Sh2JitContext, tmp0));
   jit.PushRel(offsetof(Sh2JitContext, tmp1));
   jit.Cmp(Jitter::CONDITION_BL);
   set_sr_t_after_cmp();

   assert(jit.IsStackEmpty());

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushRel(offsetof(Sh2JitContext, tmp1));
   jit.Call(reinterpret_cast<void*>(&unsigned_gt), 2, Jitter::CJitter::RETURN_VALUE_32);
   jit.PushCst(1);
   jit.BeginIf(Jitter::CONDITION_EQ);
   {
      jit.PushRel(offsetof(Sh2JitContext, sr));
      jit.PushCst(SR_T);
      jit.Or();
      jit.PullRel(offsetof(Sh2JitContext, sr));
   }
   jit.EndIf();

   assert(jit.IsStackEmpty());

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2subv(u16 instruction, u32 recompile_addr)
{
   assert(0);
}

#define ALU_AND 0
#define ALU_XOR 1
#define ALU_OR 2
#define ALU_NOT 3

void alu_select(int op)
{
   switch (op)
   {
   case ALU_AND:
      jit.And();
      break;
   case ALU_XOR:
      jit.Xor();
      break;
   case ALU_OR:
      jit.Or();
      break;
   case ALU_NOT:
      jit.Not();
      break;
   }
}

static void FASTCALL SH2y_alu_template(u16 instruction, u32 op)
{
   u32 n = INSTRUCTION_B(instruction);
   u32 m = INSTRUCTION_C(instruction);

   if (!(op == ALU_NOT))
      jit.PushRel(offsetof(Sh2JitContext, r[n]));

   jit.PushRel(offsetof(Sh2JitContext, r[m]));

   alu_select(op);

   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   assert(jit.IsStackEmpty());

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2y_and(u16 instruction, u32 recompile_addr)
{
   SH2y_alu_template(instruction, ALU_AND);
}

static void FASTCALL SH2y_not(u16 instruction, u32 recompile_addr)
{
   SH2y_alu_template(instruction, ALU_NOT);
}

static void FASTCALL SH2y_or(u16 instruction, u32 recompile_addr)
{
   SH2y_alu_template(instruction, ALU_OR);
}

static void FASTCALL SH2tst(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.And();
   jit.PushCst(0);
   jit.Cmp(Jitter::CONDITION_EQ);
   set_sr_t_after_cmp();

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2y_xor(u16 instruction, u32 recompile_addr)
{
   SH2y_alu_template(instruction, ALU_XOR);
}

//////////////////////////////////////////////////////////////////////////////

//md format
//mov.b @(disp, rm), r0 SH2movbl4
//mov.w @(disp, rm), r0 SH2movwl4

static void FASTCALL SH2movbl4_template(u16 instruction, int size)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 disp = INSTRUCTION_D(instruction);

   int tabl[] = { 1,2 };

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.PushCst(disp * tabl[size]);
   jit.Add();
   read_with_extend(size);
   jit.PullRel(offsetof(Sh2JitContext, r[0]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2movbl4(u16 instruction, u32 recompile_addr)
{
   SH2movbl4_template(instruction, 0);
}

static void FASTCALL SH2movwl4(u16 instruction, u32 recompile_addr)
{
   SH2movbl4_template(instruction, 1);
}

//////////////////////////////////////////////////////////////////////////////

//nd4 format

//mov.b r0, @(disp, rn) SH2movbs4
//mov.w r0, @(disp, rn) SH2movws4

static void FASTCALL SH2movbs4_template(u16 instruction, int size)
{
   s32 disp = INSTRUCTION_D(instruction);
   s32 n = INSTRUCTION_C(instruction);

   int tbl[] = { 1,2 };

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushCst(disp * tbl[size]);
   jit.Add();
   jit.PushRel(offsetof(Sh2JitContext, r[0]));
   write_size(size);

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2movbs4(u16 instruction, u32 recompile_addr)
{
   SH2movbs4_template(instruction, 0);
}

static void FASTCALL SH2movws4(u16 instruction, u32 recompile_addr)
{
   SH2movbs4_template(instruction, 1);
}

//////////////////////////////////////////////////////////////////////////////

//nmd format

//mov.l rm @(disp, rn)  SH2movls4
//mov.l @(disp, rm), rn SH2movll4

static void FASTCALL SH2movls4(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 disp = INSTRUCTION_D(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[n]));
   jit.PushCst(disp * 4);
   jit.Add();
   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   write_size(2);

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2movll4(u16 instruction, u32 recompile_addr)
{
   s32 m = INSTRUCTION_C(instruction);
   s32 disp = INSTRUCTION_D(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[m]));
   jit.PushCst(disp * 4);
   jit.Add();
   read_with_extend(2);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

//////////////////////////////////////////////////////////////////////////////

//d format

//mov.b/w/l r0, @(disp, gbr)  SH2movbsg SH2movwsg SH2movlsg
//mov.b/w/l @(disp, gbr), r0  SH2movblg SH2movwlg SH2movllg
//mova @(disp, pc), r0
//bf, bf/s
//bt, bt/s

static void FASTCALL SH2movbsg_template(u16 instruction, int size)
{
   s32 disp = INSTRUCTION_CD(instruction);

   int size_mul[] = { 1,2,4 };

   jit.PushRel(offsetof(Sh2JitContext, gbr));
   jit.PushCst(disp * size_mul[size]);
   jit.Add();
   jit.PushRel(offsetof(Sh2JitContext, r[0]));

   write_size(size);

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2movbsg(u16 instruction, u32 recompile_addr)
{
   SH2movbsg_template(instruction, 0);
}

static void FASTCALL SH2movwsg(u16 instruction, u32 recompile_addr)
{
   SH2movbsg_template(instruction, 1);
}

static void FASTCALL SH2movlsg(u16 instruction, u32 recompile_addr)
{
   SH2movbsg_template(instruction, 2);
}

static void FASTCALL SH2movblg_template(u16 instruction, int size)
{
   s32 disp = INSTRUCTION_CD(instruction);

   int vals[] = { 1, 2, 4 };

   jit.PushRel(offsetof(Sh2JitContext, gbr));
   jit.PushCst(disp * vals[size]);
   jit.Add();
   read_with_extend(size);
   jit.PullRel(offsetof(Sh2JitContext, r[0]));
   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2movblg(u16 instruction, u32 recompile_addr)
{
   SH2movblg_template(instruction, 0);
}

static void FASTCALL SH2movwlg(u16 instruction, u32 recompile_addr)
{
   SH2movblg_template(instruction, 1);
}

static void FASTCALL SH2movllg(u16 instruction, u32 recompile_addr)
{
   SH2movblg_template(instruction, 2);
}

static void FASTCALL SH2mova(u16 instruction, u32 recompile_addr)
{
   s32 disp = INSTRUCTION_CD(instruction);

   jit.PushRel(offsetof(Sh2JitContext, pc));
   jit.PushCst(4);
   jit.Add();
   jit.PushCst(0xFFFFFFFC);
   jit.And();
   jit.PushCst(disp * 4);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, r[0]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2bf_template(u16 instruction, u32 is_bt)
{
   s32 disp = (((s32)(s8)instruction) * 2) + 4;

   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.PushCst(SR_T);
   jit.And();
   jit.PushCst(is_bt);
   jit.BeginIf(Jitter::CONDITION_EQ);
   {
      jit.PushRel(offsetof(Sh2JitContext, pc));
      jit.PushCst(disp);
      jit.Add();
      jit.PullRel(offsetof(Sh2JitContext, pc));

      add_cycles(3);
   }
   jit.Else();
   {
      increment_pc();
      add_cycles(1);
   }
   jit.EndIf();

   basic_block = 1;
}

static void FASTCALL SH2bf(u16 instruction, u32 recompile_addr)
{
   SH2bf_template(instruction, 0);
}

static void FASTCALL SH2bfs_template(u16 instruction, int is_bts, u32 recompile_addr)
{
   s32 disp = (s32)(s8)instruction;

   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.PushCst(SR_T);
   jit.And();

   if (is_bts)
      jit.PushCst(SR_T);
   else
      jit.PushCst(0);

   jit.BeginIf(Jitter::CONDITION_EQ);
   {
      jit.PushRel(offsetof(Sh2JitContext, pc));
      jit.PushCst((disp * 2) + 4);
      jit.Add();
      jit.PullRel(offsetof(Sh2JitContext, pc));

      add_cycles(2);
      SH2delay(recompile_addr + 2);
   }
   jit.Else();
   {
      increment_pc();
      add_cycles(1);
   }
   jit.EndIf();

   basic_block = 1;
}

static void FASTCALL SH2bfs(u16 instruction, u32 recompile_addr)
{
   SH2bfs_template(instruction, 0, recompile_addr);
}

static void FASTCALL SH2bt(u16 instruction, u32 recompile_addr)
{
   SH2bf_template(instruction, 1);
}

static void FASTCALL SH2bts(u16 instruction, u32 recompile_addr)
{
   SH2bfs_template(instruction, 1, recompile_addr);
}

//////////////////////////////////////////////////////////////////////////////

//d12 format

//bra
//bsr

u32 sign_extend_12(u32 disp)
{
   if ((disp & 0x800) != 0)
      disp |= 0xFFFFF000;

   return disp;
}

void pc_add_bra_disp(s32 disp)
{
   jit.PushRel(offsetof(Sh2JitContext, pc));
   jit.PushCst((disp * 2) + 4);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, pc));
}

static void FASTCALL SH2bra(u16 instruction, u32 recompile_addr)
{
   s32 disp = INSTRUCTION_BCD(instruction);

   disp = sign_extend_12(disp);

   pc_add_bra_disp(disp);

   add_cycles(2);
   SH2delay(recompile_addr + 2);

   basic_block = 1;
}

static void FASTCALL SH2bsr(u16 instruction, u32 recompile_addr)
{
   s32 disp = INSTRUCTION_BCD(instruction);

   disp = sign_extend_12(disp);

   jit.PushRel(offsetof(Sh2JitContext, pc));
   jit.PushCst(4);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, pr));

   pc_add_bra_disp(disp);

   add_cycles(2);
   SH2delay(recompile_addr + 2);

   basic_block = 1;
}

//////////////////////////////////////////////////////////////////////////////

//nd8 format

//mov.w @(disp, pc), rn SH2movwi
//mov.l @(disp, pc) rn  SH2movli

static void FASTCALL SH2movwi_template(u16 instruction, int is_long, u32 recompile_addr)
{
   s32 disp = INSTRUCTION_CD(instruction);
   s32 n = INSTRUCTION_B(instruction);

   if (is_long)
   {
      jit.PushCst(((recompile_addr + 4) & 0xFFFFFFFC) + (disp * 4));
      jit.Call(reinterpret_cast<void*>(&mapped_memory_read_long), 1, Jitter::CJitter::RETURN_VALUE_32);
   }
   else
   {
      jit.PushCst(recompile_addr + (disp * 2) + 4);
      jit.Call(reinterpret_cast<void*>(&mapped_memory_read_word), 1, Jitter::CJitter::RETURN_VALUE_32);
      jit.SignExt16();
   }
   jit.PullRel(offsetof(Sh2JitContext, r[n]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2movwi(u16 instruction, u32 recompile_addr)
{
   SH2movwi_template(instruction, 0, recompile_addr);
}

static void FASTCALL SH2movli(u16 instruction, u32 recompile_addr)
{
   SH2movwi_template(instruction, 1, recompile_addr);
}

//////////////////////////////////////////////////////////////////////////////

//i format

//cmp.eq imm
//and imm, and.b
//or imm, or.b
//tst imm, tst.b
//xor imm, xor.b
//trapa

static void FASTCALL SH2cmpim(u16 instruction, u32 recompile_addr)
{
   s32 imm;
   s32 i = INSTRUCTION_CD(instruction);

   imm = (s32)(s8)i;

   jit.PushRel(offsetof(Sh2JitContext, r[0]));
   jit.PushCst(imm);
   jit.Cmp(Jitter::CONDITION_EQ);
   set_t();

   increment_pc();
   add_cycles(1);
}

void SH2alui_template(u16 instruction, int op)
{
   u32 imm = INSTRUCTION_CD(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[0]));
   jit.PushCst(imm);

   alu_select(op);

   jit.PullRel(offsetof(Sh2JitContext, r[0]));

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2andi(u16 instruction, u32 recompile_addr)
{
   SH2alui_template(instruction, ALU_AND);
}

static void FASTCALL SH2alum_template(u16 instruction, int alu_op)
{
   s32 source = INSTRUCTION_CD(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[0]));
   jit.PushRel(offsetof(Sh2JitContext, gbr));
   jit.Add();
   jit.PushTop();
   jit.Call(reinterpret_cast<void*>(&mapped_memory_read_byte), 1, Jitter::CJitter::RETURN_VALUE_32);

   jit.PushCst(source);

   alu_select(alu_op);

   jit.Call(reinterpret_cast<void*>(&mapped_memory_write_byte), 2, Jitter::CJitter::RETURN_VALUE_NONE);

   increment_pc();
   add_cycles(3);
}

static void FASTCALL SH2andm(u16 instruction, u32 recompile_addr)
{
   SH2alum_template(instruction, ALU_AND);
}

static void FASTCALL SH2ori(u16 instruction, u32 recompile_addr)
{
   SH2alui_template(instruction, ALU_OR);
}

static void FASTCALL SH2orm(u16 instruction, u32 recompile_addr)
{
   SH2alum_template(instruction, ALU_OR);
}

static void FASTCALL SH2tsti(u16 instruction, u32 recompile_addr)
{
   s32 i = INSTRUCTION_CD(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[0]));
   jit.PushCst(i);
   jit.And();
   set_t_if_zero();

   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2tstm(u16 instruction, u32 recompile_addr)
{
   s32 i = INSTRUCTION_CD(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[0]));
   jit.PushRel(offsetof(Sh2JitContext, gbr));
   jit.Add();
   jit.Call(reinterpret_cast<void*>(&mapped_memory_read_byte), 1, Jitter::CJitter::RETURN_VALUE_32);
   jit.PushCst(i);
   jit.And();
   set_t_if_zero();

   increment_pc();
   add_cycles(3);
}

static void FASTCALL SH2xori(u16 instruction, u32 recompile_addr)
{
   SH2alui_template(instruction, ALU_XOR);
}

static void FASTCALL SH2xorm(u16 instruction, u32 recompile_addr)
{
   SH2alum_template(instruction, ALU_XOR);
}

void decrement_r15()
{
   jit.PushRel(offsetof(Sh2JitContext, r[15]));
   jit.PushCst(4);
   jit.Sub();
   jit.PullRel(offsetof(Sh2JitContext, r[15]));
}

static void FASTCALL SH2trapa(u16 instruction, u32 recompile_addr)
{
   s32 imm = INSTRUCTION_CD(instruction);

   decrement_r15();

   jit.PushRel(offsetof(Sh2JitContext, r[15]));
   jit.PushRel(offsetof(Sh2JitContext, sr));
   jit.Call(reinterpret_cast<void*>(&mapped_memory_write_long), 2, Jitter::CJitter::RETURN_VALUE_NONE);

   decrement_r15();

   jit.PushRel(offsetof(Sh2JitContext, r[15]));
   jit.PushRel(offsetof(Sh2JitContext, pc));
   jit.PushCst(2);
   jit.Add();
   jit.Call(reinterpret_cast<void*>(&mapped_memory_write_long), 2, Jitter::CJitter::RETURN_VALUE_NONE);

   jit.PushRel(offsetof(Sh2JitContext, vbr));
   jit.PushCst(imm * 4);
   jit.Add();
   jit.Call(reinterpret_cast<void*>(&mapped_memory_read_long), 1, Jitter::CJitter::RETURN_VALUE_32);
   jit.PullRel(offsetof(Sh2JitContext, pc));

   add_cycles(8);

   basic_block = 1;
}

//////////////////////////////////////////////////////////////////////////////

//ni format
//mov imm, rn
//add imm, rn

static void FASTCALL SH2movi(u16 instruction, u32 recompile_addr)
{
   s32 i = INSTRUCTION_CD(instruction);
   s32 n = INSTRUCTION_B(instruction);

   jit.PushCst((s32)(s8)i);
   jit.PullRel(offsetof(Sh2JitContext, r[n]));
   increment_pc();
   add_cycles(1);
}

static void FASTCALL SH2addi(u16 instruction, u32 recompile_addr)
{
   s32 cd = (s32)(s8)INSTRUCTION_CD(instruction);
   s32 b = INSTRUCTION_B(instruction);

   jit.PushRel(offsetof(Sh2JitContext, r[b]));
   jit.PushCst(cd);
   jit.Add();
   jit.PullRel(offsetof(Sh2JitContext, r[b]));

   increment_pc();
   add_cycles(1);
}

static jit_opcode_func decode(enum SHMODELTYPE model, u16 instruction)
{
   switch (INSTRUCTION_A(instruction))
   {
   case 0:
      switch (INSTRUCTION_D(instruction))
      {
      case 2:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2stcsr;
         case 1: return &SH2stcgbr;
         case 2: return &SH2stcvbr;
         default: return &SH2undecoded;
         }

      case 3:
         if (model == SHMT_SH1)
            return &SH2undecoded;
         else
         {
            switch (INSTRUCTION_C(instruction))
            {
            case 0: return &SH2bsrf;
            case 2: return &SH2braf;
            default: return &SH2undecoded;
            }
         }
      case 4: return &SH2movbs0;
      case 5: return &SH2movws0;
      case 6: return &SH2movls0;
      case 7: return model == SHMT_SH1 ? &SH2undecoded : &SH2mull;
      case 8:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2clrt;
         case 1: return &SH2sett;
         case 2: return &SH2clrmac;
         default: return &SH2undecoded;
         }
      case 9:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2nop;
         case 1: return &SH2div0u;
         case 2: return &SH2movt;
         default: return &SH2undecoded;
         }
      case 10:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2stsmach;
         case 1: return &SH2stsmacl;
         case 2: return &SH2stspr;
         default: return &SH2undecoded;
         }
      case 11:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2rts;
         case 1: return &SH2sleep;
         case 2: return &SH2rte;
         default: return &SH2undecoded;
         }
      case 12: return &SH2movbl0;
      case 13: return &SH2movwl0;
      case 14: return &SH2movll0;
      case 15: return model == SHMT_SH1 ? &SH2undecoded : &SH2macl;
      default: return &SH2undecoded;
      }

   case 1: return &SH2movls4;
   case 2:
      switch (INSTRUCTION_D(instruction))
      {
      case 0: return &SH2movbs;
      case 1: return &SH2movws;
      case 2: return &SH2movls;
      case 4: return &SH2movbm;
      case 5: return &SH2movwm;
      case 6: return &SH2movlm;
      case 7: return &SH2div0s;
      case 8: return &SH2tst;
      case 9: return &SH2y_and;
      case 10: return &SH2y_xor;
      case 11: return &SH2y_or;
      case 12: return &SH2cmpstr;
      case 13: return &SH2xtrct;
      case 14: return &SH2mulu;
      case 15: return &SH2muls;
      default: return &SH2undecoded;
      }

   case 3:
      switch (INSTRUCTION_D(instruction))
      {
      case 0:  return &SH2cmpeq;
      case 2:  return &SH2cmphs;
      case 3:  return &SH2cmpge;
      case 4:  return &SH2div1;
      case 5:  return model == SHMT_SH1 ? &SH2undecoded : &SH2dmulu;
      case 6:  return &SH2cmphi;
      case 7:  return &SH2cmpgt;
      case 8:  return &SH2sub;
      case 10: return &SH2subc;
      case 11: return &SH2subv;
      case 12: return &SH2add;
      case 13: return model == SHMT_SH1 ? &SH2undecoded : &SH2dmuls;
      case 14: return &SH2addc;
      case 15: return &SH2addv;
      default: return &SH2undecoded;
      }

   case 4:
      switch (INSTRUCTION_D(instruction))
      {
      case 0:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2shll;
         case 1: return model == SHMT_SH1 ? &SH2undecoded : &SH2dt;
         case 2: return &SH2shal;
         default: return &SH2undecoded;
         }
      case 1:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2shlr;
         case 1: return &SH2cmppz;
         case 2: return &SH2shar;
         default: return &SH2undecoded;
         }
      case 2:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2stsmmach;
         case 1: return &SH2stsmmacl;
         case 2: return &SH2stsmpr;
         default: return &SH2undecoded;
         }
      case 3:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2stcmsr;
         case 1: return &SH2stcmgbr;
         case 2: return &SH2stcmvbr;
         default: return &SH2undecoded;
         }
      case 4:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2rotl;
         case 2: return &SH2rotcl;
         default: return &SH2undecoded;
         }
      case 5:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2rotr;
         case 1: return &SH2cmppl;
         case 2: return &SH2rotcr;
         default: return &SH2undecoded;
         }
      case 6:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2ldsmmach;
         case 1: return &SH2ldsmmacl;
         case 2: return &SH2ldsmpr;
         default: return &SH2undecoded;
         }
      case 7:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2ldcmsr;
         case 1: return &SH2ldcmgbr;
         case 2: return &SH2ldcmvbr;
         default: return &SH2undecoded;
         }
      case 8:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2shll2;
         case 1: return &SH2shll8;
         case 2: return &SH2shll16;
         default: return &SH2undecoded;
         }
      case 9:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2shlr2;
         case 1: return &SH2shlr8;
         case 2: return &SH2shlr16;
         default: return &SH2undecoded;
         }
      case 10:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2ldsmach;
         case 1: return &SH2ldsmacl;
         case 2: return &SH2ldspr;
         default: return &SH2undecoded;
         }
      case 11:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2jsr;
         case 1: return &SH2tas;
         case 2: return &SH2jmp;
         default: return &SH2undecoded;
         }
      case 14:
         switch (INSTRUCTION_C(instruction))
         {
         case 0: return &SH2ldcsr;
         case 1: return &SH2ldcgbr;
         case 2: return &SH2ldcvbr;
         default: return &SH2undecoded;
         }
      case 15: return &SH2macw;
      default: return &SH2undecoded;
      }
   case 5: return &SH2movll4;
   case 6:
      switch (INSTRUCTION_D(instruction))
      {
      case 0:  return &SH2movbl;
      case 1:  return &SH2movwl;
      case 2:  return &SH2movll;
      case 3:  return &SH2mov;
      case 4:  return &SH2movbp;
      case 5:  return &SH2movwp;
      case 6:  return &SH2movlp;
      case 7:  return &SH2y_not;
      case 8:  return &SH2swapb;
      case 9:  return &SH2swapw;
      case 10: return &SH2negc;
      case 11: return &SH2neg;
      case 12: return &SH2extub;
      case 13: return &SH2extuw;
      case 14: return &SH2extsb;
      case 15: return &SH2extsw;
      }

   case 7: return &SH2addi;
   case 8:
      switch (INSTRUCTION_B(instruction))
      {
      case 0:  return &SH2movbs4;
      case 1:  return &SH2movws4;
      case 4:  return &SH2movbl4;
      case 5:  return &SH2movwl4;
      case 8:  return &SH2cmpim;
      case 9:  return &SH2bt;
      case 11: return &SH2bf;
      case 13: return model == SHMT_SH1 ? &SH2undecoded : &SH2bts;
      case 15: return model == SHMT_SH1 ? &SH2undecoded : &SH2bfs;
      default: return &SH2undecoded;
      }
   case 9: return &SH2movwi;
   case 10: return &SH2bra;
   case 11: return &SH2bsr;
   case 12:
      switch (INSTRUCTION_B(instruction))
      {
      case 0:  return &SH2movbsg;
      case 1:  return &SH2movwsg;
      case 2:  return &SH2movlsg;
      case 3:  return &SH2trapa;
      case 4:  return &SH2movblg;
      case 5:  return &SH2movwlg;
      case 6:  return &SH2movllg;
      case 7:  return &SH2mova;
      case 8:  return &SH2tsti;
      case 9:  return &SH2andi;
      case 10: return &SH2xori;
      case 11: return &SH2ori;
      case 12: return &SH2tstm;
      case 13: return &SH2andm;
      case 14: return &SH2xorm;
      case 15: return &SH2orm;
      }

   case 13: return &SH2movli;
   case 14: return &SH2movi;
   default: return &SH2undecoded;
   }
}

//////////////////////////////////////////////////////////////////////////////\

u32 get_i(SH2_struct *context)
{
   return (context->jit.sr >> 4) & 0xf;
}

void set_i(SH2_struct *context, u32 data)
{
   context->jit.sr &= 0xffffff0f;
   context->jit.sr |= data << 4;
}

static INLINE void SH2HandleInterrupts(SH2_struct *context)
{
   if (context->NumberOfInterrupts != 0)
   {
      if (context->interrupts[context->NumberOfInterrupts-1].level > get_i((context)))
      {
         context->jit.r[15] -= 4;
         mapped_memory_write_long(context->jit.r[15], context->jit.sr);
         context->jit.r[15] -= 4;
         mapped_memory_write_long(context->jit.r[15], context->jit.pc);
         set_i(context, context->interrupts[context->NumberOfInterrupts-1].level);
         context->jit.pc = mapped_memory_read_long(context->jit.vbr + (context->interrupts[context->NumberOfInterrupts-1].vector << 2));
         context->NumberOfInterrupts--;
         context->isIdle = 0;
         context->isSleeping = 0;
      }
   }
}

void recompile_and_exec(SH2_struct *context)
{
   assert((context->jit.pc & 0xfff00000) == 0);

   if (code_blocks[context->jit.pc / 2].function.IsEmpty())
   {
      Framework::CMemStream stream;
      stream.Seek(0, Framework::STREAM_SEEK_DIRECTION::STREAM_SEEK_SET);
      jit.SetStream(&stream);
      jit.Begin();
      basic_block = 0;
      u32 current_pc = context->jit.pc;
      code_blocks[context->jit.pc / 2].start_pc = current_pc;
      for (;;)
      {
         u16 instr = context->instruction = ((fetchfunc *)context->fetchlist)[(current_pc >> 20) & 0x0FF](context, current_pc);
         jit_opcode_func func = decode(SHMT_SH1, instr);

         func(instr, current_pc);

         if (basic_block)
            break;

         current_pc += 2;
      }

      jit.End();

      code_blocks[context->jit.pc / 2].function = CMemoryFunction(stream.GetBuffer(), stream.GetSize());
      code_blocks[context->jit.pc / 2].end_pc = current_pc;

   }

   code_blocks[context->jit.pc / 2].function(&context->jit);
}

extern "C"
{
   FASTCALL void SH2JitExec(SH2_struct *context, u32 cycles)
   {
      SH2HandleInterrupts(context);

      current = context;

      while (context->jit.cycles < cycles)
      {
         recompile_and_exec(context);
      }

      if (UNLIKELY(context->jit.cycles < cycles))
         context->jit.cycles = 0;
      else
         context->jit.cycles -= cycles;
   }

   //////////////////////////////////////////////////////////////////////////////

   void SH2JitGetRegisters(SH2_struct *context, sh2regs_struct *regs)
   {
      for(int i = 0; i < 16; i++)
         regs->R[i] = context->jit.r[i];

      regs->GBR = context->jit.gbr;
      regs->MACH = context->jit.mach;
      regs->MACL = context->jit.macl;
      regs->PC = context->jit.pc;
      regs->PR = context->jit.pr;
      regs->SR.all = context->jit.sr;
      regs->VBR = context->jit.vbr;
   }

   //////////////////////////////////////////////////////////////////////////////

   u32 SH2JitGetGPR(SH2_struct *context, int num)
   {
      return context->jit.r[num];
   }

   //////////////////////////////////////////////////////////////////////////////

   u32 SH2JitGetSR(SH2_struct *context)
   {
      return context->jit.sr;
   }

   //////////////////////////////////////////////////////////////////////////////

   u32 SH2JitGetGBR(SH2_struct *context)
   {
      return context->jit.gbr;
   }

   //////////////////////////////////////////////////////////////////////////////

   u32 SH2JitGetVBR(SH2_struct *context)
   {
      return context->jit.vbr;
   }

   //////////////////////////////////////////////////////////////////////////////

   u32 SH2JitGetMACH(SH2_struct *context)
   {
      return context->jit.mach;
   }

   //////////////////////////////////////////////////////////////////////////////

   u32 SH2JitGetMACL(SH2_struct *context)
   {
      return context->jit.macl;
   }

   //////////////////////////////////////////////////////////////////////////////

   u32 SH2JitGetPR(SH2_struct *context)
   {
      return context->jit.pr;
   }

   //////////////////////////////////////////////////////////////////////////////

   u32 SH2JitGetPC(SH2_struct *context)
   {
      return context->jit.pc;
   }

   //////////////////////////////////////////////////////////////////////////////

   void SH2JitSetRegisters(SH2_struct *context, const sh2regs_struct *regs)
   {
      for (int i = 0; i < 16; i++)
         context->jit.r[i] = regs->R[i];

      context->jit.gbr = regs->GBR;
      context->jit.mach = regs->MACH;
      context->jit.macl = regs->MACL;
      context->jit.pc = regs->PC;
      context->jit.pr = regs->PR;
      context->jit.sr = regs->SR.all;
      context->jit.vbr = regs->VBR;
   }

   //////////////////////////////////////////////////////////////////////////////

   void SH2JitSetGPR(SH2_struct *context, int num, u32 value)
   {
      context->jit.r[num] = value;
   }

   //////////////////////////////////////////////////////////////////////////////

   void SH2JitSetSR(SH2_struct *context, u32 value)
   {
      context->jit.sr = value;
   }

   //////////////////////////////////////////////////////////////////////////////

   void SH2JitSetGBR(SH2_struct *context, u32 value)
   {
      context->jit.gbr = value;
   }

   //////////////////////////////////////////////////////////////////////////////

   void SH2JitSetVBR(SH2_struct *context, u32 value)
   {
      context->jit.vbr = value;
   }

   //////////////////////////////////////////////////////////////////////////////

   void SH2JitSetMACH(SH2_struct *context, u32 value)
   {
      context->jit.mach = value;
   }

   //////////////////////////////////////////////////////////////////////////////

   void SH2JitSetMACL(SH2_struct *context, u32 value)
   {
      context->jit.macl = value;
   }

   //////////////////////////////////////////////////////////////////////////////

   void SH2JitSetPR(SH2_struct *context, u32 value)
   {
      context->jit.pr = value;
   }

   //////////////////////////////////////////////////////////////////////////////

   void SH2JitSetPC(SH2_struct *context, u32 value)
   {
      context->jit.pc = value;
   }
}