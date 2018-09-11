#ifndef assem_x64_H
#define assem_x64_H

#define HOST_REGS 8
#define HOST_CCREG 6
#define EXCLUDE_REG 4
#define SLAVERA_REG 5

#define SH2_REGS 23
#define MAXBLOCK 4096

typedef struct
{
    signed char regmap_entry[HOST_REGS];
    signed char regmap[HOST_REGS];
    u32 wasdirty;
    u32 dirty;
    u64 u;
    u32 wasdoingcp;
    u32 isdoingcp;
    u32 cpmap[HOST_REGS];
    u32 isconst;
    u32 constmap[SH2_REGS];
} regstat;

//#define IMM_PREFETCH 1
#define HOST_IMM_ADDR32 1
#define INVERTED_CARRY 1
#define DESTRUCTIVE_WRITEBACK 1
#define DESTRUCTIVE_SHIFT 1
#define POINTERS_64BIT 1

#define USE_MINI_HT 1

#define BASE_ADDR 0x70000000 // Code generator target address
#define TARGET_SIZE_2 25 // 2^25 = 32 megabytes
#define JUMP_TABLE_SIZE 0 // Not needed for x86

/* x86-64 calling convention:
   func(rdi, rsi, rdx, rcx, r8, r9) {return rax;}
   callee-save: %rbp %rbx %r12-%r15 */

#define ARG1_REG 7 /* RDI */
#define ARG2_REG 6 /* RSI */

#define EAX 0
#define ECX 1
#define EDX 2
#define EBX 3
#define ESP 4
#define EBP 5
#define ESI 6
#define EDI 7

extern u64 memory_map[1048576]; // 64-bit
ALIGNED(8) u32 mini_ht_master[32][2];
ALIGNED(8) u32 mini_ht_slave[32][2];
ALIGNED(4) u8 restore_candidate[512];
int rccount;
int master_reg[22];
int master_cc; // Cycle count
int master_pc; // Virtual PC
void * master_ip; // Translated PC
int slave_reg[22];
int slave_cc; // Cycle count
int slave_pc; // Virtual PC
void * slave_ip; // Translated PC

void FASTCALL WriteInvalidateLong(u32 addr, u32 val);
void FASTCALL WriteInvalidateWord(u32 addr, u32 val);
void FASTCALL WriteInvalidateByte(u32 addr, u32 val);
void FASTCALL WriteInvalidateByteSwapped(u32 addr, u32 val);

void jump_vaddr_eax_master();
void jump_vaddr_ecx_master();
void jump_vaddr_edx_master();
void jump_vaddr_ebx_master();
void jump_vaddr_ebp_master();
void jump_vaddr_edi_master();
void jump_vaddr_eax_slave();
void jump_vaddr_ecx_slave();
void jump_vaddr_edx_slave();
void jump_vaddr_ebx_slave();
void jump_vaddr_ebp_slave();
void jump_vaddr_edi_slave();

const pointer jump_vaddr_reg[2][8] = {
    {
        (pointer)jump_vaddr_eax_master,
        (pointer)jump_vaddr_ecx_master,
        (pointer)jump_vaddr_edx_master,
        (pointer)jump_vaddr_ebx_master,
        0,
        (pointer)jump_vaddr_ebp_master,
        0,
        (pointer)jump_vaddr_edi_master
    },{
        (pointer)jump_vaddr_eax_slave,
        (pointer)jump_vaddr_ecx_slave,
        (pointer)jump_vaddr_edx_slave,
        (pointer)jump_vaddr_ebx_slave,
        0,
        (pointer)jump_vaddr_ebp_slave,
        0,
        (pointer)jump_vaddr_edi_slave
    }
};

// We need these for cmovcc instructions on x86
u32 const_zero=0;
u32 const_one=1;

