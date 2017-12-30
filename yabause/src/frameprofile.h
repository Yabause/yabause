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

#ifndef _FRAME_PROFILE_H_
#define _FRAME_PROFILE_H_

#if defined(WEBINTERFACE)
//#define _VDP_PROFILE_
#endif


#ifdef __cplusplus
extern "C" {
#endif


#ifdef _VDP_PROFILE_
void FrameProfileInit();
void FrameProfileAdd(char * p);
void FrameProfileShow();
#else
#define FrameProfileInit
#define FrameProfileAdd
#define FrameProfileShow
#endif

// rendering performance
typedef struct {
  char event[32];
  u32 time;
  u32 tid;
} ProfileInfo;


int FrameGetLastProfile(ProfileInfo ** p, int * size);
int FrameResume();

#ifdef __cplusplus
}
#endif


#endif

