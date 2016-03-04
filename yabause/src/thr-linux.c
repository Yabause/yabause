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
//#include <malloc.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////

// Thread handles for each Yabause subthread
static pthread_t thread_handle[YAB_NUM_THREADS];

//////////////////////////////////////////////////////////////////////////////

static void dummy_sighandler(int signum_unused) {}  // For thread sleep/wake

int YabThreadStart(unsigned int id, void (*func)(void *), void *arg)
{
   // Set up a dummy signal handler for SIGUSR1 so we can return from pause()
   // in YabThreadSleep()
   static const struct sigaction sa = {.sa_handler = dummy_sighandler};
   if (sigaction(SIGUSR1, &sa, NULL) != 0)
   {
      perror("sigaction(SIGUSR1)");
      return -1;
   }

   if (thread_handle[id])
   {
      fprintf(stderr, "YabThreadStart: thread %u is already started!\n", id);
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
   pause();
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

   pthread_kill(thread_handle[id], SIGUSR1);
}




typedef struct YabEventQueue_pthread
{
        int *buffer;
        int capacity;
        int size;
        int in;
        int out;
        pthread_mutex_t mutex;
        pthread_cond_t cond_full;
        pthread_cond_t cond_empty;
} YabEventQueue_pthread;


YabEventQueue * YabThreadCreateQueue( int qsize ){
    YabEventQueue_pthread * p = (YabEventQueue_pthread*)malloc(sizeof(YabEventQueue_pthread));
    p->buffer = (int*)malloc( sizeof(int)* qsize);
    p->capacity = qsize;
    p->size = 0;
    p->in = 0;
    p->out = 0;
    pthread_mutex_init(&p->mutex,NULL);
    pthread_cond_init(&p->cond_full,NULL);
    pthread_cond_init(&p->cond_empty,NULL);

    return (YabEventQueue *)p;
}

void YabThreadDestoryQueue( YabEventQueue * queue_t ){

    pthread_mutex_t mutex;
    YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
    mutex = queue->mutex;
    pthread_mutex_lock(&mutex);
    while (queue->size == queue->capacity)
            pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
    free(queue->buffer);
    free(queue);
    pthread_mutex_unlock(&mutex);
}



void YabAddEventQueue( YabEventQueue * queue_t, int evcode ){
    YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
    pthread_mutex_lock(&(queue->mutex));
    while (queue->size == queue->capacity)
            pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
     queue->buffer[queue->in] = evcode;
    ++ queue->size;
    ++ queue->in;
    queue->in %= queue->capacity;
    pthread_mutex_unlock(&(queue->mutex));
    pthread_cond_broadcast(&(queue->cond_empty));
}


int YabWaitEventQueue( YabEventQueue * queue_t ){
    int value;
    YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
    pthread_mutex_lock(&(queue->mutex));
    while (queue->size == 0)
            pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
    value = queue->buffer[queue->out];
    -- queue->size;
    ++ queue->out;
    queue->out %= queue->capacity;
    pthread_mutex_unlock(&(queue->mutex));
    pthread_cond_broadcast(&(queue->cond_full));
    return value;
}


typedef struct YabMutex_pthread
{
  pthread_mutex_t mutex;
} YabMutex_pthread;

void YabThreadLock( YabMutex * mtx ){
    YabMutex_pthread * pmtx;
    pmtx = (YabMutex_pthread *)mtx;
    pthread_mutex_lock(&pmtx->mutex);
}

void YabThreadUnLock( YabMutex * mtx ){
    YabMutex_pthread * pmtx;
    pmtx = (YabMutex_pthread *)mtx;
    pthread_mutex_unlock(&pmtx->mutex);
}

YabMutex * YabThreadCreateMutex(){
    YabMutex_pthread * mtx = (YabMutex_pthread *)malloc(sizeof(YabMutex_pthread));
    pthread_mutex_init( &mtx->mutex,NULL);
    return (YabMutex *)mtx;
}

void YabThreadFreeMutex( YabMutex * mtx ){
    if( mtx != NULL ){
        YabMutex_pthread * pmtx;
        pmtx = (YabMutex_pthread *)mtx;        
        pthread_mutex_destroy(&pmtx->mutex);
        free(pmtx);
    }
}




//////////////////////////////////////////////////////////////////////////////
