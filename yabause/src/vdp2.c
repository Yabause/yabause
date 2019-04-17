
/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2007 Theo Berkau
    Copyright 2015 Shinya Miyamoto(devmiyax)

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
#include "ygl.h"
#include "frameprofile.h"

u8 * Vdp2Ram;
u8 * Vdp2ColorRam;
Vdp2 * Vdp2Regs;
Vdp2Internal_struct Vdp2Internal;
Vdp2External_struct Vdp2External;

int isSkipped = 0;

u8 Vdp2ColorRamUpdated = 0;
u8 A0_Updated = 0;
u8 A1_Updated = 0;
u8 B0_Updated = 0;
u8 B1_Updated = 0;

struct CellScrollData cell_scroll_data[270];
Vdp2 Vdp2Lines[270];

int vdp2_is_odd_frame = 0;

static void startField(void);// VBLANK-OUT handler
extern void waitVdp2DrawScreensEnd(int sync, int abort);

int g_frame_count = 0;

//#define LOG yprintf

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp2RamReadByte(SH2_struct *context, u8* mem, u32 addr) {
  if (Vdp2Regs->VRSIZE & 0x8000)
    addr &= 0xEFFFF;
  else
    addr &= 0x7FFFF;
   return T1ReadByte(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp2RamReadWord(SH2_struct *context, u8* mem, u32 addr) {
  if (Vdp2Regs->VRSIZE & 0x8000)
    addr &= 0xEFFFF;
  else
    addr &= 0x7FFFF;
   return T1ReadWord(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp2RamReadLong(SH2_struct *context, u8* mem, u32 addr) {
  if (Vdp2Regs->VRSIZE & 0x8000)
    addr &= 0xEFFFF;
  else
    addr &= 0x7FFFF;
   return T1ReadLong(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2RamWriteByte(SH2_struct *context, u8* mem, u32 addr, u8 val) {
  if (Vdp2Regs->VRSIZE & 0x8000)
    addr &= 0xEFFFF;
  else
    addr &= 0x7FFFF;

   if (A0_Updated == 0 && addr >= 0 && addr < (0x20000<<(Vdp2Regs->VRSIZE>>15))){
     A0_Updated = 1;
   }
   else if (A1_Updated == 0 &&  addr >= (0x20000<<(Vdp2Regs->VRSIZE>>15)) && addr < (0x40000<<(Vdp2Regs->VRSIZE>>15))){
     A1_Updated = 1;
   }
   else if (B0_Updated == 0 && addr >= (0x40000<<(Vdp2Regs->VRSIZE>>15)) && addr < (0x60000<<(Vdp2Regs->VRSIZE>>15))){
     B0_Updated = 1;
   }
   else if (B1_Updated == 0 && addr >= (0x60000<<(Vdp2Regs->VRSIZE>>15)) && addr < (0x80000<<(Vdp2Regs->VRSIZE>>15))){
     B1_Updated = 1;
   }

   T1WriteByte(mem, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2RamWriteWord(SH2_struct *context, u8* mem, u32 addr, u16 val) {
  if (Vdp2Regs->VRSIZE & 0x8000)
    addr &= 0xEFFFF;
  else
    addr &= 0x7FFFF;

   if (A0_Updated == 0 && addr >= 0 && addr < (0x20000<<(Vdp2Regs->VRSIZE>>15))){
     A0_Updated = 1;
   }
   else if (A1_Updated == 0 &&  addr >= (0x20000<<(Vdp2Regs->VRSIZE>>15)) && addr < (0x40000<<(Vdp2Regs->VRSIZE>>15))){
     A1_Updated = 1;
   }
   else if (B0_Updated == 0 && addr >= (0x40000<<(Vdp2Regs->VRSIZE>>15)) && addr < (0x60000<<(Vdp2Regs->VRSIZE>>15))){
     B0_Updated = 1;
   }
   else if (B1_Updated == 0 && addr >= (0x60000<<(Vdp2Regs->VRSIZE>>15)) && addr < (0x80000<<(Vdp2Regs->VRSIZE>>15))){
     B1_Updated = 1;
   }

   T1WriteWord(mem, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2RamWriteLong(SH2_struct *context, u8* mem, u32 addr, u32 val) {
  if (Vdp2Regs->VRSIZE & 0x8000)
    addr &= 0xEFFFF;
  else
    addr &= 0x7FFFF;

   if (A0_Updated == 0 && addr >= 0 && addr < (0x20000<<(Vdp2Regs->VRSIZE>>15))){
     A0_Updated = 1;
   }
   else if (A1_Updated == 0 &&  addr >= (0x20000<<(Vdp2Regs->VRSIZE>>15)) && addr < (0x40000<<(Vdp2Regs->VRSIZE>>15))){
     A1_Updated = 1;
   }
   else if (B0_Updated == 0 && addr >= (0x40000<<(Vdp2Regs->VRSIZE>>15)) && addr < (0x60000<<(Vdp2Regs->VRSIZE>>15))){
     B0_Updated = 1;
   }
   else if (B1_Updated == 0 && addr >= (0x60000<<(Vdp2Regs->VRSIZE>>15)) && addr < (0x80000<<(Vdp2Regs->VRSIZE>>15))){
     B1_Updated = 1;
   }

   T1WriteLong(mem, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL Vdp2ColorRamReadByte(SH2_struct *context, u8* mem, u32 addr) {
   addr &= 0xFFF;
   return T2ReadByte(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp2ColorRamReadWord(SH2_struct *context, u8* mem, u32 addr) {
   addr &= 0xFFF;
   return T2ReadWord(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp2ColorRamReadLong(SH2_struct *context, u8* mem, u32 addr) {
   addr &= 0xFFF;
   return T2ReadLong(mem, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteByte(SH2_struct *context, u8* mem, u32 addr, u8 val) {
   addr &= 0xFFF;
   //LOG("[VDP2] Update Coloram Byte %08X:%02X", addr, val);
   if (Vdp2Internal.ColorMode == 0 ) {
     int up = ((addr & 0x800) != 0);
     if (val != T2ReadByte(mem, addr)) {
       T2WriteByte(mem, addr, val);
  #if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
       YglOnUpdateColorRamWord(addr);
  #endif
     }
     if (up) {
       if (val != T2ReadByte(mem, (addr & 0x7FF) + 0x800)) {
         T2WriteByte(mem, (addr & 0x7FF) + 0x800, val);
  #if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
         YglOnUpdateColorRamWord((addr & 0x7FF) + 0x800);
  #endif
       }
     }
   }
   else {
     if (val != T2ReadByte(mem, addr)) {
       T2WriteByte(mem, addr, val);
  #if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
       YglOnUpdateColorRamWord(addr);
  #endif
     }
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteWord(SH2_struct *context, u8* mem, u32 addr, u16 val) {
   addr &= 0xFFF;
   //LOG("[VDP2] Update Coloram Word %08X:%04X\n", addr, val);
   if (Vdp2Internal.ColorMode == 0 ) {
     int up = ((addr & 0x800) != 0);
     if (val != T2ReadWord(mem, addr)) {
       T2WriteWord(mem, addr, val);
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
       YglOnUpdateColorRamWord(addr);
#endif
     }
     if (up) {
       if (val != T2ReadWord(mem, (addr & 0x7FF) + 0x800)) {
         T2WriteWord(mem, (addr & 0x7FF) + 0x800, val);
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
         YglOnUpdateColorRamWord((addr & 0x7FF) + 0x800);
#endif
       }
     }
   }
   else {
     if (val != T2ReadWord(mem, addr)) {
       T2WriteWord(mem, addr, val);
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
       YglOnUpdateColorRamWord(addr);
#endif
     }
   }
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteLong(SH2_struct *context, u8* mem, u32 addr, u32 val) {
   addr &= 0xFFF;
   //LOG("[VDP2] Update Coloram Long %08X:%08X\n", addr, val);
   if (Vdp2Internal.ColorMode == 0) {

     int up = ((addr & 0x800) != 0);
     T2WriteLong(mem, addr, val);
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
     YglOnUpdateColorRamWord(addr + 2);
     YglOnUpdateColorRamWord(addr);
#endif

     if (up) {
       T2WriteLong(mem, (addr & 0x7FF) + 0x800, val);
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
       YglOnUpdateColorRamWord((addr & 0x7FF) + 0x800 + 2);
       YglOnUpdateColorRamWord((addr & 0x7FF) + 0x800);
#endif
     }

   }
   else {
     T2WriteLong(Vdp2ColorRam, addr, val);
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
     if (Vdp2Internal.ColorMode == 2) {
       YglOnUpdateColorRamWord(addr);
     }
     else {
       YglOnUpdateColorRamWord(addr + 2);
       YglOnUpdateColorRamWord(addr);
     }
#endif
   }

}

//////////////////////////////////////////////////////////////////////////////

int Vdp2Init(void) {
   if ((Vdp2Regs = (Vdp2 *) calloc(1, sizeof(Vdp2))) == NULL)
      return -1;

   if ((Vdp2Ram = T1MemoryInit(0x100000)) == NULL)
      return -1;

   if ((Vdp2ColorRam = T2MemoryInit(0x1000)) == NULL)
      return -1;

   Vdp2Reset();

   memset(Vdp2ColorRam, 0xFF, 0x1000);
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
   for (int i = 0; i < 0x1000; i += 2) {
     YglOnUpdateColorRamWord(i);
   }
#endif

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DeInit(void) {
   if (Vdp2Regs)
      free(Vdp2Regs);
   Vdp2Regs = NULL;

   if (Vdp2Ram)
      T1MemoryDeInit(Vdp2Ram);
   Vdp2Ram = NULL;

   if (Vdp2ColorRam)
      T2MemoryDeInit(Vdp2ColorRam);
   Vdp2ColorRam = NULL;


}

//////////////////////////////////////////////////////////////////////////////

static unsigned long nextFrameTime = 0;

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
   memset(Vdp2External.perline_alpha_a, 0, 270*sizeof(int));
   memset(Vdp2External.perline_alpha_b, 0, 270*sizeof(int));
   Vdp2External.perline_alpha = Vdp2External.perline_alpha_a;
   Vdp2External.perline_alpha_draw = Vdp2External.perline_alpha_b;

   nextFrameTime = 0;

}

//////////////////////////////////////////////////////////////////////////////

static int checkFrameSkip(void) {
  int ret = 0;
  if (yabsys.skipframe == 0) return 0;
  unsigned long now = YabauseGetTicks();
  if (nextFrameTime == 0) nextFrameTime = YabauseGetTicks();
  if(nextFrameTime < now) ret = 1;
  return ret;
}

void resetFrameSkip(void) {
  nextFrameTime = 0;
}

void Vdp2VBlankIN(void) {
  FRAMELOG("***** VIN *****");

	FrameProfileAdd("VIN start");
   /* this should be done after a frame change or a plot trigger */

   /* I'm not 100% sure about this, but it seems that when using manual change
   we should swap framebuffers in the "next field" and thus, clear the CEF...
   now we're lying a little here as we're not swapping the framebuffers. */
   //if (Vdp1External.manualchange) Vdp1Regs->EDSR >>= 1;

   if (checkFrameSkip() != 0) {
     dropFrameDisplay();
     isSkipped = 1;
   } else {
     VIDCore->Vdp2Draw();
     isSkipped = 0;
   }
   nextFrameTime  += yabsys.OneFrameTime;

   VIDCore->Sync();
   Vdp2Regs->TVSTAT |= 0x0008;

   ScuSendVBlankIN();

   if (yabsys.IsSSH2Running)
      SH2SendInterrupt(SSH2, 0x43, 0x6);

   FrameProfileAdd("VIN end");
}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

void Vdp2HBlankIN(void) {

  if (yabsys.LineCount < yabsys.VBlankLineCount) {
    Vdp2Regs->TVSTAT |= 0x0004;
    ScuSendHBlankIN();
    if (yabsys.IsSSH2Running)
      SH2SendInterrupt(SSH2, 0x41, 0x2);
  } else {
// Fix : Function doesn't exist without those defines
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
  waitVdp2DrawScreensEnd(yabsys.LineCount == yabsys.VBlankLineCount, isSkipped );
#endif
  }
}

void Vdp2HBlankOUT(void) {
  int i;
  if (yabsys.LineCount < yabsys.VBlankLineCount)
  {
    Vdp2Regs->TVSTAT &= ~0x0004;
    u32 cell_scroll_table_start_addr = (Vdp2Regs->VCSTA.all & 0x7FFFE) << 1;
    memcpy(Vdp2Lines + yabsys.LineCount, Vdp2Regs, sizeof(Vdp2));
    for (i = 0; i < 88; i++)
    {
      cell_scroll_data[yabsys.LineCount].data[i] = Vdp2RamReadLong(NULL, Vdp2Ram, cell_scroll_table_start_addr + i * 4);
    }
    if (yabsys.LineCount == 0) return;

    if ((Vdp2Lines[0].CCRNA & 0x00FF) != (Vdp2Lines[yabsys.LineCount].CCRNA & 0x00FF)){
      Vdp2External.perline_alpha[yabsys.LineCount] |= 0x1;
    }

    if ((Vdp2Lines[0].CCRNA & 0xFF00) != (Vdp2Lines[yabsys.LineCount].CCRNA & 0xFF00)){
      Vdp2External.perline_alpha[yabsys.LineCount] |= 0x2;
    }

    if ((Vdp2Lines[0].CCRNB & 0xFF00) != (Vdp2Lines[yabsys.LineCount].CCRNB & 0xFF00)){
      Vdp2External.perline_alpha[yabsys.LineCount] |= 0x4;
    }

    if ((Vdp2Lines[0].CCRNB & 0x00FF) != (Vdp2Lines[yabsys.LineCount].CCRNB & 0x00FF)){
      Vdp2External.perline_alpha [yabsys.LineCount]|= 0x8;
    }

    if (Vdp2Lines[0].CCRR != Vdp2Lines[yabsys.LineCount].CCRR){
      Vdp2External.perline_alpha[yabsys.LineCount] |= 0x10;
    }

    if (Vdp2Lines[0].COBR != Vdp2Lines[yabsys.LineCount].COBR){

      Vdp2External.perline_alpha[yabsys.LineCount] |= Vdp2Lines[yabsys.LineCount].CLOFEN;
    }
    if (Vdp2Lines[0].COAR != Vdp2Lines[yabsys.LineCount].COAR){

      Vdp2External.perline_alpha[yabsys.LineCount] |= Vdp2Lines[yabsys.LineCount].CLOFEN;
    }
    if (Vdp2Lines[0].COBG != Vdp2Lines[yabsys.LineCount].COBG){

      Vdp2External.perline_alpha[yabsys.LineCount] |= Vdp2Lines[yabsys.LineCount].CLOFEN;
    }
    if (Vdp2Lines[0].COAG != Vdp2Lines[yabsys.LineCount].COAG){

      Vdp2External.perline_alpha[yabsys.LineCount] |= Vdp2Lines[yabsys.LineCount].CLOFEN;
    }
    if (Vdp2Lines[0].COBB != Vdp2Lines[yabsys.LineCount].COBB){

      Vdp2External.perline_alpha[yabsys.LineCount] |= Vdp2Lines[yabsys.LineCount].CLOFEN;
    }
    if (Vdp2Lines[0].COAB != Vdp2Lines[yabsys.LineCount].COAB){

      Vdp2External.perline_alpha[yabsys.LineCount] |= Vdp2Lines[yabsys.LineCount].CLOFEN;
    }
    if (Vdp2Lines[0].PRISA != Vdp2Lines[yabsys.LineCount].PRISA) {

      Vdp2External.perline_alpha[yabsys.LineCount] |= 0x40;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////

Vdp2 * Vdp2RestoreRegs(int line, Vdp2* lines) {
   return line > 270 ? NULL : lines + line;
}

//////////////////////////////////////////////////////////////////////////////
void Vdp2VBlankOUT(void) {
  g_frame_count++;
  FRAMELOG("***** VOUT %d *****", g_frame_count);
  if (Vdp2External.perline_alpha == Vdp2External.perline_alpha_a){
    Vdp2External.perline_alpha = Vdp2External.perline_alpha_b;
    Vdp2External.perline_alpha_draw = Vdp2External.perline_alpha_a;
  }
  else{
    Vdp2External.perline_alpha = Vdp2External.perline_alpha_a;
    Vdp2External.perline_alpha_draw = Vdp2External.perline_alpha_b;
  }
  memset(Vdp2External.perline_alpha, 0, 270*sizeof(int));

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

u8 FASTCALL Vdp2ReadByte(SH2_struct *context, u8* mem, u32 addr) {
   LOG("VDP2 register byte read = %08X\n", addr);
   addr &= 0x1FF;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL Vdp2ReadWord(SH2_struct *context, u8* mem, u32 addr) {
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
      default:
      {
         LOG("Unhandled VDP2 word read: %08X\n", addr);
         break;
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL Vdp2ReadLong(SH2_struct *context, u8* mem, u32 addr) {
   LOG("VDP2 register long read = %08X\n", addr);
   addr &= 0x1FF;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2WriteByte(SH2_struct *context, u8* mem, u32 addr, UNUSED u8 val) {
   LOG("VDP2 register byte write = %08X\n", addr);
   addr &= 0x1FF;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2ReadReg(int addr) {
  u16 val = 0;
  addr &= 0x1FF;
 switch (addr)
   {
      case 0x000:
         val = Vdp2Regs->TVMD;
         break;
      case 0x002:
         val = Vdp2Regs->EXTEN;
         break;
      case 0x004:
         val = Vdp2Regs->TVSTAT;
         break;
      case 0x006:
         val = Vdp2Regs->VRSIZE;
         break;
      case 0x008:
         val = Vdp2Regs->HCNT;
         break;
      case 0x00A:
         val = Vdp2Regs->VCNT;
         break;
      case 0x00C:
         break;
      case 0x00E:
         val = Vdp2Regs->RAMCTL;
         break;
      case 0x010:
         val = Vdp2Regs->CYCA0L;
         break;
      case 0x012:
         val = Vdp2Regs->CYCA0U;
         break;
      case 0x014:
         val = Vdp2Regs->CYCA1L;
         break;
      case 0x016:
         val = Vdp2Regs->CYCA1U;
         break;
      case 0x018:
         val = Vdp2Regs->CYCB0L;
         break;
      case 0x01A:
         val = Vdp2Regs->CYCB0U;
         break;
      case 0x01C:
         val = Vdp2Regs->CYCB1L;
         break;
      case 0x01E:
         val = Vdp2Regs->CYCB1U;
         break;
      case 0x020:
         val = Vdp2Regs->BGON;
         break;
      case 0x022:
         val = Vdp2Regs->MZCTL;
         break;
      case 0x024:
         val = Vdp2Regs->SFSEL;
         break;
      case 0x026:
         val = Vdp2Regs->SFCODE;
         break;
      case 0x028:
         val = Vdp2Regs->CHCTLA;
         break;
      case 0x02A:
         val = Vdp2Regs->CHCTLB;
         break;
      case 0x02C:
         val = Vdp2Regs->BMPNA;
         break;
      case 0x02E:
         val = Vdp2Regs->BMPNB;
         break;
      case 0x030:
         val = Vdp2Regs->PNCN0;
         break;
      case 0x032:
         val = Vdp2Regs->PNCN1;
         break;
      case 0x034:
         val = Vdp2Regs->PNCN2;
         break;
      case 0x036:
         val = Vdp2Regs->PNCN3;
         break;
      case 0x038:
         val = Vdp2Regs->PNCR;
         break;
      case 0x03A:
         val = Vdp2Regs->PLSZ;
         break;
      case 0x03C:
         val = Vdp2Regs->MPOFN;
         break;
      case 0x03E:
         val = Vdp2Regs->MPOFR;
         break;
      case 0x040:
         val = Vdp2Regs->MPABN0;
         break;
      case 0x042:
         val = Vdp2Regs->MPCDN0;
         break;
      case 0x044:
         val = Vdp2Regs->MPABN1;
         break;
      case 0x046:
         val = Vdp2Regs->MPCDN1;
         break;
      case 0x048:
         val = Vdp2Regs->MPABN2;
         break;
      case 0x04A:
         val = Vdp2Regs->MPCDN2;
         break;
      case 0x04C:
         val = Vdp2Regs->MPABN3;
         break;
      case 0x04E:
         val = Vdp2Regs->MPCDN3;
         break;
      case 0x050:
         val = Vdp2Regs->MPABRA;
         break;
      case 0x052:
         val = Vdp2Regs->MPCDRA;
         break;
      case 0x054:
         val = Vdp2Regs->MPEFRA;
         break;
      case 0x056:
         val = Vdp2Regs->MPGHRA;
         break;
      case 0x058:
         val = Vdp2Regs->MPIJRA;
         break;
      case 0x05A:
         val = Vdp2Regs->MPKLRA;
         break;
      case 0x05C:
         val = Vdp2Regs->MPMNRA;
         break;
      case 0x05E:
         val = Vdp2Regs->MPOPRA;
         break;
      case 0x060:
         val = Vdp2Regs->MPABRB;
         break;
      case 0x062:
         val = Vdp2Regs->MPCDRB;
         break;
      case 0x064:
         val = Vdp2Regs->MPEFRB;
         break;
      case 0x066:
         val = Vdp2Regs->MPGHRB;
         break;
      case 0x068:
         val = Vdp2Regs->MPIJRB;
         break;
      case 0x06A:
         val = Vdp2Regs->MPKLRB;
         break;
      case 0x06C:
         val = Vdp2Regs->MPMNRB;
         break;
      case 0x06E:
         val = Vdp2Regs->MPOPRB;
         break;
      case 0x070:
         val = Vdp2Regs->SCXIN0;
         break;
      case 0x072:
         val = Vdp2Regs->SCXDN0;
         break;
      case 0x074:
         val = Vdp2Regs->SCYIN0;
         break;
      case 0x076:
         val = Vdp2Regs->SCYDN0;
         break;
      case 0x078:
         val = Vdp2Regs->ZMXN0.part.I;
         break;
      case 0x07A:
         val = Vdp2Regs->ZMXN0.part.D;
         break;
      case 0x07C:
         val = Vdp2Regs->ZMYN0.part.I;
         break;
      case 0x07E:
         val = Vdp2Regs->ZMYN0.part.D;
         break;
      case 0x080:
         val = Vdp2Regs->SCXIN1;
         break;
      case 0x082:
         val = Vdp2Regs->SCXDN1;
         break;
      case 0x084:
         val = Vdp2Regs->SCYIN1;
         break;
      case 0x086:
         val = Vdp2Regs->SCYDN1;
         break;
      case 0x088:
         val = Vdp2Regs->ZMXN1.part.I;
         break;
      case 0x08A:
         val = Vdp2Regs->ZMXN1.part.D;
         break;
      case 0x08C:
         val = Vdp2Regs->ZMYN1.part.I;
         break;
      case 0x08E:
         val = Vdp2Regs->ZMYN1.part.D;
         break;
      case 0x090:
         val = Vdp2Regs->SCXN2;
         break;
      case 0x092:
         val = Vdp2Regs->SCYN2;
         break;
      case 0x094:
         val = Vdp2Regs->SCXN3;
         break;
      case 0x096:
         val = Vdp2Regs->SCYN3;
         break;
      case 0x098:
         val = Vdp2Regs->ZMCTL;
         break;
      case 0x09A:
         val = Vdp2Regs->SCRCTL;
         break;
      case 0x09C:
         val = Vdp2Regs->VCSTA.part.U;
         break;
      case 0x09E:
         val = Vdp2Regs->VCSTA.part.L;
         break;
      case 0x0A0:
         val = Vdp2Regs->LSTA0.part.U;
         break;
      case 0x0A2:
         val = Vdp2Regs->LSTA0.part.L;
         break;
      case 0x0A4:
         val = Vdp2Regs->LSTA1.part.U;
         break;
      case 0x0A6:
         val = Vdp2Regs->LSTA1.part.L;
         break;
      case 0x0A8:
         val = Vdp2Regs->LCTA.part.U;
         break;
      case 0x0AA:
         val = Vdp2Regs->LCTA.part.L;
         break;
      case 0x0AC:
         val = Vdp2Regs->BKTAU;
         break;
      case 0x0AE:
         val = Vdp2Regs->BKTAL;
         break;
      case 0x0B0:
         val = Vdp2Regs->RPMD;
         break;
      case 0x0B2:
         val = Vdp2Regs->RPRCTL;
         break;
      case 0x0B4:
         val = Vdp2Regs->KTCTL;
         break;
      case 0x0B6:
         val = Vdp2Regs->KTAOF;
         break;
      case 0x0B8:
         val = Vdp2Regs->OVPNRA;
         break;
      case 0x0BA:
         val = Vdp2Regs->OVPNRB;
         break;
      case 0x0BC:
         val = Vdp2Regs->RPTA.part.U;
         break;
      case 0x0BE:
         val = Vdp2Regs->RPTA.part.L;
         break;
      case 0x0C0:
         val = Vdp2Regs->WPSX0;
         break;
      case 0x0C2:
         val = Vdp2Regs->WPSY0;
         break;
      case 0x0C4:
         val = Vdp2Regs->WPEX0;
         break;
      case 0x0C6:
         val = Vdp2Regs->WPEY0;
         break;
      case 0x0C8:
         val = Vdp2Regs->WPSX1;
         break;
      case 0x0CA:
         val = Vdp2Regs->WPSY1;
         break;
      case 0x0CC:
         val = Vdp2Regs->WPEX1;
         break;
      case 0x0CE:
         val = Vdp2Regs->WPEY1;
         break;
      case 0x0D0:
         val = Vdp2Regs->WCTLA;
         break;
      case 0x0D2:
         val = Vdp2Regs->WCTLB;
         break;
      case 0x0D4:
         val = Vdp2Regs->WCTLC;
         break;
      case 0x0D6:
         val = Vdp2Regs->WCTLD;
         break;
      case 0x0D8:
         val = Vdp2Regs->LWTA0.part.U;
         break;
      case 0x0DA:
         val = Vdp2Regs->LWTA0.part.L;
         break;
      case 0x0DC:
         val = Vdp2Regs->LWTA1.part.U;
         break;
      case 0x0DE:
         val = Vdp2Regs->LWTA1.part.L;
         break;
      case 0x0E0:
         val = Vdp2Regs->SPCTL;
         break;
      case 0x0E2:
         val = Vdp2Regs->SDCTL;
         break;
      case 0x0E4:
         val = Vdp2Regs->CRAOFA;
         break;
      case 0x0E6:
         val = Vdp2Regs->CRAOFB;
         break;
      case 0x0E8:
         val = Vdp2Regs->LNCLEN;
         break;
      case 0x0EA:
         val = Vdp2Regs->SFPRMD;
         break;
      case 0x0EC:
         val = Vdp2Regs->CCCTL;
         break;
      case 0x0EE:
         val = Vdp2Regs->SFCCMD;
         break;
      case 0x0F0:
         val = Vdp2Regs->PRISA;
         break;
      case 0x0F2:
         val = Vdp2Regs->PRISB;
         break;
      case 0x0F4:
         val = Vdp2Regs->PRISC;
         break;
      case 0x0F6:
         val = Vdp2Regs->PRISD;
         break;
      case 0x0F8:
         val = Vdp2Regs->PRINA;
         break;
      case 0x0FA:
         val = Vdp2Regs->PRINB;
         break;
      case 0x0FC:
         val = Vdp2Regs->PRIR;
         break;
      case 0x0FE:
         break;
      case 0x100:
         val = Vdp2Regs->CCRSA;
         break;
      case 0x102:
         val = Vdp2Regs->CCRSB;
         break;
      case 0x104:
         val = Vdp2Regs->CCRSC;
         break;
      case 0x106:
         val = Vdp2Regs->CCRSD;
         break;
      case 0x108:
         val = Vdp2Regs->CCRNA;
         break;
      case 0x10A:
         val = Vdp2Regs->CCRNB;
         break;
      case 0x10C:
         val = Vdp2Regs->CCRR;
         break;
      case 0x10E:
         val = Vdp2Regs->CCRLB;
         break;
      case 0x110:
         val = Vdp2Regs->CLOFEN;
         break;
      case 0x112:
         val = Vdp2Regs->CLOFSL;
         break;
      case 0x114:
         val = Vdp2Regs->COAR;
         break;
      case 0x116:
         val = Vdp2Regs->COAG;
         break;
      case 0x118:
         val = Vdp2Regs->COAB;
         break;
      case 0x11A:
         val = Vdp2Regs->COBR;
         break;
      case 0x11C:
         val = Vdp2Regs->COBG;
         break;
      case 0x11E:
         val = Vdp2Regs->COBB;
         break;
      default:
      {
         break;
      }
   }
}

void FASTCALL Vdp2WriteWord(SH2_struct *context, u8* mem, u32 addr, u16 val) {
   addr &= 0x1FF;

   switch (addr)
   {
      case 0x000:
         Vdp2Regs->TVMD = val;
         yabsys.VBlankLineCount = 225+(val & 0x30);
         if (yabsys.VBlankLineCount > 256) yabsys.VBlankLineCount = 256;
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
#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
           for (int i = 0; i < 0x1000; i += 2) {
             YglOnUpdateColorRamWord(i);
           }
#endif
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

void FASTCALL Vdp2WriteLong(SH2_struct *context, u8* mem, u32 addr, u32 val) {

   Vdp2WriteWord(context, mem, addr,val>>16);
   Vdp2WriteWord(context, mem, addr+2,val&0xFFFF);
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
   ywrite(&check, (void *)Vdp2Ram, 0x100000, 1, fp);

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
   yread(&check, (void *)Vdp2Ram, 0x100000, 1, fp);

   // Read CRAM
   yread(&check, (void *)Vdp2ColorRam, 0x1000, 1, fp);

   // Read internal variables
   yread(&check, (void *)&Vdp2Internal, sizeof(Vdp2Internal_struct), 1, fp);

   if(VIDCore) VIDCore->Resize(0,0,0,0,0);

#if defined(HAVE_LIBGL) || defined(__ANDROID__) || defined(IOS)
   for (int i = 0; i < 0x1000; i += 2) {
     YglOnUpdateColorRamWord(i);
   }
#endif

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

void ToggleRBG1(void)
{
   Vdp2External.disptoggle ^= 0x20;
}

//////////////////////////////////////////////////////////////////////////////

void ToggleFullScreen(void)
{
   if (VIDCore->IsFullscreen())
   {
      VIDCore->Resize(0,0,320, 224, 0);
   }
   else
   {
      VIDCore->Resize(0,0,320, 224, 1);
   }
}
