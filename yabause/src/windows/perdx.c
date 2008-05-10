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

u32 numpads=12;
PerPad_struct *pad[12];
padconf_struct paddevice[12];

const int pad_names[] = {
PERPAD_UP,
PERPAD_DOWN,
PERPAD_LEFT,
PERPAD_RIGHT,
PERPAD_A,
PERPAD_B,
PERPAD_C,
PERPAD_X,
PERPAD_Y,
PERPAD_Z,
PERPAD_LEFT_TRIGGER,
PERPAD_RIGHT_TRIGGER,
PERPAD_START
};

const char *pad_names2[] = {
"Up",
"Down",
"Left",
"Right",
"A",
"B",
"C",
"X",
"Y",
"Z",
"L",
"R",
"Start",
NULL
};

#define TYPE_KEYBOARD           0
#define TYPE_JOYSTICK           1
#define TYPE_MOUSE              2

#define PAD_DIR_AXISLEFT        0
#define PAD_DIR_AXISRIGHT       1
#define PAD_DIR_AXISUP          2
#define PAD_DIR_AXISDOWN        3
#define PAD_DIR_POVUP           4
#define PAD_DIR_POVRIGHT        5
#define PAD_DIR_POVDOWN         6
#define PAD_DIR_POVLEFT         7

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

