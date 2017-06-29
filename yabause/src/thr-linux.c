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


#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#ifdef ARCH_IS_MACOSX
pid_t gettid(void)
{
    return syscall(SYS_gettid);
}
#endif
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

void YabThreadUSleep( unsigned int stime )
{
	usleep(stime);
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

int YaGetQueueSize(YabEventQueue * queue_t){
  int size = 0;
  YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
  pthread_mutex_lock(&(queue->mutex));
  size = queue->size;
  pthread_mutex_unlock(&(queue->mutex));
  return size;
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



#define _GNU_SOURCE
#include <sched.h>

#if !(defined ARCH_IS_LINUX) || (defined ANDROID)
 
extern int clone(int (*)(void*), void*, int, void*, ...);
extern int unshare(int);
extern int sched_getcpu(void);
extern int setns(int, int);

#ifdef __LP64__
#define CPU_SETSIZE 1024
#else
#define CPU_SETSIZE 32
#endif

#define __CPU_BITTYPE  unsigned long int  /* mandated by the kernel  */
#define __CPU_BITS     (8 * sizeof(__CPU_BITTYPE))
#define __CPU_ELT(x)   ((x) / __CPU_BITS)
#define __CPU_MASK(x)  ((__CPU_BITTYPE)1 << ((x) & (__CPU_BITS - 1)))

typedef struct {
  __CPU_BITTYPE  __bits[ CPU_SETSIZE / __CPU_BITS ];
} cpu_set_t;

extern int sched_setaffinity(pid_t pid, size_t setsize, const cpu_set_t* set);

extern int sched_getaffinity(pid_t pid, size_t setsize, cpu_set_t* set);

#define CPU_ZERO(set)          CPU_ZERO_S(sizeof(cpu_set_t), set)
#define CPU_SET(cpu, set)      CPU_SET_S(cpu, sizeof(cpu_set_t), set)
#define CPU_CLR(cpu, set)      CPU_CLR_S(cpu, sizeof(cpu_set_t), set)
#define CPU_ISSET(cpu, set)    CPU_ISSET_S(cpu, sizeof(cpu_set_t), set)
#define CPU_COUNT(set)         CPU_COUNT_S(sizeof(cpu_set_t), set)
#define CPU_EQUAL(set1, set2)  CPU_EQUAL_S(sizeof(cpu_set_t), set1, set2)

#define CPU_AND(dst, set1, set2)  __CPU_OP(dst, set1, set2, &)
#define CPU_OR(dst, set1, set2)   __CPU_OP(dst, set1, set2, |)
#define CPU_XOR(dst, set1, set2)  __CPU_OP(dst, set1, set2, ^)

#define __CPU_OP(dst, set1, set2, op)  __CPU_OP_S(sizeof(cpu_set_t), dst, set1, set2, op)

/* Support for dynamically-allocated cpu_set_t */

#define CPU_ALLOC_SIZE(count) \
  __CPU_ELT((count) + (__CPU_BITS - 1)) * sizeof(__CPU_BITTYPE)

#define CPU_ALLOC(count)  __sched_cpualloc((count))
#define CPU_FREE(set)     __sched_cpufree((set))

extern cpu_set_t* __sched_cpualloc(size_t count);
extern void       __sched_cpufree(cpu_set_t* set);

#define CPU_ZERO_S(setsize, set)  __builtin_memset(set, 0, setsize)

#define CPU_SET_S(cpu, setsize, set) \
  do { \
    size_t __cpu = (cpu); \
    if (__cpu < 8 * (setsize)) \
      (set)->__bits[__CPU_ELT(__cpu)] |= __CPU_MASK(__cpu); \
  } while (0)

#define CPU_CLR_S(cpu, setsize, set) \
  do { \
    size_t __cpu = (cpu); \
    if (__cpu < 8 * (setsize)) \
      (set)->__bits[__CPU_ELT(__cpu)] &= ~__CPU_MASK(__cpu); \
  } while (0)

#define CPU_ISSET_S(cpu, setsize, set) \
  (__extension__ ({ \
    size_t __cpu = (cpu); \
    (__cpu < 8 * (setsize)) \
      ? ((set)->__bits[__CPU_ELT(__cpu)] & __CPU_MASK(__cpu)) != 0 \
      : 0; \
  }))

#define CPU_EQUAL_S(setsize, set1, set2)  (__builtin_memcmp(set1, set2, setsize) == 0)

#define CPU_AND_S(setsize, dst, set1, set2)  __CPU_OP_S(setsize, dst, set1, set2, &)
#define CPU_OR_S(setsize, dst, set1, set2)   __CPU_OP_S(setsize, dst, set1, set2, |)
#define CPU_XOR_S(setsize, dst, set1, set2)  __CPU_OP_S(setsize, dst, set1, set2, ^)

#define __CPU_OP_S(setsize, dstset, srcset1, srcset2, op) \
  do { \
    cpu_set_t* __dst = (dstset); \
    const __CPU_BITTYPE* __src1 = (srcset1)->__bits; \
    const __CPU_BITTYPE* __src2 = (srcset2)->__bits; \
    size_t __nn = 0, __nn_max = (setsize)/sizeof(__CPU_BITTYPE); \
    for (; __nn < __nn_max; __nn++) \
      (__dst)->__bits[__nn] = __src1[__nn] op __src2[__nn]; \
  } while (0)

#define CPU_COUNT_S(setsize, set)  __sched_cpucount((setsize), (set))

extern int __sched_cpucount(size_t setsize, cpu_set_t* set);

#endif

void YabThreadSetCurrentThreadAffinityMask(int mask)
{
#if 0    
    int err, syscallres;
    pid_t pid = gettid();

	cpu_set_t my_set;        /* Define your cpu_set bit mask. */
	CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
	CPU_SET(mask, &my_set);
	CPU_SET(mask+4, &my_set);
	sched_setaffinity(pid,sizeof(my_set), &my_set);
#endif    
}

#include <sys/syscall.h>
//...
int getCpuId() {

    unsigned cpu;
    if (syscall(__NR_getcpu, &cpu, NULL, NULL) < 0) {
        return -1;
    } else {
        return (int) cpu;
    }
}

int YabThreadGetCurrentThreadAffinityMask()
{
	//return sched_getcpu(); //my_set.__bits;
    return 0;
}



//////////////////////////////////////////////////////////////////////////////
