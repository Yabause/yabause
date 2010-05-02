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

/* Convenience macro for rounding a float down to the next lower value */

#if defined(PSP)
static __attribute((always_inline,const)) inline int32_t IFLOORF(float n)
{
    int32_t result;
    float dummy;
    asm("floor.w.s %[dummy],%[n]; mfc1 %[result],%[dummy]"
        : [result] "=r" (result), [dummy] "=f" (dummy)
        : [n] "f" (n)
    );
    return result;
}
#elif defined(HAVE_FLOORF)
# define IFLOORF(n) ((int32_t)floorf((n)))
#else
# define IFLOORF(n) ((int32_t)floor((n)))
#endif

/*************************************************************************/

/* Simple checksum routine for block validity detection */

static uint32_t checksum(const uint16_t *ptr, unsigned int count)
{
    uint32_t sum = 0;
    while (count-- > 0) {
        sum += *ptr++;
    }
    return sum;
}

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

static int BIOS_0602E630_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void BIOS_0602E630(SH2State *state);

/*----------------------------------*/

static int Azel_0600614C_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_0600614C(SH2State *state);

static int Azel_0600C59C_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_0600C59C(SH2State *state);

static FASTCALL void Azel_0600FCB0(SH2State *state);

static int Azel_06010F24_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_06010F24(SH2State *state);

static int Azel_0603A22C_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch);
static FASTCALL void Azel_0603A22C(SH2State *state);

static FASTCALL void Azel_0603A242(SH2State *state);

static FASTCALL void Azel_0603DD6E(SH2State *state);

/*----------------------------------*/

static const struct {

    /* Start address applicable to this translation */
    uint32_t address;

    /* Routine that implements the SH-2 code */
    FASTCALL void (*execute)(SH2State *state);

    /* Routine to detect whether to use this translation; returns the
     * number of 16-bit words processed (nonzero) to use it, else zero.
     * If NULL, the checksum is checked instead */
    int (*detect)(SH2State *state, uint32_t address, const uint16_t *fetch);

    /* Checksum and block length (in instructions) if detect == NULL */
    uint32_t sum;
    unsigned int length;

} hand_tuned_table[] = {

    {0x00001CFC, BIOS_000025AC, BIOS_000025AC_detect},  // 1.00 JP
    {0x000025AC, BIOS_000025AC, BIOS_000025AC_detect},  // 1.01 JP / UE
    {0x00002658, BIOS_00002EFA, BIOS_00002EFA_detect},  // 1.00 JP
    {0x00002EFA, BIOS_00002EFA, BIOS_00002EFA_detect},  // 1.01 JP / UE
    {0x06010D22, BIOS_06010D22, BIOS_06010D22_detect},  // JP
    {0x06010D36, BIOS_06010D22, BIOS_06010D22_detect},  // UE
    {0x0602E630, BIOS_0602E630, BIOS_0602E630_detect},  // JP
    {0x0603A630, BIOS_0602E630, BIOS_0602E630_detect},  // UE

    {0x0600614C, Azel_0600614C, Azel_0600614C_detect},
    {0x0600C59C, Azel_0600C59C, Azel_0600C59C_detect},
    {0x0600FCB0, Azel_0600FCB0, .sum = 0x9D430, .length = 28},
    {0x06010F24, Azel_06010F24, Azel_06010F24_detect},
    {0x06010F52, Azel_06010F24, Azel_06010F24_detect},
    {0x0603A22C, Azel_0603A22C, Azel_0603A22C_detect},
    {0x0603A242, Azel_0603A242, .sum = 0x11703E, .length = 49},
    {0x0603DD6E, Azel_0603DD6E, .sum = 0xBB96E, .length = 31},

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
        if (hand_tuned_table[i].address != address) {
            continue;
        }
        unsigned int num_insns;
        if (hand_tuned_table[i].detect) {
            num_insns = hand_tuned_table[i].detect(state, address, fetch);
            if (!num_insns) {
                continue;
            }
        } else {
            num_insns = hand_tuned_table[i].length;
            const uint32_t sum = checksum(fetch, num_insns);
            if (sum != hand_tuned_table[i].sum) {
                continue;
            }
        }
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
         || UNLIKELY(!rtl_add_insn(rtl, RTLOP_LOAD_PARAM, state_reg, 0, 0, 0))
         || UNLIKELY(!rtl_add_insn(rtl, RTLOP_LOAD_ADDR, funcptr,
                                   (uintptr_t)hand_tuned_table[i].execute,
                                   0, 0))
         || UNLIKELY(!rtl_add_insn(rtl, RTLOP_CALL, 0, state_reg, 0, funcptr))
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
    return (fetch[-2] & 0xF000) == 0xB000  // bsr 0x60101AC  [only wastes time]
        && fetch[-1] == 0xE41E  // mov #30, r4
        && fetch[ 0] == 0xD011  // mov.l @(0x6010D68,pc), r0
        && fetch[ 1] == 0x6001  // mov.w @r0, r0
        && fetch[ 2] == 0x2008  // tst r0, r0
        && fetch[ 3] == 0x8BF9  // bf 0x6010D1E
        ? 4 : 0;
}

