
/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2007 Theo Berkau

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
/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file vdp2.c
    \brief VDP2 emulation functions
*/

#include <stdlib.h>
#include "vdp2.h"
#include "debug.h"
#include "peripheral.h"
#include "scu.h"
#include "sh2core.h"
#include "smpc.h"
#include "vdp1.h"
#include "yabause.h"
#include "movie.h"
#include "osdcore.h"
#include "threads.h"
#include "yui.h"
#include "frameprofile.h"
#include "vidogl.h"
#include "vidsoft.h"
#include <atomic>
#include "vulkan/VIDVulkanCInterface.h"

u8 * Vdp2Ram;
u8 * Vdp2ColorRam;
Vdp2 * Vdp2Regs;
Vdp2Internal_struct Vdp2Internal;
Vdp2External_struct Vdp2External;

u8 Vdp2ColorRamUpdated = 0;
u8 A0_Updated = 0;
u8 A1_Updated = 0;
u8 B0_Updated = 0;
u8 B1_Updated = 0;

struct CellScrollData cell_scroll_data[270];
Vdp2 Vdp2Lines[270];


u32 skipped_frame = 0;
u32 pre_swap_frame_buffer = 0;
static int autoframeskipenab=0;
static int throttlespeed=0;
u64 lastticks=0;
static int fps;
int vdp2_is_odd_frame = 0;
// Asyn rendering
YabEventQueue * evqueue = NULL; // Event Queue for async rendring
YabEventQueue * rcv_evqueue = NULL;
YabEventQueue * vdp1_rcv_evqueue = NULL;
YabEventQueue * vout_rcv_evqueue = NULL;
static u64 syncticks = 0;       // CPU time sync for real time.
static int vdp_proc_running = 0;
YabMutex * vrammutex = NULL;
int g_frame_count = 0;
static int framestoskip = 0;
static int framesskipped = 0;
static int skipnextframe = 0;
static int previous_skipped = 0;
static u64 curticks = 0;
static u64 diffticks = 0;
static u32 framecount = 0;
static s64 onesecondticks = 0;
static int enableFrameLimit = 1;
static int frameLimitShift = 0;

//#define LOG yprintf
#define PROFILE_RENDERING 0

YabEventQueue * command_ = NULL;

//---------------------------------------------------------------------------------------

extern "C" void * VdpProc(void *arg);      // rendering thread.
static void vdp2VBlankIN(void); // VBLANK-IN handler
static void vdp2VBlankOUT(void);// VBLANK-OUT handler
void VDP2genVRamCyclePattern();
int Vdp2GenerateCCode();


void Vdp1_onHblank();

void VdpLockVram() {
  YabThreadLock(vrammutex);
}

void VdpUnLockVram() {
  YabThreadUnLock(vrammutex);
}


