/*  Copyright 2017 devMiyax(smiyaxdev@gmail.com)

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

#include <core.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h> 
#include <stdint.h>
#include "memory.h"
#include "sh2core.h"
#include "DynarecSh2.h"
#include "debug.h"
#include "yabause.h"


#define SH2CORE_DYNAMIC             3
#define SH2CORE_DYNAMIC_DEBUG             4

extern "C" {
int SH2DynInit(void);
void SH2DynDeInit(void);
void SH2DynReset(SH2_struct *context);
void SH2DynDebugReset(SH2_struct *context);
void FASTCALL SH2DynExec(SH2_struct *context, u32 cycles);
void SH2DynGetRegisters(SH2_struct *context, sh2regs_struct *regs);
u32 SH2DynGetGPR(SH2_struct *context, int num);
u32 SH2DynGetSR(SH2_struct *context);
u32 SH2DynGetGBR(SH2_struct *context);
u32 SH2DynGetVBR(SH2_struct *context);
u32 SH2DynGetMACH(SH2_struct *context);
u32 SH2DynGetMACL(SH2_struct *context);
u32 SH2DynGetPR(SH2_struct *context);
u32 SH2DynGetPC(SH2_struct *context);
void SH2DynSetRegisters(SH2_struct *context, const sh2regs_struct *regs);
void SH2DynSetGPR(SH2_struct *context, int num, u32 value);
void SH2DynSetSR(SH2_struct *context, u32 value);
void SH2DynSetGBR(SH2_struct *context, u32 value);
void SH2DynSetVBR(SH2_struct *context, u32 value);
void SH2DynSetMACH(SH2_struct *context, u32 value);
void SH2DynSetMACL(SH2_struct *context, u32 value);
void SH2DynSetPR(SH2_struct *context, u32 value);
void SH2DynSetPC(SH2_struct *context, u32 value);
void SH2DynOnFrame(SH2_struct *context);
void SH2DynSendInterrupt(SH2_struct *context, u8 level, u8 vector);
int SH2DynGetInterrupts(SH2_struct *context, interrupt_struct interrupts[MAX_INTERRUPTS]);
void SH2DynSetInterrupts(SH2_struct *context, int num_interrupts, const interrupt_struct interrupts[MAX_INTERRUPTS]);
void SH2DynWriteNotify(u32 start, u32 length);

SH2Interface_struct SH2Dyn = {
  SH2CORE_DYNAMIC,
  "SH2 Dynamic Recompiler",

  SH2DynInit,
  SH2DynDeInit,
  SH2DynReset,
  SH2DynExec,
  SH2DynExec,

  SH2DynGetRegisters,
  SH2DynGetGPR,
  SH2DynGetSR,
  SH2DynGetGBR,
  SH2DynGetVBR,
  SH2DynGetMACH,
  SH2DynGetMACL,
  SH2DynGetPR,
  SH2DynGetPC,

  SH2DynSetRegisters,
  SH2DynSetGPR,
  SH2DynSetSR,
  SH2DynSetGBR,
  SH2DynSetVBR,
  SH2DynSetMACH,
  SH2DynSetMACL,
  SH2DynSetPR,
  SH2DynSetPC,
  SH2DynOnFrame,

  SH2DynSendInterrupt,
  SH2DynGetInterrupts,
  SH2DynSetInterrupts,
  SH2DynWriteNotify
};

SH2Interface_struct SH2DynDebug = {
  SH2CORE_DYNAMIC_DEBUG,
  "SH2 Dynamic Recompiler Debug",

  SH2DynInit,
  SH2DynDeInit,
  SH2DynDebugReset,
  SH2DynExec,
  SH2DynExec,

  SH2DynGetRegisters,
  SH2DynGetGPR,
  SH2DynGetSR,
  SH2DynGetGBR,
  SH2DynGetVBR,
  SH2DynGetMACH,
  SH2DynGetMACL,
  SH2DynGetPR,
  SH2DynGetPC,

  SH2DynSetRegisters,
  SH2DynSetGPR,
  SH2DynSetSR,
  SH2DynSetGBR,
  SH2DynSetVBR,
  SH2DynSetMACH,
  SH2DynSetMACL,
  SH2DynSetPR,
  SH2DynSetPC,
  SH2DynOnFrame,

  SH2DynSendInterrupt,
  SH2DynGetInterrupts,
  SH2DynSetInterrupts,
  SH2DynWriteNotify
};

int SH2DynInit(void) {
  return 0;
}

void SH2DynDeInit(void){
}

void SH2DynReset(SH2_struct *context) {

  if (context->ext == NULL){
    DynarecSh2 * pctx = new DynarecSh2();
    context->ext = (void*)pctx;
    pctx->SetContext(context);
  }

  DynarecSh2 * pctx = (DynarecSh2*)context->ext;
  if (context->isslave) {
    pctx->SetSlave(true);
  }
  else {
    pctx->SetSlave(false);
  }
  CompileBlocks * block = CompileBlocks::getInstance();
  block->SetDebugMode(false);
  pctx->ResetCPU();
}

void SH2DynDebugReset(SH2_struct *context) {

  if (context->ext == NULL) {
    DynarecSh2 * pctx = new DynarecSh2();
    context->ext = (void*)pctx;
    pctx->SetContext(context);
  }

  DynarecSh2 * pctx = (DynarecSh2*)context->ext;
  if (context->isslave) {
    pctx->SetSlave(true);
  }
  else {
    pctx->SetSlave(false);
  }
  CompileBlocks * block = CompileBlocks::getInstance();
  block->SetDebugMode(true);
  pctx->ResetCPU();
}

void FASTCALL SH2DynExec(SH2_struct *context, u32 cycles){
  if (cycles <= 0) return;
  DynarecSh2* pctx = ((DynarecSh2*)context->ext);
  pctx->SetCurrentContext();
  pctx->ExecuteCount(cycles);
}

void SH2DynSendInterrupt(SH2_struct *context, u8 vector, u8 level){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  pctx->AddInterrupt(vector, level);
}

int SH2DynGetInterrupts(SH2_struct *context, interrupt_struct interrupts[MAX_INTERRUPTS]){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  dlstIntct::iterator itr = pctx->m_IntruptTbl.begin();
  int i = 0;
  while (itr != pctx->m_IntruptTbl.end()) {
    interrupts[i].level = itr->level;
    interrupts[i].vector = itr->Vector;
    itr++;
    i++;
  }
  return i;
}

void SH2DynSetInterrupts(SH2_struct *context, int num_interrupts, const interrupt_struct interrupts[MAX_INTERRUPTS]){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  pctx->m_IntruptTbl.clear();
  for (int i = 0; i < num_interrupts; i++) {
    dIntcTbl tmp;
    tmp.level = interrupts[i].level;
    tmp.Vector = interrupts[i].vector;
    pctx->m_IntruptTbl.push_back(tmp);
  }
  return;
}

void SH2DynGetRegisters(SH2_struct *context, sh2regs_struct *regs) {
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  memcpy(regs->R, pctx->GetGenRegPtr(), sizeof(regs->R));
  regs->GBR = pctx->GET_GBR();
  regs->VBR = pctx->GET_VBR();
  regs->SR.all = pctx->GET_SR();
  regs->MACH = pctx->GET_MACH();
  regs->MACL = pctx->GET_MACL();
  regs->PC = pctx->GET_PC();
  regs->PR = pctx->GET_PR();
}

u32 SH2DynGetGPR(SH2_struct *context, int num){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  return pctx->GetGenRegPtr()[num];
}

u32 SH2DynGetSR(SH2_struct *context){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  return pctx->GET_SR();
}

u32 SH2DynGetGBR(SH2_struct *context){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  return pctx->GET_GBR();
}

u32 SH2DynGetVBR(SH2_struct *context){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  return pctx->GET_VBR();
}

u32 SH2DynGetMACH(SH2_struct *context){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  return pctx->GET_MACH();
}

u32 SH2DynGetMACL(SH2_struct *context){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  return pctx->GET_MACL();
}

u32 SH2DynGetPR(SH2_struct *context){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  return pctx->GET_PR();
}

u32 SH2DynGetPC(SH2_struct *context){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  return pctx->GET_PC();
}

void SH2DynSetRegisters(SH2_struct *context, const sh2regs_struct *regs){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  memcpy(pctx->GetGenRegPtr(), regs->R , sizeof(regs->R));
  pctx->SET_GBR(regs->GBR);
  pctx->SET_VBR(regs->VBR);
  pctx->SET_SR(regs->SR.all);
  pctx->SET_MACH(regs->MACH);
  pctx->SET_MACL(regs->MACL);
  pctx->SET_PC(regs->PC);
  pctx->SET_PR(regs->PR);
}

void SH2DynSetGPR(SH2_struct *context, int num, u32 value) {
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  pctx->GetGenRegPtr()[num] = value;
}

void SH2DynSetSR(SH2_struct *context, u32 value){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  pctx->SET_SR(value);
}

void SH2DynSetGBR(SH2_struct *context, u32 value){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  pctx->SET_GBR(value);
}

void SH2DynSetVBR(SH2_struct *context, u32 value){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  pctx->SET_VBR(value);
}

void SH2DynSetMACH(SH2_struct *context, u32 value){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  pctx->SET_MACH(value);
}

void SH2DynSetMACL(SH2_struct *context, u32 value){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  pctx->SET_MACL(value);
}
void SH2DynSetPR(SH2_struct *context, u32 value){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  pctx->SET_PR(value);
}
void SH2DynSetPC(SH2_struct *context, u32 value){
  DynarecSh2 *pctx = (DynarecSh2*)context->ext;
  pctx->SET_PC(value);
}

static MemArea getMemArea(u32 addr) {
/*
00000000-000FFFFF : Boot ROM (512K, mirrored every 512K)
 00100000-0017FFFF : SMPC registers (128 bytes, mirrored every 128 bytes)
 00180000-001FFFFF : Backup RAM (64K, mirrored every 64K) [1]
 00200000-002FFFFF : Work RAM Low (1MB)
 00300000-003FFFFF : Random data on every read (mostly $00)
 00400000-007FFFFF : Always returns $0000.
 00800000-00FFFFFF : Always returns $00000001000200030004000500060007.
 01000000-01FFFFFF : Always returns $FFFF. [4]
 02000000-03FFFFFF : A-Bus CS0
 04000000-04FFFFFF : A-Bus CS1
 05000000-057FFFFF : A-Bus Dummy
 05800000-058FFFFF : A-Bus CS2 [2]
 05900000-059FFFFF : Lockup when read
 05A00000-05AFFFFF : 68000 Work RAM (512K) [9]
 05B00000-05BFFFFF : SCSP registers (4K, mirrored every 4K)
 05C00000-05C7FFFF : VDP1 VRAM (512K)
 05C80000-05CFFFFF : VDP1 Framebuffer (256K, mirrored every 256K) [7]
 05D00000-05D7FFFF : VDP1 Registers [6]
 05D80000-05DFFFFF : Lockup when read
 05E00000-05EFFFFF : VDP2 VRAM (512K, mirrored every 512K)
 05F00000-05F7FFFF : VDP2 CRAM (4K, mirrored every 4K) [8]
 05F80000-05FBFFFF : VDP2 registers (512 bytes, mirrored every 512 bytes)
 05FC0000-05FDFFFF : Always returns $000E0000
 05FE0000-05FEFFFF : SCU registers (256 bytes, mirrored every 256 bytes)
 05FF0000-05FFFFFF : Unknown registers (256 bytes, mirrored every 256 bytes) [3]
 06000000-07FFFFFF : Work RAM High (1MB, mirrored every 1MB)
*/
  u32 memArea = addr & 0x0FF00000;
  if (memArea < 0x00100000)
    return BIOS_MEM;
  if ((memArea >= 0x00200000) && (memArea < 0x00300000))
    return LO_MEM;
  if ((memArea >= 0x06000000) && (memArea < 0x08000000))
    return HI_MEM;
  return UNDEF_MEM;
}


