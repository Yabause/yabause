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

#include "memory.hh"
#include "intc.hh"
#include "cpu.hh"

class Instruction {
public:
  static inline unsigned long a   (unsigned long ul) { return ((ul & 0xF000) >> 12); }
  static inline unsigned long b   (unsigned long ul) { return ((ul & 0x0F00) >> 8); }
  static inline unsigned long c   (unsigned long ul) { return ((ul & 0x00F0) >> 4); }
  static inline unsigned long d   (unsigned long ul) { return (ul & 0x000F); }
  static inline unsigned long cd  (unsigned long ul) { return (ul & 0x00FF); }
  static inline unsigned long bcd (unsigned long ul) { return (ul & 0x0FFF); }
};

class SuperH : public Cpu {
private:
  unsigned long R[16];

  union {
    struct {
      unsigned long T:1;
      unsigned long S:1;
      unsigned long inutile2:2;
      unsigned long I:4;
      unsigned long Q:1;
      unsigned long M:1;
      unsigned long inutile1:22;
    } partie;
    unsigned long tout;
  } SR;
  unsigned long GBR;
  unsigned long VBR;

  unsigned long MACH;
  unsigned long MACL;
  unsigned long PR;
  unsigned long PC;

  Memory *memoire;

  typedef void (SuperH::*opcode)(void);
  opcode opcodes[0xFFFF];
  opcode decode(void);

  bool _interrupt;
  unsigned char _level;
  unsigned char _vector;
  unsigned long _delai;
  unsigned short instruction; // current opcode

  void _executer(void);
  unsigned long cycleCount;

  SDL_mutex *mutex[7];
  SDL_cond *cond[7];

  bool _pause;
  bool _run;
public:
  friend class Intc;
  friend void monitor(SuperH *);
  friend int main(int, char **);

  SuperH(void);
  ~SuperH(void);

  bool processingInterrupt(void);
  void interrupt(void);
  unsigned char& level(void);
  unsigned char& vector(void);

  void setMemory(Memory *);
  Memory *getMemory(void);

  void executer(void);
  static int lancer(SuperH *);
  void stop(void);
  void pause(void);
  void run(void);
  void step(void);
  template<int FREQ, int FRAME, int HIN, int HOUT, int VIN, int VOUT>
    void synchroStart(void);

  void microsleep(unsigned long);
  void waitHBlankIN(void);
  void waitHBlankOUT(void);
  void waitVBlankIN(void);
  void waitVBlankOUT(void);
  void waitInterruptEnd(void);

#ifndef _arch_dreamcast
  friend ostream& operator<<(ostream&, const SuperH&);
#endif

  void undecoded(void);

