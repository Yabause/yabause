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

#define PERCORE_DIRECTX 2

extern PerInterface_struct PERDIRECTX;

extern GUID GUIDDevice[256];
extern u32 numguids;

LRESULT CALLBACK ButtonConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);

void KeyStub(void);
void SetupControlUpDown(u8 padnum, u8 controlcode, void (*downfunc)(), void (*upfunc)());

void PERDXLoadDevices(char *inifilename);
void PERDXListDevices(HWND control);
int PERDXInitControlConfig(HWND hWnd, u8 padnum, int *controlmap, const char *inifilename);
BOOL PERDXWriteGUID(u32 guidnum, u8 padnum, LPCTSTR inifilename);
#endif
