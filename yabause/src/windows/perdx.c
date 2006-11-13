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
#ifdef __MINGW32__
// I have to do this because for some reason because the dxerr8.h header is fubared
const char*  __stdcall DXGetErrorString8A(HRESULT hr);
#define DXGetErrorString8 DXGetErrorString8A
const char*  __stdcall DXGetErrorDescription8A(HRESULT hr);
#define DXGetErrorDescription8 DXGetErrorDescription8A
#else
#include <dxerr8.h>
#endif
#include "../debug.h"
#include "../peripheral.h"
#include "perdx.h"
#include "../vdp1.h"
#include "../vdp2.h"
#include "../yui.h"
#include "resource.h"

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

extern HWND YabWin;
extern HINSTANCE y_hInstance;

LPDIRECTINPUT8 lpDI8 = NULL;
LPDIRECTINPUTDEVICE8 lpDIDevice[256]; // I hope that's enough
GUID GUIDDevice[256]; // I hope that's enough
u32 numguids=0;
u32 numdevices=0;

typedef struct
{
   LPDIRECTINPUTDEVICE8 lpDIDevice;
   int type;
   void (*up[256])(void);
   void (*down[256])(void);
} padconf_struct;

u32 numpads=1;
padconf_struct pad[1];

#define TYPE_KEYBOARD           0
#define TYPE_JOYSTICK           1
#define TYPE_MOUSE              2

//////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK EnumPeripheralsCallback (LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
   if (GET_DIDEVICE_TYPE(lpddi->dwDevType) == DI8DEVTYPE_GAMEPAD ||
       GET_DIDEVICE_TYPE(lpddi->dwDevType) == DI8DEVTYPE_JOYSTICK ||
       GET_DIDEVICE_TYPE(lpddi->dwDevType) == DI8DEVTYPE_KEYBOARD)
   {     
      if (IDirectInput8_CreateDevice(lpDI8, &lpddi->guidInstance, &lpDIDevice[numdevices],
          NULL) == DI_OK)
         numdevices++;
   }

   return DIENUM_CONTINUE;
}

//////////////////////////////////////////////////////////////////////////////

