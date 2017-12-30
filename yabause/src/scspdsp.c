/*  Copyright 2015 Theo Berkau

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

#include "scsp.h"
#include "scspdsp.h"

s32 float_to_int(u16 f_val);
u16 int_to_float(u32 i_val);

//saturate 24 bit signed integer
static INLINE s32 saturate_24(s32 value)
{
   if (value > 8388607)
      value = 8388607;

   if (value < (-8388608))
      value = (-8388608);

   return value;
}


#define sign_x_to_s32(_bits, _value) (((int)((u32)(_value) << (32 - _bits))) >> (32 - _bits))
#define min(a,b) (a<b)?a:b

static INLINE unsigned clz(u32 v)
{
#if defined(__GNUC__) || defined(__clang__) || defined(__ICC) || defined(__INTEL_COMPILER)
  return __builtin_clz(v);
#elif defined(_MSC_VER)
  unsigned long idx;

  _BitScanReverse(&idx, v);

  return 31 ^ idx;
#else
  unsigned ret = 0;
  unsigned tmp;

  tmp = !(v & 0xFFFF0000) << 4; v <<= tmp; ret += tmp;
  tmp = !(v & 0xFF000000) << 3; v <<= tmp; ret += tmp;
  tmp = !(v & 0xF0000000) << 2; v <<= tmp; ret += tmp;
  tmp = !(v & 0xC0000000) << 1; v <<= tmp; ret += tmp;
  tmp = !(v & 0x80000000) << 0;            ret += tmp;

  return(ret);
#endif
}

void ScspDspExec(ScspDsp* dsp, int addr, u8 * sound_ram)
{
  u16* sound_ram_16 = (u16*)sound_ram;
  u64 mul_temp = 0;
  int nofl = 0;
  u32 x_temp = 0;
  s32 y_extended = 0;
  union ScspDspInstruction inst;
  u32 address = 0;
  s32 shift_temp = 0;

  inst.all = scsp_dsp.mpro[addr];

  const unsigned TEMPWriteAddr = (inst.part.twa + dsp->mdec_ct) & 0x7F;
  const unsigned TEMPReadAddr = (inst.part.tra + dsp->mdec_ct) & 0x7F;

  if (inst.part.ira & 0x20) {
    if (inst.part.ira & 0x10) {
      if (!(inst.part.ira & 0xE))
        dsp->inputs = dsp->exts[inst.part.ira & 0x1] << 8;
    }else{
      dsp->inputs = dsp->mixs[inst.part.ira & 0xF] << 4;
    }
  }else{
    dsp->inputs = dsp->mems[inst.part.ira & 0x1F];
  }

  const int INPUTS = sign_x_to_s32(24, dsp->inputs);
  const int TEMP = sign_x_to_s32(24, dsp->temp[TEMPReadAddr]);
  const int X_SEL_Inputs[2] = { TEMP, INPUTS };
  const u16 Y_SEL_Inputs[4] = { 
    dsp->frc_reg, dsp->coef[inst.part.coef],
    (u16)((dsp->y_reg >> 11) & 0x1FFF), 
    (u16)((dsp->y_reg >> 4) & 0x0FFF) 
  };
  const u32 SGA_Inputs[2] = { (u32)TEMP, dsp->shift_reg }; // ToDO:?

  if (inst.part.yrl) {
    dsp->y_reg = INPUTS & 0xFFFFFF;
  }

  int ShifterOutput;

  ShifterOutput = (u32)sign_x_to_s32(26, dsp->shift_reg) << (inst.part.shift0 ^ inst.part.shift1);

  if (!inst.part.shift1)
  {
    if(ShifterOutput > 0x7FFFFF)
      ShifterOutput = 0x7FFFFF;
    else if(ShifterOutput < -0x800000)
      ShifterOutput = 0x800000;
  }
  ShifterOutput &= 0xFFFFFF;

  if (inst.part.ewt)
    dsp->efreg[inst.part.ewa] = (ShifterOutput >> 8);

  if (inst.part.twt)
    dsp->temp[TEMPWriteAddr] = ShifterOutput;

  if (inst.part.frcl)
  {
    const unsigned F_SEL_Inputs[2] = { (unsigned)(ShifterOutput >> 11), (unsigned)(ShifterOutput & 0xFFF) };

    dsp->frc_reg = F_SEL_Inputs[inst.part.shift0 & inst.part.shift1];
    //printf("FRCL: 0x%08x\n", DSP.FRC_REG);
  }

  dsp->product = ((s64)sign_x_to_s32(13, Y_SEL_Inputs[inst.part.ysel]) * X_SEL_Inputs[inst.part.xsel]) >> 12;

  u32 SGAOutput;

  SGAOutput = SGA_Inputs[inst.part.bsel];

  if (inst.part.negb)
    SGAOutput = -SGAOutput;

  if (inst.part.zero)
    SGAOutput = 0;

  dsp->shift_reg = (dsp->product + SGAOutput) & 0x3FFFFFF;
  //
  //
  if (inst.part.iwt)
  {
    dsp->mems[inst.part.iwa] = dsp->read_value;
  }

  if (dsp->read_pending)
  {
    u16 tmp = sound_ram_16[dsp->io_addr];
    dsp->read_value = (dsp->read_pending == 2) ? (tmp << 8) : float_to_int(tmp);
    dsp->read_pending = 0;
  }
  else if (dsp->write_pending)
  {
    if (!(dsp->io_addr & 0x40000))
      sound_ram_16[dsp->io_addr] = dsp->write_value;
    dsp->write_pending = 0;
  }
  {
    u16 addr;

    addr = dsp->madrs[inst.part.masa];
    addr += inst.part.nxadr;

    if (inst.part.adreb)
    {
      addr += sign_x_to_s32(12, dsp->adrs_reg);
    }

    if (!inst.part.table)
    {
      addr += dsp->mdec_ct;
      addr &= (0x2000 << dsp->rbl) - 1;
    }

    dsp->io_addr = (addr + (dsp->rbp << 12)) & 0x3FFFF;

    if (inst.part.mrd)
    {
      dsp->read_pending = 1 + inst.part.nofl;
    }
    if (inst.part.mwt)
    {
      dsp->write_pending = 1;
      dsp->write_value = inst.part.nofl ? (ShifterOutput >> 8) : int_to_float(ShifterOutput);
    }
    if (inst.part.adrl)
    {
      const u16 A_SEL_Inputs[2] = { (u16)((INPUTS >> 16) & 0xFFF), (u16)(ShifterOutput >> 12) };

      dsp->adrs_reg = A_SEL_Inputs[inst.part.shift0 & inst.part.shift1];
    }
  }
}


//sign extended to 32 bits instead of 24
s32 float_to_int(u16 f_val)
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

   return ret_val;
}

u16 int_to_float(u32 i_val)
{
   u32 sign = (i_val >> 23) & 1;
   u32 exponent = 0;

   if (sign != 0)
      i_val = (~i_val) & 0x7FFFFF;

   if (i_val <= 0x1FFFF)
   {
      i_val *= 64;
      exponent += 0x3000;
   }

   if (i_val <= 0xFFFFF)
   {
      i_val *= 8;
      exponent += 0x1800;
   }

   if (i_val <= 0x3FFFFF)
   {
      i_val *= 2;
      exponent += 0x800;
   }

   if (i_val <= 0x3FFFFF)
   {
      i_val *= 2;
      exponent += 0x800;
   }

   if (i_val <= 0x3FFFFF)
      exponent += 0x800;

   i_val >>= 11;
   i_val &= 0x7ff;
   i_val |= exponent;

   if (sign != 0)
      i_val ^= (0x7ff | (1 << 15));

   return i_val;
}

int ScspDspAssembleGetValue(char* instruction)
{
   char temp[512] = { 0 };
   int value = 0;
   sscanf(instruction, "%s %d", temp, &value);
   return value;
}

u64 ScspDspAssembleLine(char* line)
{
   union ScspDspInstruction instruction = { 0 };

   char* temp = NULL;

   if ((temp = strstr(line, "tra")))
   {
      instruction.part.tra = ScspDspAssembleGetValue(temp);
   }

   if (strstr(line, "twt"))
   {
      instruction.part.twt = 1;
   }

   if ((temp = strstr(line, "twa")))
   {
      instruction.part.twa = ScspDspAssembleGetValue(temp);
   }

   if (strstr(line, "xsel"))
   {
      instruction.part.xsel = 1;
   }

   if ((temp = strstr(line, "ysel")))
   {
      instruction.part.ysel = ScspDspAssembleGetValue(temp);
   }

   if ((temp = strstr(line, "ira")))
   {
      instruction.part.ira = ScspDspAssembleGetValue(temp);
   }

   if (strstr(line, "iwt"))
   {
      instruction.part.iwt = 1;
   }

   if ((temp = strstr(line, "iwa")))
   {
      instruction.part.iwa = ScspDspAssembleGetValue(temp);
   }

   if (strstr(line, "table"))
   {
      instruction.part.table = 1;
   }

   if (strstr(line, "mwt"))
   {
      instruction.part.mwt = 1;
   }

   if (strstr(line, "mrd"))
   {
      instruction.part.mrd = 1;
   }

   if (strstr(line, "ewt"))
   {
      instruction.part.ewt = 1;
   }

   if ((temp = strstr(line, "ewa")))
   {
      instruction.part.ewa = ScspDspAssembleGetValue(temp);
   }

   if (strstr(line, "adrl"))
   {
      instruction.part.adrl = 1;
   }

   if (strstr(line, "frcl"))
   {
      instruction.part.frcl = 1;
   }

   if ((temp = strstr(line, "shift")))
   {
      instruction.part.shift1 = ScspDspAssembleGetValue(temp);
   }

   if (strstr(line, "yrl"))
   {
      instruction.part.yrl = 1;
   }

   if (strstr(line, "negb"))
   {
      instruction.part.negb = 1;
   }

   if (strstr(line, "zero"))
   {
      instruction.part.zero = 1;
   }

   if (strstr(line, "bsel"))
   {
      instruction.part.bsel = 1;
   }

   if (strstr(line, "nofl"))
   {
      instruction.part.nofl = 1;
   }

   if ((temp = strstr(line, "coef")))
   {
      instruction.part.coef = ScspDspAssembleGetValue(temp);
   }

   if ((temp = strstr(line, "masa")))
   {
      instruction.part.masa = ScspDspAssembleGetValue(temp);
   }

   if (strstr(line, "adreb"))
   {
      instruction.part.adreb = 1;
   }

   if (strstr(line, "nxadr"))
   {
      instruction.part.adreb = 1;
   }

   if (strstr(line, "nop"))
   {
      instruction.all = 0;
   }

   return instruction.all;
}

void ScspDspAssembleFromFile(char * filename, u64* output)
{
   int i;
   char line[1024] = { 0 };

   FILE * fp = fopen(filename, "r");

   if (!fp)
   {
      return;
   }

   for (i = 0; i < 128; i++)
   {
      char * result = fgets(line, sizeof(line), fp);
      output[i] = ScspDspAssembleLine(line);
   }
   fclose(fp);
}

void ScspDspDisasm(u8 addr, char *outstring)
{
   union ScspDspInstruction instruction;

   instruction.all = scsp_dsp.mpro[addr];

   sprintf(outstring, "%02X: ", addr);
   outstring += strlen(outstring);

   if (instruction.all == 0)
   {
      sprintf(outstring, "nop ");
      outstring += strlen(outstring);
      return;
   }

   if (instruction.part.nofl)
   {
      sprintf(outstring, "nofl ");
      outstring += strlen(outstring);
   }

   if (instruction.part.coef)
   {
      sprintf(outstring, "coef %02X ", (unsigned int)(instruction.part.coef & 0x3F));
      outstring += strlen(outstring);
   }

   if (instruction.part.masa)
   {
      sprintf(outstring, "masa %02X ", (unsigned int)(instruction.part.masa & 0x1F));
      outstring += strlen(outstring);
   }

   if (instruction.part.adreb)
   {
      sprintf(outstring, "adreb ");
      outstring += strlen(outstring);
   }

   if (instruction.part.nxadr)
   {
      sprintf(outstring, "nxadr ");
      outstring += strlen(outstring);
   }

   if (instruction.part.table)
   {
      sprintf(outstring, "table ");
      outstring += strlen(outstring);
   }

   if (instruction.part.mwt)
   {
      sprintf(outstring, "mwt ");
      outstring += strlen(outstring);
   }

   if (instruction.part.mrd)
   {
      sprintf(outstring, "mrd ");
      outstring += strlen(outstring);
   }

   if (instruction.part.ewt)
   {
      sprintf(outstring, "ewt ");
      outstring += strlen(outstring);
   }

   if (instruction.part.ewa)
   {
      sprintf(outstring, "ewa %01X ", (unsigned int)(instruction.part.ewa & 0xf));
      outstring += strlen(outstring);
   }

   if (instruction.part.adrl)
   {
      sprintf(outstring, "adrl ");
      outstring += strlen(outstring);
   }

   if (instruction.part.frcl)
   {
      sprintf(outstring, "frcl ");
      outstring += strlen(outstring);
   }

   if (instruction.part.shift1)
   {
      sprintf(outstring, "shift %d ", (int)(instruction.part.shift1 & 3));
      outstring += strlen(outstring);
   }

   if (instruction.part.yrl)
   {
      sprintf(outstring, "yrl ");
      outstring += strlen(outstring);
   }

   if (instruction.part.negb)
   {
      sprintf(outstring, "negb ");
      outstring += strlen(outstring);
   }

   if (instruction.part.zero)
   {
      sprintf(outstring, "zero ");
      outstring += strlen(outstring);
   }

   if (instruction.part.bsel)
   {
      sprintf(outstring, "bsel ");
      outstring += strlen(outstring);
   }

   if (instruction.part.xsel)
   {
      sprintf(outstring, "xsel ");
      outstring += strlen(outstring);
   }

   if (instruction.part.ysel)
   {
      sprintf(outstring, "ysel %d ", (int)(instruction.part.ysel & 3));
      outstring += strlen(outstring);
   }

   if (instruction.part.ira)
   {
      sprintf(outstring, "ira %02X ", (int)(instruction.part.ira & 0x3F));
      outstring += strlen(outstring);
   }

   if (instruction.part.iwt)
   {
      sprintf(outstring, "iwt ");
      outstring += strlen(outstring);
   }

   if (instruction.part.iwa)
   {
      sprintf(outstring, "iwa %02X ", (unsigned int)(instruction.part.iwa & 0x1F));
      outstring += strlen(outstring);
   }

   if (instruction.part.tra)
   {
      sprintf(outstring, "tra %02X ", (unsigned int)(instruction.part.tra & 0x7F));
      outstring += strlen(outstring);
   }

   if (instruction.part.twt)
   {
      sprintf(outstring, "twt ");
      outstring += strlen(outstring);
   }

   if (instruction.part.twa)
   {
      sprintf(outstring, "twa %02X ", (unsigned int)(instruction.part.twa & 0x7F));
      outstring += strlen(outstring);
   }

   if (instruction.part.unknown)
   {
      sprintf(outstring, "unknown ");
      outstring += strlen(outstring);
   }

   if (instruction.part.unknown2)
   {
      sprintf(outstring, "unknown2 ");
      outstring += strlen(outstring);
   }

//   if (instruction.part.unknown3)
 //  {
 //     sprintf(outstring, "unknown3 %d", (int)(instruction.part.unknown3 & 3));
 //     outstring += strlen(outstring);
 //  }
}

void ScspDspDisassembleToFile(char * filename)
{
   int i;
   FILE * fp = fopen(filename, "w");

   if (!fp)
   {
      return;
   }

   for (i = 0; i < 128; i++)
   {
      char output[1024] = { 0 };
      ScspDspDisasm(i, output);
      fprintf(fp, "%s\n", output);
   }

   fclose(fp);
}
