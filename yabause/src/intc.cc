/*  Copyright 2003-2004 Guillaume Duhamel
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

#include "intc.hh"
#include "superh.hh"
#include "saturn_memory.hh"

#ifdef _arch_dreamcast
/* No STL for the dreamcast, use my tree implementation instead of a priority queue */
#include "tree.h"
tree_t int_tree;

static int __int_cmp(void *d1, void *d2)	{
	Interrupt *i1 = (Interrupt *) d1;
	Interrupt *i2 = (Interrupt *) d2;
	
	if(i1->level() < i2->level())	{
		return 1;
	}
	if(i1->level() > i2->level())	{
		return -1;
	}
	return 0;
}

void __init_tree()	{
	int_tree = create_tree(__int_cmp);
}

void __add_interrupt(const Interrupt *i)	{
	add_to_tree(&int_tree, (void *)i);
}

Interrupt *__get_highest_int()	{
	return (Interrupt *) get_highest_data(&int_tree);
}

void __del_highest_int()	{
	remove_highest_node(&int_tree);
}
#endif

Onchip::Onchip(bool slave, SaturnMemory *sm, SuperH *sh) : Dummy(0x1FF) {
	memory = sm;
        shparent = sh;
        isslave = slave;

#ifdef _arch_dreamcast
	__init_tree();
#endif
}

void Onchip::reset(void) {
        reg.SMR = 0x00;
        reg.BRR = 0xFF;
        reg.SCR = 0x00;
        reg.TDR = 0xFF;
        reg.SSR = 0x84;
        reg.RDR = 0x00;
        reg.TIER = 0x01;
        reg.FTCSR = 0x00;
        reg.FRC = 0x0000;
        reg.OCRA = 0xFFFF;
        reg.OCRB = 0xFFFF;
        reg.TCR = 0x00;
        reg.TOCR = 0xE0;
        reg.FICR = 0x0000;
        reg.IPRB = 0x0000;
        reg.VCRA = 0x0000;
        reg.VCRB = 0x0000;
        reg.VCRC = 0x0000;
        reg.VCRD = 0x0000;
        reg.DRCR0 = 0x00;
        reg.DRCR1 = 0x00;
        reg.WTCSR = 0x18;
        reg.WTCNT = 0x00;
        reg.RSTCSR = 0x1F;
        reg.SBYCR = 0x60;
        reg.CCR = 0x00;
        reg.ICR = 0x0000;
        reg.IPRA = 0x0000;
        reg.VCRWDT = 0x0000;
        reg.DVCR = 0x00000000;
        reg.BARA = 0x00000000;
        reg.BAMRA = 0x00000000;
        reg.BBRA = 0x0000;
        reg.BARB = 0x00000000;
        reg.BAMRB = 0x00000000;
        reg.BBRB = 0x0000;
        reg.BDRB = 0x00000000;
        reg.BDMRB = 0x00000000;
        reg.BRCR = 0x0000;
        reg.CHCR0 = 0x00000000;
        reg.CHCR1 = 0x00000000;
        reg.DMAOR = 0x00000000;
        reg.BCR1 = (isslave << 15) | 0x03F0;
        reg.BCR2 = 0x00FC;
        reg.WCR = 0xAAFF;
        reg.MCR = 0x0000;
        reg.RTCSR = 0x0000;
        reg.RTCNT = 0x0000;
        reg.RTCOR = 0x0000;

        // Reset FRT internal variables
	timing = 0;
        ccleftover = 0;
        frcdiv = 8;

        // Reset WDT internal variables
        wdtenable = false;
        wdtinterval = true;
        wdtdiv = 2;
        wdtleftover = 0;
}

