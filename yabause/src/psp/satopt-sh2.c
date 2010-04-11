/*  src/psp/satopt-sh2.c: Saturn-specific SH-2 optimization routines
    Copyright 2009-2010 Andrew Church

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
/*************************** Required headers ****************************/
/*************************************************************************/

#include "common.h"

#include "../core.h"
#include "../cs2.h"
#include "../memory.h"
#include "../scsp.h"
#include "../scu.h"
#include "../sh2core.h"
#include "../vdp2.h"
#include "../yabause.h"

#include "rtl.h"
#include "sh2.h"
#include "sh2-internal.h"

#include "satopt-sh2.h"

#ifdef JIT_DEBUG_TRACE
# include "../sh2d.h"
#endif

/*************************************************************************/
/********************** Optimization routine table ***********************/
/*************************************************************************/

#ifdef ENABLE_JIT  // Through end of file

/*************************************************************************/

/* List of detection and translation routines for special-case blocks */

/*----------------------------------*/

static int BIOS_000025AC_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void BIOS_000025AC(SH2State *state);

static int BIOS_00002EFA_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void BIOS_00002EFA(SH2State *state);

static int BIOS_06010D22_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void BIOS_06010D22(SH2State *state);

/*----------------------------------*/

static int Azel_0600614C_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_0600614C(SH2State *state);

static int Azel_0600C59C_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_0600C59C(SH2State *state);

static int Azel_0600FCB0_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_0600FCB0(SH2State *state);

static int Azel_06010F24_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_06010F24(SH2State *state);

static int Azel_0603A22C_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_0603A22C(SH2State *state);

static int Azel_0603A242_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_0603A242(SH2State *state);

static int Azel_0603DD6E_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_0603DD6E(SH2State *state);

/*----------------------------------*/

static const struct {

    /* Start address applicable to this translation */
    uint32_t address;

    /* Routine to detect whether to use this translation; returns the
     * number of 16-bit words processed (nonzero) to use it, else zero */
    int (*detect)(SH2State *state, uint32_t address, const uint16_t *fetch);

    /* Routine that implements the SH-2 code */
    FASTCALL void (*execute)(SH2State *state);

} hand_tuned_table[] = {

    {0x00001CFC, BIOS_000025AC_detect, BIOS_000025AC},  // 1.00 JP
    {0x000025AC, BIOS_000025AC_detect, BIOS_000025AC},  // 1.01 JP / UE
    {0x00002658, BIOS_00002EFA_detect, BIOS_00002EFA},  // 1.00 JP
    {0x00002EFA, BIOS_00002EFA_detect, BIOS_00002EFA},  // 1.01 JP / UE
    {0x06010D22, BIOS_06010D22_detect, BIOS_06010D22},  // JP
    {0x06010D36, BIOS_06010D22_detect, BIOS_06010D22},  // UE

    {0x0600614C, Azel_0600614C_detect, Azel_0600614C},
    {0x0600C59C, Azel_0600C59C_detect, Azel_0600C59C},
    {0x0600FCB0, Azel_0600FCB0_detect, Azel_0600FCB0},
    {0x06010F24, Azel_06010F24_detect, Azel_06010F24},
    {0x06010F52, Azel_06010F24_detect, Azel_06010F24},
    {0x0603A22C, Azel_0603A22C_detect, Azel_0603A22C},
    {0x0603A242, Azel_0603A242_detect, Azel_0603A242},
    {0x0603DD6E, Azel_0603DD6E_detect, Azel_0603DD6E},

};

/*************************************************************************/

#endif  // ENABLE_JIT

/*************************************************************************/
/************************** Interface function ***************************/
/*************************************************************************/

