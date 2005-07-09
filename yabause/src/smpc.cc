/*  Copyright 2003-2004 Guillaume Duhamel
    Copyright 2004 Theo Berkau

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

#include "saturn_memory.hh"
#include "yui.hh"
#include <time.h>

void Smpc::setByte(unsigned long addr, unsigned char val) {
  switch (addr) {
    case 0x01: // Maybe an INTBACK continue/break request
               reg.IREG[0] = val;

               if (intback)
               {
                  if (reg.IREG[0] & 0x40) {
                    // Break
                    intback = false;
                    reg.SR &= 0x0F;
                    break;
                  }
                  else if (reg.IREG[0] & 0x80) {                    
                    // Continue
                    reg.COMREG = 0x10;
                    setTiming();
                    reg.SF = true;
                  }
               }
               return;
    case 0x03:
               reg.IREG[1] = val;
               return;
    case 0x05:
               reg.IREG[2] = val;
               return;
    case 0x07:
               reg.IREG[3] = val;
               return;
    case 0x09:
               reg.IREG[4] = val;
               return;
    case 0x0B:
               reg.IREG[5] = val;
               return;
    case 0x0D:
               reg.IREG[6] = val;
               return;
    case 0x1F:
               reg.COMREG = val;
               setTiming();
               return;
    case 0x63:
               reg.SF = val & 0x1;
               return;
    case 0x75:
               // FIX ME (should support other peripherals)

               switch (reg.DDR[0] & 0x7F) { // Which Control Method do we use?
                 case 0x40:
#if DEBUG
                            cerr << "smpc\t: Peripheral TH Control Method not implemented" << endl;
#endif
                            break;
                 case 0x60:
                            switch (val & 0x60) {
                               case 0x60: // 1st Data
                                          val = (val & 0x80) | 0x14 | (buttonbits & 0x8);
                                          break;
                               case 0x20: // 2nd Data
                                          val = (val & 0x80) | 0x10 | (buttonbits >> 12);
                                          break;
                               case 0x40: // 3rd Data
                                          val = (val & 0x80) | 0x10 | ((buttonbits >> 8) & 0xF);
                                          break;
                               case 0x00: // 4th Data
                                          val = (val & 0x80) | 0x10 | ((buttonbits >> 4) & 0xF);
                                          break;
                               default: break;
                            }

                            reg.PDR[0] = val;
                            break;
                 default:
#if DEBUG
                            cerr << "smpc\t: Peripheral Unknown Control Method not implemented" << endl;
#endif
                            break;
               }

               return;
    case 0x77:
               // fix me
               reg.PDR[1] = val;               
               return;
    case 0x79:
               reg.DDR[0] = val;
               return;
    case 0x7B:
               reg.DDR[1] = val;
               return;
    case 0x7D:
               reg.IOSEL = val;
               return;
    case 0x7F:
               reg.EXLE = val;
               return;
    default:
#if DEBUG
               fprintf(stderr, "Unsupported smpc byte write to %08X\n", addr);
#endif
               break;
  }
}

unsigned char Smpc::getByte(unsigned long addr) {
  switch (addr) {
    case 0x21:
               return reg.OREG[0];
    case 0x23:
               return reg.OREG[1];
    case 0x25:
               return reg.OREG[2];
    case 0x27:
               return reg.OREG[3];
    case 0x29:
               return reg.OREG[4];
    case 0x2B:
               return reg.OREG[5];
    case 0x2D:
               return reg.OREG[6];
    case 0x2F:
               return reg.OREG[7];
    case 0x31:
               return reg.OREG[8];
    case 0x33:
               return reg.OREG[9];
    case 0x35:
               return reg.OREG[10];
    case 0x37:
               return reg.OREG[11];
    case 0x39:
               return reg.OREG[12];
    case 0x3B:
               return reg.OREG[13];
    case 0x3D:
               return reg.OREG[14];
    case 0x3F:
               return reg.OREG[15];
    case 0x41:
               return reg.OREG[16];
    case 0x43:
               return reg.OREG[17];
    case 0x45:
               return reg.OREG[18];
    case 0x47:
               return reg.OREG[19];
    case 0x49:
               return reg.OREG[20];
    case 0x4B:
               return reg.OREG[21];
    case 0x4D:
               return reg.OREG[22];
    case 0x4F:
               return reg.OREG[23];
    case 0x51:
               return reg.OREG[24];
    case 0x53:
               return reg.OREG[25];
    case 0x55:
               return reg.OREG[26];
    case 0x57:
               return reg.OREG[27];
    case 0x59:
               return reg.OREG[28];
    case 0x5B:                  
               return reg.OREG[29];
    case 0x5D:
               return reg.OREG[30];
    case 0x5F:
               return reg.OREG[31];
    case 0x61:
               return reg.SR;
    case 0x63:
               return reg.SF;
    case 0x75:
               return reg.PDR[0];
    case 0x77:
               return reg.PDR[1];
    default:
#if DEBUG
               fprintf(stderr, "Unsupported smpc byte read from %08X\n", addr);
#endif
               return 0;
  }
}

Smpc::Smpc(SaturnMemory *sm) : Memory(0x7F, 0x80) {
  dotsel = false;
  mshnmi = false;
  sndres = false;
  cdres = false;
  sysres = false;
  resb = false;
  ste = false;
  resd = false;
  intback = false;
  firstPeri = false;

  // get region
  regionid = yui_region();

  if (regionid == 0)
  {
     // Time to autodetect the region using the cd block
     regionid = sm->cs2->GetRegionID();          
  }
  
  this->sm = sm;
  reset();
}

void Smpc::reset(void) {
  memset(SMEM, 0, 4); // This should be loaded from file
  timing = 0;

  memset(&reg, 0, sizeof(reg));
}

void Smpc::setTiming(void) {
        switch(reg.COMREG) {
                case 0x0:
#if DEBUG
                        cerr << "smpc\t: MSHON not implemented\n";
#endif
                        timing = 1;
                        return;
                case 0x8:
#if DEBUG
                        cerr << "smpc\t: CDON not implemented\n";
#endif
                        timing = 1;
                        return;
                case 0x9:
#if DEBUG
                        cerr << "smpc\t: CDOFF not implemented\n";
#endif
                        timing = 1;
                        return;
                case 0xD:
                case 0xE:
                case 0xF:
                        timing = 1; // this has to be tested on a real saturn
                        return;
		case 0x10:
                        if (intback) timing = 20; // this will need to be verified
                        else {
                           // Calculate timing based on what data is being retrieved

                           timing = 1;

                           // If retrieving non-peripheral data, add 0.2 milliseconds
                           if (reg.IREG[0] == 0x01)
                              timing += 2;

                           // If retrieving peripheral data, add 15 milliseconds
                           if (reg.IREG[1] & 0x8)
                              timing += 150;
                        }
                        return;
                case 0x17:
                        timing = 1;
                        return;
                case 0x2:
                        timing = 1;
                        return;
                case 0x3:
                        timing = 1;                        
                        return;
		case 0x6:
		case 0x7:
		case 0x19:
		case 0x1A:
                        timing = 1;
                        return;
		default:
                        cerr << "smpc\t: unimplemented command: " << hex << (int) reg.COMREG << endl;
                        reg.SF = false;

                        break;
	}
}

void Smpc::execute2(unsigned long t) {
	if (timing > 0) {
		timing -= t;
		if (timing <= 0) {
			execute(this);
		}
	}
}

void Smpc::execute(Smpc *smpc) {
  switch(smpc->reg.COMREG) {
  case 0x0:
#if DEBUG
    cerr << "smpc\t: MSHON not implemented\n";
#endif
    break;
  case 0x2:
#if DEBUG
    cerr << "smpc\t: SSHON\n";
#endif
    smpc->SSHON();
    break;
  case 0x3:
#if DEBUG
    cerr << "smpc\t: SSHOFF\n";
#endif
    smpc->SSHOFF();
    break;
  case 0x6:
#if DEBUG
    cerr << "smpc\t: SNDON\n";
#endif
    smpc->SNDON();
    break;
  case 0x7:
#if DEBUG
    cerr << "smpc\t: SNDOFF\n";
#endif
    smpc->SNDOFF();
    break;
  case 0x8:
#if DEBUG
    cerr << "smpc\t: CDON not implemented\n";
#endif
    break;
  case 0x9:
#if DEBUG
    cerr << "smpc\t: CDOFF not implemented\n";
#endif
    break;
  case 0xD:
#if DEBUG
    cerr << "smpc\t: SYSRES not implemented\n";
#endif
    break;
  case 0xE:
#if DEBUG
    cerr << "smpc\t: CKCHG352\n";
#endif
    smpc->CKCHG352();
    break;
  case 0xF:
#if DEBUG
    cerr << "smpc\t: CKCHG320\n";
#endif
    smpc->CKCHG320();
    break;
  case 0x10:
#if DEBUG
    cerr << "smpc\t: INTBACK\n";
#endif
    smpc->INTBACK();
    break;
  case 0x17:
#if DEBUG
    cerr << "smpc\t: SETSMEM\n";
#endif
    smpc->SETSMEM();
    break;
  case 0x19:
#if DEBUG
    cerr << "smpc\t: RESENAB\n";
#endif
    smpc->RESENAB();
    break;
  case 0x1A:
#if DEBUG
    cerr << "smpc\t: RESDISA\n";
#endif
    smpc->RESDISA();
    break;
  default:
#if DEBUG
    cerr << "smpc\t: command " << ((int) reg.COMREG)
         << " not implemented\n";
#endif
    break;
  }
  
  smpc->reg.SF = false;
}

void Smpc::INTBACK(void) {
  reg.SF = true;

  if (intback) {
    INTBACKPeripheral();
    sm->scu->sendSystemManager();
    return;
  }
  if (intbackIreg0 = reg.IREG[0]) {
    // Return non-peripheral data
    firstPeri = true;
    intback = reg.IREG[1] & 0x8; // does the program want peripheral data too?
    INTBACKStatus();
    reg.SR = 0x4F | (intback << 5); // the low nibble is undefined(or 0xF)
    sm->scu->sendSystemManager();
    return;
  }
  if (reg.IREG[1] & 0x8) {
    firstPeri = true;
    intback = true;
    reg.SR = 0x40;
    INTBACKPeripheral();
    reg.OREG[31] = 0x10; // may need to be changed
    sm->scu->sendSystemManager();
    return;
  }
}

void Smpc::INTBACKStatus(void) {
  //Timer t;
  //t.wait(3000);
#if DEBUG
  //cerr << "INTBACK status\n";
#endif
    // return time, cartidge, zone, etc. data
    
    reg.OREG[0] = 0x80;   // goto normal startup
    //reg.OREG[0] = 0x0;  // goto setclock/setlanguage screen
    
    // write time data in OREG1-7
    time_t tmp = time(NULL);
    struct tm times;
#ifdef WIN32
    memcpy(&times, localtime(&tmp), sizeof(times));
#else
    localtime_r(&tmp, &times);
#endif
    unsigned char year[4] = {
      (1900 + times.tm_year) / 1000,
      ((1900 + times.tm_year) % 1000) / 100,
      (((1900 + times.tm_year) % 1000) % 100) / 10,
      (((1900 + times.tm_year) % 1000) % 100) % 10 };
    reg.OREG[1] = (year[0] << 4) | year[1];
    reg.OREG[2] = (year[2] << 4) | year[3];
    reg.OREG[3] = (times.tm_wday << 4) | (times.tm_mon + 1);
    reg.OREG[4] = ((times.tm_mday / 10) << 4) | (times.tm_mday % 10);
    reg.OREG[5] = ((times.tm_hour / 10) << 4) | (times.tm_hour % 10);
    reg.OREG[6] = ((times.tm_min / 10) << 4) | (times.tm_min % 10);
    reg.OREG[7] = ((times.tm_sec / 10) << 4) | (times.tm_sec % 10);

    // write cartidge data in OREG8
    reg.OREG[8] = 0; // FIXME : random value
    
    // write zone data in OREG9 bits 0-7
    // 1 -> japan
    // 2 -> asia/ntsc
    // 4 -> north america
    // 5 -> central/south america/ntsc
    // 6 -> corea
    // A -> asia/pal
    // C -> europe + others/pal
    // D -> central/south america/pal
    reg.OREG[9] = regionid;

    // system state, first part in OREG10, bits 0-7
    // bit | value  | comment
    // ---------------------------
    // 7   | 0      |
    // 6   | DOTSEL |
    // 5   | 1      |
    // 4   | 1      |
    // 3   | MSHNMI |
    // 2   | 1      |
    // 1   | SYSRES | 
    // 0   | SNDRES |
    reg.OREG[10] = 0x34|(dotsel<<6)|(mshnmi<<3)|(sysres<<1)|sndres;
    
    // Etat du système, deuxième partie dans OREG11, bit 6
    // system state, second part in OREG11, bit 6
    // bit 6 -> CDRES
    reg.OREG[11] = cdres << 6; // FIXME
    
    // SMEM
    for(int i = 0;i < 4;i++) reg.OREG[12+i] = SMEM[i];
    
    reg.OREG[31] = 0x10; // set to intback command
}

void Smpc::RESENAB(void) {
  resd = false;
  reg.OREG[31] = 0x19;
}

void Smpc::RESDISA(void) {
  resd = true;
  reg.OREG[31] = 0x1A;
}

void Smpc::SNDON(void) {
    sm->soundr->reset68k();  
    sm->soundr->is68kOn = true;
    reg.OREG[31] = 0x6;
}

void Smpc::SNDOFF(void) {
    sm->soundr->is68kOn = false;
    reg.OREG[31] = 0x7;
}

void Smpc::INTBACKPeripheral(void) {
#if DEBUG
  //cerr << "INTBACK peripheral\n";
#endif
  if (firstPeri)
    reg.SR = 0xC0 | (reg.IREG[1] >> 4);
  else
    reg.SR = 0x80 | (reg.IREG[1] >> 4);

  firstPeri = false;

  //Timer t;
  //t.wait(20);

  /* Port Status:
  0x04 - Sega-tap is connected
  0x16 - Multi-tap is connected
  0x21-0x2F - Clock serial peripheral is connected
  0xF0 - Not Connected or Unknown Device
  0xF1 - Peripheral is directly connected */

  /* PeripheralID:
  0x02 - Digital Device Standard Format
  0x13 - Racing Device Standard Format
  0x15 - Analog Device Standard Format
  0x23 - Pointing Device Standard Format
  0x23 - Shooting Device Standard Format
  0x34 - Keyboard Device Standard Format
  0xE1 - Mega Drive 3-Button Pad
  0xE2 - Mega Drive 6-Button Pad
  0xE3 - Saturn Mouse
  0xFF - Not Connected */

  /* Special Notes(for potential future uses):

  If a peripheral is disconnected from a port, you only return 1 byte for
  that port(which is the port status 0xF0), at the next OREG you then return
  the port status of the next port.

  e.g. If Port 1 has nothing connected, and Port 2 has a controller
       connected:

  OREG0 = 0xF0
  OREG1 = 0xF1
  OREG2 = 0x02
  etc.
  */

  // Port 1
  reg.OREG[0] = 0xF1; //Port Status(Directly Connected)
  reg.OREG[1] = 0x02; //PeripheralID(Standard Pad)
  reg.OREG[2] = buttonbits >> 8;   //First Data
  reg.OREG[3] = buttonbits & 0xFF;  //Second Data

  // Port 2
  reg.OREG[4] = 0xF0; //Port Status(Not Connected)
