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

extern "C"
{
#include "scu.h"
#include "scu_dsp_jit.h"
#include "sh2core.h"
}

#include "MemStream.h"
#include "MemoryFunction.h"
#include "Jitter_CodeGenFactory.h"
#include "Jitter.h"
#include "offsetof_def.h"

#define CONTROL_LE 0x00008000
#define CONTROL_EX 0x00010000
#define CONTROL_E  0x00040000
#define CONTROL_C  0x00100000
#define CONTROL_Z  0x00200000
#define CONTROL_S  0x00400000
#define CONTROL_T0 0x00800000

static Jitter::CJitter jit(Jitter::CreateCodeGen());

#define NUM_BLOCKS 256

struct ScuDspCodeBlock
{
   int dirty;
   CMemoryFunction function;
}scu_blocks[NUM_BLOCKS];

struct ScuDspContext
{
   u32 aluh;
   u32 alul;
   u32 ach;
   u32 acl;
   u32 ph;
   u32 pl;

   u32 control;
   u32 zero;//always zero, used in do_alu_op
   u64 temp;

   u32 lop;
   u32 top;

   u32 ct[4];
   u32 gen_increment[4];//emit ct incrementing code

   u32 program[256];
   u32 pc;
   u32 md[4][64];
   u32 jump_addr;
   u32 delayed;
   u32 rx;
   u32 ry;

   u32 mulh;
   u32 mull;
   u32 read_data;

   u32 ra0;
   u32 wa0;

   u32 data_ram_page;
   u32 data_ram_read_address;

   u32 timing;

   int need_recompile;
};

static struct ScuDspContext cxt;

void increment_ct(int which)
{
   jit.PushRel(offsetof(ScuDspContext, ct[which]));
   jit.PushCst(1);
   jit.Add();
   jit.PushCst(0x3f);
   jit.And();
   jit.PullRel(offsetof(ScuDspContext, ct[which]));
}

void do_readgensrc(u32 num, int need_increment[4])
{
   int md_bank = num & 3;
   switch (num)
   {
   case 0:
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
   case 7:
      jit.PushRelAddrRef(offsetof(ScuDspContext, md[md_bank]));
      jit.PushRel(offsetof(ScuDspContext, ct[md_bank]));

      jit.Shl(2);
      jit.AddRef();
      jit.LoadFromRef();
      jit.PullRel(offsetof(ScuDspContext, read_data));

      if (num & 4)
         need_increment[num & 3] = 1;
      break;
   case 9:
      jit.PushRel(offsetof(ScuDspContext, alul));
      jit.PullRel(offsetof(ScuDspContext, read_data));
      break;
   case 0xa:
      //return (u32)((alu.all & (u64)(0x0000ffffffff0000))  >> 16);
      jit.PushRel(offsetof(ScuDspContext, alul));
      jit.Srl(16);

      jit.PushRel(offsetof(ScuDspContext, aluh));
      jit.Shl(16);
      jit.Or();
      jit.PullRel(offsetof(ScuDspContext, read_data));
      break;
   default:
      jit.PushCst(0);
      jit.PullRel(offsetof(ScuDspContext, read_data));
      break;
   }
}

void sign_extend_32(Jitter::CJitter & jit, size_t src_offset_lo, size_t dst_offset_hi)
{
   //sign extend cmp result to fill upper 32 bits
   //if(src & 0x80000000)
   //  dest = 0xffffffff;
   jit.PushRel(src_offset_lo);
   jit.PushCst(0x80000000);
   jit.And();
   jit.PushCst(0x80000000);
   jit.Cmp(Jitter::CONDITION_EQ);
   jit.Shl(31);
   jit.Sra(31);
   jit.PullRel(dst_offset_hi);
}

//todo write hardware test, recompile
void do_ad2_flags(u64 input)
{
   u64 ac = cxt.acl | (u64)cxt.ach << 32;
   u64 p = cxt.pl | (u64)cxt.ph << 32;

   if (input == 0)
      cxt.control |= CONTROL_Z;
   else
      cxt.control &= ~CONTROL_Z;

   if (input & 0x800000000000)
      cxt.control |= CONTROL_S;
   else
      cxt.control &= ~CONTROL_S;

   if (((ac & 0xffffffffffff) + (p & 0xffffffffffff)) & (0x1000000000000))
      cxt.control |= CONTROL_C;
   else
      cxt.control &= ~CONTROL_C;
}

void do_zero_flag()
{
   //control &= ~CONTROL_Z;
   //control |= (alul == 0) << 21;
   jit.PushRel(offsetof(ScuDspContext, alul));
   jit.PushCst(0);
   jit.Cmp(Jitter::CONDITION_EQ);
   jit.Shl(21);
   jit.PushRel(offsetof(ScuDspContext, control));
   jit.PushCst(~CONTROL_Z);
   jit.And();
   jit.Or();
   jit.PullRel(offsetof(ScuDspContext, control));
}

void do_sign_flag()
{
   //control &= ~CONTROL_S;
   //control |= ((alul & 0x80000000) >> 9);
   jit.PushRel(offsetof(ScuDspContext, alul));
   jit.PushCst(0x80000000);
   jit.And();
   jit.Srl(9);
   jit.PushRel(offsetof(ScuDspContext, control));
   jit.PushCst(~CONTROL_S);
   jit.And();
   jit.Or();
   jit.PullRel(offsetof(ScuDspContext, control));
}