/**
 * saturn_optimize_sh2:  Attempt to translate SH-2 code starting at the
 * given address into a hand-tuned RTL instruction stream.
 *
 * [Parameters]
 *       state: Processor state block pointer
 *     address: Address from which to translate
 *       fetch: Pointer corresponding to "address" from which opcodes can
 *                 be fetched
 *         rtl: RTL block in which to generate optimized code
 * [Return value]
 *     Length of translated block in instructions (nonzero) if optimized
 *     code was generated, else zero
 */
unsigned int saturn_optimize_sh2(SH2State *state, uint32_t address,
                                 const uint16_t *fetch, RTLBlock *rtl)
{
#ifdef ENABLE_JIT
    int i;
    for (i = 0; i < lenof(hand_tuned_table); i++) {
        const unsigned int num_insns =
            (hand_tuned_table[i].address == address
             && hand_tuned_table[i].detect(state, address, fetch));
        if (num_insns > 0) {
#ifdef JIT_DEBUG_TRACE
            unsigned int n;
            for (n = 0; n < num_insns; n++) {
                char tracebuf[100];
                SH2Disasm(address + n*2, fetch[n], 0, tracebuf);
                fprintf(stderr, "%08X: %04X  %s\n",
                        address + n*2, fetch[n], tracebuf+12);
            }
#endif
            const uint32_t state_reg = rtl_alloc_register(rtl);
            const uint32_t funcptr = rtl_alloc_register(rtl);
            if (UNLIKELY(!state_reg)
             || UNLIKELY(!funcptr)
             || UNLIKELY(!rtl_add_insn(rtl, RTLOP_LOAD_PARAM, state_reg,
                                       0, 0, 0))
             || UNLIKELY(!rtl_add_insn(rtl, RTLOP_LOAD_ADDR, funcptr,
                                       (uintptr_t)hand_tuned_table[i].execute,
                                       0, 0))
             || UNLIKELY(!rtl_add_insn(rtl, RTLOP_CALL, 0,
                                       state_reg, 0, funcptr))
             || UNLIKELY(!rtl_add_insn(rtl, RTLOP_RETURN, 0, 0, 0, 0))
            ) {
                DMSG("Failed to create RTL block");
                return 0;
            }
            if (UNLIKELY(!rtl_finalize_block(rtl))) {
                DMSG("Failed to finalize RTL block");
                return 0;
            }
            return num_insns;
        }
    }
#endif  // ENABLE_JIT

    return 0;  // No match found
}

/*************************************************************************/
/***************** Case-specific optimization functions ******************/
/*************************************************************************/

#ifdef ENABLE_JIT  // Through end of file

/*************************************************************************/

/* RTL instruction macros borrowed from sh2.c */

#ifdef __mips__
# define ADDR_HI(address)  (((uintptr_t)(address) + 0x8000) & 0xFFFF0000)
# define ADDR_LO(address)  ((uintptr_t)(address) & 0xFFFF)
#else
# define ADDR_HI(address)  ((uintptr_t)address)
# define ADDR_LO(address)  0
#endif

/*************************************************************************/

/**** Saturn BIOS optimizations ****/

/*-----------------------------------------------------------------------*/

/* 0x000025AC: Peripheral detection(?) loop */

static int BIOS_000025AC_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return fetch[-2] == 0x4C0B  // jsr @r12
        && fetch[-1] == 0x0009  // nop
        && fetch[ 0] == 0x2008  // tst r0, r0
        && fetch[ 1] == 0x8901  // bt fetch[4] --> bf fetch[12]
        && fetch[ 2] == 0xA008  // bra fetch[12] --> nop
        && fetch[ 3] == 0x0009  // nop
        && fetch[ 4] == 0x62D3  // mov r13, r2
        && fetch[ 5] == 0x32E7  // cmp/gt r14, r2
        && fetch[ 6] == 0x8F02  // bf/s fetch[10] --> bf/s fetch[-2]
        && fetch[ 7] == 0x7D01  // add #1, r13
        && fetch[ 8] == 0xA008  // bra fetch[18]
        && fetch[ 9] == 0xE000  // mov #0, r0
        && fetch[10] == 0xAFF2  // bra fetch[-2]
        && fetch[11] == 0x0009  // nop
        ? 12 : 0;
}

