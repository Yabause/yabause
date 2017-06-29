/*  src/thr-windows.c: Windows thread functions
    Copyright 2013 Theo Berkau. Based on code by Andrew Church and Lawrence Sebald.

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

/*! \file thr-windows.c
    \brief Windows port's threading functions
*/

#include <windows.h>
#include "core.h"
#include "threads.h"

struct thd_s {
   int running;
   HANDLE thd;
   void (*func)(void *);
   void *arg;
   CRITICAL_SECTION mutex;
   HANDLE cond;
};

static struct thd_s thread_handle[YAB_NUM_THREADS];
static int hnd_key;
static int hnd_key_once=FALSE;

//////////////////////////////////////////////////////////////////////////////

static DWORD wrapper(void *hnd) 
{
   struct thd_s *hnds = (struct thd_s *)hnd;

   EnterCriticalSection(&hnds->mutex);

   /* Set the handle for the thread, and call the actual thread function. */
   TlsSetValue(hnd_key, hnd);
   hnds->func(hnds->arg);

   LeaveCriticalSection(&hnds->mutex);

   return 0;
}

int YabThreadStart(unsigned int id, void (*func)(void *), void *arg) 
{ 
   if (!hnd_key_once)
   {
      hnd_key=TlsAlloc();
      hnd_key_once = 1;
   }

   if (thread_handle[id].running)
   {
      fprintf(stderr, "YabThreadStart: thread %u is already started!\n", id);
      return -1;
   }
   
   // Create CS and condition variable for thread
   InitializeCriticalSection(&thread_handle[id].mutex);
   if ((thread_handle[id].cond = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
   {
      perror("CreateEvent");
   	  return -1;
   }

   thread_handle[id].func = func;
   thread_handle[id].arg = arg;

   if ((thread_handle[id].thd = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wrapper, &thread_handle[id], 0, NULL)) == NULL)
   {
      perror("CreateThread");
      return -1;
   }
   
   thread_handle[id].running = 1;

   return 0; 
}

void YabThreadWait(unsigned int id) 
{
   if (!thread_handle[id].thd)
      return;  // Thread wasn't running in the first place

   WaitForSingleObject(thread_handle[id].thd,INFINITE);
   CloseHandle(thread_handle[id].thd);
   thread_handle[id].thd = NULL;
   thread_handle[id].running = 0;
   if (thread_handle[id].cond)
   	   CloseHandle(thread_handle[id].cond);
}

void YabThreadYield(void) 
{
	SleepEx(0, 0);
}

void YabThreadUSleep( u32 stime )
{
	SleepEx(stime/1000, 0);
}

void YabThreadSleep(void) 
{
   struct thd_s *thd = (struct thd_s *)TlsGetValue(hnd_key);
   WaitForSingleObject(thd->cond,INFINITE);
}

void YabThreadRemoteSleep(unsigned int id) 
{
   if (!thread_handle[id].thd)
      return;  // Thread wasn't running in the first place

   WaitForSingleObject(thread_handle[id].cond,INFINITE);
}

void YabThreadWake(unsigned int id) 
{
   if (!thread_handle[id].thd)
      return;  // Thread wasn't running in the first place

   SetEvent(thread_handle[id].cond);
}

typedef struct YabEventQueue_win32
{
	int *buffer;
	int capacity;
	int size;
	int in;
	int out;
	CRITICAL_SECTION mutex;
	HANDLE cond_full;
	HANDLE cond_empty;
} YabEventQueue_win32;


YabEventQueue * YabThreadCreateQueue(int qsize){
	YabEventQueue_win32 * p = (YabEventQueue_win32*)malloc(sizeof(YabEventQueue_win32));
	p->buffer = (int*)malloc(sizeof(int)* qsize);
	p->capacity = qsize;
	p->size = 0;
	p->in = 0;
	p->out = 0;
  InitializeCriticalSection(&p->mutex);
  p->cond_full = CreateEvent(NULL, FALSE, FALSE, NULL);
  p->cond_empty = CreateEvent(NULL, FALSE, FALSE, NULL);
	return (YabEventQueue *)p;
}

void YabThreadDestoryQueue(YabEventQueue * queue_t){
#if 1
  CRITICAL_SECTION mutex;
  YabEventQueue_win32 * queue = (YabEventQueue_win32*)queue_t;
  mutex = queue->mutex;
  while (queue->size == queue->capacity)
    WaitForSingleObject(queue->cond_full,INFINITE);

  EnterCriticalSection(&mutex);
  free(queue->buffer);
  free(queue);
  LeaveCriticalSection(&mutex);
#else
	//pthread_mutex_t mutex;
	YabEventQueue_win32 * queue = (YabEventQueue_pthread*)queue_t;
	mutex = queue->mutex;
	pthread_mutex_lock(&mutex);
	while (queue->size == queue->capacity)
		pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
	free(queue->buffer);
	free(queue);
	pthread_mutex_unlock(&mutex);
#endif
}



void YabAddEventQueue(YabEventQueue * queue_t, int evcode){
#if 1
  YabEventQueue_win32 * queue = (YabEventQueue_win32*)queue_t;
  while (queue->size == queue->capacity)
    WaitForSingleObject(queue->cond_full, INFINITE);

  EnterCriticalSection(&(queue->mutex));
  queue->buffer[queue->in] = evcode;
  ++queue->size;
  ++queue->in;
  queue->in %= queue->capacity;
  LeaveCriticalSection(&(queue->mutex));
  SetEvent(queue->cond_empty);
#else
	YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == queue->capacity)
		pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
	queue->buffer[queue->in] = evcode;
	++queue->size;
	++queue->in;
	queue->in %= queue->capacity;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_empty));
