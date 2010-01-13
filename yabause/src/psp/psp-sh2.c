/*  src/psp/psp-sh2.c: Yabause interface for PSP SH-2 emulator
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

#include "common.h"

#include "../bios.h"
#include "../error.h"
#include "../memory.h"
#include "../sh2core.h"
#include "../vdp1.h"
#include "../vdp2.h"

#include "config.h"
#include "psp-sh2.h"
#include "sh2.h"

/*************************************************************************/
/************************* Interface definition **************************/
/*************************************************************************/

/* Interface function declarations (must come before interface definition) */

static int psp_sh2_init(void);
static void psp_sh2_deinit(void);
static void psp_sh2_reset(void);
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
/************************ Local data definitions *************************/
/*************************************************************************/

/* Write buffers for low and high RAM */
static void *write_buffer_lram, *write_buffer_hram;

/*-----------------------------------------------------------------------*/

/* Local function declarations */
static void invalid_opcode_handler(SH2_struct *state, uint32_t PC,
                                   uint16_t opcode);

/*************************************************************************/
/********************** External interface routines **********************/
/*************************************************************************/

/**
 * psp_sh2_init:  Initialize the SH-2 core.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, negative on error
 */
static int psp_sh2_init(void)
{
    write_buffer_lram = sh2_alloc_direct_write_buffer(0x100000);
    write_buffer_hram = sh2_alloc_direct_write_buffer(0x100000);
    if (UNLIKELY(!write_buffer_lram) || UNLIKELY(!write_buffer_hram)) {
        DMSG("WARNING: Failed to allocate RAM write buffers, performance"
             " will suffer");
    }

    if (UNLIKELY(!sh2_init())) {
        return -1;
    }
#ifdef PSP
    sh2_set_optimizations(config_get_sh2_optimizations());
    /* If we can allocate >24MB of memory, we must be on a PSP-2000 (Slim)
     * or newer model; otherwise, assume we're on a PSP-1000 (Phat) and
     * reduce the JIT data size limit to avoid crowding out other data. */
    void *memsize_test = malloc(24*1024*1024 + 1);
    if (memsize_test) {
        free(memsize_test);
        sh2_set_jit_data_limit(20*1000000);
    } else {
        sh2_set_jit_data_limit(8*1000000);
    }
#else  // !PSP
    sh2_set_optimizations(SH2_OPTIMIZE_ASSUME_SAFE_DIVISION
                        | SH2_OPTIMIZE_BRANCH_TO_RTS
                        | SH2_OPTIMIZE_LOCAL_ACCESSES
                        | SH2_OPTIMIZE_LOCAL_POINTERS
                        | SH2_OPTIMIZE_MAC_NOSAT
                        | SH2_OPTIMIZE_POINTERS
                        | SH2_OPTIMIZE_POINTERS_MAC
                        | SH2_OPTIMIZE_STACK);
    /* Give the SH-2 core some breathing room for saving RTL blocks */
    sh2_set_jit_data_limit(200*1000000);
#endif
    sh2_set_invalid_opcode_callback(invalid_opcode_handler);

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
    sh2_cleanup();

    free(write_buffer_lram);
    free(write_buffer_hram);
    write_buffer_lram = write_buffer_hram = NULL;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_sh2_reset:  Reset the SH-2 core.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_sh2_reset(void)
{
    /* Sanity checks on pointers -- none of these are actually possible
     * on the PSP, but it couldn't hurt to have them for testing this code
     * on other platforms. */
    if ((uintptr_t)BiosRom == 0x00000000UL
     || (uintptr_t)BiosRom == 0x00080000UL
     || (uintptr_t)BiosRom == 0x20000000UL
     || (uintptr_t)BiosRom == 0x20080000UL
     || (uintptr_t)BiosRom == 0xA0000000UL
     || (uintptr_t)BiosRom == 0xA0080000UL
     || (uintptr_t)LowWram == 0x00200000UL
     || (uintptr_t)LowWram == 0x20200000UL
     || (uintptr_t)LowWram == 0xA0200000UL
     || ((uintptr_t)HighWram & 0xFE0FFFFF) == 0x06000000UL
     || ((uintptr_t)HighWram & 0xFE0FFFFF) == 0x26000000UL
     || ((uintptr_t)HighWram & 0xFE0FFFFF) == 0xA6000000UL
    ) {
        DMSG("WARNING: ROM/RAM located at an inconvenient place;"
             " performance will suffer!\nROM=%p LRAM=%p HRAM=%p",
             BiosRom, LowWram, HighWram);
    }

#define SET_PAGE(sh2_addr,psp_addr,size,writebuf)  do {                 \
    if (!(psp_addr)) {                                                  \
        DMSG("WARNING: %s == NULL", #psp_addr);                         \
    } else {                                                            \
        sh2_set_direct_access((sh2_addr) | 0x00000000, (psp_addr),      \
                              (size), 1, (writebuf));                   \
        sh2_set_direct_access((sh2_addr) | 0x20000000, (psp_addr),      \
                              (size), 1, (writebuf));                   \
        sh2_set_direct_access((sh2_addr) | 0xA0000000, (psp_addr),      \
                              (size), 1, (writebuf));                   \
    }                                                                   \
} while (0)
#define SET_EXEC_PAGE(sh2_addr,psp_addr,size)  do {                     \
    if (!(psp_addr)) {                                                  \
        DMSG("WARNING: %s == NULL", #psp_addr);                         \
    } else {                                                            \
        sh2_set_direct_access((sh2_addr) | 0x00000000, (psp_addr),      \
                              (size), 0, 0);                            \
        sh2_set_direct_access((sh2_addr) | 0x20000000, (psp_addr),      \
                              (size), 0, 0);                            \
        sh2_set_direct_access((sh2_addr) | 0xA0000000, (psp_addr),      \
                              (size), 0, 0);                            \
    }                                                                   \
} while (0)
#define SET_BYTE_PAGE(sh2_addr,psp_addr,size)  do {                     \
    if (!(psp_addr)) {                                                  \
        DMSG("WARNING: %s == NULL", #psp_addr);                         \
    } else {                                                            \
        sh2_set_byte_direct_access((sh2_addr) | 0x00000000, (psp_addr), \
                                   (size));                             \
        sh2_set_byte_direct_access((sh2_addr) | 0x20000000, (psp_addr), \
                                   (size));                             \
        sh2_set_byte_direct_access((sh2_addr) | 0xA0000000, (psp_addr), \
                                   (size));                             \
    }                                                                   \
} while (0)

    SET_EXEC_PAGE(0x00000000, BiosRom, 0x80000);
    SET_EXEC_PAGE(0x00080000, BiosRom, 0x80000);
    if (write_buffer_lram) {
        SET_PAGE(0x00200000, LowWram, 0x100000, write_buffer_lram);
    }
    if (write_buffer_hram) {
        uint32_t base;
        for (base = 0x06000000; base < 0x08000000; base += 0x00100000) {
            SET_PAGE(base, HighWram, 0x100000, write_buffer_hram);
        }
    }
    SET_BYTE_PAGE(0x05C00000, Vdp1Ram, 0x80000);
    SET_BYTE_PAGE(0x05E00000, Vdp2Ram, 0x80000);
    SET_BYTE_PAGE(0x05E80000, Vdp2Ram, 0x80000);

#undef SET_PAGE
#undef SET_EXEC_PAGE
#undef SET_BYTE_PAGE

    sh2_reset();
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
    sh2_run(state, cycles);
}

/*************************************************************************/

/**
 * psp_sh2_write_notify:  Called when an external agent modifies memory.
 *
 * [Parameters]
 *     address: Beginning of address range to which data was written
 *        size: Size of address range to which data was written (in bytes)
 * [Return value]
 *     None
 */
static void psp_sh2_write_notify(u32 address, u32 size)
{
    sh2_write_notify(address, size);
}

/*************************************************************************/
/**************************** Local functions ****************************/
/*************************************************************************/

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
static void invalid_opcode_handler(SH2_struct *state, uint32_t PC,
                                   uint16_t opcode)
{
    state->instruction = opcode;
    /* Hack for translation so the error shows at least PC properly */
    const uint32_t saved_PC = state->regs.PC;
    state->regs.PC = PC;
    YabSetError(YAB_ERR_SH2INVALIDOPCODE, state);
    state->regs.PC = saved_PC;
}

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