void do_flags(u32 instr)
{
   do_zero_flag();
   do_sign_flag();

   //not add or sub
   if (!(instr == 4 || instr == 5))
   {
      jit.PushRel(offsetof(ScuDspContext, control));
      jit.PushCst(~CONTROL_C);
      jit.And();
      jit.PullRel(offsetof(ScuDspContext, control));
   }
}

void do_alu_op(u32 instr)
{
   //add / sub
   if (instr == 4 || instr == 5)
   {
      //dummy store to prevent assert (codegen match problem)
      jit.PushCst64(1);
      jit.PullRel64(offsetof(ScuDspContext, temp));

      jit.PushRel(offsetof(ScuDspContext, acl));
      jit.PushRel(offsetof(ScuDspContext, zero));//MergeTo64 seems unable to use constants
      jit.MergeTo64();
      jit.PushRel(offsetof(ScuDspContext, pl));
      jit.PushRel(offsetof(ScuDspContext, zero));
      jit.MergeTo64();

      if (instr == 4)
         jit.Add64();
      else
         jit.Sub64();

      jit.PushTop();

      jit.ExtLow64();
      jit.PullRel(offsetof(ScuDspContext, alul));

      //temp = (aluh & 0x100000000) >> 32;
      //control &= ~CONTROL_C;
      //control |= temp << 20;
      jit.PushCst64(0x100000000);
      jit.And64();
      jit.ExtHigh64();

      jit.Shl(20);
      jit.PushRel(offsetof(ScuDspContext, control));
      jit.PushCst(~CONTROL_C);
      jit.And();
      jit.Or();
      jit.PullRel(offsetof(ScuDspContext, control));

      sign_extend_32(jit, offsetof(ScuDspContext, acl), offsetof(ScuDspContext, aluh));

      do_zero_flag();
      do_sign_flag();
   }
   else
   {
      //and, or, xor

      jit.PushRel(offsetof(ScuDspContext, acl));
      jit.PushRel(offsetof(ScuDspContext, pl));

      switch (instr)
      {
      case 1:
         jit.And();
         break;
      case 2:
         jit.Or();
         break;
      case 3:
         jit.Xor();
         break;
      default:
         break;
      }

      jit.PullRel(offsetof(ScuDspContext, alul));

      sign_extend_32(jit, offsetof(ScuDspContext, acl), offsetof(ScuDspContext, aluh));

      do_flags(instr);
   }
}

void do_shift_rotate_right_carry()
{
   //control &= ~CONTROL_C;
   //control |= (acl & 1) << 20;
   jit.PushRel(offsetof(ScuDspContext, acl));
   jit.PushCst(1);
   jit.And();
   jit.Shl(20);
   jit.PushRel(offsetof(ScuDspContext, control));
   jit.PushCst(~CONTROL_C);
   jit.And();
   jit.Or();
   jit.PullRel(offsetof(ScuDspContext, control));
}

void do_left_carry(int shift)
{
   //control &= ~CONTROL_C;
   //control |= ((acl >> shift) & 1) << 20;
   jit.PushRel(offsetof(ScuDspContext, acl));
   jit.Srl(shift);
   jit.PushCst(1);
   jit.And();
   jit.Shl(20);
   jit.PushRel(offsetof(ScuDspContext, control));
   jit.PushCst(~CONTROL_C);
   jit.And();
   jit.Or();
   jit.PullRel(offsetof(ScuDspContext, control));
}

void recompile_alu(Jitter::CJitter & jit, u32 instruction)
{
   u32 instr = instruction >> 26;
   switch (instr)
   {
   case 0x0: // NOP
      sign_extend_32(jit, offsetof(ScuDspContext, acl), offsetof(ScuDspContext, aluh));
      jit.PushRel(offsetof(ScuDspContext, acl));
      jit.PullRel(offsetof(ScuDspContext, alul));
      break;
   case 0x1: // AND
      do_alu_op(instr);
      break;
   case 0x2: // OR
      do_alu_op(instr);
      break;
   case 0x3: // XOR
      do_alu_op(instr);
      break;
   case 0x4: // ADD
      do_alu_op(instr);
      break;
   case 0x5: // SUB
      do_alu_op(instr);
      break;
   case 0x6: // AD2
      //temp = (acl | (ach << 32)) + (pl | (ph << 32))
      //aluh = temp >> 32
      //alul = temp & 0xffffffff
      jit.PushRel(offsetof(ScuDspContext, acl));
      jit.PushRel(offsetof(ScuDspContext, ach));
      jit.MergeTo64();
      jit.PushRel(offsetof(ScuDspContext, pl));
      jit.PushRel(offsetof(ScuDspContext, ph));
      jit.MergeTo64();
      jit.Add64();
      jit.PushTop();
      jit.PushTop();
      jit.ExtHigh64();
      jit.PullRel(offsetof(ScuDspContext, aluh));
      jit.ExtLow64();
      jit.PullRel(offsetof(ScuDspContext, alul));
      jit.Call(reinterpret_cast<void*>(&do_ad2_flags), 1, Jitter::CJitter::RETURN_VALUE_NONE);
      break;
   case 0x8: // SR
   case 0x9: // RR
      do_shift_rotate_right_carry();

      sign_extend_32(jit, offsetof(ScuDspContext, acl), offsetof(ScuDspContext, aluh));

      jit.PushRel(offsetof(ScuDspContext, acl));
      jit.Sra(1);
      jit.PullRel(offsetof(ScuDspContext, alul));

      if (instr == 9)//RR only
      {
         //alul |= (acl << 31) & 0x7fffffff;
         jit.PushRel(offsetof(ScuDspContext, acl));
         jit.Shl(31);
         jit.PushRel(offsetof(ScuDspContext, alul));
         jit.PushCst(0x7fffffff);
         jit.And();
         jit.Or();
         jit.PullRel(offsetof(ScuDspContext, alul));
      }

      do_zero_flag();
      do_sign_flag();
      break;
   case 0xA: // SL
   case 0xB: // RL
      do_left_carry(31);
      sign_extend_32(jit, offsetof(ScuDspContext, acl), offsetof(ScuDspContext, aluh));

      jit.PushRel(offsetof(ScuDspContext, acl));
      jit.Shl(1);
      jit.PullRel(offsetof(ScuDspContext, alul));

      if (instr == 0xb)//RL only
      {
         jit.PushRel(offsetof(ScuDspContext, acl));
         jit.Srl(31);
         jit.PushRel(offsetof(ScuDspContext, alul));
         jit.PushCst(0xfffffffe);
         jit.And();
         jit.Or();
         jit.PullRel(offsetof(ScuDspContext, alul));
      }

      do_zero_flag();
      do_sign_flag();
      break;
   case 0xF: // RL8
      do_left_carry(24);

      sign_extend_32(jit, offsetof(ScuDspContext, acl), offsetof(ScuDspContext, aluh));
      //alul = acl << 8;
      jit.PushRel(offsetof(ScuDspContext, acl));
      jit.Shl(8);
      jit.PullRel(offsetof(ScuDspContext, alul));

      //alul |= (acl >> 24) & 0xffffff00
      jit.PushRel(offsetof(ScuDspContext, acl));
      jit.Srl(24);
      jit.PushRel(offsetof(ScuDspContext, alul));
      jit.PushCst(0xffffff00);
      jit.And();
      jit.Or();
      jit.PullRel(offsetof(ScuDspContext, alul));

      do_zero_flag();
      do_sign_flag();
      break;
   default:
      break;
   }
}