void SetupControlUpDown(u8 padnum, u8 controlcode, void (*downfunc)(), void (*upfunc)())
{
   pad[padnum].down[controlcode] = downfunc;
   pad[padnum].up[controlcode] = upfunc;
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
   char tempstr[512];
   HRESULT ret;

   memset(pad, 0, sizeof(padconf_struct) * numpads);

   if ((ret = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
       &IID_IDirectInput8, (LPVOID *)&lpDI8, NULL)) != DI_OK)
   {
      sprintf(tempstr, "DirectInput8Create error: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
      MessageBox (NULL, tempstr, "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   IDirectInput8_EnumDevices(lpDI8, DI8DEVCLASS_ALL, EnumPeripheralsCallback,
                      NULL, DIEDFL_ATTACHEDONLY);

   if ((ret = IDirectInput8_CreateDevice(lpDI8, &GUID_SysKeyboard, &lpDIDevice[0],
       NULL)) != DI_OK)
   {
      sprintf(tempstr, "IDirectInput8_CreateDevice error: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
      MessageBox (NULL, tempstr, "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   if ((ret = IDirectInputDevice8_SetDataFormat(lpDIDevice[0], &c_dfDIKeyboard)) != DI_OK)
   {
      sprintf(tempstr, "IDirectInputDevice8_SetDataFormat error: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
      MessageBox (NULL, tempstr, "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   if ((ret = IDirectInputDevice8_SetCooperativeLevel(lpDIDevice[0], YabWin,
       DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY)) != DI_OK)
   {
      sprintf(tempstr, "IDirectInputDevice8_SetCooperativeLevel error: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
      MessageBox (NULL, tempstr, "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   dipdw.diph.dwSize = sizeof(DIPROPDWORD);
   dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
   dipdw.diph.dwObj = 0;
   dipdw.diph.dwHow = DIPH_DEVICE;
   dipdw.dwData = 8; // should be enough

   // Setup Buffered input
   if ((ret = IDirectInputDevice8_SetProperty(lpDIDevice[0], DIPROP_BUFFERSIZE, &dipdw.diph)) != DI_OK)
   {
      sprintf(tempstr, "IDirectInputDevice8_SetProperty error: %s - %s", DXGetErrorString8(ret), DXGetErrorDescription8(ret));
      MessageBox (NULL, tempstr, "Error",  MB_OK | MB_ICONINFORMATION);
      return -1;
   }

   // Make sure Keyboard is acquired already
   IDirectInputDevice8_Acquire(lpDIDevice[0]);

   pad[0].lpDIDevice = lpDIDevice[0];
   pad[0].type = TYPE_KEYBOARD;

   for(i = 0; i < 256; i++)
       SetupControlUpDown(0, i, KeyStub, KeyStub);

   SetupControlUpDown(0, DIK_UP, PerUpPressed, PerUpReleased);
   SetupControlUpDown(0, DIK_DOWN, PerDownPressed, PerDownReleased);
   SetupControlUpDown(0, DIK_LEFT, PerLeftPressed, PerLeftReleased);
   SetupControlUpDown(0, DIK_RIGHT, PerRightPressed, PerRightReleased);
   SetupControlUpDown(0, DIK_K, PerAPressed, PerAReleased);
   SetupControlUpDown(0, DIK_L, PerBPressed, PerBReleased);
   SetupControlUpDown(0, DIK_M, PerCPressed, PerCReleased);
   SetupControlUpDown(0, DIK_U, PerXPressed, PerXReleased);
   SetupControlUpDown(0, DIK_I, PerYPressed, PerYReleased);
   SetupControlUpDown(0, DIK_O, PerZPressed, PerZReleased);
   SetupControlUpDown(0, DIK_X, PerLTriggerPressed, PerLTriggerReleased);
   SetupControlUpDown(0, DIK_Z, PerRTriggerPressed, PerRTriggerReleased);
   SetupControlUpDown(0, DIK_J, PerStartPressed, PerStartReleased);

   numdevices = 1;

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void PERDXLoadDevices(char *inifilename)
{
   char tempstr[MAX_PATH];
   char string1[20];
   GUID guid;
   DIDEVCAPS didc;
   int i, i2;
   int buttonid;
   DIPROPDWORD dipdw;

   for (i = 0; i < numpads; i++)
   {
      sprintf(string1, "Peripheral%d", i+1);

      // Let's first fetch the guid of the device
      if (GetPrivateProfileString(string1, "GUID", "", tempstr, MAX_PATH, inifilename) == 0)
         continue;

      if (pad[i].lpDIDevice)
      {
         // Free the default keyboard
         IDirectInputDevice8_Unacquire(pad[i].lpDIDevice);
         IDirectInputDevice8_Release(pad[i].lpDIDevice);
      }

      for(i2 = 0; i2 < 256; i2++)
         SetupControlUpDown(i, i2, KeyStub, KeyStub);

      sscanf(tempstr, "%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X", (int *)&guid.Data1, (int *)&guid.Data2, (int *)&guid.Data3, (int *)&guid.Data4[0], (int *)&guid.Data4[1], (int *)&guid.Data4[2], (int *)&guid.Data4[3], (int *)&guid.Data4[4], (int *)&guid.Data4[5], (int *)&guid.Data4[6], (int *)&guid.Data4[7]);

      // Ok, now that we've got the GUID of the device, let's set it up

      if (IDirectInput8_CreateDevice(lpDI8, &guid, &lpDIDevice[i],
          NULL) != DI_OK)
         continue;

      pad[i].lpDIDevice = lpDIDevice[i];

      didc.dwSize = sizeof(DIDEVCAPS);

      if (IDirectInputDevice8_GetCapabilities(lpDIDevice[i], &didc) != DI_OK)
         continue;

      if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_KEYBOARD)
      {
         if (IDirectInputDevice8_SetDataFormat(lpDIDevice[i], &c_dfDIKeyboard) != DI_OK)
            continue;
         pad[i].type = TYPE_KEYBOARD;
      }       
      else if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_GAMEPAD ||
               GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_JOYSTICK)
      {
         if (IDirectInputDevice8_SetDataFormat(lpDIDevice[i], &c_dfDIJoystick) != DI_OK)
            continue;
         pad[i].type = TYPE_JOYSTICK;
      }

      if (IDirectInputDevice8_SetCooperativeLevel(lpDIDevice[i], YabWin,
          DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY) != DI_OK)
         continue;

      dipdw.diph.dwSize = sizeof(DIPROPDWORD);
      dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
      dipdw.diph.dwObj = 0;
      dipdw.diph.dwHow = DIPH_DEVICE;
      dipdw.dwData = 8; // should be enough

      // Setup Buffered input
      if (IDirectInputDevice8_SetProperty(lpDIDevice[i], DIPROP_BUFFERSIZE, &dipdw.diph) != DI_OK)
         continue;

      IDirectInputDevice8_Acquire(lpDIDevice[i]);

      // Now that we're all setup, let's fetch the controls from the ini
      sprintf(string1, "Peripheral%d", i+1);

      buttonid = GetPrivateProfileInt(string1, "Up", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerUpPressed, PerUpReleased);

      buttonid = GetPrivateProfileInt(string1, "Down", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerDownPressed, PerDownReleased);

      buttonid = GetPrivateProfileInt(string1, "Left", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerLeftPressed, PerLeftReleased);

      buttonid = GetPrivateProfileInt(string1, "Right", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerRightPressed, PerRightReleased);

      buttonid = GetPrivateProfileInt(string1, "L", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerLTriggerPressed, PerLTriggerReleased);

      buttonid = GetPrivateProfileInt(string1, "R", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerRTriggerPressed, PerRTriggerReleased);

      buttonid = GetPrivateProfileInt(string1, "Start", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerStartPressed, PerStartReleased);

      buttonid = GetPrivateProfileInt(string1, "A", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerAPressed, PerAReleased);

      buttonid = GetPrivateProfileInt(string1, "B", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerBPressed, PerBReleased);

      buttonid = GetPrivateProfileInt(string1, "C", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerCPressed, PerCReleased);

      buttonid = GetPrivateProfileInt(string1, "X", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerXPressed, PerXReleased);

      buttonid = GetPrivateProfileInt(string1, "Y", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerYPressed, PerYReleased);

      buttonid = GetPrivateProfileInt(string1, "Z", 0, inifilename);
      SetupControlUpDown(i, buttonid, PerZPressed, PerZReleased);
   }
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
   int i, i2;
   DWORD size=8;
   DIDEVICEOBJECTDATA didod[8];
   HRESULT hr;

   for (i = 0; i < numpads; i++)
   {
      if (pad[i].lpDIDevice == NULL)
         return;

      hr = IDirectInputDevice8_Poll(pad[i].lpDIDevice);

      if (FAILED(hr))
      {
         if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
         {
            // Make sure device is acquired
            while(IDirectInputDevice8_Acquire(pad[i].lpDIDevice) == DIERR_INPUTLOST) {}
            return;
         }
      }

      // Poll events
      if (IDirectInputDevice8_GetDeviceData(pad[i].lpDIDevice,
          sizeof(DIDEVICEOBJECTDATA), didod, &size, 0) != DI_OK)
      {
         if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
         {
            // Make sure device is acquired
            while(IDirectInputDevice8_Acquire(pad[i].lpDIDevice) == DIERR_INPUTLOST) {}
            return;
         }
      }

      switch (pad[i].type)
      {
         case TYPE_KEYBOARD:
            // This probably could be optimized
            for (i2 = 0; i2 < size; i2++)
            {
               if (didod[i2].dwData & 0x80)
                  pad[i].down[didod[i2].dwOfs]();
               else
                  pad[i].up[didod[i2].dwOfs]();
            }
            break;
         case TYPE_JOYSTICK:
         {
            // This probably could be optimized
            for (i2 = 0; i2 < size; i2++)
            {
               if (didod[i2].dwOfs == 0)
               {
                  if (didod[i2].dwData < 0x7FFF)
                  {
                     pad[i].down[0]();
                     pad[i].up[1]();
                  }
                  else if (didod[i2].dwData > 0x7FFF)
                  {
                     pad[i].down[1]();
                     pad[i].up[0]();
                  }
                  else
                  {
                     pad[i].up[0]();
                     pad[i].up[1]();
                  }
               }
               else if (didod[i2].dwOfs == 4)
               {
                  if (didod[i2].dwData < 0x7FFF)
                  {
                     pad[i].down[2]();
                     pad[i].up[3]();
                  }
                  else if ( didod[i2].dwData > 0x7FFF)
                  {
                     pad[i].down[3]();
                     pad[i].up[2]();
                  }
                  else
                  {
                     pad[i].up[2]();
                     pad[i].up[3]();
                  }
               }
               else if (didod[i2].dwOfs == 0x20)
               {
                  // This should really allow for diagonals
                  // POV Up
                  if (didod[i2].dwData < 9000)
                  {
                     pad[i].down[4]();
                     pad[i].up[6]();
                  }
                  // POV Right
                  else if (didod[i2].dwData < 18000)
                  {
                     pad[i].down[5]();
                     pad[i].up[7]();
                  }
                  // POV Down
                  else if (didod[i2].dwData < 27000)
                  {
                     pad[i].down[6]();
                     pad[i].up[4]();
                  }
                  // POV Left
                  else if (didod[i2].dwData < 36000)
                  {
                     pad[i].down[7]();
                     pad[i].up[5]();
                  }
                  else
                  {
                     pad[i].up[4]();
                     pad[i].up[5]();
                     pad[i].up[6]();
                     pad[i].up[7]();
                  }
               }
               else if (didod[i2].dwOfs >= 0x30)
               {
                  if (didod[i2].dwData & 0x80)
                     pad[i].down[didod[i2].dwOfs]();
                  else
                     pad[i].up[didod[i2].dwOfs]();
               }
            }
            break;
         }
         case TYPE_MOUSE:
            break;
         default: break;
      }
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

BOOL CALLBACK EnumPeripheralsCallback2 (LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
   if (GET_DIDEVICE_TYPE(lpddi->dwDevType) == DI8DEVTYPE_GAMEPAD ||
       GET_DIDEVICE_TYPE(lpddi->dwDevType) == DI8DEVTYPE_JOYSTICK ||
       GET_DIDEVICE_TYPE(lpddi->dwDevType) == DI8DEVTYPE_KEYBOARD)
   {
      SendMessage((HWND)pvRef, CB_ADDSTRING, 0, (long)lpddi->tszInstanceName);
      memcpy(&GUIDDevice[numguids], &lpddi->guidInstance, sizeof(GUID));
      numguids++;
   }

   return DIENUM_CONTINUE;
}

//////////////////////////////////////////////////////////////////////////////

void PERDXListDevices(HWND control)
{
   LPDIRECTINPUT8 lpDI8temp = NULL;

   if (DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
       &IID_IDirectInput8, (LPVOID *)&lpDI8temp, NULL) != DI_OK)
      return;

   numguids = 0;

   SendMessage(control, CB_RESETCONTENT, 0, 0);
   SendMessage(control, CB_ADDSTRING, 0, (long)"None");

   IDirectInput8_EnumDevices(lpDI8temp, DI8DEVCLASS_ALL, EnumPeripheralsCallback2,
                             (LPVOID)control, DIEDFL_ATTACHEDONLY);

   IDirectInput8_Release(lpDI8temp);
}

//////////////////////////////////////////////////////////////////////////////

void ConvertKBIDToName(int buttonid, char *string)
{
   memset(string, 0, MAX_PATH);

   // This fixes some strange inconsistencies
   if (buttonid == DIK_PAUSE)
      buttonid = DIK_NUMLOCK;
   else if (buttonid == DIK_NUMLOCK)
      buttonid = DIK_PAUSE;
   if (buttonid & 0x80)
      buttonid += 0x80;

   GetKeyNameText(buttonid << 16, string, MAX_PATH);
}

//////////////////////////////////////////////////////////////////////////////

void ConvertJoyIDToName(int buttonid, char *string)
{
   switch (buttonid)
   {
      case 0x00:
         sprintf(string, "Axis Left");
         break;
      case 0x01:
         sprintf(string, "Axis Right");
         break;
      case 0x02:
         sprintf(string, "Axis Up");
         break;
      case 0x03:
         sprintf(string, "Axis Down");
         break;
      case 0x04:
         sprintf(string, "POV Up");
         break;
      case 0x05:
         sprintf(string, "POV Right");
         break;
      case 0x06:
         sprintf(string, "POV Down");
         break;
      case 0x07:
         sprintf(string, "POV Left");
         break;
      default:
         if (buttonid >= 0x30)
            sprintf(string, "Button %d", buttonid - 0x2F);
         break;
   }

}

int PERDXInitControlConfig(HWND hWnd, u8 padnum, int *controlmap, const char *inifilename)
{
   char tempstr[MAX_PATH];
   char string1[20];
   GUID guid;
   int i;

   sprintf(string1, "Peripheral%d", padnum+1);

   // Let's first fetch the guid of the device and see if we can get a match
   if (GetPrivateProfileString(string1, "GUID", "", tempstr, MAX_PATH, inifilename) == 0)
   {
      if (padnum == 0)
      {
         // Let's use default values
         SendDlgItemMessage(hWnd, IDC_DXDEVICECB, CB_SETCURSEL, 1, 0);
      }
      else
      {
         SendDlgItemMessage(hWnd, IDC_DXDEVICECB, CB_SETCURSEL, 0, 0);
         return -1;
      }
   }
   else
   {
      LPDIRECTINPUT8 lpDI8temp = NULL;
      LPDIRECTINPUTDEVICE8 lpDIDevicetemp;
      DIDEVCAPS didc;
      int buttonid;

      sscanf(tempstr, "%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X", (int *)&guid.Data1, (int *)&guid.Data2, (int *)&guid.Data3, (int *)&guid.Data4[0], (int *)&guid.Data4[1], (int *)&guid.Data4[2], (int *)&guid.Data4[3], (int *)&guid.Data4[4], (int *)&guid.Data4[5], (int *)&guid.Data4[6], (int *)&guid.Data4[7]);

      // Let's find a match
      for (i = 0; i < numguids; i++)
      {
         if (memcmp(&guid, &GUIDDevice[i], sizeof(GUID)) == 0)
         {
            SendDlgItemMessage(hWnd, IDC_DXDEVICECB, CB_SETCURSEL, i+1, 0);
            break;
         }
      }

      if (DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
          &IID_IDirectInput8, (LPVOID *)&lpDI8temp, NULL) != DI_OK)
         return -1;

      if (IDirectInput8_CreateDevice(lpDI8, &GUIDDevice[i], &lpDIDevicetemp,
          NULL) != DI_OK)
      {
         IDirectInput8_Release(lpDI8temp);
         return -1;
      }

      didc.dwSize = sizeof(DIDEVCAPS);

      if (IDirectInputDevice8_GetCapabilities(lpDIDevicetemp, &didc) != DI_OK)
      {
         IDirectInputDevice8_Release(lpDIDevicetemp);       
         IDirectInput8_Release(lpDI8temp);
         return -1;
      }

      if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_KEYBOARD)
      {
         sprintf(string1, "Peripheral%d", padnum+1);

         buttonid = GetPrivateProfileInt(string1, "Up", 0, inifilename);
         controlmap[0] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_UPTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Down", 0, inifilename);
         controlmap[1] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_DOWNTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Left", 0, inifilename);
         controlmap[2] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_LEFTTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Right", 0, inifilename);
         controlmap[3] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_RIGHTTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "L", 0, inifilename);
         controlmap[4] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_LTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "R", 0, inifilename);
         controlmap[5] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_RTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Start", 0, inifilename);
         controlmap[6] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_STARTTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "A", 0, inifilename);
         controlmap[7] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_ATEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "B", 0, inifilename);
         controlmap[8] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_BTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "C", 0, inifilename);
         controlmap[9] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_CTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "X", 0, inifilename);
         controlmap[10] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_XTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Y", 0, inifilename);
         controlmap[11] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_YTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Z", 0, inifilename);
         controlmap[12] = buttonid;
         ConvertKBIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_ZTEXT, tempstr);
      }       
      else if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_GAMEPAD ||
              GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_JOYSTICK)
      {
         sprintf(string1, "Peripheral%d", padnum+1);

         buttonid = GetPrivateProfileInt(string1, "Up", 0, inifilename);
         controlmap[0] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_UPTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Down", 0, inifilename);
         controlmap[1] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_DOWNTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Left", 0, inifilename);
         controlmap[2] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_LEFTTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Right", 0, inifilename);
         controlmap[3] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_RIGHTTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "L", 0, inifilename);
         controlmap[4] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_LTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "R", 0, inifilename);
         controlmap[5] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_RTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Start", 0, inifilename);
         controlmap[6] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_STARTTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "A", 0, inifilename);
         controlmap[7] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_ATEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "B", 0, inifilename);
         controlmap[8] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_BTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "C", 0, inifilename);
         controlmap[9] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_CTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "X", 0, inifilename);
         controlmap[10] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_XTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Y", 0, inifilename);
         controlmap[11] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_YTEXT, tempstr);

         buttonid = GetPrivateProfileInt(string1, "Z", 0, inifilename);
         controlmap[12] = buttonid;
         ConvertJoyIDToName(buttonid, tempstr);
         SetDlgItemText(hWnd, IDC_ZTEXT, tempstr);
      }

      IDirectInputDevice8_Release(lpDIDevicetemp);       
      IDirectInput8_Release(lpDI8temp);
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

