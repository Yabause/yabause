/*  Copyright 2003 Guillaume Duhamel

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
#include "sh2d.hh"
#include "registres.hh"

SuperH::SuperH(bool slave) {
  SR.partie.T = SR.partie.S = SR.partie.Q = SR.partie.M = 0;
  SR.partie.I = 0xF;
  SR.partie.inutile1 = 0;
  SR.partie.inutile2 = 0;
  VBR = 0;

  purgeArea   = new Dummy(0xFFFFFFFF);
  adressArray = new Memory(0xFFF, 0x3FF);
  dataArray   = new Memory(0xFFF, 0x1000);
  modeSdram = new Memory(0xFFF, 0x4FFF);

  _interrupt = false;
  _delai = 0;
  for(unsigned short i = 0;i < 0xFFFF;i++) {
    instruction = i;
    opcodes[i] = decode();
  }
  _stop = false;
  _pause = true;
  _run = false;

  isslave = slave;

  for(int i = 0;i < 7;i++) {
    mutex[i] = SDL_CreateMutex();
    cond[i] = SDL_CreateCond();
  }
#ifdef DEBUG
  verbose = 0;
#endif
}

SuperH::~SuperH(void) {
  delete purgeArea;
  delete adressArray;
  delete dataArray;
  delete modeSdram;

  for(int i = 0;i < 7;i++) {
    SDL_DestroyCond(cond[i]);
    SDL_DestroyMutex(mutex[i]);
  }
}

bool SuperH::processingInterrupt(void) {
  return _interrupt && !_stop;
}

void SuperH::interrupt(void) {
  _interrupt = true;
}

unsigned char& SuperH::level(void) {
  return _level;
}

unsigned char& SuperH::vector(void) {
  return _vector;
}

void SuperH::setMemory(Memory *mem) {
  memoire = mem;
  PC = memoire->getLong(VBR) + 4;
  R[15] = memoire->getLong(VBR + 4);
}

Memory *SuperH::getMemory(void) {
  return memoire;
}

int SuperH::lancer(SuperH *sh) {
#ifndef _arch_dreamcast
	try {
		sh->synchroStart<26874100, 60, 9, 1, 224, 39>();
	}
	catch(Exception e) {
                Memory *mem; 
		cerr << "KABOOOOM" << endl;
		cerr << "PC=" << hex << sh->PC << endl;
		cerr << "opcode=" << hex << sh->instruction << endl;
		cerr << e << endl;
	}
#else
	sh->synchroStart<26874100, 60, 9, 1, 224, 39>();
#endif
}

template<int FREQ, int FRAME, int HIN, int HOUT, int VIN, int VOUT>
void SuperH::synchroStart(void) {
  Div<FREQ, FRAME * (HIN + HOUT) * (VIN + VOUT)> deciline;
  Div<FREQ, 100000> deciufreq;

  int decilineCount = 0;
  int lineCount = 0;
  int frameCount = 0;
  int decilineStop = deciline.nextValue();
  int duf = deciufreq.nextValue();

  unsigned long ticks = 0;

  cycleCount = 0;
  unsigned long cycleCountII = 0;

  while(!_stop) {
	  /*
    for(int coin=300;coin > 0;coin--) executer();
    exit(1);
    */
    while((cycleCount < decilineStop) && _run) {
      executer();
    }

    while((cycleCount < decilineStop) && _pause) {
      SDL_mutexP(mutex[4]);
      SDL_CondWait(cond[4], mutex[4]);
      SDL_mutexV(mutex[4]);
      executer();
    }
    if (!_stop) {
      decilineCount++;
      switch(decilineCount) {
      case HIN:
	SDL_CondBroadcast(cond[5]);
	// HBlankIN
        break;
      case HIN + HOUT:
	// HBlankOUT
	SDL_CondBroadcast(cond[6]);
	decilineCount = 0;
	lineCount++;
	switch(lineCount) {
	case VIN:
	  // VBlankIN
	  SDL_CondBroadcast(cond[1]);
	  break;
	case VIN + VOUT:
	  // VBlankOUT
	  SDL_CondBroadcast(cond[2]);
	  lineCount = 0;
          frameCount++;
	  if(SDL_GetTicks() >= ticks + 1000) {
	    yui_fps(frameCount);
	    frameCount = 0;
	    ticks = SDL_GetTicks();
	  }
	  break;
	}
	break;
      }
      cycleCountII += cycleCount;

      while (cycleCountII > duf) {
        ((Smpc *) ((SaturnMemory *)memoire)->getSmpc())->execute2(10); // yurk :(
        ((Onchip *) ((SaturnMemory *)memoire)->getOnchip())->run(10); // yurk :(
	SDL_CondBroadcast(cond[0]);
	cycleCountII %= duf;
	duf = deciufreq.nextValue();
      }

      Cs2::run(((Cs2 *) ((SaturnMemory *)memoire)->getCS2()));

      cycleCount %= decilineStop;
      decilineStop = deciline.nextValue();
    }
  }
}

void SuperH::executer(void) {
  if (_delai) {
    unsigned long tmp = PC;
    PC = _delai;
    _executer();
    PC = tmp;
    _delai = 0;
  }
  else {
    Onchip * oc = (Onchip *) ((SaturnMemory *) memoire)->getOnchip();
    if ( !oc->interrupts.empty() ) {
      Interrupt interrupt = oc->interrupts.top();
      if (interrupt.level() > SR.partie.I) {
	_level = interrupt.level();
	_vector = interrupt.vector();
        oc->interrupts.pop();

        R[15] -= 4;
        memoire->setLong(R[15], SR.tout);
        R[15] -= 4;
        memoire->setLong(R[15], PC);
        SR.partie.I = _level;
        PC = memoire->getLong(VBR + (_vector << 2)) + 4;
        _interrupt = false;
      }
    }
#ifdef _arch_dreamcast
    cont_cond_t cond;
    cont_get_cond(maple_first_controller(), &cond);
    if(!(cond.buttons & CONT_A)) {
	    vid_screen_shot("/pc/saturn.raw");
    }
#endif
    _executer();
  }
}

void SuperH::_executer(void) {
  instruction = memoire->getWord(PC - 4);
#ifdef DEBUG
  if(verbose > 0) {
	  char chaine[100];
	  SH2Disasm(PC - 4, instruction, 0, chaine);
	  cerr << hex << PC << ' ' << chaine << endl;
	  verbose--;
  }
#endif

  (this->*opcodes[instruction])();
}

void SuperH::stop(void) {
  _stop = true;
  _run = false;
  _pause = false;
  for(int i = 0;i < 7;i++) {
	  SDL_CondBroadcast(cond[i]);
  }
}

