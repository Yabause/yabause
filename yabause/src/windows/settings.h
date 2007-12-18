/*  Copyright 2004-2005 Theo Berkau

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

typedef struct
{
   char *string;
   HWND hWnd;
   HWND hParent;
} helpballoon_struct;

LRESULT CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam);

LRESULT CALLBACK BackupRamDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);

extern BOOL IsPathCdrom(const char *path);

int CreateHelpBalloons(helpballoon_struct *hb[]);

extern char biosfilename[MAX_PATH];
extern char cdrompath[MAX_PATH];
extern char backupramfilename[MAX_PATH];
extern char mpegromfilename[MAX_PATH];
extern char cartfilename[MAX_PATH];
extern char inifilename[MAX_PATH];
extern char logfilename[MAX_PATH];

extern char bioslang;
extern char sh2coretype;
extern char vidcoretype;
extern char sndcoretype;
extern int sndvolume;
extern int enableautofskip;
extern int usefullscreenonstartup;
extern int fullscreenwidth;
extern int fullscreenheight;
extern int usecustomwindowsize;
extern int windowwidth;
extern int windowheight;
extern char percoretype;
extern u8 regionid;
extern int disctype;
extern int carttype;
extern DWORD netlinklocalremoteip;
extern int netlinkport;
extern int uselog;
extern int logtype;
extern int nocorechange;

