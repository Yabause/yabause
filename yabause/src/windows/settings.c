/*  Copyright 2004-2009 Theo Berkau

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <ctype.h>
#undef FASTCALL
#include "../bios.h"
#include "../cs0.h"
#include "../cs2.h"
#include "../peripheral.h"
#include "../scsp.h"
#include "../vdp1.h"
#include "../vdp2.h"
#include "cd.h"
#include "resource.h"
#include "settings.h"
#include "snddx.h"
#include "perdx.h"
#include "../vidogl.h"

char biosfilename[MAX_PATH] = "\0";
char cdrompath[MAX_PATH]="\0";
char backupramfilename[MAX_PATH] = "bkram.bin\0";
char mpegromfilename[MAX_PATH] = "\0";
char cartfilename[MAX_PATH] = "\0";
char inifilename[MAX_PATH];
char logfilename[MAX_PATH];
char mini18nlogfilename[MAX_PATH];

int num_cdroms=0;
char drive_list[24];
char bioslang=0;
char sh2coretype=0;
char vidcoretype=VIDCORE_OGL;
char sndcoretype=SNDCORE_DIRECTX;
char percoretype=PERCORE_DIRECTX;
int sndvolume=100;
int enableautofskip=0;
int usefullscreenonstartup=0;
int fullscreenwidth=640;
int fullscreenheight=480;
int usecustomwindowsize=0;
int windowwidth=320;
int windowheight=224;
u8 regionid=0;
int disctype;
int carttype;
DWORD netlinklocalremoteip=MAKEIPADDRESS(127, 0, 0, 1);
int netlinkport=7845;
int uselog=0;
int usemini18nlog=0;
int logtype=0;
int nocorechange = 0;
#ifdef USETHREADS
int changecore = 0;
int corechanged = 0;
#endif

extern HINSTANCE y_hInstance;
extern VideoInterface_struct *VIDCoreList[];
extern PerInterface_struct *PERCoreList[];
extern SoundInterface_struct *SNDCoreList[];

LRESULT CALLBACK BasicSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam);

LRESULT CALLBACK VideoSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam);

LRESULT CALLBACK SoundSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam);

LRESULT CALLBACK NetlinkSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                        LPARAM lParam);

LRESULT CALLBACK InputSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam);

LRESULT CALLBACK PadConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);

LRESULT CALLBACK MouseConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);

LRESULT CALLBACK LogSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);

//////////////////////////////////////////////////////////////////////////////

void GenerateCDROMList(HWND hWnd)
{
   int i=0;
   char tempstr[8];

   num_cdroms=0;

   // Go through drives C-Z and only add only cdrom drives to drive letter
   // list

   for (i = 0; i < 24; i++)
   {
      sprintf(tempstr, "%c:\\", 'c' + i);

      if (GetDriveTypeA(tempstr) == DRIVE_CDROM)
      {
         drive_list[num_cdroms] = 'c' + i;

         sprintf(tempstr, "%c", 'C' + i);
         SendDlgItemMessage(hWnd, IDC_DRIVELETTERCB, CB_ADDSTRING, 0, (LPARAM)_16(tempstr));
         num_cdroms++;
      } 
   }
}

//////////////////////////////////////////////////////////////////////////////

BOOL IsPathCdrom(const char *path)
{
   if (GetDriveTypeA(cdrompath) == DRIVE_CDROM)
      return TRUE;
   else
      return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

#define MAX_SETTINGS_DIALOGS    6

HWND dialoglist[MAX_SETTINGS_DIALOGS];

LRESULT CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         // Setup Tabs
         TCITEM tabitem; 
         RECT rect;
         int i;

         tabitem.mask = TCIF_TEXT | TCIF_IMAGE;
         tabitem.iImage = -1;

         i = 0;

         // Setup Tabs and create all the dialogs
         tabitem.pszText = _16("Basic");
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_BASICSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)BasicSettingsDlgProc);
         i++;

         tabitem.pszText = _16("Video");
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_VIDEOSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)VideoSettingsDlgProc);
         i++;

         tabitem.pszText = _16("Sound");
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_SOUNDSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)SoundSettingsDlgProc);
         i++;

         tabitem.pszText = _16("Input");
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_INPUTSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)InputSettingsDlgProc);
         i++;

#ifdef USESOCKET
         tabitem.pszText = _16("Netlink");
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_NETLINKSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)NetlinkSettingsDlgProc);
         i++;
#endif

#if DEBUG
         tabitem.pszText = _16("Log");
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_LOGSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)LogSettingsDlgProc);
         i++;
#endif

         // Setup Tabs
         GetClientRect(GetDlgItem(hDlg, IDC_SETTINGSTAB), &rect);
         TabCtrl_AdjustRect(GetDlgItem(hDlg, IDC_SETTINGSTAB), FALSE,
                            &rect);

         // Adjust the size of and hide all the dialogs
         for (i = 0; i < MAX_SETTINGS_DIALOGS; i++)
         {
            MoveWindow(dialoglist[i], rect.left, rect.top,
                       rect.right - rect.left, rect.bottom - rect.top,
                       TRUE);
            ShowWindow(dialoglist[i], SW_HIDE);
         }

         // Set only the first one as visible
         SetWindowPos(dialoglist[0], HWND_TOP, 0, 0, 0, 0,
                      SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
               int i;

               for (i = 0; i < MAX_SETTINGS_DIALOGS; i++)
               {
                  SendMessage(dialoglist[i], WM_COMMAND, IDOK, 0);
                  dialoglist[i] = NULL;
               }

               EndDialog(hDlg, TRUE);
               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            default: break;
         }

         break;
      }
      case WM_NOTIFY:
      {
         switch (LOWORD(wParam))
         {
            case IDC_SETTINGSTAB:
            {               
               LPNMHDR lpnmhdr = (LPNMHDR)lParam;

               if (lpnmhdr->code == TCN_SELCHANGING)
               {
                  // Hide old dialog
                  int cursel = TabCtrl_GetCurSel(GetDlgItem(hDlg, IDC_SETTINGSTAB));
                  ShowWindow(dialoglist[cursel],SW_HIDE);
                  return TRUE;
               }
               else if (lpnmhdr->code == TCN_SELCHANGE)
               {
                  // Show the new current dialog
                  int cursel = TabCtrl_GetCurSel(GetDlgItem(hDlg, IDC_SETTINGSTAB));
                  ShowWindow(dialoglist[cursel],SW_SHOWNORMAL);
                  return TRUE;
               }
               break;
            }
            default: break;
         }
         break;
      }
      case WM_DESTROY:
      {
         int i;

         for (i = 0; i < MAX_SETTINGS_DIALOGS; i++)
         {
            if (dialoglist[i])
               DestroyWindow(dialoglist[i]);
         }
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK BasicSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam)
{
   static helpballoon_struct hb[9];

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         BOOL imagebool=FALSE;
         char current_drive=0;
         int i;

         // Setup Combo Boxes

         // Disc Type Box
         SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("CD"));
         SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("Image"));

         // Drive Letter Box
         SendDlgItemMessage(hDlg, IDC_DRIVELETTERCB, CB_RESETCONTENT, 0, 0);

         // figure out which drive letters are available
         GenerateCDROMList(hDlg);

         // Set Disc Type Selection
         if (IsPathCdrom(cdrompath))
         {
            current_drive = cdrompath[0];
            imagebool = FALSE;
         }
         else
         {
            // Assume it's a file
            current_drive = 0;
            imagebool = TRUE;
         }

         if (current_drive != 0)
         {
            for (i = 0; i < num_cdroms; i++)
            {
               if (toupper(current_drive) == toupper(drive_list[i]))
               {
                  SendDlgItemMessage(hDlg, IDC_DRIVELETTERCB, CB_SETCURSEL, i, 0);
               }
            }
         }
         else
         {
            // set it to the first drive
            SendDlgItemMessage(hDlg, IDC_DRIVELETTERCB, CB_SETCURSEL, 0, 0);
         }

         // disable/enable menu options depending on whether or not 
         // image is selected
         if (imagebool == FALSE)
         {
            SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_SETCURSEL, 0, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_IMAGEEDIT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_IMAGEBROWSE), FALSE);

            EnableWindow(GetDlgItem(hDlg, IDC_DRIVELETTERCB), TRUE);
         }
         else
         {
            SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_SETCURSEL, 1, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_IMAGEEDIT), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_IMAGEBROWSE), TRUE);
            SetDlgItemText(hDlg, IDC_IMAGEEDIT, _16(cdrompath));

            EnableWindow(GetDlgItem(hDlg, IDC_DRIVELETTERCB), FALSE);
         }

/*
         // Setup Bios Language Combo box
         SendDlgItemMessage(hDlg, IDC_BIOSLANGCB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_BIOSLANGCB, CB_ADDSTRING, 0, (long)"English");
         SendDlgItemMessage(hDlg, IDC_BIOSLANGCB, CB_ADDSTRING, 0, (long)"German");
         SendDlgItemMessage(hDlg, IDC_BIOSLANGCB, CB_ADDSTRING, 0, (long)"French");
         SendDlgItemMessage(hDlg, IDC_BIOSLANGCB, CB_ADDSTRING, 0, (long)"Spanish");
         SendDlgItemMessage(hDlg, IDC_BIOSLANGCB, CB_ADDSTRING, 0, (long)"Italian");
         SendDlgItemMessage(hDlg, IDC_BIOSLANGCB, CB_ADDSTRING, 0, (long)"Japanese");

         // Set Selected Bios Language
         SendDlgItemMessage(hDlg, IDC_BIOSLANGCB, CB_SETCURSEL, bioslang, 0);

         // Since it's not fully working, let's disable it
         EnableWindow(GetDlgItem(hDlg, IDC_BIOSLANGCB), FALSE);
*/

         // Setup SH2 Core Combo box
         SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_ADDSTRING, 0, (LPARAM)_16("Fast Interpreter"));
         SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_ADDSTRING, 0, (LPARAM)_16("Debug Interpreter"));

         // Set Selected SH2 Core
         SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_SETCURSEL, sh2coretype, 0);

         // Setup Region Combo box
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)_16("Auto-detect"));
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)_16("Japan(NTSC)"));
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)_16("Asia(NTSC)"));
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)_16("North America(NTSC)"));
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)_16("Central/South America(NTSC)"));
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)_16("Korea(NTSC)"));
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)_16("Asia(PAL)"));
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)_16("Europe + others(PAL)"));
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)_16("Central/South America(PAL)"));

         // Set Selected Region
         switch(regionid)
         {
            case 0:
            case 1:
            case 2:
               SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_SETCURSEL, regionid, 0);
               break;
            case 4:
               SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_SETCURSEL, 3, 0);
               break;
            case 5:
               SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_SETCURSEL, 4, 0);
               break;
            case 6:
               SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_SETCURSEL, 5, 0);
               break;
            case 0xA:
               SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_SETCURSEL, 6, 0);
               break;
            case 0xC:
               SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_SETCURSEL, 7, 0);
               break;
            case 0xD:
               SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_SETCURSEL, 8, 0);
               break;
            default:
               SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_SETCURSEL, 0, 0);
               break;
         }

         // Set Default Bios ROM File
         SetDlgItemText(hDlg, IDC_BIOSEDIT, _16(biosfilename));

         // Set Default Backup RAM File
         SetDlgItemText(hDlg, IDC_BACKUPRAMEDIT, _16(backupramfilename));

         // Set Default MPEG ROM File
         SetDlgItemText(hDlg, IDC_MPEGROMEDIT, _16(mpegromfilename));

         // Setup Cart Type Combo box
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("None"));
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("Pro Action Replay"));
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("4 Mbit Backup Ram"));
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("8 Mbit Backup Ram"));
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("16 Mbit Backup Ram"));
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("32 Mbit Backup Ram"));
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("8 Mbit Dram"));
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("32 Mbit Dram"));
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("Netlink"));
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("16 Mbit Rom"));
//         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"Japanese Modem");

         // Set Selected Cart Type
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_SETCURSEL, carttype, 0);

         // Set Default Cart File
         SetDlgItemText(hDlg, IDC_CARTEDIT, _16(cartfilename));

         // Set Cart File window status
         switch (carttype)
         {
            case CART_NONE:
            case CART_DRAM8MBIT:
            case CART_DRAM32MBIT:
            case CART_NETLINK:
            case CART_JAPMODEM:
               EnableWindow(GetDlgItem(hDlg, IDC_CARTEDIT), FALSE);
               EnableWindow(GetDlgItem(hDlg, IDC_CARTBROWSE), FALSE);
               break;
            case CART_PAR:
            case CART_BACKUPRAM4MBIT:
            case CART_BACKUPRAM8MBIT:
            case CART_BACKUPRAM16MBIT:
            case CART_BACKUPRAM32MBIT:
            case CART_ROM16MBIT:
               EnableWindow(GetDlgItem(hDlg, IDC_CARTEDIT), TRUE);
               EnableWindow(GetDlgItem(hDlg, IDC_CARTBROWSE), TRUE);
               break;
            default: break;
         }

         // Setup Tooltips
         hb[0].string = "Select whether to use a cdrom or a disc image";
         hb[0].hParent = GetDlgItem(hDlg, IDC_DISCTYPECB);
         hb[1].string = "Use this to select the SH2 emulation method. If in doubt, leave it as 'Fast Interpreter'";
         hb[1].hParent = GetDlgItem(hDlg, IDC_SH2CORECB);
         hb[2].string = "Use this to select the region of the CD. Normally it's best to leave it as 'Auto-detect'";
         hb[2].hParent = GetDlgItem(hDlg, IDC_REGIONCB);
         hb[3].string = "This is where you put the path to a Saturn bios rom image. If you don't have one, just leave it blank";
         hb[3].hParent = GetDlgItem(hDlg, IDC_BIOSEDIT);
         hb[4].string = "This is where you put the path to internal backup ram file. This holds all your saves.";
         hb[4].hParent = GetDlgItem(hDlg, IDC_BACKUPRAMEDIT);
         hb[5].string = "If you don't know what this is, just leave it blank.";
         hb[5].hParent = GetDlgItem(hDlg, IDC_MPEGROMEDIT);  
         hb[6].string = "Use this to select what kind of cartridge to emulate.  If in doubt, leave it as 'None'";
         hb[6].hParent = GetDlgItem(hDlg, IDC_CARTTYPECB);
         hb[7].string = "This is where you put the path to the file used by the emulated cartridge. The kind of file that goes here depends on what Cartridge Type is set to";
         hb[7].hParent = GetDlgItem(hDlg, IDC_CARTEDIT);
         hb[8].string = NULL;

         CreateHelpBalloons(hb);

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_DISCTYPECB:
            {
               switch(HIWORD(wParam))
               {
                  case CBN_SELCHANGE:
                  {
                     u8 cursel=0;

                     cursel = (u8)SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_GETCURSEL, 0, 0);

                     if (cursel == 0)
                     {
                        EnableWindow(GetDlgItem(hDlg, IDC_IMAGEEDIT), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_IMAGEBROWSE), FALSE);

                        EnableWindow(GetDlgItem(hDlg, IDC_DRIVELETTERCB), TRUE);
                     }
                     else
                     {
                        EnableWindow(GetDlgItem(hDlg, IDC_IMAGEEDIT), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_IMAGEBROWSE), TRUE);

                        EnableWindow(GetDlgItem(hDlg, IDC_DRIVELETTERCB), FALSE);
                     }

                     return TRUE;
                  }
                  default: break;
               }

               return TRUE;
            }
            case IDC_CARTTYPECB:
            {
               switch(HIWORD(wParam))
               {
                  case CBN_SELCHANGE:
                  {
                     u8 cursel=0;

                     cursel = (u8)SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_GETCURSEL, 0, 0);

                     switch (cursel)
                     {
                        case CART_NONE:
                        case CART_DRAM8MBIT:
                        case CART_DRAM32MBIT:
                        case CART_NETLINK:
                           EnableWindow(GetDlgItem(hDlg, IDC_CARTEDIT), FALSE);
                           EnableWindow(GetDlgItem(hDlg, IDC_CARTBROWSE), FALSE);
                           break;
                        case CART_PAR:
                        case CART_BACKUPRAM4MBIT:
                        case CART_BACKUPRAM8MBIT:
                        case CART_BACKUPRAM16MBIT:
                        case CART_BACKUPRAM32MBIT:
                        case CART_ROM16MBIT:
                           EnableWindow(GetDlgItem(hDlg, IDC_CARTEDIT), TRUE);
                           EnableWindow(GetDlgItem(hDlg, IDC_CARTBROWSE), TRUE);
                           break;
                        default: break;
                     }

                     return TRUE;
                  }
                  default: break;
               }

               return TRUE;
            }
            case IDC_IMAGEBROWSE:
            {
               WCHAR tempwstr[MAX_PATH];
               WCHAR filter[1024];
               OPENFILENAME ofn;

               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;

               CreateFilter(filter, 1024,
                  "Supported image files (*.cue, *.iso)", "*.cue;*.iso",
                  "Cue files (*.cue)", "*.cue",
                  "Iso files (*.iso)", "*.iso",
                  "All files (*.*)", "*.*", NULL);

               ofn.lpstrFilter = filter;
               GetDlgItemText(hDlg, IDC_IMAGEEDIT, tempwstr, MAX_PATH);
               ofn.lpstrFile = tempwstr;
               ofn.nMaxFile = sizeof(tempwstr);
               ofn.Flags = OFN_FILEMUSTEXIST;

               if (GetOpenFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_IMAGEEDIT, tempwstr);
               }

               return TRUE;
            }
            case IDC_BIOSBROWSE:
            {
               WCHAR tempwstr[MAX_PATH];
               WCHAR filter[1024];
               OPENFILENAME ofn;
               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;

               CreateFilter(filter, 1024,
                  "Binaries (*.bin)", "*.bin",
                  "All Files", "*.*", NULL);

               ofn.lpstrFilter = filter;
               GetDlgItemText(hDlg, IDC_BIOSEDIT, tempwstr, MAX_PATH);
               ofn.lpstrFile = tempwstr;
               ofn.nMaxFile = sizeof(tempwstr);
               ofn.Flags = OFN_FILEMUSTEXIST;

               if (GetOpenFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_BIOSEDIT, tempwstr);
                  WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, biosfilename, MAX_PATH, NULL, NULL);
               }

               return TRUE;
            }
            case IDC_BACKUPRAMBROWSE:
            {
               WCHAR tempwstr[MAX_PATH];
               WCHAR filter[1024];
               OPENFILENAME ofn;
               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;

               CreateFilter(filter, 1024,
                  "Binaries (*.bin)", "*.bin",
                  "All Files", "*.*", NULL);

               ofn.lpstrFilter = filter;
               GetDlgItemText(hDlg, IDC_BACKUPRAMEDIT, tempwstr, MAX_PATH);
               ofn.lpstrFile = tempwstr;
               ofn.nMaxFile = sizeof(tempwstr);

               if (GetOpenFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_BACKUPRAMEDIT, tempwstr);
                  WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, backupramfilename, MAX_PATH, NULL, NULL);
               }

               return TRUE;
            }
            case IDC_MPEGROMBROWSE:
            {
               WCHAR tempwstr[MAX_PATH];
               WCHAR filter[1024];
               OPENFILENAME ofn;
               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;

               CreateFilter(filter, 1024,
                  "Binaries (*.bin)", "*.bin",
                  "All Files", "*.*", NULL);

               ofn.lpstrFilter = filter;
               GetDlgItemText(hDlg, IDC_MPEGROMEDIT, tempwstr, MAX_PATH);
               ofn.lpstrFile = tempwstr;
               ofn.nMaxFile = sizeof(tempwstr);
               ofn.Flags = OFN_FILEMUSTEXIST;

               if (GetOpenFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_MPEGROMEDIT, tempwstr);
                  WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, mpegromfilename, MAX_PATH, NULL, NULL);
               }

               return TRUE;
            }
            case IDC_CARTBROWSE:
            {
               WCHAR tempwstr[MAX_PATH];
               WCHAR filter[1024];
               OPENFILENAME ofn;
               u8 cursel=0;

               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;

               CreateFilter(filter, 1024,
                  "Binaries (*.bin)", "*.bin",
                  "All Files", "*.*", NULL);

               ofn.lpstrFilter = filter;
               GetDlgItemText(hDlg, IDC_CARTEDIT, tempwstr, MAX_PATH);
               ofn.lpstrFile = tempwstr;
               ofn.nMaxFile = sizeof(tempwstr);

               cursel = (u8)SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_GETCURSEL, 0, 0);

               switch (cursel)
               {
                  case CART_PAR:
                  case CART_ROM16MBIT:
                     ofn.Flags = OFN_FILEMUSTEXIST;
                     break;
                  default: break;
               }

               if (GetOpenFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_CARTEDIT, tempwstr);
                  WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, cartfilename, MAX_PATH, NULL, NULL);
               }

               return TRUE;
            }
            case IDOK:
            {
               WCHAR tempwstr[MAX_PATH];
               char tempstr[MAX_PATH];
               char current_drive=0;
               BOOL imagebool;
               BOOL cdromchanged=FALSE;

               // Convert Dialog items back to variables
               GetDlgItemText(hDlg, IDC_BIOSEDIT, tempwstr, MAX_PATH);
               WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, biosfilename, MAX_PATH, NULL, NULL);
               GetDlgItemText(hDlg, IDC_BACKUPRAMEDIT, tempwstr, MAX_PATH);
               WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, backupramfilename, MAX_PATH, NULL, NULL);
               GetDlgItemText(hDlg, IDC_MPEGROMEDIT, tempwstr, MAX_PATH);
               WideCharToMultiByte(CP_ACP, 0, tempwstr, -1,  mpegromfilename, MAX_PATH, NULL, NULL);
               carttype = (int)SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_GETCURSEL, 0, 0);
               GetDlgItemText(hDlg, IDC_CARTEDIT, tempwstr, MAX_PATH);
               WideCharToMultiByte(CP_ACP, 0, tempwstr, -1,  cartfilename, MAX_PATH, NULL, NULL);

               // write path/filenames
               WritePrivateProfileStringA("General", "BiosPath", biosfilename, inifilename);
               WritePrivateProfileStringA("General", "BackupRamPath", backupramfilename, inifilename);
               WritePrivateProfileStringA("General", "MpegRomPath", mpegromfilename, inifilename);

               sprintf(tempstr, "%d", carttype);
               WritePrivateProfileStringA("General", "CartType", tempstr, inifilename);

               // figure out cart type, write cartfilename if necessary
               switch (carttype)
               {
                  case CART_PAR:
                  case CART_BACKUPRAM4MBIT:
                  case CART_BACKUPRAM8MBIT:
                  case CART_BACKUPRAM16MBIT:
                  case CART_BACKUPRAM32MBIT:
                  case CART_ROM16MBIT:
                     WritePrivateProfileStringA("General", "CartPath", cartfilename, inifilename);
                     break;
                  default: break;
               }

               imagebool = (BOOL)SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_GETCURSEL, 0, 0);

               if (imagebool == FALSE)
               {
                  // convert drive letter to string
                  current_drive = (char)SendDlgItemMessage(hDlg, IDC_DRIVELETTERCB, CB_GETCURSEL, 0, 0);
                  sprintf(tempstr, "%c:", toupper(drive_list[(int)current_drive]));

                  if (strcmp(tempstr, cdrompath) != 0)
                  {
                     strcpy(cdrompath, tempstr);
                     cdromchanged = TRUE;
                  }
               }
               else
               {
                  // retrieve image filename string instead
                  GetDlgItemText(hDlg, IDC_IMAGEEDIT, tempwstr, MAX_PATH);
                  WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, tempstr, MAX_PATH, NULL, NULL);

                  if (strcmp(tempstr, cdrompath) != 0)
                  {
                     strcpy(cdrompath, tempstr);
                     cdromchanged = TRUE;
                  }
               }

               WritePrivateProfileStringA("General", "CDROMDrive", cdrompath, inifilename);

