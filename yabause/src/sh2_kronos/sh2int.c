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

#include "cs2.h"

#ifdef SSH2_ASYNC
#define LOCK(A) sem_wait(&A->lock)
#define UNLOCK(A) sem_post(&A->lock)
#else
#define LOCK(A)
#define UNLOCK(A)
#endif


extern void SH2undecoded(SH2_struct * sh);

void SH2KronosIOnFrame(SH2_struct *context) {
}

//////////////////////////////////////////////////////////////////////////////

void SH2HandleInterrupts(SH2_struct *context)
{
  LOCK(context);
  if (context->NumberOfInterrupts != 0)
  {
    if (context->interrupts[context->NumberOfInterrupts - 1].level > context->regs.SR.part.I)
    {
      u32 oldpc = context->regs.PC;
      u32 persr = context->regs.SR.part.I;
      context->regs.R[15] -= 4;
      SH2MappedMemoryWriteLong(context, context->regs.R[15], context->regs.SR.all);
      context->regs.R[15] -= 4;
      SH2MappedMemoryWriteLong(context, context->regs.R[15], context->regs.PC);
      context->regs.SR.part.I = context->interrupts[context->NumberOfInterrupts - 1].level;
      context->regs.PC = SH2MappedMemoryReadLong(context,context->regs.VBR + (context->interrupts[context->NumberOfInterrupts - 1].vector << 2));
      context->NumberOfInterrupts--;
      context->isSleeping = 0;
    }
  }
  UNLOCK(context);
}
fetchfunc krfetchlist[0x1000];
static u8 cacheId[0x1000];
//static opcode_func kropcodes[0x10000];

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL FetchBios(SH2_struct *context, u32 addr)
{
   return SH2MappedMemoryReadWord(context,addr);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL FetchLWram(SH2_struct *context, u32 addr)
{
	return SH2MappedMemoryReadWord(context,addr);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL FetchHWram(SH2_struct *context, u32 addr)
{
	return SH2MappedMemoryReadWord(context,addr);
}

static u16 FASTCALL FetchVram(SH2_struct *context, u32 addr)
{
  return SH2MappedMemoryReadWord(context, addr);
}

opcode_func cacheCode[7][0x80000];
//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL FetchInvalid(SH2_struct *context, UNUSED u32 addr)
{
   return 0xFFFF;
}
//////////////////////////////////////////////////////////////////////////////
void decode(SH2_struct *context) {
  int id = (context->regs.PC >> 20) & 0xFFF;
  u16 opcode = krfetchlist[id](context, context->regs.PC);
  cacheCode[cacheId[id]][(context->regs.PC >> 1) & 0x7FFFF] = opcodeTable[opcode];
  opcodeTable[opcode](context);
}

void biosDecode(SH2_struct *context) {
  int isBUPHandled = BackupHandled(context, context->regs.PC);
  if (isBUPHandled == 0) {
    decode(context);
  }
}


int SH2KronosInterpreterInit()
{
   int i,j;
   for(i=1; i<6; i++)
     for(j=0; j<0x80000; j++)
       cacheCode[i][j] = decode;

   for(j=0; j<0x80000; j++)
     cacheCode[6][j] = SH2undecoded;

   for(j=0; j<0x80000; j++) //Special BAckupHandled case
     cacheCode[0][j] = biosDecode;

   for (i = 0; i < 0x1000; i++)
   {
      krfetchlist[i] = FetchInvalid;
      cacheId[i] = 6;
      if (((i>>8) == 0x0) || ((i>>8) == 0x2)) {
        switch (i&0xFF)
        {
          case 0x000: // Bios
            krfetchlist[i] = FetchBios;
            cacheId[i] = 0;
            break;
          case 0x002: // Low Work Ram
            krfetchlist[i] = SH2MappedMemoryReadWord;
            cacheId[i] = 1;
            break;
          case 0x020: // CS0
            krfetchlist[i] = SH2MappedMemoryReadWord;
            cacheId[i] = 2;
            break;
          case 0x05c: // Fighting Viper
            krfetchlist[i] = SH2MappedMemoryReadWord;
            cacheId[i] = 3;
            break;
          case 0x060: // High Work Ram
          case 0x061:
          case 0x062:
          case 0x063:
          case 0x064:
          case 0x065:
          case 0x066:
          case 0x067:
          case 0x068:
          case 0x069:
          case 0x06A:
          case 0x06B:
          case 0x06C:
          case 0x06D:
          case 0x06E:
          case 0x06F:
            krfetchlist[i] = SH2MappedMemoryReadWord;
            cacheId[i] = 4;
            break;
          default:
            krfetchlist[i] = FetchInvalid;
            cacheId[i] = 6;
            break;
        }
     }
     if ((i>>8) == 0xC) {
       krfetchlist[i] = SH2MappedMemoryReadWord;
       cacheId[i] = 5;
     }
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterDeInit()
{
   // DeInitialize any internal variables here
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterReset(UNUSED SH2_struct *context)
{
  SH2KronosInterpreterInit();
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void SH2UBCInterrupt(SH2_struct *context, u32 flag)
{
   if (15 > context->regs.SR.part.I) // Since UBC's interrupt are always level 15
   {
      context->regs.R[15] -= 4;
      SH2MappedMemoryWriteLong(context, context->regs.R[15], context->regs.SR.all);
      context->regs.R[15] -= 4;
      SH2MappedMemoryWriteLong(context, context->regs.R[15], context->regs.PC);
      context->regs.SR.part.I = 15;
      context->regs.PC = SH2MappedMemoryReadLong(context, context->regs.VBR + (12 << 2));
      LOG("interrupt successfully handled\n");
   }
   context->onchip.BRCR |= flag;
}

//////////////////////////////////////////////////////////////////////////////

#if 0
static void showCPUState(SH2_struct *context)
{
  int i;

  printf("=================== %s ===================\n", (context == MSH2)?"MSH2":"SSH2");
  printf("PC = 0x%x\n", context->regs.PC);
  printf("PR = 0x%x\n", context->regs.PR);
  printf("SR = 0x%x\n", context->regs.SR);
  printf("GBR = 0x%x\n", context->regs.GBR);
  printf("VBR = 0x%x\n", context->regs.VBR);
  printf("MACH = 0x%x\n", context->regs.MACH);
  printf("MACL = 0x%x\n", context->regs.MACL);
  for (int i = 0; i<16; i++)
    printf("R[%d] = 0x%x\n", i, context->regs.R[i]);

   printf("Cs2Area HIRQ = 0%x\n",  Cs2Area->reg.HIRQ);
   printf("Cs2Area Disc Changed = %d\n", Cs2Area->isdiskchanged);
   printf("Cs2Area CR1 = 0x%x\n", Cs2Area->reg.CR1);
   printf("Cs2Area CR2 = 0x%x\n", Cs2Area->reg.CR2);
   printf("Cs2Area CR3 = 0x%x\n", Cs2Area->reg.CR3);
   printf("Cs2Area CR4 = 0x%x\n", Cs2Area->reg.CR4);
   printf("Cs2Area satauth = 0x%x\n", Cs2Area->satauth);

   printf("============Prog=============\n");
   for (i=0; i<0x100000; i+=4) {
     u32 addr = 0x6000000 + i;
     printf("@0x%x : 0x%x\n", addr, MappedMemoryReadLong(MSH2, addr));
   }
   printf("===========================================\n");
}
#endif

u8 execInterrupt = 0;

FASTCALL void SH2KronosInterpreterExec(SH2_struct *context, u32 cycles)
{
  u32 target_cycle = context->cycles + cycles;
  SH2HandleInterrupts(context);
 char res[512];
  execInterrupt = 0;
   while (execInterrupt == 0)
   {
     cacheCode[cacheId[(context->regs.PC >> 20) & 0xFFF]][(context->regs.PC >> 1) & 0x7FFFF](context);
     execInterrupt |= (context->cycles >= target_cycle);
   }
}

static int enableTrace = 0;

FASTCALL void SH2KronosDebugInterpreterExec(SH2_struct *context, u32 cycles)
{
  u32 target_cycle = context->cycles + cycles;
  SH2HandleInterrupts(context);
  char res[512];
  execInterrupt = 0;
   while (execInterrupt == 0)
   {
     int id = (context->regs.PC >> 20) & 0xFFF;

#if 0
     if ((context == MSH2) && (context->regs.PC & 0xFFFFFFF) == 0x60b0000) {
       showCPUState(MSH2);
       enableTrace = 1;
     }
#endif
     if((cacheCode[cacheId[id]][(context->regs.PC >> 1) & 0x7FFFF]) != (opcodeTable[krfetchlist[id](context, context->regs.PC)]))
        if ((cacheCode[cacheId[id]][(context->regs.PC >> 1) & 0x7FFFF] != decode) && (cacheCode[cacheId[id]][(context->regs.PC >> 1) & 0x7FFFF] != biosDecode))
          printf("Error of interpreter cache @ 0x%x\n", context->regs.PC);

     if (enableTrace) {
       SH2Disasm(context->regs.PC, krfetchlist[id](context, context->regs.PC), 0, &(context->regs), res);
     }

     cacheCode[cacheId[(context->regs.PC >> 20) & 0xFFF]][(context->regs.PC >> 1) & 0x7FFFF](context);
     execInterrupt |= (context->cycles >= target_cycle);
   }
}

FASTCALL void SH2KronosInterpreterTestExec(SH2_struct *context, u32 cycles)
{
  u32 target_cycle = context->cycles + cycles;
  cacheCode[cacheId[(context->regs.PC >> 20) & 0xFFF]][(context->regs.PC >> 1) & 0x7FFFF](context);
}


FASTCALL void SH2KronosInterpreterAddCycles(SH2_struct *context, u32 value)
{
  context->cycles += value;
}
//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterGetRegisters(SH2_struct *context, sh2regs_struct *regs)
{
   memcpy(regs, &context->regs, sizeof(sh2regs_struct));
}

//////////////////////////////////////////////////////////////////////////////

u32 SH2KronosInterpreterGetGPR(SH2_struct *context, int num)
{
    return context->regs.R[num];
}

//////////////////////////////////////////////////////////////////////////////

u32 SH2KronosInterpreterGetSR(SH2_struct *context)
{
    return context->regs.SR.all;
}

//////////////////////////////////////////////////////////////////////////////

u32 SH2KronosInterpreterGetGBR(SH2_struct *context)
{
    return context->regs.GBR;
}

//////////////////////////////////////////////////////////////////////////////

u32 SH2KronosInterpreterGetVBR(SH2_struct *context)
{
    return context->regs.VBR;
}

//////////////////////////////////////////////////////////////////////////////

u32 SH2KronosInterpreterGetMACH(SH2_struct *context)
{
    return context->regs.MACH;
}

//////////////////////////////////////////////////////////////////////////////

u32 SH2KronosInterpreterGetMACL(SH2_struct *context)
{
    return context->regs.MACL;
}

//////////////////////////////////////////////////////////////////////////////

u32 SH2KronosInterpreterGetPR(SH2_struct *context)
{
    return context->regs.PR;
}

//////////////////////////////////////////////////////////////////////////////

u32 SH2KronosInterpreterGetPC(SH2_struct *context)
{
    return context->regs.PC;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSetRegisters(SH2_struct *context, const sh2regs_struct *regs)
{
   memcpy(&context->regs, regs, sizeof(sh2regs_struct));
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSetGPR(SH2_struct *context, int num, u32 value)
{
    context->regs.R[num] = value;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSetSR(SH2_struct *context, u32 value)
{
    context->regs.SR.all = value;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSetGBR(SH2_struct *context, u32 value)
{
    context->regs.GBR = value;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSetVBR(SH2_struct *context, u32 value)
{
    context->regs.VBR = value;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSetMACH(SH2_struct *context, u32 value)
{
    context->regs.MACH = value;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSetMACL(SH2_struct *context, u32 value)
{
    context->regs.MACL = value;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSetPR(SH2_struct *context, u32 value)
{
    context->regs.PR = value;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSetPC(SH2_struct *context, u32 value)
{
    context->regs.PC = value;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSendInterrupt(SH2_struct *context, u8 vector, u8 level)
{
   u32 i, i2;
   interrupt_struct tmp;
   LOCK(context);

   if ((context == SSH2) && (yabsys.IsSSH2Running == 0)) return;
   // Make sure interrupt doesn't already exist
   for (i = 0; i < context->NumberOfInterrupts; i++)
   {
      if (context->interrupts[i].vector == vector) {
         UNLOCK(context);
         return;
      }
   }

   // Ignore Timer0 and Timer1 when masked
   //if ((vector == 67 || vector == 68) && level <= context->regs.SR.part.I){
   //  UNLOCK(context);
   //  return;
   //}

   context->interrupts[context->NumberOfInterrupts].level = level;
   context->interrupts[context->NumberOfInterrupts].vector = vector;
   context->NumberOfInterrupts++;

   // Sort interrupts
   for (i = 0; i < (context->NumberOfInterrupts-1); i++)
   {
      for (i2 = i+1; i2 < context->NumberOfInterrupts; i2++)
      {
         if (context->interrupts[i].level > context->interrupts[i2].level)
         {
            tmp.level = context->interrupts[i].level;
            tmp.vector = context->interrupts[i].vector;
            context->interrupts[i].level = context->interrupts[i2].level;
            context->interrupts[i].vector = context->interrupts[i2].vector;
            context->interrupts[i2].level = tmp.level;
            context->interrupts[i2].vector = tmp.vector;
         }
      }
   }
   UNLOCK(context);
}

void SH2KronosInterpreterRemoveInterrupt(SH2_struct *context, u8 vector, u8 level) {
  u32 i, i2;
  interrupt_struct tmp;
  int hit = -1;

  for (i = 0; i < context->NumberOfInterrupts; i++) {
    if (context->interrupts[i].vector == vector) {
      context->interrupts[i].level = 0;
      context->interrupts[i].vector = 0;
      hit = i;
      break;
    }
  }

  if (hit != -1) {
    i2 = 0;
    for (i = 0; i < context->NumberOfInterrupts; i++) {
      if (i != hit) {
        context->interrupts[i2].level = context->interrupts[i].level;
        context->interrupts[i2].vector = context->interrupts[i].vector;
        i2++;
      }
    }
    context->NumberOfInterrupts--;
  }
}

//////////////////////////////////////////////////////////////////////////////

int SH2KronosInterpreterGetInterrupts(SH2_struct *context,
                                interrupt_struct interrupts[MAX_INTERRUPTS])
{
   memcpy(interrupts, context->interrupts, sizeof(interrupt_struct) * MAX_INTERRUPTS);
   return context->NumberOfInterrupts;
}

//////////////////////////////////////////////////////////////////////////////

void SH2KronosInterpreterSetInterrupts(SH2_struct *context, int num_interrupts,
                                 const interrupt_struct interrupts[MAX_INTERRUPTS])
{
   memcpy(context->interrupts, interrupts, sizeof(interrupt_struct) * MAX_INTERRUPTS);
   context->NumberOfInterrupts = num_interrupts;
}

void SH2KronosWriteNotify(u32 start, u32 length){
  int i;
  for (i=0; i<length; i+=2) {
    int id = ((start + i) >> 20) & 0xFFF;
    int addr = (start + i) >> 1;
    if (cacheId[id] == 0) {  //Special BAckupHandled case
      cacheCode[cacheId[id]][addr & 0x7FFFF] = biosDecode;
    }
    else
      cacheCode[cacheId[id]][(addr) & 0x7FFFF] = decode;
  }
}

//////////////////////////////////////////////////////////////////////////////
SH2Interface_struct SH2KronosInterpreter = {
   SH2CORE_KRONOS_INTERPRETER,
   "SH2 Kronos Interpreter",

   SH2KronosInterpreterInit,
   SH2KronosInterpreterDeInit,
   SH2KronosInterpreterReset,
   SH2KronosInterpreterExec,
//SH2KronosDebugInterpreterExec,
   SH2KronosInterpreterTestExec,

   SH2KronosInterpreterGetRegisters,
   SH2KronosInterpreterGetGPR,
   SH2KronosInterpreterGetSR,
   SH2KronosInterpreterGetGBR,
   SH2KronosInterpreterGetVBR,
   SH2KronosInterpreterGetMACH,
   SH2KronosInterpreterGetMACL,
   SH2KronosInterpreterGetPR,
   SH2KronosInterpreterGetPC,

   SH2KronosInterpreterSetRegisters,
   SH2KronosInterpreterSetGPR,
   SH2KronosInterpreterSetSR,
   SH2KronosInterpreterSetGBR,
   SH2KronosInterpreterSetVBR,
   SH2KronosInterpreterSetMACH,
   SH2KronosInterpreterSetMACL,
   SH2KronosInterpreterSetPR,
   SH2KronosInterpreterSetPC,
   SH2KronosIOnFrame,

   SH2KronosInterpreterSendInterrupt,
   SH2KronosInterpreterRemoveInterrupt,
   SH2KronosInterpreterGetInterrupts,
   SH2KronosInterpreterSetInterrupts,

   SH2KronosWriteNotify,
   SH2KronosInterpreterAddCycles
};