void SH2DynWriteNotify(u32 start, u32 length){
  CompileBlocks * block = CompileBlocks::getInstance();
  
  switch (getMemArea(start)){
    // ROM
  case BIOS_MEM:
      block->LookupTableRom[ (start&0x0007FFFF)>>1 ] = NULL;
    break;

  // Low Memory
  case LO_MEM:
    for (u32 addr = start; addr< start + length; addr += 2)
      block->LookupTableLow[ (addr&0x000FFFFF)>>1 ] = NULL;
    break;
    // High Memory
  case HI_MEM:
    for( u32 addr = start; addr< start+length; addr+=2 )
#if defined(SET_DIRTY)
      block->setDirty(addr);
#else
      block->LookupTable[(addr & 0x000FFFFF) >> 1] = NULL;
#endif
    break;

    // Cache
  default:
    if ((start & 0xFF000000) == 0xC0000000){
      block->LookupTableC[ (start&0x000FFFFF)>>1 ] = NULL;
    }
    break;
  }
}

void SH2DynShowSttaics(SH2_struct * master, SH2_struct * slave ){
  CompileBlocks * block = CompileBlocks::getInstance();

  DynarecSh2 *pctx = (DynarecSh2*)master->ext;
  pctx->ShowStatics();

  DynarecSh2 *pctxs = (DynarecSh2*)slave->ext;
  pctxs->ShowStatics();

  block->ShowStatics();
}