static FASTCALL void BIOS_06010D22(SH2State *state)
{
    const uint32_t address =
        T2ReadLong(HighWram, (state->PC + 4 + 0x11*4) & 0xFFFFC);
    state->R[0] = T2ReadWord(HighWram, address & 0xFFFFF);
    if (state->R[0]) {
        state->SR &= ~SR_T;
        state->cycles = state->cycle_limit;
    } else {
        state->SR |= SR_T;
        state->PC += 8;
        state->cycles += 4;
    }
}

/*-----------------------------------------------------------------------*/

/* 0x0602E630: Coordinate transformation */

static int BIOS_0602E630_is_UE;

static int BIOS_0602E630_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    if (address == 0x602E630 && checksum(fetch,612) == 0xA87EE4) {
        BIOS_0602E630_is_UE = 0;
        return 612;
    }
    if (address == 0x603A630 && checksum(fetch,600) == 0xA9F4CC) {
        BIOS_0602E630_is_UE = 1;
        return 600;
    }
    return 0;
}

static FASTCALL void BIOS_0602E630(SH2State *state)
{
    int32_t counter;

    const uint32_t R11 = T2ReadLong(HighWram, (state->R[4] & 0xFFFFF) + 64);
    int16_t * const R12_ptr = (int16_t *)&HighWram[
        T2ReadLong(HighWram, (state->PC+11*2 + 4 + 0x7B*4) & 0xFFFFC)
        & 0xFFFFF];
    int32_t * const R13_ptr = (int32_t *)&HighWram[
        T2ReadLong(HighWram, (state->PC+12*2 + 4 + 0x7B*4) & 0xFFFFC)
        & 0xFFFFF];

    const uint32_t M_R7 =
        T2ReadLong(HighWram, (state->PC+24*2 + 4 + 0x76*4) & 0xFFFFC);
    const int32_t * const M = (const int32_t *)&HighWram[
        T2ReadLong(HighWram, M_R7 & 0xFFFFF) & 0xFFFFF];

    const uint32_t R5 = T2ReadLong(HighWram, (state->R[4] & 0xFFFFF) + 56) + 4;
    counter = (int16_t)T2ReadWord(HighWram, (R5 - 4) & 0xFFFFF);
    state->cycles += 30;

    if (counter > 0) {

        /* 0x602E66E */

#ifdef PSP
# define DO_MULT(dest_all,dest_z,index) \
    int32_t dest_all, dest_z;                                           \
    do {                                                                \
        int32_t temp_x, temp_y, temp_z;                                 \
        asm(".set push; .set noreorder\n"                               \
            "lw %[temp_z], %[idx]*16+8(%[M])\n"                         \
            "lw %[temp_x], %[idx]*16+0(%[M])\n"                         \
            "lw %[temp_y], %[idx]*16+4(%[M])\n"                         \
            "ror %[temp_z], %[temp_z], 16\n"                            \
            "mult %[temp_z], %[in_z]\n"                                 \
            "ror %[temp_x], %[temp_x], 16\n"                            \
            "ror %[temp_y], %[temp_y], 16\n"                            \
            "mfhi %[temp_z]\n"                                          \
            "mflo %[dst_z]\n"                                           \
            "madd %[temp_x], %[in_x]\n"                                 \
            "sll %[temp_z], %[temp_z], 16\n"                            \
            "srl %[dst_z], %[dst_z], 16\n"                              \
            "lw %[temp_x], %[idx]*16+12(%[M])\n"                        \
            "madd %[temp_y], %[in_y]\n"                                 \
            "or %[dst_z], %[dst_z], %[temp_z]\n"                        \
            "ror %[temp_z], %[temp_x], 16\n"                            \
            "mfhi %[temp_x]\n"                                          \
            "mflo %[temp_y]\n"                                          \
            "sll %[temp_x], %[temp_x], 16\n"                            \
            "srl %[temp_y], %[temp_y], 16\n"                            \
            "or %[dst_all], %[temp_x], %[temp_y]\n"                     \
            "addu %[dst_all], %[dst_all], %[temp_z]\n"                  \
            ".set pop"                                                  \
            : [dst_all] "=r" (dest_all), [dst_z] "=&r" (dest_z),        \
              [temp_x] "=&r" (temp_x), [temp_y] "=&r" (temp_y),         \
              [temp_z] "=&r" (temp_z)                                   \
            : [M] "r" (M), [idx] "i" (index), [in_x] "r" (in_x),        \
              [in_y] "r" (in_y), [in_z] "r" (in_z)                      \
            : "hi", "lo"                                                \
        );                                                              \
    } while (0)
#else  // !PSP
# define GET_M(i)  ((int64_t)(int32_t)WSWAP32(M[(i)]))
# define DO_MULT(dest_all,dest_z,index) \
    const int32_t dest_z = ((int64_t)in_z * GET_M((index)*4+2)) >> 16;  \
    int32_t dest_all = (((int64_t)in_x * GET_M((index)*4+0)             \
                       + (int64_t)in_y * GET_M((index)*4+1)             \
                       + (int64_t)in_z * GET_M((index)*4+2)) >> 16)     \
                     + GET_M((index)*4+3);
#endif

        const uint32_t testflag =
            T2ReadLong(HighWram, (state->R[4] & 0xFFFFF) + 20);
        const int32_t *in = (const int32_t *)&HighWram[R5 & 0xFFFFF];
        int32_t *out = R13_ptr;
        int16_t *coord_out = R12_ptr;

        do {
            const int32_t in_x = WSWAP32(in[0]);
            const int32_t in_y = WSWAP32(in[1]);
            const int32_t in_z = WSWAP32(in[2]);

            DO_MULT(out_z, zz, 2);
            if (out_z < 0 && testflag) {
                out_z += out_z >> 3;
            }
            out[2] = WSWAP32(out_z);
            out[14] = WSWAP32(out_z - (zz<<1));

            DO_MULT(out_x, zx, 0);
            out[0] = WSWAP32(out_x);
            out[12] = WSWAP32(out_x - (zx<<1));

            DO_MULT(out_y, zy, 1);
            out[1] = WSWAP32(out_y);
            out[13] = WSWAP32(out_y - (zy<<1));

            /* The result gets truncated to 16 bits here, so we don't need
             * to worry about the 32->24 bit precision loss with floats.
             * (There are only a few pixels out of place during the entire
             * animation as a result of rounding error.) */
            const float coord_mult = 192.0f / out_z;
            *coord_out++ = (int16_t)IFLOORF(out_x * coord_mult);
            *coord_out++ = (int16_t)IFLOORF(out_y * coord_mult);

            in += 3;
            out += 3;
            counter -= 2;
            state->cycles += 110;  // Minimum value
        } while (counter > 0);

        state->cycles += 19;

#undef GET_M
#undef DO_MULT

    }  // if (counter > 0)

    /* 0x602E840 */

    /* Offset for second-half local data accesses */
    const int UE_PC_offset = (BIOS_0602E630_is_UE ? -12*2 : 0);

    counter = (int16_t)T2ReadWord(HighWram, R11 & 0xFFFFF);
    state->cycles += 19;

int savc=counter;
    if (counter > 0) {

#ifdef PSP
# define DOT3_16(v,x,y,z)  __extension__({                              \
    int32_t __temp1, __temp2, __result;                                 \
    asm(".set push; .set noreorder\n"                                   \
        "lw %[temp1], 0(%[V])\n"                                        \
        "lw %[temp2], 4(%[V])\n"                                        \
        "ror %[temp1], %[temp1], 16\n"                                  \
        "mult %[temp1], %[X]\n"                                         \
        "lw %[temp1], 8(%[V])\n"                                        \
        "ror %[temp2], %[temp2], 16\n"                                  \
        "madd %[temp2], %[Y]\n"                                         \
        "ror %[temp1], %[temp1], 16\n"                                  \
        "madd %[temp1], %[Z]\n"                                         \
        "mflo %[result]\n"                                              \
        "mfhi %[temp1]\n"                                               \
        "sra %[result], %[result], 16\n"                                \
        "ins %[result], %[temp1], 16, 16\n"                             \
        ".set pop"                                                      \
        : [temp1] "=&r" (__temp1), [temp2] "=&r" (__temp2),             \
          [result] "=r" (__result)                                      \
        : [V] "r" (v), [X] "r" (x), [Y] "r" (y), [Z] "r" (z)            \
        : "hi", "lo"                                                    \
    );                                                                  \
    __result;                                                           \
})
# define DOT3_32(v,x,y,z)  __extension__({                              \
    int32_t __temp1, __temp2, __result;                                 \
    asm(".set push; .set noreorder\n"                                   \
        "lw %[temp1], 0(%[V])\n"                                        \
        "lw %[temp2], 4(%[V])\n"                                        \
        "ror %[temp1], %[temp1], 16\n"                                  \
        "mult %[temp1], %[X]\n"                                         \
        "lw %[temp1], 8(%[V])\n"                                        \
        "ror %[temp2], %[temp2], 16\n"                                  \
        "madd %[temp2], %[Y]\n"                                         \
        "ror %[temp1], %[temp1], 16\n"                                  \
        "madd %[temp1], %[Z]\n"                                         \
        "mfhi %[result]\n"                                              \
        ".set pop"                                                      \
        : [temp1] "=&r" (__temp1), [temp2] "=&r" (__temp2),             \
          [result] "=r" (__result)                                      \
        : [V] "r" (v), [X] "r" (x), [Y] "r" (y), [Z] "r" (z)            \
        : "hi", "lo"                                                    \
    );                                                                  \
    __result;                                                           \
})
#else  // !PSP
# define DOT3_16(v,x,y,z)                                               \
    (((int64_t)(int32_t)WSWAP32((v)[0]) * (int64_t)(int32_t)(x)         \
    + (int64_t)(int32_t)WSWAP32((v)[1]) * (int64_t)(int32_t)(y)         \
    + (int64_t)(int32_t)WSWAP32((v)[2]) * (int64_t)(int32_t)(z)) >> 16)
