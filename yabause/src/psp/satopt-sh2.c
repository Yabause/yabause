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
#include "../vdp1.h"
#include "../vdp2.h"
#include "../yabause.h"

#include "sh2.h"
#include "sh2-internal.h"

#include "satopt-sh2.h"

#ifdef JIT_DEBUG_TRACE
# include "../sh2d.h"
#endif

/*************************************************************************/

/* Shorthand macros for memory access, both saving us from having to write
 * out 0xFFFFF all over the place and allowing optimized memory access
 * using constant offsets where possible (we assume small constant offsets
 * don't fall outside the relevant memory region).  Note that offsetted
 * byte accesses to 16-bit-organized memory generally will not be optimized
 * on little-endian systems, because the XOR has to be applied after adding
 * the offset to the base; if the base is known to be 16-bit aligned, it
 * may be faster to load a 16-bit word and mask or shift as appropriate. */

#define HRAM_PTR(base)  ((uint8_t *)&HighWram[(base) & 0xFFFFF])
#define HRAM_LOADB(base,offset) \
    T2ReadByte(HighWram, ((base) + (offset)) & 0xFFFFF)
#define HRAM_LOADW(base,offset) \
    T2ReadWord(HighWram, ((base) & 0xFFFFF) + (offset))
#define HRAM_LOADL(base,offset) \
    T2ReadLong(HighWram, ((base) & 0xFFFFF) + (offset))
#define HRAM_STOREB(base,offset,data) \
    T2WriteByte(HighWram, ((base) + (offset)) & 0xFFFFF, (data))
#define HRAM_STOREW(base,offset,data) \
    T2WriteWord(HighWram, ((base) & 0xFFFFF) + (offset), (data))
#define HRAM_STOREL(base,offset,data) \
    T2WriteLong(HighWram, ((base) & 0xFFFFF) + (offset), (data))

#define LRAM_PTR(base)  ((uint8_t *)&LowWram[(base) & 0xFFFFF])
#define LRAM_LOADB(base,offset) \
    T2ReadByte(LowWram, ((base) + (offset)) & 0xFFFFF)
#define LRAM_LOADW(base,offset) \
    T2ReadWord(LowWram, ((base) & 0xFFFFF) + (offset))
#define LRAM_LOADL(base,offset) \
    T2ReadLong(LowWram, ((base) & 0xFFFFF) + (offset))
#define LRAM_STOREB(base,offset,data) \
    T2WriteByte(LowWram, ((base) + (offset)) & 0xFFFFF, (data))
#define LRAM_STOREW(base,offset,data) \
    T2WriteWord(LowWram, ((base) & 0xFFFFF) + (offset), (data))
#define LRAM_STOREL(base,offset,data) \
    T2WriteLong(LowWram, ((base) & 0xFFFFF) + (offset), (data))

#define VDP1_PTR(base)  ((uint8_t *)&Vdp1Ram[(base) & 0x7FFFF])
#define VDP1_LOADB(base,offset) \
    T1ReadByte(Vdp1Ram, ((base) & 0x7FFFF) + (offset))
#define VDP1_LOADW(base,offset) \
    T1ReadWord(Vdp1Ram, ((base) & 0x7FFFF) + (offset))
#define VDP1_LOADL(base,offset) \
    T1ReadLong(Vdp1Ram, ((base) & 0x7FFFF) + (offset))
#define VDP1_STOREB(base,offset,data) \
    T1WriteByte(Vdp1Ram, ((base) & 0x7FFFF) + (offset), (data))
#define VDP1_STOREW(base,offset,data) \
    T1WriteWord(Vdp1Ram, ((base) & 0x7FFFF) + (offset), (data))
#define VDP1_STOREL(base,offset,data) \
    T1WriteLong(Vdp1Ram, ((base) & 0x7FFFF) + (offset), (data))

#define VDP2_PTR(base)  ((uint8_t *)&Vdp2Ram[(base) & 0x7FFFF])
#define VDP2_LOADB(base,offset) \
    T1ReadByte(Vdp2Ram, ((base) & 0x7FFFF) + (offset))
#define VDP2_LOADW(base,offset) \
    T1ReadWord(Vdp2Ram, ((base) & 0x7FFFF) + (offset))
#define VDP2_LOADL(base,offset) \
    T1ReadLong(Vdp2Ram, ((base) & 0x7FFFF) + (offset))
#define VDP2_STOREB(base,offset,data) \
    T1WriteByte(Vdp2Ram, ((base) & 0x7FFFF) + (offset), (data))
#define VDP2_STOREW(base,offset,data) \
    T1WriteWord(Vdp2Ram, ((base) & 0x7FFFF) + (offset), (data))
#define VDP2_STOREL(base,offset,data) \
    T1WriteLong(Vdp2Ram, ((base) & 0x7FFFF) + (offset), (data))

/* Use these macros instead of [BW]SWAP{16,32} when loading from or
 * storing to work RAM or VDP memory, to avoid breaking things on
 * big-endian systems.  RAM_VDP_SWAPL is for copying longwords directly
 * between work RAM and VDP memory, and is one operation faster on
 * little-endian machines than VDP_SWAPL(RAM_SWAPL(x)). */

#ifdef WORDS_BIGENDIAN
# define RAM_SWAPL(x)      (x)
# define VDP_SWAPW(x)      (x)
# define VDP_SWAPL(x)      (x)
# define RAM_VDP_SWAPL(x)  (x)
#else
# define RAM_SWAPL(x)      WSWAP32(x)
# define VDP_SWAPW(x)      BSWAP16(x)
# define VDP_SWAPL(x)      BSWAP32(x)
# define RAM_VDP_SWAPL(x)  BSWAP16(x)  // BSWAP32(WSWAP32(x)) == BSWAP16(x)
#endif

/*************************************************************************/

/* Simple checksum routine for block validity detection. */

static inline uint32_t checksum(const uint16_t *ptr, unsigned int count)
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

#define DECLARE_OPTIMIZER(name)                                 \
    static FASTCALL void name(SH2State *state)

#define DECLARE_OPTIMIZER_WITH_DETECT(name)                     \
    static int name##_detect(SH2State *state, uint32_t address, \
                                    const uint16_t *fetch);     \
    DECLARE_OPTIMIZER(name)

/*----------------------------------*/

DECLARE_OPTIMIZER_WITH_DETECT(BIOS_000025AC);
DECLARE_OPTIMIZER_WITH_DETECT(BIOS_00002EFA);
DECLARE_OPTIMIZER            (BIOS_00003BC6);
DECLARE_OPTIMIZER            (BIOS_06001670);
DECLARE_OPTIMIZER_WITH_DETECT(BIOS_06010D22);
DECLARE_OPTIMIZER            (BIOS_060115A4);
DECLARE_OPTIMIZER_WITH_DETECT(BIOS_060115B6);
DECLARE_OPTIMIZER_WITH_DETECT(BIOS_060115D4);
DECLARE_OPTIMIZER            (BIOS_0602E364);
DECLARE_OPTIMIZER_WITH_DETECT(BIOS_0602E630);

/*----------------------------------*/

DECLARE_OPTIMIZER_WITH_DETECT(Azel_0600614C);
DECLARE_OPTIMIZER            (Azel_060061F0);
DECLARE_OPTIMIZER_WITH_DETECT(Azel_0600C59C);
DECLARE_OPTIMIZER_WITH_DETECT(Azel_06010F24);
DECLARE_OPTIMIZER            (Azel_0601EC20);
DECLARE_OPTIMIZER            (Azel_0601EC3C);
DECLARE_OPTIMIZER            (Azel_0601F30E);
DECLARE_OPTIMIZER            (Azel_0601F3F4);
DECLARE_OPTIMIZER            (Azel_0601F58C);
DECLARE_OPTIMIZER_WITH_DETECT(Azel_0601F5F0);
DECLARE_OPTIMIZER            (Azel_0601FA56);
DECLARE_OPTIMIZER            (Azel_0601FA68);
DECLARE_OPTIMIZER            (Azel_06035530);
DECLARE_OPTIMIZER            (Azel_06035552);
DECLARE_OPTIMIZER            (Azel_0603556C);
DECLARE_OPTIMIZER            (Azel_06035A8C);
DECLARE_OPTIMIZER            (Azel_06035A9C);
DECLARE_OPTIMIZER            (Azel_06035AA0);
DECLARE_OPTIMIZER            (Azel_06035B14);
DECLARE_OPTIMIZER            (Azel_06035F00);
DECLARE_OPTIMIZER            (Azel_06035F04);
DECLARE_OPTIMIZER_WITH_DETECT(Azel_0603A22C);
DECLARE_OPTIMIZER            (Azel_0603A242);
DECLARE_OPTIMIZER            (Azel_0603DD6E);

/*----------------------------------*/

