/*  Copyright 2004 Theo Berkau

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
#include <stdio.h>
#include <ctype.h>
#include "resource.h"

char biosfilename[MAX_PATH] = "\0";
char cdrompath[MAX_PATH]="\0";
char backupramfilename[MAX_PATH] = "\0";
char mpegromfilename[MAX_PATH] = "\0";
char inifilename[MAX_PATH];

int num_cdroms=0;
char drive_list[24];
char bioslang=0;
unsigned char regionid=0;

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
      sprintf(tempstr, "%c:\\\0", 'c' + i);

      if (GetDriveType(tempstr) == DRIVE_CDROM)
      {
         drive_list[num_cdroms] = 'c' + i;

         sprintf(tempstr, "%c\0", 'C' + i);
         SendDlgItemMessage(hWnd, IDC_DRIVELETTERCB, CB_ADDSTRING, 0, (long)tempstr);
         num_cdroms++;
      } 
   }
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         DWORD fflags;
         BOOL imagebool=FALSE;
         char current_drive=0;
         int i;

         // Setup Combo Boxes

         // Disc Type Box
         // Disabled for now
         EnableWindow(HWND(GetDlgItem(hDlg, IDC_DISCTYPECB)), FALSE);
         SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_ADDSTRING, 0, (long)"CD");
         SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_ADDSTRING, 0, (long)"Image");

         // Drive Letter Box
         SendDlgItemMessage(hDlg, IDC_DRIVELETTERCB, CB_RESETCONTENT, 0, 0);

         // figure out which drive letters are available
         GenerateCDROMList(hDlg);

         // Set Disc Type Selection
         fflags = GetFileAttributes(cdrompath);

         if (fflags != INVALID_FILE_ATTRIBUTES)
         {
            if (fflags & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY) ==
                (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY))
            {
               // Almost sure this is a cdrom drive path, need to do
               // a few more checks
//               if (GetDriveType(cdrompath)
               current_drive = cdrompath[0];
               imagebool = FALSE; // FIX ME
            }
            else
            {
               // Assume it's a file

               current_drive = 0;
               imagebool = FALSE; // FIX ME
            }
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
            EnableWindow(HWND(GetDlgItem(hDlg, IDC_IMAGEEDIT)), FALSE);
            EnableWindow(HWND(GetDlgItem(hDlg, IDC_IMAGEBROWSE)), FALSE);

            EnableWindow(HWND(GetDlgItem(hDlg, IDC_DRIVELETTERCB)), TRUE);
         }
         else
         {
            EnableWindow(HWND(GetDlgItem(hDlg, IDC_IMAGEEDIT)), TRUE);
            EnableWindow(HWND(GetDlgItem(hDlg, IDC_IMAGEBROWSE)), TRUE);
            SetDlgItemText(hDlg, IDC_IMAGEEDIT, cdrompath);

            EnableWindow(HWND(GetDlgItem(hDlg, IDC_DRIVELETTERCB)), FALSE);
         }

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
         EnableWindow(HWND(GetDlgItem(hDlg, IDC_BIOSLANGCB)), FALSE);

         // Setup Region Combo box
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (long)"Auto-detect");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (long)"Japan(NTSC)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (long)"Asia(NTSC)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (long)"North America(NTSC)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (long)"Central/South America(NTSC)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (long)"Korea(NTSC)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (long)"Asia(PAL)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (long)"Europe + others(PAL)");
         SendDlgItemMessage(hDlg, IDC_REGIONCB, CB_ADDSTRING, 0, (long)"Central/South America(PAL)");

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
                     unsigned char cursel=0;

                     cursel = (unsigned char)SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_GETCURSEL, 0, 0);

                     if (cursel == 0)
                     {
                        EnableWindow(HWND(GetDlgItem(hDlg, IDC_IMAGEEDIT)), FALSE);
                        EnableWindow(HWND(GetDlgItem(hDlg, IDC_IMAGEBROWSE)), FALSE);

                        EnableWindow(HWND(GetDlgItem(hDlg, IDC_DRIVELETTERCB)), TRUE);
                     }
                     else
                     {
                        EnableWindow(HWND(GetDlgItem(hDlg, IDC_IMAGEEDIT)), TRUE);
                        EnableWindow(HWND(GetDlgItem(hDlg, IDC_IMAGEBROWSE)), TRUE);

                        EnableWindow(HWND(GetDlgItem(hDlg, IDC_DRIVELETTERCB)), FALSE);
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
               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;
               ofn.lpstrFilter = "Cue files (*.cue)\0*.cue\0All Files (*.*)\0*.*\0";
               ofn.lpstrFile = cdrompath;
               ofn.nMaxFile = sizeof(cdrompath);
               ofn.Flags = OFN_FILEMUSTEXIST;

               if (GetOpenFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_IMAGEEDIT, cdrompath);
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
/*
               OPENFILENAME ofn;
               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hDlg;
               ofn.lpstrFilter = "Executables (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
               ofn.lpstrFile = girigirifilename;
               ofn.nMaxFile = sizeof(girigirifilename);
//               ofn.lpstrInitialDir = gszInitialDir;
//               ofn.Flags = OFN_FILEMUSTEXIST;

               if (GetSaveFileName(&ofn))
               {
                  // adjust appropriate edit box
                  SetDlgItemText(hDlg, IDC_BACKUPRAMEDIT, backupramfilename);
               }
*/
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

            case IDC_VIDEOSETTINGS:
            {
//               DialogBox(g_hInstance, "VideoSettingsDlg", hDlg, (DLGPROC)VideoSettingsDlgProc);

               return TRUE;
            }
            case IDC_INPUTSETTINGS:
            {
//               DialogBox(g_hInstance, "InputSettingsDlg", hDlg, (DLGPROC)InputSettingsDlgProc);

               return TRUE;
            }
            case IDOK:
            {
               char tempstr[MAX_PATH];
               char current_drive=0;
               BOOL imagebool;

               // Convert Dialog items back to variables
               GetDlgItemText(hDlg, IDC_BIOSEDIT, biosfilename, MAX_PATH);
               GetDlgItemText(hDlg, IDC_BACKUPRAMEDIT, backupramfilename, MAX_PATH);
               GetDlgItemText(hDlg, IDC_MPEGROMEDIT, mpegromfilename, MAX_PATH);

               // write path/filenames
               WritePrivateProfileString("General", "BiosPath", biosfilename, inifilename);
               WritePrivateProfileString("General", "BackupRamPath", backupramfilename, inifilename);
               WritePrivateProfileString("General", "MpegRomPath", mpegromfilename, inifilename);

               imagebool = (BOOL)SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_GETCURSEL, 0, 0);

