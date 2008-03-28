/*  Copyright 2008 Theo Berkau

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

#include <stdio.h>
#include <ogcsys.h>
#include "../peripheral.h"
#include "keys.h"

#define MSG_CONNECT             0
#define MSG_DISCONNECT          1
#define MSG_EVENT               2

s32 kbdfd = -1;
volatile BOOL kbdconnected = FALSE;
extern u16 buttonbits;

struct
{
   u32 msg;
   u32 unknown;
   u8 modifier;
   u8 unknown2;
   u8 keydata[6];
} kbdevent ATTRIBUTE_ALIGN(32);

void (*keypush[256])(void);
s32 KeyboardConnectCallback(s32 result,void *usrdata);

void SetupKeyPush(u8 key, void (*pushfunc)())
{
   keypush[key] = pushfunc;
}

void KeyStub(void)
{
}

s32 KeyboardPoll(s32 result, void *usrdata)
{
   int i;

   if (kbdconnected)
   {
      switch(kbdevent.msg)
      {
         case MSG_DISCONNECT:
            kbdconnected = FALSE;
            IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, KeyboardConnectCallback, NULL);
            break;
         case MSG_EVENT:
            buttonbits = 0xFFFF;

            for (i = 0; i < 6; i++)
            {
               if (kbdevent.keydata[0] == 0)
                  break;
               keypush[kbdevent.keydata[i]]();
            }
            IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, KeyboardPoll, NULL);
            break;
         default: break;
      }
   }

   return 0;
}

s32 KeyboardConnectCallback(s32 result,void *usrdata)
{
   // Should be the connect msg
   if (kbdevent.msg == MSG_CONNECT)
   {
      IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, KeyboardPoll, NULL);
      kbdconnected = TRUE;
   }
   return 0;
}

int KeyboardInit()
{
   int i;
   static char kbdstr[] ATTRIBUTE_ALIGN(32) = "/dev/usb/kbd";

   if ((kbdfd = IOS_Open(kbdstr, IPC_OPEN_NONE)) < 0)
      return -1;

   for(i = 0; i < 256; i++)
       SetupKeyPush(i, KeyStub);

   SetupKeyPush(KEY_UP, PerUpPressed);
   SetupKeyPush(KEY_DOWN, PerDownPressed);
   SetupKeyPush(KEY_LEFT, PerLeftPressed);
   SetupKeyPush(KEY_RIGHT, PerRightPressed);
   SetupKeyPush(KEY_K, PerAPressed);
   SetupKeyPush(KEY_L, PerBPressed);
   SetupKeyPush(KEY_M, PerCPressed);
   SetupKeyPush(KEY_U, PerXPressed);
   SetupKeyPush(KEY_I, PerYPressed);
   SetupKeyPush(KEY_O, PerZPressed);
   SetupKeyPush(KEY_X, PerLTriggerPressed);
   SetupKeyPush(KEY_Z, PerRTriggerPressed);
   SetupKeyPush(KEY_J, PerStartPressed);

   IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, KeyboardConnectCallback, NULL);
   return 0;
}

void KeyboardDeInit()
{
   if (kbdfd > -1)
   {
      IOS_Close(kbdfd);
      kbdfd = -1;
   }
}
