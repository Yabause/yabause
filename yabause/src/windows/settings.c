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
#include <stdio.h>
#include <ctype.h>
#include "../cs0.h"
#include "resource.h"

char biosfilename[MAX_PATH] = "\0";
char cdrompath[MAX_PATH]="\0";
char backupramfilename[MAX_PATH] = "\0";
char mpegromfilename[MAX_PATH] = "\0";
char cartfilename[MAX_PATH] = "\0";
char inifilename[MAX_PATH];

int num_cdroms=0;
char drive_list[24];
char bioslang=0;
char sh2coretype=0;
u8 regionid=0;
int disctype;
int carttype;

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
         SendDlgItemMessage(hWnd, IDC_DRIVELETTERCB, CB_ADDSTRING, 0, (long)tempstr);
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

LRESULT CALLBACK SettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam)
{
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
         SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_ADDSTRING, 0, (long)"CD");
         SendDlgItemMessage(hDlg, IDC_DISCTYPECB, CB_ADDSTRING, 0, (long)"Image");

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
         SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_ADDSTRING, 0, (long)"Fast Interpreter");
         SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_ADDSTRING, 0, (long)"Debug Interpreter");

         // Set Selected SH2 Core
         SendDlgItemMessage(hDlg, IDC_SH2CORECB, CB_SETCURSEL, sh2coretype, 0); // fix me

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

         // Setup Cart Type Combo box
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_RESETCONTENT, 0, 0);
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (long)"None");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (long)"Pro Action Replay");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (long)"4 Mbit Backup Ram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (long)"8 Mbit Backup Ram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (long)"16 Mbit Backup Ram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (long)"32 Mbit Backup Ram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (long)"8 Mbit Dram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (long)"32 Mbit Dram");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (long)"Netlink");
         SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_ADDSTRING, 0, (long)"16 Mbit Rom");

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
            case IDC_VIDEOSETTINGS:
            {
//               DialogBox(g_hInstance, "VideoSettingsDlg", hDlg, (DLGPROC)VideoSettingsDlgProc);

               return TRUE;
            }
            case IDC_SOUNDSETTINGS:
            {
//               DialogBox(g_hInstance, "SoundSettingsDlg", hDlg, (DLGPROC)SoundSettingsDlgProc);

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
               carttype = SendDlgItemMessage(hDlg, IDC_CARTTYPECB, CB_GETCURSEL, 0, 0);
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
                  sprintf(cdrompath, "%c:", toupper(drive_list[current_drive]));
                  WritePrivateProfileString("General", "CDROMDrive", cdrompath, inifilename);
               }
               else
               {
                  // retrieve image filename string instead
                  GetDlgItemText(hDlg, IDC_IMAGEEDIT, cdrompath, MAX_PATH);
                  WritePrivateProfileString("General", "CDROMDrive", cdrompath, inifilename);
               }
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
