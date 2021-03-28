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
#include "nanovg_osdcore.h"
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

#include "vulkan/vulkan.h"

#define NANOVG_VULKAN_IMPLEMENTATION
#include "nanovg.h"
#include "nanovg_vk.h"

//#include "Roboto-Bold.h"
//#include "Roboto-Regular.h"

extern "C" {

  extern unsigned char Roboto_Bold_ttf[];
  extern unsigned char Roboto_Regular_ttf[];
  extern unsigned int Roboto_Bold_ttf_len;
  extern unsigned int Roboto_Regular_ttf_len;

  static int OSDNanovgVInit(void);
  static void OSDNanovgVDeInit(void);
  static void OSDNanovgVReset(void);
  static void OSDNanovgVDisplayMessage(OSDMessage_struct * message, pixel_t * buffer, int w, int h);
  static void OSDNanovgVAddFrameProfileData(char * label, u32 data);
  static int OSDNanovgVUseBuffer(void);
  static void OSDNanovgVAddLogString(char * log);

  OSD_struct OSDNnovgVulkan = {
    OSDCORE_NANOVG_VULKAN,
    "nanovg Vunlan OSD Interface",
    OSDNanovgVInit,
    OSDNanovgVDeInit,
    OSDNanovgVReset,
    OSDNanovgVDisplayMessage,
    OSDNanovgVUseBuffer,
    OSDNanovgVAddFrameProfileData,
    OSDNanovgVAddLogString
  };
}



static NVGcontext* vg = NULL;
static int fontNormal, fontBold;

#define MAX_HISTORY 4
#define MAX_PROFILE_CNT 32

typedef struct FrameProfileInfo {
  char label[MAX_PROFILE_CNT][64];
  u32 time[MAX_PROFILE_CNT];
  int cnt;
} FrameProfileInfo;

static FrameProfileInfo frameinfo_histroy[MAX_HISTORY];
static int current_history_index = 0;
static int profile_index = 0;

int NanovgVulkanSetDevices(VkDevice device, VkPhysicalDevice gpu, VkRenderPass renderPass, VkCommandBuffer cmdBuffer) {
  
  VKNVGCreateInfo createInfo={0};
  createInfo.device = device;
  createInfo.gpu = gpu;
  createInfo.renderpass = renderPass;
  createInfo.cmdBuffer = cmdBuffer;

  if (vg != NULL) {
    nvgUpdateVk(vg,createInfo);
    return 0;
  }

  vg = nvgCreateVk(createInfo, NVG_ANTIALIAS);
  if (vg == NULL) {
    printf("Could not init nanovg.\n");
    return -1;
  }
  OSDNanovgVInit();
  return 0;

}

extern "C" {

static int OSDNanovgVInit(void)
{
  VKNVGCreateInfo create_info = { 0 };
  //create_info.device = device->device;
  //create_info.gpu = device->gpu;
  //create_info.renderpass = fb.render_pass;
  //create_info.cmdBuffer = cmd_buffer;

  if (vg == NULL) {
    printf("Could not init nanovg.\n");
    return 0;
  }

  fontNormal = nvgCreateFontMem(vg, "sans", Roboto_Regular_ttf, Roboto_Regular_ttf_len, 0);
  if (fontNormal == -1) {
    printf("Could not add font italic.\n");
    return -1;
  }
  fontBold = nvgCreateFontMem(vg, "sans", Roboto_Bold_ttf, Roboto_Bold_ttf_len, 0);
  if (fontBold == -1) {
    printf("Could not add font bold.\n");
    return -1;
  }

  memset(frameinfo_histroy, 0, sizeof(FrameProfileInfo)*MAX_HISTORY);
  current_history_index = 0;
  profile_index = 0;

  return 0;
}

static void OSDNanovgVDeInit(void)
{
  nvgDeleteVk(vg);
  vg = NULL;

}

static void OSDNanovgVReset(void)
{
}

static int OSDNanovgVUseBuffer() {
  return 0;
}

}

#if 0
#define MAX_COL 80
#define MAX_LOW 256
char g_message[MAX_LOW][MAX_COL];
int msgcnt = 0;

void OSDPushMessageDirect(char * msg) {

  if (msgcnt >= MAX_LOW) return;
  strncpy(g_message[msgcnt], msg, MAX_COL);
  msgcnt++;

}
#endif

#define MAX_LOG_HISTORY (256)
static char log_histroy[MAX_LOG_HISTORY][128];
static int current_log_history_index = 0;

extern "C" {
static void OSDNanovgVAddLogString(char * log) {
  int index;
  index = current_log_history_index;
  strncpy(log_histroy[index], log, 128);
  current_log_history_index++;
  if (current_log_history_index >= MAX_LOG_HISTORY) {
    current_log_history_index = 0;
  }
}


static void OSDNanovgVAddFrameProfileData(char * label, u32 data) {
  int index;
  if (MAX_PROFILE_CNT <= profile_index) return;
  index = current_history_index % MAX_HISTORY;
  strncpy(frameinfo_histroy[index].label[profile_index], label, 64);
  frameinfo_histroy[index].time[profile_index] = data;
  frameinfo_histroy[index].cnt = profile_index;
  profile_index++;
}
}