static FASTCALL void BIOS_000025AC(SH2State *state)
{
    if (UNLIKELY(state->R[0])) {
        state->SR &= ~SR_T;
        state->PC += 2*12;
        state->cycles += 5;
        return;
    }

    state->R[2] = state->R[13];
    state->SR &= ~SR_T;
    state->SR |= (state->R[13] > state->R[14]) << SR_T_SHIFT;
    if (UNLIKELY(state->R[13]++ > state->R[14])) {
        state->R[0] = 0;
        state->PC += 2*18;
        state->cycles += 11;
        return;
    }

    if (LIKELY(state->R[13]+15 <= state->R[14])) {
        state->R[13] += 15;
        state->cycles += 15*76 + 15;
    } else {
        state->cycles += 15;
    }
    state->PR = state->PC;
    state->PC = state->R[12];
}

/*-----------------------------------------------------------------------*/

/* 0x00002EFA: CD read loop */

static int BIOS_00002EFA_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return fetch[0] == 0x6503  // mov r0, r5
        && fetch[1] == 0x3CB3  // cmp/ge r11, r12
        && fetch[2] == 0x8D06  // bt/s fetch[10]
        && fetch[3] == 0x64C3  // mov r12, r4
        // fetch[4] == 0x7401  // add #1, r4      -- these two are swapped
        // fetch[5] == 0x6352  // mov.l @r5, r3   -- in some BIOS versions
        && fetch[6] == 0x2E32  // mov.l r3, @r14
        && fetch[7] == 0x34B3  // cmp/ge r11, r4
        && fetch[8] == 0x8FFA  // bf/s fetch[4]
        && fetch[9] == 0x7E04  // add #4, r14
        ? 10 : 0;
}

static FASTCALL void BIOS_00002EFA(SH2State *state)
{
    uint8_t *page_base;

    if (UNLIKELY(state->R[0] != 0x25818000)
     || UNLIKELY(!(page_base = direct_pages[state->R[14]>>19]))
    ) {
        state->R[5] = state->R[0];
        state->PC += 2;
        state->cycles += 1;
        return;
    }

    state->R[5] = state->R[0];
    state->SR |= SR_T;  // Always ends up set from here down

    int32_t count = state->R[11];
    int32_t i = state->R[12];
    int32_t left = count - i;
    if (UNLIKELY(left <= 0)) {
        state->R[4] = i;
        state->PC += 2*10;
        state->cycles += 5;
        return;
    }

    uint8_t *ptr = page_base + state->R[14];
    state->R[4] = count;
    state->R[14] += left*4;
    state->PC += 2*10;
    state->cycles += 7*left + (4-1);

    /* Call the copy routine last to avoid unnecessary register saves and
     * restores */
    Cs2RapidCopyT2(ptr, left);
}

/*-----------------------------------------------------------------------*/

/* 0x06010D22: 3-D intro animation idle loop */

static int BIOS_06010D22_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return (fetch[-2] & 0xF000) == 0xBA45  // bsr 0x60101AC  [only wastes time]
        && fetch[-1] == 0xE41E  // mov #30, r4
        && fetch[ 0] == 0xD011  // mov.l @(0x6010D68,pc), r0
        && fetch[ 1] == 0x6001  // mov.w @r0, r0
        && fetch[ 2] == 0x2008  // tst r0, r0
        && fetch[ 3] == 0x8BF9  // bf 0x6010D1E
        ? 4 : 0;
}

static FASTCALL void BIOS_06010D22(SH2State *state)
{
    uint32_t address = T2ReadLong(HighWram, 0x10D68);
    state->R[0] = MappedMemoryReadWord(address);
    if (state->R[0]) {
        state->SR &= ~SR_T;
        state->cycles = state->cycle_limit;
    } else {
        state->SR |= SR_T;
        state->PC += 8;
        state->cycles += 4;
    }
}