void recompile_set_md(u32 bank, u32 data)
{
   jit.PushRelAddrRef(offsetof(ScuDspContext, md[bank]));

   jit.PushRel(offsetof(ScuDspContext, ct[bank]));

   //ct value is on stack, adjust for u32
   jit.Shl(2);

   jit.AddRef();

   jit.PushCst(data);

   jit.StoreAtRef();
}

void recompile_move_immediate(u32 instruction, int is_imm_d)
{
   int num = (instruction >> 26) & 0xF;
   int bank = num & 3;

   u32 data = instruction & 0x7ffff;

   if (instruction & 0x40000)
      data |= 0xFFF80000;

   if (is_imm_d)
   {
      //sign extend
      data = (instruction & 0x1FFFFFF);
      if (data & 0x1000000) data |= 0xfe000000;
   }

   switch (num)
   {
   case 0:
   case 1:
   case 2:
   case 3:
      recompile_set_md(bank, data);
      increment_ct(bank);
      break;
   case 4:
      jit.PushCst(data);
      jit.PullRel(offsetof(ScuDspContext, rx));
      break;
   case 5:
      jit.PushCst(data);
      jit.PullRel(offsetof(ScuDspContext, pl));

      jit.PushCst(0);
      jit.PullRel(offsetof(ScuDspContext, ph));

      //todo write a test to see if this gets sign extended
      //sign_extend_32(jit, offsetof(ScuDspContext, pl), offsetof(ScuDspContext, ph), 0x0000FFFF);
      break;
   case 6:
      jit.PushCst(data & 0x1FFFFFF);
      jit.PullRel(offsetof(ScuDspContext, ra0));
      break;
   case 7:
      jit.PushCst(data & 0x1FFFFFF);
      jit.PullRel(offsetof(ScuDspContext, wa0));
      break;
   case 0xa:
      jit.PushCst(data & 0xFFFF);
      jit.PullRel(offsetof(ScuDspContext, lop));
      break;
   case 0xc:
      jit.PushRel(offsetof(ScuDspContext, pc));
      jit.PushCst(1);
      jit.Add();
      jit.PushCst(0xff);
      jit.And();
      jit.PullRel(offsetof(ScuDspContext, top));

      jit.PushCst(data);
      jit.PullRel(offsetof(ScuDspContext, jump_addr));

      jit.PushCst(0);
      jit.PullRel(offsetof(ScuDspContext, delayed));
      break;
   default:
      break;
   }
}

void recompile_mvi_imm_d(u32 instruction, u32 val, enum Jitter::CONDITION cond)
{
   jit.PushRel(offsetof(ScuDspContext, control));
   jit.PushCst(val);
   jit.And();
   jit.PushCst(0);
   jit.BeginIf(cond);
   {
      recompile_move_immediate(instruction, 0);
   }
   jit.EndIf();
}

void recompile_z_s(u32 instruction, enum Jitter::CONDITION cond)
{
   jit.PushRel(offsetof(ScuDspContext, control));
   jit.PushCst(CONTROL_Z);
   jit.And();
   jit.PushCst(0);
   jit.BeginIf(cond);
   {
      recompile_move_immediate(instruction, 0);
   }
   jit.Else();
   {
      recompile_mvi_imm_d(instruction, CONTROL_S, cond);
   }
   jit.EndIf();
}

