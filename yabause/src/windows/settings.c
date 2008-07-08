/*  Copyright 2004-2008 Theo Berkau

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

      if (GetDriveType(tempstr) == DRIVE_CDROM)
      {
         drive_list[num_cdroms] = 'c' + i;

         sprintf(tempstr, "%c", 'C' + i);
         SendDlgItemMessage(hWnd, IDC_DRIVELETTERCB, CB_ADDSTRING, 0, (LPARAM)tempstr);
         num_cdroms++;
      } 
   }
}

//////////////////////////////////////////////////////////////////////////////

BOOL IsPathCdrom(const char *path)
{
   if (GetDriveType(cdrompath) == DRIVE_CDROM)
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
         tabitem.pszText = "Basic";
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_BASICSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)BasicSettingsDlgProc);
         i++;

         tabitem.pszText = "Video";
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_VIDEOSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)VideoSettingsDlgProc);
         i++;

         tabitem.pszText = "Sound";
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_SOUNDSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)SoundSettingsDlgProc);
         i++;

         tabitem.pszText = "Input";
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_INPUTSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)InputSettingsDlgProc);
         i++;

#ifdef USESOCKET
         tabitem.pszText = "Netlink";
         TabCtrl_InsertItem(GetDlgItem(hDlg, IDC_SETTINGSTAB), i, &tabitem);

         dialoglist[i] = CreateDialog(y_hInstance,
                                      MAKEINTRESOURCE(IDD_NETLINKSETTINGS),
                                      GetDlgItem(hDlg, IDC_SETTINGSTAB),
                                      (DLGPROC)NetlinkSettingsDlgProc);
         i++;
#endif

#if DEBUG
         tabitem.pszText = "Log";
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
         SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_ADDSTRING, 0, (LPARAM)"CD");
         SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_ADDSTRING, 0, (LPARAM)"Image");

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
            SetDlgItemText(hDlg, IDC_IMAGEEDIT, cdrompath);

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
         SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_ADDSTRING, 0, (LPARAM)"Fast Interpreter");
         SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_ADDSTRING, 0, (LPARAM)"Debug Interpreter");

         // Set Selected SH2 Core
         SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_SETCURSEL, sh2coretype, 0);

         // Setup Region Combo box
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)"Auto-detect");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)"Japan(NTSC)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)"Asia(NTSC)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)"North America(NTSC)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)"Central/South America(NTSC)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)"Korea(NTSC)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)"Asia(PAL)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)"Europe + others(PAL)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (LPARAM)"Central/South America(PAL)");

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
         SetDlgItemText(hDlg, IDC_BIOSEDIT, biosfilename);

         // Set Default Backup RAM File
         SetDlgItemText(hDlg, IDC_BACKUPRAMEDIT, backupramfilename);

         // Set Default MPEG ROM File
         SetDlgItemText(hDlg, IDC_MPEGROMEDIT, mpegromfilename);

         // Setup Cart Type Combo box
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"None");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"Pro Action Replay");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"4 Mbit Backup Ram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"8 Mbit Backup Ram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"16 Mbit Backup Ram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"32 Mbit Backup Ram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"8 Mbit Dram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"32 Mbit Dram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"Netlink");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"16 Mbit Rom");
//         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (LPARAM)"Japanese Modem");

         // Set Selected Cart Type
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_SETCURSEL, carttype, 0);

         // Set Default Cart File
         SetDlgItemText(hDlg, IDC_CARTEDIT, cartfilename);

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
               OPENFILENAME ofn;
               char tempstr[MAX_PATH];

               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;
               ofn.lpstrFilter = "Supported image files (*.cue, *.iso)\0*.cue;*.iso\0Cue files (*.cue)\0*.cue\0Iso files (*.iso)\0*.iso\0All Files (*.*)\0*.*\0";
               GetDlgItemText(hDlg, IDC_IMAGEEDIT, tempstr, MAX_PATH);
               ofn.lpstrFile = tempstr;
               ofn.nMaxFile = sizeof(tempstr);
               ofn.Flags = OFN_FILEMUSTEXIST;

               if (GetOpenFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_IMAGEEDIT, tempstr);
               }

               return TRUE;
            }
            case IDC_BIOSBROWSE:
            {
               OPENFILENAME ofn;
               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;
               ofn.lpstrFilter = "Binaries (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
               ofn.lpstrFile = biosfilename;
               ofn.nMaxFile = sizeof(biosfilename);
               ofn.Flags = OFN_FILEMUSTEXIST;

               if (GetOpenFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_BIOSEDIT, biosfilename);
               }

               return TRUE;
            }
            case IDC_BACKUPRAMBROWSE:
            {
               OPENFILENAME ofn;
               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;
               ofn.lpstrFilter = "Binaries (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
               ofn.lpstrFile = backupramfilename;
               ofn.nMaxFile = sizeof(backupramfilename);

               if (GetOpenFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_BACKUPRAMEDIT, backupramfilename);
               }

               return TRUE;
            }
            case IDC_MPEGROMBROWSE:
            {
               OPENFILENAME ofn;
               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;
               ofn.lpstrFilter = "Binaries (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
               ofn.lpstrFile = mpegromfilename;
               ofn.nMaxFile = sizeof(mpegromfilename);
               ofn.Flags = OFN_FILEMUSTEXIST;

               if (GetOpenFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_MPEGROMEDIT, mpegromfilename);
               }

               return TRUE;
            }
            case IDC_CARTBROWSE:
            {
               OPENFILENAME ofn;
               u8 cursel=0;

               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;
               ofn.lpstrFilter = "Binaries (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
               ofn.lpstrFile = cartfilename;
               ofn.nMaxFile = sizeof(cartfilename);

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
                  SetDlgItemText(hDlg, IDC_CARTEDIT, cartfilename);
               }

               return TRUE;
            }
            case IDOK:
            {
               char tempstr[MAX_PATH];
               char current_drive=0;
               BOOL imagebool;
               BOOL cdromchanged=FALSE;

               // Convert Dialog items back to variables
               GetDlgItemText(hDlg, IDC_BIOSEDIT, biosfilename, MAX_PATH);
               GetDlgItemText(hDlg, IDC_BACKUPRAMEDIT, backupramfilename, MAX_PATH);
               GetDlgItemText(hDlg, IDC_MPEGROMEDIT, mpegromfilename, MAX_PATH);
               carttype = (int)SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_GETCURSEL, 0, 0);
               GetDlgItemText(hDlg, IDC_CARTEDIT, cartfilename, MAX_PATH);

               // write path/filenames
               WritePrivateProfileString("General", "BiosPath", biosfilename, inifilename);
               WritePrivateProfileString("General", "BackupRamPath", backupramfilename, inifilename);
               WritePrivateProfileString("General", "MpegRomPath", mpegromfilename, inifilename);

               sprintf(tempstr, "%d", carttype);
               WritePrivateProfileString("General", "CartType", tempstr, inifilename);

               // figure out cart type, write cartfilename if necessary
               switch (carttype)
               {
                  case CART_PAR:
                  case CART_BACKUPRAM4MBIT:
                  case CART_BACKUPRAM8MBIT:
                  case CART_BACKUPRAM16MBIT:
                  case CART_BACKUPRAM32MBIT:
                  case CART_ROM16MBIT:
                     WritePrivateProfileString("General", "CartPath", cartfilename, inifilename);
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
                  GetDlgItemText(hDlg, IDC_IMAGEEDIT, tempstr, MAX_PATH);

                  if (strcmp(tempstr, cdrompath) != 0)
                  {
                     strcpy(cdrompath, tempstr);
                     cdromchanged = TRUE;
                  }
               }

               WritePrivateProfileString("General", "CDROMDrive", cdrompath, inifilename);

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

//               WritePrivateProfileString("General", "BiosLanguage", tempstr, inifilename);
*/
               // Write SH2 core type
               sh2coretype = (char)SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_GETCURSEL, 0, 0);

               sprintf(tempstr, "%d", sh2coretype);
               WritePrivateProfileString("General", "SH2Core", tempstr, inifilename);

               // Convert Combo Box ID to Region ID
               regionid = (char)SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_GETCURSEL, 0, 0);

               switch(regionid)
               {
                  case 0:
                     WritePrivateProfileString("General", "Region", "Auto", inifilename);
                     break;
                  case 1:
                     WritePrivateProfileString("General", "Region", "J", inifilename);
                     break;
                  case 2:
                     WritePrivateProfileString("General", "Region", "T", inifilename);
                     break;
                  case 3:
                     WritePrivateProfileString("General", "Region", "U", inifilename);
                     regionid = 4;
                     break;
                  case 4:
                     WritePrivateProfileString("General", "Region", "B", inifilename);
                     regionid = 5;
                     break;
                  case 5:
                     WritePrivateProfileString("General", "Region", "K", inifilename);
                     regionid = 6;
                     break;
                  case 6:
                     WritePrivateProfileString("General", "Region", "A", inifilename);
                     regionid = 0xA;
                     break;
                  case 7:
                     WritePrivateProfileString("General", "Region", "E", inifilename);
                     regionid = 0xC;
                     break;
                  case 8:
                     WritePrivateProfileString("General", "Region", "L", inifilename);
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
         SendDlgItemMessage(hDlg, IDC_VIDEOCORECB, CB_ADDSTRING, 0, (LPARAM)"None");

         for (i = 1; VIDCoreList[i] != NULL; i++)
            SendDlgItemMessage(hDlg, IDC_VIDEOCORECB, CB_ADDSTRING, 0, (LPARAM)VIDCoreList[i]->Name);

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
               if (SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_FINDSTRINGEXACT, 0, (LPARAM)tempstr) == CB_ERR)
               {
                  index = (int)SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_ADDSTRING, 0, (LPARAM)tempstr);

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
         SetDlgItemText(hDlg, IDC_WIDTHEDIT, tempstr);
         sprintf(tempstr, "%d", windowheight);
         SetDlgItemText(hDlg, IDC_HEIGHTEDIT, tempstr);

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
                  WritePrivateProfileString("Video", "VideoCore", tempstr, inifilename);
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
               WritePrivateProfileString("Video", "AutoFrameSkip", tempstr, inifilename);

               // Write full screen on startup setting
               if (SendDlgItemMessage(hDlg, IDC_FULLSCREENSTARTUPCB, BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  usefullscreenonstartup = 1;
                  WritePrivateProfileString("Video", "UseFullScreenOnStartup", "1", inifilename);
               }
               else
               {
                  usefullscreenonstartup = 0;
                  WritePrivateProfileString("Video", "UseFullScreenOnStartup", "0", inifilename);
               }

               // Write full screen size settings
               cursel = (int)SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_GETCURSEL, 0, 0);
               if (SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_GETLBTEXTLEN, cursel, 0) <= MAX_PATH)
               {
                  SendDlgItemMessage(hDlg, IDC_FSSIZECB, CB_GETLBTEXT, cursel, (LPARAM)tempstr);
                  sscanf(tempstr, "%dx%d", &fullscreenwidth, &fullscreenheight);
               }

               sprintf(tempstr, "%d", fullscreenwidth);
               WritePrivateProfileString("Video", "FullScreenWidth", tempstr, inifilename);
               sprintf(tempstr, "%d", fullscreenheight);
               WritePrivateProfileString("Video", "FullScreenHeight", tempstr, inifilename);

               // Write use custom window size setting
               if (SendDlgItemMessage(hDlg, IDC_CUSTOMWINDOWCB, BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  usecustomwindowsize = 1;
                  WritePrivateProfileString("Video", "UseCustomWindowSize", "1", inifilename);
               }
               else
               {
                  usecustomwindowsize = 0;
                  WritePrivateProfileString("Video", "UseCustomWindowSize", "0", inifilename);
               }
               
               // Write window width and height settings
               GetDlgItemText(hDlg, IDC_WIDTHEDIT, tempstr, MAX_PATH);
               windowwidth = atoi(tempstr);
               WritePrivateProfileString("Video", "WindowWidth", tempstr, inifilename);
               GetDlgItemText(hDlg, IDC_HEIGHTEDIT, tempstr, MAX_PATH);
               windowheight = atoi(tempstr);
               WritePrivateProfileString("Video", "WindowHeight", tempstr, inifilename);

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
         SendDlgItemMessage(hDlg, IDC_SOUNDCORECB, CB_ADDSTRING, 0, (LPARAM)"None");

         for (i = 1; SNDCoreList[i] != NULL; i++)
            SendDlgItemMessage(hDlg, IDC_SOUNDCORECB, CB_ADDSTRING, 0, (LPARAM)SNDCoreList[i]->Name);

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
                  WritePrivateProfileString("Sound", "SoundCore", tempstr, inifilename);
               }

               // Write Volume
               sndvolume = (int)SendDlgItemMessage(hDlg, IDC_SLVOLUME, TBM_GETPOS, 0, 0);
               sprintf(tempstr, "%d", sndvolume);
               WritePrivateProfileString("Sound", "Volume", tempstr, inifilename);
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
   char tempstr[MAX_PATH];

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         SendDlgItemMessage(hDlg, IDC_LOCALREMOTEIP, IPM_SETADDRESS, 0, netlinklocalremoteip);

         sprintf(tempstr, "%d", netlinkport);
         SetDlgItemText(hDlg, IDC_PORTET, tempstr);

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
               WritePrivateProfileString("Netlink", "LocalRemoteIP", tempstr, inifilename);

               // Port Number
               GetDlgItemText(hDlg, IDC_PORTET, tempstr, MAX_PATH);
               netlinkport = atoi(tempstr);
               WritePrivateProfileString("Netlink", "Port", tempstr, inifilename);
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
         SendDlgItemMessage(hDlg, IDC_PORT1CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)"None");
         SendDlgItemMessage(hDlg, IDC_PORT1CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)"Direct");
         SendDlgItemMessage(hDlg, IDC_PORT1CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)"6-port Multitap");

         SendDlgItemMessage(hDlg, IDC_PORT2CONNTYPECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_PORT2CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)"None");
         SendDlgItemMessage(hDlg, IDC_PORT2CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)"Direct");
         SendDlgItemMessage(hDlg, IDC_PORT2CONNTYPECB, CB_ADDSTRING, 0, (LPARAM)"6-port Multitap");

         for (i = 0; i < 6; i++)
         {
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"None");
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"Standard Pad");
/*
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"Analog Pad");
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"Stunner");
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"Mouse");
            SendDlgItemMessage(hDlg, IDC_PORT1ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"Keyboard");
*/
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_RESETCONTENT, 0, 0);
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"None");
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"Standard Pad");
/*
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"Analog Pad");
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"Stunner");
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"Mouse");
            SendDlgItemMessage(hDlg, IDC_PORT2ATYPECB+i, CB_ADDSTRING, 0, (LPARAM)"Keyboard");
*/
            // finish me
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
               int id;
               switch (pad[j]->perid)
               {
                  case 0x02: // Standard Pad
                     id = 1;
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
                     DialogBoxParam(y_hInstance, MAKEINTRESOURCE(IDD_PADCONFIG), hDlg, (DLGPROC)PadConfigDlgProc, (LPARAM)LOWORD(wParam)-IDC_PORT1ACFGPB);
                     break;
                  default: break;
               }
               return TRUE;
            case IDOK:
            {
               u32 i;
               char string1[13], string2[3];

               for (i = 0; i < numpads; i++)
               {
                  sprintf(string1, "Peripheral%d", i+1);
                  sprintf(string2, "%d", ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PORT1ATYPECB+i)));
                  WritePrivateProfileString(string1, "EmulateType", string2, inifilename);
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
      SetDlgItemText(hDlg, controlid, buttonname);
      controlmap[controlid - basecontrolid] = ret;
   }
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK PadConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
   static u8 cursel;
   static int controlmap[13];
   static int padnum;
   int i;

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         PERDXListDevices(GetDlgItem(hDlg, IDC_DXDEVICECB));
         padnum = (int)lParam;

         // Load settings from ini here, if necessary
         PERDXInitControlConfig(hDlg, padnum, controlmap, inifilename);

         cursel = (u8)SendDlgItemMessage(hDlg, IDC_DXDEVICECB, CB_GETCURSEL, 0, 0);
         if (cursel == 0)
         {
            // Disable all the controls
            EnableWindow(GetDlgItem(hDlg, IDC_UPPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_DOWNPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_LEFTPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_RIGHTPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_LPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_RPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_STARTPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_APB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_XPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_YPB), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ZPB), FALSE);
         }
         else
         {
            // Enable all the controls
            EnableWindow(GetDlgItem(hDlg, IDC_UPPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_DOWNPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_LEFTPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_RIGHTPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_LPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_RPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_STARTPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_APB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_BPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_XPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_YPB), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_ZPB), TRUE);
         }

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
                  WritePrivateProfileString(string1, pad_names2[i], string2, inifilename);
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
                     if (cursel == 0)
                     {
                        // Disable all the controls
                        EnableWindow(GetDlgItem(hDlg, IDC_UPPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_DOWNPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_LEFTPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_RIGHTPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_LPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_RPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_STARTPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_APB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_CPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_XPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_YPB), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_ZPB), FALSE);
                     }
                     else
                     {
                        // Enable all the controls
                        EnableWindow(GetDlgItem(hDlg, IDC_UPPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_DOWNPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_LEFTPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_RIGHTPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_LPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_RPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_STARTPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_APB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_BPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_CPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_XPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_YPB), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_ZPB), TRUE);
                     }
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
         if (uselog)
         {
            SendDlgItemMessage(hDlg, IDC_USELOGCB, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(hDlg, WM_COMMAND, IDC_USELOGCB, 0);
         }
         else
            SendDlgItemMessage(hDlg, IDC_USELOGCB, BM_SETCHECK, BST_UNCHECKED, 0);

         SendMessage(hDlg, WM_COMMAND, IDC_USELOGCB, 0);

         // Setup log type setting
         SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_ADDSTRING, 0, (LPARAM)"Write to File");
         SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_ADDSTRING, 0, (LPARAM)"Write to Window");
         SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_SETCURSEL, logtype, 0);

         // Setup log filename setting
         SetDlgItemText(hDlg, IDC_LOGFILENAMEET, logfilename);

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_USELOGCB:
            {
               if (SendDlgItemMessage(hDlg, IDC_USELOGCB, BM_GETCHECK, 0, 0) == BST_CHECKED)
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
               OPENFILENAME ofn;

               // setup ofn structure
               ZeroMemory(&ofn, sizeof(ofn));
               ofn.lStructSize = sizeof(ofn);
               ofn.hwndOwner = hDlg;
               ofn.lpstrFilter = "All Files\0*.*\0Text Files\0*.txt\0";
               ofn.nFilterIndex = 1;
               ofn.lpstrFile = logfilename;
               ofn.nMaxFile = sizeof(logfilename);
               ofn.Flags = OFN_OVERWRITEPROMPT;
 
               if (GetSaveFileName(&ofn))
               {
                  SetDlgItemText(hDlg, IDC_LOGFILENAMEET, logfilename);
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
               WritePrivateProfileString("Log", "Enable", tempstr, inifilename);

               // Write log type setting
               logtype = (int)SendDlgItemMessage(hDlg, IDC_LOGTYPECB, CB_GETCURSEL, 0, 0);
               sprintf(tempstr, "%d", logtype);
               WritePrivateProfileString("Log", "Type", tempstr, inifilename);

               // Write log filename
               WritePrivateProfileString("Log", "Filename", logfilename, inifilename);

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
      SendDlgItemMessage(hDlg, IDC_BUPSAVELB, LB_ADDSTRING, 0, (LPARAM)saves[i].filename);

   BupGetStats(currentbupdevice, &freespace, &maxspace);
   sprintf(tempstr, "%d/%d blocks free", (int)freespace, (int)maxspace);
   SetDlgItemText(hDlg, IDC_BUPFREESPACELT, tempstr);                     
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
            SendDlgItemMessage(hDlg, IDC_BUPDEVICECB, CB_ADDSTRING, 0, (LPARAM)devices[i].name);

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

                     SendDlgItemMessage(hDlg, IDC_BUPSAVELB, LB_GETTEXT, cursel, (LPARAM)tempstr);

                     for (i = 0; i < numsaves; i++)
                     {
                        if (strcmp(tempstr, saves[i].filename) == 0)
                        {
                           cursel = i;
                           break;
                        }
                     }

                     SetDlgItemText(hDlg, IDC_BUPFILENAMEET, saves[cursel].filename);
                     SetDlgItemText(hDlg, IDC_BUPCOMMENTET, saves[cursel].comment);
                     switch(saves[cursel].language)
                     {
                        case 0:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, "Japanese");
                           break;
                        case 1:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, "English");
                           break;
                        case 2:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, "French");
                           break;
                        case 3:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, "German");
                           break;
                        case 4:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, "Spanish");
                           break;
                        case 5:
                           SetDlgItemText(hDlg, IDC_BUPLANGUAGEET, "Italian");
                           break;
                        default: break;
                     }
                     sprintf(tempstr, "%d", (int)saves[cursel].datasize);
                     SetDlgItemText(hDlg, IDC_BUPDATASIZEET, tempstr);
                     sprintf(tempstr, "%d", saves[cursel].blocksize);
                     SetDlgItemText(hDlg, IDC_BUPBLOCKSIZEET, tempstr);
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
               if (MessageBox (hDlg, tempstr2, "Confirm Delete",  MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
               {
                  BupDeleteSave(currentbupdevice, tempstr);
                  RefreshSaveList(hDlg);
               }
               return TRUE;
            }
            case IDC_BUPFORMATBT:
            {
               sprintf(tempstr, "Are you sure you want to format %s?", devices[currentbupdevice].name);
               if (MessageBox (hDlg, tempstr, "Confirm Delete",  MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
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
      ti.lpszText = hb[i].string;
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