/*************************************************************************/

/**** Azel: Panzer Dragoon RPG (JP) optimizations ****/

/*-----------------------------------------------------------------------*/

/* 0x600614C: Idle loop with a JSR */

static int Azel_0600614C_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return fetch[-2] == 0x4B0B  // jsr @r11
        && fetch[-1] == 0x0009  // nop
        && fetch[ 0] == 0x60E2  // mov.l @r14, r0
        && fetch[ 1] == 0x2008  // tst r0, r0
        && fetch[ 2] == 0x8BFA  // bf fetch[-2]
        && state->R[14]>>20 == 0x060
        ? 3 : 0;
}

static FASTCALL void Azel_0600614C(SH2State *state)
{
    state->R[0] = T2ReadLong(HighWram, state->R[14] & 0xFFFFF);
    if (state->R[0]) {
        state->SR &= ~SR_T;
        state->PR = state->PC;
        state->PC = state->R[11];
        state->cycles = state->cycle_limit;
    } else {
        state->SR |= SR_T;
        state->PC += 2*3;
        state->cycles += 3;
    }
}

/*-----------------------------------------------------------------------*/

/* 0x600C59C: SSH2 idle loop */

static int Azel_0600C59C_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return fetch[0] == 0x60C0  // mov.b @r12, r0
        && fetch[1] == 0x600C  // extu.b r0, r0
        && fetch[2] == 0xC880  // tst #0x80, r0
        && fetch[3] == 0x89FB  // bt fetch[0]
        && state->R[12] == 0xFFFFFE11
        ? 4 : 0;
}

static FASTCALL void Azel_0600C59C(SH2State *state)
{
    state->R[0] = (uint8_t)(((SH2_struct *)(state->userdata))->onchip.FTCSR);
    if (LIKELY(!(state->R[0] & 0x80))) {
        /* This loop has already been executed once in the fall-in case,
         * so SR.T will already be set. */
        state->cycles = state->cycle_limit;
    } else {
        state->SR &= ~SR_T;
        state->PC += 2*4;
        state->cycles += 4;
    }
}

/*-----------------------------------------------------------------------*/

/* 0x600FCB0: Mysterious routine called frequently from 0x6006148 */

static int Azel_0600FCB0_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return fetch[-2] == 0x000B  // rts
        && fetch[-1] == 0x0009  // nop
        && fetch[ 0] == 0xD30D  // mov.l @(0x38,pc), r3  [0x604B68C]
        && fetch[ 1] == 0x8430  // mov.b @(0,r3), r0
        && fetch[ 2] == 0x2008  // tst r0, r0
        && fetch[ 3] == 0x89F9  // bt fetch[-2]
        && fetch[ 4] == 0x8431  // mov.b @(1,r3), r0
        && fetch[ 5] == 0x2008  // tst r0, r0
        && fetch[ 6] == 0x8950  // bt fetch[88]
        && fetch[ 7] == 0x4001  // shlr r0
        && fetch[ 8] == 0x8B62  // bf fetch[108]
        && fetch[ 9] == 0x5435  // mov.l @(20,r3), r4
        && fetch[10] == 0x5536  // mov.l @(24,r3), r5
        && fetch[11] == 0x5631  // mov.l @(4,r3), r6
        && fetch[12] == 0x5732  // mov.l @(8,r3), r7
        && fetch[13] == 0x4710  // dt r7
        && fetch[14] == 0x8F03  // bf/s fetch[19]
        && fetch[15] == 0x4601  // shlr r6
        && fetch[16] == 0x6644  // mov.b @r4+, r6
        && fetch[17] == 0xE708  // mov #8, r7
        && fetch[18] == 0x4601  // shlr r6
        && fetch[19] == 0x8B09  // bf 0x600FCEC
        && fetch[20] == 0x6144  // mov.b @r4+, r1
        && fetch[21] == 0x1345  // mov.l r4, @(20,r3)
        && fetch[22] == 0x2510  // mov.b r1, @r5
        && fetch[23] == 0x7501  // add #1, r5
        && fetch[24] == 0x1356  // mov.l r5, @(24,r3)
        && fetch[25] == 0x1361  // mov.l r6, @(4,r3)
        && fetch[26] == 0x000B  // rts
        && fetch[27] == 0x1372  // mov.l r7, @(8,r3)
        ? 4 : 0;
}

