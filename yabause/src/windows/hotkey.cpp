///*  This file is part of DeSmuME, derived from several files in Snes9x 1.51 which are 
//    licensed under the terms supplied at the end of this file (for the terms are very long!)
//    Differences from that baseline version are:
//
//    Copyright (C) 2009 DeSmuME team
//
//    DeSmuME is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    DeSmuME is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with DeSmuME; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//*/

#include "hotkey.h"
#include "resource.h"
#include "ramwatch.h"		//In order to call UpdateRamWatch (for loadstate functions)
#include "ram_search.h"		//In order to call UpdateRamSearch (for loadstate functions)

#include <commctrl.h>
extern "C" {
#include "../movie.h"
#include "../vdp2.h"
#include "../vdp1.h"
#include "./settings/settings.h"

static HWND YabWin;
static HINSTANCE y_hInstance;
}

extern void TogglePause();	//adelikat: TODO: maybe there should be a driver.h or a main.h to put this and the handles in
extern void ResetGame();
extern void HardResetGame();
extern void SaveState(int num);
extern void LoadState(int num);
extern void YuiPlayMovie(HWND hWnd);
extern void YuiRecordMovie(HWND hWnd);
extern void ToggleFullScreenHK();
extern void YuiScreenshot(HWND hWnd);
extern void YuiRecordAvi(HWND hWnd);
extern void YuiStopAvi();

static TCHAR szHotkeysClassName[] = _T("InputCustomHot");

static LRESULT CALLBACK HotInputCustomWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

//extern LRESULT OpenFile();	//adelikat: Made this an extern here instead of main.h  Seemed icky not to limit the scope of this function

SCustomKeys CustomKeys;

bool AutoHoldPressed=false;

int SaveStateSlot=1;

///////////////////////////

#define INPUTCONFIG_LABEL_BLUE "Blue means the button is already mapped.\nPink means it conflicts with a custom hotkey.\nRed means it's reserved by Windows.\nButtons can be disabled using Escape.\nGrayed buttons arent supported yet (sorry!)"
#define INPUTCONFIG_LABEL_UNUSED ""

// gaming buttons and axes
#define GAMEDEVICE_JOYNUMPREFIX "(J%x)" // don't change this
#define GAMEDEVICE_JOYBUTPREFIX "#[%d]" // don't change this
#define GAMEDEVICE_XNEG "Left"
#define GAMEDEVICE_XPOS "Right"
#define GAMEDEVICE_YPOS "Up"
#define GAMEDEVICE_YNEG "Down"
#define GAMEDEVICE_POVLEFT "POV Left"
#define GAMEDEVICE_POVRIGHT "POV Right"
#define GAMEDEVICE_POVUP "POV Up"
#define GAMEDEVICE_POVDOWN "POV Down"
#define GAMEDEVICE_POVDNLEFT "POV Dn Left"
#define GAMEDEVICE_POVDNRIGHT "POV Dn Right"
#define GAMEDEVICE_POVUPLEFT  "POV Up Left"
#define GAMEDEVICE_POVUPRIGHT "POV Up Right"
#define GAMEDEVICE_ZPOS "Z Up"
#define GAMEDEVICE_ZNEG "Z Down"
#define GAMEDEVICE_RPOS "R Up"
#define GAMEDEVICE_RNEG "R Down"
#define GAMEDEVICE_UPOS "U Up"
#define GAMEDEVICE_UNEG "U Down"
#define GAMEDEVICE_VPOS "V Up"
#define GAMEDEVICE_VNEG "V Down"
#define GAMEDEVICE_BUTTON "Button %d"

// gaming general
#define GAMEDEVICE_DISABLED "Disabled"

// gaming keys
#define GAMEDEVICE_KEY "#%d"
#define GAMEDEVICE_NUMPADPREFIX "Numpad-%c"
#define GAMEDEVICE_VK_TAB "Tab"
#define GAMEDEVICE_VK_BACK "Backspace"
#define GAMEDEVICE_VK_CLEAR "Delete"
#define GAMEDEVICE_VK_RETURN "Enter"
#define GAMEDEVICE_VK_LSHIFT "LShift"
#define GAMEDEVICE_VK_RSHIFT "RShift"
#define GAMEDEVICE_VK_LCONTROL "LCtrl"
#define GAMEDEVICE_VK_RCONTROL "RCtrl"
#define GAMEDEVICE_VK_LMENU "LAlt"
#define GAMEDEVICE_VK_RMENU "RAlt"
#define GAMEDEVICE_VK_PAUSE "Pause"
#define GAMEDEVICE_VK_CAPITAL "Capslock"
#define GAMEDEVICE_VK_ESCAPE "Disabled"
#define GAMEDEVICE_VK_SPACE "Space"
#define GAMEDEVICE_VK_PRIOR "PgUp"
#define GAMEDEVICE_VK_NEXT "PgDn"
#define GAMEDEVICE_VK_HOME "Home"
#define GAMEDEVICE_VK_END "End"
#define GAMEDEVICE_VK_LEFT "Left"
#define GAMEDEVICE_VK_RIGHT "Right"
#define GAMEDEVICE_VK_UP "Up"
#define GAMEDEVICE_VK_DOWN "Down"
#define GAMEDEVICE_VK_SELECT "Select"
#define GAMEDEVICE_VK_PRINT "Print"
#define GAMEDEVICE_VK_EXECUTE "Execute"
#define GAMEDEVICE_VK_SNAPSHOT "SnapShot"
#define GAMEDEVICE_VK_INSERT "Insert"
#define GAMEDEVICE_VK_DELETE "Delete"
#define GAMEDEVICE_VK_HELP "Help"
#define GAMEDEVICE_VK_LWIN "LWinKey"
#define GAMEDEVICE_VK_RWIN "RWinKey"
#define GAMEDEVICE_VK_APPS "AppKey"
#define GAMEDEVICE_VK_MULTIPLY "Numpad *"
#define GAMEDEVICE_VK_ADD "Numpad +"
#define GAMEDEVICE_VK_SEPARATOR "Separator"
#define GAMEDEVICE_VK_OEM_1 "Semi-Colon"
#define GAMEDEVICE_VK_OEM_7 "Apostrophe"
#define GAMEDEVICE_VK_OEM_COMMA "Comma"
#define GAMEDEVICE_VK_OEM_PERIOD "Period"
#define GAMEDEVICE_VK_SUBTRACT "Numpad -"
#define GAMEDEVICE_VK_DECIMAL "Numpad ."
#define GAMEDEVICE_VK_DIVIDE "Numpad /"
#define GAMEDEVICE_VK_NUMLOCK "Num-lock"
#define GAMEDEVICE_VK_SCROLL "Scroll-lock"
#define GAMEDEVICE_VK_OEM_MINUS "-"
#define GAMEDEVICE_VK_OEM_PLUS "="
#define GAMEDEVICE_VK_SHIFT "Shift"
#define GAMEDEVICE_VK_CONTROL "Control"
#define GAMEDEVICE_VK_MENU "Alt"
#define GAMEDEVICE_VK_OEM_4 "["
#define GAMEDEVICE_VK_OEM_6 "]"
#define GAMEDEVICE_VK_OEM_5 "\\"
#define GAMEDEVICE_VK_OEM_2 "/"
#define GAMEDEVICE_VK_OEM_3 "`"
#define GAMEDEVICE_VK_F1 "F1"
#define GAMEDEVICE_VK_F2 "F2"
#define GAMEDEVICE_VK_F3 "F3"
#define GAMEDEVICE_VK_F4 "F4"
#define GAMEDEVICE_VK_F5 "F5"
#define GAMEDEVICE_VK_F6 "F6"
#define GAMEDEVICE_VK_F7 "F7"
#define GAMEDEVICE_VK_F8 "F8"
#define GAMEDEVICE_VK_F9 "F9"
#define GAMEDEVICE_VK_F10 "F10"
#define GAMEDEVICE_VK_F11 "F11"
#define GAMEDEVICE_VK_F12 "F12"
#define BUTTON_OK "&OK"
#define BUTTON_CANCEL "&Cancel"


////////////////////////////

// Hotkeys Dialog Strings
#define HOTKEYS_TITLE "Hotkey Configuration"
#define HOTKEYS_CONTROL_MOD "Ctrl + "
#define HOTKEYS_SHIFT_MOD "Shift + "
#define HOTKEYS_ALT_MOD "Alt + "
#define HOTKEYS_LABEL_BLUE "Blue means the hotkey is already mapped.\nPink means it conflicts with a game button.\nRed means it's reserved by Windows.\nA hotkey can be disabled using Escape."
#define HOTKEYS_HKCOMBO "Page %d"

#define CUSTKEY_ALT_MASK   0x01
#define CUSTKEY_CTRL_MASK  0x02
#define CUSTKEY_SHIFT_MASK 0x04

#define NUM_HOTKEY_CONTROLS 20

#define COUNT(a) (sizeof (a) / sizeof (a[0]))

const int IDC_LABEL_HK_Table[NUM_HOTKEY_CONTROLS] = {
	IDC_LABEL_HK1 , IDC_LABEL_HK2 , IDC_LABEL_HK3 , IDC_LABEL_HK4 , IDC_LABEL_HK5 ,
	IDC_LABEL_HK6 , IDC_LABEL_HK7 , IDC_LABEL_HK8 , IDC_LABEL_HK9 , IDC_LABEL_HK10,
	IDC_LABEL_HK11, IDC_LABEL_HK12, IDC_LABEL_HK13, IDC_LABEL_HK14, IDC_LABEL_HK15,
	IDC_LABEL_HK16, IDC_LABEL_HK17, IDC_LABEL_HK18, IDC_LABEL_HK19, IDC_LABEL_HK20,
};
const int IDC_HOTKEY_Table[NUM_HOTKEY_CONTROLS] = {
	IDC_HOTKEY1 , IDC_HOTKEY2 , IDC_HOTKEY3 , IDC_HOTKEY4 , IDC_HOTKEY5 ,
	IDC_HOTKEY6 , IDC_HOTKEY7 , IDC_HOTKEY8 , IDC_HOTKEY9 , IDC_HOTKEY10,
	IDC_HOTKEY11, IDC_HOTKEY12, IDC_HOTKEY13, IDC_HOTKEY14, IDC_HOTKEY15,
	IDC_HOTKEY16, IDC_HOTKEY17, IDC_HOTKEY18, IDC_HOTKEY19, IDC_HOTKEY20,
};