DIDEVICEOBJECTDATA nextpress;

int PERDXFetchNextPress(HWND hWnd, u32 guidnum, char *buttonname)
{
   LPDIRECTINPUT8 lpDI8temp = NULL;
   LPDIRECTINPUTDEVICE8 lpDIDevicetemp;
   DIDEVCAPS didc;
   int buttonid=-1;

   if (DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
       &IID_IDirectInput8, (LPVOID *)&lpDI8temp, NULL) != DI_OK)
      return -1;

   if (IDirectInput8_CreateDevice(lpDI8temp, &GUIDDevice[guidnum], &lpDIDevicetemp,
       NULL) != DI_OK)
   {
      IDirectInput8_Release(lpDI8temp);
      return -1;
   }

   didc.dwSize = sizeof(DIDEVCAPS);

   if (IDirectInputDevice8_GetCapabilities(lpDIDevicetemp, &didc) != DI_OK)
   {
      IDirectInputDevice8_Release(lpDIDevicetemp);       
      IDirectInput8_Release(lpDI8temp);
      return -1;
   }

   if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_KEYBOARD)
   {
      if (IDirectInputDevice8_SetDataFormat(lpDIDevicetemp, &c_dfDIKeyboard) != DI_OK)
      {
         IDirectInputDevice8_Release(lpDIDevicetemp);       
         IDirectInput8_Release(lpDI8temp);
         return -1;
      }
   }       
   else if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_GAMEPAD ||
           GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_JOYSTICK)
   {
      if (IDirectInputDevice8_SetDataFormat(lpDIDevicetemp, &c_dfDIJoystick) != DI_OK)
      {
         IDirectInputDevice8_Release(lpDIDevicetemp);       
         IDirectInput8_Release(lpDI8temp);
         return -1;
      }
   }

   if (DialogBoxParam(y_hInstance, "ButtonConfigDlg", hWnd, (DLGPROC)ButtonConfigDlgProc, (LPARAM)lpDIDevicetemp) == TRUE)
   {
      // Figure out what kind of code to generate
      if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_KEYBOARD)
      {
         memset(buttonname, 0, MAX_PATH);
         buttonid = nextpress.dwOfs;
         // This fixes some strange inconsistencies
         if (buttonid == DIK_PAUSE)
            buttonid = DIK_NUMLOCK;
         else if (buttonid == DIK_NUMLOCK)
            buttonid = DIK_PAUSE;
         if (buttonid & 0x80)
            buttonid += 0x80;

         GetKeyNameText(buttonid << 16, buttonname, MAX_PATH);
         buttonid = nextpress.dwOfs;
      }
      else if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_GAMEPAD ||
               GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_JOYSTICK)
      {
         if (nextpress.dwOfs == 0)
         {
            if (nextpress.dwData <= 0x8000)
            {
               sprintf(buttonname, "Axis Left");
               buttonid = 0x00;
            }
            else
            {
               sprintf(buttonname, "Axis Right");
               buttonid = 0x01;
            }
         }
         else if (nextpress.dwOfs == 4)
         {
            if (nextpress.dwData <= 0x8000)
            {
               sprintf(buttonname, "Axis Up");
               buttonid = 0x02;
            }
            else
            {
               sprintf(buttonname, "Axis Down");
               buttonid = 0x03;
            }
         }
         else if (nextpress.dwOfs == 0x20)
         {
            if (nextpress.dwData < 9000)
            {
               sprintf(buttonname, "POV Up");
               buttonid = 0x04;
            }
            else if (nextpress.dwData < 18000)
            {
               sprintf(buttonname, "POV Right");
               buttonid = 0x05;
            }
            else if (nextpress.dwData < 27000)
            {
               sprintf(buttonname, "POV Down");
               buttonid = 0x06;
            }
            else
            {
               sprintf(buttonname, "POV Left");
               buttonid = 0x07;
            }
         }
         else if (nextpress.dwOfs >= 0x30)
         {
            sprintf(buttonname, "Button %d", (int)(nextpress.dwOfs - 0x2F));
            buttonid = nextpress.dwOfs;
         }
      }
   }

   IDirectInputDevice8_Unacquire(lpDIDevicetemp);
   IDirectInputDevice8_Release(lpDIDevicetemp);       
   IDirectInput8_Release(lpDI8temp);

   return buttonid;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK ButtonConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam)
{
   static LPDIRECTINPUTDEVICE8 lpDIDevicetemp;
   DIPROPDWORD dipdw;
   HRESULT hr;
   DWORD size;
   DIDEVICEOBJECTDATA didod[8];
   int i;
   DIDEVCAPS didc;

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         RECT dlgsize;
         int cxScreen, cyScreen;
         int newx, newy;

         cxScreen = GetSystemMetrics(SM_CXSCREEN);
         cyScreen = GetSystemMetrics(SM_CYSCREEN);
         GetWindowRect(hDlg, &dlgsize);

         newx = (cxScreen - dlgsize.right) / 2;
         newy = (cyScreen - dlgsize.bottom) / 2;

         MoveWindow(hDlg, newx, newy, dlgsize.right-dlgsize.left, dlgsize.bottom-dlgsize.top, TRUE);

         lpDIDevicetemp = (LPDIRECTINPUTDEVICE8)lParam;

         if (IDirectInputDevice8_SetCooperativeLevel(lpDIDevicetemp, hDlg,
              DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY) != DI_OK)
            return FALSE;

         dipdw.diph.dwSize = sizeof(DIPROPDWORD);
         dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
         dipdw.diph.dwObj = 0;
         dipdw.diph.dwHow = DIPH_DEVICE;
         dipdw.dwData = 8; // should be enough

         // Setup Buffered input
         if ((hr = IDirectInputDevice8_SetProperty(lpDIDevicetemp, DIPROP_BUFFERSIZE, &dipdw.diph)) != DI_OK)
            return FALSE;

         if (!SetTimer(hDlg, 1, 100, NULL))
             return FALSE;

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_CUSTOMCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            default: break;
         }

         break;
      }
      case WM_TIMER:
      {
         size = 8;

         if (wParam == 1)
         {
            memset(&didod, 0, sizeof(DIDEVICEOBJECTDATA) * 8);

            // Let's see if there's any data waiting
            hr = IDirectInputDevice8_Poll(lpDIDevicetemp);

            if (FAILED(hr))
            {
               if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
               {
                  // Make sure device is acquired
                  while(IDirectInputDevice8_Acquire(lpDIDevicetemp) == DIERR_INPUTLOST) {}
                  return TRUE;
               }
            }

            // Poll events
            if (IDirectInputDevice8_GetDeviceData(lpDIDevicetemp,
                sizeof(DIDEVICEOBJECTDATA), didod, &size, 0) != DI_OK)
            {
               if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
               {
                  // Make sure device is acquired
                  while(IDirectInputDevice8_Acquire(lpDIDevicetemp) == DIERR_INPUTLOST) {}
                  return TRUE;
               }
            }

            didc.dwSize = sizeof(DIDEVCAPS);

            if (IDirectInputDevice8_GetCapabilities(lpDIDevicetemp, &didc) != DI_OK)
               return TRUE;

            if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_KEYBOARD)
            {
               for (i = 0; i < size; i++)
               {
                  if (didod[i].dwData & 0x80)
                  {
                     // We're done. time to bail
                     EndDialog(hDlg, TRUE);
                     memcpy(&nextpress, &didod[i], sizeof(DIDEVICEOBJECTDATA));
                     break;
                  }
               }
            }
            else if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_GAMEPAD ||
                     GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_JOYSTICK)
            {
               for (i = 0; i < size; i++)
               {
                  if (didod[i].dwOfs == 0 ||
                      didod[i].dwOfs == 4)
                  {
                     if (didod[i].dwData <= 0x1000 ||
                         didod[i].dwData >= 0xF000)
                     {
                        // We're done. time to bail
                        EndDialog(hDlg, TRUE);
                        memcpy(&nextpress, &didod[i], sizeof(DIDEVICEOBJECTDATA));
                        break;
                     }
                  }
                  else if (didod[i].dwOfs == 0x20)
                  {
                     if (((int)didod[i].dwData) >= 0)
                     {
                        // We're done. time to bail
                        EndDialog(hDlg, TRUE);
                        memcpy(&nextpress, &didod[i], sizeof(DIDEVICEOBJECTDATA));
                     }                     
                  }
                  else if (didod[i].dwOfs >= 0x30)
                  {
                     if (didod[i].dwData & 0x80)
                     {
                        // We're done. time to bail
                        EndDialog(hDlg, TRUE);
                        memcpy(&nextpress, &didod[i], sizeof(DIDEVICEOBJECTDATA));
                        break;
                     }
                  }
               }
            }

            return TRUE;
         }

         return FALSE;
      }
      case WM_DESTROY:
      {
         KillTimer(hDlg, 1);
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

BOOL PERDXWriteGUID(u32 guidnum, u8 padnum, LPCTSTR inifilename)
{
   char string1[20];
   char string2[40];
   sprintf(string1, "Peripheral%d", padnum+1);
   sprintf(string2, "%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X", (int)GUIDDevice[guidnum].Data1, (int)GUIDDevice[guidnum].Data2, (int)GUIDDevice[guidnum].Data3, (int)GUIDDevice[guidnum].Data4[0], (int)GUIDDevice[guidnum].Data4[1], (int)GUIDDevice[guidnum].Data4[2], (int)GUIDDevice[guidnum].Data4[3], (int)GUIDDevice[guidnum].Data4[4], (int)GUIDDevice[guidnum].Data4[5], (int)GUIDDevice[guidnum].Data4[6], (int)GUIDDevice[guidnum].Data4[7]);
   return WritePrivateProfileString(string1, "GUID", string2, inifilename);
}

//////////////////////////////////////////////////////////////////////////////

