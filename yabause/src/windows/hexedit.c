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
#include "../core.h"
#include "../memory.h"
#include "../debug.h"

typedef struct
{
   HWND hwnd;
   HFONT font;
   COLORREF text_color1;
   COLORREF text_color2;
   COLORREF text_color3;
   COLORREF edit_color;
   COLORREF bg_color;
   int hasfocus;
   u32 addr;
   SCROLLINFO scrollinfo;
   TEXTMETRIC fontmetric;
   int curx, cury;
   int maxcurx, maxcury;
   int curmode;
   int editmode;
} HexEditCtl_struct;

//////////////////////////////////////////////////////////////////////////////

enum CURMODE { HEXMODE = 0x00, ASCIIMODE };

//////////////////////////////////////////////////////////////////////////////

LRESULT InitHexEditCtl(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
   HexEditCtl_struct *cc;

   if ((cc = (HexEditCtl_struct *)malloc(sizeof(HexEditCtl_struct))) == NULL)
      return FALSE;

   cc->hwnd = hwnd;
   cc->font = GetStockObject(DEFAULT_GUI_FONT);
   cc->text_color1 = GetSysColor(COLOR_WINDOWTEXT);
   cc->text_color2 = RGB(0, 0, 255);
   cc->text_color3 = RGB(0, 0, 128);
   cc->edit_color = GetSysColor(COLOR_WINDOWTEXT);
   cc->bg_color = GetSysColor(COLOR_WINDOW);
   cc->hasfocus = FALSE;
   cc->addr = 0;
   cc->curx = 0;
   cc->cury = 0;
   cc->maxcurx = 16;
   cc->maxcury = 16;
   cc->curmode = HEXMODE;
   cc->editmode = 0;

   // Set the text
   SetWindowText(hwnd, ((CREATESTRUCT *)lParam)->lpszName);

   // Retrieve scroll info
   cc->scrollinfo.cbSize = sizeof(SCROLLINFO);
   cc->scrollinfo.fMask = SIF_RANGE;
   GetScrollInfo(hwnd, SB_VERT, &cc->scrollinfo);

   // Set our newly created structure to the extra area                                                                                                SetCustCtrl(hwnd, ccp);
   SetWindowLong(hwnd, 0, (LONG)cc);
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////

void DestroyHexEditCtl(HexEditCtl_struct *cc)
{
   if (cc)
      free(cc);
}

//////////////////////////////////////////////////////////////////////////////

LRESULT HexEditCtl_SetFont(HexEditCtl_struct *cc, WPARAM wParam, LPARAM lParam)
{
   HDC hdc;
   HFONT oldfont;

   cc->font = (HFONT)wParam;
   hdc = GetDC(cc->hwnd);
   oldfont = SelectObject(hdc, cc->font);
   GetTextMetrics(hdc, &cc->fontmetric);
   SelectObject(hdc, oldfont);
   ReleaseDC(cc->hwnd, hdc);
   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

void HexEditCtl_SetCaretPos(HexEditCtl_struct *cc)
{
   if (cc->curmode == HEXMODE)   
      SetCaretPos((10 + (cc->curx*2) + (cc->curx / 2) + cc->editmode) * cc->fontmetric.tmAveCharWidth,
                  cc->cury*cc->fontmetric.tmHeight);
   else
      SetCaretPos((10+32+8+ cc->curx) * cc->fontmetric.tmAveCharWidth,
                  cc->cury*cc->fontmetric.tmHeight);
}

//////////////////////////////////////////////////////////////////////////////

void HexEditCtl_SetFocus(HexEditCtl_struct *cc)
{
   cc->hasfocus = TRUE;
   CreateCaret(cc->hwnd, NULL, 2, cc->fontmetric.tmHeight);
   HexEditCtl_SetCaretPos(cc);
   ShowCaret(cc->hwnd);

   InvalidateRect(cc->hwnd, NULL, FALSE);
   UpdateWindow(cc->hwnd); 
}

//////////////////////////////////////////////////////////////////////////////

void HexEditCtl_KillFocus(HexEditCtl_struct *cc)
{
   cc->hasfocus = FALSE;
   DestroyCaret(); 
   InvalidateRect(cc->hwnd, NULL, FALSE);
   UpdateWindow(cc->hwnd); 
}

//////////////////////////////////////////////////////////////////////////////

LRESULT HexEditCtl_OnPaint(HexEditCtl_struct *cc, WPARAM wParam, LPARAM lParam)
{
   HDC hdc=(HDC)wParam;
   PAINTSTRUCT ps;
   RECT rect;
   HANDLE oldfont;
   int x=0, y=0;
   char text[MAX_PATH];
   u32 addr;
   SIZE size;
   int i;
   RECT clip;

   // Setup everything for rendering
   if (hdc == NULL)
      hdc = BeginPaint(cc->hwnd, &ps);
   oldfont = SelectObject(hdc, cc->font);

   GetClientRect(cc->hwnd, &rect);

   addr = cc->addr;
   for(;;)
   {
      x = 0;
      sprintf(text, "%08X  ", (int)addr);
      GetTextExtentPoint32(hdc, text, strlen(text), &size);
      if (size.cy+y >= rect.bottom)
         break;

      // adjust clipping values
      if (y+(size.cy*2) >= rect.bottom)
         clip.bottom = rect.bottom;
      else
         clip.bottom = y+size.cy;

      // Draw the address text
      clip.left = x;
      clip.top = y;
      clip.right = clip.left+size.cx;
      SetTextColor(hdc, cc->text_color1);
      ExtTextOut(hdc, x, y, ETO_OPAQUE | ETO_CLIPPED, &clip, text, lstrlen(text), 0);
      x += size.cx;

      // Draw the Hex values for the address
      for (i = 0; i < 16; i+=2)
      {
         if ((i % 4) == 0)
            SetTextColor(hdc, cc->text_color2);
         else
            SetTextColor(hdc, cc->text_color3);
         sprintf(text, "%02X%02X ", MappedMemoryReadByte(addr), MappedMemoryReadByte(addr+1));
         GetTextExtentPoint32(hdc, text, strlen(text), &size);
         clip.left = x;
         clip.top = y;
         clip.right = clip.left+size.cx;
         ExtTextOut(hdc, x, y, ETO_OPAQUE | ETO_CLIPPED, &clip, text, lstrlen(text), 0);
         x += size.cx;
         addr += 2;
      }
      addr -= 16;

      // Draw the ANSI equivalents
      SetTextColor(hdc, cc->text_color1);

      for (i = 0; i < 16; i++)
      {
         u8 byte=MappedMemoryReadByte(addr);

         if (byte < 0x20 || byte >= 0x7F)
            byte = '.';

         text[0] = byte;
         text[1] = '\0';
         GetTextExtentPoint32(hdc, text, strlen(text), &size);
         clip.left = x;
         clip.top = y;
         clip.right = rect.right;
         ExtTextOut(hdc, x, y, ETO_OPAQUE | ETO_CLIPPED, &clip, text, lstrlen(text), 0);
         x += size.cx;
         addr++;
      }
      y += size.cy;
   }

   // Let's clean up, and we're done
   SelectObject(hdc, oldfont);

   if ((HDC)wParam == NULL)
      EndPaint(cc->hwnd, &ps);

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT HexEditCtl_Vscroll(HexEditCtl_struct *cc, WPARAM wParam, LPARAM lParam)
{
   switch (LOWORD(wParam))
   {
      case SB_LINEDOWN:
         if (cc->addr < 0xFFFFFF00)
         {
            cc->addr += 0x10;
            InvalidateRect(cc->hwnd, NULL, FALSE);
         }
         return 0;
      case SB_LINEUP:
         if (cc->addr >= 0x00000010)
         {
            cc->addr -= 0x10;
            InvalidateRect(cc->hwnd, NULL, FALSE);
         }
         return 0;
      case SB_PAGEDOWN:
        if (cc->addr <= 0xFFFFFE00)
        {
           cc->addr += 0x100;
           InvalidateRect(cc->hwnd, NULL, FALSE);
        }
        return 0;
     case SB_PAGEUP:
        if (cc->addr >= 0x00000100)
        {
           cc->addr -= 0x100;
           InvalidateRect(cc->hwnd, NULL, FALSE);
        }
        return 0;
     case SB_THUMBTRACK:
        cc->addr = 0xFFFFFF00 / 64 * HIWORD(wParam);
        InvalidateRect(cc->hwnd, NULL, FALSE);
        return 0;
     case SB_THUMBPOSITION:
        SetScrollPos(cc->hwnd, SB_VERT, HIWORD(wParam), TRUE);
        return 0;
     default:
        break;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT HexEditCtl_KeyDown(HexEditCtl_struct *cc, WPARAM wParam, LPARAM lParam)
{
   switch (wParam)
   {
      case VK_LEFT:
         if (cc->curx == 0)
         {
            if (cc->cury == 0)
            {
               if (cc->addr >= 0x00000010)
               {
                  cc->addr -= 0x10;
                  cc->curx = cc->maxcurx-1;
                  InvalidateRect(cc->hwnd, NULL, FALSE);
               }
            }
            else
            {
               cc->cury--;
               cc->curx = cc->maxcurx-1;
            }
         }
         else
            cc->curx--;
         HexEditCtl_SetCaretPos(cc);
         break;
      case VK_UP:
         if (cc->cury == 0)
         {
            if (cc->addr >= 0x00000010)
            {
               cc->addr -= 0x10;
               InvalidateRect(cc->hwnd, NULL, FALSE);
            }
            else
               cc->curx = 0;
         }
         else
            cc->cury--;
         HexEditCtl_SetCaretPos(cc);
         break;
      case VK_DOWN:
         if (cc->cury == (cc->maxcury-1))
         {
            if (cc->addr < 0xFFFFFF00)
            {
               cc->addr += 0x10;
               InvalidateRect(cc->hwnd, NULL, FALSE);
            }
            else
               cc->curx = cc->maxcurx-1;
         }
         else
            cc->cury++;
         HexEditCtl_SetCaretPos(cc);
         break;
      case VK_RIGHT:
         if (cc->curx == (cc->maxcurx-1))
         {
            if (cc->cury == (cc->maxcury-1))
            {
               if (cc->addr < 0xFFFFFF00)
               {
                  cc->addr += 0x10;
                  cc->curx = 0;
                  InvalidateRect(cc->hwnd, NULL, FALSE);
               }
            }
            else
            {
               cc->cury++;
               cc->curx = 0;
            }
         }
         else
            cc->curx++;
         HexEditCtl_SetCaretPos(cc);
         break;
      case VK_PRIOR:
         if (cc->addr >= 0x00000100)
         {
            cc->addr -= 0x100;
            InvalidateRect(cc->hwnd, NULL, FALSE);
         }
         else
         {
            cc->addr = 0;
            cc->curx = cc->cury = 0;
            InvalidateRect(cc->hwnd, NULL, FALSE);
            HexEditCtl_SetCaretPos(cc);
         }
         break;
      case VK_NEXT:
         if (cc->addr <= 0xFFFFFE00)
         {
            cc->addr += 0x100;
            InvalidateRect(cc->hwnd, NULL, FALSE);
         }
         else
         {
            cc->curx = cc->maxcurx-1;
            cc->cury = cc->maxcury-1;
            HexEditCtl_SetCaretPos(cc);
         }
         break;
      case VK_END:
         cc->addr = 0xFFFFFF00;
         cc->curx = cc->maxcurx-1;
         cc->cury = cc->maxcury-1;
         InvalidateRect(cc->hwnd, NULL, FALSE);
         HexEditCtl_SetCaretPos(cc);
         break;
      case VK_HOME:
         cc->addr = 0;
         cc->curx = cc->cury = 0;
         InvalidateRect(cc->hwnd, NULL, FALSE);
         HexEditCtl_SetCaretPos(cc);
         break;
      case VK_TAB:
         cc->curmode ^= ASCIIMODE;
         HexEditCtl_SetCaretPos(cc);
         break;
      default:
         if (cc->curmode == HEXMODE)
         {
            if ((wParam >= '0' && wParam <= '9') ||
                (wParam >= 'A' && wParam <= 'F'))
            {
               u32 addr;
               u8 data;

               if (wParam >= '0' && wParam <= '9')
                  data = wParam - 0x30;
               else
                  data = wParam - 0x37;

               // Modify data in memory
               addr = cc->addr + (cc->cury * cc->maxcurx) + cc->curx;
               if (cc->editmode == 0)
                  data = (data << 4) | (MappedMemoryReadByte(addr) & 0x0F);
               else
                  data = (MappedMemoryReadByte(addr) & 0xF0) | data;

               MappedMemoryWriteByte(addr, data);

               cc->editmode ^= 1;
               if (cc->editmode == 0)
                   HexEditCtl_KeyDown(cc, VK_RIGHT, 0);                  
               InvalidateRect(cc->hwnd, NULL, FALSE);
               HexEditCtl_SetCaretPos(cc);
            }
         }
         else
         {
            u32 addr;
            u8 data;

            // So long as it's an ANSI character, we're all good

            // Modify data in memory
//            addr = cc->addr + (cc->cury * cc->maxcurx) + cc->curx;
//            MappedMemoryWriteByte(addr, data);
         }
         break;
   }
   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK HexEditCtl(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   HexEditCtl_struct *cc=(HexEditCtl_struct *)GetWindowLong(hwnd, 0);

   switch(message)
   {
      case WM_NCCREATE:
         return InitHexEditCtl(hwnd, wParam, lParam);
      case WM_NCDESTROY:
         DestroyHexEditCtl((HexEditCtl_struct *)cc);
         break;
      case WM_PAINT:
         return HexEditCtl_OnPaint(cc, wParam, lParam);
      case WM_ERASEBKGND:
         return TRUE;
      case WM_SETFONT:
         return HexEditCtl_SetFont(cc, wParam, lParam);
      case WM_SETFOCUS:
         HexEditCtl_SetFocus(cc);
         break;
      case WM_KILLFOCUS:
         HexEditCtl_KillFocus(cc);
         break;
      case WM_VSCROLL:
         return HexEditCtl_Vscroll(cc, wParam, lParam);
      case WM_GETDLGCODE:
         return DLGC_WANTALLKEYS;
      case WM_KEYDOWN:
         return HexEditCtl_KeyDown(cc, wParam, lParam);
      default:
         break;
   }

   return DefWindowProc(hwnd, message, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////////

void InitHexEdit()
{
   WNDCLASSEX wc;

   wc.cbSize         = sizeof(wc);
   wc.lpszClassName  = "YabauseHexEdit";
   wc.hInstance      = GetModuleHandle(0);
   wc.lpfnWndProc    = HexEditCtl;
   wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
   wc.hIcon          = 0;
   wc.lpszMenuName   = 0;
   wc.hbrBackground  = (HBRUSH)GetSysColorBrush(COLOR_WINDOW);
   wc.style          = 0;
   wc.cbClsExtra     = 0;
   wc.cbWndExtra     = sizeof(HexEditCtl_struct *);
   wc.hIconSm        = 0;

   RegisterClassEx(&wc);
}

//////////////////////////////////////////////////////////////////////////////

