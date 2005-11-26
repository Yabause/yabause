/*  Copyright 2004 Guillaume Duhamel
    Copyright 2004-2005 Theo Berkau
    Copyright 2005 Joost Peters

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
#include "SDL.h"
#undef FASTCALL
#include "../memory.h"
#include "../scu.h"
#include "../sh2core.h"
#include "../sh2d.h"
#include "../vdp2.h"
#include "../yui.h"
#include "../sndsdl.h"
#include "../vidsdlgl.h"
#include "../vidsdlsoft.h"
#include "../persdl.h"
#include "../cs0.h"
#include "resource.h"
#include "settings.h"
#include "cd.h"

int stop;
int yabwinw;
int yabwinh;

char SDL_windowhack[32];
HINSTANCE y_hInstance;
HWND YabWin;

u32 mtrnssaddress=0x06004000;
u32 mtrnseaddress=0x06100000;
char mtrnsfilename[MAX_PATH] = "\0";
int mtrnsreadwrite=1;
int mtrnssetpc=TRUE;

u32 memaddr=0;

SH2_struct *debugsh;

LRESULT CALLBACK WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK MemTransferDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);
LRESULT CALLBACK SH2DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam);
LRESULT CALLBACK VDP1DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);
LRESULT CALLBACK VDP2DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);
LRESULT CALLBACK M68KDebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);
LRESULT CALLBACK SCUDSPDebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam);

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERSDL,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
&SPTICD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
&SNDSDL,
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDSDLGL,
&VIDSDLSoft,
NULL
};

//////////////////////////////////////////////////////////////////////////////

void YuiSetBiosFilename(const char *bios)
{
}

//////////////////////////////////////////////////////////////////////////////

void YuiSetIsoFilename(const char *iso) {
}

//////////////////////////////////////////////////////////////////////////////

void YuiSetCdromFilename(const char * cdromfilename)
{
}

//////////////////////////////////////////////////////////////////////////////

void YuiSetSoundEnable(int enablesound)
{
}

//////////////////////////////////////////////////////////////////////////////

void YuiHideShow(void)
{
}

//////////////////////////////////////////////////////////////////////////////

void YuiQuit(void)
{
	stop = 1;
}

//////////////////////////////////////////////////////////////////////////////

void YuiErrorMsg(const char *string)
{
   MessageBox (NULL, string, "Error",  MB_OK | MB_ICONINFORMATION);
}

//////////////////////////////////////////////////////////////////////////////

void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen)
{
   RECT rect;

   rect.left = 0;
   rect.top = 0;
   rect.right = w;
   rect.bottom = h;

   AdjustWindowRectEx(&rect, GetWindowLong(YabWin, GWL_STYLE), TRUE, 0);

   w = rect.right - rect.left;
   h = rect.bottom - rect.top;

   if (isfullscreen)
      SetWindowPos(YabWin, HWND_TOPMOST, (GetSystemMetrics(SM_CXSCREEN) - w) / 2, ((GetSystemMetrics(SM_CYSCREEN) - h) / 2), w, h, SWP_NOCOPYBITS | SWP_SHOWWINDOW);
   else
      SetWindowPos(YabWin, HWND_NOTOPMOST, (GetSystemMetrics(SM_CXSCREEN) - w) / 2, (GetSystemMetrics(SM_CYSCREEN) - h) / 2, w, h, SWP_NOCOPYBITS | SWP_SHOWWINDOW);

   SetForegroundWindow(YabWin);
}

//////////////////////////////////////////////////////////////////////////////

int YuiInit(void)
{
   WNDCLASS                    MyWndClass;
   HWND                        hWnd;
   DWORD inifilenamesize=0;
   char *pinifilename;
   static char szAppName[128];
   static char szClassName[] = "Yabause";
//   RECT                        workarearect;
//   DWORD ret;
   char tempstr[MAX_PATH];
   yabauseinit_struct yinit;

   sprintf(szAppName, "Yabause %s", VERSION);

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
      return -1;

   // get program pathname
   inifilenamesize = GetModuleFileName(y_hInstance, inifilename, MAX_PATH);

   // set pointer to start of extension
   pinifilename = inifilename + inifilenamesize - 4;

   // replace .exe with .ini
   sprintf(pinifilename, ".ini");

   if (GetPrivateProfileString("General", "BiosPath", "", biosfilename, MAX_PATH, inifilename) == 0 ||
       GetPrivateProfileString("General", "CDROMDrive", "", cdrompath, MAX_PATH, inifilename) == 0)
   {
      // Startup Settings Configuration here
      if (DialogBox(y_hInstance, "SettingsDlg", NULL, (DLGPROC)SettingsDlgProc) != TRUE)
      {
         // exit program with error
         MessageBox (NULL, "yabause.ini must be properly setup before program can be used.", "Error",  MB_OK | MB_ICONINFORMATION);
         return -1;
      }
   }

   GetPrivateProfileString("General", "BackupRamPath", "", backupramfilename, MAX_PATH, inifilename);
   GetPrivateProfileString("General", "MpegRomPath", "", mpegromfilename, MAX_PATH, inifilename);

   GetPrivateProfileString("General", "CartType", "", tempstr, MAX_PATH, inifilename);

   // figure out cart type here, grab cartfilename if necessary
   carttype = atoi(tempstr);

   switch (carttype)
   {
      case CART_PAR:
      case CART_BACKUPRAM4MBIT:
      case CART_BACKUPRAM8MBIT:
      case CART_BACKUPRAM16MBIT:
      case CART_BACKUPRAM32MBIT:
      case CART_ROM16MBIT:
         GetPrivateProfileString("General", "CartPath", "", cartfilename, MAX_PATH, inifilename);
         break;
      default: break;
   }

   // Grab Bios Language Settings
//   GetPrivateProfileString("General", "BiosLanguage", "", tempstr, MAX_PATH, inifilename);

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
//   if (SystemParametersInfo(SPI_GETWORKAREA, 0, &workarearect, 0) == FALSE)
//   {
      // Since we can't retrieve it, use a default values
      yabwinw = 320 + GetSystemMetrics(SM_CXSIZEFRAME) * 2;
      yabwinh = 224 + (GetSystemMetrics(SM_CYSIZEFRAME) * 2) + GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION);
//   }
//   else
//   {
//      // Calculate sizes that fit in the work area
//      yabwinw = workarearect.right - workarearect.left;
//      yabwinh = workarearect.bottom - workarearect.top;
//   }

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
      return -1;

   SetWindowPos(hWnd, HWND_TOP, 0, 0, yabwinw, yabwinh, SWP_NOREPOSITION);

   // may change this
   ShowWindow(hWnd, SW_SHOWDEFAULT);
   UpdateWindow(hWnd);

   sprintf(SDL_windowhack,"SDL_WINDOWID=%ld", (long int)hWnd);
   putenv(SDL_windowhack);

   YabWin = hWnd;

   stop = 0;

   yinit.percoretype = PERCORE_SDL;
   yinit.sh2coretype = SH2CORE_DEFAULT;
   yinit.vidcoretype = VIDCORE_SDLGL;
//   yinit.vidcoretype = VIDCORE_SDLSOFT;
   yinit.sndcoretype = SNDCORE_SDL;
   if (IsPathCdrom(cdrompath))
      yinit.cdcoretype = CDCORE_SPTI;
   else
      yinit.cdcoretype = CDCORE_ISO;
   yinit.carttype = carttype;
   yinit.regionid = regionid;
   yinit.biospath = biosfilename;
   yinit.cdpath = cdrompath;
   yinit.buppath = backupramfilename;
   yinit.mpegpath = mpegromfilename;
   yinit.cartpath = cartfilename;

   if (YabauseInit(&yinit) == -1)
      return -1;

   while (!stop)
   {
      if (PERCore->HandleEvents() != 0)
         return -1;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WindowProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
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
               DialogBox(y_hInstance, "MemTransferDlg", hWnd, (DLGPROC)MemTransferDlgProc);
               break;
            }
            case IDM_SETTINGS:
            {
               DialogBox(y_hInstance, "SettingsDlg", hWnd, (DLGPROC)SettingsDlgProc);
               break;
            }
            case IDM_MSH2DEBUG:
            {
               debugsh = MSH2;
               DialogBox(y_hInstance, "SH2DebugDlg", hWnd, (DLGPROC)SH2DebugDlgProc);
               break;
            }
            case IDM_SSH2DEBUG:
            {
               debugsh = SSH2;
               DialogBox(y_hInstance, "SH2DebugDlg", hWnd, (DLGPROC)SH2DebugDlgProc);
               break;
            }
            case IDM_VDP1DEBUG:
            {
               DialogBox(y_hInstance, "VDP1DebugDlg", hWnd, (DLGPROC)VDP1DebugDlgProc);
               break;
            }
            case IDM_VDP2DEBUG:
            {
               DialogBox(y_hInstance, "VDP2DebugDlg", hWnd, (DLGPROC)VDP2DebugDlgProc);
               break;
            }
            case IDM_M68KDEBUG:
            {
               DialogBox(y_hInstance, "M68KDebugDlg", hWnd, (DLGPROC)M68KDebugDlgProc);
               break;
            }
            case IDM_SCUDSPDEBUG:
            {
               DialogBox(y_hInstance, "SCUDSPDebugDlg", hWnd, (DLGPROC)SCUDSPDebugDlgProc);
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
         return 0L;
      }
      case WM_EXITMENULOOP:
      {
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

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK MemTransferDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
   char tempstr[256];

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         SetDlgItemText(hDlg, IDC_EDITTEXT1, mtrnsfilename);

         sprintf(tempstr, "%08X", (int)mtrnssaddress);
         SetDlgItemText(hDlg, IDC_EDITTEXT2, tempstr);

         sprintf(tempstr, "%08X", (int)mtrnseaddress);
         SetDlgItemText(hDlg, IDC_EDITTEXT3, tempstr);

         if (mtrnsreadwrite == 0)
         {
            SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADMEM), BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDC_UPLOADMEM), BM_SETCHECK, BST_UNCHECKED, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_EDITTEXT3), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_CHECKBOX1), FALSE);
         }
         else
         {
            SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADMEM), BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDC_UPLOADMEM), BM_SETCHECK, BST_CHECKED, 0);
            if (mtrnssetpc)
               SendMessage(GetDlgItem(hDlg, IDC_CHECKBOX1), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hDlg, IDC_EDITTEXT3), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_CHECKBOX1), TRUE);
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
               GetDlgItemText(hDlg, IDC_EDITTEXT1, mtrnsfilename, MAX_PATH);

               GetDlgItemText(hDlg, IDC_EDITTEXT2, tempstr, 9);
               sscanf(tempstr, "%08X", &mtrnssaddress);

               GetDlgItemText(hDlg, IDC_EDITTEXT3, tempstr, 9);
               sscanf(tempstr, "%08X", &mtrnseaddress);

               if ((mtrnseaddress - mtrnssaddress) < 0)
               {
                  MessageBox (hDlg, "Invalid Start/End Address Combination", "Error",  MB_OK | MB_ICONINFORMATION);
                  EndDialog(hDlg, TRUE);
                  return FALSE;
               }

               if (SendMessage(GetDlgItem(hDlg, IDC_DOWNLOADMEM), BM_GETCHECK, 0, 0) == BST_CHECKED)
               {
                  // Let's do a ram dump
                  MappedMemorySave(mtrnsfilename, mtrnssaddress, mtrnseaddress - mtrnssaddress);
                  mtrnsreadwrite = 0;
               }
               else
               {
                  // upload to ram and possibly execute
                  mtrnsreadwrite = 1;

                  // Is this a program?
                  if (SendMessage(GetDlgItem(hDlg, IDC_CHECKBOX1), BM_GETCHECK, 0, 0) == BST_CHECKED)
                  {
                     MappedMemoryLoadExec(mtrnsfilename, mtrnssaddress);
                     mtrnssetpc = TRUE;
                  }
                  else
                  {
                     MappedMemoryLoad(mtrnsfilename, mtrnssaddress);
                     mtrnssetpc = FALSE;
                  }
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
                  EnableWindow(GetDlgItem(hDlg, IDC_EDITTEXT3), FALSE);
                  EnableWindow(GetDlgItem(hDlg, IDC_CHECKBOX1), TRUE);
               }

               break;
            }
            case IDC_DOWNLOADMEM:
            {
               if (HIWORD(wParam) == BN_CLICKED)
               {
                  EnableWindow(GetDlgItem(hDlg, IDC_EDITTEXT3), TRUE);
                  EnableWindow(GetDlgItem(hDlg, IDC_CHECKBOX1), FALSE);
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

//////////////////////////////////////////////////////////////////////////////

void SH2UpdateRegList(HWND hDlg, sh2regs_struct *regs)
{
   char tempstr[128];
   int i;

   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_RESETCONTENT, 0, 0);

   for (i = 0; i < 16; i++)
   {                                       
      sprintf(tempstr, "R%02d =  %08x", i, (int)regs->R[i]);
      strupr(tempstr);
      SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);
   }

   // SR
   sprintf(tempstr, "SR =   %08x", (int)regs->SR.all);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // GBR
   sprintf(tempstr, "GBR =  %08x", (int)regs->GBR);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // VBR
   sprintf(tempstr, "VBR =  %08x", (int)regs->VBR);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // MACH
   sprintf(tempstr, "MACH = %08x", (int)regs->MACH);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // MACL
   sprintf(tempstr, "MACL = %08x", (int)regs->MACL);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // PR
   sprintf(tempstr, "PR =   %08x", (int)regs->PR);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // PC
   sprintf(tempstr, "PC =   %08x", (int)regs->PC);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);
}

//////////////////////////////////////////////////////////////////////////////

void SH2UpdateCodeList(HWND hDlg, u32 addr)
{
   int i;
   char buf[60];
   u32 offset;

   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_RESETCONTENT, 0, 0);

   offset = addr - (12 * 2);

   for (i=0; i < 24; i++) // amount of lines
   {
      SH2Disasm(offset, MappedMemoryReadWord(offset), 0, buf);

      SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_ADDSTRING, 0,
                  (s32)buf);
      offset += 2;
   }

   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_SETCURSEL,12,0);
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK MemDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         char buf[8];

         sprintf(buf, "%08X", (int)memaddr);
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

void SH2BreakpointHandler (SH2_struct *context, u32 addr)
{
   MessageBox (NULL, "Breakpoint Reached", "Notice",  MB_OK | MB_ICONINFORMATION);

   debugsh = context;
   DialogBox(y_hInstance, "SH2DebugDlg", YabWin, (DLGPROC)SH2DebugDlgProc);
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK SH2DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         sh2regs_struct sh2regs;
         codebreakpoint_struct *cbp;
         char tempstr[10];
         int i;

         cbp = SH2GetBreakpointList(debugsh);

         for (i = 0; i < MAX_BREAKPOINTS; i++)
         {
            if (cbp[i].addr != 0xFFFFFFFF)
            {
               sprintf(tempstr, "%08X", (int)cbp[i].addr);
               SendMessage(GetDlgItem(hDlg, IDC_LISTBOX3), LB_ADDSTRING, 0, (LPARAM)tempstr);
            }
         }

//         if (proc->paused())
//         {
            SH2GetRegisters(debugsh, &sh2regs);
            SH2UpdateRegList(hDlg, &sh2regs);
            SH2UpdateCodeList(hDlg, sh2regs.PC);
//         }


         SH2SetBreakpointCallBack(debugsh, (void (*)(void *, u32))&SH2BreakpointHandler);
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
               sh2regs_struct sh2regs;
               SH2Step(debugsh);
               SH2GetRegisters(debugsh, &sh2regs);
               SH2UpdateRegList(hDlg, &sh2regs);
               SH2UpdateCodeList(hDlg, sh2regs.PC);

               break;
            }
            case IDC_STEPOVER:
            {
               break;
            }
            case IDC_MEMTRANSFER:
            {
               DialogBox(y_hInstance, "MemTransferDlg", hDlg, (DLGPROC)MemTransferDlgProc);
               break;
            }
            case IDC_ADDBP1:
            {
               char bptext[10];
               u32 addr=0;
               memset(bptext, 0, 10);
               GetDlgItemText(hDlg, IDC_EDITTEXT1, bptext, 10);

               if (bptext[0] != 0)
               {
                  sscanf(bptext, "%X", &addr);
                  sprintf(bptext, "%08X", (int)addr);

                  if (SH2AddCodeBreakpoint(debugsh, addr) == 0)
                     SendMessage(GetDlgItem(hDlg, IDC_LISTBOX3), LB_ADDSTRING, 0, (LPARAM)bptext);
               }
               break;
            }
            case IDC_DELBP1:
            {
               LRESULT ret;
               char bptext[10];
               u32 addr=0;

               if ((ret = SendMessage(GetDlgItem(hDlg, IDC_LISTBOX3), LB_GETCURSEL, 0, 0)) != LB_ERR)
               {
                  SendMessage(GetDlgItem(hDlg, IDC_LISTBOX3), LB_GETTEXT, 0, (LPARAM)bptext);
                  sscanf(bptext, "%X", &addr);
                  SH2DelCodeBreakpoint(debugsh, addr);
                  SendMessage(GetDlgItem(hDlg, IDC_LISTBOX3), LB_DELETESTRING, ret, 0);
               }
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

                     sh2regs_struct sh2regs;
                     SH2GetRegisters(debugsh, &sh2regs);
                     cursel = SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_GETCURSEL,0,0);

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
                           SH2UpdateCodeList(hDlg, sh2regs.PC);
                        }
                     }

                     SH2SetRegisters(debugsh, &sh2regs);
                     SH2UpdateRegList(hDlg, &sh2regs);
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

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK VDP1DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam)
{
   char tempstr[1024];

   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         char *string;
         u32 i=0;

         // execute yabause until vblank-out
         while (yabsys.LineCount != 262)
         {
            if (YabauseExec() != 0)
               return FALSE;
         }

         // Build command list
         SendMessage(GetDlgItem(hDlg, IDC_VDP1CMDLB), LB_RESETCONTENT, 0, 0);

         for (;;)
         {
            if ((string = Vdp1DebugGetCommandNumberName(i)) == NULL)
               break;

            SendMessage(GetDlgItem(hDlg, IDC_VDP1CMDLB), LB_ADDSTRING, 0, (LPARAM)string);

            i++;
         }

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDC_VDP1CMDLB:
            {
               switch(HIWORD(wParam))
               {
                  case LBN_SELCHANGE:
                  {
                     u8 cursel=0;

                     cursel = (u8)SendDlgItemMessage(hDlg, IDC_VDP1CMDLB, LB_GETCURSEL, 0, 0);

                     Vdp1DebugCommand(cursel, tempstr);
                     SetDlgItemText(hDlg, IDC_VDP1CMDET, tempstr);

                     return TRUE;
                  }
                  default: break;
               }

               return TRUE;
            }
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

LRESULT CALLBACK VDP2DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         char tempstr[1024];
         int isscrenabled;

         // is NBG0/RBG1 enabled?
         Vdp2DebugStatsNBG0(tempstr, &isscrenabled);

         if (isscrenabled)
         {
            SendMessage(GetDlgItem(hDlg, IDC_NBG0ENABCB), BM_SETCHECK, BST_CHECKED, 0);
            SetDlgItemText(hDlg, IDC_NBG0ET, tempstr);
         }
         else
            SendMessage(GetDlgItem(hDlg, IDC_NBG0ENABCB), BM_SETCHECK, BST_UNCHECKED, 0);

         Vdp2DebugStatsNBG1(tempstr, &isscrenabled);

         // is NBG1 enabled?
         if (isscrenabled)
         {
            // enabled
            SendMessage(GetDlgItem(hDlg, IDC_NBG1ENABCB), BM_SETCHECK, BST_CHECKED, 0);
            SetDlgItemText(hDlg, IDC_NBG1ET, tempstr);
         }
         else
            // disabled
            SendMessage(GetDlgItem(hDlg, IDC_NBG1ENABCB), BM_SETCHECK, BST_UNCHECKED, 0);

         Vdp2DebugStatsNBG2(tempstr, &isscrenabled);

         // is NBG2 enabled?
         if (isscrenabled)
         {
            // enabled
            SendMessage(GetDlgItem(hDlg, IDC_NBG2ENABCB), BM_SETCHECK, BST_CHECKED, 0);
            SetDlgItemText(hDlg, IDC_NBG2ET, tempstr);
         }
         else
            // disabled
            SendMessage(GetDlgItem(hDlg, IDC_NBG2ENABCB), BM_SETCHECK, BST_UNCHECKED, 0);

         Vdp2DebugStatsNBG3(tempstr, &isscrenabled);

         // is NBG3 enabled?
         if (isscrenabled)
         {
            // enabled
            SendMessage(GetDlgItem(hDlg, IDC_NBG3ENABCB), BM_SETCHECK, BST_CHECKED, 0);
            SetDlgItemText(hDlg, IDC_NBG3ET, tempstr);
         }
         else
            // disabled
            SendMessage(GetDlgItem(hDlg, IDC_NBG3ENABCB), BM_SETCHECK, BST_UNCHECKED, 0);

         Vdp2DebugStatsRBG0(tempstr, &isscrenabled);

         // is RBG0 enabled?
         if (isscrenabled)
         {
            // enabled
            SendMessage(GetDlgItem(hDlg, IDC_RBG0ENABCB), BM_SETCHECK, BST_CHECKED, 0);
            SetDlgItemText(hDlg, IDC_RBG0ET, tempstr);
         }
         else
            // disabled
            SendMessage(GetDlgItem(hDlg, IDC_RBG0ENABCB), BM_SETCHECK, BST_UNCHECKED, 0);

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

            default: break;
         }
         break;
      }
      default: break;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

void M68KUpdateRegList(HWND hDlg, m68kregs_struct *regs)
{
   char tempstr[128];
   int i;

   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_RESETCONTENT, 0, 0);

   // Data registers
   for (i = 0; i < 8; i++)
   {
      sprintf(tempstr, "D%d =   %08x", i, (int)regs->D[i]);
      strupr(tempstr);
      SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);
   }

   // Address registers
   for (i = 0; i < 8; i++)
   {
      sprintf(tempstr, "A%d =   %08x", i, (int)regs->A[i]);
      strupr(tempstr);
      SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);
   }

   // SR
   sprintf(tempstr, "SR =   %08x", (int)regs->SR);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   // PC
   sprintf(tempstr, "PC =   %08x", (int)regs->PC);
   strupr(tempstr);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);
}

//////////////////////////////////////////////////////////////////////////////

void M68KUpdateCodeList(HWND hDlg, u32 addr)
{
/*
   u32 buf_size;
   u32 buf_addr;
   int i, i2;
   char buf[60];
   u32 offset;
   char op[64], inst[32], arg[24];
   unsigned char *buffer;
   u32 op_size;

   buffer = ((ScspRam *)((Scsp *)mem->soundr)->getSRam())->getBuffer();
        
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_RESETCONTENT, 0, 0);

//   offset = addr - (12 * 2);
   offset = addr;

   for (i = 0; i < 24; i++)
   {
      op_size += Dis68000One(offset, &buffer[offset], op, inst, arg);
      sprintf(buf, "%06x: %s %s", offset, inst, arg);
      offset += op_size;

      SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_ADDSTRING, 0,
                  (s32)buf);
   }

//   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_SETCURSEL,12,0);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_SETCURSEL,0,0);
*/
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK M68KDebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         m68kregs_struct m68kregs;

