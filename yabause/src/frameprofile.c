#include "yabause.h"
#include "frameprofile.h"
#include "osdcore.h"

#ifdef _VDP_PROFILE_

// rendering performance
typedef struct {
	char event[32];
	u32 time;
} ProfileInfo;

#define MAX_PROFILE_COUNT (256)
ProfileInfo g_pf[MAX_PROFILE_COUNT];
u32 current_profile_index = 0;


void FrameProfileInit(){
	current_profile_index = 0;
}

void FrameProfileAdd(char * p){
	u32 time;
	if (current_profile_index >= MAX_PROFILE_COUNT) return;
	time = clock();
	strcpy(g_pf[current_profile_index].event, p);
	g_pf[current_profile_index].time = time;
	current_profile_index++;
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
}
#endif