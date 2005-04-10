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

#include "vdp1.hh"
#include "vdp2.hh"
#include <unistd.h>
#include "exception.hh"
#include "superh.hh"
#ifdef _arch_dreamcast
#include <kos.h>
#endif
#include "yui.hh"
#include "registres.hh"

SuperH::SuperH(bool slave, SaturnMemory *sm) {
  onchip = new Onchip(slave, sm, this);
  purgeArea   = new Dummy(0xFFFFFFFF);
  adressArray = new Memory(0xFFF, 0x3FF);
  dataArray   = new Memory(0xFFF, 0x1000);
  modeSdram = new Memory(0xFFF, 0x4FFF);

  regs = (sh2regs_struct *) &regs_array;

  reset();

  for(int i = 0;i < 0x10000;i++) {
    instruction = i;
    opcodes[i] = decode();
  }

  for (int i = 0; i < MAX_BREAKPOINTS; i++) {
     codebreakpoint[i].addr = 0xFFFFFFFF;
     codebreakpoint[i].oldopcode = 0xFFFF;
  }
  numcodebreakpoints = 0;
  BreakpointCallBack=NULL;
  inbreakpoint=false;

  isslave = slave;
  cycleCount = 0;
}

SuperH::~SuperH(void) {
  delete onchip;
  delete purgeArea;
  delete adressArray;
  delete dataArray;
  delete modeSdram;
}

void SuperH::reset(void) {
  regs->SR.part.T = regs->SR.part.S = regs->SR.part.Q = regs->SR.part.M = 0;
  regs->SR.part.I = 0xF;
  regs->SR.part.useless1 = 0;
  regs->SR.part.useless2 = 0;
  regs->VBR = 0;
  for(int i = 0;i < 16;i++) regs->R[i] = 0;

  // reset interrupts
  while ( !interrupts.empty() ) {
        interrupts.pop();
  }

  ((Onchip*)onchip)->reset();
 
  timing = 0;
}

void SuperH::setMemory(SaturnMemory *mem) {
         memoire = mem;
         regs->PC = memoire->getLong(regs->VBR);
         regs->R[15] = memoire->getLong(regs->VBR + 4);
}

Memory *SuperH::getMemory(void) {
  return memoire;
}

void SuperH::send(const Interrupt& i) {
	interrupts.push(i);
}

void SuperH::sendNMI(void) {
	send(Interrupt(16, 11));
}

void /*inline*/ _executer(SuperH * sh) {
	if (sh->regs->PC & 0x6000000) {
		sh->instruction = readWord(sh->memoire->ramHigh, sh->regs->PC);
	}
	else {
                sh->instruction = readWord(sh->memoire->rom, sh->regs->PC);
	}
        (*sh->opcodes[sh->instruction])(sh);
}

void SuperH::executer(void) {
/*
  if (_delai) {
    unsigned long tmp = regs->PC;
    regs->PC = _delai;
    _executer(this);
    regs->PC = tmp;
    _delai = 0;
  }
  else {
*/
/*
    if ( !interrupts.empty() ) {
      Interrupt interrupt = interrupts.top();
      if (interrupt.level() > regs->SR.part.I) {
        interrupts.pop();

        R[15] -= 4;
        memoire->setLong(R[15], regs->SR.all);
        R[15] -= 4;
        memoire->setLong(R[15], regs->PC);
        regs->SR.part.I = interrupt.level();
        regs->PC = memoire->getLong(regs->VBR + (interrupt.vector() << 2));
      }
    }
*/
#ifdef _arch_dreamcast
    cont_cond_t cond;
    cont_get_cond(maple_first_controller(), &cond);
    if(!(cond.buttons & CONT_A)) {
	    vid_screen_shot("/pc/saturn.raw");
    }
#endif
    _executer(this);
  //}
}

void SuperH::delay(unsigned long addr) {
        switch ((addr >> 20) & 0x0FF) {
           case 0x000: // Bios              
                       instruction = readWord(memoire->rom, addr);
                       break;
           case 0x002: // Low Work Ram
                       instruction = readWord(memoire->ramLow, addr);
                       break;
           case 0x020: // CS0
                       instruction = memoire->getWord(addr);
                       break;
           case 0x060: // High Work Ram
           case 0x061: 
           case 0x062: 
           case 0x063: 
           case 0x064: 
           case 0x065: 
           case 0x066: 
           case 0x067: 
           case 0x068: 
           case 0x069: 
           case 0x06A: 
           case 0x06B: 
           case 0x06C: 
           case 0x06D: 
           case 0x06E: 
           case 0x06F:
                       instruction = readWord(memoire->ramHigh, addr);
                       break;
           default:
                       break;
        }

        (*opcodes[instruction])(this);
        regs->PC -= 2;
}

/*
void SuperH::_executer(void) {
//        if (isslave) cerr << "regs->PC=" << regs->PC << endl;
        unsigned long tmp = (regs->PC >> 19) & 0xFF;
        switch(tmp) {
                case 0:
                        // rom
                        instruction = readWord(memoire->rom, regs->PC);
                        break;
                case 0xC0:
                        // work ram high
                        instruction = readWord(memoire->ramHigh, regs->PC);
                        break;
                default:
                        cerr << hex << "coin = " <<  tmp << endl;
                        instruction = memoire->getWord(regs->PC);
                        break;
        }
	if (regs->PC & 0x6000000) {
		instruction = readWord(memoire->ramHigh, regs->PC);
	}
	else {
                instruction = readWord(memoire->rom, regs->PC);
	}
        (*opcodes[instruction])(this);
}
*/

void SuperH::run(int t) {
	if (timing > 0) {
		timing -= t;
		if (timing <= 0) {
			sendNMI();
		}
	}
}

void SuperH::runCycles(unsigned long cc) {
    if ( !interrupts.empty() ) {
      Interrupt interrupt = interrupts.top();
      if (interrupt.level() > regs->SR.part.I) {
        interrupts.pop();

        regs->R[15] -= 4;
        memoire->setLong(regs->R[15], regs->SR.all);
        regs->R[15] -= 4;
        memoire->setLong(regs->R[15], regs->PC);
        regs->SR.part.I = interrupt.level();
        regs->PC = memoire->getLong(regs->VBR + (interrupt.vector() << 2));
      }
    }
        while(cycleCount < cc) {
           // Make sure it isn't one of our breakpoints
           for (int i=0; i < numcodebreakpoints; i++) {
              if ((regs->PC == codebreakpoint[i].addr) && inbreakpoint == false) {

                 inbreakpoint = true;
                 if (BreakpointCallBack) BreakpointCallBack(isslave, codebreakpoint[i].addr);
                 inbreakpoint = false;
              }
           }

           //_executer(this);
        switch ((regs->PC >> 20) & 0x0FF) {
           case 0x000: // Bios              
                       instruction = readWord(memoire->rom, regs->PC);
                       break;
           case 0x002: // Low Work Ram
                       instruction = readWord(memoire->ramLow, regs->PC);
                       break;
           case 0x020: // CS0(fix me)
                       instruction = memoire->getWord(regs->PC);
                       break;
           case 0x060: // High Work Ram
           case 0x061: 
           case 0x062: 
           case 0x063: 
           case 0x064: 
           case 0x065: 
           case 0x066: 
           case 0x067: 
           case 0x068: 
           case 0x069: 
           case 0x06A: 
           case 0x06B: 
           case 0x06C: 
           case 0x06D: 
           case 0x06E: 
           case 0x06F:
                       instruction = readWord(memoire->ramHigh, regs->PC);
                       break;
           default:
                       break;
        }

        (*opcodes[instruction])(this);
        }

        ((Onchip *)onchip)->runFRT(cc);
        ((Onchip *)onchip)->runWDT(cc);
}

