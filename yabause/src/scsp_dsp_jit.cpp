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
#include "scsp.h"
#include "scspdsp.h"
#include "scsp_dsp_jit.h"
}

#include "MemStream.h"
#include "MemoryFunction.h"
#include "Jitter_CodeGenFactory.h"
#include "Jitter.h"
#include "offsetof_def.h"

struct DspContext
{
   u32 need_recompile;

   u32 inputs;
   u32 y_reg;
   u32 shifted;
   u32 adrs_reg;
   u32 frc_reg;
   u32 y_sel_input;
   u32 x_sel_input;
   u32 temp_val;
   u32 multiply_val_lo;
   u32 multiply_val;
   u32 b_val;
   u32 sft_reg;
   u32 rbl;
   u32 rbp;
   u32 mdec_ct;

   u32 need_read;
   u32 need_write;
   u32 read_data;
   u32 write_data;
   u32 io_addr;
   u32 addr;

   u32 coef[64];
   u32 madrs[32];
   u64 mpro[128];
   s32 temp[128];
   s32 mems[32];
   s32 mixs[16];
   u32 efreg[16];
   s32 exts[2];
};

Jitter::CJitter jit(Jitter::CreateCodeGen());
static struct DspContext cxt;

#define BLOCK_SIZE 4 //number of dsp instructions per block
#define NUM_BLOCKS (128 / BLOCK_SIZE)

struct DspCodeBlock
{
   int dirty;
   CMemoryFunction function;
}blocks[NUM_BLOCKS];

int shifter_is_used(ScspDspInstruction instruction)
{
   //shifter result only used in these conditions
   if (instruction.part.ewt ||
      instruction.part.twt ||
      (instruction.part.adrl == 3) ||
      instruction.part.frcl ||
      instruction.part.mwt)
   {
      return 1;
   }

   return 0;
}

int temp_is_used(ScspDspInstruction instruction)
{
   return instruction.part.xsel == 0 || instruction.part.bsel == 0;
}

void op1(Jitter::CJitter& jit, ScspDspInstruction instruction)
{
   if (instruction.part.xsel == 1 || instruction.part.yrl || instruction.part.adrl != 3)//optimization, only calculate inputs if necessary
   {
      if (instruction.part.ira <= 0x1f)
      {
         jit.PushRel(offsetof(DspContext, mems[instruction.part.ira & 0x1f]));
      }
      else if (instruction.part.ira >= 0x20 && instruction.part.ira <= 0x2f)
      {
         jit.PushRel(offsetof(DspContext, mixs[instruction.part.ira - 0x20]));
         jit.Shl(4);
      }
      else
      {
         jit.PushRel(offsetof(DspContext, exts[(instruction.part.ira - 0x30) & 1]));
         jit.Shl(8);
      }

      //sign extend inputs
      jit.Shl(8);
      jit.Sra(8);
      jit.PullRel(offsetof(DspContext, inputs));
   }

   if (temp_is_used(instruction))//optimization, emit only when used
   {
      //temp_val = sign_extend(temp[addr])
      jit.PushRelAddrRef(offsetof(DspContext, temp));

      //addr = (instruction.part.tra + mdec_ct) & 0x7f
      jit.PushCst(instruction.part.tra);
      jit.PushRel(offsetof(DspContext, mdec_ct));
      jit.Add();
      jit.PushCst(0x7F);
      jit.And();

      jit.Shl(2);
      jit.AddRef();
      jit.LoadFromRef();

      //sign extend
      jit.Shl(8);
      jit.Sra(8);

      jit.PullRel(offsetof(DspContext, temp_val));
   }

   switch (instruction.part.ysel)
   {
   case 0:
      jit.PushRel(offsetof(DspContext, frc_reg));
      break;
   case 1:
      jit.PushRel(offsetof(DspContext, coef[instruction.part.coef]));
      jit.PushCst(0xFFFF);
      jit.And();
      break;
   case 2:
      jit.PushRel(offsetof(DspContext, y_reg));
      jit.PushCst(11);
      jit.Srl();
      jit.PushCst(0x1FFF);
      jit.And();
      break;
   case 3:
      //(y_reg >> 4) & 0x0FFF
      jit.PushRel(offsetof(DspContext, y_reg));
      jit.Srl(4);
      jit.PushCst(0xFFF);
      jit.And();
      break;
   }

   jit.PullRel(offsetof(DspContext, y_sel_input));

   if (!instruction.part.xsel)
      jit.PushRel(offsetof(DspContext, temp_val));
   else
      jit.PushRel(offsetof(DspContext, inputs));

   jit.PullRel(offsetof(DspContext, x_sel_input));

   if (instruction.part.yrl)
   {
      //y_reg = inputs & 0xFFFFFF;
      jit.PushRel(offsetof(DspContext, inputs));
      jit.PushCst(0xFFFFFF);
      jit.And();
      jit.PullRel(offsetof(DspContext, y_reg));
   }

   if (!shifter_is_used(instruction))//optimization, only calculate shifted value when it will be used
      return;

   //sign extend
   jit.PushRel(offsetof(DspContext, sft_reg));
   jit.Shl(6);
   jit.Sra(6);

   if ((instruction.part.shift == 1) || (instruction.part.shift == 2))
      jit.Shl(1);

   jit.PullRel(offsetof(DspContext, shifted));

   //saturate
   if (!(instruction.part.shift & 2))
   {
      //if (shifted > 0x7FFFFF)
      jit.PushRel(offsetof(DspContext, shifted));
      jit.PushCst(0x7FFFFF);
      jit.BeginIf(Jitter::CONDITION_GT);
      {
         //shifted = 0x7FFFFF;
         jit.PushCst(0x7FFFFF);
         jit.PullRel(offsetof(DspContext, shifted));
      }
      jit.Else();
      {
         //else if (shifted < 0xff800000)
         jit.PushRel(offsetof(DspContext, shifted));
         jit.PushCst(0xFF800000);
         jit.BeginIf(Jitter::CONDITION_LT);
         {
            //shifted = 0x800000;
            jit.PushCst(0x800000);
            jit.PullRel(offsetof(DspContext, shifted));
         }
         jit.EndIf();
      }
      jit.EndIf();
   }
   //shifted &= 0xFFFFFF;
   jit.PushRel(offsetof(DspContext, shifted));
   jit.PushCst(0xFFFFFF);
   jit.And();
   jit.PullRel(offsetof(DspContext, shifted));
}

