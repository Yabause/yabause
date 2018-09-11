#ifndef SH2_DYNAREC_H
#define SH2_DYNAREC_H

#define SH2CORE_DYNAREC 2

void sh2_dynarec_init(void);
int verify_dirty(pointer addr);
void invalidate_all_pages(void);
void YabauseDynarecOneFrameExec(int, int);
int literalcount;
u32 literals[1024][2];

#if defined(TARGET_CPU_X86) || defined(__i386__)
#include "assem_x86.h"
#elif defined(TARGET_CPU_X86_64) || defined(__x86_64__)
#include "assem_x64.h"
// assem_arm64 isn't available, but will build on AMR64 this way
#elif defined(TARGET_CPU_ARM) || defined(TARGET_CPU_ARM64) || defined (__arm__)
#include "assem_arm.h"
//#elif defined(TARGET_CPU_ARM64) || defined (__arm64__)
//#include "assem_arm64.h"
#endif

#define MAX_OUTPUT_BLOCK_SIZE 262144
#define CLOCK_DIVIDER 1

struct ll_entry
{
    u32 vaddr;
    u32 reg32;
    void *addr;
    struct ll_entry *next;
};
u32 start;
u16 *source;
void *alignedsource;
u32 pagelimit;
char insn[MAXBLOCK][10];
unsigned char itype[MAXBLOCK];
unsigned char opcode[MAXBLOCK];
unsigned char opcode2[MAXBLOCK];
unsigned char opcode3[MAXBLOCK];
unsigned char addrmode[MAXBLOCK];
unsigned char bt[MAXBLOCK];
signed char rs1[MAXBLOCK];
signed char rs2[MAXBLOCK];
signed char rs3[MAXBLOCK];
signed char rt1[MAXBLOCK];
signed char rt2[MAXBLOCK];
unsigned char us1[MAXBLOCK];
unsigned char us2[MAXBLOCK];
unsigned char dep1[MAXBLOCK];
unsigned char dep2[MAXBLOCK];
signed char lt1[MAXBLOCK];
int imm[MAXBLOCK];
u32 ba[MAXBLOCK];
char is_ds[MAXBLOCK];
char ooo[MAXBLOCK];
u64 unneeded_reg[MAXBLOCK];
u64 branch_unneeded_reg[MAXBLOCK];
signed char regmap_pre[MAXBLOCK][HOST_REGS];
u32 cpmap[MAXBLOCK][HOST_REGS];
regstat regs[MAXBLOCK];
regstat branch_regs[MAXBLOCK];
signed char minimum_free_regs[MAXBLOCK];
u32 needed_reg[MAXBLOCK];
u32 wont_dirty[MAXBLOCK];
u32 will_dirty[MAXBLOCK];
int cycles[MAXBLOCK];
int ccadj[MAXBLOCK];
int slen;
pointer instr_addr[MAXBLOCK];
u32 yab_link_addr[MAXBLOCK][3];
int linkcount;
u32 stubs[MAXBLOCK*3][8];
int stubcount;
pointer ccstub_return[MAXBLOCK];
int is_delayslot;
u8 *out;
struct ll_entry *jump_in[2048];
struct ll_entry *jump_out[2048];
struct ll_entry *jump_dirty[2048];
ALIGNED(16) u32 hash_table[65536][4];
ALIGNED(16) char dyn_shadow[2097152];
char *copy;
int expirep;
unsigned int stop_after_jal;
//char invalid_code[0x100000];
char cached_code[0x20000];
char cached_code_words[2048*128];
u32 recent_writes[8];
u32 recent_write_index=0;
unsigned int slave;
u32 invalidate_count;
extern int master_reg[22];
extern int master_cc;
extern int master_pc; // Virtual PC
extern void * master_ip; // Translated PC
extern int slave_reg[22];
extern int slave_cc;
extern int slave_pc; // Virtual PC
extern void * slave_ip; // Translated PC
extern u8 restore_candidate[512];

// asm linkage
int sh2_recompile_block(int addr);
void *get_addr_ht(u32 vaddr);
void get_bounds(pointer addr,u32 *start,u32 *end);
void invalidate_addr(u32 addr);
void remove_hash(int vaddr);
void dyna_linker();
void verify_code();
void cc_interrupt();
void cc_interrupt_master();
void slave_entry();
void div1();
void macl();
void macw();
void master_handle_bios();
void slave_handle_bios();

// Needed by assembler
void wb_register(signed char r,signed char regmap[],u32 dirty);
void wb_dirtys(signed char i_regmap[],u32 i_dirty);
void wb_needed_dirtys(signed char i_regmap[],u32 i_dirty,int addr);
void load_regs(signed char entry[],signed char regmap[],int rs1,int rs2,int rs3);
void load_all_regs(signed char i_regmap[]);
void load_needed_regs(signed char i_regmap[],signed char next_regmap[]);
void load_regs_entry(int t);
void load_all_consts(signed char regmap[],u32 dirty,int i);

