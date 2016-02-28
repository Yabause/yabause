/*  Copyright 2012 Guillaume Duhamel
	Copyright 2016 devMiyax(smiyaxdev@gmail.com)

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

#ifndef NANOVG_OSDCORE_H
#define NANOVG_OSDCORE_H

extern OSD_struct OSDNnovg;

#if 0
#include "core.h"

#define OSDCORE_DUMMY    0
#define OSDCORE_GLUT     1
#define OSDCORE_SOFT     2
#define OSDCORE_NANOVG   3

#define OSDCORE_DEFAULT OSDCORE_NANOVG

#define OSDMSG_FPS       0
#define OSDMSG_STATUS    1
#define OSDMSG_DEBUG     2
#define OSDMSG_COUNT     3

typedef struct {
   int type;
   char * message;
   int timetolive;
   int timeleft;
   int hidden;
} OSDMessage_struct;

typedef struct {
	int id;
	const char *Name;

	int (*Init)(void);
	void (*DeInit)(void);
	void (*Reset)(void);

    void (*DisplayMessage)(OSDMessage_struct * message, pixel_t * buffer, int w, int h);
	int (*UseBuffer)(void);
	void (*AddFrameProfileData)( char * label, u32 data );
} OSD_struct;

int OSDInit(int coreid);
int OSDChangeCore(int coreid);

void OSDPushMessage(int msgtype, int ttl, const char * message, ...);
int  OSDDisplayMessages(pixel_t * buffer, int w, int h);
void OSDToggle(int what);
int  OSDIsVisible(int what);
void OSDSetVisible(int what, int visible);
void OSDAddFrameProfileData( char * label, u32 data );

extern OSD_struct OSDDummy;
#ifdef HAVE_LIBGLUT
extern OSD_struct OSDGlut;
#endif
extern OSD_struct OSDSoft;

/* defined for backward compatibility (used to be in vdp2.h) */
void ToggleFPS(void);
int  GetOSDToggle(void);
void SetOSDToggle(int toggle);
void DisplayMessage(const char* str);
#endif

#endif