typedef struct
{
    COLORREF crForeGnd;    // Foreground text colour
    COLORREF crBackGnd;    // Background text colour
    HFONT    hFont;        // The font
    HWND     hwnd;         // The control's window handle
} InputCust;
InputCust * GetInputCustom(HWND hwnd);

static TCHAR szClassName[] = _T("InputCustom");

/////////////

DWORD hKeyInputTimer;

int KeyInDelayInCount=10;

static int lastTime = timeGetTime();
int repeattime;

/////////////

bool S9xGetState (WORD KeyIdent)
{
	if(KeyIdent == 0 || KeyIdent == VK_ESCAPE) // if it's the 'disabled' key, it's never pressed
		return true;

	//TODO - option for background game keys
	if(YabWin != GetForegroundWindow())
		return true;

    if (KeyIdent & 0x8000) // if it's a joystick 'key':
    {
        int j = (KeyIdent >> 8) & 15;

		//S9xUpdateJoyState();

        switch (KeyIdent & 0xff)
        {
/*            case 0: return !Joystick [j].Left;  //TODO maybe
            case 1: return !Joystick [j].Right;
            case 2: return !Joystick [j].Up;
            case 3: return !Joystick [j].Down;
            case 4: return !Joystick [j].PovLeft;
            case 5: return !Joystick [j].PovRight;
            case 6: return !Joystick [j].PovUp;
            case 7: return !Joystick [j].PovDown;
			case 49:return !Joystick [j].PovDnLeft;
			case 50:return !Joystick [j].PovDnRight;
			case 51:return !Joystick [j].PovUpLeft;
			case 52:return !Joystick [j].PovUpRight;
            case 41:return !Joystick [j].ZUp;
            case 42:return !Joystick [j].ZDown;
            case 43:return !Joystick [j].RUp;
            case 44:return !Joystick [j].RDown;
            case 45:return !Joystick [j].UUp;
            case 46:return !Joystick [j].UDown;
            case 47:return !Joystick [j].VUp;
            case 48:return !Joystick [j].VDown;*/

            default:
                if ((KeyIdent & 0xff) > 40)
                    return true; // not pressed

//                return !Joystick [j].Button [(KeyIdent & 0xff) - 8]; //TODO maybe
        }
    }

	// the pause key is special, need this to catch all presses of it
	// Both GetKeyState and GetAsyncKeyState cannot catch it anyway,
	// so this should be handled in WM_KEYDOWN message.
	if(KeyIdent == VK_PAUSE)
	{
		return true; // not pressed
//		if(GetAsyncKeyState(VK_PAUSE)) // not &'ing this with 0x8000 is intentional and necessary
//			return false;
	}

	SHORT gks = GetKeyState (KeyIdent);
    return ((gks & 0x80) == 0);
}


/////////////

VOID CALLBACK KeyInputTimer( UINT idEvent, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	//
//	if(timeGetTime() - lastTime > 5)
//	{
		bool S9xGetState (WORD KeyIdent);

		/*		if(GUI.JoystickHotkeys)
		{
		static uint32 joyState [256];

		for(int i = 0 ; i < 255 ; i++)
		{
		bool active = !S9xGetState(0x8000|i);

		if(active)
		{
		if(joyState[i] < ULONG_MAX) // 0xffffffffUL
		joyState[i]++;
		if(joyState[i] == 1 || joyState[i] >= (unsigned) KeyInDelayInCount)
		PostMessage(GUI.hWnd, WM_CUSTKEYDOWN, (WPARAM)(0x8000|i),(LPARAM)(NULL));
		}
		else
		{
		if(joyState[i])
		{
		joyState[i] = 0;
		PostMessage(GUI.hWnd, WM_CUSTKEYUP, (WPARAM)(0x8000|i),(LPARAM)(NULL));
		}
		}
		}
		}*/
		//		if((!GUI.InactivePause || !Settings.ForcedPause)
		//				|| (GUI.BackgroundInput || !(Settings.ForcedPause & (PAUSE_INACTIVE_WINDOW | PAUSE_WINDOW_ICONISED))))
		//		{
		static u32 joyState [256];//TODO
		for(int i = 0 ; i < 255 ; i++)
		{
			bool active = !S9xGetState(i);
			if(active)
			{
				if(joyState[i] < ULONG_MAX) // 0xffffffffUL
					joyState[i]++;
				if(joyState[i] == 1 || joyState[i] >= (unsigned) KeyInDelayInCount) {
					//sort of fix the auto repeating
					//TODO find something better
				//	repeattime++;
				//	if(repeattime % 10 == 0) {

						PostMessage(YabWin, WM_CUSTKEYDOWN, (WPARAM)(i),(LPARAM)(NULL));
						repeattime=0;
				//	}
				}
			}
			else
			{
				if(joyState[i])
				{
					joyState[i] = 0;
					PostMessage(YabWin, WM_CUSTKEYUP, (WPARAM)(i),(LPARAM)(NULL));
				}
			}
		}
		//	}
	//	lastTime = timeGetTime();
//	}
}



/////////////


bool IsReserved (WORD Key, int modifiers)
{
	// keys that do other stuff in Windows
	if(Key == VK_CAPITAL || Key == VK_NUMLOCK || Key == VK_SCROLL || Key == VK_SNAPSHOT
	|| Key == VK_LWIN    || Key == VK_RWIN    || Key == VK_APPS || Key == /*VK_SLEEP*/0x5F
	|| (Key == VK_F4 && (modifiers & CUSTKEY_ALT_MASK) != 0)) // alt-f4 (behaves unlike accelerators)
		return true;

	// menu shortcuts (accelerators) -- TODO: should somehow parse GUI.Accelerators for this information
	if(modifiers == CUSTKEY_CTRL_MASK
	 && (Key == 'O')
	|| modifiers == CUSTKEY_ALT_MASK
	 && (Key == VK_F5 || Key == VK_F7 || Key == VK_F8 || Key == VK_F9
	  || Key == 'R' || Key == 'T' || Key == /*VK_OEM_4*/0xDB || Key == /*VK_OEM_6*/0xDD
	  || Key == 'E' || Key == 'A' || Key == VK_RETURN || Key == VK_DELETE)
	  || Key == VK_MENU || Key == VK_CONTROL)
		return true;

	return false;
}

int HandleKeyUp(WPARAM wParam, LPARAM lParam, int modifiers)
{
	SCustomKey *key = &CustomKeys.key(0);

	while (!IsLastCustomKey(key)) {
		if (wParam == key->key && modifiers == key->modifiers && key->handleKeyUp) {
			key->handleKeyUp(key->param);
		}
		key++;
	}

	return 1;
}

int HandleKeyMessage(WPARAM wParam, LPARAM lParam, int modifiers)
{
	//some crap from snes9x I dont understand with toggles and macros...

	bool hitHotKey = false;

	if(!(wParam == 0 || wParam == VK_ESCAPE)) // if it's the 'disabled' key, it's never pressed as a hotkey
	{
		SCustomKey *key = &CustomKeys.key(0);
		while (!IsLastCustomKey(key)) {
			if (wParam == key->key && modifiers == key->modifiers && key->handleKeyDown) {
				key->handleKeyDown(key->param);
				hitHotKey = true;
			}
			key++;
		}

		// don't pull down menu if alt is a hotkey or the menu isn't there, unless no game is running
		//if(!Settings.StopEmulation && ((wParam == VK_MENU || wParam == VK_F10) && (hitHotKey || GetMenu (GUI.hWnd) == NULL) && !GetAsyncKeyState(VK_F4)))
		/*if(((wParam == VK_MENU || wParam == VK_F10) && (hitHotKey || GetMenu (MainWindow->getHWnd()) == NULL) && !GetAsyncKeyState(VK_F4)))
			return 0;*/
		return 1;
	}

	return 1;
}

int GetModifiers(int key)
{
	int modifiers = 0;

	if (key == VK_MENU || key == VK_CONTROL || key == VK_SHIFT)
		return 0;

	if(GetAsyncKeyState(VK_MENU   )&0x8000) modifiers |= CUSTKEY_ALT_MASK;
	if(GetAsyncKeyState(VK_CONTROL)&0x8000) modifiers |= CUSTKEY_CTRL_MASK;
	if(GetAsyncKeyState(VK_SHIFT  )&0x8000) modifiers |= CUSTKEY_SHIFT_MASK;
	return modifiers;
}