/*
         SendMessage(GetDlgItem(hDlg, IDC_CHKREAD), BM_SETCHECK, BST_UNCHECKED, 0);
         SendMessage(GetDlgItem(hDlg, IDC_CHKWRITE), BM_SETCHECK, BST_UNCHECKED, 0);
         SendMessage(GetDlgItem(hDlg, IDC_CHKBYTE), BM_SETCHECK, BST_UNCHECKED, 0);
         SendMessage(GetDlgItem(hDlg, IDC_CHKWORD), BM_SETCHECK, BST_UNCHECKED, 0);
         SendMessage(GetDlgItem(hDlg, IDC_CHKDWORD), BM_SETCHECK, BST_UNCHECKED, 0);
*/

         EnableWindow(GetDlgItem(hDlg, IDC_STEP), TRUE);

         M68KGetRegisters(&m68kregs);
         M68KUpdateRegList(hDlg, &m68kregs);
         M68KUpdateCodeList(hDlg, m68kregs.PC);

         return TRUE;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam))
         {
            case IDOK:
            {
/*
               if (scsprunthreadhandle != INVALID_HANDLE_VALUE)
               {
                  killScspRunThread=1;

                  // wait for thread to end(should really set it to timeout after a
                  // certain time so I can test for keypresses)
                  WaitForSingleObject(scsprunthreadhandle,INFINITE);
                  CloseHandle(scsprunthreadhandle);                                           
                  scsprunthreadhandle = INVALID_HANDLE_VALUE;
               }
*/

               EndDialog(hDlg, TRUE);

               return TRUE;
            }
            case IDC_STEP:
            {
               m68kregs_struct m68kregs;

               // execute instruction
               M68KStep();

               M68KGetRegisters(&m68kregs);
               M68KUpdateRegList(hDlg, &m68kregs);
               M68KUpdateCodeList(hDlg, m68kregs.PC);
               break;
            }
            case IDC_ADDBP1:
            {
               // add a code breakpoint
               break;
            }
            case IDC_DELBP1:
            {
               // delete a code breakpoint
               break;
            }
/*
            case IDC_ADDBP2:
            {
               // add a memory breakpoint
               break;
            }
*/
            case IDC_LISTBOX1:
            {
               switch (HIWORD(wParam))
               {
                  case LBN_DBLCLK:
                  {
                     // dialogue for changing register values
                     int cursel;
                     m68kregs_struct m68kregs;

                     cursel = SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_GETCURSEL,0,0);

                     M68KGetRegisters(&m68kregs);

                     switch (cursel)
                     {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                           memaddr = m68kregs.D[cursel];                           
                           break;
                        case 8:
                        case 9:
                        case 10:
                        case 11:
                        case 12:
                        case 13:
                        case 14:
                        case 15:
                           memaddr = m68kregs.A[cursel - 8];
                           break;
                        case 16:
                           memaddr = m68kregs.SR;
                           break;
                        case 17:
                           memaddr = m68kregs.PC;
                           break;
                        default: break;
                     }

                     if (DialogBox(y_hInstance, "MemDlg", hDlg, (DLGPROC)MemDlgProc) == TRUE)
                     {
                        switch (cursel)
                        {
                           case 0:
                           case 1:
                           case 2:
                           case 3:
                           case 4:
                           case 5:
                           case 6:
                           case 7:
                              m68kregs.D[cursel] = memaddr;
                              break;
                           case 8:
                           case 9:
                           case 10:
                           case 11:
                           case 12:
                           case 13:
                           case 14:
                           case 15:
                              m68kregs.A[cursel - 8] = memaddr;
                              break;
                           case 16:
                              m68kregs.SR = memaddr;
                              break;
                           case 17:
                              m68kregs.PC = memaddr;
                              M68KUpdateCodeList(hDlg, m68kregs.PC);
                              break;
                           default: break;
                        }

                        M68KSetRegisters(&m68kregs);
                     }

                     M68KUpdateRegList(hDlg, &m68kregs);
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

//////////////////////////////////////////////////////////////////////////////

void SCUDSPUpdateRegList(HWND hDlg, scudspregs_struct *regs)
{
   char tempstr[128];

   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_RESETCONTENT, 0, 0);

   sprintf(tempstr, "PR = %d   EP = %d", regs->ProgControlPort.part.PR, regs->ProgControlPort.part.EP);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "T0 = %d   S =  %d", regs->ProgControlPort.part.T0, regs->ProgControlPort.part.S);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "Z =  %d   C =  %d", regs->ProgControlPort.part.Z, regs->ProgControlPort.part.C);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "V =  %d   E =  %d", regs->ProgControlPort.part.V, regs->ProgControlPort.part.E);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "ES = %d   EX = %d", regs->ProgControlPort.part.ES, regs->ProgControlPort.part.EX);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "LE =          %d", regs->ProgControlPort.part.LE);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "P =          %02X", regs->ProgControlPort.part.P);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "TOP =        %02X", regs->TOP);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "LOP =        %02X", regs->LOP);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "CT = %02X:%02X:%02X:%02X", regs->CT[0], regs->CT[1], regs->CT[2], regs->CT[3]);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "RA =   %08X", regs->RA0);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "WA =   %08X", regs->WA0);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "RX =   %08X", regs->RX);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "RY =   %08X", regs->RX);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "PH =       %04X", regs->P.part.H & 0xFFFF);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "PL =   %08X", regs->P.part.L & 0xFFFFFFFF);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "ACH =      %04X", regs->AC.part.H & 0xFFFF);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);

   sprintf(tempstr, "ACL =  %08X", regs->AC.part.L & 0xFFFFFFFF);
   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX1), LB_ADDSTRING, 0, (LPARAM)tempstr);
}

