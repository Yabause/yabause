/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2005 Theo Berkau

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// SH2 Shared Code
#include <stdlib.h>
#include "sh2core.h"
#include "debug.h"
#include "memory.h"
#include "yabause.h"

SH2_struct *MSH2=NULL;
SH2_struct *SSH2=NULL;
static SH2_struct *CurrentSH2;
SH2Interface_struct *SH2Core=NULL;
extern SH2Interface_struct *SH2CoreList[];

void OnchipReset(SH2_struct *context);
void FRTExec(u32 cycles);
void WDTExec(u32 cycles);
u8 SCIReceiveByte(void);
void SCITransmitByte(u8);

//////////////////////////////////////////////////////////////////////////////

int SH2Init(int coreid)
{
   int i;

   // MSH2
   if ((MSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
      return -1;

   MSH2->onchip.BCR1 = 0x0000;
   MSH2->isslave = 0;

   // SSH2
   if ((SSH2 = (SH2_struct *)calloc(1, sizeof(SH2_struct))) == NULL)
      return -1;

   SSH2->onchip.BCR1 = 0x8000;
   SSH2->isslave = 1;

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

   if (SH2Core)
      SH2Core->Init();

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SH2DeInit()
{
   if (SH2Core)
      SH2Core->DeInit();

   if (MSH2)
      free(MSH2);

   if (SSH2)
      free(SSH2);
}

//////////////////////////////////////////////////////////////////////////////

void SH2Reset(SH2_struct *context)
{
   int i;
   
   // Reset general registers
   for (i = 0; i < 15; i++)
      context->regs.R[i] = 0x00000000;
                   
   context->regs.SR.all = 0x000000F0;
   context->regs.GBR = 0x00000000;
   context->regs.VBR = 0x00000000;
   context->regs.MACH = 0x00000000;
   context->regs.MACL = 0x00000000;
   context->regs.PR = 0x00000000;

   // Internal variables
   context->delay = 0x00000000;
   context->cycles = 0;
   context->isIdle = 0;

   context->frc.leftover = 0;
   context->frc.div = 8;
 
   context->wdt.isenable = 0;
   context->wdt.isinterval = 1;
   context->wdt.div = 2;
   context->wdt.leftover = 0;

   // Reset Interrupts
   memset((void *)context->interrupts, 0, sizeof(interrupt_struct) * MAX_INTERRUPTS);
   context->NumberOfInterrupts = 0;

   // Core specific reset
   SH2Core->Reset();

   // Reset Onchip modules
   OnchipReset(context);
}

//////////////////////////////////////////////////////////////////////////////

void SH2PowerOn(SH2_struct *context) {
   context->regs.PC = MappedMemoryReadLong(context->regs.VBR);
   context->regs.R[15] = MappedMemoryReadLong(context->regs.VBR+4);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL SH2Exec(SH2_struct *context, u32 cycles)
{
   CurrentSH2 = context;

   // Handle interrupts
   if (context->NumberOfInterrupts != 0)
   {
      if (context->interrupts[context->NumberOfInterrupts-1].level > context->regs.SR.part.I)
      {
         context->regs.R[15] -= 4;
         MappedMemoryWriteLong(context->regs.R[15], context->regs.SR.all);
         context->regs.R[15] -= 4;
         MappedMemoryWriteLong(context->regs.R[15], context->regs.PC);
         context->regs.SR.part.I = context->interrupts[context->NumberOfInterrupts-1].level;
         context->regs.PC = MappedMemoryReadLong(context->regs.VBR + (context->interrupts[context->NumberOfInterrupts-1].vector << 2));
         context->NumberOfInterrupts--;
	 context->isIdle = 0;
      }
   }

   SH2Core->Exec(context, cycles);
  
   FRTExec(cycles);
   WDTExec(cycles);

   return 0; // fix me
}

//////////////////////////////////////////////////////////////////////////////

void SH2SendInterrupt(SH2_struct *context, u8 vector, u8 level)
{
   u32 i, i2;
   interrupt_struct tmp;

   // Make sure interrupt doesn't already exist
   for (i = 0; i < context->NumberOfInterrupts; i++)
   {
      if (context->interrupts[i].vector == vector)
         return;
   }

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
   u32 tmp = context->regs.PC;

   // Execute 1 instruction
   SH2Exec(context, context->cycles+1);

   // Sometimes it doesn't always execute one instruction,
   // let's make sure it did
   if (tmp == context->regs.PC)
      SH2Exec(context, context->cycles+1);
}

//////////////////////////////////////////////////////////////////////////////

void SH2GetRegisters(SH2_struct *context, sh2regs_struct * r)
{
   if (r != NULL) {
      memcpy(r, &context->regs, sizeof(context->regs));
   }
}

//////////////////////////////////////////////////////////////////////////////

void SH2SetRegisters(SH2_struct *context, sh2regs_struct * r)
{
   if (r != NULL) {
      memcpy(&context->regs, r, sizeof(context->regs));
   }
}

//////////////////////////////////////////////////////////////////////////////

void SH2SetBreakpointCallBack(SH2_struct *context, void (*func)(void *, u32)) {
   context->BreakpointCallBack = func;
}

//////////////////////////////////////////////////////////////////////////////

int SH2AddCodeBreakpoint(SH2_struct *context, u32 addr) {
   int i;

   if (context->numcodebreakpoints < MAX_BREAKPOINTS) {
      // Make sure it isn't already on the list
      for (i = 0; i < context->numcodebreakpoints; i++)
      {
         if (addr == context->codebreakpoint[i].addr)
            return -1;
      }

      context->codebreakpoint[context->numcodebreakpoints].addr = addr;
      context->numcodebreakpoints++;

      return 0;
   }

   return -1;
}

//////////////////////////////////////////////////////////////////////////////

void SH2SortCodeBreakpoints(SH2_struct *context) {
   int i, i2;
   u32 tmp;

   for (i = 0; i < (MAX_BREAKPOINTS-1); i++)
   {
      for (i2 = i+1; i2 < MAX_BREAKPOINTS; i2++)
      {
         if (context->codebreakpoint[i].addr == 0xFFFFFFFF &&
             context->codebreakpoint[i2].addr != 0xFFFFFFFF)
         {
            tmp = context->codebreakpoint[i].addr;
            context->codebreakpoint[i].addr = context->codebreakpoint[i2].addr;
            context->codebreakpoint[i2].addr = tmp;
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

int SH2DelCodeBreakpoint(SH2_struct *context, u32 addr) {
   int i;

   if (context->numcodebreakpoints > 0) {
      for (i = 0; i < context->numcodebreakpoints; i++) {
         if (context->codebreakpoint[i].addr == addr)
         {
            context->codebreakpoint[i].addr = 0xFFFFFFFF;
            SH2SortCodeBreakpoints(context);
            context->numcodebreakpoints--;
            return 0;
         }
      }
   }

   return -1;
}

//////////////////////////////////////////////////////////////////////////////

codebreakpoint_struct *SH2GetBreakpointList(SH2_struct *context) {
   return context->codebreakpoint;
}

//////////////////////////////////////////////////////////////////////////////

void SH2ClearCodeBreakpoints(SH2_struct *context) {
   int i;
   for (i = 0; i < MAX_BREAKPOINTS; i++) {
      context->codebreakpoint[i].addr = 0xFFFFFFFF;
   }

   context->numcodebreakpoints = 0;
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL SH2MemoryBreakpointReadByte(u32 addr) {
   int i;

   for (i = 0; i < CurrentSH2->nummemorybreakpoints; i++)
   {
      if (CurrentSH2->memorybreakpoint[i].addr == addr)
      {
         if (CurrentSH2->BreakpointCallBack && CurrentSH2->inbreakpoint == 0)
         {
            CurrentSH2->inbreakpoint = 1;
            CurrentSH2->BreakpointCallBack(CurrentSH2, 0);
            CurrentSH2->inbreakpoint = 0;
         }

         return CurrentSH2->memorybreakpoint[i].oldreadbyte(addr);
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < MSH2->nummemorybreakpoints; i++)
   {
      if (((MSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
         return MSH2->memorybreakpoint[i].oldreadbyte(addr);
   }
   for (i = 0; i < SSH2->nummemorybreakpoints; i++)
   {
      if (((SSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
         return SSH2->memorybreakpoint[i].oldreadbyte(addr);
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL SH2MemoryBreakpointReadWord(u32 addr) {
   int i;

   for (i = 0; i < CurrentSH2->nummemorybreakpoints; i++)
   {
      if (CurrentSH2->memorybreakpoint[i].addr == addr)
      {
         if (CurrentSH2->BreakpointCallBack && CurrentSH2->inbreakpoint == 0)
         {
            CurrentSH2->inbreakpoint = 1;
            CurrentSH2->BreakpointCallBack(CurrentSH2, 0);
            CurrentSH2->inbreakpoint = 0;
         }

         return CurrentSH2->memorybreakpoint[i].oldreadword(addr);
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < MSH2->nummemorybreakpoints; i++)
   {
      if (((MSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
         return MSH2->memorybreakpoint[i].oldreadword(addr);
   }
   for (i = 0; i < SSH2->nummemorybreakpoints; i++)
   {
      if (((SSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
         return SSH2->memorybreakpoint[i].oldreadword(addr);
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL SH2MemoryBreakpointReadLong(u32 addr) {
   int i;

   for (i = 0; i < CurrentSH2->nummemorybreakpoints; i++)
   {
      if (CurrentSH2->memorybreakpoint[i].addr == addr)
      {
         if (CurrentSH2->BreakpointCallBack && CurrentSH2->inbreakpoint == 0)
         {
            CurrentSH2->inbreakpoint = 1;
            CurrentSH2->BreakpointCallBack(CurrentSH2, 0);
            CurrentSH2->inbreakpoint = 0;
         }

         return CurrentSH2->memorybreakpoint[i].oldreadlong(addr);
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < MSH2->nummemorybreakpoints; i++)
   {
      if (((MSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
         return MSH2->memorybreakpoint[i].oldreadlong(addr);
   }
   for (i = 0; i < SSH2->nummemorybreakpoints; i++)
   {
      if (((SSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
         return SSH2->memorybreakpoint[i].oldreadlong(addr);
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SH2MemoryBreakpointWriteByte(u32 addr, u8 val) {
   int i;

   for (i = 0; i < CurrentSH2->nummemorybreakpoints; i++)
   {
      if (CurrentSH2->memorybreakpoint[i].addr == addr)
      {
         if (CurrentSH2->BreakpointCallBack && CurrentSH2->inbreakpoint == 0)
         {
            CurrentSH2->inbreakpoint = 1;
            CurrentSH2->BreakpointCallBack(CurrentSH2, 0);
            CurrentSH2->inbreakpoint = 0;
         }

         CurrentSH2->memorybreakpoint[i].oldwritebyte(addr, val);
         return;
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < MSH2->nummemorybreakpoints; i++)
   {
      if (((MSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
      {
         MSH2->memorybreakpoint[i].oldwritebyte(addr, val);
         return;
      }
   }
   for (i = 0; i < SSH2->nummemorybreakpoints; i++)
   {
      if (((SSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
      {
         SSH2->memorybreakpoint[i].oldwritebyte(addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SH2MemoryBreakpointWriteWord(u32 addr, u16 val) {
   int i;

   for (i = 0; i < CurrentSH2->nummemorybreakpoints; i++)
   {
      if (CurrentSH2->memorybreakpoint[i].addr == addr)
      {
         if (CurrentSH2->BreakpointCallBack && CurrentSH2->inbreakpoint == 0)
         {
            CurrentSH2->inbreakpoint = 1;
            CurrentSH2->BreakpointCallBack(CurrentSH2, 0);
            CurrentSH2->inbreakpoint = 0;
         }

         CurrentSH2->memorybreakpoint[i].oldwriteword(addr, val);
         return;
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < MSH2->nummemorybreakpoints; i++)
   {
      if (((MSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
      {
         MSH2->memorybreakpoint[i].oldwriteword(addr, val);
         return;
      }
   }
   for (i = 0; i < SSH2->nummemorybreakpoints; i++)
   {
      if (((SSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
      {
         SSH2->memorybreakpoint[i].oldwriteword(addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SH2MemoryBreakpointWriteLong(u32 addr, u32 val) {
   int i;

   for (i = 0; i < CurrentSH2->nummemorybreakpoints; i++)
   {
      if (CurrentSH2->memorybreakpoint[i].addr == addr)
      {
         if (CurrentSH2->BreakpointCallBack && CurrentSH2->inbreakpoint == 0)
         {
            CurrentSH2->inbreakpoint = 1;
            CurrentSH2->BreakpointCallBack(CurrentSH2, 0);
            CurrentSH2->inbreakpoint = 0;
         }

         CurrentSH2->memorybreakpoint[i].oldwritelong(addr, val);
         return;
      }
   }

   // Use the closest match if address doesn't match
   for (i = 0; i < MSH2->nummemorybreakpoints; i++)
   {
      if (((MSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
      {
         MSH2->memorybreakpoint[i].oldwritelong(addr, val);
         return;
      }
   }
   for (i = 0; i < SSH2->nummemorybreakpoints; i++)
   {
      if (((SSH2->memorybreakpoint[i].addr >> 16) & 0xFFF) == ((addr >> 16) & 0xFFF))
      {
         SSH2->memorybreakpoint[i].oldwritelong(addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

int CheckForMemoryBreakpointDupes(SH2_struct *context, u32 addr, u32 flag, int *which)
{
   int i;

   for (i = 0; i < context->nummemorybreakpoints; i++)
   {
      if (((context->memorybreakpoint[i].addr >> 16) & 0xFFF) ==
          ((addr >> 16) & 0xFFF))
      {
         // See it actually was using the same operation flag
         if (context->memorybreakpoint[i].flags & flag)
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

   if (context->nummemorybreakpoints < MAX_BREAKPOINTS) {
      // Only regular addresses are supported at this point(Sorry, no onchip!)
      switch (addr >> 29) {
         case 0x0:
         case 0x1:
         case 0x5:
            break;
         default:
            return -1;
      }

      // Make sure it isn't already on the list
      for (i = 0; i < context->nummemorybreakpoints; i++)
      {
         if ((addr & 0x0FFFFFFF) == (context->memorybreakpoint[i].addr & 0xFFFFFFF))
            return -1;
      }

      context->memorybreakpoint[context->nummemorybreakpoints].addr = addr;
      context->memorybreakpoint[context->nummemorybreakpoints].flags = flags;

      context->memorybreakpoint[context->nummemorybreakpoints].oldreadbyte = ReadByteList[(addr >> 16) & 0xFFF];
      context->memorybreakpoint[context->nummemorybreakpoints].oldreadword = ReadWordList[(addr >> 16) & 0xFFF];
      context->memorybreakpoint[context->nummemorybreakpoints].oldreadlong = ReadLongList[(addr >> 16) & 0xFFF];
      context->memorybreakpoint[context->nummemorybreakpoints].oldwritebyte = WriteByteList[(addr >> 16) & 0xFFF];
      context->memorybreakpoint[context->nummemorybreakpoints].oldwriteword = WriteWordList[(addr >> 16) & 0xFFF];
      context->memorybreakpoint[context->nummemorybreakpoints].oldwritelong = WriteLongList[(addr >> 16) & 0xFFF];

      if (flags & BREAK_BYTEREAD)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_BYTEREAD, &which))
            ReadByteList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointReadByte;
         else
            // fix old memory access function
            context->memorybreakpoint[context->nummemorybreakpoints].oldreadbyte = context->memorybreakpoint[which].oldreadbyte;
      }

      if (flags & BREAK_WORDREAD)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_WORDREAD, &which))
            ReadWordList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointReadWord;
         else
            // fix old memory access function
            context->memorybreakpoint[context->nummemorybreakpoints].oldreadword = context->memorybreakpoint[which].oldreadword;
      }

      if (flags & BREAK_LONGREAD)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_LONGREAD, &which))
            ReadLongList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointReadLong;
         else
            // fix old memory access function
            context->memorybreakpoint[context->nummemorybreakpoints].oldreadword = context->memorybreakpoint[which].oldreadword;
      }

      if (flags & BREAK_BYTEWRITE)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_BYTEWRITE, &which))
            WriteByteList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointWriteByte;
         else
            // fix old memory access function
            context->memorybreakpoint[context->nummemorybreakpoints].oldwritebyte = context->memorybreakpoint[which].oldwritebyte;
      }

      if (flags & BREAK_WORDWRITE)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_WORDWRITE, &which))
            WriteWordList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointWriteWord;
         else
            // fix old memory access function
            context->memorybreakpoint[context->nummemorybreakpoints].oldwriteword = context->memorybreakpoint[which].oldwriteword;
      }

      if (flags & BREAK_LONGWRITE)
      {
         // Make sure function isn't already being breakpointed by another breakpoint
         if (!CheckForMemoryBreakpointDupes(context, addr, BREAK_LONGWRITE, &which))
            WriteLongList[(addr >> 16) & 0xFFF] = &SH2MemoryBreakpointWriteLong;
         else
            // fix old memory access function
            context->memorybreakpoint[context->nummemorybreakpoints].oldwritelong = context->memorybreakpoint[which].oldwritelong;
      }

      context->nummemorybreakpoints++;

      return 0;
   }

   return -1;
}

//////////////////////////////////////////////////////////////////////////////

void SH2SortMemoryBreakpoints(SH2_struct *context) {
   int i, i2;
   memorybreakpoint_struct tmp;

   for (i = 0; i < (MAX_BREAKPOINTS-1); i++)
   {
      for (i2 = i+1; i2 < MAX_BREAKPOINTS; i2++)
      {
         if (context->memorybreakpoint[i].addr == 0xFFFFFFFF &&
             context->memorybreakpoint[i2].addr != 0xFFFFFFFF)
         {
            memcpy(&tmp, context->memorybreakpoint+i, sizeof(memorybreakpoint_struct));
            memcpy(context->memorybreakpoint+i, context->memorybreakpoint+i2, sizeof(memorybreakpoint_struct));
            memcpy(context->memorybreakpoint+i2, &tmp, sizeof(memorybreakpoint_struct));
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

int SH2DelMemoryBreakpoint(SH2_struct *context, u32 addr) {
   int i, i2;

   if (context->nummemorybreakpoints > 0) {
      for (i = 0; i < context->nummemorybreakpoints; i++) {
         if (context->memorybreakpoint[i].addr == addr)
         {
            // Remove memory access piggyback function to memory access function table

            // Make sure no other breakpoints need the breakpoint functions first
            for (i2 = 0; i2 < context->nummemorybreakpoints; i2++)
            {
               if (((context->memorybreakpoint[i].addr >> 16) & 0xFFF) ==
                   ((context->memorybreakpoint[i2].addr >> 16) & 0xFFF) &&
                   i != i2)
               {
                  // Clear the flags
                  context->memorybreakpoint[i].flags &= ~context->memorybreakpoint[i2].flags;
               }                
            }
            
            if (context->memorybreakpoint[i].flags & BREAK_BYTEREAD)
               ReadByteList[(addr >> 16) & 0xFFF] = context->memorybreakpoint[i].oldreadbyte;

            if (context->memorybreakpoint[i].flags & BREAK_WORDREAD)
               ReadWordList[(addr >> 16) & 0xFFF] = context->memorybreakpoint[i].oldreadword;

            if (context->memorybreakpoint[i].flags & BREAK_LONGREAD)
               ReadLongList[(addr >> 16) & 0xFFF] = context->memorybreakpoint[i].oldreadlong;

            if (context->memorybreakpoint[i].flags & BREAK_BYTEWRITE)
               WriteByteList[(addr >> 16) & 0xFFF] = context->memorybreakpoint[i].oldwritebyte;

            if (context->memorybreakpoint[i].flags & BREAK_WORDWRITE)
               WriteWordList[(addr >> 16) & 0xFFF] = context->memorybreakpoint[i].oldwriteword;

            if (context->memorybreakpoint[i].flags & BREAK_LONGWRITE)
               WriteLongList[(addr >> 16) & 0xFFF] = context->memorybreakpoint[i].oldwritelong;

            context->memorybreakpoint[i].addr = 0xFFFFFFFF;
            SH2SortMemoryBreakpoints(context);
            context->nummemorybreakpoints--;
            return 0;
         }
      }
   }

   return -1;
}

//////////////////////////////////////////////////////////////////////////////

memorybreakpoint_struct *SH2GetMemoryBreakpointList(SH2_struct *context) {
   return context->memorybreakpoint;
}

//////////////////////////////////////////////////////////////////////////////

void SH2ClearMemoryBreakpoints(SH2_struct *context) {
   int i;
   for (i = 0; i < MAX_BREAKPOINTS; i++)
   {
      context->memorybreakpoint[i].addr = 0xFFFFFFFF;
      context->memorybreakpoint[i].flags = 0;
      context->memorybreakpoint[i].oldreadbyte = NULL;
      context->memorybreakpoint[i].oldreadword = NULL;
      context->memorybreakpoint[i].oldreadlong = NULL;
      context->memorybreakpoint[i].oldwritebyte = NULL;
      context->memorybreakpoint[i].oldwriteword = NULL;
      context->memorybreakpoint[i].oldwritelong = NULL;
   }
   context->nummemorybreakpoints = 0;
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

u8 FASTCALL OnchipReadByte(u32 addr) {
   switch(addr)
   {
      case 0x000:
//         LOG("Serial Mode Register read: %02X\n", CurrentSH2->onchip.SMR);
         return CurrentSH2->onchip.SMR;
      case 0x001:
//         LOG("Bit Rate Register read: %02X\n", CurrentSH2->onchip.BRR);
         return CurrentSH2->onchip.BRR;
      case 0x002:
//         LOG("Serial Control Register read: %02X\n", CurrentSH2->onchip.SCR);
         return CurrentSH2->onchip.SCR;
      case 0x003:
//         LOG("Transmit Data Register read: %02X\n", CurrentSH2->onchip.TDR);
         return CurrentSH2->onchip.TDR;
      case 0x004:
//         LOG("Serial Status Register read: %02X\n", CurrentSH2->onchip.SSR);

/*
         // if Receiver is enabled, clear SSR's TDRE bit, set SSR's RDRF and update RDR.

         if (CurrentSH2->onchip.SCR & 0x10)
         {
            CurrentSH2->onchip.RDR = SCIReceiveByte();
            CurrentSH2->onchip.SSR = (CurrentSH2->onchip.SSR & 0x7F) | 0x40;
         }
         // if Transmitter is enabled, clear SSR's RDRF bit, and set SSR's TDRE bit.
         else if (CurrentSH2->onchip.SCR & 0x20)
         {
            CurrentSH2->onchip.SSR = (CurrentSH2->onchip.SSR & 0xBF) | 0x80;
         }
*/
         return CurrentSH2->onchip.SSR;
      case 0x005:
//         LOG("Receive Data Register read: %02X PC = %08X\n", CurrentSH2->onchip.RDR, CurrentSH2->regs.PC);
         return CurrentSH2->onchip.RDR;
      case 0x011:
         return CurrentSH2->onchip.FTCSR;
      case 0x012:         
         return CurrentSH2->onchip.FRC.part.H;
      case 0x013:
         return CurrentSH2->onchip.FRC.part.L;
      case 0x016:
         return CurrentSH2->onchip.TCR;
      case 0x017:
         return CurrentSH2->onchip.TOCR;
      case 0x080:
         return CurrentSH2->onchip.WTCSR;
      case 0x081:
         return CurrentSH2->onchip.WTCNT;
      case 0x092:
         return CurrentSH2->onchip.CCR;
      default:
         LOG("Unhandled Onchip byte read %08X\n", (int)addr);
         break;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL OnchipReadWord(u32 addr) {
   switch(addr)
   {
      case 0x060:
         return CurrentSH2->onchip.IPRB;
      case 0x0E0:
         return CurrentSH2->onchip.ICR;
      case 0x0E2:
         return CurrentSH2->onchip.IPRA;
      default:
         LOG("Unhandled Onchip word read %08X\n", (int)addr);
         return 0;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL OnchipReadLong(u32 addr) {
   switch(addr)
   {
      case 0x100:
      case 0x120:
         return CurrentSH2->onchip.DVSR;
      case 0x104: // DVDNT
      case 0x124:
         return CurrentSH2->onchip.DVDNTL;
      case 0x108:
      case 0x128:
         return CurrentSH2->onchip.DVCR;
      case 0x10C:
      case 0x12C:
         return CurrentSH2->onchip.VCRDIV;
      case 0x110:
      case 0x130:
         return CurrentSH2->onchip.DVDNTH;
      case 0x114:
      case 0x134:
         return CurrentSH2->onchip.DVDNTL;
      case 0x11C: // DVDNTL mirror
      case 0x13C:
         return CurrentSH2->onchip.DVDNTL;
      case 0x18C:
         return CurrentSH2->onchip.CHCR0;
      case 0x19C:
         return CurrentSH2->onchip.CHCR1;
      case 0x1B0:
         return CurrentSH2->onchip.DMAOR;
      case 0x1E0:
         return CurrentSH2->onchip.BCR1;
      default:
         LOG("Unhandled Onchip long read %08X\n", (int)addr);
         return 0;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL OnchipWriteByte(u32 addr, u8 val) {
   switch(addr) {
      case 0x000:
//         LOG("Serial Mode Register write: %02X\n", val);
         CurrentSH2->onchip.SMR = val;
         return;
      case 0x001:
//         LOG("Bit Rate Register write: %02X\n", val);
         CurrentSH2->onchip.BRR = val;
         return;
      case 0x002:
//         LOG("Serial Control Register write: %02X\n", val);

         // If Transmitter is getting disabled, set TDRE
         if (!(val & 0x20))
            CurrentSH2->onchip.SSR |= 0x80;

         CurrentSH2->onchip.SCR = val;
         return;
      case 0x003:
//         LOG("Transmit Data Register write: %02X. PC = %08X\n", val, CurrentSH2->regs.PC);
         CurrentSH2->onchip.TDR = val;
         return;
      case 0x004:
//         LOG("Serial Status Register write: %02X\n", val);
         
         if (CurrentSH2->onchip.SCR & 0x20)
         {
            // Transmitter Mode

            // If the TDRE bit cleared, let's do a transfer
            if (!(val & 0x80))
               SCITransmitByte(CurrentSH2->onchip.TDR);

            // Generate an interrupt if need be here
         }
         return;
      case 0x010:
         CurrentSH2->onchip.TIER = (val & 0x8E) | 0x1;
         return;
      case 0x011:
         CurrentSH2->onchip.FTCSR = (CurrentSH2->onchip.FTCSR & (val & 0xFE)) | (val & 0x1);
      case 0x012:
         CurrentSH2->onchip.FRC.part.H = val;
         return;
      case 0x013:
         CurrentSH2->onchip.FRC.part.L = val;
         return;
      case 0x014:
         if (!(CurrentSH2->onchip.TOCR & 0x10))
            CurrentSH2->onchip.OCRA = (val << 8) | (CurrentSH2->onchip.OCRA & 0xFF);
         else                  
            CurrentSH2->onchip.OCRB = (val << 8) | (CurrentSH2->onchip.OCRB & 0xFF);
         return;
      case 0x015:
         if (!(CurrentSH2->onchip.TOCR & 0x10))
            CurrentSH2->onchip.OCRA = (CurrentSH2->onchip.OCRA & 0xFF00) | val;
         else
            CurrentSH2->onchip.OCRB = (CurrentSH2->onchip.OCRB & 0xFF00) | val;
         return;
      case 0x016:
         CurrentSH2->onchip.TCR = val & 0x83;

         switch (val & 3)
         {
            case 0:
               CurrentSH2->frc.div = 8;
               break;
            case 1:
               CurrentSH2->frc.div = 32;
               break;
            case 2:
               CurrentSH2->frc.div = 128;
               break;
            case 3:
               LOG("FRT external input clock not implemented.\n");
               break;
         }
         return;
      case 0x017:
         CurrentSH2->onchip.TOCR = 0xE0 | (val & 0x13);
         return;
      case 0x060:
         CurrentSH2->onchip.IPRB = (val << 8);
         return;
      case 0x061:
         return;
      case 0x062:
         CurrentSH2->onchip.VCRA = ((val & 0x7F) << 8) | (CurrentSH2->onchip.VCRA & 0x00FF);
         return;
      case 0x063:
         CurrentSH2->onchip.VCRA = (CurrentSH2->onchip.VCRA & 0xFF00) | (val & 0x7F);
         return;
      case 0x064:
         CurrentSH2->onchip.VCRB = ((val & 0x7F) << 8) | (CurrentSH2->onchip.VCRB & 0x00FF);
         return;
      case 0x065:
         CurrentSH2->onchip.VCRB = (CurrentSH2->onchip.VCRB & 0xFF00) | (val & 0x7F);
         return;
      case 0x066:
         CurrentSH2->onchip.VCRC = ((val & 0x7F) << 8) | (CurrentSH2->onchip.VCRC & 0x00FF);
         return;
      case 0x067:
         CurrentSH2->onchip.VCRC = (CurrentSH2->onchip.VCRC & 0xFF00) | (val & 0x7F);
         return;
      case 0x068:
         CurrentSH2->onchip.VCRD = (val & 0x7F) << 8;
         return;
      case 0x069:
         return;
      case 0x071:
         CurrentSH2->onchip.DRCR0 = val & 0x3;
         return;
      case 0x072:
         CurrentSH2->onchip.DRCR1 = val & 0x3;
         return;
      case 0x091:
         CurrentSH2->onchip.SBYCR = val & 0xDF;
         return;
      case 0x092:
         CurrentSH2->onchip.CCR = val & 0xCF;
         return;
      case 0x0E0:
         CurrentSH2->onchip.ICR = ((val & 0x1) << 8) | (CurrentSH2->onchip.ICR & 0xFEFF);
         return;
      case 0x0E1:
         CurrentSH2->onchip.ICR = (CurrentSH2->onchip.ICR & 0xFFFE) | (val & 0x1);
         return;
      case 0x0E2:
         CurrentSH2->onchip.IPRA = (val << 8) | (CurrentSH2->onchip.IPRA & 0x00FF);
         return;
      case 0x0E3:
         CurrentSH2->onchip.IPRA = (CurrentSH2->onchip.IPRA & 0xFF00) | (val & 0xF0);
         return;
      case 0x0E4:
         CurrentSH2->onchip.VCRWDT = ((val & 0x7F) << 8) | (CurrentSH2->onchip.VCRWDT & 0x00FF);
         return;
      case 0x0E5:
         CurrentSH2->onchip.VCRWDT = (CurrentSH2->onchip.VCRWDT & 0xFF00) | (val & 0x7F);
         return;
      default:
         LOG("Unhandled Onchip byte write %08X\n", (int)addr);
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL OnchipWriteWord(u32 addr, u16 val) {
   switch(addr)
   {
      case 0x060:
         CurrentSH2->onchip.IPRB = val & 0xFF00;
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
                  CurrentSH2->wdt.div = 2;
                  break;
               case 1:
                  CurrentSH2->wdt.div = 64;
                  break;
               case 2:
                  CurrentSH2->wdt.div = 128;
                  break;
               case 3:
                  CurrentSH2->wdt.div = 256;
                  break;
               case 4:
                  CurrentSH2->wdt.div = 512;
                  break;
               case 5:
                  CurrentSH2->wdt.div = 1024;
                  break;
               case 6:
                  CurrentSH2->wdt.div = 4096;
                  break;
               case 7:
                  CurrentSH2->wdt.div = 8192;
                  break;
            }

            CurrentSH2->wdt.isenable = (val & 0x20);
            CurrentSH2->wdt.isinterval = (~val & 0x40);

            CurrentSH2->onchip.WTCSR = (u8)val | 0x18;
         }
         else if (val >> 8 == 0x5A)
         {
            // WTCNT
            CurrentSH2->onchip.WTCNT = (u8)val;
         }
         return;
      case 0x082:
         if (val == 0xA500)
            // clear WOVF bit
            CurrentSH2->onchip.RSTCSR &= 0x7F;
         else if (val >> 8 == 0x5A)
            // RSTE and RSTS bits
            CurrentSH2->onchip.RSTCSR = (CurrentSH2->onchip.RSTCSR & 0x80) | (val & 0x60) | 0x1F;
         return;
      case 0x092:
         CurrentSH2->onchip.CCR = val & 0xCF;
         return;
      case 0x0E0:
         CurrentSH2->onchip.ICR = val & 0x0101;
         return;
      case 0x0E2:
         CurrentSH2->onchip.IPRA = val & 0xFFF0;
         return;
      case 0x108:
      case 0x128:
         CurrentSH2->onchip.DVCR = val & 0x3;
         return;
      case 0x148:
         CurrentSH2->onchip.BBRA = val & 0xFF;
         return;
      case 0x178:
         CurrentSH2->onchip.BRCR = val & 0xF4DC;
         return;
      default:
         LOG("Unhandled Onchip word write %08X(%04X)\n", (int)addr, val);
         return;
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL OnchipWriteLong(u32 addr, u32 val)  {
   switch (addr)
   {
      case 0x100:
      case 0x120:
         CurrentSH2->onchip.DVSR = val;
         return;
      case 0x104:
      case 0x124:
      {
         s32 divisor = (s32) CurrentSH2->onchip.DVSR;
         if (divisor == 0)
         {
            CurrentSH2->onchip.DVDNTL = val;
            if (val & 0x80000000)
            {
               CurrentSH2->onchip.DVDNTH = 0xFFFFFFFF;
            }
            else
            {
               CurrentSH2->onchip.DVDNTH = 0;
            }
            CurrentSH2->onchip.DVCR |= 1;
#if DEBUG
            if (CurrentSH2->onchip.DVCR & 0x2)
               LOG("should be triggering DIVU interrupt\n");
#endif
         }
         else
         {
            s32 quotient = ((s32) val) / divisor;
            s32 remainder = ((s32) val) % divisor;
            CurrentSH2->onchip.DVDNTL = quotient;
            CurrentSH2->onchip.DVDNTH = remainder;
            CurrentSH2->onchip.DVCR &= ~1;
         }
         return;
      }
      case 0x108:
      case 0x128:
         CurrentSH2->onchip.DVCR = val & 0x3;
         return;
      case 0x10C:
      case 0x12C:
         CurrentSH2->onchip.VCRDIV = val & 0xFFFF;
         return;
      case 0x110:
      case 0x130:
         CurrentSH2->onchip.DVDNTH = val;
         return;
      case 0x114:
      case 0x134: {
         s32 divisor = (s32) CurrentSH2->onchip.DVSR;
         s64 dividend = CurrentSH2->onchip.DVDNTH;
         dividend <<= 32;
         dividend |= val;

         if (divisor == 0)
         {
            CurrentSH2->onchip.DVDNTL = val;
            CurrentSH2->onchip.DVCR |= 1;
#if DEBUG
            if (CurrentSH2->onchip.DVCR & 0x2)
               LOG("should be triggering DIVU interrupt\n");
#endif
         }
         else
         {
            s64 quotient = dividend / divisor;
            s32 remainder = dividend % divisor;

            // check for overflow
            if (quotient >> 32) {
               CurrentSH2->onchip.DVCR |= 1;
#if DEBUG
               if (CurrentSH2->onchip.DVCR & 0x2)
                  LOG("should be triggering DIVU interrupt\n");
#endif
            }
            else
               CurrentSH2->onchip.DVCR &= ~1;

            CurrentSH2->onchip.DVDNTL = quotient;
            CurrentSH2->onchip.DVDNTH = remainder;
         }
         return;
      }
      case 0x140:
         CurrentSH2->onchip.BARA.all = val;         
         return;
      case 0x144:
         CurrentSH2->onchip.BAMRA.all = val;         
         return;
      case 0x180:
         CurrentSH2->onchip.SAR0 = val;
         return;
      case 0x184:
         CurrentSH2->onchip.DAR0 = val;
         return;
      case 0x188:
         CurrentSH2->onchip.TCR0 = val & 0xFFFFFF;
         return;
      case 0x18C:
         CurrentSH2->onchip.CHCR0 = val & 0xFFFF;

         // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
         // and CHCR's DE bit is set and TE bit is cleared,
         // do a dma transfer
         if ((CurrentSH2->onchip.DMAOR & 7) == 1 && (val & 0x3) == 1)
            DMAExec();
         return;
      case 0x190:
         CurrentSH2->onchip.SAR1 = val;
         return;
      case 0x194:
         CurrentSH2->onchip.DAR1 = val;
         return;
      case 0x198:
         CurrentSH2->onchip.TCR1 = val & 0xFFFFFF;
         return;
      case 0x19C:
         CurrentSH2->onchip.CHCR1 = val & 0xFFFF;

         // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
         // and CHCR's DE bit is set and TE bit is cleared,
         // do a dma transfer
         if ((CurrentSH2->onchip.DMAOR & 7) == 1 && (val & 0x3) == 1)
            DMAExec();
         return;
      case 0x1A0:
         CurrentSH2->onchip.VCRDMA0 = val & 0xFFFF;
         return;
      case 0x1A8:
         CurrentSH2->onchip.VCRDMA1 = val & 0xFFFF;
         return;
      case 0x1B0:
         CurrentSH2->onchip.DMAOR = val & 0xF;

         // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
         // and CHCR's DE bit is set and TE bit is cleared,
         // do a dma transfer
         if ((val & 7) == 1)
            DMAExec();
         return;
      case 0x1E0:
         CurrentSH2->onchip.BCR1 &= 0x8000;
         CurrentSH2->onchip.BCR1 |= val & 0x1FF7;
         return;
      case 0x1E4:
         CurrentSH2->onchip.BCR2 = val & 0xFC;
         return;
      case 0x1E8:
         CurrentSH2->onchip.WCR = val;
         return;
      case 0x1EC:
         CurrentSH2->onchip.MCR = val & 0xFEFC;
         return;
      case 0x1F0:
         CurrentSH2->onchip.RTCSR = val & 0xF8;
         return;
      case 0x1F8:
         CurrentSH2->onchip.RTCOR = val & 0xFF;
         return;
      default:
         LOG("Unhandled Onchip long write %08X\n", (int)addr);
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL AddressArrayReadLong(u32 addr) {
   return CurrentSH2->AddressArray[(addr & 0x3FC) >> 2];
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL AddressArrayWriteLong(u32 addr, u32 val)  {
   CurrentSH2->AddressArray[(addr & 0x3FC) >> 2] = val;
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL DataArrayReadByte(u32 addr) {
   return T2ReadByte(CurrentSH2->DataArray, addr & 0xFFF);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL DataArrayReadWord(u32 addr) {
   return T2ReadWord(CurrentSH2->DataArray, addr & 0xFFF);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL DataArrayReadLong(u32 addr) {
   return T2ReadLong(CurrentSH2->DataArray, addr & 0xFFF);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DataArrayWriteByte(u32 addr, u8 val)  {
   T2WriteByte(CurrentSH2->DataArray, addr & 0xFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DataArrayWriteWord(u32 addr, u16 val)  {
   T2WriteWord(CurrentSH2->DataArray, addr & 0xFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL DataArrayWriteLong(u32 addr, u32 val)  {
   T2WriteLong(CurrentSH2->DataArray, addr & 0xFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

void FRTExec(u32 cycles) {
   u32 frcold;
   u32 frctemp;

   frcold = frctemp = (u32)CurrentSH2->onchip.FRC.all;
   
   // Increment FRC
   frctemp += ((CurrentSH2->cycles + CurrentSH2->frc.leftover) / CurrentSH2->frc.div);
   CurrentSH2->frc.leftover = (CurrentSH2->cycles + CurrentSH2->frc.leftover) % CurrentSH2->frc.div;

   // Check to see if there is or was a Output Compare A match
   if (frctemp >= CurrentSH2->onchip.OCRA && frcold < CurrentSH2->onchip.OCRA)
   {
      // Do we need to trigger an interrupt?
      if (CurrentSH2->onchip.TIER & 0x8)
         SH2SendInterrupt(CurrentSH2, CurrentSH2->onchip.VCRC & 0x7F, (CurrentSH2->onchip.IPRB & 0xF00) >> 8);

      // Do we need to clear the FRC?
      if (CurrentSH2->onchip.FTCSR & 0x1)
      {
         frctemp = 0;
         CurrentSH2->frc.leftover = 0;
      }

      // Set OCFA flag
      CurrentSH2->onchip.FTCSR |= 0x8;
   }

   // Check to see if there is or was a Output Compare B match
   if (frctemp >= CurrentSH2->onchip.OCRB && frcold < CurrentSH2->onchip.OCRB)
   {
      // Do we need to trigger an interrupt?
      if (CurrentSH2->onchip.TIER & 0x4)
         SH2SendInterrupt(CurrentSH2, CurrentSH2->onchip.VCRC & 0x7F, (CurrentSH2->onchip.IPRB & 0xF00) >> 8);

      // Set OCFB flag
      CurrentSH2->onchip.FTCSR |= 0x4;
   }

   // If FRC overflows, set overflow flag
   if (frctemp > 0xFFFF)
   {
      // Do we need to trigger an interrupt?
      if (CurrentSH2->onchip.TIER & 0x2)
         SH2SendInterrupt(CurrentSH2, (CurrentSH2->onchip.VCRD >> 8) & 0x7F, (CurrentSH2->onchip.IPRB & 0xF00) >> 8);

      CurrentSH2->onchip.FTCSR |= 2;
   }

   // Write new FRC value
   CurrentSH2->onchip.FRC.all = frctemp;
}

//////////////////////////////////////////////////////////////////////////////

void WDTExec(u32 cycles) {
   u32 wdttemp;

   if (!CurrentSH2->wdt.isenable || CurrentSH2->onchip.WTCSR & 0x80 || CurrentSH2->onchip.RSTCSR & 0x80)
      return;

   wdttemp = (u32)CurrentSH2->onchip.WTCNT;
   wdttemp += ((cycles + CurrentSH2->wdt.leftover) / CurrentSH2->wdt.div);
   CurrentSH2->wdt.leftover = (cycles + CurrentSH2->wdt.leftover) % CurrentSH2->wdt.div;

   // Are we overflowing?
   if (wdttemp > 0xFF)
   {
      // Obviously depending on whether or not we're in Watchdog or Interval
      // Modes, they'll handle an overflow differently.

      if (CurrentSH2->wdt.isinterval)
      {
         // Interval Timer Mode

         // Set OVF flag
         CurrentSH2->onchip.WTCSR |= 0x80;

         // Trigger interrupt
         SH2SendInterrupt(CurrentSH2, (CurrentSH2->onchip.VCRWDT >> 8) & 0x7F, (CurrentSH2->onchip.IPRA >> 4) & 0xF);
      }
      else
      {
         // Watchdog Timer Mode(untested)
         LOG("Watchdog timer(WDT mode) overflow not implemented\n");
      }
   }

   // Write new WTCNT value
   CurrentSH2->onchip.WTCNT = (u8)wdttemp;
}

//////////////////////////////////////////////////////////////////////////////

void DMAExec(void) {
   // If AE and NMIF bits are set, we can't continue
   if (CurrentSH2->onchip.DMAOR & 0x6)
      return;

   if ((CurrentSH2->onchip.CHCR0 & 0x1) && (CurrentSH2->onchip.CHCR1 & 0x1)) { // both channel wants DMA
      if (CurrentSH2->onchip.DMAOR & 0x8) { // round robin priority
         LOG("dma\t: FIXME: two channel dma - round robin priority not properly implemented\n");
         DMATransfer(&CurrentSH2->onchip.CHCR0, &CurrentSH2->onchip.SAR0,
		     &CurrentSH2->onchip.DAR0,  &CurrentSH2->onchip.TCR0,
		     &CurrentSH2->onchip.VCRDMA0);
         DMATransfer(&CurrentSH2->onchip.CHCR1, &CurrentSH2->onchip.SAR1,
		     &CurrentSH2->onchip.DAR1,  &CurrentSH2->onchip.TCR1,
                     &CurrentSH2->onchip.VCRDMA1);
      }
      else { // channel 0 > channel 1 priority
         DMATransfer(&CurrentSH2->onchip.CHCR0, &CurrentSH2->onchip.SAR0,
		     &CurrentSH2->onchip.DAR0,  &CurrentSH2->onchip.TCR0,
		     &CurrentSH2->onchip.VCRDMA0);
         DMATransfer(&CurrentSH2->onchip.CHCR1, &CurrentSH2->onchip.SAR1,
		     &CurrentSH2->onchip.DAR1,  &CurrentSH2->onchip.TCR1,
		     &CurrentSH2->onchip.VCRDMA1);
      }
   }
   else { // only one channel wants DMA
      if (CurrentSH2->onchip.CHCR0 & 0x1) { // DMA for channel 0
         DMATransfer(&CurrentSH2->onchip.CHCR0, &CurrentSH2->onchip.SAR0,
		     &CurrentSH2->onchip.DAR0,  &CurrentSH2->onchip.TCR0,
		     &CurrentSH2->onchip.VCRDMA0);
         return;
      }
      if (CurrentSH2->onchip.CHCR1 & 0x1) { // DMA for channel 1
         DMATransfer(&CurrentSH2->onchip.CHCR1, &CurrentSH2->onchip.SAR1,
		     &CurrentSH2->onchip.DAR1,  &CurrentSH2->onchip.TCR1,
		     &CurrentSH2->onchip.VCRDMA1);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void DMATransfer(u32 *CHCR, u32 *SAR, u32 *DAR, u32 *TCR, u32 *VCRDMA)
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
               MappedMemoryWriteByte(*DAR, MappedMemoryReadByte(*SAR));
               *SAR += srcInc;
               *DAR += destInc;
            }

            *TCR = 0;
            break;
         case 1:
            destInc *= 2;
            srcInc *= 2;

            for (i = 0; i < *TCR; i++) {
               MappedMemoryWriteWord(*DAR, MappedMemoryReadWord(*SAR));
               *SAR += srcInc;
               *DAR += destInc;
            }

            *TCR = 0;
            break;
         case 2:
            destInc *= 4;
            srcInc *= 4;

            for (i = 0; i < *TCR; i++) {
               MappedMemoryWriteLong(*DAR, MappedMemoryReadLong(*SAR));
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
                  MappedMemoryWriteLong(*DAR, MappedMemoryReadLong(*SAR));
                  *DAR += destInc;
                  *SAR += srcInc;
               }
            }

            *TCR = 0;
            break;
      }
   }

   if (*CHCR & 0x4)
      SH2SendInterrupt(CurrentSH2, *VCRDMA, (CurrentSH2->onchip.IPRA & 0xF00) >> 8);

   // Set Transfer End bit
   *CHCR |= 0x2;
}

//////////////////////////////////////////////////////////////////////////////
// Input Capture Specific
//////////////////////////////////////////////////////////////////////////////

void FASTCALL MSH2InputCaptureWriteWord(u32 addr, u16 data)
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

void FASTCALL SSH2InputCaptureWriteWord(u32 addr, u16 data)
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

void SCITransmitByte(u8 val) {
}

//////////////////////////////////////////////////////////////////////////////

int SH2SaveState(SH2_struct *context, FILE *fp)
{
   int offset;

   // Write header
   if (context->isslave == 0)
      offset = StateWriteHeader(fp, "MSH2", 1);
   else
   {
      offset = StateWriteHeader(fp, "SSH2", 1);
      fwrite((void *)&yabsys.IsSSH2Running, 1, 1, fp);
   }

/*
   // Write registers
   fwrite((void *)regs_array, 4, 23, fp);

   // Write onchip registers
   fwrite((void *)onchip->getBuffer(), 0x200, 1, fp);
*/
   return StateFinishHeader(fp, offset);
}

//////////////////////////////////////////////////////////////////////////////

int SH2LoadState(SH2_struct *context, FILE *fp, int version, int size)
{
/*
   if (context->isslave == 1)
      fread((void *)&yabsys.IsSSH2Running, 1, 1, fp);

   // Read registers
   fread((void *)regs_array, 4, 23, fp);

   // Read onchip registers
   fread((void *)onchip->getBuffer(), 0x200, 1, fp);
*/
   return size;
}

//////////////////////////////////////////////////////////////////////////////