void *check_addr(u32 vaddr);
void *get_addr(u32 vaddr);
void add_link(u32 vaddr,void *src);
void add_stub(int type,int addr,int retaddr,int a,int b,int c,int d,int e);
void add_to_linker(int addr,int target,int ext);
void address_generation(int i,regstat *i_regs,signed char entry[]);
void alloc_all(regstat *cur,int i);
void alu_alloc(regstat *current,int i);
void alu_assemble(int i,regstat *i_regs);
void bios_assemble(int i,regstat *i_regs);
int can_direct_read(int address);
int can_direct_write(int address);
void cjump_assemble(int i,regstat *i_regs);
void clean_blocks(u32 page);
void clean_registers(int istart,int iend,int wr);
void clear_all_regs(signed char regmap[]);
void clear_const(regstat *cur,signed char reg);
void complex_alloc(regstat *current,int i);
void complex_assemble(int i,regstat *i_regs);
int count_free_regs(signed char regmap[]);
void delayslot_alloc(regstat *current,int i);
void dirty_reg(regstat *cur,signed char reg);
void disassemble_inst(int i);
void do_cc(int i,signed char i_regmap[],int *adj,int addr,int taken,int invert);
void do_ccstub(int n);
void do_consts(int i,u32 *isconst,u32 *constmap);
void ds_assemble_entry(int i);
void ds_assemble(int i,regstat *i_regs);
void DynarecMasterHandleInterrupts();
void DynarecSlaveHandleInterrupts();
void enabletrace();
void ext_alloc(regstat *current,int i);
void ext_assemble(int i,regstat *i_regs);
void flags_alloc(regstat *current,int i);
void flags_assemble(int i,regstat *i_regs);
signed char get_alt_reg(signed char regmap[],int r);
u64 get_const(regstat *cur,signed char reg);
int get_final_value(int hr, int i, int *value);
signed char get_reg2(signed char regmap1[],signed char regmap2[],int r);
signed char get_reg(signed char regmap[],int r);
void imm8_alloc(regstat *current,int i);
void imm8_assemble(int i,regstat *i_regs);
int internal_branch(int addr);
void invalidate_all_pages();
void invalidate_blocks(u32 firstblock,u32 lastblock);
void invalidate_page(u32 page);
int is_const(regstat *cur,signed char reg);
void ll_add_nodup(struct ll_entry **head,int vaddr,void *addr);
void ll_add(struct ll_entry **head,int vaddr,void *addr);
void ll_clear(struct ll_entry **head);
void ll_kill_pointers(struct ll_entry *head,int addr,int shift);
void ll_remove_matching_addrs(struct ll_entry **head,int addr,int shift);
void load_alloc(regstat *current,int i);
void load_assemble(int i,regstat *i_regs);
void load_consts(signed char pre[],signed char regmap[],int i);
void load_regs_bt(signed char i_regmap[],u32 i_dirty,int addr);
static void loop_preload(signed char pre[],signed char entry[]);
int loop_reg(int i, int r, int hr);
void lsn(unsigned char hsn[], int i, int *preferred_reg);
static pointer map_address(u32 address);
int match_bt(signed char i_regmap[],u32 i_dirty,int addr);
void memdebug(int i);
void mov_alloc(regstat *current,int i);
void mov_assemble(int i,regstat *i_regs);
void multdiv_alloc(regstat *current,int i);
void multdiv_assemble(int i,regstat *i_regs);
int needed_again(int r, int i);
void nullf(const char *format, ...);
void pcrel_alloc(regstat *current,int i);
void pcrel_assemble(int i,regstat *i_regs);
void rjump_assemble(int i,regstat *i_regs);
void rmw_alloc(regstat *current,int i);
void rmw_assemble(int i,regstat *i_regs);
void set_const(regstat *cur,signed char reg,u64 value);
void sh2_clear_const(u32 *isconst,u32 *constmap,signed char reg);
void sh2_dynarec_cleanup();
void sh2_dynarec_init();
void sh2_set_const(u32 *isconst,u32 *constmap,signed char reg,u64 value);
void SH2DynarecDeInit();
void FASTCALL SH2DynarecExec(SH2_struct *context, u32 cycles);
u32 SH2DynarecGetGBR(SH2_struct *context);
u32 SH2DynarecGetGPR(SH2_struct *context, int num);
u32 SH2DynarecGetMACH(SH2_struct *context);
u32 SH2DynarecGetMACL(SH2_struct *context);
u32 SH2DynarecGetPC(SH2_struct *context);
u32 SH2DynarecGetPR(SH2_struct *context);
void SH2DynarecGetRegisters(SH2_struct *context, sh2regs_struct *regs);
u32 SH2DynarecGetSR(SH2_struct *context);
u32 SH2DynarecGetVBR(SH2_struct *context);
int SH2DynarecInit(enum SHMODELTYPE model, SH2_struct *msh, SH2_struct *ssh);
void SH2DynarecReset(SH2_struct *context);
void SH2DynarecSetGBR(SH2_struct *context, u32 value);
void SH2DynarecSetGPR(SH2_struct *context, int num, u32 value);
void SH2DynarecSetMACH(SH2_struct *context, u32 value);
void SH2DynarecSetMACL(SH2_struct *context, u32 value);
void SH2DynarecSetPC(SH2_struct *context, u32 value);
void SH2DynarecSetPR(SH2_struct *context, u32 value);
void SH2DynarecSetRegisters(SH2_struct *context, const sh2regs_struct *regs);
void SH2DynarecSetSR(SH2_struct *context, u32 value);
void SH2DynarecSetVBR(SH2_struct *context, u32 value);
void SH2DynarecWriteNotify(u32 start, u32 length);
void shiftimm_alloc(regstat *current,int i);
void shiftimm_assemble(int i,regstat *i_regs);
void sjump_assemble(int i,regstat *i_regs);
void store_alloc(regstat *current,int i);
void store_assemble(int i,regstat *i_regs);
void store_regs_bt(signed char i_regmap[],u32 i_dirty,int addr);
void system_alloc(regstat *current,int i);
void system_assemble(int i,regstat *i_regs);
void ujump_assemble(int i,regstat *i_regs);
void unneeded_registers(int istart,int iend,int r);
void wb_invalidate(signed char pre[],signed char entry[],u32 dirty, u64 u);

#endif /* SH2_DYNAREC_H */
