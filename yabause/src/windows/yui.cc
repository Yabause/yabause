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
#include <commctrl.h>
#include <SDL/SDL.h>
#include "../superh.hh"
#include "../sh2d.hh"
#include "../yui.hh"
#include "resource.h"
#include "settings.hh"

int stop;
int yabwinw;
int yabwinh;

char SDL_windowhack[32];
HINSTANCE y_hInstance;

unsigned long mtrnssaddress=0x06000000;
unsigned long mtrnseaddress=0x06100000;
char mtrnsfilename[MAX_PATH] = "\0";
char mtrnsreadwrite=0;
bool mtrnssetpc=true;

unsigned long memaddr=0;

SaturnMemory *yabausemem;

bool shwaspaused=true;

LRESULT CALLBACK WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK MemTransferDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);
LRESULT CALLBACK SH2DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam);

void yui_fps(int i) {
}

char * yui_bios(void) {
        return biosfilename;
}

char * yui_cdrom(void) {
        return cdrompath; 
}

char * yui_saveram(void) {
        return saveramfilename;
}

char * yui_mpegrom(void) {
        return mpegromfilename;
}

unsigned char yui_region(void) {
        return regionid;
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
        MSG                         msg;
        int                         i, i2;
        DWORD inifilenamesize=0;
        char *pinifilename;
        static char szAppName[] = "Yabause 0.0.6";
        static char szClassName[] = "Yabause";
        RECT                        workarearect;
        DWORD ret;
        char tempstr[MAX_PATH];

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
        inifilenamesize = GetModuleFileName(y_hInstance, inifilename, MAX_PATH);

        // set pointer to start of extension
        pinifilename = inifilename + inifilenamesize - 4;

        // replace .exe with .ini
        sprintf(pinifilename, ".ini\0");

        if (GetPrivateProfileString("General", "BiosPath", "", biosfilename, MAX_PATH, inifilename) == 0 ||
            GetPrivateProfileString("General", "CDROMDrive", "", cdrompath, MAX_PATH, inifilename) == 0)
        {
           // Startup Settings Configuration here
           if (DialogBox(y_hInstance, "SettingsDlg", NULL, (DLGPROC)SettingsDlgProc) != TRUE)
           {
              // exit program with error
              MessageBox (NULL, "yabause.ini must be properly setup before program can be used.", "Error",  MB_OK | MB_ICONINFORMATION);
              return;
           }
        }

        GetPrivateProfileString("General", "SaveRamPath", "", saveramfilename, MAX_PATH, inifilename);
        GetPrivateProfileString("General", "MpegRomPath", "", mpegromfilename, MAX_PATH, inifilename);

        // Grab Bios Language Settings
//        GetPrivateProfileString("General", "BiosLanguage", "", tempstr, MAX_PATH, inifilename);

        // Grab Region Settings
        GetPrivateProfileString("General", "Region", "", tempstr, MAX_PATH, inifilename);

        if (strlen(tempstr) == 1)
        {
           switch (tempstr[0])
           {
              case 'J':
                 regionid = 1;
                 break;
              case 'T':
                 regionid = 2;
                 break;
              case 'U':
                 regionid = 4;
                 break;
              case 'B':
                 regionid = 5;
                 break;
              case 'K':
                 regionid = 6;
                 break;
              case 'A':
                 regionid = 0xA;
                 break;
              case 'E':
                 regionid = 0xC;
                 break;
              case 'L':
                 regionid = 0xD;
                 break;
              default: break;
           }
        }
        else if (stricmp(tempstr, "AUTO") == 0)
           regionid = 0;

        // Figure out how much of the screen is useable
//        if (SystemParametersInfo(SPI_GETWORKAREA, 0, &workarearect, 0) == FALSE)
//        {
           // Since we can't retrieve it, use a default values
           yabwinw = 320 + GetSystemMetrics(SM_CXSIZEFRAME) * 2;
           yabwinh = 224 + (GetSystemMetrics(SM_CYSIZEFRAME) * 2) + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION);
