/*  src/psp/psp-sh2.c: SH-2 emulator for PSP with dynamic translation support
    Copyright 2009 Andrew Church

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

/*************************************************************************/

/*
 * This SH-2 emulator is designed to execute SH-2 instructions by
 * translating them into equivalent MIPS instructions for the PSP and
 * executing those MIPS instructions, working as best as possible to
 * maintain a 1:1 instruction ratio.  This is complicated by several
 * factors:
 *
 * - The SH-2 is big-endian, while the MIPS implementation used on the PSP
 *   is little-endian.
 *
 * - While the SH-2 is generally considered a RISC architecture, it
 *   supports unusually (for a RISC architecture) complex addressing modes
 *   as well as a CISC-like ALU flag register.
 *
 * - The MIPS architecture has more general purpose registers (31 excluding
 *   $zero) than the SH-2 has registers in total (23), but the MIPS ABI
 *   only provides 9 callee-saved registers (10 if $gp is included, but GCC
 *   expects $gp to be unmodified on entry to a subroutine), and proper
 *   emulation will at times require calling external routines for memory
 *   access.
 *
 * - Stores by SH-2 code may modify memory which was previously translated,
 *   so every store must be checked against a bitmap of modified pages of
 *   memory.
 *
 * - The emulation must maintain an accurate clock cycle count and check at
 *   reasonably frequent intervals whether the cycle limit for the current
 *   execution quantum has been reached.
 *
 * The emulation (handled by psp_sh2_exec()) proceeds as follows:
 *
 * 1) If there is no currently executing block, psp_sh2_exec() calls
 *    jit_find() on the current PC to check whether there is a translated
 *    block starting at that address.
 *
 * 2) If no translated block exists, psp_sh2_exec() calls jit_translate() to
 *    translate a block beginning at the current PC.  jit_translate()
 *    continues translating through the end of the block of code
 *    (determined heuristically) and returns the translated block to be
 *    executed.
 *
 * 3A) psp_sh2_exec() calls jit_exec(), which sets up registers for the
 *     current, found, or created translated block, then  calls the current
 *     execution address within the block (which is the initial address if
 *     the block was just searched or created).
 *
 * 3B) If translation failed for some reason, psp_sh2_exec() calls
 *     interpret_insn() to interpret and execute one instruction at the
 *     current PC, then starts over at step 1.
 *
 * 4) The translated code returns either when it reaches the end of the
 *    block, or when the number of cycles executed exceeds the requested
 *    cycle count.  (For efficiency, cycle count checks are only performed
 *    before branching instructions.)
 *
 * 5) If the translated code returns before the end of the block, the block
 *    is recorded as the currently executing block, and the next execution
 *    address is saved so that execution can be resumed there on the next
 *    psp_sh2_exec() call.  Otherwise, the currently executing block is
 *    cleared.
 *
 * 6A) If a write is made to a translated block, the block's translation is
 *     deleted by calling jit_clear_write() so the modified code can be
 *     retranslated.
 *
 * 6B) If an external agent (such as DMA) modifies memory, jit_clear_range()
 *     should be called to clear any translations in the affected area.  In
 *     this case, the modified range is expanded to a multiple of
 *     1<<JIT_PAGE_BITS bytes for efficiency, assuming that such writes will
 *     tend to cover relatively large areas rather than single addresses.
 *
 * ------------------------------------------------------------------------
 *
 * Register usage by translated code is as follows:
 *
 * $at     -- temporary (used for value calculation)
 * $v0     -- temporary (used for value calculation)
 * $v1     -- temporary (used for value calculation)
 * $a0     -- last direct-access address, bits 19-31
 * $a1     -- native base pointer for memory region starting at $a0
 * $a2     -- SH-2 PC register
 * $a3     -- SH-2 SR register
 * $t0-$t7 -- SH-2 registers R0-R7
 * $s0-$s7 -- SH-2 registers R8-R15
 * $t8     -- temporary (used for address calculation)
 * $t9     -- temporary; indirect function call pointer
 * $gp     -- SH-2 GBR register
 * $s8     -- processor state block
 * $ra     -- not used
 *
 * Stack usage: (32 bytes total)
 *
 *  0($sp) -- temporary (used in load/store logic)
 *  4($sp) -- cycle limit for execution
 *  8($sp) -- saved internal $at (for calling external routines)
 * 12($sp) -- saved internal $v0 (for calling external routines)
 * 16($sp) -- saved internal $v1 (for calling external routines)
 * 20($sp) -- saved internal $ra (for calling external routines)
 * 24($sp) -- saved $gp
 * 28($sp) -- saved $ra
 *
 * The MACH, MACL, PR, and VBR registers are not mirrored to native
 * registers (accesses go directly to the processor state block).
 *
 * ------------------------------------------------------------------------
 *
 * The following are potential optimizations that could be made to the
 * translated code but are not (yet) implemented:
 *
 * - The GBR and SR registers could be loaded as needed, as is done with
 *   the general-purpose registers, rather than always being loaded and
 *   stored on code entry and exit.
 *
 * - Since the PC at each translated instruction is constant at the time of
 *   translation, all PC-relative accesses (loads, stores, branches, and
 *   MOVA instructions) could be replaced with constant values.  The PC
 *   register could then be updated only on code return (via termination or
 *   interrupt), saving the expense of an "addiu REG_PC, REG_PC, 2" for
 *   every SH-2 instruction.  Whether this would in fact result in faster
 *   code, however, is unclear (due to the extra overhead required to load
 *   32-bit values in MIPS code).
 *
 * - As an extension to the above, constants embedded into SH-2 code which
 *   are accessed using PC-relative loads could be extracted and converted
 *   to translation-time constants, eliminating the expensive load
 *   operation.  (The is_data[] array is intended for this purpose.)  This
 *   could be extended to tracking register values, allowing loads from or
 *   stores to known regions of memory to use more optimized code than the
 *   generic load/store operations.
 *
 * - MIPS mirror registers could be assigned dynamically rather than
 *   statically.  This would in particular allow most blocks to avoid
 *   register spills when calling external routines (e.g. MappedMemoryRead*
 *   or MappedMemoryWrite*).  However, dynamic register allocation would
 *   complicate branching logic, requiring block-level tracking of which
 *   SH-2 registers are loaded into which MIPS registers.
 *
 * - At a significant memory cost, native code addresses and register state
 *   could be stored for every potential code word.  This would require a
 *   minimum of 12 bytes (JitEntry pointer, native address, register map)
 *   and potentially much more if registers are assigned dynamically, but
 *   could cleanly handle dynamic branches, which seem to be fairly common
 *   in SH-2 code.
 *
 * - Load instructions could be floated up within a block to reduce the
 *   number of load stalls in the code.  This would require an additional
 *   pass through the entire block (or a method to record all potential
 *   branch targets, such as an extension of GET_LABEL()) to ensure that
 *   loads are not moved across branch targets.
 *
 * Additionally, purging of entries from a full table could be optimized by
 * keeping a doubly-linked list of entries sorted by last-used time, moving
 * each entry to the head of the list as it's used.  However, the speed
 * penalty caused by updating the list pointers on every call may outweigh
 * the savings from not having to traverse the entire array on a purge
 * operation.
 *
 * ------------------------------------------------------------------------
 *
 * Note that a fair amount of voodoo is required to allow tracing of
 * JIT-translated instructions.  Reading code located between
 * #ifdef TRACE{,_ALL,_LIKE_SH2INT} and the corresponding #else or #endif
 * may be dangerous to your health.
 */

/*************************************************************************/
/************************* Configuration options *************************/
/*************************************************************************/

/*============ General options ============*/

/**
 * ENABLE_JIT:  When defined, enables the use of dynamic translation.
 * Automatically disabled when compiling on non-PSP platforms.
 */
#define ENABLE_JIT

/**
 * JIT_ACCURATE_ACCESS_TIMING:  When defined, checks the current cycle
 * count against the cycle limit before each load or store operation to
 * ensure that external accesses occur at the proper times (as compared to
 * interpreted execution).
 */
#define JIT_ACCURATE_ACCESS_TIMING

/**
 * JIT_ALLOW_FORWARD_BRANCHES:  When defined, the translator will scan
 * forward past an unconditional branch for branch targets later in the
 * SH-2 code and attempt to include them in the same block.  Otherwise,
 * only a branch targeting the instruction immediately following the
 * branch's delay slot (or targeting the delay slot itself) will be
 * considered as part of the same block.
 *
 * FIXME:  Defining this seems to cause crashes if the block subsequently
 * exceeds the JIT_INSNS_PER_BLOCK limit, though it's unclear whether
 * that's because of this in particular or just a bug with the block
 * termination code.
 */
// #define JIT_ALLOW_FORWARD_BRANCHES

/**
 * JIT_TABLE_SIZE:  Specifies the size of the dynamic translation (JIT)
 * routine table.  The larger the table, the more translated code can be
 * retained in memory, but the greater the cost of stores that overwrite
 * previously-translated code.  This should always be a prime number.
 */
#ifndef JIT_TABLE_SIZE
# define JIT_TABLE_SIZE 2003
#endif

/**
 * JIT_DATA_LIMIT:  Specifies the maximum total size of translated code,
 * in bytes of native code.  When this limit is reached, the least recently
 * used translations will be purged from memory to make room for new
 * translations.
 */
#ifndef JIT_DATA_LIMIT
# define JIT_DATA_LIMIT 20000000
#endif

/**
 * JIT_INSNS_PER_BLOCK:  Specifies the maximum number of SH-2 instructions
 * (or words of local data) to translate in a single block.  A larger value
 * allows larger blocks of code to be translated as a single unit,
 * potentially increasing execution speed at the cost of longer translation
 * delays.
 */
#ifndef JIT_INSNS_PER_BLOCK
# define JIT_INSNS_PER_BLOCK 512
#endif

/**
 * JIT_BLOCK_EXPAND_SIZE:  Specifies the block size (in bytes) by which to
 * expand the native code buffer when translating.
 */
#ifndef JIT_BLOCK_EXPAND_SIZE
# define JIT_BLOCK_EXPAND_SIZE 1024
#endif

/**
 * JIT_LOCAL_LABEL_SIZE:  Specifies the size of the local label table, used
 * in translating SH-2 instructions.  The default value normally does not
 * need to be changed unless the translation implementation is modified.
 */
#ifndef JIT_LOCAL_LABEL_SIZE
# define JIT_LOCAL_LABEL_SIZE 8
#endif

/**
 * JIT_BTCACHE_SIZE:  Specifies the size of the branch target cache, in
 * instructions.  A larger cache increases the amount of SH-2 code that can
 * be translated as a single block, but increases the time required to
 * translate branch instructions.
 */
#ifndef JIT_BTCACHE_SIZE
# define JIT_BTCACHE_SIZE 256
#endif

/**
 * JIT_UNRES_BRANCH_SIZE:  Specifies the size of unresolved branch table,
 * in bytes.  A larger table size increases the complexity of SH-2 code
 * that can be translated as a single block, but increases the time
 * required to translate all instructions.
 */
#ifndef JIT_UNRES_BRANCH_SIZE
# define JIT_UNRES_BRANCH_SIZE 20
#endif

/**
 * JIT_PAGE_BITS:  Specifies the page size used for checking whether a
 * store operation affects a previously-translated block, in powers of two
 * (e.g. a value of 8 means a page size of 256 bytes).  A larger page size
 * decreases the amount of memory needed for the page tables, but increases
 * the chance of an ordinary data write triggering an expensive check of
 * translated blocks.
 */
#ifndef JIT_PAGE_BITS
# define JIT_PAGE_BITS 8
#endif

/**
 * JIT_BLACKLIST_SIZE:  Specifies the size of the blacklist used for
 * tracking regions of memory that should not be translated due to runtime
 * modifications by nearby code.  A smaller blacklist size increases the
 * speed of handling writes to such regions as well as the speed of code
 * translation, but also increases the chance of thrashing on the table,
 * which can significantly degrade performance.
 */
#ifndef JIT_BLACKLIST_SIZE
# define JIT_BLACKLIST_SIZE 10
#endif

/**
 * JIT_BLACKLIST_EXPIRE:  Specifies the time after which a blacklist entry
 * will expire if it has not been written to.  The unit is calls to
 * jit_exec(), so the optimal value will need to be found through
 * experimentation.  (Since jit_exec() is called for every instruction when
 * tracing, blacklist expiration is disabled when tracing is active.)
 */
#ifndef JIT_BLACKLIST_EXPIRE
# define JIT_BLACKLIST_EXPIRE 1000000
#endif

/**
 * JIT_CALL_STACK_SIZE:  Specifies the size of the call stack, used to
 * optimize subroutine calls by remembering the location in native code
 * from which the subroutine was called, thus avoiding the need to
 * retranslate from the return address after executing an RTS.  A larger
 * call stack increases the depth of nested subroutines which can be
 * tracked, but slows down the processing of mismatched call/return pairs
 * (such as when a called routine manually pops the return address off
 * the stack and jumps elsewhere) or returns to routines which have since
 * been purged from memory.
 */
#ifndef JIT_CALL_STACK_SIZE
# define JIT_CALL_STACK_SIZE 10
#endif

/*============ Optimization options ============*/

/**
 * OPTIMIZE_STATE_BLOCK:  When defined, attempts to minimize accesses to
 * the processor state block (such as register loads and stores).
 */
#define OPTIMIZE_STATE_BLOCK

/**
 * OPTIMIZE_IDLE:  When defined, attempts to find "idle loops", i.e. loops
 * which continue indefinitely until some external event occurs, and modify
 * their behavior to increase processing speed.  Specifically, when an idle
 * loop finishes an iteration and branches back to the beginning of the
 * loop, the virtual processor will consume all pending execution cycles
 * immediately rather than continue executing the loop until the requested
 * number of cycles have passed.  (This can result in a slight timing
 * discrepancy from real hardware, since the number of cycles in the loop
 * may not be a factor of the number of cycles executed in each call to
 * psp_sh2_exec().)
 */
#define OPTIMIZE_IDLE

/**
 * OPTIMIZE_IDLE_MAX_INSNS:  When OPTIMIZE_IDLE is defined, specifies the
 * maximum number of instructions to consider when looking at a single
 * potential idle loop.
 */
#ifndef OPTIMIZE_IDLE_MAX_INSNS
# define OPTIMIZE_IDLE_MAX_INSNS 8
#endif

/**
 * OPTIMIZE_DIVISION:  When defined, attempts to find instruction sequences
 * that perform division operations and replace them with native division
 * instructions.  On the PSP, this provides a speedup of roughly 18x
 * (928 -> 51 cycles).
 */
#define OPTIMIZE_DIVISION

/*============ Debugging options ============*/

/**
 * TRACE, TRACE_ALL:  When TRACE defined, all instructions are traced using
 * the interface in ../sh2trace.h.  If TRACE_ALL is also defined, every
 * instruction is traced, even when dynamic translation is used; otherwise,
 * only the first instruction of each native code segment is traced,
 * letting subsequent instructions execute normally.  Additionally, if
 * TRACE_ALL is defined, all stores to memory are traced as well.
 *
 * If TRACE is not defined, TRACE_ALL is ignored.
 */
// #define TRACE
// #define TRACE_ALL

/**
 * TRACE_LIKE_SH2INT:  When defined and TRACE_ALL is also defined, alters
 * certain behaviors to match the core interpreter (../sh2int.c).  This
 * option is intended only for verifying the accuracy of the instruction
 * decoder and should be disabled for normal usage.
 */
// #define TRACE_LIKE_SH2INT

/**
 * JIT_DEBUG_VERBOSE:  When defined, debug messages are output in certain
 * cases considered useful in fine-tuning the translation and optimization.
 */
// #define JIT_DEBUG_VERBOSE

/**
 * JIT_DEBUG_TRACE:  When defined, a trace line is printed for each SH-2
 * instruction translated.  This option is independent of the other trace
 * options.
 */
// #define JIT_DEBUG_TRACE

/**
 * JIT_DEBUG_INSERT_PC:  When defined, causes the native code generator to
 * insert dummy instructions at the beginning of the code for each SH-2
 * instruction, indicating the SH-2 PC for that instruction.  The dummy
 * instructions are of the form:
 *     lui $zero, 0x1234
 *     ori $zero, $zero, 0x5678
 * for SH-2 PC 0x12345678.
 */
// #define JIT_DEBUG_INSERT_PC

/*************************************************************************/

/* Sanity-check configuration options */

#ifndef PSP
# undef ENABLE_JIT
#endif

#ifndef TRACE
# undef TRACE_ALL
#endif

#ifndef TRACE_ALL
# undef TRACE_LIKE_SH2INT
#endif

/*************************************************************************/
/*************************** Required headers ****************************/
/*************************************************************************/

#include <stdint.h>
#include "../yabause.h"
#include "../bios.h"
#include "../error.h"
#include "../memory.h"
#include "../sh2core.h"

#include "common.h"
#include "mips.h"
#include "psp-sh2.h"

#ifdef TRACE
# include "../sh2trace.h"
#endif
#ifdef JIT_DEBUG_TRACE
# include "../sh2d.h"
#endif

/*************************************************************************/
/********************* Yabause interface declaration *********************/
/*************************************************************************/

/* Interface function declarations (must come before interface definition) */

static int psp_sh2_init(void);
static void psp_sh2_deinit(void);
static int psp_sh2_reset(void);
static FASTCALL void psp_sh2_exec(SH2_struct *state, u32 cycles);
static void psp_sh2_write_notify(u32 start, u32 length);


/* Module interface definition */

SH2Interface_struct SH2PSP = {
    .id          = SH2CORE_PSP,
    .Name        = "PSP SH-2 Core",
    .Init        = psp_sh2_init,
    .DeInit      = psp_sh2_deinit,
    .Reset       = psp_sh2_reset,
    .Exec        = psp_sh2_exec,
    .WriteNotify = psp_sh2_write_notify,
};

/*************************************************************************/
/************* Local constants, data, and other declarations *************/
/*************************************************************************/

/* SH-2 related constants */

#define SR_T    0x001
#define SR_S    0x002
#define SR_Q    0x100
#define SR_M    0x200

#define SR_T_SHIFT  0
#define SR_S_SHIFT  1
#define SR_Q_SHIFT  8
#define SR_M_SHIFT  9

/*************************************************************************/

/* Array of directly-accessible page pointers, indexed by bits 19-31 of the
 * SH-2 address; each pointer is either NULL (meaning the page is not
 * directly accessible) or a value which, when added to the low 19 bits of
 * the SH-2 address, gives the corresponding native address (ignoring byte
 * order issues).  All pages are assumed to be organized as 16-bit units in
 * native byte order. */

static uint8_t *direct_pages[0x2000];

/*************************************************************************/

/* JIT translation data structure */
typedef struct JitEntry_ JitEntry;
struct JitEntry_ {
    JitEntry *next, *prev;    // Hash table collision chain pointers
    uint32_t sh2_start;       // Code start address in SH-2 address space
                              //    (zero indicates a free entry)
    uint32_t sh2_end;         // Code end address in SH-2 address space
    void *native_code;        // Pointer to native code
    uint32_t native_length;   // Length of native code (bytes)
    uint32_t native_size;     // Size of native code buffer (bytes)
    void *exec_address;       // Next execution address (NULL if not started)
    uint32_t timestamp;       // Time this entry was added
    uint8_t running;          // Nonzero if entry is currently running
    uint8_t must_clear;       // Nonzero if entry must be cleared on completion
};

/* Hash function */
#define JIT_HASH(addr)  ((uint32_t)(addr) % JIT_TABLE_SIZE)

/*-----------------------------------------------------------------------*/

#ifdef ENABLE_JIT

/* Hash table of translated routines (shared between virtual processors) */
static JitEntry jit_table[JIT_TABLE_SIZE];       // Record buffer
static JitEntry *jit_hashchain[JIT_TABLE_SIZE];  // Hash collision chains

/* Total size of translated data */
static int32_t jit_total_data;  // Signed to protect against its going negative

/* Global timestamp for LRU expiration mechanism */
static uint32_t jit_timestamp;

/* Address from which data is being read */
static uint32_t jit_PC;

/* Cycle accumulator (accumulates cycles from a block of instructions so
 * the field doesn't have to be updated with every instruction) */
static int jit_cycles;

/* Bitmask of "active" registers (registers whose MIPS mirrors are valid) */
static uint16_t active_regs;

/* Bitmask of changed registers (registers which have not yet been flushed) */
static uint16_t changed_regs;

/* Flags indicating the status of each word in the current block (0: code,
 * 1: data, -1: unknown) */
