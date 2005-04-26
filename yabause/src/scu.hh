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

#ifndef SCU_HH
#define SCU_HH

#include "memory.hh"
#include "intc.hh"
#include "superh.hh"

typedef struct {
  unsigned long ProgramRam[256];
  unsigned long MD[4][64];
#ifdef WORDS_BIGENDIAN
  union {
    struct {
       unsigned long unused1:5;
       unsigned long PR:1; // Pause cancel flag
       unsigned long EP:1; // Temporary stop execution flag
       unsigned long unused2:1;
       unsigned long T0:1; // D0 bus use DMA execute flag
       unsigned long S:1;  // Sine flag
       unsigned long Z:1;  // Zero flag
       unsigned long C:1;  // Carry flag
       unsigned long V:1;  // Overflow flag
       unsigned long E:1;  // Program end interrupt flag
       unsigned long ES:1; // Program step execute control bit
       unsigned long EX:1; // Program execute control bit
       unsigned long LE:1; // Program counter load enable bit
       unsigned long unused3:7;
       unsigned long P:8;  // Program Ram Address
    } part;
    unsigned long all;
  } ProgControlPort;
#else
  union {
    struct {
       unsigned long P:8;  // Program Ram Address
       unsigned long unused3:7;
       unsigned long LE:1; // Program counter load enable bit
       unsigned long EX:1; // Program execute control bit
       unsigned long ES:1; // Program step execute control bit
       unsigned long E:1;  // Program end interrupt flag
       unsigned long V:1;  // Overflow flag
       unsigned long C:1;  // Carry flag
       unsigned long Z:1;  // Zero flag
       unsigned long S:1;  // Sine flag
       unsigned long T0:1; // D0 bus use DMA execute flag
       unsigned long unused2:1;
       unsigned long EP:1; // Temporary stop execution flag
       unsigned long PR:1; // Pause cancel flag
       unsigned long unused1:5;
    } part;
    unsigned long all;
  } ProgControlPort;
#endif
  unsigned char PC;
  unsigned char TOP;
  unsigned short LOP;
  int jmpaddr;
  bool delayed;
  unsigned char DataRamPage;
  unsigned char DataRamReadAddress;
  unsigned char CT[4];
  unsigned long RX;
  unsigned long RY;
  unsigned long RA0;
  unsigned long WA0;

#ifdef WORDS_BIGENDIAN
  union {
    struct {
       long long unused:16;
       long long H:16;
       long long L:32;
    } part;
    long long all;
  } AC;

  union {
    struct {
       long long unused:16;
       long long H:16;
       long long L:32;
    } part;
    long long all;
  } P;

  union {
    struct {
       long long unused:16;
       long long H:16;
       long long L:32;
    } part;
    long long all;
  } ALU;

  union {
    struct {
       long long unused:16;
       long long H:16;
       long long L:32;
    } part;
    long long all;
  } MUL;
#else
  union {
    struct {
       long long L:32;
       long long H:16;
       long long unused:16;
    } part;
    long long all;
  } AC;

  union {
    struct {
       long long L:32;
       long long H:16;
       long long unused:16;
    } part;
    long long all;
  } P;

  union {
    struct {
       long long L:32;
       long long H:16;
       long long unused:16;
    } part;
    long long all;
  } ALU;

  union {
    struct {
       long long L:32;
       long long H:16;
       long long unused:16;
    } part;
    long long all;
  } MUL;
#endif

} scudspregs_struct;

typedef struct
{
  unsigned long addr;
} scucodebreakpoint_struct;

#define MAX_BREAKPOINTS 10

class SaturnMemory;

class Scu : public Memory {
private:
  template<unsigned char V, unsigned char L, unsigned short M>
	  void sendInterrupt(void);
  SaturnMemory *satmem;
  unsigned long timer0;
  unsigned long timer1;

  scudspregs_struct dsp;

public:
  Scu(SaturnMemory *);
  void reset(void);

  unsigned long getLong(unsigned long);
  void setLong(unsigned long, unsigned long);

