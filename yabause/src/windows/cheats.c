#include <windows.h>
#include <commctrl.h>
#include "cheats.h"
#include "../cheat.h"
#include "resource.h"
#include "../debug.h"

extern HINSTANCE y_hInstance;

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK AddARCodeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
         SendDlgItemMessage(hDlg, IDC_CODE, EM_LIMITTEXT, 13, 0);
         return TRUE;
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
               char tempstr[MAX_PATH];
               char tempstr2[MAX_PATH];
               LVITEM itemdata;

               GetDlgItemText(hDlg, IDC_CODE, tempstr, 14);

               // should verify text here

               if (CheatAddARCode(tempstr) != 0)
               {
                   MessageBox (hDlg, "Unable to add code", "Error",  MB_OK | MB_ICONINFORMATION);
                   return TRUE;
               }

               // Add code to code list
               itemdata.mask = LVIF_TEXT;
               itemdata.iItem = SendDlgItemMessage(GetParent(hDlg), IDC_CHEATLIST, LVM_GETITEMCOUNT, 0, 0);
               itemdata.iSubItem = 0;
               sprintf(tempstr2, "AR %s", tempstr);
               itemdata.pszText = tempstr2;
               itemdata.cchTextMax = strlen(itemdata.pszText);
               SendDlgItemMessage(GetParent(hDlg), IDC_CHEATLIST, LVM_INSERTITEM, 0, (LPARAM)&itemdata);

               GetDlgItemText(hDlg, IDC_CODEDESC, tempstr, MAX_PATH);

               itemdata.iSubItem = 1;
               itemdata.pszText = tempstr;
               itemdata.cchTextMax = strlen(itemdata.pszText);
               SendDlgItemMessage(GetParent(hDlg), IDC_CHEATLIST, LVM_SETITEM, 0, (LPARAM)&itemdata);

               itemdata.iSubItem = 2;
               itemdata.pszText = "Enabled";
               itemdata.cchTextMax = strlen(itemdata.pszText);
               SendDlgItemMessage(GetParent(hDlg), IDC_CHEATLIST, LVM_SETITEM, 0, (LPARAM)&itemdata);
               EnableWindow(GetDlgItem(GetParent(hDlg), IDC_CLEARCODES), TRUE);

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
               char tempstr2[MAX_PATH];
               LVITEM itemdata;
               int type;
               u32 addr;
               u32 val;

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
               {
                  type = CHEATTYPE_ENABLE;
                  sprintf(tempstr2, "Enable code - %08X %08X", (int)addr, (int)val);
               }
               else if (SendDlgItemMessage(hDlg, IDC_CTBYTEWRITE, BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  type = CHEATTYPE_BYTEWRITE;
                  sprintf(tempstr2, "Byte Write code - %08X %02X", (int)addr, (int)val);
               }
               else if (SendDlgItemMessage(hDlg, IDC_CTWORDWRITE, BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  type = CHEATTYPE_WORDWRITE;
                  sprintf(tempstr2, "Word write code - %08X %04X", (int)addr, (int)val);
               }
               else
               {
                  type = CHEATTYPE_LONGWRITE;
                  sprintf(tempstr2, "Long write code - %08X %08X", (int)addr, (int)val);
               }

               if (CheatAddCode(type, addr, val) != 0)
               {
                   MessageBox (hDlg, "Unable to add code", "Error",  MB_OK | MB_ICONINFORMATION);
                   return TRUE;
               }

               // Add code to code list
               itemdata.mask = LVIF_TEXT;
               itemdata.iItem = SendDlgItemMessage(GetParent(hDlg), IDC_CHEATLIST, LVM_GETITEMCOUNT, 0, 0);
               itemdata.iSubItem = 0;
               itemdata.pszText = tempstr2;
               itemdata.cchTextMax = strlen(itemdata.pszText);
               SendDlgItemMessage(GetParent(hDlg), IDC_CHEATLIST, LVM_INSERTITEM, 0, (LPARAM)&itemdata);
               GetDlgItemText(hDlg, IDC_CODEDESC, tempstr, MAX_PATH);

               itemdata.iSubItem = 1;
               itemdata.pszText = tempstr;
               itemdata.cchTextMax = strlen(itemdata.pszText);
               SendDlgItemMessage(GetParent(hDlg), IDC_CHEATLIST, LVM_SETITEM, 0, (LPARAM)&itemdata);

               itemdata.iSubItem = 2;
               itemdata.pszText = "Enabled";
               itemdata.cchTextMax = strlen(itemdata.pszText);
               SendDlgItemMessage(GetParent(hDlg), IDC_CHEATLIST, LVM_SETITEM, 0, (LPARAM)&itemdata);
               EnableWindow(GetDlgItem(GetParent(hDlg), IDC_CLEARCODES), TRUE);

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
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         LVCOLUMN coldata;

         ListView_SetExtendedListViewStyleEx(GetDlgItem(hDlg, IDC_CHEATLIST),
                                             LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

         coldata.mask = LVCF_TEXT | LVCF_WIDTH;
         coldata.pszText = "Code\0";
         coldata.cchTextMax = strlen(coldata.pszText);
         coldata.cx = 91;
         SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_INSERTCOLUMN, (WPARAM)0, (LPARAM)&coldata);

         coldata.pszText = "Description\0";
         coldata.cchTextMax = strlen(coldata.pszText);
         coldata.cx = 211;
         SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_INSERTCOLUMN, (WPARAM)1, (LPARAM)&coldata);

         coldata.pszText = "Status\0";
         coldata.cchTextMax = strlen(coldata.pszText);
         coldata.cx = 70;
         SendDlgItemMessage(hDlg, IDC_CHEATLIST, LVM_INSERTCOLUMN, (WPARAM)2, (LPARAM)&coldata);

         EnableWindow(GetDlgItem(hDlg, IDC_DELETECODE), FALSE);
         EnableWindow(GetDlgItem(hDlg, IDC_CLEARCODES), FALSE);

         EnableWindow(GetDlgItem(hDlg, IDC_ADDFROMFILE), FALSE);

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_ADDAR:
               DialogBox(y_hInstance, "AddARCodeDlg", hDlg, (DLGPROC)AddARCodeDlgProc);
               break;
            case IDC_ADDRAWMEMADDR:
               DialogBox(y_hInstance, "AddCodeDlg", hDlg, (DLGPROC)AddCodeDlgProc);
               break;
            case IDC_ADDFROMFILE:
               // do an open file dialog here
               break;
            case IDC_DELETECODE:
            {
               int cursel=SendDlgItemMessage(hDlg, IDC_CHEATLIST,
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
                     int cursel=SendDlgItemMessage(hDlg, IDC_CHEATLIST,
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
                     int cursel=SendDlgItemMessage(hDlg, IDC_CHEATLIST,
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