void recompile_load_immediate(u32 instruction)
{
   auto cond = Jitter::CONDITION_EQ;

   switch (((instruction >> 19) & 0x30))
   {
   case 0x00:
      //EQ
      break;
   case 0x10:
      //invalid
      return;
      break;
   case 0x20:
      cond = Jitter::CONDITION_NE;
      break;
   case 0x30:
      //invalid
      return;
      break;
   }

   switch ((instruction >> 19) & 0xF)
   {
   case 0x01: // MVI Imm,[d]NZ  
      recompile_mvi_imm_d(instruction, CONTROL_Z, cond);
      break;
   case 0x02: // MVI Imm,[d]NS
      recompile_mvi_imm_d(instruction, CONTROL_S, cond);
      break;
   case 0x03: // MVI Imm,[d]NZS
      recompile_z_s(instruction, cond);
      break;
   case 0x04: // MVI Imm,[d]NC
      recompile_mvi_imm_d(instruction, CONTROL_C, cond);
      break;
   case 0x08: // MVI Imm,[d]NT0
      recompile_mvi_imm_d(instruction, CONTROL_T0, cond);
      break;
   default:
      break;
   }
}

void recompile_jump_helper(u32 instruction)
{
   //jump_addr = instruction & 0xff;
   jit.PushCst(instruction & 0xff);
   jit.PullRel(offsetof(ScuDspContext, jump_addr));

   //delayed = 0;
   jit.PushCst(0);
   jit.PullRel(offsetof(ScuDspContext, delayed));
}

void check_jump_cond(u32 instruction, u32 val, enum Jitter::CONDITION cond)
{
   jit.PushRel(offsetof(ScuDspContext, control));
   jit.PushCst(val);
   jit.And();
   jit.PushCst(0);
   jit.BeginIf(cond);
   {
      recompile_jump_helper(instruction);
   }
   jit.EndIf();
}

void check_jmp_sz(u32 instruction, enum Jitter::CONDITION cond)
{
   jit.PushRel(offsetof(ScuDspContext, control));
   jit.PushCst(CONTROL_Z);
   jit.And();
   jit.PushCst(0);
   jit.BeginIf(cond);
   {
      recompile_jump_helper(instruction);
   }
   jit.Else();
   {
      check_jump_cond(instruction, CONTROL_S, cond);
   }
   jit.EndIf();
}

void recompile_jump(u32 instruction)
{
   u32 instr = ((instruction >> 19) & 0x7F);
   if (instr == 0)
   {
      recompile_jump_helper(instruction);
   }
   else if ((instr & 0xf0) == 0x40 || (instr & 0xf0) == 0x60)
   {
      auto cond = Jitter::CONDITION_EQ;

      if ((instr & 0xf0) == 0x60)
         cond = Jitter::CONDITION_NE;

      switch (instr & 0xf)
      {
      case 0x1: // JMP NZ, Imm
         check_jump_cond(instruction, CONTROL_Z, cond);
         break;
      case 0x2: // JMP NS, Imm
         check_jump_cond(instruction, CONTROL_S, cond);
         break;
      case 0x3: // JMP NZS, Imm
         check_jmp_sz(instruction, cond);
         break;
      case 0x4: // JMP NC, Imm
         check_jump_cond(instruction, CONTROL_C, cond);
         break;
      case 0x8: // JMP NT0, Imm
         check_jump_cond(instruction, CONTROL_T0, cond);
         break;
      }
   }

   assert(jit.IsStackEmpty());
}

void readgen_helper(u32 readgen, size_t off_hi, size_t off_lo, int need_increment[4])
{
   do_readgensrc(readgen, need_increment);

   jit.PushRel(offsetof(ScuDspContext, read_data));
   jit.PullRel(off_lo);

   sign_extend_32(jit, offsetof(ScuDspContext, read_data), off_hi);
}

void write_d1_bus(int num, u32 value, int is_const, size_t offset)
{
   int which = num - 0xc;
   int bank = num & 3;
   switch (num) {
   case 0x0:
   case 0x1:
   case 0x2:
   case 0x3:
      jit.PushRelAddrRef(offsetof(ScuDspContext, md[bank]));
      jit.PushRel(offsetof(ScuDspContext, ct[bank]));

      //ct value is on stack, adjust for u32
      jit.Shl(2);

      jit.AddRef();

      if (is_const)
      {
         jit.PushCst(value);
         jit.SignExt8();
      }
      else
         jit.PushRel(offset);

      jit.StoreAtRef();

      increment_ct(bank);
      break;
   case 0x4:
      if (is_const)
      {
         jit.PushCst(value);
         jit.SignExt8();
      }
      else
         jit.PushRel(offset);

      jit.PullRel(offsetof(ScuDspContext, rx));
      break;
   case 0x5:
      if (is_const)
      {
         jit.PushCst(value);
         jit.SignExt8();
      }
      else
         jit.PushRel(offset);

      jit.PullRel(offsetof(ScuDspContext, pl));

      sign_extend_32(jit, offsetof(ScuDspContext, pl), offsetof(ScuDspContext, ph));
      break;
   case 0x6:
      if (is_const)
      {
         jit.PushCst(value);
         jit.SignExt8();
      }
      else
         jit.PushRel(offset);
      jit.PullRel(offsetof(ScuDspContext, ra0));
      break;
   case 0x7:
      if (is_const)
      {
         jit.PushCst(value);
         jit.SignExt8();
      }
      else
         jit.PushRel(offset);
      jit.PullRel(offsetof(ScuDspContext, wa0));
      break;
   case 0xA:
      if (is_const)
      {
         jit.PushCst(value);
         jit.SignExt8();
         jit.PushCst(0xFFFF);
         jit.And();
      }
      else
      {
         jit.PushRel(offset);
         jit.PushCst(0xFFFF);
         jit.And();
      }

      jit.PullRel(offsetof(ScuDspContext, lop));
      break;
   case 0xB:

      if (is_const)
      {
         jit.PushCst(value);
         jit.SignExt8();
         jit.PushCst(0xFF);
         jit.And();
      }
      else
      {

         jit.PushRel(offset);
         jit.PushCst(0xFF);
         jit.And();
      }

      jit.PullRel(offsetof(ScuDspContext, top));
      break;
   case 0xC:
   case 0xD:
   case 0xE:
   case 0xF:
      //ct[which] = val;
      if (is_const)
      {
         jit.PushCst(value);
         jit.SignExt8();
         jit.PushCst(0x3F);
         jit.And();
      }
      else
      {
         jit.PushRel(offset);
         jit.PushCst(0x3F);
         jit.And();
      }

      //ct[which] = temp;
      jit.PullRel(offsetof(ScuDspContext, ct[which]));

      break;
   default: 
      break;
   }
}