//********************************************************************
// MemoyAcess from DynarecCPU
//********************************************************************
void memSetByte(u32 addr , u8 data )
{
  dynaLock();
  //LOG("memSetWord %08X, %08X\n", addr, data);

  CompileBlocks * block = CompileBlocks::getInstance();
  switch (getMemArea(addr))
  {
  // Low Memory
  case LO_MEM:
    block->LookupTableLow[  (addr&0x000FFFFF)>>1 ] = NULL;
    break;
  // High Memory
  case HI_MEM:
#if defined(SET_DIRTY)
    block->setDirty(addr);
#else
    block->LookupTable[ (addr&0x000FFFFF)>>1 ] = NULL;
#endif
    break;

  // Cache
  default:
    if ((addr & 0xFF000000) == 0xC0000000)
    {
      block->LookupTableC[ (addr&0x000FFFFF)>>1] = NULL;
    }
  }
  SH2MappedMemoryWriteByte(DynarecSh2::CurrentContext->GetContext(), addr, data);
  dynaFree();
}

void memSetWord(u32 addr, u16 data )
{
  dynaLock();
  //LOG("memSetWord %08X, %08X\n", addr, data);

  CompileBlocks * block = CompileBlocks::getInstance();
  switch (getMemArea(addr))
  {
  // Low Memory
   case LO_MEM:
    block->LookupTableLow[ (addr&0x000FFFFF)>>1 ] = NULL;
    break;
  // High Memory
   case HI_MEM: {
#if defined(SET_DIRTY)
     block->setDirty(addr);
#else
     block->LookupTable[(addr & 0x000FFFFF) >> 1] = NULL;
#endif
    
   }
    break;
  // Cache
  default:
    if ((addr & 0xFF000000) == 0xC0000000)
    {
      block->LookupTableC[ (addr&0x000FFFFF) >> 1] = NULL;
    }
  }
  SH2MappedMemoryWriteWord(DynarecSh2::CurrentContext->GetContext(), addr, data);
  dynaFree();
}