void op2(Jitter::CJitter& jit, ScspDspInstruction instruction)
{
   if (instruction.part.ewt)
   {
      //efreg[ewa] = (shifted >> 8);
      jit.PushRel(offsetof(DspContext, shifted));
      jit.Srl(8);
      jit.PullRel(offsetof(DspContext, efreg[instruction.part.ewa]));
   }

   if (instruction.part.twt)//temp[((instruction.part.twa) + mdec_ct) & 0x7F] = shifted
   {
      jit.PushRelAddrRef(offsetof(DspContext, temp));

      //addr = ((instruction.part.twa) + mdec_ct) & 0x7F;
      jit.PushCst(instruction.part.twa);
      jit.PushRel(offsetof(DspContext, mdec_ct));
      jit.Add();
      jit.PushCst(0x7F);
      jit.And();
      jit.Shl(2);
      jit.AddRef();

      //store shifted at temp[addr]
      jit.PushRel(offsetof(DspContext, shifted));
      jit.StoreAtRef();
   }

   if (instruction.part.adrl)
   {
      switch (instruction.part.shift)
      {
      case 0:
      case 1:
      case 2:
         //arrs_reg = (inputs >> 16) & 0xFFF
         jit.PushRel(offsetof(DspContext, inputs));
         jit.PushCst(16);
         jit.Srl();
         jit.PushCst(0xFFF);
         jit.And();
         break;
      case 3:
         //adrs_reg = shifted >> 12
         jit.PushRel(offsetof(DspContext, shifted));
         jit.PushCst(12);
         jit.Srl();
         break;
      }

      jit.PullRel(offsetof(DspContext, adrs_reg));
   }

   if (instruction.part.frcl)
   {
      switch (instruction.part.shift)
      {
      case 0:
      case 1:
      case 2:
         //frc_reg = shifted >> 11
         jit.PushRel(offsetof(DspContext, shifted));
         jit.PushCst(11);
         jit.Srl();
         break;
      case 3:
         //frc_reg = shifted & 0xFFF
         jit.PushRel(offsetof(DspContext, shifted));
         jit.PushCst(0xFFF);
         jit.And();
         break;
      }

      jit.PullRel(offsetof(DspContext, frc_reg));
   }
}

