/*  Copyright 2003 Guillaume Duhamel
    Copyright 2005 Theo Berkau

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

#include "scu.hh"
#include "superh.hh"
#include "saturn_memory.hh"

template<unsigned char V, unsigned char L, unsigned short M>
void Scu::sendInterrupt(void) {
    Memory::setLong(0xA4, Memory::getLong(0xA4)|M);
  if (!(Memory::getWord(0xA2) & M)) {
    ((SuperH *) satmem->getMasterSH())->send(Interrupt(L, V));
#if 0
    cerr << "interrupt send " << (int) V << endl;
#endif
  }
//  else {
//    if (V == 0x47) cerr << "sm interrupt masked " << endl;
//  }
}

unsigned long Scu::getLong(unsigned long addr) {
  unsigned long val=0;
  switch(addr) {
     case 0x80: // DSP Program Control Port
                val = dspProgControlPort.all & 0x00FD00FF;
                break;
     case 0x8C: // DSP Data Ram Data Port
                if (!dspProgControlPort.part.EX) {
                  val = dspMD[dspDataRamPage][dspDataRamReadAddress];
                  dspDataRamReadAddress++;                           
                }
                else val = 0;
                break;
     default: val = Memory::getLong(addr);
  }

  return val;
}

void Scu::setLong(unsigned long addr, unsigned long val) {
	switch(addr) {
                case 0x10: if (val & 0x1) DMA(0);
			   Memory::setLong(addr, val);
			   break;
                case 0x14: if ((val & 0x7) != 7) {
#if DEBUG
                             cerr << "scu\t: DMA mode 0 interrupt start factor not implemented" << endl;
#endif
                           }
			   Memory::setLong(addr, val);
                           break;
                case 0x30: if (val & 0x1) DMA(1);
			   Memory::setLong(addr, val);
			   break;
                case 0x34: if ((val & 0x7) != 7) {
#if DEBUG
                             cerr << "scu\t: DMA mode 1 interrupt start factor not implemented" << endl;
#endif
                           }
			   Memory::setLong(addr, val);
                           break;
                case 0x50: if (val & 0x1) DMA(2);
			   Memory::setLong(addr, val);
			   break;
                case 0x54: if ((val & 0x7) != 7) {
#if DEBUG
                             cerr << "scu\t: DMA mode 2 interrupt start factor not implemented" << endl;
#endif
                           }
			   Memory::setLong(addr, val);
                           break;
                case 0x80: // DSP Program Control Port
                           dspProgControlPort.all = (dspProgControlPort.all & 0x00FC0000) | (val & 0x060380FF);

                           if (dspProgControlPort.part.LE) {
                              // set pc
                              dsppc = (unsigned char)dspProgControlPort.part.P;
//#if DEBUG
//                              fprintf(stderr, "scu\t: DSP set pc = %02X\n", dsppc);
//#endif
                           }

#if DEBUG
                           if (dspProgControlPort.part.EX)
                              fprintf(stderr, "scu\t: DSP executing\n");
#endif

                           break;
                case 0x84: // DSP Program Ram Data Port
//#if DEBUG
//                           fprintf(stderr, "scu\t: wrote %08X to DSP Program ram offset %02X\n", val, dsppc);
//#endif

                           dspProgramRam[dsppc] = val;
                           dsppc++;
                           break;
                case 0x88: // DSP Data Ram Address Port
                           dspDataRamPage = (val >> 6) & 3;
                           dspDataRamReadAddress = val & 0x3F;
                           break;
                case 0x8C: // DSP Data Ram Data Port
//#if DEBUG
//                           fprintf(stderr, "scu\t: wrote %08X to DSP Data Ram Data Port Page %d offset %02X\n", val, dspDataRamPage, dspDataRamReadAddress);
//#endif
                           if (!dspProgControlPort.part.EX) {
                              dspMD[dspDataRamPage][dspDataRamReadAddress] = val;
                              dspDataRamReadAddress++;
                           }
                           break;
                case 0x90:
#if DEBUG
                           cerr << "scu\t: Timer 0 Compare Register set to " << hex << val << endl;
#endif
			   Memory::setLong(addr, val);
                           break;
                case 0xA4: 
                           Memory::setLong(addr, Memory::getLong(addr) & val);
                           break;
		default:
                           Memory::setLong(addr, val);
	}
}

Scu::Scu(SaturnMemory *i) : Memory(0xFF, 0x100) {
 	satmem = i;
        reset();
}

void Scu::reset(void) {
	Memory::setLong(0xA0, 0x0000BFFF);
	Memory::setLong(0xA4, 0);
        timer0 = timer1 = 0;
        dspProgControlPort.all=false;
}

void Scu::DMA(int mode) {
	int i = mode * 0x20;
	unsigned long readAddress = getLong(i);
	unsigned long writeAddress = getLong(i + 0x4);
	unsigned long transferNumber = getLong(i + 0x8);
	unsigned long addValue = getLong(i + 0xC);
	unsigned char readAdd, writeAdd;

	if (addValue & 0x100) readAdd = 4;
	else readAdd = 0;
	switch(addValue & 0x7) {
		case 0x0: writeAdd = 0; break;
		case 0x1: writeAdd = 2; break;
		case 0x2: writeAdd = 4; break;
		case 0x3: writeAdd = 8; break;
		case 0x4: writeAdd = 16; break;
		case 0x5: writeAdd = 32; break;
		case 0x6: writeAdd = 64; break;
		case 0x7: writeAdd = 128; break;
	}
	if (getLong(i + 0x14) & 0x1000000) {
                // Indirect DMA
                unsigned long tempreadAddress;
                unsigned long tempwriteAddress;
                unsigned long temptransferNumber;
                unsigned long test, test2;

                for (;;) {
                   unsigned long counter = 0;

                   temptransferNumber=satmem->getLong(writeAddress);
                   tempwriteAddress=satmem->getLong(writeAddress+4);
                   tempreadAddress=satmem->getLong(writeAddress+8);
                   test = tempwriteAddress & 0x1FFFFFFF;
                   test2 = tempreadAddress & 0x80000000;

                   if (mode > 0) {
                      temptransferNumber &= 0xFFF;

                      if (temptransferNumber == 0) temptransferNumber = 0x1000;
                   }
                   else {
                      if (temptransferNumber == 0) temptransferNumber = 0x100000;
                   }

                   tempreadAddress &= 0x7FFFFFFF;

                   if ((test >= 0x5A00000) && (test < 0x5FF0000)) {
                        while(counter < temptransferNumber) {
                                unsigned long tmp = satmem->getLong(tempreadAddress);
                                satmem->setWord(tempwriteAddress, tmp >> 16);
                                tempwriteAddress += writeAdd;
                                satmem->setWord(tempwriteAddress, tmp & 0xFFFF);
                                tempwriteAddress += writeAdd;
                                tempreadAddress += readAdd;
				counter += 4;
			}
                   }
                   else {
#if DEBUG
                        cerr << "indirect DMA, A Bus, not implemented" << endl;
#endif
                   }


                   if (test2) break;

                   writeAddress+= 0xC;
                }

		switch(mode) {
			case 0: sendLevel0DMAEnd(); break;
			case 1: sendLevel1DMAEnd(); break;
			case 2: sendLevel2DMAEnd(); break;
		}
	}
	else {
                // Direct DMA
		unsigned long counter = 0;
		unsigned long test = writeAddress & 0x1FFFFFFF;

                if (mode > 0) {
                   transferNumber &= 0xFFF;

                   if (transferNumber == 0) transferNumber = 0x1000;
                }
                else {
                   if (transferNumber == 0) transferNumber = 0x100000;
                }

		if ((test >= 0x5A00000) && (test < 0x5FF0000)) {
			while(counter < transferNumber) {
				unsigned long tmp = satmem->getLong(readAddress);
				satmem->setWord(writeAddress, tmp >> 16);
				writeAddress += writeAdd;
				satmem->setWord(writeAddress, tmp & 0xFFFF);
				writeAddress += writeAdd;
				readAddress += readAdd;
				counter += 4;
			}
		}
		else {
#if DEBUG
                        cerr << "direct DMA, A Bus, not tested yet" << endl;
#endif
			while(counter < transferNumber) {
				satmem->setLong(writeAddress, satmem->getLong(readAddress));
				readAddress += readAdd;
				writeAddress += writeAdd;
				counter += 4;
			}
		}
		switch(mode) {
                        case 0: sendLevel0DMAEnd(); break;
                        case 1: sendLevel1DMAEnd(); break;
                        case 2: sendLevel2DMAEnd(); break;
		}
	}
}

void Scu::run(unsigned long timing) {
   // timing needs work

   // is dsp executing?
   if (dspProgControlPort.part.EX) {
      unsigned long instruction;

      instruction = dspProgramRam[dsppc];

      // execute here
      switch (instruction >> 30) {
         case 0x00: // Operation Commands
#if DEBUG
                    fprintf(stderr, "scu\t: Unimplemented DSP opcode %08X at offset %02X\n", instruction, dsppc);
#endif
                    break;
         case 0x02: // Load Immediate Commands
#if DEBUG
                    fprintf(stderr, "scu\t: Unimplemented DSP opcode %08X at offset %02X\n", instruction, dsppc);
#endif
                    break;
         case 0x03: // Other
                    switch((instruction >> 28) & 0x3) {
                       case 0x00: // DMA Commands
                       case 0x01: // Jump Commands
                       case 0x02: // Loop bottom Commands
#if DEBUG
                                  fprintf(stderr, "scu\t: Unimplemented DSP opcode %08X at offset %02X\n", instruction, dsppc);
#endif
                                  break;
                       case 0x03: // End Commands

                                  dspProgControlPort.part.EX = false;

                                  if (instruction & 0x8000000) {
                                     // End with Interrupt(send dsp interrupt here?)
                                     dspProgControlPort.part.E = true;
                                     sendDSPEnd();
                                  }

                                  break;
                       default: break;
                    }
                    break;
         default: 
#if DEBUG
                    fprintf(stderr, "scu\t: Invalid DSP opcode %08X at offset %02X\n", instruction, dsppc);
#endif
                    break;
      }

      dsppc++;
   }
}

char *disd1bussrc(unsigned char num)
{
   switch(num) { 
      case 0x0:
          return "M0";
      case 0x1:
          return "M1";
      case 0x2:
          return "M2";
      case 0x3:
          return "M3";
      case 0x4:
          return "MC0";
      case 0x5:
          return "MC1";
      case 0x6:
          return "MC2";
      case 0x7:
          return "MC3";
      case 0x9:
          return "ALL";
      case 0xA:
          return "ALH";
      default: break;
   }

   return "??";
}

char *disd1busdest(unsigned char num)
{
   switch(num) { 
      case 0x0:
          return "MC0";
      case 0x1:
          return "MC1";
      case 0x2:
          return "MC2";
      case 0x3:
          return "MC3";
      case 0x4:
          return "RX";
      case 0x5:
          return "PL";
      case 0x6:
          return "RA0";
      case 0x7:
          return "WA0";
      case 0xA:
          return "LOP";
      case 0xB:
          return "TOP";
      case 0xC:
          return "CT0";
      case 0xD:
          return "CT1";
      case 0xE:
          return "CT2";
      case 0xF:
          return "CT3";
      default: break;
   }

   return "??";
}

void Scu::DSPDisasm(unsigned char addr, char *outstring){
   unsigned long instruction;

   instruction = dspProgramRam[addr];

   sprintf(outstring, "%08X: \0", addr);
   outstring+=strlen(outstring);

   if (instruction == 0)
   {
      sprintf(outstring, "NOP\0");
      return;
   }

   // Handle ALU commands here

   switch (instruction >> 30) {
      case 0x00: // Operation Commands
                 // Y-bus
                 if ((instruction >> 17) & 0x4)
                 {
                    sprintf(outstring, "MOV [??], Y\0");
                    outstring+=strlen(outstring);
                 }

                 switch ((instruction >> 17) & 0x3)
                 {
                    case 1:
                            sprintf(outstring, "CLR A\0");
                            outstring+=strlen(outstring);
                            break;
                    case 2:
                            sprintf(outstring, "MOV ALU, A\0");
                            outstring+=strlen(outstring);
                            break;
                    case 3:
                            sprintf(outstring, "MOV [??], A\0");
                            outstring+=strlen(outstring);
                            break;
                    default: break;
                 }

                 // D1-bus
                 switch ((instruction >> 12) & 0x3)
                 {
                    case 1:
                            sprintf(outstring, "MOV #$%02X, %s\0", instruction & 0xFF, disd1busdest((instruction >> 8) & 0xF));
                            outstring+=strlen(outstring);                             
                            break;
                    case 3:
                            sprintf(outstring, "MOV %s, %s\0", disd1bussrc(instruction & 0xF), disd1busdest((instruction >> 8) & 0xF));
                            outstring+=strlen(outstring);                             
                            break;
                    default: break;
                 }

                 break;
      case 0x02: // Load Immediate Commands
                 if ((instruction >> 25) & 1)
                 {
                    switch ((instruction >> 19) & 0x3F) {
                         case 0x01:
                                    sprintf(outstring, "MVI #$%05X,??,NZ\0", instruction & 0x7FFFF);
                                    break;
                         case 0x02:
                                    sprintf(outstring, "MVI #$%05X,??,NS\0", instruction & 0x7FFFF);
                                    break;
                         case 0x03:
                                    sprintf(outstring, "MVI #$%05X,??,NZS\0", instruction & 0x7FFFF);
                                    break;
                         case 0x04:
                                    sprintf(outstring, "MVI #$%05X,??,NC\0", instruction & 0x7FFFF);
                                    break;
                         case 0x08:
                                    sprintf(outstring, "MVI #$%05X,??,NT0\0", instruction & 0x7FFFF);
                                    break;
                         case 0x21:
                                    sprintf(outstring, "MVI #$%05X,??,Z\0", instruction & 0x7FFFF);
                                    break;
                         case 0x22:
                                    sprintf(outstring, "MVI #$%05X,??,S\0", instruction & 0x7FFFF);
                                    break;
                         case 0x23:
                                    sprintf(outstring, "MVI #$%05X,??,ZS\0", instruction & 0x7FFFF);
                                    break;
                         case 0x24:
                                    sprintf(outstring, "MVI #$%05X,??,C\0", instruction & 0x7FFFF);
                                    break;
                         case 0x28:
                                    sprintf(outstring, "MVI #$%05X,??,T0\0", instruction & 0x7FFFF);
                                    break;
                         default: break;
                    }
                 }
                 else
                 {
                    sprintf(outstring, "MVI #$%05X,??\0", instruction & 0x7FFFF);
                 }

                 break;
      case 0x03: // Other
                 switch((instruction >> 28) & 0x3) {
                    case 0x00: // DMA Commands
                               sprintf(outstring, "DMA/DMAH ??, ??, ??\0");
                               break;
                    case 0x01: // Jump Commands
                               switch ((instruction >> 19) & 0x7F) {
                                  case 0x00:
                                             sprintf(outstring, "JMP %02X\0", instruction & 0xFF);
                                             break;
                                  case 0x41:
                                             sprintf(outstring, "JMP NZ, %02X\0", instruction & 0xFF);
                                             break;
                                  case 0x42:
                                             sprintf(outstring, "JMP NS, %02X\0", instruction & 0xFF);
                                             break;
                                  case 0x43:
                                             sprintf(outstring, "JMP NZS, %02X\0", instruction & 0xFF);
                                             break;
                                  case 0x44:
                                             sprintf(outstring, "JMP NC, %02X\0", instruction & 0xFF);
                                             break;
                                  case 0x48:
                                             sprintf(outstring, "JMP NT0, %02X\0", instruction & 0xFF);
                                             break;
                                  case 0x61:
                                             sprintf(outstring, "JMP Z, %02X\0", instruction & 0xFF);
                                             break;
                                  case 0x62:
                                             sprintf(outstring, "JMP S, %02X\0", instruction & 0xFF);
                                             break;
                                  case 0x63:
                                             sprintf(outstring, "JMP ZS, %02X\0", instruction & 0xFF);
                                             break;
                                  case 0x64:
                                             sprintf(outstring, "JMP C, %02X\0", instruction & 0xFF);
                                             break;
                                  case 0x68:
                                             sprintf(outstring, "JMP T0, %02X\0", instruction & 0xFF);
                                             break;
                                  default:
                                             sprintf(outstring, "Unknown JMP\0");
                                             break;
                               }
                               break;
                    case 0x02: // Loop bottom Commands
                               sprintf(outstring, "BTM/LPS\0");
                               break;
                    case 0x03: // End Commands
                               if (instruction & 0x8000000)
                                  sprintf(outstring, "ENDI\0");
                               else
                                  sprintf(outstring, "END\0");

                               break;
                    default: break;
                 }
                 break;
      default: 
                 sprintf(outstring, "Invalid opcode\0", instruction, dsppc);
                 break;
   }
}

void Scu::GetDSPRegisters(scudspregs_struct *regs) {
  if (regs != NULL) {
     regs->dspProgControlPort.all = dspProgControlPort.all;
  }
}

void Scu::SetDSPRegisters(scudspregs_struct *regs) {
  if (regs != NULL) {
     dspProgControlPort.all = regs->dspProgControlPort.all;
  }
}

void Scu::sendVBlankIN(void)      { sendInterrupt<0x40, 0xF, 0x0001>(); }
void Scu::sendVBlankOUT(void) {
   sendInterrupt<0x41, 0xE, 0x0002>();
   timer0 = 0;
}
void Scu::sendHBlankIN(void) {
   sendInterrupt<0x42, 0xD, 0x0004>();
   timer0++;
   // if timer0 equals timer 0 compare register, do an interrupt
   if (timer0 == Memory::getLong(0x90)) sendTimer0();
}
void Scu::sendTimer0(void)        { sendInterrupt<0x43, 0xC, 0x0008>(); }
void Scu::sendTimer1(void)        { sendInterrupt<0x44, 0xB, 0x0010>(); }
void Scu::sendDSPEnd(void)        { sendInterrupt<0x45, 0xA, 0x0020>(); }
void Scu::sendSoundRequest(void)  { sendInterrupt<0x46, 0x9, 0x0040>(); }
void Scu::sendSystemManager(void) { sendInterrupt<0x47, 0x8, 0x0080>(); }
void Scu::sendPadInterrupt(void)  { sendInterrupt<0x48, 0x8, 0x0100>(); }
void Scu::sendLevel2DMAEnd(void)  { sendInterrupt<0x49, 0x6, 0x0200>(); }
void Scu::sendLevel1DMAEnd(void)  { sendInterrupt<0x4A, 0x6, 0x0400>(); }
void Scu::sendLevel0DMAEnd(void)  { sendInterrupt<0x4B, 0x5, 0x0800>(); }
void Scu::sendDMAIllegal(void)    { sendInterrupt<0x4C, 0x3, 0x1000>(); }
void Scu::sendDrawEnd(void)       { sendInterrupt<0x4D, 0x2, 0x2000>(); }
void Scu::sendExternalInterrupt00(void) { sendInterrupt<0x50, 0x7, 0x8000>(); }
void Scu::sendExternalInterrupt01(void) { sendInterrupt<0x51, 0x7, 0x8000>(); }
void Scu::sendExternalInterrupt02(void) { sendInterrupt<0x52, 0x7, 0x8000>(); }
void Scu::sendExternalInterrupt03(void) { sendInterrupt<0x53, 0x7, 0x8000>(); }
void Scu::sendExternalInterrupt04(void) { sendInterrupt<0x54, 0x4, 0x8000>(); }
void Scu::sendExternalInterrupt05(void) { sendInterrupt<0x55, 0x4, 0x8000>(); }
void Scu::sendExternalInterrupt06(void) { sendInterrupt<0x56, 0x4, 0x8000>(); }
void Scu::sendExternalInterrupt07(void) { sendInterrupt<0x57, 0x4, 0x8000>(); }
void Scu::sendExternalInterrupt08(void) { sendInterrupt<0x58, 0x1, 0x8000>(); }
void Scu::sendExternalInterrupt09(void) { sendInterrupt<0x59, 0x1, 0x8000>(); }
void Scu::sendExternalInterrupt10(void) { sendInterrupt<0x5A, 0x1, 0x8000>(); }
void Scu::sendExternalInterrupt11(void) { sendInterrupt<0x5B, 0x1, 0x8000>(); }
void Scu::sendExternalInterrupt12(void) { sendInterrupt<0x5C, 0x1, 0x8000>(); }
void Scu::sendExternalInterrupt13(void) { sendInterrupt<0x5D, 0x1, 0x8000>(); }
void Scu::sendExternalInterrupt14(void) { sendInterrupt<0x5E, 0x1, 0x8000>(); }
void Scu::sendExternalInterrupt15(void) { sendInterrupt<0x5F, 0x1, 0x8000>(); }

int Scu::saveState(FILE *fp) {
   int offset;

   offset = stateWriteHeader(fp, "SCU ", 1);

   // Write registers
   fwrite((void *)this->getBuffer(), 0x100, 1, fp);
   fwrite((void *)&timer0, 4, 1, fp);
   fwrite((void *)&timer1, 4, 1, fp);

   // Write DSP area(needs more work)
   fwrite((void *)dspProgramRam, 256, 4, fp);
   fwrite((void *)dspMD, 4 * 64, 4, fp);
   fwrite((void *)&dspProgControlPort.all, 4, 1, fp);
   fwrite((void *)&dsppc, 1, 1, fp);
   fwrite((void *)&dspDataRamPage, 1, 1, fp);
   fwrite((void *)&dspDataRamReadAddress, 1, 1, fp);

   return stateFinishHeader(fp, offset);
}

int Scu::loadState(FILE *fp, int version, int size) {
   // Read registers
   fread((void *)this->getBuffer(), 0x100, 1, fp);
   fread((void *)&timer0, 4, 1, fp);
   fread((void *)&timer1, 4, 1, fp);

   // Read DSP area(needs more work)
   fread((void *)dspProgramRam, 256, 4, fp);
   fread((void *)dspMD, 4 * 64, 4, fp);
   fread((void *)&dspProgControlPort.all, 4, 1, fp);
   fread((void *)&dsppc, 1, 1, fp);
   fread((void *)&dspDataRamPage, 1, 1, fp);
   fread((void *)&dspDataRamReadAddress, 1, 1, fp);

   return size;
}