void memSetLong(u32 addr , u32 data )
{
  dynaLock();
  //LOG("memSetLong %08X, %08X\n", addr, data);

  CompileBlocks * block = CompileBlocks::getInstance();

  switch (getMemArea(addr))
  {  
    // Low Memory
  case LO_MEM:
    block->LookupTableLow[ (addr & 0x000FFFFF)>>1  ] = NULL;
    block->LookupTableLow[ ((addr & 0x000FFFFF)>>1) + 1 ] = NULL;
    break;
  // High Memory
  case HI_MEM:
#if defined(SET_DIRTY)
    block->setDirty(addr);
    block->setDirty(addr+2);
#else
    block->LookupTable[(addr & 0x000FFFFF) >> 1] = NULL;
    block->LookupTable[((addr & 0x000FFFFF) >> 1) + 1] = NULL;
#endif
    break;

  // Cache
  default:
    if ((addr & 0xFF000000) == 0xC0000000)
    {
      block->LookupTableC[ (addr&0x000FFFFF)>>1 ] = NULL;
    }
  }

  SH2MappedMemoryWriteLong(DynarecSh2::CurrentContext->GetContext(), addr, data);
  dynaFree();
}

u8 memGetByte(u32 addr)
{
  dynaLock();
  u8 val;
  val = SH2MappedMemoryReadByte(DynarecSh2::CurrentContext->GetContext(), addr);
  dynaFree();
  return val;
}
 
u16 memGetWord(u32 addr)
{
  dynaLock();
  u16 val;
  val = SH2MappedMemoryReadWord(DynarecSh2::CurrentContext->GetContext(), addr);
  dynaFree();
  return val;
}



u32 memGetLong(u32 addr)
{
  dynaLock();
  u32 val;
  val = SH2MappedMemoryReadLong(DynarecSh2::CurrentContext->GetContext(), addr);
  dynaFree();
  return val;
}

void SH2DynOnFrame(SH2_struct *context) {

}