//////////////////////////////////////////////////////////////////////////////

void SCUDSPUpdateCodeList(HWND hDlg, u32 addr)
{
   int i;
   char buf[60];
   u32 offset;

   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_RESETCONTENT, 0, 0);

   offset = addr;

   for (i = 0; i < 24; i++)
   {
      ScuDspDisasm(offset, buf);
      offset++;

      SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_ADDSTRING, 0,
                  (s32)buf);
   }

   SendMessage(GetDlgItem(hDlg, IDC_LISTBOX2), LB_SETCURSEL,0,0);
}

//////////////////////////////////////////////////////////////////////////////

void SCUDSPBreakpointHandler (u32 addr)
{
   MessageBox (NULL, "Breakpoint Reached", "Notice",  MB_OK | MB_ICONINFORMATION);
   DialogBox(y_hInstance, "SCUDSPDebugDlg", YabWin, (DLGPROC)SCUDSPDebugDlgProc);
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK SCUDSPDebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
      {
         scudspregs_struct dspregs;
         scucodebreakpoint_struct *cbp;
         char tempstr[10];
         int i;

         cbp = ScuDspGetBreakpointList();

         for (i = 0; i < MAX_BREAKPOINTS; i++)
         {
            if (cbp[i].addr != 0xFFFFFFFF)
            {
               sprintf(tempstr, "%02X", (int)cbp[i].addr);
               SendMessage(GetDlgItem(hDlg, IDC_LISTBOX3), LB_ADDSTRING, 0, (LPARAM)tempstr);
            }
         }

         EnableWindow(GetDlgItem(hDlg, IDC_STEP), TRUE);

         ScuDspGetRegisters(&dspregs);
         SCUDSPUpdateRegList(hDlg, &dspregs);
         SCUDSPUpdateCodeList(hDlg, dspregs.ProgControlPort.part.P);

         ScuDspSetBreakpointCallBack(&SCUDSPBreakpointHandler);

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
               scudspregs_struct dspregs;

               ScuDspStep();

               ScuDspGetRegisters(&dspregs);
               SCUDSPUpdateRegList(hDlg, &dspregs);
               SCUDSPUpdateCodeList(hDlg, dspregs.ProgControlPort.part.P);

               break;
            }
            case IDC_ADDBP1:
            {
               // add a code breakpoint
               char bptext[10];
               u32 addr=0;
               memset(bptext, 0, 4);
               GetDlgItemText(hDlg, IDC_EDITTEXT1, bptext, 4);

               if (bptext[0] != 0)
               {
                  sscanf(bptext, "%X", &addr);
                  sprintf(bptext, "%02X", (int)addr);

                  if (ScuDspAddCodeBreakpoint(addr) == 0)
                     SendMessage(GetDlgItem(hDlg, IDC_LISTBOX3), LB_ADDSTRING, 0, (LPARAM)bptext);
               }
               break;
            }
            case IDC_DELBP1:
            {
               // delete a code breakpoint
               LRESULT ret;
               char bptext[10];
               u32 addr=0;

               if ((ret = SendMessage(GetDlgItem(hDlg, IDC_LISTBOX3), LB_GETCURSEL, 0, 0)) != LB_ERR)
               {
                  SendMessage(GetDlgItem(hDlg, IDC_LISTBOX3), LB_GETTEXT, 0, (LPARAM)bptext);
                  sscanf(bptext, "%X", &addr);
                  ScuDspDelCodeBreakpoint(addr);
                  SendMessage(GetDlgItem(hDlg, IDC_LISTBOX3), LB_DELETESTRING, ret, 0);
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

//////////////////////////////////////////////////////////////////////////////