void SuperH::pause(void) {
  _pause = true;
  _run = false;
}

bool SuperH::paused(void) {
	return _pause;
}

void SuperH::run(void) {
  _run = true;
  _pause = false;
  SDL_CondBroadcast(cond[4]);
}

void SuperH::step(void) {
  SDL_CondBroadcast(cond[4]);
}

void SuperH::microsleep(unsigned long nbusec) {
  if (_stop) return;
  unsigned long tmp = nbusec / 10;
  while (tmp--) {
    SDL_mutexP(mutex[0]);
    SDL_CondWait(cond[0], mutex[0]);
    SDL_mutexV(mutex[0]);
  }
}

void SuperH::waitHBlankIN(void) {
  if (_stop) return;
  SDL_mutexP(mutex[5]);
  SDL_CondWait(cond[5], mutex[5]);
  SDL_mutexV(mutex[5]);
}

void SuperH::waitHBlankOUT(void) {
  if (_stop) return;
  SDL_mutexP(mutex[6]);
  SDL_CondWait(cond[6], mutex[6]);
  SDL_mutexV(mutex[6]);
}

void SuperH::waitVBlankIN(void) {
  if (_stop) return;
  SDL_mutexP(mutex[1]);
  SDL_CondWait(cond[1], mutex[1]);
  SDL_mutexV(mutex[1]);
}

void SuperH::waitVBlankOUT(void) {
  if (_stop) return;
  SDL_mutexP(mutex[2]);
  SDL_CondWait(cond[2], mutex[2]);
  SDL_mutexV(mutex[2]);
}

