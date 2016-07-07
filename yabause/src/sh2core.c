/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2005, 2013 Theo Berkau

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

/*! \file sh2core.c
    \brief SH2 shared emulation functions.
*/

#include <stdlib.h>
#include "sh2core.h"
#include "debug.h"
#include "memory.h"
#include "yabause.h"
#include "sh7034.h"
#include "cs0.h"
#include "cs1.h"
#include "cs2.h"
#include "scsp.h"
#include "vdp1.h"
#include "vdp2.h"
#include "ygr.h"
#include "assert.h"
#include "smpc.h"
#include "scu.h"

// SH1/SH2 differences
// SH1's mac.w operates at a smaller precision. 16x16+42 instead of 16x16+64
// SH1 is missing the following instructions: bf/s, braf, bsrf, bt/s, dmuls.l, dmulu.l, dt, mac.l, mul.l

#if defined(SH2_DYNAREC)
#include "sh2_dynarec/sh2_dynarec.h"
#endif

SH2_struct *SH1=NULL;
SH2_struct *MSH2=NULL;
SH2_struct *SSH2=NULL;
SH2Interface_struct *SH1Core=NULL;
SH2Interface_struct *SH2Core=NULL;
extern SH2Interface_struct *SH2CoreList[];

void OnchipReset(SH2_struct *context);
void FRTExec(SH2_struct *sh, u32 cycles);
void WDTExec(SH2_struct *sh, u32 cycles);
u8 SCIReceiveByte(void);
void SCITransmitByte(u8);

//////////////////////////////////////////////////////////////////////////////

