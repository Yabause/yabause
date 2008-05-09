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

#ifndef PERDX_H
#define PERDX_H

#define DIRECTINPUT_VERSION 0x0800
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


#define PERCORE_DIRECTX 2

extern PerInterface_struct PERDIRECTX;

extern GUID GUIDDevice[256];
extern u32 numguids;
extern const int pad_names[];
extern const char * pad_names2[];
extern PerPad_struct *pad[12];
extern u32 numpads;

typedef struct
{
   LPDIRECTINPUTDEVICE8 lpDIDevice;
   int type;
   int emulatetype;
} padconf_struct;

extern padconf_struct paddevice[12];

LRESULT CALLBACK ButtonConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);

void KeyStub(PerPad_struct *pad);
void SetupControlUpDown(u8 padnum, u8 controlcode, void (*downfunc)(PerPad_struct *), void (*upfunc)(PerPad_struct *));

void PERDXLoadDevices(char *inifilename);
void PERDXListDevices(HWND control);
int PERDXInitControlConfig(HWND hWnd, u8 padnum, int *controlmap, const char *inifilename);
int PERDXFetchNextPress(HWND hWnd, u32 guidnum, char *buttonname);
BOOL PERDXWriteGUID(u32 guidnum, u8 padnum, LPCTSTR inifilename);
#endif
