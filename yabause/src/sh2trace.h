/*  src/sh2trace.h: SH-2 tracing code for debugging
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

#include <stdint.h>
#include <stdio.h>
#include "sh2d.h"

#ifdef __linux__
# define GZIP_LOGS  // Compress logs as they're created
#endif

static FILE *ssh2_log, *msh2_log;
static int trace_skip = 0;  // Number of insns to skip before tracing
static int trace_log = 10000000;  // Number of insns to trace

#define MappedMemoryWriteByte(a,v) __extension__({ \
    uint32_t __a = (a), __v = (v) & 0xFF; \
    if (msh2_log) \
        fprintf(msh2_log, "WRITEB %08X <- %02X\n", (int)__a, (int)__v); \
    MappedMemoryWriteByte(__a, __v); \
})

#define MappedMemoryWriteWord(a,v) __extension__({ \
    uint32_t __a = (a), __v = (v) & 0xFFFF; \
    if (msh2_log) \
        fprintf(msh2_log, "WRITEW %08X <- %04X\n", (int)__a, (int)__v); \
    MappedMemoryWriteWord(__a, __v); \
})

#define MappedMemoryWriteLong(a,v) __extension__({ \
    uint32_t __a = (a), __v = (v); \
    if (msh2_log) \
        fprintf(msh2_log, "WRITEL %08X <- %08X\n", (int)__a, (int)__v); \
    MappedMemoryWriteLong(__a, __v); \
})

static uint32_t total_cycles = 0;  // Global cycle accumulator
#define SH2_TRACE_ADD_CYCLES(cycles)  (total_cycles += (cycles))

#ifdef __GNUC__
__attribute__((unused))  // Just in case
#endif
static void sh2_trace(SH2_struct *state, uint32_t addr)
{
    FILE **fp_ptr = (state==SSH2 ? &ssh2_log : &msh2_log);

    if (trace_skip > 0) {
        trace_skip--;
        return;
    }

    if (trace_log == 0) {
        if (*fp_ptr) {
#ifdef GZIP_LOGS
            pclose(*fp_ptr);
#else
            fclose(*fp_ptr);
#endif
            *fp_ptr = NULL;
        }
        return;
    }
    trace_log--;

    if (!*fp_ptr) {
        const char *filename = (state==SSH2 ? "ssh2.log" : "msh2.log");
#ifdef GZIP_LOGS
        char cmdbuf[100];
        snprintf(cmdbuf, sizeof(cmdbuf), "gzip -9 >'%s'.gz", filename);
        *fp_ptr = popen(cmdbuf, "w");
#else
        *fp_ptr = fopen(filename, "w");
#endif
        if (!*fp_ptr) {
            return;
        }
    }

    uint16_t opcode = MappedMemoryReadWord(addr);
    char buf[100];
    SH2Disasm(addr, opcode, 0, buf);
    fprintf(*fp_ptr, "%08X: %04X  %-50s [%10lu]\n",
            (int)addr, (int)opcode, buf+12,
            (unsigned long)(total_cycles + state->cycles));
    /* This looks ugly, but it's faster than fprintf() in this case */
    static char regbuf[] = "  R0: XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX\n  R8: XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX XXXXXXXX\n  PR: XXXXXXXX  SR: XXX  MAC: XXXXXXXX/XXXXXXXX  GBR: XXXXXXXX  VBR: XXXXXXXX\n";
    auto inline void HEXIT(char * const ptr, uint32_t val, int ndigits);
    void HEXIT(char * const ptr, uint32_t val, int ndigits) {
        while (ndigits-- > 0) {
            const int digit = val & 0xF;
            val >>= 4;
            ptr[ndigits] = (digit>9 ? digit+7+'0' : digit+'0');
        }
    }
    int i;
    for (i = 0; i < 16; i++) {
        HEXIT(i>=8 ? &regbuf[12+i*9] : &regbuf[6+i*9], state->regs.R[i], 8);
    }
    HEXIT(&regbuf[162], state->regs.PR, 8);
    HEXIT(&regbuf[176], state->regs.SR.all, 3);
    HEXIT(&regbuf[186], state->regs.MACH, 8);
    HEXIT(&regbuf[195], state->regs.MACL, 8);
    HEXIT(&regbuf[210], state->regs.GBR, 8);
    HEXIT(&regbuf[225], state->regs.VBR, 8);
    fwrite(regbuf, sizeof(regbuf)-1, 1, *fp_ptr);
}