void *kill_pointer(void *stub);
void alloc_cc(regstat *cur,int i);
void alloc_reg_temp(regstat *cur,int i,signed char reg);
void alloc_reg(regstat *cur,int i,signed char reg);
void alloc_x86_reg(regstat *cur,int i,signed char reg,char hr);
void arch_init();
unsigned int count_bits(u32 reglist);
int do_dirty_stub(int i);
int do_map_r_branch(int map, int c, u32 addr, int *jaddr);
int do_map_r(int s,int ar,int map,int cache,int x,int a,int shift,int c,u32 addr);
void do_map_w_branch(int map, int c, u32 addr, int *jaddr);
int do_map_w(int s,int ar,int map,int cache,int x,int c,u32 addr);
void do_miniht_insert(int return_address,int rt,int temp);
void do_miniht_jump(int rs,int rh,int ht);
void do_miniht_load(int ht,int rh);
void do_preload_rhash(int r);
void do_preload_rhtbl(int r);
void do_readstub(int n);
void do_rhash(int rs,int rh);
void do_rmwstub(int n);
void do_unalignedwritestub(int n);
void do_writestub(int n);
void emit_adcimm(int imm,unsigned int rt);
void emit_adc(int rs,int rt);
void emit_addc(int s, int t, int sr);
void emit_addimm_and_set_flags(int imm,int rt);
void emit_addimm_no_flags(int imm,int rt);
void emit_addimm64_32(int rsh,int rsl,int imm,int rth,int rtl);
void emit_addimm64(int rs,int imm,int rt);
void emit_addimm(int rs,int imm,int rt);
void emit_adds(int rs1,int rs2,int rt);
void emit_add(int rs1,int rs2,int rt);
void emit_andimm(int rs,int imm,int rt);
void emit_and(unsigned int rs1,unsigned int rs2,unsigned int rt);
void emit_callreg(unsigned int r);
void emit_call(int a);
void emit_cdq();
void emit_cmova_reg(int rs,int rt);
void emit_cmovl_reg(int rs,int rt);
void emit_cmovle_reg(int rs,int rt);
void emit_cmovl(u32 *addr,int rt);
void emit_cmovnc_reg(int rs,int rt);
void emit_cmovne_reg(int rs,int rt);
void emit_cmovne(u32 *addr,int rt);
void emit_cmovnp_reg(int rs,int rt);
void emit_cmovp_reg(int rs,int rt);
void emit_cmovs_reg(int rs,int rt);
void emit_cmovs(u32 *addr,int rt);
void emit_cmpeqimm(int s, int imm, int sr, int temp);
void emit_cmpeq(int s1, int s2, int sr, int temp);
void emit_cmpge(int s1, int s2, int sr, int temp);
void emit_cmpgt(int s1, int s2, int sr, int temp);
void emit_cmphi(int s1, int s2, int sr, int temp);
void emit_cmphs(int s1, int s2, int sr, int temp);
void emit_cmpimm(int rs,int imm);
void emit_cmpmem_imm_byte(pointer addr,int imm);
void emit_cmpmem_indexedsr12_imm(int addr,int r,int imm);
void emit_cmpmem_indexed(int addr,int rs,int rt);
void emit_cmpmem(int addr,int rt);
void emit_cmppl(int s, int sr, int temp);
void emit_cmppz(int s, int sr);
void emit_cmpstr(int s1, int s2, int sr, int temp);
void emit_cmp(int rs,int rt);
void emit_cvttpd2dq(unsigned int ssereg1,unsigned int ssereg2);
void emit_cvttps2dq(unsigned int ssereg1,unsigned int ssereg2);
void emit_div0s(int s1, int s2, int sr, int temp);
void emit_div(int rs);
void emit_dt(int t, int sr);
void emit_extjump(pointer addr, int target);
void emit_fabs();
void emit_faddl(int r);
void emit_fadds(int r);
void emit_fadd(int r);
void emit_fchs();
void emit_fdivl(int r);
void emit_fdivs(int r);
void emit_fdiv(int r);
void emit_fildll(int r);
void emit_fildl(int r);
void emit_fistpll(int r);
void emit_fistpl(int r);
void emit_fldcw_indexed(int addr,int r);
void emit_fldcw_stack();
void emit_fldcw(int addr);
void emit_fldl(int r);
void emit_flds(int r);
void emit_fmull(int r);
void emit_fmuls(int r);
void emit_fmul(int r);
void emit_fnstcw_stack();
void emit_fpop();
void emit_fsqrt();
void emit_fstpl(int r);
void emit_fstps(int r);
void emit_fsubl(int r);
void emit_fsubs(int r);
void emit_fsub(int r);
void emit_fucomip(unsigned int r);
void emit_idiv(int rs);
void emit_imul(int rs);
void emit_jc(int a);
void emit_jeq(int a);
void emit_jge(int a);
void emit_jl(int a);
void emit_jmpmem_indexed(u32 addr,unsigned int r);
void emit_jmpreg(unsigned int r);
void emit_jmp(int a);
void emit_jne(int a);
void emit_jno(int a);
void emit_jns(int a);
void emit_js(int a);
void emit_lea8(int rs1,int rt);
void emit_leairrx1(int imm,int rs1,int rs2,int rt);
void emit_leairrx4(int imm,int rs1,int rs2,int rt);
void emit_load_return_address(unsigned int rt);
void emit_loadreg(int r, int hr);
void emit_mov2imm_compact(int imm1,unsigned int rt1,int imm2,unsigned int rt2);
void emit_mov64(int rs,int rt);
void emit_movd_store(unsigned int ssereg,unsigned int addr);
void emit_movimm64(u64 imm,unsigned int rt);
void emit_movimm(int imm,unsigned int rt);
void emit_movmem_indexedx4_addr32(int addr, int rs, int rt);
void emit_movmem_indexedx4(int addr, int rs, int rt);
void emit_movmem_indexedx8_64(int addr, int rs, int rt);
void emit_movmem_indexedx8(int addr, int rs, int rt);
void emit_movq(u64 addr, int rt);
void emit_movsbl_indexed_map(int addr, int rs, int map, int rt);
void emit_movsbl_indexed(int addr, int rs, int rt);
void emit_movsbl_map(u64 addr, int map, int rt);
void emit_movsbl_reg(int rs, int rt);
void emit_movsbl(u64 addr, int rt);
void emit_movsd_load(unsigned int addr,unsigned int ssereg);
void emit_movss_load(unsigned int addr,unsigned int ssereg);
void emit_movswl_indexed_map(int addr, int rs, int map, int rt);
void emit_movswl_indexed(int addr, int rs, int rt);
void emit_movswl_map(u64 addr, int map, int rt);
void emit_movswl_reg(int rs, int rt);
void emit_movswl(u64 addr, int rt);
void emit_mov(int rs,int rt);
void emit_movzbl_indexed_map(int addr, int rs, int map, int rt);
void emit_movzbl_indexed(int addr, int rs, int rt);
void emit_movzbl_map(int addr, int map, int rt);
void emit_movzbl_reg(int rs, int rt);
void emit_movzbl(int addr, int rt);
void emit_movzwl_indexed(int addr, int rs, int rt);
void emit_movzwl_map(int addr, int map, int rt);
void emit_movzwl_reg(int rs, int rt);
void emit_movzwl(int addr, int rt);
void emit_multiply(int rs1,int rs2,int rt);
void emit_mul(int rs);
void emit_negc(int rs, int rt, int sr);
void emit_negs(int rs, int rt);
void emit_neg(int rs, int rt);
void emit_not(int rs,int rt);
void emit_or_and_set_flags(int rs1,int rs2,int rt);
void emit_orimm(int rs,int imm,int rt);
void emit_or(unsigned int rs1,unsigned int rs2,unsigned int rt);
void emit_popreg(unsigned int r);
void emit_prefetch(void *addr);
void emit_pushimm(int imm);
void emit_pushreg(unsigned int r);
void emit_readword_indexed_map(int addr, int rs, int map, int rt);
void emit_readword_indexed(int addr, int rs, int rt);
void emit_readword_map(int addr, int map, int rt);
void emit_readword(u64 addr, int rt);
void emit_rmw_andimm(int addr, int map, int imm);
void emit_rmw_orimm(int addr, int map, int imm);
void emit_rmw_xorimm(int addr, int map, int imm);
void emit_rorimm(int rs,unsigned int imm,int rt);
void emit_rotclsr(int t, int sr);
void emit_rotcrsr(int t, int sr);
void emit_rotlsr(int t, int sr);
void emit_rotl(int t);
void emit_rotrsr(int t, int sr);
void emit_rotr(int t);
void emit_sarcl(int r);
void emit_sarimm(int rs,unsigned int imm,int rt);
void emit_sarsr(int t, int sr);
void emit_sbbimm(int imm,unsigned int rt);
void emit_sbb(int rs1,int rs2);
void emit_set_gz32(int rs, int rt);
void emit_set_gz64_32(int rsh, int rsl, int rt);
void emit_set_if_carry32(int rs1, int rs2, int rt);
void emit_set_if_carry64_32(int u1, int l1, int u2, int l2, int rt);
void emit_set_if_less32(int rs1, int rs2, int rt);
void emit_set_if_less64_32(int u1, int l1, int u2, int l2, int rt);
void emit_set_nz32(int rs, int rt);
void emit_set_nz64_32(int rsh, int rsl, int rt);
void emit_setl(int rt);
void emit_sh2tas(int addr, int map, int sr);
void emit_sh2tstimm(int s, int imm, int sr, int temp);
void emit_sh2tst(int s1, int s2, int sr, int temp);
void emit_shlcl(int r);
void emit_shldcl(int r1,int r2);
void emit_shldimm(int rs,int rs2,unsigned int imm,int rt);
void emit_shlimm64(int rs,unsigned int imm,int rt);
void emit_shlimm(int rs,unsigned int imm,int rt);
void emit_shlsr(int t, int sr);
void emit_shrcl(int r);
void emit_shrdcl(int r1,int r2);
void emit_shrdimm(int rs,int rs2,unsigned int imm,int rt);
void emit_shrimm64(int rs,unsigned int imm,int rt);
void emit_shrimm(int rs,unsigned int imm,int rt);
void emit_shrsr(int t, int sr);
void emit_slti32(int rs,int imm,int rt);
void emit_slti64_32(int rsh,int rsl,int imm,int rt);
void emit_sltiu32(int rs,int imm,int rt);
void emit_sltiu64_32(int rsh,int rsl,int imm,int rt);
void emit_storereg(int r, int hr);
void emit_subc(int s, int t, int sr);
void emit_subs(int rs1,int rs2,int rt);
void emit_sub(int rs1,int rs2,int rt);
void emit_swapb(int rs,int rt);
void emit_test64(int rs, int rt);
void emit_testimm(int rs,int imm);
void emit_test(int rs, int rt);
void emit_writebyte_imm(int imm, int addr);
void emit_writebyte_indexed_map(int rt, int addr, int rs, int map, int temp);
void emit_writebyte_indexed(int rt, int addr, int rs);
void emit_writebyte_map(int rt, int addr, int map);
void emit_writebyte(int rt, int addr);
void emit_writedword_imm32(int imm, int addr);
void emit_writehword_indexed_map(int rt, int addr, int rs, int map, int temp);
void emit_writehword_indexed(int rt, int addr, int rs);
void emit_writehword_map(int rt, int addr, int map);
void emit_writehword(int rt, int addr);
void emit_writeword_imm_esp(int imm, int addr);
void emit_writeword_imm(int imm, int addr);
void emit_writeword_indexed_map(int rt, int addr, int rs, int map, int temp);
void emit_writeword_indexed(int rt, int addr, int rs);
void emit_writeword_map(int rt, int addr, int map);
void emit_writeword(int rt, int addr);
void emit_xchg(int rs, int rt);
void emit_xorimm(int rs,int imm,int rt);
void emit_xor(unsigned int rs1,unsigned int rs2,unsigned int rt);
void emit_zeroreg(int rt);
void gen_tlb_addr_r(int ar, int map);
void gen_tlb_addr_w(int ar, int map);
void generate_map_const(u32 addr,int reg);
void get_bounds(pointer addr,u32 *start,u32 *end);
pointer get_clean_addr(pointer addr);
pointer get_pointer(void *stub);
void inline_readstub(int type, int i, u32 addr, signed char regmap[], int target, int adj, u32 reglist);
void inline_writestub(int type, int i, u32 addr, signed char regmap[], int target, int adj, u32 reglist);
int isclean(pointer addr);
void literal_pool_jumpover(int n);
void literal_pool(int n);
void output_byte(u8 byte);
void output_modrm(u8 mod,u8 rm,u8 ext);
void output_rex(u8 w,u8 r,u8 x,u8 b);
void output_sib(u8 scale,u8 index,u8 base);
void output_w32(u32 word);
void output_w64(u64 word);
void printregs(int edi,int esi,int ebp,int esp,int b,int d,int c,int a);
void restore_regs(u32 reglist);
void save_regs(u32 reglist);
void set_jump_target(pointer addr,pointer target);
int verify_dirty(pointer addr);
void wb_valid(signed char pre[],signed char entry[],u32 dirty_pre,u32 dirty,u64 u);

#endif /* assem_x64_H */