void Onchip::setByte(unsigned long addr, unsigned char val) {
  switch(addr) {
     case 0x000:
       reg.SMR = val;
       return;
     case 0x001:
       reg.BRR = val;
       return;
     case 0x002:
       reg.SCR = val;
       return;
     case 0x003:
       reg.TDR = val;
       return;
     case 0x004:
       // The only thing you can do to bits 3-7 is clear them
       reg.SSR = (reg.SSR & (val & 0xF8)) | (reg.SSR & 0x6) | (val & 0x1);
       return;
     case 0x010:
       reg.TIER = (val & 0x8E) | 0x1;
       return;
     case 0x011:
       // the only thing you can do to bits 1-7 is clear them
       reg.FTCSR = (reg.FTCSR & (val & 0xFE)) | (val & 0x1);
       return;
     case 0x012:
       reg.FRC = ((val & 0xFF) << 8) | (reg.FRC & 0x00FF);
       return;
     case 0x013:
       reg.FRC = (reg.FRC & 0xFF00) | (val & 0xFF);
       return;
     case 0x014:
       if (!(reg.TOCR & 0x10))
          reg.OCRA = (val << 8) | (reg.OCRA & 0xFF);
       else
          reg.OCRB = (val << 8) | (reg.OCRB & 0xFF);
       return;
     case 0x015:
       if (!(reg.TOCR & 0x10))
          reg.OCRA = (reg.OCRA & 0xFF00) | val;
       else
          reg.OCRB = (reg.OCRB & 0xFF00) | val;
       return;
     case 0x016: {
       reg.TCR = val & 0x83;

       switch (val & 3) {
          case 0:
                  frcdiv = 8;
                  break;
          case 1:
                  frcdiv = 32;
                  break;
          case 2:
                  frcdiv = 128;
                  break;
          case 3:
#if DEBUG
                  cerr << "onchip\t: FRT external input clock not implemented" << endl;
#endif
                  break;
       }

       return;
     }
     case 0x017:
       reg.TOCR = 0xE0 | val;
       return;
     case 0x060:
       reg.IPRB = (val << 8);
       return;
     case 0x061:
       return;
     case 0x062:
       reg.VCRA = ((val & 0x7F) << 8) | (reg.VCRA & 0x00FF);
       return;
     case 0x063:
       reg.VCRA = (reg.VCRA & 0xFF00) | (val & 0x7F);
       return;
     case 0x064:
       reg.VCRB = ((val & 0x7F) << 8) | (reg.VCRB & 0x00FF);
       return;
     case 0x065:
       reg.VCRB = (reg.VCRB & 0xFF00) | (val & 0x7F);
       return;
     case 0x066:
       reg.VCRC = ((val & 0x7F) << 8) | (reg.VCRC & 0x00FF);
       return;
     case 0x067:
       reg.VCRC = (reg.VCRC & 0xFF00) | (val & 0x7F);
       return;
     case 0x068:
       reg.VCRD = (val & 0x7F) << 8;
       return;
     case 0x069:
       return;     
     case 0x071:
       reg.DRCR0 = val & 0x3;
       return;
     case 0x072:
       reg.DRCR1 = val & 0x3;
       return;
     case 0x091:
       reg.SBYCR = val & 0xDF;
       return;
     case 0x092:
       reg.CCR = val & 0xDF;
       return;
     case 0x0E0:
       reg.ICR = ((val & 0x1) << 8) | (reg.ICR & 0xFEFF);
       return;
     case 0x0E1:
       reg.ICR = (reg.ICR & 0xFFFE) | (val & 0x01);
       return;
     case 0x0E2:
       reg.IPRA = (val << 8) | (reg.IPRA & 0x00FF);
       return;
     case 0x0E3:
       reg.IPRA = (reg.IPRA & 0xFF00) | (val & 0xF0);
       return;
     case 0x0E4:
       reg.VCRWDT = ((val & 0x7F) << 8) | (reg.VCRWDT & 0x00FF);
       return;
     case 0x0E5:
       reg.VCRWDT = (reg.VCRWDT & 0xFF00) | (val & 0x7F);
       return;
     default:
//#if DEBUG
       fprintf(stderr, "Unimplemented onchip byte write: %08X\n", addr);
       break;
//#endif
  }
}

unsigned char Onchip::getByte(unsigned long addr) {
  switch(addr) {
     case 0x002:
       return reg.SCR;
     case 0x004:
       return reg.SSR;
     case 0x011:
       return reg.FTCSR;
     case 0x012:
       return (reg.FRC >> 8);
     case 0x013:
       return (reg.FRC & 0xFF);
     case 0x016:
       return reg.TCR;
     case 0x017:       
       return reg.TOCR;
     case 0x080:
       return reg.WTCSR;
     case 0x092:
       return reg.CCR;
     default:
              fprintf(stderr, "Unimplemented onchip byte read: %08X\n", addr);
              break;
  }

  return 0;
}