static int8_t is_data[JIT_INSNS_PER_BLOCK];

/* Local label reference table (used to resolve forward references to local
 * labels used in micro-op definitions) */
static struct {
    int in_use;
    uint32_t id;
    uint32_t native_offset;
    uint32_t sh2_address;  // Used for error messages
} local_labels[JIT_LOCAL_LABEL_SIZE];

/* Branch target address list (indicates all SH-2 addresses in the current
 * block which are targeted by branches within the same block) */
static uint32_t btlist[JIT_BTCACHE_SIZE];
static int btlist_len;  // Number of valid entries in btlist[]

/* Branch target lookup table (indicates where in the native code each
 * address is located) */
static struct {
    uint32_t sh2_address;   // Address of SH-2 instruction
    uint16_t active_regs;   // Active register bitmask
    uint32_t native_offset; // Byte offset into entry->native_code
    uint32_t pad;           // Pad entry size to 16 bytes
} btcache[JIT_BTCACHE_SIZE];
static unsigned int btcache_index;  // Where to store the next instruction

/* Unresolved branch list (saves locations and targets of forward branches) */
static struct {
    uint32_t sh2_target;    // Branch target (SH-2 address, 0 = unused entry)
    uint16_t active_regs;   // Active register bitmask
    uint32_t native_offset; // Offset of native branch instruction to update
    int32_t check_offset;   // Offset of associated cycle check, -1 if none
} unres_branches[JIT_UNRES_BRANCH_SIZE];

/* JIT translation blacklist; note that entries are always word-aligned */
static struct {
    uint32_t start, end;  // Zero in both fields indicates an empty entry
    uint32_t timestamp;   // jit_timestamp when region was last written to
    uint32_t pad;           // Pad entry size to 16 bytes
} blacklist[JIT_BLACKLIST_SIZE];

/* JIT call stack entry format (data is stored in processor state block) */
typedef struct CallStackEntry_ {
    uint32_t return_PC;     // PC on return from subroutine (0 = unused)
    JitEntry *return_entry; // JIT block containing native code
    void *return_native;    // Native address to jump to
    uint32_t pad;           // Pad entry size to 16 bytes
} CallStackEntry;

#endif  // ENABLE_JIT

/*-----------------------------------------------------------------------*/

/* Bitmap of pages in low/high work RAM containing translated code */
static uint8_t jit_pages_low [0x100000 >> (JIT_PAGE_BITS+3)];
static uint8_t jit_pages_high[0x100000 >> (JIT_PAGE_BITS+3)];
#define JIT_PAGE_SET(base,page)    ((base)[(page)>>3] |=   1<<((page)&7))
#define JIT_PAGE_TEST(base,page)   ((base)[(page)>>3] &    1<<((page)&7))
#define JIT_PAGE_CLEAR(base,page)  ((base)[(page)>>3] &= ~(1<<((page)&7)))

/* Array of pointers to JIT page bitmaps (set up like direct_pages[]) */
static uint8_t *direct_jit_pages[0x2000];

/*************************************************************************/

/* Local function declarations */

#if defined(__GNUC__) && defined(TRACE_LIKE_SH2INT)  // Not used in this case
__attribute__((unused))
#endif
static int check_interrupts(SH2_struct *state);

#ifdef ENABLE_JIT
JitEntry *jit_find(uint32_t address);
JitEntry *jit_translate(SH2_struct *state, uint32_t address);
static int jit_scan_block(JitEntry *entry, uint32_t address);
static inline void jit_exec(SH2_struct *state, JitEntry *entry, int cycles);
static void jit_clear_write(SH2_struct *state, uint32_t address);
static void jit_clear_range(uint32_t address, uint32_t size);
#endif  // ENABLE_JIT

static void interpret_insn(SH2_struct *state);
#ifdef ENABLE_JIT
static int translate_insn(SH2_struct *state, JitEntry *entry);
#endif

#ifdef OPTIMIZE_IDLE
static int can_optimize_idle(const uint16_t *insn_ptr, uint32_t PC,
                             unsigned int count);
#endif
#ifdef OPTIMIZE_DIVISION
static int can_optimize_div0u(const uint16_t *insn_ptr, uint32_t PC,
                              int *Rhi_ret, int *Rlo_ret, int *Rdiv_ret);
static int can_optimize_div0s(const uint16_t *insn_ptr, uint32_t PC,
                              int Rhi, int *Rlo_ret, int Rdiv);
#endif

#ifdef ENABLE_JIT
static int append_insn(JitEntry *entry, uint32_t opcode);
static int append_block(JitEntry *entry, const void *block, int length);
static int append_prologue(JitEntry *entry);
static int append_epilogue(JitEntry *entry);
static int expand_buffer(JitEntry *entry);
static void clear_entry(JitEntry *entry);
static void clear_oldest_entry(void);
static int ref_local_label(JitEntry *entry, uint32_t id);
static void define_local_label(JitEntry *entry, uint32_t id);
static int clear_local_labels(void);
static int32_t btcache_lookup(uint32_t address, uint16_t *active_ret);
static void record_unresolved_branch(JitEntry *entry, uint32_t sh2_target,
                                     uint32_t native_offset,
                                     int32_t check_offset);
static inline int fixup_branch(JitEntry *entry, uint32_t offset,
                               uint32_t target);
static inline void flush_cache(void);
__attribute__((const)) static inline int timestamp_compare(
    uint32_t reference, uint32_t a, uint32_t b);
#endif  // ENABLE_JIT

/*************************************************************************/
/********************** External interface routines **********************/
/*************************************************************************/

/**
 * psp_sh2_init:  Initialize the SH-2 core.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     FIXME: unused by caller
 */