void InitCustomControls()
{

    WNDCLASSEX wc;
/*
    wc.cbSize         = sizeof(wc);
    wc.lpszClassName  = szClassName;
    wc.hInstance      = GetModuleHandle(0);
    wc.lpfnWndProc    = InputCustomWndProc;
    wc.hCursor        = LoadCursor (NULL, IDC_ARROW);
    wc.hIcon          = 0;
    wc.lpszMenuName   = 0;
    wc.hbrBackground  = (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
    wc.style          = 0;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = sizeof(InputCust *);
    wc.hIconSm        = 0;
*/

 //   RegisterClassEx(&wc);

    wc.cbSize         = sizeof(wc);
    wc.lpszClassName  = szHotkeysClassName;
    wc.hInstance      = GetModuleHandle(0);
    wc.lpfnWndProc    = HotInputCustomWndProc;
    wc.hCursor        = LoadCursor (NULL, IDC_ARROW);
    wc.hIcon          = 0;
    wc.lpszMenuName   = 0;
    wc.hbrBackground  = (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
    wc.style          = 0;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = sizeof(InputCust *);
    wc.hIconSm        = 0;


    RegisterClassEx(&wc);

}

int GetNumHotKeysAssignedTo (WORD Key, int modifiers)
{
	int count = 0;
	{
		#define MATCHES_KEY(k) \
			(Key != 0 && Key != VK_ESCAPE \
		   && ((Key == k->key && modifiers == k->modifiers) \
		   || (Key == VK_SHIFT   && k->modifiers & CUSTKEY_SHIFT_MASK) \
		   || (Key == VK_MENU    && k->modifiers & CUSTKEY_ALT_MASK) \
		   || (Key == VK_CONTROL && k->modifiers & CUSTKEY_CTRL_MASK) \
		   || (k->key == VK_SHIFT   && modifiers & CUSTKEY_SHIFT_MASK) \
		   || (k->key == VK_MENU    && modifiers & CUSTKEY_ALT_MASK) \
		   || (k->key == VK_CONTROL && modifiers & CUSTKEY_CTRL_MASK)))

		SCustomKey *key = &CustomKeys.key(0);
		while (!IsLastCustomKey(key)) {
			if (MATCHES_KEY(key)) {
				count++;
			}
			key++;
		}


		#undef MATCHES_KEY
	}
	return count;
}

COLORREF CheckHotKey( WORD Key, int modifiers)
{
	COLORREF red,magenta,blue,white;
	red =RGB(255,0,0);
	magenta =RGB(255,0,255);
	blue = RGB(0,0,255);
	white = RGB(255,255,255);

	// Check for conflict with reserved windows keys
    if(IsReserved(Key,modifiers))
		return red;

    // Check for conflict with button keys
//    if(modifiers == 0 && GetNumButtonsAssignedTo(Key) > 0)
  //      return magenta;

	// Check for duplicate Snes9X hotkeys
	if(GetNumHotKeysAssignedTo(Key,modifiers) > 1)
        return blue;

    return white;
}

void InitKeyCustomControl()
{

    WNDCLASSEX wc;

    wc.cbSize         = sizeof(wc);
    wc.lpszClassName  = szHotkeysClassName;
    wc.hInstance      = GetModuleHandle(0);
    wc.lpfnWndProc    = HotInputCustomWndProc;
    wc.hCursor        = LoadCursor (NULL, IDC_ARROW);
    wc.hIcon          = 0;
    wc.lpszMenuName   = 0;
    wc.hbrBackground  = (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
    wc.style          = 0;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = sizeof(InputCust *);
    wc.hIconSm        = 0;

}

void TranslateKey(WORD keyz,char *out)
{
//	sprintf(out,"%d",keyz);
//	return;

	char temp[128];
	if(keyz&0x8000)
	{
		sprintf(out,GAMEDEVICE_JOYNUMPREFIX,((keyz>>8)&0xF));
		switch(keyz&0xFF)
		{
		case 0:  strcat(out,GAMEDEVICE_XNEG); break;
		case 1:  strcat(out,GAMEDEVICE_XPOS); break;
        case 2:  strcat(out,GAMEDEVICE_YPOS); break;
		case 3:  strcat(out,GAMEDEVICE_YNEG); break;
		case 4:  strcat(out,GAMEDEVICE_POVLEFT); break;
		case 5:  strcat(out,GAMEDEVICE_POVRIGHT); break;
		case 6:  strcat(out,GAMEDEVICE_POVUP); break;
		case 7:  strcat(out,GAMEDEVICE_POVDOWN); break;
		case 49: strcat(out,GAMEDEVICE_POVDNLEFT); break;
		case 50: strcat(out,GAMEDEVICE_POVDNRIGHT); break;
		case 51: strcat(out,GAMEDEVICE_POVUPLEFT); break;
		case 52: strcat(out,GAMEDEVICE_POVUPRIGHT); break;
		case 41: strcat(out,GAMEDEVICE_ZPOS); break;
		case 42: strcat(out,GAMEDEVICE_ZNEG); break;
		case 43: strcat(out,GAMEDEVICE_RPOS); break;
		case 44: strcat(out,GAMEDEVICE_RNEG); break;
		case 45: strcat(out,GAMEDEVICE_UPOS); break;
		case 46: strcat(out,GAMEDEVICE_UNEG); break;
		case 47: strcat(out,GAMEDEVICE_VPOS); break;
		case 48: strcat(out,GAMEDEVICE_VNEG); break;
		default:
			if ((keyz & 0xff) > 40)
            {
				sprintf(temp,GAMEDEVICE_JOYBUTPREFIX,keyz&0xFF);
				strcat(out,temp);
				break;
            }

			sprintf(temp,GAMEDEVICE_BUTTON,(keyz&0xFF)-8);
			strcat(out,temp);
			break;

		}
		return;
	}
	sprintf(out,GAMEDEVICE_KEY,keyz);
	if((keyz>='0' && keyz<='9')||(keyz>='A' &&keyz<='Z'))
	{
		sprintf(out,"%c",keyz);
		return;
	}

	if( keyz >= VK_NUMPAD0 && keyz <= VK_NUMPAD9)
    {

		sprintf(out,GAMEDEVICE_NUMPADPREFIX,'0'+(keyz-VK_NUMPAD0));

        return ;
    }
	switch(keyz)
    {
        case 0:				sprintf(out,GAMEDEVICE_DISABLED); break;
        case VK_TAB:		sprintf(out,GAMEDEVICE_VK_TAB); break;
        case VK_BACK:		sprintf(out,GAMEDEVICE_VK_BACK); break;
        case VK_CLEAR:		sprintf(out,GAMEDEVICE_VK_CLEAR); break;
        case VK_RETURN:		sprintf(out,GAMEDEVICE_VK_RETURN); break;
        case VK_LSHIFT:		sprintf(out,GAMEDEVICE_VK_LSHIFT); break;
		case VK_RSHIFT:		sprintf(out,GAMEDEVICE_VK_RSHIFT); break;
        case VK_LCONTROL:	sprintf(out,GAMEDEVICE_VK_LCONTROL); break;
		case VK_RCONTROL:	sprintf(out,GAMEDEVICE_VK_RCONTROL); break;
        case VK_LMENU:		sprintf(out,GAMEDEVICE_VK_LMENU); break;
		case VK_RMENU:		sprintf(out,GAMEDEVICE_VK_RMENU); break;
        case VK_PAUSE:		sprintf(out,GAMEDEVICE_VK_PAUSE); break;
        case VK_CANCEL:		sprintf(out,GAMEDEVICE_VK_PAUSE); break; // the Pause key can resolve to either "Pause" or "Cancel" depending on when it's pressed
        case VK_CAPITAL:	sprintf(out,GAMEDEVICE_VK_CAPITAL); break;
        case VK_ESCAPE:		sprintf(out,GAMEDEVICE_VK_ESCAPE); break;
        case VK_SPACE:		sprintf(out,GAMEDEVICE_VK_SPACE); break;
        case VK_PRIOR:		sprintf(out,GAMEDEVICE_VK_PRIOR); break;
        case VK_NEXT:		sprintf(out,GAMEDEVICE_VK_NEXT); break;
        case VK_HOME:		sprintf(out,GAMEDEVICE_VK_HOME); break;
        case VK_END:		sprintf(out,GAMEDEVICE_VK_END); break;
        case VK_LEFT:		sprintf(out,GAMEDEVICE_VK_LEFT ); break;
        case VK_RIGHT:		sprintf(out,GAMEDEVICE_VK_RIGHT); break;
        case VK_UP:			sprintf(out,GAMEDEVICE_VK_UP); break;
        case VK_DOWN:		sprintf(out,GAMEDEVICE_VK_DOWN); break;
        case VK_SELECT:		sprintf(out,GAMEDEVICE_VK_SELECT); break;
        case VK_PRINT:		sprintf(out,GAMEDEVICE_VK_PRINT); break;
        case VK_EXECUTE:	sprintf(out,GAMEDEVICE_VK_EXECUTE); break;
        case VK_SNAPSHOT:	sprintf(out,GAMEDEVICE_VK_SNAPSHOT); break;
        case VK_INSERT:		sprintf(out,GAMEDEVICE_VK_INSERT); break;
        case VK_DELETE:		sprintf(out,GAMEDEVICE_VK_DELETE); break;
        case VK_HELP:		sprintf(out,GAMEDEVICE_VK_HELP); break;
        case VK_LWIN:		sprintf(out,GAMEDEVICE_VK_LWIN); break;
        case VK_RWIN:		sprintf(out,GAMEDEVICE_VK_RWIN); break;
        case VK_APPS:		sprintf(out,GAMEDEVICE_VK_APPS); break;
        case VK_MULTIPLY:	sprintf(out,GAMEDEVICE_VK_MULTIPLY); break;
        case VK_ADD:		sprintf(out,GAMEDEVICE_VK_ADD); break;
        case VK_SEPARATOR:	sprintf(out,GAMEDEVICE_VK_SEPARATOR); break;
		case /*VK_OEM_1*/0xBA:		sprintf(out,GAMEDEVICE_VK_OEM_1); break;
        case /*VK_OEM_2*/0xBF:		sprintf(out,GAMEDEVICE_VK_OEM_2); break;
        case /*VK_OEM_3*/0xC0:		sprintf(out,GAMEDEVICE_VK_OEM_3); break;
        case /*VK_OEM_4*/0xDB:		sprintf(out,GAMEDEVICE_VK_OEM_4); break;
        case /*VK_OEM_5*/0xDC:		sprintf(out,GAMEDEVICE_VK_OEM_5); break;
        case /*VK_OEM_6*/0xDD:		sprintf(out,GAMEDEVICE_VK_OEM_6); break;
		case /*VK_OEM_7*/0xDE:		sprintf(out,GAMEDEVICE_VK_OEM_7); break;
		case /*VK_OEM_COMMA*/0xBC:	sprintf(out,GAMEDEVICE_VK_OEM_COMMA );break;
		case /*VK_OEM_PERIOD*/0xBE:	sprintf(out,GAMEDEVICE_VK_OEM_PERIOD);break;
        case VK_SUBTRACT:	sprintf(out,GAMEDEVICE_VK_SUBTRACT); break;
        case VK_DECIMAL:	sprintf(out,GAMEDEVICE_VK_DECIMAL); break;
        case VK_DIVIDE:		sprintf(out,GAMEDEVICE_VK_DIVIDE); break;
        case VK_NUMLOCK:	sprintf(out,GAMEDEVICE_VK_NUMLOCK); break;
        case VK_SCROLL:		sprintf(out,GAMEDEVICE_VK_SCROLL); break;
        case /*VK_OEM_MINUS*/0xBD:	sprintf(out,GAMEDEVICE_VK_OEM_MINUS); break;
        case /*VK_OEM_PLUS*/0xBB:	sprintf(out,GAMEDEVICE_VK_OEM_PLUS); break;
        case VK_SHIFT:		sprintf(out,GAMEDEVICE_VK_SHIFT); break;
        case VK_CONTROL:	sprintf(out,GAMEDEVICE_VK_CONTROL); break;
        case VK_MENU:		sprintf(out,GAMEDEVICE_VK_MENU); break;
        case VK_F1:			sprintf(out,GAMEDEVICE_VK_F1); break;
        case VK_F2:			sprintf(out,GAMEDEVICE_VK_F2); break;
        case VK_F3:			sprintf(out,GAMEDEVICE_VK_F3); break;
        case VK_F4:			sprintf(out,GAMEDEVICE_VK_F4); break;
        case VK_F5:			sprintf(out,GAMEDEVICE_VK_F5); break;
        case VK_F6:			sprintf(out,GAMEDEVICE_VK_F6); break;
        case VK_F7:			sprintf(out,GAMEDEVICE_VK_F7); break;
        case VK_F8:			sprintf(out,GAMEDEVICE_VK_F8); break;
        case VK_F9:			sprintf(out,GAMEDEVICE_VK_F9); break;
        case VK_F10:		sprintf(out,GAMEDEVICE_VK_F10); break;
        case VK_F11:		sprintf(out,GAMEDEVICE_VK_F11); break;
        case VK_F12:		sprintf(out,GAMEDEVICE_VK_F12); break;
    }

    return ;



}
static void TranslateKeyWithModifiers(int wParam, int modifiers, char * outStr)
{

	// if the key itself is a modifier, special case output:
	if(wParam == VK_SHIFT)
		strcpy(outStr, "Shift");
	else if(wParam == VK_MENU)
		strcpy(outStr, "Alt");
	else if(wParam == VK_CONTROL)
		strcpy(outStr, "Control");
	else
	{
		// otherwise, prepend the modifier(s)
		if(wParam != VK_ESCAPE && wParam != 0)
		{
			if((modifiers & CUSTKEY_CTRL_MASK) != 0)
			{
				sprintf(outStr,HOTKEYS_CONTROL_MOD);
				outStr += strlen(HOTKEYS_CONTROL_MOD);
			}
			if((modifiers & CUSTKEY_ALT_MASK) != 0)
			{
				sprintf(outStr,HOTKEYS_ALT_MOD);
				outStr += strlen(HOTKEYS_ALT_MOD);
			}
			if((modifiers & CUSTKEY_SHIFT_MASK) != 0)
			{
				sprintf(outStr,HOTKEYS_SHIFT_MOD);
				outStr += strlen(HOTKEYS_SHIFT_MOD);
			}
		}

		// and append the translated non-modifier key
		TranslateKey(wParam,outStr);
	}
}

static bool keyPressLock = false;

HWND funky;

InputCust * GetInputCustom(HWND hwnd)
{
	return (InputCust *)GetWindowLong(hwnd, 0);
}

void SetInputCustom(HWND hwnd, InputCust *icp)
{
    SetWindowLong(hwnd, 0, (LONG)icp);
}
LRESULT InputCustom_OnPaint(InputCust *ccp, WPARAM wParam, LPARAM lParam)
{
    HDC				hdc;
    PAINTSTRUCT		ps;
    HANDLE			hOldFont;
    TCHAR			szText[200];
    RECT			rect;
	SIZE			sz;
	int				x,y;

    // Get a device context for this window
    hdc = BeginPaint(ccp->hwnd, &ps);

    // Set the font we are going to use
    hOldFont = SelectObject(hdc, ccp->hFont);

    // Set the text colours
    SetTextColor(hdc, ccp->crForeGnd);
    SetBkColor  (hdc, ccp->crBackGnd);

    // Find the text to draw
    GetWindowText(ccp->hwnd, szText, sizeof(szText));

    // Work out where to draw
    GetClientRect(ccp->hwnd, &rect);


    // Find out how big the text will be
    GetTextExtentPoint32(hdc, szText, lstrlen(szText), &sz);

    // Center the text
    x = (rect.right  - sz.cx) / 2;
    y = (rect.bottom - sz.cy) / 2;

    // Draw the text
    ExtTextOut(hdc, x, y, ETO_OPAQUE, &rect, szText, lstrlen(szText), 0);

    // Restore the old font when we have finished
    SelectObject(hdc, hOldFont);

    // Release the device context
    EndPaint(ccp->hwnd, &ps);

    return 0;
}

static LRESULT CALLBACK HotInputCustomWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// retrieve the custom structure POINTER for THIS window
    InputCust *icp = GetInputCustom(hwnd);
	HWND pappy = (HWND__ *)GetWindowLongPtr(hwnd,GWL_HWNDPARENT);
	funky= hwnd;

	static HWND selectedItem = NULL;

	char temp[100];
	COLORREF col;
    switch(msg)
    {

	case WM_GETDLGCODE:
		return DLGC_WANTARROWS|DLGC_WANTALLKEYS|DLGC_WANTCHARS;
		break;


    case WM_NCCREATE:

        // Allocate a new CustCtrl structure for this window.
        icp = (InputCust *) malloc( sizeof(InputCust) );

        // Failed to allocate, stop window creation.
        if(icp == NULL) return FALSE;

        // Initialize the CustCtrl structure.
        icp->hwnd      = hwnd;
        icp->crForeGnd = GetSysColor(COLOR_WINDOWTEXT);
        icp->crBackGnd = GetSysColor(COLOR_WINDOW);
        icp->hFont     = (HFONT__ *) GetStockObject(DEFAULT_GUI_FONT);

        // Assign the window text specified in the call to CreateWindow.
        SetWindowText(hwnd, ((CREATESTRUCT *)lParam)->lpszName);

        // Attach custom structure to this window.
        SetInputCustom(hwnd, icp);

		InvalidateRect(icp->hwnd, NULL, FALSE);
		UpdateWindow(icp->hwnd);

		keyPressLock = false;

		selectedItem = NULL;

		SetTimer(hwnd,747,125,NULL);

        // Continue with window creation.
        return TRUE;

    // Clean up when the window is destroyed.
    case WM_NCDESTROY:
        free(icp);
        break;
	case WM_PAINT:
		return InputCustom_OnPaint(icp,wParam,lParam);
		break;
	case WM_ERASEBKGND:
		return 1;
/*
	case WM_KEYUP:
		{
			int count = 0;
			for(int i=0;i<256;i++)
				if(GetAsyncKeyState(i) & 1)
					count++;

			if(count < 2)
			{
				int p = count;
			}
			if(count < 1)
			{
				int p = count;
			}

			TranslateKey(wParam,temp);
			col = CheckButtonKey(wParam);

			icp->crForeGnd = ((~col) & 0x00ffffff);
			icp->crBackGnd = col;
			SetWindowText(hwnd,temp);
			InvalidateRect(icp->hwnd, NULL, FALSE);
			UpdateWindow(icp->hwnd);
			SendMessage(pappy,WM_USER+43,wParam,(LPARAM)hwnd);
		}
		break;
*/
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:

		{
			int count = 0;
			for(int i=2;i<256;i++)
			{
				if(i >= VK_LSHIFT && i <= VK_RMENU)
					continue;
				if(GetAsyncKeyState(i) & 1)
					count++;
			}

			if(count <= 1)
			{
				keyPressLock = false;
			}
		}

		// no break

	case WM_USER+45:
		// assign a hotkey:
		{
			// don't assign pure modifiers on key-down (they're assigned on key-up)
			if(wParam == VK_SHIFT || wParam == VK_MENU || wParam == VK_CONTROL)
				break;

			int modifiers = 0;
			if(GetAsyncKeyState(VK_MENU))
				modifiers |= CUSTKEY_ALT_MASK;
			if(GetAsyncKeyState(VK_CONTROL))
				modifiers |= CUSTKEY_CTRL_MASK;
			if(GetAsyncKeyState(VK_SHIFT))
				modifiers |= CUSTKEY_SHIFT_MASK;

			TranslateKeyWithModifiers(wParam, modifiers, temp);

			col = CheckHotKey(wParam,modifiers);
///			if(col == RGB(255,0,0)) // un-redify
///				col = RGB(255,255,255);

			icp->crForeGnd = ((~col) & 0x00ffffff);
			icp->crBackGnd = col;
			SetWindowText(hwnd,(LPCWSTR)temp);
			InvalidateRect(icp->hwnd, NULL, FALSE);
			UpdateWindow(icp->hwnd);
			SendMessage(pappy,WM_USER+43,wParam,(LPARAM)hwnd);

			keyPressLock = true;

		}
		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		if(!keyPressLock)
		{
			int count = 0;
			for(int i=2;i<256;i++)
			{
				if(i >= VK_LSHIFT && i <= VK_RMENU)
					continue;
				if(GetAsyncKeyState(i) & 1) // &1 seems to solve an weird non-zero return problem, don't know why
					count++;
			}
			if(count <= 1)
			{
				if(wParam == VK_SHIFT || wParam == VK_MENU || wParam == VK_CONTROL)
				{
					if(wParam == VK_SHIFT)
						sprintf(temp, "Shift");
					if(wParam == VK_MENU)
						sprintf(temp, "Alt");
					if(wParam == VK_CONTROL)
						sprintf(temp, "Control");

					col = CheckHotKey(wParam,0);

					icp->crForeGnd = ((~col) & 0x00ffffff);
					icp->crBackGnd = col;
					SetWindowText(hwnd,(LPCWSTR)temp);
					InvalidateRect(icp->hwnd, NULL, FALSE);
					UpdateWindow(icp->hwnd);
					SendMessage(pappy,WM_USER+43,wParam,(LPARAM)hwnd);
				}
			}
		}
		break;
	case WM_USER+44:

		// set a hotkey field:
		{
		int modifiers = lParam;

		TranslateKeyWithModifiers(wParam, modifiers, temp);

		if(IsWindowEnabled(hwnd))
		{
			col = CheckHotKey(wParam,modifiers);
///			if(col == RGB(255,0,0)) // un-redify
///				col = RGB(255,255,255);
		}
		else
		{
			col = RGB( 192,192,192);
		}
		icp->crForeGnd = ((~col) & 0x00ffffff);
		icp->crBackGnd = col;
		SetWindowText(hwnd,(LPCWSTR)_16(temp));
		InvalidateRect(icp->hwnd, NULL, FALSE);
		UpdateWindow(icp->hwnd);
		}
		break;

	case WM_SETFOCUS:
	{
		selectedItem = hwnd;
		col = RGB( 0,255,0);
		icp->crForeGnd = ((~col) & 0x00ffffff);
		icp->crBackGnd = col;
		InvalidateRect(icp->hwnd, NULL, FALSE);
		UpdateWindow(icp->hwnd);
//		tid = wParam;

		break;
	}
	case WM_KILLFOCUS:
	{
		selectedItem = NULL;
		SendMessage(pappy,WM_USER+46,wParam,(LPARAM)hwnd); // refresh fields on deselect
		break;
	}

	case WM_TIMER:
		if(hwnd == selectedItem)
		{
			//FunkyJoyStickTimer();
		}
		SetTimer(hwnd,747,125,NULL);
		break;
	case WM_LBUTTONDOWN:
		SetFocus(hwnd);
		break;
	case WM_ENABLE:
		COLORREF col;
		if(wParam)
		{
			col = RGB( 255,255,255);
			icp->crForeGnd = ((~col) & 0x00ffffff);
			icp->crBackGnd = col;
		}
		else
		{
			col = RGB( 192,192,192);
			icp->crForeGnd = ((~col) & 0x00ffffff);
			icp->crBackGnd = col;
		}
		InvalidateRect(icp->hwnd, NULL, FALSE);
		UpdateWindow(icp->hwnd);
		return true;
    default:
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
/////////////////

static void set_hotkeyinfo(HWND hDlg)
{
	HotkeyPage page = (HotkeyPage) SendDlgItemMessage(hDlg,IDC_HKCOMBO,CB_GETCURSEL,0,0);
	SCustomKey *key = &CustomKeys.key(0);
	int i = 0;

	while (!IsLastCustomKey(key) && i < NUM_HOTKEY_CONTROLS) {
		if (page == key->page) {
			SendDlgItemMessage(hDlg, IDC_HOTKEY_Table[i], WM_USER+44, key->key, key->modifiers);
			SetDlgItemTextW(hDlg, IDC_LABEL_HK_Table[i], key->name.c_str());
			ShowWindow(GetDlgItem(hDlg, IDC_HOTKEY_Table[i]), SW_SHOW);
			i++;
		}
		key++;
	}
	// disable unused controls
	for (; i < NUM_HOTKEY_CONTROLS; i++) {
		SendDlgItemMessage(hDlg, IDC_HOTKEY_Table[i], WM_USER+44, 0, 0);
		SetDlgItemText(hDlg, IDC_LABEL_HK_Table[i], (LPCWSTR)INPUTCONFIG_LABEL_UNUSED);
		ShowWindow(GetDlgItem(hDlg, IDC_HOTKEY_Table[i]), SW_HIDE);
	}
}

static void ReadHotkey(const char* name, WORD& output)
{
	UINT temp;
	temp = GetPrivateProfileIntA("Hotkeys",name,-1,inifilename);
	if(temp != -1) {
		output = temp;
	}
}

void LoadHotkeyConfig()
{
	
	SCustomKey *key = &CustomKeys.key(0); //TODO

	while (!IsLastCustomKey(key)) {
		ReadHotkey(key->code,key->key); 
		std::string modname = (std::string)key->code + (std::string)" MOD";
		ReadHotkey(modname.c_str(),key->modifiers);
		key++;
	}
}

///////////////////


void WritePrivateProfileInt(char* appname, char* keyname, int val, char* file)
{
	char temp[256] = "";
	sprintf(temp, "%d", val);
	WritePrivateProfileStringA(appname, keyname, temp, file);
}


///////////////

static void SaveHotkeyConfig()//TODO
{
	SCustomKey *key = &CustomKeys.key(0);

	while (!IsLastCustomKey(key)) {
		WritePrivateProfileInt("Hotkeys",(char*)key->code,key->key,inifilename);
		std::string modname = (std::string)key->code + (std::string)" MOD";
		WritePrivateProfileInt("Hotkeys",(char*)modname.c_str(),key->modifiers,inifilename);
		key++;
	}
}

// DlgHotkeyConfig
INT_PTR CALLBACK DlgHotkeyConfig(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i, which;
	static HotkeyPage page = (HotkeyPage) 0;


	static SCustomKeys keys;

	//HBRUSH g_hbrBackground;
switch(msg)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint (hDlg, &ps);

			EndPaint (hDlg, &ps);
		}
		return true;
	case WM_INITDIALOG:
		//if(DirectX.Clipped) S9xReRefresh();
		SetWindowText(hDlg,(LPCWSTR)_16(HOTKEYS_TITLE));

		// insert hotkey page list items
		for(i = 0 ; i < NUM_HOTKEY_PAGE ; i++)
		{
			SendDlgItemMessage(hDlg, IDC_HKCOMBO, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)hotkeyPageTitle[i]);
		}

		SendDlgItemMessage(hDlg,IDC_HKCOMBO,CB_SETCURSEL,(WPARAM)0,0);

		InitCustomKeys(&keys);
		CopyCustomKeys(&keys, &CustomKeys); //TODO
//		CopyCustomKeys(&CustomKeys, &keys); // TODO TODO TODO why did I have to reverse this?
		for( i=0;i<256;i++)
		{
			GetAsyncKeyState(i);
		}

		SetDlgItemText(hDlg,IDC_LABEL_BLUE,(LPCWSTR)_16(HOTKEYS_LABEL_BLUE));

		set_hotkeyinfo(hDlg);

		PostMessage(hDlg,WM_COMMAND, CBN_SELCHANGE<<16, 0);

		SetFocus(GetDlgItem(hDlg,IDC_HKCOMBO));


		return true;
		break;
	case WM_CLOSE:
		EndDialog(hDlg, 0);
		return TRUE;
	case WM_USER+46:
		// refresh command, for clicking away from a selected field
		page = (HotkeyPage) SendDlgItemMessage(hDlg, IDC_HKCOMBO, CB_GETCURSEL, 0, 0);
		set_hotkeyinfo(hDlg);
		return TRUE;
	case WM_USER+43:
	{
		//MessageBox(hDlg,"USER+43 CAUGHT","moo",MB_OK);
		int modifiers = GetModifiers(wParam);

		page = (HotkeyPage) SendDlgItemMessage(hDlg, IDC_HKCOMBO, CB_GETCURSEL, 0, 0);
		TCHAR text[256];

		which = GetDlgCtrlID((HWND)lParam);
		for (i = 0; i < NUM_HOTKEY_CONTROLS; i++) {
			if (which == IDC_HOTKEY_Table[i])
				break;
		}
		GetDlgItemText(hDlg, IDC_LABEL_HK_Table[i], text, COUNT(text));

		SCustomKey *key = &CustomKeys.key(0);//TODO
		while (!IsLastCustomKey(key)) {
			if (page == key->page) {
				if (lstrcmp(text, key->name.c_str()) == 0) {
					key->key = wParam;
					key->modifiers = modifiers;
					break;
				}
			}
			key++;
		}

		set_hotkeyinfo(hDlg);
		PostMessage(hDlg,WM_NEXTDLGCTL,0,0);
//		PostMessage(hDlg,WM_KILLFOCUS,0,0);
	}
		return true;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			CopyCustomKeys(&CustomKeys, &keys);
			EndDialog(hDlg,0);
			break;
		case IDOK:
			SaveHotkeyConfig();
			EndDialog(hDlg,0);
			break;
		}
		switch(HIWORD(wParam))
		{
			case CBN_SELCHANGE:
				page = (HotkeyPage) SendDlgItemMessage(hDlg, IDC_HKCOMBO, CB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hDlg, IDC_HKCOMBO, CB_SETCURSEL, (WPARAM)page, 0);

				set_hotkeyinfo(hDlg);

				SetFocus(GetDlgItem(hDlg, IDC_HKCOMBO));

				break;
		}
		return FALSE;

	}

	return FALSE;
}