static const struct {

    /* Start address applicable to this translation. */
    uint32_t address;

    /* Routine that implements the SH-2 code.  If NULL, no optimized
     * translation is generated; instead, the hints specified in the
     * .hint_* fields are applied to the current block. */
    FASTCALL void (*execute)(SH2State *state);

    /* Routine to detect whether to use this translation; returns the
     * number of 16-bit words processed (nonzero) to use it, else zero.
     * If NULL, the checksum is checked instead */
    int (*detect)(SH2State *state, uint32_t address, const uint16_t *fetch);

    /* Checksum and block length (in instructions) if detect == NULL. */
    uint32_t sum;
    unsigned int length;

    /* Nonzero if this function is foldable (i.e. does not modify any
     * registers other than R0-R7, MACH/MACL, and PC). */
    uint8_t foldable;

    /* Bitmask indicating which general purpose registers should be hinted
     * as having a constant value at block start. */
    uint16_t hint_constant_reg;

    /* Bitmask indicating which general purpose registers should be hinted
     * as containing a data pointer at block start. */
    uint16_t hint_data_pointer_reg;

    /* Bitmask indicating which of the first 32 instructions in the block
     * should be hinted as loading a data pointer. */
    uint32_t hint_data_pointer_load;

} hand_tuned_table[] = {

    /******** BIOS startup animation ********/

    {0x00001CFC, BIOS_000025AC, BIOS_000025AC_detect},  // 1.00 JP
    {0x000025AC, BIOS_000025AC, BIOS_000025AC_detect},  // 1.01 JP / UE
    {0x00002658, BIOS_00002EFA, BIOS_00002EFA_detect},  // 1.00 JP
    {0x00002EFA, BIOS_00002EFA, BIOS_00002EFA_detect},  // 1.01 JP / UE
    {0x00002D44, BIOS_00003BC6, .sum = 0x1A6ECE, .length = 70}, // 1.00 JP
    {0x00003BC6, BIOS_00003BC6, .sum = 0x1A40C9, .length = 71}, // 1.01 JP / UE
    {0x06001664, BIOS_06001670, .sum = 0x04A2D6, .length = 13}, // 1.00 JP
    {0x06001670, BIOS_06001670, .sum = 0x04A2D6, .length = 13}, // 1.01 JP
    {0x06001674, BIOS_06001670, .sum = 0x04A2D6, .length = 13}, // UE
    {0x06010D22, BIOS_06010D22, BIOS_06010D22_detect},  // JP
    {0x06010D36, BIOS_06010D22, BIOS_06010D22_detect},  // UE
    {0x060115C0, BIOS_060115A4, .sum = 0x032D22, .length =  9}, // 1.00 JP
    {0x060115A4, BIOS_060115A4, .sum = 0x032D22, .length =  9}, // 1.01 JP
    {0x06011654, BIOS_060115A4, .sum = 0x032D22, .length =  9}, // UE
    {0x060115D2, BIOS_060115B6, BIOS_060115B6_detect},  // 1.00 JP
    {0x060115B6, BIOS_060115B6, BIOS_060115B6_detect},  // 1.01 JP
    {0x06011666, BIOS_060115B6, BIOS_060115B6_detect},  // UE
    {0x060115F0, BIOS_060115D4, BIOS_060115D4_detect},  // 1.00 JP
    {0x060115D4, BIOS_060115D4, BIOS_060115D4_detect},  // 1.01 JP
    {0x06011684, BIOS_060115D4, BIOS_060115D4_detect},  // UE
    {0x0602DA80, NULL,          .sum = 0x0A6FD6, .length = 31,  // JP
         .hint_data_pointer_load = 1<<20},
    {0x06039A80, NULL,          .sum = 0x0A6FD6, .length = 31,  // UE
         .hint_data_pointer_load = 1<<20},
    {0x0602DABE, NULL,          .sum = 0x016AF5, .length =  3,  // JP
         .hint_data_pointer_reg = 1<<12},
    {0x06039ABE, NULL,          .sum = 0x016AF5, .length =  3,  // UE
         .hint_data_pointer_reg = 1<<12},
    {0x0602DAC4, NULL,          .sum = 0x016AF5, .length =  3,  // JP
         .hint_data_pointer_reg = 1<<12},
    {0x06039AC4, NULL,          .sum = 0x016AF5, .length =  3,  // UE
         .hint_data_pointer_reg = 1<<12},
    {0x0602DACA, NULL,          .sum = 0x016AF6, .length =  3,  // JP
         .hint_data_pointer_reg = 1<<12},
    {0x06039ACA, NULL,          .sum = 0x016AF6, .length =  3,  // UE
         .hint_data_pointer_reg = 1<<12},
    {0x0602DF10, NULL,          .sum = 0x04634B, .length = 13,  // 1.00 JP
         .hint_data_pointer_load = 1<<2},
    {0x0602DF30, NULL,          .sum = 0x04634B, .length = 13,  // 1.01 JP
         .hint_data_pointer_load = 1<<2},
    {0x06039F30, NULL,          .sum = 0x04634B, .length = 13,  // UE
         .hint_data_pointer_load = 1<<2},
    {0x0602E364, BIOS_0602E364, .sum = 0x074A3C, .length = 20,  // JP
         .foldable = 1},
    {0x0603A364, BIOS_0602E364, .sum = 0x074A3C, .length = 20,  // UE
         .foldable = 1},
    {0x0602E3B0, NULL,          .sum = 0x02AAF2, .length =  7,  // JP
         .hint_data_pointer_load = 1<<6},
    {0x0603A3B0, NULL,          .sum = 0x02AAF2, .length =  7,  // UE
         .hint_data_pointer_load = 1<<6},
    {0x0602E410, NULL,          .sum = 0x0298A3, .length =  5,  // JP
         .hint_data_pointer_load = 1<<4},
    {0x0603A410, NULL,          .sum = 0x0298A3, .length =  5,  // UE
         .hint_data_pointer_load = 1<<4},
    {0x0602E4B8, NULL,          .sum = 0x02984F, .length =  5,  // JP
         .hint_data_pointer_load = 1<<4},
    {0x0603A4B8, NULL,          .sum = 0x02984F, .length =  5,  // UE
         .hint_data_pointer_load = 1<<4},
    {0x0602E560, NULL,          .sum = 0x0297FB, .length =  5,  // JP
         .hint_data_pointer_load = 1<<4},
    {0x0603A560, NULL,          .sum = 0x0297FB, .length =  5,  // UE
         .hint_data_pointer_load = 1<<4},
    {0x0602E38C, NULL,          .sum = 0x04E94B, .length = 18,  // JP
         .hint_data_pointer_load = 1<<0 | 1<<7},
    {0x0603A38C, NULL,          .sum = 0x07294B, .length = 18,  // UE
         .hint_data_pointer_load = 1<<0 | 1<<7},
    {0x0602E630, BIOS_0602E630, BIOS_0602E630_detect},  // JP
    {0x0603A630, BIOS_0602E630, BIOS_0602E630_detect},  // UE

    /******** Azel: Panzer Dragoon RPG (JP) ********/

    {0x0600614C, Azel_0600614C, Azel_0600614C_detect},
    {0x060061F0, Azel_060061F0, .sum = 0x2FB438, .length = 167,
         .foldable = 1},
    {0x0600C59C, Azel_0600C59C, Azel_0600C59C_detect},
    {0x06010F24, Azel_06010F24, Azel_06010F24_detect},
    {0x06010F52, Azel_06010F24, Azel_06010F24_detect},
    {0x0601EC20, Azel_0601EC20, .sum = 0x05F967, .length = 14},
    {0x0601EC3C, Azel_0601EC3C, .sum = 0x05F97D, .length = 14},
    {0x0601F240, NULL,          .sum = 0x13322C, .length = 75,
         .hint_data_pointer_reg = 1<<5 | 1<<6},
    {0x0601F24C, NULL,          .sum = 0x11E92D, .length = 69,
         .hint_data_pointer_reg = 1<<5 | 1<<6},
    {0x0601F30E, Azel_0601F30E, .sum = 0x250327, .length = 115},
    {0x0601F3F4, Azel_0601F3F4, .sum = 0x1A4DA5, .length = 84},
    {0x0601F58C, Azel_0601F58C, .sum = 0x10AE46, .length = 49},
    {0x0601F5F0, Azel_0601F5F0, Azel_0601F5F0_detect},
    {0x0601FA56, Azel_0601FA56, .sum = 0x13FA32, .length = 61},
    {0x0601FA68, Azel_0601FA68, .sum = 0x103D59, .length = 52,
         .foldable = 1},
    {0x06021FA2, NULL,          .sum = 0x2A12AF, .length = 136,
         .hint_constant_reg = 1<<13},
    {0x06028EE8, NULL,          .sum = 0x1F09D1, .length = 103,
         .hint_constant_reg = 1<<10},
    {0x0602F834, NULL,          .sum = 0x0676B0, .length = 16,
         .hint_data_pointer_load = 1<<11},
    {0x06035530, Azel_06035530, .sum = 0x05ABFA, .length = 17,
         .foldable = 1},
    {0x06035552, Azel_06035552, .sum = 0x0358EB, .length = 13,
         .foldable = 1},
    {0x0603556C, Azel_0603556C, .sum = 0x0332E9, .length = 11,
         .foldable = 1},
    {0x06035A8C, Azel_06035A8C, .sum = 0x0A1ECF, .length = 36,
         .foldable = 1},
    {0x06035A9C, Azel_06035A9C, .sum = 0x065B51, .length = 28,
         .foldable = 1},
    {0x06035AA0, Azel_06035AA0, .sum = 0x052642, .length = 26,
         .foldable = 1},
    {0x06035B14, Azel_06035B14, .sum = 0x05CBE1, .length = 37,
         .foldable = 1},
    {0x06035F00, Azel_06035F00, .sum = 0x04AC41, .length = 35,
         .foldable = 1},
    {0x06035F04, Azel_06035F04, .sum = 0x036FCE, .length = 33,
         .foldable = 1},
    {0x0603A22C, Azel_0603A22C, Azel_0603A22C_detect},
    {0x0603A242, Azel_0603A242, .sum = 0x11703E, .length = 49},
    {0x0603DD6E, Azel_0603DD6E, .sum = 0x0BB96E, .length = 31},
    {0x0605444C, NULL,          .sum = 0x102906, .length = 56,
         .hint_constant_reg = 1<<12},
    {0x06058910, NULL,          .sum = 0x0F2F81, .length = 52,
         .hint_constant_reg = 1<<12},
    {0x06059068, NULL,          .sum = 0x42DE2F, .length = 196,
         .hint_constant_reg = 1<<11},
    {0x0605906E, NULL,          .sum = 0x40F2FD, .length = 193,
         .hint_constant_reg = 1<<11},
    {0x060693AE, NULL,          .sum = 0x452D32, .length = 237,
         .hint_constant_reg = 1<<14},
    {0x0606B7E4, NULL,          .sum = 0x5AFDCE, .length = 292,
         .hint_constant_reg = 1<<10},
    {0x0606B898, NULL,          .sum = 0x2FFF9D, .length = 155,
         .hint_constant_reg = 1<<10 | 1<<12},
    {0x06080280, NULL,          .sum = 0x22D6B8, .length = 134,
         .hint_constant_reg = 1<<13},
    {0x0608DE80, NULL,          .sum = 0x114129, .length = 53,
         .hint_constant_reg = 1<<13},
    {0x060A03FE, NULL,          .sum = 0x4D786E, .length = 246,
         .hint_constant_reg = 1<<10 | 1<<14},

};

/*************************************************************************/

#endif  // ENABLE_JIT

/*************************************************************************/
/************************** Interface function ***************************/
/*************************************************************************/

/**
 * saturn_optimize_sh2:  Search for and return, if available, a native
 * implementation of the SH-2 routine starting at the given address.
 *
 * [Parameters]
 *        state: Processor state block pointer
 *      address: Address from which to translate
 *        fetch: Pointer corresponding to "address" from which opcodes can
 *                  be fetched
 *     func_ret: Pointer to variable to receive address of native function
 *                  implementing this routine if return value is nonzero
 *     for_fold: Nonzero if the callback is being called to look up a
 *                  subroutine for folding, zero if being called for a
 *                  full block translation
 * [Return value]
 *     Length of translated block in instructions (nonzero) if a native
 *     function is returned, else zero
 */
unsigned int saturn_optimize_sh2(SH2State *state, uint32_t address,
                                 const uint16_t *fetch,
                                 SH2NativeFunctionPointer *func_ret,
                                 int for_fold)
{
#ifdef ENABLE_JIT

    unsigned int i;
    for (i = 0; i < lenof(hand_tuned_table); i++) {

        if (hand_tuned_table[i].address != address) {
            continue;
        }
        if (for_fold && !hand_tuned_table[i].foldable) {
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

        if (hand_tuned_table[i].execute == NULL) {

            uint32_t bitmask;
            unsigned int bit;

            for (bitmask = hand_tuned_table[i].hint_constant_reg, bit = 0;
                 bitmask != 0;
                 bitmask &= ~(1<<bit), bit++
            ) {
                if (bitmask & (1<<bit)) {
                    sh2_optimize_hint_constant_register(state, bit);
                }
            }

            for (bitmask = hand_tuned_table[i].hint_data_pointer_reg, bit = 0;
                 bitmask != 0;
                 bitmask &= ~(1<<bit), bit++
            ) {
                if (bitmask & (1<<bit)) {
                    sh2_optimize_hint_data_pointer_register(state, bit);
                }
            }

            for (bitmask = hand_tuned_table[i].hint_data_pointer_load, bit = 0;
                 bitmask != 0;
                 bitmask &= ~(1<<bit), bit++
            ) {
                if (bitmask & (1<<bit)) {
                    sh2_optimize_hint_data_pointer_load(state, address+bit*2);
                }
            }

            return 0;

        } else {  // hand_tuned_table[i].execute != NULL

#ifdef JIT_DEBUG_TRACE
            unsigned int n;
            for (n = 0; n < num_insns; n++) {
                char tracebuf[100];
                SH2Disasm(address + n*2, fetch[n], 0, tracebuf);
                fprintf(stderr, "%08X: %04X  %s\n",
                        address + n*2, fetch[n], tracebuf+12);
            }
#endif
            *func_ret = hand_tuned_table[i].execute;
            return num_insns;

        }

    }  // for (i = 0; i < lenof(hand_tuned_table); i++)

#endif  // ENABLE_JIT

    return 0;  // No match found
}

/*************************************************************************/
/***************** Case-specific optimization functions ******************/
/*************************************************************************/

#ifdef ENABLE_JIT  // Through end of file

/*************************************************************************/

/**** Saturn BIOS optimizations ****/

/*-----------------------------------------------------------------------*/

/* 0x25AC: Peripheral detection(?) loop */

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

/* 0x2EFA: CD read loop */

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
     * restores. */
    Cs2RapidCopyT2(ptr, left);
}

/*-----------------------------------------------------------------------*/

/* 0x3BC6: CD status read routine */

static FASTCALL void BIOS_00003BC6(SH2State *state)
{
    /* With the current CS2 implementation, this all amounts to a simple
     * read of registers CR1-CR4, but let's not depend on that behavior. */

    state->R[0] = -3;  // Default return value (error)

    unsigned int try;
    for (try = 0; try < 100; try++, state->cycles += 67) {
        const unsigned int CR1 = Cs2ReadWord(0x90018);
        const unsigned int CR2 = Cs2ReadWord(0x9001C);
        const unsigned int CR3 = Cs2ReadWord(0x90020);
        const unsigned int CR4 = Cs2ReadWord(0x90024);
        HRAM_STOREW(state->R[4], 0, CR1);
        HRAM_STOREW(state->R[4], 2, CR2);
        HRAM_STOREW(state->R[4], 4, CR3);
        HRAM_STOREW(state->R[4], 6, CR4);

        const unsigned int CR1_test = Cs2ReadWord(0x90018);
        const unsigned int CR2_test = Cs2ReadWord(0x9001C);
        const unsigned int CR3_test = Cs2ReadWord(0x90020);
        const unsigned int CR4_test = Cs2ReadWord(0x90024);
        if (CR1_test==CR1 && CR2_test==CR2 && CR3_test==CR3 && CR4_test==CR4) {
            state->R[0] = 0;
            state->cycles += 65;
            break;
        }
    }

    state->PC = state->PR;
    state->cycles += 15 + 12;
}

/*-----------------------------------------------------------------------*/

/* 0x6001670: Sound RAM load loop (optimized to avoid slowdowns from ME
 * cache issues, since it includes a read-back test) */

static FASTCALL void BIOS_06001670(SH2State *state)
{
    const uint32_t data = MappedMemoryReadLong(state->R[1]);
    state->R[1] += 4;
    MappedMemoryWriteLong(state->R[3], data);
    state->R[3] += 4;
    state->R[6]--;
    if (state->R[6] != 0) {
        state->cycles += 50;
    } else {
        state->PC = state->PR;
        state->cycles += 52;
    }
}

/*-----------------------------------------------------------------------*/

/* 0x6010D22: 3-D intro animation idle loop */

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
    const uint32_t address = HRAM_LOADL(state->PC & -4, 4 + 0x11*4);
    state->R[0] = HRAM_LOADW(address, 0);
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