void op3(Jitter::CJitter& jit, ScspDspInstruction instruction, int step)
{
   //product = (ysel_input_sign_extended * xsel_input) >> 12 (64 bit multiply)
   jit.PushRel(offsetof(DspContext, y_sel_input));
   jit.Shl(19);//sign extend
   jit.Sra(19);
   jit.PushRel(offsetof(DspContext, x_sel_input));
   jit.MultS();
   jit.PushTop();//make a second copy so we can extract the upper and lower part

   //shift low part into place, mask it and store
   jit.ExtLow64();
   jit.Srl(12);
   jit.PushCst(0x000fffff);
   jit.And();
   jit.PullRel(offsetof(DspContext, multiply_val_lo));

   //the PushTop value is now used
   //shift, mask
   jit.ExtHigh64();
   jit.Shl(20);
   jit.PushCst(0xfff00000);
   jit.And();

   //product_hi is on the stack already, load lo and OR them together
   jit.PushRel(offsetof(DspContext, multiply_val_lo));
   jit.Or();
   jit.PullRel(offsetof(DspContext, multiply_val));

   if (!instruction.part.bsel)
      jit.PushRel(offsetof(DspContext, temp_val));
   else
      jit.PushRel(offsetof(DspContext, sft_reg));

   jit.PullRel(offsetof(DspContext, b_val));

   if (instruction.part.negb)
   {
      //b_val = -b_val; aka (~b_val) - 1
      jit.PushRel(offsetof(DspContext, b_val));
      jit.Not();
      jit.PushCst(1);
      jit.Add();
      jit.PullRel(offsetof(DspContext, b_val));
   }

   if (instruction.part.zero)
   {
      //b_val = 0;
      jit.PushCst(0);
      jit.PullRel(offsetof(DspContext, b_val));
   }

   //sft_reg = (multiply_val + b_val) & 0x3FFFFFF;
   jit.PushRel(offsetof(DspContext, multiply_val));
   jit.PushRel(offsetof(DspContext, b_val));
   jit.Add();
   jit.PushCst(0x3FFFFFF);
   jit.And();
   jit.PullRel(offsetof(DspContext, sft_reg));

   if (instruction.part.iwt)
   {
      jit.PushRel(offsetof(DspContext, read_data));
      jit.PullRel(offsetof(DspContext, mems[instruction.part.iwa]));
   }
}

static void write_word(const u32 addr, const u32 data)
{
   if (addr >= 0x00040000)
      assert(0);

   u16 * ram = (u16*)SoundRam;
   ram[addr] = data;
}

static u32 read_word(const u32 addr)
{
   if (addr >= 0x00040000)
      assert(0);

   u16 * ram = (u16*)SoundRam;
   return ram[addr];
}

//not sign extended
s32 float_to_int_jit(u32 f_val)
{
   u32 sign = (f_val >> 15) & 1;
   u32 sign_inverse = (!sign) & 1;
   u32 exponent = (f_val >> 11) & 0xf;
   u32 mantissa = f_val & 0x7FF;

   s32 ret_val = sign << 31;

   if (exponent > 11)
   {
      exponent = 11;
      ret_val |= (sign << 30);
   }
   else
      ret_val |= (sign_inverse << 30);

   ret_val |= mantissa << 19;

   ret_val = ret_val >> (exponent + (1 << 3));

   return ret_val & 0xffffff;
}


void op4(Jitter::CJitter& jit, ScspDspInstruction instruction)
{
   jit.PushRel(offsetof(DspContext, need_read));
   jit.PushCst(0);
   jit.BeginIf(Jitter::CONDITION_NE);
   {

      jit.PushRel(offsetof(DspContext, io_addr));
      jit.PushCst(0x40000);//check for out of bounds accesses
      jit.And();
      jit.PushCst(0);
      jit.BeginIf(Jitter::CONDITION_EQ);
      {
         jit.PushRel(offsetof(DspContext, io_addr));
         jit.Call(reinterpret_cast<void*>(&read_word), 1, Jitter::CJitter::RETURN_VALUE_32);
         jit.PullRel(offsetof(DspContext, read_data));
      }
      jit.Else();
      {
         jit.PushCst(0);
         jit.PullRel(offsetof(DspContext, read_data));
      }
      jit.EndIf();

      jit.PushRel(offsetof(DspContext, need_read));
      jit.PushCst(2);
      jit.BeginIf(Jitter::CONDITION_EQ);
      {
         //the read value was nofl
         jit.PushRel(offsetof(DspContext, read_data));
         jit.Shl(8);
         jit.PullRel(offsetof(DspContext, read_data));
      }
      jit.Else();
      {
         //need conversion
         jit.PushRel(offsetof(DspContext, read_data));
         jit.Call(reinterpret_cast<void*>(&float_to_int_jit), 1, Jitter::CJitter::RETURN_VALUE_32);
         jit.PullRel(offsetof(DspContext, read_data));
      }
      jit.EndIf();

      jit.PushCst(0);
      jit.PullRel(offsetof(DspContext, need_read));
   }
   jit.Else();
   {
      jit.PushRel(offsetof(DspContext, need_write));
      jit.PushCst(0);
      jit.BeginIf(Jitter::CONDITION_NE);
      {
         jit.PushRel(offsetof(DspContext, io_addr));
         jit.PushCst(0x40000);//check for out of bounds accesses
         jit.And();
         jit.PushCst(0);
         jit.BeginIf(Jitter::CONDITION_EQ);
         {
            jit.PushRel(offsetof(DspContext, io_addr));
            jit.PushRel(offsetof(DspContext, write_data));
            jit.Call(reinterpret_cast<void*>(&write_word), 2, Jitter::CJitter::RETURN_VALUE_NONE);
         }
         jit.EndIf();

         jit.PushCst(0);
         jit.PullRel(offsetof(DspContext, need_write));
      }
      jit.EndIf();
   }
   jit.EndIf();
}

