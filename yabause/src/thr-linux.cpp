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

#define _GNU_SOURCE
#include <sched.h>

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
#include <sys/stat.h>
#include <sys/types.h>


#if GCC_VERSION < 9
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

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

extern "C" {

int YabThreadStart(unsigned int id, void* (*func)(void *), void *arg)
{
   // Set up a dummy signal handler for SIGUSR1 so we can return from pause()
   // in YabThreadSleep()
   static struct sigaction sa; // = {.sa_handler = dummy_sighandler};
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

   if (pthread_create(&thread_handle[id], NULL, func, arg) != 0)
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


int YabClearEventQueue(YabEventQueue * queue_t) {
  YabEventQueue_pthread * queue = (YabEventQueue_pthread*)queue_t;
  pthread_mutex_lock(&(queue->mutex));
  while (queue->size > 0) {
    --queue->size;
    ++queue->out;
    queue->out %= queue->capacity;
  }
  pthread_mutex_unlock(&(queue->mutex));
  return 0;
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

void YabThreadSetCurrentThreadAffinityMask(int mask)
{
#if defined(__XU4__) || defined(IOS) || defined(__JETSON__)
	return;
#else
	#if !defined(ANDROID) // it needs more than android-21
	    int err, syscallres;
		#ifdef SYS_gettid
		    pid_t pid = syscall(SYS_gettid);
		#else
		    pid_t pid = gettid();
		#endif    
	    mask = 1 << mask;
	    syscallres = syscall(__NR_sched_setaffinity, pid, sizeof(mask), &mask);
	    if (syscallres)
	    {
	        err = errno;
	        //LOG("Error in the syscall setaffinity: mask=%d=0x%x err=%d=0x%x", mask, mask, err, err);
	    }
	
	//    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
	//    CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
	//    CPU_SET(mask, &my_set);
	//	CPU_SET(mask+4, &my_set);
	//    sched_setaffinity(pid,sizeof(my_set), &my_set);
	#endif
#endif
}

#include <sys/syscall.h>
//...
int getCpuId() {
#if 0
    unsigned cpu;
    if (syscall(__NR_getcpu, &cpu, NULL, NULL) < 0) {
        return -1;
    } else {
        return (int) cpu;
    }
 #endif
  return 0;
}


int YabThreadGetCurrentThreadAffinityMask()
{
#if 1 // it needs more than android-21
	return getCpuId();  //sched_getcpu(); //my_set.__bits;
#else
	return 0;
#endif

}

int YabMakeCleanDir( const char * dirname ){
#if defined(IOS)
  return 0;
#elif defined(ANDROID)
  std::string cmd;
  cmd = "exec rm -r " + std::string(dirname) + "/*";
  system(cmd.c_str());
  rmdir(dirname);
  mkdir(dirname,777);
#else
  fs::remove_all(dirname);
  if (fs::create_directories(dirname) == false) {
    printf("Fail to create %s\n", dirname);
  }
#endif  
  return 0;
}

int YabCopyFile( const char * src, const char * dst) {
#if defined(IOS)
  return 0;
#elif defined(ANDROID)
  std::string cmd;
  cmd = "exec cp -f " + std::string(src) + " " + std::string(dst);
  system(cmd.c_str());
#else
  std::error_code ec;
  if( !fs::copy_file(src,dst,fs::copy_options::overwrite_existing,ec ) ){
    return -1;
  }
#endif  
  return 0;
}


 #include <time.h>

int YabNanosleep(u64 ns) {
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = ns*1000;   
  nanosleep(&ts,NULL);
  return 0;
}

} // extern "C"

//////////////////////////////////////////////////////////////////////////////

