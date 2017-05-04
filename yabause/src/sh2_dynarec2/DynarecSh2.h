#ifndef _DYNAREC_SH2_H_
#define _DYNAREC_SH2_H_

#include <list>

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
#else
#define ALLOCATE(x)	malloc(x)
#define FREEMEM(x)	if(x){ free(x); x = NULL;}
#endif

const int MAX_INSTSIZE = 0xFFFF+1;

//****************************************************
// 構造体
//****************************************************

const int NUMOFBLOCKS = 4240;
const int MAXBLOCKSIZE = 3200;
#define MAINMEMORY_SIZE (0x100000);
#define ROM_SIZE (0x80000);

struct Block
{
  unsigned char code[MAXBLOCKSIZE];
  unsigned long b_addr; //beginning PC
  unsigned long e_addr; //ending PC
  unsigned long reserved;
};

// Sh2 Registris
struct tagSH2
{
  u32 GenReg[16];
  u32 CtrlReg[3];
  u32 SysReg[6];
  u16 instr;
  u32 dmy[6];
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

  x86op_desc(void(*ifunc)(), const unsigned short *isize, const unsigned char *isrc,
    const unsigned char *idest, const unsigned char *ioff1, const unsigned char *iimm,
    const unsigned char *ioff3, const unsigned char idelay)
  {
    func = ifunc;
    size = isize;
    src = isrc;
    dest = idest;
    off1 = ioff1;
    imm = iimm;
    off3 = ioff3;
    delay = idelay;
  };

};

class CompileBlocks {
private:
  CompileBlocks(){
    BuildInstructionList();
    dCode = Init(dCode);
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
  Block * g_CompleBlock;
  
  u8 dsh2_instructions[MAX_INSTSIZE];
  u8* LookupTable[0x100000 >> 1];
  u16 LookupParentTable[0x100000 >> 1];
  u8* LookupTableRom[0x80000];
  u8* LookupTableLow[0x100000];
  u8* LookupTableC[0x8000];
  Block * dCode;
  
 
  Block *Init(Block*);

  u8 * CompileBlock( u32 pc, u32 * ParentT );

  void opcodePass(x86op_desc *op, u16 opcode, u8 *ptr);  
  int  opcodeIndex(u16 code );
  void FindOpCode(u16 opcode, u8 * instindex);
  void BuildInstructionList();

  void xlatPageInter(Block *page, u32 * ParentT = NULL);
  void xlatPageDebug(Block *page, u32 * ParentT = NULL);

  // statics
  u32 compile_count_;
  u32 exec_count_;

  void ShowStatics();
};

typedef void(*dynaFunc)(tagSH2*);

class DynarecSh2
{
  tagSH2 *  m_pDynaSh2;
  CompileBlocks * m_pCompiler;
  int       m_ClockCounter;
  dlstIntct m_IntruptTbl;
  bool      m_bIntruptSort;

public:
  DynarecSh2();
  static DynarecSh2 * CurrentContext;
  void SetCurrentContext(){ CurrentContext = this; }
  int InitContextLock();
  void RelaseContextLock();

  void AddInterrupt( u8 Vector, u8 level );
  int  CheckInterupt();
  int InterruptRutine(u8 Vector, u8 level);

  void ResetCPU();  
  void ExecuteCount( u32 Count );
  void Execute();

  u32 pre_cnt_;
  u32 interruput_chk_cnt_;
  u32 interruput_cnt_ ;
  void ShowStatics();
 
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
  
};


// callback from cpu emulation
extern "C"
{
  int EachClock();
  int DelayEachClock();
  int DebugEachClock();
  int DebugDeleayClock();

  void memSetByte(u32, u8 );
  void memSetWord(u32, u16);
  void memSetLong(u32, u32);

  u8 memGetByte(u32);
  u16 memGetWord(u32);
  u32 memGetLong(u32);  
  
}

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

#endif // _DYNAREC_SH2_H_