# define DOT3_32(v,x,y,z)                                               \
    (((int64_t)(int32_t)WSWAP32((v)[0]) * (int64_t)(int32_t)(x)         \
    + (int64_t)(int32_t)WSWAP32((v)[1]) * (int64_t)(int32_t)(y)         \
    + (int64_t)(int32_t)WSWAP32((v)[2]) * (int64_t)(int32_t)(z)) >> 32)
#endif

        state->cycles += 68 + 95*(counter-2);

        /* 0x602E850 */

        const int32_t *in = (const int32_t *)&HighWram[(R11 & 0xFFFFF) + 28];
        const int32_t *coord_in = &R13_ptr[12];
        const uint32_t out_address =
            T2ReadLong(HighWram, (state->PC+264*2 + 4 + 0xA2*4 + UE_PC_offset) & 0xFFFFC);
        int32_t *out = (int32_t *)&HighWram[out_address & 0xFFFFF];
        int16_t *coord_out = &R12_ptr[8];

        const uint16_t *R6_ptr = (const uint16_t *)&HighWram[
            (T2ReadLong(HighWram, (state->R[4] & 0xFFFFF) + 60) & 0xFFFFF)
            + 4];
        const uint32_t flag_address =
            T2ReadLong(HighWram, (state->PC+348*2 + 4 + 0x79*4 + UE_PC_offset) & 0xFFFFC);
        int16_t *flag = (int16_t *)&HighWram[flag_address & 0xFFFFF];

        {
            const int32_t M_2  = WSWAP32(M[ 2]);
            const int32_t M_6  = WSWAP32(M[ 6]);
            const int32_t M_10 = WSWAP32(M[10]);

            out[0] = WSWAP32(-M_2);
            out[1] = WSWAP32(-M_6);
            out[2] = WSWAP32(-M_10);
            const int32_t *in0_0 =
                (const int32_t *)((uintptr_t)R13_ptr + R6_ptr[3]);
            R6_ptr += 10;
            const int32_t test_0 = DOT3_32(in0_0, -M_2, -M_6, -M_10);
            *flag++ = (test_0 < 0) ? 64 : -1;
            out += 3;
            counter--;

            out[0] = WSWAP32(M_2);
            out[1] = WSWAP32(M_6);
            out[2] = WSWAP32(M_10);
            const int32_t *in0_1 =
                (const int32_t *)((uintptr_t)R13_ptr + R6_ptr[3]);
            R6_ptr += 10;
            const int32_t test_1 = DOT3_32(in0_1, M_2, M_6, M_10);
            *flag++ = (test_1 < 0) ? 64 : -1;
            out += 3;
            counter--;
        }

        do {
            const int32_t in_x = WSWAP32(in[0]);
            const int32_t in_y = WSWAP32(in[1]);
            const int32_t in_z = WSWAP32(in[2]);

            const int32_t out_x = DOT3_16(&M[0], in_x, in_y, in_z);
            const int32_t out_y = DOT3_16(&M[4], in_x, in_y, in_z);
            const int32_t out_z = DOT3_16(&M[8], in_x, in_y, in_z);

            out[0] = WSWAP32(out_x);
            out[1] = WSWAP32(out_y);
            out[2] = WSWAP32(out_z);

            const float coord_mult = 192.0f / (int32_t)WSWAP32(coord_in[2]);
            *coord_out++ =
                (int16_t)IFLOORF((int32_t)WSWAP32(coord_in[0]) * coord_mult);
            *coord_out++ =
                (int16_t)IFLOORF((int32_t)WSWAP32(coord_in[1]) * coord_mult);
            coord_in += 3;

            const int32_t *in0 =
                (const int32_t *)((uintptr_t)R13_ptr + R6_ptr[3]);
            R6_ptr += 10;
            const int32_t test = DOT3_32(in0, out_x, out_y, out_z);
            *flag++ = (test < 0) ? 64 : -1;

            in += 3;
            out += 3;
            counter--;
        } while (counter > 0);