void Onchip::setWord(unsigned long addr, unsigned short val) {
  switch(addr) {
     case 0x060:
       reg.IPRB = val & 0xFF00;
       return;
     case 0x080:
       // This and RSTCSR have got to be the most wackiest register
       // mappings I've ever seen
       if (val >> 8 == 0xA5) {
         // WTCSR
         switch (val & 7) {
           case 0:
             wdtdiv = 2;
             break;
           case 1:
             wdtdiv = 64;
             break;
           case 2:
             wdtdiv = 128;
             break;
           case 3:
             wdtdiv = 256;
             break;
           case 4:
             wdtdiv = 512;
             break;
           case 5:
             wdtdiv = 1024;
             break;
           case 6:
             wdtdiv = 4096;
             break;
           case 7:
             wdtdiv = 8192;
             break;
         }

         wdtenable = (val & 0x20);
         wdtinterval = (~val & 0x40);

#if DEBUG
         fprintf(stderr, "WTCSR write = %02X. Timer Enabled = %s, Timer Mode = %s, wdtdiv = %d\n", val & 0xFF, wdtenable ? "true" : "false", wdtinterval ? "Interval Timer" : "Watchdog Timer", wdtdiv);
#endif
         reg.WTCSR = (unsigned char)val | 0x18;
       }
       else if (val >> 8 == 0x5A) {
         // WTCNT
         reg.WTCNT = (unsigned char)val;
       }
       return;
     case 0x082:
       if (val == 0xA500) {
         // clear WOVF bit
         reg.RSTCSR &= 0x7F;
       }
       else if (val >> 8 == 0x5A) {
         // RSTE and RSTS bits
         reg.RSTCSR = (reg.RSTCSR & 0x80) | (val & 0x60) | 0x1F;
       }
       return;
     case 0x0E0:
       if (val & 0x8000) {
         //SDL_CreateThread(&Timer::call<Onchip, &Onchip::sendNMI, 100>, this); // random value
         memory->getMasterSH()->timing = 100;
       }
       reg.ICR = val & 0x8101;
       return;
     case 0x0E2:
       reg.IPRA = val & 0xFFF0;
       return;
     case 0x108:
       reg.DVCR = val & 0x3;
       return;
     case 0x1E2:
       reg.BCR1 = (isslave << 15) | (val & 0x1FF7);
       return;
     default:
//#if DEBUG
       fprintf(stderr, "Unimplemented onchip word write: %08X\n", addr);
//#endif
       break;
  }
}

unsigned short Onchip::getWord(unsigned long addr) {
  switch(addr) {
     case 0x060:       
       return reg.IPRB;
     case 0x0E0:       
       return reg.ICR;
     case 0x0E2:       
       return reg.IPRA;
     default:
//#if DEBUG
       fprintf(stderr, "Unimplemented onchip word read: %08X\n", addr);
//#endif
       break;
  }

  return 0;
}