static FASTCALL void Azel_0600FCB0(SH2State *state)
{
    state->R[3] = 0x604B68C;
    uint8_t *r3_ptr = (uint8_t *)(HighWram + 0x4B68C);

    state->R[0] = r3_ptr[0^1];
    if (!state->R[0]) {
        state->SR |= SR_T;
        state->PC = state->PR;
        state->cycles += 9;
        return;
    }

    // No need to set SR, as the next code takes care of that
    state->PC += 2*4;
    state->cycles += 4;
#if 0
    state->R[0] = r3_ptr[1^1];
    if (!state->R[0]) {
        state->SR |= SR_T;
        state->PC += 88;
        state->cycles += 9;
        return;
    }
#endif
}

/*-----------------------------------------------------------------------*/

/* 0x6010F24/0x6010F52: Bitmap copy loop (for movies) */

static int Azel_06010F24_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return fetch[ 0] == 0x6693  // mov r9, r6
        && fetch[ 1] == 0x65D3  // mov r13, r5
        && fetch[ 2] == 0x4A0B  // jsr @r10  [DMA copy routine]
        && fetch[ 3] == 0x64E3  // mov r14, r4
        // fetch[ 4] == 0x53F1  // {mov.l @(4,r15), r3 [counter] | add r8, r14}
        // fetch[ 5] == 0x3E8C  // {add r8, r14 | mov.l @r15, r3 [counter]}
        && fetch[ 6] == 0x73FF  // add #-1, r3
        // fetch[ 7] == 0x2338  // {tst r3, r3 | mov.l r3, @r15}
        // fetch[ 8] == 0x1F31  // {mov.l r3, @(4,r15) | tst r3, r3}
        && fetch[ 9] == 0x8FF5  // bf/s fetch[0]
        && fetch[10] == 0x3DCC  // add r12, r13
        && state->R[13]>>20 == 0x25E
        ? 11 : 0;
}

static FASTCALL void Azel_06010F24(SH2State *state)
{
    const uint32_t src_addr = state->R[14];
    const uint32_t dest_addr = state->R[13];
    const uint32_t size = state->R[9];
    const uint32_t *src = (const uint32_t *)(HighWram + (src_addr & 0xFFFFF));
    uint32_t *dest = (uint32_t *)(Vdp2Ram + (dest_addr & 0xFFFFF));

    uint32_t i;
    for (i = 0; i < size; i += 16, src += 4, dest += 4) {
        const uint32_t word0 = src[0];
        const uint32_t word1 = src[1];
        const uint32_t word2 = src[2];
        const uint32_t word3 = src[3];
        dest[0] = BSWAP16(word0);
        dest[1] = BSWAP16(word1);
        dest[2] = BSWAP16(word2);
        dest[3] = BSWAP16(word3);
    }

    state->R[14] = src_addr + state->R[8];
    state->R[13] = dest_addr + state->R[12];

    /* Conveniently, the counter is always stored in R3 when we get here
     * (it's first loaded when the loop is executed on the fall-in case)
     * and the stack variables are never referenced once their respective
     * loops complete, so we don't have to worry about which loop we're in. */
    unsigned int counter = state->R[3];
    counter--;
    state->R[3] = counter;

    if (counter != 0) {
        state->SR &= ~SR_T;
        state->cycles += 290;
    } else {
        state->SR |= SR_T;
        state->PC += 2*11;
        state->cycles += 289;
    }
}