int SH1Init(int coreid)
{
   int i;

   if ((SH1 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
      return -1;

   if (SH2TrackInfLoopInit(SH1) != 0)
      return -1;

   SH1->onchip.BCR1 = 0x0000;
   SH1->isslave = 0;
   SH1->model = SHMT_SH1;

   SH1->MappedMemoryReadByte = Sh1MemoryReadByte;
   SH1->MappedMemoryReadWord = Sh1MemoryReadWord;
   SH1->MappedMemoryReadLong = Sh1MemoryReadLong;
   SH1->MappedMemoryWriteByte = Sh1MemoryWriteByte;
   SH1->MappedMemoryWriteWord = Sh1MemoryWriteWord;
   SH1->MappedMemoryWriteLong = Sh1MemoryWriteLong;

   // So which core do we want?
   if (coreid == SH2CORE_DEFAULT)
      coreid = 0; // Assume we want the first one

   // Go through core list and find the id
   for (i = 0; SH2CoreList[i] != NULL; i++)
   {
      if (SH2CoreList[i]->id == coreid)
      {
         // Set to current core
         SH1Core = SH2CoreList[i];
         break;
      }
   }

   if ((SH1Core == NULL) || (SH1Core->Init(SHMT_SH1, SH1, NULL) != 0)) {
      free(SH1);
      SH1 = NULL;
      return -1;
   }

   SH1->core = SH1Core;

   sh1_init_func();

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void sh2_set_read_write_funcs(SH2_struct * sh)
{
   if (yabsys.sh2_cache_enabled)
   {
      sh->MappedMemoryReadByte = MappedMemoryReadByteCacheEnabled;
      sh->MappedMemoryReadWord = MappedMemoryReadWordCacheEnabled;
      sh->MappedMemoryReadLong = MappedMemoryReadLongCacheEnabled;
      sh->MappedMemoryWriteByte = MappedMemoryWriteByteCacheEnabled;
      sh->MappedMemoryWriteWord = MappedMemoryWriteWordCacheEnabled;
      sh->MappedMemoryWriteLong = MappedMemoryWriteLongCacheEnabled;
   }
   else
   {
      sh->MappedMemoryReadByte = MappedMemoryReadByteNocache;
      sh->MappedMemoryReadWord = MappedMemoryReadWordNocache;
      sh->MappedMemoryReadLong = MappedMemoryReadLongNocache;
      sh->MappedMemoryWriteByte = MappedMemoryWriteByteNocache;
      sh->MappedMemoryWriteWord = MappedMemoryWriteWordNocache;
      sh->MappedMemoryWriteLong = MappedMemoryWriteLongNocache;
   }
}

int SH2Init(int coreid)
{
   int i;

   // MSH2
   if ((MSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
      return -1;

   if (SH2TrackInfLoopInit(MSH2) != 0)
      return -1;

   MSH2->onchip.BCR1 = 0x0000;
   MSH2->isslave = 0;
   MSH2->model = SHMT_SH2;

   sh2_set_read_write_funcs(MSH2);

   // SSH2
   if ((SSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
      return -1;

   if (SH2TrackInfLoopInit(SSH2) != 0)
      return -1;

   SSH2->onchip.BCR1 = 0x8000;
   SSH2->isslave = 1;
   SSH2->model = SHMT_SH2;

   sh2_set_read_write_funcs(SSH2);

   // So which core do we want?
   if (coreid == SH2CORE_DEFAULT)
      coreid = 0; // Assume we want the first one

   // Go through core list and find the id
   for (i = 0; SH2CoreList[i] != NULL; i++)
   {
      if (SH2CoreList[i]->id == coreid)
      {
         // Set to current core
         SH2Core = SH2CoreList[i];
         break;
      }
   }

   if ((SH2Core == NULL) || (SH2Core->Init(SHMT_SH2, MSH2, SSH2) != 0)) {
      free(MSH2);
      free(SSH2);
      MSH2 = SSH2 = NULL;
      return -1;
   }

   MSH2->core = SSH2->core = SH2Core;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH1DeInit()
{
   if (SH1Core)
      SH1Core->DeInit();
   SH1Core = NULL;

   if (SH1)
   {
      SH2TrackInfLoopDeInit(SH1);
      free(SH1);
   }
   SH1 = NULL;
}

//////////////////////////////////////////////////////////////////////////////

void SH2DeInit()
{
   if (SH2Core)
      SH2Core->DeInit();
   SH2Core = NULL;

   if (MSH2)
   {
      SH2TrackInfLoopDeInit(MSH2);
      free(MSH2);
   }
   MSH2 = NULL;

   if (SSH2)
   {
      SH2TrackInfLoopDeInit(SSH2);
      free(SSH2);
   }
   SSH2 = NULL;
}

//////////////////////////////////////////////////////////////////////////////

void SH2Reset(SH2_struct *context)
{
   int i;
   SH2Interface_struct *core=context->core;
   
   // Reset general registers
   for (i = 0; i < 15; i++)
      core->SetGPR(context, i, 0x00000000);

   core->SetSR(context, 0x000000F0);
   core->SetGBR(context, 0x00000000);
   core->SetVBR(context, 0x00000000);
   core->SetMACH(context, 0x00000000);
   core->SetMACL(context, 0x00000000);
   core->SetPR(context, 0x00000000);

   // Internal variables
   context->delay = 0x00000000;
   context->cycles = 0;
   context->isIdle = 0;

   context->frc.leftover = 0;
   context->frc.shift = 3;
 
   context->wdt.isenable = 0;
   context->wdt.isinterval = 1;
   context->wdt.shift = 1;
   context->wdt.leftover = 0;

   // Reset Interrupts
   memset((void *)context->interrupts, 0, sizeof(interrupt_struct) * MAX_INTERRUPTS);
   core->SetInterrupts(context, 0, context->interrupts);

   // Core specific reset
   core->Reset(context);

   // Reset Onchip modules
   OnchipReset(context);
   cache_clear(&context->onchip.cache);

   // Reset backtrace
   context->bt.numbacktrace = 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2PowerOn(SH2_struct *context) {
   SH2Interface_struct *core=context->core;
   u32 VBR = core->GetVBR(context);
   core->SetPC(context, context->MappedMemoryReadLong(context, VBR));
   core->SetGPR(context, 15, context->MappedMemoryReadLong(context, VBR+4));
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SH2Exec(SH2_struct *context, u32 cycles)
{
   context->core->Exec(context, cycles);

   if(context->model == SHMT_SH1)
      sh1_onchip_run_cycles(cycles);
   else
      FRTExec(context, cycles);

   WDTExec(context, cycles);

   if (context->model == SHMT_SH2 && yabsys.use_sh2_dma_timing)
      sh2_dma_exec(context, cycles);

   if (UNLIKELY(context->cycles < cycles))
      context->cycles = 0;
   else
      context->cycles -= cycles;
}

//////////////////////////////////////////////////////////////////////////////

void SH2SendInterrupt(SH2_struct *context, u8 vector, u8 level)
{
   context->core->SendInterrupt(context, vector, level);
}

//////////////////////////////////////////////////////////////////////////////

void SH2NMI(SH2_struct *context)
{
   context->onchip.ICR |= 0x8000;
   SH2SendInterrupt(context, 0xB, 0x10);
}

//////////////////////////////////////////////////////////////////////////////

void SH2Step(SH2_struct *context)
{
   SH2Interface_struct *core=context->core;
   if (core)
   {
      u32 tmp = core->GetPC(context);

      // Execute 1 instruction
      SH2Exec(context, context->cycles+1);

      // Sometimes it doesn't always execute one instruction,
      // let's make sure it did
      if (tmp == core->GetPC(context))
         SH2Exec(context, context->cycles+1);
   }
}

//////////////////////////////////////////////////////////////////////////////

int SH2StepOver(SH2_struct *context, void (*func)(void *, u32, void *))
{
   SH2Interface_struct *core=context->core;
   if (core)
   {
      u32 tmp = core->GetPC(context);
      u16 inst= context->MappedMemoryReadWord(context, context->regs.PC);

      // If instruction is jsr, bsr, or bsrf, step over it
      if ((inst & 0xF000) == 0xB000 || // BSR 
         (inst & 0xF0FF) == 0x0003 || // BSRF
         (inst & 0xF0FF) == 0x400B)   // JSR
      {
         // Set breakpoint after at PC + 4
         context->stepOverOut.callBack = func;
         context->stepOverOut.type = SH2ST_STEPOVER;
         context->stepOverOut.enabled = 1;
         context->stepOverOut.address = context->regs.PC+4;
         return 1;
      }
      else
      {
         // Execute 1 instruction instead
         SH2Exec(context, context->cycles+1);

         // Sometimes it doesn't always execute one instruction,
         // let's make sure it did
         if (tmp == SH2Core->GetPC(context))
            SH2Exec(context, context->cycles+1);
      }
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2StepOut(SH2_struct *context, void (*func)(void *, u32, void *))
{
   if (SH2Core)
   {
      context->stepOverOut.callBack = func;
      context->stepOverOut.type = SH2ST_STEPOUT;
      context->stepOverOut.enabled = 1;
      context->stepOverOut.address = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

int SH2TrackInfLoopInit(SH2_struct *context)
{
   context->trackInfLoop.maxNum = 100;
   if ((context->trackInfLoop.match = calloc(context->trackInfLoop.maxNum, sizeof(tilInfo_struct))) == NULL)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2TrackInfLoopDeInit(SH2_struct *context)
{
   if (context->trackInfLoop.match)
      free(context->trackInfLoop.match);
}

//////////////////////////////////////////////////////////////////////////////

void SH2TrackInfLoopStart(SH2_struct *context)
{
   context->trackInfLoop.enabled = 1;
}

//////////////////////////////////////////////////////////////////////////////

void SH2TrackInfLoopStop(SH2_struct *context)
{
   context->trackInfLoop.enabled = 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2TrackInfLoopClear(SH2_struct *context)
{
   memset(context->trackInfLoop.match, 0, sizeof(tilInfo_struct) * context->trackInfLoop.maxNum);
   context->trackInfLoop.num = 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2GetRegisters(SH2_struct *context, sh2regs_struct * r)
{
   if (r != NULL) {
      SH2Core->GetRegisters(context, r);
   }
}

//////////////////////////////////////////////////////////////////////////////

void SH2SetRegisters(SH2_struct *context, sh2regs_struct * r)
{
   if (r != NULL) {
      SH2Core->SetRegisters(context, r);
   }
}

//////////////////////////////////////////////////////////////////////////////

void SH2WriteNotify(u32 start, u32 length) {
   if (SH2Core->WriteNotify)
      SH2Core->WriteNotify(start, length);
}

//////////////////////////////////////////////////////////////////////////////

void SH2SetBreakpointCallBack(SH2_struct *context, void (*func)(void *, u32, void *), void *userdata) {
   context->bp.BreakpointCallBack = func;
   context->bp.BreakpointUserData = userdata;
}

//////////////////////////////////////////////////////////////////////////////

int SH2AddCodeBreakpoint(SH2_struct *context, u32 addr) {
   int i;

   if (context->bp.numcodebreakpoints < MAX_BREAKPOINTS) {
      // Make sure it isn't already on the list
      for (i = 0; i < context->bp.numcodebreakpoints; i++)
      {
         if (addr == context->bp.codebreakpoint[i].addr)
            return -1;
      }

      context->bp.codebreakpoint[context->bp.numcodebreakpoints].addr = addr;
      context->bp.numcodebreakpoints++;

      return 0;
   }

   return -1;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2SortCodeBreakpoints(SH2_struct *context) {
   int i, i2;
   u32 tmp;

   for (i = 0; i < (MAX_BREAKPOINTS-1); i++)
   {
      for (i2 = i+1; i2 < MAX_BREAKPOINTS; i2++)
      {
         if (context->bp.codebreakpoint[i].addr == 0xFFFFFFFF &&
             context->bp.codebreakpoint[i2].addr != 0xFFFFFFFF)
         {
            tmp = context->bp.codebreakpoint[i].addr;
            context->bp.codebreakpoint[i].addr = context->bp.codebreakpoint[i2].addr;
            context->bp.codebreakpoint[i2].addr = tmp;
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

int SH2DelCodeBreakpoint(SH2_struct *context, u32 addr) {
   int i, i2;

   LOG("Deleting breakpoint %08X...\n", addr);

   if (context->bp.numcodebreakpoints > 0) {
      for (i = 0; i < context->bp.numcodebreakpoints; i++) {
         if (context->bp.codebreakpoint[i].addr == addr)
         {
            context->bp.codebreakpoint[i].addr = 0xFFFFFFFF;
            SH2SortCodeBreakpoints(context);
            context->bp.numcodebreakpoints--;

            LOG("Remaining breakpoints: \n");

            for (i2 = 0; i2 < context->bp.numcodebreakpoints; i2++)
            {
               LOG("%08X", context->bp.codebreakpoint[i2].addr);
            }

            return 0;
         }
      }
   }

   LOG("Failed deleting breakpoint\n");

   return -1;
}

//////////////////////////////////////////////////////////////////////////////

codebreakpoint_struct *SH2GetBreakpointList(SH2_struct *context) {
   return context->bp.codebreakpoint;
}

//////////////////////////////////////////////////////////////////////////////

void SH2ClearCodeBreakpoints(SH2_struct *context) {
   int i;
   for (i = 0; i < MAX_BREAKPOINTS; i++) {
      context->bp.codebreakpoint[i].addr = 0xFFFFFFFF;
   }

   context->bp.numcodebreakpoints = 0;
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL SH2MemoryBreakpointReadByte(SH2_struct *sh, u32 addr) {
   int i;

   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (sh->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
      {
         if (sh->bp.BreakpointCallBack && sh->bp.inbreakpoint == 0)
         {
            sh->bp.inbreakpoint = 1;
            sh->bp.BreakpointCallBack(sh, 0, sh->bp.BreakpointUserData);
            sh->bp.inbreakpoint = 0;
         }

         return sh->bp.memorybreakpoint[i].oldreadbyte(sh, addr);
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (((sh->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
         return sh->bp.memorybreakpoint[i].oldreadbyte(sh, addr);
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL SH2MemoryBreakpointReadWord(SH2_struct *sh, u32 addr) {
   int i;

   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (sh->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
      {
         if (sh->bp.BreakpointCallBack && sh->bp.inbreakpoint == 0)
         {
            sh->bp.inbreakpoint = 1;
            sh->bp.BreakpointCallBack(sh, 0, sh->bp.BreakpointUserData);
            sh->bp.inbreakpoint = 0;
         }

         return sh->bp.memorybreakpoint[i].oldreadword(sh, addr);
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (((sh->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
         return sh->bp.memorybreakpoint[i].oldreadword(sh, addr);
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL SH2MemoryBreakpointReadLong(SH2_struct *sh, u32 addr) {
   int i;

   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (sh->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
      {
         if (sh->bp.BreakpointCallBack && sh->bp.inbreakpoint == 0)
         {
            sh->bp.inbreakpoint = 1;
            sh->bp.BreakpointCallBack(sh, 0, sh->bp.BreakpointUserData);
            sh->bp.inbreakpoint = 0;
         }

         return sh->bp.memorybreakpoint[i].oldreadlong(sh, addr);
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (((sh->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
         return sh->bp.memorybreakpoint[i].oldreadlong(sh, addr);
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL SH2MemoryBreakpointWriteByte(SH2_struct *sh, u32 addr, u8 val) {
   int i;

   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (sh->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
      {
         if (sh->bp.BreakpointCallBack && sh->bp.inbreakpoint == 0)
         {
            sh->bp.inbreakpoint = 1;
            sh->bp.BreakpointCallBack(sh, 0, sh->bp.BreakpointUserData);
            sh->bp.inbreakpoint = 0;
         }

         sh->bp.memorybreakpoint[i].oldwritebyte(sh, addr, val);
         return;
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (((sh->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
      {
         sh->bp.memorybreakpoint[i].oldwritebyte(sh, addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL SH2MemoryBreakpointWriteWord(SH2_struct *sh, u32 addr, u16 val) {
   int i;

   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (sh->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
      {
         if (sh->bp.BreakpointCallBack && sh->bp.inbreakpoint == 0)
         {
            sh->bp.inbreakpoint = 1;
            sh->bp.BreakpointCallBack(sh, 0, sh->bp.BreakpointUserData);
            sh->bp.inbreakpoint = 0;
         }

         sh->bp.memorybreakpoint[i].oldwriteword(sh, addr, val);
         return;
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (((sh->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
      {
         sh->bp.memorybreakpoint[i].oldwriteword(sh, addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL SH2MemoryBreakpointWriteLong(SH2_struct *sh, u32 addr, u32 val) {
   int i;

   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (sh->bp.memorybreakpoint[i].addr == (addr & 0x0FFFFFFF))
      {
         if (sh->bp.BreakpointCallBack && sh->bp.inbreakpoint == 0)
         {
            sh->bp.inbreakpoint = 1;
            sh->bp.BreakpointCallBack(sh, 0, sh->bp.BreakpointUserData);
            sh->bp.inbreakpoint = 0;
         }

         sh->bp.memorybreakpoint[i].oldwritelong(sh, addr, val);
         return;
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < sh->bp.nummemorybreakpoints; i++)
   {
      if (((sh->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
      {
         sh->bp.memorybreakpoint[i].oldwritelong(sh, addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

static int CheckForMemoryBreakpointDupes(SH2_struct *context, u32 addr, u32 flag, int *which)
{
   int i;

   for (i = 0; i < context->bp.nummemorybreakpoints; i++)
   {
      if (((context->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) ==
          ((addr >> 16) & 0xFFF))
      {
         // See it actually was using the same operation flag
         if (context->bp.memorybreakpoint[i].flags & flag)
         {
            *which = i;
            return 1;
         }
      }                
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int SH2AddMemoryBreakpoint(SH2_struct *context, u32 addr, u32 flags) {
   int which;
   int i;

   if (flags == 0)
      return -1;

   if (context->bp.nummemorybreakpoints < MAX_BREAKPOINTS) {
      // Only regular addresses are supported at this point(Sorry, no onchip!)
      switch (addr >> 29) {
         case 0x0:
         case 0x1:
         case 0x5:
            break;
         default:
            return -1;
      }

      addr &= 0x0FFFFFFF;

      // Make sure it isn't already on the list
      for (i = 0; i < context->bp.nummemorybreakpoints; i++)
      {
         if (addr == context->bp.memorybreakpoint[i].addr)
            return -1;
      }

      context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].addr = addr;
      context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].flags = flags;

      context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadbyte = context->ReadByteList[(addr >> 16) & 0xFFF];
      context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadword = context->ReadWordList[(addr >> 16) & 0xFFF];
      context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadlong = context->ReadLongList[(addr >> 16) & 0xFFF];
      context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwritebyte = context->WriteByteList[(addr >> 16) & 0xFFF];
      context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwriteword = context->WriteWordList[(addr >> 16) & 0xFFF];
      context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwritelong = context->WriteLongList[(addr >> 16) & 0xFFF];

      if (flags & BREAK_BYTEREAD)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_BYTEREAD, &which))
            context->ReadByteList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointReadByte;
         else
            // fix old memory access function
            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadbyte = context->bp.memorybreakpoint[which].oldreadbyte;
      }

      if (flags & BREAK_WORDREAD)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_WORDREAD, &which))
            context->ReadWordList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointReadWord;
         else
            // fix old memory access function
            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadword = context->bp.memorybreakpoint[which].oldreadword;
      }

      if (flags & BREAK_LONGREAD)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_LONGREAD, &which))
            context->ReadLongList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointReadLong;
         else
            // fix old memory access function
            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldreadword = context->bp.memorybreakpoint[which].oldreadword;
      }

      if (flags & BREAK_BYTEWRITE)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_BYTEWRITE, &which))
            context->WriteByteList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointWriteByte;
         else
            // fix old memory access function
            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwritebyte = context->bp.memorybreakpoint[which].oldwritebyte;
      }

      if (flags & BREAK_WORDWRITE)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_WORDWRITE, &which))
            context->WriteWordList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointWriteWord;
         else
            // fix old memory access function
            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwriteword = context->bp.memorybreakpoint[which].oldwriteword;
      }

      if (flags & BREAK_LONGWRITE)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_LONGWRITE, &which))
            context->WriteLongList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointWriteLong;
         else
            // fix old memory access function
            context->bp.memorybreakpoint[context->bp.nummemorybreakpoints].oldwritelong = context->bp.memorybreakpoint[which].oldwritelong;
      }

      context->bp.nummemorybreakpoints++;

      return 0;
   }

   return -1;
}

//////////////////////////////////////////////////////////////////////////////

static void SH2SortMemoryBreakpoints(SH2_struct *context) {
   int i, i2;
   memorybreakpoint_struct tmp;

   for (i = 0; i < (MAX_BREAKPOINTS-1); i++)
   {
      for (i2 = i+1; i2 < MAX_BREAKPOINTS; i2++)
      {
         if (context->bp.memorybreakpoint[i].addr == 0xFFFFFFFF &&
             context->bp.memorybreakpoint[i2].addr != 0xFFFFFFFF)
         {
            memcpy(&tmp, context->bp.memorybreakpoint+i, sizeof(memorybreakpoint_struct));
            memcpy(context->bp.memorybreakpoint+i, context->bp.memorybreakpoint+i2, sizeof(memorybreakpoint_struct));
            memcpy(context->bp.memorybreakpoint+i2, &tmp, sizeof(memorybreakpoint_struct));
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

int SH2DelMemoryBreakpoint(SH2_struct *context, u32 addr) {
   int i, i2;

   if (context->bp.nummemorybreakpoints > 0) {
      for (i = 0; i < context->bp.nummemorybreakpoints; i++) {
         if (context->bp.memorybreakpoint[i].addr == addr)
         {
            // Remove memory access piggyback function to memory access function table

            // Make sure no other breakpoints need the breakpoint functions first
            for (i2 = 0; i2 < context->bp.nummemorybreakpoints; i2++)
            {
               if (((context->bp.memorybreakpoint[i].addr >> 16) & 0xFFF) ==
                   ((context->bp.memorybreakpoint[i2].addr >> 16) & 0xFFF) &&
                   i != i2)
               {
                  // Clear the flags
                  context->bp.memorybreakpoint[i].flags &= ~context->bp.memorybreakpoint[i2].flags;
               }                
            }
            
            if (context->bp.memorybreakpoint[i].flags & BREAK_BYTEREAD)
               context->ReadByteList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldreadbyte;

            if (context->bp.memorybreakpoint[i].flags & BREAK_WORDREAD)
               context->ReadWordList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldreadword;

            if (context->bp.memorybreakpoint[i].flags & BREAK_LONGREAD)
               context->ReadLongList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldreadlong;

            if (context->bp.memorybreakpoint[i].flags & BREAK_BYTEWRITE)
               context->WriteByteList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldwritebyte;

            if (context->bp.memorybreakpoint[i].flags & BREAK_WORDWRITE)
               context->WriteWordList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldwriteword;

            if (context->bp.memorybreakpoint[i].flags & BREAK_LONGWRITE)
               context->WriteLongList[(addr >> 16) & 0xFFF] = context->bp.memorybreakpoint[i].oldwritelong;

            context->bp.memorybreakpoint[i].addr = 0xFFFFFFFF;
            SH2SortMemoryBreakpoints(context);
            context->bp.nummemorybreakpoints--;
            return 0;
         }
      }
   }

   return -1;
}

//////////////////////////////////////////////////////////////////////////////

memorybreakpoint_struct *SH2GetMemoryBreakpointList(SH2_struct *context) {
   return context->bp.memorybreakpoint;
}

//////////////////////////////////////////////////////////////////////////////

void SH2ClearMemoryBreakpoints(SH2_struct *context) {
   int i;
   for (i = 0; i < MAX_BREAKPOINTS; i++)
   {
      context->bp.memorybreakpoint[i].addr = 0xFFFFFFFF;
      context->bp.memorybreakpoint[i].flags = 0;
      context->bp.memorybreakpoint[i].oldreadbyte = NULL;
      context->bp.memorybreakpoint[i].oldreadword = NULL;
      context->bp.memorybreakpoint[i].oldreadlong = NULL;
      context->bp.memorybreakpoint[i].oldwritebyte = NULL;
      context->bp.memorybreakpoint[i].oldwriteword = NULL;
      context->bp.memorybreakpoint[i].oldwritelong = NULL;
   }
   context->bp.nummemorybreakpoints = 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2HandleBackTrace(SH2_struct *context)
{
   u16 inst = context->instruction;
   if ((inst & 0xF000) == 0xB000 || // BSR 
      (inst & 0xF0FF) == 0x0003 || // BSRF
      (inst & 0xF0FF) == 0x400B)   // JSR
   {
      if (context->bt.numbacktrace < sizeof(context->bt.addr)/sizeof(u32))
      {
         context->bt.addr[context->bt.numbacktrace] = context->regs.PC;
         context->bt.numbacktrace++;
      }
   }
   else if (inst == 0x000B) // RTS
   {
      if (context->bt.numbacktrace > 0)
         context->bt.numbacktrace--;
   }
}

//////////////////////////////////////////////////////////////////////////////

u32 *SH2GetBacktraceList(SH2_struct *context, int *size)
{
   *size = context->bt.numbacktrace;
   return context->bt.addr;
}

//////////////////////////////////////////////////////////////////////////////

void SH2HandleStepOverOut(SH2_struct *context)
{
   if (context->stepOverOut.enabled)
   {
      switch ((int)context->stepOverOut.type)
      {
      case SH2ST_STEPOVER: // Step Over
         if (context->regs.PC == context->stepOverOut.address)
         {
            context->stepOverOut.enabled = 0;
            context->stepOverOut.callBack(context, context->regs.PC, (void *)context->stepOverOut.type);
         }
         break;
      case SH2ST_STEPOUT: // Step Out
         {
            u16 inst;

            if (context->stepOverOut.levels < 0 && context->regs.PC == context->regs.PR)
            {
               context->stepOverOut.enabled = 0;
               context->stepOverOut.callBack(context, context->regs.PC, (void *)context->stepOverOut.type);
               return;
            }

            inst = context->instruction;;

            if ((inst & 0xF000) == 0xB000 || // BSR 
               (inst & 0xF0FF) == 0x0003 || // BSRF
               (inst & 0xF0FF) == 0x400B)   // JSR
               context->stepOverOut.levels++;
            else if (inst == 0x000B || // RTS
                     inst == 0x002B)   // RTE
               context->stepOverOut.levels--;

            break;
         }
      default: break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void SH2HandleTrackInfLoop(SH2_struct *context)
{
   if (context->trackInfLoop.enabled)
   {
      // Look for specific bf/bt/bra instructions that branch to address < PC
      if ((context->instruction & 0x8B80) == 0x8B80 || // bf
          (context->instruction & 0x8F80) == 0x8F80 || // bf/s 
          (context->instruction & 0x8980) == 0x8980 || // bt
          (context->instruction & 0x8D80) == 0x8D80 || // bt/s 
          (context->instruction & 0xA800) == 0xA800)   // bra
      {
         int i;

         // See if it's already on match list
         for (i = 0; i < context->trackInfLoop.num; i++)
         {
            if (context->regs.PC == context->trackInfLoop.match[i].addr)
            {
               context->trackInfLoop.match[i].count++;
               return;
            }
         }

         if (context->trackInfLoop.num >= context->trackInfLoop.maxNum)
         {
            context->trackInfLoop.match = realloc(context->trackInfLoop.match, sizeof(tilInfo_struct) * (context->trackInfLoop.maxNum * 2));
            context->trackInfLoop.maxNum *= 2;
         }

         // Add new
         i=context->trackInfLoop.num;
         context->trackInfLoop.match[i].addr = context->regs.PC;
         context->trackInfLoop.match[i].count = 1;
         context->trackInfLoop.num++;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
// Onchip specific
//////////////////////////////////////////////////////////////////////////////

void OnchipReset(SH2_struct *context) {
   context->onchip.SMR = 0x00;
   context->onchip.BRR = 0xFF;
   context->onchip.SCR = 0x00;
   context->onchip.TDR = 0xFF;
   context->onchip.SSR = 0x84;
   context->onchip.RDR = 0x00;
   context->onchip.TIER = 0x01;
   context->onchip.FTCSR = 0x00;
   context->onchip.FRC.all = 0x0000;
   context->onchip.OCRA = 0xFFFF;
   context->onchip.OCRB = 0xFFFF;
   context->onchip.TCR = 0x00;
   context->onchip.TOCR = 0xE0;
   context->onchip.FICR = 0x0000;
   context->onchip.IPRB = 0x0000;
   context->onchip.VCRA = 0x0000;
   context->onchip.VCRB = 0x0000;
   context->onchip.VCRC = 0x0000;
   context->onchip.VCRD = 0x0000;
   context->onchip.DRCR0 = 0x00;
   context->onchip.DRCR1 = 0x00;
   context->onchip.WTCSR = 0x18;
   context->onchip.WTCNT = 0x00;
   context->onchip.RSTCSR = 0x1F;
   context->onchip.SBYCR = 0x60;
   context->onchip.CCR = 0x00;
   context->onchip.ICR = 0x0000;
   context->onchip.IPRA = 0x0000;
   context->onchip.VCRWDT = 0x0000;
   context->onchip.DVCR = 0x00000000;
   context->onchip.VCRDIV = 0x00000000;
   context->onchip.BARA.all = 0x00000000;
   context->onchip.BAMRA.all = 0x00000000;
   context->onchip.BBRA = 0x0000;
   context->onchip.BARB.all = 0x00000000;
   context->onchip.BAMRB.all = 0x00000000;
   context->onchip.BDRB.all = 0x00000000;
   context->onchip.BDMRB.all = 0x00000000;
   context->onchip.BBRB = 0x0000;
   context->onchip.BRCR = 0x0000;
   context->onchip.CHCR0 = 0x00000000;
   context->onchip.CHCR1 = 0x00000000;
   context->onchip.DMAOR = 0x00000000;
   context->onchip.BCR1 &= 0x8000; // preserve MASTER bit
   context->onchip.BCR1 |= 0x03F0;
   context->onchip.BCR2 = 0x00FC;
   context->onchip.WCR = 0xAAFF;
   context->onchip.MCR = 0x0000;
   context->onchip.RTCSR = 0x0000;
   context->onchip.RTCNT = 0x0000;
   context->onchip.RTCOR = 0x0000;
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL OnchipReadByte(SH2_struct *sh, u32 addr) {
   switch(addr)
   {
      case 0x000:
//         LOG("Serial Mode Register read: %02X\n", sh->onchip.SMR);
         return sh->onchip.SMR;
      case 0x001:
//         LOG("Bit Rate Register read: %02X\n", sh->onchip.BRR);
         return sh->onchip.BRR;
      case 0x002:
//         LOG("Serial Control Register read: %02X\n", sh->onchip.SCR);
         return sh->onchip.SCR;
      case 0x003:
//         LOG("Transmit Data Register read: %02X\n", sh->onchip.TDR);
         return sh->onchip.TDR;
      case 0x004:
//         LOG("Serial Status Register read: %02X\n", sh->onchip.SSR);

/*
         // if Receiver is enabled, clear SSR's TDRE bit, set SSR's RDRF and update RDR.

         if (sh->onchip.SCR & 0x10)
         {
            sh->onchip.RDR = SCIReceiveByte();
            sh->onchip.SSR = (sh->onchip.SSR & 0x7F) | 0x40;
         }
         // if Transmitter is enabled, clear SSR's RDRF bit, and set SSR's TDRE bit.
         else if (sh->onchip.SCR & 0x20)
         {
            sh->onchip.SSR = (sh->onchip.SSR & 0xBF) | 0x80;
         }
*/
         return sh->onchip.SSR;
      case 0x005:
//         LOG("Receive Data Register read: %02X PC = %08X\n", sh->onchip.RDR, SH2Core->GetPC(sh));
         return sh->onchip.RDR;
      case 0x010:
         return sh->onchip.TIER;
      case 0x011:
         return sh->onchip.FTCSR;
      case 0x012:         
         return sh->onchip.FRC.part.H;
      case 0x013:
         return sh->onchip.FRC.part.L;
      case 0x014:
         if (!(sh->onchip.TOCR & 0x10))
            return sh->onchip.OCRA >> 8;
         else
            return sh->onchip.OCRB >> 8;
      case 0x015:
         if (!(sh->onchip.TOCR & 0x10))
            return sh->onchip.OCRA & 0xFF;
         else
            return sh->onchip.OCRB & 0xFF;
      case 0x016:
         return sh->onchip.TCR;
      case 0x017:
         return sh->onchip.TOCR;
      case 0x018:
         return sh->onchip.FICR >> 8;
      case 0x019:
         return sh->onchip.FICR & 0xFF;
      case 0x060:
         return sh->onchip.IPRB >> 8;
      case 0x062:
         return sh->onchip.VCRA >> 8;
      case 0x063:
         return sh->onchip.VCRA & 0xFF;
      case 0x064:
         return sh->onchip.VCRB >> 8;
      case 0x065:
         return sh->onchip.VCRB & 0xFF;
      case 0x066:
         return sh->onchip.VCRC >> 8;
      case 0x067:
         return sh->onchip.VCRC & 0xFF;
      case 0x068:
         return sh->onchip.VCRD >> 8;
      case 0x080:
         return sh->onchip.WTCSR;
      case 0x081:
         return sh->onchip.WTCNT;
      case 0x092:
         return sh->onchip.CCR;
      case 0x0E0:
         return sh->onchip.ICR >> 8;
      case 0x0E1:
         return sh->onchip.ICR & 0xFF;
      case 0x0E2:
         return sh->onchip.IPRA >> 8;
      case 0x0E3:
         return sh->onchip.IPRA & 0xFF;
      case 0x0E4:
         return sh->onchip.VCRWDT >> 8;
      case 0x0E5:
         return sh->onchip.VCRWDT & 0xFF;
      default:
         LOG("Unhandled Onchip byte read %08X\n", (int)addr);
         break;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL OnchipReadWord(SH2_struct *sh, u32 addr) {
   switch(addr)
   {
      case 0x060:
         return sh->onchip.IPRB;
      case 0x062:
         return sh->onchip.VCRA;
      case 0x064:
         return sh->onchip.VCRB;
      case 0x066:
         return sh->onchip.VCRC;
      case 0x068:
         return sh->onchip.VCRD;
      case 0x0E0:
         return sh->onchip.ICR;
      case 0x0E2:
         return sh->onchip.IPRA;
      case 0x0E4:
         return sh->onchip.VCRWDT;
      case 0x1E2: // real BCR1 register is located at 0x1E2-0x1E3; Sega Rally OK
         return sh->onchip.BCR1;
      case 0x1E6:
         return sh->onchip.BCR2;
      case 0x1EA:
         return sh->onchip.WCR;
      case 0x1EE:
         return sh->onchip.MCR;
      case 0x1F2:
         return sh->onchip.RTCSR;
      case 0x1F6:
         return sh->onchip.RTCNT;
      case 0x1FA:
         return sh->onchip.RTCOR;
      default:
         LOG("Unhandled Onchip word read %08X\n", (int)addr);
         return 0;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL OnchipReadLong(SH2_struct *sh, u32 addr) {
   switch(addr)
   {
      case 0x100:
      case 0x120:
         return sh->onchip.DVSR;
      case 0x104: // DVDNT
      case 0x124:
         return sh->onchip.DVDNTL;
      case 0x108:
      case 0x128:
         return sh->onchip.DVCR;
      case 0x10C:
      case 0x12C:
         return sh->onchip.VCRDIV;
      case 0x110:
      case 0x130:
         return sh->onchip.DVDNTH;
      case 0x114:
      case 0x134:
         return sh->onchip.DVDNTL;
      case 0x118: // Acts as a separate register, but is set to the same value
      case 0x138: // as DVDNTH after division
         return sh->onchip.DVDNTUH;
      case 0x11C: // Acts as a separate register, but is set to the same value
      case 0x13C: // as DVDNTL after division
         return sh->onchip.DVDNTUL;
      case 0x180:
         return sh->onchip.SAR0;
      case 0x184:
         return sh->onchip.DAR0;
      case 0x188:
         return sh->onchip.TCR0;
      case 0x18C:
         return sh->onchip.CHCR0;
      case 0x190:
         return sh->onchip.SAR1;
      case 0x194:
         return sh->onchip.DAR1;
      case 0x198:
         return sh->onchip.TCR1;
      case 0x19C:
         return sh->onchip.CHCR1;
      case 0x1A0:
         return sh->onchip.VCRDMA0;
      case 0x1A8:
         return sh->onchip.VCRDMA1;
      case 0x1B0:
         return sh->onchip.DMAOR;
      case 0x1E0:
         return sh->onchip.BCR1;
      case 0x1E4:
         return sh->onchip.BCR2;
      case 0x1E8:
         return sh->onchip.WCR;
      case 0x1EC:
         return sh->onchip.MCR;
      case 0x1F0:
         return sh->onchip.RTCSR;
      case 0x1F4:
         return sh->onchip.RTCNT;
      case 0x1F8:
         return sh->onchip.RTCOR;
      default:
         LOG("Unhandled Onchip long read %08X\n", (int)addr);
         return 0;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL OnchipWriteByte(SH2_struct *sh, u32 addr, u8 val) {
   switch(addr) {
      case 0x000:
//         LOG("Serial Mode Register write: %02X\n", val);
         sh->onchip.SMR = val;
         return;
      case 0x001:
//         LOG("Bit Rate Register write: %02X\n", val);
         sh->onchip.BRR = val;
         return;
      case 0x002:
//         LOG("Serial Control Register write: %02X\n", val);

         // If Transmitter is getting disabled, set TDRE
         if (!(val & 0x20))
            sh->onchip.SSR |= 0x80;

         sh->onchip.SCR = val;
         return;
      case 0x003:
//         LOG("Transmit Data Register write: %02X. PC = %08X\n", val, SH2Core->GetPC(sh));
         sh->onchip.TDR = val;
         return;
      case 0x004:
//         LOG("Serial Status Register write: %02X\n", val);
         
         if (sh->onchip.SCR & 0x20)
         {
            // Transmitter Mode

            // If the TDRE bit cleared, let's do a transfer
            if (!(val & 0x80))
               SCITransmitByte(sh->onchip.TDR);

            // Generate an interrupt if need be here
         }
         return;
      case 0x010:
         sh->onchip.TIER = (val & 0x8E) | 0x1;
         return;
      case 0x011:
         sh->onchip.FTCSR = (sh->onchip.FTCSR & (val & 0xFE)) | (val & 0x1);
         return;
      case 0x012:
         sh->onchip.FRC.part.H = val;
         return;
      case 0x013:
         sh->onchip.FRC.part.L = val;
         return;
      case 0x014:
         if (!(sh->onchip.TOCR & 0x10))
            sh->onchip.OCRA = (val << 8) | (sh->onchip.OCRA & 0xFF);
         else                  
            sh->onchip.OCRB = (val << 8) | (sh->onchip.OCRB & 0xFF);
         return;
      case 0x015:
         if (!(sh->onchip.TOCR & 0x10))
            sh->onchip.OCRA = (sh->onchip.OCRA & 0xFF00) | val;
         else
            sh->onchip.OCRB = (sh->onchip.OCRB & 0xFF00) | val;
         return;
      case 0x016:
         sh->onchip.TCR = val & 0x83;

         switch (val & 3)
         {
            case 0:
               sh->frc.shift = 3;
               break;
            case 1:
               sh->frc.shift = 5;
               break;
            case 2:
               sh->frc.shift = 7;
               break;
            case 3:
               LOG("FRT external input clock not implemented.\n");
               break;
         }
         return;
      case 0x017:
         sh->onchip.TOCR = 0xE0 | (val & 0x13);
         return;
      case 0x060:
         sh->onchip.IPRB = (val << 8);
         return;
      case 0x061:
         return;
      case 0x062:
         sh->onchip.VCRA = ((val & 0x7F) << 8) | (sh->onchip.VCRA & 0x00FF);
         return;
      case 0x063:
         sh->onchip.VCRA = (sh->onchip.VCRA & 0xFF00) | (val & 0x7F);
         return;
      case 0x064:
         sh->onchip.VCRB = ((val & 0x7F) << 8) | (sh->onchip.VCRB & 0x00FF);
         return;
      case 0x065:
         sh->onchip.VCRB = (sh->onchip.VCRB & 0xFF00) | (val & 0x7F);
         return;
      case 0x066:
         sh->onchip.VCRC = ((val & 0x7F) << 8) | (sh->onchip.VCRC & 0x00FF);
         return;
      case 0x067:
         sh->onchip.VCRC = (sh->onchip.VCRC & 0xFF00) | (val & 0x7F);
         return;
      case 0x068:
         sh->onchip.VCRD = (val & 0x7F) << 8;
         return;
      case 0x069:
         return;
      case 0x071:
         sh->onchip.DRCR0 = val & 0x3;
         return;
      case 0x072:
         sh->onchip.DRCR1 = val & 0x3;
         return;
      case 0x091:
         sh->onchip.SBYCR = val & 0xDF;
         return;
      case 0x092:
         sh->onchip.CCR = val & 0xCF;
		 if (val & 0x10){
			 cache_clear(&sh->onchip.cache);
		 }
		 if ( (sh->onchip.CCR & 0x01)  ){
			 cache_enable(&sh->onchip.cache);
		 }
		 else{
			 cache_disable(&sh->onchip.cache);
		 }
         return;
      case 0x0E0:
         sh->onchip.ICR = ((val & 0x1) << 8) | (sh->onchip.ICR & 0xFEFF);
         return;
      case 0x0E1:
         sh->onchip.ICR = (sh->onchip.ICR & 0xFFFE) | (val & 0x1);
         return;
      case 0x0E2:
         sh->onchip.IPRA = (val << 8) | (sh->onchip.IPRA & 0x00FF);
         return;
      case 0x0E3:
         sh->onchip.IPRA = (sh->onchip.IPRA & 0xFF00) | (val & 0xF0);
         return;
      case 0x0E4:
         sh->onchip.VCRWDT = ((val & 0x7F) << 8) | (sh->onchip.VCRWDT & 0x00FF);
         return;
      case 0x0E5:
         sh->onchip.VCRWDT = (sh->onchip.VCRWDT & 0xFF00) | (val & 0x7F);
         return;
      default:
         LOG("Unhandled Onchip byte write %08X\n", (int)addr);
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL OnchipWriteWord(SH2_struct *sh, u32 addr, u16 val) {
   switch(addr)
   {
      case 0x060:
         sh->onchip.IPRB = val & 0xFF00;
         return;
      case 0x062:
         sh->onchip.VCRA = val & 0x7F7F;
         return;
      case 0x064:
         sh->onchip.VCRB = val & 0x7F7F;
         return;
      case 0x066:
         sh->onchip.VCRC = val & 0x7F7F;
         return;
      case 0x068:
         sh->onchip.VCRD = val & 0x7F7F;
         return;
      case 0x080:
         // This and RSTCSR have got to be the most wackiest register
         // mappings I've ever seen

         if (val >> 8 == 0xA5)
         {
            // WTCSR
            switch (val & 7)
            {
               case 0:
                  sh->wdt.shift = 1;
                  break;
               case 1:
                  sh->wdt.shift = 6;
                  break;
               case 2:
                  sh->wdt.shift = 7;
                  break;
               case 3:
                  sh->wdt.shift = 8;
                  break;
               case 4:
                  sh->wdt.shift = 9;
                  break;
               case 5:
                  sh->wdt.shift = 10;
                  break;
               case 6:
                  sh->wdt.shift = 12;
                  break;
               case 7:
                  sh->wdt.shift = 13;
                  break;
            }

            sh->wdt.isenable = (val & 0x20);
            sh->wdt.isinterval = (~val & 0x40);

            sh->onchip.WTCSR = (u8)val | 0x18;
         }
         else if (val >> 8 == 0x5A)
         {
            // WTCNT
            sh->onchip.WTCNT = (u8)val;
         }
         return;
      case 0x082:
         if (val == 0xA500)
            // clear WOVF bit
            sh->onchip.RSTCSR &= 0x7F;
         else if (val >> 8 == 0x5A)
            // RSTE and RSTS bits
            sh->onchip.RSTCSR = (sh->onchip.RSTCSR & 0x80) | (val & 0x60) | 0x1F;
         return;
      case 0x092:
         sh->onchip.CCR = val & 0xCF;
		 if (val&0x10){
			 cache_clear( &sh->onchip.cache );
		 }
		 if ((sh->onchip.CCR & 0x01)){
			 cache_enable(&sh->onchip.cache);
		 }
		 else{
			 cache_disable(&sh->onchip.cache);
		 }
         return;
      case 0x0E0:
         sh->onchip.ICR = val & 0x0101;
         return;
      case 0x0E2:
         sh->onchip.IPRA = val & 0xFFF0;
         return;
      case 0x0E4:
         sh->onchip.VCRWDT = val & 0x7F7F;
         return;
      case 0x108:
      case 0x128:
         sh->onchip.DVCR = val & 0x3;
         return;
      case 0x148:
         sh->onchip.BBRA = val & 0xFF;
         return;
      case 0x178:
         sh->onchip.BRCR = val & 0xF4DC;
         return;
      default:
         LOG("Unhandled Onchip word write %08X(%04X)\n", (int)addr, val);
         return;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL OnchipWriteLong(SH2_struct *sh, u32 addr, u32 val)  {
   switch (addr)
   {
      case 0x100:
      case 0x120:
         sh->onchip.DVSR = val;
         return;
      case 0x104: // 32-bit / 32-bit divide operation
      case 0x124:
      {
         s32 divisor = (s32) sh->onchip.DVSR;
         if (divisor == 0)
         {
            // Regardless of what DVDNTL is set to, the top 3 bits
            // are used to create the new DVDNTH value
            if (val & 0x80000000)
            {
               sh->onchip.DVDNTL = 0x80000000;
               sh->onchip.DVDNTH = 0xFFFFFFFC | ((val >> 29) & 0x3);
            }
            else
            {
               sh->onchip.DVDNTL = 0x7FFFFFFF;
               sh->onchip.DVDNTH = 0 | (val >> 29);
            }
            sh->onchip.DVDNTUL = sh->onchip.DVDNTL;
            sh->onchip.DVDNTUH = sh->onchip.DVDNTH;
            sh->onchip.DVCR |= 1;

            if (sh->onchip.DVCR & 0x2)
               SH2SendInterrupt(sh, sh->onchip.VCRDIV & 0x7F, (MSH2->onchip.IPRA >> 12) & 0xF);
         }
         else
         {
            s32 quotient = ((s32) val) / divisor;
            s32 remainder = ((s32) val) % divisor;
            sh->onchip.DVDNTL = quotient;
            sh->onchip.DVDNTUL = quotient;
            sh->onchip.DVDNTH = remainder;
            sh->onchip.DVDNTUH = remainder;
         }
         return;
      }
      case 0x108:
      case 0x128:
         sh->onchip.DVCR = val & 0x3;
         return;
      case 0x10C:
      case 0x12C:
         sh->onchip.VCRDIV = val & 0xFFFF;
         return;
      case 0x110:
      case 0x130:
         sh->onchip.DVDNTH = val;
         return;
      case 0x114:
      case 0x134: { // 64-bit / 32-bit divide operation
         s32 divisor = (s32) sh->onchip.DVSR;
         s64 dividend = sh->onchip.DVDNTH;
         dividend <<= 32;
         dividend |= val;

         if (divisor == 0)
         {
            if (sh->onchip.DVDNTH & 0x80000000)
            {
               sh->onchip.DVDNTL = 0x80000000;
               sh->onchip.DVDNTH = sh->onchip.DVDNTH << 3; // fix me
            }
            else
            {
               sh->onchip.DVDNTL = 0x7FFFFFFF;
               sh->onchip.DVDNTH = sh->onchip.DVDNTH << 3; // fix me
            }

            sh->onchip.DVDNTUL = sh->onchip.DVDNTL;
            sh->onchip.DVDNTUH = sh->onchip.DVDNTH;
            sh->onchip.DVCR |= 1;

            if (sh->onchip.DVCR & 0x2)
               SH2SendInterrupt(sh, sh->onchip.VCRDIV & 0x7F, (MSH2->onchip.IPRA >> 12) & 0xF);
         }
         else
         {
            s64 quotient = dividend / divisor;
            s32 remainder = dividend % divisor;

            if (quotient > 0x7FFFFFFF)
            {
               sh->onchip.DVCR |= 1;
               sh->onchip.DVDNTL = 0x7FFFFFFF;
               sh->onchip.DVDNTH = 0xFFFFFFFE; // fix me

               if (sh->onchip.DVCR & 0x2)
                  SH2SendInterrupt(sh, sh->onchip.VCRDIV & 0x7F, (MSH2->onchip.IPRA >> 12) & 0xF);
            }
            else if ((s32)(quotient >> 32) < -1)
            {
               sh->onchip.DVCR |= 1;
               sh->onchip.DVDNTL = 0x80000000;
               sh->onchip.DVDNTH = 0xFFFFFFFE; // fix me

               if (sh->onchip.DVCR & 0x2)
                  SH2SendInterrupt(sh, sh->onchip.VCRDIV & 0x7F, (MSH2->onchip.IPRA >> 12) & 0xF);
            }
            else
            {
               sh->onchip.DVDNTL = quotient;
               sh->onchip.DVDNTH = remainder;
            }

            sh->onchip.DVDNTUL = sh->onchip.DVDNTL;
            sh->onchip.DVDNTUH = sh->onchip.DVDNTH;
         }
         return;
      }
      case 0x118:
      case 0x138:
         sh->onchip.DVDNTUH = val;
         return;
      case 0x11C:
      case 0x13C:
         sh->onchip.DVDNTUL = val;
         return;
      case 0x140:
         sh->onchip.BARA.all = val;         
         return;
      case 0x144:
         sh->onchip.BAMRA.all = val;         
         return;
      case 0x180:
         sh->onchip.SAR0 = val;
         return;
      case 0x184:
         sh->onchip.DAR0 = val;
         return;
      case 0x188:
         sh->onchip.TCR0 = val & 0xFFFFFF;
         return;
      case 0x18C:
         sh->onchip.CHCR0 = val & 0xFFFF;

         // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
         // and CHCR's DE bit is set and TE bit is cleared,
         // do a dma transfer
         if ((sh->onchip.DMAOR & 7) == 1 && (val & 0x3) == 1)
         {
            if(yabsys.use_sh2_dma_timing)
                sh->onchip.dma0_active = 1;
            else
                DMAExec(sh);
         }
         return;
      case 0x190:
         sh->onchip.SAR1 = val;
         return;
      case 0x194:
         sh->onchip.DAR1 = val;
         return;
      case 0x198:
         sh->onchip.TCR1 = val & 0xFFFFFF;
         return;
      case 0x19C:
         sh->onchip.CHCR1 = val & 0xFFFF;

         // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
         // and CHCR's DE bit is set and TE bit is cleared,
         // do a dma transfer
         if ((sh->onchip.DMAOR & 7) == 1 && (val & 0x3) == 1)
         {
            if(yabsys.use_sh2_dma_timing)
                sh->onchip.dma1_active = 1;
            else
                DMAExec(sh);
         }
         return;
      case 0x1A0:
         sh->onchip.VCRDMA0 = val & 0xFFFF;
         return;
      case 0x1A8:
         sh->onchip.VCRDMA1 = val & 0xFFFF;
         return;
      case 0x1B0:
         sh->onchip.DMAOR = val & 0xF;

         // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
         // and CHCR's DE bit is set and TE bit is cleared,
         // do a dma transfer
         if ((val & 7) == 1)
         {
            if(yabsys.use_sh2_dma_timing)
            {
                if((sh->onchip.CHCR0 & 0x3) == 1)
                   sh->onchip.dma0_active = 1;
                if ((sh->onchip.CHCR1 & 0x3) == 1)
                   sh->onchip.dma1_active = 1;
            }
            else
            {
                DMAExec(sh);
            }
         }
         return;
      case 0x1E0:
         sh->onchip.BCR1 &= 0x8000;
         sh->onchip.BCR1 |= val & 0x1FF7;
         return;
      case 0x1E4:
         sh->onchip.BCR2 = val & 0xFC;
         return;
      case 0x1E8:
         sh->onchip.WCR = val;
         return;
      case 0x1EC:
         sh->onchip.MCR = val & 0xFEFC;
         return;
      case 0x1F0:
         sh->onchip.RTCSR = val & 0xF8;
         return;
      case 0x1F8:
         sh->onchip.RTCOR = val & 0xFF;
         return;
      default:
         LOG("Unhandled Onchip long write %08X\n", (int)addr);
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL AddressArrayReadLong(SH2_struct *sh, u32 addr) {
   if (yabsys.sh2_cache_enabled)
   {
      int way = (sh->onchip.CCR >> 6) & 3;
      int entry = (addr & 0x3FC) >> 4;
      u32 data = sh->onchip.cache.way[way][entry].tag;
      data |= sh->onchip.cache.lru[entry] << 4;
      data |= sh->onchip.cache.way[way][entry].v << 2;
      return data;
   }
   else
      return sh->AddressArray[(addr & 0x3FC) >> 2];
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL AddressArrayWriteLong(SH2_struct *sh, u32 addr, u32 val)  {
   if (yabsys.sh2_cache_enabled)
   {
      int way = (sh->onchip.CCR >> 6) & 3;
      int entry = (addr & 0x3FC) >> 4;
      sh->onchip.cache.way[way][entry].tag = addr & 0x1FFFFC00;
      sh->onchip.cache.way[way][entry].v = (addr >> 2) & 1;
      sh->onchip.cache.lru[entry] = (val >> 4) & 0x3f;
   }
   else
      sh->AddressArray[(addr & 0x3FC) >> 2] = val;
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL DataArrayReadByte(SH2_struct *sh, u32 addr) {
   if (yabsys.sh2_cache_enabled)
   {
      int way = (addr >> 10) & 3;
      int entry = (addr >> 4) & 0x3f;
      return sh->onchip.cache.way[way][entry].data[addr & 0xf];
   }
   else
      return T2ReadByte(sh->DataArray, addr & 0xFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL DataArrayReadWord(SH2_struct *sh, u32 addr) {
   if (yabsys.sh2_cache_enabled)
   {
      int way = (addr >> 10) & 3;
      int entry = (addr >> 4) & 0x3f;
      return ((u16)(sh->onchip.cache.way[way][entry].data[addr & 0xf]) << 8) | sh->onchip.cache.way[way][entry].data[(addr & 0xf) + 1];
   }
   else
      return T2ReadWord(sh->DataArray, addr & 0xFFF);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL DataArrayReadLong(SH2_struct *sh, u32 addr) {
   if (yabsys.sh2_cache_enabled)
   {
      int way = (addr >> 10) & 3;
      int entry = (addr >> 4) & 0x3f;
      u32 data = ((u32)(sh->onchip.cache.way[way][entry].data[addr & 0xf]) << 24) |
         ((u32)(sh->onchip.cache.way[way][entry].data[(addr & 0xf) + 1]) << 16) |
         ((u32)(sh->onchip.cache.way[way][entry].data[(addr & 0xf) + 2]) << 8) |
         ((u32)(sh->onchip.cache.way[way][entry].data[(addr & 0xf) + 3]) << 0);
      return data;
   }
   else
      return T2ReadLong(sh->DataArray, addr & 0xFFF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DataArrayWriteByte(SH2_struct *sh, u32 addr, u8 val)  {
   if (yabsys.sh2_cache_enabled)
   {
      int way = (addr >> 10) & 3;
      int entry = (addr >> 4) & 0x3f;
      sh->onchip.cache.way[way][entry].data[addr & 0xf] = val;
   }
   else
      T2WriteByte(sh->DataArray, addr & 0xFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DataArrayWriteWord(SH2_struct *sh, u32 addr, u16 val)  {
   if (yabsys.sh2_cache_enabled)
   {
      int way = (addr >> 10) & 3;
      int entry = (addr >> 4) & 0x3f;
      sh->onchip.cache.way[way][entry].data[addr & 0xf] = val >> 8;
      sh->onchip.cache.way[way][entry].data[(addr & 0xf) + 1] = val;
   }
   else
      T2WriteWord(sh->DataArray, addr & 0xFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DataArrayWriteLong(SH2_struct *sh, u32 addr, u32 val)  {
   if (yabsys.sh2_cache_enabled)
   {
      int way = (addr >> 10) & 3;
      int entry = (addr >> 4) & 0x3f;
      sh->onchip.cache.way[way][entry].data[(addr & 0xf)] = ((val >> 24) & 0xFF);
      sh->onchip.cache.way[way][entry].data[(addr & 0xf) + 1] = ((val >> 16) & 0xFF);
      sh->onchip.cache.way[way][entry].data[(addr & 0xf) + 2] = ((val >> 8) & 0xFF);
      sh->onchip.cache.way[way][entry].data[(addr & 0xf) + 3] = ((val >> 0) & 0xFF);
   }
   else
      T2WriteLong(sh->DataArray, addr & 0xFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FRTExec(SH2_struct *sh, u32 cycles)
{
   u32 frcold;
   u32 frctemp;
   u32 mask;

   frcold = frctemp = (u32)sh->onchip.FRC.all;
   mask = (1 << sh->frc.shift) - 1;
   
   // Increment FRC
   frctemp += ((cycles + sh->frc.leftover) >> sh->frc.shift);
   sh->frc.leftover = (cycles + sh->frc.leftover) & mask;

   // Check to see if there is or was a Output Compare A match
   if (frctemp >= sh->onchip.OCRA && frcold < sh->onchip.OCRA)
   {
      // Do we need to trigger an interrupt?
      if (sh->onchip.TIER & 0x8)
         SH2SendInterrupt(sh, sh->onchip.VCRC & 0x7F, (sh->onchip.IPRB & 0xF00) >> 8);

      // Do we need to clear the FRC?
      if (sh->onchip.FTCSR & 0x1)
      {
         frctemp = 0;
         sh->frc.leftover = 0;
      }

      // Set OCFA flag
      sh->onchip.FTCSR |= 0x8;
   }

   // Check to see if there is or was a Output Compare B match
   if (frctemp >= sh->onchip.OCRB && frcold < sh->onchip.OCRB)
   {
      // Do we need to trigger an interrupt?
      if (sh->onchip.TIER & 0x4)
         SH2SendInterrupt(sh, sh->onchip.VCRC & 0x7F, (sh->onchip.IPRB & 0xF00) >> 8);

      // Set OCFB flag
      sh->onchip.FTCSR |= 0x4;
   }

   // If FRC overflows, set overflow flag
   if (frctemp > 0xFFFF)
   {
      // Do we need to trigger an interrupt?
      if (sh->onchip.TIER & 0x2)
         SH2SendInterrupt(sh, (sh->onchip.VCRD >> 8) & 0x7F, (sh->onchip.IPRB & 0xF00) >> 8);

      sh->onchip.FTCSR |= 2;
   }

   // Write new FRC value
   sh->onchip.FRC.all = frctemp;
}

//////////////////////////////////////////////////////////////////////////////

void WDTExec(SH2_struct *sh, u32 cycles) {
   u32 wdttemp;
   u32 mask;

   if (!sh->wdt.isenable || sh->onchip.WTCSR & 0x80 || sh->onchip.RSTCSR & 0x80)
      return;

   wdttemp = (u32)sh->onchip.WTCNT;
   mask = (1 << sh->wdt.shift) - 1;
   wdttemp += ((cycles + sh->wdt.leftover) >> sh->wdt.shift);
   sh->wdt.leftover = (cycles + sh->wdt.leftover) & mask;

   // Are we overflowing?
   if (wdttemp > 0xFF)
   {
      // Obviously depending on whether or not we're in Watchdog or Interval
      // Modes, they'll handle an overflow differently.

      if (sh->wdt.isinterval)
      {
         // Interval Timer Mode

         // Set OVF flag
         sh->onchip.WTCSR |= 0x80;

         // Trigger interrupt
         SH2SendInterrupt(sh, (sh->onchip.VCRWDT >> 8) & 0x7F, (sh->onchip.IPRA >> 4) & 0xF);
      }
      else
      {
         // Watchdog Timer Mode(untested)
         LOG("Watchdog timer(WDT mode) overflow not implemented\n");
      }
   }

   // Write new WTCNT value
   sh->onchip.WTCNT = (u8)wdttemp;
}

//////////////////////////////////////////////////////////////////////////////

u32 sh2_dma_access(u32 addr, u32 data, int is_read, int size)
{
   addr &= 0xfffffff;

   if (addr <= 0x00fffff)
   {
      //bios
      if (is_read)
      {
         if (size == 0)
            return BiosRomMemoryReadByte(addr);
         else if (size == 1)
            return BiosRomMemoryReadWord(addr);
         else
            return BiosRomMemoryReadLong(addr);
      }
      else
      {
         if (size == 0)
            BiosRomMemoryWriteByte(addr, data);
         else if (size == 1)
            BiosRomMemoryWriteWord(addr, data);
         else
            BiosRomMemoryWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x0100000 && addr <= 0x017ffff)
   {
      //smpc
      if (is_read)
      {
         if (size == 0)
            return SmpcReadByte(MSH2, addr);
         else if (size == 1)
            return SmpcReadWord(MSH2, addr);
         else
            return SmpcReadLong(MSH2, addr);
      }
      else
      {
         if (size == 0)
            SmpcWriteByte(MSH2, addr, data);
         else if (size == 1)
            SmpcWriteWord(MSH2, addr, data);
         else
            SmpcWriteLong(MSH2, addr, data);
         return 0;
      }
   }
   else if (addr >= 0x0180000 && addr <= 0x01fffff)
   {
      //backup ram
      if (is_read)
      {
         if (size == 0)
            return BupRamMemoryReadByte(addr);
         else if (size == 1)
            return BupRamMemoryReadWord(addr);
         else
            return BupRamMemoryReadLong(addr);
      }
      else
      {
         if (size == 0)
            BupRamMemoryWriteByte(addr, data);
         else if (size == 1)
            BupRamMemoryWriteWord(addr, data);
         else
            BupRamMemoryWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x0200000 && addr <= 0x02fffff)
   {
      //low wram
      if (is_read)
      {
         if (size == 0)
            return LowWramMemoryReadByte(addr);
         else if (size == 1)
            return LowWramMemoryReadWord(addr);
         else
            return LowWramMemoryReadLong(addr);
      }
      else
      {
         if (size == 0)
            LowWramMemoryWriteByte(addr, data);
         else if (size == 1)
            LowWramMemoryWriteWord(addr, data);
         else
            LowWramMemoryWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x1000000 && addr <= 0x17fffff)
   {
      //ssh2 input capture
      if (is_read)
      {
         if (size == 0)
            return UnhandledMemoryReadByte(addr);
         else if (size == 1)
            return UnhandledMemoryReadWord(addr);
         else
            return UnhandledMemoryReadLong(addr);
      }
      else
      {
         if (size == 0)
            UnhandledMemoryWriteByte(addr, data);
         else if (size == 1)
            SSH2InputCaptureWriteWord(MSH2, addr, data);
         else
            UnhandledMemoryWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x1800000 && addr <= 0x1ffffff)
   {
      //msh2 input capture
      if (is_read)
      {
         if (size == 0)
            return UnhandledMemoryReadByte(addr);
         else if (size == 1)
            return UnhandledMemoryReadWord(addr);
         else
            return UnhandledMemoryReadLong(addr);
      }
      else
      {
         if (size == 0)
            UnhandledMemoryWriteByte(addr, data);
         else if (size == 1)
            MSH2InputCaptureWriteWord(MSH2, addr, data);
         else
            UnhandledMemoryWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x2000000 && addr <= 0x3ffffff)
   {
      //cs0
      if (is_read)
      {
         if(size == 0)
            return CartridgeArea->Cs0ReadByte(MSH2, addr);
         else if (size == 1)
            return CartridgeArea->Cs0ReadWord(MSH2, addr);
         else
            return CartridgeArea->Cs0ReadLong(MSH2, addr);
      }
      else
      {
         if(size == 0)
            CartridgeArea->Cs0WriteByte(MSH2, addr, data);
         else if (size == 1)
            CartridgeArea->Cs0WriteWord(MSH2, addr, data);
         else
            CartridgeArea->Cs0WriteLong(MSH2, addr, data);
         return 0;
      }
   }
   else if (addr >= 0x4000000 && addr <= 0x4ffffff)
   {
      if (is_read)
      {
         if (size == 0)
            return Cs1ReadByte(MSH2, addr);
         else if (size == 1)
            return Cs1ReadWord(MSH2, addr);
         else
            return Cs1ReadLong(MSH2, addr);
      }
      else
      {
         if (size == 0)
            Cs1WriteByte(MSH2, addr, data);
         else if (size == 1)
            Cs1WriteWord(MSH2, addr, data);
         else
            Cs1WriteLong(MSH2, addr, data);
         return 0;
      }
   }
   else if (addr >= 0x5000000 && addr <= 0x57fffff)
   {
      //dummy
   }
   else if (addr >= 0x5800000 && addr <= 0x58fffff)
   {
      //cs2
      if (yabsys.use_cd_block_lle)
      {
         if (is_read)
         {
            if (size == 0)
               return Cs2ReadByte(MSH2, addr);
            else if (size == 1)
               return ygr_a_bus_read_word(addr);
            else
               return ygr_a_bus_read_long(addr);
         }
         else
         {
            if (size == 0)
               Cs2WriteByte(MSH2, addr, data);
            else if (size == 1)
               ygr_a_bus_write_word(addr, data);
            else
               ygr_a_bus_write_long(addr, data);
            return 0;
         }
      }
      else
      {
         if (is_read)
         {
            if (size == 0)
               return Cs2ReadByte(MSH2, addr);
            else if (size == 1)
               return Cs2ReadWord(MSH2, addr);
            else
               return Cs2ReadLong(MSH2, addr);
         }
         else
         {
            if (size == 0)
               Cs2WriteByte(MSH2, addr, data);
            else if (size == 1)
               Cs2WriteWord(MSH2, addr, data);
            else
               Cs2WriteLong(MSH2, addr, data);
            return 0;
         }
      }
   }
   else if (addr >= 0x5a00000 && addr <= 0x5afffff)
   {
      //sound ram
      if (is_read)
      {
         if (size == 0)
            return SoundRamReadByte(addr);
         else if (size == 1)
            return SoundRamReadWord(addr);
         else
            return SoundRamReadLong(addr);
      }
      else
      {
         if (size == 0)
            SoundRamWriteByte(addr, data);
         else if (size == 1)
            SoundRamWriteWord(addr, data);
         else
            SoundRamWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x5b00000 && addr <= 0x5bfffff)
   {
      //scsp regs
      if (is_read)
      {
         if (size == 0)
            return ScspReadByte(addr);
         else if (size == 1)
            return ScspReadWord(addr);
         else
            return ScspReadLong(addr);
      }
      else
      {
         if (size == 0)
            ScspWriteByte(addr, data);
         else if (size == 1)
            ScspWriteWord(addr, data);
         else
            ScspWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x5c00000 && addr <= 0x5c7ffff)
   {
      //vdp1 ram
      if (is_read)
      {
         if (size == 0)
            return Vdp1RamReadByte(addr);
         else if (size == 1)
            return Vdp1RamReadWord(addr);
         else
            return Vdp1RamReadLong(addr);
      }
      else
      {
         if (size == 0)
            Vdp1RamWriteByte(addr, data);
         else if (size == 1)
            Vdp1RamWriteWord(addr, data);
         else
            Vdp1RamWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x5c80000 && addr <= 0x5cfffff)
   {
      //vdp1 framebuffer
      if (is_read)
      {
         if (size == 0)
            return Vdp1FrameBufferReadByte(addr);
         else if (size == 1)
            return Vdp1FrameBufferReadWord(addr);
         else
            return Vdp1FrameBufferReadLong(addr);
      }
      else
      {
         if (size == 0)
            Vdp1FrameBufferWriteByte(addr, data);
         else if (size == 1)
            Vdp1FrameBufferWriteWord(addr, data);
         else
            Vdp1FrameBufferWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x5d00000 && addr <= 0x5d7ffff)
   {
      //vdp1 registers
      if (is_read)
      {
         if (size == 0)
            return Vdp1ReadByte(addr);
         else if (size == 1)
            return Vdp1ReadWord(addr);
         else
            return Vdp1ReadLong(addr);
      }
      else
      {
         if (size == 0)
            Vdp1WriteByte(addr, data);
         else if (size == 1)
            Vdp1WriteWord(addr, data);
         else
            Vdp1WriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x5e00000 && addr <= 0x5efffff)
   {
      //vdp2 ram
      if (is_read)
      {
         if (size == 0)
            return Vdp2RamReadByte(addr);
         else if (size == 1)
            return Vdp2RamReadWord(addr);
         else
            return Vdp2RamReadLong(addr);
      }
      else
      {
         if (size == 0)
            Vdp2RamWriteByte(addr, data);
         else if (size == 1)
            Vdp2RamWriteWord(addr, data);
         else
            Vdp2RamWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x5f00000 && addr <= 0x5f7ffff)
   {
      //vdp2 color ram
      if (is_read)
      {
         if (size == 0)
            return Vdp2ColorRamReadByte(addr);
         else if (size == 1)
            return Vdp2ColorRamReadWord(addr);
         else
            return Vdp2ColorRamReadLong(addr);
      }
      else
      {
         if (size == 0)
            Vdp2ColorRamWriteByte(addr, data);
         else if (size == 1)
            Vdp2ColorRamWriteWord(addr, data);
         else
            Vdp2ColorRamWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x5f80000 && addr <= 0x5fbffff)
   {
      //vdp2 registers
      if (is_read)
      {
         if (size == 0)
            return Vdp2ReadByte(addr);
         else if (size == 1)
            return Vdp2ReadWord(addr);
         else
            return Vdp2ReadLong(addr);
      }
      else
      {
         if (size == 0)
            Vdp2WriteByte(addr, data);
         else if (size == 1)
            Vdp2WriteWord(addr, data);
         else
            Vdp2WriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x5fe0000 && addr <= 0x5feffff)
   {
      //scu registers
      if (is_read)
      {
         if (size == 0)
            return ScuReadByte(addr);
         else if (size == 1)
            return ScuReadWord(addr);
         else
            return ScuReadLong(addr);
      }
      else
      {
         if (size == 0)
            ScuWriteByte(addr, data);
         else if (size == 1)
            ScuWriteWord(addr, data);
         else
            ScuWriteLong(addr, data);
         return 0;
      }
   }
   else if (addr >= 0x6000000 && addr <= 0x7ffffff)
   {
      //scu registers
      if (is_read)
      {
         if (size == 0)
            return HighWramMemoryReadByte(addr);
         else if (size == 1)
            return HighWramMemoryReadWord(addr);
         else
            return HighWramMemoryReadLong(addr);
      }
      else
      {
         if (size == 0)
            HighWramMemoryWriteByte(addr, data);
         else if (size == 1)
            HighWramMemoryWriteWord(addr, data);
         else
            HighWramMemoryWriteLong(addr, data);
         return 0;
      }
   }
   assert(0);
   return 0;
}

int sh2_check_wait(SH2_struct * sh, u32 addr, int size);

void dma_tick(SH2_struct *sh, u32 *CHCR, u32 *SAR, u32 *DAR, u32 *TCR, u32 *VCRDMA, int * active)
{
   int src_increment = 0;
   int dst_increment = 0;
   u8 dest_inc_mode = (*CHCR >> 14) & 3;
   u8 src_inc_mode = (*CHCR >> 12) & 3;
   u8 size = (*CHCR >> 10) & 3;
   int check_size = size;

   if (check_size > 2)
      check_size = 2;//size 3 is also long size writes

   if (sh2_check_wait(sh, *SAR, check_size))
      return;

   if (dest_inc_mode == 1)
      dst_increment = 1;
   else if (dest_inc_mode == 2)
      dst_increment = -1;

   if (src_inc_mode == 1)
      src_increment = 1;
   else if (src_inc_mode == 2)
      src_increment = -1;

   if (size == 0)
   {
      u8 source_val = sh2_dma_access(*SAR,0,1,0);
      sh2_dma_access(*DAR, source_val,0,0);
   }
   else if (size == 1)
   {
      u16 source_val = sh2_dma_access(*SAR, 0, 1, 1);
      sh2_dma_access(*DAR, source_val, 0, 1);
      src_increment *= 2;
      dst_increment *= 2;
   }
   else if (size == 2 || size == 3)
   {
      u32 source_val = sh2_dma_access(*SAR, 0, 1, 2);
      sh2_dma_access(*DAR, source_val, 0, 2);
      src_increment *= 4;
      dst_increment *= 4;
   }

   if(dst_increment > 0)
      SH2WriteNotify(*DAR, *DAR + dst_increment);
   else
      SH2WriteNotify(*DAR + dst_increment, *DAR);

   *TCR = *TCR - 1;
   *SAR = *SAR + src_increment;
   *DAR = *DAR + dst_increment;

   if (*TCR == 0)
   {
      *active = 0;

      if ((*CHCR >> 2) & 1)
         SH2SendInterrupt(sh, *VCRDMA, (sh->onchip.IPRA & 0xF00) >> 8);

      *CHCR |= 0x2;
   }
}

void tick_dma0(SH2_struct *sh)
{
   dma_tick(
      sh,
      &sh->onchip.CHCR0,
      &sh->onchip.SAR0,
      &sh->onchip.DAR0,
      &sh->onchip.TCR0,
      &sh->onchip.VCRDMA0,
      &sh->onchip.dma0_active);
}

void tick_dma1(SH2_struct *sh)
{
   dma_tick(
      sh,
      &sh->onchip.CHCR1,
      &sh->onchip.SAR1,
      &sh->onchip.DAR1,
      &sh->onchip.TCR1,
      &sh->onchip.VCRDMA1,
      &sh->onchip.dma1_active);//channel 0
}

void sh2_dma_exec(SH2_struct *sh, u32 cycles) {
   int i;

   if (!sh->onchip.dma0_active && !sh->onchip.dma1_active)
      return;

   for (i = 0; i < cycles; i++)
   {
      if ((sh->onchip.DMAOR >> 3) & 1)//round robin priority
      {
         //both channels are active, round robin
         if (sh->onchip.dma0_active && sh->onchip.dma1_active)
         {
            if (sh->onchip.dma_robin)
            {
               sh->onchip.dma_robin = 0;
               tick_dma0(sh);
            }
            else
            {
               tick_dma1(sh);//channel 1
            }
         }
         else
         {
            //only one channel is active
            if (sh->onchip.dma0_active)
               tick_dma0(sh);
            if (sh->onchip.dma1_active)
               tick_dma1(sh);
         }
      }
      else
      {
         //channel 0 > channel 1

         if(sh->onchip.dma0_active)
            tick_dma0(sh);//channel 0
         else if (sh->onchip.dma1_active)
            tick_dma1(sh);//channel 1
      }
   }
}


void DMAExec(SH2_struct *sh) {
   // If AE and NMIF bits are set, we can't continue
   if (sh->onchip.DMAOR & 0x6)
      return;

   if ( ((sh->onchip.CHCR0 & 0x3)==0x01)  && ((sh->onchip.CHCR1 & 0x3)==0x01) ) { // both channel wants DMA
      if (sh->onchip.DMAOR & 0x8) { // round robin priority
         LOG("dma\t: FIXME: two channel dma - round robin priority not properly implemented\n");
         DMATransfer(sh, &sh->onchip.CHCR0, &sh->onchip.SAR0,
		     &sh->onchip.DAR0,  &sh->onchip.TCR0,
		     &sh->onchip.VCRDMA0);
         DMATransfer(sh, &sh->onchip.CHCR1, &sh->onchip.SAR1,
		     &sh->onchip.DAR1,  &sh->onchip.TCR1,
                     &sh->onchip.VCRDMA1);
      }
      else { // channel 0 > channel 1 priority
         DMATransfer(sh, &sh->onchip.CHCR0, &sh->onchip.SAR0,
		     &sh->onchip.DAR0,  &sh->onchip.TCR0,
		     &sh->onchip.VCRDMA0);
         DMATransfer(sh, &sh->onchip.CHCR1, &sh->onchip.SAR1,
		     &sh->onchip.DAR1,  &sh->onchip.TCR1,
		     &sh->onchip.VCRDMA1);
      }
   }
   else { // only one channel wants DMA
	   if (((sh->onchip.CHCR0 & 0x3) == 0x01)) { // DMA for channel 0
         DMATransfer(sh, &sh->onchip.CHCR0, &sh->onchip.SAR0,
		     &sh->onchip.DAR0,  &sh->onchip.TCR0,
		     &sh->onchip.VCRDMA0);
         return;
      }
	   if (((sh->onchip.CHCR1 & 0x3) == 0x01)) { // DMA for channel 1
         DMATransfer(sh, &sh->onchip.CHCR1, &sh->onchip.SAR1,
		     &sh->onchip.DAR1,  &sh->onchip.TCR1,
		     &sh->onchip.VCRDMA1);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void DMATransfer(SH2_struct *sh, u32 *CHCR, u32 *SAR, u32 *DAR, u32 *TCR, u32 *VCRDMA)
{
   int size;
   u32 i, i2;

   if (!(*CHCR & 0x2)) { // TE is not set
      int srcInc;
      int destInc;

      switch(*CHCR & 0x3000) {
         case 0x0000: srcInc = 0; break;
         case 0x1000: srcInc = 1; break;
         case 0x2000: srcInc = -1; break;
         default: srcInc = 0; break;
      }

      switch(*CHCR & 0xC000) {
         case 0x0000: destInc = 0; break;
         case 0x4000: destInc = 1; break;
         case 0x8000: destInc = -1; break;
         default: destInc = 0; break;
      }

      switch (size = ((*CHCR & 0x0C00) >> 10)) {
         case 0:
            for (i = 0; i < *TCR; i++) {
				MappedMemoryWriteByteNocache(sh, *DAR, MappedMemoryReadByteNocache(sh, *SAR));
               *SAR += srcInc;
               *DAR += destInc;
            }

            *TCR = 0;
            break;
         case 1:
            destInc *= 2;
            srcInc *= 2;

            for (i = 0; i < *TCR; i++) {
				MappedMemoryWriteWordNocache(sh, *DAR, MappedMemoryReadWordNocache(sh, *SAR));
               *SAR += srcInc;
               *DAR += destInc;
            }

            *TCR = 0;
            break;
         case 2:
            destInc *= 4;
            srcInc *= 4;

            for (i = 0; i < *TCR; i++) {
				MappedMemoryWriteLongNocache(sh, *DAR, MappedMemoryReadLongNocache(sh, *SAR));
               *DAR += destInc;
               *SAR += srcInc;
            }

            *TCR = 0;
            break;
         case 3:
            destInc *= 4;
            srcInc *= 4;

            for (i = 0; i < *TCR; i+=4) {
               for(i2 = 0; i2 < 4; i2++) {
				   MappedMemoryWriteLongNocache(sh, *DAR, MappedMemoryReadLongNocache(sh, *SAR));
                  *DAR += destInc;
                  *SAR += srcInc;
               }
            }

            *TCR = 0;
            break;
      }
      SH2WriteNotify(destInc<0?*DAR:*DAR-i*destInc,i*abs(destInc));
   }

   if (*CHCR & 0x4)
      SH2SendInterrupt(sh, *VCRDMA, (sh->onchip.IPRA & 0xF00) >> 8);

   // Set Transfer End bit
   *CHCR |= 0x2;
}

//////////////////////////////////////////////////////////////////////////////
// Input Capture Specific
//////////////////////////////////////////////////////////////////////////////

void FASTCALL MSH2InputCaptureWriteWord(SH2_struct *sh, UNUSED u32 addr, UNUSED u16 data)
{
   // Set Input Capture Flag
   MSH2->onchip.FTCSR |= 0x80;

   // Copy FRC register to FICR
   MSH2->onchip.FICR = MSH2->onchip.FRC.all;

   // Time for an Interrupt?
   if (MSH2->onchip.TIER & 0x80)
      SH2SendInterrupt(MSH2, (MSH2->onchip.VCRC >> 8) & 0x7F, (MSH2->onchip.IPRB >> 8) & 0xF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SSH2InputCaptureWriteWord(SH2_struct *sh, UNUSED u32 addr, UNUSED u16 data)
{
   // Set Input Capture Flag
   SSH2->onchip.FTCSR |= 0x80;

   // Copy FRC register to FICR
   SSH2->onchip.FICR = SSH2->onchip.FRC.all;

   // Time for an Interrupt?
   if (SSH2->onchip.TIER & 0x80)
      SH2SendInterrupt(SSH2, (SSH2->onchip.VCRC >> 8) & 0x7F, (SSH2->onchip.IPRB >> 8) & 0xF);
}

//////////////////////////////////////////////////////////////////////////////
// SCI Specific
//////////////////////////////////////////////////////////////////////////////

u8 SCIReceiveByte(void) {
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SCITransmitByte(UNUSED u8 val) {
}

//////////////////////////////////////////////////////////////////////////////

int SH2SaveState(SH2_struct *context, FILE *fp)
{
   int offset;
   IOCheck_struct check = { 0, 0 };
   sh2regs_struct regs;

	if (context->model == SHMT_SH1)
	{
		offset = StateWriteHeader(fp, "SH1 ", 1);
	}
	else if (context->model == SHMT_SH2)
	{
		// Write header
		if (context->isslave == 0)
			offset = StateWriteHeader(fp, "MSH2", 1);
		else
		{
			offset = StateWriteHeader(fp, "SSH2", 1);
			ywrite(&check, (void *)&yabsys.IsSSH2Running, 1, 1, fp);
		}
	}

   // Write registers
   SH2GetRegisters(context, &regs);
   ywrite(&check, (void *)&regs, sizeof(sh2regs_struct), 1, fp);

   // Write onchip registers
   ywrite(&check, (void *)&context->onchip, sizeof(Onchip_struct), 1, fp);

   // Write internal variables
   // FIXME: write the clock divisor rather than the shift amount for
   // backward compatibility (fix this next time the save state version
   // is updated)
   context->frc.shift = 1 << context->frc.shift;
   ywrite(&check, (void *)&context->frc, sizeof(context->frc), 1, fp);
   {
      u32 div = context->frc.shift;
      context->frc.shift = 0;
      while ((div >>= 1) != 0)
         context->frc.shift++;
   }
   context->NumberOfInterrupts = SH2Core->GetInterrupts(context, context->interrupts);
   ywrite(&check, (void *)context->interrupts, sizeof(interrupt_struct), MAX_INTERRUPTS, fp);
   ywrite(&check, (void *)&context->NumberOfInterrupts, sizeof(u32), 1, fp);
   ywrite(&check, (void *)context->AddressArray, sizeof(u32), 0x100, fp);
   ywrite(&check, (void *)context->DataArray, sizeof(u8), 0x1000, fp);
   ywrite(&check, (void *)&context->delay, sizeof(u32), 1, fp);
   ywrite(&check, (void *)&context->cycles, sizeof(u32), 1, fp);
   ywrite(&check, (void *)&context->isslave, sizeof(u8), 1, fp);
   ywrite(&check, (void *)&context->isIdle, sizeof(u8), 1, fp);
   ywrite(&check, (void *)&context->instruction, sizeof(u16), 1, fp);

   return StateFinishHeader(fp, offset);
}

//////////////////////////////////////////////////////////////////////////////

int SH2LoadState(SH2_struct *context, FILE *fp, UNUSED int version, int size)
{
   IOCheck_struct check = { 0, 0 };
   sh2regs_struct regs;

   if (context->isslave == 1)
      yread(&check, (void *)&yabsys.IsSSH2Running, 1, 1, fp);

   // Read registers
   yread(&check, (void *)&regs, sizeof(sh2regs_struct), 1, fp);
   SH2SetRegisters(context, &regs);

   // Read onchip registers
   yread(&check, (void *)&context->onchip, sizeof(Onchip_struct), 1, fp);

   // Read internal variables
   yread(&check, (void *)&context->frc, sizeof(context->frc), 1, fp);
   {  // FIXME: backward compatibility hack (see SH2SaveState() comment)
      u32 div = context->frc.shift;
      context->frc.shift = 0;
      while ((div >>= 1) != 0)
         context->frc.shift++;
   }
   yread(&check, (void *)context->interrupts, sizeof(interrupt_struct), MAX_INTERRUPTS, fp);
   yread(&check, (void *)&context->NumberOfInterrupts, sizeof(u32), 1, fp);
   SH2Core->SetInterrupts(context, context->NumberOfInterrupts, context->interrupts);
   yread(&check, (void *)context->AddressArray, sizeof(u32), 0x100, fp);
   yread(&check, (void *)context->DataArray, sizeof(u8), 0x1000, fp);
   yread(&check, (void *)&context->delay, sizeof(u32), 1, fp);
   yread(&check, (void *)&context->cycles, sizeof(u32), 1, fp);
   yread(&check, (void *)&context->isslave, sizeof(u8), 1, fp);
   yread(&check, (void *)&context->isIdle, sizeof(u8), 1, fp);
   yread(&check, (void *)&context->instruction, sizeof(u16), 1, fp);

   #if defined(SH2_DYNAREC)
   if(SH2Core->id==2) {
     invalidate_all_pages();
     if (context->isslave == 1) {
       // If the slave SH2 isn't running, make sure the dynarec stops it
       if(!yabsys.IsSSH2Running) SH2Core->Reset(SSH2);
     }
   }
   #endif

   return size;
}

//////////////////////////////////////////////////////////////////////////////
