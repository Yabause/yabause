/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2004-2006 Theo Berkau

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

#include <stdlib.h>
#include "vdp2.h"
#include "debug.h"
#include "scu.h"
#include "sh2core.h"
#include "vdp1.h"
#include "yabause.h"

u8 * Vdp2Ram;
u8 * Vdp2ColorRam;
Vdp2 * Vdp2Regs;
Vdp2Internal_struct Vdp2Internal;

static int autoframeskipenab=0;
static int framestoskip=0;
static int framesskipped=0;
static int throttlespeed=0;
static int skipnextframe=0;
u64 lastticks=0;
static u64 curticks=0;
static u64 diffticks=0;
static u32 framecount=0;
static u64 onesecondticks=0;
static int fps;
static int fpsframecount=0;
static u64 fpsticks;
static int fpstoggle=0;

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

void FASTCALL Vdp2RamWriteByte(u32 addr, u8 val) {
   addr &= 0x7FFFF;
   T1WriteByte(Vdp2Ram, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2RamWriteWord(u32 addr, u16 val) {
   addr &= 0x7FFFF;
   T1WriteWord(Vdp2Ram, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2RamWriteLong(u32 addr, u32 val) {
   addr &= 0x7FFFF;
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
   T2WriteByte(Vdp2ColorRam, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteWord(u32 addr, u16 val) {
   addr &= 0xFFF;
   T2WriteWord(Vdp2ColorRam, addr, val);
//   if (Vdp2Internal.ColorMode == 0)
//      T1WriteWord(Vdp2ColorRam, addr + 0x800, val);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2ColorRamWriteLong(u32 addr, u32 val) {
   addr &= 0xFFF;
   T2WriteLong(Vdp2ColorRam, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

int Vdp2Init(void) {
   if ((Vdp2Regs = (Vdp2 *) calloc(1, sizeof(Vdp2))) == NULL)
      return -1;

   if ((Vdp2Ram = T1MemoryInit(0x80000)) == NULL)
      return -1;

   if ((Vdp2ColorRam = T2MemoryInit(0x1000)) == NULL)
      return -1;

   Vdp2Reset();
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DeInit(void) {
   if (Vdp2Regs)
      free(Vdp2Regs);

   if (Vdp2Ram)
      T1MemoryDeInit(Vdp2Ram);

   if (Vdp2ColorRam)
      T2MemoryDeInit(Vdp2ColorRam);
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

   yabsys.VBlankLineCount = 224;
   Vdp2Internal.ColorMode = 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2VBlankIN(void) {
   Vdp2Regs->TVSTAT |= 0x0008;
   ScuSendVBlankIN();

   if (yabsys.IsSSH2Running)
      SH2SendInterrupt(SSH2, 0x43, 0x6);
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2HBlankIN(void) {
   Vdp2Regs->TVSTAT |= 0x0004;
   ScuSendHBlankIN();

   if (yabsys.IsSSH2Running)
      SH2SendInterrupt(SSH2, 0x41, 0x2);
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2HBlankOUT(void) {
   Vdp2Regs->TVSTAT &= ~0x0004;
}

//////////////////////////////////////////////////////////////////////////////

void FPSDisplay(void)
{
   if (fpstoggle)
   {
      VIDCore->OnScreenDebugMessage("%02d/%02d FPS", fps, yabsys.IsPal ? 50 : 60);

      fpsframecount++;
      if(YabauseGetTicks() >= fpsticks + yabsys.tickfreq)
      {
         fps = fpsframecount;
         fpsframecount = 0;
         fpsticks = YabauseGetTicks();
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void ToggleFPS(void)
{
   fpstoggle ^= 1;
}

//////////////////////////////////////////////////////////////////////////////

void SpeedThrottleEnable(void) {
   throttlespeed = 1;
}

//////////////////////////////////////////////////////////////////////////////

void SpeedThrottleDisable(void) {
   throttlespeed = 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2VBlankOUT(void) {
   Vdp2Regs->TVSTAT = (Vdp2Regs->TVSTAT & ~0x0008) | 0x0002;

   if (!skipnextframe)
   {
      VIDCore->Vdp2DrawStart();

      if (Vdp2Regs->TVMD & 0x8000) {
         VIDCore->Vdp2DrawScreens();
         Vdp1Draw();
      }
      else
         Vdp1NoDraw();

      FPSDisplay();
      VIDCore->Vdp2DrawEnd();

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
      Vdp1NoDraw();

      framesskipped++;
   }

   // Do Frame Skip/Frame Limiting/Speed Throttling here
   if (throttlespeed)
   {
      // Should really depend on how fast we're rendering the frames
      if (framestoskip < 1)
         framestoskip = 6;
   }
   else if (autoframeskipenab)
   {
      framecount++;

      if (framecount > (yabsys.IsPal ? 50 : 60))
      {
         framecount = 1;
         onesecondticks = 0;
      }

      curticks = YabauseGetTicks();
      diffticks = curticks-lastticks;

      if ((onesecondticks+diffticks) > ((yabsys.OneFrameTime * (u64)framecount) + (yabsys.OneFrameTime / 2)) &&
          framesskipped < 9)
      {
         // Skip the next frame
         skipnextframe = 1;

         // How many frames should we skip?
         framestoskip = 1;
      }
      else if ((onesecondticks+diffticks) < ((yabsys.OneFrameTime * (u64)framecount) - (yabsys.OneFrameTime / 2)))
      {
         // Check to see if we need to limit speed at all
         for (;;)
         {
            curticks = YabauseGetTicks();
            diffticks = curticks-lastticks;
            if ((onesecondticks+diffticks) >= (yabsys.OneFrameTime * (u64)framecount))
               break;
         }
      }

      onesecondticks += diffticks;
      lastticks = curticks;
   }

   ScuSendVBlankOUT();
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
         // Clear External latch and sync flags
         Vdp2Regs->TVSTAT &= 0xFCFF;

         // if TVMD's DISP bit is cleared, TVSTAT's VBLANK bit is always set
         if (Vdp2Regs->TVMD & 0x8000)
            return Vdp2Regs->TVSTAT;
         else
            return (Vdp2Regs->TVSTAT | 0x8);
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

u32 FASTCALL Vdp2ReadLong(u32 addr) {
   LOG("VDP2 register long read = %08X\n", addr);
   addr &= 0x1FF;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2WriteByte(u32 addr, u8 val) {
   LOG("VDP2 register byte write = %08X\n", addr);
   addr &= 0x1FF;
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL Vdp2WriteWord(u32 addr, u16 val) {
   addr &= 0x1FF;

   switch (addr)
   {
      case 0x000:
         VIDCore->Vdp2SetResolution(val);
         Vdp2Regs->TVMD = val;
         yabsys.VBlankLineCount = 224+(val & 0x30);
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
         Vdp2Internal.ColorMode = (val >> 12) & 0x3;
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
         VIDCore->Vdp2SetPriorityNBG0(val & 0x7);
         VIDCore->Vdp2SetPriorityNBG1((val >> 8) & 0x7);
         Vdp2Regs->PRINA = val;
         return;
      case 0x0FA:
         VIDCore->Vdp2SetPriorityNBG2(val & 0x7);
         VIDCore->Vdp2SetPriorityNBG3((val >> 8) & 0x7);
         Vdp2Regs->PRINB = val;
         return;
      case 0x0FC:
         VIDCore->Vdp2SetPriorityRBG0(val & 0x7);
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
   addr &= 0x1FF;

   switch (addr)
   {
      case 0x000:
         Vdp2Regs->TVMD = val >> 16;
         Vdp2Regs->EXTEN = val & 0xFFFF;
         VIDCore->Vdp2SetResolution(Vdp2Regs->TVMD);
         yabsys.VBlankLineCount = 224+(Vdp2Regs->TVMD & 0x30);
         return;
      case 0x004:
         Vdp2Regs->VRSIZE = val & 0xFFFF;
         return;
      case 0x008:
         // read-only
         return;
      case 0x00C:
         Vdp2Regs->RAMCTL = val & 0xFFFF;
         return;
      case 0x010:
         Vdp2Regs->CYCA0L = val >> 16;
         Vdp2Regs->CYCA0U = val & 0xFFFF;
         return;
      case 0x014:
         Vdp2Regs->CYCA1L = val >> 16;
         Vdp2Regs->CYCA1U = val & 0xFFFF;
         return;
      case 0x018:
         Vdp2Regs->CYCB0L = val >> 16;
         Vdp2Regs->CYCB0U = val & 0xFFFF;
         return;
      case 0x01C:
         Vdp2Regs->CYCB1L = val >> 16;
         Vdp2Regs->CYCB1U = val & 0xFFFF;
         return;
      case 0x020:
         Vdp2Regs->BGON = val >> 16;
         Vdp2Regs->MZCTL = val & 0xFFFF;
         return;
      case 0x024:
         Vdp2Regs->SFSEL = val >> 16;
         Vdp2Regs->SFCODE = val & 0xFFFF;
         return;
      case 0x028:
         Vdp2Regs->CHCTLA = val >> 16;
         Vdp2Regs->CHCTLB = val & 0xFFFF;
         return;
      case 0x02C:
         Vdp2Regs->BMPNA = val >> 16;
         Vdp2Regs->BMPNB = val & 0xFFFF;
         return;
      case 0x030:
         Vdp2Regs->PNCN0 = val >> 16;
         Vdp2Regs->PNCN1 = val & 0xFFFF;
         return;
      case 0x034:
         Vdp2Regs->PNCN2 = val >> 16;
         Vdp2Regs->PNCN3 = val & 0xFFFF;
         return;
      case 0x038:
         Vdp2Regs->PNCR = val >> 16;
         Vdp2Regs->PLSZ = val & 0xFFFF;
         return;
      case 0x03C:
         Vdp2Regs->MPOFN = val >> 16;
         Vdp2Regs->MPOFR = val & 0xFFFF;
         return;
      case 0x040:
         Vdp2Regs->MPABN0 = val >> 16;
         Vdp2Regs->MPCDN0 = val & 0xFFFF;
         return;
      case 0x044:
         Vdp2Regs->MPABN1 = val >> 16;
         Vdp2Regs->MPCDN1 = val & 0xFFFF;
         return;
      case 0x048:
         Vdp2Regs->MPABN2 = val >> 16;
         Vdp2Regs->MPCDN2 = val & 0xFFFF;
         return;
      case 0x04C:
         Vdp2Regs->MPABN3 = val >> 16;
         Vdp2Regs->MPCDN3 = val & 0xFFFF;
         return;
      case 0x050:
         Vdp2Regs->MPABRA = val >> 16;
         Vdp2Regs->MPCDRA = val & 0xFFFF;
         return;
      case 0x054:
         Vdp2Regs->MPEFRA = val >> 16;
         Vdp2Regs->MPGHRA = val & 0xFFFF;
         return;
      case 0x058:
         Vdp2Regs->MPIJRA = val >> 16;
         Vdp2Regs->MPKLRA = val & 0xFFFF;
         return;
      case 0x05C:
         Vdp2Regs->MPMNRA = val >> 16;
         Vdp2Regs->MPOPRA = val & 0xFFFF;
         return;
      case 0x060:
         Vdp2Regs->MPABRB = val >> 16;
         Vdp2Regs->MPCDRB = val & 0xFFFF;
         return;
      case 0x064:
         Vdp2Regs->MPEFRB = val >> 16;
         Vdp2Regs->MPGHRB = val & 0xFFFF;
         return;
      case 0x068:
         Vdp2Regs->MPIJRB = val >> 16;
         Vdp2Regs->MPKLRB = val & 0xFFFF;
         return;
      case 0x06C:
         Vdp2Regs->MPMNRB = val >> 16;
         Vdp2Regs->MPOPRB = val & 0xFFFF;
         return;
      case 0x070:
         Vdp2Regs->SCXIN0 = val >> 16;
         // SCXDN0 here
         return;
      case 0x074:
         Vdp2Regs->SCYIN0 = val >> 16;
         // SCYDN0 here
         return;
      case 0x078:
         Vdp2Regs->ZMXN0.all = val;
         return;
      case 0x07C:
         Vdp2Regs->ZMYN0.all = val;
         return;
      case 0x080:
         Vdp2Regs->SCXIN1 = val >> 16;
         // SCXDN1 here
         return;
      case 0x084:
         Vdp2Regs->SCYIN1 = val >> 16;
         // SCYDN1 here
         return;
      case 0x088:
         Vdp2Regs->ZMXN1.all = val;
         return;
      case 0x08C:
         Vdp2Regs->ZMYN1.all = val;
         return;
      case 0x090:
         Vdp2Regs->SCXN2 = val >> 16;
         Vdp2Regs->SCYN2 = val & 0xFFFF;
         return;
      case 0x094:
         Vdp2Regs->SCXN3 = val >> 16;
         Vdp2Regs->SCYN3 = val & 0xFFFF;
         return;
      case 0x09C:
         Vdp2Regs->VCSTA.all = val;
         return;
      case 0x0A0:
         Vdp2Regs->LSTA0.all = val;
         return;
      case 0x0A4:
         Vdp2Regs->LSTA1.all = val;
         return;
      case 0x0AC:
         Vdp2Regs->BKTAU = val >> 16;
         Vdp2Regs->BKTAL = val & 0xFFFF;
         return;
      case 0x0D0:
         Vdp2Regs->WCTLA = val >> 16;
         Vdp2Regs->WCTLB = val & 0xFFFF;
         return;
      case 0x0E0:
         Vdp2Regs->SPCTL = val >> 16;
         Vdp2Regs->SDCTL = val & 0xFFFF;
         return;
      case 0x0E4:
         Vdp2Regs->CRAOFA = val >> 16;
         Vdp2Regs->CRAOFB = val & 0xFFFF;
         return;
      case 0x0E8:
         Vdp2Regs->LNCLEN = val >> 16;
         Vdp2Regs->SFPRMD = val & 0xFFFF;
         return;
      case 0x0EC:
         Vdp2Regs->CCCTL = val >> 16;
         Vdp2Regs->SFCCMD = val & 0xFFFF;
         return;
      case 0x0F0:
         Vdp2Regs->PRISA = val >> 16;
         Vdp2Regs->PRISB = val & 0xFFFF;
         return;
      case 0x0F4:
         Vdp2Regs->PRISC = val >> 16;
         Vdp2Regs->PRISD = val & 0xFFFF;
         return;
      case 0x0F8:
         Vdp2Regs->PRINA = val >> 16;
         Vdp2Regs->PRINB = val & 0xFFFF;
         VIDCore->Vdp2SetPriorityNBG0(Vdp2Regs->PRINA & 0x7);
         VIDCore->Vdp2SetPriorityNBG1((Vdp2Regs->PRINA >> 8) & 0x7);
         VIDCore->Vdp2SetPriorityNBG2(Vdp2Regs->PRINB & 0x7);
         VIDCore->Vdp2SetPriorityNBG3((Vdp2Regs->PRINB >> 8) & 0x7);
         return;
      case 0x100:
         Vdp2Regs->CCRSA = val >> 16;
         Vdp2Regs->CCRSB = val & 0xFFFF;
         return;
      case 0x104:
         Vdp2Regs->CCRSC = val >> 16;
         Vdp2Regs->CCRSD = val & 0xFFFF;
         return;
      case 0x108:
         Vdp2Regs->CCRNA = val >> 16;
         Vdp2Regs->CCRNB = val & 0xFFFF;
         return;
      case 0x10C:
         Vdp2Regs->CCRR = val >> 16;
         Vdp2Regs->CCRLB = val & 0xFFFF;
         return;
      case 0x110:
         Vdp2Regs->CLOFEN = val >> 16;
         Vdp2Regs->CLOFSL = val & 0xFFFF;
         return;
      case 0x114:
         Vdp2Regs->COAR = val >> 16;
         Vdp2Regs->COAG = val & 0xFFFF;
         return;
      case 0x118:
         Vdp2Regs->COAB = val >> 16;
         Vdp2Regs->COBR = val & 0xFFFF;
         return;
      case 0x11C:
         Vdp2Regs->COBG = val >> 16;
         Vdp2Regs->COBB = val & 0xFFFF;
         return;
      case 0x120:
      case 0x124:
      case 0x128:
      case 0x12C:
      case 0x130:
      case 0x134:
      case 0x138:
      case 0x13C:
      case 0x140:
      case 0x144:
      case 0x148:
      case 0x14C:
      case 0x150:
      case 0x154:
      case 0x158:
      case 0x15C:
      case 0x160:
      case 0x164:
      case 0x168:
      case 0x16C:
      case 0x170:
      case 0x174:
      case 0x178:
      case 0x17C:
      case 0x180:
      case 0x184:
      case 0x188:
      case 0x18C:
      case 0x190:
      case 0x194:
      case 0x198:
      case 0x19C:
      case 0x1A0:
      case 0x1A4:
      case 0x1A8:
      case 0x1AC:
      case 0x1B0:
      case 0x1B4:
      case 0x1B8:
      case 0x1BC:
      case 0x1C0:
      case 0x1C4:
      case 0x1C8:
      case 0x1CC:
      case 0x1D0:
      case 0x1D4:
      case 0x1D8:
      case 0x1DC:
      case 0x1E0:
      case 0x1E4:
      case 0x1E8:
      case 0x1EC:
      case 0x1F0:
      case 0x1F4:
      case 0x1F8:
      case 0x1FC:
         // Reserved
         return;
      default:
      {
         LOG("Unhandled VDP2 long write: %08X\n", addr);
         break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

int Vdp2SaveState(FILE *fp)
{
   int offset;

   offset = StateWriteHeader(fp, "VDP2", 1);

   // Write registers
   fwrite((void *)Vdp2Regs, sizeof(Vdp2), 1, fp);

   // Write VDP2 ram
   fwrite((void *)Vdp2Ram, 0x80000, 1, fp);

   // Write CRAM
   fwrite((void *)Vdp2ColorRam, 0x1000, 1, fp);

   // Write internal variables
   fwrite((void *)&Vdp2Internal, sizeof(Vdp2Internal_struct), 1, fp);

   return StateFinishHeader(fp, offset);
}

//////////////////////////////////////////////////////////////////////////////

int Vdp2LoadState(FILE *fp, int version, int size)
{
   // Read registers
   fread((void *)Vdp2Regs, sizeof(Vdp2), 1, fp);

   // Read VDP2 ram
   fread((void *)Vdp2Ram, 0x80000, 1, fp);

   // Read CRAM
   fread((void *)Vdp2ColorRam, 0x1000, 1, fp);

   // Read internal variables
   fread((void *)&Vdp2Internal, sizeof(Vdp2Internal_struct), 1, fp);

   VIDCore->Vdp2SetResolution(Vdp2Regs->TVMD);
   VIDCore->Vdp2SetPriorityNBG0(Vdp2Regs->PRINA & 0x7);
   VIDCore->Vdp2SetPriorityNBG1((Vdp2Regs->PRINA >> 8) & 0x7);
   VIDCore->Vdp2SetPriorityNBG2(Vdp2Regs->PRINB & 0x7);
   VIDCore->Vdp2SetPriorityNBG3((Vdp2Regs->PRINB >> 8) & 0x7);
   VIDCore->Vdp2SetPriorityRBG0(Vdp2Regs->PRIR & 0x7);

   return size;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE void Vdp2GetPlaneSize(int planedata, int *planew, int *planeh)
{
   switch(planedata)
   {
      case 0:
         *planew = *planeh = 1;
         break;
      case 1:
         *planew = 2;
         *planeh = 1;
         break;
      case 2:
         *planew = *planeh = 2;
         break;
      default:
         *planew = *planeh = 1;         
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

static INLINE char *AddBppString(char *outstring, int bpp)
{
   switch (bpp)
   {
      case 0:
         AddString(outstring, "4-bit(16 colors)\r\n");
         break;
      case 1:
         AddString(outstring, "8-bit(256 colors)\r\n");
         break;
      case 2:
         AddString(outstring, "16-bit(2048 colors)\r\n");
         break;
      case 3:
         AddString(outstring, "16-bit(32,768 colors)\r\n");
         break;
      case 4:
         AddString(outstring, "32-bit(16.7 mil colors)\r\n");
         break;
      default:
         AddString(outstring, "Unsupported BPP\r\n");
         break;
   }

   return outstring;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE char *AddMosaicString(char *outstring, int mask)
{
   if (Vdp2Regs->MZCTL & mask)
   {
      AddString(outstring, "Mosaic Size = width %d height %d\r\n", ((Vdp2Regs->MZCTL >> 8) & 0xf) + 1, (Vdp2Regs->MZCTL >> 12) + 1);
   }

   return outstring;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE char *AddBitmapInfoString(char *outstring, int wh, int palnum, int mapofn)
{
   int cellw=0, cellh=0;

   // Bitmap
   switch(wh)
   {
      case 0:
         cellw = 512;
         cellh = 256;
         break;
      case 1:
         cellw = 512;
         cellh = 512;
         break;
      case 2:
         cellw = 1024;
         cellh = 256;
         break;
      case 3:
         cellw = 1024;
         cellh = 512;
         break;                                                           
   }

   AddString(outstring, "Bitmap(%dx%d)\r\n", cellw, cellh);

   if (palnum & 0x20)
   {
      AddString(outstring, "Bitmap Special Priority enabled\r\n");
   }

   if (palnum & 0x10)
   {              
      AddString(outstring, "Bitmap Special Color Calculation enabled\r\n");
   }

   AddString(outstring, "Bitmap Address = %X\r\n", (mapofn & 0x7) * 0x20000);
   AddString(outstring, "Bitmap Palette Address = %X\r\n", (palnum & 0x7) << 4);

   return outstring;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE char *AddWindowInfoString(char *outstring, int wctl)
{
   if (wctl & 0x2)
   {
      int hstart=0, vstart=0, hend=0, vend=0;

      AddString(outstring, "Window W0 Enabled:\r\n");

      // Retrieve Window Points
      if (Vdp2Regs->LWTA0.all & 0x80000000)
      {
         // Line Window
      }
      else
      {
         // Normal Window
         hstart = Vdp2Regs->WPSX0 & 0x3FF;
         vstart = Vdp2Regs->WPSY0 & 0x1FF;
         hend = Vdp2Regs->WPEX0 & 0x3FF;
         vend = Vdp2Regs->WPEY0 & 0x1FF;
      }

      AddString(outstring, "Horizontal start = %d\r\n", hstart);
      AddString(outstring, "Vertical start = %d\r\n", vstart);
      AddString(outstring, "Horizontal end = %d\r\n", hend);
      AddString(outstring, "Vertical end = %d\r\n", vend);
      AddString(outstring, "Display %s of Window\r\n", (wctl & 0x1) ? "outside" : "inside");
   }

   if (wctl & 0x8)
   {
      int hstart=0, vstart=0, hend=0, vend=0;

      AddString(outstring, "Window W1 Enabled:\r\n");

      // Retrieve Window Points
      if (Vdp2Regs->LWTA1.all & 0x80000000)
      {
         // Line Window
      }
      else
      {
         // Normal Window
         hstart = Vdp2Regs->WPSX1 & 0x3FF;
         vstart = Vdp2Regs->WPSY1 & 0x1FF;
         hend = Vdp2Regs->WPEX1 & 0x3FF;
         vend = Vdp2Regs->WPEY1 & 0x1FF;
      }

      AddString(outstring, "Horizontal start = %d\r\n", hstart);
      AddString(outstring, "Vertical start = %d\r\n", vstart);
      AddString(outstring, "Horizontal end = %d\r\n", hend);
      AddString(outstring, "Vertical end = %d\r\n", vend);
      AddString(outstring, "Display %s of Window\r\n", (wctl & 0x4) ? "outside" : "inside");
   }

   if (wctl & 0x20)
   {
      AddString(outstring, "Sprite Window Enabled:\r\n");
      AddString(outstring, "Display %s of Window\r\n", (wctl & 0x10) ? "outside" : "inside");
   }

   if (wctl & 0x2A)
   {
      AddString(outstring, "Window Overlap Logic: %s\r\n", (wctl & 0x80) ? "AND" : "OR");
   }
   else
   {
      if (wctl & 0x80)
      {
         // Whole screen window enabled
         AddString(outstring, "Window enabled whole screen\r\n");      
      }
      else
      {
         // Whole screen window disabled
         AddString(outstring, "Window disabled whole screen\r\n");
      }
   }

   return outstring;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE char *AddMapInfo(char *outstring, int patternwh, u16 PNC, u8 PLSZ, int mapoffset, int numplanes, u8 *map)
{
   int deca;
   int multi;
   int i;
   int patterndatasize;
   int planew, planeh;
   u32 tmp=0;
   u32 addr;

   if(PNC & 0x8000)
      patterndatasize = 1;
   else
      patterndatasize = 2;

   switch(PLSZ)
   {
      case 0:
         planew = planeh = 1;
         break;
      case 1:
         planew = 2;
         planeh = 1;
         break;
      case 2:
         planew = planeh = 2;
         break;
      default: // Not sure what 0x3 does
         planew = planeh = 1;
         break;
   }

   deca = planeh + planew - 2;
   multi = planeh * planew;

   // Map Planes A-D
   for (i = 0; i < numplanes; i++)
   {
      tmp = mapoffset | map[i];

      if (patterndatasize == 1)
      {
         if (patternwh == 1)
            addr = ((tmp & 0x3F) >> deca) * (multi * 0x2000);
         else
            addr = (tmp >> deca) * (multi * 0x800);
      }
      else
      {
         if (patternwh == 1)
            addr = ((tmp & 0x1F) >> deca) * (multi * 0x4000);
         else
            addr = ((tmp & 0x7F) >> deca) * (multi * 0x1000);
      }
  
      AddString(outstring, "Plane %C Address = %08X\r\n", 0x41+i, (unsigned int)addr);
   }

   return outstring;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE char *AddColorCalcInfo(char *outstring, u16 calcenab, u16 gradnum, u16 calcratio, u16 sfcnum)
{
   if (Vdp2Regs->CCCTL & calcenab)
   {
      AddString(outstring, "Color Calculation Enabled\r\n");

      if (Vdp2Regs->CCCTL & 0x8000 && (Vdp2Regs->CCCTL & 0x0700) == gradnum)
      {
         AddString(outstring, "Gradation Calculation Enabled\r\n");
      }
      else if (Vdp2Regs->CCCTL & 0x0400)
      {
         AddString(outstring, "Extended Color Calculation Enabled\r\n");
      }
      else
      {
         AddString(outstring, "Special Color Calculation Mode = %d\r\n", sfcnum);
      }

      AddString(outstring, "Color Calculation Ratio = %d:%d\r\n", 31 - calcratio, 1 + calcratio);
   }

   return outstring;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE char *AddColorOffsetInfo(char *outstring, u16 offsetselectenab)
{
   s32 r, g, b;

   if (Vdp2Regs->CLOFEN & offsetselectenab)
   {
      if (Vdp2Regs->CLOFSL & offsetselectenab)
      {
         r = Vdp2Regs->COBR & 0xFF;
         if (Vdp2Regs->COBR & 0x100)
            r |= 0xFFFFFF00;

         g = Vdp2Regs->COBG & 0xFF;
         if (Vdp2Regs->COBG & 0x100)
            g |= 0xFFFFFF00;

         b = Vdp2Regs->COBB & 0xFF;
         if (Vdp2Regs->COBB & 0x100)
            b |= 0xFFFFFF00;

         AddString(outstring, "Color Offset B Enabled\r\n");
         AddString(outstring, "R = %ld, G = %ld, B = %ld\r\n", r, g, b);
      }
      else
      {
         r = Vdp2Regs->COAR & 0xFF;
         if (Vdp2Regs->COAR & 0x100)
            r |= 0xFFFFFF00;

         g = Vdp2Regs->COAG & 0xFF;
         if (Vdp2Regs->COAG & 0x100)
            g |= 0xFFFFFF00;

         b = Vdp2Regs->COAB & 0xFF;
         if (Vdp2Regs->COAB & 0x100)
            b |= 0xFFFFFF00;

         AddString(outstring, "Color Offset A Enabled\r\n");
         AddString(outstring, "R = %ld, G = %ld, B = %ld\r\n", r, g, b);
      }
   }

   return outstring;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DebugStatsRBG0(char *outstring, int *isenabled)
{
   int patternwh=((Vdp2Regs->CHCTLB & 0x100) >> 8) + 1;
   u8 map[16];

   if (Vdp2Regs->BGON & 0x10)
   {
      // enabled
      int rotatenum=0;
      int coeftbl=0, coefmode=0;

      *isenabled = 1;

      // Which Rotation Parameter is being used
      switch (Vdp2Regs->RPMD & 0x3)
      {
         case 0:
            // Parameter A
            rotatenum = 0;
            coeftbl = Vdp2Regs->KTCTL & 0x1;
            coefmode = (Vdp2Regs->KTCTL >> 2) & 0x3;
            break;
         case 1:
            // Parameter B
            rotatenum = 1;
            coeftbl = Vdp2Regs->KTCTL & 0x100;
            coefmode = (Vdp2Regs->KTCTL >> 10) & 0x3;
            break;
         case 2:
            // Parameter A+B switched via coefficients
            // FIX ME(need to figure out which Parameter is being used)
            break;
         case 3:
            // Parameter A+B switched via rotation parameter window
            // FIX ME(need to figure out which Parameter is being used)
            break;
      }

      AddString(outstring, "Using Parameter %C\r\n", 'A' + rotatenum);

      if (coeftbl)
      {
         AddString(outstring, "Coefficient Table Enabled(Mode %d)\r\n", coefmode);
      }

      // Mosaic
      outstring = AddMosaicString(outstring, 0x10);

      // BPP
      outstring = AddBppString(outstring, (Vdp2Regs->CHCTLB >> 12) & 0x7);

      // Bitmap or Tile mode?
      if (Vdp2Regs->CHCTLB & 0x200)
      {
         // Bitmap mode
         if (rotatenum == 0)
         {
            // Parameter A
            outstring = AddBitmapInfoString(outstring, (Vdp2Regs->CHCTLB & 0x400) >> 10, Vdp2Regs->BMPNB, Vdp2Regs->MPOFR);
         }
         else
         {
            // Parameter B
            outstring = AddBitmapInfoString(outstring, (Vdp2Regs->CHCTLB & 0x400) >> 10, Vdp2Regs->BMPNB, Vdp2Regs->MPOFR >> 4);
         }
      }
      else
      {
         // Tile mode
         int patterndatasize; 
         u16 supplementdata=Vdp2Regs->PNCR & 0x3FF;
         int planew=0, planeh=0;

         if(Vdp2Regs->PNCR & 0x8000)
            patterndatasize = 1;
         else
            patterndatasize = 2;

         AddString(outstring, "Tile(%dH x %dV)\r\n", patternwh, patternwh);

         if (rotatenum == 0)
         {
            // Parameter A
            Vdp2GetPlaneSize((Vdp2Regs->PLSZ & 0x300) >> 8, &planew, &planeh);
         }
         else
         {
            // Parameter B
            Vdp2GetPlaneSize((Vdp2Regs->PLSZ & 0x3000) >> 8, &planew, &planeh);
         }

         AddString(outstring, "Plane Size = %dH x %dV\r\n", planew, planeh);

         // Pattern Name Control stuff
         if (patterndatasize == 2) 
         {
            AddString(outstring, "Pattern Name data size = 2 words\r\n");
         }
         else
         {
            AddString(outstring, "Pattern Name data size = 1 word\r\n");
            AddString(outstring, "Character Number Supplement bit = %d\r\n", (supplementdata >> 16));
            AddString(outstring, "Special Priority bit = %d\r\n", (supplementdata >> 9) & 0x1);
            AddString(outstring, "Special Color Calculation bit = %d\r\n", (supplementdata >> 8) & 0x1);
            AddString(outstring, "Supplementary Palette number = %d\r\n", (supplementdata >> 5) & 0x7);
            AddString(outstring, "Supplementary Color number = %d\r\n", supplementdata & 0x1f);
         }

         if (rotatenum == 0)
         {
            // Parameter A
            map[0] = Vdp2Regs->MPABRA & 0xFF;
            map[1] = Vdp2Regs->MPABRA >> 8;
            map[2] = Vdp2Regs->MPCDRA & 0xFF;
            map[3] = Vdp2Regs->MPCDRA >> 8;
            map[4] = Vdp2Regs->MPEFRA & 0xFF;
            map[5] = Vdp2Regs->MPEFRA >> 8;
            map[6] = Vdp2Regs->MPGHRA & 0xFF;
            map[7] = Vdp2Regs->MPGHRA >> 8;
            map[8] = Vdp2Regs->MPIJRA & 0xFF;
            map[9] = Vdp2Regs->MPIJRA >> 8;
            map[10] = Vdp2Regs->MPKLRA & 0xFF;
            map[11] = Vdp2Regs->MPKLRA >> 8;
            map[12] = Vdp2Regs->MPMNRA & 0xFF;
            map[13] = Vdp2Regs->MPMNRA >> 8;
            map[14] = Vdp2Regs->MPOPRA & 0xFF;
            map[15] = Vdp2Regs->MPOPRA >> 8;
            outstring = AddMapInfo(outstring, patternwh, Vdp2Regs->PNCR, (Vdp2Regs->PLSZ >> 8) & 0x3, (Vdp2Regs->MPOFR & 0x7) << 6, 16, map);
         }
         else
         {
            // Parameter B
            map[0] = Vdp2Regs->MPABRB & 0xFF;
            map[1] = Vdp2Regs->MPABRB >> 8;
            map[2] = Vdp2Regs->MPCDRB & 0xFF;
            map[3] = Vdp2Regs->MPCDRB >> 8;
            map[4] = Vdp2Regs->MPEFRB & 0xFF;
            map[5] = Vdp2Regs->MPEFRB >> 8;
            map[6] = Vdp2Regs->MPGHRB & 0xFF;
            map[7] = Vdp2Regs->MPGHRB >> 8;
            map[8] = Vdp2Regs->MPIJRB & 0xFF;
            map[9] = Vdp2Regs->MPIJRB >> 8;
            map[10] = Vdp2Regs->MPKLRB & 0xFF;
            map[11] = Vdp2Regs->MPKLRB >> 8;
            map[12] = Vdp2Regs->MPMNRB & 0xFF;
            map[13] = Vdp2Regs->MPMNRB >> 8;
            map[14] = Vdp2Regs->MPOPRB & 0xFF;
            map[15] = Vdp2Regs->MPOPRB >> 8;
            outstring = AddMapInfo(outstring, patternwh, Vdp2Regs->PNCR, (Vdp2Regs->PLSZ >> 12) & 0x3, (Vdp2Regs->MPOFR & 0x70) << 2, 16, map);
         }

/*
         // Figure out Cell start address
         switch(patterndatasize)
         {
            case 1:
            {
               tmp = readWord(vram, addr);

               switch(auxMode)
               {
                  case 0:
                     switch(patternwh)
                     {
                        case 1:
                           charAddr = (tmp & 0x3FF) | ((supplementdata & 0x1F) << 10);
                           break;
                        case 2:
                           charAddr = ((tmp & 0x3FF) << 2) |  (supplementdata & 0x3) | ((supplementdata & 0x1C) << 10);
                           break;
                     }
                     break;
                  case 1:
                     switch(patternwh)
                     {
                        case 1:
                           charAddr = (tmp & 0xFFF) | ((supplementdata & 0x1C) << 10);
                           break;
                        case 4:
                           charAddr = ((tmp & 0xFFF) << 2) |  (supplementdata & 0x3) | ((supplementdata & 0x10) << 10);
                           break;
                     }
                     break;
               }
               break;
            }
            case 2:
            {
               unsigned short tmp1 = readWord(vram, addr);
               unsigned short tmp2 = readWord(vram, addr+2);
   
               charAddr = tmp2 & 0x7FFF;
               break;
            }
         }
   
         if (!(readWord(reg, 0x6) & 0x8000))
            charAddr &= 0x3FFF;

         charAddr *= 0x20; // selon Runik
   
         AddString(outstring, "Cell Data Address = %X\r\n", charAddr);
*/
      }

      // Window Control
      outstring = AddWindowInfoString(outstring, Vdp2Regs->WCTLC);

      // Shadow Control here

      // Color Ram Address Offset
      AddString(outstring, "Color Ram Address Offset = %X\r\n", (Vdp2Regs->CRAOFB & 0x7) << 8);
       
      // Special Priority Mode here

      // Color Calculation Control here

      // Special Color Calculation Mode here

      // Priority Number
      AddString(outstring, "Priority = %d\r\n", Vdp2Regs->PRIR & 0x7);
        
      // Color Calculation
      outstring = AddColorCalcInfo(outstring, 0x0010, 0x0001, Vdp2Regs->CCRR & 0x1F, (Vdp2Regs->SFCCMD >> 8) & 0x3);

      // Color Offset
      outstring = AddColorOffsetInfo(outstring, 0x0010);
   }
   else
   {
      // disabled
      *isenabled = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DebugStatsNBG0(char *outstring, int *isenabled)
{
   u16 lineVerticalScrollReg = Vdp2Regs->SCRCTL & 0x3F;
   int isbitmap=Vdp2Regs->CHCTLA & 0x2;
   int patternwh=(Vdp2Regs->CHCTLA & 0x1) + 1;
   u8 map[4];

   if (Vdp2Regs->BGON & 0x1 || Vdp2Regs->BGON & 0x20)
   {
      // enabled
      *isenabled = 1;

      // Generate specific Info for NBG0/RBG1
      if (Vdp2Regs->BGON & 0x20)
      {
         AddString(outstring, "RBG1 mode\r\n");

         if (Vdp2Regs->KTCTL & 0x100)
         {
            AddString(outstring, "Coefficient Table Enabled(Mode %d)\r\n", (Vdp2Regs->KTCTL >> 10) & 0x3);
         }
      }
      else
      {
         AddString(outstring, "NBG0 mode\r\n");
      }

      // Mosaic
      outstring = AddMosaicString(outstring, 0x1);

      // BPP
      outstring = AddBppString(outstring, (Vdp2Regs->CHCTLA & 0x70) >> 4);

      // Bitmap or Tile mode?(RBG1 can only do Tile mode)
      if (isbitmap && !(Vdp2Regs->BGON & 0x20))
      {
         // Bitmap
         outstring = AddBitmapInfoString(outstring, (Vdp2Regs->CHCTLA & 0xC) >> 2, Vdp2Regs->BMPNA, Vdp2Regs->MPOFN);
      }
      else
      {
         // Tile
         int patterndatasize; 
         u16 supplementdata=Vdp2Regs->PNCN0 & 0x3FF;
         int planew=0, planeh=0;

         if(Vdp2Regs->PNCN0 & 0x8000)
            patterndatasize = 1;
         else
            patterndatasize = 2;

         AddString(outstring, "Tile(%dH x %dV)\r\n", patternwh, patternwh);

         Vdp2GetPlaneSize(Vdp2Regs->PLSZ & 0x3, &planew, &planeh);
         AddString(outstring, "Plane Size = %dH x %dV\r\n", planew, planeh);

         // Pattern Name Control stuff
         if (patterndatasize == 2) 
         {
            AddString(outstring, "Pattern Name data size = 2 words\r\n");
         }
         else
         {
            AddString(outstring, "Pattern Name data size = 1 word\r\n");
            AddString(outstring, "Character Number Supplement bit = %d\r\n", (supplementdata >> 16));
            AddString(outstring, "Special Priority bit = %d\r\n", (supplementdata >> 9) & 0x1);
            AddString(outstring, "Special Color Calculation bit = %d\r\n", (supplementdata >> 8) & 0x1);
            AddString(outstring, "Supplementary Palette number = %d\r\n", (supplementdata >> 5) & 0x7);
            AddString(outstring, "Supplementary Color number = %d\r\n", supplementdata & 0x1f);
         }

         map[0] = Vdp2Regs->MPABN0 & 0xFF;
         map[1] = Vdp2Regs->MPABN0 >> 8;
         map[2] = Vdp2Regs->MPCDN0 & 0xFF;
         map[3] = Vdp2Regs->MPCDN0 >> 8;
         outstring = AddMapInfo(outstring, patternwh, Vdp2Regs->PNCN0, Vdp2Regs->PLSZ & 0x3, (Vdp2Regs->MPOFN & 0x7) << 6, 4, map);

/*
         // Figure out Cell start address
         switch(patterndatasize)
         {
            case 1:
            {
               tmp = T1ReadWord(Vdp2Ram, addr);
         
               switch(auxMode)
               {
                  case 0:
                     switch(patternwh)
                     {
                        case 1:
                           charAddr = (tmp & 0x3FF) | ((supplementdata & 0x1F) << 10);
                           break;
                        case 2:
                           charAddr = ((tmp & 0x3FF) << 2) |  (supplementdata & 0x3) | ((supplementdata & 0x1C) << 10);
                           break;
                     }
                     break;
                  case 1:
                     switch(patternwh)
                     {
                        case 1:
                           charAddr = (tmp & 0xFFF) | ((supplementdata & 0x1C) << 10);
                           break;
                        case 4:
                           charAddr = ((tmp & 0xFFF) << 2) |  (supplementdata & 0x3) | ((supplementdata & 0x10) << 10);
                           break;
                     }
                     break;
               }
               break;
            }
            case 2:
            {
               u16 tmp1 = T1ReadWord(Vdp2Ram, addr);
               u16 tmp2 = T1ReadWord(Vdp2Ram, addr+2);

               charAddr = tmp2 & 0x7FFF;
               break;
            }
         }
         if (!(readWord(reg, 0x6) & 0x8000))
            charAddr &= 0x3FFF;

         charAddr *= 0x20; // selon Runik

         AddString(outstring, "Cell Data Address = %X\r\n", charAddr);
*/
      }

      if (Vdp2Regs->BGON & 0x20)
      {
//         unsigned long mapOffsetReg=(readWord(reg, 0x3E) & 0x70) << 2;
                                        
         // RBG1

         // Map Planes A-P here

         // Rotation Parameter Read Control
         if (Vdp2Regs->RPRCTL & 0x400)
         {
            AddString(outstring, "Read KAst Parameter = TRUE\r\n");
         }
         else
         {
            AddString(outstring, "Read KAst Parameter = FALSE\r\n");
         }

         if (Vdp2Regs->RPRCTL & 0x200)
         {
            AddString(outstring, "Read Yst Parameter = TRUE\r\n");
         }
         else
         {
            AddString(outstring, "Read Yst Parameter = FALSE\r\n");
         }
 
         if (Vdp2Regs->RPRCTL & 0x100)
         {
            AddString(outstring, "Read Xst Parameter = TRUE\r\n");
         }
         else
         {
            AddString(outstring, "Read Xst Parameter = FALSE\r\n");
         }

         // Coefficient Table Control

         // Coefficient Table Address Offset

         // Screen Over Pattern Name(should this be moved?)

         // Rotation Parameter Table Address
      }
      else
      {
         // NBG0
/*
         // Screen scroll values
         AddString(outstring, "Screen Scroll x = %f, y = %f\r\n", (float)(reg->getLong(0x70) & 0x7FFFF00) / 65536, (float)(reg->getLong(0x74) & 0x7FFFF00) / 65536);
*/
      
         // Coordinate Increments
         AddString(outstring, "Coordinate Increments x = %f, y = %f\r\n", (float) 65536 / (Vdp2Regs->ZMXN0.all & 0x7FF00), (float) 65536 / (Vdp2Regs->ZMYN0.all & 0x7FF00));

         // Reduction Enable
         switch (Vdp2Regs->ZMCTL & 3)
         {
            case 1:
               AddString(outstring, "Horizontal Reduction = 1/2\r\n");
               break;
            case 2:
            case 3:
               AddString(outstring, "Horizontal Reduction = 1/4\r\n");
               break;
            default: break;
         }

         switch (lineVerticalScrollReg >> 4)
         {
            case 0:
               AddString(outstring, "Line Scroll Interval = Each Line\r\n");
               break;
            case 1:
               AddString(outstring, "Line Scroll Interval = Every 2 Lines\r\n");
               break;
            case 2:
               AddString(outstring, "Line Scroll Interval = Every 4 Lines\r\n");
               break;
            case 3:
               AddString(outstring, "Line Scroll Interval = Every 8 Lines\r\n");
               break;
         }

         if (lineVerticalScrollReg & 0x8)
         {
            AddString(outstring, "Line Zoom enabled\r\n");
         }

         if (lineVerticalScrollReg & 0x4)
         {
            AddString(outstring, "Line Scroll Vertical enabled\r\n");
         }
   
         if (lineVerticalScrollReg & 0x2)
         {
            AddString(outstring, "Line Scroll Horizontal enabled\r\n");
         }

         if (lineVerticalScrollReg & 0x6) 
         {
            AddString(outstring, "Line Scroll Enabled\r\n");
            AddString(outstring, "Line Scroll Table Address = %08X\r\n", (int)(0x05E00000 + ((Vdp2Regs->LSTA0.all & 0x7FFFE) << 1)));
         }

         if (lineVerticalScrollReg & 0x1)
         {      
            AddString(outstring, "Vertical Cell Scroll enabled\r\n");
            AddString(outstring, "Vertical Cell Scroll Table Address = %08X\r\n", (int)(0x05E00000 + ((Vdp2Regs->VCSTA.all & 0x7FFFE) << 1)));
         }
      }

      // Window Control
      outstring = AddWindowInfoString(outstring, Vdp2Regs->WCTLA);

      // Shadow Control here

      // Color Ram Address Offset
      AddString(outstring, "Color Ram Address Offset = %X\r\n", (Vdp2Regs->CRAOFA & 0x7) << 8);
 
      // Special Priority Mode here

      // Color Calculation Control here

      // Special Color Calculation Mode here

      // Priority Number
      AddString(outstring, "Priority = %d\r\n", Vdp2Regs->PRINA & 0x7);

      // Color Calculation 
      outstring = AddColorCalcInfo(outstring, 0x0001, 0x0002, Vdp2Regs->CCRNA & 0x1F, Vdp2Regs->SFCCMD & 0x3);

      // Color Offset
      outstring = AddColorOffsetInfo(outstring, 0x0001);
   }
   else
   {
      // disabled
      *isenabled = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DebugStatsNBG1(char *outstring, int *isenabled)
{
   u16 lineVerticalScrollReg = (Vdp2Regs->SCRCTL >> 8) & 0x3F;
   int isbitmap=Vdp2Regs->CHCTLA & 0x200;
   int patternwh=((Vdp2Regs->CHCTLA & 0x100) >> 8) + 1;
   u8 map[4];

   if (Vdp2Regs->BGON & 0x2)
   {
      // enabled
      *isenabled = 1;

      // Mosaic
      outstring = AddMosaicString(outstring, 0x2);

      // BPP
      outstring = AddBppString(outstring, (Vdp2Regs->CHCTLA & 0x3000) >> 12);

      // Bitmap or Tile mode?     
      if (isbitmap)
      {
         // Bitmap
         outstring = AddBitmapInfoString(outstring, (Vdp2Regs->CHCTLA & 0xC00) >> 10, Vdp2Regs->BMPNA >> 8, Vdp2Regs->MPOFN >> 4);
      }
      else
      {
         int patterndatasize;
         u16 supplementdata=Vdp2Regs->PNCN1 & 0x3FF;
         int planew=0, planeh=0;

         if(Vdp2Regs->PNCN1 & 0x8000)
           patterndatasize = 1;
         else
           patterndatasize = 2;

         // Tile
         AddString(outstring, "Tile(%dH x %dV)\r\n", patternwh, patternwh);

         Vdp2GetPlaneSize((Vdp2Regs->PLSZ & 0xC) >> 2, &planew, &planeh);
         AddString(outstring, "Plane Size = %dH x %dV\r\n", planew, planeh);

         // Pattern Name Control stuff
         if (patterndatasize == 2) 
         {
            AddString(outstring, "Pattern Name data size = 2 words\r\n");
         }
         else
         {
            AddString(outstring, "Pattern Name data size = 1 word\r\n");
            AddString(outstring, "Character Number Supplement bit = %d\r\n", (supplementdata >> 16));
            AddString(outstring, "Special Priority bit = %d\r\n", (supplementdata >> 9) & 0x1);
            AddString(outstring, "Special Color Calculation bit = %d\r\n", (supplementdata >> 8) & 0x1);
            AddString(outstring, "Supplementary Palette number = %d\r\n", (supplementdata >> 5) & 0x7);
            AddString(outstring, "Supplementary Color number = %d\r\n", supplementdata & 0x1f);
         }

         map[0] = Vdp2Regs->MPABN1 & 0xFF;
         map[1] = Vdp2Regs->MPABN1 >> 8;
         map[2] = Vdp2Regs->MPCDN1 & 0xFF;
         map[3] = Vdp2Regs->MPCDN1 >> 8;
         outstring = AddMapInfo(outstring, patternwh, Vdp2Regs->PNCN1, (Vdp2Regs->PLSZ & 0xC) >> 2, (Vdp2Regs->MPOFN & 0x70) << 2, 4, map);

/*
         // Figure out Cell start address
         switch(patterndatasize)
         {
            case 1:
            {
               tmp = readWord(vram, addr);

               switch(auxMode)
               {
                  case 0:
                     switch(patternwh)
                     {
                        case 1:
                           charAddr = (tmp & 0x3FF) | ((supplementdata & 0x1F) << 10);
                           break;
                        case 2:
                           charAddr = ((tmp & 0x3FF) << 2) |  (supplementdata & 0x3) | ((supplementdata & 0x1C) << 10);
                           break;
                     }
                     break;
                  case 1:
                     switch(patternwh)
                     {
                        case 1:
                           charAddr = (tmp & 0xFFF) | ((supplementdata & 0x1C) << 10);
                           break;
                        case 4:
                           charAddr = ((tmp & 0xFFF) << 2) |  (supplementdata & 0x3) | ((supplementdata & 0x10) << 10);
                           break;
                     }
                     break;
               }
               break;
            }
            case 2:
            {
               unsigned short tmp1 = readWord(vram, addr);
               unsigned short tmp2 = readWord(vram, addr+2);

               charAddr = tmp2 & 0x7FFF;
               break;
            }
         }
         if (!(readWord(reg, 0x6) & 0x8000))
            charAddr &= 0x3FFF;

         charAddr *= 0x20; // selon Runik
  
         AddString(outstring, "Cell Data Address = %X\r\n", charAddr);
*/
      }
     
/*
      // Screen scroll values
      AddString(outstring, "Screen Scroll x = %f, y = %f\r\n", (float)(reg->getLong(0x80) & 0x7FFFF00) / 65536, (float)(reg->getLong(0x84) & 0x7FFFF00) / 65536);
*/

      // Coordinate Increments
      AddString(outstring, "Coordinate Increments x = %f, y = %f\r\n", (float) 65536 / (Vdp2Regs->ZMXN1.all & 0x7FF00), (float) 65536 / (Vdp2Regs->ZMXN1.all & 0x7FF00));

      // Reduction Enable
      switch ((Vdp2Regs->ZMCTL >> 8) & 3)
      {
         case 1:
            AddString(outstring, "Horizontal Reduction = 1/2\r\n");
            break;
         case 2:
         case 3:
            AddString(outstring, "Horizontal Reduction = 1/4\r\n");
            break;
         default: break;
      }

      switch (lineVerticalScrollReg >> 4)
      {
         case 0:
            AddString(outstring, "Line Scroll Interval = Each Line\r\n");
            break;
         case 1:
            AddString(outstring, "Line Scroll Interval = Every 2 Lines\r\n");
            break;
         case 2:
            AddString(outstring, "Line Scroll Interval = Every 4 Lines\r\n");
            break;
         case 3:
            AddString(outstring, "Line Scroll Interval = Every 8 Lines\r\n");
            break;
      }

      if (lineVerticalScrollReg & 0x8)
      {
         AddString(outstring, "Line Zoom X enabled\r\n");
      }

      if (lineVerticalScrollReg & 0x4)
      {
         AddString(outstring, "Line Scroll Vertical enabled\r\n");
      }
   
      if (lineVerticalScrollReg & 0x2)
      {
         AddString(outstring, "Line Scroll Horizontal enabled\r\n");
      }

      if (lineVerticalScrollReg & 0x6) 
      {
         AddString(outstring, "Line Scroll Enabled\r\n");
         AddString(outstring, "Line Scroll Table Address = %08X\r\n", (int)(0x05E00000 + ((Vdp2Regs->LSTA1.all & 0x7FFFE) << 1)));
      }

      if (lineVerticalScrollReg & 0x1)
      {
         AddString(outstring, "Vertical Cell Scroll enabled\r\n");
         AddString(outstring, "Vertical Cell Scroll Table Address = %08X\r\n", 0x05E00000 + (int)(0x05E00000 + ((Vdp2Regs->VCSTA.all & 0x7FFFE) << 1)));
      }

      // Window Control
      outstring = AddWindowInfoString(outstring, Vdp2Regs->WCTLA >> 8);

      // Shadow Control here

      // Color Ram Address Offset
      AddString(outstring, "Color Ram Address Offset = %X\r\n", (Vdp2Regs->CRAOFA & 0x70) << 4);

      // Special Priority Mode here

      // Color Calculation Control here

      // Special Color Calculation Mode here

      // Priority Number
      AddString(outstring, "Priority = %d\r\n", (Vdp2Regs->PRINA >> 8) & 0x7);

      // Color Calculation
      outstring = AddColorCalcInfo(outstring, 0x0002, 0x0004, (Vdp2Regs->CCRNA >> 8) & 0x1F, (Vdp2Regs->SFCCMD >> 2) & 0x3);

      // Color Offset
      outstring = AddColorOffsetInfo(outstring, 0x0002);
   }
   else
     // disabled
     *isenabled = 0;
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DebugStatsNBG2(char *outstring, int *isenabled)
{
   u8 map[4];

   if (Vdp2Regs->BGON & 0x4)
   {
      int patterndatasize;
      u16 supplementdata=Vdp2Regs->PNCN2 & 0x3FF;
      int planew=0, planeh=0;
      int patternwh=(Vdp2Regs->CHCTLB & 0x1) + 1;

      // enabled
      *isenabled = 1;

      // Mosaic
      outstring = AddMosaicString(outstring, 0x4);

      // BPP
      outstring = AddBppString(outstring, (Vdp2Regs->CHCTLB & 0x2) >> 1);

      if(Vdp2Regs->PNCN2 & 0x8000)
         patterndatasize = 1;
      else
         patterndatasize = 2;

      AddString(outstring, "Tile(%dH x %dV)\r\n", patternwh, patternwh);

      Vdp2GetPlaneSize((Vdp2Regs->PLSZ & 0x30) >> 4, &planew, &planeh);
      AddString(outstring, "Plane Size = %dH x %dV\r\n", planew, planeh);

      // Pattern Name Control stuff
      if (patterndatasize == 2) 
      {
         AddString(outstring, "Pattern Name data size = 2 words\r\n");
      }
      else
      {
         AddString(outstring, "Pattern Name data size = 1 word\r\n");
         AddString(outstring, "Character Number Supplement bit = %d\r\n", (supplementdata >> 16));
         AddString(outstring, "Special Priority bit = %d\r\n", (supplementdata >> 9) & 0x1);
         AddString(outstring, "Special Color Calculation bit = %d\r\n", (supplementdata >> 8) & 0x1);
         AddString(outstring, "Supplementary Palette number = %d\r\n", (supplementdata >> 5) & 0x7);
         AddString(outstring, "Supplementary Color number = %d\r\n", supplementdata & 0x1f);
      }
     
      map[0] = Vdp2Regs->MPABN2 & 0xFF;
      map[1] = Vdp2Regs->MPABN2 >> 8;
      map[2] = Vdp2Regs->MPCDN2 & 0xFF;
      map[3] = Vdp2Regs->MPCDN2 >> 8;
      outstring = AddMapInfo(outstring, patternwh, Vdp2Regs->PNCN2, (Vdp2Regs->PLSZ >> 4) & 0x3, (Vdp2Regs->MPOFN & 0x700) >> 2, 4, map);
/*
      // Figure out Cell start address
      switch(patterndatasize)
      {
         case 1:
         {
            tmp = readWord(vram, addr);

            switch(auxMode)
            {
               case 0:
                  switch(patternwh)
                  {
                     case 1:
                        charAddr = (tmp & 0x3FF) | ((supplementdata & 0x1F) << 10);
                        break;
                     case 2:
                        charAddr = ((tmp & 0x3FF) << 2) |  (supplementdata & 0x3) | ((supplementdata & 0x1C) << 10);
                        break;
                  }
                  break;
               case 1:
                  switch(patternwh)
                  {
                     case 1:
                        charAddr = (tmp & 0xFFF) | ((supplementdata & 0x1C) << 10);
                        break;
                     case 4:
                        charAddr = ((tmp & 0xFFF) << 2) |  (supplementdata & 0x3) | ((supplementdata & 0x10) << 10);
                        break;
                  }
                  break;
            }
            break;
         }
         case 2:
         {
            unsigned short tmp1 = readWord(vram, addr);
            unsigned short tmp2 = readWord(vram, addr+2);

            charAddr = tmp2 & 0x7FFF;
            break;
         }
      }
      if (!(readWord(reg, 0x6) & 0x8000)) charAddr &= 0x3FFF;

      charAddr *= 0x20; // selon Runik

      AddString(outstring, "Cell Data Address = %X\r\n", charAddr);
*/
      // Screen scroll values
      AddString(outstring, "Screen Scroll x = %d, y = %d\r\n", - ((Vdp2Regs->SCXN2 & 0x7FF) % 512), - ((Vdp2Regs->SCYN2 & 0x7FF) % 512));

      // Window Control
      outstring = AddWindowInfoString(outstring, Vdp2Regs->WCTLB);

      // Shadow Control here

      // Color Ram Address Offset
      AddString(outstring, "Color Ram Address Offset = %X\r\n", Vdp2Regs->CRAOFA & 0x700);

      // Special Priority Mode here

      // Color Calculation Control here

      // Special Color Calculation Mode here

      // Priority Number
      AddString(outstring, "Priority = %d\r\n", Vdp2Regs->PRINB & 0x7);
                       
      // Color Calculation
      outstring = AddColorCalcInfo(outstring, 0x0004, 0x0005, Vdp2Regs->CCRNB & 0x1F, (Vdp2Regs->SFCCMD >> 4) & 0x3);

      // Color Offset
      outstring = AddColorOffsetInfo(outstring, 0x0004);
   }
   else
   {
     // disabled
     *isenabled = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

void Vdp2DebugStatsNBG3(char *outstring, int *isenabled)
{
   u8 map[4];

   if (Vdp2Regs->BGON & 0x8)
   {
      int patterndatasize;
      u16 supplementdata=Vdp2Regs->PNCN3 & 0x3FF;
      int planew=0, planeh=0;
      int patternwh=((Vdp2Regs->CHCTLB & 0x10) >> 4) + 1;

      // enabled
      *isenabled = 1;

      // Mosaic
      outstring = AddMosaicString(outstring, 0x8);

      // BPP
      outstring = AddBppString(outstring, (Vdp2Regs->CHCTLB & 0x20) >> 5);

      if(Vdp2Regs->PNCN3 & 0x8000)
         patterndatasize = 1;
      else
         patterndatasize = 2;

      AddString(outstring, "Tile(%dH x %dV)\r\n", patternwh, patternwh);

      Vdp2GetPlaneSize((Vdp2Regs->PLSZ & 0xC0) >> 6, &planew, &planeh);
      AddString(outstring, "Plane Size = %dH x %dV\r\n", planew, planeh);

      // Pattern Name Control stuff
      if (patterndatasize == 2) 
      {
         AddString(outstring, "Pattern Name data size = 2 words\r\n");
      }
      else
      {
         AddString(outstring, "Pattern Name data size = 1 word\r\n");
         AddString(outstring, "Character Number Supplement bit = %d\r\n", (supplementdata >> 16));
         AddString(outstring, "Special Priority bit = %d\r\n", (supplementdata >> 9) & 0x1);
         AddString(outstring, "Special Color Calculation bit = %d\r\n", (supplementdata >> 8) & 0x1);
         AddString(outstring, "Supplementary Palette number = %d\r\n", (supplementdata >> 5) & 0x7);
         AddString(outstring, "Supplementary Color number = %d\r\n", supplementdata & 0x1f);
      }

      map[0] = Vdp2Regs->MPABN3 & 0xFF;
      map[1] = Vdp2Regs->MPABN3 >> 8;
      map[2] = Vdp2Regs->MPCDN3 & 0xFF;
      map[3] = Vdp2Regs->MPCDN3 >> 8;
      outstring = AddMapInfo(outstring, patternwh, Vdp2Regs->PNCN3, (Vdp2Regs->PLSZ & 0xC0) >> 6, (Vdp2Regs->MPOFN & 0x7000) >> 6, 4, map);

/*
      // Figure out Cell start address
      switch(patterndatasize)
      {
         case 1:
         {
            tmp = readWord(vram, addr);

            switch(auxMode)
            {
               case 0:
                  switch(patternwh)
                  {
                     case 1:
                        charAddr = (tmp & 0x3FF) | ((supplementdata & 0x1F) << 10);
                        break;
                     case 2:
                        charAddr = ((tmp & 0x3FF) << 2) |  (supplementdata & 0x3) | ((supplementdata & 0x1C) << 10);
                        break;
                  }
                  break;
               case 1:
                  switch(patternwh)
                  {
                     case 1:
                        charAddr = (tmp & 0xFFF) | ((supplementdata & 0x1C) << 10);
                        break;
                     case 4:
                        charAddr = ((tmp & 0xFFF) << 2) |  (supplementdata & 0x3) | ((supplementdata & 0x10) << 10);
                        break;
                  }
                  break;
            }
            break;
         }
         case 2:
         {
            unsigned short tmp1 = readWord(vram, addr);
            unsigned short tmp2 = readWord(vram, addr+2);

            charAddr = tmp2 & 0x7FFF;
            break;
         }
      }

      if (!(readWord(reg, 0x6) & 0x8000))
         charAddr &= 0x3FFF;

      charAddr *= 0x20; // selon Runik

      AddString(outstring, "Cell Data Address = %X\r\n", charAddr);
*/
      // Screen scroll values
      AddString(outstring, "Screen Scroll x = %d, y = %d\r\n", - ((Vdp2Regs->SCXN3 & 0x7FF) % 512), - ((Vdp2Regs->SCYN3 & 0x7FF) % 512));

      // Window Control
      outstring = AddWindowInfoString(outstring, Vdp2Regs->WCTLB >> 8);

      // Shadow Control here

      // Color Ram Address Offset
      AddString(outstring, "Color Ram Address Offset = %X\r\n", Vdp2Regs->CRAOFA & 0x7000);

      // Special Priority Mode here

      // Special Color Calculation Mode here

      // Priority Number
      AddString(outstring, "Priority = %d\r\n", (Vdp2Regs->PRINB >> 8) & 0x7);

      // Color Calculation
      outstring = AddColorCalcInfo(outstring, 0x0008, 0x0006, (Vdp2Regs->CCRNB >> 8) & 0x1F, (Vdp2Regs->SFCCMD >> 6) & 0x3);

      // Color Offset
      outstring = AddColorOffsetInfo(outstring, 0x0008);
   }
   else
   {
      // disabled
      *isenabled = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG0(void)
{
   VIDCore->Vdp2ToggleDisplayNBG0();
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG1(void)
{
   VIDCore->Vdp2ToggleDisplayNBG1();
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG2(void)
{
   VIDCore->Vdp2ToggleDisplayNBG2();
}

//////////////////////////////////////////////////////////////////////////////

void ToggleNBG3(void)
{
   VIDCore->Vdp2ToggleDisplayNBG3();
}

//////////////////////////////////////////////////////////////////////////////

void ToggleRBG0(void)
{
   VIDCore->Vdp2ToggleDisplayRBG0();
}

//////////////////////////////////////////////////////////////////////////////

void ToggleFullScreen(void)
{
   if (VIDCore->IsFullscreen())
   {
      VIDCore->Resize(320, 224, 0);
      YuiVideoResize(320, 224, 0);
   }
   else
   {
      VIDCore->Resize(640, 480, 1);
      YuiVideoResize(640, 480, 1);
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

//////////////////////////////////////////////////////////////////////////////