//        }
//        else
//        {
//           // Calculate sizes that fit in the work area
//           yabwinw = workarearect.right - workarearect.left - (GetSystemMetrics(SM_CXSIZEFRAME) * 2);
//           yabwinh = workarearect.bottom - workarearect.top - ((GetSystemMetrics(SM_CYSIZEFRAME) * 2) + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION));
//        }

        // Create a window
        hWnd = CreateWindow(szClassName,        // class
                            szAppName,          // caption
                            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                            WS_THICKFRAME | WS_MINIMIZEBOX |  // style
                            WS_CLIPCHILDREN,  
                            CW_USEDEFAULT,      // x pos
                            CW_USEDEFAULT,      // y pos
                            yabwinw,        // width
                            yabwinh,       // height
                            HWND_DESKTOP,       // parent window
                            NULL,               // menu 
                            y_hInstance,          // instance
                            NULL);              // parms

        if (!hWnd)
            return;

        SetWindowPos(hWnd, HWND_TOP, 0, 0, yabwinw, yabwinh, SWP_NOREPOSITION);

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
            case IDM_CHOOSEBIOS:
            {
               OPENFILENAME ofn;

               // setup ofn structure
               ZeroMemory(&ofn, sizeof(OPENFILENAME));
               ofn.lStructSize = sizeof(OPENFILENAME);
               ofn.hwndOwner = hWnd;
               ofn.lpstrFilter = "All Files\0*.*\0Binary Files\0*.BIN\0";
               ofn.nFilterIndex = 1;
               ofn.lpstrFile = biosfilename;
               ofn.nMaxFile = MAX_PATH;
               ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

               if (GetOpenFileName(&ofn))
               {
                  // should ungray run button at this point

                  // write new settings to ini file
                  WritePrivateProfileString("General", "BiosPath", biosfilename, inifilename);
               }

               break;
            }
            case IDM_CHOOSECDROM:
            {
               break;
            }            
//            case IDM_RUN:
//            {
//               break;
//            }
            case IDM_MEMTRANSFER:
            {
               SuperH *msh = yabausemem->getMasterSH();

               shwaspaused = msh->paused();

               if (!shwaspaused) msh->pause();

               DialogBox(y_hInstance, "MemTransferDlg", hWnd, (DLGPROC)MemTransferDlgProc);

               if (!shwaspaused) msh->run();

               break;
            }
            case IDM_SETTINGS:
            {
               SuperH *msh = yabausemem->getMasterSH();

               shwaspaused = msh->paused();

               if (!shwaspaused) msh->pause();

               DialogBox(y_hInstance, "SettingsDlg", hWnd, (DLGPROC)SettingsDlgProc);

               if (!shwaspaused) msh->run();
               break;
            }
            case IDM_MSH2DEBUG:
            {
               SuperH *msh = yabausemem->getMasterSH();

               shwaspaused = msh->paused();

               if (!shwaspaused) msh->pause();

               DialogBox(y_hInstance, "SH2DebugDlg", hWnd, (DLGPROC)SH2DebugDlgProc);

               if (!shwaspaused) msh->run();

               break;
            }
            case IDM_EXIT:
               PostMessage(hWnd, WM_CLOSE, 0, 0);
               break;
         }
         return 0L;
      }
      case WM_ENTERMENULOOP:
      {
         SuperH *msh = yabausemem->getMasterSH();

         shwaspaused = msh->paused();

         if (!shwaspaused) msh->pause();

         return 0L;
      }
      case WM_EXITMENULOOP:
      {
         SuperH *msh = yabausemem->getMasterSH();

         if (!shwaspaused) msh->run();

         return 0L;
      }
      case WM_SIZE:
      {
         SetWindowPos(hWnd, HWND_TOP, 0, 0, yabwinw, yabwinh, SWP_NOREPOSITION);
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
               {
                  SuperH *proc=yabausemem->getMasterSH();
                  sh2regs_struct sh2regs;
                  proc->GetRegisters(&sh2regs);
                  sh2regs.PC = mtrnssaddress;
                  proc->SetRegisters(&sh2regs);

                  mtrnssetpc = true;
               }
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

void UpdateRegList(HWND hDlg, sh2regs_struct *regs)
{
   char tempstr[128];
   int i;

   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_RESETCONTENT, 0, 0);

   for (i = 0; i < 16; i++)
   {                                       
      sprintf(tempstr, "R%02d =  %08x\0", i, regs->R[i]);
      strupr(tempstr);
      SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);
   }

   // SR
   sprintf(tempstr, "SR =   %08x\0", regs->SR.all);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // GBR
   sprintf(tempstr, "GBR =  %08x\0", regs->GBR);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // VBR
   sprintf(tempstr, "VBR =  %08x\0", regs->VBR);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // MACH
   sprintf(tempstr, "MACH = %08x\0", regs->MACH);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // MACL
   sprintf(tempstr, "MACL = %08x\0", regs->MACL);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // PR
   sprintf(tempstr, "PR =   %08x\0", regs->PR);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // PC
   sprintf(tempstr, "PC =   %08x\0", regs->PC);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);
}