void SuperH::waitInterruptEnd(void) {
  if (_stop) return;
  SDL_mutexP(mutex[3]);
  SDL_CondWait(cond[3], mutex[3]);
  SDL_mutexV(mutex[3]);
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

void SuperH::undecoded(void) {
#ifndef _arch_dreamcast
	throw IllegalOpcode(instruction, PC);
#else
	printf("Illegal Opcode: 0x%8x, PC: 0x%8x\n", instruction, PC);
	exit(-1);
#endif
}

void SuperH::add(void) {
  R[Instruction::b(instruction)] += R[Instruction::c(instruction)];
  PC += 2;
  cycleCount++;
}

void SuperH::addi(void) {
  long source = (long)(signed char)Instruction::cd(instruction);
  long dest = Instruction::b(instruction);

  R[dest] += source;
  PC += 2;
  cycleCount++;
}

void SuperH::addc(void) {
  unsigned long tmp0, tmp1;
  long source = Instruction::c(instruction);
  long dest = Instruction::b(instruction);

  tmp1 = R[source] + R[dest];
  tmp0 = R[dest];

  R[dest] = tmp1 + SR.partie.T;
  if (tmp0 > tmp1)
    SR.partie.T = 1;
  else
    SR.partie.T = 0;
  if (tmp1 > R[dest])
    SR.partie.T = 1;
  PC += 2;
  cycleCount++;
}

void SuperH::addv(void) {
  long i,j,k;
  long source = Instruction::c(instruction);
  long dest = Instruction::b(instruction);

  if ((long) R[dest] >= 0) i = 0; else i = 1;
  //i = ((long) R[dest] < 0);
  
  if ((long) R[source] >= 0) j = 0; else j = 1;
  //j = ((long) R[source] < 0);
  
  j += i;
  R[dest] += R[source];

  if ((long) R[dest] >= 0) k = 0; else k = 1;
  //k = ((long) R[dest] < 0);

  k += j;
  
  if (j == 0 || j == 2)
    if (k == 1) SR.partie.T = 1;
    else SR.partie.T = 0;
  else SR.partie.T = 0;
  //SR.partie.T = ((j == 0 || j == 2) && (k == 1));
  PC += 2;
  cycleCount++;
}

void SuperH::y_and(void) {
  R[Instruction::b(instruction)] &= R[Instruction::c(instruction)];
  PC += 2;
  cycleCount++;
}

void SuperH::andi(void) {
  R[0] &= Instruction::cd(instruction);
  PC += 2;
  cycleCount++;
}

void SuperH::andm(void) {
  long temp;
  long source = Instruction::cd(instruction);

  temp = (long) memoire->getByte(GBR + R[0]);
  temp &= source;
  memoire->setByte((GBR + R[0]),temp);
  PC += 2;
  cycleCount += 3;
}
 
void SuperH::bf(void) { // FIXME peut etre amelioré
  long disp;
  long d = Instruction::cd(instruction);

  disp = (long)(signed char)d;

  if (SR.partie.T == 0) {
    PC = PC+(disp<<1)+4;
    cycleCount += 3;
  }
  else {
    PC+=2;
    cycleCount++;
  }
}

void SuperH::bfs(void) { // FIXME peut être amélioré
  long disp;
  unsigned long temp;
  long d = Instruction::cd(instruction);

  temp = PC;
  disp = (long)(signed char)d;

  if (SR.partie.T == 0) {
    PC = PC + (disp << 1) + 4;

    _delai = temp + 2;
    cycleCount += 2;
  }
  else {
    PC += 2;
    cycleCount++;
  }
}

void SuperH::bra(void) {
  unsigned long temp;
  long d = Instruction::bcd(instruction);

  long disp;
  if ((d&0x800)==0) disp=(0x00000FFF & d);
  else disp=(0xFFFFF000 | d);
  temp=PC;
  PC = PC + (disp<<1) + 4;

  _delai = temp + 2;
  cycleCount += 2;
}

void SuperH::braf(void) {
  unsigned long temp;
  long m = Instruction::b(instruction);

  temp = PC;
  PC += R[m] + 4; /* FIXME : Correction trouvée dans semu, est-ce bon ?
		     Hyperion est d'accord ! */

  _delai = temp + 2;
  cycleCount += 2;
}

void SuperH::bsr(void) {
  long disp;
  long d = Instruction::bcd(instruction);

  if ((d&0x800)==0) disp = (0x00000FFF & d);
  else disp = (0xFFFFF000 | d);
  PR = PC;
  PC = PC+(disp<<1) + 4;

  _delai = PR + 2;
  cycleCount += 2;
}

void SuperH::bsrf(void) {
  PR = PC;
  PC += R[Instruction::b(instruction)] + 4; // correction donnée par Hyperion
  _delai = PR + 2;
  cycleCount += 2;
}

void SuperH::bt(void) { // FIXME ya plus rapide
  long disp;
  long d = Instruction::cd(instruction);

  disp = (long)(signed char)d;
  if (SR.partie.T == 1) {
    PC = PC+(disp<<1)+4;
    cycleCount += 3;
  }
  else {
    PC += 2;
    cycleCount++;
  }
}

void SuperH::bts(void) {
  long disp;
  unsigned long temp;
  long d = Instruction::cd(instruction);
  
  if (SR.partie.T) {
    temp = PC;
    disp = (long)(signed char)d;
    PC += (disp << 1) + 4;
    _delai = temp + 2;
    cycleCount += 2;
  }
  else {
    PC+=2;
    cycleCount++;
  }
}

void SuperH::clrmac(void) {
  MACH = 0;
  MACL = 0;
  PC += 2;
  cycleCount++;
}

void SuperH::clrt(void) {
  SR.partie.T = 0;
  PC += 2;
  cycleCount++;
}

void SuperH::cmpeq(void) {
  if (R[Instruction::b(instruction)] == R[Instruction::c(instruction)])
    SR.partie.T = 1;
  else
    SR.partie.T = 0;
  PC += 2;
  cycleCount++;
}

void SuperH::cmpge(void) {
  if ((long)R[Instruction::b(instruction)] >=
		  (long)R[Instruction::c(instruction)])
    SR.partie.T = 1;
  else
    SR.partie.T = 0;
  PC += 2;
  cycleCount++;
}

void SuperH::cmpgt(void) {
  if ((long)R[Instruction::b(instruction)]>(long)R[Instruction::c(instruction)])
    SR.partie.T = 1;
  else
    SR.partie.T = 0;
  PC+=2;
  cycleCount++;
}

void SuperH::cmphi(void) {
  if ((unsigned long)R[Instruction::b(instruction)]>
		  (unsigned long)R[Instruction::c(instruction)])
    SR.partie.T = 1;
  else
    SR.partie.T = 0;
  PC+=2;
  cycleCount++;
}

void SuperH::cmphs(void) {
  if ((unsigned long)R[Instruction::b(instruction)]>=
		  (unsigned long)R[Instruction::c(instruction)])
    SR.partie.T = 1;
  else
    SR.partie.T = 0;
  PC+=2;
  cycleCount++;
}

void SuperH::cmpim(void) {
  long imm;
  long i = Instruction::cd(instruction);

  imm = (long)(signed char)i;

  if (R[0] == (unsigned long) imm) // FIXME: ouais ça doit être bon...
    SR.partie.T = 1;
  else
    SR.partie.T = 0;
  PC+=2;
  cycleCount++;
}

void SuperH::cmppl(void) {
  if ((long)R[Instruction::b(instruction)]>0)
    SR.partie.T = 1;
  else
    SR.partie.T = 0;
  PC+=2;
  cycleCount++;
}

void SuperH::cmppz(void) {
  if ((long)R[Instruction::b(instruction)]>=0)
    SR.partie.T = 1;
  else
    SR.partie.T = 0;
  PC+=2;
  cycleCount++;
}

void SuperH::cmpstr(void) {
  unsigned long temp;
  long HH,HL,LH,LL;
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  temp=R[n]^R[m];
  HH = (temp>>24) & 0x000000FF;
  HL = (temp>>16) & 0x000000FF;
  LH = (temp>>8) & 0x000000FF;
  LL = temp & 0x000000FF;
  HH = HH && HL && LH && LL;
  if (HH == 0)
    SR.partie.T = 1;
  else
    SR.partie.T = 0;
  PC+=2;
  cycleCount++;
}

void SuperH::div0s(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  if ((R[n]&0x80000000)==0)
    SR.partie.Q = 0;
  else
    SR.partie.Q = 1;
  if ((R[m]&0x80000000)==0)
    SR.partie.M = 0;
  else
    SR.partie.M = 1;
  SR.partie.T = !(SR.partie.M == SR.partie.Q);
  PC += 2;
  cycleCount++;
}

void SuperH::div0u(void) {
  SR.partie.M = SR.partie.Q = SR.partie.T = 0;
  PC += 2;
  cycleCount++;
}

void SuperH::div1(void) {
  unsigned long tmp0;
  unsigned char old_q, tmp1;
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  
  old_q = SR.partie.Q;
  SR.partie.Q = (unsigned char)((0x80000000 & R[n])!=0);
  R[n] <<= 1;
  R[n]|=(unsigned long) SR.partie.T;
  switch(old_q){
  case 0:
    switch(SR.partie.M){
    case 0:
      tmp0 = R[n];
      R[n] -= R[m];
      tmp1 = (R[n] > tmp0);
      switch(SR.partie.Q){
      case 0:
	SR.partie.Q = tmp1;
	break;
      case 1:
	SR.partie.Q = (unsigned char) (tmp1 == 0);
	break;
      }
      break;
    case 1:
      tmp0 = R[n];
      R[n] += R[m];
      tmp1 = (R[n] < tmp0);
      switch(SR.partie.Q){
      case 0:
	SR.partie.Q = (unsigned char) (tmp1 == 0);
	break;
      case 1:
	SR.partie.Q = tmp1;
	break;
      }
      break;
    }
    break;
  case 1:switch(SR.partie.M){
  case 0:
    tmp0 = R[n];
    R[n] += R[m];
    tmp1 = (R[n] < tmp0);
    switch(SR.partie.Q){
    case 0:
      SR.partie.Q = tmp1;
      break;
    case 1:
      SR.partie.Q = (unsigned char) (tmp1 == 0);
      break;
    }
    break;
  case 1:
    tmp0 = R[n];
    R[n] -= R[m];
    tmp1 = (R[n] > tmp0);
    switch(SR.partie.Q){
    case 0:
      SR.partie.Q = (unsigned char) (tmp1 == 0);
      break;
    case 1:
      SR.partie.Q = tmp1;
      break;
    }
    break;
  }
  break;
  }
  SR.partie.T = (SR.partie.Q == SR.partie.M);
  PC += 2;
  cycleCount++;
}


void SuperH::dmuls(void) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  
  tempn = (long)R[n];
  tempm = (long)R[m];
  if (tempn < 0) tempn = 0 - tempn;
  if (tempm < 0) tempm = 0 - tempm;
  if ((long) (R[n] ^ R[m]) < 0) fnLmL = -1;
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
  MACH = Res2;
  MACL = Res0;
  PC += 2;
  cycleCount += 2;
}

void SuperH::dmulu(void) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  RnL = R[n] & 0x0000FFFF;
  RnH = (R[n] >> 16) & 0x0000FFFF;
  RmL = R[m] & 0x0000FFFF;
  RmH = (R[m] >> 16) & 0x0000FFFF;

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

  MACH = Res2;
  MACL = Res0;
  PC += 2;
  cycleCount += 2;
}