#undef DOT3_16
#undef DOT3_32

    }  // if (counter > 0)

    /* 0x602E914 */
    /* Note: At this point, all GPRs except R9, R12, R13, and R15 are dead */

    const int16_t *flag = (const int16_t *)&HighWram[
        T2ReadLong(HighWram, (state->PC+572*2 + 4 + 0x0F*4 + UE_PC_offset) & 0xFFFFC)
        & 0xFFFFF];
    const int32_t *R1_ptr = (const int32_t *)&HighWram[
        T2ReadLong(HighWram, (state->PC+378*2 + 4 + 0x6C*4 + UE_PC_offset) & 0xFFFFC)
        & 0xFFFFF];
    const uint16_t *R6_ptr = (const uint16_t *)&HighWram[
        T2ReadLong(HighWram, (state->R[4] & 0xFFFFF) + 60) & 0xFFFFF];
    const int32_t *R7_ptr = (const int32_t *)&HighWram[
        T2ReadLong(HighWram, (state->PC+371*2 + 4 + 0x6D*4 + UE_PC_offset) & 0xFFFFC)
        & 0xFFFFF];
    uint32_t *R9_ptr = (uint32_t *)&HighWram[
        T2ReadLong(HighWram, (state->PC+9*2 + 4 + 0x7A*4) & 0xFFFFC)
        & 0xFFFFF];
    uint16_t *R10_ptr = (uint16_t *)&HighWram[
        T2ReadLong(HighWram, (state->PC+370*2 + 4 + 0x73*4 + UE_PC_offset) & 0xFFFFC)
        & 0xFFFFF];

    const int32_t limit = *R6_ptr;
    R6_ptr += 2;
    state->cycles += 13;