  /**** A ******************************************************************/
  void add(void); // addition de deux registres                   1
  void addi(void); // addition d'un entier et d'un registre        2
  void addc(void); // addition de deux registres avec retenue      3
  void addv(void); // addition de deux registres avec vérification 4
  void y_and(void); // fait le `et' logique entre deux registres    5
  void andi(void);      // fait le `et' logique entre un entier et R0   6
  void andm(void);      // fait le `et' logique entre un entier et R0   7
  /**** B ******************************************************************/
  void bf(void);      // branchement si T == 0                        8
  void bfs(void);      // branchement si T == 0 avec délai             9
  void bra(void);      // branchement inconditionnel (avec délai)     10
  void braf(void);      // bra inconditionnel éloigné (avec délai)     11
  void bsr(void);      // branchement vers un sous-programme          12
  void bsrf(void);      // branchement vers un sous-programme éloigné  13
  void bt(void);      // branchement si T == 1                       14
  void bts(void);      // branchement si T == 1 avec délai            15
  /**** C ******************************************************************/
  void clrmac(void);      // vide les registres MACH et MACL             16
  void clrt(void);      // place le bit T à 0                          17
  void cmpeq(void); // ==                                          18
  void cmpge(void); // >= signé                                    19
  void cmpgt(void); // > signé                                     20
  void cmphi(void); // > non signé                                 21
  void cmphs(void); // >= signé                                    22
  void cmpim(void);      // ==                                          23
  void cmppl(void);      // > 0                                         24
  void cmppz(void);      // >= 0                                        25
  void cmpstr(void); // ???                                         26
  /**** D ******************************************************************/
  void div0s(void); // \                                           27
  void div0u(void);      //  > division                                 28
  void div1(void); // /                                           29
  void dmuls(void); // multiplication                              30
  void dmulu(void); // multiplication                              31
  void dt(void);      // décrement et test                           32
  /**** E ******************************************************************/
  void extsb(void); // étend                                       33
  void extsw(void); // étend aussi    :-)                          34
  void extub(void); // étend toujours :-|                          35
  void extuw(void); // étend encore   :-(                          36
  /**** J ******************************************************************/
  void jmp(void);      // saut avec délai                             37
  void jsr(void);      // saut vers un sous-programme                 38
  /**** L ******************************************************************/
  void ldcgbr(void);      // charge le registre gbr                      39
  void ldcmgbr(void);      //                                             40
  void ldcmsr(void);      //                                             41
  void ldcmvbr(void);      //                                             42
  void ldcsr(void);      // charge le registre sr                       43
  void ldcvbr(void);      // charge le registre vbr                      44
  void ldsmach(void);      //                                             45
  void ldsmacl(void);      //                                             46
  void ldsmmach(void);      //                                             47
  void ldsmmacl(void);      //                                             48
  void ldsmpr(void);      //                                             49
  void ldspr(void);      //                                             50
  /**** M ******************************************************************/
  void macl(void); //                                             51
  void macw(void); //                                             53
  void mov(void); //                                             54
  void mova(void);      //                                             55
  void movbl(void); //                                             56
  void movbl0(void); //                                             57
  void movbl4(void); //                                             58
  void movblg(void);      //                                             59
  void movbm(void); //                                             60
  void movbp(void); //                                             61
  void movbs(void); //                                             62
  void movbs0(void); //                                             63
  void movbs4(void); //                                             64
  void movbsg(void);      //                                             65
  void movi(void); //                                             66
  void movli(void); //                                             67
  void movll(void); //                                             68
  void movll0(void); //                                             69
  void movll4(void); //                                        70
  void movllg(void);      //                                             71
  void movlm(void); //                                             72
  void movlp(void); //                                             73
  void movls(void); //                                             74
  void movls0(void); //                                             75
  void movls4(void); //                                        76
  void movlsg(void);      //                                             77
  void movt(void);      //                                             78
  void movwi(void); //                                             79
  void movwl(void); //                                             80
  void movwl0(void); //                                             81
  void movwl4(void); //                                             82
  void movwlg(void);      //                                             83
  void movwm(void); //                                             84
  void movwp(void); //                                             85
  void movws(void); //                                             86
  void movws0(void); //                                             87
  void movws4(void); //                                             88
  void movwsg(void);      //                                             89
  void mull(void); //                                             90
  void muls(void); //                                             91
  void mulu(void); //                                             92
  /**** N ******************************************************************/
  void neg(void); // négation                                    93
  void negc(void); // négation avec retenue                       94
  void nop(void);      // rien                                        95
  void y_not(void); // complément logique                          96
  /**** O ******************************************************************/
  void y_or(void); // ou                                          97
  void ori(void);      //                                             98
  void orm(void);      //                                             99
  /**** R ******************************************************************/
  void rotcl(void);      // rotation gauche                            100
  void rotcr(void);      // rotation droite                            101
  void rotl(void);      // rotation gauche                            102
  void rotr(void);      // rotation droite                            103
  void rte(void);      //                                            104
  void rts(void);      //                                            105
  /**** S ******************************************************************/
  void sett(void);      // T <- 1                                     106
  void shal(void);      // décalage gauche                            107
  void shar(void);      // décalage droite                            108
  void shll(void);      // décalage gauche                            109
  void shll2(void);      // décalage gauche                            110
  void shll8(void);      // décalage gauche                            111
  void shll16(void);      // décalage gauche                            112
  void shlr(void);      // décalage droite                            113
  void shlr2(void);      // décalage droite                            114
  void shlr8(void);      // décalage droite                            115
  void shlr16(void);      // décalage droite                            116
  void sleep(void);      // fait dodo...                               117
  void stcgbr(void);      //                                            118
  void stcmgbr(void);      //                                            119
  void stcmsr(void);      //                                            120
  void stcmvbr(void);      //                                            121
  void stcsr(void);      //                                            122
  void stcvbr(void);      //                                            124
  void stsmach(void);      //                                            125
  void stsmacl(void);      //                                            126
  void stsmmach(void);      //                                            127
  void stsmmacl(void);      //                                            128
  void stsmpr(void);      //                                            129
  void stspr(void);      //                                            130
  void sub(void); // soustraction                               131
  void subc(void); // soustraction avec retenue                  132
  void subv(void); // soustraction avec vérification             133
  void swapb(void); //                                            134
  void swapw(void); //                                            135
  /**** T ******************************************************************/
  void tas(void);      //                                            136
  void trapa(void);      //                                            137
  void tst(void); //                                            138
  void tsti(void);      //                                            139
  void tstm(void);      //                                            140
  /**** X ******************************************************************/
  void y_xor(void); // le shérif de l'espace                      141
  void xori(void);      //                                            142
  void xorm(void);      //                                            143
  void xtrct(void); //                                            144
};

#endif