/* 0x60115A4, etc.: Routine with multiple JSRs and a looping recursive BSR
 * (this helps cover the block switch overhead) */

static FASTCALL void BIOS_060115A4(SH2State *state)
{
    state->R[15] -= 12;
    HRAM_STOREL(state->R[15], 8, state->R[14]);
    HRAM_STOREL(state->R[15], 4, state->R[13]);
    HRAM_STOREL(state->R[15], 0, state->PR);
    state->R[14] = state->R[4];
    state->PR = state->PC + 9*2;

    state->PC = HRAM_LOADL((state->PC + 3*2) & -4, 4 + 0x24*4);
    BIOS_0602E364(state);

    state->PC = HRAM_LOADL(state->R[14], 16);
    state->cycles += 11;
}

/*----------------------------------*/

static int BIOS_060115B6_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    const uint32_t jsr_address = HRAM_LOADL(state->PC & -4, 4 + 0x22*4);
    return checksum(fetch, 5) == 0x3081A
        && BIOS_0602E630_detect(state, jsr_address,
                                fetch + ((jsr_address - address) >> 1))
        ? 5 : 0;
}

static FASTCALL void BIOS_060115B6(SH2State *state)
{
    state->R[4] = state->R[14];
    state->R[13] = 0;
    state->PR = state->PC + 15*2;
    state->PC = HRAM_LOADL(state->PC & -4, 4 + 0x22*4);
    state->cycles += 7;
    return BIOS_0602E630(state);
}

/*----------------------------------*/

static int BIOS_060115D4_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return checksum(fetch-10, 21) == 0x8E03C ? 11 : 0;
}

static FASTCALL void BIOS_060115D4(SH2State *state)
{
    const uint32_t R14_68_address = HRAM_LOADL(state->R[14], 68);
    if (state->R[13] < HRAM_LOADL(R14_68_address, 0)) {
        state->R[4] = HRAM_LOADL(R14_68_address, 4 + 4*state->R[13]);
        state->R[13]++;
        state->PC -= 24*2;  // Recursive call to 0x60115A4 (PR is already set)
        state->cycles += 19;
    } else {
        state->PR = HRAM_LOADL(state->R[15], 0);
        state->R[13] = HRAM_LOADL(state->R[15], 4);
        state->R[14] = HRAM_LOADL(state->R[15], 8);
        state->R[15] += 12;
        state->PC = HRAM_LOADL((state->PC + 8*2) & -4, 4 + 0x17*4);
        state->cycles += 12;
    }
}

/*-----------------------------------------------------------------------*/

/* 0x602E364: Short but difficult-to-optimize data initialization routine */

static FASTCALL void BIOS_0602E364(SH2State *state)
{
    const uint32_t r2 = HRAM_LOADL(state->PC & -4, 0*2 + 4 + 0xF*4);
    const uint32_t r5 = HRAM_LOADL(state->PC & -4, 2*2 + 4 + 0xF*4);
    const uint32_t r0 = HRAM_LOADL(state->PC & -4, 4*2 + 4 + 0xF*4);
    const uint32_t r3 = HRAM_LOADW(r2, 0) + 1;
    HRAM_STOREW(r2, 0, r3);
    const uint32_t r6 = HRAM_LOADL(r0, 0);
    const uint32_t r5_new = r5 + r3*48;
    HRAM_STOREL(r0, 0, r5_new);

    /* Help out the optimizer by telling it we can load multiple values
     * at once. */
    const uint32_t *src = (const uint32_t *)HRAM_PTR(r6);
    uint32_t *dest = (uint32_t *)HRAM_PTR(r5_new);
    unsigned int i;
    for (i = 0; i < 12; i += 4) {
        const uint32_t a = src[i+0];
        const uint32_t b = src[i+1];
        const uint32_t c = src[i+2];
        const uint32_t d = src[i+3];
        dest[i+0] = a;
        dest[i+1] = b;
        dest[i+2] = c;
        dest[i+3] = d;
    }

    state->PC = state->PR;
    state->cycles += 87;
}

/*-----------------------------------------------------------------------*/

/* 0x602E630: Coordinate transformation */

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

    int16_t * const R12_ptr =
        (int16_t *)HRAM_PTR(HRAM_LOADL((state->PC + 11*2) & -4, 4 + 0x7B*4));
    int32_t * const R13_ptr =
        (int32_t *)HRAM_PTR(HRAM_LOADL((state->PC + 12*2) & -4, 4 + 0x7B*4));

    const uint32_t M_R7 = HRAM_LOADL((state->PC + 24*2) & -4, 4 + 0x76*4);
    const int32_t * const M = (const int32_t *)HRAM_PTR(HRAM_LOADL(M_R7, 0));

    const uint32_t R5 = HRAM_LOADL(state->R[4], 56) + 4;
    counter = (int16_t)HRAM_LOADW(R5, -4);
    state->cycles += 30;

    if (counter > 0) {

        /* 0x602E66E */

        state->cycles += 19 + (counter * /*minimum of*/ 110);

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
# define GET_M(i)  ((int64_t)(int32_t)RAM_SWAPL(M[(i)]))
# define DO_MULT(dest_all,dest_z,index) \
    const int32_t dest_z = ((int64_t)in_z * GET_M((index)*4+2)) >> 16;  \
    int32_t dest_all = (((int64_t)in_x * GET_M((index)*4+0)             \
                       + (int64_t)in_y * GET_M((index)*4+1)             \
                       + (int64_t)in_z * GET_M((index)*4+2)) >> 16)     \
                     + GET_M((index)*4+3);
#endif

        const uint32_t testflag = HRAM_LOADL(state->R[4], 20);
        const int32_t *in = (const int32_t *)HRAM_PTR(R5);
        int32_t *out = R13_ptr;
        int16_t *coord_out = R12_ptr;

        do {
            const int32_t in_x = RAM_SWAPL(in[0]);
            const int32_t in_y = RAM_SWAPL(in[1]);
            const int32_t in_z = RAM_SWAPL(in[2]);

            DO_MULT(out_z, zz, 2);
            if (out_z < 0 && testflag) {
                out_z += out_z >> 3;
            }
            out[ 2] = RAM_SWAPL(out_z);
            out[14] = RAM_SWAPL(out_z - (zz<<1));

            DO_MULT(out_x, zx, 0);
            out[ 0] = RAM_SWAPL(out_x);
            out[12] = RAM_SWAPL(out_x - (zx<<1));

            DO_MULT(out_y, zy, 1);
            out[ 1] = RAM_SWAPL(out_y);
            out[13] = RAM_SWAPL(out_y - (zy<<1));

            /* The result gets truncated to 16 bits here, so we don't need
             * to worry about the 32->24 bit precision loss with floats.
             * (There are only a few pixels out of place during the entire
             * animation as a result of rounding error.) */
            const float coord_mult = 192.0f / out_z;
            *coord_out++ = (int16_t)ifloorf(out_x * coord_mult);
            *coord_out++ = (int16_t)ifloorf(out_y * coord_mult);

            in += 3;
            out += 3;
            counter -= 2;
        } while (counter > 0);

#undef GET_M
#undef DO_MULT

    }  // if (counter > 0)

    /* 0x602E840 */

    /* Offset for second-half local data accesses */
    const int UE_PC_offset = (BIOS_0602E630_is_UE ? -12*2 : 0);

    const uint32_t R11 = HRAM_LOADL(state->R[4], 64);
    counter = (int16_t)HRAM_LOADW(R11, 0);
    state->cycles += 19;

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
    (((int64_t)(int32_t)RAM_SWAPL((v)[0]) * (int64_t)(int32_t)(x)       \
    + (int64_t)(int32_t)RAM_SWAPL((v)[1]) * (int64_t)(int32_t)(y)       \
    + (int64_t)(int32_t)RAM_SWAPL((v)[2]) * (int64_t)(int32_t)(z)) >> 16)
# define DOT3_32(v,x,y,z)                                               \
    (((int64_t)(int32_t)RAM_SWAPL((v)[0]) * (int64_t)(int32_t)(x)       \
    + (int64_t)(int32_t)RAM_SWAPL((v)[1]) * (int64_t)(int32_t)(y)       \
    + (int64_t)(int32_t)RAM_SWAPL((v)[2]) * (int64_t)(int32_t)(z)) >> 32)
#endif

        state->cycles += 68 + 95*(counter-2);

        /* 0x602E850 */

        const int32_t *in = (const int32_t *)(HRAM_PTR(R11) + 28);
        const int32_t *coord_in = &R13_ptr[12];
        const uint32_t out_address =
            HRAM_LOADL((state->PC + 264*2 + UE_PC_offset) & -4, 4 + 0xA2*4);
        int32_t *out = (int32_t *)HRAM_PTR(out_address);
        int16_t *coord_out = &R12_ptr[8];

        const uint16_t *R6_ptr =
            (const uint16_t *)(HRAM_PTR(HRAM_LOADL(state->R[4], 60)) + 4);
        const uint32_t flag_address =
            HRAM_LOADL((state->PC + 348*2 + UE_PC_offset) & -4, 4 + 0x79*4);
        int16_t *flag = (int16_t *)HRAM_PTR(flag_address);

        {
            const int32_t M_2  = RAM_SWAPL(M[ 2]);
            const int32_t M_6  = RAM_SWAPL(M[ 6]);
            const int32_t M_10 = RAM_SWAPL(M[10]);

            out[0] = RAM_SWAPL(-M_2);
            out[1] = RAM_SWAPL(-M_6);
            out[2] = RAM_SWAPL(-M_10);
            const int32_t *in0_0 =
                (const int32_t *)((uintptr_t)R13_ptr + R6_ptr[3]);
            R6_ptr += 10;
            const int32_t test_0 = DOT3_32(in0_0, -M_2, -M_6, -M_10);
            *flag++ = (test_0 < 0);
            out += 3;
            counter--;

            out[0] = RAM_SWAPL(M_2);
            out[1] = RAM_SWAPL(M_6);
            out[2] = RAM_SWAPL(M_10);
            const int32_t *in0_1 =
                (const int32_t *)((uintptr_t)R13_ptr + R6_ptr[3]);
            R6_ptr += 10;
            const int32_t test_1 = DOT3_32(in0_1, M_2, M_6, M_10);
            *flag++ = (test_1 < 0);
            out += 3;
            counter--;
        }

        do {
            const int32_t in_x = RAM_SWAPL(in[0]);
            const int32_t in_y = RAM_SWAPL(in[1]);
            const int32_t in_z = RAM_SWAPL(in[2]);

            const int32_t out_x = DOT3_16(&M[0], in_x, in_y, in_z);
            const int32_t out_y = DOT3_16(&M[4], in_x, in_y, in_z);
            const int32_t out_z = DOT3_16(&M[8], in_x, in_y, in_z);

            out[0] = RAM_SWAPL(out_x);
            out[1] = RAM_SWAPL(out_y);
            out[2] = RAM_SWAPL(out_z);

            const float coord_mult = 192.0f / (int32_t)RAM_SWAPL(coord_in[2]);
            *coord_out++ =
                (int16_t)ifloorf((int32_t)RAM_SWAPL(coord_in[0]) * coord_mult);
            *coord_out++ =
                (int16_t)ifloorf((int32_t)RAM_SWAPL(coord_in[1]) * coord_mult);
            coord_in += 3;

            const int32_t *in0 =
                (const int32_t *)((uintptr_t)R13_ptr + R6_ptr[3]);
            R6_ptr += 10;
            const int32_t test = DOT3_32(in0, out_x, out_y, out_z);
            *flag++ = (test < 0);

            in += 3;
            out += 3;
            counter--;
        } while (counter > 0);

