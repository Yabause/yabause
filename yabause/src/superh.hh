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

#ifndef SUPERH_HH
#define SUPERH_HH

#include "saturn_memory.hh"
#ifdef DYNAREC
#include "jit/hash.hh"
#include "../lightning/lightning.h"
#endif

typedef struct
{
  unsigned long R[16];

#ifdef WORDS_BIGENDIAN
  union {
    struct {
      unsigned long useless1:22;
      unsigned long M:1;
      unsigned long Q:1;
      unsigned long I:4;
      unsigned long useless2:2;
      unsigned long S:1;
      unsigned long T:1;
    } part;
    unsigned long all;
  } SR;
#else
  union {
    struct {
      unsigned long T:1;
      unsigned long S:1;
      unsigned long useless2:2;
      unsigned long I:4;
      unsigned long Q:1;
      unsigned long M:1;
      unsigned long useless1:22;
    } part;
    unsigned long all;
  } SR;
#endif
  unsigned long GBR;
  unsigned long VBR;

  unsigned long MACH;
  unsigned long MACL;
  unsigned long PR;
  unsigned long PC;
} sh2regs_struct;


typedef struct
{
  unsigned long addr;
  unsigned oldopcode;
} codebreakpoint_struct;

#define MAX_BREAKPOINTS 10

//typedef void (*BpCBFunc)(unsigned long addr);

class Instruction {
public:
  static inline unsigned long a   (unsigned long ul) { return ((ul & 0xF000) >> 12); }
  static inline unsigned long b   (unsigned long ul) { return ((ul & 0x0F00) >> 8); }
  static inline unsigned long c   (unsigned long ul) { return ((ul & 0x00F0) >> 4); }
  static inline unsigned long d   (unsigned long ul) { return (ul & 0x000F); }
  static inline unsigned long cd  (unsigned long ul) { return (ul & 0x00FF); }
  static inline unsigned long bcd (unsigned long ul) { return (ul & 0x0FFF); }
};

#ifdef DYNAREC
typedef int (*pifi)(void);

enum { BLOCK_FIRST , BLOCK_COMPILED , BLOCK_FAILED };


struct Block {
	int status;
	unsigned long cycleCount;
	unsigned long length;
	jit_insn codeBuffer[1024];
	pifi execute;
};

struct Jitreg {
        int reg;
        int sh2reg;
};

enum sh2reg_names {
        R0, R1, R2, R3, R4, R5, R6, R7,
        R8, R9, RA, RB, RC, RD, RE, RF,
        R_SR, R_GBR, R_VBR, R_MACH, R_MACL, R_PR, R_PC
};
#endif

class SuperH {
public:
#ifdef DYNAREC
  bool blockEnd;
  LRU * lru;
  Hash * hash;
  int currentBlock;
  struct Block * block;
  bool compile;
  bool compile_only;
  struct Jitreg jitreg[6];
  int sh2reg_jit[23];

  void mapReg(int);
  void flushRegs(void);
  int reg(int);
  void beginBlock(void);
  void endBlock(void);

  char * startAddress;
  bool verbose;
#endif

  unsigned long regs_array[23];

  sh2regs_struct * regs;

  SaturnMemory * memoire;

  typedef void (*opcode)(SuperH *);
  opcode opcodes[0x10000];
  opcode decode(void);

  unsigned short instruction; // current opcode

  bool isslave;
  priority_queue<Interrupt> interrupts;

  codebreakpoint_struct codebreakpoint[MAX_BREAKPOINTS];
  int numcodebreakpoints;
  void (*BreakpointCallBack)(bool, unsigned long);
  void SortCodeBreakpoints();
  bool inbreakpoint;

  Memory *purgeArea;	// 40000000 - 50000000
  Memory *adressArray;	// 60000000 - 600003FF
  Memory *dataArray;	// C0000000 - C0001000
  Memory *modeSdram;	// FFFF8000 - FFFFBFFF
  Onchip *onchip;	// FFFFFE00 - FFFFFFFF
  int timing;
  unsigned long cycleCount;
  friend class Onchip;
  friend int main(int, char *[]);

  SuperH(bool, SaturnMemory *);
 ~SuperH(void);
  void reset(void);

  void PowerOn();
  Memory *getMemory(void);

  void delay(unsigned long);

  void send(const Interrupt&);
  void sendNMI(void);
  void run(int);
  void runCycles(unsigned long);
  void step(void);

