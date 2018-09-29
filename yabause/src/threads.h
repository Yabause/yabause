/*  src/threads.h: Constants and prototypes for thread handling
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

#ifndef THREADS_H
#define THREADS_H

#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif
///////////////////////////////////////////////////////////////////////////
// Thread constants
///////////////////////////////////////////////////////////////////////////

// Thread IDs
enum {
   YAB_THREAD_SCSP = 0,
   YAB_THREAD_MSH2,
   YAB_THREAD_SSH2,
   YAB_THREAD_VDP,
   YAB_THREAD_GDBSTUBCLIENT,
   YAB_THREAD_GDBSTUBLISTENER,
   YAB_THREAD_NETLINKLISTENER,
   YAB_THREAD_NETLINKCONNECT,
   YAB_THREAD_NETLINKCLIENT,
   YAB_THREAD_OPENAL,
   YAB_THREAD_VIDSOFT_LAYER_NBG3,
   YAB_THREAD_VIDSOFT_LAYER_NBG2,
   YAB_THREAD_VIDSOFT_LAYER_NBG1,
   YAB_THREAD_VIDSOFT_LAYER_NBG0,
   YAB_THREAD_VIDSOFT_LAYER_RBG0,
   YAB_THREAD_VIDSOFT_VDP1,
   YAB_THREAD_VIDSOFT_PRIORITY_0,
   YAB_THREAD_VIDSOFT_PRIORITY_1,
   YAB_THREAD_VIDSOFT_PRIORITY_2,
   YAB_THREAD_VIDSOFT_PRIORITY_3,
   YAB_THREAD_VIDSOFT_PRIORITY_4,
   YAB_THREAD_VIDSOFT_LAYER_SPRITE,

   YAB_THREAD_VDP1_0,
   YAB_THREAD_VDP1_1,
   YAB_THREAD_VDP1_2,
   YAB_THREAD_VDP1_3,

   YAB_THREAD_VDP2_BACK,
   YAB_THREAD_VDP2_LINE,
   YAB_THREAD_VDP2_NBG3,
   YAB_THREAD_VDP2_NBG2,
   YAB_THREAD_VDP2_NBG1,
   YAB_THREAD_VDP2_NBG0,
   YAB_THREAD_VDP2_RBG0,
   YAB_THREAD_VDP2_RBG1,
   YAB_NUM_THREADS      // Total number of subthreads
};

// Number of (boolean) semaphores available per thread
#define YAB_NUM_SEMAPHORES  2

///////////////////////////////////////////////////////////////////////////
// Thread functions (must be implemented by the port; only used if
// yabauseinit_struct.usethreads != 0 at YabauseInit() time)
///////////////////////////////////////////////////////////////////////////

// YabThreadStart:  Start a new thread for the given function.  Only one
// thread will be started for each thread ID (YAB_THREAD_*).  Returns 0 on
// success, -1 on error.
int YabThreadStart(unsigned int id, void (*func)(void *), void *arg);

// YabThreadWait:  Wait for the given ID's thread to terminate.  Returns
// immediately if no thread has been started on the given ID.
void YabThreadWait(unsigned int id);

// YabThreadCancel: send a cancellation request to a specific thread.
void YabThreadCancel(unsigned int id);

// YabThreadYield:  Yield CPU execution to another thread.
void YabThreadYield(void);

// YabThreadSleep:  Put the current thread to sleep.
void YabThreadSleep(void);

// YabThreadSleep:  Put the specified thread to sleep.
void YabThreadRemoteSleep(unsigned int id);

// YabThreadWake:  Wake up the given thread if it is asleep.
void YabThreadWake(unsigned int id);

// Event Queue
typedef void * YabEventQueue;

// YabThreadCreateQueue:
YabEventQueue * YabThreadCreateQueue( int qsize );

// YabThreadDestoryQueue:
void YabThreadDestoryQueue( YabEventQueue * queue_t );

// YabAddEventQueue: send event
void YabAddEventQueue( YabEventQueue * queue_t, void* evcode );

// YabWaitEventQueue: recive event
void* YabWaitEventQueue( YabEventQueue * queue_t );
 
int YaGetQueueSize(YabEventQueue * queue_t);

void YabWaitEmptyQueue( YabEventQueue * queue_t );

typedef void * YabMutex;

void YabThreadLock( YabMutex * mtx );
void YabThreadUnLock( YabMutex * mtx );
YabMutex * YabThreadCreateMutex();
void YabThreadFreeMutex( YabMutex * mtx );

typedef void * YabCond;

void YabThreadCondWait(YabCond *cond, YabMutex * mtx);
void YabThreadCondSignal(YabCond *cond);
YabCond * YabThreadCreateCond();
void YabThreadFreeCond( YabCond * mtx );

typedef void * YabBarrier;

void YabThreadBarrierWait(YabBarrier *bar);
YabBarrier * YabThreadCreateBarrier(int nbWorkers);

void YabThreadSetCurrentThreadAffinityMask(int mask);
int YabThreadGetCurrentThreadAffinityMask();

void YabThreadUSleep( unsigned int stime );

///////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif  // THREADS_H
