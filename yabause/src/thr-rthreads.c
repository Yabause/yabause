/*  src/thr-rthreads.c: Thread functions for Libretro
    Copyright 2019 barbudreadmon

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

#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#endif

#include "core.h"
#include "threads.h"
#include "rthreads/rthreads.h"

struct thd_s {
	int running;
	sthread_t *thd;
	slock_t *mutex;
	scond_t *cond;
};

static struct thd_s thread_handle[YAB_NUM_THREADS];

//////////////////////////////////////////////////////////////////////////////

int YabThreadStart(unsigned int id, void (*func)(void *), void *arg)
{
	if (thread_handle[id].running == 1)
	{
		return -1;
	}

	thread_handle[id].mutex = slock_new();

	if ((thread_handle[id].cond = scond_new()) == NULL)
	{
		return -1;
	}

	if ((thread_handle[id].thd = sthread_create((void *)func, arg)) == NULL)
	{
		return -1;
	}

	thread_handle[id].running = 1;

	return 0;
}

void YabThreadWait(unsigned int id)
{
	if (thread_handle[id].running != 1)
		return;  // Thread wasn't running in the first place

	sthread_join(thread_handle[id].thd);

	thread_handle[id].running = 0;
}

void YabThreadYield(void)
{
#ifdef _WIN32
	SwitchToThread();
#else
	sched_yield();
#endif
}

void YabThreadSleep(void)
{
	unsigned int i, id;

	id = YAB_NUM_THREADS;

	for(i = 0;i < YAB_NUM_THREADS;i++)
	{
		if(sthread_isself(thread_handle[i].thd)) {
			id = i;
		}
	}

	if (id == YAB_NUM_THREADS) return;

	slock_lock(thread_handle[id].mutex);
	scond_wait(thread_handle[id].cond, thread_handle[id].mutex);
	slock_unlock(thread_handle[id].mutex);
}

void YabThreadRemoteSleep(unsigned int id)
{
}

void YabThreadWake(unsigned int id)
{
	if (thread_handle[id].running != 1)
		return;  // Thread wasn't running in the first place

	slock_lock(thread_handle[id].mutex);
	scond_signal(thread_handle[id].cond);
	slock_unlock(thread_handle[id].mutex);
}

//////////////////////////////////////////////////////////////////////////////