  void GetRegisters(sh2regs_struct *regs);
  void SetRegisters(sh2regs_struct *regs);

  void SetBreakpointCallBack(void (*func)(bool, unsigned long));
  int AddCodeBreakpoint(unsigned long addr);
  int DelCodeBreakpoint(unsigned long addr);
  codebreakpoint_struct *GetBreakpointList();
  void ClearCodeBreakpoints();

  int saveState(FILE *fp);
  int loadState(FILE *fp, int version, int size);

#ifndef _arch_dreamcast
  friend ostream& operator<<(ostream&, const SuperH&);
#endif

};

  void undecoded(SuperH *);

  /**** A ******************************************************************/
  void add(SuperH *); // addition de deux registres                   1
  void addi(SuperH *); // addition d'un entier et d'un registre        2
  void addc(SuperH *); // addition de deux registres avec retenue      3
  void addv(SuperH *); // addition de deux registres avec v�ification 4
  void y_and(SuperH *); // fait le `et' logique entre deux registres    5
  void andi(SuperH *);      // fait le `et' logique entre un entier et R0   6
  void andm(SuperH *);      // fait le `et' logique entre un entier et R0   7
  /**** B ******************************************************************/
  void bf(SuperH *);      // branchement si T == 0                        8
  void bfs(SuperH *);      // branchement si T == 0 avec d�ai             9
  void bra(SuperH *);      // branchement inconditionnel (avec d�ai)     10
  void braf(SuperH *);      // bra inconditionnel �oign�(avec d�ai)     11
  void bsr(SuperH *);      // branchement vers un sous-programme          12
  void bsrf(SuperH *);      // branchement vers un sous-programme �oign� 13
  void bt(SuperH *);      // branchement si T == 1                       14
  void bts(SuperH *);      // branchement si T == 1 avec d�ai            15
  /**** C ******************************************************************/
  void clrmac(SuperH *);      // vide les registres MACH et MACL             16
  void clrt(SuperH *);      // place le bit T �0                          17
  void cmpeq(SuperH *); // ==                                          18
  void cmpge(SuperH *); // >= sign�                                   19
  void cmpgt(SuperH *); // > sign�                                    20
  void cmphi(SuperH *); // > non sign�                                21
  void cmphs(SuperH *); // >= sign�                                   22
  void cmpim(SuperH *);      // ==                                          23
  void cmppl(SuperH *);      // > 0                                         24
  void cmppz(SuperH *);      // >= 0                                        25
  void cmpstr(SuperH *); // ???                                         26
  /**** D ******************************************************************/
  void div0s(SuperH *); // \                                           27
  void div0u(SuperH *);      //  > division                                 28
  void div1(SuperH *); // /                                           29
  void dmuls(SuperH *); // multiplication                              30
  void dmulu(SuperH *); // multiplication                              31
  void dt(SuperH *);      // d�rement et test                           32
  /**** E ******************************************************************/
  void extsb(SuperH *); // �end                                       33
  void extsw(SuperH *); // �end aussi    :-)                          34
  void extub(SuperH *); // �end toujours :-|                          35
  void extuw(SuperH *); // �end encore   :-(                          36
  /**** J ******************************************************************/
  void jmp(SuperH *);      // saut avec d�ai                             37
  void jsr(SuperH *);      // saut vers un sous-programme                 38
  /**** L ******************************************************************/
  void ldcgbr(SuperH *);      // charge le registre gbr                      39
  void ldcmgbr(SuperH *);      //                                             40
  void ldcmsr(SuperH *);      //                                             41
  void ldcmvbr(SuperH *);      //                                             42
  void ldcsr(SuperH *);      // charge le registre sr                       43
  void ldcvbr(SuperH *);      // charge le registre vbr                      44
  void ldsmach(SuperH *);      //                                             45
  void ldsmacl(SuperH *);      //                                             46
  void ldsmmach(SuperH *);      //                                             47
  void ldsmmacl(SuperH *);      //                                             48
  void ldsmpr(SuperH *);      //                                             49
  void ldspr(SuperH *);      //                                             50
  /**** M ******************************************************************/
  void macl(SuperH *); //                                             51
  void macw(SuperH *); //                                             53
  void mov(SuperH *); //                                             54
  void mova(SuperH *);      //                                             55
  void movbl(SuperH *); //                                             56
  void movbl0(SuperH *); //                                             57
  void movbl4(SuperH *); //                                             58
  void movblg(SuperH *);      //                                             59
  void movbm(SuperH *); //                                             60
  void movbp(SuperH *); //                                             61
  void movbs(SuperH *); //                                             62
  void movbs0(SuperH *); //                                             63
  void movbs4(SuperH *); //                                             64
  void movbsg(SuperH *);      //                                             65
  void movi(SuperH *); //                                             66
  void movli(SuperH *); //                                             67
  void movll(SuperH *); //                                             68
  void movll0(SuperH *); //                                             69
  void movll4(SuperH *); //                                        70
  void movllg(SuperH *);      //                                             71
  void movlm(SuperH *); //                                             72
  void movlp(SuperH *); //                                             73
  void movls(SuperH *); //                                             74
  void movls0(SuperH *); //                                             75
  void movls4(SuperH *); //                                        76
  void movlsg(SuperH *);      //                                             77
  void movt(SuperH *);      //                                             78
  void movwi(SuperH *); //                                             79
  void movwl(SuperH *); //                                             80
  void movwl0(SuperH *); //                                             81
  void movwl4(SuperH *); //                                             82
  void movwlg(SuperH *);      //                                             83
  void movwm(SuperH *); //                                             84
  void movwp(SuperH *); //                                             85
  void movws(SuperH *); //                                             86
  void movws0(SuperH *); //                                             87
  void movws4(SuperH *); //                                             88
  void movwsg(SuperH *);      //                                             89
  void mull(SuperH *); //                                             90
  void muls(SuperH *); //                                             91
  void mulu(SuperH *); //                                             92
  /**** N ******************************************************************/
  void neg(SuperH *); // n�ation                                    93
  void negc(SuperH *); // n�ation avec retenue                       94
  void nop(SuperH *);      // rien                                        95
  void y_not(SuperH *); // compl�ent logique                          96
  /**** O ******************************************************************/
  void y_or(SuperH *); // ou                                          97
  void ori(SuperH *);      //                                             98
  void orm(SuperH *);      //                                             99
  /**** R ******************************************************************/
  void rotcl(SuperH *);      // rotation gauche                            100
  void rotcr(SuperH *);      // rotation droite                            101
  void rotl(SuperH *);      // rotation gauche                            102
  void rotr(SuperH *);      // rotation droite                            103
  void rte(SuperH *);      //                                            104
  void rts(SuperH *);      //                                            105
  /**** S ******************************************************************/
  void sett(SuperH *);      // T <- 1                                     106
  void shal(SuperH *);      // d�alage gauche                            107
  void shar(SuperH *);      // d�alage droite                            108
  void shll(SuperH *);      // d�alage gauche                            109
  void shll2(SuperH *);      // d�alage gauche                            110
  void shll8(SuperH *);      // d�alage gauche                            111
  void shll16(SuperH *);      // d�alage gauche                            112
  void shlr(SuperH *);      // d�alage droite                            113
  void shlr2(SuperH *);      // d�alage droite                            114
  void shlr8(SuperH *);      // d�alage droite                            115
  void shlr16(SuperH *);      // d�alage droite                            116
  void sleep(SuperH *);      // fait dodo...                               117
  void stcgbr(SuperH *);      //                                            118
  void stcmgbr(SuperH *);      //                                            119
  void stcmsr(SuperH *);      //                                            120
  void stcmvbr(SuperH *);      //                                            121
  void stcsr(SuperH *);      //                                            122
  void stcvbr(SuperH *);      //                                            124
  void stsmach(SuperH *);      //                                            125
  void stsmacl(SuperH *);      //                                            126
  void stsmmach(SuperH *);      //                                            127
  void stsmmacl(SuperH *);      //                                            128
  void stsmpr(SuperH *);      //                                            129
  void stspr(SuperH *);      //                                            130
  void sub(SuperH *); // soustraction                               131
  void subc(SuperH *); // soustraction avec retenue                  132
  void subv(SuperH *); // soustraction avec v�ification             133
  void swapb(SuperH *); //                                            134
  void swapw(SuperH *); //                                            135
  /**** T ******************************************************************/
  void tas(SuperH *);      //                                            136
  void trapa(SuperH *);      //                                            137
  void tst(SuperH *); //                                            138
  void tsti(SuperH *);      //                                            139
  void tstm(SuperH *);      //                                            140
  /**** X ******************************************************************/
  void y_xor(SuperH *); // le sh�if de l'espace                      141
  void xori(SuperH *);      //                                            142
  void xorm(SuperH *);      //                                            143
  void xtrct(SuperH *); //                                            144

#endif