void recompile_ct_increments(int need_increment[4])
{
   for (int i = 0; i < 4; i++)
   {
      if (need_increment[i])
      {
         increment_ct(i);
         need_increment[i] = 0;
      }
   }
}

void recompile_operation(u32 instruction)
{
   int need_increment[4] = { 0 };

   if ((instruction >> 23) & 0x4)
   {
      // MOV [s], X
      do_readgensrc((instruction >> 20) & 0x7, need_increment);

      jit.PushRel(offsetof(ScuDspContext, read_data));
      jit.PullRel(offsetof(ScuDspContext, rx));
   }
   //X bus
   switch ((instruction >> 23) & 0x3)
   {
   case 2: // MOV MUL, P
      jit.PushRel(offsetof(ScuDspContext, mulh));
      jit.PullRel(offsetof(ScuDspContext, ph));

      jit.PushRel(offsetof(ScuDspContext, mull));
      jit.PullRel(offsetof(ScuDspContext, pl));
      break;
   case 3: // MOV [s], P
      readgen_helper((instruction >> 20) & 0x7, offsetof(ScuDspContext, ph), offsetof(ScuDspContext, pl), need_increment);
      break;
   default: break;
   }

   // Y-bus
   if ((instruction >> 17) & 0x4)
   {
      // MOV [s], Y
      do_readgensrc((instruction >> 14) & 0x7, need_increment);

      jit.PushRel(offsetof(ScuDspContext, read_data));
      jit.PullRel(offsetof(ScuDspContext, ry));
   }

   switch ((instruction >> 17) & 0x3)
   {
   case 1: // CLR A
      jit.PushCst(0);
      jit.PullRel(offsetof(ScuDspContext, ach));

      jit.PushCst(0);
      jit.PullRel(offsetof(ScuDspContext, acl));
      break;
   case 2: // MOV ALU,A
      jit.PushRel(offsetof(ScuDspContext, aluh));
      jit.PullRel(offsetof(ScuDspContext, ach));

      jit.PushRel(offsetof(ScuDspContext, alul));
      jit.PullRel(offsetof(ScuDspContext, acl));
      break;
   case 3: // MOV [s],A
      readgen_helper((instruction >> 14) & 0x7, offsetof(ScuDspContext, ach), offsetof(ScuDspContext, acl), need_increment);
      break;
   default: break;
   }

   recompile_ct_increments(need_increment);

   // D1-bus
   switch ((instruction >> 12) & 0x3)
   {
   case 1: // MOV SImm,[d]
      write_d1_bus((instruction >> 8) & 0xF, instruction & 0xFF, 1, 0);
      break;
   case 3: // MOV [s],[d]
      do_readgensrc(instruction & 0xF, need_increment);
      write_d1_bus((instruction >> 8) & 0xF, instruction & 0xFF, 0, offsetof(ScuDspContext, read_data));
      recompile_ct_increments(need_increment);
      break;
   default: break;
   }
}

void recompile_multiply(u32 pc)
{
   //mul = rx * ry;
   jit.PushRel(offsetof(ScuDspContext, rx));
   jit.PushRel(offsetof(ScuDspContext, ry));
   jit.MultS();
   jit.PushTop();

   jit.ExtLow64();
   jit.PullRel(offsetof(ScuDspContext, mull));

   jit.ExtHigh64();
   jit.PullRel(offsetof(ScuDspContext, mulh));
}

//todo recompile dmas
void dsp_dma01(u32 inst)
{
   u32 imm = ((inst & 0xFF));
   u8  sel = ((inst >> 8) & 0x03);
   u8  add;
   u8  addr = cxt.ct[sel];
   u32 i;

   switch (((inst >> 15) & 0x07))
   {
   case 0: add = 0; break;
   case 1: add = 1; break;
   case 2: add = 2; break;
   case 3: add = 4; break;
   case 4: add = 8; break;
   case 5: add = 16; break;
   case 6: add = 32; break;
   case 7: add = 64; break;
   }

   if (add != 1)
   {
      for (i = 0; i < imm; i++)
      {
         cxt.md[sel][cxt.ct[sel]] = MappedMemoryReadLongNocache(MSH2, (cxt.ra0 << 2));
         cxt.ct[sel]++;
         cxt.ct[sel] &= 0x3F;
         cxt.ra0 += 1; // add?
      }
   }
   else {
      for (i = 0; i < imm; i++)
      {
         cxt.md[sel][cxt.ct[sel]] = MappedMemoryReadLongNocache(MSH2, (cxt.ra0 << 2));
         cxt.ct[sel]++;
         cxt.ct[sel] &= 0x3F;
         cxt.ra0 += 1;
      }
   }
   cxt.control &= ~CONTROL_T0;
}