void RunHotkeyConfig()
{
	DialogBox(y_hInstance, MAKEINTRESOURCE(IDD_KEYCUSTOM), YabWin, DlgHotkeyConfig);
}

////////////////////////////
bool IsLastCustomKey (const SCustomKey *key)
{
	return (key->key == 0xFFFF && key->modifiers == 0xFFFF);
}

void SetLastCustomKey (SCustomKey *key)
{
	key->key = 0xFFFF;
	key->modifiers = 0xFFFF;
}

void ZeroCustomKeys (SCustomKeys *keys)
{
	UINT i = 0;

	SetLastCustomKey(&keys->LastItem);
	while (!IsLastCustomKey(&keys->key(i))) {
		keys->key(i).key = 0;
		keys->key(i).modifiers = 0;
		i++;
	};
}


void CopyCustomKeys (SCustomKeys *dst, const SCustomKeys *src)
{
	UINT i = 0;

	do {
		dst->key(i) = src->key(i);
	} while (!IsLastCustomKey(&src->key(i++)));
}

//======================================================================================
//=====================================HANDLERS=========================================
//======================================================================================
//void HK_OpenROM(int) {OpenFile();}
void HK_Screenshot(int param)
{
    /*
	OPENFILENAME ofn;
	char * ptr;
    char filename[MAX_PATH] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = YabWin;
    ofn.lpstrFilter = (LPCWSTR)"png file (*.png)\0*.png\0Bmp file (*.bmp)\0*.bmp\0Any file (*.*)\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile =  (LPWSTR)filename;
	ofn.lpstrTitle = (LPCWSTR)"Print Screen Save As";
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = (LPWSTR)"png";
	ofn.Flags = OFN_OVERWRITEPROMPT;
	GetSaveFileName(&ofn);

	ptr = strrchr(filename,'.');//look for the last . in the filename

	if ( ptr != 0 ) {
		if (( strcmp ( ptr, ".BMP" ) == 0 ) ||
			( strcmp ( ptr, ".bmp" ) == 0 )) 
		{
//			NDS_WriteBMP(filename);
		}
		if (( strcmp ( ptr, ".PNG" ) == 0 ) ||
			( strcmp ( ptr, ".png" ) == 0 )) 
		{
	//		NDS_WritePNG(filename);
		}
	}
	*/
	//adelikat: TODO: I guess we should delete the above code?
	YuiScreenshot(YabWin);
}