static int psp_sh2_init(void)
{
#ifdef ENABLE_JIT
    /* Allocate call stacks for each processor */
    MSH2->psp.call_stack = malloc(sizeof(CallStackEntry) * JIT_CALL_STACK_SIZE);
    SSH2->psp.call_stack = malloc(sizeof(CallStackEntry) * JIT_CALL_STACK_SIZE);
    if (!MSH2->psp.call_stack || !SSH2->psp.call_stack) {
        DMSG("Out of memory allocating call stacks");
        free(MSH2->psp.call_stack);
        free(SSH2->psp.call_stack);
        MSH2->psp.call_stack = SSH2->psp.call_stack = NULL;
        return -1;
    }
#endif

    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sh2_deinit:  Perform cleanup for the SH-2 core.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_sh2_deinit(void)
{
    /* Free call stacks */
    free(MSH2->psp.call_stack);
    free(SSH2->psp.call_stack);
    MSH2->psp.call_stack = SSH2->psp.call_stack = NULL;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sh2_reset:  Reset the SH-2 core.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     FIXME: unused by caller
 */
static int psp_sh2_reset(void)
{
#define SET_PAGE(sh2_addr,psp_addr,jit_addr,offset)  do {   \
    if (!(psp_addr)) {                                      \
        DMSG("WARNING: %s == NULL", #psp_addr);             \
    } else {                                                \
        uint32_t page = (sh2_addr) >> 19;                   \
        uint8_t *target = (uint8_t *)(psp_addr) + (offset); \
        direct_pages[0x0000|page] = target;                 \
        direct_pages[0x0400|page] = target;                 \
        direct_pages[0x1400|page] = target;                 \
        target = (uint8_t *)(jit_addr) + (offset >> (JIT_PAGE_BITS+3)); \
        direct_jit_pages[0x0000|page] = target;             \
        direct_jit_pages[0x0400|page] = target;             \
        direct_jit_pages[0x1400|page] = target;             \
    }                                                       \
} while (0)

    SET_PAGE(0x00000000, BiosRom, NULL, 0);
    SET_PAGE(0x00080000, BiosRom, NULL, 0);
    SET_PAGE(0x00200000, LowWram, jit_pages_low, 0);
    SET_PAGE(0x00280000, LowWram, jit_pages_low, 0x80000);
    int i;
    for (i = 0x06000000; i < 0x08000000; i += 0x00100000) {
        SET_PAGE(i, HighWram, jit_pages_high, 0);
        SET_PAGE(i + 0x80000, HighWram, jit_pages_high, 0x80000);
    }

#undef SET_PAGE

    MSH2->psp.current_entry = NULL;
    MSH2->psp.current_PC = MSH2->regs.PC;
    MSH2->psp.cached_page = ~0;
    MSH2->psp.cached_ptr = NULL;
    MSH2->psp.div_data.target_PC = 0;
#ifdef ENABLE_JIT
    memset(MSH2->psp.call_stack, 0,
           sizeof(CallStackEntry) * JIT_CALL_STACK_SIZE);
    MSH2->psp.call_stack_top = 0;
#endif

    SSH2->psp.current_entry = NULL;
    SSH2->psp.current_PC = SSH2->regs.PC;
    SSH2->psp.cached_page = ~0;
    SSH2->psp.cached_ptr = NULL;
    SSH2->psp.div_data.target_PC = 0;
#ifdef ENABLE_JIT
    memset(SSH2->psp.call_stack, 0,
           sizeof(CallStackEntry) * JIT_CALL_STACK_SIZE);
    SSH2->psp.call_stack_top = 0;
#endif

    return 0;
}

/*************************************************************************/

/**
 * psp_sh2_exec:  Execute instructions for the given number of clock cycles.
 *
 * [Parameters]
 *      state: SH-2 processor state
 *     cycles: Number of clock cycles to execute
 * [Return value]
 *     None
 */
static FASTCALL void psp_sh2_exec(SH2_struct *state, u32 cycles)
{
    if (UNLIKELY(state->isSleeping)) {
        state->cycles = cycles;
#ifdef TRACE
        SH2_TRACE_ADD_CYCLES(cycles);
#endif
        return;
    }

    /* If an interrupt occurred, the PC will have changed, so we need to
     * clear any current JIT pointer */
    if (UNLIKELY(state->regs.PC != state->psp.current_PC)) {
        state->psp.current_entry = NULL;
    }

#if defined(TRACE) && defined(JIT_ACCURATE_ACCESS_TIMING)
    /* Track the last instruction we logged, to make sure we don't log an
     * instruction twice */
    int last_logged_cycles = -1;
#endif

    /* Loop until we've consumed the requested number of execution cycles.
     * There's no need to check state->delay here, because it's handled
     * either at translation time or by the if() below the interpret_insn()
     * call. */

    while (state->cycles < cycles) {

        /* If SR was just changed, check whether any interrupts are pending */
#ifndef TRACE_LIKE_SH2INT
        if (UNLIKELY(state->psp.need_interrupt_check)) {
            check_interrupts(state);
        }
#endif

#if defined(TRACE_ALL) && defined(ENABLE_JIT)
        /* Delay slot execution starts here (after the interrupt check) */
      delay_restart:
#endif

#ifdef TRACE
# if defined(TRACE) && defined(JIT_ACCURATE_ACCESS_TIMING)
      if (last_logged_cycles != (int)state->cycles)  // Avoid double logging
        last_logged_cycles = (int)state->cycles,
# endif
        sh2_trace(state, state->regs.PC);
#endif

        /* If we don't have a current native code block, search for one and
         * translate if necessary */
#ifdef ENABLE_JIT
        if (!state->psp.current_entry) {
            state->psp.current_entry = jit_find(state->regs.PC);
            if (UNLIKELY(!state->psp.current_entry)) {
                state->psp.current_entry = jit_translate(state, state->regs.PC);
            }
        }

        /* If we (now) have a native code block, execute from it */
        if (LIKELY(state->psp.current_entry)) {

            jit_exec(state, state->psp.current_entry, cycles);
            if ((intptr_t)state->psp.current_entry->exec_address < 0) {
                /* We just called a subroutine, so push the current block
                 * and address onto the call stack before clearing */
                uintptr_t address = (uintptr_t)state->psp.current_entry->exec_address & 0x7FFFFFFF;
                state->psp.current_entry->exec_address = (void *)address;
                CallStackEntry *call_stack =
                    (CallStackEntry *)state->psp.call_stack;
                int top = state->psp.call_stack_top;
                call_stack[top].return_PC     = state->regs.PR;
                call_stack[top].return_entry  = state->psp.current_entry;
                call_stack[top].return_native = (void *)address;
                top++;
                if (UNLIKELY(top >= JIT_CALL_STACK_SIZE)) {
                    top = 0;
                }
                state->psp.call_stack_top = top;
                state->psp.current_entry = NULL;
            } else if ((intptr_t)state->psp.current_entry->exec_address == 1) {
                /* We just returned from a subroutine, so try to find the
                 * return address on the call stack */
                JitEntry *entry = NULL;
                CallStackEntry *call_stack =
                    (CallStackEntry *)state->psp.call_stack;
                int top = state->psp.call_stack_top;
                int index;
                for (index = JIT_CALL_STACK_SIZE; index > 0; index--) {
                    top--;
                    if (UNLIKELY(top < 0)) {
                        top = JIT_CALL_STACK_SIZE - 1;
                    }
                    if (call_stack[top].return_PC == state->regs.PC) {
                        entry = call_stack[top].return_entry;
                        entry->exec_address = call_stack[top].return_native;
                        state->psp.call_stack_top = top;
                        break;
                    }
                }
                state->psp.current_entry = entry;
            } else if (!state->psp.current_entry->exec_address) {
                /* We hit the end of the block, so find a new one next time */
                state->psp.current_entry = NULL;
            }
# ifdef TRACE_ALL
            /* May have returned before a delay slot, so execute that now */
            if (UNLIKELY(state->delay) && state->psp.current_entry) {
                state->delay = 0;
                goto delay_restart;
            }
# endif

        } else {  // !state->psp.current_entry
#endif  // ENABLE_JIT

            /* We couldn't translate the code at this address, so interpret
             * it instead */
            interpret_insn(state);
            if (UNLIKELY(state->delay)) {
                /* Make sure we interpret the delay slot too, rather than
                 * trying to translate/execute something starting there
                 * (else we'll potentially mess up the state block) */
#ifdef TRACE
                sh2_trace(state, state->regs.PC);
#endif
                interpret_insn(state);
            }

#ifdef ENABLE_JIT
        }  // if (state->psp.current_entry)
#endif

        /* If we just executed a SLEEP instruction, check for interrupts,
         * and if there aren't any then consume all remaining cycles */
        if (UNLIKELY(state->isSleeping)) {
#ifdef TRACE_LIKE_SH2INT  // No interrupt check to match behavior of sh2int.c
            state->regs.PC -= 2;  // Execute the SLEEP instruction again
            state->isSleeping = 0;
#else
            if (!check_interrupts(state)) {
                state->cycles = cycles;
                break;
            }
#endif
        }

    }  // while (state->cycles < cycles)

#ifdef TRACE
    SH2_TRACE_ADD_CYCLES(cycles);
#endif

    /* Save the current PC for the next loop's interrupt check */
    state->psp.current_PC = state->regs.PC;
}

/*-----------------------------------------------------------------------*/

/**
 * check_interrupts:  Check whether there are any pending interrupts, and
 * service the highest-priority one if so.  Helper routine for psp_sh2_exec().
 *
 * [Parameters]
 *     state: Processor state block
 * [Return value]
 *     Nonzero if an interrupt was services, else zero
 */
static int check_interrupts(SH2_struct *state)
{
    if (state->NumberOfInterrupts > 0) {
        const int level = state->interrupts[state->NumberOfInterrupts-1].level;
        if (level > state->regs.SR.part.I) {
            const int vector =
                state->interrupts[state->NumberOfInterrupts-1].vector;
            state->regs.R[15] -= 4;
            MappedMemoryWriteLong(state->regs.R[15], state->regs.SR.all);
            state->regs.R[15] -= 4;
            MappedMemoryWriteLong(state->regs.R[15], state->regs.PC);
            state->regs.SR.part.I = level;
            state->regs.PC =
                MappedMemoryReadLong(state->regs.VBR + (vector << 2));
            state->psp.current_entry = NULL;
            state->NumberOfInterrupts--;
            return 1;
        }
    }
    return 0;
}

/*************************************************************************/

/**
 * psp_sh2_write_notify:  Called when an external agent modifies memory.
 * Used here to clear any JIT translations in the modified range.
 *
 * [Parameters]
 *     address: Beginning of address range to which data was written
 *        size: Size of address range to which data was written (in bytes)
 * [Return value]
 *     None
 */
static void psp_sh2_write_notify(u32 address, u32 size)
{
#ifdef ENABLE_JIT
    jit_clear_range(address, size);
#endif
}

/*************************************************************************/
/**************** Dynamic translation management routines ****************/
/*************************************************************************/

#ifdef ENABLE_JIT

/*************************************************************************/

/**
 * jit_find:  Find the translated block for a given address, if any.
 *
 * [Parameters]
 *     address: Start address in SH-2 address space
 * [Return value]
 *     Translated block, or NULL if no such block exists
 */
JitEntry *jit_find(uint32_t address)
{
    const int hashval = JIT_HASH(address);
    JitEntry *entry = jit_hashchain[hashval];
    while (entry) {
        if (entry->sh2_start == address) {
            entry->exec_address = entry->native_code;
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/*************************************************************************/

/**
 * jit_translate:  Dynamically translate a block of instructions starting
 * at the given address.  If a translation already exists for the given
 * address, it is cleared.
 *
 * [Parameters]
 *     address: Start address in SH-2 address space
 * [Return value]
 *     Translated block, or NULL on error
 */
JitEntry *jit_translate(SH2_struct *state, uint32_t address)
{
    JitEntry *entry;
    int index;

    /* First check for untranslatable addresses */
    if (address == 0) {
        /* We use address 0 to indicate an unused entry, so we can't
         * translate from address 0.  But this should never happen except
         * in pathological cases (or unhandled exceptions), so just punt
         * and let the interpreter handle it. */
        return NULL;
    }
    if (address & 1) {
        /* Odd addresses are invalid, so we can't translate them in the
         * first place. */
         return NULL;
    }
    if (!direct_pages[state->regs.PC >> 19]) {
        /* Don't try to translate anything from non-directly-accessible
         * memory, since we can't track changes to such memory. */
        return NULL;
    }

    /* Check whether the starting address is blacklisted */
    for (index = 0; index < lenof(blacklist); index++) {
#ifndef TRACE_ALL  // See documentation for JIT_BLACKLIST_EXPIRE
        uint32_t age = jit_timestamp - blacklist[index].timestamp;
        if (age >= JIT_BLACKLIST_EXPIRE) {
            /* Entry expired, so clear it */
            blacklist[index].start = blacklist[index].end = 0;
            continue;
        }
#endif
        if (blacklist[index].start <= address
                                   && address <= blacklist[index].end) {
            return NULL;
        }
    }

    /* Clear out any existing translation, then search for an empty slot in
     * the hash table.  If we've reached the data size limit, first evict
     * old entries until we're back under the limit. */
    if ((entry = jit_find(address)) != NULL) {
        clear_entry(entry);
    }
    if (UNLIKELY(jit_total_data >= JIT_DATA_LIMIT)) {
        DMSG("JIT data size over limit (%u >= %u), clearing entries",
             jit_total_data, JIT_DATA_LIMIT);
        while (jit_total_data >= JIT_DATA_LIMIT) {
            clear_oldest_entry();
        }
    }
    const int hashval = JIT_HASH(address);
    index = hashval;
    int oldest = index;
    while (jit_table[index].sh2_start != 0) {
        if (timestamp_compare(jit_timestamp,
                              jit_table[index].timestamp,
                              jit_table[oldest].timestamp) < 0) {
            oldest = index;
        }
        index++;
        /* Using an if here is faster than taking the remainder with % */
        if (UNLIKELY(index >= JIT_TABLE_SIZE)) {
            index = 0;
        }
        if (UNLIKELY(index == hashval)) {
            /* Out of entries, so clear the oldest one and use it */
            DMSG("No free slots for code at 0x%08X, clearing oldest (0x%08X,"
                " age %u)", address, jit_table[oldest].sh2_start,
                jit_timestamp - jit_table[oldest].timestamp);
            clear_entry(&jit_table[oldest]);
            index = oldest;
        }
    }
    entry = &jit_table[index];

    /* Initialize the new entry */
    entry->native_code = malloc(JIT_BLOCK_EXPAND_SIZE);
    if (!entry->native_code) {
        DMSG("No memory for code at 0x%08X", address);
        entry = NULL;
        return NULL;
    }
    entry->next = jit_hashchain[hashval];
    if (jit_hashchain[hashval]) {
        jit_hashchain[hashval]->prev = entry;
    }
    jit_hashchain[hashval] = entry;
    entry->prev = NULL;
    entry->sh2_start = address;
    entry->native_size = JIT_BLOCK_EXPAND_SIZE;
    entry->native_length = 0;
    entry->exec_address = NULL;
    entry->timestamp = jit_timestamp;
    entry->must_clear = 0;

    /* Insert the common prologue */
    if (!append_prologue(entry)) {
        clear_entry(entry);
        return NULL;
    }

    /* Clear out the branch target cache and unresolved branch list */
    for (index = 0; index < lenof(btcache); index++) {
        btcache[index].sh2_address = 0;
    }
    btcache_index = 0;
    for (index = 0; index < lenof(unres_branches); index++) {
        unres_branches[index].sh2_target = 0;
    }

    /* Scan through the SH-2 code to find the end of the block (as well as
     * all addresses targeted by branches) and determine what parts of it
     * are translatable code */
    int block_len = jit_scan_block(entry, address);
    if (!block_len) {
        clear_entry(entry);
        return NULL;
    }

    /* Translate a block of SH-2 code */
#ifdef JIT_DEBUG_VERBOSE
    DMSG("Starting translation at %08X", address);
#endif
    jit_PC = address;
    jit_cycles = 0;
    active_regs = 0;
    changed_regs = 0;
    int insn_count;
    for (insn_count = 0; insn_count < block_len; insn_count++) {
        if (is_data[insn_count] == 0) {
            if (UNLIKELY(!translate_insn(state, entry))) {
                /* Translation failed, so clear the entry */
                clear_entry(entry);
                return NULL;
            }
        } else {
            /* It's not code, so just skip the word */
            jit_PC += 2;
        }
    }
#ifdef JIT_DEBUG_VERBOSE
    DMSG("Translation ended at %08X", jit_PC-2);
#endif

    /* Nullify cycle check count (or unconditional interrupt in trace mode)
     * before unresolved static branches */
    for (index = 0; index < lenof(unres_branches); index++) {
        if (unres_branches[index].sh2_target != 0
         && unres_branches[index].check_offset >= 0
        ) {
#ifdef JIT_DEBUG_VERBOSE
            DMSG("FAILED to resolve branch to %08X at %p",
                 unres_branches[index].sh2_target,
                 (uint8_t *)entry->native_code
                     + unres_branches[index].native_offset);
#endif
            uint32_t *address = (uint32_t *)((uint8_t *)entry->native_code
                                    + unres_branches[index].check_offset);
            uint32_t disp = unres_branches[index].native_offset
                            - (unres_branches[index].check_offset + 4);
            address[0] = MIPS_B(disp/4);
            address[1] = MIPS_NOP();
        }
    }

    /* Append the common epilogue */
    if (!append_epilogue(entry)) {
        clear_entry(entry);
        return NULL;
    }

    /* Close out the translated block */
    entry->sh2_end = jit_PC - 1;
    uint8_t *jit_base = direct_jit_pages[entry->sh2_start >> 19];
    if (jit_base) {
        uint32_t page_base = entry->sh2_start & 0xFFF80000;
        for (index =  (entry->sh2_start - page_base) >> JIT_PAGE_BITS;
             index <= (entry->sh2_end   - page_base) >> JIT_PAGE_BITS;
             index++
        ) {
            JIT_PAGE_SET(jit_base, index);
        }
    }
    flush_cache();
    void *newptr = realloc(entry->native_code,
                           entry->native_length);
    if (newptr) {
        entry->native_code = newptr;
        entry->native_size = entry->native_length;
    }
    jit_total_data += entry->native_size;
    /* Prepare the block for execution so it can be immediately started */
    entry->exec_address = entry->native_code;

    return entry;
}

/*-----------------------------------------------------------------------*/

/**
 * jit_scan_block:  Scan through a block of SH-2 code, finding the length
 * of the block, recording whether each word in the block is code or data,
 * and recording all addresses targeted by branches in the btlist[] array.
 * Helper routine for jit_translate().
 *
 * [Parameters]
 *       entry: Translated block data structure
 *     address: Start of block in SH-2 address space
 * [Return value]
 *     Length of block, in instructions
 * [Side effects]
 *     Fills in is_data[] and btlist[]; updates btlist_len to the number of
 *     branch targets
 */
static int jit_scan_block(JitEntry *entry, uint32_t address)
{
#ifdef PSP_DEBUG  // Only used in a DMSG at the moment
    const uint32_t start_address = address;  // Save the block's start address
#endif

    const uint16_t *fetch_base = (const uint16_t *)direct_pages[address >> 19];
    const uint16_t *fetch;
    if (fetch_base) {
        fetch = (const uint16_t *)((uintptr_t)fetch_base + (address & 0x7FFFF));
    } else {
        fetch = NULL;
    }

    btlist_len = 0;
    memset(is_data, -1, sizeof(is_data));

    int block_len = 0;
    int end_block = 0;
    int delay = 0;
    int uncond_branch = 0;
    for (; !end_block || delay; address += 2, block_len++) {

        /* Make sure we haven't run into a blacklisted region */
        int index;
        for (index = 0; index < lenof(blacklist); index++) {
            if (UNLIKELY(blacklist[index].start <= address
                                     && address <= blacklist[index].end)) {
                if (delay) {
                    /* Ack!  we hit a blacklisted region while trying to
                     * translate a delay slot.  We can't leave the delay
                     * slot to the interpreter (since state->delay isn't
                     * updated in translated code), so complain and force
                     * the translation to be flushed when it exits.  Also
                     * extend the blacklist to cover the preceding
                     * instruction so we don't have to do this again. */
                    DMSG("Delay slot in blacklist at 0x%08X, performance"
                         " will suffer", address);
                    entry->must_clear = 1;
                    blacklist[index].start -= 2;
                    blacklist[index].timestamp = jit_timestamp;
                    end_block = 1;  // Abort translation after this instruction
                } else {
                    goto abort_scan;
                }
            }
        }

        /* Save whether or not this was a delay slot, then clear "delay" */
        const int now_in_delay = delay;
        delay = 0;

        /* If this is a known data word, skip over it */
        if (is_data[block_len] > 0) {
            goto skip_word;
        }

        /* Get the opcode */
        uint16_t opcode;
        if (LIKELY(fetch)) {
            opcode = *fetch++;
        } else {
            opcode = MappedMemoryReadWord(address);
        }

        /* Check whether the opcode is valid, and abort if not.  Don't
         * include the invalid instruction in the block; if we get there
         * and it's still invalid, we'll interpret it (but who knows, maybe
         * it'll be modified in the meantime). */
        if (((opcode & 0x0030) >= 0x0030  // $xx[0-2]x-type opcodes
             && ((opcode & 0xF004) == 0x0000
              || (opcode & 0xF008) == 0x4000
              || (opcode & 0xF00C) == 0x4008
              || (opcode & 0xF00F) == 0x400E))
         || (opcode & 0xF00E) == 0x0000
         || (opcode & 0xF0FF) == 0x0013
         || (opcode & 0xF00F) == 0x2003
         || (opcode & 0xF007) == 0x3001
         || (opcode & 0xF0FF) == 0x4014
         || (opcode & 0xF00E) == 0x400C
         || (opcode & 0xFA00) == 0x8200
         || (opcode & 0xFF00) == 0x8A00
         || (opcode & 0xFD00) == 0x8C00
         || (opcode & 0xF000) == 0xF000
        ) {
            goto abort_scan;
        }

        /* Mark this word as code */
        is_data[block_len] = 0;

        /* If the instruction is a static branch, record the target */
        if ((opcode & 0xF900) == 0x8900  // B[TF]{,_S}
         || (opcode & 0xF000) == 0xA000  // BRA
        ) {
            if (btlist_len >= lenof(btlist)) {
                DMSG("Branch target list overflow at 0x%08X", address);
                goto abort_scan;  // Terminate the block before this insn
            } else {
                int disp;
                if ((opcode & 0xF000) == 0xA000) {
                    disp = opcode & 0xFFF;
                    disp <<= 20;  // Sign-extend
                    disp >>= 19;  // And double
                } else {
                    disp = opcode & 0xFF;
                    disp <<= 24;
                    disp >>= 23;
                }
                uint32_t target = (address + 4) + disp;
                btlist[btlist_len++] = target;
            }
        }

        /* Check whether it's an instruction with a delay slot */
        delay = ((opcode & 0xF0D7) == 0x0003    // BSRF/BRAF/RTS/RTE
              || (opcode & 0xF0DF) == 0x400B    // JSR/JMP
              || (opcode & 0xFD00) == 0x8D00    // B[TF]_S
              || (opcode & 0xE000) == 0xA000);  // BRA/BSR

        /* Check whether this is an unconditional branch or similar
         * instruction; when we get past its delay slot (if applicable),
         * we'll check whether to end the block. */
        uncond_branch |= ((opcode & 0xFF00) == 0xC300    // TRAPA
                       || (opcode & 0xF0FF) == 0x000B    // RTS
                       || (opcode & 0xF0FF) == 0x001B    // SLEEP
                       || (opcode & 0xF0FF) == 0x002B    // RTE
                       || (opcode & 0xF0FF) == 0x402B    // JMP
                       || (opcode & 0xF0FF) == 0x0023    // BRAF
                       || (opcode & 0xF000) == 0xA000);  // BRA

        /* If we have a pending unconditional branch and we're past its
         * delay slot (if applicable), check whether there are any branch
         * targets beyond this address which could be part of the same
         * block (i.e. are within the instruction limit).  If there are,
         * we'll continue translating at the first such address; otherwise
         * we'll end the block here. */
        if (uncond_branch && !delay) {
            uncond_branch = 0;
            /* See below for why the "-1" */
            const uint32_t address_limit =
#ifdef JIT_ALLOW_FORWARD_BRANCHES
                start_address + ((JIT_INSNS_PER_BLOCK-1) * 2);
#else
                address + 4;
#endif
            const uint32_t first_valid = (now_in_delay ? address : address+2);
            uint32_t first_target = address_limit;
            for (index = 0; index < btlist_len; index++) {
                if (btlist[index] >= first_valid
                 && btlist[index] < first_target
                ) {
                    first_target = btlist[index];
                }
            }
            if (first_target < address_limit) {
                int skip_bytes = first_target - (address+2);
                if (skip_bytes > 0) {
                    block_len += skip_bytes / 2;
                    address += skip_bytes;
                    if (fetch) {
                        fetch += skip_bytes / 2;
                    }
                }
            } else {
                end_block = 1;
            }
        }

      skip_word:
        /* Check whether we've hit the instruction limit.  To avoid the
         * potential for problems arising from an instruction with a delay
         * slot crossing the limit, we stop one instruction early; if this
         * instruction had a delay slot, the delay slot will still fall
         * within the limit. */
        if (block_len+2 >= JIT_INSNS_PER_BLOCK) {
            DMSG("WARNING: Terminating block 0x%08X after 0x%08X due to"
                 " instruction limit (%d insns)",
                 start_address, address + 2, JIT_INSNS_PER_BLOCK);
            end_block = 1;
        }

    }  // for (; !end_block || delay; address += 2, block_len++)

  abort_scan:
    return block_len;
}

/*************************************************************************/

/**
 * jit_exec:  Execute translated code from the given block.
 *
 * [Parameters]
 *      state: Processor state block
 *      entry: Translated code block
 *     cycles: Number of cycles to execute
 * [Return value]
 *     None
 */
static inline void jit_exec(SH2_struct *state, JitEntry *entry, int cycles)
{
    jit_timestamp++;
    entry->running = 1;

    {
        /* Can't place __state in $s8 to begin with because a GCC bug(?)
         * seems to cause it to try and use the same register for "entry" */
        register SH2_struct *__state asm("a0") = state;
        register void *    __address asm("v0") = entry->exec_address;
        register int        __cycles asm("v1") = cycles;
        asm(".set push; .set noreorder\n"
            "jalr $v0\n"
            "move $30, %3\n"
            ".set pop"
            : "=r" (__state), "=r" (__address), "=r" (__cycles)
            : "0" (__state), "1" (__address), "2" (__cycles)
            : "at", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5",
              "t6", "t7", "t8", "t9", "s0", "s1", "s2", "s3", "s4", "s5", "s6",
              "s7", /*s8*/"$30", "ra", "memory"
        );
        entry->exec_address = __address;
    }

    entry->running = 0;
}

/*************************************************************************/

/**
 * jit_clear_write:  Clear any translation which includes the given
 * address.  Intended to be called from within translated code.
 *
 * [Parameters]
 *       state: Processor state block
 *     address: Address to which data was written
 * [Return value]
 *     None
 */
static void jit_clear_write(SH2_struct *state, uint32_t address)
{
    int index;

    /* If it's a blacklisted address, we don't need to do anything (since
     * the address couldn't have been translated); just update the entry's
     * write timestamp and return */
    for (index = 0; index < lenof(blacklist); index++) {
        if (blacklist[index].start <= address
                                   && address <= blacklist[index].end) {
            blacklist[index].timestamp = jit_timestamp;
            return;
        }
    }

    DMSG("WARNING: jit_clear_write(0x%08X)", address);

    /* Clear any translations on the affected page */
    uint8_t * const jit_base  = direct_jit_pages[address >> 19];
    const uint32_t page       = (address & 0x7FFFF) >> JIT_PAGE_BITS;
    const uint32_t page_start = (address & 0xFFF80000) | page << JIT_PAGE_BITS;
    const uint32_t page_end   = page_start + ((1 << JIT_PAGE_BITS) - 1);
    JIT_PAGE_CLEAR(jit_base, page);
    int do_blacklist = 0;  // Flag: Blacklist this address?
    for (index = 0; index < JIT_TABLE_SIZE; index++) {
        if (jit_table[index].sh2_start == 0) {
            continue;
        }
        if (jit_table[index].sh2_start <= page_end
         && jit_table[index].sh2_end >= page_start
        ) {
            if (UNLIKELY(jit_table[index].running)) {
                jit_table[index].must_clear = 1;
                do_blacklist = 1;
            } else {
                clear_entry(&jit_table[index]);
            }
        }
    }

    /* Blacklist this address if appropriate.  We always keep the blacklist
     * entries word-aligned; since we don't pass the write size in (due to
     * a dearth of assembly registers), we assume the write is longword-
     * sized if on a longword-aligned address, and word-sized otherwise. */
    if (do_blacklist) {
        const uint32_t start = address & ~1;
        const uint32_t end = (address & ~3) + 3;
        /* Merge this to the beginning or end of another entry if possible */
        for (index = 0; index < lenof(blacklist); index++) {
            if (blacklist[index].start == end+1) {
                blacklist[index].start = start;
                /* See if there's another one we can join to this one */
                int index2;
                for (index2 = 0; index2 < lenof(blacklist); index2++) {
                    if (start == blacklist[index2].end+1) {
                        blacklist[index].start = blacklist[index2].start;
                        blacklist[index2].start = blacklist[index2].end = 0;
                        break;
                    }
                }
                goto entry_added;
            } else if (blacklist[index].end+1 == start) {
                blacklist[index].end = end;
                int index2;
                for (index2 = 0; index2 < lenof(blacklist); index2++) {
                    if (end+1 == blacklist[index2].start) {
                        blacklist[index].end = blacklist[index2].end;
                        blacklist[index2].start = blacklist[index2].end = 0;
                        break;
                    }
                }
                goto entry_added;
            }
        }
        /* We didn't find an entry to merge this into, so allocate a new one */
        for (index = 0; index < lenof(blacklist); index++) {
            if (!blacklist[index].start && !blacklist[index].end) {
                break;
            }
        }
        /* If the table is full, purge the oldest entry */
        if (index >= lenof(blacklist)) {
            int oldest = 0;
            for (index = 1; index < lenof(blacklist); index++) {
                if (timestamp_compare(jit_timestamp,
                                      blacklist[index].timestamp,
                                      blacklist[oldest].timestamp) < 0) {
                    oldest = index;
                }
            }
            DMSG("Blacklist full, purging entry %d (0x%X-0x%X)",
                 oldest, blacklist[oldest].start, blacklist[oldest].end);
            index = oldest;
        }
        /* Fill in the new entry */
        blacklist[index].start = start;
        blacklist[index].end = end;
      entry_added:
        blacklist[index].timestamp = jit_timestamp;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * jit_clear_range:  Clear any translation on any page touched by the given
 * range of addresses.  Intended to be called when an external agent
 * modifies memory.
 *
 * [Parameters]
 *       state: Processor state block
 *     address: Beginning of address range to which data was written
 *        size: Size of address range to which data was written (in bytes)
 * [Return value]
 *     None
 */
static void jit_clear_range(uint32_t address, uint32_t size)
{
    const uint32_t first_page = address >> JIT_PAGE_BITS;
    const uint32_t last_page  = (address + size - 1) >> JIT_PAGE_BITS;
    const uint32_t page_start = first_page << JIT_PAGE_BITS;
    const uint32_t page_end   = ((last_page+1) << JIT_PAGE_BITS) - 1;

    int page;
    for (page = first_page; page <= last_page; page++) {
        if (direct_jit_pages[page >> (19-JIT_PAGE_BITS)]) {
            JIT_PAGE_CLEAR(direct_jit_pages[page >> (19-JIT_PAGE_BITS)], page);
        }
    }

    int index;
    for (index = 0; index < JIT_TABLE_SIZE; index++) {
        if (jit_table[index].sh2_start == 0) {
            continue;
        }
        if (jit_table[index].sh2_start <= page_end
         && jit_table[index].sh2_end >= page_start
        ) {
            clear_entry(&jit_table[index]);
        }
    }
}

/*************************************************************************/

#endif  // ENABLE_JIT

/*************************************************************************/
/*********** SH-2 interpreted execution (mostly for debugging) ***********/
/*************************************************************************/

/* Access the state block directly for the PC and cycle count */
#define cur_PC      (state->regs.PC)
#define cur_cycles  (state->cycles)

/* No pre-decode processing needed */
#define OPCODE_INIT(opcode)  /*nothing*/

/* cur_PC and REG_PC are the same thing, so only need to update one of them */
#define INC_PC()  (cur_PC += 2)

/* Cycle count is already stored in the state block, so nothing to commit */
#define COMMIT_CYCLES(scratch)  /*nothing*/

/* No need to check the cycle limit, since we only execute one instruction */
#define CHECK_CYCLES(scratch1,scratch2)  /*nothing*/

/* Idle checking not implemented for interpreted execution */
#define CONSUME_CYCLES(scratch)  /*nothing*/

/* No need to flush registers */
#define FLUSH_REGS(mask)  /*nothing*/

/* Register definitions (as used in translated code) */
#define REG_ZERO  0
#define REG_R(n)  state->regs.R[(n)]
#define REG_PC    cur_PC
#define REG_SR    state->regs.SR.all
#define REG_GBR   state->regs.GBR
#define REG_VAL0  value0   // Scratch register
#define REG_VAL1  value1   // Scratch register
#define REG_VAL2  value2   // Scratch register

/* Register-register operations */
#define NOP()                {/*nothing*/}
#define MOVE(dest,src)       ((dest) = (src))
#define MOVN(dest,src,test)  {if ((test)) (dest) = (src);}
#define MOVZ(dest,src,test)  {if (!(test)) (dest) = (src);}
#define ADD(dest,src1,src2)  ((dest) = (src1) + (src2))
#define SUB(dest,src1,src2)  ((dest) = (src1) - (src2))
#define AND(dest,src1,src2)  ((dest) = (src1) & (src2))
#define OR(dest,src1,src2)   ((dest) = (src1) | (src2))
#define XOR(dest,src1,src2)  ((dest) = (src1) ^ (src2))
#define NEG(dest,src)        ((dest) = -(src))
#define NOT(dest,src)        ((dest) = ~(src))
#define SLT(dest,src1,src2)  ((dest) = ((int32_t)(src1) < (int32_t)(src2)))
#define SLTU(dest,src1,src2) ((dest) = ((src1) < (src2)))
#define EXTS_B(dest,src)     ((dest) = (int8_t)(src))
#define EXTS_W(dest,src)     ((dest) = (int16_t)(src))
#define EXTZ_B(dest,src)     ((dest) = (uint8_t)(src))
#define EXTZ_W(dest,src)     ((dest) = (uint16_t)(src))
#ifdef PSP
#define EXTBITS(dest,src,first,count) \
    asm("ext %0, %1, %2, %3" \
        : "=r" (dest) \
        : "r" (src), "i" (first), "i" (count) \
    );
#define INSBITS(dest,src,first,count) \
    asm("ins %0, %2, %3, %4" \
        : "=r" (dest) \
        : "0" (dest), "r" (src), "i" (first), "i" (count) \
    );
#else
#define EXTBITS(dest,src,first,count)  do { \
    const uint32_t __mask = ((uint32_t)1 << (count)) - 1; \
    (dest) = ((src) >> (first)) & __mask; \
} while (0)
#define INSBITS(dest,src,first,count)  do { \
    const uint32_t __mask = ((uint32_t)1 << (count)) - 1; \
    const uint32_t __first = (first); \
    (dest) &= ~(__mask << __first); \
    (dest) |= ((src) & __mask) << __first; \
} while (0)
#endif

/* Multiplication and division */
#define MUL_START(src1,src2) { uint32_t __product = (src1) * (src2)
#define MUL_END(dest)          (dest) = __product; }
#ifdef PSP
#define DMULS_START(src1,src2) \
    asm("mult %0, %1" : : "r" (src1), "r" (src2) : "lo", "hi")
#define DMULU_START(src1,src2) \
    asm("multu %0, %1" : : "r" (src1), "r" (src2) : "lo", "hi")
#define DMUL_END(destlo,desthi) \
    asm("" : "=l" (destlo), "=h" (desthi))
#define DIVS_START(dividend,divisor) \
    asm("div $zero, %0, %1" : : "r" (dividend), "r" (divisor) : "lo", "hi")
#define DIVU_START(dividend,divisor) \
    asm("divu $zero, %0, %1" : : "r" (dividend), "r" (divisor) : "lo", "hi")
#define DIV_END(quotient,remainder) \
    asm("" : "=l" (quotient), "=h" (remainder))
#else
#define DMULS_START(src1,src2) \
    { uint64_t __product = (int64_t)(int32_t)(src1) * (int64_t)(int32_t)(src2)
#define DMULU_START(src1,src2) \
    { uint64_t __product = (uint64_t)(src1) * (uint64_t)(src2)
#define DMUL_END(destlo,desthi) \
    (destlo) = __product; (desthi) = __product>>32; }
#define DIVS_START(dividend,divisor) \
    {uint32_t __saved_quotient  = (int32_t)(dividend) / (int32_t)(divisor), \
              __saved_remainder = (int32_t)(dividend) % (int32_t)(divisor)
#define DIVU_START(dividend,divisor) \
    {uint32_t __saved_quotient  = (uint32_t)(dividend) / (uint32_t)(divisor), \
              __saved_remainder = (uint32_t)(dividend) % (uint32_t)(divisor)
#define DIV_END(quotient,remainder) \
    (quotient)  = __saved_quotient; \
    (remainder) = __saved_remainder; }
#endif

/* Immediate operations */
#define MOVEI(dest,imm)      ((dest) = (imm))
#define ADDI(dest,src,imm)   ((dest) = (src) + (imm))
#define ANDI(dest,src,imm)   ((dest) = (src) & (imm))
#define ORI(dest,src,imm)    ((dest) = (src) | (imm))
#define XORI(dest,src,imm)   ((dest) = (src) ^ (imm))
#define SLLI(dest,src,imm)   ((dest) = (src) << (imm))
#define SRLI(dest,src,imm)   ((dest) = (src) >> (imm))
#define SRAI(dest,src,imm)   ((dest) = (int32_t)(src) >> (imm))
#define SLTI(dest,src,imm)   ((dest) = ((int32_t)(src) < (int32_t)(imm)))
#define SLTIU(dest,src,imm)  ((dest) = ((src) < (uint32_t)(imm)))

/* Variants on SLTI/SLTIU */
#define SEQZ(dest,src)  SLTIU((dest), (src), 1)
#define SNEZ(dest,src)  SLTU((dest), REG_ZERO, (src))
#define SGTZ(dest,src)  SLT((dest), REG_ZERO, (src))
#define SLEZ(dest,src)  SRLI((dest), (src), 31)
#define SLTZ(dest,src)  SLT((dest), (src), REG_ZERO)

/* Register-stateblock operations */
#define STATE_LOAD(dest,field)  ((dest) = state->field)
#define STATE_STORE(src,field)  (state->field = (src))
#define STATE_STOREB(src,field) (state->field = (src))

/* Load/store operations */

#define DO_LOAD(dest,address,direct,fallback)  do {             \
    const uint32_t __address = (address);                       \
    const uint32_t __page = __address >> 19;                    \
    if (LIKELY(__page == state->psp.cached_page)) {             \
        (dest) = (int32_t)(direct);                             \
    } else {                                                    \
        uint8_t *__ptr = direct_pages[__page];                  \
        if (__ptr) {                                            \
            state->psp.cached_page = __page;                    \
            state->psp.cached_ptr = __ptr;                      \
            (dest) = (int32_t)(direct);                         \
        } else {                                                \
            (dest) = (int32_t)(fallback(__address));            \
        }                                                       \
    }                                                           \
} while (0)
#define LOAD_BS(dest,address)  DO_LOAD((dest), (address), \
     *(int8_t *)(state->psp.cached_ptr + ((__address^1) & 0x7FFFF)), \
     (int8_t)MappedMemoryReadByte)
#define LOAD_BU(dest,address)  DO_LOAD((dest), (address), \
     *(uint8_t *)(state->psp.cached_ptr + ((__address^1) & 0x7FFFF)), \
     (uint8_t)MappedMemoryReadByte)
#define LOAD_WS(dest,address)  DO_LOAD((dest), (address), \
     *(int16_t *)(state->psp.cached_ptr + (__address & 0x7FFFF)), \
     (int16_t)MappedMemoryReadWord)
#define LOAD_WU(dest,address)  DO_LOAD((dest), (address), \
     *(uint16_t *)(state->psp.cached_ptr + (__address & 0x7FFFF)), \
     (uint16_t)MappedMemoryReadWord)
#define LOAD_L(dest,address)  DO_LOAD((dest), (address), \
     *(uint16_t *)(state->psp.cached_ptr + (__address & 0x7FFFF)) << 16 \
     | *(uint16_t *)(state->psp.cached_ptr + (__address & 0x7FFFF) + 2), \
     (int32_t)MappedMemoryReadLong)

#ifdef TRACE
# define LOG_STORE(addr,val,digits)  do { \
    uint32_t __a = (addr), __v = (val); \
    __v &= (uint32_t)(((uint64_t)1 << (digits*4)) - 1); \
    if (msh2_log) \
        fprintf(msh2_log, "WRITE%c %08X <- %0" #digits "X\n", \
                digits==2 ? 'B' : digits==4 ? 'W' : 'L', (int)__a, (int)__v); \
} while (0)
#else
# define LOG_STORE(addr,val,digits)  /*nothing*/
#endif