void SuperH::dt(void) {
  long n = Instruction::b(instruction);
  R[n]--;
  if (R[n] == 0) SR.partie.T = 1;
  else SR.partie.T = 0;
  PC += 2;
  cycleCount++;
}

void SuperH::extsb(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  R[n] = (unsigned long)(signed char)R[m];
  PC += 2;
  cycleCount++;
}

void SuperH::extsw(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  R[n] = (unsigned long)(signed short)R[m];
  PC += 2;
  cycleCount++;
}

void SuperH::extub(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  R[n] = (unsigned long)(unsigned char)R[m];
  PC += 2;
  cycleCount++;
}

void SuperH::extuw(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  R[n] = (unsigned long)(unsigned short)R[m];
  PC += 2;
  cycleCount++;
}

void SuperH::jmp(void) {
  unsigned long temp;
  long m = Instruction::b(instruction);

  temp=PC;
  PC = R[m] + 4;
  _delai = temp + 2;
  cycleCount += 2;
}

void SuperH::jsr(void) {
  long m = Instruction::b(instruction);
  PR = PC;
  PC = R[m] + 4;
  _delai = PR + 2;
  cycleCount += 2;
}

void SuperH::ldcgbr(void) {
  GBR = R[Instruction::b(instruction)];
  PC += 2;
  cycleCount++;
}

void SuperH::ldcmgbr(void) {
  long m = Instruction::b(instruction);

  GBR = memoire->getLong(R[m]);
  R[m] += 4;
  PC += 2;
  cycleCount += 3;
}

void SuperH::ldcmsr(void) {
  long m = Instruction::b(instruction);

  SR.tout = memoire->getLong(R[m]) & 0x000003F3;
  R[m] += 4;
  PC += 2;
  cycleCount += 3;
}

void SuperH::ldcmvbr(void) {
  long m = Instruction::b(instruction);

  VBR = memoire->getLong(R[m]);
  R[m] += 4;
  PC += 2;
  cycleCount += 3;
}

void SuperH::ldcsr(void) {
  SR.tout = R[Instruction::b(instruction)]&0x000003F3;
  PC += 2;
  cycleCount++;
}

void SuperH::ldcvbr(void) {
  long m = Instruction::b(instruction);

  VBR = R[m];
  PC += 2;
  cycleCount++;
}

void SuperH::ldsmach(void) {
  MACH = R[Instruction::b(instruction)];
  PC+=2;
  cycleCount++;
}

void SuperH::ldsmacl(void) {
  MACL = R[Instruction::b(instruction)];
  PC += 2;
  cycleCount++;
}

void SuperH::ldsmmach(void) {
  long m = Instruction::b(instruction);
  MACH = memoire->getLong(R[m]);
  R[m] += 4;
  PC += 2;
  cycleCount++;
}

void SuperH::ldsmmacl(void) {
  long m = Instruction::b(instruction);
  MACL = memoire->getLong(R[m]);
  R[m] += 4;
  PC += 2;
  cycleCount++;
}

void SuperH::ldsmpr(void) {
  long m = Instruction::b(instruction);
  PR = memoire->getLong(R[m]);
  R[m] += 4;
  PC += 2;
  cycleCount++;
}

void SuperH::ldspr(void) {
  PR = R[Instruction::b(instruction)];
  PC += 2;
  cycleCount++;
}

void SuperH::macl(void) {
  unsigned long RnL,RnH,RmL,RmH,Res0,Res1,Res2;
  unsigned long temp0,temp1,temp2,temp3;
  long tempm,tempn,fnLmL;
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  
  tempn = (long) memoire->getLong(R[n]);
  R[n] += 4;
  tempm = (long) memoire->getLong(R[m]);
  R[m] += 4;

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
  if(SR.partie.S == 1){
    Res0=MACL+Res0;
    if (MACL>Res0) Res2++;
    if (MACH & 0x00008000);
    else Res2 += MACH | 0xFFFF0000;
    Res2+=(MACH&0x0000FFFF);
    if(((long)Res2<0)&&(Res2<0xFFFF8000)){
      Res2=0x00008000;
      Res0=0x00000000;
    }
    if(((long)Res2>0)&&(Res2>0x00007FFF)){
      Res2=0x00007FFF;
      Res0=0xFFFFFFFF;
    };
    
    MACH=Res2;
    MACL=Res0;
  }
  else {
    Res0=MACL+Res0;
    if (MACL>Res0) Res2++;
    Res2+=MACH;

    MACH=Res2;
    MACL=Res0;
  }
  
  PC+=2;
  cycleCount += 3;
}

void SuperH::macw(void) {
  long tempm,tempn,dest,src,ans;
  unsigned long templ;
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  tempn=(long) memoire->getWord(R[n]);
  R[n]+=2;
  tempm=(long) memoire->getWord(R[m]);
  R[m]+=2;
  templ=MACL;
  tempm=((long)(short)tempn*(long)(short)tempm);

  if ((long)MACL>=0) dest=0;
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
  MACL+=tempm;
  if ((long)MACL>=0) ans=0;
  else ans=1;
  ans+=dest;
  if (SR.partie.S == 1) {
    if (ans==1) {
      if (src==0) MACL=0x7FFFFFFF;
      if (src==2) MACL=0x80000000;
    }
  }
  else {
    MACH+=tempn;
    if (templ>MACL) MACH+=1;
  }
  PC+=2;
  cycleCount += 3;
}

void SuperH::mov(void) {
  R[Instruction::b(instruction)]=R[Instruction::c(instruction)];
  PC+=2;
  cycleCount++;
}

void SuperH::mova(void) {
  long disp = Instruction::cd(instruction);

  R[0]=(PC&0xFFFFFFFC)+(disp<<2);
  PC+=2;
  cycleCount++;
}

void SuperH::movbl(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  R[n] = (long)(signed char)memoire->getByte(R[m]);
  PC += 2;
  cycleCount++;
}

void SuperH::movbl0(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  long temp;

  R[n] = (long)(signed char)memoire->getByte(R[m] + R[0]);
  PC += 2;
  cycleCount++;
}

void SuperH::movbl4(void) {
  long m = Instruction::c(instruction);
  long disp = Instruction::d(instruction);

  R[0] = (long)(signed char)memoire->getByte(R[m] + disp);
  PC+=2;
  cycleCount++;
}

void SuperH::movblg(void) {
  long disp = Instruction::cd(instruction);
  long temp;
  
  R[0] = (long)(signed char)memoire->getByte(GBR + disp);
  PC+=2;
  cycleCount++;
}

void SuperH::movbm(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  memoire->setByte((R[n] - 1),R[m]);
  R[n] -= 1;
  PC += 2;
  cycleCount++;
}