//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp2RamReadByte(u32 addr) {
   addr &= 0x7FFFF;
   return T1ReadByte(Vdp2Ram, addr);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp2RamReadWord(u32 addr) {
   addr &= 0x7FFFF;
   return T1ReadWord(Vdp2Ram, addr);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp2RamReadLong(u32 addr) {
   addr &= 0x7FFFF;
   return T1ReadLong(Vdp2Ram, addr);
}

//////////////////////////////////////////////////////////////////////////////
//#define VRAM_WRITE_CHECK 1
#if VRAM_WRITE_CHECK
int prelinev = 0;
#endif
void FASTCALL Vdp2RamWriteByte(u32 addr, u8 val) {
   addr &= 0x7FFFF;
#if VRAM_WRITE_CHECK
   if (yabsys.LineCount != prelinev) {
     LOG("VRAM: write byte @%d, cycle_a=%d cycle_b=%d A0=%04X%04X A1=%04X%04X B0=%04X%04X B1=%04X%04X ",
       yabsys.LineCount,
       Vdp2External.cpu_cycle_a, Vdp2External.cpu_cycle_b,
       Vdp2Regs->CYCA0L, Vdp2Regs->CYCA0U, Vdp2Regs->CYCA0L, Vdp2Regs->CYCA0U, Vdp2Regs->CYCB0L, Vdp2Regs->CYCB0U, Vdp2Regs->CYCB1L, Vdp2Regs->CYCB1U);
     prelinev = yabsys.LineCount;
   }
#endif
   if (A0_Updated == 0 && addr >= 0 && addr < 0x20000){
     A0_Updated = 1;
   }
   else if (A1_Updated == 0 &&  addr >= 0x20000 && addr < 0x40000){
     A1_Updated = 1;
   }
   else if (B0_Updated == 0 && addr >= 0x40000 && addr < 0x60000){
     B0_Updated = 1;
   }
   else if (B1_Updated == 0 && addr >= 0x60000 && addr < 0x80000){
     B1_Updated = 1;
   }

   T1WriteByte(Vdp2Ram, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2RamWriteWord(u32 addr, u16 val) {
   addr &= 0x7FFFF;
#if VRAM_WRITE_CHECK
   if (yabsys.LineCount != prelinev) {
     LOG("VRAM: write word @%d, cycle_a=%d cycle_b=%d A0=%04X%04X A1=%04X%04X B0=%04X%04X B1=%04X%04X ",
       yabsys.LineCount,
       Vdp2External.cpu_cycle_a, Vdp2External.cpu_cycle_b,
       Vdp2Regs->CYCA0L, Vdp2Regs->CYCA0U, Vdp2Regs->CYCA0L, Vdp2Regs->CYCA0U, Vdp2Regs->CYCB0L, Vdp2Regs->CYCB0U, Vdp2Regs->CYCB1L, Vdp2Regs->CYCB1U);
     prelinev = yabsys.LineCount;
   }
#endif
   if (A0_Updated == 0 && addr >= 0 && addr < 0x20000){
     A0_Updated = 1;
   }
   else if (A1_Updated == 0 && addr >= 0x20000 && addr < 0x40000){
     A1_Updated = 1;
   }
   else if (B0_Updated == 0 && addr >= 0x40000 && addr < 0x60000){
     B0_Updated = 1;
   }
   else if (B1_Updated == 0 && addr >= 0x60000 && addr < 0x80000){
     B1_Updated = 1;
   }

   T1WriteWord(Vdp2Ram, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2RamWriteLong(u32 addr, u32 val) {
  addr &= 0x7FFFF;
#if VRAM_WRITE_CHECK
  if (yabsys.LineCount != prelinev) {
    LOG("VRAM: write long @%d, cycle_a=%d cycle_b=%d A0=%04X%04X A1=%04X%04X B0=%04X%04X B1=%04X%04X ", 
      yabsys.LineCount, 
      Vdp2External.cpu_cycle_a, Vdp2External.cpu_cycle_b,
      Vdp2Regs->CYCA0L, Vdp2Regs->CYCA0U, Vdp2Regs->CYCA0L, Vdp2Regs->CYCA0U, Vdp2Regs->CYCB0L, Vdp2Regs->CYCB0U, Vdp2Regs->CYCB1L, Vdp2Regs->CYCB1U );
    prelinev = yabsys.LineCount;
  }
#endif
  if (A0_Updated == 0 && addr >= 0 && addr < 0x20000){
     A0_Updated = 1;
   }
   else if (A1_Updated == 0 && addr >= 0x20000 && addr < 0x40000){
     A1_Updated = 1;
   }
   else if (B0_Updated == 0 && addr >= 0x40000 && addr < 0x60000){
     B0_Updated = 1;
   }
   else if (B1_Updated == 0 && addr >= 0x60000 && addr < 0x80000){
     B1_Updated = 1;
   }

   T1WriteLong(Vdp2Ram, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp2ColorRamReadByte(u32 addr) {
   addr &= 0xFFF;
   return T2ReadByte(Vdp2ColorRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp2ColorRamReadWord(u32 addr) {
   addr &= 0xFFF;
   return T2ReadWord(Vdp2ColorRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp2ColorRamReadLong(u32 addr) {
   addr &= 0xFFF;
   return T2ReadLong(Vdp2ColorRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteByte(u32 addr, u8 val) {
   addr &= 0xFFF;
   //LOG("[VDP2] Update Coloram Byte %08X:%02X", addr, val);
   T2WriteByte(Vdp2ColorRam, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteWord(u32 addr, u16 val) {
   addr &= 0xFFF;
   //LOG("[VDP2] Update Coloram Word %08X:%04X", addr, val);
   if (Vdp2Internal.ColorMode == 0 ) {
     if (val != T2ReadWord(Vdp2ColorRam, addr)) {
       T2WriteWord(Vdp2ColorRam, addr, val);
       VIDCore->OnUpdateColorRamWord(addr);
     }

     if (addr < 0x800) {
       if (val != T2ReadWord(Vdp2ColorRam, addr + 0x800)) {
         T2WriteWord(Vdp2ColorRam, addr + 0x800, val);
         VIDCore->OnUpdateColorRamWord(addr + 0x800);
       }
     }
   }
   else {
     if (val != T2ReadWord(Vdp2ColorRam, addr)) {
       T2WriteWord(Vdp2ColorRam, addr, val);
       VIDCore->OnUpdateColorRamWord(addr);
     }
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteLong(u32 addr, u32 val) {
   addr &= 0xFFF;
   //LOG("[VDP2] Update Coloram Long %08X:%08X", addr, val);

   if (Vdp2Internal.ColorMode == 0) {

     const u32 base_addr = addr;
     T2WriteLong(Vdp2ColorRam, base_addr, val);
     VIDCore->OnUpdateColorRamWord(base_addr + 2);
     VIDCore->OnUpdateColorRamWord(base_addr);

     if (addr < 0x800) {
       const u32 mirror_addr = base_addr + 0x800;
       T2WriteLong(Vdp2ColorRam, mirror_addr, val);
       VIDCore->OnUpdateColorRamWord(mirror_addr + 2);
       VIDCore->OnUpdateColorRamWord(mirror_addr);
     }
   }
   else {
     T2WriteLong(Vdp2ColorRam, addr, val);
     if (Vdp2Internal.ColorMode == 2) {
       VIDCore->OnUpdateColorRamWord(addr);
     }
     else {
       VIDCore->OnUpdateColorRamWord(addr + 2);
       VIDCore->OnUpdateColorRamWord(addr);
     }
   }

}

//////////////////////////////////////////////////////////////////////////////

int Vdp2Init(void) {
   if ((Vdp2Regs = (Vdp2 *) calloc(1, sizeof(Vdp2))) == NULL)
      return -1;

   if ((Vdp2Ram = T1MemoryInit(0x80000)) == NULL)
      return -1;

   if ((Vdp2ColorRam = T2MemoryInit(0x1000)) == NULL)
      return -1;

   memset(Vdp2Lines, 0, sizeof(Vdp2) * 270);

   Vdp2Reset();

#if defined(YAB_ASYNC_RENDERING)
   if (rcv_evqueue==NULL) rcv_evqueue = YabThreadCreateQueue(8);
   if (vdp1_rcv_evqueue==NULL) vdp1_rcv_evqueue = YabThreadCreateQueue(8);
   if (vout_rcv_evqueue==NULL) vout_rcv_evqueue = YabThreadCreateQueue(2);
   yabsys.wait_line_count = -1;
#endif

   vrammutex = YabThreadCreateMutex();

   command_ = YabThreadCreateQueue(1);


   memset(Vdp2ColorRam, 0xFF, 0x1000);
   for (int i = 0; i < 0x1000; i += 2) {
     VIDCore->OnUpdateColorRamWord(i);
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DeInit(void) {
#if defined(YAB_ASYNC_RENDERING)
   if (vdp_proc_running == 1) {
   	YabAddEventQueue(evqueue,VDPEV_FINSH);
   	//vdp_proc_running = 0;
   	YabThreadWait(YAB_THREAD_VDP);
   }
#endif
   if (Vdp2Regs)
      free(Vdp2Regs);
   Vdp2Regs = NULL;

   if (Vdp2Ram)
      T1MemoryDeInit(Vdp2Ram);
   Vdp2Ram = NULL;

   if (Vdp2ColorRam)
      T2MemoryDeInit(Vdp2ColorRam);
   Vdp2ColorRam = NULL;

   YabThreadFreeMutex(vrammutex);

}

//////////////////////////////////////////////////////////////////////////////

void Vdp2Reset(void) {
   Vdp2Regs->TVMD = 0x0000;
   Vdp2Regs->EXTEN = 0x0000;
   Vdp2Regs->TVSTAT = Vdp2Regs->TVSTAT & 0x1;
   Vdp2Regs->VRSIZE = 0x0000; // fix me(version should be set)
   Vdp2Regs->RAMCTL = 0x0000;
   Vdp2Regs->BGON = 0x0000;
   Vdp2Regs->CHCTLA = 0x0000;
   Vdp2Regs->CHCTLB = 0x0000;
   Vdp2Regs->BMPNA = 0x0000;
   Vdp2Regs->MPOFN = 0x0000;
   Vdp2Regs->MPABN2 = 0x0000;
   Vdp2Regs->MPCDN2 = 0x0000;
   Vdp2Regs->SCXIN0 = 0x0000;
   Vdp2Regs->SCXDN0 = 0x0000;
   Vdp2Regs->SCYIN0 = 0x0000;
   Vdp2Regs->SCYDN0 = 0x0000;
   Vdp2Regs->ZMXN0.all = 0x00000000;
   Vdp2Regs->ZMYN0.all = 0x00000000;
   Vdp2Regs->SCXIN1 = 0x0000;
   Vdp2Regs->SCXDN1 = 0x0000;
   Vdp2Regs->SCYIN1 = 0x0000;
   Vdp2Regs->SCYDN1 = 0x0000;
   Vdp2Regs->ZMXN1.all = 0x00000000;
   Vdp2Regs->ZMYN1.all = 0x00000000;
   Vdp2Regs->SCXN2 = 0x0000;
   Vdp2Regs->SCYN2 = 0x0000;
   Vdp2Regs->SCXN3 = 0x0000;
   Vdp2Regs->SCYN3 = 0x0000;
   Vdp2Regs->ZMCTL = 0x0000;
   Vdp2Regs->SCRCTL = 0x0000;
   Vdp2Regs->VCSTA.all = 0x00000000;
   Vdp2Regs->BKTAU = 0x0000;
   Vdp2Regs->BKTAL = 0x0000;
   Vdp2Regs->RPMD = 0x0000;
   Vdp2Regs->RPRCTL = 0x0000;
   Vdp2Regs->KTCTL = 0x0000;
   Vdp2Regs->KTAOF = 0x0000;
   Vdp2Regs->OVPNRA = 0x0000;
   Vdp2Regs->OVPNRB = 0x0000;
   Vdp2Regs->WPSX0 = 0x0000;
   Vdp2Regs->WPSY0 = 0x0000;
   Vdp2Regs->WPEX0 = 0x0000;
   Vdp2Regs->WPEY0 = 0x0000;
   Vdp2Regs->WPSX1 = 0x0000;
   Vdp2Regs->WPSY1 = 0x0000;
   Vdp2Regs->WPEX1 = 0x0000;
   Vdp2Regs->WPEY1 = 0x0000;
   Vdp2Regs->WCTLA = 0x0000;
   Vdp2Regs->WCTLB = 0x0000;
   Vdp2Regs->WCTLC = 0x0000;
   Vdp2Regs->WCTLD = 0x0000;
   Vdp2Regs->SPCTL = 0x0000;
   Vdp2Regs->SDCTL = 0x0000;
   Vdp2Regs->CRAOFA = 0x0000;
   Vdp2Regs->CRAOFB = 0x0000;
   Vdp2Regs->LNCLEN = 0x0000;
   Vdp2Regs->SFPRMD = 0x0000;
   Vdp2Regs->CCCTL = 0x0000;
   Vdp2Regs->SFCCMD = 0x0000;
   Vdp2Regs->PRISA = 0x0000;
   Vdp2Regs->PRISB = 0x0000;
   Vdp2Regs->PRISC = 0x0000;
   Vdp2Regs->PRISD = 0x0000;
   Vdp2Regs->PRINA = 0x0000;
   Vdp2Regs->PRINB = 0x0000;
   Vdp2Regs->PRIR = 0x0000;
   Vdp2Regs->CCRNA = 0x0000;
   Vdp2Regs->CCRNB = 0x0000;
   Vdp2Regs->CLOFEN = 0x0000;
   Vdp2Regs->CLOFSL = 0x0000;
   Vdp2Regs->COAR = 0x0000;
   Vdp2Regs->COAG = 0x0000;
   Vdp2Regs->COAB = 0x0000;
   Vdp2Regs->COBR = 0x0000;
   Vdp2Regs->COBG = 0x0000;
   Vdp2Regs->COBB = 0x0000;

   yabsys.VBlankLineCount = 225;
   Vdp2Internal.ColorMode = 0;

   Vdp2External.disptoggle = 0xFF;
   Vdp2External.perline_alpha_a = 0;
   Vdp2External.perline_alpha_b = 0;
   Vdp2External.perline_alpha = &Vdp2External.perline_alpha_a;
   Vdp2External.perline_alpha_draw = &Vdp2External.perline_alpha_b;
   Vdp2External.cpu_cycle_a = 0;
   Vdp2External.cpu_cycle_b = 0;

#if defined(YAB_ASYNC_RENDERING)
   if (rcv_evqueue != NULL){
     YabThreadDestoryQueue(rcv_evqueue);
     rcv_evqueue = YabThreadCreateQueue(8);
   }
   if (vdp1_rcv_evqueue != NULL){
     YabThreadDestoryQueue(vdp1_rcv_evqueue);
     vdp1_rcv_evqueue = YabThreadCreateQueue(8);
   }
   yabsys.wait_line_count = -1;
#endif

}


///////////////////////////////////////////////////////////////////////////////
extern "C" void * VdpProc( void *arg ){

  int evcode;

  if( YuiUseOGLOnThisThread() < 0 ){
    LOG("VDP2 Fail to USE GL");
    return NULL;
  }

  while( vdp_proc_running ){
#if defined(__RP64__) || defined(__N2__)	  
    YabThreadSetCurrentThreadAffinityMask(0x5);
#else
    YabThreadSetCurrentThreadAffinityMask(0x1);
#endif
    evcode = YabWaitEventQueue(evqueue);
    switch(evcode){
    case VDPEV_VBLANK_IN:
      FrameProfileAdd("VIN start");
      vdp2VBlankIN();
      FrameProfileAdd("VIN end");
      break;
    case VDPEV_VBLANK_OUT:
      FrameProfileAdd("VOUT start");
      vdp2VBlankOUT();
      FrameProfileAdd("VOUT end");
      //YabAddEventQueue(vout_rcv_evqueue, 0);
      break;
    case VDPEV_DIRECT_DRAW:
      FrameProfileAdd("DirectDraw start");
      FRAMELOG("VDP1: VDPEV_DIRECT_DRAW(T)");
      Vdp1Draw();
      VIDCore->Vdp1DrawEnd();
      Vdp1External.frame_change_plot = 0;
      FrameProfileAdd("DirectDraw end");
      YabAddEventQueue(vdp1_rcv_evqueue, 0);
      break;
    case VDPEV_MAKECURRENT:
      YuiUseOGLOnThisThread();
      YabAddEventQueue(command_,0);
      break;
    case VDPEV_REVOKE:
      YuiRevokeOGLOnThisThread();
      YabAddEventQueue(command_,0);
      break;
    case VDPEV_FINSH:
      vdp_proc_running = 0;
      break;
    }
  }
  return NULL;
}

void VDP2genVRamCyclePattern() {
  int cpu_cycle_a = 0;
  int cpu_cycle_b = 0;
  int i = 0;

  Vdp2External.AC_VRAM[0][0] = (Vdp2Regs->CYCA0L >> 12) & 0x0F;
  Vdp2External.AC_VRAM[0][1] = (Vdp2Regs->CYCA0L >> 8) & 0x0F;
  Vdp2External.AC_VRAM[0][2] = (Vdp2Regs->CYCA0L >> 4) & 0x0F;
  Vdp2External.AC_VRAM[0][3] = (Vdp2Regs->CYCA0L >> 0) & 0x0F;
  Vdp2External.AC_VRAM[0][4] = (Vdp2Regs->CYCA0U >> 12) & 0x0F;
  Vdp2External.AC_VRAM[0][5] = (Vdp2Regs->CYCA0U >> 8) & 0x0F;
  Vdp2External.AC_VRAM[0][6] = (Vdp2Regs->CYCA0U >> 4) & 0x0F;
  Vdp2External.AC_VRAM[0][7] = (Vdp2Regs->CYCA0U >> 0) & 0x0F;

  for (i = 0; i < 8; i++) {
    if (Vdp2External.AC_VRAM[0][i] >= 0x0E) {
      cpu_cycle_a++;
    }
    else if (Vdp2External.AC_VRAM[0][i] >= 4 && Vdp2External.AC_VRAM[0][i] <= 7) {
      if ((Vdp2Regs->BGON & (1 << (Vdp2External.AC_VRAM[0][i] - 4))) == 0) {
        cpu_cycle_a++;
      }
    }
  }

  if (Vdp2Regs->RAMCTL & 0x100) {
    int fcnt = 0;
    Vdp2External.AC_VRAM[1][0] = (Vdp2Regs->CYCA1L >> 12) & 0x0F;
    Vdp2External.AC_VRAM[1][1] = (Vdp2Regs->CYCA1L >> 8) & 0x0F;
    Vdp2External.AC_VRAM[1][2] = (Vdp2Regs->CYCA1L >> 4) & 0x0F;
    Vdp2External.AC_VRAM[1][3] = (Vdp2Regs->CYCA1L >> 0) & 0x0F;
    Vdp2External.AC_VRAM[1][4] = (Vdp2Regs->CYCA1U >> 12) & 0x0F;
    Vdp2External.AC_VRAM[1][5] = (Vdp2Regs->CYCA1U >> 8) & 0x0F;
    Vdp2External.AC_VRAM[1][6] = (Vdp2Regs->CYCA1U >> 4) & 0x0F;
    Vdp2External.AC_VRAM[1][7] = (Vdp2Regs->CYCA1U >> 0) & 0x0F;

    for (i = 0; i < 8; i++) {
      if (Vdp2External.AC_VRAM[0][i] == 0x0E) {
        if (Vdp2External.AC_VRAM[1][i] != 0x0E) {
          cpu_cycle_a--;
        }
        else {
          if (fcnt == 0) {
            cpu_cycle_a--;
          }
        }
      }
      if (Vdp2External.AC_VRAM[1][i] == 0x0F) {
        fcnt++;
      }
    }
    if (fcnt == 0)cpu_cycle_a = 0;
    if (cpu_cycle_a < 0)cpu_cycle_a = 0;
  }
  else {
    Vdp2External.AC_VRAM[1][0] = Vdp2External.AC_VRAM[0][0];
    Vdp2External.AC_VRAM[1][1] = Vdp2External.AC_VRAM[0][1];
    Vdp2External.AC_VRAM[1][2] = Vdp2External.AC_VRAM[0][2];
    Vdp2External.AC_VRAM[1][3] = Vdp2External.AC_VRAM[0][3];
    Vdp2External.AC_VRAM[1][4] = Vdp2External.AC_VRAM[0][4];
    Vdp2External.AC_VRAM[1][5] = Vdp2External.AC_VRAM[0][5];
    Vdp2External.AC_VRAM[1][6] = Vdp2External.AC_VRAM[0][6];
    Vdp2External.AC_VRAM[1][7] = Vdp2External.AC_VRAM[0][7];
  }

  Vdp2External.AC_VRAM[2][0] = (Vdp2Regs->CYCB0L >> 12) & 0x0F;
  Vdp2External.AC_VRAM[2][1] = (Vdp2Regs->CYCB0L >> 8) & 0x0F;
  Vdp2External.AC_VRAM[2][2] = (Vdp2Regs->CYCB0L >> 4) & 0x0F;
  Vdp2External.AC_VRAM[2][3] = (Vdp2Regs->CYCB0L >> 0) & 0x0F;
  Vdp2External.AC_VRAM[2][4] = (Vdp2Regs->CYCB0U >> 12) & 0x0F;
  Vdp2External.AC_VRAM[2][5] = (Vdp2Regs->CYCB0U >> 8) & 0x0F;
  Vdp2External.AC_VRAM[2][6] = (Vdp2Regs->CYCB0U >> 4) & 0x0F;
  Vdp2External.AC_VRAM[2][7] = (Vdp2Regs->CYCB0U >> 0) & 0x0F;

  for (i = 0; i < 8; i++) {
    if (Vdp2External.AC_VRAM[2][i] >= 0x0E) {
      cpu_cycle_b++;
    }
    else if (Vdp2External.AC_VRAM[2][i] >= 4 && Vdp2External.AC_VRAM[2][i] <= 7) {
      if ((Vdp2Regs->BGON & (1 << (Vdp2External.AC_VRAM[2][i] - 4))) == 0) {
        cpu_cycle_b++;
      }
    }
  }


  if (Vdp2Regs->RAMCTL & 0x200) {
    int fcnt = 0;
    Vdp2External.AC_VRAM[3][0] = (Vdp2Regs->CYCB1L >> 12) & 0x0F;
    Vdp2External.AC_VRAM[3][1] = (Vdp2Regs->CYCB1L >> 8) & 0x0F;
    Vdp2External.AC_VRAM[3][2] = (Vdp2Regs->CYCB1L >> 4) & 0x0F;
    Vdp2External.AC_VRAM[3][3] = (Vdp2Regs->CYCB1L >> 0) & 0x0F;
    Vdp2External.AC_VRAM[3][4] = (Vdp2Regs->CYCB1U >> 12) & 0x0F;
    Vdp2External.AC_VRAM[3][5] = (Vdp2Regs->CYCB1U >> 8) & 0x0F;
    Vdp2External.AC_VRAM[3][6] = (Vdp2Regs->CYCB1U >> 4) & 0x0F;
    Vdp2External.AC_VRAM[3][7] = (Vdp2Regs->CYCB1U >> 0) & 0x0F;

    for (i = 0; i < 8; i++) {
      if (Vdp2External.AC_VRAM[2][i] == 0x0E) {
        if (Vdp2External.AC_VRAM[3][i] != 0x0E) {
          cpu_cycle_b--;
        }
        else {
          if (fcnt == 0) {
            cpu_cycle_b--;
          }
        }
      }
      if (Vdp2External.AC_VRAM[3][i] == 0x0F) {
        fcnt++;
      }
    }
    if (fcnt == 0)cpu_cycle_b = 0;
    if (cpu_cycle_b < 0)cpu_cycle_b = 0;
  }
  else {
    Vdp2External.AC_VRAM[3][0] = Vdp2External.AC_VRAM[2][0];
    Vdp2External.AC_VRAM[3][1] = Vdp2External.AC_VRAM[2][1];
    Vdp2External.AC_VRAM[3][2] = Vdp2External.AC_VRAM[2][2];
    Vdp2External.AC_VRAM[3][3] = Vdp2External.AC_VRAM[2][3];
    Vdp2External.AC_VRAM[3][4] = Vdp2External.AC_VRAM[2][4];
    Vdp2External.AC_VRAM[3][5] = Vdp2External.AC_VRAM[2][5];
    Vdp2External.AC_VRAM[3][6] = Vdp2External.AC_VRAM[2][6];
    Vdp2External.AC_VRAM[3][7] = Vdp2External.AC_VRAM[2][7];
  }

  if (cpu_cycle_a == 0) {
    Vdp2External.cpu_cycle_a = 200;
  }
  else if (Vdp2External.cpu_cycle_a == 1) {
    Vdp2External.cpu_cycle_a = 24;
  }
  else {
    Vdp2External.cpu_cycle_a = 2;
  }

  if (cpu_cycle_b == 0) {
    Vdp2External.cpu_cycle_b = 200;
  }
  else if (Vdp2External.cpu_cycle_a == 1) {
    Vdp2External.cpu_cycle_b = 24;
  }
  else {
    Vdp2External.cpu_cycle_b = 2;
  }
}

// 0 .. 60Hz, 1 .. no limit, 2 .. 2x(120Hz)
void VDP2SetFrameLimit(int mode) {
  switch (mode) {
  case 0:
    enableFrameLimit = 1;
    frameLimitShift = 0; // 60Hz
    framecount = 0;
    onesecondticks = 0;
    lastticks = YabauseGetTicks();
    break;
  case 1:
    enableFrameLimit = 0;
    frameLimitShift = 0;
    break;
  case 2:
    enableFrameLimit = 1;
    frameLimitShift = 1; // 120Hz
    framecount = 0;
    onesecondticks = 0;
    lastticks = YabauseGetTicks();
    break;
  default:
    enableFrameLimit = 1;
    frameLimitShift = 0;
    framecount = 0;
    onesecondticks = 0;
    lastticks = YabauseGetTicks();
    break;
  }
}

void frameSkipAndLimit() {
  if (FrameAdvanceVariable == 0 && enableFrameLimit )
  {
    const u32 fps = (yabsys.IsPal ? 50 : 60) << frameLimitShift ;
    framecount++;
    curticks = YabauseGetTicks();
    if (framecount > fps)
    {
      onesecondticks -= yabsys.tickfreq;
      if (onesecondticks > (s64)( (yabsys.OneFrameTime>>frameLimitShift)  * 4)) {
        onesecondticks = 0;
      }
      framecount = 1;
      lastticks = (curticks - (yabsys.OneFrameTime>>frameLimitShift) );
    }

    u64 targetTime = ( (yabsys.OneFrameTime>>frameLimitShift)  * (u64)framecount);
    if (framecount == fps) {
      targetTime = yabsys.tickfreq; // 1sec
    }

    diffticks = curticks - lastticks;

    if ( autoframeskipenab && (onesecondticks + diffticks) > targetTime )
    {
      // Skip the next frame
      skipnextframe = 1;

      // How many frames should we skip?
      framestoskip = 1;

    }
    
    if ( (onesecondticks + diffticks) < targetTime)
    {
      YabNanosleep(targetTime - (onesecondticks + diffticks));
    }

    onesecondticks += diffticks;
    lastticks = curticks;
  }

}


//////////////////////////////////////////////////////////////////////////////
void vdp2VBlankIN(void) {
   /* this should be done after a frame change or a plot trigger */
   //Vdp1Regs->COPR = 0;
   //printf("COPR = 0 at %d\n", __LINE__);
   /* I'm not 100% sure about this, but it seems that when using manual change
   we should swap framebuffers in the "next field" and thus, clear the CEF...
   now we're lying a little here as we're not swapping the framebuffers. */
   //if (Vdp1External.manualchange) Vdp1Regs->EDSR >>= 1;

   VIDCore->Vdp2DrawEnd();
   frameSkipAndLimit();
   VIDCore->Sync();
   Vdp2Regs->TVSTAT |= 0x0008;

   ScuSendVBlankIN();

   //if (yabsys.IsSSH2Running)
   //   SH2SendInterrupt(SSH2, 0x43, 0x6);
   FrameProfileAdd("VIN flag");
   FRAMELOG("**** VIN(T) *****\n");
   YabAddEventQueue(rcv_evqueue, 0);
   

}

//////////////////////////////////////////////////////////////////////////////
void Vdp2VBlankIN(void) {
  FRAMELOG("***** VIN *****");

#if defined(YAB_ASYNC_RENDERING)
  if( vdp_proc_running == 0 ){
    vdp_proc_running = 1;
    YuiRevokeOGLOnThisThread();
    evqueue = YabThreadCreateQueue(32);
    YabThreadStart(YAB_THREAD_VDP, VdpProc, NULL);
  }

  FrameProfileAdd("VIN event");
  YabAddEventQueue(evqueue,VDPEV_VBLANK_IN);

   // sync
  //do {
    YabWaitEventQueue(rcv_evqueue);
  //} while (YaGetQueueSize(rcv_evqueue) != 0);
   FrameProfileAdd("VIN sync");

#else
	FrameProfileAdd("VIN start");
   /* this should be done after a frame change or a plot trigger */
   //Vdp1Regs->COPR = 0;
   //printf("COPR = 0 at %d\n", __LINE__);

   /* I'm not 100% sure about this, but it seems that when using manual change
   we should swap framebuffers in the "next field" and thus, clear the CEF...
   now we're lying a little here as we're not swapping the framebuffers. */
   //if (Vdp1External.manualchange) Vdp1Regs->EDSR >>= 1;

   VIDCore->Vdp2DrawEnd();
   frameSkipAndLimit();
   VIDCore->Sync();
   Vdp2Regs->TVSTAT |= 0x0008;

   ScuSendVBlankIN();

   //if (yabsys.IsSSH2Running)
   //   SH2SendInterrupt(SSH2, 0x40, 0x0F);

   FrameProfileAdd("VIN end");
#endif
}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

void Vdp2HBlankIN(void) {

  if (yabsys.LineCount < yabsys.VBlankLineCount) {
    Vdp2Regs->TVSTAT |= 0x0004;
    ScuSendHBlankIN();
    //if (yabsys.IsSSH2Running)
    //  SH2SendInterrupt(SSH2, 0x42, 0x2);
  }
}

using std::atomic;
extern atomic<int> vdp1_clock;


void Vdp2HBlankOUT(void) {
  int i;
  if (yabsys.LineCount < yabsys.VBlankLineCount)
  {
    Vdp2Regs->TVSTAT &= ~0x0004;
    u32 cell_scroll_table_start_addr = (Vdp2Regs->VCSTA.all & 0x7FFFE) << 1;
    memcpy(Vdp2Lines + yabsys.LineCount, Vdp2Regs, sizeof(Vdp2));
    for (i = 0; i < 88; i++)
    {
      cell_scroll_data[yabsys.LineCount].data[i] = Vdp2RamReadLong(cell_scroll_table_start_addr + i * 4);
    }


    if ((Vdp2Lines[0].BGON & 0x01) != (Vdp2Lines[yabsys.LineCount].BGON & 0x01)){
      *Vdp2External.perline_alpha |= 0x1;
    }
    else if ((Vdp2Lines[0].CCRNA & 0x00FF) != (Vdp2Lines[yabsys.LineCount].CCRNA & 0x00FF)){
      *Vdp2External.perline_alpha |= 0x1;
    }

    if ((Vdp2Lines[0].BGON & 0x02) != (Vdp2Lines[yabsys.LineCount].BGON & 0x02)){
      *Vdp2External.perline_alpha |= 0x2;
    }
    else if ((Vdp2Lines[0].CCRNA & 0xFF00) != (Vdp2Lines[yabsys.LineCount].CCRNA & 0xFF00)){
      *Vdp2External.perline_alpha |= 0x2;
    }

    if ((Vdp2Lines[0].BGON & 0x04) != (Vdp2Lines[yabsys.LineCount].BGON & 0x04)){
      *Vdp2External.perline_alpha |= 0x4;
    }
    else if ((Vdp2Lines[0].CCRNB & 0xFF00) != (Vdp2Lines[yabsys.LineCount].CCRNB & 0xFF00)){
      *Vdp2External.perline_alpha |= 0x4;
    }

    if ((Vdp2Lines[0].BGON & 0x08) != (Vdp2Lines[yabsys.LineCount].BGON & 0x08)){
      *Vdp2External.perline_alpha |= 0x8;
    }
    else if ((Vdp2Lines[0].CCRNB & 0x00FF) != (Vdp2Lines[yabsys.LineCount].CCRNB & 0x00FF)){
      *Vdp2External.perline_alpha |= 0x8;
    }

    if ((Vdp2Lines[0].BGON & 0x10) != (Vdp2Lines[yabsys.LineCount].BGON & 0x10)){
      *Vdp2External.perline_alpha |= 0x10;
    }
    else if (Vdp2Lines[0].CCRR != Vdp2Lines[yabsys.LineCount].CCRR){
      *Vdp2External.perline_alpha |= 0x10;
    }

    if (Vdp2Lines[0].COBR != Vdp2Lines[yabsys.LineCount].COBR){

      *Vdp2External.perline_alpha |= Vdp2Lines[yabsys.LineCount].CLOFEN;
    }
    if (Vdp2Lines[0].COAR != Vdp2Lines[yabsys.LineCount].COAR){

      *Vdp2External.perline_alpha |= Vdp2Lines[yabsys.LineCount].CLOFEN;
    }

    if (Vdp2Lines[0].CLOFSL != Vdp2Lines[yabsys.LineCount].CLOFSL) {

      *Vdp2External.perline_alpha |= Vdp2Lines[yabsys.LineCount].CLOFEN;
    }

    if (Vdp2Lines[0].PRISA != Vdp2Lines[yabsys.LineCount].PRISA) {

      *Vdp2External.perline_alpha |= 0x40;
    }

    if ( Vdp2Lines[0].SCYN2 != Vdp2Lines[yabsys.LineCount].SCYN2  ||  Vdp2Lines[0].SCXN2 != Vdp2Lines[yabsys.LineCount].SCXN2 ) {

      *Vdp2External.perline_alpha |= 0x100;
    }

    if ( Vdp2Lines[0].SCYN3 != Vdp2Lines[yabsys.LineCount].SCYN3  ||  Vdp2Lines[0].SCXN3 != Vdp2Lines[yabsys.LineCount].SCXN3 ) {

      *Vdp2External.perline_alpha |= 0x80;
    }


    if (Vdp2Lines[0].PRINA != Vdp2Lines[yabsys.LineCount].PRINA) {
      //printf("Perline priority");
    }
  }

  if (yabsys.LineCount == 1) {
    VDP2genVRamCyclePattern();
    Vdp2External.frame_render_flg = 0;
  }

  if (Vdp2External.frame_render_flg == 0 && vdp1_clock>0 ){ // Delay if vdp1 ram was written
    FrameProfileAdd("VOUT event");
    Vdp2External.frame_render_flg = 1;
    // Manual Change
    if (Vdp1External.manualchange == 1) {
      Vdp1External.swap_frame_buffer = 1;
      Vdp1External.manualchange = 0;
    }

    // One Cyclemode
    if ((Vdp1Regs->FBCR & 0x03) == 0x00 ||
      (Vdp1Regs->FBCR & 0x03) == 0x01) {  // 0x01 is treated as one cyscle mode in Sonic R.
      Vdp1External.swap_frame_buffer = 1;
    }

    // Plot trigger mode = Draw when frame is changed
    if (Vdp1Regs->PTMR == 2) {
      Vdp1External.frame_change_plot = 1;
      FRAMELOG("frame_change_plot 1");
    }
    else {
      Vdp1External.frame_change_plot = 0;
      FRAMELOG("frame_change_plot 0");
    }
#if defined(YAB_ASYNC_RENDERING)
    if (vdp_proc_running == 0) {
      YuiRevokeOGLOnThisThread();
      evqueue = YabThreadCreateQueue(32);
      vdp_proc_running = 1;
      YabThreadStart(YAB_THREAD_VDP, VdpProc, NULL);
    }
    if (Vdp1External.swap_frame_buffer == 1)
    {
      Vdp1Regs->EDSR >>= 1;
      if (Vdp1External.frame_change_plot == 1) {
        yabsys.wait_line_count += 45;
        yabsys.wait_line_count %= yabsys.VBlankLineCount;
        FRAMELOG("SET Vdp1 end wait at %d", yabsys.wait_line_count);
      }
    }
    //YabClearEventQueue(vdp1_rcv_evqueue);
    if (YaGetQueueSize(vdp1_rcv_evqueue) != 0) {
      FRAMELOG("YaGetQueueSizeYaGetQueueSize !=0  %d", YaGetQueueSize(vdp1_rcv_evqueue));
    }

    FRAMELOG("YabAddEventQueue(evqueue, VDPEV_VBLANK_OUT)");
    YabAddEventQueue(evqueue, VDPEV_VBLANK_OUT);
    YabThreadYield();
    //YabThreadUSleep(10000);

  }
  if (yabsys.wait_line_count != -1 && yabsys.LineCount == yabsys.wait_line_count) {
      if (Vdp1External.status == VDP1_STATUS_IDLE) {
        FRAMELOG("**WAIT START %d %d**", yabsys.wait_line_count, YaGetQueueSize(vdp1_rcv_evqueue));
        YabWaitEventQueue(vdp1_rcv_evqueue); // sync VOUT
        YabClearEventQueue(vdp1_rcv_evqueue);
        FRAMELOG("**WAIT END**");
        FrameProfileAdd("DirectDraw sync");        
        ScuSendDrawEnd();
        FRAMELOG("Vdp1Draw end at %d line EDSR=%02X", yabsys.LineCount, Vdp1Regs->EDSR);
        yabsys.wait_line_count = -1;
        Vdp1Regs->EDSR |= 2;
      } else {
        yabsys.wait_line_count += 10;
        yabsys.wait_line_count %= yabsys.VBlankLineCount;
        FRAMELOG("Vdp1Draw wait at %d line EDSR=%02X", yabsys.LineCount, Vdp1Regs->EDSR);
      }
  }
#else
    vdp2VBlankOUT();
  }
  
  if (yabsys.wait_line_count != -1 && yabsys.LineCount == yabsys.wait_line_count) {
    //Vdp1Regs->COPR = Vdp1Regs->addr >> 3;
    //printf("COPR = %d at %d\n", Vdp1Regs->COPR, __LINE__);
    if ( Vdp1External.status == VDP1_STATUS_IDLE) {
      ScuSendDrawEnd();
      FRAMELOG("Vdp1Draw end at %d line EDSR=%02X", yabsys.LineCount, Vdp1Regs->EDSR);
      yabsys.wait_line_count = -1;
      Vdp1Regs->EDSR |= 2;
    }
    else {
      yabsys.wait_line_count += 10;
      yabsys.wait_line_count %= yabsys.VBlankLineCount;
    }
    //VIDCore->Vdp1DrawEnd();
  }
  
#endif
  Vdp1_onHblank();
}

//////////////////////////////////////////////////////////////////////////////

Vdp2 * Vdp2RestoreRegs(int line, Vdp2* lines) {
   return line > 270 ? NULL : lines + line;
}

//////////////////////////////////////////////////////////////////////////////
int vdp1_frame = 0;
int show_vdp1_frame = 0;
u32 show_skipped_frame = 0;
static void FPSDisplay(void)
{
  static int fpsframecount = 0;
  static u64 fpsticks;
  //yprintf("%02d/%02d FPS skip=%d vdp1=%02d", fps, yabsys.IsPal ? 50 : 60, show_skipped_frame, show_vdp1_frame);
#if 1 // FPS only
   OSDPushMessage(OSDMSG_FPS, 1, "%02d/%02d FPS skip=%d vdp1=%02d", fps, yabsys.IsPal ? 50 : 60, show_skipped_frame, show_vdp1_frame);
   //printf("\033[%d;%dH %02d/%02d FPS skip=%d vdp1=%02d \n", 0, 0, fps, yabsys.IsPal ? 50 : 60, show_skipped_frame, show_vdp1_frame);
#else
  FILE * fp = NULL;
  FILE * gup_fp = NULL;
  char fname[128];
  char buf[64];
  int i;
  int cpu_f[8];
  int gpu_f;

  if (gup_fp == NULL){
    gup_fp = fopen("/sys/class/kgsl/kgsl-3d0/devfreq/cur_freq", "r");
  }

  if (gup_fp != NULL){
    fread(buf, 1, 64, gup_fp);
    gpu_f = atoi(buf);
    fclose(gup_fp);
  }
  else{
    gpu_f = 0;
  }

  for (i = 0; i < 8; i++){
    sprintf(fname, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
    fp = fopen(fname, "r");
    if (fp){
      fread(buf, 1, 64, fp);
      cpu_f[i] = atoi(buf);
      fclose(fp);
    }
    else{
      cpu_f[i] = 0;
    }
  }


  OSDPushMessage(OSDMSG_FPS, 1, "%02d/%02d FPS , gpu = %d, cpu0 = %d, cpu1 = %d, cpu2 = %d, cpu3 = %d, cpu4 = %d, cpu5 = %d, cpu6 = %d, cpu7 = %d"
    , fps, yabsys.IsPal ? 50 : 60, gpu_f / 1000000,
    cpu_f[0] / 1000, cpu_f[1] / 1000, cpu_f[2] / 1000, cpu_f[3] / 1000,
    cpu_f[4] / 1000, cpu_f[5] / 1000, cpu_f[6] / 1000, cpu_f[7] / 1000);
#endif   
  //OSDPushMessage(OSDMSG_DEBUG, 1, "%d %d %s %s", framecounter, lagframecounter, MovieStatus, InputDisplayString);
  fpsframecount++;
  if (YabauseGetTicks() >= fpsticks + yabsys.tickfreq)
  {
    fps = fpsframecount;
    fpsframecount = 0;
    show_vdp1_frame = vdp1_frame;
    vdp1_frame = 0;
    show_skipped_frame = skipped_frame;
    skipped_frame = 0;
    fpsticks = YabauseGetTicks();
  }
}

//////////////////////////////////////////////////////////////////////////////

void SpeedThrottleEnable(void) {
  throttlespeed = 1;
}

//////////////////////////////////////////////////////////////////////////////

void SpeedThrottleDisable(void) {
  throttlespeed = 0;
}

void dumpvram() {
  FILE * fp = fopen("vdp2vram.bin", "wb");
  fwrite(Vdp2Regs, sizeof(Vdp2), 1, fp);
  fwrite(Vdp2Ram, 0x80000, 1, fp);
  fwrite(Vdp2ColorRam, 0x1000, 1, fp);
  fwrite(&Vdp2Internal, sizeof(Vdp2Internal_struct), 1, fp);
  fwrite((void *)Vdp1Regs, sizeof(Vdp1), 1, fp);
  fwrite((void *)Vdp1Ram, 0x80000, 1, fp);
  fwrite(&Vdp1External, sizeof(Vdp1External_struct), 1, fp);
  fclose(fp);
}

void restorevram() {
  FILE * fp = fopen("vdp2vram.bin", "rb");
  fread(Vdp2Regs, sizeof(Vdp2), 1, fp);
  fread(Vdp2Ram, 0x80000, 1, fp);
  fread(Vdp2ColorRam, 0x1000, 1, fp);
  fread(&Vdp2Internal, sizeof(Vdp2Internal_struct), 1, fp);
  fread((void *)Vdp1Regs, sizeof(Vdp1), 1, fp);
  fread((void *)Vdp1Ram, 0x80000, 1, fp);
  fread(&Vdp1External, sizeof(Vdp1External_struct), 1, fp);
  fclose(fp);

  for (int i = 0; i < 0x1000; i += 2) {
    VIDCore->OnUpdateColorRamWord(i);
  }
}

int g_vdp_debug_dmp = 0;

void vdp2ReqDump() {
  g_vdp_debug_dmp = 2;
}

void vdp2ReqRestore() {
  g_vdp_debug_dmp = 1;
}


//////////////////////////////////////////////////////////////////////////////
void vdp2VBlankOUT(void) {
  static VideoInterface_struct * saved = NULL;
  int isrender = 0;
#if PROFILE_RENDERING
  u64 starttime = YabauseGetTicks();
#endif
  VdpLockVram();
  FRAMELOG("***** VOUT(T) swap=%d,plot=%d,vdp1status=%d*****", Vdp1External.swap_frame_buffer, Vdp1External.frame_change_plot, Vdp1External.status );

#if _DEBUG
  if (g_vdp_debug_dmp == 1) {
    g_vdp_debug_dmp = 0;
    restorevram();
  }

  if (g_vdp_debug_dmp == 2) {
    g_vdp_debug_dmp = 0;
    dumpvram();
    Vdp2GenerateCCode();
  }
#endif 

  if (pre_swap_frame_buffer == 0 && skipnextframe && Vdp1External.swap_frame_buffer ){
    skipnextframe = 0;
    previous_skipped = 0;
    framestoskip = 1;
  }

  if (previous_skipped != 0 && skipnextframe != 0) {
    skipnextframe = 0;
    previous_skipped = 0;
    framestoskip = 1;
  }

  pre_swap_frame_buffer = Vdp1External.swap_frame_buffer;


  if (skipnextframe && (!saved))
  {
    skipped_frame++;
    saved = VIDCore;
    
    previous_skipped = 1;
    VIDCore->Vdp2DrawStart = VIDDummy.Vdp2DrawStart;
    VIDCore->Vdp2DrawEnd   = VIDDummy.Vdp2DrawEnd;
    VIDCore->Vdp2DrawScreens = VIDDummy.Vdp2DrawScreens;

  }
  else if (saved && (!skipnextframe))
  {
    skipnextframe = 0;
    previous_skipped = 0;
    //VIDCore = saved;
    if( saved != NULL ){

      if (VIDCore->id == VIDCORE_VULKAN) {
#if defined(HAVE_VULKAN)
        VIDCore->Vdp2DrawStart = VIDVulkanVdp2DrawStart;
        VIDCore->Vdp2DrawEnd = VIDVulkanVdp2DrawEnd;
        VIDCore->Vdp2DrawScreens = VIDVulkanVdp2DrawScreens;
#endif
      }
      else if (VIDCore->id == VIDCORE_OGL) {
        VIDCore->Vdp2DrawStart = VIDOGLVdp2DrawStart;
        VIDCore->Vdp2DrawEnd = VIDOGLVdp2DrawEnd;
        VIDCore->Vdp2DrawScreens = VIDOGLVdp2DrawScreens;
      }
      else if (VIDCore->id == VIDCORE_SOFT ) {
        VIDCore->Vdp2DrawStart = VIDSoftVdp2DrawStart;
        VIDCore->Vdp2DrawEnd = VIDSoftVdp2DrawEnd;
        VIDCore->Vdp2DrawScreens = VIDSoftVdp2DrawScreens;
      }
    }
    saved = NULL;
  }

  VIDCore->Vdp2DrawStart();

  // VBlank Erase
  if (Vdp1External.vbalnk_erase ||  // VBlank Erace (VBE1) 
    ((Vdp1Regs->FBCR & 2) == 0)){  // One cycle mode
    VIDCore->Vdp1EraseWrite();
  }

  // Frame Change
  if (Vdp1External.swap_frame_buffer == 1)
  {
    vdp1_frame++;
    if (Vdp1External.manualerase){  // Manual Erace (FCM1 FCT0) Just before frame changing
      VIDCore->Vdp1EraseWrite();
      Vdp1External.manualerase = 0;
    }

    FRAMELOG("Vdp1FrameChange swap=%d,plot=%d*****", Vdp1External.swap_frame_buffer, Vdp1External.frame_change_plot);
    VIDCore->Vdp1FrameChange();
    Vdp1External.current_frame = !Vdp1External.current_frame;
    Vdp1External.swap_frame_buffer = 0;
#if !defined(YAB_ASYNC_RENDERING)
    Vdp1Regs->EDSR >>= 1;
#endif

    FRAMELOG("[VDP1] Displayed framebuffer changed. EDSR=%02X", Vdp1Regs->EDSR);

    // if Plot Trigger mode == 0x02 draw start
    if (Vdp1External.frame_change_plot == 1 || Vdp1External.status == VDP1_STATUS_RUNNING ){
      FRAMELOG("[VDP1] frame_change_plot == 1 start drawing immidiatly", Vdp1Regs->EDSR);
      LOG("[VDP1] Start Drawing");
      Vdp1Regs->addr = 0;
      Vdp1Regs->COPR = 0;
      Vdp1Draw();
      isrender = 1;
    }
  }
  else {
    if ( Vdp1External.status == VDP1_STATUS_RUNNING) {
      LOG("[VDP1] Start Drawing continue");
      Vdp1Draw();
      isrender = 1;
    }
  }

#if defined(YAB_ASYNC_RENDERING)
  if (isrender){
    YabAddEventQueue(vdp1_rcv_evqueue, 0);
  }
#else
  //yabsys.wait_line_count = 45;
#endif

  if (Vdp2Regs->TVMD & 0x8000) {
     FRAMELOG("Vdp2DrawScreens");
    VIDCore->Vdp2DrawScreens();
  }

  if (isrender){
     FRAMELOG("Vdp1DrawEnd");
    VIDCore->Vdp1DrawEnd();
#if !defined(YAB_ASYNC_RENDERING)
    yabsys.wait_line_count += 45;
    yabsys.wait_line_count %= yabsys.VBlankLineCount;
#endif
  }

   FPSDisplay();
#if 1
   //if ((Vdp1Regs->FBCR & 2) && (Vdp1Regs->TVMR & 8))
   //   Vdp1External.manualerase = 1;

   if ( skipnextframe == 0)
   {
      framesskipped = 0;

      if (framestoskip > 0)
         skipnextframe = 1;
   }
   else
   {
      framestoskip--;

      if (framestoskip < 1) 
         skipnextframe = 0;
      else
         skipnextframe = 1;

      framesskipped++;
   }

#endif
   VdpUnLockVram();
#if PROFILE_RENDERING
   static FILE * framefp = NULL;
   if( framefp == NULL ){
#if defined(ANDROID)
     framefp = fopen("/mnt/sdcard/frame.csv", "w");
#else
     framefp = fopen("frame.csv","w");
#endif
   }

   if( framefp != NULL ){
      fprintf(framefp,"%d,%d\n", g_frame_count, (u32)(YabauseGetTicks()-starttime) );
      fflush(framefp);
   }
#endif
}

//////////////////////////////////////////////////////////////////////////////
void Vdp2VBlankOUT(void) {
  g_frame_count++;

  //if (g_frame_count == 60){
  //  YabSaveStateSlot(".\\", 1);
  //}

  //if (g_frame_count >= 1){
  //  YabLoadStateSlot(".\\", 1);
  //}

  FRAMELOG("***** VOUT %d *****", g_frame_count);
  if (Vdp2External.perline_alpha == &Vdp2External.perline_alpha_a){
    Vdp2External.perline_alpha = &Vdp2External.perline_alpha_b;
    Vdp2External.perline_alpha_draw = &Vdp2External.perline_alpha_a;
    *Vdp2External.perline_alpha = 0;
  }
  else{
    Vdp2External.perline_alpha = &Vdp2External.perline_alpha_a;
    Vdp2External.perline_alpha_draw = &Vdp2External.perline_alpha_b;
    *Vdp2External.perline_alpha = 0;
  }

  if (((Vdp1Regs->TVMR >> 3) & 0x01) == 1){  // VBlank Erace (VBE1)
    Vdp1External.vbalnk_erase = 1;
  }else{
    Vdp1External.vbalnk_erase = 0;
  }

#ifdef _VDP_PROFILE_
  FrameProfileShow();
  FrameProfileInit();
#endif

   if (((Vdp2Regs->TVMD >> 6) & 0x3) == 0){
     vdp2_is_odd_frame = 1;
   }else{ // p02_50.htm#TVSTAT_
     if (vdp2_is_odd_frame)
       vdp2_is_odd_frame = 0;
     else
       vdp2_is_odd_frame = 1;
   }

   Vdp2Regs->TVSTAT = ((Vdp2Regs->TVSTAT & ~0x0008) & ~0x0002) | (vdp2_is_odd_frame << 1);

   ScuSendVBlankOUT();

   if (Vdp2Regs->EXTEN & 0x200) // Should be revised for accuracy(should occur only occur on the line it happens at, etc.)
   {
      // Only Latch if EXLTEN is enabled
      if (SmpcRegs->EXLE & 0x1)
         Vdp2SendExternalLatch((PORTDATA1.data[3]<<8)|PORTDATA1.data[4], (PORTDATA1.data[5]<<8)|PORTDATA1.data[6]);
   }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2SendExternalLatch(int hcnt, int vcnt)
{
   Vdp2Regs->HCNT = hcnt << 1;
   Vdp2Regs->VCNT = vcnt;
   Vdp2Regs->TVSTAT |= 0x200;
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp2ReadByte(u32 addr) {
   LOG("VDP2 register byte read = %08X\n", addr);
   addr &= 0x1FF;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp2ReadWord(u32 addr) {
   addr &= 0x1FF;

   switch (addr)
   {
   case 0x000:
     return Vdp2Regs->TVMD;
   case 0x002:
     if (!(Vdp2Regs->EXTEN & 0x200))
     {
       // Latch HV counter on read
       // Vdp2Regs->HCNT = ?;
       Vdp2Regs->VCNT = yabsys.LineCount;
       Vdp2Regs->TVSTAT |= 0x200;
     }

     return Vdp2Regs->EXTEN;
   case 0x004:
   {
     u16 tvstat = Vdp2Regs->TVSTAT;

     // Clear External latch and sync flags
     Vdp2Regs->TVSTAT &= 0xFCFF;

     // if TVMD's DISP bit is cleared, TVSTAT's VBLANK bit is always set
     if (Vdp2Regs->TVMD & 0x8000)
       return tvstat;
     else
       return (tvstat | 0x8);
   }
   case 0x006:
     return Vdp2Regs->VRSIZE;
   case 0x008:
     return Vdp2Regs->HCNT;
   case 0x00A:
     return Vdp2Regs->VCNT;
   case 0x00C:
     return 0 ;
   case 0x00E:
     return Vdp2Regs->RAMCTL;
   case 0x010:
     return Vdp2Regs->CYCA0L ;
     
   case 0x012:
     return Vdp2Regs->CYCA0U ;
     
   case 0x014:
     return Vdp2Regs->CYCA1L ;
     
   case 0x016:
     return Vdp2Regs->CYCA1U ;
     
   case 0x018:
     return Vdp2Regs->CYCB0L ;
     
   case 0x01A:
     return Vdp2Regs->CYCB0U ;
     
   case 0x01C:
     return Vdp2Regs->CYCB1L ;
     
   case 0x01E:
     return Vdp2Regs->CYCB1U ;
     
   case 0x020:
     return Vdp2Regs->BGON ;
     
   case 0x022:
     return Vdp2Regs->MZCTL ;
     
   case 0x024:
     return Vdp2Regs->SFSEL ;
     
   case 0x026:
     return Vdp2Regs->SFCODE ;
     
   case 0x028:
     return Vdp2Regs->CHCTLA ;
     
   case 0x02A:
     return Vdp2Regs->CHCTLB ;
     
   case 0x02C:
     return Vdp2Regs->BMPNA ;
     
   case 0x02E:
     return Vdp2Regs->BMPNB ;
     
   case 0x030:
     return Vdp2Regs->PNCN0 ;
     
   case 0x032:
     return Vdp2Regs->PNCN1 ;
     
   case 0x034:
     return Vdp2Regs->PNCN2 ;
     
   case 0x036:
     return Vdp2Regs->PNCN3 ;
     
   case 0x038:
     return Vdp2Regs->PNCR ;
     
   case 0x03A:
     return Vdp2Regs->PLSZ ;
     
   case 0x03C:
     return Vdp2Regs->MPOFN ;
     
   case 0x03E:
     return Vdp2Regs->MPOFR ;
     
   case 0x040:
     return Vdp2Regs->MPABN0 ;
     
   case 0x042:
     return Vdp2Regs->MPCDN0 ;
     
   case 0x044:
     return Vdp2Regs->MPABN1 ;
     
   case 0x046:
     return Vdp2Regs->MPCDN1 ;
     
   case 0x048:
     return Vdp2Regs->MPABN2 ;
     
   case 0x04A:
     return Vdp2Regs->MPCDN2 ;
     
   case 0x04C:
     return Vdp2Regs->MPABN3 ;
     
   case 0x04E:
     return Vdp2Regs->MPCDN3 ;
     
   case 0x050:
     return Vdp2Regs->MPABRA ;
     
   case 0x052:
     return Vdp2Regs->MPCDRA ;
     
   case 0x054:
     return Vdp2Regs->MPEFRA ;
     
   case 0x056:
     return Vdp2Regs->MPGHRA ;
     
   case 0x058:
     return Vdp2Regs->MPIJRA ;
     
   case 0x05A:
     return Vdp2Regs->MPKLRA ;
     
   case 0x05C:
     return Vdp2Regs->MPMNRA ;
     
   case 0x05E:
     return Vdp2Regs->MPOPRA ;
     
   case 0x060:
     return Vdp2Regs->MPABRB ;
     
   case 0x062:
     return Vdp2Regs->MPCDRB ;
     
   case 0x064:
     return Vdp2Regs->MPEFRB ;
     
   case 0x066:
     return Vdp2Regs->MPGHRB ;
     
   case 0x068:
     return Vdp2Regs->MPIJRB ;
     
   case 0x06A:
     return Vdp2Regs->MPKLRB ;
     
   case 0x06C:
     return Vdp2Regs->MPMNRB ;
     
   case 0x06E:
     return Vdp2Regs->MPOPRB ;
     
   case 0x070:
     return Vdp2Regs->SCXIN0 ;
     
   case 0x072:
     return Vdp2Regs->SCXDN0 ;
     
   case 0x074:
     return Vdp2Regs->SCYIN0 ;
     
   case 0x076:
     return Vdp2Regs->SCYDN0 ;
     
   case 0x078:
     return Vdp2Regs->ZMXN0.part.I ;
     
   case 0x07A:
     return Vdp2Regs->ZMXN0.part.D ;
     
   case 0x07C:
     return Vdp2Regs->ZMYN0.part.I ;
     
   case 0x07E:
     return Vdp2Regs->ZMYN0.part.D ;
     
   case 0x080:
     return Vdp2Regs->SCXIN1 ;
     
   case 0x082:
     return Vdp2Regs->SCXDN1 ;
     
   case 0x084:
     return Vdp2Regs->SCYIN1 ;
     
   case 0x086:
     return Vdp2Regs->SCYDN1 ;
     
   case 0x088:
     return Vdp2Regs->ZMXN1.part.I ;
     
   case 0x08A:
     return Vdp2Regs->ZMXN1.part.D ;
     
   case 0x08C:
     return Vdp2Regs->ZMYN1.part.I ;
     
   case 0x08E:
     return Vdp2Regs->ZMYN1.part.D ;
     
   case 0x090:
     return Vdp2Regs->SCXN2 ;
     
   case 0x092:
     return Vdp2Regs->SCYN2 ;
     
   case 0x094:
     return Vdp2Regs->SCXN3 ;
     
   case 0x096:
     return Vdp2Regs->SCYN3 ;
     
   case 0x098:
     return Vdp2Regs->ZMCTL ;
     
   case 0x09A:
     return Vdp2Regs->SCRCTL ;
     
   case 0x09C:
     return Vdp2Regs->VCSTA.part.U ;
     
   case 0x09E:
     return Vdp2Regs->VCSTA.part.L ;
     
   case 0x0A0:
     return Vdp2Regs->LSTA0.part.U ;
     
   case 0x0A2:
     return Vdp2Regs->LSTA0.part.L ;
     
   case 0x0A4:
     return Vdp2Regs->LSTA1.part.U ;
     
   case 0x0A6:
     return Vdp2Regs->LSTA1.part.L ;
     
   case 0x0A8:
     return Vdp2Regs->LCTA.part.U ;
     
   case 0x0AA:
     return Vdp2Regs->LCTA.part.L ;
     
   case 0x0AC:
     return Vdp2Regs->BKTAU ;
     
   case 0x0AE:
     return Vdp2Regs->BKTAL ;
     
   case 0x0B0:
     return Vdp2Regs->RPMD ;
     
   case 0x0B2:
     return Vdp2Regs->RPRCTL ;
     
   case 0x0B4:
     return Vdp2Regs->KTCTL ;
     
   case 0x0B6:
     return Vdp2Regs->KTAOF ;
     
   case 0x0B8:
     return Vdp2Regs->OVPNRA ;
     
   case 0x0BA:
     return Vdp2Regs->OVPNRB ;
     
   case 0x0BC:
     return Vdp2Regs->RPTA.part.U ;
     
   case 0x0BE:
     return Vdp2Regs->RPTA.part.L ;
     
   case 0x0C0:
     return Vdp2Regs->WPSX0 ;
     
   case 0x0C2:
     return Vdp2Regs->WPSY0 ;
     
   case 0x0C4:
     return Vdp2Regs->WPEX0 ;
     
   case 0x0C6:
     return Vdp2Regs->WPEY0 ;
     
   case 0x0C8:
     return Vdp2Regs->WPSX1 ;
     
   case 0x0CA:
     return Vdp2Regs->WPSY1 ;
     
   case 0x0CC:
     return Vdp2Regs->WPEX1 ;
     
   case 0x0CE:
     return Vdp2Regs->WPEY1 ;
     
   case 0x0D0:
     return Vdp2Regs->WCTLA ;
     
   case 0x0D2:
     return Vdp2Regs->WCTLB ;
     
   case 0x0D4:
     return Vdp2Regs->WCTLC ;
     
   case 0x0D6:
     return Vdp2Regs->WCTLD ;
     
   case 0x0D8:
     return Vdp2Regs->LWTA0.part.U ;
     
   case 0x0DA:
     return Vdp2Regs->LWTA0.part.L ;
     
   case 0x0DC:
     return Vdp2Regs->LWTA1.part.U ;
     
   case 0x0DE:
     return Vdp2Regs->LWTA1.part.L ;
     
   case 0x0E0:
     return Vdp2Regs->SPCTL ;
     
   case 0x0E2:
     return Vdp2Regs->SDCTL ;
     
   case 0x0E4:
     return Vdp2Regs->CRAOFA ;
     
   case 0x0E6:
     return Vdp2Regs->CRAOFB ;
     
   case 0x0E8:
     return Vdp2Regs->LNCLEN ;
     
   case 0x0EA:
     return Vdp2Regs->SFPRMD ;
     
   case 0x0EC:
     return Vdp2Regs->CCCTL ;
     
   case 0x0EE:
     return Vdp2Regs->SFCCMD ;
     
   case 0x0F0:
     return Vdp2Regs->PRISA ;
     
   case 0x0F2:
     return Vdp2Regs->PRISB ;
     
   case 0x0F4:
     return Vdp2Regs->PRISC ;
     
   case 0x0F6:
     return Vdp2Regs->PRISD ;
     
   case 0x0F8:
     return Vdp2Regs->PRINA ;
     
   case 0x0FA:
     return Vdp2Regs->PRINB ;
     
   case 0x0FC:
     return Vdp2Regs->PRIR ;
     
   case 0x0FE:
     // Reserved
     return 0;
     
   case 0x100:
     return Vdp2Regs->CCRSA ;
     
   case 0x102:
     return Vdp2Regs->CCRSB ;
     
   case 0x104:
     return Vdp2Regs->CCRSC ;
     
   case 0x106:
     return Vdp2Regs->CCRSD ;
     
   case 0x108:
     return Vdp2Regs->CCRNA ;
     
   case 0x10A:
     return Vdp2Regs->CCRNB ;
     
   case 0x10C:
     return Vdp2Regs->CCRR ;
     
   case 0x10E:
     return Vdp2Regs->CCRLB ;
     
   case 0x110:
     return Vdp2Regs->CLOFEN ;
     
   case 0x112:
     return Vdp2Regs->CLOFSL ;
     
   case 0x114:
     return Vdp2Regs->COAR ;
     
   case 0x116:
     return Vdp2Regs->COAG ;
     
   case 0x118:
     return Vdp2Regs->COAB ;
     
   case 0x11A:
     return Vdp2Regs->COBR ;
     
   case 0x11C:
     return Vdp2Regs->COBG ;
     
   case 0x11E:
     return Vdp2Regs->COBB ;
     
   default:
   {
     LOG("Unhandled VDP2 word write: %08X\n", addr);
     break;
   }
   }


   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp2ReadLong(u32 addr) {
   LOG("VDP2 register long read = %08X\n", addr);
   addr &= 0x1FF;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2WriteByte(u32 addr, UNUSED u8 val) {
   LOG("VDP2 register byte write = %08X\n", addr);
   addr &= 0x1FF;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2WriteWord(u32 addr, u16 val) {
   addr &= 0x1FF;

   switch (addr)
   {
      case 0x000:
         Vdp2Regs->TVMD = val;
         yabsys.VBlankLineCount = 225+(val & 0x30);
         return;
      case 0x002:
         Vdp2Regs->EXTEN = val;
         return;
      case 0x004:
         // TVSTAT is read-only
         return;
      case 0x006:
         Vdp2Regs->VRSIZE = val;
         return;
      case 0x008:
         // HCNT is read-only
         return;
      case 0x00A:
         // VCNT is read-only
         return;
      case 0x00C:
         // Reserved
         return;
      case 0x00E:
         Vdp2Regs->RAMCTL = val;
         if (Vdp2Internal.ColorMode != ((val >> 12) & 0x3) ) {
           Vdp2Internal.ColorMode = (val >> 12) & 0x3;
           for (int i = 0; i < 0x1000; i += 2) {
             VIDCore->OnUpdateColorRamWord(i);
           }
         }
         
         return;
      case 0x010:
         Vdp2Regs->CYCA0L = val;
         return;
      case 0x012:
         Vdp2Regs->CYCA0U = val;
         return;
      case 0x014:
         Vdp2Regs->CYCA1L = val;
         return;
      case 0x016:
         Vdp2Regs->CYCA1U = val;
         return;
      case 0x018:
         Vdp2Regs->CYCB0L = val;
         return;
      case 0x01A:
         Vdp2Regs->CYCB0U = val;
         return;
      case 0x01C:
         Vdp2Regs->CYCB1L = val;
         return;
      case 0x01E:
         Vdp2Regs->CYCB1U = val;
         return;
      case 0x020:
         Vdp2Regs->BGON = val;
         return;
      case 0x022:
         Vdp2Regs->MZCTL = val;
         return;
      case 0x024:
         Vdp2Regs->SFSEL = val;
         return;
      case 0x026:
         Vdp2Regs->SFCODE = val;
         return;
      case 0x028:
         Vdp2Regs->CHCTLA = val;
         return;
      case 0x02A:
         Vdp2Regs->CHCTLB = val;
         return;
      case 0x02C:
         Vdp2Regs->BMPNA = val;
         return;
      case 0x02E:
         Vdp2Regs->BMPNB = val;
         return;
      case 0x030:
         Vdp2Regs->PNCN0 = val;
         return;
      case 0x032:
         Vdp2Regs->PNCN1 = val;
         return;
      case 0x034:
         Vdp2Regs->PNCN2 = val;
         return;
      case 0x036:
         Vdp2Regs->PNCN3 = val;
         return;
      case 0x038:
         Vdp2Regs->PNCR = val;
         return;
      case 0x03A:
         Vdp2Regs->PLSZ = val;
         return;
      case 0x03C:
         Vdp2Regs->MPOFN = val;
         return;
      case 0x03E:
         Vdp2Regs->MPOFR = val;
         return;
      case 0x040:
         Vdp2Regs->MPABN0 = val;
         return;
      case 0x042:
         Vdp2Regs->MPCDN0 = val;
         return;
      case 0x044:
         Vdp2Regs->MPABN1 = val;
         return;
      case 0x046:
         Vdp2Regs->MPCDN1 = val;
         return;
      case 0x048:
         Vdp2Regs->MPABN2 = val;
         return;
      case 0x04A:
         Vdp2Regs->MPCDN2 = val;
         return;
      case 0x04C:
         Vdp2Regs->MPABN3 = val;
         return;
      case 0x04E:
         Vdp2Regs->MPCDN3 = val;
         return;
      case 0x050:
         Vdp2Regs->MPABRA = val;
         return;
      case 0x052:
         Vdp2Regs->MPCDRA = val;
         return;
      case 0x054:
         Vdp2Regs->MPEFRA = val;
         return;
      case 0x056:
         Vdp2Regs->MPGHRA = val;
         return;
      case 0x058:
         Vdp2Regs->MPIJRA = val;
         return;
      case 0x05A:
         Vdp2Regs->MPKLRA = val;
         return;
      case 0x05C:
         Vdp2Regs->MPMNRA = val;
         return;
      case 0x05E:
         Vdp2Regs->MPOPRA = val;
         return;
      case 0x060:
         Vdp2Regs->MPABRB = val;
         return;
      case 0x062:
         Vdp2Regs->MPCDRB = val;
         return;
      case 0x064:
         Vdp2Regs->MPEFRB = val;
         return;
      case 0x066:
         Vdp2Regs->MPGHRB = val;
         return;
      case 0x068:
         Vdp2Regs->MPIJRB = val;
         return;
      case 0x06A:
         Vdp2Regs->MPKLRB = val;
         return;
      case 0x06C:
         Vdp2Regs->MPMNRB = val;
         return;
      case 0x06E:
         Vdp2Regs->MPOPRB = val;
         return;
      case 0x070:
         Vdp2Regs->SCXIN0 = val;
         return;
      case 0x072:
         Vdp2Regs->SCXDN0 = val;
         return;
      case 0x074:
         Vdp2Regs->SCYIN0 = val;
         return;
      case 0x076:
         Vdp2Regs->SCYDN0 = val;
         return;
      case 0x078:
         Vdp2Regs->ZMXN0.part.I = val;
         return;
      case 0x07A:
         Vdp2Regs->ZMXN0.part.D = val;
         return;
      case 0x07C:
         Vdp2Regs->ZMYN0.part.I = val;
         return;
      case 0x07E:
         Vdp2Regs->ZMYN0.part.D = val;
         return;
      case 0x080:
         Vdp2Regs->SCXIN1 = val;
         return;
      case 0x082:
         Vdp2Regs->SCXDN1 = val;
         return;
      case 0x084:
         Vdp2Regs->SCYIN1 = val;
         return;
      case 0x086:
         Vdp2Regs->SCYDN1 = val;
         return;
      case 0x088:
         Vdp2Regs->ZMXN1.part.I = val;
         return;
      case 0x08A:
         Vdp2Regs->ZMXN1.part.D = val;
         return;
      case 0x08C:
         Vdp2Regs->ZMYN1.part.I = val;
         return;
      case 0x08E:
         Vdp2Regs->ZMYN1.part.D = val;
         return;
      case 0x090:
         Vdp2Regs->SCXN2 = val;
         return;
      case 0x092:
         Vdp2Regs->SCYN2 = val;
         return;
      case 0x094:
         Vdp2Regs->SCXN3 = val;
         return;
      case 0x096:
         Vdp2Regs->SCYN3 = val;
         return;
      case 0x098:
         Vdp2Regs->ZMCTL = val;
         return;
      case 0x09A:
         Vdp2Regs->SCRCTL = val;
         return;
      case 0x09C:
         Vdp2Regs->VCSTA.part.U = val;
         return;
      case 0x09E:
         Vdp2Regs->VCSTA.part.L = val;
         return;
      case 0x0A0:
         Vdp2Regs->LSTA0.part.U = val;
         return;
      case 0x0A2:
         Vdp2Regs->LSTA0.part.L = val;
         return;
      case 0x0A4:
         Vdp2Regs->LSTA1.part.U = val;
         return;
      case 0x0A6:
         Vdp2Regs->LSTA1.part.L = val;
         return;
      case 0x0A8:
         Vdp2Regs->LCTA.part.U = val;
         return;
      case 0x0AA:
         Vdp2Regs->LCTA.part.L = val;
         return;
      case 0x0AC:
         Vdp2Regs->BKTAU = val;
         return;
      case 0x0AE:
         Vdp2Regs->BKTAL = val;
         return;
      case 0x0B0:
         Vdp2Regs->RPMD = val;
         return;
      case 0x0B2:
         Vdp2Regs->RPRCTL = val;
         return;
      case 0x0B4:
         Vdp2Regs->KTCTL = val;
         return;
      case 0x0B6:
         Vdp2Regs->KTAOF = val;
         return;
      case 0x0B8:
         Vdp2Regs->OVPNRA = val;
         return;
      case 0x0BA:
         Vdp2Regs->OVPNRB = val;
         return;
      case 0x0BC:
         Vdp2Regs->RPTA.part.U = val;
         return;
      case 0x0BE:
         Vdp2Regs->RPTA.part.L = val;
         return;
      case 0x0C0:
         Vdp2Regs->WPSX0 = val;
         return;
      case 0x0C2:
         Vdp2Regs->WPSY0 = val;
         return;
      case 0x0C4:
         Vdp2Regs->WPEX0 = val;
         return;
      case 0x0C6:
         Vdp2Regs->WPEY0 = val;
         return;
      case 0x0C8:
         Vdp2Regs->WPSX1 = val;
         return;
      case 0x0CA:
         Vdp2Regs->WPSY1 = val;
         return;
      case 0x0CC:
         Vdp2Regs->WPEX1 = val;
         return;
      case 0x0CE:
         Vdp2Regs->WPEY1 = val;
         return;
      case 0x0D0:
         Vdp2Regs->WCTLA = val;
         return;
      case 0x0D2:
         Vdp2Regs->WCTLB = val;
         return;
      case 0x0D4:
         Vdp2Regs->WCTLC = val;
         return;
      case 0x0D6:
         Vdp2Regs->WCTLD = val;
         return;
      case 0x0D8:
         Vdp2Regs->LWTA0.part.U = val;
         return;
      case 0x0DA:
         Vdp2Regs->LWTA0.part.L = val;
         return;
      case 0x0DC:
         Vdp2Regs->LWTA1.part.U = val;
         return;
      case 0x0DE:
         Vdp2Regs->LWTA1.part.L = val;
         return;
      case 0x0E0:
         Vdp2Regs->SPCTL = val;
         return;
      case 0x0E2:
         Vdp2Regs->SDCTL = val;
         return;
      case 0x0E4:
         Vdp2Regs->CRAOFA = val;
         return;
      case 0x0E6:
         Vdp2Regs->CRAOFB = val;
         return;     
      case 0x0E8:
         Vdp2Regs->LNCLEN = val;
         return;
      case 0x0EA:
         Vdp2Regs->SFPRMD = val;
         return;
      case 0x0EC:
         Vdp2Regs->CCCTL = val;
         return;     
      case 0x0EE:
         Vdp2Regs->SFCCMD = val;
         return;
      case 0x0F0:
         Vdp2Regs->PRISA = val;
         return;
      case 0x0F2:
         Vdp2Regs->PRISB = val;
         return;
      case 0x0F4:
         Vdp2Regs->PRISC = val;
         return;
      case 0x0F6:
         Vdp2Regs->PRISD = val;
         return;
      case 0x0F8:
         Vdp2Regs->PRINA = val;
         return;
      case 0x0FA:
         Vdp2Regs->PRINB = val;
         return;
      case 0x0FC:
         Vdp2Regs->PRIR = val;
         return;
      case 0x0FE:
         // Reserved
         return;
      case 0x100:
         Vdp2Regs->CCRSA = val;
         return;
      case 0x102:
         Vdp2Regs->CCRSB = val;
         return;
      case 0x104:
         Vdp2Regs->CCRSC = val;
         return;
      case 0x106:
         Vdp2Regs->CCRSD = val;
         return;
      case 0x108:
         Vdp2Regs->CCRNA = val;
         return;
      case 0x10A:
         Vdp2Regs->CCRNB = val;
         return;
      case 0x10C:
         Vdp2Regs->CCRR = val;
         return;
      case 0x10E:
         Vdp2Regs->CCRLB = val;
         return;
      case 0x110:
         Vdp2Regs->CLOFEN = val;
         return;
      case 0x112:
         Vdp2Regs->CLOFSL = val;
         return;
      case 0x114:
         Vdp2Regs->COAR = val;
         return;
      case 0x116:
         Vdp2Regs->COAG = val;
         return;
      case 0x118:
         Vdp2Regs->COAB = val;
         return;
      case 0x11A:
         Vdp2Regs->COBR = val;
         return;
      case 0x11C:
         Vdp2Regs->COBG = val;
         return;
      case 0x11E:
         Vdp2Regs->COBB = val;
         return;
      default:
      {
         LOG("Unhandled VDP2 word write: %08X\n", addr);
         break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2WriteLong(u32 addr, u32 val) {
   
   Vdp2WriteWord(addr,val>>16);
   Vdp2WriteWord(addr+2,val&0xFFFF);
   return;
}

//////////////////////////////////////////////////////////////////////////////

int Vdp2SaveState(FILE *fp)
{
   int offset;
   IOCheck_struct check = { 0, 0 };

   offset = StateWriteHeader(fp, "VDP2", 1);

   // Write registers
   ywrite(&check, (void *)Vdp2Regs, sizeof(Vdp2), 1, fp);

   // Write VDP2 ram
   ywrite(&check, (void *)Vdp2Ram, 0x80000, 1, fp);

   // Write CRAM
   ywrite(&check, (void *)Vdp2ColorRam, 0x1000, 1, fp);

   // Write internal variables
   ywrite(&check, (void *)&Vdp2Internal, sizeof(Vdp2Internal_struct), 1, fp);

   return StateFinishHeader(fp, offset);
}

//////////////////////////////////////////////////////////////////////////////

int Vdp2LoadState(FILE *fp, UNUSED int version, int size)
{
   IOCheck_struct check = { 0, 0 };

   // Read registers
   yread(&check, (void *)Vdp2Regs, sizeof(Vdp2), 1, fp);

   // Read VDP2 ram
   yread(&check, (void *)Vdp2Ram, 0x80000, 1, fp);

   // Read CRAM
   yread(&check, (void *)Vdp2ColorRam, 0x1000, 1, fp);

   // Read internal variables
   yread(&check, (void *)&Vdp2Internal, sizeof(Vdp2Internal_struct), 1, fp);

   //if(VIDCore) VIDCore->Resize(0,0,-1,-1,0,0);

   for (int i = 0; i < 0x1000; i += 2) {
     VIDCore->OnUpdateColorRamWord(i);
   }

   return size;
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG0(void)
{
   Vdp2External.disptoggle ^= 0x1;
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG1(void)
{
   Vdp2External.disptoggle ^= 0x2;
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG2(void)
{
   Vdp2External.disptoggle ^= 0x4;
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG3(void)
{
   Vdp2External.disptoggle ^= 0x8;
}

//////////////////////////////////////////////////////////////////////////////

void ToggleRBG0(void)
{
   Vdp2External.disptoggle ^= 0x10;
}

//////////////////////////////////////////////////////////////////////////////

void ToggleFullScreen(void)
{
   if (VIDCore->IsFullscreen())
   {
      VIDCore->Resize(0,0,320, 224, 0, 0);
   }
   else
   {
      VIDCore->Resize(0,0,640, 480, 1, 0);
   }
}

//////////////////////////////////////////////////////////////////////////////

void EnableAutoFrameSkip(void)
{
   autoframeskipenab = 1;
   lastticks = YabauseGetTicks();
}

//////////////////////////////////////////////////////////////////////////////

void DisableAutoFrameSkip(void)
{
   autoframeskipenab = 0;
}



void VdpResume( void ){
#if defined(YAB_ASYNC_RENDERING)
	YabAddEventQueue(evqueue,VDPEV_MAKECURRENT);
  YabWaitEventQueue(command_);
#endif
}

void VdpRevoke( void ){
#if defined(YAB_ASYNC_RENDERING)
	YabAddEventQueue(evqueue,VDPEV_REVOKE);
  YabWaitEventQueue(command_);
#endif
}



//////////////////////////////////////////////////////////////////////////////
// This dump code can be used by the real SEGA saturn link this code.
/*
#include	"sgl.h"

extern short vreg[];
extern char vram[];
extern short cram[];

volatile Uint8* vrm_dst = (Uint8*)0x25E00000;
volatile Uint16* const crm_dst = (Uint8*)0x25F00000;
volatile Uint16* const vreg_dst = (Uint16*)0x260ffcc0; //0x25F80000;
volatile Uint16* const frame_dst = (Uint16*)0x25C00000;

#define N0ON 0x01
#define N1ON 0x02
#define N2ON 0x04
#define N3ON 0x08
#define R0ON 0x10
#define R1ON 0x20
  
void ss_main(void)
{
	int i;

	slInitSystem(TV_320x224, NULL, 1);
	slTVOn();
	for (i = 0; i < (0xFFF>>1) ; i++ ) {
		crm_dst[i] = cram[i];
	}

	for (i = 0; i < 0x7FFFF; i += 1) {
		vrm_dst[i] = vram[i];
	}

	while(1){

    for (i = 7; i < (0x11E>>1); i++) {
			vreg_dst[i] = vreg[i];
		}		

    slSynch();
	}
}

*/
int Vdp2GenerateCCode() {

  FILE * regfp = fopen("vreg.c", "w");
  fprintf(regfp, "short vreg[] = { \n");
  for (int i = 0; i < 0x11E; i += 2) {
    u16 data = Vdp2ReadWord(i);
    fprintf(regfp, "0x%04X", data);
    if (i != 0 && (i % 16) == 0) {
      fprintf(regfp, ",\n");
    }
    else {
      fprintf(regfp, ",");
    }
  }
  fprintf(regfp, "};\n");
  fclose(regfp);


  FILE * ramfp = fopen("vram.c","w");
  fprintf(ramfp, "char vram[] = { \n");
  for (int i = 0; i < 0x7FFFF; i++) {
    u8 data = Vdp2RamReadByte(i);
    fprintf(ramfp, "0x%02X", data);
    if ( i != 0 && (i % 16) == 0) {
      fprintf(ramfp, ",\n");
    }
    else {
      fprintf(ramfp, ",");
    }
  }
  fprintf(ramfp, "};\n");
  fclose(ramfp);

  FILE * cramfp = fopen("cram.c", "w");
  fprintf(cramfp, "short cram[] = { \n");
  for (int i = 0; i < 0xFFF; i += 2) {
    u16 data = Vdp2ColorRamReadWord(i);
    fprintf(cramfp, "0x%04X", data);
    if (i != 0 && (i % 16) == 0) {
      fprintf(cramfp, ",\n");
    }
    else {
      fprintf(cramfp, ",");
    }
  }
  fprintf(cramfp, "};\n");
  fclose(cramfp);

  return 0;
}