void HK_StateSaveSlot(int num)
{
	char str[64];
	SaveState(num);
	sprintf(str, "State %d saved.", num);	
	DisplayMessage(str);
}

void HK_StateLoadSlot(int num)
{
	char str[64];
	LoadState(num);
	sprintf(str, "State %d loaded.", num);	
	DisplayMessage(str);
}

void HK_StateSetSlot(int num)
{
	char str[64];
	SaveStateSlot=num;
	sprintf(str, "State %d selected.", SaveStateSlot);	
	DisplayMessage(str);
}

void HK_StateQuickSaveSlot(int)
{
	char str[64];
	SaveState(SaveStateSlot);
	sprintf(str, "State %d saved.", SaveStateSlot);	
	DisplayMessage(str);
}

void HK_StateQuickLoadSlot(int)
{
	char str[64];
	LoadState(SaveStateSlot);
	sprintf(str, "State %d loaded.", SaveStateSlot);	
	DisplayMessage(str);
}

void HK_AutoHoldClearKeyDown(int) {
	
//	ClearAutoHold();
}

void HK_Reset(int) {ResetGame();}
void HK_HardReset(int) {HardResetGame();}

void HK_RecordAVI(int) {YuiRecordAvi(YabWin);}
void HK_StopAVI(int) {YuiStopAvi();}

