/*  src/thr-linux.c: Thread functions for Linux
    Copyright 2010 Andrew Church

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

#include "core.h"
#include "threads.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

//////////////////////////////////////////////////////////////////////////////

// Thread handles for each Yabause subthread
static pthread_t thread_handle[YAB_NUM_THREADS];
static pthread_mutex_t thread_mutex[YAB_NUM_THREADS];
static pthread_cond_t thread_cond[YAB_NUM_THREADS];

//////////////////////////////////////////////////////////////////////////////

int YabThreadStart(unsigned int id, void (*func)(void *), void *arg)
{
   if (thread_handle[id])
   {
      fprintf(stderr, "YabThreadStart: thread %u is already started!\n", id);
      return -1;
   }

   pthread_mutex_init(&thread_mutex[id], NULL);

   if (pthread_cond_init(&thread_cond[id], NULL) != 0)
   {
      perror("pthread_cond_init");
      return -1;
   }

   if ((errno = pthread_create(&thread_handle[id], NULL, (void *)func, arg)) != 0)
   {
      perror("pthread_create");
      return -1;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadWait(unsigned int id)
{
   if (!thread_handle[id])
      return;  // Thread wasn't running in the first place

   pthread_join(thread_handle[id], NULL);

   thread_handle[id] = 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadYield(void)
{
   sched_yield();
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadSleep(void)
{
   pthread_t thread;
   unsigned int i, id;

   id = YAB_NUM_THREADS;

   thread = pthread_self();
   for(i = 0;i < YAB_NUM_THREADS;i++)
   {
      if(thread_handle[i] == thread) {
         id = i;
      }
   }

   if (id == YAB_NUM_THREADS) return;

   pthread_mutex_lock(&thread_mutex[id]);
   pthread_cond_wait(&thread_cond[id], &thread_mutex[id]);
   pthread_mutex_unlock(&thread_mutex[id]);
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadRemoteSleep(unsigned int id)
{
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadWake(unsigned int id)
{
   if (!thread_handle[id])
      return;  // Thread isn't running

   pthread_mutex_lock(&thread_mutex[id]);
   pthread_cond_signal(&thread_cond[id]);
   pthread_mutex_unlock(&thread_mutex[id]);
}

//////////////////////////////////////////////////////////////////////////////