void SuperH::movbp(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  R[n] = (long)(signed char)memoire->getByte(R[m]);
  if (n != m)
    R[m] += 1;
  PC += 2;
  cycleCount++;
}

void SuperH::movbs(void) {
  memoire->setByte(R[Instruction::b(instruction)],
	R[Instruction::c(instruction)]);
  PC += 2;
  cycleCount++;
}

void SuperH::movbs0(void) {
  memoire->setByte(R[Instruction::b(instruction)] + R[0],
	R[Instruction::c(instruction)]);
  PC += 2;
  cycleCount++;
}

void SuperH::movbs4(void) {
  long disp = Instruction::d(instruction);
  long n = Instruction::c(instruction);

  memoire->setByte(R[n]+disp,R[0]);
  PC+=2;
  cycleCount++;
}

void SuperH::movbsg(void) {
  long disp = Instruction::cd(instruction);

  memoire->setByte(GBR + disp,R[0]);
  PC += 2;
  cycleCount++;
}

void SuperH::movi(void) {
  long i = Instruction::cd(instruction);
  long n = Instruction::b(instruction);

  R[n] = (long)(signed char)i;
  PC += 2;
  cycleCount++;
}

void SuperH::movli(void) {
  long disp = Instruction::cd(instruction);
  long n = Instruction::b(instruction);

  R[n] = memoire->getLong((PC & 0xFFFFFFFC) + (disp << 2));
  PC += 2;
  cycleCount++;
}

void SuperH::movll(void) {
  R[Instruction::b(instruction)] =
	memoire->getLong(R[Instruction::c(instruction)]);
  PC += 2;
  cycleCount++;
}

void SuperH::movll0(void) {
  R[Instruction::b(instruction)] =
	memoire->getLong(R[Instruction::c(instruction)] + R[0]);
  PC += 2;
  cycleCount++;
}

void SuperH::movll4(void) {
  long m = Instruction::c(instruction);
  long disp = Instruction::d(instruction);
  long n = Instruction::b(instruction);

  R[n] = memoire->getLong(R[m] + (disp << 2));
  PC += 2;
  cycleCount++;
}

void SuperH::movllg(void) {
  long disp = Instruction::cd(instruction);

  R[0] = memoire->getLong(GBR + (disp << 2));
  PC+=2;
  cycleCount++;
}

void SuperH::movlm(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  memoire->setLong(R[n] - 4,R[m]);
  R[n] -= 4;
  PC += 2;
  cycleCount++;
}

void SuperH::movlp(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  R[n] = memoire->getLong(R[m]);
  if (n != m) R[m] += 4;
  PC += 2;
  cycleCount++;
}

void SuperH::movls(void) {
  memoire->setLong(R[Instruction::b(instruction)],
	R[Instruction::c(instruction)]);
  PC += 2;
  cycleCount++;
}

void SuperH::movls0(void) {
  memoire->setLong(R[Instruction::b(instruction)] + R[0],
	R[Instruction::c(instruction)]);
  PC += 2;
  cycleCount++;
}

void SuperH::movls4(void) {
  long m = Instruction::c(instruction);
  long disp = Instruction::d(instruction);
  long n = Instruction::b(instruction);

  memoire->setLong(R[n]+(disp<<2),R[m]);
  PC += 2;
  cycleCount++;
}

void SuperH::movlsg(void) {
  long disp = Instruction::cd(instruction);

  memoire->setLong(GBR+(disp<<2),R[0]);
  PC+=2;
  cycleCount++;
}

void SuperH::movt(void) {
  R[Instruction::b(instruction)] = (0x00000001 & SR.tout);
  PC += 2;
  cycleCount++;
}

void SuperH::movwi(void) {
  long disp = Instruction::cd(instruction);
  long n = Instruction::b(instruction);

  R[n] = (long)(signed short)memoire->getWord(PC + (disp<<1));
  PC+=2;
  cycleCount++;
}

void SuperH::movwl(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  R[n] = (long)(signed short)memoire->getWord(R[m]);
  PC += 2;
  cycleCount++;
}

void SuperH::movwl0(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  R[n] = (long)(signed short)memoire->getWord(R[m]+R[0]);
  PC+=2;
  cycleCount++;
}

void SuperH::movwl4(void) {
  long m = Instruction::c(instruction);
  long disp = Instruction::d(instruction);

  R[0] = (long)(signed short)memoire->getWord(R[m]+(disp<<1));
  PC+=2;
  cycleCount++;
}

void SuperH::movwlg(void) {
  long disp = Instruction::cd(instruction);

  R[0] = (long)(signed short)memoire->getWord(GBR+(disp<<1));
  PC += 2;
  cycleCount++;
}

void SuperH::movwm(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  memoire->setWord(R[n] - 2,R[m]);
  R[n] -= 2;
  PC += 2;
  cycleCount++;
}

void SuperH::movwp(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  R[n] = (long)(signed short)memoire->getWord(R[m]);
  if (n != m)
    R[m] += 2;
  PC += 2;
  cycleCount++;
}

void SuperH::movws(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  memoire->setWord(R[n],R[m]);
  PC += 2;
  cycleCount++;
}

void SuperH::movws0(void) {
  memoire->setWord(R[Instruction::b(instruction)] + R[0],
	R[Instruction::c(instruction)]);
  PC+=2;
  cycleCount++;
}

void SuperH::movws4(void) {
  long disp = Instruction::d(instruction);
  long n = Instruction::c(instruction);

  memoire->setWord(R[n]+(disp<<1),R[0]);
  PC+=2;
  cycleCount++;
}

void SuperH::movwsg(void) {
  long disp = Instruction::cd(instruction);

  memoire->setWord(GBR+(disp<<1),R[0]);
  PC+=2;
  cycleCount++;
}

void SuperH::mull(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  MACL = R[n] * R[m];
  PC+=2;
  cycleCount += 2;
}

void SuperH::muls(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  MACL = ((long)(short)R[n]*(long)(short)R[m]);
  PC += 2;
  cycleCount++;
}

void SuperH::mulu(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  MACL = ((unsigned long)(unsigned short)R[n]
	*(unsigned long)(unsigned short)R[m]);
  PC+=2;
  cycleCount++;
}

void SuperH::neg(void) {
  R[Instruction::b(instruction)]=0-R[Instruction::c(instruction)];
  PC+=2;
  cycleCount++;
}

void SuperH::negc(void) {
  unsigned long temp;
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  
  temp=0-R[m];
  R[n] = temp - SR.partie.T;
  if (0 < temp) SR.partie.T=1;
  else SR.partie.T=0;
  if (temp < R[n]) SR.partie.T=1;
  PC+=2;
  cycleCount++;
}

