/*  src/psp/sh2.h: SH-2 emulator header
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

#ifndef SH2_H
#define SH2_H

#ifndef SH2CORE_H
# include "../sh2core.h"
#endif

/*************************************************************************/

/**** Optimization flags used with sh2_{set,get}_optimizations() ****/

/**
 * SH2_OPTIMIZE_ASSUME_SAFE_DIVISION:  When defined, optimizes division
 * operations by assuming that quotients in division operations will not
 * overflow and that division by zero will not occur.
 *
 * This optimization will cause the translated code to behave incorrectly
 * if this assumption is violated.
 */
#define SH2_OPTIMIZE_ASSUME_SAFE_DIVISION  (1<<0)

/**
 * SH2_OPTIMIZE_BRANCH_TO_RTS:  When defined, optimizes static branch
 * instructions (BRA, BT, BF, BT/S, BF/S) which target a "RTS; NOP" pair by
 * performing the RTS operation at the same time as the branch.
 *
 * This optimization may cause the translated code to behave incorrectly
 * or crash the host program if the memory word containing the targeted RTS
 * is modified independently of the branch which targets it.
 *
 * This optimization can cause the timing of cycle count checks to change,
 * and therefore alters trace output in all trace modes.
 */
#define SH2_OPTIMIZE_BRANCH_TO_RTS  (1<<1)

/**
 * SH2_OPTIMIZE_LOCAL_ACCESSES:  When enabled, checks for instructions
 * which modify memory via PC-relative addresses, and bypasses the normal
 * check for overwriting translated blocks on the assumption that such
 * stores will not overwrite program code.
 *
 * This optimization will cause the translated code to behave incorrectly
 * if the above assumption is violated.
 */
#define SH2_OPTIMIZE_LOCAL_ACCESSES  (1<<2)

/**
 * SH2_OPTIMIZE_LOCAL_POINTERS:  When enabled, assumes that pointers
 * loaded via PC-relative addresses will always point to the same region
 * of memory, and bypasses the normal translation from SH-2 addresses to
 * native addresses.
 *
 * This optimization will cause the translated code to behave incorrectly
 * or crash the host program if the above assumption is violated.
 *
 * This optimization depends on both SH2_OPTIMIZE_LOCAL_ACCESSES and
 * SH2_OPTIMIZE_POINTERS, and has no effect if either optimization is
 * disabled.
 */
#define SH2_OPTIMIZE_LOCAL_POINTERS  (1<<3)

/**
 * SH2_OPTIMIZE_MAC_NOSAT:  When enabled, checks the S flag of the SR
 * register when translating any block containing a MAC.W or MAC.L
 * instruction; if the S flag is clear at the beginning of the block and is
 * not modified within the block, the saturation checks are omitted
 * entirely from the generated code.  (In such blocks, a check is added to
 * the beginning of the block to ensure that S is still clear at runtime,
 * and the block is retranslated if S is set.)
 *
 * This optimization will cause the translated code to behave incorrectly
 * if a subroutine or exception called while the block is executing sets
 * the S flag and does not restore it on return.
 */
#define SH2_OPTIMIZE_MAC_NOSAT  (1<<4)

/**
 * SH2_OPTIMIZE_POINTERS:  When enabled, attempts to optimize accesses
 * through pointers to RAM by making the following assumptions:
 *
 * - Any general-purpose register (R0-R15) which points to internal RAM at
 *   the start of a block of code, and which is actually used to access
 *   memory, will continue pointing to a valid RAM address unless and until
 *   the register is the target of a subsequent instruction other than the
 *   immediate-operand forms of ADD, XOR, or OR (since XOR and OR cannot,
 *   and ADD normally will not, affect the high bits of the register's
 *   value).
 *
 * - Accesses through such a pointer register will not run over the end of
 *   the memory region to which the pointer points.  (For example, a store
 *   to @(4,R0) when R0 = 0x060FFFFC would violate this assumption.)
 *
 * This optimization will cause the translated code to behave incorrectly
 * or crash the host program if either of the assumptions above is violated.
 */
#define SH2_OPTIMIZE_POINTERS  (1<<5)

/**
 * SH2_OPTIMIZE_POINTERS_MAC:  When enabled, terminates a translation block
 * before a MAC instruction when either or both of the operands is not a
 * known pointer, or before a CLRMAC instruction followed by such a MAC
 * instruction.
 *
 * This optimization can cause the timing of cycle count checks to change,
 * and therefore alters trace output in all trace modes.
 *
 * This flag is ignored if SH2_OPTIMIZE_POINTERS is not enabled.
 */
#define SH2_OPTIMIZE_POINTERS_MAC  (1<<6)

/**
 * SH2_OPTIMIZE_STACK:  When defined, optimizes accesses to the stack by
 * making the following assumptions:
 *
 * - The R15 register will always point to a directly-accessible region of
 *   memory.
 *
 * - The R15 register will always be 32-bit aligned.
 *
 * - Accesses through R15 will always access the same region of memory
 *   pointed to by R15 itself, regardless of any offset in the accessing
 *   instruction.
 *
 * - Writes through R15 will never overwrite SH-2 program code.
 *
 * This optimization will cause the translated code to behave incorrectly
 * or crash the host program if either of the assumptions above is violated.
 */
