/*  src/psp/psp-m68k.c: PSP M68k emulator interface (uses Q68)
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

#include "../core.h"
#include "../error.h"
#include "../m68kcore.h"
#include "../memory.h"
#include "../scsp.h"

#include "../q68/q68.h"
#include "../q68/q68-const.h"  // for Q68_JIT_PAGE_BITS

#include "common.h"
#include "config.h"
#include "psp-m68k.h"
#include "sys.h"

#include "me.h"
#include "me-utility.h"

/*************************************************************************/

/**
 * SCSP_READ_INLINE:  When defined, 58k code running on the ME will read
 * SCSP registers by directly calling scsp_r_[bw]() rather than sending an
 * interrupt to the main CPU.  This may result in incorrect operation if
 * SCSP data is cached by the main CPU and has not been flushed (though at
 * present things seem to work).
 */
#define SCSP_READ_INLINE

/*************************************************************************/
/************************* Interface definition **************************/
/*************************************************************************/

/* Interface function declarations (must come before interface definition) */

static int psp_m68k_init(void);
static void psp_m68k_deinit(void);
static void psp_m68k_reset(void);

static FASTCALL s32 psp_m68k_exec(s32 cycles);
static void psp_m68k_sync(void);

static u32 psp_m68k_get_dreg(u32 num);
static u32 psp_m68k_get_areg(u32 num);
static u32 psp_m68k_get_pc(void);
static u32 psp_m68k_get_sr(void);
static u32 psp_m68k_get_usp(void);
static u32 psp_m68k_get_ssp(void);

static void psp_m68k_set_dreg(u32 num, u32 val);
static void psp_m68k_set_areg(u32 num, u32 val);
static void psp_m68k_set_pc(u32 val);
static void psp_m68k_set_sr(u32 val);
static void psp_m68k_set_usp(u32 val);
static void psp_m68k_set_ssp(u32 val);

static FASTCALL void psp_m68k_set_irq(s32 level);
static FASTCALL void psp_m68k_write_notify(u32 address, u32 size);

static void psp_m68k_set_fetch(u32 low_addr, u32 high_addr, pointer fetch_addr);
static void psp_m68k_set_readb(M68K_READ *func);
static void psp_m68k_set_readw(M68K_READ *func);
static void psp_m68k_set_writeb(M68K_WRITE *func);
static void psp_m68k_set_writew(M68K_WRITE *func);

/*-----------------------------------------------------------------------*/

/* Module interface definition */

M68K_struct M68KPSP = {
    .id          = M68KCORE_PSP,
    .Name        = "PSP M68k Emulator Interface",

    .Init        = psp_m68k_init,
    .DeInit      = psp_m68k_deinit,
    .Reset       = psp_m68k_reset,

    .Exec        = psp_m68k_exec,
    .Sync        = psp_m68k_sync,

    .GetDReg     = psp_m68k_get_dreg,
    .GetAReg     = psp_m68k_get_areg,
    .GetPC       = psp_m68k_get_pc,
    .GetSR       = psp_m68k_get_sr,
    .GetUSP      = psp_m68k_get_usp,
    .GetMSP      = psp_m68k_get_ssp,

    .SetDReg     = psp_m68k_set_dreg,
    .SetAReg     = psp_m68k_set_areg,
    .SetPC       = psp_m68k_set_pc,
    .SetSR       = psp_m68k_set_sr,
    .SetUSP      = psp_m68k_set_usp,
    .SetMSP      = psp_m68k_set_ssp,

    .SetIRQ      = psp_m68k_set_irq,
    .WriteNotify = psp_m68k_write_notify,

    .SetFetch    = psp_m68k_set_fetch,
    .SetReadB    = psp_m68k_set_readb,
    .SetReadW    = psp_m68k_set_readw,
    .SetWriteB   = psp_m68k_set_writeb,
    .SetWriteW   = psp_m68k_set_writew,
};

/*************************************************************************/

/* Virtual processor state block */
static Q68State *q68_state;

/* Cacheable memory pointer to sound RAM (see psp_m68k_init() for details;
 * stored in a separate section to avoid cache line collisions, since
 * repeated uncached access to this variable would slow things down) */
static __attribute__((section(".bss.me"), aligned(64)))
    uint8_t *SoundRam_cached;

/*----------------------------------*/

/* Flag: Are we using the ME for M68k execution? */
static int using_me;

/* Flag: Is there currently an iteration running on the ME? */
static int me_running;

/* Last result returned from a q68_run() call on the ME */
static int me_last_result;

/* IRQ level to be set before the next q68_run() call (-1 = none) */
static int saved_irq;

/*----------------------------------*/

/* Parameter block for SCSP write requests from ME */