/*
  Use this as a reference for implementing other peripherals
  // Port 1
  reg.OREG[0] = 0xF1; //Port Status(Directly Connected)
  reg.OREG[1] = 0xE3; //PeripheralID(Shuttle Mouse)
  reg.OREG[2] = 0x00; //First Data
  reg.OREG[3] = 0x00; //Second Data
  reg.OREG[4] = 0x00; //Third Data

  // Port 2
  reg.OREG[5] = 0xF0; //Port Status(Not Connected)
*/
}

void Smpc::INTBACKEnd(void) {
  intback = false;
}

void Smpc::SETSMEM(void) {
  for(int i = 0;i < 4;i++) SMEM[i] = reg.IREG[i];
  reg.OREG[31] = 0x17;
}

void Smpc::SSHOFF(void) {
        sm->stopSlave();
}

void Smpc::SSHON(void) {
	sm->startSlave();
}

void Smpc::CKCHG352(void) {
  // Reset VDP1, VDP2, SCU, and SCSP
  sm->vdp1_2->reset();  
  sm->vdp2_3->reset();  
  sm->scu->reset();  
//  ((Scsp *) sm->getScsp())->reset();  

  // Clear VDP1/VDP2 ram

  sm->stopSlave();

  // change clock
}

void Smpc::CKCHG320(void) {
  // Reset VDP1, VDP2, SCU, and SCSP
  sm->vdp1_2->reset();  
  sm->vdp2_3->reset();  
  sm->scu->reset();  
//  ((Scsp *) sm->getScsp())->reset();  

  // Clear VDP1/VDP2 ram

  sm->stopSlave();

  // change clock
}

int Smpc::saveState(FILE *fp) {
   int offset;

   offset = stateWriteHeader(fp, "SMPC", 1);

   // Write registers
   fwrite((void *)this->getBuffer(), 0x80, 1, fp);

   return stateFinishHeader(fp, offset);
}

int Smpc::loadState(FILE *fp, int version, int size) {
   // Read registers
   fread((void *)this->getBuffer(), 0x80, 1, fp);

   // May need to rework the following
   timing = 0;

   return size;
}