#define DO_STORE(src,address,digits,direct,fallback)  do {      \
    __label__ do_fallback;  /* GCC local label declaration */   \
    const uint32_t __address = (address);                       \
    const uint32_t __page = __address >> 19;                    \
    if (UNLIKELY((__page & 0x3FF) < 2)) {                       \
        goto do_fallback;  /* Prevent writes to Saturn ROM */   \
    }                                                           \
    if (LIKELY(__page == state->psp.cached_page)) {             \
        LOG_STORE(__address, (src), digits);                    \
        direct;                                                 \
    } else {                                                    \
        uint8_t *__ptr = direct_pages[__page];                  \
        if (__ptr) {                                            \
            state->psp.cached_page = __page;                    \
            state->psp.cached_ptr = __ptr;                      \
            LOG_STORE(__address, (src), digits);                \
            direct;                                             \
        } else {                                                \
          do_fallback:                                          \
            fallback(__address, (src));                         \
        }                                                       \
    }                                                           \
} while (0)
#define STORE_B(src,address)  DO_STORE((src), (address), 2, \
    *(uint8_t *)(state->psp.cached_ptr + ((__address^1) & 0x7FFFF)) \
        = (src), \
    MappedMemoryWriteByte)
#define STORE_W(src,address)  DO_STORE((src), (address), 4, \
    *(uint16_t *)(state->psp.cached_ptr + (__address & 0x7FFFF)) \
        = (src), \
    MappedMemoryWriteWord)
#define STORE_L(src,address)  DO_STORE((src), (address), 8, \
    (*(uint16_t *)(state->psp.cached_ptr + (__address & 0x7FFFF)) \
         = (src)>>16, \
     *(uint16_t *)(state->psp.cached_ptr + (__address & 0x7FFFF) + 2) \
         = (src)), \
    MappedMemoryWriteLong)

#if 0  // Non-caching versions, for reference/testing
# undef LOAD_BS
# undef LOAD_BU
# undef LOAD_WS
# undef LOAD_WU
# undef LOAD_L
# undef STORE_B
# undef STORE_W
# undef STORE_L
# define LOAD_BS(dest,address) \
    ((dest) = (int32_t)(int8_t)MappedMemoryReadByte((address)))
# define LOAD_BU(dest,address) ((dest) = MappedMemoryReadByte((address)))
# define LOAD_WS(dest,address) \
    ((dest) = (int32_t)(int16_t)MappedMemoryReadWord((address)))
# define LOAD_WU(dest,address) ((dest) = MappedMemoryReadWord((address)))
# define LOAD_L(dest,address)  ((dest) = MappedMemoryReadLong((address)))
# define STORE_B(src,address)  (MappedMemoryWriteByte((address), (src)))
# define STORE_W(src,address)  (MappedMemoryWriteWord((address), (src)))
# define STORE_L(src,address)  (MappedMemoryWriteLong((address), (src)))
#endif  // 0

/* Conditional blocks and branches */
#define IFZ(label,reg,delay)    {uint32_t temp = (reg); \
                                 delay; if (temp) goto label_##label;}
#define IFNZ(label,reg,delay)   {uint32_t temp = (reg); \
                                 delay; if (!temp) goto label_##label;}
#define IFGEZ(label,reg,delay)  {int32_t temp = (reg); \
                                 delay; if (temp < 0) goto label_##label;}
#define ELSE(lab1,lab2,delay)   delay; goto label_##lab2; label_##lab1:
#define ENDIF(label)            label_##label:
#define GOTO(label,delay)       delay; goto label_##label
#define BRANCH_DYNAMIC(reg)     cur_PC = (reg)
#define BRANCH_STATIC(reg)      cur_PC = (reg)
#define BRANCH_CALL(reg)        cur_PC = (reg)
#define BRANCH_RETURN(reg)      cur_PC = (reg)

/*************************************************************************/

/**
 * interpret_insn:  Interpret and execute a single SH-2 instruction at the
 * current PC.
 *
 * [Parameters]
 *     state: SH-2 processor state
 * [Return value]
 *     None
 */
static void interpret_insn(SH2_struct *state)
{
    /* Initial PC (here just the current instruction's PC) */
    __attribute__((unused)) const uint32_t initial_PC = state->regs.PC;

    /* Scratch variables */
    uint32_t value0, value1, value2;

#include "sh2-core.i"

}

/*************************************************************************/

#undef cur_PC
#undef cur_cycles

#undef OPCODE_INIT
#undef INC_PC
#undef COMMIT_CYCLES
#undef CHECK_CYCLES
#undef CONSUME_CYCLES
#undef FLUSH_REGS

#undef REG_ZERO
#undef REG_R
#undef REG_PC
#undef REG_SR
#undef REG_GBR
#undef REG_VAL0
#undef REG_VAL1
#undef REG_VAL2

#undef NOP
#undef MOVE
#undef MOVN
#undef MOVZ
#undef ADD
#undef SUB
#undef AND
#undef OR
#undef XOR
#undef NEG
#undef NOT
#undef SLT
#undef SLTU
#undef EXTS_B
#undef EXTS_W
#undef EXTZ_B
#undef EXTZ_W
#undef EXTBITS
#undef INSBITS

#undef MUL_START
#undef MUL_END
#undef DMULS_START
#undef DMULU_START
#undef DMUL_END
#undef DIVS_START
#undef DIVU_START
#undef DIV_END

#undef MOVEI
#undef ADDI
#undef ANDI
#undef ORI
#undef XORI
#undef SLLI
#undef SRLI
#undef SRAI
#undef SLTI
#undef SLTIU

#undef SEQZ
#undef SNEZ
#undef SGTZ
#undef SLEZ
#undef SLTZ

#undef STATE_LOAD
#undef STATE_STORE
#undef STATE_STOREB

#undef DO_LOAD
#undef LOAD_BS
#undef LOAD_BU
#undef LOAD_WS
#undef LOAD_WU
#undef LOAD_L
#undef LOG_STORE
#undef DO_STORE
#undef STORE_B
#undef STORE_W
#undef STORE_L

#undef IFZ
#undef IFNZ
#undef IFGEZ
#undef ELSE
#undef ENDIF
#undef GOTO
#undef BRANCH_DYNAMIC
#undef BRANCH_STATIC
#undef BRANCH_CALL
#undef BRANCH_RETURN

/*************************************************************************/
/*********************** SH-2 -> MIPS translation ************************/
/*************************************************************************/

#ifdef ENABLE_JIT

/*************************************************************************/

/* Tell the decoding core that we're translating rather than interpreting */
#define DECODE_JIT

/*************************************************************************/

/* Basic macro to append an instruction, aborting on error */
#define APPEND(insn)  do {                              \
    if (UNLIKELY(!append_insn(entry, (insn)))) {        \
        return 0;                                       \
    }                                                   \
} while (0)

/* Macros to reference and resolve labels (pass a unique number as the ID) */
#define GET_LABEL(id)  do {                             \
    if (UNLIKELY(!ref_local_label(entry, (id)))) {      \
        return 0;                                       \
    }                                                   \
} while (0)
#define SET_LABEL(id)  define_local_label(entry, (id))

/*-----------------------------------------------------------------------*/

/**
 * LOAD_REG:  Load the given register from the register file (if not
 * already loaded) and mark it as active.
 *
 * Note that this code (as well as TOUCH_REG()) has to be implemented as a
 * macro/subroutine pair because, if done as a regular macro, GCC complains
 * about negative shift counts when "reg" is not a mirror register (e.g.
 * REG_VAL0).
 *
 * [Parameters]
 *     reg: MIPS register number
 */
#define LOAD_REG(reg)  do {                                     \
    if (reg >= REG_R(0) && reg <= REG_R(15)) {                  \
        if (UNLIKELY(!load_reg(entry, reg, reg - REG_R(0)))) return 0; \
    }                                                           \
} while (0)
static inline int load_reg(JitEntry *entry, int reg, int regnum) {
    if (!(active_regs & (1 << regnum))) {
        APPEND(MIPS_LW(reg, offsetof(SH2_struct,regs.R[regnum]), MIPS_s8));
        active_regs |= 1 << regnum;
    }
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * LOAD_REGS:  Load all registers (which are not already loaded) specified
 * by the given mask from the register file.  Does not update the set of
 * active registers.
 *
 * [Parameters]
 *     mask: Mask of SH-2 registers to load
 */
#define LOAD_REGS(mask)  do {                                   \
    const uint16_t __mask = (mask);                             \
    int __regnum;                                               \
    for (__regnum = 0; __regnum < 16; __regnum++) {             \
        if (!(active_regs & __mask & (1 << __regnum))) {        \
            APPEND(MIPS_LW(REG_R(__regnum),                     \
                           offsetof(SH2_struct,regs.R[__regnum]), MIPS_s8)); \
        }                                                       \
    }                                                           \
} while (0)

/*-----------------------------------------------------------------------*/

/**
 * TOUCH_REG:  Mark the given register as active and changed without
 * loading it (e.g. when used as the destination of an ALU or load
 * instruction).
 *
 * [Parameters]
 *     reg: MIPS register number
 */
#define TOUCH_REG(reg)  do { \
    if (reg >= REG_R(0) && reg <= REG_R(15)) touch_reg(reg - REG_R(0)); \
} while (0)
static inline void touch_reg(int regnum) {
    active_regs |= 1 << regnum;
    changed_regs |= 1 << regnum;
}

/*-----------------------------------------------------------------------*/

/**
 * FLUSH_REGS:  Flush any changed registers to the register file.
 *
 * [Parameters]
 *     mask: Mask of SH-2 registers to flush
 */
#define FLUSH_REGS(mask)  do {                                  \
    const uint16_t __mask = (mask);                             \
    int __regnum;                                               \
    for (__regnum = 0; __regnum < 16; __regnum++) {             \
        if (changed_regs & __mask & (1 << __regnum)) {          \
            STATE_STORE(REG_R(__regnum), regs.R[__regnum]);     \
        }                                                       \
    }                                                           \
    changed_regs &= (uint16_t)~__mask;                          \
} while (0)

/*-----------------------------------------------------------------------*/

/**
 * CLEAR_REGS:  Flush any changed registers to the register file, and clear
 * the active and changed register bitmasks.
 *
 * [Parameters]
 *     mask: Mask of SH-2 registers to clear
 */
#define CLEAR_REGS(mask)  do {                    \
    const uint16_t __clearmask = (mask) & 0xFFFF; \
    FLUSH_REGS(__clearmask);                      \
    active_regs &= (uint16_t)~__clearmask;        \
} while (0)

/*-----------------------------------------------------------------------*/

/* Other local macros (implemented by routines below) */

#define CALL_EXT_ROUTINE()  do {                        \
    if (!UNLIKELY(call_ext_routine(entry))) {           \
        return 0;                                       \
    }                                                   \
} while (0)

#define INTERRUPT(is_call)  do {                        \
    if (UNLIKELY(!interrupt(entry, (is_call)))) {       \
        return 0;                                       \
    }                                                   \
} while (0)

/*************************************************************************/

/* Use global variables for the PC and cycle count; the initial PC can be
 * taken from the JitEntry structure */
#define initial_PC  (entry->sh2_start)
#define cur_PC      jit_PC
#define cur_cycles  jit_cycles

/* Pre-decode processing is implemented by a separate function defined below */
#define OPCODE_INIT(opcode)  do {                       \
    if (UNLIKELY(!opcode_init(state, entry, (opcode)))) { \
        return 0;                                       \
    }                                                   \
} while (0)

/* Need to increment both jit_PC and native code register */
#define INC_PC()  do {       \
    cur_PC += 2;             \
    ADDI(REG_PC, REG_PC, 2); \
} while (0)

/* Commit cycles in "cur_cycles" to the processor state block */
#define COMMIT_CYCLES(scratch)  do {            \
    if (jit_cycles > 0) {                       \
        STATE_LOAD((scratch), cycles);          \
        ADDI((scratch), (scratch), jit_cycles); \
        STATE_STORE((scratch), cycles);         \
        jit_cycles = 0;                         \
    }                                           \
} while (0)

/* Check whether we've hit the cycle limit and interrupt execution if so */
#define CHECK_CYCLES(scratch1,scratch2)  do {                   \
    STATE_LOAD((scratch1), cycles);                             \
    APPEND(MIPS_LW((scratch2), SP_cycle_limit, MIPS_sp));       \
    SUB((scratch2), (scratch1), (scratch2));  /* Check cycle limit... */ \
    GET_LABEL(0);                                               \
    APPEND(MIPS_BLTZ((scratch2), 0));                           \
    APPEND(MIPS_NOP());                                         \
    INTERRUPT(0);  /* ... and interrupt execution if reached */ \
    SET_LABEL(0);                                               \
} while (0)

/* Consume all remaining cycles */
#define CONSUME_CYCLES(scratch)  do {           \
    APPEND(MIPS_LW((scratch), SP_cycle_limit, MIPS_sp)); \
    STATE_STORE((scratch), cycles);             \
} while (0)

/*-----------------------------------------------------------------------*/

/* Register definitions */
#define REG_ZERO  MIPS_zero
#define REG_R(n)  (MIPS_t0 + (n))
#define REG_PC    MIPS_a2
#define REG_SR    MIPS_a3
#define REG_GBR   MIPS_gp
#define REG_VAL0  MIPS_v0  // Scratch register
#define REG_VAL1  MIPS_v1  // Scratch register
#define REG_VAL2  MIPS_at  // Scratch register

/* Stack frame size and offsets */
#define SP_frame_size  32
#define SP_saved_ra    28
#define SP_saved_gp    24
#define SP_internal_ra 20
#define SP_internal_v1 16
#define SP_internal_v0 12
#define SP_internal_at  8
#define SP_cycle_limit  4
#define SP_temp         0

/* Register-register operations */
#define NOP()                APPEND(MIPS_NOP())
#define MOVE(dest,src)       do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_MOVE((dest), (src))); \
                             } while (0)
#define MOVN(dest,src,test)  do { LOAD_REG(src); LOAD_REG(test); \
                                  TOUCH_REG(dest); \
                                  APPEND(MIPS_MOVN((dest), (src), (test))); \
                             } while (0)