void dsp_dma02(u32 inst)
{
   u32 imm = ((inst & 0xFF));
   u8  sel = ((inst >> 8) & 0x03);
   u8  addr = cxt.ct[sel];
   u8  add;
   u32 i;

   switch (((inst >> 15) & 0x07))
   {
   case 0: add = 0; break;
   case 1: add = 1; break;
   case 2: add = 2; break;
   case 3: add = 4; break;
   case 4: add = 8; break;
   case 5: add = 16; break;
   case 6: add = 32; break;
   case 7: add = 64; break;
   }

   if (add != 1)
   {
      for (i = 0; i < imm; i++)
      {
         u32 Val = cxt.md[sel][cxt.ct[sel]];
         u32 Adr = (cxt.wa0 << 2);
         //LOG("SCU DSP DMA02 D:%08x V:%08x", Adr, Val);
         MappedMemoryWriteLongNocache(MSH2, Adr, Val);
         cxt.ct[sel]++;
         cxt.wa0 += add >> 1;
         cxt.ct[sel] &= 0x3F;
      }
   }
   else {

      for (i = 0; i < imm; i++)
      {
         u32 Val = cxt.md[sel][cxt.ct[sel]];
         u32 Adr = (cxt.wa0 << 2);

         MappedMemoryWriteLongNocache(MSH2, Adr, Val);
         cxt.ct[sel]++;
         cxt.ct[sel] &= 0x3F;
         cxt.wa0 += 1;
      }

   }
   cxt.control &= ~CONTROL_T0;
}

void dsp_dma03(u32 inst)
{
   u32 Counter = 0;
   u32 i;
   int DestinationId;

   switch ((inst & 0x7))
   {
   case 0x00: Counter = cxt.md[0][cxt.ct[0]]; break;
   case 0x01: Counter = cxt.md[1][cxt.ct[1]]; break;
   case 0x02: Counter = cxt.md[2][cxt.ct[2]]; break;
   case 0x03: Counter = cxt.md[3][cxt.ct[3]]; break;
   case 0x04: Counter = cxt.md[0][cxt.ct[0]]; cxt.ct[0]++; break;
   case 0x05: Counter = cxt.md[1][cxt.ct[1]]; cxt.ct[1]++; break;
   case 0x06: Counter = cxt.md[2][cxt.ct[2]]; cxt.ct[2]++; break;
   case 0x07: Counter = cxt.md[3][cxt.ct[3]]; cxt.ct[3]++; break;
   }

   DestinationId = (inst >> 8) & 0x7;

   if (DestinationId > 3)
   {
      int incl = 1; //((cxt.inst >> 15) & 0x01);
      for (i = 0; i < Counter; i++)
      {
         u32 Adr = (cxt.ra0 << 2);
         cxt.program[i] = MappedMemoryReadLongNocache(MSH2, Adr);
         cxt.ra0 += incl;
      }
   }
   else {

      int incl = 1; //((cxt.inst >> 15) & 0x01);
      for (i = 0; i < Counter; i++)
      {
         u32 Adr = (cxt.ra0 << 2);

         cxt.md[DestinationId][cxt.ct[DestinationId]] = MappedMemoryReadLongNocache(MSH2, Adr);
         cxt.ct[DestinationId]++;
         cxt.ct[DestinationId] &= 0x3F;
         cxt.ra0 += incl;
      }
   }
   cxt.control &= ~CONTROL_T0;
}

void dsp_dma04(u32 inst)
{
   u32 Counter = 0;
   u32 add = 0;
   u32 sel = ((inst >> 8) & 0x03);
   u32 i;

   switch ((inst & 0x7))
   {
   case 0x00: Counter = cxt.md[0][cxt.ct[0]]; break;
   case 0x01: Counter = cxt.md[1][cxt.ct[1]]; break;
   case 0x02: Counter = cxt.md[2][cxt.ct[2]]; break;
   case 0x03: Counter = cxt.md[3][cxt.ct[3]]; break;
   case 0x04: Counter = cxt.md[0][cxt.ct[0]]; cxt.ct[0]++; break;
   case 0x05: Counter = cxt.md[1][cxt.ct[1]]; cxt.ct[1]++; break;
   case 0x06: Counter = cxt.md[2][cxt.ct[2]]; cxt.ct[2]++; break;
   case 0x07: Counter = cxt.md[3][cxt.ct[3]]; cxt.ct[3]++; break;
   }

   switch (((inst >> 15) & 0x07))
   {
   case 0: add = 0; break;
   case 1: add = 1; break;
   case 2: add = 2; break;
   case 3: add = 4; break;
   case 4: add = 8; break;
   case 5: add = 16; break;
   case 6: add = 32; break;
   case 7: add = 64; break;
   }

   for (i = 0; i < Counter; i++)
   {
      u32 Val = cxt.md[sel][cxt.ct[sel]];
      u32 Adr = (cxt.wa0 << 2);
      MappedMemoryWriteLongNocache(MSH2, Adr, Val);
      cxt.ct[sel]++;
      cxt.ct[sel] &= 0x3F;
      cxt.wa0 += 1;

   }
   cxt.control &= ~CONTROL_T0;
}

void dsp_dma05(u32 inst)
{
   u32 savera0 = cxt.ra0;
   dsp_dma01(inst);
   cxt.ra0 = savera0;
}