typedef enum ScspReqType_ {
    SCSPREQ_NONE   = 0x0,
    SCSPREQ_READB  = 0x1,
    SCSPREQ_READW  = 0x2,
    SCSPREQ_WRITEB = 0x9,
    SCSPREQ_WRITEW = 0xA,
} ScspReqType;

#define SCSPREQ_MAKE_NONE() \
    (SCSPREQ_NONE<<28)
#define SCSPREQ_MAKE_READB(address) \
    (SCSPREQ_READB<<28 | ((address)&0xFFF)<<16)
#define SCSPREQ_MAKE_READW(address) \
    (SCSPREQ_READW<<28 | ((address)&0xFFF)<<16)
#define SCSPREQ_MAKE_WRITEB(address,data) \
    (SCSPREQ_WRITEB<<28 | ((address)&0xFFF)<<16 | ((data)&0xFF))
#define SCSPREQ_MAKE_WRITEW(address,data) \
    (SCSPREQ_WRITEW<<28 | ((address)&0xFFF)<<16 | ((data)&0xFFFF))

#define SCSPREQ_TYPE(req)     ((req)>>28)
#define SCSPREQ_ADDRESS(req)  ((req)>>16 & 0xFFF)
#define SCSPREQ_DATA(req)     ((req) & 0xFFFF)

/* Put it in its own cache line for best performance */
static __attribute__((aligned(64))) struct {
    /* Read/write request (generated by SCSPREQ_MAKE_* macros) */
    volatile uint32_t req;

    /* Flag: Has the last SCSP read/write request completed?  Set by main
     * CPU, cleared by ME. */
    volatile int done;

    /* Value read for an SCSP read request */
    volatile uint32_t read_data;

    /* Pad to the size of a cache line */
    int pad[13];
} scsp_rw_block;

/* Termination flag for SCSP read/write thread */
static volatile int scsp_rw_thread_terminate;

/*----------------------------------*/

/* List of 68k addresses touched by the SH-2s or other hardware during the
 * current 68k iteration */

typedef struct TouchEntry_ {
    uint32_t address;
    uint32_t size;
} TouchEntry;
static __attribute__((aligned(64))) TouchEntry touch_list[256];
static unsigned int touch_list_size;

/* Macros to set/get touch_list_size with an uncached access */
#define SET_TOUCH_LIST_SIZE(n) \
    (*(unsigned int *)((uintptr_t)&touch_list_size | 0x40000000) = (n))
#define GET_TOUCH_LIST_SIZE() \
    (*(unsigned int *)((uintptr_t)&touch_list_size | 0x40000000))

/*----------------------------------*/

/* Local function declarations */

static void flush_cache(void);

static __attribute__((noreturn)) void scsp_rw_thread(void);

static __attribute__((section(".text.me"), aligned(64)))
    void me_trampoline_reset(void);
static __attribute__((section(".text.me")))
    int32_t me_trampoline_run(int cycles);
static __attribute__((section(".text.me")))
    void me_trampoline_set_irq(int level);
static __attribute__((section(".text.me")))
    void me_trampoline_touch_memory(void);

static __attribute__((section(".text.me")))
    uint32_t m68k_readb_me(uint32_t address);
static __attribute__((section(".text.me")))
    uint32_t m68k_readw_me(uint32_t address);
static __attribute__((section(".text.me")))
    void m68k_writeb_me(uint32_t address, uint32_t data);
static __attribute__((section(".text.me")))
    void m68k_writew_me(uint32_t address, uint32_t data);
static __attribute__((section(".text.me"))) void flush_cache_me(void);

static __attribute__((section(".text.me"))) void me_malloc_init(void);
static __attribute__((section(".text.me"))) void *me_malloc(size_t size);
static __attribute__((section(".text.me")))
    void *me_realloc(void *ptr, size_t size);
static __attribute__((section(".text.me"))) void me_free(void *ptr);

/*************************************************************************/
/************************** Interface functions **************************/
/*************************************************************************/

/**
 * psp_m68k_init:  Initialize the virtual processpr.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, negative on failure
 */
