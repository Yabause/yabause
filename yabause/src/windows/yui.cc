/*  Copyright 2004 Guillaume Duhamel
    Copyright 2004 Theo Berkau

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
#include <SDL/SDL.h>
#include "../superh.hh"
#include "../yui.hh"
#include "resource.h"

int stop;

char SDL_windowhack[32];
HINSTANCE y_hInstance;

char biosfilename[MAX_PATH] = "\0";
char cdromdriveletter=0;

unsigned long mtrnssaddress=0x06000000;
unsigned long mtrnseaddress=0x06100000;
char mtrnsfilename[MAX_PATH] = "\0";
char mtrnsreadwrite=0;
bool mtrnssetpc=true;

SaturnMemory *yabausemem;

LRESULT CALLBACK WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK MemTransferDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);

void yui_fps(int i) {
	fprintf(stderr, "%d\n", i);
}

char * yui_bios(void) {
        return "c:\\yabause\\src\\sat_jap.rom";
}

char * yui_cdrom(void) {
        return "E:";
}

void yui_hide_show(void) {
}

void yui_quit(void) {
	stop = 1;
}

void yui_init(int (*yab_main)(void*)) {
	SaturnMemory *mem;
        WNDCLASS                    MyWndClass;
        HRESULT                     hRet;
        HWND                        hWnd;
        int                         cx, cy;
        MSG                         msg;
        int                         i, i2;
        char INIFileName[MAX_PATH];
        DWORD INIFileNameSize=0;
        char *pINIFileName;
        static char szAppName[] = "Yabause 0.0.6";
        static char szClassName[] = "Yabause";

        y_hInstance = GetModuleHandle(NULL);

        // Set up and register window class
        MyWndClass.style = CS_HREDRAW | CS_VREDRAW;
        MyWndClass.lpfnWndProc = (WNDPROC) WindowProc;
        MyWndClass.cbClsExtra = 0;
        MyWndClass.cbWndExtra = sizeof(DWORD);
        MyWndClass.hInstance = y_hInstance;
        MyWndClass.hIcon = LoadIcon(y_hInstance, MAKEINTRESOURCE(IDI_ICON));
        MyWndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        MyWndClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
        MyWndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
        MyWndClass.lpszClassName = szClassName;

        if (!RegisterClass(&MyWndClass))
          return;

        // get program pathname
        INIFileNameSize = GetModuleFileName(y_hInstance, INIFileName, MAX_PATH);

        // set pointer to start of extension
        pINIFileName = INIFileName + INIFileNameSize - 4;

        // replace .exe with .ini
        sprintf(pINIFileName, ".ini\0");

        GetPrivateProfileString("GeneralSection", "BiosPath", "", biosfilename, MAX_PATH, INIFileName);
        GetPrivateProfileString("GeneralSection", "CDROMDrive", "", &cdromdriveletter, 1, INIFileName);

        cx = 320 + GetSystemMetrics(SM_CXSIZEFRAME) * 2;
        cy = 224 + (GetSystemMetrics(SM_CYSIZEFRAME) * 2) + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION);

        // Create a window
        hWnd = CreateWindow(szClassName,        // class
                            szAppName,          // caption
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                            WS_THICKFRAME | WS_MINIMIZEBOX |  // style
                            WS_CLIPCHILDREN,  
                            CW_USEDEFAULT,      // x pos
                            CW_USEDEFAULT,      // y pos
                            cx,        // width
                            cy,       // height
                            HWND_DESKTOP,       // parent window
                            NULL,               // menu 
                            y_hInstance,          // instance
                            NULL);              // parms

        if (!hWnd)
            return;

        // may change this
        ShowWindow(hWnd, SW_SHOWDEFAULT);
        UpdateWindow(hWnd);

        sprintf(SDL_windowhack,"SDL_WINDOWID=%ld", hWnd);
	putenv(SDL_windowhack);

	stop = 0;
	mem = new SaturnMemory();
	mem->start();
        yabausemem = mem;

        while (!stop) yab_main(mem);
	delete(mem);
}

LRESULT CALLBACK WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
   HRESULT hRet;
   HMENU hMenu;

   switch (uMsg)
   {
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
//            case IDM_RUN:
//            {
//               break;
//            }
            case IDM_MEMTRANSFER:
            {
               SuperH *msh = yabausemem->getMasterSH();
               bool shwaspaused;

               shwaspaused = msh->paused();

               if (!shwaspaused) msh->pause();

               DialogBox(y_hInstance, "MemTransferDlg", hWnd, (DLGPROC)MemTransferDlgProc);

               if (!shwaspaused) msh->run();

               break;
            }
            case IDM_EXIT:
               PostMessage(hWnd, WM_CLOSE, 0, 0);
               break;
         }
         return 0L;
      }
      case WM_PAINT:
      {
         PAINTSTRUCT ps;

         BeginPaint(hWnd, &ps);
         EndPaint(hWnd, &ps);
         return 0L;
      }
      case WM_DESTROY:
      {
         PostQuitMessage(0);

         return 0L;
      }

    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MemTransferDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
   char tempstr[256];

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         SetDlgItemText(hDlg, IDC_EDITTEXT1, mtrnsfilename);

         sprintf(tempstr, "%08x\0", mtrnssaddress);
         SetDlgItemText(hDlg, IDC_EDITTEXT2, tempstr);

         sprintf(tempstr, "%08x\0", mtrnseaddress);
         SetDlgItemText(hDlg, IDC_EDITTEXT3, tempstr);

         if (mtrnsreadwrite == 0)
         {
            SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADMEM), BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDC_UPLOADMEM), BM_SETCHECK, BST_UNCHECKED, 0);
            EnableWindow(HWND(GetDlgItem(hDlg, IDC_EDITTEXT3)), TRUE);
            EnableWindow(HWND(GetDlgItem(hDlg, IDC_CHECKBOX1)), FALSE);
         }
         else
         {
            SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADMEM), BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDC_UPLOADMEM), BM_SETCHECK, BST_CHECKED, 0);
            if (mtrnssetpc)
               SendMessage(GetDlgItem(hDlg, IDC_CHECKBOX1), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(HWND(GetDlgItem(hDlg, IDC_EDITTEXT3)), FALSE);
            EnableWindow(HWND(GetDlgItem(hDlg, IDC_CHECKBOX1)), TRUE);
         }

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_BROWSE:
            {
               OPENFILENAME ofn;

               if (SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADMEM), BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  // setup ofn structure
                  ZeroMemory(&ofn, sizeof(ofn));
                  ofn.lStructSize = sizeof(ofn);
                  ofn.hwndOwner = hDlg;
                  ofn.lpstrFilter = "All Files\0*.*\0Binary Files\0*.BIN\0";
                  ofn.nFilterIndex = 1;
                  ofn.lpstrFile = mtrnsfilename;
                  ofn.nMaxFile = sizeof(mtrnsfilename);
                  ofn.Flags = OFN_OVERWRITEPROMPT;
 
                  if (GetSaveFileName(&ofn))
                  {
                     SetDlgItemText(hDlg, IDC_EDITTEXT1, mtrnsfilename);
                  }
               }
               else
               {
                  // setup ofn structure
                  ZeroMemory(&ofn, sizeof(OPENFILENAME));
                  ofn.lStructSize = sizeof(OPENFILENAME);
                  ofn.hwndOwner = hDlg;
                  ofn.lpstrFilter = "All Files\0*.*\0Binary Files\0*.BIN\0";
                  ofn.nFilterIndex = 1;
                  ofn.lpstrFile = mtrnsfilename;
                  ofn.nMaxFile = sizeof(mtrnsfilename);
                  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                  if (GetOpenFileName(&ofn))
                  {
                     SetDlgItemText(hDlg, IDC_EDITTEXT1, mtrnsfilename);
                  }
               }

               return TRUE;
            }
            case IDOK:
            {
               HANDLE hFile;

               GetDlgItemText(hDlg, IDC_EDITTEXT1, mtrnsfilename, MAX_PATH);

               GetDlgItemText(hDlg, IDC_EDITTEXT2, tempstr, 9);
               sscanf(tempstr, "%08x", &mtrnssaddress);

               GetDlgItemText(hDlg, IDC_EDITTEXT3, tempstr, 9);
               sscanf(tempstr, "%08x", &mtrnseaddress);

               if ((mtrnseaddress - mtrnssaddress) < 0)
               {
                  MessageBox (hDlg, "Invalid Start/End Address Combination", "Error",  MB_OK | MB_ICONINFORMATION);
                  EndDialog(hDlg, TRUE);
                  return FALSE;
               }

               if (SendMessage(GetDlgItem(hDlg, IDC_CHECKBOX1), BM_GETCHECK, 0, 0) == BST_CHECKED)
                  mtrnssetpc = true;
               else
                  mtrnssetpc = false;

               if (SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADMEM), BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  // Let's do a ram dump
                  yabausemem->save(mtrnsfilename, mtrnssaddress, mtrnseaddress - mtrnssaddress);
                  mtrnsreadwrite = 0;
               }
               else
               {
                  // upload to ram
                  yabausemem->load(mtrnsfilename, mtrnssaddress);
                  mtrnsreadwrite = 1;
               }

               EndDialog(hDlg, TRUE);

               return TRUE;
            }
            case IDCANCEL:
            {
               EndDialog(hDlg, FALSE);

               return TRUE;
            }
            case IDC_UPLOADMEM:
            {
               if (HIWORD(wParam) == BN_CLICKED)
               {
                  EnableWindow(HWND(GetDlgItem(hDlg, IDC_EDITTEXT3)), FALSE);
                  EnableWindow(HWND(GetDlgItem(hDlg, IDC_CHECKBOX1)), TRUE);
               }

               break;
            }
            case IDC_DOWNLOADMEM:
            {
               if (HIWORD(wParam) == BN_CLICKED)
               {
                  EnableWindow(HWND(GetDlgItem(hDlg, IDC_EDITTEXT3)), TRUE);
                  EnableWindow(HWND(GetDlgItem(hDlg, IDC_CHECKBOX1)), FALSE);
               }
            }
            default: break;
         }
         break;
      }

      default: break;
   }

   return FALSE;
}
