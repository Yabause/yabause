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

  reset();

  for(unsigned short i = 0;i < 0xFFFF;i++) {
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
  SR.partie.T = SR.partie.S = SR.partie.Q = SR.partie.M = 0;
  SR.partie.I = 0xF;
  SR.partie.inutile1 = 0;
  SR.partie.inutile2 = 0;
  VBR = 0;
  for(int i = 0;i < 16;i++) R[i] = 0;

  // reset interrupts
  while ( !interrupts.empty() ) {
        interrupts.pop();
  }

  ((Onchip*)onchip)->reset();
 
  _delai = 0;

#ifdef DEBUG
  verbose = 0;
#endif
  timing = 0;
}

void SuperH::setMemory(SaturnMemory *mem) {
         memoire = mem;
         PC = memoire->getLong(VBR);
         R[15] = memoire->getLong(VBR + 4);
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
	if (sh->PC & 0x6000000) {
		sh->instruction = readWord(sh->memoire->ramHigh, sh->PC);
	}
	else {
                sh->instruction = readWord(sh->memoire->rom, sh->PC);
	}
        (*sh->opcodes[sh->instruction])(sh);
}

void SuperH::executer(void) {
/*
  if (_delai) {
    unsigned long tmp = PC;
    PC = _delai;
    _executer(this);
    PC = tmp;
    _delai = 0;
  }
  else {
*/
/*
    if ( !interrupts.empty() ) {
      Interrupt interrupt = interrupts.top();
      if (interrupt.level() > SR.partie.I) {
        interrupts.pop();

        R[15] -= 4;
        memoire->setLong(R[15], SR.tout);
        R[15] -= 4;
        memoire->setLong(R[15], PC);
        SR.partie.I = interrupt.level();
        PC = memoire->getLong(VBR + (interrupt.vector() << 2));
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
	switch((addr >> 19) & 0xFF) {
		case 0:
			instruction = readWord(memoire->rom, addr);
			break;
		case 0xC0:
			instruction = readWord(memoire->ramHigh, addr);
			break;
		default:
			instruction = memoire->getWord(addr);
	}
        (*opcodes[instruction])(this);
        PC -= 2;
}

/*
void SuperH::_executer(void) {
//        if (isslave) cerr << "PC=" << PC << endl;
        unsigned long tmp = (PC >> 19) & 0xFF;
        switch(tmp) {
                case 0:
                        // rom
                        instruction = readWord(memoire->rom, PC);
                        break;
                case 0xC0:
                        // work ram high
                        instruction = readWord(memoire->ramHigh, PC);
                        break;
                default:
                        cerr << hex << "coin = " <<  tmp << endl;
                        instruction = memoire->getWord(PC);
                        break;
        }
	if (PC & 0x6000000) {
		instruction = readWord(memoire->ramHigh, PC);
	}
	else {
                instruction = readWord(memoire->rom, PC);
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
      if (interrupt.level() > SR.partie.I) {
        interrupts.pop();

        R[15] -= 4;
        memoire->setLong(R[15], SR.tout);
        R[15] -= 4;
        memoire->setLong(R[15], PC);
        SR.partie.I = interrupt.level();
        PC = memoire->getLong(VBR + (interrupt.vector() << 2));
      }
    }
        while(cycleCount < cc) {
           // Make sure it isn't one of our breakpoints
           for (int i=0; i < numcodebreakpoints; i++) {
              if (PC == codebreakpoint[i].addr ||
                  _delai == codebreakpoint[i].addr &&
                  !inbreakpoint) {
                 inbreakpoint = true;
                 if (BreakpointCallBack) BreakpointCallBack(isslave, codebreakpoint[i].addr);
                 inbreakpoint = false;
              }
           }

           //_executer(this);
	switch((PC >> 19) & 0xFF) {
		case 0:
			instruction = readWord(memoire->rom, PC);
			break;
		case 0xC0:
			instruction = readWord(memoire->ramHigh, PC);
			break;
		default:
			instruction = memoire->getWord(PC);
	}
        (*opcodes[instruction])(this);
        }

        ((Onchip *)onchip)->runFRT(cc);
}

void SuperH::step(void) {
   unsigned long tmp = PC;

   // Execute 1 instruction
   runCycles(cycleCount+1);

   // Sometimes it doesn't always seem to execute one instruction,
   // let's make sure it did
   if (tmp == PC)
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
		<< setw(10) << sh.R[i] << "  ";
    }
    os << "\n";
  }

  os << dec
#ifdef STL_LEFT_RIGHT
     << left
#endif
                     << "\tSR    M  " << setw(11) << sh.SR.partie.M
                            << "Q  " << setw(11) << sh.SR.partie.Q
                     << hex << "I  " << setw(11) << sh.SR.partie.I
                     << dec << "S  " << setw(11) << sh.SR.partie.S
                            << "T  " <<             sh.SR.partie.T << endl;

  os << hex
#ifdef STL_LEFT_RIGHT
     << right
#endif
                     << "\tGBR\t" << setw(10) << sh.GBR
                     << "\tMACH\t" << setw(10) << sh.MACH
                     << "\tPR\t" << setw(10) << sh.PR << endl
                     << "\tVBR\t" << setw(10) << sh.VBR
                     << "\tMACL\t" << setw(10) << sh.MACL
                     << "\tPC\t" << setw(10) << sh.PC << endl;

  return os;
}
#endif

void undecoded(SuperH * sh) {
#ifndef _arch_dreamcast
        if (sh->isslave)
           throw IllegalOpcode("SSH2", sh->instruction, sh->PC);
        else
           throw IllegalOpcode("MSH2", sh->instruction, sh->PC);
#else
	printf("Illegal Opcode: 0x%8x, PC: 0x%8x\n", sh->instruction, sh->PC);
	exit(-1);
#endif
}

void add(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)] += sh->R[Instruction::c(sh->instruction)];
  sh->PC += 2;
  sh->cycleCount++;
}

void addi(SuperH * sh) {
  long source = (long)(signed char)Instruction::cd(sh->instruction);
  long dest = Instruction::b(sh->instruction);

  sh->R[dest] += source;
  sh->PC += 2;
  sh->cycleCount++;
}

void addc(SuperH * sh) {
  unsigned long tmp0, tmp1;
  long source = Instruction::c(sh->instruction);
  long dest = Instruction::b(sh->instruction);

  tmp1 = sh->R[source] + sh->R[dest];
  tmp0 = sh->R[dest];

  sh->R[dest] = tmp1 + sh->SR.partie.T;
  if (tmp0 > tmp1)
    sh->SR.partie.T = 1;
  else
    sh->SR.partie.T = 0;
  if (tmp1 > sh->R[dest])
    sh->SR.partie.T = 1;
  sh->PC += 2;
  sh->cycleCount++;
}

void addv(SuperH * sh) {
  long i,j,k;
  long source = Instruction::c(sh->instruction);
  long dest = Instruction::b(sh->instruction);

  if ((long) sh->R[dest] >= 0) i = 0; else i = 1;
  //i = ((long) R[dest] < 0);
  
  if ((long) sh->R[source] >= 0) j = 0; else j = 1;
  //j = ((long) R[source] < 0);
  
  j += i;
  sh->R[dest] += sh->R[source];

  if ((long) sh->R[dest] >= 0) k = 0; else k = 1;
  //k = ((long) R[dest] < 0);

  k += j;
  
  if (j == 0 || j == 2)
    if (k == 1) sh->SR.partie.T = 1;
    else sh->SR.partie.T = 0;
  else sh->SR.partie.T = 0;
  //SR.partie.T = ((j == 0 || j == 2) && (k == 1));
  sh->PC += 2;
  sh->cycleCount++;
}

void y_and(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)] &= sh->R[Instruction::c(sh->instruction)];
  sh->PC += 2;
  sh->cycleCount++;
}

void andi(SuperH * sh) {
  sh->R[0] &= Instruction::cd(sh->instruction);
  sh->PC += 2;
  sh->cycleCount++;
}

void andm(SuperH * sh) {
  long temp;
  long source = Instruction::cd(sh->instruction);

  temp = (long) sh->memoire->getByte(sh->GBR + sh->R[0]);
  temp &= source;
  sh->memoire->setByte((sh->GBR + sh->R[0]),temp);
  sh->PC += 2;
  sh->cycleCount += 3;
}
 
void bf(SuperH * sh) { // FIXME peut etre amelioré
  long disp;
  long d = Instruction::cd(sh->instruction);

  disp = (long)(signed char)d;

  if (sh->SR.partie.T == 0) {
    sh->PC = sh->PC+(disp<<1)+4;
    sh->cycleCount += 3;
  }
  else {
    sh->PC+=2;
    sh->cycleCount++;
  }
}

void bfs(SuperH * sh) { // FIXME peut être amélioré
  long disp;
  unsigned long temp;
  long d = Instruction::cd(sh->instruction);

  temp = sh->PC;
  disp = (long)(signed char)d;

  if (sh->SR.partie.T == 0) {
    sh->PC = sh->PC + (disp << 1) + 4;

    //sh->_delai = temp + 2;
    sh->cycleCount += 2;
    sh->delay(temp + 2);
  }
  else {
    sh->PC += 2;
    sh->cycleCount++;
  }
}

void bra(SuperH * sh) {
  long disp = Instruction::bcd(sh->instruction);
  unsigned long temp;

  temp = sh->PC;
  if ((disp&0x800) != 0) disp |= 0xFFFFF000;
  //sh->_delai = sh->PC + 2;
  sh->PC = sh->PC + (disp<<1) + 4;

  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void braf(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::b(sh->instruction);

  temp = sh->PC;
  sh->PC += sh->R[m] + 4; 

  //sh->_delai = temp + 2;
  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void bsr(SuperH * sh) {
  unsigned long temp;
  long disp = Instruction::bcd(sh->instruction);

  temp = sh->PC;
  if ((disp&0x800) != 0) disp |= 0xFFFFF000;
  sh->PR = sh->PC + 4;
  //sh->_delai = sh->PC + 2;
  sh->PC = sh->PC+(disp<<1) + 4;

  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void bsrf(SuperH * sh) {
  unsigned long temp = sh->PC;
  sh->PR = sh->PC + 4;
  sh->_delai = sh->PC + 2;
  sh->PC += sh->R[Instruction::b(sh->instruction)] + 4;
  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void bt(SuperH * sh) { // FIXME ya plus rapide
  long disp;
  long d = Instruction::cd(sh->instruction);

  disp = (long)(signed char)d;
  if (sh->SR.partie.T == 1) {
    sh->PC = sh->PC+(disp<<1)+4;
    sh->cycleCount += 3;
  }
  else {
    sh->PC += 2;
    sh->cycleCount++;
  }
}

void bts(SuperH * sh) {
  long disp;
  unsigned long temp;
  long d = Instruction::cd(sh->instruction);
  
  if (sh->SR.partie.T) {
    temp = sh->PC;
    disp = (long)(signed char)d;
    sh->PC += (disp << 1) + 4;
    //sh->_delai = temp + 2;
    sh->cycleCount += 2;
    sh->delay(temp + 2);
  }
  else {
    sh->PC+=2;
    sh->cycleCount++;
  }
}

void clrmac(SuperH * sh) {
  sh->MACH = 0;
  sh->MACL = 0;
  sh->PC += 2;
  sh->cycleCount++;
}

void clrt(SuperH * sh) {
  sh->SR.partie.T = 0;
  sh->PC += 2;
  sh->cycleCount++;
}

void cmpeq(SuperH * sh) {
  if (sh->R[Instruction::b(sh->instruction)] == sh->R[Instruction::c(sh->instruction)])
    sh->SR.partie.T = 1;
  else
    sh->SR.partie.T = 0;
  sh->PC += 2;
  sh->cycleCount++;
}

void cmpge(SuperH * sh) {
  if ((long)sh->R[Instruction::b(sh->instruction)] >=
		  (long)sh->R[Instruction::c(sh->instruction)])
    sh->SR.partie.T = 1;
  else
    sh->SR.partie.T = 0;
  sh->PC += 2;
  sh->cycleCount++;
}

void cmpgt(SuperH * sh) {
  if ((long)sh->R[Instruction::b(sh->instruction)]>(long)sh->R[Instruction::c(sh->instruction)])
    sh->SR.partie.T = 1;
  else
    sh->SR.partie.T = 0;
  sh->PC+=2;
  sh->cycleCount++;
}

void cmphi(SuperH * sh) {
  if ((unsigned long)sh->R[Instruction::b(sh->instruction)]>
		  (unsigned long)sh->R[Instruction::c(sh->instruction)])
    sh->SR.partie.T = 1;
  else
    sh->SR.partie.T = 0;
  sh->PC+=2;
  sh->cycleCount++;
}

void cmphs(SuperH * sh) {
  if ((unsigned long)sh->R[Instruction::b(sh->instruction)]>=
		  (unsigned long)sh->R[Instruction::c(sh->instruction)])
    sh->SR.partie.T = 1;
  else
    sh->SR.partie.T = 0;
  sh->PC+=2;
  sh->cycleCount++;
}

void cmpim(SuperH * sh) {
  long imm;
  long i = Instruction::cd(sh->instruction);

  imm = (long)(signed char)i;

  if (sh->R[0] == (unsigned long) imm) // FIXME: ouais ça doit être bon...
    sh->SR.partie.T = 1;
  else
    sh->SR.partie.T = 0;
  sh->PC+=2;
  sh->cycleCount++;
}

void cmppl(SuperH * sh) {
  if ((long)sh->R[Instruction::b(sh->instruction)]>0)
    sh->SR.partie.T = 1;
  else
    sh->SR.partie.T = 0;
  sh->PC+=2;
  sh->cycleCount++;
}

void cmppz(SuperH * sh) {
  if ((long)sh->R[Instruction::b(sh->instruction)]>=0)
    sh->SR.partie.T = 1;
  else
    sh->SR.partie.T = 0;
  sh->PC+=2;
  sh->cycleCount++;
}

void cmpstr(SuperH * sh) {
  unsigned long temp;
  long HH,HL,LH,LL;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  temp=sh->R[n]^sh->R[m];
  HH = (temp>>24) & 0x000000FF;
  HL = (temp>>16) & 0x000000FF;
  LH = (temp>>8) & 0x000000FF;
  LL = temp & 0x000000FF;
  HH = HH && HL && LH && LL;
  if (HH == 0)
    sh->SR.partie.T = 1;
  else
    sh->SR.partie.T = 0;
  sh->PC+=2;
  sh->cycleCount++;
}

void div0s(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  if ((sh->R[n]&0x80000000)==0)
    sh->SR.partie.Q = 0;
  else
    sh->SR.partie.Q = 1;
  if ((sh->R[m]&0x80000000)==0)
    sh->SR.partie.M = 0;
  else
    sh->SR.partie.M = 1;
  sh->SR.partie.T = !(sh->SR.partie.M == sh->SR.partie.Q);
  sh->PC += 2;
  sh->cycleCount++;
}

void div0u(SuperH * sh) {
  sh->SR.partie.M = sh->SR.partie.Q = sh->SR.partie.T = 0;
  sh->PC += 2;
  sh->cycleCount++;
}

void div1(SuperH * sh) {
  unsigned long tmp0;
  unsigned char old_q, tmp1;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  old_q = sh->SR.partie.Q;
  sh->SR.partie.Q = (unsigned char)((0x80000000 & sh->R[n])!=0);
  sh->R[n] <<= 1;
  sh->R[n]|=(unsigned long) sh->SR.partie.T;
  switch(old_q){
  case 0:
    switch(sh->SR.partie.M){
    case 0:
      tmp0 = sh->R[n];
      sh->R[n] -= sh->R[m];
      tmp1 = (sh->R[n] > tmp0);
      switch(sh->SR.partie.Q){
      case 0:
	sh->SR.partie.Q = tmp1;
	break;
      case 1:
	sh->SR.partie.Q = (unsigned char) (tmp1 == 0);
	break;
      }
      break;
    case 1:
      tmp0 = sh->R[n];
      sh->R[n] += sh->R[m];
      tmp1 = (sh->R[n] < tmp0);
      switch(sh->SR.partie.Q){
      case 0:
	sh->SR.partie.Q = (unsigned char) (tmp1 == 0);
	break;
      case 1:
	sh->SR.partie.Q = tmp1;
	break;
      }
      break;
    }
    break;
  case 1:switch(sh->SR.partie.M){
  case 0:
    tmp0 = sh->R[n];
    sh->R[n] += sh->R[m];
    tmp1 = (sh->R[n] < tmp0);
    switch(sh->SR.partie.Q){
    case 0:
      sh->SR.partie.Q = tmp1;
      break;
    case 1:
      sh->SR.partie.Q = (unsigned char) (tmp1 == 0);
      break;
    }
    break;
  case 1:
    tmp0 = sh->R[n];
    sh->R[n] -= sh->R[m];
    tmp1 = (sh->R[n] > tmp0);
    switch(sh->SR.partie.Q){
    case 0:
      sh->SR.partie.Q = (unsigned char) (tmp1 == 0);
      break;
    case 1:
      sh->SR.partie.Q = tmp1;
      break;
    }
    break;
  }
  break;
  }
  sh->SR.partie.T = (sh->SR.partie.Q == sh->SR.partie.M);
  sh->PC += 2;
  sh->cycleCount++;
}


void dmuls(SuperH * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  tempn = (long)sh->R[n];
  tempm = (long)sh->R[m];
  if (tempn < 0) tempn = 0 - tempn;
  if (tempm < 0) tempm = 0 - tempm;
  if ((long) (sh->R[n] ^ sh->R[m]) < 0) fnLmL = -1;
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
  sh->MACH = Res2;
  sh->MACL = Res0;
  sh->PC += 2;
  sh->cycleCount += 2;
}

void dmulu(SuperH * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  RnL = sh->R[n] & 0x0000FFFF;
  RnH = (sh->R[n] >> 16) & 0x0000FFFF;
  RmL = sh->R[m] & 0x0000FFFF;
  RmH = (sh->R[m] >> 16) & 0x0000FFFF;

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

  sh->MACH = Res2;
  sh->MACL = Res0;
  sh->PC += 2;
  sh->cycleCount += 2;
}

void dt(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]--;
  if (sh->R[n] == 0) sh->SR.partie.T = 1;
  else sh->SR.partie.T = 0;
  sh->PC += 2;
  sh->cycleCount++;
}

void extsb(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (unsigned long)(signed char)sh->R[m];
  sh->PC += 2;
  sh->cycleCount++;
}

void extsw(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (unsigned long)(signed short)sh->R[m];
  sh->PC += 2;
  sh->cycleCount++;
}

void extub(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (unsigned long)(unsigned char)sh->R[m];
  sh->PC += 2;
  sh->cycleCount++;
}

void extuw(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (unsigned long)(unsigned short)sh->R[m];
  sh->PC += 2;
  sh->cycleCount++;
}

void jmp(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::b(sh->instruction);

  temp=sh->PC;
  sh->PC = sh->R[m];
  //sh->_delai = temp + 2;
  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void jsr(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::b(sh->instruction);

  temp = sh->PC;
  sh->PR = sh->PC + 4;
  //sh->_delai = sh->PC + 2;
  sh->PC = sh->R[m];
  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void ldcgbr(SuperH * sh) {
  sh->GBR = sh->R[Instruction::b(sh->instruction)];
  sh->PC += 2;
  sh->cycleCount++;
}

void ldcmgbr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->GBR = sh->memoire->getLong(sh->R[m]);
  sh->R[m] += 4;
  sh->PC += 2;
  sh->cycleCount += 3;
}

void ldcmsr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->SR.tout = sh->memoire->getLong(sh->R[m]) & 0x000003F3;
  sh->R[m] += 4;
  sh->PC += 2;
  sh->cycleCount += 3;
}

void ldcmvbr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->VBR = sh->memoire->getLong(sh->R[m]);
  sh->R[m] += 4;
  sh->PC += 2;
  sh->cycleCount += 3;
}

void ldcsr(SuperH * sh) {
  sh->SR.tout = sh->R[Instruction::b(sh->instruction)]&0x000003F3;
  sh->PC += 2;
  sh->cycleCount++;
}

void ldcvbr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);

  sh->VBR = sh->R[m];
  sh->PC += 2;
  sh->cycleCount++;
}

void ldsmach(SuperH * sh) {
  sh->MACH = sh->R[Instruction::b(sh->instruction)];
  sh->PC+=2;
  sh->cycleCount++;
}

void ldsmacl(SuperH * sh) {
  sh->MACL = sh->R[Instruction::b(sh->instruction)];
  sh->PC += 2;
  sh->cycleCount++;
}

void ldsmmach(SuperH * sh) {
  long m = Instruction::b(sh->instruction);
  sh->MACH = sh->memoire->getLong(sh->R[m]);
  sh->R[m] += 4;
  sh->PC += 2;
  sh->cycleCount++;
}

void ldsmmacl(SuperH * sh) {
  long m = Instruction::b(sh->instruction);
  sh->MACL = sh->memoire->getLong(sh->R[m]);
  sh->R[m] += 4;
  sh->PC += 2;
  sh->cycleCount++;
}

void ldsmpr(SuperH * sh) {
  long m = Instruction::b(sh->instruction);
  sh->PR = sh->memoire->getLong(sh->R[m]);
  sh->R[m] += 4;
  sh->PC += 2;
  sh->cycleCount++;
}

void ldspr(SuperH * sh) {
  sh->PR = sh->R[Instruction::b(sh->instruction)];
  sh->PC += 2;
  sh->cycleCount++;
}

void macl(SuperH * sh) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  tempn = (long) sh->memoire->getLong(sh->R[n]);
  sh->R[n] += 4;
  tempm = (long) sh->memoire->getLong(sh->R[m]);
  sh->R[m] += 4;

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
  if(sh->SR.partie.S == 1){
    Res0=sh->MACL+Res0;
    if (sh->MACL>Res0) Res2++;
    if (sh->MACH & 0x00008000);
    else Res2 += sh->MACH | 0xFFFF0000;
    Res2+=(sh->MACH&0x0000FFFF);
    if(((long)Res2<0)&&(Res2<0xFFFF8000)){
      Res2=0x00008000;
      Res0=0x00000000;
    }
    if(((long)Res2>0)&&(Res2>0x00007FFF)){
      Res2=0x00007FFF;
      Res0=0xFFFFFFFF;
    };
    
    sh->MACH=Res2;
    sh->MACL=Res0;
  }
  else {
    Res0=sh->MACL+Res0;
    if (sh->MACL>Res0) Res2++;
    Res2+=sh->MACH;

    sh->MACH=Res2;
    sh->MACL=Res0;
  }
  
  sh->PC+=2;
  sh->cycleCount += 3;
}

void macw(SuperH * sh) {
  long tempm,tempn,dest,src,ans;
  unsigned long templ;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  tempn=(long) sh->memoire->getWord(sh->R[n]);
  sh->R[n]+=2;
  tempm=(long) sh->memoire->getWord(sh->R[m]);
  sh->R[m]+=2;
  templ=sh->MACL;
  tempm=((long)(short)tempn*(long)(short)tempm);

  if ((long)sh->MACL>=0) dest=0;
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
  sh->MACL+=tempm;
  if ((long)sh->MACL>=0) ans=0;
  else ans=1;
  ans+=dest;
  if (sh->SR.partie.S == 1) {
    if (ans==1) {
      if (src==0) sh->MACL=0x7FFFFFFF;
      if (src==2) sh->MACL=0x80000000;
    }
  }
  else {
    sh->MACH+=tempn;
    if (templ>sh->MACL) sh->MACH+=1;
  }
  sh->PC+=2;
  sh->cycleCount += 3;
}

void mov(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)]=sh->R[Instruction::c(sh->instruction)];
  sh->PC+=2;
  sh->cycleCount++;
}

void mova(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->R[0]=((sh->PC+4)&0xFFFFFFFC)+(disp<<2);
  sh->PC+=2;
  sh->cycleCount++;
}

void movbl(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (long)(signed char)sh->memoire->getByte(sh->R[m]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movbl0(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (long)(signed char)sh->memoire->getByte(sh->R[m] + sh->R[0]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movbl4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);

  sh->R[0] = (long)(signed char)sh->memoire->getByte(sh->R[m] + disp);
  sh->PC+=2;
  sh->cycleCount++;
}

void movblg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);
  
  sh->R[0] = (long)(signed char)sh->memoire->getByte(sh->GBR + disp);
  sh->PC+=2;
  sh->cycleCount++;
}

void movbm(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setByte((sh->R[n] - 1),sh->R[m]);
  sh->R[n] -= 1;
  sh->PC += 2;
  sh->cycleCount++;
}

void movbp(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (long)(signed char)sh->memoire->getByte(sh->R[m]);
  if (n != m)
    sh->R[m] += 1;
  sh->PC += 2;
  sh->cycleCount++;
}

void movbs(SuperH * sh) {
  sh->memoire->setByte(sh->R[Instruction::b(sh->instruction)],
	sh->R[Instruction::c(sh->instruction)]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movbs0(SuperH * sh) {
  sh->memoire->setByte(sh->R[Instruction::b(sh->instruction)] + sh->R[0],
	sh->R[Instruction::c(sh->instruction)]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movbs4(SuperH * sh) {
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::c(sh->instruction);

  sh->memoire->setByte(sh->R[n]+disp,sh->R[0]);
  sh->PC+=2;
  sh->cycleCount++;
}

void movbsg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->memoire->setByte(sh->GBR + disp,sh->R[0]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movi(SuperH * sh) {
  long i = Instruction::cd(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (long)(signed char)i;
  sh->PC += 2;
  sh->cycleCount++;
}

void movli(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = sh->memoire->getLong(((sh->PC + 4) & 0xFFFFFFFC) + (disp << 2));
  sh->PC += 2;
  sh->cycleCount++;
}

void movll(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)] =
	sh->memoire->getLong(sh->R[Instruction::c(sh->instruction)]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movll0(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)] =
	sh->memoire->getLong(sh->R[Instruction::c(sh->instruction)] + sh->R[0]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movll4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = sh->memoire->getLong(sh->R[m] + (disp << 2));
  sh->PC += 2;
  sh->cycleCount++;
}

void movllg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->R[0] = sh->memoire->getLong(sh->GBR + (disp << 2));
  sh->PC+=2;
  sh->cycleCount++;
}

void movlm(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setLong(sh->R[n] - 4,sh->R[m]);
  sh->R[n] -= 4;
  sh->PC += 2;
  sh->cycleCount++;
}

void movlp(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = sh->memoire->getLong(sh->R[m]);
  if (n != m) sh->R[m] += 4;
  sh->PC += 2;
  sh->cycleCount++;
}

void movls(SuperH * sh) {
  sh->memoire->setLong(sh->R[Instruction::b(sh->instruction)],
	sh->R[Instruction::c(sh->instruction)]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movls0(SuperH * sh) {
  sh->memoire->setLong(sh->R[Instruction::b(sh->instruction)] + sh->R[0],
	sh->R[Instruction::c(sh->instruction)]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movls4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setLong(sh->R[n]+(disp<<2),sh->R[m]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movlsg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->memoire->setLong(sh->GBR+(disp<<2),sh->R[0]);
  sh->PC+=2;
  sh->cycleCount++;
}

void movt(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)] = (0x00000001 & sh->SR.tout);
  sh->PC += 2;
  sh->cycleCount++;
}

void movwi(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (long)(signed short)sh->memoire->getWord(sh->PC + (disp<<1) + 4);
  sh->PC+=2;
  sh->cycleCount++;
}

void movwl(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (long)(signed short)sh->memoire->getWord(sh->R[m]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movwl0(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (long)(signed short)sh->memoire->getWord(sh->R[m]+sh->R[0]);
  sh->PC+=2;
  sh->cycleCount++;
}

void movwl4(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long disp = Instruction::d(sh->instruction);

  sh->R[0] = (long)(signed short)sh->memoire->getWord(sh->R[m]+(disp<<1));
  sh->PC+=2;
  sh->cycleCount++;
}

void movwlg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->R[0] = (long)(signed short)sh->memoire->getWord(sh->GBR+(disp<<1));
  sh->PC += 2;
  sh->cycleCount++;
}

void movwm(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setWord(sh->R[n] - 2,sh->R[m]);
  sh->R[n] -= 2;
  sh->PC += 2;
  sh->cycleCount++;
}

void movwp(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->R[n] = (long)(signed short)sh->memoire->getWord(sh->R[m]);
  if (n != m)
    sh->R[m] += 2;
  sh->PC += 2;
  sh->cycleCount++;
}

void movws(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->memoire->setWord(sh->R[n],sh->R[m]);
  sh->PC += 2;
  sh->cycleCount++;
}

void movws0(SuperH * sh) {
  sh->memoire->setWord(sh->R[Instruction::b(sh->instruction)] + sh->R[0],
	sh->R[Instruction::c(sh->instruction)]);
  sh->PC+=2;
  sh->cycleCount++;
}

void movws4(SuperH * sh) {
  long disp = Instruction::d(sh->instruction);
  long n = Instruction::c(sh->instruction);

  sh->memoire->setWord(sh->R[n]+(disp<<1),sh->R[0]);
  sh->PC+=2;
  sh->cycleCount++;
}

void movwsg(SuperH * sh) {
  long disp = Instruction::cd(sh->instruction);

  sh->memoire->setWord(sh->GBR+(disp<<1),sh->R[0]);
  sh->PC+=2;
  sh->cycleCount++;
}

void mull(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->MACL = sh->R[n] * sh->R[m];
  sh->PC+=2;
  sh->cycleCount += 2;
}

void muls(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->MACL = ((long)(short)sh->R[n]*(long)(short)sh->R[m]);
  sh->PC += 2;
  sh->cycleCount++;
}

void mulu(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  sh->MACL = ((unsigned long)(unsigned short)sh->R[n]
	*(unsigned long)(unsigned short)sh->R[m]);
  sh->PC+=2;
  sh->cycleCount++;
}

void neg(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)]=0-sh->R[Instruction::c(sh->instruction)];
  sh->PC+=2;
  sh->cycleCount++;
}

void negc(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  
  temp=0-sh->R[m];
  sh->R[n] = temp - sh->SR.partie.T;
  if (0 < temp) sh->SR.partie.T=1;
  else sh->SR.partie.T=0;
  if (temp < sh->R[n]) sh->SR.partie.T=1;
  sh->PC+=2;
  sh->cycleCount++;
}

void nop(SuperH * sh) {
  sh->PC += 2;
  sh->cycleCount++;
}

void y_not(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)] = ~sh->R[Instruction::c(sh->instruction)];
  sh->PC += 2;
  sh->cycleCount++;
}

void y_or(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)] |= sh->R[Instruction::c(sh->instruction)];
  sh->PC += 2;
  sh->cycleCount++;
}

void ori(SuperH * sh) {
  sh->R[0] |= Instruction::cd(sh->instruction);
  sh->PC += 2;
  sh->cycleCount++;
}

void orm(SuperH * sh) {
  long temp;
  long source = Instruction::cd(sh->instruction);

  temp = (long) sh->memoire->getByte(sh->GBR + sh->R[0]);
  temp |= source;
  sh->memoire->setByte(sh->GBR + sh->R[0],temp);
  sh->PC += 2;
  sh->cycleCount += 3;
}

void rotcl(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  if ((sh->R[n]&0x80000000)==0) temp=0;
  else temp=1;
  sh->R[n]<<=1;
  if (sh->SR.partie.T == 1) sh->R[n]|=0x00000001;
  else sh->R[n]&=0xFFFFFFFE;
  if (temp==1) sh->SR.partie.T=1;
  else sh->SR.partie.T=0;
  sh->PC+=2;
  sh->cycleCount++;
}

void rotcr(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  if ((sh->R[n]&0x00000001)==0) temp=0;
  else temp=1;
  sh->R[n]>>=1;
  if (sh->SR.partie.T == 1) sh->R[n]|=0x80000000;
  else sh->R[n]&=0x7FFFFFFF;
  if (temp==1) sh->SR.partie.T=1;
  else sh->SR.partie.T=0;
  sh->PC+=2;
  sh->cycleCount++;
}

void rotl(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->R[n]&0x80000000)==0) sh->SR.partie.T=0;
  else sh->SR.partie.T=1;
  sh->R[n]<<=1;
  if (sh->SR.partie.T==1) sh->R[n]|=0x00000001;
  else sh->R[n]&=0xFFFFFFFE;
  sh->PC+=2;
  sh->cycleCount++;
}

void rotr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->R[n]&0x00000001)==0) sh->SR.partie.T = 0;
  else sh->SR.partie.T = 1;
  sh->R[n]>>=1;
  if (sh->SR.partie.T == 1) sh->R[n]|=0x80000000;
  else sh->R[n]&=0x7FFFFFFF;
  sh->PC+=2;
  sh->cycleCount++;
}

void rte(SuperH * sh) {
  unsigned long temp;
  temp=sh->PC;
  sh->PC = sh->memoire->getLong(sh->R[15]);
  sh->R[15] += 4;
  sh->SR.tout = sh->memoire->getLong(sh->R[15]) & 0x000003F3;
  sh->R[15] += 4;
  //sh->_delai = temp + 2;
  sh->cycleCount += 4;
  sh->delay(temp + 2);
}

void rts(SuperH * sh) {
  unsigned long temp;
  
  temp = sh->PC;
  sh->PC = sh->PR;

  //sh->_delai = temp + 2;
  sh->cycleCount += 2;
  sh->delay(temp + 2);
}

void sett(SuperH * sh) {
  sh->SR.partie.T = 1;
  sh->PC += 2;
  sh->cycleCount++;
}

void shal(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  if ((sh->R[n] & 0x80000000) == 0) sh->SR.partie.T = 0;
  else sh->SR.partie.T = 1;
  sh->R[n] <<= 1;
  sh->PC += 2;
  sh->cycleCount++;
}

void shar(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  if ((sh->R[n]&0x00000001)==0) sh->SR.partie.T = 0;
  else sh->SR.partie.T = 1;
  if ((sh->R[n]&0x80000000)==0) temp = 0;
  else temp = 1;
  sh->R[n] >>= 1;
  if (temp == 1) sh->R[n] |= 0x80000000;
  else sh->R[n] &= 0x7FFFFFFF;
  sh->PC += 2;
  sh->cycleCount++;
}

void shll(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->R[n]&0x80000000)==0) sh->SR.partie.T=0;
  else sh->SR.partie.T=1;
  sh->R[n]<<=1;
  sh->PC+=2;
  sh->cycleCount++;
}

void shll2(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)] <<= 2;
  sh->PC+=2;
  sh->cycleCount++;
}

void shll8(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)]<<=8;
  sh->PC+=2;
  sh->cycleCount++;
}

void shll16(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)]<<=16;
  sh->PC+=2;
  sh->cycleCount++;
}

void shlr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);

  if ((sh->R[n]&0x00000001)==0) sh->SR.partie.T=0;
  else sh->SR.partie.T=1;
  sh->R[n]>>=1;
  sh->PC+=2;
  sh->cycleCount++;
}

void shlr2(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]>>=2;
  sh->PC+=2;
  sh->cycleCount++;
}

void shlr8(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]>>=8;
  sh->PC+=2;
  sh->cycleCount++;
}

void shlr16(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]>>=16;
  sh->PC+=2;
  sh->cycleCount++;
}

void stcgbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]=sh->GBR;
  sh->PC+=2;
  sh->cycleCount++;
}

void stcmgbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]-=4;
  sh->memoire->setLong(sh->R[n],sh->GBR);
  sh->PC+=2;
  sh->cycleCount += 2;
}

void stcmsr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]-=4;
  sh->memoire->setLong(sh->R[n],sh->SR.tout);
  sh->PC+=2;
  sh->cycleCount += 2;
}

void stcmvbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]-=4;
  sh->memoire->setLong(sh->R[n],sh->VBR);
  sh->PC+=2;
  sh->cycleCount += 2;
}

void stcsr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n] = sh->SR.tout;
  sh->PC+=2;
  sh->cycleCount++;
}

void stcvbr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]=sh->VBR;
  sh->PC+=2;
  sh->cycleCount++;
}

void stsmach(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]=sh->MACH;
  sh->PC+=2;
  sh->cycleCount++;
}

void stsmacl(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n]=sh->MACL;
  sh->PC+=2;
  sh->cycleCount++;
}

void stsmmach(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n] -= 4;
  sh->memoire->setLong(sh->R[n],sh->MACH); 
  sh->PC+=2;
  sh->cycleCount++;
}

void stsmmacl(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n] -= 4;
  sh->memoire->setLong(sh->R[n],sh->MACL);
  sh->PC+=2;
  sh->cycleCount++;
}

void stsmpr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n] -= 4;
  sh->memoire->setLong(sh->R[n],sh->PR);
  sh->PC+=2;
  sh->cycleCount++;
}

void stspr(SuperH * sh) {
  long n = Instruction::b(sh->instruction);
  sh->R[n] = sh->PR;
  sh->PC+=2;
  sh->cycleCount++;
}

void sub(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  sh->R[n]-=sh->R[m];
  sh->PC+=2;
  sh->cycleCount++;
}

void subc(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  unsigned long tmp0,tmp1;
  
  tmp1 = sh->R[n] - sh->R[m];
  tmp0 = sh->R[n];
  sh->R[n] = tmp1 - sh->SR.partie.T;
  if (tmp0 < tmp1) sh->SR.partie.T = 1;
  else sh->SR.partie.T = 0;
  if (tmp1 < sh->R[n]) sh->SR.partie.T = 1;
  sh->PC += 2;
  sh->cycleCount++;
}

void subv(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  long dest,src,ans;

  if ((long)sh->R[n]>=0) dest=0;
  else dest=1;
  if ((long)sh->R[m]>=0) src=0;
  else src=1;
  src+=dest;
  sh->R[n]-=sh->R[m];
  if ((long)sh->R[n]>=0) ans=0;
  else ans=1;
  ans+=dest;
  if (src==1) {
    if (ans==1) sh->SR.partie.T=1;
    else sh->SR.partie.T=0;
  }
  else sh->SR.partie.T=0;
  sh->PC+=2;
  sh->cycleCount++;
}

void swapb(SuperH * sh) {
  unsigned long temp0,temp1;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  temp0=sh->R[m]&0xffff0000;
  temp1=(sh->R[m]&0x000000ff)<<8;
  sh->R[n]=(sh->R[m]>>8)&0x000000ff;
  sh->R[n]=sh->R[n]|temp1|temp0;
  sh->PC+=2;
  sh->cycleCount++;
}

void swapw(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  temp=(sh->R[m]>>16)&0x0000FFFF;
  sh->R[n]=sh->R[m]<<16;
  sh->R[n]|=temp;
  sh->PC+=2;
  sh->cycleCount++;
}

void tas(SuperH * sh) {
  long temp;
  long n = Instruction::b(sh->instruction);

  temp=(long) sh->memoire->getByte(sh->R[n]);
  if (temp==0) sh->SR.partie.T=1;
  else sh->SR.partie.T=0;
  temp|=0x00000080;
  sh->memoire->setByte(sh->R[n],temp);
  sh->PC+=2;
  sh->cycleCount += 4;
}

void trapa(SuperH * sh) {
  long imm = Instruction::cd(sh->instruction);

  sh->R[15]-=4;
  sh->memoire->setLong(sh->R[15],sh->SR.tout);
  sh->R[15]-=4;
  sh->memoire->setLong(sh->R[15],sh->PC + 2);
  sh->PC = sh->memoire->getLong(sh->VBR+(imm<<2));
  sh->cycleCount += 8;
}

void tst(SuperH * sh) {
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);
  if ((sh->R[n]&sh->R[m])==0) sh->SR.partie.T = 1;
  else sh->SR.partie.T = 0;
  sh->PC += 2;
  sh->cycleCount++;
}

void tsti(SuperH * sh) {
  long temp;
  long i = Instruction::cd(sh->instruction);

  temp=sh->R[0]&i;
  if (temp==0) sh->SR.partie.T = 1;
  else sh->SR.partie.T = 0;
  sh->PC+=2;
  sh->cycleCount++;
}

void tstm(SuperH * sh) {
  long temp;
  long i = Instruction::cd(sh->instruction);

  temp=(long) sh->memoire->getByte(sh->GBR+sh->R[0]);
  temp&=i;
  if (temp==0) sh->SR.partie.T = 1;
  else sh->SR.partie.T = 0;
  sh->PC+=2;
  sh->cycleCount += 3;
}

void y_xor(SuperH * sh) {
  sh->R[Instruction::b(sh->instruction)] ^= sh->R[Instruction::c(sh->instruction)];
  sh->PC += 2;
  sh->cycleCount++;
}

void xori(SuperH * sh) {
  long source = Instruction::cd(sh->instruction);
  sh->R[0] ^= source;
  sh->PC += 2;
  sh->cycleCount++;
}

void xorm(SuperH * sh) {
  long source = Instruction::cd(sh->instruction);
  long temp;

  temp = (long) sh->memoire->getByte(sh->GBR + sh->R[0]);
  temp ^= source;
  sh->memoire->setByte(sh->GBR + sh->R[0],temp);
  sh->PC += 2;
  sh->cycleCount += 3;
}

void xtrct(SuperH * sh) {
  unsigned long temp;
  long m = Instruction::c(sh->instruction);
  long n = Instruction::b(sh->instruction);

  temp=(sh->R[m]<<16)&0xFFFF0000;
  sh->R[n]=(sh->R[n]>>16)&0x0000FFFF;
  sh->R[n]|=temp;
  sh->PC+=2;
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

/*
Memory *SuperH::GetSdramMode() {
   return modeSdram;
}

Memory *SuperH::GetPurgeArea() {
   return purgeArea;
}

Memory *SuperH::GetAddressArray() {
   return adressArray;
}

Memory *SuperH::GetDataArray() {
   return dataArray;
}

Memory *SuperH::GetOnchip() {
   return onchip;
}
*/

// pending approval
void SuperH::GetRegisters(sh2regs_struct *regs) {
  if (regs != NULL) {
    memcpy(regs->R, R, sizeof(R));
    regs->SR.all = SR.tout;
    regs->GBR = GBR;
    regs->VBR = VBR;
    regs->MACH = MACH;
    regs->MACL = MACL;
    regs->PR = PR;
    regs->PC = PC;
    regs->delay = _delai;
  }
}

void SuperH::SetRegisters(sh2regs_struct *regs) {
  if (regs != NULL) {
    memcpy(R, regs->R, sizeof(R));
    SR.tout = regs->SR.all;
    GBR = regs->GBR;
    VBR = regs->VBR;
    MACH = regs->MACH;
    MACL = regs->MACL;
    PR = regs->PR;
    PC = regs->PC;
    _delai = regs->delay;
  }
}

void SuperH::SetBreakpointCallBack(void (*func)(bool, unsigned long)) {
   BreakpointCallBack = func;
}

int SuperH::AddCodeBreakpoint(unsigned long addr) {
  if (numcodebreakpoints < MAX_BREAKPOINTS) {
     codebreakpoint[numcodebreakpoints].addr = addr;
//     // backup old opcode
//     codebreakpoint[numcodebreakpoints].oldopcode = memoire->getWord(addr);
//     // set opcode to an invalid opcode(so there's no extra overhead)
//     memoire->setWord(addr, 0x8383); 
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
//           // Return opcode to old opcode(first make sure our invalid opcode
//           // wasn't replaced)
//           if (memoire->getWord(addr) == 0x8383)
//              memoire->setWord(addr, codebreakpoint[i].oldopcode);

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
//        // Return opcode to old opcode(first make sure our invalid opcode
//        // wasn't replaced)
//        if (memoire->getWord(addr) == 0x8383)
//           memoire->setWord(addr, codebreakpoints[i].oldopcode);

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
