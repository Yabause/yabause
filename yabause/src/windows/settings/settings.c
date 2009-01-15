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
//#include <windowsx.h>
#include <commctrl.h>
#undef FASTCALL
#include "../resource.h"
#include "settings.h"

char inifilename[MAX_PATH];
int nocorechange = 0;
#ifdef USETHREADS
int changecore = 0;
int corechanged = 0;
#endif

extern HINSTANCE y_hInstance;

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

LRESULT CALLBACK LogSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);

//////////////////////////////////////////////////////////////////////////////

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

BOOL CreatePropertySheet(psp_struct *psplist, LPCTSTR lpTemplate, LPCTSTR pszTitle, DLGPROC pfnDlgProc)
{
    PROPSHEETPAGE *newpsp;

    if (psplist->numprops+1 > MAXPROPPAGES)
       return FALSE;

    if ((newpsp = (PROPSHEETPAGE *)calloc(psplist->numprops+1, sizeof(PROPSHEETPAGE))) == NULL)
       return FALSE;

    if (psplist->psp)
    {
       memcpy(newpsp, psplist->psp, sizeof(PROPSHEETPAGE) * psplist->numprops);
       free(psplist->psp);
    }

    psplist->psp = newpsp;

    psplist->psp[psplist->numprops].dwSize = sizeof(PROPSHEETPAGE);
    psplist->psp[psplist->numprops].dwFlags = PSP_USETITLE;
    psplist->psp[psplist->numprops].hInstance = y_hInstance;
    psplist->psp[psplist->numprops].pszTemplate = lpTemplate;
    psplist->psp[psplist->numprops].pszIcon = NULL;
    psplist->psp[psplist->numprops].pfnDlgProc = pfnDlgProc;
    psplist->psp[psplist->numprops].pszTitle = pszTitle;
    psplist->psp[psplist->numprops].lParam = 0;
    psplist->psp[psplist->numprops].pfnCallback = NULL;
    psplist->numprops++;
    return TRUE;
}

INT_PTR SettingsCreatePropertySheets(HWND hParent, BOOL ismodal, psp_struct *psplist)
{
   PROPSHEETHEADER psh;

   CreatePropertySheet(psplist, MAKEINTRESOURCE(IDD_BASICSETTINGS), _16("Basic"), (DLGPROC)BasicSettingsDlgProc);
   CreatePropertySheet(psplist, MAKEINTRESOURCE(IDD_VIDEOSETTINGS), _16("Video"), (DLGPROC)VideoSettingsDlgProc);
   CreatePropertySheet(psplist, MAKEINTRESOURCE(IDD_SOUNDSETTINGS), _16("Sound"), (DLGPROC)SoundSettingsDlgProc);
   CreatePropertySheet(psplist, MAKEINTRESOURCE(IDD_INPUTSETTINGS), _16("Input"), (DLGPROC)InputSettingsDlgProc);

#ifdef USESOCKET
   CreatePropertySheet(psplist, MAKEINTRESOURCE(IDD_NETLINKSETTINGS), _16("Netlink"), (DLGPROC)NetlinkSettingsDlgProc);
#endif

#if DEBUG
   CreatePropertySheet(psplist, MAKEINTRESOURCE(IDD_LOGSETTINGS), _16("Log"), (DLGPROC)LogSettingsDlgProc);
#endif

   psplist->psh.dwSize = sizeof(PROPSHEETHEADER);
   psplist->psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
   if (!ismodal)
      psplist->psh.dwFlags |= PSH_MODELESS;
   psplist->psh.hwndParent = hParent;
   psplist->psh.hInstance = y_hInstance;
   psplist->psh.pszIcon = NULL;
   psplist->psh.pszCaption = (LPTSTR)_16("Settings");
   psplist->psh.nPages = psplist->numprops;
   psplist->psh.nStartPage = 0;
   psplist->psh.ppsp = (LPCPROPSHEETPAGE)psplist->psp;
   psplist->psh.pfnCallback = NULL;
   return PropertySheet(&psplist->psh);
}