static int psp_m68k_init(void)
{
    if (!(q68_state = q68_create())) {
        DMSG("Failed to create Q68 state block");
        return -1;
    }
    q68_set_irq(q68_state, 0);
    q68_set_jit_memory_funcs(q68_state, malloc, realloc, free, flush_cache);

    if (me_available && config_get_use_me()) {
#ifdef PSP_DEBUG
        meExceptionSetFatal(1);
#endif
        int res;
        if ((res = meStart()) == 0
         && (res = meWait()) == 0
         && (res = meCall((void *)me_malloc_init, 0)) == 0
         && (res = meWait()) == 0
        ) {
            /* Make sure the SCSP read/write thread has a higher priority
             * than the main thread so the main thread can be interrupted */
            const unsigned int scsp_rw_thread_pri =
                sceKernelGetThreadCurrentPriority() - 1;
            scsp_rw_thread_terminate = 0;
            if ((res = sys_start_thread("YabauseScspRWThread",
                                        scsp_rw_thread, scsp_rw_thread_pri,
                                        0x4000, 0, NULL)) >= 0) {
                using_me = 1;
                me_running = 0;
                me_last_result = 0;
                saved_irq = -1;
                q68_set_readb_func(q68_state, m68k_readb_me);
                q68_set_readw_func(q68_state, m68k_readw_me);
                q68_set_writeb_func(q68_state, m68k_writeb_me);
                q68_set_writew_func(q68_state, m68k_writew_me);
                q68_set_jit_memory_funcs(q68_state,
                                         me_malloc, me_realloc, me_free,
                                         flush_cache_me);
                sceKernelDcacheWritebackInvalidateAll();
                /* No Q68 functions should be called on the main CPU after
                 * this point (until cleanup). */
            } else {
                DMSG("sys_start_thread() failed: %s", psp_strerror(res));
                YabSetError(YAB_ERR_OTHER,
                            "Failed to start the Media Engine!\n");
                meStop();
                return -1;
            }
        } else {
            DMSG("meStart()/meCall()/meWait() failed: %s", psp_strerror(res));
            YabSetError(YAB_ERR_OTHER, "Failed to start the Media Engine!\n");
            return -1;
        }
    } else {
        using_me = 0;
        me_running = 0;
    }

    scsp_rw_block.req = SCSPREQ_MAKE_NONE();
    sceKernelDcacheWritebackInvalidateRange(&scsp_rw_block,
                                            sizeof(scsp_rw_block));

    /* Since the PSP does not implement cache coherency, we need to take
     * special steps when using the ME to ensure that the caches in each
     * CPU do not mask or clobber changes to memory made by the other CPU.
     * One of these steps is to convert the sound RAM pointer to an
     * uncacheable address for use by the main CPU, saving the original
     * cacheable pointer for use by the ME only (which flushes its caches
     * after each execution iteration).  If we are not using the ME,
     * however, we leave the original, cacheable pointer alone to avoid
     * performance degradation. */
    SoundRam_cached = SoundRam;
    if (using_me) {
        sceKernelDcacheWritebackInvalidateRange(SoundRam, 0x80000);
        SoundRam = (u8 *)((uintptr_t)SoundRam | 0x40000000);
    }

    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_m68k_deinit:  Destroy the virtual processor.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_m68k_deinit(void)
{
    psp_m68k_sync();

    scsp_rw_thread_terminate = 1;

    SoundRam = SoundRam_cached;  // Avoid a potential crash when we free() it

    q68_destroy(q68_state);
    q68_state = NULL;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_m68k_reset:  Reset the virtual processor.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_m68k_reset(void)
{
    psp_m68k_sync();

    if (using_me) {
        sceKernelDcacheWritebackInvalidateAll();
        if (meCall((void *)me_trampoline_reset, q68_state) == 0) {
            meWait();
        } else {
            DMSG("meCall() failed for q68_reset");
        }
        me_last_result = 0;
        saved_irq = -1;
    } else {
        q68_reset(q68_state);
    }

    SET_TOUCH_LIST_SIZE(0);
}

/*************************************************************************/

/**
 * psp_m68k_exec:  Execute instructions for the given number of clock cycles.
 *
 * [Parameters]
 *     cycles: Number of clock cycles to execute
 * [Return value]
 *     Number of clock cycles actually executed
 */
static FASTCALL s32 psp_m68k_exec(s32 cycles)
{
    if (using_me) {
        int res;
        int32_t cycles_to_run = cycles;
        psp_m68k_sync();
        if (me_running) {
            cycles_to_run -= me_last_result;
        }
        if (GET_TOUCH_LIST_SIZE() > 0) {
            sceKernelDcacheWritebackInvalidateAll();
            if ((res = meCall((void *)me_trampoline_touch_memory, NULL)) == 0) {
                meWait();
            } else {
                DMSG("meCall() failed for touch: %s", psp_strerror(res));
            }
            SET_TOUCH_LIST_SIZE(0);
        }
        if (saved_irq >= 0) {
            if ((res = meCall((void *)me_trampoline_set_irq,
                              (void *)saved_irq)) == 0) {
                meWait();
            } else {
                DMSG("meCall() failed for set_irq: %s", psp_strerror(res));
            }
            saved_irq = -1;
        }
        if ((res = meCall((void *)me_trampoline_run,
                          (void *)cycles_to_run)) == 0) {
            me_running = 1;
            return cycles;
        } else {
            DMSG("meCall() failed for run: %s", psp_strerror(res));
            return cycles;
        }
    } else {
        return q68_run(q68_state, cycles);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * psp_m68k_sync:  Wait for background execution to finish.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_m68k_sync(void)
{
    if (me_running) {
        meWait();
        me_last_result = meResult();
        me_running = 0;
    }
}

/*************************************************************************/

/**
 * psp_m68k_get_{dreg,areg,pc,sr,usp,ssp}:  Return the current value of
 * the specified register.
 *
 * [Parameters]
 *     num: Register number (psp_m68k_get_dreg(), psp_m68k_get_areg() only)
 * [Return value]
 *     None
 */

static u32 psp_m68k_get_dreg(u32 num)
{
    return q68_get_dreg(q68_state, num);
}

static u32 psp_m68k_get_areg(u32 num)
{
    return q68_get_areg(q68_state, num);
}

static u32 psp_m68k_get_pc(void)
{
    return q68_get_pc(q68_state);
}

static u32 psp_m68k_get_sr(void)
{
    return q68_get_sr(q68_state);
}

static u32 psp_m68k_get_usp(void)
{
    return q68_get_usp(q68_state);
}

static u32 psp_m68k_get_ssp(void)
{
    return q68_get_ssp(q68_state);
}

/*-----------------------------------------------------------------------*/

/**
 * psp_m68k_set_{dreg,areg,pc,sr,usp,ssp}:  Set the value of the specified
 * register.
 *
 * [Parameters]
 *     num: Register number (psp_m68k_set_dreg(), psp_m68k_set_areg() only)
 *     val: Value to set
 * [Return value]
 *     None
 */

static void psp_m68k_set_dreg(u32 num, u32 val)
{
    q68_set_dreg(q68_state, num, val);
}

static void psp_m68k_set_areg(u32 num, u32 val)
{
    q68_set_areg(q68_state, num, val);
}

static void psp_m68k_set_pc(u32 val)
{
    q68_set_pc(q68_state, val);
}

static void psp_m68k_set_sr(u32 val)
{
    q68_set_sr(q68_state, val);
}

static void psp_m68k_set_usp(u32 val)
{
    q68_set_usp(q68_state, val);
}

static void psp_m68k_set_ssp(u32 val)
{
    q68_set_ssp(q68_state, val);
}

/*************************************************************************/

/**
 * psp_m68k_set_irq:  Deliver an interrupt to the processor.
 *
 * [Parameters]
 *     level: Interrupt level (0-7)
 * [Return value]
 *     None
 */
static FASTCALL void psp_m68k_set_irq(s32 level)
{
    if (using_me) {
        /* This can be called from the SCSP access thread, where we can't
         * wait for ME execution to finish, so we have to store the IRQ
         * level change and signal it before the next iteration. */
        saved_irq = level;
    } else {
        q68_set_irq(q68_state, level);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * psp_m68k_write_notify:  Inform the 68k emulator that the given address
 * range has been modified.
 *
 * [Parameters]
 *     address: 68000 address of modified data
 *        size: Size of modified data in bytes
 * [Return value]
 *     None
 */
static FASTCALL void psp_m68k_write_notify(u32 address, u32 size)
{
    if (using_me) {
        /* Note that it's safe to store to sound RAM via the cache
         * (assuming the write doesn't have to be seen by a currently-
         * executing 68k iteration) as long as write_notify() is called;
         * when at least one write notification is present, the main CPU's
         * cache is flushed before the next 68k iteration. */
        unsigned int listsize = GET_TOUCH_LIST_SIZE();
        /* This may cause translated blocks to be freed, which can only be
         * done on the ME, so we have to call q68_touch_memory() on the ME
         * no matter what. */
        if (listsize >= lenof(touch_list)) {
            psp_m68k_sync();
            sceKernelDcacheWritebackInvalidateAll();
            int res;
            if ((res = meCall((void *)me_trampoline_touch_memory, NULL)) == 0) {
                meWait();
            } else {
                DMSG("meCall() failed for touch: %s", psp_strerror(res));
            }
            listsize = 0;
        } else if (listsize > 0) {
            const uint32_t prev_address = touch_list[listsize-1].address;
            const uint32_t prev_size = touch_list[listsize-1].size;
            if (prev_address + prev_size == address) {
                /* Immediately follows previous entry */
                touch_list[listsize-1].size += size;
                return;
            } else if (address + size == prev_address) {
                /* Immediately precedes previous entry */
                touch_list[listsize-1].address = address;
                touch_list[listsize-1].size += size;
                /* Merge once more if the previous entry now immediately
                 * follows its predecessor (e.g. for a write sequence like
                 * 0x0C,0x08,0x04,0x00,0x1C,0x18,0x14,0x10,...) */
                if (listsize >= 2) {
                    const uint32_t prev2_address =
                        touch_list[listsize-2].address;
                    const uint32_t prev2_size =
                        touch_list[listsize-2].size;
                    if (prev2_address + prev2_size == address) {
                        touch_list[listsize-2].size +=
                            touch_list[listsize-1].size;
                        listsize--;
                        SET_TOUCH_LIST_SIZE(listsize);
                    }
                }
                return;
            }
        }
        touch_list[listsize].address = address;
        touch_list[listsize].size = size;
        listsize++;
        SET_TOUCH_LIST_SIZE(listsize);
    } else {
        q68_touch_memory(q68_state, address, size);
    }
}

/*************************************************************************/

/**
 * psp_m68k_set_fetch:  Set the instruction fetch pointer for a region of
 * memory.  Not used by Q68.
 *
 * [Parameters]
 *       low_addr: Low address of memory region to set
 *      high_addr: High address of memory region to set
 *     fetch_addr: Pointer to corresponding memory region (NULL to disable)
 * [Return value]
 *     None
 */
static void psp_m68k_set_fetch(u32 low_addr, u32 high_addr, pointer fetch_addr)
{
}

/*-----------------------------------------------------------------------*/

/**
 * psp_m68k_set_{readb,readw,writeb,writew}:  Set functions for reading or
 * writing bytes or words in memory.  Ignored when the ME is in use (we
 * implement our own functions in that case to handle interprocessor
 * synchronization properly).
 *
 * [Parameters]
 *     func: Function to set
 * [Return value]
 *     None
 */

static void psp_m68k_set_readb(M68K_READ *func)
{
    if (!using_me) {
        q68_set_readb_func(q68_state, (Q68ReadFunc *)func);
    }
}

static void psp_m68k_set_readw(M68K_READ *func)
{
    if (!using_me) {
        q68_set_readw_func(q68_state, (Q68ReadFunc *)func);
    }
}

static void psp_m68k_set_writeb(M68K_WRITE *func)
{
    if (!using_me) {
        q68_set_writeb_func(q68_state, (Q68WriteFunc *)func);
    }
}

static void psp_m68k_set_writew(M68K_WRITE *func)
{
    if (!using_me) {
        q68_set_writew_func(q68_state, (Q68WriteFunc *)func);
    }
}

/*************************************************************************/
/*********************** Local routines (main CPU) ***********************/
/*************************************************************************/

/**
 * flush_cache:  Flush the data and instruction caches of the main CPU.
 * Called by Q68 after a new block of code has been translated.
 */
static void flush_cache(void)
{
    sceKernelDcacheWritebackInvalidateAll();
    sceKernelIcacheInvalidateAll();
}

/*************************************************************************/

/**
 * scsp_rw_thread:  Watch for SCSP read/write requests from the ME and
 * execute them on the main CPU.  Started as a separate thread when the ME
 * is used for M68k execution.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Does not return
 */
static __attribute__((noreturn)) void scsp_rw_thread(void)
{
    while (!scsp_rw_thread_terminate) {
        meInterruptWait();
        const uint32_t req = scsp_rw_block.req;
        const ScspReqType  type    = SCSPREQ_TYPE(req);
        const unsigned int address = SCSPREQ_ADDRESS(req);
        const unsigned int data    = SCSPREQ_DATA(req);
        switch (type) {
          case SCSPREQ_NONE:  // Impossible, but avoid a compiler warning
            break;
          case SCSPREQ_READB:
            scsp_rw_block.read_data = scsp_r_b(address);
            break;
          case SCSPREQ_READW:
            scsp_rw_block.read_data = scsp_r_w(address);
            break;
          case SCSPREQ_WRITEB:
            scsp_w_b(address, data);
            break;
          case SCSPREQ_WRITEW:
            scsp_w_w(address, data);
            break;
        }
#ifdef SCSP_READ_INLINE
        if ((int32_t)req < 0) {  // i.e. it was a write request
            /* Write out any changes so the ME can see them.  Expensive, but
             * since there's no cache coherency we don't have much choice... */
            sceKernelDcacheWritebackAll();
        }
#endif
        scsp_rw_block.done = 1;
        sceKernelDcacheWritebackInvalidateRange(&scsp_rw_block,
                                                sizeof(scsp_rw_block));
    }
    sceKernelExitDeleteThread(0);
}

/*************************************************************************/
/********************* Local routines (Media Engine) *********************/
/*************************************************************************/

/*
 * All Media Engine code is placed in a separate section, ".text.me", to
 * ensure that no cache lines are shared between the main CPU and the Media
 * Engine CPU.  (Since all data in this area is read-only program code,
 * there isn't technically a problem with sharing cache lines here, but
 * this way is cleaner.)  We also force 64-byte (cache-line) alignment on
 * one of the functions, specifically me_trampoline_reset(), to cause the
 * section as a whole to be 64-byte aligned.
 */

/*************************************************************************/

/**
 * me_trampoline_reset:  Call q68_reset() to reset the 68k emulator.  This
 * function must be executed on the Media Engine.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static __attribute__((section(".text.me"), aligned(64)))
    void me_trampoline_reset(void)
{
    Q68State *state = *(Q68State **)((uintptr_t)&q68_state | 0x40000000);
    q68_reset(state);
    meUtilityDcacheWritebackInvalidateAll();
}

/*-----------------------------------------------------------------------*/

/**
 * me_trampoline_run:  Call q68_run() for the given number of cycles.  This
 * function must be executed on the Media Engine.
 *
 * [Parameters]
 *     cycles: Number of clock cycles to execute
 * [Return value]
 *     Number of clock cycles executed beyond "cycles"
 */
static __attribute__((section(".text.me")))
    int32_t me_trampoline_run(int cycles)
{
    Q68State *state = *(Q68State **)((uintptr_t)&q68_state | 0x40000000);
    int32_t result = q68_run(state, cycles) - cycles;
    meUtilityDcacheWritebackInvalidateAll();
    return result;
}

/*-----------------------------------------------------------------------*/

/**
 * me_trampoline_set_irq:  Call q68_set_irq() to signal an interrupt to the
 * 68k processor.  This function must be executed on the Media Engine.
 *
 * [Parameters]
 *     level: IRQ level to set
 * [Return value]
 *     None
 */
static __attribute__((section(".text.me")))
    void me_trampoline_set_irq(int level)
{
    Q68State *state = *(Q68State **)((uintptr_t)&q68_state | 0x40000000);
    q68_set_irq(state, level);
    meUtilityDcacheWritebackInvalidateAll();
}

/*-----------------------------------------------------------------------*/

/**
 * me_trampoline_touch_memory:  Call q68_touch_memory() for all entries in
 * the touched memory list (touch_list[]).  This function must be executed
 * on the Media Engine.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static __attribute__((section(".text.me")))
    void me_trampoline_touch_memory(void)
{
    Q68State *state = *(Q68State **)((uintptr_t)&q68_state | 0x40000000);
    const unsigned int listsize = GET_TOUCH_LIST_SIZE();
    unsigned int i;
    for (i = 0; i < listsize; i++) {
        q68_touch_memory(state, touch_list[i].address, touch_list[i].size);
    }
    meUtilityDcacheWritebackInvalidateAll();
}

/*************************************************************************/

/**
 * m68k_readb_me, m68k_readw_me:  Read a byte or word from the given
 * address when running on the Media Engine.
 *
 * [Parameters]
 *     address: Address to read from
 * [Return value]
 *     Value read (always zero)
 */

static __attribute__((section(".text.me")))
    uint32_t m68k_readb_me(uint32_t address)
{
    if (address < 0x100000) {
        return T2ReadByte(SoundRam_cached, address & 0x7FFFF);
    } else {
#ifdef SCSP_READ_INLINE
        return scsp_r_b(address);
#else
        /* We generate these pointers on the fly because if we loaded them
         * from static variables, the accesses to those static variables
         * would pull the cache line into memory, and our uncached writes
         * would subsequently get clobbered when the cache was flushed. */
        volatile int *scsp_rw_done_ptr =
            (volatile int *)((uintptr_t)&scsp_rw_block.done | 0x40000000);
        *scsp_rw_done_ptr = 0;
        volatile uint32_t *scsp_rw_req_ptr =
            (volatile uint32_t *)((uintptr_t)&scsp_rw_block.req | 0x40000000);
        *scsp_rw_req_ptr = SCSPREQ_MAKE_READB(address);
        meUtilitySendInterrupt();
        while (!*scsp_rw_done_ptr) { /*spin*/ }
        volatile uint32_t *scsp_read_data_ptr =
            (volatile uint32_t *)((uintptr_t)&scsp_rw_block.read_data | 0x40000000);
        return *scsp_read_data_ptr;
#endif  // SCSP_READ_INLINE
    }
}

static __attribute__((section(".text.me")))
    uint32_t m68k_readw_me(uint32_t address)
{
    if (address < 0x100000) {
        return T2ReadWord(SoundRam_cached, address & 0x7FFFF);
    } else {
#ifdef SCSP_READ_INLINE
        return scsp_r_w(address);
#else
        volatile int *scsp_rw_done_ptr =
            (volatile int *)((uintptr_t)&scsp_rw_block.done | 0x40000000);
        *scsp_rw_done_ptr = 0;
        volatile uint32_t *scsp_rw_req_ptr =
            (volatile uint32_t *)((uintptr_t)&scsp_rw_block.req | 0x40000000);
        *scsp_rw_req_ptr = SCSPREQ_MAKE_READW(address);
        meUtilitySendInterrupt();
        while (!*scsp_rw_done_ptr) { /*spin*/ }
        volatile uint32_t *scsp_read_data_ptr =
            (volatile uint32_t *)((uintptr_t)&scsp_rw_block.read_data | 0x40000000);
        return *scsp_read_data_ptr;
#endif  // SCSP_READ_INLINE
    }
}

/*-----------------------------------------------------------------------*/

/**
 * m68k_writeb_me, m68k_writew_me:  Write a byte or word to the given
 * address when running on the ME.
 *
 * [Parameters]
 *     address: Address to write to
 *        data: Value to write
 * [Return value]
 *     None
 */

static __attribute__((section(".text.me")))
    void m68k_writeb_me(uint32_t address, uint32_t data)
{
    if (address < 0x100000) {
        T2WriteByte(SoundRam_cached, address & 0x7FFFF, data);
    } else {
        volatile int *scsp_rw_done_ptr =
            (volatile int *)((uintptr_t)&scsp_rw_block.done | 0x40000000);
        *scsp_rw_done_ptr = 0;
        volatile uint32_t *scsp_rw_req_ptr =
            (volatile uint32_t *)((uintptr_t)&scsp_rw_block.req | 0x40000000);
        *scsp_rw_req_ptr = SCSPREQ_MAKE_WRITEB(address, data);
        meUtilitySendInterrupt();
#ifdef SCSP_READ_INLINE
        /* Clear out the data cache in case subsequent code reads from the
         * SCSP and expects to see the changes from this write reflected. */
        meUtilityDcacheWritebackInvalidateAll();
#endif
        while (!*scsp_rw_done_ptr) { /*spin*/ }
    }
}

static __attribute__((section(".text.me")))
    void m68k_writew_me(uint32_t address, uint32_t data)
{
    if (address < 0x100000) {
        T2WriteWord(SoundRam_cached, address & 0x7FFFF, data);
    } else {
        volatile int *scsp_rw_done_ptr =
            (volatile int *)((uintptr_t)&scsp_rw_block.done | 0x40000000);
        *scsp_rw_done_ptr = 0;
        volatile uint32_t *scsp_rw_req_ptr =
            (volatile uint32_t *)((uintptr_t)&scsp_rw_block.req | 0x40000000);
        *scsp_rw_req_ptr = SCSPREQ_MAKE_WRITEW(address, data);
        meUtilitySendInterrupt();
#ifdef SCSP_READ_INLINE
        meUtilityDcacheWritebackInvalidateAll();
#endif
        while (!*scsp_rw_done_ptr) { /*spin*/ }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * flush_cache_me:  Flush the data and instruction caches of the Media
 * Engine CPU.  Called by Q68 after a new block of code has been
 * translated.
 */
static __attribute__((section(".text.me"))) void flush_cache_me(void)
{
    meUtilityDcacheWritebackInvalidateAll();
    meUtilityIcacheInvalidateAll();
}

/*************************************************************************/

/* For the moment, we use a very simplistic memory allocator to manage ME
 * memory.  Since we only allocate or free during 68k code translation,
 * this should hopefully be good enough. */

/* Memory management structure prefixed to each block */
typedef struct BlockInfo_ BlockInfo;
struct BlockInfo_ {
    BlockInfo *next, *prev;  // Next and previous blocks sorted by address
    uint32_t size;           // Size of this block (excluding this structure)
    int free;                // Nonzero if this is a free block
    uint32_t pad[12];        // Pad to 1 cache line (64 bytes)
};

/* Base of memory region used for me_malloc() and friends */
static __attribute__((section(".bss.me"), aligned(64)))
    BlockInfo *me_malloc_base;

/*-----------------------------------------------------------------------*/

/**
 * me_malloc_init:  Initialize the memory arena used by me_malloc() and
 * friends.
 *
 * This routine may only be called on the Media Engine.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static __attribute__((section(".text.me"))) void me_malloc_init(void)
{
    /* Set the base address of the memory region to the kernel address,
     * so we can access the full 2MB of local memory (the first 32k is
     * unmapped at 0x00000000, presumably to catch NULL pointer accesses) */
    me_malloc_base = (BlockInfo *)0x80000000;

    /* Initialize the region; leave 64k free for the stack */
    me_malloc_base->next = NULL;
    me_malloc_base->prev = NULL;
    me_malloc_base->size = 0x1F0000 - sizeof(BlockInfo);
    me_malloc_base->free = 1;
}

/*-----------------------------------------------------------------------*/

/**
 * me_malloc:  Allocate a block of Media Engine memory.
 *
 * This routine may only be called on the Media Engine.
 *
 * [Parameters]
 *     size: Size of memory block to allocate
 * [Return value]
 *     Allocated memory block, or NULL on failure
 */
static __attribute__((section(".text.me"))) void *me_malloc(size_t size)
{
    /* Round the size up to a multiple of sizeof(BlockInfo) for simplicity */
    size = (size + sizeof(BlockInfo)-1)
           / sizeof(BlockInfo) * sizeof(BlockInfo);

    /* Find a free block of sufficient size and return it, splitting the
     * block if appropriate */
    BlockInfo *block;
    for (block = me_malloc_base; block; block = block->next) {
        if (!block->free) {
            continue;
        }
        if (block->size >= size) {
            void *ptr = (void *)((uintptr_t)block + sizeof(BlockInfo));
            if (block->size > size) {
                BlockInfo *split_block = (BlockInfo *)((uintptr_t)ptr + size);
                split_block->next = block->next;
                if (split_block->next) {
                    split_block->next->prev = split_block;
                }
                split_block->prev = block;
                block->next = split_block;
                split_block->size = block->size - (size + sizeof(BlockInfo));
                split_block->free = 1;
            }
            block->size = size;
            block->free = 0;
            return ptr;
        }
    }

    return NULL;
}

/*-----------------------------------------------------------------------*/

/**
 * me_realloc:  Adjust the size of a block of Media Engine memory.
 *
 * This routine may only be called on the Media Engine.
 *
 * [Parameters]
 *      ptr: Pointer to memory block to adjust
 *     size: New size of memory block
 * [Return value]
 *     Allocated memory block, or NULL on failure
 */
static __attribute__((section(".text.me")))
    void *me_realloc(void *ptr, size_t size)
{
    if (size == 0) {
        me_free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return me_malloc(size);
    }

    BlockInfo *block = (BlockInfo *)((uintptr_t)ptr - sizeof(BlockInfo));
    const size_t oldsize = block->size;
    size = (size + sizeof(BlockInfo)-1)
           / sizeof(BlockInfo) * sizeof(BlockInfo);
    if (size < oldsize - sizeof(BlockInfo)) {
        /* Adjust the block size downward and split off the remaining space
         * into a new free block (or add it to the next block, if the next
         * block is a free block) */
        block->size = size;
        BlockInfo *newblock = (BlockInfo *)
            ((uintptr_t)block + sizeof(BlockInfo) + size);
        if (block->next && block->next->free) {
            newblock->next = block->next->next;
            newblock->prev = block;
            newblock->size = block->next->size;
            newblock->free = 1;
            if (newblock->next) {
                newblock->next->prev = newblock;
            }
            block->next = newblock;
            newblock->size += oldsize - size;
        } else {
            newblock->next = block->next;
            newblock->prev = block;
            if (block->next) {
                block->next->prev = newblock;
            }
            block->next = newblock;
            newblock->size = oldsize - size - sizeof(BlockInfo);
            newblock->free = 1;
        }
        return ptr;
    } else if (size <= sizeof(BlockInfo)) {
        /* No need to adjust anything; just return the same block */
        return ptr;
    } else if (block->next && block->next->free
               && (sizeof(BlockInfo) + block->next->size) >= size - oldsize) {
        /* Append the next block to this one, then resize downward with a
         * recursive call */
        block->size += sizeof(BlockInfo) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
        return me_realloc(ptr, size);
    } else {
        /* No simple path, so alloc/copy/free */
        void *newptr = me_malloc(size);
        if (!newptr) {
            return NULL;
        }
        const size_t copysize = (oldsize < size) ? oldsize : size;
        memcpy(newptr, ptr, copysize);
        me_free(ptr);
        return newptr;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * me_free:  Free a block of Media Engine memory.
 *
 * This routine may only be called on the Media Engine.
 *
 * [Parameters]
 *     ptr: Pointer to memory block to free
 * [Return value]
 *     None
 */
static __attribute__((section(".text.me"))) void me_free(void *ptr)
{
    if (ptr != NULL) {
        BlockInfo *block = (BlockInfo *)((uintptr_t)ptr - sizeof(BlockInfo));
        block->free = 1;
        if (block->prev && block->prev->free) {
            block->prev->next = block->next;
            if (block->next) {
                block->next->prev = block->prev;
            }
            block->prev->size += block->size + sizeof(BlockInfo);
            block = block->prev;
        }
        if (block->next && block->next->free) {
            block->size += block->next->size + sizeof(BlockInfo);
            block->next = block->next->next;
            if (block->next) {
                block->next->prev = block;
            }
        }
    }
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