void dsp_dma06(u32 inst)
{
   u32 savewa0 = cxt.wa0;
   dsp_dma02(inst);
   cxt.wa0 = savewa0;
}

void dsp_dma07(u32 inst)
{
   u32 savera0 = cxt.ra0;
   dsp_dma03(inst);
   cxt.ra0 = savera0;
}

void dsp_dma08(u32 inst)
{
   u32 savewa0 = cxt.wa0;
   dsp_dma04(inst);
   cxt.wa0 = savewa0;
}

void recompile_dma(u32 instruction)
{
   if (((instruction >> 10) & 0x1F) == 0x00/*0x08*/)
   {
      jit.PushCst(instruction);
      jit.Call(reinterpret_cast<void*>(&dsp_dma01), 1, Jitter::CJitter::RETURN_VALUE_NONE);
   }
   else if (((instruction >> 10) & 0x1F) == 0x04)
   {
      jit.PushCst(instruction);
      jit.Call(reinterpret_cast<void*>(&dsp_dma02), 1, Jitter::CJitter::RETURN_VALUE_NONE);
   }
   else if (((instruction >> 11) & 0x0F) == 0x04)
   {
      jit.PushCst(instruction);
      jit.Call(reinterpret_cast<void*>(&dsp_dma03), 1, Jitter::CJitter::RETURN_VALUE_NONE);
   }
   else if (((instruction >> 10) & 0x1F) == 0x0C)
   {
      jit.PushCst(instruction);
      jit.Call(reinterpret_cast<void*>(&dsp_dma04), 1, Jitter::CJitter::RETURN_VALUE_NONE);
   }
   else if (((instruction >> 11) & 0x0F) == 0x08)
   {
      jit.PushCst(instruction);
      jit.Call(reinterpret_cast<void*>(&dsp_dma05), 1, Jitter::CJitter::RETURN_VALUE_NONE);
   }
   else if (((instruction >> 10) & 0x1F) == 0x14)
   {
      jit.PushCst(instruction);
      jit.Call(reinterpret_cast<void*>(&dsp_dma06), 1, Jitter::CJitter::RETURN_VALUE_NONE);
   }
   else if (((instruction >> 11) & 0x0F) == 0x0C)
   {
      jit.PushCst(instruction);
      jit.Call(reinterpret_cast<void*>(&dsp_dma07), 1, Jitter::CJitter::RETURN_VALUE_NONE);
   }
   else if (((instruction >> 10) & 0x1F) == 0x1C)
   {
      jit.PushCst(instruction);
      jit.Call(reinterpret_cast<void*>(&dsp_dma08), 1, Jitter::CJitter::RETURN_VALUE_NONE);
   }
}

void do_lps_btm(size_t pc_top)
{
   jit.PushRel(offsetof(ScuDspContext, lop));
   jit.PushCst(0);
   jit.BeginIf(Jitter::CONDITION_NE);
   {
      //jmp_addr = pc/top;
      jit.PushRel(pc_top);
      jit.PullRel(offsetof(ScuDspContext, jump_addr));

      //delayed = 0;
      jit.PushCst(0);
      jit.PullRel(offsetof(ScuDspContext, delayed));

      //lop--
      jit.PushRel(offsetof(ScuDspContext, lop));
      jit.PushCst(1);
      jit.Sub();
      jit.PullRel(offsetof(ScuDspContext, lop));
   }
   jit.EndIf();
}

void recompile_loop(u32 instruction)
{
   jit.PushCst(instruction);
   jit.PushCst(0x8000000);
   jit.And();
   jit.PushCst(0);
   jit.BeginIf(Jitter::CONDITION_NE);
   {
      do_lps_btm(offsetof(ScuDspContext, pc));
   }
   jit.Else();
   {
      do_lps_btm(offsetof(ScuDspContext, top));
   }
   jit.EndIf();
}

void recompile_end(u32 instruction)
{
   //control.ex = 0;
   jit.PushRel(offsetof(ScuDspContext, control));
   jit.PushCst(~CONTROL_EX);
   jit.And();
   jit.PullRel(offsetof(ScuDspContext, control));

   jit.PushCst(instruction);
   jit.PushCst(0x8000000);
   jit.And();
   jit.PushCst(0);
   jit.BeginIf(Jitter::CONDITION_NE);
   {
      //control.e = 1;
      jit.PushRel(offsetof(ScuDspContext, control));
      jit.PushCst(CONTROL_E);
      jit.Or();
      jit.PullRel(offsetof(ScuDspContext, control));

      jit.Call(reinterpret_cast<void*>(&ScuSendDSPEnd), 0, Jitter::CJitter::RETURN_VALUE_NONE);
   }
   jit.EndIf();

   //control &= ~0xFF;
   //control |= (pc + 1) & 0xFF;
   jit.PushRel(offsetof(ScuDspContext, control));
   jit.PushCst(~0xFF);
   jit.And();
   jit.PushRel(offsetof(ScuDspContext, pc));
   jit.PushCst(1);
   jit.Add();
   jit.PushCst(0xFF);
   jit.And();
   jit.Or();
   jit.PullRel(offsetof(ScuDspContext, control));

   //timing = 1; (finished)
   jit.PushCst(1);
   jit.PullRel(offsetof(ScuDspContext, timing));
}

void recompile_mv_imm_d(u32 instruction)
{
   recompile_move_immediate(instruction, 1);
}