void op5(Jitter::CJitter& jit, ScspDspInstruction instruction)
{
   //addr = madrs[masa];
   jit.PushRel(offsetof(DspContext, madrs[instruction.part.masa]));
   
   jit.PushCst(0xFFFF);
   jit.And();

   if (instruction.part.nxadr)//addr += nxaddr;
   {
      jit.PushCst(1);
      jit.Add();
   }

   jit.PullRel(offsetof(DspContext, addr));

   if (instruction.part.adreb)
   {
      //addr += sign_extend(adrs_reg)
      jit.PushRel(offsetof(DspContext, adrs_reg));
      jit.PushCst(0xFFFF);
      jit.And();
      jit.Shl(20);
      jit.Sra(20);
      jit.PushRel(offsetof(DspContext, addr));
      jit.Add();
      jit.PullRel(offsetof(DspContext, addr));
   }

   if (!instruction.part.table)
   {
      // addr += mdec_ct
      jit.PushRel(offsetof(DspContext, addr));
      jit.PushRel(offsetof(DspContext, mdec_ct));
      jit.Add();

      //addr &= (0x2000 << rbl) - 1;
      jit.PushCst(0x2000);
      jit.PushRel(offsetof(DspContext, rbl));
      jit.Shl();
      jit.PushCst(1);
      jit.Sub();

      jit.And();
      jit.PullRel(offsetof(DspContext, addr));
   }

   //io_addr = (addr + (rbp << 12)) & 0x7FFFF;
   jit.PushRel(offsetof(DspContext, addr));
   jit.PushCst(0xFFFF);
   jit.And();
   jit.PushRel(offsetof(DspContext, rbp));
   jit.Shl(12);

   jit.Add();
   jit.PushCst(0x7FFFF);
   jit.And();
   jit.PullRel(offsetof(DspContext, io_addr));

   if (instruction.part.mrd)
   {
      int nofl = (instruction.all >> 8) & 1;
      jit.PushCst(nofl + 1);
      jit.PullRel(offsetof(DspContext, need_read));
   }

   if (instruction.part.mwt)
   {
      jit.PushCst(1);
      jit.PullRel(offsetof(DspContext, need_write));

      int nofl = (instruction.all >> 8) & 1;

      if (nofl)
      {
         jit.PushRel(offsetof(DspContext, shifted));
         jit.Srl(8);
         jit.PullRel(offsetof(DspContext, write_data));
      }
      else
      {
         jit.PushRel(offsetof(DspContext, shifted));
         jit.Call(reinterpret_cast<void*>(&int_to_float), 1, Jitter::CJitter::RETURN_VALUE_32);
         jit.PullRel(offsetof(DspContext, write_data));
      }
   }
}