void SuperH::nop(void) {
  PC += 2;
  cycleCount++;
}

void SuperH::y_not(void) {
  R[Instruction::b(instruction)] = ~R[Instruction::c(instruction)];
  PC += 2;
  cycleCount++;
}

void SuperH::y_or(void) {
  R[Instruction::b(instruction)] |= R[Instruction::c(instruction)];
  PC += 2;
  cycleCount++;
}

void SuperH::ori(void) {
  R[0] |= Instruction::cd(instruction);
  PC += 2;
  cycleCount++;
}

void SuperH::orm(void) {
  long temp;
  long source = Instruction::cd(instruction);

  temp = (long) memoire->getByte(GBR + R[0]);
  temp |= source;
  memoire->setByte(GBR + R[0],temp);
  PC += 2;
  cycleCount += 3;
}

void SuperH::rotcl(void) {
  long temp;
  long n = Instruction::b(instruction);

  if ((R[n]&0x80000000)==0) temp=0;
  else temp=1;
  R[n]<<=1;
  if (SR.partie.T == 1) R[n]|=0x00000001;
  else R[n]&=0xFFFFFFFE;
  if (temp==1) SR.partie.T=1;
  else SR.partie.T=0;
  PC+=2;
  cycleCount++;
}

void SuperH::rotcr(void) {
  long temp;
  long n = Instruction::b(instruction);

  if ((R[n]&0x00000001)==0) temp=0;
  else temp=1;
  R[n]>>=1;
  if (SR.partie.T == 1) R[n]|=0x80000000;
  else R[n]&=0x7FFFFFFF;
  if (temp==1) SR.partie.T=1;
  else SR.partie.T=0;
  PC+=2;
  cycleCount++;
}

void SuperH::rotl(void) {
  long n = Instruction::b(instruction);

  if ((R[n]&0x80000000)==0) SR.partie.T=0;
  else SR.partie.T=1;
  R[n]<<=1;
  if (SR.partie.T==1) R[n]|=0x00000001;
  else R[n]&=0xFFFFFFFE;
  PC+=2;
  cycleCount++;
}

void SuperH::rotr(void) {
  long n = Instruction::b(instruction);

  if ((R[n]&0x00000001)==0) SR.partie.T = 0;
  else SR.partie.T = 1;
  R[n]>>=1;
  if (SR.partie.T == 1) R[n]|=0x80000000;
  else R[n]&=0x7FFFFFFF;
  PC+=2;
  cycleCount++;
}

void SuperH::rte() {
  unsigned long temp;

  temp=PC;
  PC = memoire->getLong(R[15]); // + 4;
  R[15] += 4;
  SR.tout = memoire->getLong(R[15]) & 0x000003F3;
  R[15] += 4;
  _delai = temp + 2;
  cycleCount += 4;
}

void SuperH::rts() {
  unsigned long temp;
  
  temp = PC;
  PC = PR + 4;

  _delai = temp + 2;
  cycleCount += 2;
}

void SuperH::sett(void) {
  SR.partie.T = 1;
  PC += 2;
  cycleCount++;
}

void SuperH::shal(void) {
  long n = Instruction::b(instruction);
  if ((R[n] & 0x80000000) == 0) SR.partie.T = 0;
  else SR.partie.T = 1;
  R[n] <<= 1;
  PC += 2;
  cycleCount++;
}

void SuperH::shar(void) {
  long temp;
  long n = Instruction::b(instruction);

  if ((R[n]&0x00000001)==0) SR.partie.T = 0;
  else SR.partie.T = 1;
  if ((R[n]&0x80000000)==0) temp = 0;
  else temp = 1;
  R[n] >>= 1;
  if (temp == 1) R[n] |= 0x80000000;
  else R[n] &= 0x7FFFFFFF;
  PC += 2;
  cycleCount++;
}

void SuperH::shll(void) {
  long n = Instruction::b(instruction);

  if ((R[n]&0x80000000)==0) SR.partie.T=0;
  else SR.partie.T=1;
  R[n]<<=1;
  PC+=2;
  cycleCount++;
}

void SuperH::shll2(void) {
  R[Instruction::b(instruction)] <<= 2;
  PC+=2;
  cycleCount++;
}

void SuperH::shll8(void) {
  R[Instruction::b(instruction)]<<=8;
  PC+=2;
  cycleCount++;
}

void SuperH::shll16(void) {
  R[Instruction::b(instruction)]<<=16;
  PC+=2;
  cycleCount++;
}

void SuperH::shlr(void) {
  long n = Instruction::b(instruction);

  if ((R[n]&0x00000001)==0) SR.partie.T=0;
  else SR.partie.T=1;
  R[n]>>=1;
  PC+=2;
  cycleCount++;
}

void SuperH::shlr2(void) {
  long n = Instruction::b(instruction);
  R[n]>>=2;
  PC+=2;
  cycleCount++;
}

void SuperH::shlr8(void) {
  long n = Instruction::b(instruction);
  R[n]>>=8;
  PC+=2;
  cycleCount++;
}

void SuperH::shlr16(void) {
  long n = Instruction::b(instruction);
  R[n]>>=16;
  PC+=2;
  cycleCount++;
}

void SuperH::stcgbr(void) {
  long n = Instruction::b(instruction);
  R[n]=GBR;
  PC+=2;
  cycleCount++;
}

void SuperH::stcmgbr(void) {
  long n = Instruction::b(instruction);
  R[n]-=4;
  memoire->setLong(R[n],GBR);
  PC+=2;
  cycleCount += 2;
}

void SuperH::stcmsr(void) {
  long n = Instruction::b(instruction);
  R[n]-=4;
  memoire->setLong(R[n],SR.tout);
  PC+=2;
  cycleCount += 2;
}

void SuperH::stcmvbr(void) {
  long n = Instruction::b(instruction);
  R[n]-=4;
  memoire->setLong(R[n],VBR);
  PC+=2;
  cycleCount += 2;
}

void SuperH::stcsr(void) {
  long n = Instruction::b(instruction);
  R[n] = SR.tout;
  PC+=2;
  cycleCount++;
}

void SuperH::stcvbr(void) {
  long n = Instruction::b(instruction);
  R[n]=VBR;
  PC+=2;
  cycleCount++;
}

void SuperH::stsmach(void) {
  long n = Instruction::b(instruction);
  R[n]=MACH;
  PC+=2;
  cycleCount++;
}

void SuperH::stsmacl(void) {
  long n = Instruction::b(instruction);
  R[n]=MACL;
  PC+=2;
  cycleCount++;
}

