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
#include <fcntl.h>
#include <errno.h>

int  PERLinuxJoyInit(void);
void PERLinuxJoyDeInit(void);
int  PERLinuxJoyHandleEvents(void);
void PERLinuxJoyNothing(void);
u32  PERLinuxJoyScan(const char *);
void PERLinuxJoyFlush(void);
void PERLinuxKeyName(u32 key, char * name, int size);

PerInterface_struct PERLinuxJoy = {
PERCORE_LINUXJOY,
"Linux Joystick Interface",
PERLinuxJoyInit,
PERLinuxJoyDeInit,
PERLinuxJoyHandleEvents,
PERLinuxJoyNothing,
PERLinuxJoyScan,
1,
PERLinuxJoyFlush
#ifdef PERKEYNAME
,PERLinuxKeyName
#endif
};

static int hJOY = -1;

//////////////////////////////////////////////////////////////////////////////

int PERLinuxJoyInit(void)
{
   hJOY = open("/dev/js0", O_RDONLY | O_NONBLOCK);

   if (hJOY == -1) return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERLinuxJoyDeInit(void)
{
   if (hJOY != -1) close(hJOY);
}

//////////////////////////////////////////////////////////////////////////////

void PERLinuxJoyNothing(void)
{
}

//////////////////////////////////////////////////////////////////////////////

int PERLinuxJoyHandleEvents(void)
{
   struct js_event evt;

   while (read(hJOY, &evt, sizeof(struct js_event)) > 0)
   {
      printf("got event:\n");
      printf(" - value  = %d\n", evt.value);
      printf(" - type   = %d\n", evt.type);
      printf(" - number = %d\n", evt.number);
   }

   // there was an error while reading events
   if (errno != EAGAIN) {
      return -1;
   }

   // execute yabause
   if ( YabauseExec() != 0 )
   {
      return -1;
   }
   
   // return success
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

u32 PERLinuxJoyScan( UNUSED const char* n ) {
  return 0;
}

void PERLinuxJoyFlush(void) {
}

void PERLinuxKeyName(u32 key, char * name, UNUSED int size)
{
   sprintf(name, "%x", key);
}