#endif
}


int YabWaitEventQueue(YabEventQueue * queue_t){
#if 1
  int value;
  YabEventQueue_win32 * queue = (YabEventQueue_win32*)queue_t;
  while (queue->size == 0)
    WaitForSingleObject(queue->cond_empty, INFINITE);

  EnterCriticalSection(&(queue->mutex));
  value = queue->buffer[queue->out];
  --queue->size;
  ++queue->out;
  queue->out %= queue->capacity;
  LeaveCriticalSection(&(queue->mutex));
  SetEvent(queue->cond_full);
  return value; 
#else
	int value;
	YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == 0)
		pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
	value = queue->buffer[queue->out];
	--queue->size;
	++queue->out;
	queue->out %= queue->capacity;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_full));
	return value;
#endif
}

int YaGetQueueSize(YabEventQueue * queue_t){
  int size = 0;
  YabEventQueue_win32 * queue = (YabEventQueue_win32*)queue_t;
  EnterCriticalSection(&(queue->mutex));
  size = queue->size;
  LeaveCriticalSection(&(queue->mutex));
  return size;
}

typedef struct YabMutex_win32
{
	CRITICAL_SECTION mutex;
} YabMutex_win32;

void YabThreadLock(YabMutex * mtx){
	YabMutex_win32 * pmtx;
	pmtx = (YabMutex_win32 *)mtx;
  if (mtx == NULL) return;
	EnterCriticalSection(&pmtx->mutex);
}

void YabThreadUnLock(YabMutex * mtx){
	YabMutex_win32 * pmtx;
  if (mtx == NULL) return;
	pmtx = (YabMutex_win32 *)mtx;
	LeaveCriticalSection(&pmtx->mutex);
}

YabMutex * YabThreadCreateMutex(){
	YabMutex_win32 * mtx = (YabMutex_win32 *)malloc(sizeof(YabMutex_win32));
	InitializeCriticalSection(&mtx->mutex);
	return (YabMutex *)mtx;
}

void YabThreadFreeMutex( YabMutex * mtx ){

	if (mtx != NULL){
		DeleteCriticalSection(&((YabMutex_win32 *)mtx)->mutex);
		free(mtx);
	}
}

void YabThreadSetCurrentThreadAffinityMask(int mask)
{
	SetThreadIdealProcessor(GetCurrentThread(), mask);
}

int YabThreadGetCurrentThreadAffinityMask(){
	return GetCurrentProcessorNumber();
}

//////////////////////////////////////////////////////////////////////////////
