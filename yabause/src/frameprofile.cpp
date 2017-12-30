/*  Copyright 2016 devMiyax(smiyaxdev@gmail.com)

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

#include "yabause.h"
#include "frameprofile.h"
#include "osdcore.h"
#ifdef WIN32
#include <windows.h>
#endif
#ifdef _VDP_PROFILE_
#include "time.h"
#include "threads.h"


#include <atomic>
using std::atomic;

enum enDebugState {
  FP_NORMAL,
  FP_REQUESTED,
  FP_COLLECTING,
  FP_FINISHED,
};

std::atomic<int> _frame_profile_status(FP_NORMAL);
YabMutex * _mutex = NULL; // YabThreadCreateMutex;
extern "C" {

#define MAX_PROFILE_COUNT (256)
ProfileInfo g_pf[MAX_PROFILE_COUNT];
u32 current_profile_index = 0;


void FrameProfileInit(){
  while (_frame_profile_status.load() == FP_FINISHED) {
    YabThreadUSleep(10000);
  }

  if (_mutex == NULL) {
    _mutex = YabThreadCreateMutex();
  }

	current_profile_index = 0;
}

void FrameProfileAdd(char * p){
	u32 time;

  YabThreadLock(_mutex);

  if (current_profile_index >= MAX_PROFILE_COUNT) {
    YabThreadUnLock(_mutex);
    return;
  }



#ifdef WIN32
	static LARGE_INTEGER freq = { 0 };
	u64 ticks;
	if (freq.QuadPart == 0){
		QueryPerformanceFrequency(&freq);
	}
	QueryPerformanceCounter((LARGE_INTEGER *)&ticks);
	ticks = ticks * 1000000L / freq.QuadPart;
	time = ticks;
#else
	time = clock();
#endif
	sprintf(g_pf[current_profile_index].event, "%s(%d)",p,YabThreadGetCurrentThreadAffinityMask());
	g_pf[current_profile_index].time = time;
	g_pf[current_profile_index].tid = YabThreadGetCurrentThreadAffinityMask();
	current_profile_index++;

  YabThreadUnLock(_mutex);
}


void FrameProfileShow(){
	u32 intime = 0;
	u32 extime = 0;
	int i = 0;
	if (current_profile_index <= 0) return;
	for ( i = 0; i < current_profile_index; i++){
		if (i > 0){
			intime = g_pf[i].time - g_pf[i - 1].time;
			extime += intime;
		}
		OSDAddFrameProfileData(g_pf[i].event, intime);
	}
  if (_frame_profile_status.load() == FP_REQUESTED) {
    _frame_profile_status.store(FP_FINISHED);
  }else{
    _frame_profile_status.store(FP_NORMAL);
  }
}



int FrameGetLastProfile( ProfileInfo ** p, int * size ) {
  
  if (_frame_profile_status == FP_REQUESTED) return -1; // dupe

  _frame_profile_status.store(FP_REQUESTED);
  while (_frame_profile_status.load() != FP_FINISHED) {
  }

  (*p) = (ProfileInfo*)malloc(sizeof(ProfileInfo) * current_profile_index);
  for (int i = 0; i < current_profile_index; i++) {
    memcpy( &(*p)[i], &g_pf[i], sizeof(ProfileInfo) );
  }
  *size = current_profile_index;
  return 0;
}

int FrameResume() {
  _frame_profile_status.store(FP_NORMAL);
  return 0;
}

}


#else
int FrameGetLastProfile(ProfileInfo ** p, int * size) {
  return -1;
}
int FrameResume() {
  return -1;
}
#endif