void Onchip::setLong(unsigned long addr, unsigned long val) {
  switch(addr) {
     case 0x100:
       reg.DVSR = val;
       return;
     case 0x104: {
       long divisor = (long) reg.DVSR;

       if (divisor == 0) {
         reg.DVDNTL = val;

         if (val & 0x80000000)
            reg.DVDNTH = 0xFFFFFFFF;
         else
            reg.DVDNTH = 0x00000000;

         reg.DVCR |= 1;

#if DEBUG
         if (reg.DVCR & 0x2)
           fprintf(stderr, "should be triggering DIVU interrupt\n");
#endif            
       }
       else {
         long quotient = ((long) val) / divisor;
         long remainder = ((long) val) % divisor;
         reg.DVDNTL = quotient;
         reg.DVDNTH = remainder;
         reg.DVCR &= ~1;
       }

       return;
     }
     case 0x108:
       reg.DVCR = val & 0x3;
       return;
     case 0x10C:
       reg.VCRDIV = val & 0xFFFF;
       return;
     case 0x110:
       reg.DVDNTH = val;
       return;
     case 0x114: {
       long divisor = (long)reg.DVSR;
       signed long long dividend = reg.DVDNTH;
       dividend <<= 32;
       dividend |= val;

       if (divisor == 0) {
         reg.DVDNTL = val;
         reg.DVCR |= 1;

#if DEBUG
         if (reg.DVCR & 0x2)
           fprintf(stderr, "should be triggering DIVU interrupt\n");
#endif            
       }
       else {
         long long quotient = dividend / divisor;
         long remainder = dividend % divisor;

         // check for overflow
         if (quotient >> 32) {
           reg.DVCR |= 1;
#if DEBUG
           if (reg.DVCR & 0x2)
             fprintf(stderr, "should be triggering DIVU interrupt\n");
#endif            
         }
         else
           reg.DVCR &= ~1;

         reg.DVDNTL = quotient;
         reg.DVDNTH = remainder;
       }

       return;
     }
     case 0x180:
       reg.SAR0 = val;
       return;
     case 0x184:
       reg.DAR0 = val;
       return;
     case 0x188:
       reg.TCR0 = val & 0xFFFFFF;
       return;
     case 0x18C:
       reg.CHCR0 = val & 0xFFFF;

       // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
       // and CHCR's DE bit is set and TE bit is cleared,
       // do a dma transfer
       if ((reg.DMAOR & 7) == 1 && (val & 0x3) == 1)
          runDMA();
       return;
     case 0x190:
       reg.SAR1 = val;
       return;
     case 0x194:
       reg.DAR1 = val;
       return;
     case 0x198:
       reg.TCR1 = val & 0xFFFFFF;
       return;
     case 0x19C:
       reg.CHCR1 = val & 0xFFFF;

       // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
       // and CHCR's DE bit is set and TE bit is cleared,
       // do a dma transfer
       if ((reg.DMAOR & 7) == 1 && (val & 0x3) == 1)
          runDMA();
       return;
     case 0x1A0:
       reg.VCRDMA0 = val & 0xFFFF;
       return;
     case 0x1A8:
       reg.VCRDMA1 = val & 0xFFFF;
       return;
     case 0x1B0:
       reg.DMAOR = val & 0xF;
   
       // If the DMAOR DME bit is set and AE and NMIF bits are cleared,
       // and CHCR's DE bit is set and TE bit is cleared,
       // do a dma transfer
       if ((val & 7) == 1)
          runDMA();
       return;
     case 0x1B8:
       fprintf(stderr, "Unimplemented onchip long write: %08X(%08X)\n", addr, val);
       return;
     case 0x1E0:
       reg.BCR1 &= (isslave << 15) | (val & 0x1FF7);
       return;
     case 0x1E4:
       reg.BCR2 = val & 0xFC;
       return;
     case 0x1E8:
       reg.WCR = val;
       return;
     case 0x1EC:
       reg.MCR = val & 0xFC;
       return;
     case 0x1F0:
       reg.RTCSR = val & 0xF8; // fix me
       return;
     case 0x1F8:
       reg.RTCOR = val & 0xFF;
       return;
     default:
//#if DEBUG
       fprintf(stderr, "Unimplemented onchip long write: %08X\n", addr);
//#endif
       break;
  }
}

unsigned long Onchip::getLong(unsigned long addr) {
  switch(addr) {
     case 0x100:
       return reg.DVSR;
     case 0x104: // DVDNT
       return reg.DVDNTL;
     case 0x108:
       return reg.DVCR;
     case 0x10C:
       return reg.VCRDIV;
     case 0x110:
       return reg.DVDNTH;
     case 0x114:
       return reg.DVDNTL;
     case 0x118: // DVDNTH mirror
       return reg.DVDNTH;
     case 0x11C: // DVDNTL mirror
       return reg.DVDNTL;
     case 0x120: // DVSR Mirror
       return reg.DVSR;
     case 0x124: // DVDNT Mirror
       return reg.DVDNTL;
     case 0x128: // DVCR Mirror
       return reg.DVCR;
     case 0x12C: // VCRDIV Mirror
       return reg.VCRDIV;
     case 0x130: // DVDNTH Mirror
       return reg.DVDNTH;
     case 0x134: // DVDNTL Mirror
       return reg.DVDNTL;
     case 0x138: // DVDNTH mirror
       return reg.DVDNTH;
     case 0x13C: // DVDNTL mirror
       return reg.DVDNTL;
     case 0x18C:
       return reg.CHCR0;
     case 0x19C:
       return reg.CHCR1;
     case 0x1B0:
       return reg.DMAOR;
     case 0x1E0:
       return reg.BCR1;
     default:
//#if DEBUG
       fprintf(stderr, "Unimplemented onchip long read: %08X\n", addr);
//#endif
       break;
  }

  return 0;
}