#define MOVZ(dest,src,test)  do { LOAD_REG(src); LOAD_REG(test); \
                                  TOUCH_REG(dest); \
                                  APPEND(MIPS_MOVZ((dest), (src), (test))); \
                             } while (0)
#define ADD(dest,src1,src2)  do { LOAD_REG(src1); LOAD_REG(src2); \
                                  TOUCH_REG(dest); \
                                  APPEND(MIPS_ADDU((dest), (src1), (src2))); \
                             } while (0)
#define SUB(dest,src1,src2)  do { LOAD_REG(src1); LOAD_REG(src2); \
                                  TOUCH_REG(dest); \
                                  APPEND(MIPS_SUBU((dest), (src1), (src2))); \
                             } while (0)
#define AND(dest,src1,src2)  do { LOAD_REG(src1); LOAD_REG(src2); \
                                  TOUCH_REG(dest); \
                                  APPEND(MIPS_AND((dest), (src1), (src2))); \
                             } while (0)
#define OR(dest,src1,src2)   do { LOAD_REG(src1); LOAD_REG(src2); \
                                  TOUCH_REG(dest); \
                                  APPEND(MIPS_OR((dest), (src1), (src2))); \
                             } while (0)
#define XOR(dest,src1,src2)  do { LOAD_REG(src1); LOAD_REG(src2); \
                                  TOUCH_REG(dest); \
                                  APPEND(MIPS_XOR((dest), (src1), (src2))); \
                             } while (0)
#define NEG(dest,src)        do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_NEG((dest), (src))); \
                             } while (0)
#define NOT(dest,src)        do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_NOT((dest), (src))); \
                             } while (0)
#define SLT(dest,src1,src2)  do { LOAD_REG(src1); LOAD_REG(src2); \
                                  TOUCH_REG(dest); \
                                  APPEND(MIPS_SLT((dest), (src1), (src2))); \
                             } while (0)
#define SLTU(dest,src1,src2) do { LOAD_REG(src1); LOAD_REG(src2); \
                                  TOUCH_REG(dest); \
                                  APPEND(MIPS_SLTU((dest), (src1), (src2))); \
                             } while (0)
#define EXTS_B(dest,src)     do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_SEB((dest), (src))); \
                             } while (0)
#define EXTS_W(dest,src)     do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_SEH((dest), (src))); \
                             } while (0)
#define EXTZ_B(dest,src)     do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_ANDI((dest), (src), 0xFF)); \
                             } while (0)
#define EXTZ_W(dest,src)     do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_ANDI((dest), (src), 0xFFFF)); \
                             } while (0)
#define EXTBITS(dest,src,first,count)  do { \
    LOAD_REG(src); TOUCH_REG(dest); \
    APPEND((first)==0 && (count)<=16 \
           ? MIPS_ANDI((dest), (src), (1<<(count))-1) \
           : MIPS_EXT((dest), (src), (first), (count))); \
} while (0)
#define INSBITS(dest,src,first,count)  do { \
    LOAD_REG(src); TOUCH_REG(dest); \
    APPEND(MIPS_INS((dest), (src), (first), (count))); \
} while (0)

/* Multiplication and division */
#define MUL_START(src1,src2)    do { LOAD_REG(src1); LOAD_REG(src2); \
                                     APPEND(MIPS_MULTU((src1), (src2))); \
                                } while (0)
#define MUL_END(dest)           do { TOUCH_REG(dest); \
                                     APPEND(MIPS_MFLO((dest))); \
                                } while (0)
#define DMULS_START(src1,src2)  do { LOAD_REG(src1); LOAD_REG(src2); \
                                     APPEND(MIPS_MULT((src1), (src2))); \
                                } while (0)
#define DMULU_START(src1,src2)  do { LOAD_REG(src1); LOAD_REG(src2); \
                                     APPEND(MIPS_MULTU((src1), (src2))); \
                                } while (0)
#define DMUL_END(destlo,desthi) do { TOUCH_REG(destlo); TOUCH_REG(desthi); \
                                     APPEND(MIPS_MFLO((destlo))); \
                                     APPEND(MIPS_MFHI((desthi))); \
                                } while (0)
#define DIVS_START(dividend,divisor)  do { \
    LOAD_REG(dividend); LOAD_REG(divisor); \
    APPEND(MIPS_DIV((dividend), (divisor))); \
} while (0)
#define DIVU_START(dividend,divisor)  do { \
    LOAD_REG(dividend); LOAD_REG(divisor); \
    APPEND(MIPS_DIVU((dividend), (divisor))); \
} while (0)
#define DIV_END(quotient,remainder)  do { \
    TOUCH_REG(quotient); TOUCH_REG(remainder); \
    DMUL_END((quotient), (remainder)); \
} while (0)

/* Immediate operations */
#define MOVEI(dest,imm)  do { \
    TOUCH_REG(dest);                                            \
    const uint32_t __imm = (imm);                               \
    if ((int32_t)__imm >= -0x8000 && (int32_t)__imm <= 0x7FFF) { \
        APPEND(MIPS_ADDIU((dest), MIPS_zero, __imm));           \
    } else if (__imm <= 0xFFFF) {                               \
        APPEND(MIPS_ORI((dest), MIPS_zero, __imm));             \
    } else if ((__imm & 0xFFFF) == 0) {                         \
        APPEND(MIPS_LUI((dest), __imm >> 16));                  \
    } else {                                                    \
        DMSG("Immediate value out of range for MOVEI");         \
        return 0;                                               \
    }                                                           \
} while (0)
#define ADDI(dest,src,imm)   do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_ADDIU((dest), (src), (imm))); \
                             } while (0)
#define ANDI(dest,src,imm)   do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_ANDI((dest), (src), (imm))); \
                             } while (0)
#define ORI(dest,src,imm)    do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_ORI((dest), (src), (imm))); \
                             } while (0)
#define XORI(dest,src,imm)   do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_XORI((dest), (src), (imm))); \
                             } while (0)
#define SLLI(dest,src,imm)   do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_SLL((dest), (src), (imm))); \
                             } while (0)
#define SRLI(dest,src,imm)   do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_SRL((dest), (src), (imm))); \
                             } while (0)
#define SRAI(dest,src,imm)   do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_SRA((dest), (src), (imm))); \
                             } while (0)
#define SLTI(dest,src,imm)   do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_SLTI((dest), (src), (imm))); \
                             } while (0)
#define SLTIU(dest,src,imm)  do { LOAD_REG(src); TOUCH_REG(dest); \
                                  APPEND(MIPS_SLTIU((dest), (src), (imm))); \
                             } while (0)

/* Variants on SLTI/SLTIU */
#define SEQZ(dest,src)  SLTIU((dest), (src), 1)
#define SNEZ(dest,src)  SLTU((dest), REG_ZERO, (src))
#define SGTZ(dest,src)  SLT((dest), REG_ZERO, (src))
#define SLEZ(dest,src)  SRLI((dest), (src), 31)
#define SLTZ(dest,src)  SLT((dest), (src), REG_ZERO)

/* Register-stateblock operations */
#define STATE_LOAD(dest,field)  do { \
    TOUCH_REG(dest); \
    APPEND(MIPS_LW((dest), offsetof(SH2_struct,field), MIPS_s8)); \
} while (0)
#define STATE_STORE(src,field)  do { \
    LOAD_REG(src); \
    APPEND(MIPS_SW((src), offsetof(SH2_struct,field), MIPS_s8)); \
} while (0)
#define STATE_STOREB(src,field)  do { \
    LOAD_REG(src); \
    APPEND(MIPS_SB((src), offsetof(SH2_struct,field), MIPS_s8)); \
} while (0)

/*-----------------------------------------------------------------------*/

/* Load/store operations */

#define DO_LOAD(dest,address,load_insn,fallback,fallback_ext)  do { \
    LOAD_REG(address);                                          \
    TOUCH_REG(dest);                                            \
    APPEND(MIPS_SRL(MIPS_t8, (address), 19));                   \
    GET_LABEL(0);                                               \
    APPEND(MIPS_BEQL(MIPS_t8, MIPS_a0, 0));                     \
    APPEND(MIPS_EXT(MIPS_t9, (address), 0, 19));                \
    /* Not in the current page, so see if it's direct-access */ \
    APPEND(MIPS_SW(MIPS_t8, SP_temp, MIPS_sp));                 \
    APPEND(MIPS_SLL(MIPS_t8, MIPS_t8, 2));                      \
    APPEND(MIPS_LUI(MIPS_t9, (uint32_t)direct_pages >> 16));    \
    APPEND(MIPS_ORI(MIPS_t9, MIPS_t9, (uint32_t)direct_pages & 0xFFFF)); \
    APPEND(MIPS_ADDU(MIPS_t8, MIPS_t9, MIPS_t8));               \
    APPEND(MIPS_LW(MIPS_t8, 0, MIPS_t8));                       \
    APPEND(MIPS_LUI(MIPS_t9, (uint32_t)fallback >> 16));        \
    APPEND(MIPS_ORI(MIPS_t9, MIPS_t9, (uint32_t)fallback & 0xFFFF)); \
    GET_LABEL(1);                                               \
    APPEND(MIPS_BNEZL(MIPS_t8, 0));                             \
    APPEND(MIPS_LW(MIPS_a0, SP_temp, MIPS_sp));                 \
    /* Not directly-accessible, so call the fallback routine */ \
    APPEND(MIPS_SW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8)); \
    APPEND(MIPS_SW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8)); \
    if ((dest) != MIPS_v0) {                                    \
        APPEND(MIPS_SW(MIPS_v0, SP_internal_v0, MIPS_sp));      \
    }                                                           \
    APPEND(MIPS_MOVE(MIPS_a0, (address)));                      \
    CALL_EXT_ROUTINE();                                         \
    APPEND(MIPS_LW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8)); \
    APPEND(MIPS_LW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8)); \
    fallback_ext;                                               \
    GET_LABEL(2);                                               \
    APPEND(MIPS_B(0));                                          \
    if ((dest) != MIPS_v0) {                                    \
        APPEND(MIPS_LW(MIPS_v0, SP_internal_v0, MIPS_sp));      \
    } else {                                                    \
        APPEND(MIPS_NOP());                                     \
    }                                                           \
    SET_LABEL(1);                                               \
    APPEND(MIPS_MOVE(MIPS_a1, MIPS_t8));                        \
    APPEND(MIPS_EXT(MIPS_t9, (address), 0, 19));                \
    SET_LABEL(0);                                               \
    APPEND(MIPS_ADDU(MIPS_t8, MIPS_a1, MIPS_t9));               \
    load_insn;                                                  \
    SET_LABEL(2);                                               \
} while (0)

#define LOAD_BS(dest,address)  DO_LOAD((dest), (address), \
    do { APPEND(MIPS_XORI(MIPS_t8, MIPS_t8, 1)); \
         APPEND(MIPS_LB((dest), 0, MIPS_t8)); } while (0), \
    MappedMemoryReadByte, APPEND(MIPS_SEB((dest), MIPS_v0)))

#define LOAD_BU(dest,address)  DO_LOAD((dest), (address), \
    do { APPEND(MIPS_XORI(MIPS_t8, MIPS_t8, 1)); \
         APPEND(MIPS_LBU((dest), 0, MIPS_t8)); } while (0), \
    MappedMemoryReadByte, APPEND(MIPS_ANDI((dest), MIPS_v0, 0xFF)))

#define LOAD_WS(dest,address)  DO_LOAD((dest), (address), \
    APPEND(MIPS_LH((dest), 0, MIPS_t8)), \
    MappedMemoryReadWord, APPEND(MIPS_SEH((dest), MIPS_v0)))

#define LOAD_WU(dest,address)  DO_LOAD((dest), (address), \
    APPEND(MIPS_LHU((dest), 0, MIPS_t8)), \
    MappedMemoryReadWord, APPEND(MIPS_ANDI((dest), MIPS_v0, 0xFFFF)))

#define LOAD_L(dest,address)  DO_LOAD((dest), (address), \
    do { APPEND(MIPS_LHU((dest), 0, MIPS_t8)); \
         APPEND(MIPS_LHU(MIPS_t9, 2, MIPS_t8)); \
         APPEND(MIPS_SLL((dest), (dest), 16)); \
         APPEND(MIPS_OR((dest), (dest), MIPS_t9)); } while (0), \
    MappedMemoryReadLong, \
    do {if ((dest) != MIPS_v0) APPEND(MIPS_MOVE((dest), MIPS_v0));} while (0))

/*----------------------------------*/

#ifdef TRACE_ALL

# define LOG_STORE_C(addr,val,digits)  do { \
    uint32_t __a = (addr), __v = (val); \
    __v &= (uint32_t)(((uint64_t)1 << (digits*4)) - 1); \
    if (msh2_log) \
        fprintf(msh2_log, "WRITE%c %08X <- %0" #digits "X\n", \
                digits==2 ? 'B' : digits==4 ? 'W' : 'L', (int)__a, (int)__v); \
} while (0)
static void log_store_b(uint32_t address, uint32_t value)
{ LOG_STORE_C(address, value, 2); }
static void log_store_w(uint32_t address, uint32_t value)
{ LOG_STORE_C(address, value, 4); }
static void log_store_l(uint32_t address, uint32_t value)
{ LOG_STORE_C(address, value, 8); }

# define LOG_STORE(src,address,size)  do {                      \
    APPEND(MIPS_SW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8)); \
    APPEND(MIPS_SW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8)); \
    APPEND(MIPS_LUI(MIPS_t9, (uint32_t)log_store_##size >> 16)); \
    APPEND(MIPS_ORI(MIPS_t9, MIPS_t9, (uint32_t)log_store_##size & 0xFFFF)); \
    APPEND(MIPS_SW(MIPS_v0, SP_internal_v0, MIPS_sp));          \
    APPEND(MIPS_MOVE(MIPS_a0, (address)));                      \
    APPEND(MIPS_MOVE(MIPS_a1, (src)));                          \
    CALL_EXT_ROUTINE();                                         \
    APPEND(MIPS_LW(MIPS_v0, SP_internal_v0, MIPS_sp));          \
    APPEND(MIPS_LW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8)); \
    APPEND(MIPS_LW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8)); \
} while (0)

#else  // !TRACE_ALL

# define LOG_STORE(src,address,size)  /*nothing*/

#endif

#define DO_STORE(src,address,store_insn,fallback,logsize)  do { \
    LOAD_REG(address);                                          \
    LOAD_REG(src);                                              \
    LOG_STORE((src), (address), logsize);                       \
    /* First check for writes to ROM and disallow */            \
    APPEND(MIPS_EXT(MIPS_t9, (address), 20, 9));                \
    GET_LABEL(2);                                               \
    APPEND(MIPS_BEQZ(MIPS_t9, 0));                              \
    APPEND(MIPS_SRL(MIPS_t8, (address), 19));                   \
    GET_LABEL(0);                                               \
    APPEND(MIPS_BEQL(MIPS_t8, MIPS_a0, 0));                     \
    APPEND(MIPS_EXT(MIPS_t9, (address), 0, 19));                \
    /* Not in the current page, so see if it's direct-access */ \
    APPEND(MIPS_SW(MIPS_t8, SP_temp, MIPS_sp));                 \
    APPEND(MIPS_SLL(MIPS_t8, MIPS_t8, 2));                      \
    APPEND(MIPS_LUI(MIPS_t9, (uint32_t)direct_pages >> 16));    \
    APPEND(MIPS_ORI(MIPS_t9, MIPS_t9, (uint32_t)direct_pages & 0xFFFF)); \
    APPEND(MIPS_ADDU(MIPS_t8, MIPS_t9, MIPS_t8));               \
    APPEND(MIPS_LW(MIPS_t8, 0, MIPS_t8));                       \
    APPEND(MIPS_LUI(MIPS_t9, (uint32_t)fallback >> 16));        \
    APPEND(MIPS_ORI(MIPS_t9, MIPS_t9, (uint32_t)fallback & 0xFFFF)); \
    GET_LABEL(1);                                               \
    APPEND(MIPS_BNEZL(MIPS_t8, 0));                             \
    APPEND(MIPS_LW(MIPS_a0, SP_temp, MIPS_sp));                 \
    /* Not directly-accessible, so call the fallback routine */ \
    APPEND(MIPS_SW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8)); \
    APPEND(MIPS_SW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8)); \
    APPEND(MIPS_SW(MIPS_v0, SP_internal_v0, MIPS_sp));          \
    APPEND(MIPS_MOVE(MIPS_a0, (address)));                      \
    APPEND(MIPS_MOVE(MIPS_a1, (src)));                          \
    CALL_EXT_ROUTINE();                                         \
    APPEND(MIPS_LW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8)); \
    APPEND(MIPS_LW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8)); \
    GET_LABEL(2);                                               \
    APPEND(MIPS_B(0));                                          \
    APPEND(MIPS_LW(MIPS_v0, SP_internal_v0, MIPS_sp));          \
    SET_LABEL(1);                                               \
    APPEND(MIPS_MOVE(MIPS_a1, MIPS_t8));                        \
    APPEND(MIPS_EXT(MIPS_t9, (address), 0, 19));                \
    SET_LABEL(0);                                               \
    APPEND(MIPS_ADDU(MIPS_t8, MIPS_a1, MIPS_t9));               \
    store_insn;                                                 \
    /* Check for modified translations and clear if needed */   \
    APPEND(MIPS_SRL(MIPS_t8, (address), 19));                   \
    APPEND(MIPS_LUI(MIPS_t9, (uint32_t)direct_jit_pages >> 16)); \
    APPEND(MIPS_ORI(MIPS_t9, MIPS_t9, (uint32_t)direct_jit_pages & 0xFFFF)); \
    APPEND(MIPS_SLL(MIPS_t8, MIPS_t8, 2));                      \
    APPEND(MIPS_ADDU(MIPS_t9, MIPS_t9, MIPS_t8));               \
    APPEND(MIPS_LW(MIPS_t9, 0, MIPS_t9));                       \
    APPEND(MIPS_EXT(MIPS_t8, (address), JIT_PAGE_BITS+3, 19-(JIT_PAGE_BITS+3))); \
    GET_LABEL(2);                                               \
    APPEND(MIPS_BEQZ(MIPS_t9, 0));                              \
    APPEND(MIPS_ADDU(MIPS_t9, MIPS_t8, MIPS_t9));               \
    APPEND(MIPS_LBU(MIPS_t8, 0, MIPS_t9));                      \
    APPEND(MIPS_EXT(MIPS_t9, (address), JIT_PAGE_BITS, 3));     \
    APPEND(MIPS_SRLV(MIPS_t8, MIPS_t8, MIPS_t9));               \
    APPEND(MIPS_ANDI(MIPS_t8, MIPS_t8, 1));                     \
    GET_LABEL(2);                                               \
    APPEND(MIPS_BEQZ(MIPS_t8, 0));                              \
    APPEND(MIPS_LUI(MIPS_t9, (uint32_t)jit_clear_write >> 16)); \
    APPEND(MIPS_ORI(MIPS_t9, MIPS_t9, (uint32_t)jit_clear_write & 0xFFFF)); \
    APPEND(MIPS_SW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8)); \
    APPEND(MIPS_SW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8)); \
    APPEND(MIPS_SW(MIPS_v0, SP_internal_v0, MIPS_sp));          \
    APPEND(MIPS_MOVE(MIPS_a0, MIPS_s8));                        \
    APPEND(MIPS_MOVE(MIPS_a1, (address)));                      \
    CALL_EXT_ROUTINE();                                         \
    APPEND(MIPS_LW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8)); \
    APPEND(MIPS_LW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8)); \
    APPEND(MIPS_LW(MIPS_v0, SP_internal_v0, MIPS_sp));          \
    SET_LABEL(2);                                               \
} while (0)

#define STORE_B(src,address)  DO_STORE((src), (address), \
    do { APPEND(MIPS_XORI(MIPS_t8, MIPS_t8, 1)); \
         APPEND(MIPS_SB((src), 0, MIPS_t8)); } while (0), \
    MappedMemoryWriteByte, b)

#define STORE_W(src,address)  DO_STORE((src), (address), \
    APPEND(MIPS_SH((src), 0, MIPS_t8)), \
    MappedMemoryWriteWord, w)

#define STORE_L(src,address)  DO_STORE((src), (address), \
    do { APPEND(MIPS_SRL(MIPS_t9, (src), 16)); \
         APPEND(MIPS_SH(MIPS_t9, 0, MIPS_t8)); \
         APPEND(MIPS_SH((src), 2, MIPS_t8)); } while (0), \
    MappedMemoryWriteLong, l)

/*-----------------------------------------------------------------------*/