//************************************************
// Callbacks from DynarecCPU
//************************************************


int DelayEachClock() {
  return 0;
}

int DebugDelayClock() {
  return 0;
}

int DebugEachClock() {
  dynaLock();

  #define INSTRUCTION_B(x) ((x & 0x0F00) >> 8)
  #define INSTRUCTION_C(x) ((x & 0x00F0) >> 4)

#if 0
  u32 pc = DynarecSh2::CurrentContext->GET_PC();
  u16 inst = memGetWord(pc);
  s32 m = INSTRUCTION_C(inst);
  s32 n = INSTRUCTION_B(inst);

  LOG("%08X: rotcrout R%d=%08X, SR=%08X\n", 
    DynarecSh2::CurrentContext->GET_PC(), 
    n,DynarecSh2::CurrentContext->GetGenRegPtr()[n], 
    DynarecSh2::CurrentContext->GET_SR());
#endif

#if 0  
  LOG("%08X: subc R%d=%08X R%d=%08X SR=%08X\n", 
    DynarecSh2::CurrentContext->GET_PC(), 
    m,DynarecSh2::CurrentContext->GetGenRegPtr()[m], 
    n,DynarecSh2::CurrentContext->GetGenRegPtr()[n], 
    DynarecSh2::CurrentContext->GET_SR());
#endif

#if 0
if( DynarecSh2::CurrentContext->GET_PC() >= 0x0602E3C2 &&  DynarecSh2::CurrentContext->GET_PC() < 0x0602E468 ) {
   u32 addrn = DynarecSh2::CurrentContext->GetGenRegPtr()[6]-4;
   u32 addrm = DynarecSh2::CurrentContext->GetGenRegPtr()[7]-4;
   printf("%08X: MACL R[%d]=%08X@%08X,R[%d]=%08X@%08X,MACH=%08X,MACL=%08X\n",
      DynarecSh2::CurrentContext->GET_PC(),
      6,addrn,MappedMemoryReadLong(addrn),
      7,addrm,MappedMemoryReadLong(addrm),
      DynarecSh2::CurrentContext->GET_MACH(),
      DynarecSh2::CurrentContext->GET_MACL()
   );
}
#endif

#if 0
  #define INSTRUCTION_B(x) ((x & 0x0F00) >> 8)
  #define INSTRUCTION_C(x) ((x & 0x00F0) >> 4)

  u32 pc = DynarecSh2::CurrentContext->GET_PC();
  u16 inst = memGetWord(pc);
  s32 m = INSTRUCTION_C(inst);
  s32 n = INSTRUCTION_B(inst);
  printf("%08X: DIV0S %04X R[%d]:%08X,R[%d]:%08X,SR:%08X\n", 
    DynarecSh2::CurrentContext->GET_PC(), 
    inst,
    m,DynarecSh2::CurrentContext->GetGenRegPtr()[m], 
    n,DynarecSh2::CurrentContext->GetGenRegPtr()[n], 
    DynarecSh2::CurrentContext->GET_SR());
#endif

#if 0
u32 pc = DynarecSh2::CurrentContext->GET_PC();
if( pc == 0x060133C8 ) {
  u16 inst = memGetWord(pc);
  s32 m = INSTRUCTION_C(inst);
  s32 n = INSTRUCTION_B(inst);
  printf("%08X: DIV1(O) m:%08X,n:%08X,SR:%08X\n", 
    DynarecSh2::CurrentContext->GET_PC(), 
    DynarecSh2::CurrentContext->GetGenRegPtr()[m], 
    DynarecSh2::CurrentContext->GetGenRegPtr()[n], 
    DynarecSh2::CurrentContext->GET_SR());
}
#endif


#ifdef DMPHISTORY
  CurrentSH2->pchistory_index++;
  CurrentSH2->pchistory[CurrentSH2->pchistory_index & 0xFF] = DynarecSh2::CurrentContext->GET_PC();
  //CurrentSH2->regshistory[CurrentSH2->pchistory_index & 0xFF] = NULL;
#endif

  if (DynarecSh2::CurrentContext->CheckOneStep()){
    dynaFree();
    return 1;
  }

  dynaFree();
  return 0;
}
  
int EachClock() {
  dynaLock();
  if (DynarecSh2::CurrentContext->CheckInterupt()) {
      dynaFree();
      return 1;
  }
  dynaFree();
  return 0;
}


}
