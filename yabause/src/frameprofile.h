#ifndef _FRAME_PROFILE_H_
#define _FRAME_PROFILE_H_

#define _VDP_PROFILE_

#ifdef _VDP_PROFILE_
void FrameProfileInit();
void FrameProfileAdd(char * p);
void FrameProfileShow();
#else
#define FrameProfileInit
#define FrameProfileAdd
#define FrameProfileShow
#endif

#endif

