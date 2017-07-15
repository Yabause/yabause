/*  Copyright 2017 devMiyax(smiyaxdev@gmail.com)

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef _DYNAREC_SH2_H_
#define _DYNAREC_SH2_H_

#include <list>
#include <map>
#include <string>

#include <sys/types.h>
#include <stdint.h>
#include "threads.h"

//****************************************************
// Defiens
//****************************************************

// instruction Format
#define ZERO_F  0
#define N_F     1
#define M_F     2
#define NM_F    3
#define MD_F    4
#define ND4_F   5
#define NMD_F   6
#define D_F     7 
#define D12_F   8
#define ND8_F   9
#define I_F     10
#define NI_F    11

#ifdef _WINDOWS
#include <Windows.h>
#define ALLOCATE(x) VirtualAlloc(NULL, x, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#define FREEMEM(x,a)	if(x){ VirtualFree(x, a,MEM_RELEASE ); x = NULL;}
#elif defined(ARCH_IS_LINUX)
#include <sys/mman.h>
#define ALLOCATE(x) mmap (NULL, x, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_FILE|MAP_PRIVATE ,-1, 0);
//#define ALLOCATE(x) mmap ((void*)0x6000000, x, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS ,-1, 0);
#define FREEMEM(x,a) munmap(x,a);
#else
#define ALLOCATE(x)	malloc(x)
#define FREEMEM(x,a)	if(x){ free(x); x = NULL;}
#endif

const int MAX_INSTSIZE = 0xFFFF+1;

using std::list;
using std::map;
using std::string;

typedef list<u32> addrs;

struct CompileStaticsNode {
  u32 time;
  u32 count;
  u32 end_addr;
};

typedef map<u32, CompileStaticsNode> MapCompileStatics;

//****************************************************
// Structs
//****************************************************

const int NUMOFBLOCKS = 1024*4;
const int MAXBLOCKSIZE = 3072-(4*4);
#define MAINMEMORY_SIZE (0x100000);
#define ROM_SIZE (0x80000);

struct Block
{
  u8  code[MAXBLOCKSIZE];
  u32 b_addr; //beginning PC
  u32 e_addr; //ending PC
  u32 pad;
  u32 flags;
};

#define BLOCK_LOOP  (0x01)
#define BLOCK_WRITE (0x02)

#define IN_INFINITY_LOOP (-1)

// Sh2 Registris
struct tagSH2
{
  u32 GenReg[16];
  u32 CtrlReg[3];
  u32 SysReg[6];
  uintptr_t getmembyte;
  uintptr_t getmemword;
  uintptr_t getmemlong;
  uintptr_t setmembyte;
  uintptr_t setmemword;
  uintptr_t setmemlong;
  uintptr_t eachclock;
};

// Instruction
struct i_desc
{
  s32 format;
  const char *mnem;
  u16 mask;
  u16 bits;
  u8 dat;
  void (FASTCALL *func)(tagSH2*);
} ;

 
extern i_desc opcode_list[];


inline u32 adress_mask(u32 addr) {
  return (addr & 0x000FFFFF)>>1;
}

// Interrupt Table
struct dIntcTbl
{
  u8 Vector;
  u8 level;
};
typedef std::list<dIntcTbl> dlstIntct;


struct x86op_desc
{
  void(*func)();
  const unsigned short *size;
  const unsigned char *src;
  const unsigned char *dest;
  const unsigned char *off1;
  const unsigned char *imm;
  const unsigned char *off3;
  unsigned char delay;
  unsigned char cycle;
  unsigned char write_count;
  unsigned char build_count;

  x86op_desc(void(*ifunc)(), const unsigned short *isize, const unsigned char *isrc,
    const unsigned char *idest, const unsigned char *ioff1, const unsigned char *iimm,
    const unsigned char *ioff3, const unsigned char idelay, const unsigned char icycle, const unsigned char iwrite_count = 0)
  {
    func = ifunc;
    size = isize;
    src = isrc;
    dest = idest;
    off1 = ioff1;
    imm = iimm;
    off3 = ioff3;
    delay = idelay;
    cycle = icycle;
    write_count = iwrite_count;
    build_count = 0;
  };

};

#define SET_DIRTY
extern "C" {
  void DebugLog(const char * format, ...);
}

class CompileBlocks {
private:
  CompileBlocks(){
    debug_mode_ = false;
    BuildInstructionList();
    dCode = Init(dCode);
#ifdef SET_DIRTY
    LookupParentTable = new addrs[0x100000 >> 1];
#else
    LookupParentTable = NULL;
#endif
  }
  ~CompileBlocks(){
    FREEMEM(dCode, sizeof(Block)*NUMOFBLOCKS);
  }
  static CompileBlocks * instance_;

public:
  static CompileBlocks * getInstance(){
    if( instance_ == NULL ){
      instance_ = new CompileBlocks();
    }
    return instance_;
  }

  int blockCount;
  int LastMakeBlock; 
  bool debug_mode_;
  Block * g_CompleBlock;
  
  u8 dsh2_instructions[MAX_INSTSIZE];
  Block* LookupTable[0x100000>>1];    
  //addrs LookupParentTable[0x100000>>1];
  addrs * LookupParentTable;
  Block* LookupTableRom[0x80000>>1];
  Block* LookupTableLow[0x100000>>1];
  Block* LookupTableC[0x8000>>1];
  Block * dCode;
  
  inline void setDirty(u32 addr) {
    addr = adress_mask(addr);
    if (LookupParentTable[addr].size() == 0) return;
    for (auto it = LookupParentTable[addr].begin(); it != LookupParentTable[addr].end(); it++) {
      if (LookupTable[*it] != NULL) {
        for (u32 i = adress_mask(LookupTable[*it]->b_addr) ;
             i <= adress_mask(LookupTable[*it]->e_addr); i++ ) {
          if (i != addr) {
            LookupParentTable[i].remove(*it);
          }
        }
        LookupTable[*it] = NULL;
      }
    }
    LookupParentTable[addr].clear();
  }

  Block *Init(Block*);

  Block * CompileBlock( u32 pc, addrs * ParentT );

  void opcodePass(x86op_desc *op, u16 opcode, u8 *ptr);  
  int  opcodeIndex(u16 code );
  void FindOpCode(u16 opcode, u8 * instindex);
  void BuildInstructionList();

  int EmmitCode(Block *page, addrs * ParentT = NULL);

  // statics
  u32 compile_count_;
  u32 exec_count_;


  void ShowStatics();
  void SetDebugMode(bool debug) { debug_mode_ = debug;  }
};

typedef void(*dynaFunc)(tagSH2*);

class DynarecSh2
{
protected:
  SH2_struct *parent;
  tagSH2 *  m_pDynaSh2;
  CompileBlocks * m_pCompiler;
  int       m_ClockCounter;
  dlstIntct m_IntruptTbl;
  bool      m_bIntruptSort;
  bool one_step_;
  s32 pre_exe_count_;
  bool is_slave_ = false;
  u32 pre_PC_;
  SH2_struct * ctx_;
  YabMutex * mtx_;
  bool logenable_;

  u32 pre_cnt_;
  u32 interruput_chk_cnt_;
  u32 interruput_cnt_;
  u32 loopskip_cnt_;

  enum enDebugState {
    NORMAL,
    REQUESTED,
    COLLECTING,
    FINISHED,
  };
  
  enDebugState statics_trigger_ = NORMAL;
  MapCompileStatics compie_statics_;
  string message_buf;


public:
  DynarecSh2();
  ~DynarecSh2();
  static DynarecSh2 * CurrentContext;
  void SetCurrentContext(){ CurrentContext = this; }
  void SetSlave(bool is_slave) { is_slave_ = is_slave; }
  void SetContext(SH2_struct * ctx) { ctx_= ctx;}
  bool IsSlave() { return is_slave_;  }

  void AddInterrupt( u8 Vector, u8 level );
  int CheckInterupt();
  int InterruptRutine(u8 Vector, u8 level);
  int CheckOneStep();

  void ResetCPU();  
  void ExecuteCount(u32 Count );
  int Execute();
  void Undecoded();

  void ShowStatics();
  void ShowCompileInfo();
  void ResetCompileInfo();

  inline u32 * GetGenRegPtr() { return m_pDynaSh2->GenReg; }
  inline u32 GET_MACH() { return m_pDynaSh2->SysReg[0]; }
  inline u32 GET_MACL() { return m_pDynaSh2->SysReg[1]; }
  inline u32 GET_PR() { return m_pDynaSh2->SysReg[2]; }
  inline u32 GET_PC() { return m_pDynaSh2->SysReg[3]; }
  inline u32 GET_COUNT() { return m_pDynaSh2->SysReg[4]; } 
  inline u32 GET_ICOUNT() { return m_pDynaSh2->SysReg[5]; } 
  inline u32 GET_SR() { return m_pDynaSh2->CtrlReg[0]; }
  inline u32 GET_GBR() { return m_pDynaSh2->CtrlReg[1]; }
  inline u32 GET_VBR() { return m_pDynaSh2->CtrlReg[2]; }
  inline void SET_MACH( u32 v ) { m_pDynaSh2->SysReg[0] = v; }
  inline void SET_MACL( u32 v ) { m_pDynaSh2->SysReg[1] = v; }
  inline void SET_PR( u32 v ) { m_pDynaSh2->SysReg[2] = v; }
  inline void SET_PC( u32 v ) { m_pDynaSh2->SysReg[3] = v; }
  inline void SET_COUNT( u32 v ) { m_pDynaSh2->SysReg[4] = v; } 
  inline void SET_ICOUNT(u32 v ) { m_pDynaSh2->SysReg[5] = v; } 
  inline void SET_SR(u32 v ) { m_pDynaSh2->CtrlReg[0] = v; }
  inline void SET_GBR( u32 v ) { m_pDynaSh2->CtrlReg[1] = v; }
  inline void SET_VBR( u32 v ) { m_pDynaSh2->CtrlReg[2] = v; }  

  int GetCurrentStatics(MapCompileStatics & buf);
  int Resume();

};


// callback from cpu emulation
extern "C"
{
  int EachClock();
  int DelayEachClock();
  int DebugEachClock();
  int DebugDelayClock();

  void memSetByte(u32, u8 );
  void memSetWord(u32, u16);
  void memSetLong(u32, u32);

  u8 memGetByte(u32);
  u16 memGetWord(u32);
  u32 memGetLong(u32);  
  
}

#ifdef _WINDOWS

#define dynaLock()	__asm \
{                         \
    __asm push edx         \
   /*__asm push ebx*/         \
}
#define dynaFree()	__asm \
{                         \
   /*__asm pop ebx*/          \
   __asm pop edx          \
}

#else
#define dynaLock()
#define dynaFree()
#endif


#endif // _DYNAREC_SH2_H_