#undef DOT3_16
#undef DOT3_32

    }  // if (counter > 0)

    /* 0x602E914 */
    /* Note: At this point, all GPRs except R9, R12, R13, and R15 are dead */

    const int16_t *flag = (const int16_t *)HRAM_PTR(
        HRAM_LOADL((state->PC + 572*2 + UE_PC_offset) & -4, 4 + 0x0F*4));
    const int32_t *R1_ptr = (const int32_t *)HRAM_PTR(
        HRAM_LOADL((state->PC + 378*2 + UE_PC_offset) & -4, 4 + 0x6C*4));
    const uint16_t *R6_ptr = (const uint16_t *)HRAM_PTR(
        HRAM_LOADL(state->R[4], 60));
    const int32_t *R7_ptr = (const int32_t *)HRAM_PTR(
        HRAM_LOADL((state->PC + 371*2 + UE_PC_offset) & -4, 4 + 0x6D*4));
    const uint32_t R9 = HRAM_LOADL((state->PC + 9*2) & -4, 4 + 0x7A*4);
    uint16_t *R10_ptr = (uint16_t *)HRAM_PTR(
        HRAM_LOADL((state->PC + 370*2 + UE_PC_offset) & -4, 4 + 0x73*4));

    const int32_t limit = *R6_ptr;
    R6_ptr += 2;
    state->cycles += 13;

    for (counter = 0; counter < limit; counter++, R7_ptr += 3, R6_ptr += 10) {

        /* 0x602EAA8 */

        if (!flag[counter]) {
            state->cycles += 15;
            continue;
        }

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
           ((int64_t) (int32_t)RAM_SWAPL(R7_ptr[0]) * (int32_t)RAM_SWAPL(R1_ptr[0])
          + (int64_t) (int32_t)RAM_SWAPL(R7_ptr[1]) * (int32_t)RAM_SWAPL(R1_ptr[1])
          + (int64_t) (int32_t)RAM_SWAPL(R7_ptr[2]) * (int32_t)RAM_SWAPL(R1_ptr[2])
          ) >> 16;
        const int32_t R2_temp = (mac + 0x10000) >> 10;
        const int32_t R2 = R2_temp < 0 ? 0 : R2_temp > 127 ? 127 : R2_temp;
#endif
        const uint32_t R2_tableaddr = RAM_SWAPL(*(const uint32_t *)&R6_ptr[8]);
        const uint16_t *R2_table = (const uint16_t *)HRAM_PTR(R2_tableaddr);

        const uint32_t R9_address = HRAM_LOADL(R9, 0);
        HRAM_STOREL(R9, 0, R9_address + 48);
        HRAM_STOREW(R9_address, 0, *R6_ptr);
        HRAM_STOREW(R9_address, 42, R2_table[R2]);

        uint32_t * const R9_data32 = (uint32_t *)(HRAM_PTR(R9_address) + 4);
        int32_t R3;
        switch (R6_ptr[1] & 0xFF) {
          case 0x00:
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 0];
            R9_data32[1] = *(uint32_t *)&R12_ptr[ 2];
            R9_data32[2] = *(uint32_t *)&R12_ptr[ 4];
            R9_data32[3] = *(uint32_t *)&R12_ptr[ 6];
            R3 = RAM_SWAPL(R13_ptr[5]) + RAM_SWAPL(R13_ptr[11]);
            break;

          case 0x30:
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 8];
            R9_data32[1] = *(uint32_t *)&R12_ptr[14];
            R9_data32[2] = *(uint32_t *)&R12_ptr[12];
            R9_data32[3] = *(uint32_t *)&R12_ptr[10];
            R3 = RAM_SWAPL(R13_ptr[17]) + RAM_SWAPL(R13_ptr[23]);
            break;

          case 0x60:
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 0];
            R9_data32[1] = *(uint32_t *)&R12_ptr[ 8];
            R9_data32[2] = *(uint32_t *)&R12_ptr[10];
            R9_data32[3] = *(uint32_t *)&R12_ptr[ 2];
            R3 = RAM_SWAPL(R13_ptr[5]) + RAM_SWAPL(R13_ptr[14]);
            break;

          case 0x90:
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 4];
            R9_data32[1] = *(uint32_t *)&R12_ptr[12];
            R9_data32[2] = *(uint32_t *)&R12_ptr[14];
            R9_data32[3] = *(uint32_t *)&R12_ptr[ 6];
            R3 = RAM_SWAPL(R13_ptr[8]) + RAM_SWAPL(R13_ptr[23]);
            break;

          case 0xC0:
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 2];
            R9_data32[1] = *(uint32_t *)&R12_ptr[10];
            R9_data32[2] = *(uint32_t *)&R12_ptr[12];
            R9_data32[3] = *(uint32_t *)&R12_ptr[ 4];
            R3 = RAM_SWAPL(R13_ptr[5]) + RAM_SWAPL(R13_ptr[20]);
            break;

          default:  // case 0xF0
            R9_data32[0] = *(uint32_t *)&R12_ptr[ 0];
            R9_data32[1] = *(uint32_t *)&R12_ptr[ 6];
            R9_data32[2] = *(uint32_t *)&R12_ptr[14];
            R9_data32[3] = *(uint32_t *)&R12_ptr[ 8];
            R3 = RAM_SWAPL(R13_ptr[11]) + RAM_SWAPL(R13_ptr[14]);
            break;
        }

        if (!BIOS_0602E630_is_UE && R3 < -0x30000 && (R6_ptr[1] & 0xFF00)) {
            R3 = -R3;
        }
        R3 >>= 1;
        uint32_t *R3_buffer = (uint32_t *)HRAM_PTR(
            HRAM_LOADL((state->PC + 558*2 + UE_PC_offset) & -4, 4 + 0x17*4));
        R3_buffer[(*R10_ptr)++] = RAM_SWAPL(R3);

        state->cycles += 39 + /*approximately*/ 54;
    }

    /* 0x602EAB8 */

    state->PC = state->PR;
    state->cycles += 10;
}

/*************************************************************************/

/**** Azel: Panzer Dragoon RPG (JP) optimizations ****/

/*-----------------------------------------------------------------------*/

/* Common color calculation logic used by several routines */

static uint32_t Azel_color_calc(const int16_t *local_ptr,
                                const int16_t *r4_ptr, const int16_t *r5_ptr,
                                int32_t r, int32_t g, int32_t b)
{
    int32_t dot = r4_ptr[0] * local_ptr[0]
                + r4_ptr[1] * local_ptr[1]
                + r4_ptr[2] * local_ptr[2];
    if (dot > 0) {
        dot >>= 16;
        b += dot * r5_ptr[0];
        g += dot * r5_ptr[1];
        r += dot * r5_ptr[2];
    }
    return (bound(b, 0, 0x1F00) & 0x1F00) << 2
         | (bound(g, 0, 0x1F00) & 0x1F00) >> 3
         | (bound(r, 0, 0x1F00)         ) >> 8;
}

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
    /* For a comparison against zero, we don't need to swap bytes. */
    if (*(uint32_t *)HRAM_PTR(state->R[14])) {

        /* Help out the optimizer by loading this early. */
        const int32_t cycle_limit = state->cycle_limit;

        /* 0x600FCB0 almost always returns straight to us, so we implement
         * the first part of it here. */
        const uint32_t r3 = 0x604B68C;  // @(0x38,pc)
        if (UNLIKELY(HRAM_LOADB(r3, 0) != 0)) {
            state->PR = state->PC;
            state->PC = 0x600FCB0 + 2*4;
            state->R[3] = r3;
        }

        state->cycles = cycle_limit;

    } else {

        state->SR |= SR_T;
        state->PC += 2*3;
        state->cycles += 3;

    }
}

/*-----------------------------------------------------------------------*/

/* 0x60061F0: RBG0 parameter generation for sky/ground backgrounds */

