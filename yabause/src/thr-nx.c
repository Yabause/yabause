/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <switch/kernel/thread.h>
#include <switch/kernel/uevent.h>
#include <switch/kernel/svc.h>
#include <switch/kernel/mutex.h>
#include "core.h"
#include "threads.h"

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////

// Thread handles for each Yabause subthread
static Thread thread_handle[YAB_NUM_THREADS] = {0};

//////////////////////////////////////////////////////////////////////////////


int YabThreadStart(unsigned int id, void * (*func)(void *), void *arg)
{
  Result result;
  if((result = threadCreate(&thread_handle[id],(ThreadFunc)func,arg, 0x10000, 0x2C, -2)) != 0 ){
    printf("Fail to create thread\n");
    return -1;    
  }

  if((result = threadStart(&thread_handle[id])) != 0 ){
    printf("Fail to start thread\n");
    return -1;
  }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadWait(unsigned int id)
{
   //if (!thread_handle[id])
   //   return;  // Thread wasn't running in the first place

  threadWaitForExit(&thread_handle[id]);
  //thread_handle[id] = 0;
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadYield(void)
{
   svcSleepThread(-2);
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadSleep(void)
{
   //pause();
}

void YabThreadUSleep( unsigned int stime )
{
	svcSleepThread(stime);
}


//////////////////////////////////////////////////////////////////////////////

void YabThreadRemoteSleep(unsigned int id)
{
}

//////////////////////////////////////////////////////////////////////////////

void YabThreadWake(unsigned int id)
{
   //if (&thread_handle[id])
   //   return;  // Thread isn't running

   threadClose(&thread_handle[id]);
}




typedef struct YabEventQueue_pthread
{
        int *buffer;
        int capacity;
        int size;
        int in;
        int out;
        Mutex mutex;
        UEvent cond_full;
        UEvent cond_empty;
} YabEventQueue_pthread;


YabEventQueue * YabThreadCreateQueue( int qsize ){
    YabEventQueue_pthread * p = (YabEventQueue_pthread*)malloc(sizeof(YabEventQueue_pthread));
    p->buffer = (int*)malloc( sizeof(int)* qsize);
    p->capacity = qsize;
    p->size = 0;
    p->in = 0;
    p->out = 0;
    mutexInit(&p->mutex);
    ueventCreate(&p->cond_full,false);
    ueventCreate(&p->cond_empty,false);

    return (YabEventQueue *)p;
}

void YabThreadDestoryQueue( YabEventQueue * queue_t ){

    Mutex mutex;
    YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
    mutex = queue->mutex;
    while (queue->size == queue->capacity)
            waiterForUEvent(&(queue->cond_full));  
    mutexLock(&mutex);
    free(queue->buffer);
    free(queue);
    mutexUnlock(&mutex);
}



void YabAddEventQueue( YabEventQueue * queue_t, int evcode ){
    YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
    while (queue->size == queue->capacity)
            waiterForUEvent(&(queue->cond_full));

    mutexLock(&(queue->mutex));
     queue->buffer[queue->in] = evcode;
    ++ queue->size;
    ++ queue->in;
    queue->in %= queue->capacity;
    mutexUnlock(&(queue->mutex));
    ueventSignal(&(queue->cond_empty));
    printf("YabAddEventQueue\n");
}


int YabClearEventQueue(YabEventQueue * queue_t) {
  YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
  mutexLock(&(queue->mutex));
  while (queue->size > 0) {
    --queue->size;
    ++queue->out;
    queue->out %= queue->capacity;
  }
  mutexUnlock(&(queue->mutex));
  return 0;
}

int YabWaitEventQueue( YabEventQueue * queue_t ){
    int value;
    YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
    while (queue->size == 0)
            waiterForUEvent(&(queue->cond_empty));
    printf("YabWaitEventQueue: Get\n");
    mutexLock(&(queue->mutex));
    value = queue->buffer[queue->out];
    -- queue->size;
    ++ queue->out;
    queue->out %= queue->capacity;
    mutexUnlock(&(queue->mutex));
    ueventSignal(&(queue->cond_full));
    return value;
}

int YaGetQueueSize(YabEventQueue * queue_t){
  int size = 0;
  YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
  mutexLock(&(queue->mutex));
  size = queue->size;
  mutexUnlock(&(queue->mutex));
  return size;
}


typedef struct YabMutex_pthread
{
  Mutex mutex;
} YabMutex_pthread;

void YabThreadLock( YabMutex * mtx ){
    YabMutex_pthread * pmtx;
    pmtx = (YabMutex_pthread *)mtx;
    mutexLock(&pmtx->mutex);
}

void YabThreadUnLock( YabMutex * mtx ){
    YabMutex_pthread * pmtx;
    pmtx = (YabMutex_pthread *)mtx;
    mutexUnlock(&pmtx->mutex);
}

YabMutex * YabThreadCreateMutex(){
    YabMutex_pthread * mtx = (YabMutex_pthread *)malloc(sizeof(YabMutex_pthread));
    mutexInit( &mtx->mutex);
    return (YabMutex *)mtx;
}

void YabThreadFreeMutex( YabMutex * mtx ){
    if( mtx != NULL ){
        YabMutex_pthread * pmtx;
        pmtx = (YabMutex_pthread *)mtx;        
        //pthread_mutex_destroy(&pmtx->mutex);
        free(pmtx);
    }
}


void YabThreadSetCurrentThreadAffinityMask(int mask)
{
}



int YabThreadGetCurrentThreadAffinityMask()
{
  return 0;
}



//////////////////////////////////////////////////////////////////////////////