  void DMA(int);

  unsigned long readgensrc(unsigned char num);
  void writed1busdest(unsigned char num, unsigned long val);
  void writeloadimdest(unsigned char num, unsigned long val);
  unsigned long readdmasrc(unsigned char num, unsigned char add);
  void writedmadest(unsigned char num, unsigned long val, unsigned char add);
  void run(unsigned long timing);
  void DSPDisasm(unsigned char addr, char *outstring);
  void DSPStep(void);
  void GetDSPRegisters(scudspregs_struct *regs);
  void SetDSPRegisters(scudspregs_struct *regs);

  scucodebreakpoint_struct codebreakpoint[MAX_BREAKPOINTS];
  int numcodebreakpoints;
  void (*BreakpointCallBack)(unsigned long);
  void SortCodeBreakpoints();
  bool inbreakpoint;
  void SetBreakpointCallBack(void (*func)(unsigned long));
  int AddCodeBreakpoint(unsigned long addr);
  int DelCodeBreakpoint(unsigned long addr);
  scucodebreakpoint_struct *GetBreakpointList();
  void ClearCodeBreakpoints();

                                      //source|vector | level | mask
                                      //------------------------------
  void sendVBlankIN(void);            // VDP2 | 40    | F     | 0x0001
  void sendVBlankOUT(void);           // VDP2 | 41    | E     | 0x0002
  void sendHBlankIN(void);            // VDP2 | 42    | D     | 0x0004
  void sendTimer0(void);              // SCU  | 43    | C     | 0x0008
  void sendTimer1(void);              // SCU  | 44    | B     | 0x0010
  void sendDSPEnd(void);              // SCU  | 45    | A     | 0x0020
  void sendSoundRequest(void);        // SCSP | 46    | 9     | 0x0040
  void sendSystemManager(void);       // SM   | 47    | 8     | 0x0080
  void sendPadInterrupt(void);        // PAD  | 48    | 8     | 0x0100
  void sendLevel2DMAEnd(void);        // ABus | 49    | 6     | 0x0200
  void sendLevel1DMAEnd(void);        // ABus | 4A    | 6     | 0x0400
  void sendLevel0DMAEnd(void);        // ABus | 4B    | 5     | 0x0800
  void sendDMAIllegal(void);          // SCU  | 4C    | 3     | 0x1000
  void sendDrawEnd(void);             // VDP1 | 4D    | 2     | 0x2000
  void sendExternalInterrupt00(void); // ABus | 50    | 7     | 0x8000
  void sendExternalInterrupt01(void); // ABus | 51    | 7     | 0x8000
  void sendExternalInterrupt02(void); // ABus | 52    | 7     | 0x8000
  void sendExternalInterrupt03(void); // ABus | 53    | 7     | 0x8000
  void sendExternalInterrupt04(void); // ABus | 54    | 4     | 0x8000
  void sendExternalInterrupt05(void); // ABus | 55    | 4     | 0x8000
  void sendExternalInterrupt06(void); // ABus | 56    | 4     | 0x8000
  void sendExternalInterrupt07(void); // ABus | 57    | 4     | 0x8000
  void sendExternalInterrupt08(void); // ABus | 58    | 1     | 0x8000
  void sendExternalInterrupt09(void); // ABus | 59    | 1     | 0x8000
  void sendExternalInterrupt10(void); // ABus | 5A    | 1     | 0x8000
  void sendExternalInterrupt11(void); // ABus | 5B    | 1     | 0x8000
  void sendExternalInterrupt12(void); // ABus | 5C    | 1     | 0x8000
  void sendExternalInterrupt13(void); // ABus | 5D    | 1     | 0x8000
  void sendExternalInterrupt14(void); // ABus | 5E    | 1     | 0x8000
  void sendExternalInterrupt15(void); // ABus | 5F    | 1     | 0x8000

  int saveState(FILE *fp);
  int loadState(FILE *fp, int version, int size);
};

#endif