//void HK_ToggleFrame(int) {frameCounterDisplay ^= true;}
//void HK_ToggleFPS(int) {FpsDisplay ^= true;}
//void HK_ToggleInput(int) {ShowInputDisplay ^= true;}
//void HK_ToggleLag(int) {ShowLagFrameCounter ^= true;}
void HK_ToggleReadOnly(int) 
{
	MovieToggleReadOnly();
}

void HK_PlayMovie(int)   {YuiPlayMovie(YabWin); };
void HK_RecordMovie(int) {YuiRecordMovie(YabWin); };
void HK_StopMovie(int)   {StopMovie(); };

void HK_ToggleNBG0(int) {ToggleNBG0();}
void HK_ToggleNBG1(int) {ToggleNBG1();}
void HK_ToggleNBG2(int) {ToggleNBG2();}
void HK_ToggleNBG3(int) {ToggleNBG3();}
void HK_ToggleRBG0(int) {ToggleRBG0();}
void HK_ToggleVDP1(int) {ToggleVDP1();}				
void HK_ToggleOSD(int) {ToggleFPS();}			
/*
void HK_AutoHoldKeyDown(int) {AutoHoldPressed = true;}
void HK_AutoHoldKeyUp(int) {AutoHoldPressed = false;}

void HK_TurboRightKeyDown(int) { Turbo.Right = true; }
void HK_TurboRightKeyUp(int) { Turbo.Right = false; }

void HK_TurboLeftKeyDown(int) { Turbo.Left = true; }
void HK_TurboLeftKeyUp(int) { Turbo.Left = false; }

void HK_TurboRKeyDown(int) { Turbo.R = true; }
void HK_TurboRKeyUp(int) { Turbo.R = false; }

void HK_TurboLKeyDown(int) { Turbo.L = true; }
void HK_TurboLKeyUp(int) { Turbo.L = false; }

void HK_TurboDownKeyDown(int) { Turbo.Down = true; }
void HK_TurboDownKeyUp(int) { Turbo.Down = false; }

void HK_TurboUpKeyDown(int) { Turbo.Up = true; }
void HK_TurboUpKeyUp(int) { Turbo.Up = false; }

void HK_TurboBKeyDown(int) { Turbo.B = true; }
void HK_TurboBKeyUp(int) { Turbo.B = false; }

void HK_TurboAKeyDown(int) { Turbo.A = true; }
void HK_TurboAKeyUp(int) { Turbo.A = false; }

void HK_TurboXKeyDown(int) { Turbo.X = true; }
void HK_TurboXKeyUp(int) { Turbo.X = false; }

void HK_TurboYKeyDown(int) { Turbo.Y = true; }
void HK_TurboYKeyUp(int) { Turbo.Y = false; }

void HK_TurboStartKeyDown(int) { Turbo.Start = true; }
void HK_TurboStartKeyUp(int) { Turbo.Start = false; }

void HK_TurboSelectKeyDown(int) { Turbo.Select = true; }
void HK_TurboSelectKeyUp(int) { Turbo.Select = false; }
*/

void HK_ToggleFullScreen(int) { ToggleFullScreenHK(); }

void HK_NextSaveSlot(int) { 
/*	lastSaveState++; 
	if(lastSaveState>9) 
		lastSaveState=0; 
	SaveStateMessages(lastSaveState,2);*/
}

void HK_PreviousSaveSlot(int) { 

/*	if(lastSaveState==0) 
		lastSaveState=9; 
	else
		lastSaveState--;
	SaveStateMessages(lastSaveState,2); */
}

bool FrameAdvance;

void HK_FrameAdvanceKeyDown(int) { 
	FrameAdvanceVariable = NeedAdvance;
}
void HK_FrameAdvanceKeyUp(int) { 
	FrameAdvance=false; //dummy todo
}

void HK_Pause(int) { TogglePause(); }


//void HK_FastForwardToggle(int) { FastForward ^=1; }
void HK_FastForwardKeyDown(int) { SpeedThrottleEnable(); }
void HK_FastForwardKeyUp(int) { SpeedThrottleDisable(); }
//void HK_IncreaseSpeed(int) { IncreaseSpeed(); }
//void HK_DecreaseSpeed(int) { DecreaseSpeed(); }



//======================================================================================
//=====================================DEFINITIONS======================================
//======================================================================================