#define SH2_OPTIMIZE_STACK  (1<<7)

/*-----------------------------------------------------------------------*/

/**
 * SH2InvalidOpcodeCallback:  Type of an invalid opcode callback function,
 * as passed to sh2_set_invalid_opcode_callback().
 *
 * [Parameters]
 *      state: Processor state block pointer
 *         PC: PC at which the invalid instruction was found
 *     opcode: The invalid opcode itself
 * [Return value]
 *     None
 */
typedef void SH2InvalidOpcodeCallback(SH2_struct *state, uint32_t PC,
                                      uint16_t opcode);

/*************************************************************************/

/**
 * sh2_init:  Initialize the SH-2 core.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Nonzero on success, zero on error
 */
extern int sh2_init(void);

/**
 * sh2_cleanup:  Perform cleanup for the SH-2 core.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
extern void sh2_cleanup(void);

/**
 * sh2_set_optimizations, sh2_get_optimizations:  Set or retrieve the
 * bitmask of currently-enabled optional optimizations, as defined by the
 * SH2_OPTIMIZE_* constants.
 *
 * [Parameters]
 *     flags: Bitmask of optimizations to enable (sh2_set_optimizations() only)
 * [Return value]
 *     Bitmask of enabled optimizations (sh2_get_optimizations() only)
 */
extern void sh2_set_optimizations(uint32_t flags);
extern uint32_t sh2_get_optimizations(void);

/**
 * sh2_set_jit_data_limit:  Set the limit on the total size of translated
 * code, in bytes of native code (or bytes of RTL data when using the RTL
 * interpreter).  Does nothing if dynamic translation is disablde.
 *
 * [Parameters]
 *     limit: Total JIT data size limit
 * [Return value]
 *     None
 */
extern void sh2_set_jit_data_limit(uint32_t limit);

/**
 * sh2_set_invalid_opcode_callback:  Set a callback function to be called
 * when the SH-2 decoder encounters an invalid instruction.
 *
 * [Parameters]
 *     funcptr: Callback function pointer (NULL to unset a previously-set
 *                 function)
 * [Return value]
 *     None
 */
extern void sh2_set_invalid_opcode_callback(SH2InvalidOpcodeCallback *funcptr);

/**
 * sh2_alloc_direct_write_buffer:  Allocate a buffer which can be passed
 * as the write_buffer parameter to sh2_set_direct_access().  The buffer
 * should be freed with free() when no longer needed, but no earlier than
 * calling sh2_cleanup().
 *
 * [Parameters]
 *     size: Size of SH-2 address region to be marked as directly accessible
 * [Return value]
 *     Buffer pointer, or NULL on error
 */
extern void *sh2_alloc_direct_write_buffer(uint32_t size);

/**
 * sh2_set_direct_access:  Mark a region of memory as being directly
 * accessible.  The data format of the memory region is assumed to be
 * sequential 16-bit words in native order.
 *
 * As memory is internally divided into 512k (2^19) byte pages, both
 * sh2_address and size must be aligned to a multiple of 2^19.
 *
 * Passing a NULL parameter for native_address causes the region to be
 * marked as not directly accessible.
 *
 * [Parameters]
 *        sh2_address: Base address of region in SH-2 address space
 *     native_address: Pointer to memory block in native address space
 *               size: Size of memory block, in bytes
 *           readable: Nonzero if region is readable, zero if execute-only
 *       write_buffer: Buffer for checking writes to this region (allocated
 *                        with sh2_alloc_direct_write_buffer()), or NULL to
 *                        indicate that the region is read- or execute-only
 * [Return value]
 *     None
 */
extern void sh2_set_direct_access(uint32_t sh2_address, void *native_address,
                                  uint32_t size, int readable, void *write_buffer);

/**
 * sh2_set_byte_direct_access:  Mark a region of memory as being directly
 * accessible in 8-bit units.
 *
 * As memory is internally divided into 512k (2^19) byte pages, both
 * sh2_address and size must be aligned to a multiple of 2^19.
 *
 * Passing a NULL parameter for native_address causes the region to be
 * marked as not directly accessible.
 *
 * [Parameters]
 *        sh2_address: Base address of region in SH-2 address space
 *     native_address: Pointer to memory block in native address space
 *               size: Size of memory block, in bytes
 * [Return value]
 *     None
 */
extern void sh2_set_byte_direct_access(uint32_t sh2_address, void *native_address,
                                       uint32_t size);

/**
 * sh2_reset:  Reset the SH-2 core.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
extern void sh2_reset(void);

/**
 * sh2_run:  Execute instructions for the given number of clock cycles.
 *
 * [Parameters]
 *      state: SH-2 processor state block
 *     cycles: Number of clock cycles to execute
 * [Return value]
 *     None
 */
extern void sh2_run(SH2_struct *state, uint32_t cycles);

/**
 * sh2_write_notify:  Called when an external agent modifies memory.
 * Used here to clear any JIT translations in the modified range.
 *
 * [Parameters]
 *     address: Beginning of address range to which data was written
 *        size: Size of address range to which data was written (in bytes)
 * [Return value]
 *     None
 */
extern void sh2_write_notify(uint32_t address, uint32_t size);

/*************************************************************************/

#endif  // SH2_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