void increment_pc()
{
   //pc = (pc + 1) & 0xff;
   jit.PushRel(offsetof(ScuDspContext, pc));
   jit.PushCst(1);
   jit.Add();
   jit.PushCst(0xFF);
   jit.And();
   jit.PullRel(offsetof(ScuDspContext, pc));
}

void handle_delayed_jumps()
{
   jit.PushRel(offsetof(ScuDspContext, jump_addr));
   jit.PushCst(0xFFFFFFFF);
   jit.BeginIf(Jitter::CONDITION_NE);
   {
      jit.PushRel(offsetof(ScuDspContext, delayed));
      jit.PushCst(0);
      jit.BeginIf(Jitter::CONDITION_NE);
      {
         jit.PushRel(offsetof(ScuDspContext, jump_addr));
         jit.PullRel(offsetof(ScuDspContext, pc));
         jit.PushCst(0xFFFFFFFF);
         jit.PullRel(offsetof(ScuDspContext, jump_addr));
      }
      jit.Else();
      {
         jit.PushCst(1);
         jit.PullRel(offsetof(ScuDspContext, delayed));
      }
      jit.EndIf();
   }
   jit.EndIf();
}

void recompile_instruction(u32 pc) 
{
   u32 instruction = cxt.program[pc];

   recompile_alu(jit, instruction);

   switch (instruction >> 30)
   {
   case 0x00: // Operation Commands
      recompile_operation(instruction);
      break;
   case 0x02: // Load Immediate Commands
      if ((instruction >> 25) & 1)
         recompile_load_immediate(instruction);
      else   
         recompile_mv_imm_d(instruction);// MVI Imm,[d]
      break;
   case 0x03: // Other
   {
      switch ((instruction >> 28) & 0xF)
      {
      case 0x0C: // DMA Commands
         recompile_dma(instruction);
         break;
      case 0x0D: // Jump Commands
         recompile_jump(instruction);
         break;
      case 0x0E: // Loop bottom Commands
         recompile_loop(instruction);
         break;
      case 0x0F: // End Commands
         recompile_end(instruction);
         break;
      default: break;
      }
      break;
   }
   default:
      break;
   }

   recompile_multiply(pc);

   increment_pc();

   handle_delayed_jumps();
}

void scu_dsp_jit_exec(u32 cycles)
{
   cxt.timing = cycles / 2;

   if (cxt.need_recompile)
   {
      //todo recompile into basic blocks and handle jumps/loops
      for (int i = 0; i < NUM_BLOCKS; i++)
      {
         if (!scu_blocks[i].dirty)
            continue;

         Framework::CMemStream stream;
         stream.Seek(0, Framework::STREAM_SEEK_DIRECTION::STREAM_SEEK_SET);
         jit.SetStream(&stream);
         jit.Begin();
         recompile_instruction(i);
         jit.End();

         scu_blocks[i].function = CMemoryFunction(stream.GetBuffer(), stream.GetSize());
         scu_blocks[i].dirty = 0;
      }
      cxt.need_recompile = 0;
   }

   if (cxt.control & CONTROL_EX)
   {
      while (cxt.timing > 0)
      {
         scu_blocks[cxt.pc].function(&cxt);
         cxt.timing--;
      }
   }
}

extern "C" void scu_dsp_jit_set_program(u32 val)
{
   cxt.need_recompile = 1;
   scu_blocks[cxt.pc].dirty = 1;

   cxt.program[cxt.pc] = val;
   cxt.pc++;
   cxt.pc &= 0xff;
   cxt.control = (cxt.control & 0xffffff00) | cxt.pc;
}

extern "C" void scu_dsp_jit_set_data_address(u32 val)
{
   cxt.data_ram_page = (val >> 6) & 3;
   cxt.data_ram_read_address = val & 0x3F;
}

extern "C" void scu_dsp_jit_set_data_ram_data(u32 val)
{
   if (!(cxt.control & CONTROL_EX)) 
   {
      cxt.md[cxt.data_ram_page][cxt.data_ram_read_address] = val;
      cxt.data_ram_read_address++;
      cxt.data_ram_read_address &= 0x3f;
   }
}

extern "C" void scu_dsp_jit_set_program_control(u32 val)
{
   cxt.control = (cxt.control & 0x00FC0000) | (val & 0x060380FF);

   if (cxt.control & CONTROL_LE)
      cxt.pc = cxt.control & 0xFF;
}

extern "C" u32 scu_dsp_jit_get_program_control()
{
   return cxt.control & 0x00FD00FF;
}

extern "C" u32 scu_dsp_jit_get_data_ram()
{
   if (!(cxt.control & CONTROL_EX))
      return cxt.md[cxt.data_ram_page][cxt.data_ram_read_address];
   else
      return 0;
}

extern "C" void scu_dsp_jit_init()
{
   memset(scu_blocks, 0, sizeof(struct ScuDspCodeBlock) * NUM_BLOCKS);
   memset(&cxt, 0, sizeof(struct ScuDspContext));

   cxt.jump_addr = 0xFFFFFFFF;

   scu_dsp_inf.get_data_ram = scu_dsp_jit_get_data_ram;
   scu_dsp_inf.get_program_control = scu_dsp_jit_get_program_control;
   scu_dsp_inf.set_data_address = scu_dsp_jit_set_data_address;
   scu_dsp_inf.set_data_ram_data = scu_dsp_jit_set_data_ram_data;
   scu_dsp_inf.set_program = scu_dsp_jit_set_program;
   scu_dsp_inf.set_program_control = scu_dsp_jit_set_program_control;
}