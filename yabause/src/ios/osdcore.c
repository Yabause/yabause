/*  Copyright 2012 Guillaume Duhamel

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

#include "osdcore.h"
#include "yabause.h"
#include "scsp.h"
#include "vidsoft.h"
#include "vidogl.h"
#include "peripheral.h"
#include "m68kcore.h"
#include "sh2core.h"
#include "sh2int.h"
#include "cdbase.h"
#include "cs2.h"
#include "debug.h"

#include <stdlib.h>
#include <stdarg.h>

#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>

#include "nanovg.h"
#define NANOVG_GLES3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "Roboto-Bold.h"
#include "Roboto-Regular.h"

static int OSDNanovgInit(void);
static void OSDNanovgDeInit(void);
static void OSDNanovgReset(void);
static void OSDNanovgAddFrameProfileData( char * label, u32 data );
static void OSDNanovgDisplayMessage(OSDMessage_struct * message,pixel_t * buffer, int w, int h);
static int OSDNanovgUseBuffer(void);

OSD_struct OSDNnovg = {
    OSDCORE_NANOVG,
    "nanovg ios OSD Interface",
    OSDNanovgInit,
    OSDNanovgDeInit,
    OSDNanovgReset,
    OSDNanovgDisplayMessage,
    OSDNanovgUseBuffer,
    OSDNanovgAddFrameProfileData
};

static NVGcontext* vg = NULL;
static int fontNormal, fontBold;

#define MAX_HISTORY 4
#define MAX_PROFILE_CNT 32

typedef struct FrameProfileInfo{
	char label[MAX_PROFILE_CNT][64];
	u32 time[MAX_PROFILE_CNT];
	int cnt;
} FrameProfileInfo;

static FrameProfileInfo frameinfo_histroy[MAX_HISTORY];
static int current_history_index = 0;
static int profile_index = 0;

int OSDNanovgInit(void)
{
    printf("OSDNanovgInit\n");
    vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    if (vg == NULL) {
        printf("Could not init nanovg.\n");
        return -1;
    }

    fontNormal = nvgCreateFontMem(vg, "sans", Roboto_Regular_ttf, Roboto_Regular_ttf_len,0);
    if (fontNormal == -1) {
        printf("Could not add font italic.\n");
        return -1;
    }
    fontBold = nvgCreateFontMem(vg, "sans", Roboto_Bold_ttf, Roboto_Bold_ttf_len,0);
    if (fontBold == -1) {
        printf("Could not add font bold.\n");
        return -1;
    }
}

void OSDNanovgDeInit(void)
{
}

void OSDNanovgReset(void)
{
}

int OSDNanovgUseBuffer(){
	return 0;
}

void OSDNanovgAddFrameProfileData( char * label, u32 data ){
	int index;
	if( MAX_PROFILE_CNT <= profile_index) return;
	index = current_history_index % MAX_HISTORY;
	strncpy( frameinfo_histroy[index].label[profile_index],label,64);
	frameinfo_histroy[index].time[profile_index] = data;
	frameinfo_histroy[index].cnt = profile_index;
	profile_index++;
}

#define TOF(a) (((float)a)/255.0f)
#define BASE_ALPHA (0.8f)

NVGcolor graph_colors[MAX_PROFILE_CNT]={
	{ TOF(255),TOF(0),TOF(0),BASE_ALPHA },
	{ TOF(255),TOF(255),TOF(0),BASE_ALPHA },
	{ TOF(128),TOF(255),TOF(0),BASE_ALPHA },
	{ TOF(255),TOF(64),TOF(0),BASE_ALPHA },
	{ TOF(0),TOF(255),TOF(255),BASE_ALPHA },
	{ TOF(0),TOF(128),TOF(192),BASE_ALPHA },
	{ TOF(128),TOF(128),TOF(192),BASE_ALPHA },
	{ TOF(255),TOF(0),TOF(255),BASE_ALPHA },
	{ TOF(128),TOF(64),TOF(64),BASE_ALPHA },
	{ TOF(255),TOF(128),TOF(64),BASE_ALPHA },
	{ TOF(0),TOF(255),TOF(0),BASE_ALPHA },
	{ TOF(0),TOF(128),TOF(128),BASE_ALPHA },
	{ TOF(0),TOF(64),TOF(128),BASE_ALPHA },
	{ TOF(128),TOF(128),TOF(255),BASE_ALPHA },
	{ TOF(128),TOF(0),TOF(64),BASE_ALPHA },
	{ TOF(255),TOF(0),TOF(128),BASE_ALPHA },
	{ TOF(128),TOF(0),TOF(0),BASE_ALPHA },
	{ TOF(255),TOF(128),TOF(0),BASE_ALPHA },
	{ TOF(0),TOF(128),TOF(0),BASE_ALPHA },
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5},
	{ 0.5, 0.5, 0.5, 0.5}
};


void ProfileDrawGraph(){
	int i,j;
	int historyindx;
	float targetY = 500.0f;
	
	if( current_history_index<MAX_HISTORY ){
		current_history_index++;
		return;
	} 
	
	historyindx = current_history_index % MAX_HISTORY;
	
	if( frameinfo_histroy[historyindx].cnt == 0 ){
		return;
	}

	float startX = 0.0f;
	float width = 32.0f;
	
	for( i=0; i< MAX_HISTORY; i++ ){
		
		float startY = 32;
		for( j=0; j<frameinfo_histroy[historyindx].cnt; j++ ){
			
			float height = frameinfo_histroy[historyindx].time[j] * targetY/16666.0f;
			nvgBeginPath(vg);
			nvgRect(vg, startX,startY, width,height);
			nvgFillColor(vg, graph_colors[j]);
			nvgFill(vg);
			startY += height;
		}
		
		if( i ==(MAX_HISTORY-1) ) {
			float textY = 32;
			for( j=0; j<frameinfo_histroy[historyindx].cnt; j++ ){
				char buf[128];
				nvgFillColor(vg, nvgRGBA(255,255,255,255));
				sprintf(buf,"%s:%d",frameinfo_histroy[historyindx].label[j],(int)(frameinfo_histroy[historyindx].time[j]));
				nvgText(vg, startX + width, textY,buf, NULL);
				textY += 24;
			}
		}
		
		startX += width;
		historyindx = (historyindx+1) % MAX_HISTORY;
	}
	

	// Draw target line
	nvgBeginPath(vg);
	nvgMoveTo(vg, 0, targetY+32);
	nvgLineTo(vg, 32*32, targetY+32);
	nvgStrokeColor(vg, nvgRGBA(255,0,0,255));
	nvgStroke(vg);


	// Reset indexs;
	current_history_index++;
	profile_index = 0;

}

void OSDNanovgDisplayMessage(OSDMessage_struct * message,pixel_t * buffer, int w, int h)
{
   int LeftX=9;
   int Width=500;
   int TxtY=11;
   int Height=13;
   int i, msglength;
   int vidwidth, vidheight;

   if( message->type != OSDMSG_FPS ) return;

   VIDCore->GetGlSize(&vidwidth, &vidheight);
   Width = vidwidth - 2 * LeftX;

   switch(message->type) {
      case OSDMSG_STATUS:
         TxtY = vidheight - (Height + TxtY);
         break;
   }

   msglength = strlen(message->message);



   nvgBeginFrame(vg, vidwidth, vidheight, 1.0f);

   nvgFontSize(vg, 32.0f);
   nvgFontFace(vg, "sans");
   nvgFillColor(vg, nvgRGBA(255,255,255,255));

   nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
   nvgText(vg, 10,TxtY+11,message->message, NULL);

    ProfileDrawGraph();
   nvgEndFrame(vg);

/*

   glColor3f(1.0f, 1.0f, 1.0f);
   glRasterPos2i(10, TxtY + 11);
   for (i = 0; i < msglength; i++) {
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, message->message[i]);
   }
   glColor3f(1, 1, 1);
*/
}



OSD_struct *OSDCoreList[] = {
&OSDNnovg,
&OSDSoft,
&OSDDummy,
NULL
};