void SuperH::stsmmach(void) {
  long n = Instruction::b(instruction);
  R[n] -= 4;
  memoire->setLong(R[n],MACH); 
  PC+=2;
  cycleCount++;
}

void SuperH::stsmmacl(void) {
  long n = Instruction::b(instruction);
  R[n] -= 4;
  memoire->setLong(R[n],MACL);
  PC+=2;
  cycleCount++;
}

void SuperH::stsmpr(void) {
  long n = Instruction::b(instruction);
  R[n] -= 4;
  memoire->setLong(R[n],PR);
  PC+=2;
  cycleCount++;
}

void SuperH::stspr(void) {
  long n = Instruction::b(instruction);
  R[n] = PR;
  PC+=2;
  cycleCount++;
}

void SuperH::sub(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  R[n]-=R[m];
  PC+=2;
  cycleCount++;
}

void SuperH::subc(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  unsigned long tmp0,tmp1;
  
  tmp1 = R[n] - R[m];
  tmp0 = R[n];
  R[n] = tmp1 - SR.partie.T;
  if (tmp0 < tmp1) SR.partie.T = 1;
  else SR.partie.T = 0;
  if (tmp1 < R[n]) SR.partie.T = 1;
  PC += 2;
  cycleCount++;
}

void SuperH::subv(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  long dest,src,ans;

  if ((long)R[n]>=0) dest=0;
  else dest=1;
  if ((long)R[m]>=0) src=0;
  else src=1;
  src+=dest;
  R[n]-=R[m];
  if ((long)R[n]>=0) ans=0;
  else ans=1;
  ans+=dest;
  if (src==1) {
    if (ans==1) SR.partie.T=1;
    else SR.partie.T=0;
  }
  else SR.partie.T=0;
  PC+=2;
  cycleCount++;
}

void SuperH::swapb(void) {
  unsigned long temp0,temp1;
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  temp0=R[m]&0xffff0000;
  temp1=(R[m]&0x000000ff)<<8;
  R[n]=(R[m]>>8)&0x000000ff;
  R[n]=R[n]|temp1|temp0;
  PC+=2;
  cycleCount++;
}

void SuperH::swapw(void) {
  unsigned long temp;
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  temp=(R[m]>>16)&0x0000FFFF;
  R[n]=R[m]<<16;
  R[n]|=temp;
  PC+=2;
  cycleCount++;
}

void SuperH::tas(void) {
  long temp;
  long n = Instruction::b(instruction);

  temp=(long) memoire->getByte(R[n]);
  if (temp==0) SR.partie.T=1;
  else SR.partie.T=0;
  temp|=0x00000080;
  memoire->setByte(R[n],temp);
  PC+=2;
  cycleCount += 4;
}

void SuperH::trapa(void) {
  long imm = Instruction::cd(instruction);

  R[15]-=4;
  memoire->setLong(R[15],SR.tout);
  R[15]-=4;
  memoire->setLong(R[15],PC - 2);
  PC = memoire->getLong(VBR+(imm<<2)) + 4;
  cycleCount += 8;
}

void SuperH::tst(void) {
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);
  if ((R[n]&R[m])==0) SR.partie.T = 1;
  else SR.partie.T = 0;
  PC += 2;
  cycleCount++;
}

void SuperH::tsti(void) {
  long temp;
  long i = Instruction::cd(instruction);

  temp=R[0]&i;
  if (temp==0) SR.partie.T = 1;
  else SR.partie.T = 0;
  PC+=2;
  cycleCount++;
}

void SuperH::tstm(void) {
  long temp;
  long i = Instruction::cd(instruction);

  temp=(long) memoire->getByte(GBR+R[0]);
  temp&=i;
  if (temp==0) SR.partie.T = 1;
  else SR.partie.T = 0;
  PC+=2;
  cycleCount += 3;
}

void SuperH::y_xor(void) {
  R[Instruction::b(instruction)] ^= R[Instruction::c(instruction)];
  PC += 2;
  cycleCount++;
}

void SuperH::xori(void) {
  long source = Instruction::cd(instruction);
  R[0] ^= source;
  PC += 2;
  cycleCount++;
}

void SuperH::xorm(void) {
  long source = Instruction::cd(instruction);
  long temp;

  temp = (long) memoire->getByte(GBR + R[0]);
  temp ^= source;
  memoire->setByte(GBR + R[0],temp);
  PC += 2;
  cycleCount += 3;
}

void SuperH::xtrct(void) {
  unsigned long temp;
  long m = Instruction::c(instruction);
  long n = Instruction::b(instruction);

  temp=(R[m]<<16)&0xFFFF0000;
  R[n]=(R[n]>>16)&0x0000FFFF;
  R[n]|=temp;
  PC+=2;
  cycleCount++;
}

void SuperH::sleep(void) {
  cerr << "SLEEP" << endl;
  cycleCount += 3;
}

