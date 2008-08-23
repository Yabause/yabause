/*  Copyright 2007 Theo Berkau

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
#include "cheats.h"
#include "../cheat.h"
#include "resource.h"
#include "../memory.h"
#include "yuidebug.h"

extern HINSTANCE y_hInstance;

char cheatfilename[MAX_PATH] = "\0";

//////////////////////////////////////////////////////////////////////////////

void AddCode(HWND hParent, int i)
{
   LVITEM itemdata;
   char code[MAX_PATH];
   cheatlist_struct *cheat;
   int cheatnum;   

   cheat = CheatGetList(&cheatnum);

   switch(cheat[i].type)
   {
      case CHEATTYPE_ENABLE:
         sprintf(code, "Enable code: %08X %08X", (int)cheat[i].addr, (int)cheat[i].val);
         break;
      case CHEATTYPE_BYTEWRITE:
         sprintf(code, "Byte Write: %08X %02X", (int)cheat[i].addr, (int)cheat[i].val);
         break;
      case CHEATTYPE_WORDWRITE:
         sprintf(code, "Word write: %08X %04X", (int)cheat[i].addr, (int)cheat[i].val);
         break;
      case CHEATTYPE_LONGWRITE:
         sprintf(code, "Long write: %08X %08X", (int)cheat[i].addr, (int)cheat[i].val);
         break;
      default: break;
   }

   itemdata.mask = LVIF_TEXT;
   itemdata.iItem = (int)SendDlgItemMessage(hParent, IDC_CHEATLIST, LVM_GETITEMCOUNT, 0, 0);
   itemdata.iSubItem = 0;
   itemdata.pszText = (LPTSTR)code;
   itemdata.cchTextMax = (int)strlen(itemdata.pszText);
   SendDlgItemMessage(hParent, IDC_CHEATLIST, LVM_INSERTITEM, 0, (LPARAM)&itemdata);

   itemdata.iSubItem = 1;
   itemdata.pszText = (LPTSTR)cheat[i].desc;
   itemdata.cchTextMax = (int)strlen(itemdata.pszText);
   SendDlgItemMessage(hParent, IDC_CHEATLIST, LVM_SETITEM, 0, (LPARAM)&itemdata);

   itemdata.iSubItem = 2;
   if (cheat[i].enable)
      itemdata.pszText = "Enabled";
   else
      itemdata.pszText = "Disabled";
      
   itemdata.cchTextMax = (int)strlen(itemdata.pszText);
   SendDlgItemMessage(hParent, IDC_CHEATLIST, LVM_SETITEM, 0, (LPARAM)&itemdata);
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK AddARCodeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
         SendDlgItemMessage(hDlg, IDC_CODE, EM_LIMITTEXT, 13, 0);
         Button_Enable(GetDlgItem(hDlg, IDOK), FALSE);
         return TRUE;
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
               char code[MAX_PATH];
               char desc[MAX_PATH];
               int cheatnum;

               GetDlgItemText(hDlg, IDC_CODE, code, 14);

               // Should verify text
               if (strlen(code) < 12)
               {
                   MessageBox (hDlg, "Invalid code. Should be in the format: XXXXXXXX YYYY", "Error",  MB_OK | MB_ICONINFORMATION);
                   return TRUE;
               }

               if (CheatAddARCode(code) != 0)
               {
                   MessageBox (hDlg, "Invalid code. Should be in the format: XXXXXXXX YYYY", "Error",  MB_OK | MB_ICONINFORMATION);
                   return TRUE;
               }

               GetDlgItemText(hDlg, IDC_CODEDESC, desc, MAX_PATH);
               CheatGetList(&cheatnum);
               CheatChangeDescriptionByIndex(cheatnum-1, desc);
               AddCode(GetParent(hDlg), cheatnum-1);

               EnableWindow(GetDlgItem(GetParent(hDlg), IDC_CLEARCODES), TRUE);
               EnableWindow(GetDlgItem(GetParent(hDlg), IDC_SAVETOFILE), TRUE);

               EndDialog(hDlg, TRUE);
               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            case IDC_CODE:
            {
               if (HIWORD(wParam) == EN_CHANGE)
               {
                  char text[14];
                  LRESULT ret;

                  if ((ret = GetDlgItemText(hDlg, IDC_CODEDESC, text, 14)) <= 0)
                     Button_Enable(GetDlgItem(hDlg, IDOK), FALSE);
                  else 
                     Button_Enable(GetDlgItem(hDlg, IDOK), TRUE);
               }
               return TRUE;
            }
            default: break;
         }
         break;
      }
      default: break;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK AddCodeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
         SendDlgItemMessage(hDlg, IDC_CODEADDR, EM_LIMITTEXT, 8, 0);
         return TRUE;
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
               char tempstr[MAX_PATH];
               char desc[MAX_PATH];
               int type;
               u32 addr;
               u32 val;
               int cheatnum;

               // Get address
               GetDlgItemText(hDlg, IDC_CODEADDR, tempstr, 9);
               if (sscanf(tempstr, "%08lX", &addr) != 1)
               {
                  MessageBox (hDlg, "Invalid Address", "Error",  MB_OK | MB_ICONINFORMATION);
                  return TRUE;
               }

               // Get value
               GetDlgItemText(hDlg, IDC_CODEVAL, tempstr, 11);
               if (sscanf(tempstr, "%ld", &val) != 1)
               {
                  MessageBox (hDlg, "Invalid Value", "Error",  MB_OK | MB_ICONINFORMATION);
                  return TRUE;
               }

               // Get type
               if (SendDlgItemMessage(hDlg, IDC_CTENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                  type = CHEATTYPE_ENABLE;
               else if (SendDlgItemMessage(hDlg, IDC_CTBYTEWRITE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                  type = CHEATTYPE_BYTEWRITE;
               else if (SendDlgItemMessage(hDlg, IDC_CTWORDWRITE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                  type = CHEATTYPE_WORDWRITE;
               else
                  type = CHEATTYPE_LONGWRITE;

               if (CheatAddCode(type, addr, val) != 0)
               {
                   MessageBox (hDlg, "Unable to add code", "Error",  MB_OK | MB_ICONINFORMATION);
                   return TRUE;
               }

               GetDlgItemText(hDlg, IDC_CODEDESC, desc, MAX_PATH);
               CheatGetList(&cheatnum);
               CheatChangeDescriptionByIndex(cheatnum-1, desc);
               AddCode(GetParent(hDlg), cheatnum-1);

               EnableWindow(GetDlgItem(GetParent(hDlg), IDC_CLEARCODES), TRUE);
               EnableWindow(GetDlgItem(GetParent(hDlg), IDC_SAVETOFILE), TRUE);

               EndDialog(hDlg, TRUE);
               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);
               return TRUE;
            }
            case IDC_CTENABLE:
            {
               if (HIWORD(wParam) == BN_CLICKED)
                  SendDlgItemMessage(hDlg, IDC_CODEVAL, EM_LIMITTEXT, 10, 0);
               break;
            }
            case IDC_CTBYTEWRITE:
            {
               if (HIWORD(wParam) == BN_CLICKED)
                  SendDlgItemMessage(hDlg, IDC_CODEVAL, EM_LIMITTEXT, 3, 0);
               break;
            }
            case IDC_CTWORDWRITE:
            {
               if (HIWORD(wParam) == BN_CLICKED)
                  SendDlgItemMessage(hDlg, IDC_CODEVAL, EM_LIMITTEXT, 5, 0);
               break;
            }
            case IDC_CTLONGWRITE:
            {
               if (HIWORD(wParam) == BN_CLICKED)
                  SendDlgItemMessage(hDlg, IDC_CODEVAL, EM_LIMITTEXT, 10, 0);
               break;
            }
            default: break;
         }
         break;
      }
      default: break;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK CheatListDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
   int cheatnum;
   int i;

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         LVCOLUMN coldata;

         ListView_SetExtendedListViewStyleEx(GetDlgItem(hDlg, IDC_CHEATLIST),
                                             LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

         coldata.mask = LVCF_TEXT | LVCF_WIDTH;
         coldata.pszText = "Code\0";
         coldata.cchTextMax = (int)strlen(coldata.pszText);
         coldata.cx = 190;
         SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_INSERTCOLUMN, (WPARAM)0, (LPARAM)&coldata);

         coldata.pszText = "Description\0";
         coldata.cchTextMax = (int)strlen(coldata.pszText);
         coldata.cx = 111;
         SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_INSERTCOLUMN, (WPARAM)1, (LPARAM)&coldata);

         coldata.pszText = "Status\0";
         coldata.cchTextMax = (int)strlen(coldata.pszText);
         coldata.cx = 70;
         SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_INSERTCOLUMN, (WPARAM)2, (LPARAM)&coldata);

         // Generate cheat list
         CheatGetList(&cheatnum);

         for (i = 0; i < cheatnum; i++)
         {
            // Add code to code list
            AddCode(hDlg, i);
         }

         if (cheatnum == 0)
         {
            EnableWindow(GetDlgItem(hDlg, IDC_CLEARCODES), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_SAVETOFILE), FALSE);
         }
         else
            EnableWindow(GetDlgItem(hDlg, IDC_SAVETOFILE), TRUE);

         EnableWindow(GetDlgItem(hDlg, IDC_DELETECODE), FALSE);
         EnableWindow(GetDlgItem(hDlg, IDC_ADDFROMFILE), TRUE);

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_ADDAR:
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_ADDARCODE), hDlg, (DLGPROC)AddARCodeDlgProc);
               break;
            case IDC_ADDRAWMEMADDR:
               DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_ADDCODE), hDlg, (DLGPROC)AddCodeDlgProc);
               break;
            case IDC_SAVETOFILE:
            {
               OPENFILENAME ofn;

               SetupOFN(&ofn, OFN_DEFAULTSAVE, hDlg,
                        "Yabause Cheat Files\0*.YCT\0All Files\0*.*\0",
                        cheatfilename, sizeof(cheatfilename));
               ofn.lpstrDefExt = "YCT";

               if (GetSaveFileName(&ofn))
               {
                  if (CheatSave(cheatfilename) != 0)
                     MessageBox (hDlg, "Unable to open file for saving", "Error",  MB_OK | MB_ICONINFORMATION);
               }
               break;
            }
            case IDC_ADDFROMFILE:
            {
               OPENFILENAME ofn;

               // setup ofn structure
               SetupOFN(&ofn, OFN_DEFAULTLOAD, hDlg,
                        "Yabause Cheat Files\0*.YCT\0All Files\0*.*\0",
                        cheatfilename, sizeof(cheatfilename));

               if (GetOpenFileName(&ofn))
               {
                  if (CheatLoad(cheatfilename) == 0)
                  {
                     EnableWindow(GetDlgItem(GetParent(hDlg), IDC_SAVETOFILE), TRUE);
                     SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_DELETEALLITEMS, 0, 0);

                     // Generate cheat list
                     CheatGetList(&cheatnum);

                     for (i = 0; i < cheatnum; i++)
                     {
                        // Add code to code list
                        AddCode(hDlg, i);
                     }

                     if (cheatnum == 0)
                     {
                        EnableWindow(GetDlgItem(hDlg, IDC_CLEARCODES), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_SAVETOFILE), FALSE);
                     }
                     else
                     {
                        EnableWindow(GetDlgItem(hDlg, IDC_CLEARCODES), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_SAVETOFILE), TRUE);
                     }
                  }
                  else
                     MessageBox (hDlg, "Unable to open file for saving", "Error",  MB_OK | MB_ICONINFORMATION);
               }

               break;
            }
            case IDC_DELETECODE:
            {
               int cursel=(int)SendDlgItemMessage(hDlg, IDC_CHEATLIST,
                                                  LVM_GETNEXTITEM, -1,
                                                  LVNI_SELECTED);
               if (cursel != -1)
               {
                  if (CheatRemoveCodeByIndex(cursel) != 0)
                  {
                     MessageBox (hDlg, "Unable to remove code", "Error",  MB_OK | MB_ICONINFORMATION);
                     return TRUE;
                  }

                  SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_DELETEITEM, cursel, 0);
               }
               break;
            }
            case IDC_CLEARCODES:
               CheatClearCodes();
               SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_DELETEALLITEMS, 0, 0);
               EnableWindow(GetDlgItem(GetParent(hDlg), IDC_CLEARCODES), FALSE);
               EnableWindow(GetDlgItem(GetParent(hDlg), IDC_DELETECODE), FALSE);
               EnableWindow(GetDlgItem(GetParent(hDlg), IDC_SAVETOFILE), FALSE);
               break;
            case IDCANCEL:
            case IDOK:
            {
               EndDialog(hDlg, TRUE);
               return TRUE;
            }
            default: break;
         }
         break;
      }
      case WM_NOTIFY:
      {
         NMHDR *hdr;

         hdr = (NMHDR *)lParam;

         switch (hdr->idFrom)
         {
            case IDC_CHEATLIST:
            {
               switch(hdr->code)
               {
                  case NM_DBLCLK:
                  {
                     int cursel=(int)SendDlgItemMessage(hDlg, IDC_CHEATLIST,
                                                        LVM_GETNEXTITEM, -1,
                                                        LVNI_SELECTED);

                     if (cursel != -1)
                     {                        
                        // Enable or disable code
                        LVITEM itemdata;
                        char tempstr[MAX_PATH];

                        itemdata.mask = LVIF_TEXT;
                        itemdata.iItem = cursel;
                        itemdata.iSubItem = 2;
                        itemdata.pszText = tempstr;
                        itemdata.cchTextMax = MAX_PATH;
                        SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_GETITEM, 0, (LPARAM)&itemdata);
                        if (strcmp(tempstr, "Enabled") == 0)
                        {
                           CheatDisableCode(cursel);
                           strcpy(tempstr, "Disabled");
                        }
                        else
                        {
                           CheatEnableCode(cursel);
                           strcpy(tempstr, "Enabled");
                        }

                        SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_SETITEM, 0, (LPARAM)&itemdata);
                     }

                     break;
                  }
                  case NM_CLICK:
                  {
                     int cursel=(int)SendDlgItemMessage(hDlg, IDC_CHEATLIST,
                                                        LVM_GETNEXTITEM, -1,
                                                        LVNI_SELECTED);
                     if (cursel != -1)
                        EnableWindow(GetDlgItem(hDlg, IDC_DELETECODE), TRUE);
                     else
                        EnableWindow(GetDlgItem(hDlg, IDC_DELETECODE), FALSE);

                     break;
                  }
                  default:
                     break;
               }
               break;
            }
            default: break;
         }

         return 0L;
      }

      default: break;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

static result_struct *cheatresults=NULL;
//static u32 numresults;

LRESULT CALLBACK CheatSearchDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         // If cheat search hasn't been started yet, disable search and add
         // cheat
         if (cheatresults == NULL)
         {
            SetDlgItemText(hDlg, IDC_CTSEARCHRESTARTBT, (LPCTSTR)"Start");
            EnableWindow(GetDlgItem(hDlg, IDC_CTSEARCHBT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CTADDCHEATBT), FALSE);
         }

         SendDlgItemMessage(hDlg, IDC_EXACTRB, BM_SETCHECK, BST_CHECKED, 0);
         SendDlgItemMessage(hDlg, IDC_LESSTHANRB, BM_SETCHECK, BST_UNCHECKED, 0);
         SendDlgItemMessage(hDlg, IDC_GREATERTHANRB, BM_SETCHECK, BST_UNCHECKED, 0);

         SendDlgItemMessage(hDlg, IDC_UNSIGNEDRB, BM_SETCHECK, BST_CHECKED, 0);
         SendDlgItemMessage(hDlg, IDC_SIGNEDRB, BM_SETCHECK, BST_UNCHECKED, 0);

         SendDlgItemMessage(hDlg, IDC_8BITRB, BM_SETCHECK, BST_CHECKED, 0);
         SendDlgItemMessage(hDlg, IDC_16BITRB, BM_SETCHECK, BST_UNCHECKED, 0);
         SendDlgItemMessage(hDlg, IDC_32BITRB, BM_SETCHECK, BST_UNCHECKED, 0);

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_CTSEARCHRESTARTBT:
               if (cheatresults == NULL)
               {
                  SetDlgItemText(hDlg, IDC_CTSEARCHRESTARTBT, (LPCTSTR)"Restart");
                  EnableWindow(GetDlgItem(hDlg, IDC_CTSEARCHBT), TRUE);
               }
               else
                  free(cheatresults);

               // Setup initial values
               break;
            case IDC_CTSEARCHBT:
               // Search low wram and high wram areas
               return TRUE;
            case IDC_CTADDCHEATBT:
               return TRUE;
            case IDCANCEL:
            case IDOK:
            {
               EndDialog(hDlg, TRUE);
               return TRUE;
            }
            default: break;
         }
         break;
      }

      default: break;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