inline void Onchip::DMATransfer(unsigned long *CHCR, unsigned long *SAR,
                                unsigned long *DAR, unsigned long *TCR,
                                unsigned long *VCRDMA)
{
   int size;

   if (!(*CHCR & 0x2)) { // TE is not set
      int srcInc;
      switch(*CHCR & 0x3000) {
         case 0x0000: srcInc = 0; break;
         case 0x1000: srcInc = 1; break;
         case 0x2000: srcInc = -1; break;
         default: srcInc = 0; break;
      }

      int destInc;
      switch(*CHCR & 0xC000) {
         case 0x0000: destInc = 0; break;
         case 0x4000: destInc = 1; break;
         case 0x8000: destInc = -1; break;
         default: destInc = 0; break;
      }

      switch (size = ((*CHCR & 0x0C00) >> 10)) {
         case 0:
            for (unsigned long i = 0; i < *TCR; i++) {
               memory->setByte(*DAR, memory->getByte(*SAR));
               *SAR += srcInc;
               *DAR += destInc;
            }

            *TCR = 0;
            break;
         case 1:
            destInc *= 2;
            srcInc *= 2;

            for (unsigned long i = 0; i < *TCR; i++) {
               memory->setWord(*DAR, memory->getWord(*SAR));
               *SAR += srcInc;
               *DAR += destInc;
            }

            *TCR = 0;
            break;
         case 2:
            destInc *= 4;
            srcInc *= 4;

            for (unsigned long i = 0; i < *TCR; i++) {
               memory->setLong(*DAR, memory->getLong(*SAR));
               *DAR += destInc;
               *SAR += srcInc;
            }

            *TCR = 0;
            break;
         case 3:
            destInc *= 4;
            srcInc *= 4;

            for (unsigned long i = 0; i < *TCR; i++) {
               for(int i2 = 0; i2 < 4; i2++) {
                  memory->setLong(*DAR, memory->getLong(*SAR));
                  *DAR += destInc;
                  *SAR += srcInc;
               }
            }

            *TCR = 0;
            break;
      }
   }

   if (*CHCR & 0x4)
   {
#if DEBUG
      cerr << "FIXME should launch an interrupt\n";
#endif
        shparent->send(Interrupt((reg.IPRA & 0xF00) >> 8, *VCRDMA));
   }                                                                    

   // Set Transfer End bit
   *CHCR |= 0x2;
}

void Onchip::runDMA(void) {
   // If AE and NMIF bits are set, we can't continue
   if (reg.DMAOR & 0x6)
      return;

   if ((reg.CHCR0 & 0x1) && (reg.CHCR1 & 0x1)) { // both channel wants DMA
      if (reg.DMAOR & 0x8) { // round robin priority
#if DEBUG
         cerr << "dma\t: FIXME: two channel dma - round robin priority not properly implemented\n";
#endif
         DMATransfer(&reg.CHCR0, &reg.SAR0, &reg.DAR0, &reg.TCR0, &reg.VCRDMA0);
         DMATransfer(&reg.CHCR1, &reg.SAR1, &reg.DAR1, &reg.TCR1, &reg.VCRDMA1);
      }
      else { // channel 0 > channel 1 priority
         DMATransfer(&reg.CHCR0, &reg.SAR0, &reg.DAR0, &reg.TCR0, &reg.VCRDMA0);
         DMATransfer(&reg.CHCR1, &reg.SAR1, &reg.DAR1, &reg.TCR1, &reg.VCRDMA1);
      }
   }
   else { // only one channel wants DMA
      if (reg.CHCR0 & 0x1) { // DMA for channel 0
         DMATransfer(&reg.CHCR0, &reg.SAR0, &reg.DAR0, &reg.TCR0, &reg.VCRDMA0);
         return;
      }
      if (reg.CHCR1 & 0x1) { // DMA for channel 1
         DMATransfer(&reg.CHCR1, &reg.SAR1, &reg.DAR1, &reg.TCR1, &reg.VCRDMA1);
         return;
      }
   }
}