//               if (imagebool == FALSE)
//               {
                  // convert drive letter to string
                  current_drive = (char)SendDlgItemMessage(hDlg, IDC_DRIVELETTERCB, CB_GETCURSEL, 0, 0);
                  sprintf(cdrompath, "%c:\0", toupper(drive_list[current_drive]));
                  WritePrivateProfileString("General", "CDROMDrive", cdrompath, inifilename);
//               }
//               else
//               {
//                  // retrieve image filename string instead
//                  GetDlgItemText(hDlg, IDC_IMAGEEDIT, cdrompath, MAX_PATH);
//                  WritePrivateProfileString("General", "CDROMDrive", cdrompath, inifilename);
//               }

               // Convert ID to language string
               bioslang = (char)SendDlgItemMessage(hDlg, IDC_BIOSLANGCB, CB_GETCURSEL, 0, 0);

               switch (bioslang)
               {
                  case 0:
                     sprintf(tempstr, "English\0");
                     break;
                  case 1:
                     sprintf(tempstr, "German\0");
                     break;
                  case 2:
                     sprintf(tempstr, "French\0");
                     break;
                  case 3:
                     sprintf(tempstr, "Spanish\0");
                     break;
                  case 4:
                     sprintf(tempstr, "Italian\0");
                     break;
                  case 5:
                     sprintf(tempstr, "Japanese\0");
                     break;
                  default:break;
               }

//               WritePrivateProfileString("General", "BiosLanguage", tempstr, inifilename);

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