/*
               // Convert ID to language string
               bioslang = (char)SendDlgItemMessage(hDlg, IDC_BIOSLANGCB, CB_GETCURSEL, 0, 0);

               switch (bioslang)
               {
                  case 0:
                     sprintf(tempstr, "English");
                     break;
                  case 1:
                     sprintf(tempstr, "German");
                     break;
                  case 2:
                     sprintf(tempstr, "French");
                     break;
                  case 3:
                     sprintf(tempstr, "Spanish");
                     break;
                  case 4:
                     sprintf(tempstr, "Italian");
                     break;
                  case 5:
                     sprintf(tempstr, "Japanese");
                     break;
                  default:break;
               }

//               WritePrivateProfileStringA("General", "BiosLanguage", tempstr, inifilename);
*/
               // Write SH2 core type
               sh2coretype = (char)SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_GETCURSEL, 0, 0);

               sprintf(tempstr, "%d", sh2coretype);
               WritePrivateProfileStringA("General", "SH2Core", tempstr, inifilename);

               // Convert Combo Box ID to Region ID
               regionid = (char)SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_GETCURSEL, 0, 0);

               switch(regionid)
               {
                  case 0:
                     WritePrivateProfileStringA("General", "Region", "Auto", inifilename);
                     break;
                  case 1:
                     WritePrivateProfileStringA("General", "Region", "J", inifilename);
                     break;
                  case 2:
                     WritePrivateProfileStringA("General", "Region", "T", inifilename);
                     break;
                  case 3:
                     WritePrivateProfileStringA("General", "Region", "U", inifilename);
                     regionid = 4;
                     break;
                  case 4:
                     WritePrivateProfileStringA("General", "Region", "B", inifilename);
                     regionid = 5;
                     break;
                  case 5:
                     WritePrivateProfileStringA("General", "Region", "K", inifilename);
                     regionid = 6;
                     break;
                  case 6:
                     WritePrivateProfileStringA("General", "Region", "A", inifilename);
                     regionid = 0xA;
                     break;
                  case 7:
                     WritePrivateProfileStringA("General", "Region", "E", inifilename);
                     regionid = 0xC;
                     break;
                  case 8:
                     WritePrivateProfileStringA("General", "Region", "L", inifilename);
                     regionid = 0xD;
                     break;
                  default:
                     regionid = 0;
                     break;
               }

               if (cdromchanged && nocorechange == 0)
               {
#ifndef USETHREADS
                  if (IsPathCdrom(cdrompath))
                     Cs2ChangeCDCore(CDCORE_SPTI, cdrompath);
                  else
                     Cs2ChangeCDCore(CDCORE_ISO, cdrompath);
#else
                  corechanged = 0;
                  changecore |= 1;
                  while (corechanged == 0) { Sleep(0); }
#endif
               }

               EndDialog(hDlg, TRUE);
               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            default: break;
         }

         break;
      }
      case WM_DESTROY:
      {
         DestroyHelpBalloons(hb);
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK VideoSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         char tempstr[MAX_PATH];
         DEVMODE dmSettings;
         int i;

         // Setup Video Core Combo box
         SendDlgItemMessage(hDlg, IDC_VIDEOCORECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_VIDEOCORECB, CB_ADDSTRING, 0, (LPARAM)_16("None"));

         for (i = 1; VIDCoreList[i] != NULL; i++)
            SendDlgItemMessage(hDlg, IDC_VIDEOCORECB, CB_ADDSTRING, 0, (LPARAM)_16(VIDCoreList[i]->Name));

         // Set Selected Video Core
         for (i = 0; VIDCoreList[i] != NULL; i++)
         {
            if (vidcoretype == VIDCoreList[i]->id)
               SendDlgItemMessage(hDlg, IDC_VIDEOCORECB, CB_SETCURSEL, i, 0);
         }

         // Setup Auto Frameskip checkbox
         if (enableautofskip)
            SendDlgItemMessage(hDlg, IDC_AUTOFRAMESKIPCB, BM_SETCHECK, BST_CHECKED, 0);
         else
            SendDlgItemMessage(hDlg, IDC_AUTOFRAMESKIPCB, BM_SETCHECK, BST_UNCHECKED, 0);

         // Setup Fullscreen on Startup checkbox
         if (usefullscreenonstartup)
            SendDlgItemMessage(hDlg, IDC_FULLSCREENSTARTUPCB, BM_SETCHECK, BST_CHECKED, 0);
         else
            SendDlgItemMessage(hDlg, IDC_FULLSCREENSTARTUPCB, BM_SETCHECK, BST_UNCHECKED, 0);

         // Setup FullScreen width/height settings
         SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_RESETCONTENT, 0, 0);

         for (i = 0;; i++)
         {
            if (EnumDisplaySettings(NULL, i, &dmSettings) == FALSE)
               break;
            if (dmSettings.dmBitsPerPel == 32)
            {
               int index;

               sprintf(tempstr, "%dx%d", (int)dmSettings.dmPelsWidth, (int)dmSettings.dmPelsHeight);
               if (SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_FINDSTRINGEXACT, 0, (LPARAM)_16(tempstr)) == CB_ERR)
               {
                  index = (int)SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_ADDSTRING, 0, (LPARAM)_16(tempstr));

                  if (dmSettings.dmPelsWidth == fullscreenwidth &&
                      dmSettings.dmPelsHeight == fullscreenheight)
                     SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_SETCURSEL, index, 0);
               }
            }
         }

         // Setup use custom window size
         if (usecustomwindowsize)
         {
            SendDlgItemMessage(hDlg, IDC_CUSTOMWINDOWCB, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_WIDTHEDIT), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_HEIGHTEDIT), TRUE);
         }
         else
            SendDlgItemMessage(hDlg, IDC_CUSTOMWINDOWCB, BM_SETCHECK, BST_UNCHECKED, 0);

         // Setup window width and height
         sprintf(tempstr, "%d", windowwidth);
         _16(tempstr);
         SetDlgItemText(hDlg, IDC_WIDTHEDIT, _16(tempstr));
         sprintf(tempstr, "%d", windowheight);
         SetDlgItemText(hDlg, IDC_HEIGHTEDIT, _16(tempstr));

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_CUSTOMWINDOWCB:
            {
               if (SendDlgItemMessage(hDlg, IDC_CUSTOMWINDOWCB, BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  EnableWindow(GetDlgItem(hDlg, IDC_WIDTHEDIT), TRUE);
                  EnableWindow(GetDlgItem(hDlg, IDC_HEIGHTEDIT), TRUE);
               }
               else
               {
                  EnableWindow(GetDlgItem(hDlg, IDC_WIDTHEDIT), FALSE);
                  EnableWindow(GetDlgItem(hDlg, IDC_HEIGHTEDIT), FALSE);
               }

               return TRUE;
            }
            case IDOK:
            {
               WCHAR tempwstr[MAX_PATH];
               char tempstr[MAX_PATH];
               int cursel;
               int newvidcoretype;
               BOOL vidcorechanged=FALSE;

               EndDialog(hDlg, TRUE);

               // Write Video core type
               newvidcoretype = VIDCoreList[SendDlgItemMessage(hDlg, IDC_VIDEOCORECB, CB_GETCURSEL, 0, 0)]->id;
               if (newvidcoretype != vidcoretype)
               {
                  vidcoretype = newvidcoretype;
                  vidcorechanged = TRUE;
                  sprintf(tempstr, "%d", vidcoretype);
                  WritePrivateProfileStringA("Video", "VideoCore", tempstr, inifilename);
               }

               if (SendDlgItemMessage(hDlg, IDC_AUTOFRAMESKIPCB, BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  EnableAutoFrameSkip();
                  enableautofskip = 1;
               }
               else
               {
                  DisableAutoFrameSkip();
                  enableautofskip = 0;
               }

               // Write Auto frameskip
               sprintf(tempstr, "%d", enableautofskip);
               WritePrivateProfileStringA("Video", "AutoFrameSkip", tempstr, inifilename);

               // Write full screen on startup setting
               if (SendDlgItemMessage(hDlg, IDC_FULLSCREENSTARTUPCB, BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  usefullscreenonstartup = 1;
                  WritePrivateProfileStringA("Video", "UseFullScreenOnStartup", "1", inifilename);
               }
               else
               {
                  usefullscreenonstartup = 0;
                  WritePrivateProfileStringA("Video", "UseFullScreenOnStartup", "0", inifilename);
               }

               // Write full screen size settings
               cursel = (int)SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_GETCURSEL, 0, 0);
               if (SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_GETLBTEXTLEN, cursel, 0) <= MAX_PATH)
               {
                  SendDlgItemMessageA(hDlg, IDC_FSSIZECB, CB_GETLBTEXT, cursel, (LPARAM)tempstr);
                  sscanf(tempstr, "%dx%d", &fullscreenwidth, &fullscreenheight);
               }

               sprintf(tempstr, "%d", fullscreenwidth);
               WritePrivateProfileStringA("Video", "FullScreenWidth", tempstr, inifilename);
               sprintf(tempstr, "%d", fullscreenheight);
               WritePrivateProfileStringA("Video", "FullScreenHeight", tempstr, inifilename);

               // Write use custom window size setting
               if (SendDlgItemMessage(hDlg, IDC_CUSTOMWINDOWCB, BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  usecustomwindowsize = 1;
                  WritePrivateProfileStringA("Video", "UseCustomWindowSize", "1", inifilename);
               }
               else
               {
                  usecustomwindowsize = 0;
                  WritePrivateProfileStringA("Video", "UseCustomWindowSize", "0", inifilename);
               }
               
               // Write window width and height settings
               GetDlgItemText(hDlg, IDC_WIDTHEDIT, tempwstr, MAX_PATH);
               WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, tempstr, MAX_PATH, NULL, NULL);
               windowwidth = atoi(tempstr);
               WritePrivateProfileStringA("Video", "WindowWidth", tempstr, inifilename);
               GetDlgItemText(hDlg, IDC_HEIGHTEDIT, tempwstr, MAX_PATH);
               WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, tempstr, MAX_PATH, NULL, NULL);
               windowheight = atoi(tempstr);
               WritePrivateProfileStringA("Video", "WindowHeight", tempstr, inifilename);

               // Re-initialize Video
               if (vidcorechanged && nocorechange == 0)
               {
#ifndef USETHREADS
                  VideoChangeCore(vidcoretype);
#else
                  corechanged = 0;
                  changecore |= 2;
                  while (corechanged == 0) { Sleep(0); }
#endif
               }

               if (VIDCore && !VIDCore->IsFullscreen() && usecustomwindowsize)
                  VIDCore->Resize(windowwidth, windowheight, 0);

               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            default: break;
         }

         break;
      }
      case WM_DESTROY:
      {
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK SoundSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         int i;

         // Setup Sound Core Combo box
         SendDlgItemMessage(hDlg, IDC_SOUNDCORECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_SOUNDCORECB, CB_ADDSTRING, 0, (LPARAM)_16("None"));

         for (i = 1; SNDCoreList[i] != NULL; i++)
            SendDlgItemMessage(hDlg, IDC_SOUNDCORECB, CB_ADDSTRING, 0, (LPARAM)_16(SNDCoreList[i]->Name));

         // Set Selected Sound Core
         for (i = 0; SNDCoreList[i] != NULL; i++)
         {
            if (sndcoretype == SNDCoreList[i]->id)
               SendDlgItemMessage(hDlg, IDC_SOUNDCORECB, CB_SETCURSEL, i, 0);
         }

         // Setup Volume Slider
         SendDlgItemMessage(hDlg, IDC_SLVOLUME, TBM_SETRANGE, 0, MAKELONG(0, 100));

         // Set Selected Volume
         SendDlgItemMessage(hDlg, IDC_SLVOLUME, TBM_SETPOS, TRUE, sndvolume);

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
               char tempstr[MAX_PATH];
               int newsndcoretype;
               BOOL sndcorechanged=FALSE;

               EndDialog(hDlg, TRUE);

               // Write Sound core type
               newsndcoretype = SNDCoreList[SendDlgItemMessage(hDlg, IDC_SOUNDCORECB, CB_GETCURSEL, 0, 0)]->id;
               if (newsndcoretype != sndcoretype)
               {
                  sndcoretype = newsndcoretype;
                  sndcorechanged = TRUE;
                  sprintf(tempstr, "%d", sndcoretype);
                  WritePrivateProfileStringA("Sound", "SoundCore", tempstr, inifilename);
               }

               // Write Volume
               sndvolume = (int)SendDlgItemMessage(hDlg, IDC_SLVOLUME, TBM_GETPOS, 0, 0);
               sprintf(tempstr, "%d", sndvolume);
               WritePrivateProfileStringA("Sound", "Volume", tempstr, inifilename);
               if (sndcorechanged && nocorechange == 0)
               {
#ifndef USETHREADS
                  ScspChangeSoundCore(sndcoretype);
#else
                  corechanged = 0;
                  changecore |= 4;
                  while (corechanged == 0) { Sleep(0); }
#endif
               }

               ScspSetVolume(sndvolume);
               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            default: break;
         }

         break;
      }
      case WM_DESTROY:
      {
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK NetlinkSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                        LPARAM lParam)
{
   WCHAR tempwstr[MAX_PATH];
   char tempstr[MAX_PATH];

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         SendDlgItemMessage(hDlg, IDC_LOCALREMOTEIP, IPM_SETADDRESS, 0, netlinklocalremoteip);

         sprintf(tempstr, "%d", netlinkport);
         SetDlgItemText(hDlg, IDC_PORTET, _16(tempstr));

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
               EndDialog(hDlg, TRUE);

               // Local/Remote IP
               SendDlgItemMessage(hDlg, IDC_LOCALREMOTEIP, IPM_GETADDRESS, 0, (LPARAM)&netlinklocalremoteip);
               sprintf(tempstr, "%d.%d.%d.%d", (int)FIRST_IPADDRESS(netlinklocalremoteip), (int)SECOND_IPADDRESS(netlinklocalremoteip), (int)THIRD_IPADDRESS(netlinklocalremoteip), (int)FOURTH_IPADDRESS(netlinklocalremoteip));
               WritePrivateProfileStringA("Netlink", "LocalRemoteIP", tempstr, inifilename);

               // Port Number
               GetDlgItemText(hDlg, IDC_PORTET, tempwstr, MAX_PATH);
               WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, tempstr, MAX_PATH, NULL, NULL);
               netlinkport = atoi(tempstr);
               WritePrivateProfileStringA("Netlink", "Port", tempstr, inifilename);
               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            default: break;
         }

         break;
      }
      case WM_DESTROY:
      {
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

int ConvertEmulateTypeSelStringToID(HWND hDlg, int id)
{
   WCHAR wtext[MAX_PATH];
   char text[MAX_PATH];

   ComboBox_GetLBText(GetDlgItem(hDlg, id),
                      ComboBox_GetCurSel(GetDlgItem(hDlg, id)),
                      wtext);
   WideCharToMultiByte(CP_ACP, 0, wtext, -1, text, MAX_PATH, NULL, NULL);

   if (strcmp(text, "Standard Pad") == 0)
      return EMUTYPE_STANDARDPAD;
   else if (strcmp(text, "Analog Pad") == 0)
      return EMUTYPE_ANALOGPAD;
   else if (strcmp(text, "Stunner") == 0)
      return EMUTYPE_STUNNER;
   else if (strcmp(text, "Mouse") == 0)
      return EMUTYPE_MOUSE;
   else if (strcmp(text, "Keyboard") == 0)
      return EMUTYPE_KEYBOARD;
   else
      return EMUTYPE_NONE;
}

//////////////////////////////////////////////////////////////////////////////

int configpadnum=0;

LRESULT CALLBACK InputSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                      LPARAM lParam)
{
   static helpballoon_struct hb[26];

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         int i;
         u32 j;

         SendDlgItemMessage(hDlg, IDC_PORT1CONNTYPECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_PORT1CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("None"));
         SendDlgItemMessage(hDlg, IDC_PORT1CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("Direct"));
         SendDlgItemMessage(hDlg, IDC_PORT1CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("6-port Multitap"));

         SendDlgItemMessage(hDlg, IDC_PORT2CONNTYPECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_PORT2CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("None"));
         SendDlgItemMessage(hDlg, IDC_PORT2CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("Direct"));
         SendDlgItemMessage(hDlg, IDC_PORT2CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("6-port Multitap"));

         for (i = 0; i < 6; i++)
         {
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("None"));
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("Standard Pad"));
/*
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("Analog Pad"));
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("Stunner"));
*/
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("Mouse"));
/*
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("Keyboard"));
*/
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("None"));
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("Standard Pad"));
/*
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("Analog Pad"));
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("Stunner"));
*/
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("Mouse"));
/*
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)_16("Keyboard"));
*/
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_SETCURSEL, 0, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_PORT1ATYPECB+i), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_PORT1ACFGPB+i), FALSE);

            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_SETCURSEL, 0, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_PORT2ATYPECB+i), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_PORT2ACFGPB+i), FALSE);
         }

         SendDlgItemMessage(hDlg, IDC_PORT1CONNTYPECB, CB_SETCURSEL, 1, 0);
         EnableWindow(GetDlgItem(hDlg, IDC_PORT1CONNTYPECB), FALSE);
         SendDlgItemMessage(hDlg, IDC_PORT2CONNTYPECB, CB_SETCURSEL, 1, 0);
         EnableWindow(GetDlgItem(hDlg, IDC_PORT2CONNTYPECB), FALSE);

         EnableWindow(GetDlgItem(hDlg, IDC_PORT1ATYPECB), TRUE);
         EnableWindow(GetDlgItem(hDlg, IDC_PORT2ATYPECB), TRUE);

         if (!PERCore)
         {
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB, CB_SETCURSEL, 1, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_PORT1ACFGPB), TRUE);            
         }

         // Go through previous settings and figure out which controls to 
         // enable, etc.
         for (j = 0; j < numpads; j++)
         {
            if (paddevice[j].emulatetype != 0)
            {
               int id=0;
               switch (pad[j]->perid)
               {
                  case PERPAD:
                     id = 1;
                     break;
                  case PERMOUSE:
                     id = 2;
                     break;
                  default: break;
               }

               SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+j, CB_SETCURSEL, id, 0);
               EnableWindow(GetDlgItem(hDlg, IDC_PORT1ACFGPB+j), TRUE);            
            }
         }

         // Setup Tooltips
         hb[0].string = "Use this to select whether to use a multi-tap or direct connection";
         hb[0].hParent = GetDlgItem(hDlg, IDC_PORT1CONNTYPECB);
         hb[1].string = hb[0].string;
         hb[1].hParent = GetDlgItem(hDlg, IDC_PORT2CONNTYPECB);

         for (i = 0; i < 6; i++)
         {
            hb[2+i].string = "Use this to select what kind of peripheral to emulate";
            hb[2+i].hParent = GetDlgItem(hDlg, IDC_PORT1ATYPECB+i);
            hb[8+i].string = hb[2+i].string;
            hb[8+i].hParent = GetDlgItem(hDlg, IDC_PORT2ATYPECB+i);            

            hb[14+i].string = "Press this to change the button configuration, etc.";
            hb[14+i].hParent = GetDlgItem(hDlg, IDC_PORT1ACFGPB+i);
            hb[20+i].string = hb[14+i].string;
            hb[20+i].hParent = GetDlgItem(hDlg, IDC_PORT2ACFGPB+i);            
         }
         
         hb[26].string = NULL;

         CreateHelpBalloons(hb);

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_PORT1ATYPECB:
            case IDC_PORT1BTYPECB:
            case IDC_PORT1CTYPECB:
            case IDC_PORT1DTYPECB:
            case IDC_PORT1ETYPECB:
            case IDC_PORT1FTYPECB:
            case IDC_PORT2ATYPECB:
            case IDC_PORT2BTYPECB:
            case IDC_PORT2CTYPECB:
            case IDC_PORT2DTYPECB:
            case IDC_PORT2ETYPECB:
            case IDC_PORT2FTYPECB:
            {
               switch(HIWORD(wParam))
               {
                  case CBN_SELCHANGE:
                     // If emulated peripheral is set to none, we don't want 
                     // the config button enabled
                     if (ComboBox_GetCurSel((HWND)lParam) != 0)
                        Button_Enable(GetDlgItem(hDlg, IDC_PORT1ACFGPB+(int)LOWORD(wParam)-IDC_PORT1ATYPECB), TRUE);
                     else
                        Button_Enable(GetDlgItem(hDlg, IDC_PORT1ACFGPB+(int)LOWORD(wParam)-IDC_PORT1ATYPECB), FALSE);

                     break;
                  default: break;
               }
               break;
            }
            case IDC_PORT1ACFGPB:
            case IDC_PORT1BCFGPB:
            case IDC_PORT1CCFGPB:
            case IDC_PORT1DCFGPB:
            case IDC_PORT1ECFGPB:
            case IDC_PORT1FCFGPB:
            case IDC_PORT2ACFGPB:
            case IDC_PORT2BCFGPB:
            case IDC_PORT2CCFGPB:
            case IDC_PORT2DCFGPB:
            case IDC_PORT2ECFGPB:
            case IDC_PORT2FCFGPB:
               switch (ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PORT1ATYPECB+LOWORD(wParam)-IDC_PORT1ACFGPB)))
               {
                  case 1:
                     // Standard Pad
                     DialogBoxParam(y_hInstance, MAKEINTRESOURCE(IDD_PADCONFIG), hDlg, (DLGPROC)PadConfigDlgProc, (LPARAM)LOWORD(wParam)-IDC_PORT1ACFGPB);
                     break;
                  case 2:
                     // Mouse
                     DialogBoxParam(y_hInstance, MAKEINTRESOURCE(IDD_MOUSECONFIG), hDlg, (DLGPROC)MouseConfigDlgProc, (LPARAM)LOWORD(wParam)-IDC_PORT1ACFGPB);
                     break;
                  default: break;
               }
               return TRUE;
            case IDOK:
            {
               u32 i;
               char string1[13], string2[32];

               for (i = 0; i < numpads; i++)
               {
                  sprintf(string1, "Peripheral%ld", i+1);
                  sprintf(string2, "%d", ConvertEmulateTypeSelStringToID(hDlg, IDC_PORT1ATYPECB+i));
                  WritePrivateProfileStringA(string1, "EmulateType", string2, inifilename);
               }
               EndDialog(hDlg, TRUE);
               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            default: break;
         }

         break;
      }
      case WM_DESTROY:
      {
         // Reload device(s)
         PERDXLoadDevices(inifilename);

         DestroyHelpBalloons(hb);
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

void DoControlConfig(HWND hDlg, u8 cursel, int controlid, int basecontrolid, int *controlmap)
{
   char buttonname[MAX_PATH];
   int ret;

   if ((ret = PERDXFetchNextPress(hDlg, cursel, buttonname)) >= 0)
   {
      SetDlgItemText(hDlg, controlid, _16(buttonname));
      controlmap[controlid - basecontrolid] = ret;
   }
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK PadConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
   static u8 cursel;
   static BOOL enablebuttons;
   static int controlmap[13];
   static int padnum;
   static HWND hParent;
   int i;

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         padnum = (int)lParam;
         i = TabCtrl_GetCurSel(GetDlgItem(GetParent(hDlg), IDC_SETTINGSTAB));
         hParent = dialoglist[i];
         PERDXListDevices(GetDlgItem(hDlg, IDC_DXDEVICECB), ConvertEmulateTypeSelStringToID(hParent, IDC_PORT1ATYPECB+padnum));

         // Load settings from ini here, if necessary
         PERDXInitControlConfig(hDlg, padnum, controlmap, inifilename);

         cursel = (u8)SendDlgItemMessage(hDlg, IDC_DXDEVICECB, CB_GETCURSEL, 0, 0);
         enablebuttons = cursel ? TRUE : FALSE;

         EnableWindow(GetDlgItem(hDlg, IDC_UPPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_DOWNPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_LEFTPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_RIGHTPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_LPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_RPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_STARTPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_APB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_BPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_CPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_XPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_YPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_ZPB), enablebuttons);
         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_UPPB:
            case IDC_DOWNPB:
            case IDC_LEFTPB:
            case IDC_RIGHTPB:
            case IDC_LPB:
            case IDC_RPB:
            case IDC_STARTPB:
            case IDC_APB:
            case IDC_BPB:
            case IDC_CPB:
            case IDC_XPB:
            case IDC_YPB:
            case IDC_ZPB:
            {
               DoControlConfig(hDlg, cursel-1, IDC_UPTEXT+(LOWORD(wParam)-IDC_UPPB), IDC_UPTEXT, controlmap);

               return TRUE;
            }
            case IDOK:
            {
               char string1[20];
               char string2[20];

               EndDialog(hDlg, TRUE);

               sprintf(string1, "Peripheral%d", padnum+1);

               // Write GUID
               PERDXWriteGUID(cursel-1, padnum, inifilename);

               for (i = 0; i < 13; i++)
               {
                  sprintf(string2, "%d", controlmap[i]);
                  WritePrivateProfileStringA(string1, pad_names[i], string2, inifilename);
               }

               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            case IDC_DXDEVICECB:
            {
               switch(HIWORD(wParam))
               {
                  case CBN_SELCHANGE:
                  {
                     cursel = (u8)SendDlgItemMessage(hDlg, IDC_DXDEVICECB, CB_GETCURSEL, 0, 0);
                     enablebuttons = cursel ? TRUE : FALSE;

                     EnableWindow(GetDlgItem(hDlg, IDC_UPPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_DOWNPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_LEFTPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_RIGHTPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_LPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_RPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_STARTPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_APB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_BPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_CPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_XPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_YPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_ZPB), enablebuttons);
                     break;
                  }
                  default: break;
               }
               break;
            }
            default: break;
         }

         break;
      }
      case WM_DESTROY:
      {
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK MouseConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
   static u8 cursel;
   static BOOL enablebuttons;
   static int controlmap[13];
   static int padnum;
   static HWND hParent;
   static int emulatetype;
   int i;

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         padnum = (int)lParam;
         i = TabCtrl_GetCurSel(GetDlgItem(GetParent(hDlg), IDC_SETTINGSTAB));
         hParent = dialoglist[i];
         emulatetype = ConvertEmulateTypeSelStringToID(hParent, IDC_PORT1ATYPECB+padnum);
         PERDXListDevices(GetDlgItem(hDlg, IDC_DXDEVICECB), emulatetype);

         // Load settings from ini here, if necessary
         PERDXInitControlConfig(hDlg, padnum, controlmap, inifilename);

         cursel = (u8)SendDlgItemMessage(hDlg, IDC_DXDEVICECB, CB_GETCURSEL, 0, 0);
         enablebuttons = cursel ? TRUE : FALSE;

         EnableWindow(GetDlgItem(hDlg, IDC_STARTPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_APB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_BPB), enablebuttons);
         EnableWindow(GetDlgItem(hDlg, IDC_CPB), enablebuttons);
         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_STARTPB:
            case IDC_APB:
            case IDC_BPB:
            case IDC_CPB:
            {
               DoControlConfig(hDlg, cursel-1, IDC_UPTEXT+(LOWORD(wParam)-IDC_UPPB), IDC_UPTEXT, controlmap);

               return TRUE;
            }
            case IDOK:
            {
               char string1[20];
               char string2[20];

               EndDialog(hDlg, TRUE);

               sprintf(string1, "Peripheral%d", padnum+1);

               // Write GUID
               PERDXWriteGUID(cursel-1, padnum, inifilename);

               for (i = 0; i < 13; i++)
               {
                  sprintf(string2, "%d", controlmap[i]);
                  WritePrivateProfileStringA(string1, pad_names[i], string2, inifilename);
               }
               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            case IDC_DXDEVICECB:
            {
               switch(HIWORD(wParam))
               {
                  case CBN_SELCHANGE:
                  {
                     cursel = (u8)SendDlgItemMessage(hDlg, IDC_DXDEVICECB, CB_GETCURSEL, 0, 0);
                     enablebuttons = cursel ? TRUE : FALSE;

                     EnableWindow(GetDlgItem(hDlg, IDC_STARTPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_APB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_BPB), enablebuttons);
                     EnableWindow(GetDlgItem(hDlg, IDC_CPB), enablebuttons);
                     break;
                  }
                  default: break;
               }
               break;
            }
            default: break;
         }

         break;
      }
      case WM_DESTROY:
      {
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK LogSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         // Setup use log setting
         SendDlgItemMessage(hDlg, IDC_USELOGCB, BM_SETCHECK, uselog ? BST_CHECKED : BST_UNCHECKED, 0);
         SendMessage(hDlg, WM_COMMAND, IDC_USELOGCB, 0);

         // Setup log type setting
         SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("Write to File"));
         SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_ADDSTRING, 0, (LPARAM)_16("Write to Window"));
         SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_SETCURSEL, logtype, 0);

         // Setup log filename setting
         SetDlgItemText(hDlg, IDC_LOGFILENAMEET, _16(logfilename));

         // mini18n log settings
         SendDlgItemMessage(hDlg, IDC_USEMINI18NLOG, BM_SETCHECK, usemini18nlog ? BST_CHECKED : BST_UNCHECKED, 0);

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_USELOGCB:
            {
               if (SendDlgItemMessage(hDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  SendMessage(hDlg, WM_COMMAND, (CBN_SELCHANGE << 16) | IDC_LOGTYPECB, 0);
                  EnableWindow(GetDlgItem(hDlg, IDC_LOGTYPECB), TRUE);
               }
               else
               {
                  EnableWindow(GetDlgItem(hDlg, IDC_LOGTYPECB), FALSE);
                  EnableWindow(GetDlgItem(hDlg, IDC_LOGFILENAMEET), FALSE);
                  EnableWindow(GetDlgItem(hDlg, IDC_LOGBROWSEBT), FALSE);
               }

               return TRUE;
            }
            case IDC_LOGTYPECB:
            {
               switch(HIWORD(wParam))
               {
                  case CBN_SELCHANGE:
                  {
                     if (SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_GETCURSEL, 0, 0) == 0)
                     {
                        EnableWindow(GetDlgItem(hDlg, IDC_LOGFILENAMEET), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_LOGBROWSEBT), TRUE);
                     }
                     else
                     {
                        EnableWindow(GetDlgItem(hDlg, IDC_LOGFILENAMEET), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_LOGBROWSEBT), FALSE);
                     }

                     return TRUE;
                  }
                  default: break;
               }

               return TRUE;
            }
            case IDC_LOGBROWSEBT:
            {
               WCHAR tempwstr[MAX_PATH];
               WCHAR filter[1024];
               OPENFILENAME ofn;

               // setup ofn structure
               ZeroMemory(&ofn, sizeof(ofn));
               ofn.lStructSize = sizeof(ofn);
               ofn.hwndOwner = hDlg;

               CreateFilter(filter, 1024,
                  "Text Files", "*.txt",
                  "All Files", "*.*", NULL);

               ofn.lpstrFilter = filter;
               ofn.nFilterIndex = 1;
               GetDlgItemText(hDlg, IDC_LOGFILENAMEET, tempwstr, MAX_PATH);
               ofn.lpstrFile = tempwstr;
               ofn.nMaxFile = sizeof(tempwstr);
               ofn.Flags = OFN_OVERWRITEPROMPT;
               ofn.lpstrDefExt = _16("TXT");

               if (GetSaveFileName(&ofn))
               {
                  SetDlgItemText(hDlg, IDC_LOGFILENAMEET, tempwstr);
                  WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, logfilename, MAX_PATH, NULL, NULL);
               }

               return TRUE;
            }
            case IDC_USEMINI18NLOG:
            {
               BOOL enabled;

               enabled = SendDlgItemMessage(hDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) == BST_CHECKED ? TRUE : FALSE;

               EnableWindow(GetDlgItem(hDlg, IDC_MINI18NLOGFILENAME), enabled);
               EnableWindow(GetDlgItem(hDlg, IDC_MINI18NLOGBROWSE), enabled);

               return TRUE;
            }
            case IDC_MINI18NLOGBROWSE:
            {
               WCHAR tempwstr[MAX_PATH];
               WCHAR filter[1024];
               OPENFILENAME ofn;

               // setup ofn structure
               ZeroMemory(&ofn, sizeof(ofn));
               ofn.lStructSize = sizeof(ofn);
               ofn.hwndOwner = hDlg;

               CreateFilter(filter, 1024,
                  "Yabause Translation Files", "*.YTS",
                  "All Files", "*.*", NULL);

               ofn.lpstrFilter = filter;
               ofn.nFilterIndex = 1;
               GetDlgItemText(hDlg, IDC_MINI18NLOGFILENAME, tempwstr, MAX_PATH);
               ofn.lpstrFile = tempwstr;
               ofn.nMaxFile = sizeof(tempwstr);
               ofn.Flags = OFN_OVERWRITEPROMPT;
               ofn.lpstrDefExt = _16("YTS");

               if (GetSaveFileName(&ofn))
               {
                  SetDlgItemText(hDlg, IDC_MINI18NLOGFILENAME, tempwstr);
                  WideCharToMultiByte(CP_ACP, 0, tempwstr, -1, logfilename, MAX_PATH, NULL, NULL);
               }

               return TRUE;
            }
            case IDOK:
            {
               char tempstr[MAX_PATH];

               // Write use log setting
               if (SendDlgItemMessage(hDlg, IDC_USELOGCB, BM_GETCHECK, 0, 0) == BST_CHECKED)
                  uselog = 1;
               else
                  uselog = 0;

               sprintf(tempstr, "%d", uselog);
               WritePrivateProfileStringA("Log", "Enable", tempstr, inifilename);

               // Write log type setting
               logtype = (int)SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_GETCURSEL, 0, 0);
               sprintf(tempstr, "%d", logtype);
               WritePrivateProfileStringA("Log", "Type", tempstr, inifilename);

               // Write log filename
               WritePrivateProfileStringA("Log", "Filename", logfilename, inifilename);

               // Write use mini18n log setting
               if (SendDlgItemMessage(hDlg, IDC_USELOGCB, BM_GETCHECK, 0, 0) == BST_CHECKED)
                  usemini18nlog = 1;
               else
                  usemini18nlog = 0;

               sprintf(tempstr, "%d", usemini18nlog);
               WritePrivateProfileStringA("Mini18nLog", "Enable", tempstr, inifilename);

               // Write mini18n log filename
               WritePrivateProfileStringA("Mini18nLog", "Filename", mini18nlogfilename, inifilename);

               EndDialog(hDlg, TRUE);
               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            default: break;
         }

         break;
      }
      case WM_DESTROY:
      {
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

u32 currentbupdevice=0;
deviceinfo_struct *devices=NULL;
int numbupdevices=0;
saveinfo_struct *saves=NULL;
int numsaves=0;

//////////////////////////////////////////////////////////////////////////////

void RefreshSaveList(HWND hDlg)
{
   int i;
   u32 freespace=0, maxspace=0;
   char tempstr[MAX_PATH];

   saves = BupGetSaveList(currentbupdevice, &numsaves);

   SendDlgItemMessage(hDlg, IDC_BUPSAVELB, LB_RESETCONTENT, 0, 0);

   for (i = 0; i < numsaves; i++)
      SendDlgItemMessageA(hDlg, IDC_BUPSAVELB, LB_ADDSTRING, 0, (LPARAM)saves[i].filename);

   BupGetStats(currentbupdevice, &freespace, &maxspace);
   sprintf(tempstr, "%d/%d blocks free", (int)freespace, (int)maxspace);
   SetDlgItemText(hDlg, IDC_BUPFREESPACELT, _16(tempstr));                     
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK BackupRamDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
   char tempstr[MAX_PATH];
   char tempstr2[MAX_PATH];

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         int i;

         // Get available devices
         if ((devices = BupGetDeviceList(&numbupdevices)) == NULL)
            return FALSE;

         SendDlgItemMessage(hDlg, IDC_BUPDEVICECB, CB_RESETCONTENT, 0, 0);
         for (i = 0; i < numbupdevices; i++)
            SendDlgItemMessage(hDlg, IDC_BUPDEVICECB, CB_ADDSTRING, 0, (LPARAM)_16(devices[i].name));

         SendDlgItemMessage(hDlg, IDC_BUPDEVICECB, CB_SETCURSEL, 0, 0);
         RefreshSaveList(hDlg);
         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_BUPDEVICECB:
            {
               switch(HIWORD(wParam))
               {
                  case CBN_SELCHANGE:
                  {
                     currentbupdevice = (u8)SendDlgItemMessage(hDlg, IDC_BUPDEVICECB, CB_GETCURSEL, 0, 0);
                     RefreshSaveList(hDlg);
                     return TRUE;
                  }
                  default: break;
               }

               return TRUE;
            }
            case IDC_BUPSAVELB:
            {
               switch(HIWORD(wParam))
               {
                  case LBN_SELCHANGE:
                  {
                     u8 cursel=0;
                     int i;

                     cursel = (u8)SendDlgItemMessage(hDlg, IDC_BUPSAVELB, LB_GETCURSEL, 0, 0);

                     SendDlgItemMessage(hDlg, IDC_BUPSAVELB, LB_GETTEXT, cursel, (LPARAM)_16(tempstr));

                     for (i = 0; i < numsaves; i++)
                     {
                        if (strcmp(tempstr, saves[i].filename) == 0)
                        {
                           cursel = i;
                           break;
                        }
                     }

                     SetDlgItemText(hDlg, IDC_BUPFILENAMEET, _16(saves[cursel].filename));
                     SetDlgItemText(hDlg, IDC_BUPCOMMENTET, _16(saves[cursel].comment));
                     switch(saves[cursel].language)
                     {
                        case 0:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, _16("Japanese"));
                           break;
                        case 1:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, _16("English"));
                           break;
                        case 2:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, _16("French"));
                           break;
                        case 3:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, _16("German"));
                           break;
                        case 4:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, _16("Spanish"));
                           break;
                        case 5:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, _16("Italian"));
                           break;
                        default: break;
                     }
                     sprintf(tempstr, "%d", (int)saves[cursel].datasize);
                     SetDlgItemText(hDlg, IDC_BUPDATASIZEET, _16(tempstr));
                     sprintf(tempstr, "%d", saves[cursel].blocksize);
                     SetDlgItemText(hDlg, IDC_BUPBLOCKSIZEET, _16(tempstr));
                     return TRUE;
                  }
                  default: break;
               }

               return TRUE;
            }
            case IDC_BUPDELETEBT:
            {
               LRESULT cursel = SendDlgItemMessage(hDlg, IDC_BUPSAVELB, LB_GETCURSEL, 0, 0);

               if (cursel == LB_ERR)
                  return TRUE;

               SendDlgItemMessage(hDlg, IDC_BUPSAVELB, LB_GETTEXT, cursel, (LPARAM)tempstr);

               sprintf(tempstr2, "Are you sure you want to delete %s?", tempstr);
               if (MessageBox (hDlg, _16(tempstr2), _16("Confirm Delete"),  MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
               {
                  BupDeleteSave(currentbupdevice, tempstr);
                  RefreshSaveList(hDlg);
               }
               return TRUE;
            }
            case IDC_BUPFORMATBT:
            {
               sprintf(tempstr, "Are you sure you want to format %s?", devices[currentbupdevice].name);
               if (MessageBox (hDlg, _16(tempstr), _16("Confirm Delete"),  MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2) == IDYES)
               {
                  BupFormat(currentbupdevice);
                  RefreshSaveList(hDlg);
               }
               return TRUE;
            }
            case IDC_BUPIMPORTBT:
            {
               RefreshSaveList(hDlg);
               return TRUE;
            }
            case IDC_BUPEXPORTBT:
            {
               RefreshSaveList(hDlg);
               return TRUE;
            }
            case IDOK:
            {
               EndDialog(hDlg, TRUE);
               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            default: break;
         }

         break;
      }
      case WM_CLOSE:
      {
         EndDialog(hDlg, TRUE);

         return TRUE;
      }
      case WM_DESTROY:
      {
         if (saves)
            free(saves);
         break;
      }
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

int CreateHelpBalloons(helpballoon_struct *hb)
{
   TOOLINFO ti;
   RECT rect;
   int i;

   for (i = 0; hb[i].string != NULL; i++)
   {
      hb[i].hWnd = CreateWindowEx(WS_EX_TOPMOST,
                                   TOOLTIPS_CLASS,
                                   NULL,
                                   WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   CW_USEDEFAULT,
                                   hb[i].hParent,
                                   NULL,
                                   y_hInstance,
                                   NULL);

      if (!hb[i].hWnd)
         return -1;

      SetWindowPos(hb[i].hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

      // Create some help balloons
      ti.cbSize = sizeof(TOOLINFO);
      ti.uFlags = TTF_SUBCLASS;
      ti.hwnd = hb[i].hParent;
      ti.hinst = y_hInstance;
      ti.uId = 0;
      ti.lpszText = _16(hb[i].string);
      GetClientRect(hb[i].hParent, &rect);
      ti.rect.left = rect.left;
      ti.rect.top = rect.top;
      ti.rect.right = rect.right;
      ti.rect.bottom = rect.bottom;

      // Add it
      SendMessage(hb[i].hWnd, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void DestroyHelpBalloons(helpballoon_struct *hb)
{
   int i;
   for (i = 0; hb[i].string != NULL; i++)
   {
      if (hb[i].hWnd)
      {
         DestroyWindow(hb[i].hWnd);
         hb[i].hWnd = NULL;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void CreateFilter(WCHAR * filter, size_t maxlen, ...)
{
   va_list list;
   const char * str;
   WCHAR * filterpos = filter;
   int wrote;

   va_start(list, maxlen);

   str = va_arg(list, const char *);
   while(str != NULL) {
#ifdef __MINGW32_VERSION
      wrote = swprintf(filterpos, _16(str));
#else
      wrote = swprintf(filterpos, maxlen, _16(str));
#endif
      filterpos += 1 + wrote;
      maxlen -= 1 + wrote;
      str = va_arg(list, const char *);
   }
   *filterpos = '\0';

   va_end(list);
}

//////////////////////////////////////////////////////////////////////////////