/* Conditional blocks and branches */
#define IFZ(label,reg,delay)  do {      \
    LOAD_REG(reg);                      \
    GET_LABEL(label);                   \
    APPEND(MIPS_BNEZ((reg), 0));        \
    delay;                              \
} while (0);
#define IFNZ(label,reg,delay)  do {     \
    LOAD_REG(reg);                      \
    GET_LABEL(label);                   \
    APPEND(MIPS_BEQZ((reg), 0));        \
    delay;                              \
} while (0);
#define IFGEZ(label,reg,delay)  do {    \
    LOAD_REG(reg);                      \
    GET_LABEL(label);                   \
    APPEND(MIPS_BLTZ((reg), 0));        \
    delay;                              \
} while (0);
#define ELSE(lab1,lab2,delay)  do {     \
    GET_LABEL(lab2);                    \
    APPEND(MIPS_B(0));                  \
    delay;                              \
    SET_LABEL(lab1);                    \
} while (0);
#define ENDIF(label)  SET_LABEL(label)
#define GOTO(label,delay)  do {         \
    GET_LABEL(label);                   \
    APPEND(MIPS_B(0));                  \
    delay;                              \
} while (0)

#define BRANCH_DYNAMIC(reg)  do {       \
    MOVE(REG_PC, (reg));                \
    APPEND(MIPS_J(((uint32_t)mips_terminate & 0x0FFFFFFC) >> 2)); \
    MOVEI(MIPS_v0, 0);                  \
} while (0)

#define BRANCH_STATIC(reg)  do {        \
    if (UNLIKELY(!branch_static(state, entry, (reg)))) {  \
        return 0;                       \
    }                                   \
} while (0)

#define BRANCH_CALL(reg)  do {          \
    CLEAR_REGS(~0);                     \
    MOVE(REG_PC, (reg));                \
    INTERRUPT(1);                       \
} while (0)

#define BRANCH_RETURN(reg)  do {        \
    CLEAR_REGS(~0);                     \
    MOVE(REG_PC, (reg));                \
    APPEND(MIPS_J(((uint32_t)mips_terminate & 0x0FFFFFFC) >> 2)); \
    MOVEI(MIPS_v0, 1);                  \
} while (0)

/*************************************************************************/

/**
 * mips_prologue:  Common prologue for native code blocks.
 */
static const uint32_t mips_prologue[] = {
    MIPS_ADDIU(MIPS_sp, MIPS_sp, -SP_frame_size),
    MIPS_SW(MIPS_ra, SP_saved_ra, MIPS_sp),
    MIPS_SW(MIPS_gp, SP_saved_gp, MIPS_sp),
    MIPS_SW(MIPS_v1, SP_cycle_limit, MIPS_sp),
    MIPS_LW(MIPS_a2, offsetof(SH2_struct,regs.PC ), MIPS_s8),
    MIPS_LW(MIPS_a3, offsetof(SH2_struct,regs.SR ), MIPS_s8),
    MIPS_LW(MIPS_gp, offsetof(SH2_struct,regs.GBR), MIPS_s8),
    MIPS_LW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8),
    MIPS_LW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8),
};

/*-----------------------------------------------------------------------*/

/**
 * mips_terminate:  Terminate execution of the current code block.  The
 * value to be returned should be stored in $v0.  Registers must be flushed
 * before jumping to this address.
 */
static const uint32_t mips_terminate[] = {
    MIPS_LW(MIPS_ra, SP_saved_ra, MIPS_sp),
    MIPS_SW(MIPS_a2, offsetof(SH2_struct,regs.PC ), MIPS_s8),
    MIPS_SW(MIPS_a3, offsetof(SH2_struct,regs.SR ), MIPS_s8),
    MIPS_SW(MIPS_gp, offsetof(SH2_struct,regs.GBR), MIPS_s8),
    MIPS_SW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8),
    MIPS_SW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8),
    MIPS_LW(MIPS_gp, SP_saved_gp, MIPS_sp),
    MIPS_JR(MIPS_ra),
    MIPS_ADDIU(MIPS_sp, MIPS_sp, SP_frame_size),
};

/*-----------------------------------------------------------------------*/

/**
 * mips_interrupt:  Interrupt execution of the current code block,
 * returning the address at which to continue execution.  Call this
 * routine with a JAL instruction; execution will resume at the following
 * instruction when the block is next called.  Registers must be flushed
 * before jumping to this address.
 */
static const uint32_t mips_interrupt[] = {
    MIPS_OR(MIPS_v0, MIPS_v0, MIPS_ra),
    MIPS_LW(MIPS_ra, SP_saved_ra, MIPS_sp),
    MIPS_SW(MIPS_a2, offsetof(SH2_struct,regs.PC ), MIPS_s8),
    MIPS_SW(MIPS_a3, offsetof(SH2_struct,regs.SR ), MIPS_s8),
    MIPS_SW(MIPS_gp, offsetof(SH2_struct,regs.GBR), MIPS_s8),
    MIPS_SW(MIPS_a0, offsetof(SH2_struct,psp.cached_page), MIPS_s8),
    MIPS_SW(MIPS_a1, offsetof(SH2_struct,psp.cached_ptr ), MIPS_s8),
    MIPS_LW(MIPS_gp, SP_saved_gp, MIPS_sp),
    MIPS_JR(MIPS_ra),
    MIPS_ADDIU(MIPS_sp, MIPS_sp, SP_frame_size),
};

/*************************************************************************/

/**
 * call_ext_routine:  Call an external routine in $t9, preserving mirror
 * register state.
 *
 * [Parameters]
 *     entry: Block being translated
 * [Return value]
 *     Nonzero on success, zero on failure
 */
static int call_ext_routine(JitEntry *entry)
{
    /* Flush any modified registers in $t0-$t7 */
    int regnum;
    for (regnum = 0; regnum < 8; regnum++) {
        if (changed_regs & (1 << regnum)) {
            STATE_STORE(REG_R(regnum), regs.R[regnum]);
        }
    }
 
   /* Flush common registers and call the routine */
    STATE_STORE(MIPS_a2, regs.PC);
    STATE_STORE(MIPS_a3, regs.SR);
    STATE_STORE(MIPS_gp, regs.GBR);
    APPEND(MIPS_SW(MIPS_v1, SP_internal_v1, MIPS_sp));
    APPEND(MIPS_SW(MIPS_at, SP_internal_at, MIPS_sp));
    APPEND(MIPS_JALR(MIPS_t9));

    /* Restore common registers */
    uint16_t saved_changed_regs = changed_regs;
    APPEND(MIPS_LW(MIPS_gp, SP_saved_gp, MIPS_sp));
    APPEND(MIPS_LW(MIPS_v1, SP_internal_v1, MIPS_sp));
    APPEND(MIPS_LW(MIPS_at, SP_internal_at, MIPS_sp));
    STATE_LOAD(MIPS_a2, regs.PC);
    STATE_LOAD(MIPS_a3, regs.SR);
    STATE_LOAD(MIPS_gp, regs.GBR);

    /* Restore any active registers in $t0-$t7 */
    for (regnum = 0; regnum < 8; regnum++) {
        if (active_regs & (1 << regnum)) {
            STATE_LOAD(REG_R(regnum), regs.R[regnum]);
        }
    }

    /* Reverse any "changes" caused by STATE_LOAD */
    changed_regs = saved_changed_regs;

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * interrupt:  Interrupt execution of the code block.  Execution will
 * continue with the next instruction on the next jit_exec() call.
 *
 * [Parameters]
 *       entry: Block being translated
 *     is_call: Nonzero if the interruption is for a JSR/BSR, else zero
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int interrupt(JitEntry *entry, int is_call)
{
    int regnum;
    for (regnum = 0; regnum < 16; regnum++) {
        if (changed_regs & (1 << regnum)) {
            STATE_STORE(REG_R(regnum), regs.R[regnum]);
        }
    }

    APPEND(MIPS_JAL(((uint32_t)mips_interrupt & 0x0FFFFFFC) >> 2));
    if ((is_call)) {
        MOVEI(REG_VAL0, 0x80000000);
    } else {
        MOVEI(REG_VAL0, 0);
    }
    if (!append_block(entry, mips_prologue, sizeof(mips_prologue))) {
        return 0;
    }

    uint16_t saved_changed_regs = changed_regs;
    for (regnum = 0; regnum < 16; regnum++) {
        if (active_regs & (1 << regnum)) {
            STATE_LOAD(REG_R(regnum), regs.R[regnum]);
        }
    }
    changed_regs = saved_changed_regs;

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * opcode_init:  Perform pre-decode processing for an instruction.
 * Implements the OPCODE_INIT() macro used by the decoding core.
 *
 * [Parameters]
 *      state: Processor state block
 *      entry: Block being translated
 *     opcode: SH-2 opcode of instruction being decoded
 * [Return value]
 *     Nonzero on success, zero on failure
 */
static int opcode_init(SH2_struct *state, JitEntry *entry, uint16_t opcode)
{
#ifdef JIT_DEBUG_TRACE
    /*
     * (0A) If tracing is enabled, print a trace line for the instruction.
     */
    char tracebuf[100];
    SH2Disasm(jit_PC, opcode, 0, tracebuf);
    fprintf(stderr, "%08X: %04X  %s\n", jit_PC, opcode, tracebuf+12);
#endif

#ifdef JIT_DEBUG_INSERT_PC
    /*
     * (0B) If enabled, insert dummy instructions indicating the current PC.
     */
    APPEND(MIPS_LUI(MIPS_zero, jit_PC>>16));
    APPEND(MIPS_ORI(MIPS_zero, MIPS_zero, jit_PC & 0xFFFF));
#endif

    /*
     * (1) Check whether this address is the target of a branch from
     *     elsewhere within the same block.  If so, flush all modified
     *     registers to the register file and commit any pending cycles.
     */
    int is_target = 0;
    int index;
    for (index = 0; index < btlist_len; index++) {
        if (btlist[index] == jit_PC) {
            is_target = 1;
            break;
        }
    }
    if (is_target) {
        if (jit_cycles > 0) {
            /* Load early so we can potentially avoid a load stall */
            STATE_LOAD(MIPS_v0, cycles);
        }
        FLUSH_REGS(~0);
        if (jit_cycles > 0) {
            ADDI(MIPS_v0, MIPS_v0, jit_cycles);
            STATE_STORE(MIPS_v0, cycles);
            jit_cycles = 0;
        }
    }

    /*
     * (2A) Check whether there are any pending static branches to the
     *      current address.  If so, record which mirror registers need
     *      to be cleared to maintain consistency with the state following
     *      each branch source.
     */
    int have_branch = 0;
    uint16_t clear_regs = 0;
    for (index = 0; index < lenof(unres_branches); index++) {
        if (unres_branches[index].sh2_target == jit_PC) {
            have_branch = 1;
            clear_regs |= active_regs & ~unres_branches[index].active_regs;
        }
    }

    /*
     * (2B) If a branch was found, clear all necessary mirror registers.
     */
    if (clear_regs) {
        CLEAR_REGS(clear_regs);
    }

    /*
     * (3) In JIT_ACCURATE_ACCESS_TIMING mode, if the current instruction is
     *     not in a delay slot, determine whether the instruction performs
     *     a load or store and check the cycle count if so.  (This is done
     *     before the branch target because a cycle check or unconditional
     *     interrupt/termination is always performed on a branch.)
     */
#ifdef JIT_ACCURATE_ACCESS_TIMING
    if (!state->delay) {
        if ((opcode & 0xF00E) == 0x0004
         || (opcode & 0xF00F) == 0x0006
         ||  opcode           == 0x002B
         || (opcode & 0xF00C) == 0x000C
         || (opcode & 0x3000) == 0x1000
         || (opcode & 0xB00A) == 0x2000
         || (opcode & 0xB00B) == 0x2002
         || (opcode & 0xF00A) == 0x4002
         || (opcode & 0xF0FF) == 0x401B
         || (opcode & 0xF00F) == 0x400F
         || (opcode & 0xFA00) == 0x8000
         || (opcode & 0xFC00) == 0xC000
         || (opcode & 0xFE00) == 0xC400
         || (opcode & 0xFF00) == 0xC600
         || (opcode & 0xFC00) == 0xCC00
        ) {
            COMMIT_CYCLES(REG_VAL0);
            CHECK_CYCLES(REG_VAL0, REG_VAL1);
        }
    }
#endif

    /*
     * (4) Record the current native offset in the branch target cache.
     */
    btcache[btcache_index].sh2_address = jit_PC;
    btcache[btcache_index].active_regs = active_regs;
    btcache[btcache_index].native_offset = entry->native_length;
    btcache_index++;
    if (UNLIKELY(btcache_index >= lenof(btcache))) {
        btcache_index = 0;
    }

    /*
     * (5) Update any pending static branches to the current address.
     */
    if (have_branch) {
        for (index = 0; index < lenof(unres_branches); index++) {
            if (unres_branches[index].sh2_target == jit_PC) {
                if (fixup_branch(entry, unres_branches[index].native_offset,
                                 entry->native_length)) {
                    unres_branches[index].sh2_target = 0;
                }
            }
        }
    }

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * branch_static:  Generate code to branch to a static address.
 * Implements the BRANCH_STATIC() macro used by the decoding core.
 *
 * [Parameters]
 *     state: Processor state block
 *     entry: Block being translated
 *       reg: Register containing branch target (note that the branch
 *               target is also stored in state->psp.branch_target)
 * [Return value]
 *     Nonzero on success, zero on failure
 */
static int branch_static(SH2_struct *state, JitEntry *entry, int reg)
{
    MOVE(REG_PC, (reg));

    const uint32_t check_offset = entry->native_length;
#ifdef TRACE_ALL
    INTERRUPT(0);
#else
    CHECK_CYCLES(REG_VAL0, REG_VAL1);
#endif

    uint16_t new_active;
    int32_t offset = btcache_lookup(state->psp.branch_target, &new_active);
    if (offset >= 0) {
        CLEAR_REGS(active_regs & ~new_active);
        LOAD_REGS(~active_regs & new_active);
        offset -= (entry->native_length + 4);
        offset /= 4;
        APPEND(MIPS_B(offset));
        NOP();
    } else {
#ifdef JIT_DEBUG_VERBOSE
        DMSG("Unresolved branch from %08X to %08X at %p", jit_PC - 2,
             (int)state->psp.branch_target,
             (uint8_t *)entry->native_code + entry->native_length);
#endif
        record_unresolved_branch(entry, state->psp.branch_target,
                                 entry->native_length, check_offset);
        APPEND(MIPS_J(((uint32_t)mips_terminate & 0x0FFFFFFC) >> 2));
        MOVEI(MIPS_v0, 0);
    }

    return 1;
}

/*************************************************************************/

/**
 * translate_insn:  Translate a single SH-2 instruction at the given
 * address into one or more equivalent MIPS instructions.
 *
 * [Parameters]
 *     state: SH-2 processor state
 *     entry: Block being translated
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int translate_insn(SH2_struct *state, JitEntry *entry)
{

#include "sh2-core.i"

    if (UNLIKELY(clear_local_labels())) {  // Make sure nothing's left over
        return 0;
    }

#ifdef TRACE_ALL
    COMMIT_CYCLES(REG_VAL0);
    if (state->delay
     && !(state->psp.branch_type == SH2BRTYPE_BT_S
       || state->psp.branch_type == SH2BRTYPE_BF_S)
    ) {
        /* Make sure we continue properly for a delay slot */
        MOVEI(REG_VAL0, 1);
        STATE_STORE(REG_VAL0, delay);
    }
    INTERRUPT(0);
#else
# ifndef OPTIMIZE_STATE_BLOCK
    /* Flush everything to the state block */
    CLEAR_REGS(~0);
    COMMIT_CYCLES(REG_VAL0);
# endif
    /* After "LDC ...,SR", interrupt execution so we have a chance to check
     * for interrupts */
    if ((opcode & 0xF0FF) == 0x4007
     || (opcode & 0xF0FF) == 0x400E
    ) {
        COMMIT_CYCLES(REG_VAL0);
        INTERRUPT(0);
    }
#endif

    return 1;
}

/*************************************************************************/

#undef DECODE_JIT

#undef APPEND
#undef GET_LABEL
#undef SET_LABEL

#undef LOAD_REG
#undef TOUCH_REG
#undef FLUSH_REGS
#undef CLEAR_REGS
#undef INTERRUPT
#undef CALL_EXT_ROUTINE

#undef cur_PC
#undef cur_cycles

#undef OPCODE_INIT
#undef INC_PC
#undef COMMIT_CYCLES
#undef CHECK_CYCLES
#undef CONSUME_CYCLES

#undef REG_ZERO
#undef REG_R
#undef REG_PC
#undef REG_SR
#undef REG_GBR
#undef REG_VAL0
#undef REG_VAL1
#undef REG_VAL2

#undef NOP
#undef MOVE
#undef MOVN
#undef MOVZ
#undef ADD
#undef SUB
#undef AND
#undef OR
#undef XOR
#undef NEG
#undef NOT
#undef SLT
#undef SLTU
#undef EXTS_B
#undef EXTS_W
#undef EXTZ_B
#undef EXTZ_W
#undef EXTBITS
#undef INSBITS

#undef MUL_START
#undef MUL_END
#undef DMULS_START
#undef DMULU_START
#undef DMUL_END
#undef DIVS_START
#undef DIVU_START
#undef DIV_END

#undef MOVEI
#undef ADDI
#undef ANDI
#undef ORI
#undef XORI
#undef SLLI
#undef SRLI
#undef SRAI
#undef SLTI
#undef SLTIU

#undef SEQZ
#undef SNEZ
#undef SGTZ
#undef SLEZ
#undef SLTZ

#undef STATE_LOAD
#undef STATE_STORE
#undef STATE_STOREB

#undef DO_LOAD
#undef LOAD_BS
#undef LOAD_BU
#undef LOAD_WS
#undef LOAD_WU
#undef LOAD_L
#undef LOG_STORE_C
#undef LOG_STORE
#undef DO_STORE
#undef STORE_B
#undef STORE_W
#undef STORE_L

#undef IFZ
#undef IFNZ
#undef IFGEZ
#undef ELSE
#undef ENDIF
#undef GOTO
#undef BRANCH_DYNAMIC
#undef BRANCH_STATIC
#undef BRANCH_CALL
#undef BRANCH_RETURN

/*************************************************************************/

#endif  // ENABLE_JIT

/*************************************************************************/
/********************* Optimization utility routines *********************/
/*************************************************************************/

#ifdef OPTIMIZE_IDLE

/*
 * A loop is "idle" if a single execution of the entire loop, starting from
 * the first instruction of the loop and ending immediately before the next
 * time the first instruction is executed, has no net effect on the state
 * of the system after the first iteration of the loop.  Each individual
 * instruction may alter system state (and in fact, the program counter
 * will change with each instruction executed), but provided that the
 * external system state remains constant, the final result of executing N
 * loops must be identical to the result of executing N-1 loops for N
 * greater than 1.
 *
 * For example, a loop consisting solely of a branch to the same
 * instruction:
 *     0x1000: bra 0x1000
 *     0x1002: nop
 * would naturally qualify, as would a loop reading from the same memory
 * location (here, waiting for a memory address to read as zero):
 *     0x1000: mov.l @r0, r1
 *     0x1002: tst r1, r1
 *     0x1004: bf 0x1000
 *
 * On the flip side, a loop with a counter:
 *     0x1000: dt r0
 *     0x1002: bf 0x1000
 * would _not_ qualify, because the state of the system changes with each
 * iteration of the loop.  Likewise, a loop containing a store to memory:
 *     0x1000: mov.l r2, @r3
 *     0x1002: mov.l @r0, r1
 *     0x1004: tst r1, r1
 *     0x1006: bt 0x1000
 * would not qualify, because a store operation is assumed to change system
 * state even if the value stored is the same.  (Even for ordinary memory,
 * in a multiprocessor system like the Saturn memory can be accessed by
 * agents other than the processor, and an identical store operation may
 * have differing effects from one iteration to the next.)
 *
 * To determine whether a loop is idle, we check whether all operations
 * performed in the loop depend on constant values (the contents of memory
 * are assumed to be constant for this purpose).  In other words, no
 * instruction in the loop may modify a register which is used as a source
 * for that or an earlier instruction.  One result of this is that any loop
 * with an ALU-type operation other than CMP is never (except as discussed
 * below) considered idle, even in cases where the loop does not in fact
 * have a net change in state:
 *     0x1000: mov.l @r0+, r1
 *     0x1002: cmp/pl @r1
 *     0x1004: bt 0x100A
 *     0x1006: bra 0x1000
 *     0x1008: add #-4, r0
 *     0x100A: ...
 * While admittedly somewhat contrived, this example has no net effect on
 * system state because R0 is decremented at the end of each loop; but
 * because postincrement addressing mode is used, the value of R0 cannot
 * be considered a constant, so the loop is not treated as idle.  (A more
 * sophisticated algorithm could track such changes in value and determine
 * their cumulative effect, but in most cases the simplistic algorithm we
 * use is sufficient.)
 *
 * As an exception to the above, if a register is modified before it is
 * used by another instruction, the register becomes a "don't care" for
 * the remainder of the loop.  Thus a loop like:
 *     0x1000: mov.b @r0, r1
 *     0x1002: extu.w r1, r1
 *     0x1004: tst r1, r1
 *     0x1006: bt 0x1000
 * qualifies as an idle loop despite the "extu" instruction writing to
 * the same register it reads from.
 *
 * The constants and tables below are used by can_optimize_idle() to look
 * up the effects of each instruction.
 */

/*-----------------------------------------------------------------------*/

/* Bit values for register bitmasks */
#define IDLE_R(n)  (1U << (n))
#define IDLE_SR_T  (1U << 16)
#define IDLE_SR_S  (1U << 17)
#define IDLE_SR_Q  (1U << 18)
#define IDLE_SR_M  (1U << 19)
#define IDLE_SR_MQT (IDLE_SR_T | IDLE_SR_Q | IDLE_SR_M)
#define IDLE_SR    (IDLE_SR_T | IDLE_SR_S | IDLE_SR_Q | IDLE_SR_M)
#define IDLE_GBR   (1U << 20)
#define IDLE_VBR   (1U << 21)
#define IDLE_PR    (1U << 22)
#define IDLE_MACL  (1U << 23)
#define IDLE_MACH  (1U << 24)
#define IDLE_MAC   (IDLE_MACL | IDLE_MACH)

/* Value used in IdleInfo.changed field to indicate an instruction which
 * can never be part of an idle loop */
#define IDLE_BAD   (1U << 31)

/* Virtual bits used for the Rn (bits 8-11) and Rm (bits 4-7) fields of
 * the instruction; e.g. IDLE_Rn translates to IDLE_R(opcode>>8 & 0xF) */
#define IDLE_Rn    (1U << 30)
#define IDLE_Rm    (1U << 29)

/* Table data structure; fields "used" and "changed" are ignored if
 * "subtable" is non-NULL */
typedef struct IdleInfo_ IdleInfo;
struct IdleInfo_ {
    uint32_t used;      // Bitmask of registers used by the instruction
    uint32_t changed;   // Bitmask of registers changed by the instruction
    int next_shift;     // Bit position of subtable index (0, 4, or 8)
    const IdleInfo *subtable; // NULL if no subtable for this opcode
};

/*-----------------------------------------------------------------------*/

/* Opcode table for $0xx2 opcodes */
static const IdleInfo idle_table_0xx2[16] = {
    {.used = IDLE_SR, .changed = IDLE_Rn},  // STC SR,Rn
    {.used = IDLE_GBR, .changed = IDLE_Rn}, // STC GBR,Rn
    {.used = IDLE_VBR, .changed = IDLE_Rn}, // STC VBR,Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $0xx3 opcodes */
static const IdleInfo idle_table_0xx3[16] = {
    {.changed = IDLE_BAD},              // BSRF Rn
    {.changed = IDLE_BAD},              // invalid
    {.used = IDLE_Rn, .changed = 0},    // BRAF Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $0xx8 opcodes */
static const IdleInfo idle_table_0xx8[16] = {
    {.used = 0, .changed = IDLE_SR_T},  // CLRT
    {.used = 0, .changed = IDLE_SR_T},  // SETT
    {.used = 0, .changed = IDLE_MAC},   // CLRMAC
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $0xx9 opcodes */
static const IdleInfo idle_table_0xx9[16] = {
    {.used = 0, .changed = 0},          // NOP
    {.used = 0, .changed = IDLE_SR_MQT}, // DIV0U
    {.used = IDLE_SR_T, .changed = IDLE_Rn}, // MOVT Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

static const IdleInfo idle_table_0xxA[16] = {
    {.used = IDLE_MACH, .changed = IDLE_Rn}, // STS MACH,Rn
    {.used = IDLE_MACL, .changed = IDLE_Rn}, // STS MACL,Rn
    {.used = IDLE_PR, .changed = IDLE_Rn},   // STS PR,Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $0xxx opcodes */
static const IdleInfo idle_table_0xxx[16] = {
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.next_shift = 4, .subtable = idle_table_0xx2},
    {.next_shift = 4, .subtable = idle_table_0xx3},

    {.changed = IDLE_BAD},              // MOV.B Rm,@(R0,Rn)
    {.changed = IDLE_BAD},              // MOV.W Rm,@(R0,Rn)
    {.changed = IDLE_BAD},              // MOV.L Rm,@(R0,Rn)
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_MACL}, // MUL.L Rm,Rn

    {.next_shift = 4, .subtable = idle_table_0xx8},
    {.next_shift = 4, .subtable = idle_table_0xx9},
    {.next_shift = 4, .subtable = idle_table_0xxA},
    {.changed = IDLE_BAD},              // RTS, SLEEP, RTE

    {.used = IDLE_R(0)|IDLE_Rm, .changed = IDLE_Rn}, // MOV.B @(R0,Rm),Rn
    {.used = IDLE_R(0)|IDLE_Rm, .changed = IDLE_Rn}, // MOV.W @(R0,Rm),Rn
    {.used = IDLE_R(0)|IDLE_Rm, .changed = IDLE_Rn}, // MOV.L @(R0,Rm),Rn
    {.changed = IDLE_BAD},              // MAC.L @Rm+,@Rn+
};

/*----------------------------------*/

/* Opcode table for $2xxx opcodes */
static const IdleInfo idle_table_2xxx[16] = {
    {.changed = IDLE_BAD},              // MOV.B Rm,@Rn
    {.changed = IDLE_BAD},              // MOV.W Rm,@Rn
    {.changed = IDLE_BAD},              // MOV.L Rm,@Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // MOV.B Rm,@-Rn
    {.changed = IDLE_BAD},              // MOV.W Rm,@-Rn
    {.changed = IDLE_BAD},              // MOV.L Rm,@-Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_SR_MQT}, // DIV0S Rm,Rn

    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_SR_T}, // TST Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_Rn},   // AND Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_Rn},   // XOR Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_Rn},   // OR Rm,Rn

    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_SR_T}, // CMP/ST Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_Rn},   // XTRCT Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_MACL}, // MULU.W Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_MACL}, // MULS.W Rm,Rn
};

/*----------------------------------*/

/* Opcode table for $3xxx opcodes */
static const IdleInfo idle_table_3xxx[16] = {
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_SR_T}, // CMP/EQ Rm,Rn
    {.changed = IDLE_BAD},              // invalid
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_SR_T}, // CMP/HS Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_SR_T}, // CMP/GE Rm,Rn

    {.changed = IDLE_BAD},              // DIV1 Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_MAC}, // DMULU.L Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_SR_T}, // CMP/HI Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_SR_T}, // CMP/GT Rm,Rn

    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_Rn}, // SUB Rm,Rn
    {.changed = IDLE_BAD},              // invalid
    {.used = IDLE_Rm|IDLE_Rn|IDLE_SR_T, .changed = IDLE_Rn|IDLE_SR_T}, // SUBC Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn|IDLE_SR_T, .changed = IDLE_Rn|IDLE_SR_T}, // SUBV Rm,Rn

    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_Rn}, // ADD Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_MAC}, // DMULS.L Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn|IDLE_SR_T, .changed = IDLE_Rn|IDLE_SR_T}, // ADDC Rm,Rn
    {.used = IDLE_Rm|IDLE_Rn|IDLE_SR_T, .changed = IDLE_Rn|IDLE_SR_T}, // ADDV Rm,Rn
};