void SuperH::step(void) {
   unsigned long tmp = regs->PC;

   // Execute 1 instruction
   runCycles(cycleCount+1);

   // Sometimes it doesn't always seem to execute one instruction,
   // let's make sure it did
   if (tmp == regs->PC)
      runCycles(cycleCount+1);
}

#ifndef _arch_dreamcast
ostream& operator<<(ostream& os, const SuperH& sh) {
  for(int j = 0;j < 4;j++) {
    os << "\t";
    for(int i = 4*j;i < 4 * (j + 1);i++) {
      os << "R" << dec
#ifdef STL_LEFT_RIGHT
		<< left
#endif
		<< setw(4) << i << hex
#ifdef STL_LEFT_RIGHT
		<< right
#endif
		<< setw(10) << sh.regs->R[i] << "  ";
    }
    os << "\n";
  }

  os << dec
#ifdef STL_LEFT_RIGHT
     << left
#endif
                     << "\tregs->SR    M  " << setw(11) << sh.regs->SR.part.M
                            << "Q  " << setw(11) << sh.regs->SR.part.Q
                     << hex << "I  " << setw(11) << sh.regs->SR.part.I
                     << dec << "S  " << setw(11) << sh.regs->SR.part.S
                            << "T  " <<             sh.regs->SR.part.T << endl;

  os << hex
#ifdef STL_LEFT_RIGHT
     << right
#endif
                     << "\tregs->GBR\t" << setw(10) << sh.regs->GBR
                     << "\tregs->MACH\t" << setw(10) << sh.regs->MACH
                     << "\tregs->PR\t" << setw(10) << sh.regs->PR << endl
                     << "\tregs->VBR\t" << setw(10) << sh.regs->VBR
                     << "\tregs->MACL\t" << setw(10) << sh.regs->MACL
                     << "\tregs->PC\t" << setw(10) << sh.regs->PC << endl;

  return os;
}
#endif

void undecoded(SuperH * sh) {
#ifdef DYNAREC
#ifndef _arch_dreamcast
        if (sh->isslave)
           throw IllegalOpcode("SSH2", sh->instruction, sh->sh2reg[regs->PC].value);
        else
           throw IllegalOpcode("MSH2", sh->instruction, sh->sh2reg[regs->PC].value);
#else
	printf("Illegal Opcode: 0x%8x, regs->PC: 0x%8x\n", sh->instruction, sh->sh2reg[regs->PC].value);
	exit(-1);
#endif
#else
        if (sh->isslave)
        {
           int vectnum;

           fprintf(stderr, "Slave SH2 Illegal Opcode: %04X, regs->PC: %08X. Jumping to Exception Service Routine.\n", sh->instruction, sh->regs->PC);

           // Save regs->SR on stack
           sh->regs->R[15]-=4;
           sh->memoire->setLong(sh->regs->R[15],sh->regs->SR.all);

           // Save regs->PC on stack
           sh->regs->R[15]-=4;
           sh->memoire->setLong(sh->regs->R[15],sh->regs->PC + 2);

           // What caused the exception? The delay slot or a general instruction?
           // 4 for General Instructions, 6 for delay slot
           vectnum = 4; //  Fix me

           // Jump to Exception service routine
           sh->regs->PC = sh->memoire->getLong(sh->regs->VBR+(vectnum<<2));
           sh->cycleCount++;
        }
        else
        {
           int vectnum;

           fprintf(stderr, "Master SH2 Illegal Opcode: %04X, regs->PC: %08X. Jumping to Exception Service Routine.\n", sh->instruction, sh->regs->PC);

           // Save regs->SR on stack
           sh->regs->R[15]-=4;
           sh->memoire->setLong(sh->regs->R[15],sh->regs->SR.all);

           // Save regs->PC on stack
           sh->regs->R[15]-=4;
           sh->memoire->setLong(sh->regs->R[15],sh->regs->PC + 2);

           // What caused the exception? The delay slot or a general instruction?
           // 4 for General Instructions, 6 for delay slot
           vectnum = 4; //  Fix me

           // Jump to Exception service routine
           sh->regs->PC = sh->memoire->getLong(sh->regs->VBR+(vectnum<<2));
           sh->cycleCount++;
        }
#endif
}