int PERDXInit(void)
{
   DIPROPDWORD dipdw;
   char tempstr[512];
   HRESULT ret;

   memset(pad, 0, sizeof(pad));
   memset(paddevice, 0, sizeof(paddevice));

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

   paddevice[0].lpDIDevice = lpDIDevice[0];
   paddevice[0].type = TYPE_KEYBOARD;
   paddevice[0].emulatetype = 1;

   PerPortReset();
   pad[0] = PerPadAdd(&PORTDATA1);

   PerSetKey(DIK_UP, PERPAD_UP, pad[0]);
   PerSetKey(DIK_DOWN, PERPAD_DOWN, pad[0]);
   PerSetKey(DIK_LEFT, PERPAD_LEFT, pad[0]);
   PerSetKey(DIK_RIGHT, PERPAD_RIGHT, pad[0]);
   PerSetKey(DIK_K, PERPAD_A, pad[0]);
   PerSetKey(DIK_L, PERPAD_B, pad[0]);
   PerSetKey(DIK_M, PERPAD_C, pad[0]);
   PerSetKey(DIK_U, PERPAD_X, pad[0]);
   PerSetKey(DIK_I, PERPAD_Y, pad[0]);
   PerSetKey(DIK_O, PERPAD_Z, pad[0]);
   PerSetKey(DIK_X, PERPAD_LEFT_TRIGGER, pad[0]);
   PerSetKey(DIK_Z, PERPAD_RIGHT_TRIGGER, pad[0]);
   PerSetKey(DIK_J, PERPAD_START, pad[0]);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void StringToGUID(const char *string, GUID *guid)
{
   int data4[8];
   int i;

   sscanf(string, "%08lX-%04hX-%04hX-%02X%02X%02X%02X%02X%02X%02X%02X", (int *)&guid->Data1, (int *)&guid->Data2, (int *)&guid->Data3, &data4[0], &data4[1], &data4[2], &data4[3], &data4[4], &data4[5], &data4[6], &data4[7]);
   for (i = 0; i < 8; i++)
     guid->Data4[i] = (BYTE)data4[i];
}

//////////////////////////////////////////////////////////////////////////////

void PERDXLoadDevices(char *inifilename)
{
   char tempstr[MAX_PATH];
   char string1[20];
   char string2[20];
   GUID guid;
   DIDEVCAPS didc;
   u32 i;
   int i2;
   int buttonid;
   DIPROPDWORD dipdw;

   if (!PERCore)
      return;
   PerPortReset();
   memset(pad, 0, sizeof(pad));

   for (i = 0; i < numpads; i++)
   {
      sprintf(string1, "Peripheral%d", (int)i+1);

      // Let's first fetch the guid of the device
      if (GetPrivateProfileString(string1, "GUID", "", tempstr, MAX_PATH, inifilename) == 0)
         continue;


      if (GetPrivateProfileString(string1, "EmulateType", "0", string2, MAX_PATH, inifilename))
      {
         paddevice[i].emulatetype = atoi(string2);
         if (paddevice[i].emulatetype == 0)
            continue;
      }

      if (paddevice[i].lpDIDevice)
      {
         // Free the default keyboard
         IDirectInputDevice8_Unacquire(paddevice[i].lpDIDevice);
         IDirectInputDevice8_Release(paddevice[i].lpDIDevice);
      }

      StringToGUID(tempstr, &guid);

      // Ok, now that we've got the GUID of the device, let's set it up
      if (IDirectInput8_CreateDevice(lpDI8, &guid, &lpDIDevice[i],
          NULL) != DI_OK)
      {
         paddevice[i].lpDIDevice = NULL;
         paddevice[i].emulatetype = 0;
         continue;
      }

      paddevice[i].lpDIDevice = lpDIDevice[i];

      didc.dwSize = sizeof(DIDEVCAPS);

      if (IDirectInputDevice8_GetCapabilities(lpDIDevice[i], &didc) != DI_OK)
         continue;

      if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_KEYBOARD)
      {
         if (IDirectInputDevice8_SetDataFormat(lpDIDevice[i], &c_dfDIKeyboard) != DI_OK)
            continue;
         paddevice[i].type = TYPE_KEYBOARD;
      }       
      else if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_GAMEPAD ||
               GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_JOYSTICK)
      {
         if (IDirectInputDevice8_SetDataFormat(lpDIDevice[i], &c_dfDIJoystick2) != DI_OK)
            continue;
         paddevice[i].type = TYPE_JOYSTICK;
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

      // Make sure we're added to the smpc list 
      if (i < 6)
         pad[i] = PerPadAdd(&PORTDATA1);
      else
         pad[i] = PerPadAdd(&PORTDATA2);

      // Now that we're all setup, let's fetch the controls from the ini
      sprintf(string1, "Peripheral%d", (int)i+1);

      for (i2 = 0; i2 < 13; i2++)
      {
         buttonid = GetPrivateProfileInt(string1, pad_names2[i2], 0, inifilename);
         PerSetKey(buttonid, pad_names[i2], pad[i]);
      }
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
   u32 i;
   DWORD i2;
   DWORD size=8;
   DIDEVICEOBJECTDATA didod[8];
   HRESULT hr;

   for (i = 0; i < numpads; i++)
   {
      if (paddevice[i].lpDIDevice == NULL)
         continue;

      hr = IDirectInputDevice8_Poll(paddevice[i].lpDIDevice);

      if (FAILED(hr))
      {
         if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
         {
            // Make sure device is acquired
            while(IDirectInputDevice8_Acquire(paddevice[i].lpDIDevice) == DIERR_INPUTLOST) {}
            continue;
         }
      }

      size = 8;

      // Poll events
      if (IDirectInputDevice8_GetDeviceData(paddevice[i].lpDIDevice,
          sizeof(DIDEVICEOBJECTDATA), didod, &size, 0) != DI_OK)
      {
         if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
         {
            // Make sure device is acquired
            while(IDirectInputDevice8_Acquire(paddevice[i].lpDIDevice) == DIERR_INPUTLOST) {}
            continue;
         }
      }

      if (size == 0)
         continue;

      switch (paddevice[i].type)
      {
         case TYPE_KEYBOARD:
            // This probably could be optimized
            for (i2 = 0; i2 < size; i2++)
            {
               if (didod[i2].dwData & 0x80)
                  PerKeyDown(didod[i2].dwOfs);
               else
                  PerKeyUp(didod[i2].dwOfs);
            }
            break;
         case TYPE_JOYSTICK:
         {
            // This probably could be optimized
            for (i2 = 0; i2 < size; i2++)
            {
               // X Axis
               if (didod[i2].dwOfs == 0)
               {
                  if (didod[i2].dwData < 0x3FFF)
                  {
                     PerKeyDown(PAD_DIR_AXISLEFT);
                     PerKeyUp(PAD_DIR_AXISRIGHT);
                  }
                  else if (didod[i2].dwData > 0xBFFF)
                  {
                     PerKeyDown(PAD_DIR_AXISRIGHT);
                     PerKeyUp(PAD_DIR_AXISLEFT);
                  }
                  else
                  {
                     PerKeyUp(PAD_DIR_AXISLEFT);
                     PerKeyUp(PAD_DIR_AXISRIGHT);
                  }
               }
               // Y Axis
               else if (didod[i2].dwOfs == 4)
               {
                  if (didod[i2].dwData < 0x3FFF)
                  {
                     PerKeyDown(PAD_DIR_AXISUP);
                     PerKeyUp(PAD_DIR_AXISDOWN);
                  }
                  else if (didod[i2].dwData > 0xBFFF)
                  {
                     PerKeyDown(PAD_DIR_AXISDOWN);
                     PerKeyUp(PAD_DIR_AXISUP);
                  }
                  else
                  {
                     PerKeyUp(PAD_DIR_AXISUP);
                     PerKeyUp(PAD_DIR_AXISDOWN);
                  }
               }
               else if (didod[i2].dwOfs == 0x20)
               {
                  // POV Center
                  if (LOWORD(didod[i2].dwData) == 0xFFFF)
                  {
                     PerKeyUp(PAD_DIR_POVUP);
                     PerKeyUp(PAD_DIR_POVRIGHT);
                     PerKeyUp(PAD_DIR_POVDOWN);
                     PerKeyUp(PAD_DIR_POVLEFT);
                  }
                  // POV Up
                  else if (didod[i2].dwData < 4500)
                  {
                     PerKeyDown(PAD_DIR_POVUP);
                     PerKeyUp(PAD_DIR_POVRIGHT);
                     PerKeyUp(PAD_DIR_POVLEFT);
                  }
                  // POV Up-right
                  else if (didod[i2].dwData < 9000)
                  {
                     PerKeyDown(PAD_DIR_POVUP);
                     PerKeyDown(PAD_DIR_POVRIGHT);
                  }
                  // POV Right
                  else if (didod[i2].dwData < 13500)
                  {
                     PerKeyDown(PAD_DIR_POVRIGHT);
                     PerKeyUp(PAD_DIR_POVDOWN);
                     PerKeyUp(PAD_DIR_POVUP);
                  }
                  // POV Right-down
                  else if (didod[i2].dwData < 18000)
                  {
                     PerKeyDown(PAD_DIR_POVRIGHT);
                     PerKeyDown(PAD_DIR_POVDOWN);
                  }
                  // POV Down
                  else if (didod[i2].dwData < 22500)
                  {
                     PerKeyDown(PAD_DIR_POVDOWN);
                     PerKeyUp(PAD_DIR_POVLEFT);
                     PerKeyUp(PAD_DIR_POVRIGHT);
                  }
                  // POV Down-left
                  else if (didod[i2].dwData < 27000)
                  {
                     PerKeyDown(PAD_DIR_POVDOWN);
                     PerKeyDown(PAD_DIR_POVLEFT);
                  }
                  // POV Left
                  else if (didod[i2].dwData < 31500)
                  {
                     PerKeyDown(PAD_DIR_POVLEFT);
                     PerKeyUp(PAD_DIR_POVUP);
                     PerKeyUp(PAD_DIR_POVDOWN);
                  }
                  // POV Left-up
                  else if (didod[i2].dwData < 36000)
                  {
                     PerKeyDown(PAD_DIR_POVLEFT);
                     PerKeyDown(PAD_DIR_POVUP);
                  }
               }
               else if (didod[i2].dwOfs >= 0x30 && didod[i2].dwOfs <= 0xFF)
               {
                  if (didod[i2].dwData & 0x80)
                     PerKeyDown(didod[i2].dwOfs);
                  else
                     PerKeyUp(didod[i2].dwOfs);
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
      SendMessage((HWND)pvRef, CB_ADDSTRING, 0, (LPARAM)lpddi->tszInstanceName);
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
   SendMessage(control, CB_ADDSTRING, 0, (LPARAM)"None");

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
   u32 i;
   int idlist[] = { IDC_UPTEXT, IDC_DOWNTEXT, IDC_LEFTTEXT, IDC_RIGHTTEXT,
                    IDC_ATEXT, IDC_BTEXT, IDC_CTEXT,
                    IDC_XTEXT, IDC_YTEXT, IDC_ZTEXT,
                    IDC_LTEXT, IDC_RTEXT, IDC_STARTTEXT
                  };


   sprintf(string1, "Peripheral%d", padnum+1);

   // Let's first fetch the guid of the device and see if we can get a match
   if (GetPrivateProfileString(string1, "GUID", "", tempstr, MAX_PATH, inifilename) == 0)
   {
      if (padnum == 0)
      {
         // Let's use default values
         SendDlgItemMessage(hWnd, IDC_DXDEVICECB, CB_SETCURSEL, 1, 0);

         controlmap[0] = DIK_UP;
         controlmap[1] = DIK_DOWN;
         controlmap[2] = DIK_LEFT;
         controlmap[3] = DIK_RIGHT;
         controlmap[4] = DIK_K;
         controlmap[5] = DIK_L;
         controlmap[6] = DIK_M;
         controlmap[7] = DIK_U;
         controlmap[8] = DIK_I;
         controlmap[9] = DIK_O;
         controlmap[10] = DIK_X;
         controlmap[11] = DIK_Z;
         controlmap[12] = DIK_J;
         for (i = 0; i < 13; i++)
         {
            ConvertKBIDToName(controlmap[i], tempstr);
            SetDlgItemText(hWnd, idlist[i], tempstr);
         }
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

      StringToGUID(tempstr, &guid);

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

      if (IDirectInput8_CreateDevice(lpDI8temp, &GUIDDevice[i], &lpDIDevicetemp,
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

         for (i = 0; i < 13; i++)
         {
            buttonid = GetPrivateProfileInt(string1, pad_names2[i], 0, inifilename);
            controlmap[i] = buttonid;
            ConvertKBIDToName(buttonid, tempstr);
            SetDlgItemText(hWnd, idlist[i], tempstr);
         }
      }       
      else if (GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_GAMEPAD ||
              GET_DIDEVICE_TYPE(didc.dwDevType) == DI8DEVTYPE_JOYSTICK)
      {
         sprintf(string1, "Peripheral%d", padnum+1);

         for (i = 0; i < 13; i++)
         {
            buttonid = GetPrivateProfileInt(string1, pad_names2[i], 0, inifilename);
            controlmap[i] = buttonid;
            ConvertJoyIDToName(buttonid, tempstr);
            SetDlgItemText(hWnd, idlist[i], tempstr);
         }
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

   if (DialogBoxParam(y_hInstance, MAKEINTRESOURCE(IDD_BUTTONCONFIG), hWnd, (DLGPROC)ButtonConfigDlgProc, (LPARAM)lpDIDevicetemp) == TRUE)
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
   DWORD i;
   DIDEVCAPS didc;

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
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