/*----------------------------------*/

/* Opcode table for $4xx0 opcodes */
static const IdleInfo idle_table_4xx0[16] = {
    {.used = IDLE_Rn, .changed = IDLE_Rn|IDLE_SR_T}, // SHLL Rn
    {.used = IDLE_Rn, .changed = IDLE_Rn|IDLE_SR_T}, // DT Rn
    {.used = IDLE_Rn, .changed = IDLE_Rn|IDLE_SR_T}, // SHAL Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $4xx1 opcodes */
static const IdleInfo idle_table_4xx1[16] = {
    {.used = IDLE_Rn, .changed = IDLE_Rn|IDLE_SR_T}, // SHLR Rn
    {.used = IDLE_Rn, .changed = IDLE_SR_T}, // CMP/PZ Rn
    {.used = IDLE_Rn, .changed = IDLE_Rn|IDLE_SR_T}, // SHAR Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $4xx4 opcodes */
static const IdleInfo idle_table_4xx4[16] = {
    {.used = IDLE_Rn, .changed = IDLE_Rn|IDLE_SR_T}, // ROTL Rn
    {.changed = IDLE_BAD},              // invalid
    {.used = IDLE_Rn|IDLE_SR_T, .changed = IDLE_Rn|IDLE_SR_T}, // ROTCL Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $4xx5 opcodes */
static const IdleInfo idle_table_4xx5[16] = {
    {.used = IDLE_Rn, .changed = IDLE_Rn|IDLE_SR_T}, // ROTR Rn
    {.used = IDLE_Rn, .changed = IDLE_SR_T}, // CMP/PL Rn
    {.used = IDLE_Rn|IDLE_SR_T, .changed = IDLE_Rn|IDLE_SR_T}, // ROTCR Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $4xx8 opcodes */
static const IdleInfo idle_table_4xx8[16] = {
    {.used = IDLE_Rn, .changed = IDLE_Rn}, // SHLL2 Rn
    {.used = IDLE_Rn, .changed = IDLE_Rn}, // SHLL8 Rn
    {.used = IDLE_Rn, .changed = IDLE_Rn}, // SHLL16 Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $4xx9 opcodes */
static const IdleInfo idle_table_4xx9[16] = {
    {.used = IDLE_Rn, .changed = IDLE_Rn}, // SHLR2 Rn
    {.used = IDLE_Rn, .changed = IDLE_Rn}, // SHLR8 Rn
    {.used = IDLE_Rn, .changed = IDLE_Rn}, // SHLR16 Rn
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $4xxA opcodes */
static const IdleInfo idle_table_4xxA[16] = {
    {.used = IDLE_Rn, .changed = IDLE_MACH}, // LDS Rn,MACH
    {.used = IDLE_Rn, .changed = IDLE_MACL}, // LDS Rn,MACL
    {.used = IDLE_Rn, .changed = IDLE_PR},   // LDS Rn,PR
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $4xxB opcodes */
static const IdleInfo idle_table_4xxB[16] = {
    {.changed = IDLE_BAD},              // JSR
    {.changed = IDLE_BAD},              // TAS @Rn
    {.used = 0, .changed = 0},          // JMP
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $4xxE opcodes */
static const IdleInfo idle_table_4xxE[16] = {
    {.used = IDLE_Rn, .changed = IDLE_SR},  // LDC Rn,SR
    {.used = IDLE_Rn, .changed = IDLE_GBR}, // LDC Rn,GBR
    {.used = IDLE_Rn, .changed = IDLE_VBR}, // LDC Rn,VBR
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
};

/* Opcode table for $4xxx opcodes */
static const IdleInfo idle_table_4xxx[16] = {
    {.next_shift = 4, .subtable = idle_table_4xx0},
    {.next_shift = 4, .subtable = idle_table_4xx1},
    {.changed = IDLE_BAD},              // STSL ...,@-Rn
    {.changed = IDLE_BAD},              // STCL ...,@-Rn

    {.next_shift = 4, .subtable = idle_table_4xx4},
    {.next_shift = 4, .subtable = idle_table_4xx5},
    {.changed = IDLE_BAD},              // LDSL @Rm+,...
    {.changed = IDLE_BAD},              // LDCL @Rm+,...

    {.next_shift = 4, .subtable = idle_table_4xx8},
    {.next_shift = 4, .subtable = idle_table_4xx9},
    {.next_shift = 4, .subtable = idle_table_4xxA},
    {.next_shift = 4, .subtable = idle_table_4xxB},

    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.next_shift = 4, .subtable = idle_table_4xxE},
    {.changed = IDLE_BAD},              // MAC.W @Rm+,@Rn+
};

/*----------------------------------*/

/* Opcode table for $6xxx opcodes */
static const IdleInfo idle_table_6xxx[16] = {
    {.used = IDLE_Rm, .changed = IDLE_Rn}, // MOV.B @Rm,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rn}, // MOV.W @Rm,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rn}, // MOV.L @Rm,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rn}, // MOV Rm,Rn

    {.used = IDLE_Rm, .changed = IDLE_Rm|IDLE_Rn}, // MOV.B @Rm+,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rm|IDLE_Rn}, // MOV.W @Rm+,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rm|IDLE_Rn}, // MOV.L @Rm+,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rn}, // NOT Rm,Rn

    {.used = IDLE_Rm, .changed = IDLE_Rn}, // SWAP.B Rm,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rn}, // SWAP.W Rm,Rn
    {.used = IDLE_Rm|IDLE_SR_T, .changed = IDLE_Rn|IDLE_SR_T}, // NEGC Rm,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rn}, // NEG Rm,Rn

    {.used = IDLE_Rm, .changed = IDLE_Rn}, // EXTU.B Rm,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rn}, // EXTU.W Rm,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rn}, // EXTS.B Rm,Rn
    {.used = IDLE_Rm, .changed = IDLE_Rn}, // EXTS.W Rm,Rn
};

/*----------------------------------*/

/* Opcode table for $8xxx opcodes */
static const IdleInfo idle_table_8xxx[16] = {
    {.changed = IDLE_BAD},              // invalid
    {.changed = IDLE_BAD},              // invalid
    {.next_shift = 4, .subtable = idle_table_0xx2},
    {.next_shift = 4, .subtable = idle_table_0xx3},

    {.changed = IDLE_BAD},              // MOV.B Rm,@(R0,Rn)
    {.changed = IDLE_BAD},              // MOV.W Rm,@(R0,Rn)
    {.changed = IDLE_BAD},              // MOV.L Rm,@(R0,Rn)
    {.used = IDLE_Rm|IDLE_Rn, .changed = IDLE_MACL}, // MUL.L Rm,Rn

    {.used = IDLE_R(0), .changed = IDLE_SR_T}, // CMP/EQ #imm,R0
    {.used = IDLE_SR_T, .changed = 0},  // BT label
    {.changed = IDLE_BAD},              // invalid
    {.used = IDLE_SR_T, .changed = 0},  // BF label

    {.changed = IDLE_BAD},              // invalid
    {.used = IDLE_SR_T, .changed = 0},  // BT/S label
    {.changed = IDLE_BAD},              // invalid
    {.used = IDLE_SR_T, .changed = 0},  // BF/S label
};

/*----------------------------------*/

/* Opcode table for $Cxxx opcodes */
static const IdleInfo idle_table_Cxxx[16] = {
    {.changed = IDLE_BAD},              // MOV.B R0,@(disp,GBR)
    {.changed = IDLE_BAD},              // MOV.W R0,@(disp,GBR)
    {.changed = IDLE_BAD},              // MOV.L R0,@(disp,GBR)
    {.changed = IDLE_BAD},              // TRAPA #imm

    {.used = IDLE_GBR, .changed = IDLE_R(0)}, // MOV.B @(disp,GBR),R0
    {.used = IDLE_GBR, .changed = IDLE_R(0)}, // MOV.W @(disp,GBR),R0
    {.used = IDLE_GBR, .changed = IDLE_R(0)}, // MOV.L @(disp,GBR),R0
    {.used = 0, .changed = IDLE_R(0)},    // MOVA @(disp,PC),R0

    {.used = IDLE_R(0), .changed = IDLE_SR_T}, // TST #imm,R0
    {.used = IDLE_R(0), .changed = IDLE_R(0)}, // AND #imm,R0
    {.used = IDLE_R(0), .changed = IDLE_R(0)}, // XOR #imm,R0
    {.used = IDLE_R(0), .changed = IDLE_R(0)}, // OR #imm,R0

    {.used = IDLE_R(0)|IDLE_GBR, .changed = IDLE_SR_T}, // TST #imm,@(R0,GBR)
    {.changed = IDLE_BAD},              // AND #imm,@(R0,GBR)
    {.changed = IDLE_BAD},              // XOR #imm,@(R0,GBR)
    {.changed = IDLE_BAD},              // OR #imm,@(R0,GBR)
};

/*----------------------------------*/

/* Main opcode table (for bits 12-15) */
static const IdleInfo idle_table_main[16] = {
    {.next_shift = 0, .subtable = idle_table_0xxx},
    {.changed = IDLE_BAD},  // MOV.L Rm,@(disp,Rn)
    {.next_shift = 0, .subtable = idle_table_2xxx},
    {.next_shift = 0, .subtable = idle_table_3xxx},

    {.next_shift = 0, .subtable = idle_table_4xxx},
    {.used = IDLE_Rm, .changed = IDLE_Rn},  // MOV.L @(disp,Rn),Rm
    {.next_shift = 0, .subtable = idle_table_6xxx},
    {.changed = IDLE_BAD},              // ADD #imm,Rn

    {.next_shift = 8, .subtable = idle_table_8xxx},
    {.used = 0, .changed = IDLE_Rn},    // MOV.W @(disp,PC),Rn
    {.used = 0, .changed = 0},          // BRA label
    {.changed = IDLE_BAD},              // BSR label

    {.next_shift = 8, .subtable = idle_table_Cxxx},
    {.used = 0, .changed = IDLE_Rn},    // MOV.L @(disp,PC),Rn
    {.used = 0, .changed = IDLE_Rn},    // MOV #imm,Rn
    {.changed = IDLE_BAD},              // invalid
};

/*-----------------------------------------------------------------------*/

/**
 * can_optimize_idle:  Return whether the given sequence of instructions
 * forms an "idle loop", in which the processor remains in a constant state
 * (or repeating sequence of states) indefinitely until a certain external
 * event occurs (such as a change in the value of a memory-mapped register).
 * If an idle loop is detected, also return information allowing the loop
 * to be translated into a faster sequence of native instructions.
 *
 * The sequence of instructions is assumed to end with a branch instruction
 * to the beginning of the sequence (possibly including a delay slot).
 *
 * [Parameters]
 *     insn_ptr: Pointer to first instruction
 *           PC: PC of first instruction
 *        count: Number of instructions to check
 * [Return value]
 *     Nonzero if the given sequence of SH-2 instructions form an idle
 *     loop, else zero
 */
static int can_optimize_idle(const uint16_t *insn_ptr, uint32_t PC,
                             unsigned int count)
{
    uint32_t used_regs;  // Which registers have been used so far?
    uint32_t dont_care;  // Which registers are "don't cares"?
    int can_optimize = 1;
    unsigned int i;

    /* We detect failure by matching the changed-registers bitmask against
     * used_regs; set the initial value so that non-idleable instructions
     * are automatically rejected */
    used_regs = IDLE_BAD;
    /* Initially, no registers are don't cares */
    dont_care = 0;

    for (i = 0; can_optimize && i < count; i++) {
        const uint16_t opcode = *insn_ptr++;

        /* Find the entry for this opcode */
        int shift = 12;
        const IdleInfo *table = idle_table_main;
        while (table[opcode>>shift & 0xF].subtable) {
            int newshift = table[opcode>>shift & 0xF].next_shift;
            table = table[opcode>>shift & 0xF].subtable;
            shift = newshift;
        }

        /* Replace virtual Rn/Rm bits with real registers from opcode */
        uint32_t used = table[opcode>>shift & 0xF].used;
        uint32_t changed = table[opcode>>shift & 0xF].changed;
        if (used & IDLE_Rn) {
            used &= ~IDLE_Rn;
            used |= IDLE_R(opcode>>8 & 0xF);
        }
        if (used & IDLE_Rm) {
            used &= ~IDLE_Rm;
            used |= IDLE_R(opcode>>4 & 0xF);
        }
        if (changed & IDLE_Rn) {
            changed &= ~IDLE_Rn;
            changed |= IDLE_R(opcode>>8 & 0xF);
        }
        if (changed & IDLE_Rm) {
            changed &= ~IDLE_Rm;
            changed |= IDLE_R(opcode>>4 & 0xF);
        }

        /* Add any registers used by this instruction to used_regs */
        used_regs |= used;

        /* Add any not-yet-used registers modified by this instruction to
         * dont_care */
        dont_care |= changed & ~used_regs;

        /* See whether we can still treat this as an idle loop */
        if (changed & ~dont_care & used_regs) {
            can_optimize = 0;
        }
    }

    return can_optimize;
}

#endif  // OPTIMIZE_IDLE

/*************************************************************************/

#ifdef OPTIMIZE_DIVISION

/**
 * can_optimize_div0u:  Return whether a sequence of instructions starting
 * from a DIV0U instruction can be optimized to a native divide operation.
 *
 * On the SH-2, unsigned 64bit/32bit division follows the sequence:
 *     DIV0U
 *     .arepeat 32  ;Repeat 32 times
 *         ROTCL Rlo
 *         DIV1 Rdiv,Rhi
 *     .aendr
 * and finishes with the following state changes:
 *     M   = 0
 *     T   = low bit of quotient
 *     Q   = !T
 *     Rlo = high 31 bits of quotient in bits 0-30, with 0 in bit 31
 *     Rhi = !Q ? remainder : (remainder - Rdiv)
 *
 * [Parameters]
 *     insn_ptr: Pointer to instruction following DIV0U instruction
 *           PC: PC of instruction following DIV0U instruction
 *      Rhi_ret: Pointer to variable to receive index of dividend high register
 *      Rlo_ret: Pointer to variable to receive index of dividend low register
 *     Rdiv_ret: Pointer to variable to receive index of divisor register
 * [Return value]
 *     Nonzero if the next 64 SH-2 instructions form a 32-bit division
 *     operation, else zero
 */
static int can_optimize_div0u(const uint16_t *insn_ptr, uint32_t PC,
                              int *Rhi_ret, int *Rlo_ret, int *Rdiv_ret)
{
    uint16_t rotcl_op = 0, div1_op = 0;  // Expected opcodes
    int can_optimize = 1;
    unsigned int i;

    for (i = 0; can_optimize && i < 64; i++) {
        const uint16_t nextop = *insn_ptr++;

        if (i%2 == 0) {  // ROTCL Rlo

            if (i == 0) {
                if ((nextop & 0xF0FF) == 0x4024) {
                    *Rlo_ret = nextop>>8 & 0xF;
                    rotcl_op = nextop;
                } else {
                    DMSG("DIV0U optimization failed: PC+2 (0x%08X) is 0x%04X,"
                         " not ROTCL", PC, nextop);
                    can_optimize = 0;
                }
            } else {
                if (UNLIKELY(nextop != rotcl_op)) {
                    DMSG("DIV0U optimization failed: PC+%d (0x%08X) is 0x%04X,"
                         " not ROTCL R%d", 2 + 2*i, PC + 2*i, nextop,
                         *Rlo_ret);
                    can_optimize = 0;
                }
            }

        } else {  // DIV1 Rdiv,Rhi

            if (i == 1) {
                if ((nextop & 0xF00F) == 0x3004) {
                    *Rhi_ret  = nextop>>8 & 0xF;
                    *Rdiv_ret = nextop>>4 & 0xF;
                    div1_op = nextop;
                } else {
                    DMSG("DIV0U optimization failed: PC+4 (0x%08X) is 0x%04X,"
                         " not DIV1", PC + 2, nextop);
                    can_optimize = 0;
                }
            } else {
                if (UNLIKELY(nextop != div1_op)) {
                    DMSG("DIV0U optimization failed: PC+%d (0x%08X) is 0x%04X,"
                         " not DIV1 R%d,R%d", 2 + 2*i, PC + 2*i, nextop,
                         *Rdiv_ret, *Rhi_ret);
                    can_optimize = 0;
                }
            }

        }
    }  // for 64 instructions

    return can_optimize;
}

/*-----------------------------------------------------------------------*/

/**
 * can_optimize_div0s:  Return whether a sequence of instructions starting
 * from a DIV0S instruction can be optimized to a native divide operation.
 *
 * On the SH-2, signed 64bit/32bit division follows a sequence identical
 * to unsigned division except for the first instruction:
 *     DIV0S Rdiv,Rhi
 *     .arepeat 32  ;Repeat 32 times
 *         ROTCL Rlo
 *         DIV1 Rdiv,Rhi
 *     .aendr
 * and finishes with the following state changes:
 *     M   = sign bit of divisor
 * (if dividend >= 0)
 *     T   = low bit of (quotient - M)
 *     Q   = !(T ^ M)
 *     Rlo = high 31 bits of (quotient - M) in bits 0-30,
 *              sign-extended to 32 bits
 *     Rhi = !Q ? remainder : (remainder - abs(Rdiv))
 * (if dividend < 0)
 *     qtemp = quotient + (remainder ? (M ? 1 : -1) : 0)
 *     T   = low bit of (qtemp - M)
 *     Q   = !(T ^ M)
 *     Rlo = high 31 bits of (qtemp - M) in bits 0-30,
 *              sign-extended to 32 bits
 *     Rhi = !Q ? (remainder == 0 ? remainder : remainder + abs(Rdiv))
 *              : (remainder != 0 ? remainder : remainder - abs(Rdiv))
 *
 * [Parameters]
 *     insn_ptr: Pointer to instruction following DIV0S instruction
 *           PC: PC of instruction following DIV0S instruction
 *          Rhi: Index of dividend high register
 *      Rlo_ret: Pointer to variable to receive index of dividend low register
 *         Rdiv: Index of divisor register
 * [Return value]
 *     Nonzero if the next 64 SH-2 instructions form a 32-bit division
 *     operation, else zero
 */
static int can_optimize_div0s(const uint16_t *insn_ptr, uint32_t PC,
                              int Rhi, int *Rlo_ret, int Rdiv)
{
    uint16_t rotcl_op = 0, div1_op = 0x3004 | Rhi<<8 | Rdiv<<4;
    int can_optimize = 1;
    unsigned int i;

    for (i = 0; can_optimize && i < 64; i++) {
        uint16_t nextop = *insn_ptr++;

        if (i%2 == 0) {  // ROTCL Rlo

            if (i == 0) {
                if ((nextop & 0xF0FF) == 0x4024) {
                    *Rlo_ret = nextop>>8 & 0xF;
                    rotcl_op = nextop;
                } else {
                    /* Don't complain for the first instruction, since
                     * DIV0S seems to be put to uses other than division
                     * as well */
                    can_optimize = 0;
                }
            } else {
                if (UNLIKELY(nextop != rotcl_op)) {
                    DMSG("DIV0S optimization failed: PC+%d (0x%08X) is 0x%04X,"
                         " not ROTCL R%d", 2 + 2*i, PC + 2*i, nextop,
                         *Rlo_ret);
                    can_optimize = 0;
                }
            }

        } else {  // DIV1 Rdiv,Rhi

            if (UNLIKELY(nextop != div1_op)) {
                DMSG("DIV0S optimization failed: PC+%d (0x%08X) is 0x%04X,"
                     " not DIV1 R%d,R%d", 2 + 2*i, PC + 2*i, nextop, Rdiv, Rhi);
                can_optimize = 0;
            }

        }
    }  // for 64 instructions

    return can_optimize;
}

#endif  // OPTIMIZE_DIVISION

/*************************************************************************/
/************************ Other utility routines *************************/
/*************************************************************************/

#ifdef ENABLE_JIT

/*************************************************************************/

/**
 * append_insn:  Append a single MIPS instruction to the given JIT entry,
 * expanding the native code buffer if necessary.
 *
 * [Parameters]
 *      entry: JitEntry structure pointer
 *     opcode: Opcode of instruction to add
 * [Return value]
 *     Nonzero on success, zero on failure (out of memory)
 */
static int append_insn(JitEntry *entry, uint32_t opcode)
{
    if (UNLIKELY(entry->native_length + 4 > entry->native_size)) {
        if (UNLIKELY(!expand_buffer(entry))) {
            return 0;
        }
    }
    *((uint32_t *)((uint8_t *)entry->native_code + entry->native_length))
        = opcode;
    entry->native_length += 4;
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * append_block:  Append a block of MIPS instructions to the given JIT
 * entry, expanding the native code buffer if necessary.
 *
 * [Parameters]
 *      entry: JitEntry structure pointer
 *      block: Pointer to MIPS code to add
 *     length: Length of MIPS code to add (bytes)
 * [Return value]
 *     Nonzero on success, zero on failure (out of memory)
 */
static int append_block(JitEntry *entry, const void *block, int length)
{
    if (UNLIKELY(entry->native_length + length > entry->native_size)) {
        if (UNLIKELY(!expand_buffer(entry))) {
            return 0;
        }
    }
    memcpy((uint8_t *)entry->native_code + entry->native_length, block, length);
    entry->native_length += length;
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * append_prologue:  Append the MIPS prologue code to the given JIT entry,
 * expanding the native code buffer if necessary.  Helper function for
 * jit_translate().
 *
 * [Parameters]
 *     entry: JitEntry structure pointer
 * [Return value]
 *     Nonzero on success, zero on failure (out of memory)
 */
static int append_prologue(JitEntry *entry)
{
    return append_block(entry, mips_prologue, sizeof(mips_prologue));
}

/*-----------------------------------------------------------------------*/

/**
 * append_epilogue:  Append the MIPS epilogue code to the given JIT entry,
 * expanding the native code buffer if necessary.  Helper function for
 * jit_translate().
 *
 * [Parameters]
 *     entry: JitEntry structure pointer
 * [Return value]
 *     Nonzero on success, zero on failure (out of memory)
 */
static int append_epilogue(JitEntry *entry)
{
    if (jit_cycles > 0) {
        uint32_t i0 = MIPS_LW(MIPS_v0, offsetof(SH2_struct,cycles), MIPS_s8);
        uint32_t i1 = MIPS_ADDI(MIPS_v0, MIPS_v0, jit_cycles);
        uint32_t i2 = MIPS_SW(MIPS_v0, offsetof(SH2_struct,cycles), MIPS_s8);
        if (!append_insn(entry, i0)
         || !append_insn(entry, i1)
         || !append_insn(entry, i2)
        ) {
            return 0;
        }
        jit_cycles = 0;
    }
    uint32_t i3 = MIPS_J(((uint32_t)mips_terminate & 0x0FFFFFFC) >> 2);
    uint32_t i4 = MIPS_ADDIU(MIPS_v0, MIPS_zero, 0);
    if (!append_insn(entry, i3) || !append_insn(entry, i4)) {
        return 0;
    }
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * expand_buffer:  Expands the native code buffer in the given JIT entry by
 * JIT_BLOCK_EXPAND_SIZE.
 *
 * [Parameters]
 *     entry: JitEntry structure pointer
 * [Return value]
 *     Nonzero on success, zero on failure (out of memory)
 */
static int expand_buffer(JitEntry *entry)
{
    const uint32_t newsize = entry->native_size + JIT_BLOCK_EXPAND_SIZE;
    void *newptr = realloc(entry->native_code, newsize);
    if (!newptr) {
        DMSG("out of memory");
        return 0;
    }
    entry->native_code = newptr;
    entry->native_size = newsize;
    return 1;
}

/*************************************************************************/

/**
 * clear_entry:  Clear a specific entry from the JIT table, freeing the
 * native code buffer and unlinking the entry from its references.
 *
 * [Parameters]
 *     entry: JitEntry structure pointer
 * [Return value]
 *     None
 */
static void clear_entry(JitEntry *entry)
{
    /* Clear the entry out of the call stack first */
    int i;
    for (i = 0; i < JIT_CALL_STACK_SIZE; i++) {
        CallStackEntry *call_stack;
        call_stack = (CallStackEntry *)MSH2->psp.call_stack;
        if (call_stack[i].return_entry == entry) {
            call_stack[i].return_PC = 0;
            call_stack[i].return_entry = NULL;
        }
        call_stack = (CallStackEntry *)SSH2->psp.call_stack;
        if (call_stack[i].return_entry == entry) {
            call_stack[i].return_PC = 0;
            call_stack[i].return_entry = NULL;
        }
    }

    /* Make sure the non-active processor isn't trying to use this entry
     * (if so, clear it out) */
    if (UNLIKELY(MSH2->psp.current_entry == entry)) {
        MSH2->psp.current_entry = NULL;
    }
    if (UNLIKELY(SSH2->psp.current_entry == entry)) {
        SSH2->psp.current_entry = NULL;
    }

    /* Free the native code */
    jit_total_data -= entry->native_size;
    free(entry->native_code);
    entry->native_code = NULL;

    /* Clear the entry from the table and hash chain */
    if (entry->next) {
        entry->next->prev = entry->prev;
    }
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        jit_hashchain[JIT_HASH(entry->sh2_start)] = entry->next;
    }

    /* Mark the entry as free */
    entry->sh2_start = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * clear_oldest_entry:  Clear the oldest entry from the JIT table.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void clear_oldest_entry(void)
{
    int oldest = -1;
    int index;
    for (index = 0; index < JIT_TABLE_SIZE; index++) {
        if (jit_table[index].sh2_start == 0) {
            continue;
        }
        if (oldest < 0
         || timestamp_compare(jit_timestamp,
                              jit_table[index].timestamp,
                              jit_table[oldest].timestamp) < 0) {
            oldest = index;
        }
    }
    if (LIKELY(oldest >= 0)) {
        clear_entry(&jit_table[oldest]);
    } else {
        DMSG("Tried to clear oldest entry from an empty table!");
        /* Set the total size to zero, just in case something weird happened */
        jit_total_data = 0;
    }
}

/*************************************************************************/

/**
 * ref_local_label:  Record a forward branch reference at the current
 * native offset to a local label to be defined later.
 *
 * [Parameters]
 *     entry: Block being translated
 *        id: Label ID
 * [Return value]
 *     Nonzero on success, zero on error
 */
static int ref_local_label(JitEntry *entry, uint32_t id)
{
    int i;
    for (i = 0; i < lenof(local_labels); i++) {
        if (!local_labels[i].in_use) {
            local_labels[i].in_use = 1;
            local_labels[i].id = id;
            local_labels[i].native_offset = entry->native_length;
            local_labels[i].sh2_address = jit_PC;
            return 1;
        }
    }
    DMSG("No room for local label 0x%X at SH-2 address 0x%08X, native"
         " offset 0x%X", id, jit_PC, entry->native_length);
    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * define_local_label:  Define a local label at the current native offset,
 * and resolve all previous forward references to the label.
 *
 * [Parameters]
 *     entry: Block being translated
 *        id: Label ID
 * [Return value]
 *     None
 */
static void define_local_label(JitEntry *entry, uint32_t id)
{
    int i;
    for (i = 0; i < lenof(local_labels); i++) {
        if (local_labels[i].in_use && local_labels[i].id == id) {
            int32_t displacement =
                entry->native_length - (local_labels[i].native_offset + 4);
            uint8_t *native_base = (uint8_t *)entry->native_code;
            int16_t *ptr =
                (int16_t *)(native_base + local_labels[i].native_offset);
            *ptr = displacement / 4;
            local_labels[i].in_use = 0;
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * clear_local_labels:  If any unresolved local label references are in the
 * local label table, clear them and return false (indicating that an
 * invalid branch instruction remains in the native code).
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Nonzero if any unresolved local label references remain, else zero
 */
static int clear_local_labels(void)
{
    int found = 0;
    int i;
    for (i = 0; i < lenof(local_labels); i++) {
        if (UNLIKELY(local_labels[i].in_use)) {
            DMSG("Local label 0x%X referenced at SH-2 address 0x%08X,"
                 " native offset 0x%X, but never defined",
                 local_labels[i].id, local_labels[i].sh2_address,
                 local_labels[i].native_offset);
            found = 1;
            local_labels[i].in_use = 0;
        }
    }
    return found;
}

/*************************************************************************/

/**
 * btcache_lookup:  Search the branch target cache for the given SH-2
 * address.
 *
 * [Parameters]
 *        address: SH-2 address to search for
 *     active_ret: Pointer to variable to receive active register mask at
 *                    the given address (cleared to zero on failure)
 * [Return value]
 *     Corresponding byte offset into entry->native_code, or -1 if the
 *     address could not be found
 */
static int32_t btcache_lookup(uint32_t address, uint16_t *active_ret)
{
    /* Search backwards from the current instruction so we can handle short
     * loops quickly; note that btcache_index is now pointing to where the
     * _next_ instruction will go */
    const int current = (btcache_index + (lenof(btcache)-1)) % lenof(btcache);
    int index = current;
    do {
        if (btcache[index].sh2_address == address) {
            *active_ret = btcache[index].active_regs;
            return btcache[index].native_offset;
        }
        index--;
        if (UNLIKELY(index < 0)) {
            index = lenof(btcache) - 1;
        }
    } while (index != current);
    *active_ret = 0;
    return -1;
}

/*-----------------------------------------------------------------------*/

/**
 * record_unresolved_branch:  Record the given branch target and native
 * offset in an empty slot in the unresolved branch table.  If there are
 * no empty slots, purge the oldest (lowest native offset) entry.
 *
 * The "check_offset" parameter records the address of an instruction which
 * should be changed to a branch to "native_offset" if the branch cannot be
 * resolved, or -1 for none.  (This allows the check to be skipped if the
 * code will terminate anyway.)
 *
 * [Parameters]
 *             entry: Block being translated
 *        sh2_target: Branch target address in SH2 address space
 *     native_offset: Offset of branch to update in native code
 *      check_offset: Offset of associated cycle count check in native code
 * [Return value]
 *     None
 */
static void record_unresolved_branch(JitEntry *entry, uint32_t sh2_target,
                                     uint32_t native_offset,
                                     int32_t check_offset)
{
    int oldest = 0;
    int i;
    for (i = 0; i < lenof(unres_branches); i++) {
        if (unres_branches[i].sh2_target == 0) {
            oldest = i;
            break;
        } else if (unres_branches[i].native_offset
                   < unres_branches[oldest].native_offset) {
            oldest = i;
        }
    }
    if (UNLIKELY(unres_branches[oldest].sh2_target != 0)) {
        DMSG("WARNING: Unresolved branch table full, dropping branch from"
             " offset 0x%X to PC 0x%08X",
             unres_branches[oldest].native_offset,
             unres_branches[oldest].sh2_target);
        /* Nullify any cycle check so we don't trace twice */
        if (unres_branches[oldest].check_offset >= 0) {
            uint32_t *address = (uint32_t *)((uint8_t *)entry->native_code
                                    + unres_branches[oldest].check_offset);
            uint32_t disp = unres_branches[oldest].native_offset
                            - (unres_branches[oldest].check_offset + 4);
            address[0] = MIPS_B(disp/4);
            address[1] = MIPS_NOP();
        }
    }
    unres_branches[oldest].sh2_target    = sh2_target;
    unres_branches[oldest].active_regs   = active_regs;
    unres_branches[oldest].native_offset = native_offset;
    unres_branches[oldest].check_offset  = check_offset;
}

/*-----------------------------------------------------------------------*/

/**
 * fixup_branch:  Update the instruction at the given native offset to
 * branch to the given target.
 *
 * [Parameters]
 *      entry: Block being translated
 *     offset: Offset of branch to update in native code
 *     target: Target offset within native code
 * [Return value]
 *     Nonzero on success, zero on error (displacement out of range)
 */
static inline int fixup_branch(JitEntry *entry, uint32_t offset,
                               uint32_t target)
{
    int32_t disp_4 = (target - (offset + 4)) / 4;
    if (disp_4 >= -32768 && disp_4 <= 32767) {
        disp_4 &= 0xFFFF;
        *(uint32_t *)((uint8_t *)entry->native_code + offset) = MIPS_B(disp_4);
        return 1;
    } else {
        DMSG("WARNING: displacement out of range (0x%X -> 0x%X)",
             offset, target);
        return 0;
    }
}

/*************************************************************************/

/**
 * flush_cache:  Flush the native CPU's caches.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static inline void flush_cache(void)
{
#ifdef PSP  // Protect so we can test this on other platforms
    sceKernelDcacheWritebackAll();
    sceKernelIcacheInvalidateAll();
#endif
}

/*************************************************************************/

/**
 * timestamp_compare:  Compare two timestamps.
 *
 * [Parameters]
 *          a, b: Timestamps to compare
 *     reference: Reference timestamp by which the comparison is made
 * [Return value]
 *     -1 if a < b (i.e. "a is older than b")
 *      0 if a == b
 *      1 if a > b
 */
__attribute__((const)) static inline int timestamp_compare(
    uint32_t reference, uint32_t a, uint32_t b)
{
    const uint32_t age_a = reference - a;
    const uint32_t age_b = reference - b;
    return age_a > age_b ? -1 :
           age_a < age_b ?  1 : 0;
}

/*************************************************************************/

#endif  // ENABLE_JIT

/*************************************************************************/
/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