void add(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] += sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void addi(SuperH * sh) {
  long source = (long)(signed char)Instruction::cd(sh->instruction);
  long dest = Instruction::b(sh->instruction);

  sh->regs->R[dest] += source;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void addc(SuperH * sh) {
  unsigned long tmp0, tmp1;
  long source = Instruction::c(sh->instruction);
  long dest = Instruction::b(sh->instruction);

  tmp1 = sh->regs->R[source] + sh->regs->R[dest];
  tmp0 = sh->regs->R[dest];

  sh->regs->R[dest] = tmp1 + sh->regs->SR.part.T;
  if (tmp0 > tmp1)
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  if (tmp1 > sh->regs->R[dest])
    sh->regs->SR.part.T = 1;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void addv(SuperH * sh) {
  long i,j,k;
  long source = Instruction::c(sh->instruction);
  long dest = Instruction::b(sh->instruction);

  if ((long) sh->regs->R[dest] >= 0) i = 0; else i = 1;
  //i = ((long) R[dest] < 0);
  
  if ((long) sh->regs->R[source] >= 0) j = 0; else j = 1;
  //j = ((long) R[source] < 0);
  
  j += i;
  sh->regs->R[dest] += sh->regs->R[source];

  if ((long) sh->regs->R[dest] >= 0) k = 0; else k = 1;
  //k = ((long) R[dest] < 0);

  k += j;
  
  if (j == 0 || j == 2)
    if (k == 1) sh->regs->SR.part.T = 1;
    else sh->regs->SR.part.T = 0;
  else sh->regs->SR.part.T = 0;
  //regs->SR.part.T = ((j == 0 || j == 2) && (k == 1));
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void y_and(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] &= sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void andi(SuperH * sh) {
  sh->regs->R[0] &= Instruction::cd(sh->instruction);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void andm(SuperH * sh) {
  long temp;
  long source = Instruction::cd(sh->instruction);

  temp = (long) sh->memoire->getByte(sh->regs->GBR + sh->regs->R[0]);
  temp &= source;
  sh->memoire->setByte((sh->regs->GBR + sh->regs->R[0]),temp);
  sh->regs->PC += 2;
  sh->cycleCount += 3;
}
 
void bf(SuperH * sh) { // FIXME peut etre amelioré
  long disp;
  long d = Instruction::cd(sh->instruction);

  disp = (long)(signed char)d;

  if (sh->regs->SR.part.T == 0) {
    sh->regs->PC = sh->regs->PC+(disp<<1)+4;
    sh->cycleCount += 3;
  }
  else {
    sh->regs->PC+=2;
    sh->cycleCount++;
  }
}

void bfs(SuperH * sh) { // FIXME peut être amélioré
  long disp;
  unsigned long temp;
  long d = Instruction::cd(sh->instruction);

  temp = sh->regs->PC;
  disp = (long)(signed char)d;

  if (sh->regs->SR.part.T == 0) {
    sh->regs->PC = sh->regs->PC + (disp << 1) + 4;

    sh->cycleCount += 2;
    sh->delay(temp + 2);
  }
  else {
    sh->regs->PC += 2;
    sh->cycleCount++;
  }
}

void bra(SuperH * sh) {
  long disp = Instruction::bcd(sh->instruction);
  unsigned long temp;

  temp = sh->regs->PC;
  if ((disp&0x800) != 0) disp |= 0xFFFFF000;
  sh->regs->PC = sh->regs->PC + (disp<<1) + 4;

  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void braf(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::b(sh->instruction);

  temp = sh->regs->PC;
  sh->regs->PC += sh->regs->R[m] + 4; 

  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void bsr(SuperH * sh) {
  unsigned long temp;
  long disp = Instruction::bcd(sh->instruction);

  temp = sh->regs->PC;
  if ((disp&0x800) != 0) disp |= 0xFFFFF000;
  sh->regs->PR = sh->regs->PC + 4;
  sh->regs->PC = sh->regs->PC+(disp<<1) + 4;

  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void bsrf(SuperH * sh) {
  unsigned long temp = sh->regs->PC;
  sh->regs->PR = sh->regs->PC + 4;
  sh->regs->PC += sh->regs->R[Instruction::b(sh->instruction)] + 4;
  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void bt(SuperH * sh) { // FIXME ya plus rapide
  long disp;
  long d = Instruction::cd(sh->instruction);

  disp = (long)(signed char)d;
  if (sh->regs->SR.part.T == 1) {
    sh->regs->PC = sh->regs->PC+(disp<<1)+4;
    sh->cycleCount += 3;
  }
  else {
    sh->regs->PC += 2;
    sh->cycleCount++;
  }
}

void bts(SuperH * sh) {
  long disp;
  unsigned long temp;
  long d = Instruction::cd(sh->instruction);
  
  if (sh->regs->SR.part.T) {
    temp = sh->regs->PC;
    disp = (long)(signed char)d;
    sh->regs->PC += (disp << 1) + 4;
    sh->cycleCount += 2;
    sh->delay(temp + 2);
  }
  else {
    sh->regs->PC+=2;
    sh->cycleCount++;
  }
}

void clrmac(SuperH * sh) {
  sh->regs->MACH = 0;
  sh->regs->MACL = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void clrt(SuperH * sh) {
  sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void cmpeq(SuperH * sh) {
  if (sh->regs->R[Instruction::b(sh->instruction)] == sh->regs->R[Instruction::c(sh->instruction)])
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void cmpge(SuperH * sh) {
  if ((long)sh->regs->R[Instruction::b(sh->instruction)] >=
		  (long)sh->regs->R[Instruction::c(sh->instruction)])
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void cmpgt(SuperH * sh) {
  if ((long)sh->regs->R[Instruction::b(sh->instruction)]>(long)sh->regs->R[Instruction::c(sh->instruction)])
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void cmphi(SuperH * sh) {
  if ((unsigned long)sh->regs->R[Instruction::b(sh->instruction)]>
		  (unsigned long)sh->regs->R[Instruction::c(sh->instruction)])
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void cmphs(SuperH * sh) {
  if ((unsigned long)sh->regs->R[Instruction::b(sh->instruction)]>=
		  (unsigned long)sh->regs->R[Instruction::c(sh->instruction)])
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void cmpim(SuperH * sh) {
  long imm;
  long i = Instruction::cd(sh->instruction);

  imm = (long)(signed char)i;

  if (sh->regs->R[0] == (unsigned long) imm) // FIXME: ouais ça doit être bon...
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void cmppl(SuperH * sh) {
  if ((long)sh->regs->R[Instruction::b(sh->instruction)]>0)
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void cmppz(SuperH * sh) {
  if ((long)sh->regs->R[Instruction::b(sh->instruction)]>=0)
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void cmpstr(SuperH * sh) {
  unsigned long temp;
  long HH,HL,LH,LL;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  temp=sh->regs->R[n]^sh->regs->R[m];
  HH = (temp>>24) & 0x000000FF;
  HL = (temp>>16) & 0x000000FF;
  LH = (temp>>8) & 0x000000FF;
  LL = temp & 0x000000FF;
  HH = HH && HL && LH && LL;
  if (HH == 0)
    sh->regs->SR.part.T = 1;
  else
    sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void div0s(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  if ((sh->regs->R[n]&0x80000000)==0)
    sh->regs->SR.part.Q = 0;
  else
    sh->regs->SR.part.Q = 1;
  if ((sh->regs->R[m]&0x80000000)==0)
    sh->regs->SR.part.M = 0;
  else
    sh->regs->SR.part.M = 1;
  sh->regs->SR.part.T = !(sh->regs->SR.part.M == sh->regs->SR.part.Q);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void div0u(SuperH * sh) {
  sh->regs->SR.part.M = sh->regs->SR.part.Q = sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void div1(SuperH * sh) {
  unsigned long tmp0;
  unsigned char old_q, tmp1;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  old_q = sh->regs->SR.part.Q;
  sh->regs->SR.part.Q = (unsigned char)((0x80000000 & sh->regs->R[n])!=0);
  sh->regs->R[n] <<= 1;
  sh->regs->R[n]|=(unsigned long) sh->regs->SR.part.T;
  switch(old_q){
  case 0:
    switch(sh->regs->SR.part.M){
    case 0:
      tmp0 = sh->regs->R[n];
      sh->regs->R[n] -= sh->regs->R[m];
      tmp1 = (sh->regs->R[n] > tmp0);
      switch(sh->regs->SR.part.Q){
      case 0:
	sh->regs->SR.part.Q = tmp1;
	break;
      case 1:
	sh->regs->SR.part.Q = (unsigned char) (tmp1 == 0);
	break;
      }
      break;
    case 1:
      tmp0 = sh->regs->R[n];
      sh->regs->R[n] += sh->regs->R[m];
      tmp1 = (sh->regs->R[n] < tmp0);
      switch(sh->regs->SR.part.Q){
      case 0:
	sh->regs->SR.part.Q = (unsigned char) (tmp1 == 0);
	break;
      case 1:
	sh->regs->SR.part.Q = tmp1;
	break;
      }
      break;
    }
    break;
  case 1:switch(sh->regs->SR.part.M){
  case 0:
    tmp0 = sh->regs->R[n];
    sh->regs->R[n] += sh->regs->R[m];
    tmp1 = (sh->regs->R[n] < tmp0);
    switch(sh->regs->SR.part.Q){
    case 0:
      sh->regs->SR.part.Q = tmp1;
      break;
    case 1:
      sh->regs->SR.part.Q = (unsigned char) (tmp1 == 0);
      break;
    }
    break;
  case 1:
    tmp0 = sh->regs->R[n];
    sh->regs->R[n] -= sh->regs->R[m];
    tmp1 = (sh->regs->R[n] > tmp0);
    switch(sh->regs->SR.part.Q){
    case 0:
      sh->regs->SR.part.Q = (unsigned char) (tmp1 == 0);
      break;
    case 1:
      sh->regs->SR.part.Q = tmp1;
      break;
    }
    break;
  }
  break;
  }
  sh->regs->SR.part.T = (sh->regs->SR.part.Q == sh->regs->SR.part.M);
  sh->regs->PC += 2;
  sh->cycleCount++;
}


void dmuls(SuperH * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  tempn = (long)sh->regs->R[n];
  tempm = (long)sh->regs->R[m];
  if (tempn < 0) tempn = 0 - tempn;
  if (tempm < 0) tempm = 0 - tempm;
  if ((long) (sh->regs->R[n] ^ sh->regs->R[m]) < 0) fnLmL = -1;
  else fnLmL = 0;
  
  temp1 = (unsigned long) tempn;
  temp2 = (unsigned long) tempm;

  RnL = temp1 & 0x0000FFFF;
  RnH = (temp1 >> 16) & 0x0000FFFF;
  RmL = temp2 & 0x0000FFFF;
  RmH = (temp2 >> 16) & 0x0000FFFF;
  
  temp0 = RmL * RnL;
  temp1 = RmH * RnL;
  temp2 = RmL * RnH;
  temp3 = RmH * RnH;

  Res2 = 0;
  Res1 = temp1 + temp2;
  if (Res1 < temp1) Res2 += 0x00010000;

  temp1 = (Res1 << 16) & 0xFFFF0000;
  Res0 = temp0 + temp1;
  if (Res0 < temp0) Res2++;
  
  Res2 = Res2 + ((Res1 >> 16) & 0x0000FFFF) + temp3;

  if (fnLmL < 0) {
    Res2 = ~Res2;
    if (Res0 == 0)
      Res2++;
    else
      Res0 =(~Res0) + 1;
  }
  sh->regs->MACH = Res2;
  sh->regs->MACL = Res0;
  sh->regs->PC += 2;
  sh->cycleCount += 2;
}

void dmulu(SuperH * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  RnL = sh->regs->R[n] & 0x0000FFFF;
  RnH = (sh->regs->R[n] >> 16) & 0x0000FFFF;
  RmL = sh->regs->R[m] & 0x0000FFFF;
  RmH = (sh->regs->R[m] >> 16) & 0x0000FFFF;

  temp0 = RmL * RnL;
  temp1 = RmH * RnL;
  temp2 = RmL * RnH;
  temp3 = RmH * RnH;
  
  Res2 = 0;
  Res1 = temp1 + temp2;
  if (Res1 < temp1) Res2 += 0x00010000;
  
  temp1 = (Res1 << 16) & 0xFFFF0000;
  Res0 = temp0 + temp1;
  if (Res0 < temp0) Res2++;
  
  Res2 = Res2 + ((Res1 >> 16) & 0x0000FFFF) + temp3;

  sh->regs->MACH = Res2;
  sh->regs->MACL = Res0;
  sh->regs->PC += 2;
  sh->cycleCount += 2;
}

void dt(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]--;
  if (sh->regs->R[n] == 0) sh->regs->SR.part.T = 1;
  else sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void extsb(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (unsigned long)(signed char)sh->regs->R[m];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void extsw(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (unsigned long)(signed short)sh->regs->R[m];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void extub(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (unsigned long)(unsigned char)sh->regs->R[m];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void extuw(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (unsigned long)(unsigned short)sh->regs->R[m];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void jmp(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::b(sh->instruction);

  temp=sh->regs->PC;
  sh->regs->PC = sh->regs->R[m];
  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void jsr(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::b(sh->instruction);

  temp = sh->regs->PC;
  sh->regs->PR = sh->regs->PC + 4;
  sh->regs->PC = sh->regs->R[m];
  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void ldcgbr(SuperH * sh) {
  sh->regs->GBR = sh->regs->R[Instruction::b(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void ldcmgbr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->regs->GBR = sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount += 3;
}

void ldcmsr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->regs->SR.all = sh->memoire->getLong(sh->regs->R[m]) & 0x000003F3;
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount += 3;
}

void ldcmvbr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->regs->VBR = sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount += 3;
}

void ldcsr(SuperH * sh) {
  sh->regs->SR.all = sh->regs->R[Instruction::b(sh->instruction)]&0x000003F3;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void ldcvbr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->regs->VBR = sh->regs->R[m];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void ldsmach(SuperH * sh) {
  sh->regs->MACH = sh->regs->R[Instruction::b(sh->instruction)];
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void ldsmacl(SuperH * sh) {
  sh->regs->MACL = sh->regs->R[Instruction::b(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void ldsmmach(SuperH * sh) {
  long m = Instruction::b(sh->instruction);
  sh->regs->MACH = sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void ldsmmacl(SuperH * sh) {
  long m = Instruction::b(sh->instruction);
  sh->regs->MACL = sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void ldsmpr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);
  sh->regs->PR = sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void ldspr(SuperH * sh) {
  sh->regs->PR = sh->regs->R[Instruction::b(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void macl(SuperH * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  tempn = (long) sh->memoire->getLong(sh->regs->R[n]);
  sh->regs->R[n] += 4;
  tempm = (long) sh->memoire->getLong(sh->regs->R[m]);
  sh->regs->R[m] += 4;

  if ((long) (tempn^tempm) < 0) fnLmL =- 1;
  else fnLmL = 0;
  if (tempn < 0) tempn = 0 - tempn;
  if (tempm < 0) tempm = 0 - tempm;

  temp1 = (unsigned long) tempn;
  temp2 = (unsigned long) tempm;

  RnL = temp1 & 0x0000FFFF;
  RnH = (temp1 >> 16) & 0x0000FFFF;
  RmL = temp2 & 0x0000FFFF;
  RmH = (temp2 >> 16) & 0x0000FFFF;

  temp0 = RmL * RnL;
  temp1 = RmH * RnL;
  temp2 = RmL * RnH;
  temp3 = RmH * RnH;
     
  Res2 = 0;
  Res1 = temp1 + temp2;
  if (Res1 < temp1) Res2 += 0x00010000;

  temp1 = (Res1 << 16) & 0xFFFF0000;
  Res0 = temp0 + temp1;
  if (Res0 < temp0) Res2++;

  Res2=Res2+((Res1>>16)&0x0000FFFF)+temp3;

  if(fnLmL < 0){
    Res2=~Res2;
    if (Res0==0) Res2++;
    else Res0=(~Res0)+1;
  }
  if(sh->regs->SR.part.S == 1){
    Res0=sh->regs->MACL+Res0;
    if (sh->regs->MACL>Res0) Res2++;
    if (sh->regs->MACH & 0x00008000);
    else Res2 += sh->regs->MACH | 0xFFFF0000;
    Res2+=(sh->regs->MACH&0x0000FFFF);
    if(((long)Res2<0)&&(Res2<0xFFFF8000)){
      Res2=0x00008000;
      Res0=0x00000000;
    }
    if(((long)Res2>0)&&(Res2>0x00007FFF)){
      Res2=0x00007FFF;
      Res0=0xFFFFFFFF;
    };
    
    sh->regs->MACH=Res2;
    sh->regs->MACL=Res0;
  }
  else {
    Res0=sh->regs->MACL+Res0;
    if (sh->regs->MACL>Res0) Res2++;
    Res2+=sh->regs->MACH;

    sh->regs->MACH=Res2;
    sh->regs->MACL=Res0;
  }
  
  sh->regs->PC+=2;
  sh->cycleCount += 3;
}

void macw(SuperH * sh) {
  long tempm,tempn,dest,src,ans;
  unsigned long templ;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  tempn=(long) sh->memoire->getWord(sh->regs->R[n]);
  sh->regs->R[n]+=2;
  tempm=(long) sh->memoire->getWord(sh->regs->R[m]);
  sh->regs->R[m]+=2;
  templ=sh->regs->MACL;
  tempm=((long)(short)tempn*(long)(short)tempm);

  if ((long)sh->regs->MACL>=0) dest=0;
  else dest=1;
  if ((long)tempm>=0) {
    src=0;
    tempn=0;
  }
  else {
    src=1;
    tempn=0xFFFFFFFF;
  }
  src+=dest;
  sh->regs->MACL+=tempm;
  if ((long)sh->regs->MACL>=0) ans=0;
  else ans=1;
  ans+=dest;
  if (sh->regs->SR.part.S == 1) {
    if (ans==1) {
      if (src==0) sh->regs->MACL=0x7FFFFFFF;
      if (src==2) sh->regs->MACL=0x80000000;
    }
  }
  else {
    sh->regs->MACH+=tempn;
    if (templ>sh->regs->MACL) sh->regs->MACH+=1;
  }
  sh->regs->PC+=2;
  sh->cycleCount += 3;
}

void mov(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)]=sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void mova(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->regs->R[0]=((sh->regs->PC+4)&0xFFFFFFFC)+(disp<<2);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movbl(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed char)sh->memoire->getByte(sh->regs->R[m]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movbl0(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed char)sh->memoire->getByte(sh->regs->R[m] + sh->regs->R[0]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movbl4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);

  sh->regs->R[0] = (long)(signed char)sh->memoire->getByte(sh->regs->R[m] + disp);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movblg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);
  
  sh->regs->R[0] = (long)(signed char)sh->memoire->getByte(sh->regs->GBR + disp);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movbm(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setByte((sh->regs->R[n] - 1),sh->regs->R[m]);
  sh->regs->R[n] -= 1;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movbp(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed char)sh->memoire->getByte(sh->regs->R[m]);
  if (n != m)
    sh->regs->R[m] += 1;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movbs(SuperH * sh) {
  sh->memoire->setByte(sh->regs->R[Instruction::b(sh->instruction)],
	sh->regs->R[Instruction::c(sh->instruction)]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movbs0(SuperH * sh) {
  sh->memoire->setByte(sh->regs->R[Instruction::b(sh->instruction)] + sh->regs->R[0],
	sh->regs->R[Instruction::c(sh->instruction)]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movbs4(SuperH * sh) {
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::c(sh->instruction);

  sh->memoire->setByte(sh->regs->R[n]+disp,sh->regs->R[0]);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movbsg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->memoire->setByte(sh->regs->GBR + disp,sh->regs->R[0]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movi(SuperH * sh) {
  long i = Instruction::cd(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed char)i;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movli(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = sh->memoire->getLong(((sh->regs->PC + 4) & 0xFFFFFFFC) + (disp << 2));
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movll(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] =
	sh->memoire->getLong(sh->regs->R[Instruction::c(sh->instruction)]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movll0(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] =
	sh->memoire->getLong(sh->regs->R[Instruction::c(sh->instruction)] + sh->regs->R[0]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movll4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = sh->memoire->getLong(sh->regs->R[m] + (disp << 2));
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movllg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->regs->R[0] = sh->memoire->getLong(sh->regs->GBR + (disp << 2));
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movlm(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setLong(sh->regs->R[n] - 4,sh->regs->R[m]);
  sh->regs->R[n] -= 4;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movlp(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = sh->memoire->getLong(sh->regs->R[m]);
  if (n != m) sh->regs->R[m] += 4;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movls(SuperH * sh) {
  sh->memoire->setLong(sh->regs->R[Instruction::b(sh->instruction)],
	sh->regs->R[Instruction::c(sh->instruction)]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movls0(SuperH * sh) {
  sh->memoire->setLong(sh->regs->R[Instruction::b(sh->instruction)] + sh->regs->R[0],
	sh->regs->R[Instruction::c(sh->instruction)]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movls4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setLong(sh->regs->R[n]+(disp<<2),sh->regs->R[m]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movlsg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->memoire->setLong(sh->regs->GBR+(disp<<2),sh->regs->R[0]);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movt(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] = (0x00000001 & sh->regs->SR.all);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movwi(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed short)sh->memoire->getWord(sh->regs->PC + (disp<<1) + 4);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movwl(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed short)sh->memoire->getWord(sh->regs->R[m]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movwl0(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed short)sh->memoire->getWord(sh->regs->R[m]+sh->regs->R[0]);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movwl4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);

  sh->regs->R[0] = (long)(signed short)sh->memoire->getWord(sh->regs->R[m]+(disp<<1));
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movwlg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->regs->R[0] = (long)(signed short)sh->memoire->getWord(sh->regs->GBR+(disp<<1));
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movwm(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setWord(sh->regs->R[n] - 2,sh->regs->R[m]);
  sh->regs->R[n] -= 2;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movwp(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->R[n] = (long)(signed short)sh->memoire->getWord(sh->regs->R[m]);
  if (n != m)
    sh->regs->R[m] += 2;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movws(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setWord(sh->regs->R[n],sh->regs->R[m]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void movws0(SuperH * sh) {
  sh->memoire->setWord(sh->regs->R[Instruction::b(sh->instruction)] + sh->regs->R[0],
	sh->regs->R[Instruction::c(sh->instruction)]);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movws4(SuperH * sh) {
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::c(sh->instruction);

  sh->memoire->setWord(sh->regs->R[n]+(disp<<1),sh->regs->R[0]);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void movwsg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->memoire->setWord(sh->regs->GBR+(disp<<1),sh->regs->R[0]);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void mull(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->MACL = sh->regs->R[n] * sh->regs->R[m];
  sh->regs->PC+=2;
  sh->cycleCount += 2;
}

void muls(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->MACL = ((long)(short)sh->regs->R[n]*(long)(short)sh->regs->R[m]);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void mulu(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->regs->MACL = ((unsigned long)(unsigned short)sh->regs->R[n]
	*(unsigned long)(unsigned short)sh->regs->R[m]);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void neg(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)]=0-sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void negc(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  temp=0-sh->regs->R[m];
  sh->regs->R[n] = temp - sh->regs->SR.part.T;
  if (0 < temp) sh->regs->SR.part.T=1;
  else sh->regs->SR.part.T=0;
  if (temp < sh->regs->R[n]) sh->regs->SR.part.T=1;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void nop(SuperH * sh) {
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void y_not(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] = ~sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void y_or(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] |= sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void ori(SuperH * sh) {
  sh->regs->R[0] |= Instruction::cd(sh->instruction);
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void orm(SuperH * sh) {
  long temp;
  long source = Instruction::cd(sh->instruction);

  temp = (long) sh->memoire->getByte(sh->regs->GBR + sh->regs->R[0]);
  temp |= source;
  sh->memoire->setByte(sh->regs->GBR + sh->regs->R[0],temp);
  sh->regs->PC += 2;
  sh->cycleCount += 3;
}

void rotcl(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x80000000)==0) temp=0;
  else temp=1;
  sh->regs->R[n]<<=1;
  if (sh->regs->SR.part.T == 1) sh->regs->R[n]|=0x00000001;
  else sh->regs->R[n]&=0xFFFFFFFE;
  if (temp==1) sh->regs->SR.part.T=1;
  else sh->regs->SR.part.T=0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void rotcr(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x00000001)==0) temp=0;
  else temp=1;
  sh->regs->R[n]>>=1;
  if (sh->regs->SR.part.T == 1) sh->regs->R[n]|=0x80000000;
  else sh->regs->R[n]&=0x7FFFFFFF;
  if (temp==1) sh->regs->SR.part.T=1;
  else sh->regs->SR.part.T=0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void rotl(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x80000000)==0) sh->regs->SR.part.T=0;
  else sh->regs->SR.part.T=1;
  sh->regs->R[n]<<=1;
  if (sh->regs->SR.part.T==1) sh->regs->R[n]|=0x00000001;
  else sh->regs->R[n]&=0xFFFFFFFE;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void rotr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x00000001)==0) sh->regs->SR.part.T = 0;
  else sh->regs->SR.part.T = 1;
  sh->regs->R[n]>>=1;
  if (sh->regs->SR.part.T == 1) sh->regs->R[n]|=0x80000000;
  else sh->regs->R[n]&=0x7FFFFFFF;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void rte(SuperH * sh) {
  unsigned long temp;
  temp=sh->regs->PC;
  sh->regs->PC = sh->memoire->getLong(sh->regs->R[15]);
  sh->regs->R[15] += 4;
  sh->regs->SR.all = sh->memoire->getLong(sh->regs->R[15]) & 0x000003F3;
  sh->regs->R[15] += 4;
  sh->cycleCount += 4;
  sh->delay(temp + 2);
}

void rts(SuperH * sh) {
  unsigned long temp;
  
  temp = sh->regs->PC;
  sh->regs->PC = sh->regs->PR;

  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void sett(SuperH * sh) {
  sh->regs->SR.part.T = 1;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void shal(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  if ((sh->regs->R[n] & 0x80000000) == 0) sh->regs->SR.part.T = 0;
  else sh->regs->SR.part.T = 1;
  sh->regs->R[n] <<= 1;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void shar(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x00000001)==0) sh->regs->SR.part.T = 0;
  else sh->regs->SR.part.T = 1;
  if ((sh->regs->R[n]&0x80000000)==0) temp = 0;
  else temp = 1;
  sh->regs->R[n] >>= 1;
  if (temp == 1) sh->regs->R[n] |= 0x80000000;
  else sh->regs->R[n] &= 0x7FFFFFFF;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void shll(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x80000000)==0) sh->regs->SR.part.T=0;
  else sh->regs->SR.part.T=1;
  sh->regs->R[n]<<=1;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void shll2(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] <<= 2;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void shll8(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)]<<=8;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void shll16(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)]<<=16;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void shlr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->regs->R[n]&0x00000001)==0) sh->regs->SR.part.T=0;
  else sh->regs->SR.part.T=1;
  sh->regs->R[n]>>=1;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void shlr2(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]>>=2;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void shlr8(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]>>=8;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void shlr16(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]>>=16;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void stcgbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]=sh->regs->GBR;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void stcmgbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]-=4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->GBR);
  sh->regs->PC+=2;
  sh->cycleCount += 2;
}

void stcmsr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]-=4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->SR.all);
  sh->regs->PC+=2;
  sh->cycleCount += 2;
}

void stcmvbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]-=4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->VBR);
  sh->regs->PC+=2;
  sh->cycleCount += 2;
}

void stcsr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n] = sh->regs->SR.all;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void stcvbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]=sh->regs->VBR;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void stsmach(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]=sh->regs->MACH;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void stsmacl(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]=sh->regs->MACL;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void stsmmach(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n] -= 4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->MACH); 
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void stsmmacl(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n] -= 4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->MACL);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void stsmpr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n] -= 4;
  sh->memoire->setLong(sh->regs->R[n],sh->regs->PR);
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void stspr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n] = sh->regs->PR;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void sub(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  sh->regs->R[n]-=sh->regs->R[m];
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void subc(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  unsigned long tmp0,tmp1;
  
  tmp1 = sh->regs->R[n] - sh->regs->R[m];
  tmp0 = sh->regs->R[n];
  sh->regs->R[n] = tmp1 - sh->regs->SR.part.T;
  if (tmp0 < tmp1) sh->regs->SR.part.T = 1;
  else sh->regs->SR.part.T = 0;
  if (tmp1 < sh->regs->R[n]) sh->regs->SR.part.T = 1;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void subv(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  long dest,src,ans;

  if ((long)sh->regs->R[n]>=0) dest=0;
  else dest=1;
  if ((long)sh->regs->R[m]>=0) src=0;
  else src=1;
  src+=dest;
  sh->regs->R[n]-=sh->regs->R[m];
  if ((long)sh->regs->R[n]>=0) ans=0;
  else ans=1;
  ans+=dest;
  if (src==1) {
    if (ans==1) sh->regs->SR.part.T=1;
    else sh->regs->SR.part.T=0;
  }
  else sh->regs->SR.part.T=0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void swapb(SuperH * sh) {
  unsigned long temp0,temp1;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  temp0=sh->regs->R[m]&0xffff0000;
  temp1=(sh->regs->R[m]&0x000000ff)<<8;
  sh->regs->R[n]=(sh->regs->R[m]>>8)&0x000000ff;
  sh->regs->R[n]=sh->regs->R[n]|temp1|temp0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void swapw(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  temp=(sh->regs->R[m]>>16)&0x0000FFFF;
  sh->regs->R[n]=sh->regs->R[m]<<16;
  sh->regs->R[n]|=temp;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void tas(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  temp=(long) sh->memoire->getByte(sh->regs->R[n]);
  if (temp==0) sh->regs->SR.part.T=1;
  else sh->regs->SR.part.T=0;
  temp|=0x00000080;
  sh->memoire->setByte(sh->regs->R[n],temp);
  sh->regs->PC+=2;
  sh->cycleCount += 4;
}

void trapa(SuperH * sh) {
  long imm = Instruction::cd(sh->instruction);

  sh->regs->R[15]-=4;
  sh->memoire->setLong(sh->regs->R[15],sh->regs->SR.all);
  sh->regs->R[15]-=4;
  sh->memoire->setLong(sh->regs->R[15],sh->regs->PC + 2);
  sh->regs->PC = sh->memoire->getLong(sh->regs->VBR+(imm<<2));
  sh->cycleCount += 8;
}

void tst(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  if ((sh->regs->R[n]&sh->regs->R[m])==0) sh->regs->SR.part.T = 1;
  else sh->regs->SR.part.T = 0;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void tsti(SuperH * sh) {
  long temp;
  long i = Instruction::cd(sh->instruction);

  temp=sh->regs->R[0]&i;
  if (temp==0) sh->regs->SR.part.T = 1;
  else sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void tstm(SuperH * sh) {
  long temp;
  long i = Instruction::cd(sh->instruction);

  temp=(long) sh->memoire->getByte(sh->regs->GBR+sh->regs->R[0]);
  temp&=i;
  if (temp==0) sh->regs->SR.part.T = 1;
  else sh->regs->SR.part.T = 0;
  sh->regs->PC+=2;
  sh->cycleCount += 3;
}

void y_xor(SuperH * sh) {
  sh->regs->R[Instruction::b(sh->instruction)] ^= sh->regs->R[Instruction::c(sh->instruction)];
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void xori(SuperH * sh) {
  long source = Instruction::cd(sh->instruction);
  sh->regs->R[0] ^= source;
  sh->regs->PC += 2;
  sh->cycleCount++;
}

void xorm(SuperH * sh) {
  long source = Instruction::cd(sh->instruction);
  long temp;

  temp = (long) sh->memoire->getByte(sh->regs->GBR + sh->regs->R[0]);
  temp ^= source;
  sh->memoire->setByte(sh->regs->GBR + sh->regs->R[0],temp);
  sh->regs->PC += 2;
  sh->cycleCount += 3;
}

void xtrct(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  temp=(sh->regs->R[m]<<16)&0xFFFF0000;
  sh->regs->R[n]=(sh->regs->R[n]>>16)&0x0000FFFF;
  sh->regs->R[n]|=temp;
  sh->regs->PC+=2;
  sh->cycleCount++;
}

void sleep(SuperH * sh) {
  //cerr << "SLEEP" << endl;
  sh->cycleCount += 3;
}

SuperH::opcode SuperH::decode(void) {
  
  switch (Instruction::a(instruction)) {
  case 0:
    switch (Instruction::d(instruction)) {
    case 2:
      switch (Instruction::c(instruction)) {
      case 0: return &stcsr;
      case 1: return &stcgbr;
      case 2: return &stcvbr;
      default: return &undecoded;
      }
     
    case 3:
      switch (Instruction::c(instruction)) {
      case 0: return &bsrf;
      case 2: return &braf;
      default: return &undecoded;
      }
     
    case 4: return &movbs0;
    case 5: return &movws0;
    case 6: return &movls0;
    case 7: return &mull;
    case 8:
      switch (Instruction::c(instruction)) {
      case 0: return &clrt;
      case 1: return &sett;
      case 2: return &clrmac;
      default: return &undecoded;
      }
     
    case 9:
      switch (Instruction::c(instruction)) {
      case 0: return &nop;
      case 1: return &div0u;
      case 2: return &movt;
      default: return &undecoded;
      }
     
    case 10:
      switch (Instruction::c(instruction)) {
      case 0: return &stsmach;
      case 1: return &stsmacl;
      case 2: return &stspr;
      default: return &undecoded;
      }
     
    case 11:
      switch (Instruction::c(instruction)) {
      case 0: return &rts;
      case 1: return &sleep;
      case 2: return &rte;
      default: return &undecoded;
      }
     
    case 12: return &movbl0;
    case 13: return &movwl0;
    case 14: return &movll0;
    case 15: return &macl;
    default: return &undecoded;
    }
   
  case 1: return &movls4;
  case 2:
    switch (Instruction::d(instruction)) {
    case 0: return &movbs;
    case 1: return &movws;
    case 2: return &movls;
    case 4: return &movbm;
    case 5: return &movwm;
    case 6: return &movlm;
    case 7: return &div0s;
    case 8: return &tst;
    case 9: return &y_and;
    case 10: return &y_xor;
    case 11: return &y_or;
    case 12: return &cmpstr;
    case 13: return &xtrct;
    case 14: return &mulu;
    case 15: return &muls;
    default: return &undecoded;
    }
   
  case 3:
    switch(Instruction::d(instruction)) {
    case 0:  return &cmpeq;
    case 2:  return &cmphs;
    case 3:  return &cmpge;
    case 4:  return &div1;
    case 5:  return &dmulu;
    case 6:  return &cmphi;
    case 7:  return &cmpgt;
    case 8:  return &sub;
    case 10: return &subc;
    case 11: return &subv;
    case 12: return &add;
    case 13: return &dmuls;
    case 14: return &addc;
    case 15: return &addv;
    default: return &undecoded;
    }
   
  case 4:
    switch(Instruction::d(instruction)) {
    case 0:
      switch(Instruction::c(instruction)) {
      case 0: return &shll;
      case 1: return &dt;
      case 2: return &shal;
      default: return &undecoded;
      }
     
    case 1:
      switch(Instruction::c(instruction)) {
      case 0: return &shlr;
      case 1: return &cmppz;
      case 2: return &shar;
      default: return &undecoded;
      }
     
    case 2:
      switch(Instruction::c(instruction)) {
      case 0: return &stsmmach;
      case 1: return &stsmmacl;
      case 2: return &stsmpr;
      default: return &undecoded;
      }
     
    case 3:
      switch(Instruction::c(instruction)) {
      case 0: return &stcmsr;
      case 1: return &stcmgbr;
      case 2: return &stcmvbr;
      default: return &undecoded;
      }
     
    case 4:
      switch(Instruction::c(instruction)) {
      case 0: return &rotl;
      case 2: return &rotcl;
      default: return &undecoded;
      }
     
    case 5:
      switch(Instruction::c(instruction)) {
      case 0: return &rotr;
      case 1: return &cmppl;
      case 2: return &rotcr;
      default: return &undecoded;
      }
     
    case 6:
      switch(Instruction::c(instruction)) {
      case 0: return &ldsmmach;
      case 1: return &ldsmmacl;
      case 2: return &ldsmpr;
      default: return &undecoded;
      }
     
    case 7:
      switch(Instruction::c(instruction)) {
      case 0: return &ldcmsr;
      case 1: return &ldcmgbr;
      case 2: return &ldcmvbr;
      default: return &undecoded;
      }
     
    case 8:
      switch(Instruction::c(instruction)) {
      case 0: return &shll2;
      case 1: return &shll8;
      case 2: return &shll16;
      default: return &undecoded;
      }
     
    case 9:
      switch(Instruction::c(instruction)) {
      case 0: return &shlr2;
      case 1: return &shlr8;
      case 2: return &shlr16;
      default: return &undecoded;
      }
     
    case 10:
      switch(Instruction::c(instruction)) {
      case 0: return &ldsmach;
      case 1: return &ldsmacl;
      case 2: return &ldspr;
      default: return &undecoded;
      }
     
    case 11:
      switch(Instruction::c(instruction)) {
      case 0: return &jsr;
      case 1: return &tas;
      case 2: return &jmp;
      default: return &undecoded;
      }
     
    case 14:
      switch(Instruction::c(instruction)) {
      case 0: return &ldcsr;
      case 1: return &ldcgbr;
      case 2: return &ldcvbr;
      default: return &undecoded;
      }
     
    case 15: return &macw;
    default: return &undecoded;
    }
   
  case 5: return &movll4;
  case 6:
    switch (Instruction::d(instruction)) {
    case 0:  return &movbl;
    case 1:  return &movwl;
    case 2:  return &movll;
    case 3:  return &mov;
    case 4:  return &movbp;
    case 5:  return &movwp;
    case 6:  return &movlp;
    case 7:  return &y_not;
    case 8:  return &swapb;
    case 9:  return &swapw;
    case 10: return &negc;
    case 11: return &neg;
    case 12: return &extub;
    case 13: return &extuw;
    case 14: return &extsb;
    case 15: return &extsw;
    }
   
  case 7: return &addi;
  case 8:
    switch (Instruction::b(instruction)) {
    case 0:  return &movbs4;
    case 1:  return &movws4;
    case 4:  return &movbl4;
    case 5:  return &movwl4;
    case 8:  return &cmpim;
    case 9:  return &bt;
    case 11: return &bf;
    case 13: return &bts;
    case 15: return &bfs;
    default: return &undecoded;
    }
   
  case 9: return &movwi;
  case 10: return &bra;
  case 11: return &bsr;
  case 12:
    switch(Instruction::b(instruction)) {
    case 0:  return &movbsg;
    case 1:  return &movwsg;
    case 2:  return &movlsg;
    case 3:  return &trapa;
    case 4:  return &movblg;
    case 5:  return &movwlg;
    case 6:  return &movllg;
    case 7:  return &mova;
    case 8:  return &tsti;
    case 9:  return &andi;
    case 10: return &xori;
    case 11: return &ori;
    case 12: return &tstm;
    case 13: return &andm;
    case 14: return &xorm;
    case 15: return &orm;
    }
   
  case 13: return &movli;
  case 14: return &movi;
  default: return &undecoded;
  }
}

void SuperH::GetRegisters(sh2regs_struct * r) {
  if (r != NULL) {
    memcpy(r->R, regs->R, sizeof(regs->R));
    r->SR.all = regs->SR.all;
    r->GBR = regs->GBR;
    r->VBR = regs->VBR;
    r->MACH = regs->MACH;
    r->MACL = regs->MACL;
    r->PR = regs->PR;
    r->PC = regs->PC;
  }
}

void SuperH::SetRegisters(sh2regs_struct * r) {
  if (regs != NULL) {
    memcpy(regs->R, r->R, sizeof(regs->R));
    regs->SR.all = r->SR.all;
    regs->GBR = r->GBR;
    regs->VBR = r->VBR;
    regs->MACH = r->MACH;
    regs->MACL = r->MACL;
    regs->PR = r->PR;
    regs->PC = r->PC;
  }
}

void SuperH::SetBreakpointCallBack(void (*func)(bool, unsigned long)) {
   BreakpointCallBack = func;
}

int SuperH::AddCodeBreakpoint(unsigned long addr) {
  if (numcodebreakpoints < MAX_BREAKPOINTS) {
     codebreakpoint[numcodebreakpoints].addr = addr;
     numcodebreakpoints++;

     return 0;
  }

  return -1;
}

int SuperH::DelCodeBreakpoint(unsigned long addr) {
  if (numcodebreakpoints > 0) {
     for (int i = 0; i < numcodebreakpoints; i++) {
        if (codebreakpoint[i].addr == addr)
        {
           codebreakpoint[i].addr = 0xFFFFFFFF;
           codebreakpoint[i].oldopcode = 0xFFFF;
           SortCodeBreakpoints();
           numcodebreakpoints--;
           return 0;
        }
     }
  }

  return -1;
}

codebreakpoint_struct *SuperH::GetBreakpointList() {
  return codebreakpoint;
}

void SuperH::ClearCodeBreakpoints() {
     for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        codebreakpoint[i].addr = 0xFFFFFFFF;
        codebreakpoint[i].oldopcode = 0xFFFF;
     }

     numcodebreakpoints = 0;
}

void SuperH::SortCodeBreakpoints() {
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

           tmp = codebreakpoint[i].oldopcode;
           codebreakpoint[i].oldopcode = codebreakpoint[i2].oldopcode;
           codebreakpoint[i2].oldopcode = tmp;
        }
     }
  }
}