void Onchip::runFRT(unsigned long cc) {
   unsigned long frctemp;
   unsigned long frcold;

   frcold = frctemp = (unsigned long)reg.FRC;

   // Increment FRC
   frctemp += ((cc + ccleftover) / frcdiv);
   ccleftover = (cc + ccleftover) % frcdiv;

   // Check to see if there is or was a Output Compare A match
   if (frctemp >= ocra && frcold < ocra)
   {
      char ftcsrreg=reg.FTCSR;

      // Do we need to trigger an interrupt?
      if (reg.TIER & 0x8)  {
         shparent->send(Interrupt((reg.IPRB & 0xF00) >> 8, reg.VCRC & 0x7F));
      }

      // Do we need to clear the FRC?
      if (ftcsrreg & 0x1) {
         frctemp = 0;
         ccleftover = 0;
      }

      // Set OCFA flag
      reg.FTCSR = ftcsrreg | 0x8;
   }

   // Check to see if there is or was a Output Compare B match
   if (frctemp >= ocrb && frcold < ocrb)
   {
      // Do we need to trigger an interrupt?
      if (reg.TIER & 0x4)  {
         shparent->send(Interrupt((reg.IPRB & 0xF00) >> 8, reg.VCRC & 0x7F)); 
      }

      // Set OCFB flag
      reg.FTCSR |= 0x4;
   }

   // If FRC overflows, set overflow flag
   if (frctemp > 0xFFFF) {
      // Do we need to trigger an interrupt?
      if (reg.TIER & 0x2)  {
         shparent->send(Interrupt((reg.IPRB & 0xF00) >> 8, (reg.VCRD >> 8) & 0x7F)); 
      }

      reg.FTCSR |= 0x2;
   }

   // Write new FRC value
   reg.FRC = (unsigned short)frctemp;
}

Interrupt::Interrupt(unsigned char l, unsigned char v) {
  _level = l;
  _vect = v;
}

unsigned char Interrupt::level(void) const {
  return _level;
}

unsigned char Interrupt::vector(void) const {
  return _vect;
}

void Onchip::inputCaptureSignal(void) {
   // Set Input Capture Flag
   reg.FTCSR |= 0x80;

   // Copy FRC registers to Input Capture Registers
   reg.FICR = reg.FRC;

   // Generate an Interrupt?
   if (reg.TIER & 0x80)  {
        shparent->send(Interrupt((reg.IPRB & 0xF00) >> 8, reg.VCRC & 0x7F));
   }
}

void Onchip::runWDT(unsigned long cc) {
   unsigned long wdttemp;

   if (!wdtenable || reg.WTCSR & 0x80 || reg.RSTCSR & 0x80) return;

   wdttemp = (unsigned long)reg.WTCNT;
   wdttemp += ((cc + wdtleftover) / wdtdiv);
   wdtleftover = (cc + wdtleftover) % wdtdiv;

   // Are we overflowing?
   if (wdttemp > 0xFF) {
      // Obviously depending on whether or not we're in Watchdog or Interval
      // Modes, they'll handle an overflow differently.

      if (wdtinterval) {
         // Interval Timer Mode

         // Set OVF flag
         reg.WTCSR |= 0x80;

         // Trigger interrupt
         shparent->send(Interrupt((reg.IPRA >> 4) & 0xF, (reg.VCRWDT >> 8) & 0x7F));
      }
      else {
         // Watchdog Timer Mode(untested)
#if DEBUG
         cerr << "onchip\t: watchdog timer(WDT mode) overflow not implemented" << endl;
#endif
      }
   }

   // Write new WTCNT value
   reg.WTCNT = (unsigned char)wdttemp;
}


InputCaptureSignal::InputCaptureSignal(SuperH *icsh) : Memory(0, 4) {
   onchip = icsh->onchip;
}

void InputCaptureSignal::setByte(unsigned long addr, unsigned char val) {
   onchip->inputCaptureSignal();
}

void InputCaptureSignal::setWord(unsigned long addr, unsigned short val) {
   onchip->inputCaptureSignal();
}

void InputCaptureSignal::setLong(unsigned long addr, unsigned long val) {
   onchip->inputCaptureSignal();
}