void UpdateCodeList(HWND hDlg, unsigned long addr)
{
   unsigned long buf_size;
   unsigned long buf_addr;
   int i, i2;
   char buf[60];
   unsigned long offset;
//   SuperH *proc=yabausemem->getSH();

   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_RESETCONTENT, 0, 0);

   offset = addr - (12 * 2);

   for (i=0; i < 24; i++) // amount of lines
   {
//      SH2Disasm(offset, proc->getMemory()->getWord(offset), 0, buf);
      SH2Disasm(offset, yabausemem->getWord(offset), 0, buf);

      SendMessage(HWND(GetDlgItem(hDlg, IDC_LISTBOX2)), LB_ADDSTRING, 0,
                  (long)buf);
      offset += 2;
   }

   SendMessage(HWND(GetDlgItem(hDlg, IDC_LISTBOX2)), LB_SETCURSEL,12,0);
}

LRESULT CALLBACK MemDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         char buf[8];

         sprintf(buf, "%08X",memaddr);
         SetDlgItemText(hDlg, IDC_EDITTEXT1, buf);
         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (wParam)
         {
            case IDOK:
            {
               char buf[9];

               EndDialog(hDlg, TRUE);
               GetDlgItemText(hDlg, IDC_EDITTEXT1, buf, 11);

               sscanf(buf, "%08x", &memaddr);

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
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK SH2DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         unsigned long i;
         SuperH *proc=yabausemem->getMasterSH();
         sh2regs_struct sh2regs;

         if (proc->paused())
         {
            proc->GetRegisters(&sh2regs);
            UpdateRegList(hDlg, &sh2regs);
            UpdateCodeList(hDlg, sh2regs.PC);
         }

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
               EndDialog(hDlg, TRUE);

               return TRUE;
            }
            case IDC_STEP:
            {
               SuperH *proc=yabausemem->getMasterSH();
               sh2regs_struct sh2regs;
               proc->step();
               proc->GetRegisters(&sh2regs);
               UpdateRegList(hDlg, &sh2regs);
               UpdateCodeList(hDlg, sh2regs.PC);
               break;
            }
            case IDC_RUN:
            {
               SuperH *proc=yabausemem->getMasterSH();
               proc->run();
               break;
            }
            case IDC_PAUSE:
            {
               SuperH *proc=yabausemem->getMasterSH();
               sh2regs_struct sh2regs;
               proc->pause();
               proc->GetRegisters(&sh2regs);
               UpdateRegList(hDlg, &sh2regs);
               UpdateCodeList(hDlg, sh2regs.PC);
               break;
            }
            case IDC_LISTBOX1:
            {
               switch (HIWORD(wParam))
               {
                  case LBN_DBLCLK:
                  {
                     // dialogue for changing register values
                     int cursel;

                     SuperH *proc=yabausemem->getMasterSH();
                     sh2regs_struct sh2regs;
                     proc->GetRegisters(&sh2regs);

                     cursel = SendMessage(HWND(GetDlgItem(hDlg, IDC_LISTBOX1)), LB_GETCURSEL,0,0);

                     if (cursel < 16)
                     {
                        memaddr = sh2regs.R[cursel];
                     }
                     else if (cursel == 16)
                     {
                        memaddr = sh2regs.SR.all;
                     }
                     else if (cursel == 17)
                     {
                        memaddr = sh2regs.GBR;
                     }
                     else if (cursel == 18)
                     {
                        memaddr = sh2regs.VBR;
                     }
                     else if (cursel == 19)
                     {
                        memaddr = sh2regs.MACH;
                     }
                     else if (cursel == 20)
                     {
                        memaddr = sh2regs.MACL;
                     }
                     else if (cursel == 21)
                     {
                        memaddr = sh2regs.PR;
                     }
                     else if (cursel == 22)
                     {
                        memaddr = sh2regs.PC;
                     }

                     if (DialogBox(GetModuleHandle(0), "MemDlg", hDlg, (DLGPROC)MemDlgProc) != FALSE)
                     {
                        if (cursel < 16)
                        {
                           sh2regs.R[cursel] = memaddr;
                        }
                        else if (cursel == 16)
                        {
                           sh2regs.SR.all = memaddr;
                        }
                        else if (cursel == 17)
                        {
                           sh2regs.GBR = memaddr;
                        }
                        else if (cursel == 18)
                        {
                           sh2regs.VBR = memaddr;
                        }
                        else if (cursel == 19)
                        {
                           sh2regs.MACH = memaddr;
                        }
                        else if (cursel == 20)
                        {
                           sh2regs.MACL = memaddr;
                        }
                        else if (cursel == 21)
                        {
                           sh2regs.PR = memaddr;
                        }
                        else if (cursel == 22)
                        {
                           sh2regs.PC = memaddr;
                           UpdateCodeList(hDlg, sh2regs.PC);
                        }
                     }

                     proc->SetRegisters(&sh2regs);
                     UpdateRegList(hDlg, &sh2regs);

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
      default: break;
   }

   return FALSE;
}

