/*  Copyright 2006 Theo Berkau

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

#include <windows.h>
#include <dinput.h>
#include "../peripheral.h"
#include "perdx.h"
#include "../vdp1.h"
#include "../vdp2.h"
#include "../yui.h"

int PERDXInit(void);
void PERDXDeInit(void);
int PERDXHandleEvents(void);

PerInterface_struct PERDIRECTX = {
PERCORE_DIRECTX,
"DirectX Input Interface",
PERDXInit,
PERDXDeInit,
PERDXHandleEvents
};

void (*keydownfunclist[256])(void);
void (*keyupfunclist[256])(void);

extern HWND YabWin;

LPDIRECTINPUT8 lpDI8 = NULL;
LPDIRECTINPUTDEVICE8 lpDIDevice[256]; // I hope that's enough
u32 numdevices=0;

/* // This will be used later
struct
{
   u32 devicenum;
   u8 ButtonMap[13];
   void (*ButtonUp[13])();
   void (*ButtonDown[13])();
} padconf;

//////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EnumPeripheralsCallback (LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
   if (IDirectInput8_CreateDevice(lpDI8, &lpddi->guidInstance, &lpDIDevice[numdevices],
       NULL) == DI_OK)
      numdevices++;

   return DIENUM_CONTINUE;
}
*/
//////////////////////////////////////////////////////////////////////////////

void SetupKeyUpDown(u8 key, void (*downfunc)(), void (*upfunc)())
{
   keydownfunclist[key] = downfunc;
   keyupfunclist[key] = upfunc;
}

//////////////////////////////////////////////////////////////////////////////

void KeyStub(void)
{
}

//////////////////////////////////////////////////////////////////////////////

int PERDXInit(void)
{
   int i;
   DIPROPDWORD dipdw;

   if (DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
       &IID_IDirectInput8, (LPVOID *)&lpDI8, NULL) != DI_OK)
   {
      MessageBox (NULL, "DirectInput8Create error", "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

//   IDirectInput8_EnumDevices(lpDI8, DI8DEVCLASS_ALL, EnumPeripheralsCallback,
//                      NULL, DIEDFL_ATTACHEDONLY);

   if (IDirectInput8_CreateDevice(lpDI8, &GUID_SysKeyboard, &lpDIDevice[0],
       NULL) != DI_OK)
   {
      MessageBox (NULL, "IDirectInput8_CreateDevice error", "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   if (IDirectInputDevice8_SetDataFormat(lpDIDevice[0], &c_dfDIKeyboard) != DI_OK)
   {
      MessageBox (NULL, "IDirectInputDevice8_SetDataFormat error", "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   if (IDirectInputDevice8_SetCooperativeLevel(lpDIDevice[0], YabWin,
       DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY) != DI_OK)
   {
      MessageBox (NULL, "IDirectInputDevice8_SetCooperativeLevel error", "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   dipdw.diph.dwSize = sizeof(DIPROPDWORD);
   dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
   dipdw.diph.dwObj = 0;
   dipdw.diph.dwHow = DIPH_DEVICE;
   dipdw.dwData = 8; // should be enough

   // Setup Buffered input
   if (IDirectInputDevice8_SetProperty(lpDIDevice[0], DIPROP_BUFFERSIZE, &dipdw.diph) != DI_OK)
   {
      MessageBox (NULL, "IDirectInputDevice8_SetProperty error", "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   // Make sure Keyboard is acquired already
   IDirectInputDevice8_Acquire(lpDIDevice[0]);

   for(i = 0; i < 256; i++)
       SetupKeyUpDown(i, KeyStub, KeyStub);

   SetupKeyUpDown(DIK_UP, PerUpPressed, PerUpReleased);
   SetupKeyUpDown(DIK_DOWN, PerDownPressed, PerDownReleased);
   SetupKeyUpDown(DIK_LEFT, PerLeftPressed, PerLeftReleased);
   SetupKeyUpDown(DIK_RIGHT, PerRightPressed, PerRightReleased);
   SetupKeyUpDown(DIK_K, PerAPressed, PerAReleased);
   SetupKeyUpDown(DIK_L, PerBPressed, PerBReleased);
   SetupKeyUpDown(DIK_M, PerCPressed, PerCReleased);
   SetupKeyUpDown(DIK_U, PerXPressed, PerXReleased);
   SetupKeyUpDown(DIK_I, PerYPressed, PerYReleased);
   SetupKeyUpDown(DIK_O, PerZPressed, PerZReleased);
   SetupKeyUpDown(DIK_X, PerLTriggerPressed, PerLTriggerReleased);
   SetupKeyUpDown(DIK_Z, PerRTriggerPressed, PerRTriggerReleased);
   SetupKeyUpDown(DIK_J, PerStartPressed, PerStartReleased);
   SetupKeyUpDown(DIK_GRAVE, SpeedThrottleEnable, SpeedThrottleDisable);
   SetupKeyUpDown(DIK_Q, YuiQuit, KeyStub);
   SetupKeyUpDown(DIK_1, ToggleNBG0, KeyStub);
   SetupKeyUpDown(DIK_2, ToggleNBG1, KeyStub);
   SetupKeyUpDown(DIK_3, ToggleNBG2, KeyStub);
   SetupKeyUpDown(DIK_4, ToggleNBG3, KeyStub);
   SetupKeyUpDown(DIK_5, ToggleRBG0, KeyStub);
   SetupKeyUpDown(DIK_6, ToggleVDP1, KeyStub);
   SetupKeyUpDown(DIK_F1, ToggleFPS, KeyStub);
//   SetupKeyUpDown(DIK_F, ToggleFullScreen, KeyStub); // fix me

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERDXDeInit(void)
{
   u32 i;

   for (i = 0; i < numdevices; i++)
   {
      if (lpDIDevice[i])
      {
         IDirectInputDevice8_Unacquire(lpDIDevice[i]);
         IDirectInputDevice8_Release(lpDIDevice[i]);
         lpDIDevice[i] = NULL;
      }
   }

   if (lpDI8)
   {
      IDirectInput8_Release(lpDI8);
      lpDI8 = NULL;
   }
}

//////////////////////////////////////////////////////////////////////////////

void PollKeys(void)
{
   int i;
   DWORD size;
   DIDEVICEOBJECTDATA didod[8];
   HRESULT hr;

   if (lpDIDevice[0] == NULL)
      return;

   // Poll events
   if (IDirectInputDevice8_GetDeviceData(lpDIDevice[0],
       sizeof(DIDEVICEOBJECTDATA), didod, &size, 0) != DI_OK)
   {
      // Make sure deviced is acquired

      while ((hr = IDirectInputDevice8_Acquire(lpDIDevice[0])) == DIERR_INPUTLOST) {}

      return;
   }

   // This probably could be optimized
   for (i = 0; i < size; i++)
   {
      if (didod[i].dwData & 0x80)
         keydownfunclist[didod[i].dwOfs]();
      else
         keyupfunclist[didod[i].dwOfs]();
   }

}

//////////////////////////////////////////////////////////////////////////////

int PERDXHandleEvents(void)
{
   PollKeys();

   // I may end up changing this depending on people's results
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;
   if (YabauseExec() != 0)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////