/*-----------------------------------------------------------------------*/

/* 0x603A22C: Wrapper for 0x603A242 that jumps in with a BRA */

static int Azel_0603A22C_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return fetch[0] == 0xA009  // bra 0x603A242
        && fetch[1] == 0xE700  // mov #0, r7
        ? 2 : 0;
}

static FASTCALL void Azel_0603A22C(SH2State *state)
{
    state->R[7] = 0;
    state->PC += 0x16;
    Azel_0603A242(state);
}

/*----------------------------------*/

/* 0x603A242: CD command execution routine */

static int Azel_0603A242_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return fetch[0] == 0x2FE6  // mov.l r14, @-r15
        && fetch[1] == 0x2FD6  // mov.l r13, @-r15
        && fetch[2] == 0x6D63  // mov r6, r13
        && fetch[3] == 0xD332  // mov.l @(0x603A314,pc), r3  [0x6037036]
        // etc. (we assume the rest is as expected)
        ? 49 : 0;
}

static int Azel_0603A242_state = 0;
static int Azel_0603A242_cmd51_count = 0;  // Count of sequential 0x51 commands

static FASTCALL void Azel_0603A242(SH2State *state)
{
    if (Azel_0603A242_state == 0) {
        const uint32_t r4 = state->R[4];
        uint16_t *ptr_6053278 = (uint16_t *)(&HighWram[0x53278]);
        *ptr_6053278 |= Cs2ReadWord(0x90008);
        if ((*ptr_6053278 & r4) != r4) {
            state->R[0] = -1;
            state->PC = state->PR;
            state->cycles += 77;
            return;
        }
        if (!(*ptr_6053278 & 1)) {
            state->R[0] = -2;
            state->PC = state->PR;
            state->cycles += 82;
            return;
        }

        Cs2WriteWord(0x90008, ~(r4 | 1));
        *ptr_6053278 &= ~1;
        const uint32_t r5 = state->R[5];
        uintptr_t r5_base = (uintptr_t)direct_pages[r5>>19];
        const uint16_t *r5_ptr = (const uint16_t *)(r5_base + r5);
        Cs2WriteWord(0x90018, r5_ptr[0]);
        Cs2WriteWord(0x9001C, r5_ptr[1]);
        Cs2WriteWord(0x90020, r5_ptr[2]);
        Cs2WriteWord(0x90024, r5_ptr[3]);
        state->cycles += 88;
        Azel_0603A242_state = 1;
        if (r5_ptr[0]>>8 == 0x51) {
            Azel_0603A242_cmd51_count++;
        } else if (r5_ptr[0]>>8 != 0) { // Command 0x00 doesn't reset the count
            Azel_0603A242_cmd51_count = 0;
        }
        return;
    }

    if (Azel_0603A242_state == 1) {
        uint32_t status = (uint16_t)Cs2ReadWord(0x90008);
        if (status & 1) {
            state->cycles += 23;
            Azel_0603A242_state = 2;
        } else {
            /* Technically a timeout loop, but we assume no timeouts */
            state->cycles = state->cycle_limit;
        }
        return;
    }

    if (Azel_0603A242_state == 2) {
        const uint32_t r6 = state->R[6];
        uintptr_t r6_base = (uintptr_t)direct_pages[r6>>19];
        uint16_t *r6_ptr = (uint16_t *)(r6_base + r6);
        const unsigned int CR1 = Cs2ReadWord(0x90018);
        const unsigned int CR2 = Cs2ReadWord(0x9001C);
        const unsigned int CR3 = Cs2ReadWord(0x90020);
        const unsigned int CR4 = Cs2ReadWord(0x90024);
        if (Azel_0603A242_cmd51_count >= 0 && CR4 == 0) {
            /* We're probably waiting for a sector and it hasn't arrived
             * yet, so consume enough cycles to get us to that next sector.
             * But be careful we don't wait an extra sector if the current
             * sector finished reading between executing the CS2 command
             * and retrieving the delay period. */
            const unsigned int usec_left = Cs2GetTimeToNextSector();
            if (usec_left > 0 && usec_left < (1000000/(75*2))*9/10) {
                uint32_t cycles_left = 0;
                if (yabsys.CurSH2FreqType == CLKTYPE_26MHZ) {
                    cycles_left = (26847 * usec_left) / 1000;
                } else {
                    cycles_left = (28637 * usec_left) / 1000;
                }
                state->cycles += cycles_left;
            }
        }
        r6_ptr[0] = CR1;
        r6_ptr[1] = CR2;
        r6_ptr[2] = CR3;
        r6_ptr[3] = CR4;
        uint16_t *dest = (uint16_t *)&HighWram[0x5329C];
        *((uint8_t *)dest + 1) = CR1>>8;
        if (state->R[7]) {
            dest[2] = CR1<<8 | CR2>>8;
            dest[3] = CR2<<8 | CR3>>8;
            dest[4] = CR3 & 0xFF;
            dest[5] = CR4;
        }
        state->R[0] = 0;
#if defined(TRACE) || defined(TRACE_STEALTH) || defined(TRACE_LITE)
        state->R[1] = ~0xF0;
        state->R[2] = 0;
        state->R[3] = ~0xF0;
        state->R[4] = state->R[15] - 12;
        state->R[5] = 0x605329C;
        state->SR &= ~SR_T;
#endif
        state->PC = state->PR;
        state->cycles += 121;
        Azel_0603A242_state = 0;
        return;
    }
}