if(savc!=limit)printf("BOGUS savc=%d limit=%d\n",savc,limit); //FIXME temp
    for (counter = 0; counter < limit; counter++, R7_ptr += 3, R6_ptr += 10, state->cycles += 7) {

        /* 0x602EAA8 */

        if (flag[counter] < 0) {
            state->cycles += 8;
            continue;
        }
        state->cycles += 9;

        /* 0x602E924 */

#ifdef PSP
        int32_t R2;
        {
            int32_t temp1, temp2, temp3;
            asm(".set push; .set noreorder\n"
                "lw %[temp1], 0(%[R7_ptr])\n"
                "lw %[temp2], 0(%[R1_ptr])\n"
                "lw %[temp3], 4(%[R7_ptr])\n"
                "ror %[temp1], %[temp1], 16\n"
                "ror %[temp2], %[temp2], 16\n"
                "mult %[temp1], %[temp2]\n"
                "lw %[temp1], 4(%[R1_ptr])\n"
                "lw %[temp2], 8(%[R7_ptr])\n"
                "ror %[temp3], %[temp3], 16\n"
                "ror %[temp1], %[temp1], 16\n"
                "madd %[temp3], %[temp1]\n"
                "lw %[temp3], 8(%[R1_ptr])\n"
                "ror %[temp2], %[temp2], 16\n"
                "ror %[temp3], %[temp3], 16\n"
                "madd %[temp2], %[temp3]\n"
                "mflo %[temp1]\n"
                "mfhi %[temp2]\n"
                "sra %[temp1], %[temp1], 16\n"
                "addiu %[temp2], %[temp2], 1\n"
                "ins %[temp1], %[temp2], 16, 16\n"
                "sra %[temp1], %[temp1], 10\n"
                "max %[temp1], %[temp1], $zero\n"
                "min %[R2], %[temp1], %[cst_127]\n"
                ".set pop"
                : [R2] "=r" (R2), [temp1] "=&r" (temp1),
                  [temp2] "=&r" (temp2), [temp3] "=&r" (temp3)
                : [R7_ptr] "r" (R7_ptr), [R1_ptr] "r" (R1_ptr),
                  [cst_127] "r" (127)
                : "hi", "lo"
            );
        }
#else  // !PSP
        const int32_t mac =
           ((int64_t) (int32_t)WSWAP32(R7_ptr[0]) * (int32_t)WSWAP32(R1_ptr[0])
          + (int64_t) (int32_t)WSWAP32(R7_ptr[1]) * (int32_t)WSWAP32(R1_ptr[1])
          + (int64_t) (int32_t)WSWAP32(R7_ptr[2]) * (int32_t)WSWAP32(R1_ptr[2])
          ) >> 16;
        const int32_t R2_temp = (mac + 0x10000) >> 10;
        const int32_t R2 = R2_temp < 0 ? 0 : R2_temp > 127 ? 127 : R2_temp;
#endif
        uint16_t * const R9_data16 =
            (uint16_t *)&HighWram[WSWAP32(*R9_ptr) & 0xFFFFF];
        *R9_data16 = *R6_ptr;
        uint32_t * const R9_data32 = (uint32_t *)((uintptr_t)R9_data16 + 4);
        state->cycles += 23;

        /* 0x602E93C */

        int32_t R3;
        switch (R6_ptr[1] & 0xFF) {
          case 0x00:
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 0];
            R9_data32[1] = *(uint32_t *)&R12_ptr[ 2];
            R9_data32[2] = *(uint32_t *)&R12_ptr[ 4];
            R9_data32[3] = *(uint32_t *)&R12_ptr[ 6];
            R3 = WSWAP32(R13_ptr[5]) + WSWAP32(R13_ptr[11]);
            break;

          case 0x30:
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 8];
            R9_data32[1] = *(uint32_t *)&R12_ptr[14];
            R9_data32[2] = *(uint32_t *)&R12_ptr[12];
            R9_data32[3] = *(uint32_t *)&R12_ptr[10];
            R3 = WSWAP32(R13_ptr[17]) + WSWAP32(R13_ptr[23]);
            break;

          case 0x60:
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 0];
            R9_data32[1] = *(uint32_t *)&R12_ptr[ 8];
            R9_data32[2] = *(uint32_t *)&R12_ptr[10];
            R9_data32[3] = *(uint32_t *)&R12_ptr[ 2];
            R3 = WSWAP32(R13_ptr[5]) + WSWAP32(R13_ptr[14]);
            break;

          case 0x90:
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 4];
            R9_data32[1] = *(uint32_t *)&R12_ptr[12];
            R9_data32[2] = *(uint32_t *)&R12_ptr[14];
            R9_data32[3] = *(uint32_t *)&R12_ptr[ 6];
            R3 = WSWAP32(R13_ptr[8]) + WSWAP32(R13_ptr[23]);
            break;

          case 0xC0:
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 2];
            R9_data32[1] = *(uint32_t *)&R12_ptr[10];
            R9_data32[2] = *(uint32_t *)&R12_ptr[12];
            R9_data32[3] = *(uint32_t *)&R12_ptr[ 4];
            R3 = WSWAP32(R13_ptr[5]) + WSWAP32(R13_ptr[20]);
            break;

          default:  // case 0xF0
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 0];
            R9_data32[1] = *(uint32_t *)&R12_ptr[ 6];
            R9_data32[2] = *(uint32_t *)&R12_ptr[14];
            R9_data32[3] = *(uint32_t *)&R12_ptr[ 8];
            R3 = WSWAP32(R13_ptr[11]) + WSWAP32(R13_ptr[14]);
            break;
        }
        state->cycles += 25;  // Approximate

        /* 0x602EA60 */

        const uint32_t R2_tableaddr = WSWAP32(*(const uint32_t *)&R6_ptr[8]);
        const uint16_t *R2_table =
            (const uint16_t *)&HighWram[R2_tableaddr & 0xFFFFF];
        R9_data16[21] = R2_table[R2];
        *R9_ptr = WSWAP32(WSWAP32(*R9_ptr) + 48);

        if (!BIOS_0602E630_is_UE && R3 < -0x30000 && (R6_ptr[1] & 0xFF00)) {
            R3 = -R3;
        }
        R3 >>= 1;
        uint32_t *R3_buffer = (uint32_t *)&HighWram[
            T2ReadLong(HighWram, (state->PC+558*2 + 4 + 0x17*4 + UE_PC_offset) & 0xFFFFC)
            & 0xFFFFF];
        R3_buffer[(*R10_ptr)++] = WSWAP32(R3);

        state->cycles += 29;  // Approximate
    }

    /* 0x602EAB8 */

    state->PC = state->PR;
    state->cycles += 10;
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
        M68KWriteNotify(dest & 0x7FFFF, len);
        return;
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
