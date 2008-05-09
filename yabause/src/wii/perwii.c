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
#include "../vdp1.h"
#include "../vdp2.h"
#include "perwii.h"
#include "keys.h"

#define MSG_CONNECT             0
#define MSG_DISCONNECT          1
#define MSG_EVENT               2

s32 kbdfd = -1;
volatile BOOL kbdconnected = FALSE;
extern u16 buttonbits;
PerPad_struct *pad[12];

struct
{
   u32 msg;
   u32 unknown;
   u8 modifier;
   u8 unknown2;
   u8 keydata[6];
} kbdevent ATTRIBUTE_ALIGN(32);

s32 KeyboardConnectCallback(s32 result,void *usrdata);

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
            // Hackish, horray!
            pad[0]->padbits[0] = 0xFF;
            pad[0]->padbits[1] = 0xFF;

            for (i = 0; i < 6; i++)
            {
               if (kbdevent.keydata[0] == 0)
                  break;
               PerKeyDown(kbdevent.keydata[i]);
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

int PERKeyboardInit()
{
   static char kbdstr[] ATTRIBUTE_ALIGN(32) = "/dev/usb/kbd";

   if ((kbdfd = IOS_Open(kbdstr, IPC_OPEN_NONE)) < 0)
      return -1;

   PerPortReset();
   pad[0] = PerPadAdd(&PORTDATA1);

//   SetupKeyPush(keypush, KEY_F1, ToggleFPS);
//   SetupKeyPush(keypush, KEY_1, ToggleNBG0);
//   SetupKeyPush(keypush, KEY_2, ToggleNBG1);
//   SetupKeyPush(keypush, KEY_3, ToggleNBG2);
//   SetupKeyPush(keypush, KEY_4, ToggleNBG3);
//   SetupKeyPush(keypush, KEY_4, ToggleRBG0);
//   SetupKeyPush(keypush, KEY_5, ToggleVDP1);

   PerSetKey(KEY_UP, PERPAD_UP, pad[0]);
   PerSetKey(KEY_DOWN, PERPAD_DOWN, pad[0]);
   PerSetKey(KEY_LEFT, PERPAD_LEFT, pad[0]);
   PerSetKey(KEY_RIGHT, PERPAD_RIGHT, pad[0]);
   PerSetKey(KEY_K, PERPAD_A, pad[0]);
   PerSetKey(KEY_L, PERPAD_B, pad[0]);
   PerSetKey(KEY_M, PERPAD_C, pad[0]);
   PerSetKey(KEY_U, PERPAD_X, pad[0]);
   PerSetKey(KEY_I, PERPAD_Y, pad[0]);
   PerSetKey(KEY_O, PERPAD_Z, pad[0]);
   PerSetKey(KEY_X, PERPAD_LEFT_TRIGGER, pad[0]);
   PerSetKey(KEY_Z, PERPAD_RIGHT_TRIGGER, pad[0]);
   PerSetKey(KEY_J, PERPAD_START, pad[0]);

   IOS_IoctlAsync(kbdfd, 0, NULL, 0, (void *)&kbdevent, 0x10, KeyboardConnectCallback, NULL);
   return 0;
}

void PERKeyboardDeInit()
{
   if (kbdfd > -1)
   {
      IOS_Close(kbdfd);
      kbdfd = -1;
   }
}

int PERKeyboardHandleEvents(void)
{
   return YabauseExec();
}

PerInterface_struct PERWIIKBD = {
PERCORE_WIIKBD,
"USB Keyboard Interface",
PERKeyboardInit,
PERKeyboardDeInit,
PERKeyboardHandleEvents
};