#define TOF(a) (((float)a)/255.0f)
#define BASE_ALPHA (0.8f)

NVGcolor graph_colorsvg[MAX_PROFILE_CNT] = {
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
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 },
  { 0.5, 0.5, 0.5, 0.5 }
};


void ProfileDrawGraph() {
  int i, j;
  int historyindx;
  float targetY = 500.0f;
  
  if (vg == NULL) {
    return;
  }
  
  if (current_history_index < MAX_HISTORY) {
    current_history_index++;
    profile_index = 0;
    return;
  }

  historyindx = current_history_index % MAX_HISTORY;

  if (frameinfo_histroy[historyindx].cnt == 0) {
    current_history_index++;
    profile_index = 0;
    return;
  }

  float startX = 0.0f;
  float width = 32.0f;

  for (i = 0; i < MAX_HISTORY; i++) {

    float startY = 32;
    for (j = 0; j < frameinfo_histroy[historyindx].cnt; j++) {

      float height = frameinfo_histroy[historyindx].time[j] * targetY / 16666.0f;
      nvgBeginPath(vg);
      nvgRect(vg, startX, startY, width, height);
      nvgFillColor(vg, graph_colorsvg[j]);
      nvgFill(vg);
      startY += height;
    }

    if (i == (MAX_HISTORY - 1)) {
      float textY = 32;
      for (j = 0; j < frameinfo_histroy[historyindx].cnt; j++) {
        char buf[128];
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
        sprintf(buf, "%s:%d", frameinfo_histroy[historyindx].label[j], (int)(frameinfo_histroy[historyindx].time[j]));
        nvgText(vg, startX + width, textY, buf, NULL);
        textY += 24;
      }
    }

    startX += width;
    historyindx = (historyindx + 1) % MAX_HISTORY;
  }


  // Draw target line
  nvgBeginPath(vg);
  nvgMoveTo(vg, 0, targetY + 32);
  nvgLineTo(vg, 32 * 32, targetY + 32);
  nvgStrokeColor(vg, nvgRGBA(255, 0, 0, 255));
  nvgStroke(vg);


  // Reset indexs;
  current_history_index++;
  profile_index = 0;

}


extern "C" {

static void OSDNanovgVDisplayMessage(OSDMessage_struct * message, pixel_t * buffer, int w, int h)
{
  int LeftX = 8;
  int Width = 500;
  int TxtY = 12;
  int Height = 13;
  int msglength;
  int vidwidth, vidheight;
  float fontsize = 32.0f;
  int maxlen = 0;
  int i = 0;

  if (vg == NULL) {
    return;
  }

  VIDCore->GetGlSize(&vidwidth, &vidheight);
  

  if (message->type == OSDMSG_FPS) {
    LeftX = 8;
  }
  else if (message->type == OSDMSG_RECORD) {
    LeftX = vidwidth - 8;
  }
  else {
    return;
  }

  Width = vidwidth - 2 * LeftX;


  switch (message->type) {
  case OSDMSG_STATUS:
    TxtY = vidheight - (Height + TxtY);
    break;
  }

  msglength = strlen(message->message);



  nvgBeginFrame(vg, vidwidth, vidheight, 1.0f);


  nvgFontSize(vg, fontsize);
  nvgFontFace(vg, "sans");
  
  if (message->type == OSDMSG_FPS) {

    nvgBeginPath(vg);
    nvgRect(vg, 0, 0, 320 + 8, 32);
    nvgFillColor(vg, nvgRGBA(8, 8, 8, 128));
    nvgFill(vg);
    
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgText(vg, LeftX, TxtY, message->message, NULL);
    TxtY += fontsize;
    fontsize = 16;
    nvgFontSize(vg, fontsize);
    int linecnt = (vidheight - TxtY) / fontsize;
    int start_point = current_log_history_index - linecnt;
    if (start_point < 0) {
      start_point = MAX_LOG_HISTORY + start_point;
    }
    for (i = 0; i < linecnt; i++) {
      nvgText(vg, LeftX, TxtY, log_histroy[start_point], NULL);
      start_point++;
      start_point %= MAX_LOG_HISTORY;
      TxtY += fontsize;
    }
    ProfileDrawGraph();
  }

  if (message->type == OSDMSG_RECORD) {

    nvgBeginPath(vg);
    nvgRect(vg, LeftX - (strlen(message->message)*(fontsize/2)), 0, 320 + 8, 32);
    nvgFillColor(vg, nvgRGBA(8, 8, 8, 128));
    nvgFill(vg);

    nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
    nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    nvgText(vg, LeftX, TxtY, message->message, NULL);
  }

  nvgEndFrame(vg);

}

}


#if 0
OSD_struct *OSDCoreList[] = {
  &OSDNnovg,
  &OSDSoft,
  &OSDDummy,
  NULL
};
#endif