/*-----------------------------------------------------------------------*/

/* 0x0603DD6E: CD read routine (actually a generalized copy routine, but
 * doesn't seem to be used for anything else) */

static int Azel_0603DD6E_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return fetch[0] == 0x2448  // tst r4, r4
        && fetch[1] == 0x2FE6  // mov.l r14, @-r15
        && fetch[2] == 0x2FD6  // mov.l r13, @-r15
        && fetch[3] == 0x6D63  // mov r6, r13
        && fetch[3] == 0x8D16  // bt/s 0x603DDA6
        // etc. (we assume the rest is as expected)
        ? 31 : 0;
}

static FASTCALL void Azel_0603DD6E(SH2State *state)
{
    int32_t len = MappedMemoryReadLong(state->R[15]);
    uint32_t dest = state->R[4];

    if (UNLIKELY(state->R[5] != 1)
     || UNLIKELY(state->R[6] != 0x25818000)
     || UNLIKELY(state->R[7] != 0)
     || UNLIKELY(len <= 0)
     || UNLIKELY((len & 3) != 0)
    ) {
        state->SR &= ~SR_T;
        state->SR |= (state->R[4] == 0) << SR_T_SHIFT;
        state->PC += 2;
        state->cycles += 1;
        return;
    }

    state->PC = state->PR;
    state->cycles += 30 + len*2;

    const uint32_t dest_page = dest>>19;
    uint8_t *dest_base;

    dest_base = direct_pages[dest_page];
    if (dest_base) {
        Cs2RapidCopyT2(dest_base + dest, len/4);
        sh2_write_notify(dest, len);
        return;
    }

    dest_base = byte_direct_pages[dest_page];
    if (dest_base) {
        Cs2RapidCopyT1(dest_base + dest, len/4);
        return;
    }

    if ((dest & 0x1FF00000) == 0x05A00000) {
        Cs2RapidCopyT2(SoundRam + (dest & 0x7FFFF), len/4);
        M68KWriteNotify(dest, len);
    }

    for (; len > 0; len -= 4, dest += 4) {
        const uint32_t word = MappedMemoryReadLong(0x25818000);
        MappedMemoryWriteLong(dest, word);
    }
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
