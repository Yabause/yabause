/*  src/psp/sys.c: PSP system-related functions
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

#include "../yabause.h"

#include "common.h"
#include "sys.h"

/*************************************************************************/
/*************************************************************************/

/**
 * psp_strerror:  Convert a PSP error code into a string, like strerror().
 *
 * [Parameters]
 *     code: Error code
 * [Return value]
 *     Corresponding error string
 * [Notes]
 *     The returned string is stored in a static buffer, so it will be
 *     destroyed on the next call to psp_strerror().
 */
const char *psp_strerror(const int32_t code)
{
    static char return_buf[100];

    /* Give descriptive messages for a few common codes */
    const char *message = NULL;
    switch (code) {
        case SCE_KERNEL_ERROR_NODEV: message = "No such device";      break;
        case SCE_KERNEL_ERROR_BADF:  message = "Bad file descriptor"; break;
        case SCE_KERNEL_ERROR_INVAL: message = "Invalid argument";    break;
        case SCE_KERNEL_ERROR_NOMEM: message = "Out of memory";       break;
    }

    if (message) {
        snprintf(return_buf, sizeof(return_buf), "%08X: %s", code, message);
    } else {
        snprintf(return_buf, sizeof(return_buf), "%08X", code);
    }
    return return_buf;
}

/*************************************************************************/

/**
 * sys_start_thread:  Start a new thread, returning the created thread
 * handle.
 *
 * [Parameters]
 *          name: Thread name
 *         entry: Thread entry address (function pointer)
 *      priority: Thread priority
 *     stacksize: Thread stack size
 *          args: Size of data to pass as thread argument
 *          argp: Pointer to thread argument data
 * [Return value]
 *     Nonnegative = thread handle, negative = error code
 */
int32_t sys_start_thread(const char *name, void *entry, int priority,
                         int stacksize, int args, void *argp)
{
    if (!name || !entry || priority < 0 || stacksize < 0) {
        DMSG("Invalid parameters: %p[%s] %p %d %d %d %p",
             name, name ? name : "", entry, priority, stacksize, args, argp);
        return SCE_KERNEL_ERROR_INVAL;
    }

    SceUID handle = sceKernelCreateThread(name, entry, priority, stacksize,
                                          0, NULL);
    if (handle < 0) {
        DMSG("Failed to create thread \"%s\": %s", name, psp_strerror(handle));
        return handle;
    }

    int32_t res = sceKernelStartThread(handle, args, argp);
    if (res < 0) {
        DMSG("Failed to start thread \"%s\": %s", name, psp_strerror(res));
        sceKernelDeleteThread(handle);
        return res;
    }

    return handle;
}

/*-----------------------------------------------------------------------*/

/**
 * sys_delete_thread_if_stopped:  Check whether the given thread has
 * exited, and if so, delete the thread and return its exit status.
 *
 * [Parameters]
 *         thread: Thread handle
 *     status_ret: Pointer to variable to receive exit status (may be NULL)
 * [Return value]
 *     Nonzero if the thread was deleted, else zero
 */
int sys_delete_thread_if_stopped(SceUID thread, int *status_ret)
{
    SceKernelThreadInfo thinfo;
    memset(&thinfo, 0, sizeof(thinfo));
    thinfo.size = sizeof(thinfo);
    int res = sceKernelReferThreadStatus(thread, &thinfo);
    if (res < 0) {
        DMSG("sceKernelReferThreadStatus(0x%08X) failed: %s",
             thread, psp_strerror(res));
        sceKernelTerminateThread(thread);
    } else if (thinfo.status & (PSP_THREAD_RUNNING | PSP_THREAD_READY | PSP_THREAD_WAITING)) {
        return 0;
    } else if (thinfo.status & PSP_THREAD_STOPPED) {
        res = thinfo.exitStatus;
    } else {
        res = 0x80000000 | thinfo.status;
        sceKernelTerminateThread(thread);
    }
    sceKernelDeleteThread(thread);
    if (status_ret) {
        *status_ret = res;
    }
    return 1;
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