void InitCustomKeys (SCustomKeys *keys)
{
	UINT i = 0;

	SetLastCustomKey(&keys->LastItem);
	while (!IsLastCustomKey(&keys->key(i))) {
		SCustomKey &key = keys->key(i);
		key.key = 0;
		key.modifiers = 0;
		key.handleKeyDown = NULL;
		key.handleKeyUp = NULL;
		key.page = NUM_HOTKEY_PAGE;
		key.param = 0;

		//keys->key[i].timing = PROCESS_NOW;
		i++;
	};

	//Main Page---------------------------------------
/*	keys->OpenROM.handleKeyDown = HK_OpenROM;
	keys->OpenROM.code = "OpenROM";
	keys->OpenROM.name = L"Open ROM";
	keys->OpenROM.page = HOTKEY_PAGE_MAIN;
	keys->OpenROM.key = 'O';
	keys->OpenROM.modifiers = CUSTKEY_CTRL_MASK;*/

	keys->Reset.handleKeyDown = HK_Reset;
	keys->Reset.code = "Reset";
	keys->Reset.name = L"Reset";
	keys->Reset.page = HOTKEY_PAGE_MAIN;
	keys->Reset.key = 'R';
	keys->Reset.modifiers = CUSTKEY_CTRL_MASK;

	keys->HardReset.handleKeyDown = HK_HardReset;
	keys->HardReset.code = "HardReset";
	keys->HardReset.name = L"Hard Reset";
	keys->HardReset.page = HOTKEY_PAGE_MAIN;
	keys->HardReset.key = 'P';
	keys->HardReset.modifiers = CUSTKEY_CTRL_MASK;

	keys->Pause.handleKeyDown = HK_Pause;
	keys->Pause.code = "Pause";
	keys->Pause.name = L"Pause";
	keys->Pause.page = HOTKEY_PAGE_MAIN;
	keys->Pause.key = VK_PAUSE;

	keys->FrameAdvance.handleKeyDown = HK_FrameAdvanceKeyDown;
	keys->FrameAdvance.handleKeyUp = HK_FrameAdvanceKeyUp;
	keys->FrameAdvance.code = "FrameAdvance";
	keys->FrameAdvance.name = L"Frame Advance";
	keys->FrameAdvance.page = HOTKEY_PAGE_MAIN;
	keys->FrameAdvance.key = 'N';

	keys->FastForward.handleKeyDown = HK_FastForwardKeyDown;
	keys->FastForward.handleKeyUp = HK_FastForwardKeyUp;
	keys->FastForward.code = "FastForward";
	keys->FastForward.name = L"Fast Forward";
	keys->FastForward.page = HOTKEY_PAGE_MAIN;
	keys->FastForward.key = VK_TAB;
/*
	keys->FastForwardToggle.handleKeyDown = HK_FastForwardToggle;
	keys->FastForwardToggle.code = "FastForwardToggle";
	keys->FastForwardToggle.name = L"Fast Forward Toggle";
	keys->FastForwardToggle.page = HOTKEY_PAGE_MAIN;
	keys->FastForwardToggle.key = NULL;

	keys->IncreaseSpeed.handleKeyDown = HK_IncreaseSpeed;
	keys->IncreaseSpeed.code = "IncreaseSpeed";
	keys->IncreaseSpeed.name = L"Increase Speed";
	keys->IncreaseSpeed.page = HOTKEY_PAGE_MAIN;
	keys->IncreaseSpeed.key = VK_OEM_PLUS;

	keys->DecreaseSpeed.handleKeyDown = HK_DecreaseSpeed;
	keys->DecreaseSpeed.code = "DecreaseSpeed";
	keys->DecreaseSpeed.name = L"Decrease Speed";
	keys->DecreaseSpeed.page = HOTKEY_PAGE_MAIN;
	keys->DecreaseSpeed.key = VK_OEM_MINUS;
	
*/
/*	keys->AutoHold.handleKeyDown = HK_AutoHoldKeyDown;
	keys->AutoHold.handleKeyUp = HK_AutoHoldKeyUp;
	keys->AutoHold.code = "AutoHold";
	keys->AutoHold.name = L"Auto-Hold";
	keys->AutoHold.page = HOTKEY_PAGE_MAIN;
	keys->AutoHold.key = NULL;

	keys->AutoHoldClear.handleKeyDown = HK_AutoHoldClearKeyDown;
	keys->AutoHoldClear.code = "AutoHoldClear";
	keys->AutoHoldClear.name = L"Auto-Hold Clear";
	keys->AutoHoldClear.page = HOTKEY_PAGE_MAIN;
	keys->AutoHoldClear.key = NULL;
*/

	keys->Screenshot.handleKeyDown = HK_Screenshot;
	keys->Screenshot.code = "Screenshot";
	keys->Screenshot.name = L"Screenshot";
	keys->Screenshot.page = HOTKEY_PAGE_MAIN;
	keys->Screenshot.key = VK_F12;

/*	keys->ToggleFrameCounter.handleKeyDown = HK_ToggleFrame;
	keys->ToggleFrameCounter.code = "ToggleFrameDisplay";
	keys->ToggleFrameCounter.name = L"Toggle Frame Display";
	keys->ToggleFrameCounter.page = HOTKEY_PAGE_MAIN;
	keys->ToggleFrameCounter.key = VK_OEM_PERIOD;

	keys->ToggleFPS.handleKeyDown = HK_ToggleFPS;
	keys->ToggleFPS.code = "ToggleFPSDisplay";
	keys->ToggleFPS.name = L"Toggle FPS Display";
	keys->ToggleFPS.page = HOTKEY_PAGE_MAIN;
	keys->ToggleFPS.key = NULL;

	keys->ToggleInput.handleKeyDown = HK_ToggleInput;
	keys->ToggleInput.code = "ToggleInputDisplay";
	keys->ToggleInput.name = L"Toggle Input Display";
	keys->ToggleInput.page = HOTKEY_PAGE_MAIN;
	keys->ToggleInput.key = VK_OEM_COMMA;

	keys->ToggleLag.handleKeyDown = HK_ToggleLag;
	keys->ToggleLag.code = "ToggleLagDisplay";
	keys->ToggleLag.name = L"Toggle Lag Display";
	keys->ToggleLag.page = HOTKEY_PAGE_MAIN;
	keys->ToggleLag.key = NULL;
*/
	keys->ToggleReadOnly.handleKeyDown = HK_ToggleReadOnly;
	keys->ToggleReadOnly.code = "ToggleReadOnly";
	keys->ToggleReadOnly.name = L"Toggle Read Only";
	keys->ToggleReadOnly.page = HOTKEY_PAGE_MOVIE;
	keys->ToggleReadOnly.key = 'Q';

	keys->PlayMovie.handleKeyDown = HK_PlayMovie;
	keys->PlayMovie.code = "PlayMovie";
	keys->PlayMovie.name = L"Play Movie";
	keys->PlayMovie.page = HOTKEY_PAGE_MOVIE;
	keys->PlayMovie.key = NULL;

	keys->RecordMovie.handleKeyDown = HK_RecordMovie;
	keys->RecordMovie.code = "RecordMovie";
	keys->RecordMovie.name = L"Record Movie";
	keys->RecordMovie.page = HOTKEY_PAGE_MOVIE;
	keys->RecordMovie.key = 'R';
	keys->RecordMovie.modifiers = CUSTKEY_SHIFT_MASK;

	keys->StopMovie.handleKeyDown = HK_StopMovie;
	keys->StopMovie.code = "StopMovie";
	keys->StopMovie.name = L"Stop Movie";
	keys->StopMovie.page = HOTKEY_PAGE_MOVIE;
	keys->StopMovie.key = NULL;


	keys->RecordAVI.handleKeyDown = HK_RecordAVI;
	keys->RecordAVI.code = "RecordAVI";
	keys->RecordAVI.name = L"Record AVI";
	keys->RecordAVI.page = HOTKEY_PAGE_MAIN;
	keys->RecordAVI.key = NULL;

	keys->StopAVI.handleKeyDown = HK_StopAVI;
	keys->StopAVI.code = "StopAVI";
	keys->StopAVI.name = L"Stop AVI";
	keys->StopAVI.page = HOTKEY_PAGE_MAIN;
	keys->StopAVI.key = NULL;
/*
	//Turbo Page---------------------------------------
	keys->TurboRight.handleKeyDown = HK_TurboRightKeyDown;
	keys->TurboRight.handleKeyUp = HK_TurboRightKeyUp;
	keys->TurboRight.code = "TurboRight";
	keys->TurboRight.name = L"Turbo Right";
	keys->TurboRight.page = HOTKEY_PAGE_TURBO;
	keys->TurboRight.key = NULL;

	keys->TurboLeft.handleKeyDown = HK_TurboLeftKeyDown;
	keys->TurboLeft.handleKeyUp = HK_TurboLeftKeyUp;
	keys->TurboLeft.code = "TurboLeft";
	keys->TurboLeft.name = L"Turbo Left";
	keys->TurboLeft.page = HOTKEY_PAGE_TURBO;
	keys->TurboLeft.key = NULL;

	keys->TurboR.handleKeyDown = HK_TurboRKeyDown;
	keys->TurboR.handleKeyUp = HK_TurboRKeyUp;
	keys->TurboR.code = "TurboR";
	keys->TurboR.name = L"Turbo R";
	keys->TurboR.page = HOTKEY_PAGE_TURBO;
	keys->TurboR.key = NULL;

	keys->TurboL.handleKeyDown = HK_TurboLKeyDown;
	keys->TurboL.handleKeyUp = HK_TurboLKeyUp;
	keys->TurboL.code = "TurboL";
	keys->TurboL.name = L"Turbo L";
	keys->TurboL.page = HOTKEY_PAGE_TURBO;
	keys->TurboL.key = NULL;

	keys->TurboDown.handleKeyDown = HK_TurboDownKeyDown;
	keys->TurboDown.handleKeyUp = HK_TurboDownKeyUp;
	keys->TurboDown.code = "TurboDown";
	keys->TurboDown.name = L"Turbo Down";
	keys->TurboDown.page = HOTKEY_PAGE_TURBO;
	keys->TurboDown.key = NULL;

	keys->TurboUp.handleKeyDown = HK_TurboUpKeyDown;
	keys->TurboUp.handleKeyUp = HK_TurboUpKeyUp;
	keys->TurboUp.code = "TurboUp";
	keys->TurboUp.name = L"Turbo Up";
	keys->TurboUp.page = HOTKEY_PAGE_TURBO;
	keys->TurboUp.key = NULL;

	keys->TurboB.handleKeyDown = HK_TurboBKeyDown;
	keys->TurboB.handleKeyUp = HK_TurboBKeyUp;
	keys->TurboB.code = "TurboB";
	keys->TurboB.name = L"Turbo B";
	keys->TurboB.page = HOTKEY_PAGE_TURBO;
	keys->TurboB.key = NULL;

	keys->TurboA.handleKeyDown = HK_TurboAKeyDown;
	keys->TurboA.handleKeyUp = HK_TurboAKeyUp;
	keys->TurboA.code = "TurboA";
	keys->TurboA.name = L"Turbo A";
	keys->TurboA.page = HOTKEY_PAGE_TURBO;
	keys->TurboA.key = NULL;

	keys->TurboX.handleKeyDown = HK_TurboXKeyDown;
	keys->TurboX.handleKeyUp = HK_TurboXKeyUp;
	keys->TurboX.code = "TurboX";
	keys->TurboX.name = L"Turbo X";
	keys->TurboX.page = HOTKEY_PAGE_TURBO;
	keys->TurboX.key = NULL;

	keys->TurboY.handleKeyDown = HK_TurboYKeyDown;
	keys->TurboY.handleKeyUp = HK_TurboYKeyUp;
	keys->TurboY.code = "TurboY";
	keys->TurboY.name = L"Turbo Y";
	keys->TurboY.page = HOTKEY_PAGE_TURBO;
	keys->TurboY.key = NULL;

	keys->TurboSelect.handleKeyDown = HK_TurboSelectKeyDown;
	keys->TurboSelect.handleKeyUp = HK_TurboSelectKeyUp;
	keys->TurboSelect.code = "TurboSelect";
	keys->TurboSelect.name = L"Turbo Select";
	keys->TurboSelect.page = HOTKEY_PAGE_TURBO;
	keys->TurboSelect.key = NULL;

	keys->TurboStart.handleKeyDown = HK_TurboStartKeyDown;
	keys->TurboStart.handleKeyUp = HK_TurboStartKeyUp;
	keys->TurboStart.code = "TurboStart";
	keys->TurboStart.name = L"Turbo Start";
	keys->TurboStart.page = HOTKEY_PAGE_TURBO;
	keys->TurboStart.key = NULL;
*/

	keys->ToggleOSD.handleKeyDown = HK_ToggleOSD;
	keys->ToggleOSD.code = "ToggleOSD";
	keys->ToggleOSD.name = L"Toggle OSD";
	keys->ToggleOSD.page = HOTKEY_PAGE_MAIN;
	keys->ToggleOSD.key = NULL;

	keys->ToggleNBG0.handleKeyDown = HK_ToggleNBG0;
	keys->ToggleNBG0.code = "ToggleNBG0";
	keys->ToggleNBG0.name = L"Toggle NBG0";
	keys->ToggleNBG0.page = HOTKEY_PAGE_MAIN;
	keys->ToggleNBG0.key = NULL;

	keys->ToggleNBG1.handleKeyDown = HK_ToggleNBG1;
	keys->ToggleNBG1.code = "ToggleNBG1";
	keys->ToggleNBG1.name = L"Toggle NBG1";
	keys->ToggleNBG1.page = HOTKEY_PAGE_MAIN;
	keys->ToggleNBG1.key = NULL;

	keys->ToggleNBG2.handleKeyDown = HK_ToggleNBG2;
	keys->ToggleNBG2.code = "ToggleNBG2";
	keys->ToggleNBG2.name = L"Toggle NBG2";
	keys->ToggleNBG2.page = HOTKEY_PAGE_MAIN;
	keys->ToggleNBG2.key = NULL;

	keys->ToggleNBG3.handleKeyDown = HK_ToggleNBG3;
	keys->ToggleNBG3.code = "ToggleNBG3";
	keys->ToggleNBG3.name = L"Toggle NBG3";
	keys->ToggleNBG3.page = HOTKEY_PAGE_MAIN;
	keys->ToggleNBG3.key = NULL;

	keys->ToggleRBG0.handleKeyDown = HK_ToggleRBG0;
	keys->ToggleRBG0.code = "ToggleRBG0";
	keys->ToggleRBG0.name = L"Toggle RBG0";
	keys->ToggleRBG0.page = HOTKEY_PAGE_MAIN;
	keys->ToggleRBG0.key = NULL;

	keys->ToggleVDP1.handleKeyDown = HK_ToggleVDP1;
	keys->ToggleVDP1.code = "ToggleVDP1";
	keys->ToggleVDP1.name = L"Toggle VDP1";
	keys->ToggleVDP1.page = HOTKEY_PAGE_MAIN;
	keys->ToggleVDP1.key = NULL;

	keys->NextSaveSlot.handleKeyDown = HK_NextSaveSlot;
	keys->NextSaveSlot.code = "NextSaveSlot";
	keys->NextSaveSlot.name = L"Next Save Slot";
	keys->NextSaveSlot.page = HOTKEY_PAGE_STATE_SLOTS;
	keys->NextSaveSlot.key = NULL;

	keys->PreviousSaveSlot.handleKeyDown = HK_PreviousSaveSlot;
	keys->PreviousSaveSlot.code = "PreviousSaveSlot";
	keys->PreviousSaveSlot.name = L"Previous Save Slot";
	keys->PreviousSaveSlot.page = HOTKEY_PAGE_STATE_SLOTS;
	keys->PreviousSaveSlot.key = NULL;

	keys->ToggleFullScreen.handleKeyDown = HK_ToggleFullScreen;
	keys->ToggleFullScreen.code = "ToggleFullScreen";
	keys->ToggleFullScreen.name = L"Toggle Full Screen";
	keys->ToggleFullScreen.page = HOTKEY_PAGE_MAIN;
	keys->ToggleFullScreen.key = NULL;
	
	keys->QuickSave.handleKeyDown = HK_StateQuickSaveSlot;
	keys->QuickSave.code = "QuickSave";
	keys->QuickSave.name = L"Quick Save";
	keys->QuickSave.page = HOTKEY_PAGE_STATE_SLOTS;
	keys->QuickSave.key = 'I';

	keys->QuickLoad.handleKeyDown = HK_StateQuickLoadSlot;
	keys->QuickLoad.code = "QuickLoad";
	keys->QuickLoad.name = L"Quick Load";
	keys->QuickLoad.page = HOTKEY_PAGE_STATE_SLOTS;
	keys->QuickLoad.key = 'P';

	for(int i=0;i<10;i++) {
		static const char* saveNames[] = {"SaveToSlot0","SaveToSlot1","SaveToSlot2","SaveToSlot3","SaveToSlot4","SaveToSlot5","SaveToSlot6","SaveToSlot7","SaveToSlot8","SaveToSlot9"};
		static const char* loadNames[] = {"LoadFromSlot0","LoadFromSlot1","LoadFromSlot2","LoadFromSlot3","LoadFromSlot4","LoadFromSlot5","LoadFromSlot6","LoadFromSlot7","LoadFromSlot8","LoadFromSlot9"};
		static const char* slotNames[] = {"SelectSlot0","SelectSlot1","SelectSlot2","SelectSlot3","SelectSlot4","SelectSlot5","SelectSlot6","SelectSlot7","SelectSlot8","SelectSlot9"};

		WORD key = VK_F1 + i - 1;
		if(i==0) key = VK_F10;

		SCustomKey & save = keys->Save[i];
		save.handleKeyDown = HK_StateSaveSlot;
		save.param = i;
		save.page = HOTKEY_PAGE_STATE;
		wchar_t tmp[16];
		_itow(i,tmp,10);
		save.name = (std::wstring)L"Save To Slot " + (std::wstring)tmp;
		save.code = saveNames[i];
		save.key = key;
		save.modifiers = CUSTKEY_SHIFT_MASK;

		SCustomKey & load = keys->Load[i];
		load.handleKeyDown = HK_StateLoadSlot;
		load.param = i;
		load.page = HOTKEY_PAGE_STATE;
		_itow(i,tmp,10);
		load.name = (std::wstring)L"Load from Slot " + (std::wstring)tmp;
		load.code = loadNames[i];
		load.key = key;

		key = '0' + i;

		SCustomKey & slot = keys->Slot[i];
		slot.handleKeyDown = HK_StateSetSlot;
		slot.param = i;
		slot.page = HOTKEY_PAGE_STATE_SLOTS;
		_itow(i,tmp,10);
		slot.name = (std::wstring)L"Select Save Slot " + (std::wstring)tmp;
		slot.code = slotNames[i];
		slot.key = key;
	}
}