extern "C" void scsp_dsp_jit_exec()
{
   //skip instructions == 0 at the end of programs
   //not technically correct but doesn't seem to affect anything
   int use_last_step_optimization = 1;

   if (cxt.need_recompile)
   {
      int last_block = NUM_BLOCKS;

      if (use_last_step_optimization)
      {
         int last_step = 0;

         for (int i = 127; i >= 0; i--)
         {
            if (cxt.mpro[i] != 0)
            {
               last_step = i;
               break;
            }
         }

         int last_block = (last_step / BLOCK_SIZE) + 1;

         if (last_block > NUM_BLOCKS)
            last_block = NUM_BLOCKS;

         //we have to clear the dirty bit as well for blocks that don't get recompiled
         for (int block_num = last_block; block_num < NUM_BLOCKS; block_num++)
            blocks[block_num].dirty = 0;
      }

      int size = 0;
      for (int block_num = 0; block_num < last_block; block_num++)
      {
         if (!blocks[block_num].dirty)//recompile not necessary for this block
            continue;

         Framework::CMemStream stream;
         stream.Seek(0, Framework::STREAM_SEEK_DIRECTION::STREAM_SEEK_SET);
         jit.SetStream(&stream);
         jit.Begin();
         for (int step = block_num * BLOCK_SIZE; step < (block_num + 1) * BLOCK_SIZE; step++)
         {
            ScspDspInstruction instr;
            instr.all = cxt.mpro[step];
            op1(jit, instr);
            assert(jit.IsStackEmpty());
            op2(jit, instr);
            assert(jit.IsStackEmpty());
            op3(jit, instr, step);
            assert(jit.IsStackEmpty());                            
            op4(jit, instr);
            assert(jit.IsStackEmpty());
            op5(jit, instr);
            assert(jit.IsStackEmpty());
         }
         jit.End();
         blocks[block_num].function = CMemoryFunction(stream.GetBuffer(), stream.GetSize());
         blocks[block_num].dirty = 0;
      }
      cxt.need_recompile = 0;
   }

   //execute each block
   for (int block_num = 0; block_num < NUM_BLOCKS; block_num++)
   {
      if (!blocks[block_num].function.IsEmpty())
         blocks[block_num].function(&cxt);
   }

   if (!cxt.mdec_ct)
      cxt.mdec_ct = (0x2000 << cxt.rbl);

   cxt.mdec_ct--;

   for (int i = 0; i < 16; i++)
      cxt.mixs[i] = 0;
}

extern "C" void jit_set_rbl_rbp(u32 rbl, u32 rbp)
{
   cxt.rbl = rbl;
   cxt.rbp = rbp;
}

extern "C" void jit_set_exts(u32 l, u32 r)
{
   cxt.exts[0] = l;
   cxt.exts[1] = r;
}

extern "C" void jit_set_mixs(u32 i, u32 data)
{
   cxt.mixs[i] += data << 4;
}

extern "C" u32 jit_get_effect_out(int i)
{
   if (i < 16)
      return cxt.efreg[i];
   else if (i == 16)
      return cxt.exts[0];
   else if (i == 17)
      return cxt.exts[1];
  
   return 0;
}

extern "C" void scsp_dsp_jit_need_recompile()
{
   cxt.need_recompile = 1;
}

extern "C" void jit_set_mpro(u64 input, u32 addr)
{
   cxt.mpro[addr] = input;
   blocks[addr / BLOCK_SIZE].dirty = 1;
}

extern "C" void jit_set_coef(u32 input, u32 addr)
{
   cxt.coef[addr] = input;
}

extern "C" void jit_set_madrs(u32 input, u32 addr)
{
   cxt.madrs[addr] = input;
}

u32 jit_get_coef(u32 addr)
{
   return cxt.coef[addr];
}

u32 jit_get_exts(u32 addr)
{
   return cxt.exts[addr];
}

u32 jit_get_madrs(u32 addr)
{
   return cxt.madrs[addr];
}

u32 jit_get_mems(u32 addr)
{
   return cxt.mems[addr];
}

u64 jit_get_mpro(u32 addr)
{
   return cxt.mpro[addr];
}

extern "C" void scsp_dsp_jit_init()
{
   memset(&cxt, 0, sizeof(struct DspContext));
   memset(&blocks, 0, sizeof(struct DspCodeBlock) * NUM_BLOCKS);

   dsp_inf.get_effect_out = jit_get_effect_out;
   dsp_inf.set_coef = jit_set_coef;
   dsp_inf.set_exts = jit_set_exts;
   dsp_inf.set_madrs = jit_set_madrs;
   dsp_inf.set_mixs = jit_set_mixs;
   dsp_inf.set_mpro = jit_set_mpro;
   dsp_inf.set_rbl_rbp = jit_set_rbl_rbp;
   dsp_inf.get_coef = jit_get_coef;
   dsp_inf.get_exts = jit_get_exts;
   dsp_inf.get_madrs = jit_get_madrs;
   dsp_inf.get_mems = jit_get_mems;
   dsp_inf.get_mpro = jit_get_mpro;
   dsp_inf.exec = scsp_dsp_jit_exec;
}