static FASTCALL void Azel_060061F0(SH2State *state)
{
    int32_t r4 = state->R[4];
    int32_t r5 = state->R[5];
    int32_t delta = state->R[6];
    uint32_t counter = state->R[7];
    const uint32_t out_address = HRAM_LOADL(state->R[15], 0);
    int32_t *out = (int32_t *)HRAM_PTR(out_address);

    if (r4 < 0) {
        r4 = -r4;
        r5 = -r5;
        delta = -delta;
    }

    const float r4_scaled = r4 * 65536.0f;
    const int32_t r4_test = r4 >> 14;  // See below for why we use this.

    if (delta == 0) {
        /* No horizon scaling is taking place.  Note that r5 should be
         * checked against zero here (and in similar cases below), but
         * we're a bit more conservative in order to avoid FPU overflow
         * errors on conversion to integer; since only the low 24 bits of
         * the 16.16 fixed point result are significant, this isn't a
         * problem in practice--it will only come up in cases where the
         * horizon line almost exactly coincides with a screen line. */
        if (r5 > r4_test) {
            const int32_t quotient = ifloorf(r4_scaled / (float)r5);
            state->PC = state->PR;
            state->cycles += 72 + (counter * 5);
            for (; counter != 0; counter--, out++) {
                *out = RAM_SWAPL(quotient);
            }
        } else {
            state->PC = state->PR;
            state->cycles += 30 + (counter * 5);
            for (; counter != 0; counter--, out++) {
                *out = -1;
            }
        }
        return;
    }

    const int32_t r5_final = r5 - (delta * (counter-1));

    if (delta > 0 ? (r5 <= r4_test) : (r5_final <= r4_test)) {
        /* The entire background layer is outside the horizon area. */
        state->PC = state->PR;
        state->cycles += (delta > 0 ? 40 : 43) + (counter * 5);
        for (; counter != 0; counter--, out++) {
            *out = -1;
        }
        return;
    }

    if (delta > 0 && r5_final <= r4_test) {
        /* The bottom of the background layer is outside the horizon area. */
        const uint32_t partial_count = counter - ((r5 - r4_test) / delta) + 1;
        state->cycles += 88 + (partial_count * 5);
        counter -= partial_count;
        int32_t *out_temp = out + counter;
        uint32_t i;
        for (i = partial_count; i != 0; i--, out_temp++) {
            *out_temp = -1;
        }
    } else if (delta < 0 && r5 <= r4_test) {
        /* The top of the background layer is outside the horizon area. */
        const uint32_t partial_count = (r5 - r4_test) / delta + 1;
        state->cycles += 86 + (partial_count * 5);
        r5 -= delta * partial_count;
        counter -= partial_count;
        uint32_t i;
        for (i = partial_count; i != 0; i--, out++) {
            *out = -1;
        }
    } else {
        /* The entire background layer is within the horizon area. */
        state->cycles += (delta > 0 ? 39 : 42);
    }

    state->PC = state->PR;
    state->cycles += 15 + (counter * 32) + (counter%2 ? 10 : 0);

    for (; counter != 0; counter--, out++) {
        *out = RAM_SWAPL(ifloorf(r4_scaled / (float)r5));
        r5 -= delta;
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
    int32_t test = (int8_t)(((SH2_struct *)(state->userdata))->onchip.FTCSR);

    /* Again, help out the optimizer.  This "volatile" actually speeds
     * things up, because it forces the compiler to load this during the
     * load delay slots of "test", rather than delaying the load so far
     * that the subsequent store to state->cycles ends up stalling.
     * (Granted, this is properly an optimizer bug that ought to be fixed
     * in GCC.  But since GCC's scheduler is so complex it might as well
     * be random chaos, what else can we do?) */
    int32_t cycle_limit =
#ifdef __mips__
        *(volatile int32_t *)&
#endif
        state->cycle_limit;

    if (LIKELY(test >= 0)) {
        /* This loop has already been executed once in the fall-in case,
         * so SR.T will already be set. */
        state->cycles = cycle_limit;
    } else {
        state->R[0] = test & 0xFF;
        state->SR &= ~SR_T;
        state->PC += 2*4;
        state->cycles += 4;
    }
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
    const uint32_t *src = (const uint32_t *)HRAM_PTR(src_addr);
    uint32_t *dest = (uint32_t *)VDP2_PTR(dest_addr);

    uint32_t i;
    for (i = 0; i < size; i += 16, src += 4, dest += 4) {
        const uint32_t word0 = src[0];
        const uint32_t word1 = src[1];
        const uint32_t word2 = src[2];
        const uint32_t word3 = src[3];
        dest[0] = RAM_VDP_SWAPL(word0);
        dest[1] = RAM_VDP_SWAPL(word1);
        dest[2] = RAM_VDP_SWAPL(word2);
        dest[3] = RAM_VDP_SWAPL(word3);
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

/* 0x601EC20, 0x601EC3C: Vertex manipulation routines */

static void Azel_0601EC5A(SH2State *state);
static void Azel_0601ED46(SH2State *state);
static void Azel_0601ED58(SH2State *state);
static void Azel_0601EDCA(SH2State *state);
static void Azel_0601EDDC(SH2State *state);
static void Azel_0601F58A(SH2State *state);
static void Azel_0601F5EE(SH2State *state);

/*----------------------------------*/

static FASTCALL void Azel_0601EC20(SH2State *state)
{
    const uint32_t saved_PR = state->PR;

    state->R[11] = LRAM_LOADL(state->R[4], 0);
    state->R[13] = LRAM_LOADL(state->R[4], 4);
    state->R[4] += 8;
    while (state->R[11] != state->R[13]) {
        Azel_0601F58A(state);
        if (state->SR & SR_T) {
            state->cycles += 6;
        } else {
            Azel_0601EC5A(state);
            state->cycles += 7;
        }
        state->R[11] = LRAM_LOADL(state->R[4], 0);
        state->R[13] = LRAM_LOADL(state->R[4], 4);
        state->R[4] += 8;
    }

    state->cycles += 11;
    state->PC = saved_PR;
}

/*----------------------------------*/

static FASTCALL void Azel_0601EC3C(SH2State *state)
{
    const uint32_t saved_PR = state->PR;

    state->R[11] = LRAM_LOADL(state->R[4], 0);
    state->R[13] = LRAM_LOADL(state->R[4], 4);
    state->R[4] += 8;
    while (state->R[11] != state->R[13]) {
        Azel_0601F5EE(state);
        if (state->SR & SR_T) {
            state->cycles += 12;
        } else {
            Azel_0601EC5A(state);
            state->cycles += 13;
        }
        state->R[11] = LRAM_LOADL(state->R[4], 0);
        state->R[13] = LRAM_LOADL(state->R[4], 4);
        state->R[4] += 8;
    }

    state->cycles += 11;
    state->PC = saved_PR;
}

/*----------------------------------*/

static void Azel_0601EC5A(SH2State *state)
{
    uint32_t *r13_ptr = (uint32_t *)HRAM_PTR(state->R[13]);

    r13_ptr[0] = RAM_SWAPL(state->R[14] - 32);

    if (!(state->R[9] & 3)) {
        state->PC = state->PR;
        state->cycles += 9;
        return;
    }

    const uint32_t color_addr = RAM_SWAPL(r13_ptr[4]);
    r13_ptr[4] = RAM_SWAPL(color_addr - 8);
    VDP1_STOREW(state->R[14], 28, color_addr >> 3);
    state->cycles += 13;

    switch (state->R[9] & 3) {

      case 1: {
        Azel_0601ED46(state);
        const uint32_t color = (state->R[0] << 16) | (state->R[0] & 0xFFFF);
        int32_t *out = (int32_t *)VDP1_PTR(color_addr);
        out[0] = out[1] = VDP_SWAPL(color);
        state->R[4] += 2;
        state->cycles += 24;
        break;
      }  // case 1

      case 2: {
        Azel_0601EDCA(state);
        const int32_t a = VDP_SWAPW(state->R[0]);
        Azel_0601EDDC(state);
        const int32_t b = VDP_SWAPW(state->R[0]);
        Azel_0601EDDC(state);
        const int32_t c = VDP_SWAPW(state->R[0]);
        Azel_0601EDDC(state);
        const int32_t d = VDP_SWAPW(state->R[0]);

        int16_t *out = (int16_t *)VDP1_PTR(color_addr);
        switch (state->R[8]) {
          case 0:
            out[0] = a; out[1] = b; out[2] = c; out[3] = d;
            state->cycles += 35;
            break;
          case 1:
            out[1] = a; out[0] = b; out[3] = c; out[2] = d;
            state->cycles += 41;
            break;
          case 2:
            out[2] = a; out[3] = b; out[0] = c; out[1] = d;
            state->cycles += 43;
            break;
          default:
            out[3] = a; out[2] = b; out[1] = c; out[0] = d;
            state->cycles += 41;
            break;
        }
        break;
      }  // case 2

      default: {
        Azel_0601ED46(state);
        const int32_t a = VDP_SWAPW(state->R[0]);
        Azel_0601ED58(state);
        const int32_t b = VDP_SWAPW(state->R[0]);
        Azel_0601ED58(state);
        const int32_t c = VDP_SWAPW(state->R[0]);
        Azel_0601ED58(state);
        const int32_t d = VDP_SWAPW(state->R[0]);

        int16_t *out = (int16_t *)VDP1_PTR(color_addr);
        switch (state->R[8]) {
          case 0:
            out[0] = a; out[1] = b; out[2] = c; out[3] = d;
            state->cycles += 36;
            break;
          case 1:
            out[1] = a; out[0] = b; out[3] = c; out[2] = d;
            state->cycles += 42;
            break;
          case 2:
            out[2] = a; out[3] = b; out[0] = c; out[1] = d;
            state->cycles += 44;
            break;
          default:
            out[3] = a; out[2] = b; out[1] = c; out[0] = d;
            state->cycles += 42;
            break;
        }
      }  // default (case 3)

    }  // switch (state->R[9] & 3)

    state->PC = state->PR;
}

/*----------------------------------*/

static void Azel_0601ED46(SH2State *state)
{
    const int16_t *base_ptr =
        (const int16_t *)(HRAM_PTR(0x601EF2C) + (state->R[1]>>7 & -8));
    state->R[12] = base_ptr[0];
    state->R[11] = base_ptr[1];
    state->R[10] = base_ptr[2];
    state->cycles += 9;

    return Azel_0601ED58(state);
}

/*----------------------------------*/

static void Azel_0601ED58(SH2State *state)
{
    state->R[0] = Azel_color_calc((const int16_t *)HRAM_PTR(0x601EF10),
                                  (const int16_t *)LRAM_PTR(state->R[4]),
                                  (const int16_t *)HRAM_PTR(0x601EF24),
                                  state->R[12], state->R[11], state->R[10]);
    state->R[4] += 6;
    state->cycles += 52;  // Approximate
}

/*----------------------------------*/

static void Azel_0601EDCA(SH2State *state)
{
    const int16_t *base_ptr =
        (const int16_t *)(HRAM_PTR(0x601EF2C) + (state->R[1]>>7 & 0xF8));
    state->R[12] = base_ptr[0];
    state->R[11] = base_ptr[1];
    state->R[10] = base_ptr[2];
    state->cycles += 9;

    return Azel_0601EDDC(state);
}

/*----------------------------------*/

static void Azel_0601EDDC(SH2State *state)
{
    const int16_t *r4_ptr = (const int16_t *)LRAM_PTR(state->R[4]);
    state->R[0] = Azel_color_calc((const int16_t *)HRAM_PTR(0x601EF10),
                                  r4_ptr,
                                  (const int16_t *)HRAM_PTR(0x601EF24),
                                  state->R[12] + r4_ptr[5],
                                  state->R[11] + r4_ptr[4],
                                  state->R[10] + r4_ptr[3]);
    state->R[4] += 12;
    state->cycles += 57;  // Approximate
}

/*-----------------------------------------------------------------------*/

/* 0x601F30E: Coordinate transform routine */

static FASTCALL void Azel_0601F30E(SH2State *state)
{
    const int32_t *r4_ptr = state->R[4] & 0x06000000
        ? (const int32_t *)HRAM_PTR(state->R[4])
        : (const int32_t *)LRAM_PTR(state->R[4]);
    const int32_t *r5_ptr = (const int32_t *)HRAM_PTR(state->R[5]);
    const int32_t *r6_ptr = (const int32_t *)HRAM_PTR(state->R[6]);

    /* 0x601F38E */

    const int32_t r6_x = RAM_SWAPL(r6_ptr[6]);
    const int32_t M11 = (r6_x * (int32_t)RAM_SWAPL(r5_ptr[0])) >> 12;
    const int32_t M12 = (r6_x * (int32_t)RAM_SWAPL(r5_ptr[1])) >> 12;
    const int32_t M13 = (r6_x * (int32_t)RAM_SWAPL(r5_ptr[2])) >> 12;
    const int32_t M14 =  r6_x * (int32_t)RAM_SWAPL(r5_ptr[3]);

    const int32_t r6_y = -RAM_SWAPL(r6_ptr[7]);
    const int32_t M21 = (r6_y * (int32_t)RAM_SWAPL(r5_ptr[4])) >> 12;
    const int32_t M22 = (r6_y * (int32_t)RAM_SWAPL(r5_ptr[5])) >> 12;
    const int32_t M23 = (r6_y * (int32_t)RAM_SWAPL(r5_ptr[6])) >> 12;
    const int32_t M24 =  r6_y * (int32_t)RAM_SWAPL(r5_ptr[7]);

    const int32_t M31 = (int32_t)RAM_SWAPL(r5_ptr[ 8]) >> 4;
    const int32_t M32 = (int32_t)RAM_SWAPL(r5_ptr[ 9]) >> 4;
    const int32_t M33 = (int32_t)RAM_SWAPL(r5_ptr[10]) >> 4;
    const int32_t M34 = (int32_t)RAM_SWAPL(r5_ptr[11]) << 8;

    uint32_t counter = RAM_SWAPL(r4_ptr[1]);
    const uint32_t in_address = RAM_SWAPL(r4_ptr[2]);

    state->cycles += 111;

    /* 0x601F314 */

    state->cycles += 8 + 56*counter;

    const int16_t *in = in_address & 0x06000000
        ? (const int16_t *)HRAM_PTR(in_address)
        : (const int16_t *)LRAM_PTR(in_address);
    int16_t *out = (int16_t *)HRAM_PTR(state->R[7]);

    do {
        const int32_t out_x = M11*in[0] + M12*in[1] + M13*in[2] + M14;
        const int32_t out_y = M21*in[0] + M22*in[1] + M23*in[2] + M24;
        const int32_t out_z = M31*in[0] + M32*in[1] + M33*in[2] + M34;
        const float coord_mult = 256.0f / out_z;
        out[0] = ifloorf(coord_mult * out_x);
        out[1] = ifloorf(coord_mult * out_y);
        *(int32_t *)&out[2] = RAM_SWAPL(out_z);
        in += 3;
        out += 4;
    } while (--counter != 0);

    state->R[4] += 12;
    state->PC = state->PR;
}

/*-----------------------------------------------------------------------*/

/* 0x601F3F4: Coordinate transform routine with boundary checks */

static FASTCALL void Azel_0601F3F4(SH2State *state)
{
    const int32_t *r4_ptr = state->R[4] & 0x06000000
        ? (const int32_t *)HRAM_PTR(state->R[4])
        : (const int32_t *)LRAM_PTR(state->R[4]);
    const int32_t *r5_ptr = (const int32_t *)HRAM_PTR(state->R[5]);
    const int32_t *r6_ptr = (const int32_t *)HRAM_PTR(state->R[6]);

    /* 0x601F38E */

    const int32_t r6_x = RAM_SWAPL(r6_ptr[6]);
    const int32_t M11 = (r6_x * (int32_t)RAM_SWAPL(r5_ptr[0])) >> 12;
    const int32_t M12 = (r6_x * (int32_t)RAM_SWAPL(r5_ptr[1])) >> 12;
    const int32_t M13 = (r6_x * (int32_t)RAM_SWAPL(r5_ptr[2])) >> 12;
    const int32_t M14 =  r6_x * (int32_t)RAM_SWAPL(r5_ptr[3]);

    const int32_t r6_y = -RAM_SWAPL(r6_ptr[7]);
    const int32_t M21 = (r6_y * (int32_t)RAM_SWAPL(r5_ptr[4])) >> 12;
    const int32_t M22 = (r6_y * (int32_t)RAM_SWAPL(r5_ptr[5])) >> 12;
    const int32_t M23 = (r6_y * (int32_t)RAM_SWAPL(r5_ptr[6])) >> 12;
    const int32_t M24 =  r6_y * (int32_t)RAM_SWAPL(r5_ptr[7]);

    const int32_t M31 = (int32_t)RAM_SWAPL(r5_ptr[ 8]) >> 4;
    const int32_t M32 = (int32_t)RAM_SWAPL(r5_ptr[ 9]) >> 4;
    const int32_t M33 = (int32_t)RAM_SWAPL(r5_ptr[10]) >> 4;
    const int32_t M34 = (int32_t)RAM_SWAPL(r5_ptr[11]) << 8;

    uint32_t counter = RAM_SWAPL(r4_ptr[1]);
    const uint32_t in_address = RAM_SWAPL(r4_ptr[2]);

    state->cycles += 111;

    /* 0x601F3FA */

    state->cycles += 7 + 63*counter;

    const int16_t *in = in_address & 0x06000000
        ? (const int16_t *)HRAM_PTR(in_address)
        : (const int16_t *)LRAM_PTR(in_address);
    int16_t *out = (int16_t *)HRAM_PTR(state->R[7]);

    do {
        const int32_t out_x = M11*in[0] + M12*in[1] + M13*in[2] + M14;
        const int32_t out_y = M21*in[0] + M22*in[1] + M23*in[2] + M24;
        const int32_t out_z = M31*in[0] + M32*in[1] + M33*in[2] + M34;
        *(int32_t *)&out[2] = RAM_SWAPL(out_z);
        *(int32_t *)&out[4] = RAM_SWAPL(out_x);
        *(int32_t *)&out[6] = RAM_SWAPL(out_y);
        uint32_t clip_flags = (out_z >= RAM_SWAPL(r6_ptr[5]) << 8) << 5
                            | (out_z <  RAM_SWAPL(r6_ptr[4]) << 8) << 4;
        if (!(clip_flags & 0x10)) {
            const float coord_mult = 256.0f / out_z;
            out[0] = ifloorf(coord_mult * out_x);
            out[1] = ifloorf(coord_mult * out_y);
            clip_flags |= (out[1] > ((const int16_t *)r6_ptr)[4]) << 3
                       |  (out[1] < ((const int16_t *)r6_ptr)[5]) << 2
                       |  (out[0] > ((const int16_t *)r6_ptr)[7]) << 1
                       |  (out[0] < ((const int16_t *)r6_ptr)[6]) << 0;
            state->cycles += 19;
        }
        *(uint32_t *)&out[12] = RAM_SWAPL(clip_flags);
        in += 3;
        out += 16;
    } while (--counter != 0);

    state->R[4] += 12;
    state->PC = state->PR;
}

/*-----------------------------------------------------------------------*/

/* 0x601F58A, etc.: Convoluted, intertwined computation routines */

static void Azel_0601F5A6(SH2State *state);
static void Azel_0601F5D2(SH2State *state);
static void Azel_0601F60E(SH2State *state);

static ALWAYS_INLINE void Azel_0601F762(
    SH2State *state, const uint32_t * const r10_ptr,
    const uint32_t * const r11_ptr, const uint32_t * const r12_ptr,
    const uint32_t * const r13_ptr, uint32_t * const r14_ptr,
    const uint32_t r4_0);

static int Azel_0601F824(SH2State *state, uint32_t mask,
                         int (*clipfunc)(SH2State *state));
static int Azel_0601F93A(SH2State *state);
static int Azel_0601F948(SH2State *state);
static ALWAYS_INLINE int Azel_0601F950(SH2State *state, const int32_t r8_x,
                                       const int32_t x_lim);
static int Azel_0601F988(SH2State *state);
static int Azel_0601F996(SH2State *state);
static ALWAYS_INLINE int Azel_0601F99E(SH2State *state, const int32_t r8_y,
                                       const int32_t y_lim);
static int Azel_0601F9D6(SH2State *state);

/*----------------------------------*/

static void Azel_0601F58A(SH2State *state)
{
    const uint32_t r11 = state->R[11] << 3;
    const uint32_t r13 = state->R[13] << 3;
    state->R[10] = state->R[7] + (r11 >> 16);
    state->R[11] = state->R[7] + (r11 & 0xFFFF);
    state->R[12] = state->R[7] + (r13 >> 16);
    state->R[13] = state->R[7] + (r13 & 0xFFFF);
    state->cycles += 14;
    return Azel_0601F5A6(state);
}

static FASTCALL void Azel_0601F58C(SH2State *state)
{
    /* This is the same as 0x601F58A, but some callers execute the first
     * instruction in the BSR/JSR delay slot and jump here instead. */
    const uint32_t r11 = state->R[11] << 1;
    const uint32_t r13 = state->R[13] << 3;
    state->R[10] = state->R[7] + (r11 >> 16);
    state->R[11] = state->R[7] + (r11 & 0xFFFF);
    state->R[12] = state->R[7] + (r13 >> 16);
    state->R[13] = state->R[7] + (r13 & 0xFFFF);
    state->cycles += 13;
    return Azel_0601F5A6(state);
}

static void Azel_0601F5A6(SH2State *state)
{
    const int16_t *r10_ptr = (const int16_t *)HRAM_PTR(state->R[10]);
    const int16_t *r11_ptr = (const int16_t *)HRAM_PTR(state->R[11]);
    const int16_t *r12_ptr = (const int16_t *)HRAM_PTR(state->R[12]);
    const int16_t *r13_ptr = (const int16_t *)HRAM_PTR(state->R[13]);

    const int32_t a = (r13_ptr[1] - r11_ptr[1]) * (r12_ptr[0] - r10_ptr[0]);
    const int32_t b = (r12_ptr[1] - r10_ptr[1]) * (r13_ptr[0] - r11_ptr[0]);
    if (b > a) {
        state->cycles += 21;
        return Azel_0601F5D2(state);
    }
    state->R[8] = 0;

    state->R[14] = HRAM_LOADL(state->GBR, 0);
    if (UNLIKELY(state->R[14]>>20 != 0x25C)) {
        state->R[2] = LRAM_LOADL(state->R[4], 0);
        state->R[4] += 4;
        state->PC = 0x601F74E;
        state->cycles += 3;
        return;
    }
    uint32_t *r14_ptr = (uint32_t *)VDP1_PTR(state->R[14]);

    r14_ptr[3] = RAM_VDP_SWAPL(*(const uint32_t *)r10_ptr);
    r14_ptr[4] = RAM_VDP_SWAPL(*(const uint32_t *)r11_ptr);
    r14_ptr[5] = RAM_VDP_SWAPL(*(const uint32_t *)r12_ptr);
    r14_ptr[6] = RAM_VDP_SWAPL(*(const uint32_t *)r13_ptr);

    const uint32_t r4_0 = LRAM_LOADL(state->R[4], 0);

    state->cycles += 35;
    return Azel_0601F762(state,
                         (const uint32_t *)r10_ptr,
                         (const uint32_t *)r11_ptr,
                         (const uint32_t *)r12_ptr,
                         (const uint32_t *)r13_ptr, r14_ptr, r4_0);
}

static void Azel_0601F5D2(SH2State *state)
{
    static const uint8_t r4ofs_cycles[8] = {
        12, 20, 60, 36,  // R4 offset
        10, 12, 15, 15,  // Cycle count
    };
    const unsigned int index = LRAM_LOADB(state->R[4], 0) & 0x3;
    state->R[4] += r4ofs_cycles[index];
    state->SR |= SR_T;
    state->PC = state->PR;
    state->cycles += r4ofs_cycles[index+4];
}

/*----------------------------------*/

static int Azel_0601F5F0_detect(SH2State *state, uint32_t address,
                                const uint16_t *fetch)
{
    return checksum(fetch, 56) == 0x11FA8F
        && checksum(fetch + (0x1F824-0x1F5F0)/2, 281) == 0x52D593
        ? 56 : 0;
}

static void Azel_0601F5EE(SH2State *state)
{
    const uint32_t r11 = state->R[11] << 5;
    const uint32_t r13 = state->R[13] << 5;
    state->R[10] = state->R[7] + (r11 >> 16);
    state->R[11] = state->R[7] + (r11 & 0xFFFF);
    state->R[12] = state->R[7] + (r13 >> 16);
    state->R[13] = state->R[7] + (r13 & 0xFFFF);
    state->cycles += 16;
    return Azel_0601F60E(state);
}

static FASTCALL void Azel_0601F5F0(SH2State *state)
{
    /* This is the same as 0x601F5EE, but some callers execute the first
     * instruction in the BSR/JSR delay slot and jump here instead. */
    const uint32_t r11 = state->R[11] << 3;
    const uint32_t r13 = state->R[13] << 5;
    state->R[10] = state->R[7] + (r11 >> 16);
    state->R[11] = state->R[7] + (r11 & 0xFFFF);
    state->R[12] = state->R[7] + (r13 >> 16);
    state->R[13] = state->R[7] + (r13 & 0xFFFF);
    state->cycles += 15;
    return Azel_0601F60E(state);
}

static void Azel_0601F60E(SH2State *state)
{
    uint32_t *r10_ptr = (uint32_t *)HRAM_PTR(state->R[10]);
    uint32_t *r11_ptr = (uint32_t *)HRAM_PTR(state->R[11]);
    uint32_t *r12_ptr = (uint32_t *)HRAM_PTR(state->R[12]);
    uint32_t *r13_ptr = (uint32_t *)HRAM_PTR(state->R[13]);

    if (r10_ptr[6] & r11_ptr[6] & r12_ptr[6] & r13_ptr[6]) {
        state->cycles += 11;
        return Azel_0601F5D2(state);
    }
    if ((r10_ptr[6] | r11_ptr[6] | r12_ptr[6] | r13_ptr[6]) & RAM_SWAPL(0x20)) {
        state->cycles += 17;
        return Azel_0601F5D2(state);
    }
    if (!(r10_ptr[6] | r11_ptr[6] | r12_ptr[6] | r13_ptr[6])) {
        state->cycles += 19;
        return Azel_0601F5A6(state);
    }
    r10_ptr[7] = r10_ptr[6];
    r11_ptr[7] = r11_ptr[6];
    r12_ptr[7] = r12_ptr[6];
    r13_ptr[7] = r13_ptr[6];
    r10_ptr[4] = RAM_SWAPL((int32_t)((int16_t *)r10_ptr)[0]);
    r10_ptr[5] = RAM_SWAPL((int32_t)((int16_t *)r10_ptr)[1]);
    r11_ptr[4] = RAM_SWAPL((int32_t)((int16_t *)r11_ptr)[0]);
    r11_ptr[5] = RAM_SWAPL((int32_t)((int16_t *)r11_ptr)[1]);
    r12_ptr[4] = RAM_SWAPL((int32_t)((int16_t *)r12_ptr)[0]);
    r12_ptr[5] = RAM_SWAPL((int32_t)((int16_t *)r12_ptr)[1]);
    r13_ptr[4] = RAM_SWAPL((int32_t)((int16_t *)r13_ptr)[0]);
    r13_ptr[5] = RAM_SWAPL((int32_t)((int16_t *)r13_ptr)[1]);

    if (Azel_0601F824(state, 0x10, Azel_0601F9D6)) {
        state->cycles += 45;
        return Azel_0601F5D2(state);
    }

    const int32_t r10_x = RAM_SWAPL(r10_ptr[4]);
    const int32_t r11_x = RAM_SWAPL(r11_ptr[4]);
    const int32_t r12_x = RAM_SWAPL(r12_ptr[4]);
    const int32_t r13_x = RAM_SWAPL(r13_ptr[4]);
    const int32_t r10_y = RAM_SWAPL(r10_ptr[5]);
    const int32_t r11_y = RAM_SWAPL(r11_ptr[5]);
    const int32_t r12_y = RAM_SWAPL(r12_ptr[5]);
    const int32_t r13_y = RAM_SWAPL(r13_ptr[5]);
    int32_t r0 = (r13_y - r11_y) * (r12_x - r10_x);
    int32_t r2 = (r12_y - r10_y) * (r13_x - r11_x);
    if (r2 > r0) {
        state->cycles += 63;
        return Azel_0601F5D2(state);
    }

    if (Azel_0601F824(state, 0x1, Azel_0601F948)) {
        state->cycles += 68;
        return Azel_0601F5D2(state);
    }
    if (Azel_0601F824(state, 0x2, Azel_0601F93A)) {
        state->cycles += 73;
        return Azel_0601F5D2(state);
    }
    if (Azel_0601F824(state, 0x4, Azel_0601F996)) {
        state->cycles += 78;
        return Azel_0601F5D2(state);
    }
    if (Azel_0601F824(state, 0x8, Azel_0601F988)) {
        state->cycles += 83;
        return Azel_0601F5D2(state);
    }

    uint32_t r4_0 = LRAM_LOADL(state->R[4], 0);
    if (((uint32_t *)HRAM_PTR(state->R[10]))[7]) {
        if (((uint32_t *)HRAM_PTR(state->R[11]))[7]) {
            if (((uint32_t *)HRAM_PTR(state->R[12]))[7]) {
                if (((uint32_t *)HRAM_PTR(state->R[13]))[7]) {
                    state->R[8] = 0;
                    state->cycles += 100;
                } else {
                    uint32_t temp1, temp2;
                    temp1 = state->R[10]; temp2 = state->R[13];
                            state->R[10] = temp2; state->R[13] = temp1;
                    temp1 = state->R[11]; temp2 = state->R[12];
                            state->R[11] = temp2; state->R[12] = temp1;
                    r4_0 ^= 0x20;
                    state->R[8] = 3;
                    state->cycles += 108;
                }
            } else {
                uint32_t temp1, temp2;
                temp1 = state->R[10]; temp2 = state->R[12];
                        state->R[10] = temp2; state->R[12] = temp1;
                temp1 = state->R[11]; temp2 = state->R[13];
                        state->R[11] = temp2; state->R[13] = temp1;
                r4_0 ^= 0x30;
                state->R[8] = 2;
                state->cycles += 107;
            }
        } else {
            uint32_t temp1, temp2;
            temp1 = state->R[10]; temp2 = state->R[11];
                    state->R[10] = temp2; state->R[11] = temp1;
            temp1 = state->R[12]; temp2 = state->R[13];
                    state->R[12] = temp2; state->R[13] = temp1;
            r4_0 ^= 0x10;
            state->R[8] = 1;
            state->cycles += 104;
        }
    } else {
        state->R[8] = 0;
        state->cycles += 89;
    }

    state->R[14] = HRAM_LOADL(state->GBR, 0);
    if (UNLIKELY(state->R[14]>>20 != 0x25C)) {
        state->PC = 0x601F71E;
        state->cycles += 2;
        return;
    }
    uint32_t *r14_ptr = (uint32_t *)VDP1_PTR(state->R[14]);

    /* Vertices may have been swapped above, so get the new pointers. */
    const uint16_t *new_r10_ptr = (const uint16_t *)HRAM_PTR(state->R[10]);
    const uint16_t *new_r11_ptr = (const uint16_t *)HRAM_PTR(state->R[11]);
    const uint16_t *new_r12_ptr = (const uint16_t *)HRAM_PTR(state->R[12]);
    const uint16_t *new_r13_ptr = (const uint16_t *)HRAM_PTR(state->R[13]);

    r14_ptr[3] = VDP_SWAPL(new_r10_ptr[9]<<16 | new_r10_ptr[11]);
    r14_ptr[4] = VDP_SWAPL(new_r11_ptr[9]<<16 | new_r11_ptr[11]);
    r14_ptr[5] = VDP_SWAPL(new_r12_ptr[9]<<16 | new_r12_ptr[11]);
    r14_ptr[6] = VDP_SWAPL(new_r13_ptr[9]<<16 | new_r13_ptr[11]);

    state->cycles += 24;
    return Azel_0601F762(state,
                         (const uint32_t *)new_r10_ptr,
                         (const uint32_t *)new_r11_ptr,
                         (const uint32_t *)new_r12_ptr,
                         (const uint32_t *)new_r13_ptr, r14_ptr, r4_0);
}

/*----------------------------------*/

static ALWAYS_INLINE void Azel_0601F762(
    SH2State *state, const uint32_t * const r10_ptr,
    const uint32_t * const r11_ptr, const uint32_t * const r12_ptr,
    const uint32_t * const r13_ptr, uint32_t * const r14_ptr,
    const uint32_t r4_0)
{
    const uint32_t r4_1 = LRAM_LOADL(state->R[4], 4);
    const uint32_t r4_2 = LRAM_LOADL(state->R[4], 8);
    state->R[4] += 12;

    *(uint16_t *)&r14_ptr[0] = VDP_SWAPW(r4_0 | 0x1000);
    r14_ptr[1] = VDP_SWAPL(r4_1);
    r14_ptr[2] = VDP_SWAPL(r4_2);
    state->R[9] = r4_0 >> 24;

    int32_t r1 = RAM_SWAPL(r10_ptr[1]);
    int32_t r2 = RAM_SWAPL(r11_ptr[1]);
    int32_t r3 = RAM_SWAPL(r12_ptr[1]);
    int32_t r5 = RAM_SWAPL(r13_ptr[1]);
    const uint32_t out_addr = HRAM_LOADL(state->GBR, 32);
    uint16_t *out_ptr = (uint16_t *)HRAM_PTR(out_addr);
    const int32_t mult = HRAM_LOADL(state->R[6], 52);

    switch (state->R[9] & 0xF0) {
      case 0x00:
        r1 = MAX(MAX(r1, r2), MAX(r3, r5));
        state->cycles += 21;
      l_601F7F4:
        r1 = ((int64_t)r1 * (int64_t)mult) >> 32;
        if (r1 <= 0) {
            r1 = 1;
        }
        out_ptr[2] = r1;
        out_ptr[3] = (uint16_t)(state->R[14] >> 3);
        state->cycles += 15;
        break;

      default:
        r1 = MIN(MIN(r1, r2), MIN(r3, r5));
        state->cycles += 21;
        goto l_601F7F4;

      case 0x20:
        r2 -= r1;
        r3 -= r1;
        r5 -= r1;
        r2 += r3 + r5;
        r1 += r2 >> 2;
        state->cycles += 18;
        goto l_601F7F4;

      case 0x30:
        r2 -= r1;
        r3 -= r1;
        r5 -= r1;
        r2 += r3 + r5;
        r1 += r2 >> 2;
        r1 = ((int64_t)r1 * (int64_t)mult) >> 32;
        if (r1 <= 0) {
            r1 = 1;
        }
        out_ptr[2] = 0x7FFF;
        out_ptr[3] = (uint16_t)(state->R[14] >> 3);
        state->cycles += 25;
        break;
    }  // switch (state->R[9] & 0xF0)

    /* 0x601F810 */

    HRAM_STOREL(state->GBR, 32, out_addr + 8);
    HRAM_STOREL(state->GBR, 12, HRAM_LOADL(state->GBR, 12) + 1);
    HRAM_STOREL(state->GBR, 28, HRAM_LOADL(state->GBR, 28) + 1);
    state->R[1] = r1;
    state->R[13] = state->GBR;
    state->SR &= ~SR_T;
    state->PC = state->PR;
    state->cycles += 8 + 11;
}

/*-----------------------------------------------------------------------*/

/* 0x601F824: Polygon clipping code (only used when called from 0x601F5EE;
 * returns true iff the polygon is completely clipped, the same as SR.T in
 * the original code) */

static int Azel_0601F824(SH2State *state, uint32_t mask,
                         int (*clipfunc)(SH2State *state))
{
    /* We save a bit of time by byte-swapping just this value and loading
     * the values to check directly from native memory. */
    mask = RAM_SWAPL(mask);
    const uint32_t r13_check = ((uint32_t *)HRAM_PTR(state->R[13]))[7] & mask;
    const uint32_t r12_check = ((uint32_t *)HRAM_PTR(state->R[12]))[7] & mask;
    const uint32_t r11_check = ((uint32_t *)HRAM_PTR(state->R[11]))[7] & mask;
    const uint32_t r10_check = ((uint32_t *)HRAM_PTR(state->R[10]))[7] & mask;

    /* Apply common state changes first, so the final function call (if any)
     * can be a plain jump. */
    state->SR &= ~SR_T;  // SR.T set means "entire polygon is clipped"
    state->PC = state->PR;

    if (r13_check) {
        if (r12_check) {
            state->cycles += 10;
            if (r11_check) {
                if (r10_check) {
                    state->cycles += 20;
                    return 1;
                } else {
                    state->R[9] = state->R[10];
                    state->R[8] = state->R[11];
                    (*clipfunc)(state);
                    state->R[8] = state->R[12];
                    (*clipfunc)(state);
                    state->R[8] = state->R[13];
                    state->cycles += 27;
                    return (*clipfunc)(state);
                }
            } else if (r10_check) {
                state->R[9] = state->R[11];
                state->R[8] = state->R[10];
                (*clipfunc)(state);
                state->R[8] = state->R[13];
                (*clipfunc)(state);
                state->R[8] = state->R[12];
                state->cycles += 28;
                return (*clipfunc)(state);
            } else {
                state->R[8] = state->R[13];
                state->R[9] = state->R[10];
                (*clipfunc)(state);
                state->R[8] = state->R[12];
                state->R[9] = state->R[11];
                state->cycles += 24;
                return (*clipfunc)(state);
            }
        } else if (r11_check) {
            if (r10_check) {
                state->R[9] = state->R[12];
                state->R[8] = state->R[10];
                (*clipfunc)(state);
                state->R[8] = state->R[13];
                (*clipfunc)(state);
                state->R[8] = state->R[11];
                state->cycles += 28;
                return (*clipfunc)(state);
            } else {
                state->cycles += 17;
                return 1;
            }
        } else if (r10_check) {
            state->R[8] = state->R[10];
            state->R[9] = state->R[11];
            (*clipfunc)(state);
            state->R[8] = state->R[13];
            state->R[9] = state->R[12];
            state->cycles += 25;
            return (*clipfunc)(state);
        } else {
            const int32_t r1 = HRAM_LOADL(state->R[12], 4);
            const int32_t r2 = HRAM_LOADL(state->R[10], 4);
            state->R[8] = state->R[13];
            state->R[9] = (r1 > r2) ? state->R[12] : state->R[10];
            state->cycles += (r1 > r2) ? 23 : 21;
            return (*clipfunc)(state);
        }

    } else if (r12_check) {
        if (r11_check) {
            if (r10_check) {
                state->R[9] = state->R[13];
                state->R[8] = state->R[11];
                (*clipfunc)(state);
                state->R[8] = state->R[12];
                (*clipfunc)(state);
                state->R[8] = state->R[10];
                state->cycles += 28;
                return (*clipfunc)(state);
            } else {
                state->R[8] = state->R[11];
                state->R[9] = state->R[10];
                (*clipfunc)(state);
                state->R[8] = state->R[12];
                state->R[9] = state->R[13];
                state->cycles += 24;
                return (*clipfunc)(state);
            }
        } else if (r10_check) {
            state->cycles += 18;
            return 1;
        } else {
            const int32_t r1 = HRAM_LOADL(state->R[13], 4);
            const int32_t r2 = HRAM_LOADL(state->R[11], 4);
            state->R[8] = state->R[12];
            state->R[9] = (r1 > r2) ? state->R[13] : state->R[11];
            state->cycles += (r1 > r2) ? 23 : 21;
            return (*clipfunc)(state);
        }

    } else if (r11_check) {
        if (r10_check) {
            state->R[8] = state->R[10];
            state->R[9] = state->R[13];
            (*clipfunc)(state);
            state->R[8] = state->R[11];
            state->R[9] = state->R[12];
            state->cycles += 25;
            return (*clipfunc)(state);
        } else {
            const int32_t r1 = HRAM_LOADL(state->R[10], 4);
            const int32_t r2 = HRAM_LOADL(state->R[12], 4);
            state->R[8] = state->R[11];
            state->R[9] = (r1 > r2) ? state->R[10] : state->R[12];
            state->cycles += (r1 > r2) ? 23 : 21;
            return (*clipfunc)(state);
        }

    } else if (r10_check) {
        const int32_t r1 = HRAM_LOADL(state->R[11], 4);
        const int32_t r2 = HRAM_LOADL(state->R[13], 4);
        state->R[8] = state->R[10];
        state->R[9] = (r1 > r2) ? state->R[11] : state->R[13];
        state->cycles += (r1 > r2) ? 24 : 22;
        return (*clipfunc)(state);

    } else {
        state->cycles += 15;
        return 0;
    }
}

/*----------------------------------*/

static int Azel_0601F93A(SH2State *state)
{
    const int32_t r8_x = HRAM_LOADL(state->R[8], 16);
    const int32_t x_max = (int16_t)HRAM_LOADW(state->R[6], 6);
    if (r8_x <= x_max) {
        state->cycles += 8;
        return 0;
    }
    state->cycles += 6;
    return Azel_0601F950(state, r8_x, x_max);
}

static int Azel_0601F948(SH2State *state)
{
    const int32_t r8_x = HRAM_LOADL(state->R[8], 16);
    const int32_t x_min = (int16_t)HRAM_LOADW(state->R[6], 4);
    if (r8_x >= x_min) {
        state->cycles += 10;
        return 0;
    }
    state->cycles += 4;
    return Azel_0601F950(state, r8_x, x_min);
}

static ALWAYS_INLINE int Azel_0601F950(SH2State *state, const int32_t r8_x,
                                       const int32_t x_lim)
{
    HRAM_STOREL(state->R[8], 16, x_lim);
    const int32_t dx_lim = x_lim - r8_x;
    const int32_t dx_r9 = HRAM_LOADL(state->R[9], 16) - r8_x;
    int32_t frac8;
    if (UNLIKELY(dx_r9 == 0)) {
        frac8 = (dx_lim >= 0) ? 0xFF : 0;
    } else {
        frac8 = bound((dx_lim << 8) / dx_r9, 0, 255);
    }
    const int32_t r8_y = HRAM_LOADL(state->R[8], 20);
    const int32_t r9_y = HRAM_LOADL(state->R[9], 20);
    const int32_t new_r8_y = (int16_t)(r8_y + (((r9_y - r8_y) * frac8) >> 8));
    HRAM_STOREL(state->R[8], 20, new_r8_y);

    state->cycles += 29;
    return 0;
}

/*----------------------------------*/

static int Azel_0601F988(SH2State *state)
{
    const int32_t r8_y = HRAM_LOADL(state->R[8], 20);
    const int32_t y_max = (int16_t)HRAM_LOADW(state->R[6], 0);
    if (r8_y <= y_max) {
        state->cycles += 8;
        return 0;
    }
    state->cycles += 6;
    return Azel_0601F99E(state, r8_y, y_max);
}

static int Azel_0601F996(SH2State *state)
{
    const int32_t r8_y = HRAM_LOADL(state->R[8], 20);
    const int32_t y_min = (int16_t)HRAM_LOADW(state->R[6], 2);
    if (r8_y >= y_min) {
        state->cycles += 10;
        return 0;
    }
    state->cycles += 4;
    return Azel_0601F99E(state, r8_y, y_min);
}

static ALWAYS_INLINE int Azel_0601F99E(SH2State *state, const int32_t r8_y,
                                       const int32_t y_lim)
{
    HRAM_STOREL(state->R[8], 20, y_lim);
    const int32_t dy_lim = y_lim - r8_y;
    const int32_t dy_r9 = HRAM_LOADL(state->R[9], 20) - r8_y;
    int32_t frac8;
    if (UNLIKELY(dy_r9 == 0)) {
        frac8 = (dy_lim >= 0) ? 0xFF : 0;
    } else {
        frac8 = bound((dy_lim << 8) / dy_r9, 0, 255);
    }
    const int32_t r8_x = HRAM_LOADL(state->R[8], 16);
    const int32_t r9_x = HRAM_LOADL(state->R[9], 16);
    const int32_t new_r8_x = (int16_t)(r8_x + (((r9_x - r8_x) * frac8) >> 8));
    HRAM_STOREL(state->R[8], 16, new_r8_x);

    state->cycles += 29;
    return 0;
}

/*----------------------------------*/

static int Azel_0601F9D6(SH2State *state)
{
    int32_t r3 = HRAM_LOADL(state->R[8], 4);
    int32_t r0 = (HRAM_LOADL(state->R[6], 16) << 8) - r3;
    int32_t r2 = HRAM_LOADL(state->R[9], 4) - r3;
    const float frac = (float)r0 / (float)r2;
    r2 = HRAM_LOADL(state->R[9], 8);
    r0 = HRAM_LOADL(state->R[8], 8);
    r2 = r0 + (uint32_t)((r2 - r0) * frac);
    r3 = HRAM_LOADL(state->R[9], 12);
    r0 = HRAM_LOADL(state->R[8], 12);
    r3 = r0 + (uint32_t)((r3 - r0) * frac);
    const int32_t mult = HRAM_LOADL(state->R[6], 48);
    r2 = ((int64_t)r2 * (int64_t)mult) >> 32;
    r3 = ((int64_t)r3 * (int64_t)mult) >> 32;
    HRAM_STOREL(state->R[8], 16, r2);
    HRAM_STOREL(state->R[8], 20, r3);

    const int16_t r6_4 = HRAM_LOADW(state->R[6],  8);
    const int16_t r6_5 = HRAM_LOADW(state->R[6], 10);
    const int16_t r6_6 = HRAM_LOADW(state->R[6], 12);
    const int16_t r6_7 = HRAM_LOADW(state->R[6], 14);
    const uint32_t r1 = (r3 > r6_4) << 3
                      | (r3 < r6_5) << 2
                      | (r2 > r6_7) << 1
                      | (r2 < r6_6) << 0;
    HRAM_STOREL(state->R[8], 28, r1);

    state->cycles += 73;
    return 0;
}

/*-----------------------------------------------------------------------*/

/* 0x601FA56, etc.: Color calculation routines (similar to 0x601ED46) */

static FASTCALL void Azel_0601FA56(SH2State *state)
{
    const int16_t *base_ptr =
        (const int16_t *)(HRAM_PTR(0x601FCB0) + (state->R[1]>>7 & -8));
    state->R[12] = base_ptr[0];
    state->R[11] = base_ptr[1];
    state->R[10] = base_ptr[2];
    state->cycles += 9;

    return Azel_0601FA68(state);
}

/*----------------------------------*/

static FASTCALL void Azel_0601FA68(SH2State *state)
{
    state->R[0] = Azel_color_calc((const int16_t *)HRAM_PTR(0x601FC94),
                                  (const int16_t *)LRAM_PTR(state->R[4]),
                                  (const int16_t *)HRAM_PTR(0x601FCA8),
                                  state->R[12], state->R[11], state->R[10]);
    state->R[4] += 6;
    state->PC = state->PR;
    state->cycles += 52;  // Approximate
}

/*-----------------------------------------------------------------------*/

/* 0x6035xxx: Mathematical library routines */

static FASTCALL void Azel_06035530(SH2State *state)
{
    if (LIKELY(state->R[5] != 0)) {
        state->R[0] =
            ((int64_t)(int32_t)state->R[4] << 16) / (int32_t)state->R[5];
        state->cycles += 52;
    } else {
        state->R[0] = 0;
        state->cycles += 7;
    }
    state->PC = state->PR;
}

static FASTCALL void Azel_06035552(SH2State *state)
{
    if (LIKELY(state->R[6] != 0)) {
        state->R[0] =
            ((int64_t)(int32_t)state->R[4] * (int64_t)(int32_t)state->R[5])
            / (int32_t)state->R[6];
    }
    state->cycles += 15;
    state->PC = state->PR;
}

static FASTCALL void Azel_0603556C(SH2State *state)
{
    if (LIKELY(state->R[6] != 0)) {
        state->R[0] = ((int16_t)state->R[4] * (int16_t)state->R[5])
                      / (int32_t)state->R[6];
    }
    state->cycles += 15;
    state->PC = state->PR;
}

/*----------------------------------*/

static FASTCALL void Azel_06035A8C(SH2State *state)
{
    const uint32_t ptr = HRAM_LOADL(0x604AEA4, 0);
    HRAM_STOREL(0x604AEA4, 0, ptr + 48);
    state->R[4] = ptr;
    state->R[5] = ptr + 48;
    state->cycles += 9;
    return Azel_06035AA0(state);
}

static FASTCALL void Azel_06035A9C(SH2State *state)
{
    state->R[5] = HRAM_LOADL(0x604AEA4, 0);
    state->cycles += 2;
    return Azel_06035AA0(state);
}

static FASTCALL void Azel_06035AA0(SH2State *state)
{
    const uint32_t *src = (const uint32_t *)HRAM_PTR(state->R[4]);
    uint32_t *dest = (uint32_t *)HRAM_PTR(state->R[5]);

    const uint32_t *src_limit = &src[12];
    for (; src < src_limit; src += 4, dest += 4) {
        const uint32_t word0 = src[0];
        const uint32_t word1 = src[1];
        const uint32_t word2 = src[2];
        const uint32_t word3 = src[3];
        dest[0] = word0;
        dest[1] = word1;
        dest[2] = word2;
        dest[3] = word3;
    }

    state->R[0] = state->R[5];
    state->PC = state->PR;
    state->cycles += 27;
}

/*----------------------------------*/

static ALWAYS_INLINE void Azel_06035B14_F00_common(
    SH2State *state, const int32_t *vector, const int32_t *matrix,
    int32_t *out_x_ptr, int32_t *out_y_ptr, int32_t *out_z_ptr);

static FASTCALL void Azel_06035B14(SH2State *state)
{
    const int32_t *r4_ptr = state->R[4] & 0x06000000
        ? (const int32_t *)HRAM_PTR(state->R[4])
        : (const int32_t *)LRAM_PTR(state->R[4]);
    int32_t *r5_ptr = (int32_t *)HRAM_PTR(HRAM_LOADL(0x604AEA4, 0));

    state->PC = state->PR;
    state->cycles += 56;
    return Azel_06035B14_F00_common(state, r4_ptr, r5_ptr,
                                    &r5_ptr[3], &r5_ptr[7], &r5_ptr[11]);
}

static FASTCALL void Azel_06035F00(SH2State *state)
{
    const int32_t *r4_ptr = state->R[4] & 0x06000000
        ? (const int32_t *)HRAM_PTR(state->R[4])
        : (const int32_t *)LRAM_PTR(state->R[4]);
    int32_t *r5_ptr = (int32_t *)HRAM_PTR(state->R[5]);
    const int32_t *r6_ptr = (const int32_t *)HRAM_PTR(HRAM_LOADL(0x604AEA4, 0));

    state->PC = state->PR;
    state->cycles += 54;
    return Azel_06035B14_F00_common(state, r4_ptr, r6_ptr,
                                    &r5_ptr[0], &r5_ptr[1], &r5_ptr[2]);
}

static FASTCALL void Azel_06035F04(SH2State *state)
{
    const int32_t *r4_ptr = state->R[4] & 0x06000000
        ? (const int32_t *)HRAM_PTR(state->R[4])
        : (const int32_t *)LRAM_PTR(state->R[4]);
    int32_t *r5_ptr = (int32_t *)HRAM_PTR(state->R[5]);
    const int32_t *r6_ptr = (const int32_t *)HRAM_PTR(state->R[6]);

    state->PC = state->PR;
    state->cycles += 52;
    return Azel_06035B14_F00_common(state, r4_ptr, r6_ptr,
                                    &r5_ptr[0], &r5_ptr[1], &r5_ptr[2]);
}

static ALWAYS_INLINE void Azel_06035B14_F00_common(
    SH2State *state, const int32_t *vector, const int32_t *matrix,
    int32_t *out_x_ptr, int32_t *out_y_ptr, int32_t *out_z_ptr)
{
#ifdef PSP

    int32_t v_0, v_1, v_2, M_0, M_1, M_2, M_3, hi, lo;
    asm(".set push; .set noreorder\n"

        "lw %[v_0], 0(%[vector])\n"
        "lw %[M_0], 0(%[matrix])\n"
        "lw %[v_1], 4(%[vector])\n"
        "ror %[v_0], %[v_0], 16\n"
        "ror %[M_0], %[M_0], 16\n"
        "mult %[v_0], %[M_0]\n"
        "lw %[M_1], 4(%[matrix])\n"
        "ror %[v_1], %[v_1], 16\n"
        "ror %[M_1], %[M_1], 16\n"
        "madd %[v_1], %[M_1]\n"
        "lw %[v_2], 8(%[vector])\n"
        "lw %[M_2], 8(%[matrix])\n"
        "lw %[M_3], 12(%[matrix])\n"
        "ror %[v_2], %[v_2], 16\n"
        "ror %[M_2], %[M_2], 16\n"
        "madd %[v_2], %[M_2]\n"
        "lw %[M_0], 16(%[matrix])\n"
        "lw %[M_1], 20(%[matrix])\n"
        "lw %[M_2], 24(%[matrix])\n"
        "ror %[M_3], %[M_3], 16\n"
        "ror %[M_0], %[M_0], 16\n"
        "mfhi %[hi]\n"
        "mflo %[lo]\n"

        "mult %[v_0], %[M_0]\n"
        "ror %[M_1], %[M_1], 16\n"
        "srl %[lo], %[lo], 16\n"
        "ins %[lo], %[hi], 16, 16\n"
        "madd %[v_1], %[M_1]\n"
        "ror %[M_2], %[M_2], 16\n"
        "addu %[lo], %[M_3], %[lo]\n"
        "lw %[M_3], 28(%[matrix])\n"
        "ror %[lo], %[lo], 16\n"
        "sw %[lo], 0(%[out_x_ptr])\n"
        "madd %[v_2], %[M_2]\n"
        "lw %[M_0], 32(%[matrix])\n"
        "lw %[M_1], 36(%[matrix])\n"
        "lw %[M_2], 40(%[matrix])\n"
        "ror %[M_3], %[M_3], 16\n"
        "ror %[M_0], %[M_0], 16\n"
        "mfhi %[hi]\n"
        "mflo %[lo]\n"

        "mult %[v_0], %[M_0]\n"
        "ror %[M_1], %[M_1], 16\n"
        "srl %[lo], %[lo], 16\n"
        "ins %[lo], %[hi], 16, 16\n"
        "madd %[v_1], %[M_1]\n"
        "ror %[M_2], %[M_2], 16\n"
        "addu %[lo], %[M_3], %[lo]\n"
        "lw %[M_3], 44(%[matrix])\n"
        "ror %[lo], %[lo], 16\n"
        "sw %[lo], 0(%[out_y_ptr])\n"
        "madd %[v_2], %[M_2]\n"
        "ror %[M_3], %[M_3], 16\n"
        "mfhi %[hi]\n"
        "mflo %[lo]\n"
        "srl %[lo], %[lo], 16\n"
        "ins %[lo], %[hi], 16, 16\n"
        "addu %[lo], %[M_3], %[lo]\n"
        "ror %[lo], %[lo], 16\n"
        "sw %[lo], 0(%[out_z_ptr])\n"

        ".set pop"
        : "=m" (*out_x_ptr), "=m" (*out_y_ptr), "=m" (*out_z_ptr),
          [v_0] "=&r" (v_0), [v_1] "=&r" (v_1), [v_2] "=&r" (v_2),
          [M_0] "=&r" (M_0), [M_1] "=&r" (M_1), [M_2] "=&r" (M_2),
          [M_3] "=&r" (M_3), [hi] "=&r" (hi), [lo] "=&r" (lo)
        : [vector] "r" (vector), [matrix] "r" (matrix),
          [out_x_ptr] "r" (out_x_ptr), [out_y_ptr] "r" (out_y_ptr),
          [out_z_ptr] "r" (out_z_ptr)
    );

#else // !PSP

    const int32_t temp0 =
        ((int64_t)RAM_SWAPL(vector[0]) * (int64_t)RAM_SWAPL(matrix[0])
       + (int64_t)RAM_SWAPL(vector[1]) * (int64_t)RAM_SWAPL(matrix[1])
       + (int64_t)RAM_SWAPL(vector[2]) * (int64_t)RAM_SWAPL(matrix[2])
       ) >> 16;
    *out_x_ptr = RAM_SWAPL(RAM_SWAPL(matrix[3]) + temp0);

    const int32_t temp1 =
        ((int64_t)RAM_SWAPL(vector[0]) * (int64_t)RAM_SWAPL(matrix[4])
       + (int64_t)RAM_SWAPL(vector[1]) * (int64_t)RAM_SWAPL(matrix[5])
       + (int64_t)RAM_SWAPL(vector[2]) * (int64_t)RAM_SWAPL(matrix[6])
       ) >> 16;
    *out_y_ptr = RAM_SWAPL(RAM_SWAPL(matrix[7]) + temp1);

    const int32_t temp2 =
        ((int64_t)RAM_SWAPL(vector[0]) * (int64_t)RAM_SWAPL(matrix[8])
       + (int64_t)RAM_SWAPL(vector[1]) * (int64_t)RAM_SWAPL(matrix[9])
       + (int64_t)RAM_SWAPL(vector[2]) * (int64_t)RAM_SWAPL(matrix[10])
       ) >> 16;
    *out_z_ptr = RAM_SWAPL(RAM_SWAPL(matrix[11]) + temp2);

#endif // PSP
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
    return Azel_0603A242(state);
}

/*----------------------------------*/

/* 0x603A242: CD command execution routine */

static int Azel_0603A242_state = 0;
static int Azel_0603A242_cmd51_count = 0;  // Count of sequential 0x51 commands

static FASTCALL void Azel_0603A242(SH2State *state)
{
    if (Azel_0603A242_state == 0) {
        const uint32_t r4 = state->R[4];
        uint16_t *ptr_6053278 = (uint16_t *)HRAM_PTR(0x6053278);
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
        uint16_t *dest = (uint16_t *)HRAM_PTR(0x605329C);
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

/* 0x603DD6E: CD read routine (actually a generalized copy routine, but
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
