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

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "nanovg.h"
#define NANOVG_GLES3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "Roboto-Bold.h"
#include "Roboto-Regular.h"

static int OSDNanovgInit(void);
static void OSDNanovgDeInit(void);
static void OSDNanovgReset(void);
static void OSDNanovgDisplayMessage(OSDMessage_struct * message);

OSD_struct OSDNnovg = {
    OSDCORE_NANOVG,
    "nanovg OSD Interface",
    OSDNanovgInit,
    OSDNanovgDeInit,
    OSDNanovgReset,
    OSDNanovgDisplayMessage
};

static NVGcontext* vg = NULL;
static int fontNormal, fontBold;

int OSDNanovgInit(void)
{
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

void OSDNanovgDisplayMessage(OSDMessage_struct * message)
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

   nvgFontSize(vg, 64.0f);
   nvgFontFace(vg, "sans");
   nvgFillColor(vg, nvgRGBA(255,255,255,255));

   nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
   nvgText(vg, 10,TxtY+11,message->message, NULL);

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

