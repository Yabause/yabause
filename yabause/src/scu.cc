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
                val = dsp.ProgControlPort.all & 0x00FD00FF;
                break;
     case 0x8C: // DSP Data Ram Data Port
                if (!dsp.ProgControlPort.part.EX) {
                  val = dsp.MD[dsp.DataRamPage][dsp.DataRamReadAddress];
                  dsp.DataRamReadAddress++;                           
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
                           dsp.ProgControlPort.all = (dsp.ProgControlPort.all & 0x00FC0000) | (val & 0x060380FF);

                           if (dsp.ProgControlPort.part.LE) {
                              // set pc
                              dsp.PC = (unsigned char)dsp.ProgControlPort.part.P;
#if DEBUG
                              fprintf(stderr, "scu\t: DSP set pc = %02X\n", dsp.PC);
#endif
                           }

#if DEBUG
                           if (dsp.ProgControlPort.part.EX)
                              fprintf(stderr, "scu\t: DSP executing: PC = %02X\n", dsp.PC);
#endif

                           break;
                case 0x84: // DSP Program Ram Data Port
//#if DEBUG
//                           fprintf(stderr, "scu\t: wrote %08X to DSP Program ram offset %02X\n", val, dsp.PC);
//#endif

                           dsp.ProgramRam[dsp.PC] = val;
                           dsp.PC++;
                           break;
                case 0x88: // DSP Data Ram Address Port
                           dsp.DataRamPage = (val >> 6) & 3;
                           dsp.DataRamReadAddress = val & 0x3F;
                           break;
                case 0x8C: // DSP Data Ram Data Port
//#if DEBUG
//                              fprintf(stderr, "scu\t: wrote %08X to DSP Data Ram Data Port Page %d offset %02X\n", val, dsp.DataRamPage, dsp.DataRamReadAddress);
//#endif
                           if (!dsp.ProgControlPort.part.EX) {
                              dsp.MD[dsp.DataRamPage][dsp.DataRamReadAddress] = val;
                              dsp.DataRamReadAddress++;
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

  for (int i = 0; i < MAX_BREAKPOINTS; i++)
     codebreakpoint[i].addr = 0xFFFFFFFF;
  numcodebreakpoints = 0;
  BreakpointCallBack=NULL;
  inbreakpoint=false;

  reset();
}

void Scu::reset(void) {
	Memory::setLong(0xA0, 0x0000BFFF);
	Memory::setLong(0xA4, 0);
        timer0 = timer1 = 0;
        dsp.ProgControlPort.all=0;
        dsp.jmpaddr = 0xFFFFFFFF;
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

unsigned long Scu::readgensrc(unsigned char num)
{
   unsigned long val;
   switch(num) {
      case 0x0: // M0
         return dsp.MD[0][dsp.CT[0]];
      case 0x1: // M1
         return dsp.MD[1][dsp.CT[1]];
      case 0x2: // M2
         return dsp.MD[2][dsp.CT[2]];
      case 0x3: // M3
         return dsp.MD[3][dsp.CT[3]];
      case 0x4: // MC0
         val = dsp.MD[0][dsp.CT[0]];
         dsp.CT[0]++;
         return val;
      case 0x5: // MC1
         val = dsp.MD[1][dsp.CT[1]];
         dsp.CT[1]++;
         return val;
      case 0x6: // MC2
         val = dsp.MD[2][dsp.CT[2]];
         dsp.CT[2]++;
         return val;
      case 0x7: // MC3
         val = dsp.MD[3][dsp.CT[3]];
         dsp.CT[3]++;
         return val;
      case 0x9: // ALL
         return dsp.ALU.part.L;
      case 0xA: // ALH
         return dsp.ALU.part.H;
      default: break;
   }

   return 0;
}

void Scu::writed1busdest(unsigned char num, unsigned long val)
{
   switch(num) { 
      case 0x0:
          dsp.MD[0][dsp.CT[0]] = val;
          dsp.CT[0]++;
          return;
      case 0x1:
          dsp.MD[1][dsp.CT[1]] = val;
          dsp.CT[1]++;
          return;
      case 0x2:
          dsp.MD[2][dsp.CT[2]] = val;
          dsp.CT[2]++;
          return;
      case 0x3:
          dsp.MD[3][dsp.CT[3]] = val;
          dsp.CT[3]++;
          return;
      case 0x4:
          dsp.RX = val;
          return;
      case 0x5:
          dsp.P.all = (signed)val;
          return;
      case 0x6:
          dsp.RA0 = val;
          return;
      case 0x7:
          dsp.WA0 = val;
          return;
      case 0xA:
          dsp.LOP = val;
          return;
      case 0xB:
          dsp.TOP = val;
          return;
      case 0xC:
          dsp.CT[0] = val;
          return;
      case 0xD:
          dsp.CT[1] = val;
          return;
      case 0xE:
          dsp.CT[2] = val;
          return;
      case 0xF:
          dsp.CT[3] = val;
          return;
      default: break;
   }
}

void Scu::writeloadimdest(unsigned char num, unsigned long val)
{
   switch(num) { 
      case 0x0: // MC0
          dsp.MD[0][dsp.CT[0]] = val;
          dsp.CT[0]++;
          return;
      case 0x1: // MC1
          dsp.MD[1][dsp.CT[1]] = val;
          dsp.CT[1]++;
          return;
      case 0x2: // MC2
          dsp.MD[2][dsp.CT[2]] = val;
          dsp.CT[2]++;
          return;
      case 0x3: // MC3
          dsp.MD[3][dsp.CT[3]] = val;
          dsp.CT[3]++;
          return;
      case 0x4: // RX
          dsp.RX = val;
          return;
      case 0x5: // PL
          dsp.P.all = (signed)val;
          return;
      case 0x6: // RA0
          dsp.RA0 = val;
          return;
      case 0x7: // WA0
          dsp.WA0 = val;
          return;
      case 0xA: // LOP
          dsp.LOP = val;
          return;
      case 0xC: // TOP
          dsp.TOP = val;
          return;
      default: break;
   }
}

unsigned long Scu::readdmasrc(unsigned char num, unsigned char add)
{
   unsigned long val;

   switch(num) {
      case 0x0: // M0
         val = dsp.MD[0][dsp.CT[0]];
         dsp.CT[0]+=add;
         return val;
      case 0x1: // M1
         val = dsp.MD[1][dsp.CT[1]];
         dsp.CT[1]+=add;
         return val;
      case 0x2: // M2
         val = dsp.MD[2][dsp.CT[2]];
         dsp.CT[2]+=add;
         return val;
      case 0x3: // M3
         val = dsp.MD[3][dsp.CT[3]];
         dsp.CT[3]+=add;
         return val;
      default: break;
   }

   return 0;
}

void Scu::writedmadest(unsigned char num, unsigned long val, unsigned char add)
{
   switch(num) { 
      case 0x0: // M0
          dsp.MD[0][dsp.CT[0]] = val;
          dsp.CT[0]+=add;
          return;
      case 0x1: // M1
          dsp.MD[1][dsp.CT[1]] = val;
          dsp.CT[1]+=add;
          return;
      case 0x2: // M2
          dsp.MD[2][dsp.CT[2]] = val;
          dsp.CT[2]+=add;
          return;
      case 0x3: // M3
          dsp.MD[3][dsp.CT[3]] = val;
          dsp.CT[3]+=add;
          return;
      case 0x4: // Program Ram
          fprintf(stderr, "scu\t: DMA Program writes not implemented\n");
//          dsp.ProgramRam[?] = val;
//          ?? += add;
          return;
      default: break;
   }
}

void Scu::run(unsigned long timing) {
   // timing needs work

   // is dsp executing?
   if (dsp.ProgControlPort.part.EX) {
      unsigned long instruction;

      // Make sure it isn't one of our breakpoints
      for (int i=0; i < numcodebreakpoints; i++) {
         if ((dsp.PC == codebreakpoint[i].addr) && inbreakpoint == false) {
            inbreakpoint = true;
            if (BreakpointCallBack) BreakpointCallBack(codebreakpoint[i].addr);
              inbreakpoint = false;
         }
      }

      instruction = dsp.ProgramRam[dsp.PC];

      // ALU commands
      switch (instruction >> 26)
      {
         case 0x0: // NOP
                   break;
         case 0x1: // AND
                   dsp.ALU.part.L = dsp.AC.part.L & dsp.P.part.L;

                   if (dsp.ALU.part.L == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;

                   if ((signed)dsp.ALU.part.L < 0)
                      dsp.ProgControlPort.part.S = 1;
                   else
                      dsp.ProgControlPort.part.S = 0;

                   dsp.ProgControlPort.part.C = 0;
                   break;
         case 0x2: // OR
                   dsp.ALU.part.L = dsp.AC.part.L | dsp.P.part.L;

                   if (dsp.ALU.part.L == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;

                   if ((signed)dsp.ALU.part.L < 0)
                      dsp.ProgControlPort.part.S = 1;
                   else
                      dsp.ProgControlPort.part.S = 0;

                   dsp.ProgControlPort.part.C = 0;
                   break;
         case 0x3: // XOR
                   dsp.ALU.part.L = dsp.AC.part.L ^ dsp.P.part.L;

                   if (dsp.ALU.part.L == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;

                   if ((signed)dsp.ALU.part.L < 0)
                      dsp.ProgControlPort.part.S = 1;
                   else
                      dsp.ProgControlPort.part.S = 0;

                   dsp.ProgControlPort.part.C = 0;
                   break;
         case 0x4: // ADD
                   dsp.ALU.part.L = (unsigned)((signed)dsp.AC.part.L + (signed)dsp.P.part.L);

                   if (dsp.ALU.part.L == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;

                   if ((signed)dsp.ALU.part.L < 0)
                      dsp.ProgControlPort.part.S = 1;
                   else
                      dsp.ProgControlPort.part.S = 0;

//                   if (dsp.ALU.part.L ??) // set carry flag
//                      dsp.ProgControlPort.part.C = 1;
//                   else
//                      dsp.ProgControlPort.part.C = 0;

//                   if (dsp.ALU.part.L ??) // set overflow flag
//                      dsp.ProgControlPort.part.V = 1;
//                   else
//                      dsp.ProgControlPort.part.V = 0;

                   break;
         case 0x5: // SUB
                   dsp.ALU.part.L = (unsigned)((signed)dsp.AC.part.L - (signed)dsp.P.part.L);

                   if (dsp.ALU.part.L == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;

                   if ((signed)dsp.ALU.part.L < 0)
                      dsp.ProgControlPort.part.S = 1;
                   else
                      dsp.ProgControlPort.part.S = 0;

//                   if (dsp.ALU.part.L ??) // set carry flag
//                      dsp.ProgControlPort.part.C = 1;
//                   else
//                      dsp.ProgControlPort.part.C = 0;

//                   if (dsp.ALU.part.L ??) // set overflow flag
//                      dsp.ProgControlPort.part.V = 1;
//                   else
//                      dsp.ProgControlPort.part.V = 0;
                   break;
         case 0x6: // AD2
                   dsp.ALU.all = (signed)dsp.AC.all + (signed)dsp.P.all;
                   
                   if (dsp.ALU.all == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;

                   if ((signed)dsp.ALU.all < 0)
                      dsp.ProgControlPort.part.S = 1;
                   else
                      dsp.ProgControlPort.part.S = 0;

                   if (dsp.ALU.part.unused != 0)
                      dsp.ProgControlPort.part.V = 1;
                   else
                      dsp.ProgControlPort.part.V = 0;

                   // need carry test
                   break;
         case 0x8: // SR
//                   fprintf(stderr, "scu\t: SR instruction not implemented\n");
                   dsp.ProgControlPort.part.C = dsp.AC.part.L & 0x1;

                   dsp.ALU.part.L = (dsp.AC.part.L & 0x80000000) | (dsp.AC.part.L >> 1);

                   if (dsp.ALU.part.L == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;
                   dsp.ProgControlPort.part.S = dsp.ALU.part.L >> 31;

//                   fprintf(stderr, "scu\t: SR: ACL = %08X ALL = %08X. C = %d, Z = %d, S = %d\n", dsp.AC.part.L, dsp.ALU.part.L, dsp.ProgControlPort.part.C, dsp.ProgControlPort.part.Z, dsp.ProgControlPort.part.S);
                   break;
         case 0x9: // RR
                   dsp.ProgControlPort.part.C = dsp.AC.part.L & 0x1;

                   dsp.ALU.part.L = (dsp.ProgControlPort.part.C << 31) | (dsp.AC.part.L >> 1);

                   if (dsp.ALU.part.L == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;
                   dsp.ProgControlPort.part.S = dsp.ProgControlPort.part.C;
                   break;
         case 0xA: // SL
                   dsp.ProgControlPort.part.C = dsp.AC.part.L >> 31;

                   dsp.ALU.part.L = (dsp.AC.part.L << 1);

                   if (dsp.ALU.part.L == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;
                   dsp.ProgControlPort.part.S = dsp.ALU.part.L >> 31;
                   break;
         case 0xB: // RL
//                   fprintf(stderr, "scu\t: RL instruction not implemented\n");
                   dsp.ProgControlPort.part.C = dsp.AC.part.L >> 31;

                   dsp.ALU.part.L = (dsp.AC.part.L << 1) | dsp.ProgControlPort.part.C;

                   if (dsp.ALU.part.L == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;
                   dsp.ProgControlPort.part.S = dsp.ALU.part.L >> 31;

//                   fprintf(stderr, "scu\t: RL: ACL = %08X ALL = %08X. C = %d, Z = %d, S = %d\n", dsp.AC.part.L, dsp.ALU.part.L, dsp.ProgControlPort.part.C, dsp.ProgControlPort.part.Z, dsp.ProgControlPort.part.S);
                   break;
         case 0xF: // RL8
                   dsp.ALU.part.L = (dsp.AC.part.L << 8) | ((dsp.AC.part.L >> 24) & 0xFF);
                   dsp.ProgControlPort.part.C = dsp.ALU.part.L & 0x1;

                   if (dsp.ALU.part.L == 0)
                      dsp.ProgControlPort.part.Z = 1;
                   else
                      dsp.ProgControlPort.part.Z = 0;
                   dsp.ProgControlPort.part.S = dsp.ALU.part.L >> 31;
                   break;
         default: break;
      }

      switch (instruction >> 30) {
         case 0x00: // Operation Commands

                    // X-bus
                    if ((instruction >> 23) & 0x4)
                    {
                       // MOV [s], X
                       dsp.RX = readgensrc((instruction >> 20) & 0x7);
                    }
                    switch ((instruction >> 23) & 0x3)
                    {
                       case 2: // MOV MUL, P
                               dsp.P.all = dsp.MUL.all;
                               break;
                       case 3: // MOV [s], P
                               dsp.P.all = readgensrc((instruction >> 20) & 0x7);
                               break;
                       default: break;
                    }

                    // Y-bus
                    if ((instruction >> 17) & 0x4) 
                    {
                       // MOV [s], Y
                       dsp.RY = readgensrc((instruction >> 14) & 0x7);
                    }
                    switch ((instruction >> 17) & 0x3)
                    {
                       case 1: // CLR A
                               dsp.AC.all = 0;
                               break;
                       case 2: // MOV ALU,A
                               dsp.AC.all = dsp.ALU.all;
                               break;
                       case 3: // MOV [s],A
                               dsp.AC.all = (signed)readgensrc((instruction >> 14) & 0x7);                               
                               break;
                       default: break;
                    }
   
                    // D1-bus
                    switch ((instruction >> 12) & 0x3)
                    {
                       case 1: // MOV SImm,[d]
                               writed1busdest((instruction >> 8) & 0xF, (unsigned long)(signed char)(instruction & 0xFF));
                               break;
                       case 3: // MOV [s],[d]
                               writed1busdest((instruction >> 8) & 0xF, readgensrc(instruction & 0xF));
                               break;
                       default: break;
                    }

                    break;
         case 0x02: // Load Immediate Commands
                    if ((instruction >> 25) & 1)
                    {
                       switch ((instruction >> 19) & 0x3F) {
                            case 0x01: // MVI Imm,[d]NZ
                                       if (!dsp.ProgControlPort.part.Z)
                                          writeloadimdest((instruction >> 26) & 0xF, (instruction & 0x7FFFF) | ((instruction & 0x40000) ? 0xFFF80000 : 0x00000000));
                                       break;
                            case 0x02: // MVI Imm,[d]NS
                                       if (!dsp.ProgControlPort.part.S)
                                          writeloadimdest((instruction >> 26) & 0xF, (instruction & 0x7FFFF) | ((instruction & 0x40000) ? 0xFFF80000 : 0x00000000));
                                       break;
                            case 0x03: // MVI Imm,[d]NZS
                                       if (!dsp.ProgControlPort.part.Z || !dsp.ProgControlPort.part.S)
                                          writeloadimdest((instruction >> 26) & 0xF, (instruction & 0x7FFFF) | ((instruction & 0x40000) ? 0xFFF80000 : 0x00000000));
                                       break;
                            case 0x04: // MVI Imm,[d]NC
                                       if (!dsp.ProgControlPort.part.C)
                                          writeloadimdest((instruction >> 26) & 0xF, (instruction & 0x7FFFF) | ((instruction & 0x40000) ? 0xFFF80000 : 0x00000000));
                                       break;
                            case 0x08: // MVI Imm,[d]NT0
                                       if (!dsp.ProgControlPort.part.T0)
                                          writeloadimdest((instruction >> 26) & 0xF, (instruction & 0x7FFFF) | ((instruction & 0x40000) ? 0xFFF80000 : 0x00000000));
                                       break;
                            case 0x21: // MVI Imm,[d]Z
                                       if (dsp.ProgControlPort.part.Z)
                                          writeloadimdest((instruction >> 26) & 0xF, (instruction & 0x7FFFF) | ((instruction & 0x40000) ? 0xFFF80000 : 0x00000000));
                                       break;
                            case 0x22: // MVI Imm,[d]S
                                       if (dsp.ProgControlPort.part.S)
                                          writeloadimdest((instruction >> 26) & 0xF, (instruction & 0x7FFFF) | ((instruction & 0x40000) ? 0xFFF80000 : 0x00000000));
                                       break;
                            case 0x23: // MVI Imm,[d]ZS
                                       if (dsp.ProgControlPort.part.Z || dsp.ProgControlPort.part.S)
                                          writeloadimdest((instruction >> 26) & 0xF, (instruction & 0x7FFFF) | ((instruction & 0x40000) ? 0xFFF80000 : 0x00000000));
                                       break;
                            case 0x24: // MVI Imm,[d]C
                                       if (dsp.ProgControlPort.part.C)
                                          writeloadimdest((instruction >> 26) & 0xF, (instruction & 0x7FFFF) | ((instruction & 0x40000) ? 0xFFF80000 : 0x00000000));
                                       break;
                            case 0x28: // MVI Imm,[d]T0
                                       if (dsp.ProgControlPort.part.T0)
                                          writeloadimdest((instruction >> 26) & 0xF, (instruction & 0x7FFFF) | ((instruction & 0x40000) ? 0xFFF80000 : 0x00000000));
                                       break;
                            default: break;
                       }
                    }
                    else
                    {
                       // MVI Imm,[d]
                       writeloadimdest((instruction >> 26) & 0xF, (instruction & 0xFFFFFF) | ((instruction & 0x1000000) ? 0xFF000000 : 0x00000000));
                    }
   
                    break;
         case 0x03: // Other
                    unsigned long i;

                    switch((instruction >> 28) & 0x3) {
                       case 0x00: // DMA Commands
                       {
                                  int addressAdd;
                                  unsigned long transferNumber;
                                  bool hold=instruction & 0x4000;
                                  bool direction=instruction & 0x1000;

                                  if (instruction & 0x2000)
                                  {
                                     // DMA(H) D0,[RAM],[s]/DMA(H) [RAM],D0,[s]
                                     // command format 2

                                     transferNumber = readgensrc(instruction & 0x7);

                                     addressAdd = 4;
/*
                                     switch((instruction >> 15) & 0x1)
                                     {
                                        case 0: // Add 0
                                                addressAdd = 0;
                                                break;
                                        case 1: // Add 1
                                                addressAdd = 4;
                                                break;
                                        default: break;                                     
                                     }
*/
                                  }
                                  else
                                  {
                                     // DMA(H) D0,[RAM],SImm/DMA(H) [RAM],D0,SImm
                                     // command format 1

                                     transferNumber = instruction & 0xFF;

                                     switch((instruction >> 15) & 0x7)
                                     {
                                        case 0: // Add 0
                                                addressAdd = 0;
                                                break;
                                        case 1: // Add 1
                                                addressAdd = 4;
                                                break;
                                        case 2: // Add 2
                                                addressAdd = 8;
                                                break;
                                        case 3: // Add 4
                                                addressAdd = 16;
                                                break;
                                        case 4: // Add 8
                                                addressAdd = 32;
                                                break;
                                        case 5: // Add 16
                                                addressAdd = 64;
                                                break;
                                        case 6: // Add 32
                                                addressAdd = 128;
                                                break;
                                        case 7: // Add 64
                                                addressAdd = 256;
                                                break;
                                        default: break;                                     
                                     }

//                                     fprintf(stderr, "DMA command format 1: addressAdd = %d transferNumber = %d hold = %d dir = %d\n", addressAdd, transferNumber, hold, direction);
                                  }

                                  if (direction)
                                  {
                                     unsigned long WA0temp=dsp.WA0;

                                     // Looks like some bits are ignored on a real saturn(Grandia takes advantage of this)
                                     dsp.WA0 &= 0x01FFFFFF;

                                     // DMA(H) [RAM], D0, ??
                                     for (i = 0; i < transferNumber; i++)
                                     {
                                        satmem->setLong(dsp.WA0 << 2, readdmasrc((instruction >> 8) & 0x3, addressAdd >> 2));
                                        dsp.WA0 += (addressAdd >> 2); 
                                     }

                                     if (hold) dsp.WA0 = WA0temp;                                        
                                  }
                                  else
                                  {
                                     unsigned long RA0temp=dsp.RA0;

                                     // Looks like some bits are ignored on a real saturn(Grandia takes advantage of this)
                                     dsp.RA0 &= 0x01FFFFFF;

                                     // DMA(H) D0,[RAM], ??
                                     for (i = 0; i < transferNumber; i++)
                                     {
                                        writedmadest((instruction >> 8) & 0x7, satmem->getLong(dsp.RA0 << 2), addressAdd >> 2);
                                        dsp.RA0 += (addressAdd >> 2); 
                                     }

                                     if (hold) dsp.RA0 = RA0temp;                                        
                                  }

                                  break;
                       }
                       case 0x01: // Jump Commands
                                  switch ((instruction >> 19) & 0x7F) {
                                     case 0x00: // JMP Imm
                                                dsp.jmpaddr = instruction & 0xFF;
                                                dsp.delayed = false;
                                                break;
                                     case 0x41: // JMP NZ, Imm
                                                if (!dsp.ProgControlPort.part.Z)
                                                {
                                                   dsp.jmpaddr = instruction & 0xFF;
                                                   dsp.delayed = false; 
                                                }
                                                break;
                                     case 0x42: // JMP NS, Imm
                                                if (!dsp.ProgControlPort.part.S)
                                                {
                                                   dsp.jmpaddr = instruction & 0xFF;
                                                   dsp.delayed = false; 
                                                }

                                                fprintf(stderr, "scu\t: JMP NS: S = %d, jmpaddr = %08X\n", dsp.ProgControlPort.part.S, dsp.jmpaddr);
                                                break;
                                     case 0x43: // JMP NZS, Imm
                                                if (!dsp.ProgControlPort.part.Z || !dsp.ProgControlPort.part.S)
                                                {
                                                   dsp.jmpaddr = instruction & 0xFF;
                                                   dsp.delayed = false; 
                                                }

                                                fprintf(stderr, "scu\t: JMP NZS: Z = %d, S = %d, jmpaddr = %08X\n", dsp.ProgControlPort.part.Z, dsp.ProgControlPort.part.S, dsp.jmpaddr);
                                                break;
                                     case 0x44: // JMP NC, Imm
                                                if (!dsp.ProgControlPort.part.C)
                                                {
                                                   dsp.jmpaddr = instruction & 0xFF;
                                                   dsp.delayed = false; 
                                                }
                                                break;
                                     case 0x48: // JMP NT0, Imm
                                                if (!dsp.ProgControlPort.part.T0)
                                                {
                                                   dsp.jmpaddr = instruction & 0xFF;
                                                   dsp.delayed = false; 
                                                }

                                                fprintf(stderr, "scu\t: JMP NT0: T0 = %d, jmpaddr = %08X\n", dsp.ProgControlPort.part.T0, dsp.jmpaddr);
                                                break;
                                     case 0x61: // JMP Z,Imm
                                                if (dsp.ProgControlPort.part.Z)
                                                {
                                                   dsp.jmpaddr = instruction & 0xFF;
                                                   dsp.delayed = false; 
                                                }
                                                break;
                                     case 0x62: // JMP S, Imm
                                                if (dsp.ProgControlPort.part.S)
                                                {
                                                   dsp.jmpaddr = instruction & 0xFF;
                                                   dsp.delayed = false; 
                                                }

                                                fprintf(stderr, "scu\t: JMP S: S = %d, jmpaddr = %08X\n", dsp.ProgControlPort.part.S, dsp.jmpaddr);
                                                break;
                                     case 0x63: // JMP ZS, Imm
                                                if (dsp.ProgControlPort.part.Z || dsp.ProgControlPort.part.S)
                                                {
                                                   dsp.jmpaddr = instruction & 0xFF;
                                                   dsp.delayed = false; 
                                                }

                                                fprintf(stderr, "scu\t: JMP ZS: Z = %d, S = %d, jmpaddr = %08X\n", dsp.ProgControlPort.part.Z, dsp.ProgControlPort.part.S, dsp.jmpaddr);
                                                break;
                                     case 0x64: // JMP C, Imm
                                                if (dsp.ProgControlPort.part.C)
                                                {
                                                   dsp.jmpaddr = instruction & 0xFF;
                                                   dsp.delayed = false; 
                                                }
                                                break;
                                     case 0x68: // JMP T0,Imm
                                                if (dsp.ProgControlPort.part.T0)
                                                {
                                                   dsp.jmpaddr = instruction & 0xFF;
                                                   dsp.delayed = false; 
                                                }
                                                break;
                                     default:
                                                fprintf(stderr, "scu\t: Unknown JMP instruction not implemented\n");
                                                break;
                                  }
                                  break;
                       case 0x02: // Loop bottom Commands

                                  if (instruction & 0x8000000)
                                  {
                                     // LPS
                                     if (dsp.LOP != 0)
                                     {
                                        dsp.jmpaddr = dsp.PC;
                                        dsp.delayed = false;
                                        dsp.LOP--;
                                     }
                                  }
                                  else
                                  {
                                     // BTM
                                     if (dsp.LOP != 0)
                                     {
                                        dsp.jmpaddr = dsp.TOP;
                                        dsp.delayed = false;
                                        dsp.LOP--;
                                     }
                                  }

                                  break;
                       case 0x03: // End Commands

                                  dsp.ProgControlPort.part.EX = false;

                                  if (instruction & 0x8000000) {
                                     // End with Interrupt
                                     dsp.ProgControlPort.part.E = true;
                                     sendDSPEnd();
                                  }

#if DEBUG
                                  fprintf(stderr, "dsp has ended\n");
#endif

                                  break;
                       default: break;
                    }
                    break;
         default: 
#if DEBUG
                    fprintf(stderr, "scu\t: Invalid DSP opcode %08X at offset %02X\n", instruction, dsp.PC);
#endif
                    break;
      }

      // Do RX*RY multiplication
      dsp.MUL.all = (signed)dsp.RX * (signed)dsp.RY;

      dsp.PC++;

      // Handle delayed jumps
      if (dsp.jmpaddr != 0xFFFFFFFF)
      {
         if (dsp.delayed)
         {
            dsp.PC = (unsigned char)dsp.jmpaddr;
            dsp.jmpaddr = 0xFFFFFFFF;
         }
         else
            dsp.delayed = true;
      }
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

char *disloadimdest(unsigned char num)
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
      case 0xC:
          return "TOP";
      default: break;
   }

   return "??";
}


void Scu::DSPDisasm(unsigned char addr, char *outstring){
   unsigned long instruction;

   instruction = dsp.ProgramRam[addr];

   sprintf(outstring, "%08X: \0", addr);
   outstring+=strlen(outstring);

   if (instruction == 0)
   {
      sprintf(outstring, "NOP\0");
      return;
   }

   // Handle ALU commands
   switch (instruction >> 26)
   {
      case 0x0: // NOP
                break;
      case 0x1: // AND
                sprintf(outstring, "AND\0");
                outstring+=strlen(outstring);
                break;
      case 0x2: // OR
                sprintf(outstring, "OR\0");
                outstring+=strlen(outstring);
                break;
      case 0x3: // XOR
                sprintf(outstring, "XOR\0");
                outstring+=strlen(outstring);
                break;
      case 0x4: // ADD
                sprintf(outstring, "ADD\0");
                outstring+=strlen(outstring);
                break;
      case 0x5: // SUB
                sprintf(outstring, "SUB\0");
                outstring+=strlen(outstring);
                break;
      case 0x6: // AD2
                sprintf(outstring, "AD2\0");
                outstring+=strlen(outstring);
                break;
      case 0x8: // SR
                sprintf(outstring, "AD2\0");
                outstring+=strlen(outstring);
                break;
      case 0x9: // RR
                sprintf(outstring, "RR\0");
                outstring+=strlen(outstring);
                break;
      case 0xA: // SL
                outstring+=strlen(outstring);
                sprintf(outstring, "SL\0");
                break;
      case 0xB: // RL
                sprintf(outstring, "RL\0");
                outstring+=strlen(outstring);
                break;
      case 0xF: // RL8
                sprintf(outstring, "RL8\0");
                outstring+=strlen(outstring);
                break;
      default: break;
   }


   switch (instruction >> 30) {
      case 0x00: // Operation Commands
                 if ((instruction >> 23) & 0x4)
                 {
                    sprintf(outstring, "MOV %s, X\0", disd1bussrc((instruction >> 20) & 0x7));
                    outstring+=strlen(outstring);
                 }
                 switch ((instruction >> 23) & 0x3)
                 {
                    case 2:
                            sprintf(outstring, "MOV MUL, P\0");
                            outstring+=strlen(outstring);
                            break;
                    case 3:
                            sprintf(outstring, "MOV [s], P\0");
                            outstring+=strlen(outstring);
                            break;
                    default: break;
                 }

                 // Y-bus
                 if ((instruction >> 17) & 0x4)
                 {
                    sprintf(outstring, "MOV %s, Y\0", disd1bussrc((instruction >> 14) & 0x7));
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
                            sprintf(outstring, "MOV %s, A\0", disd1bussrc((instruction >> 14) & 0x7));
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
                                    sprintf(outstring, "MVI #$%05X,%s,NZ\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                                    break;
                         case 0x02:
                                    sprintf(outstring, "MVI #$%05X,%s,NS\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                                    break;
                         case 0x03:
                                    sprintf(outstring, "MVI #$%05X,%s,NZS\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                                    break;
                         case 0x04:
                                    sprintf(outstring, "MVI #$%05X,%s,NC\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                                    break;
                         case 0x08:
                                    sprintf(outstring, "MVI #$%05X,%s,NT0\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                                    break;
                         case 0x21:
                                    sprintf(outstring, "MVI #$%05X,%s,Z\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                                    break;
                         case 0x22:
                                    sprintf(outstring, "MVI #$%05X,%s,S\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                                    break;
                         case 0x23:
                                    sprintf(outstring, "MVI #$%05X,%s,ZS\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                                    break;
                         case 0x24:
                                    sprintf(outstring, "MVI #$%05X,%s,C\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                                    break;
                         case 0x28:
                                    sprintf(outstring, "MVI #$%05X,%s,T0\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                                    break;
                         default: break;
                    }
                 }
                 else
                 {
                    sprintf(outstring, "MVI #$%05X,%s\0", instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
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
                                             sprintf(outstring, "JMP $%02X\0", instruction & 0xFF);
                                             break;
                                  case 0x41:
                                             sprintf(outstring, "JMP NZ,$%02X\0", instruction & 0xFF);
                                             break;
                                  case 0x42:
                                             sprintf(outstring, "JMP NS,$%02X\0", instruction & 0xFF);
                                             break;
                                  case 0x43:
                                             sprintf(outstring, "JMP NZS,$%02X\0", instruction & 0xFF);
                                             break;
                                  case 0x44:
                                             sprintf(outstring, "JMP NC,$%02X\0", instruction & 0xFF);
                                             break;
                                  case 0x48:
                                             sprintf(outstring, "JMP NT0,$%02X\0", instruction & 0xFF);
                                             break;
                                  case 0x61:
                                             sprintf(outstring, "JMP Z,$%02X\0", instruction & 0xFF);
                                             break;
                                  case 0x62:
                                             sprintf(outstring, "JMP S,$%02X\0", instruction & 0xFF);
                                             break;
                                  case 0x63:
                                             sprintf(outstring, "JMP ZS,$%02X\0", instruction & 0xFF);
                                             break;
                                  case 0x64:
                                             sprintf(outstring, "JMP C,$%02X\0", instruction & 0xFF);
                                             break;
                                  case 0x68:
                                             sprintf(outstring, "JMP T0,$%02X\0", instruction & 0xFF);
                                             break;
                                  default:
                                             sprintf(outstring, "Unknown JMP\0");
                                             break;
                               }
                               break;
                    case 0x02: // Loop bottom Commands
                               if (instruction & 0x8000000)
                                  sprintf(outstring, "LPS\0");
                               else
                                  sprintf(outstring, "BTM\0");

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
                 sprintf(outstring, "Invalid opcode\0", instruction, dsp.PC);
                 break;
   }
}

void Scu::DSPStep(void) {
   run(1);
}

void Scu::GetDSPRegisters(scudspregs_struct *regs) {
  if (regs != NULL) {
     memcpy(regs, &dsp, sizeof(scudspregs_struct));
  }
}

void Scu::SetDSPRegisters(scudspregs_struct *regs) {
  if (regs != NULL) {
     memcpy(&dsp, regs, sizeof(scudspregs_struct));
  }
}

void Scu::SetBreakpointCallBack(void (*func)(unsigned long)) {
   BreakpointCallBack = func;
}

int Scu::AddCodeBreakpoint(unsigned long addr) {
  if (numcodebreakpoints < MAX_BREAKPOINTS) {
     codebreakpoint[numcodebreakpoints].addr = addr;
     numcodebreakpoints++;

     return 0;
  }

  return -1;
}

int Scu::DelCodeBreakpoint(unsigned long addr) {
  if (numcodebreakpoints > 0) {
     for (int i = 0; i < numcodebreakpoints; i++) {
        if (codebreakpoint[i].addr == addr)
        {
           codebreakpoint[i].addr = 0xFFFFFFFF;
           SortCodeBreakpoints();
           numcodebreakpoints--;
           return 0;
        }
     }
  }

  return -1;
}

scucodebreakpoint_struct *Scu::GetBreakpointList() {
  return codebreakpoint;
}

void Scu::ClearCodeBreakpoints() {
     for (int i = 0; i < MAX_BREAKPOINTS; i++)
        codebreakpoint[i].addr = 0xFFFFFFFF;

     numcodebreakpoints = 0;
}

void Scu::SortCodeBreakpoints() {
  int i, i2;
  unsigned long tmp;

  for (i = 0; i < (MAX_BREAKPOINTS-1); i++)
  {
     for (i2 = i+1; i2 < MAX_BREAKPOINTS; i2++)
     {
        if (codebreakpoint[i].addr == 0xFFFFFFFF &&
            codebreakpoint[i2].addr != 0xFFFFFFFF)
        {
           tmp = codebreakpoint[i].addr;
           codebreakpoint[i].addr = codebreakpoint[i2].addr;
           codebreakpoint[i2].addr = tmp;
        }
     }
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

   // Write DSP area
   fwrite((void *)&dsp, sizeof(scudspregs_struct), 1, fp);

   return stateFinishHeader(fp, offset);
}

int Scu::loadState(FILE *fp, int version, int size) {
   // Read registers
   fread((void *)this->getBuffer(), 0x100, 1, fp);
   fread((void *)&timer0, 4, 1, fp);
   fread((void *)&timer1, 4, 1, fp);

   // Read DSP area
   fread((void *)&dsp, sizeof(scudspregs_struct), 1, fp);

   return size;
}
