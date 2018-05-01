/*  Copyright 2009 Guillaume Duhamel

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
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "debug.h"
#include "perlinuxjoy.h"
#include <linux/joystick.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <glob.h>
#include <limits.h>

int  PERLinuxJoyInit(void);
void PERLinuxJoyDeInit(void);
int  PERLinuxJoyHandleEvents(void);
u32  PERLinuxJoyScan(u32 flags);
void PERLinuxJoyFlush(void);
void PERLinuxKeyName(u32 key, char * name, int size);
static void PERLinuxKeyPress(u32 key, u8 state);

int getSupportedJoy(const char *name);

#define SERVICE_BUTTON_NUMBER 3
#define SERVICE_BUTTON_EXIT (PERGUN_AXIS+1)
#define SERVICE_BUTTON_TOGGLE (PERGUN_AXIS+2)
#define SERVICE_TOGGLE_EXIT (PERGUN_AXIS+3)

typedef struct {
   const char* name;
   int code[PERGUN_AXIS+1+SERVICE_BUTTON_NUMBER];
} joymapping_struct;

#define MAPPING_NB 3

joymapping_struct joyMapping[MAPPING_NB] = {
   {"SZMy-power LTD CO.  Dual Box WII", 
      {
         JS_EVENT_BUTTON<<8 | 12, //PERPAD_UP
         JS_EVENT_BUTTON<<8 | 13, //PERPAD_RIGHT
         JS_EVENT_BUTTON<<8 | 14, //PERPAD_DOWN
         JS_EVENT_BUTTON<<8 | 15, //PERPAD_LEFT
         JS_EVENT_BUTTON<<8 | 5,  //PERPAD_RIGHT_TRIGGER
         JS_EVENT_BUTTON<<8 | 4,  //PERPAD_LEFT_TRIGGER
         JS_EVENT_BUTTON<<8 | 9,  //PERPAD_START
         JS_EVENT_BUTTON<<8 | 2,  //PERPAD_A
         JS_EVENT_BUTTON<<8 | 1,  //PERPAD_B
         JS_EVENT_BUTTON<<8 | 7,  //PERPAD_C
         JS_EVENT_BUTTON<<8 | 3,  //PERPAD_X
         JS_EVENT_BUTTON<<8 | 0,  //PERPAD_Y
         JS_EVENT_BUTTON<<8 | 6,  //PERPAD_Z
         -1,                      //PERMOUSE_LEFT
         -1,                      //PERMOUSE_MIDDLE
         -1,                      //PERMOUSE_RIGHT
         -1,                      //PERMOUSE_START
         -1,                      //PERMOUSE_AXIS
         -1,                      //PERANALOG_AXIS1
         -1,                      //PERANALOG_AXIS2
         -1,                      //PERANALOG_AXIS3
         -1,                      //PERANALOG_AXIS4
         -1,                      //PERANALOG_AXIS5
         -1,                      //PERANALOG_AXIS6
         -1,                      //PERANALOG_AXIS7
         -1,                      //PERGUN_TRIGGER
         -1,
         -1,                      //PERGUN_START
         -1,                      //PERGUN_AXIS
         JS_EVENT_BUTTON<<8 | 10, //SERVICE_BUTTON_EXIT
         -1,                      //SERVICE_BUTTON_TOGGLE
         -1,                      //SERVICE_TOGGLE_EXIT
      }
   },
   {"Sony PLAYSTATION(R)3 Controller", 
      {
         JS_EVENT_BUTTON<<8 | 4, //PERPAD_UP
         JS_EVENT_BUTTON<<8 | 5, //PERPAD_RIGHT
         JS_EVENT_BUTTON<<8 | 6, //PERPAD_DOWN
         JS_EVENT_BUTTON<<8 | 7, //PERPAD_LEFT
         JS_EVENT_BUTTON<<8 | 9,  //PERPAD_RIGHT_TRIGGER
         JS_EVENT_BUTTON<<8 | 8,  //PERPAD_LEFT_TRIGGER
         JS_EVENT_BUTTON<<8 | 3,  //PERPAD_START
         JS_EVENT_BUTTON<<8 | 14, //PERPAD_A
         JS_EVENT_BUTTON<<8 | 13, //PERPAD_B
         JS_EVENT_BUTTON<<8 | 11, //PERPAD_C
         JS_EVENT_BUTTON<<8 | 15, //PERPAD_X
         JS_EVENT_BUTTON<<8 | 12, //PERPAD_Y
         JS_EVENT_BUTTON<<8 | 10, //PERPAD_Z
         -1,                      //PERMOUSE_LEFT
         -1,                      //PERMOUSE_MIDDLE
         -1,                      //PERMOUSE_RIGHT
         -1,                      //PERMOUSE_START
         -1,                      //PERMOUSE_AXIS
         -1,                      //PERANALOG_AXIS1
         -1,                      //PERANALOG_AXIS2
         -1,                      //PERANALOG_AXIS3
         -1,                      //PERANALOG_AXIS4
         -1,                      //PERANALOG_AXIS5
         -1,                      //PERANALOG_AXIS6
         -1,                      //PERANALOG_AXIS7
         -1,                      //PERGUN_TRIGGER
         -1,
         -1,                      //PERGUN_START
         -1,                      //PERGUN_AXIS
         JS_EVENT_BUTTON<<8 | 16, //SERVICE_BUTTON_EXIT
         -1,                      //SERVICE_BUTTON_TOGGLE
         -1,                      //SERVICE_TOGGLE_EXIT
      }
   },
   {"HuiJia  USB GamePad", 
      {
         JS_EVENT_BUTTON<<8 | 12, //PERPAD_UP
         JS_EVENT_BUTTON<<8 | 13, //PERPAD_RIGHT
         JS_EVENT_BUTTON<<8 | 14, //PERPAD_DOWN
         JS_EVENT_BUTTON<<8 | 15, //PERPAD_LEFT
         JS_EVENT_BUTTON<<8 | 7,  //PERPAD_RIGHT_TRIGGER
         JS_EVENT_BUTTON<<8 | 5,  //PERPAD_LEFT_TRIGGER
         JS_EVENT_BUTTON<<8 | 9,  //PERPAD_START
         JS_EVENT_BUTTON<<8 | 0,  //PERPAD_A
         JS_EVENT_BUTTON<<8 | 1,  //PERPAD_B
         JS_EVENT_BUTTON<<8 | 2,  //PERPAD_C
         JS_EVENT_BUTTON<<8 | 3,  //PERPAD_X
         JS_EVENT_BUTTON<<8 | 4,  //PERPAD_Y
         JS_EVENT_BUTTON<<8 | 6,  //PERPAD_Z
         -1,                      //PERMOUSE_LEFT
         -1,                      //PERMOUSE_MIDDLE
         -1,                      //PERMOUSE_RIGHT
         -1,                      //PERMOUSE_START
         -1,                      //PERMOUSE_AXIS
         -1,                      //PERANALOG_AXIS1
         -1,                      //PERANALOG_AXIS2
         -1,                      //PERANALOG_AXIS3
         -1,                      //PERANALOG_AXIS4
         -1,                      //PERANALOG_AXIS5
         -1,                      //PERANALOG_AXIS6
         -1,                      //PERANALOG_AXIS7
         -1,                      //PERGUN_TRIGGER
         -1,
         -1,                      //PERGUN_START
         -1,                      //PERGUN_AXIS
         -1,                      //SERVICE_BUTTON_EXIT
         JS_EVENT_BUTTON<<8 | 9,  //SERVICE_BUTTON_TOGGLE
         JS_EVENT_BUTTON<<8 | 0,  //SERVICE_TOGGLE_EXIT
      }
   },
};

#define KEYPAD(key, player) ((player << 17)|key)

static void KeyInit() {
  void * padbits;

  PerPortReset();
  padbits = PerPadAdd(&PORTDATA1);

  PerSetKey(KEYPAD(PERPAD_UP, 0), PERPAD_UP, padbits);
  PerSetKey(KEYPAD(PERPAD_RIGHT, 0), PERPAD_RIGHT, padbits);
  PerSetKey(KEYPAD(PERPAD_DOWN, 0), PERPAD_DOWN, padbits);
  PerSetKey(KEYPAD(PERPAD_LEFT, 0), PERPAD_LEFT, padbits);
  PerSetKey(KEYPAD(PERPAD_RIGHT_TRIGGER, 0), PERPAD_RIGHT_TRIGGER, padbits);
  PerSetKey(KEYPAD(PERPAD_LEFT_TRIGGER, 0), PERPAD_LEFT_TRIGGER, padbits);
  PerSetKey(KEYPAD(PERPAD_START, 0), PERPAD_START, padbits);
  PerSetKey(KEYPAD(PERPAD_A, 0), PERPAD_A, padbits);
  PerSetKey(KEYPAD(PERPAD_B, 0), PERPAD_B, padbits);
  PerSetKey(KEYPAD(PERPAD_C, 0), PERPAD_C, padbits);
  PerSetKey(KEYPAD(PERPAD_X, 0), PERPAD_X, padbits);
  PerSetKey(KEYPAD(PERPAD_Y, 0), PERPAD_Y, padbits);
  PerSetKey(KEYPAD(PERPAD_Z, 0), PERPAD_Z, padbits);

  padbits = PerPadAdd(&PORTDATA2);

  PerSetKey(KEYPAD(PERPAD_UP, 1), PERPAD_UP, padbits);
  PerSetKey(KEYPAD(PERPAD_RIGHT, 1), PERPAD_RIGHT, padbits);
  PerSetKey(KEYPAD(PERPAD_DOWN, 1), PERPAD_DOWN, padbits);
  PerSetKey(KEYPAD(PERPAD_LEFT, 1), PERPAD_LEFT, padbits);
  PerSetKey(KEYPAD(PERPAD_RIGHT_TRIGGER, 1), PERPAD_RIGHT_TRIGGER, padbits);
  PerSetKey(KEYPAD(PERPAD_LEFT_TRIGGER, 1), PERPAD_LEFT_TRIGGER, padbits);
  PerSetKey(KEYPAD(PERPAD_START, 1), PERPAD_START, padbits);
  PerSetKey(KEYPAD(PERPAD_A, 1), PERPAD_A, padbits);
  PerSetKey(KEYPAD(PERPAD_B, 1), PERPAD_B, padbits);
  PerSetKey(KEYPAD(PERPAD_C, 1), PERPAD_C, padbits);
  PerSetKey(KEYPAD(PERPAD_X, 1), PERPAD_X, padbits);
  PerSetKey(KEYPAD(PERPAD_Y, 1), PERPAD_Y, padbits);
  PerSetKey(KEYPAD(PERPAD_Z, 1), PERPAD_Z, padbits);

  padbits = PerCabAdd(NULL);
  PerSetKey(PERPAD_UP, PERPAD_UP, padbits);
  PerSetKey(PERPAD_RIGHT, PERPAD_RIGHT, padbits);
  PerSetKey(PERPAD_DOWN, PERPAD_DOWN, padbits);
  PerSetKey(PERPAD_LEFT, PERPAD_LEFT, padbits);
  PerSetKey(PERPAD_A, PERPAD_A, padbits);
  PerSetKey(PERPAD_B, PERPAD_B, padbits);
  PerSetKey(PERPAD_C, PERPAD_C, padbits);
  PerSetKey(PERPAD_X, PERPAD_X, padbits);

  PerSetKey(PERJAMMA_COIN1, PERJAMMA_COIN1, padbits );
  PerSetKey(PERJAMMA_COIN2, PERJAMMA_COIN2, padbits );
  PerSetKey(PERJAMMA_TEST, PERJAMMA_TEST, padbits);
  PerSetKey(PERJAMMA_SERVICE, PERJAMMA_SERVICE, padbits);
  PerSetKey(PERJAMMA_START1, PERJAMMA_START1, padbits);
  PerSetKey(PERJAMMA_START2, PERJAMMA_START2, padbits);
  PerSetKey(PERJAMMA_MULTICART, PERJAMMA_MULTICART, padbits);
  PerSetKey(PERJAMMA_PAUSE, PERJAMMA_PAUSE, padbits);

}

int requestExit = 0;
int toggle = 0;
int service = 0;

#define TOGGLE_EXIT 0

PerInterface_struct PERLinuxJoy = {
PERCORE_LINUXJOY,
"Linux Joystick Interface",
PERLinuxJoyInit,
PERLinuxJoyDeInit,
PERLinuxJoyHandleEvents,
PERLinuxJoyScan,
1,
PERLinuxJoyFlush,
PERLinuxKeyName,
PERLinuxKeyPress
};

typedef struct
{
   int fd;
   int * axis;
   int axiscount;
   int id;
   int mapping;
} perlinuxjoy_struct;

static perlinuxjoy_struct * joysticks = NULL;
static int joycount = 0;

#define PACKEVENT(evt,joy) ((joy->id << 17) | getPerPadKey(evt.value, (evt.value < 0 ? 0x10000 : 0) | (evt.type << 8) | (evt.number), joy))
#define THRESHOLD 1000
#define MAXAXIS 256

//////////////////////////////////////////////////////////////////////////////

static int handleToggle(int state, int val, perlinuxjoy_struct * joystick) {
   int i;
   int ret = 0;
   for (i=PERGUN_AXIS+2; i< PERGUN_AXIS+1+SERVICE_BUTTON_NUMBER; i++) {
      if (joyMapping[joystick->mapping].code[i] == val) {
         switch (i) {
            case SERVICE_BUTTON_TOGGLE:
               if (state != 0)
                  toggle = 1;
               else
                  toggle = 0;
            break;
            case SERVICE_TOGGLE_EXIT:
               if (state != 0)
                  service  |= (1 << TOGGLE_EXIT);
               else
                  service &= ~(1 << TOGGLE_EXIT);
            break;
            default:
            break;
         }
      }
   }
   if (toggle) {
      requestExit = service & (1<< TOGGLE_EXIT);
      if (requestExit) ret=1;
   }
   return ret;
}

static int getPerPadKey(int state, int val, perlinuxjoy_struct * joystick) {
   int i;
   int ret = -1;
   if (handleToggle(state, val, joystick)) return ret;
   for (i=0; i< PERGUN_AXIS+2; i++) {
      if (joyMapping[joystick->mapping].code[i] == val) {
         switch (i) {
            case SERVICE_BUTTON_EXIT:
               requestExit = 1;
               break;
            default:
               ret=i;
         }
      }
   }
   return ret;
}

static void PERLinuxKeyPress(u32 key, u8 state)
{
  switch(state) {
  case 0:
    PerKeyUp(key);
  break;
  case 1:
    PerKeyDown(key);
  break;
  default:
  break;
  }
}

static int LinuxJoyInit(perlinuxjoy_struct * joystick, const char * path, int id)
{
   int i;
   int fd;
   int axisinit[MAXAXIS];
   char name[128];
   struct js_event evt;
   ssize_t num_read;

   joystick->fd = open(path, O_RDONLY | O_NONBLOCK);
   if (joystick->fd == -1) return -1;
   if (ioctl(joystick->fd, JSIOCGNAME(sizeof(name)), name) < 0)
      strncpy(name, "Unknown", sizeof(name));
   printf("Joyinit open %s ", name);
   if ((joystick->mapping = getSupportedJoy(name)) == -1) {
      printf("is not supported\n");
      return -1;
   } else {
      printf("is supported => Player %d\n", id+1);
   }
   joystick->axiscount = 0;
   joystick->id = id;
   while ((num_read = read(joystick->fd, &evt, sizeof(struct js_event))) > 0)
   {      if (evt.type == (JS_EVENT_AXIS | JS_EVENT_INIT))
      {
         axisinit[evt.number] = evt.value;
         if (evt.number + 1 > joystick->axiscount)
         {
            joystick->axiscount = evt.number + 1;
         }
      }
   }

   if (joystick->axiscount > MAXAXIS) joystick->axiscount = MAXAXIS;

   joystick->axis = malloc(sizeof(int) * joystick->axiscount);
   for(i = 0;i < joystick->axiscount;i++)
   {
      joystick->axis[i] = axisinit[i];
   }

   KeyInit();
   return 0;
}

static void LinuxJoyDeInit(perlinuxjoy_struct * joystick)
{
   if (joystick->fd == -1) return;

   close(joystick->fd);
   free(joystick->axis);
}

static void LinuxJoyHandleEvents(perlinuxjoy_struct * joystick)
{
   struct js_event evt;
   ssize_t num_read;
   int key;

   if (joystick->fd == -1) return;

   while ((num_read = read(joystick->fd, &evt, sizeof(struct js_event))) > 0)
   {
      if (evt.type == JS_EVENT_AXIS)
      {
         int initvalue;
         int disp;
         u8 axis = evt.number;

         if (axis >= joystick->axiscount) return;

         initvalue = joystick->axis[axis];
         disp = abs(initvalue - evt.value);
         if (disp < THRESHOLD) evt.value = 0;
         else if (evt.value < initvalue) evt.value = -1;
         else evt.value = 1;
      }
      key = PACKEVENT(evt, joystick);
      if (evt.value != 0)
      {
         if ((key & 0x1FFFF) != 0x1FFFF) PerKeyDown(key);
      }
      else
      {
         if ((key & 0x1FFFF) != 0x1FFFF) PerKeyUp(key);
         if ((key & 0x1FFFF) != 0x1FFFF) PerKeyUp(0x10000 | key);
      }
   }
}

static int LinuxJoyScan(perlinuxjoy_struct * joystick)
{
   struct js_event evt;
   ssize_t num_read;
   int key;

   if (joystick->fd == -1) return 0;

   if ((num_read = read(joystick->fd, &evt, sizeof(struct js_event))) <= 0) return 0;

   if (evt.type == JS_EVENT_AXIS)
   {
      int initvalue;
      int disp;
      u8 axis = evt.number;

      if (axis >= joystick->axiscount) return 0;

      initvalue = joystick->axis[axis];
      disp = abs(initvalue - evt.value);
      if (disp < THRESHOLD) return 0;
      else if (evt.value < initvalue) evt.value = -1;
      else evt.value = 1;
   }
   key = PACKEVENT(evt, joystick);
   if ((key & 0x1FFFF) != 0x1FFFF)
      return key;
   else return 0;
}

static void LinuxJoyFlush(perlinuxjoy_struct * joystick)
{
   struct js_event evt;
   ssize_t num_read;

   if (joystick->fd == -1) return;

   while ((num_read = read(joystick->fd, &evt, sizeof(struct js_event))) > 0);
}

int getSupportedJoy(const char *name) {
   int i;
   for (i=0; i<MAPPING_NB; i++) {
      if (strncmp(name, joyMapping[i].name, 128) == 0) return i;
   }
   return -1;
}
//////////////////////////////////////////////////////////////////////////////

int PERLinuxJoyInit(void)
{
   int i;
   int fd;
   int j=0;
   glob_t globbuf;

   glob("/dev/input/js*", 0, NULL, &globbuf);

   joycount = globbuf.gl_pathc;
   joysticks = calloc(sizeof(perlinuxjoy_struct), joycount);
   for(i = 0;i < globbuf.gl_pathc;i++) {
      perlinuxjoy_struct *joy = joysticks + j;
      j = (LinuxJoyInit(joy, globbuf.gl_pathv[i], j)<0)?j:j+1;
   }
   joycount = j;
   globfree(&globbuf);

   if (globbuf.gl_pathc <= 0) 
     KeyInit();

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERLinuxJoyDeInit(void)
{
   int i;

   for(i = 0;i < joycount;i++)
      LinuxJoyDeInit(joysticks + i);

   free(joysticks);
}

//////////////////////////////////////////////////////////////////////////////

int PERLinuxJoyHandleEvents(void)
{
   int i;

   for(i = 0;i < joycount;i++)
      LinuxJoyHandleEvents(joysticks + i);

   if (requestExit) return -1;
   // execute yabause
   if ( YabauseExec() != 0 )
   {
      return -1;
   }
   
   // return success
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 PERLinuxJoyScan(u32 flags) {
   int i;

   for(i = 0;i < joycount;i++)
   {
      int ret = LinuxJoyScan(joysticks + i);
      if (ret != 0) return ret;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERLinuxJoyFlush(void) {
   int i;

   for (i = 0;i < joycount;i++)
      LinuxJoyFlush(joysticks + i);
}

//////////////////////////////////////////////////////////////////////////////

void PERLinuxKeyName(u32 key, char * name, UNUSED int size)
{
   sprintf(name, "%x", (int)key);
}