/**********************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.

  (c) Copyright 1996 - 2002  Gary Henderson (gary.henderson@ntlworld.com),
                             Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2002 - 2004  Matthew Kendora

  (c) Copyright 2002 - 2005  Peter Bortas (peter@bortas.org)

  (c) Copyright 2004 - 2005  Joel Yliluoma (http://iki.fi/bisqwit/)

  (c) Copyright 2001 - 2006  John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2006  funkyass (funkyass@spam.shaw.ca),
                             Kris Bleakley (codeviolation@hotmail.com)

  (c) Copyright 2002 - 2007  Brad Jorsch (anomie@users.sourceforge.net),
                             Nach (n-a-c-h@users.sourceforge.net),
                             zones (kasumitokoduck@yahoo.com)

  (c) Copyright 2006 - 2007  nitsuja


  BS-X C emulator code
  (c) Copyright 2005 - 2006  Dreamer Nom,
                             zones

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003  _Demo_ (_demo_@zsnes.com),
                             Nach,
                             zsKnight (zsknight@zsnes.com)

  C4 C++ code
  (c) Copyright 2003 - 2006  Brad Jorsch,
                             Nach

  DSP-1 emulator code
  (c) Copyright 1998 - 2006  _Demo_,
                             Andreas Naive (andreasnaive@gmail.com)
                             Gary Henderson,
                             Ivar (ivar@snes9x.com),
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora,
                             Nach,
                             neviksti (neviksti@hotmail.com)

  DSP-2 emulator code
  (c) Copyright 2003         John Weidman,
                             Kris Bleakley,
                             Lord Nightmare (lord_nightmare@users.sourceforge.net),
                             Matthew Kendora,
                             neviksti


  DSP-3 emulator code
  (c) Copyright 2003 - 2006  John Weidman,
                             Kris Bleakley,
                             Lancer,
                             z80 gaiden

  DSP-4 emulator code
  (c) Copyright 2004 - 2006  Dreamer Nom,
                             John Weidman,
                             Kris Bleakley,
                             Nach,
                             z80 gaiden

  OBC1 emulator code
  (c) Copyright 2001 - 2004  zsKnight,
                             pagefault (pagefault@zsnes.com),
                             Kris Bleakley,
                             Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002         Matthew Kendora with research by
                             zsKnight,
                             John Weidman,
                             Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003         Brad Jorsch with research by
                             Andreas Naive,
                             John Weidman

  S-RTC C emulator code
  (c) Copyright 2001-2006    byuu,
                             John Weidman

  ST010 C++ emulator code
  (c) Copyright 2003         Feather,
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora

  Super FX x86 assembler emulator code
  (c) Copyright 1998 - 2003  _Demo_,
                             pagefault,
                             zsKnight,

  Super FX C emulator code
  (c) Copyright 1997 - 1999  Ivar,
                             Gary Henderson,
                             John Weidman

  Sound DSP emulator code is derived from SNEeSe and OpenSPC:
  (c) Copyright 1998 - 2003  Brad Martin
  (c) Copyright 1998 - 2006  Charles Bilyue'

  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004  Marcus Comstedt (marcus@mc.pp.se)

  2xSaI filter
  (c) Copyright 1999 - 2001  Derek Liauw Kie Fa

  HQ2x, HQ3x, HQ4x filters
  (c) Copyright 2003         Maxim Stepin (maxim@hiend3d.com)

  Win32 GUI code
  (c) Copyright 2003 - 2006  blip,
                             funkyass,
                             Matthew Kendora,
                             Nach,
                             nitsuja

  Mac OS GUI code
  (c) Copyright 1998 - 2001  John Stiles
  (c) Copyright 2001 - 2007  zones


  Specific ports contains the works of other authors. See headers in
  individual files.


  Snes9x homepage: http://www.snes9x.com

  Permission to use, copy, modify and/or distribute Snes9x in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for Snes9x or software derived from
  Snes9x, including Snes9x or derivatives in commercial game bundles, and/or
  using Snes9x as a promotion for your commercial product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
**********************************************************************************/