SuperH::opcode SuperH::decode(void) {
  
  switch (Instruction::a(instruction)) {
  case 0:
    switch (Instruction::d(instruction)) {
    case 2:
      switch (Instruction::c(instruction)) {
      case 0: return &SuperH::stcsr;
      case 1: return &SuperH::stcgbr;
      case 2: return &SuperH::stcvbr;
      default: return &SuperH::undecoded;
      }
     
    case 3:
      switch (Instruction::c(instruction)) {
      case 0: return &SuperH::bsrf;
      case 2: return &SuperH::braf;
      default: return &SuperH::undecoded;
      }
     
    case 4: return &SuperH::movbs0;
    case 5: return &SuperH::movws0;
    case 6: return &SuperH::movls0;
    case 7: return &SuperH::mull;
    case 8:
      switch (Instruction::c(instruction)) {
      case 0: return &SuperH::clrt;
      case 1: return &SuperH::sett;
      case 2: return &SuperH::clrmac;
      default: return &SuperH::undecoded;
      }
     
    case 9:
      switch (Instruction::c(instruction)) {
      case 0: return &SuperH::nop;
      case 1: return &SuperH::div0u;
      case 2: return &SuperH::movt;
      default: return &SuperH::undecoded;
      }
     
    case 10:
      switch (Instruction::c(instruction)) {
      case 0: return &SuperH::stsmach;
      case 1: return &SuperH::stsmacl;
      case 2: return &SuperH::stspr;
      default: return &SuperH::undecoded;
      }
     
    case 11:
      switch (Instruction::c(instruction)) {
      case 0: return &SuperH::rts;
      case 1: return &SuperH::sleep;
      case 2: return &SuperH::rte;
      default: return &SuperH::undecoded;
      }
     
    case 12: return &SuperH::movbl0;
    case 13: return &SuperH::movwl0;
    case 14: return &SuperH::movll0;
    case 15: return &SuperH::macl;
    default: return &SuperH::undecoded;
    }
   
  case 1: return &SuperH::movls4;
  case 2:
    switch (Instruction::d(instruction)) {
    case 0: return &SuperH::movbs;
    case 1: return &SuperH::movws;
    case 2: return &SuperH::movls;
    case 4: return &SuperH::movbm;
    case 5: return &SuperH::movwm;
    case 6: return &SuperH::movlm;
    case 7: return &SuperH::div0s;
    case 8: return &SuperH::tst;
    case 9: return &SuperH::y_and;
    case 10: return &SuperH::y_xor;
    case 11: return &SuperH::y_or;
    case 12: return &SuperH::cmpstr;
    case 13: return &SuperH::xtrct;
    case 14: return &SuperH::mulu;
    case 15: return &SuperH::muls;
    default: return &SuperH::undecoded;
    }
   
  case 3:
    switch(Instruction::d(instruction)) {
    case 0:  return &SuperH::cmpeq;
    case 2:  return &SuperH::cmphs;
    case 3:  return &SuperH::cmpge;
    case 4:  return &SuperH::div1;
    case 5:  return &SuperH::dmulu;
    case 6:  return &SuperH::cmphi;
    case 7:  return &SuperH::cmpgt;
    case 8:  return &SuperH::sub;
    case 10: return &SuperH::subc;
    case 11: return &SuperH::subv;
    case 12: return &SuperH::add;
    case 13: return &SuperH::dmuls;
    case 14: return &SuperH::addc;
    case 15: return &SuperH::addv;
    default: return &SuperH::undecoded;
    }
   
  case 4:
    switch(Instruction::d(instruction)) {
    case 0:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::shll;
      case 1: return &SuperH::dt;
      case 2: return &SuperH::shal;
      default: return &SuperH::undecoded;
      }
     
    case 1:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::shlr;
      case 1: return &SuperH::cmppz;
      case 2: return &SuperH::shar;
      default: return &SuperH::undecoded;
      }
     
    case 2:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::stsmmach;
      case 1: return &SuperH::stsmmacl;
      case 2: return &SuperH::stsmpr;
      default: return &SuperH::undecoded;
      }
     
    case 3:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::stcmsr;
      case 1: return &SuperH::stcmgbr;
      case 2: return &SuperH::stcmvbr;
      default: return &SuperH::undecoded;
      }
     
    case 4:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::rotl;
      case 2: return &SuperH::rotcl;
      default: return &SuperH::undecoded;
      }
     
    case 5:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::rotr;
      case 1: return &SuperH::cmppl;
      case 2: return &SuperH::rotcr;
      default: return &SuperH::undecoded;
      }
     
    case 6:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::ldsmmach;
      case 1: return &SuperH::ldsmmacl;
      case 2: return &SuperH::ldsmpr;
      default: return &SuperH::undecoded;
      }
     
    case 7:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::ldcmsr;
      case 1: return &SuperH::ldcmgbr;
      case 2: return &SuperH::ldcmvbr;
      default: return &SuperH::undecoded;
      }
     
    case 8:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::shll2;
      case 1: return &SuperH::shll8;
      case 2: return &SuperH::shll16;
      default: return &SuperH::undecoded;
      }
     
    case 9:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::shlr2;
      case 1: return &SuperH::shlr8;
      case 2: return &SuperH::shlr16;
      default: return &SuperH::undecoded;
      }
     
    case 10:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::ldsmach;
      case 1: return &SuperH::ldsmacl;
      case 2: return &SuperH::ldspr;
      default: return &SuperH::undecoded;
      }
     
    case 11:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::jsr;
      case 1: return &SuperH::tas;
      case 2: return &SuperH::jmp;
      default: return &SuperH::undecoded;
      }
     
    case 14:
      switch(Instruction::c(instruction)) {
      case 0: return &SuperH::ldcsr;
      case 1: return &SuperH::ldcgbr;
      case 2: return &SuperH::ldcvbr;
      default: return &SuperH::undecoded;
      }
     
    case 15: return &SuperH::macw;
    default: return &SuperH::undecoded;
    }
   
  case 5: return &SuperH::movll4;
  case 6:
    switch (Instruction::d(instruction)) {
    case 0:  return &SuperH::movbl;
    case 1:  return &SuperH::movwl;
    case 2:  return &SuperH::movll;
    case 3:  return &SuperH::mov;
    case 4:  return &SuperH::movbp;
    case 5:  return &SuperH::movwp;
    case 6:  return &SuperH::movlp;
    case 7:  return &SuperH::y_not;
    case 8:  return &SuperH::swapb;
    case 9:  return &SuperH::swapw;
    case 10: return &SuperH::negc;
    case 11: return &SuperH::neg;
    case 12: return &SuperH::extub;
    case 13: return &SuperH::extuw;
    case 14: return &SuperH::extsb;
    case 15: return &SuperH::extsw;
    }
   
  case 7: return &SuperH::addi;
  case 8:
    switch (Instruction::b(instruction)) {
    case 0:  return &SuperH::movbs4;
    case 1:  return &SuperH::movws4;
    case 4:  return &SuperH::movbl4;
    case 5:  return &SuperH::movwl4;
    case 8:  return &SuperH::cmpim;
    case 9:  return &SuperH::bt;
    case 11: return &SuperH::bf;
    case 13: return &SuperH::bts;
    case 15: return &SuperH::bfs;
    default: return &SuperH::undecoded;
    }
   
  case 9: return &SuperH::movwi;
  case 10: return &SuperH::bra;
  case 11: return &SuperH::bsr;
  case 12:
    switch(Instruction::b(instruction)) {
    case 0:  return &SuperH::movbsg;
    case 1:  return &SuperH::movwsg;
    case 2:  return &SuperH::movlsg;
    case 3:  return &SuperH::trapa;
    case 4:  return &SuperH::movblg;
    case 5:  return &SuperH::movwlg;
    case 6:  return &SuperH::movllg;
    case 7:  return &SuperH::mova;
    case 8:  return &SuperH::tsti;
    case 9:  return &SuperH::andi;
    case 10: return &SuperH::xori;
    case 11: return &SuperH::ori;
    case 12: return &SuperH::tstm;
    case 13: return &SuperH::andm;
    case 14: return &SuperH::xorm;
    case 15: return &SuperH::orm;
    }
   
  case 13: return &SuperH::movli;
  case 14: return &SuperH::movi;
  default: return &SuperH::undecoded;
  }
}

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
    regs->PC = PC - 4;
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
    PC = regs->PC + 4;
    _delai = regs->delay;
  }
}

