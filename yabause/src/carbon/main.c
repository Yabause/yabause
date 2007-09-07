/*  Copyright 2006 Guillaume Duhamel
    Copyright 2006 Anders Montonen

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

#include <unistd.h>
#include <Carbon/Carbon.h>
#include <AGL/agl.h>

#include "settings.h"
#include "cpustatus.h"

#define YUI_COMMAND_RUN		 1
#define YUI_COMMAND_PAUSE 	 2
#define YUI_COMMAND_RESUME   3
#define YUI_COMMAND_SHOW_CPU 4
#define YUI_COMMAND_HIDE_CPU 5
#define YUI_COMMAND_TOGGLE_NBG0	6
#define YUI_COMMAND_TOGGLE_NBG1	7
#define YUI_COMMAND_TOGGLE_NBG2	8
#define YUI_COMMAND_TOGGLE_NBG3	9
#define YUI_COMMAND_TOGGLE_RBG0	10

AGLContext  myAGLContext = NULL;
yabauseinit_struct yinit;
extern const char * key_names[] = {
	"Up", "Right", "Down", "Left", "Right trigger", "Left trigger",
	"Start", "A", "B", "C", "X", "Y", "Z", NULL
};

M68K_struct * M68KCoreList[] = {
&M68KDummy,
&M68KC68K,
NULL
};

SH2Interface_struct *SH2CoreList[] = {
&SH2Interpreter,
&SH2DebugInterpreter,
NULL
};

PerInterface_struct *PERCoreList[] = {
&PERDummy,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
&ArchCD,
NULL
};

SoundInterface_struct *SNDCoreList[] = {
&SNDDummy,
#ifdef HAVE_LIBSDL
&SNDSDL,
#endif
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDOGL,
&VIDSoft,
NULL
};

static EventLoopTimerRef EventTimer;

void YuiIdle(EventLoopTimerRef a, void * b) {
	int i;
	for(i = 0;i < 200;i++) {
		YabauseExec();
	}
}

void read_settings(void) {
	int i;
	CFStringRef s;
	yinit.percoretype = PERCORE_DUMMY;
	yinit.sh2coretype = SH2CORE_INTERPRETER;
	yinit.vidcoretype = VIDCORE_OGL;
	yinit.m68kcoretype = M68KCORE_C68K;
	s = CFPreferencesCopyAppValue(CFSTR("VideoCore"),
		kCFPreferencesCurrentApplication);
	if (s)
		yinit.vidcoretype = CFStringGetIntValue(s) - 1;
	yinit.sndcoretype = SNDCORE_DUMMY;
	s = CFPreferencesCopyAppValue(CFSTR("SoundCore"),
		kCFPreferencesCurrentApplication);
	if (s)
		yinit.sndcoretype = CFStringGetIntValue(s) - 1;
	yinit.cdcoretype = CDCORE_ARCH;
	s = CFPreferencesCopyAppValue(CFSTR("CDROMCore"),
		kCFPreferencesCurrentApplication);
	if (s)
		yinit.cdcoretype = CFStringGetIntValue(s) - 1;
	yinit.carttype = CART_NONE;
    s = CFPreferencesCopyAppValue(CFSTR("CartType"),
        kCFPreferencesCurrentApplication);
    if (s)
        yinit.carttype = CFStringGetIntValue(s) - 1;
	yinit.regionid = 0;
    s = CFPreferencesCopyAppValue(CFSTR("Region"),
        kCFPreferencesCurrentApplication);
    if (s)
        yinit.regionid = CFStringGetIntValue(s) - 1;

	yinit.biospath = 0;
	s = CFPreferencesCopyAppValue(CFSTR("BiosPath"),
		kCFPreferencesCurrentApplication);
	if (s)
		yinit.biospath = strdup(CFStringGetCStringPtr(s, 0));
	yinit.cdpath = 0;
	s = CFPreferencesCopyAppValue(CFSTR("CDROMDrive"),
		kCFPreferencesCurrentApplication);
	if (s)
		yinit.cdpath = strdup(CFStringGetCStringPtr(s, 0));
	yinit.buppath = 0;
    s = CFPreferencesCopyAppValue(CFSTR("BackupRamPath"),
        kCFPreferencesCurrentApplication);
    if (s)
        yinit.buppath = strdup(CFStringGetCStringPtr(s, 0));
	yinit.mpegpath = 0;
    s = CFPreferencesCopyAppValue(CFSTR("MpegRomPath"),
        kCFPreferencesCurrentApplication);
    if (s)
        yinit.mpegpath = strdup(CFStringGetCStringPtr(s, 0));
	yinit.cartpath = 0;
    s = CFPreferencesCopyAppValue(CFSTR("CartPath"),
        kCFPreferencesCurrentApplication);
    if (s)
        yinit.cartpath = strdup(CFStringGetCStringPtr(s, 0));
    
	yinit.flags = VIDEOFORMATTYPE_NTSC;
	
	s = CFPreferencesCopyAppValue(CFSTR("AutoFrameSkip"),
		kCFPreferencesCurrentApplication);
	if (s)
		yinit.frameskip = CFStringGetIntValue(s);

	i = 0;
	while(key_names[i]) {
		s = CFPreferencesCopyAppValue(
			CFStringCreateWithCString(0, key_names[i], 0),
			kCFPreferencesCurrentApplication);
		if (s)
			PerSetKey(CFStringGetIntValue(s), key_names[i]);
		i++;
	}
}

void YuiRun(void) {
	static int FirstRun = 1;
	EventLoopTimerUPP myFrameUPP;

	if(FirstRun)
	{
		myFrameUPP = NewEventLoopTimerUPP(YuiIdle);
		InstallEventLoopTimer(GetCurrentEventLoop(), kEventDurationNoWait,
			kEventDurationMillisecond, myFrameUPP, NULL, &EventTimer);
		FirstRun = 0;
	}
	else
	{
		YabauseDeInit();
	}

	read_settings();
	YabauseInit(&yinit);
}

static void YuiPause(const int Pause)
{
    EventTimerInterval Interval;

    if(Pause)
    {
        Interval = kEventDurationForever;
        ScspMuteAudio();
    }
    else
    {
        Interval = kEventDurationMillisecond;
        ScspUnMuteAudio();
    }

    SetEventLoopTimerNextFireTime(EventTimer, Interval);
}

static void ToggleLayerMenuItem(MenuRef menu, MenuItemIndex BaseItemIndex)
{
	MenuItemAttributes ItemAttributes;

	GetMenuItemAttributes(menu, BaseItemIndex, &ItemAttributes);

	if(ItemAttributes & kMenuItemAttrHidden)
	{
		ChangeMenuItemAttributes(menu, BaseItemIndex, 0, kMenuItemAttrHidden);
		ChangeMenuItemAttributes(menu, BaseItemIndex+1, kMenuItemAttrHidden, 0);
	}
	else
	{
		ChangeMenuItemAttributes(menu, BaseItemIndex, kMenuItemAttrHidden, 0);
		ChangeMenuItemAttributes(menu, BaseItemIndex+1, 0, kMenuItemAttrHidden);
	}
}

OSStatus MyWindowEventHandler (EventHandlerCallRef myHandler, EventRef theEvent, void* userData)
{
  OSStatus ret = noErr;
  MenuRef menu;

  switch(GetEventClass(theEvent)) {
    case kEventClassWindow:
      switch (GetEventKind (theEvent)) {
        case kEventWindowClose:

          YabauseDeInit();
          QuitApplicationEventLoop();
          break;
 
        case kEventWindowBoundsChanged:
          aglUpdateContext(myAGLContext);
          {
            Rect bounds;
            GetEventParameter(theEvent, kEventParamCurrentBounds,
	      typeQDRectangle, NULL, sizeof(Rect), NULL, &bounds);
            glViewport(0, 0, bounds.right - bounds.left,
	      bounds.bottom - bounds.top);
          }
          break;
      }
      break;
    case kEventClassCommand:
      {
        HICommand command;
        GetEventParameter(theEvent, kEventParamDirectObject,
	  typeHICommand, NULL, sizeof(HICommand), NULL, &command);
        switch(command.commandID) {
          case kHICommandPreferences:
	    CreateSettingsWindow();
            break;
          case kHICommandQuit:
            YabauseDeInit();
            QuitApplicationEventLoop();
            break;
          case YUI_COMMAND_RUN:
	    YuiRun();
        menu = GetMenuRef(1);
        ChangeMenuItemAttributes(menu, 2, 0, kMenuItemAttrHidden);
        ChangeMenuItemAttributes(menu, 3, kMenuItemAttrHidden, 0);
            break;
          case YUI_COMMAND_PAUSE:
              YuiPause(1);
              menu = GetMenuRef(1);
              ChangeMenuItemAttributes(menu, 2, kMenuItemAttrHidden, 0);
              ChangeMenuItemAttributes(menu, 3, 0, kMenuItemAttrHidden);
              UpdateCPUStatusWindow();
            break;
        case YUI_COMMAND_RESUME:
            YuiPause(0);
            menu = GetMenuRef(1);
            ChangeMenuItemAttributes(menu, 2, 0, kMenuItemAttrHidden);
            ChangeMenuItemAttributes(menu, 3, kMenuItemAttrHidden, 0);
            break;
        case YUI_COMMAND_SHOW_CPU:
            ShowCPUStatusWindow();
            menu = GetMenuRef(1);
            ChangeMenuItemAttributes(menu, 4, kMenuItemAttrHidden, 0);
            ChangeMenuItemAttributes(menu, 5, 0, kMenuItemAttrHidden);
            break;
        case YUI_COMMAND_HIDE_CPU:
            HideCPUStatusWindow();
            menu = GetMenuRef(1);
            ChangeMenuItemAttributes(menu, 4, 0, kMenuItemAttrHidden);
            ChangeMenuItemAttributes(menu, 5, kMenuItemAttrHidden, 0);
            break;
	case YUI_COMMAND_TOGGLE_NBG0:
		if(VIDCore)
		{
			menu = GetMenuRef(1);
			ToggleLayerMenuItem(menu, 6);
			ToggleNBG0();
		}
		break;
	case YUI_COMMAND_TOGGLE_NBG1:
		if(VIDCore)
		{
			menu = GetMenuRef(1);
			ToggleLayerMenuItem(menu, 8);
			ToggleNBG1();
		}
		break;
	case YUI_COMMAND_TOGGLE_NBG2:
		if(VIDCore)
		{
			menu = GetMenuRef(1);
			ToggleLayerMenuItem(menu, 10);
			ToggleNBG2();
		}
		break;
	case YUI_COMMAND_TOGGLE_NBG3:
		if(VIDCore)
		{
			menu = GetMenuRef(1);
			ToggleLayerMenuItem(menu, 12);
			ToggleNBG3();
		}
		break;
	case YUI_COMMAND_TOGGLE_RBG0:
		if(VIDCore)
		{
			menu = GetMenuRef(1);
			ToggleLayerMenuItem(menu, 14);
			ToggleRBG0();
		}
		break;
          default:
            ret = eventNotHandledErr;
            printf("unhandled command\n");
            break;
        }
      }
      break;

    case kEventClassKeyboard:
      switch(GetEventKind(theEvent)) {
        int i;
        UInt32 key;
        case kEventRawKeyDown:
          GetEventParameter(theEvent, kEventParamKeyCode,
            typeUInt32, NULL, sizeof(UInt32), NULL, &key);
          PerKeyDown(key);
          break;
        case kEventRawKeyUp:
          GetEventParameter(theEvent, kEventParamKeyCode,
            typeUInt32, NULL, sizeof(UInt32), NULL, &key);
          PerKeyUp(key);
          break;
      }
      break;
    }
 
  return ret;
}

WindowRef CreateMyWindow() {

  WindowRef myWindow;
  Rect contentBounds;

  CFStringRef windowTitle = CFSTR("Yabause");
  WindowClass windowClass = kDocumentWindowClass;
  WindowAttributes attributes =
    kWindowStandardDocumentAttributes |
    kWindowStandardHandlerAttribute |
    kWindowLiveResizeAttribute;

  EventTypeSpec eventList[] = {
    { kEventClassWindow, kEventWindowClose },
    { kEventClassWindow, kEventWindowBoundsChanged },
    { kEventClassCommand, kEventCommandProcess },
    { kEventClassKeyboard, kEventRawKeyDown },
    { kEventClassKeyboard, kEventRawKeyUp }
  };
 
  SetRect(&contentBounds, 200, 200, 520, 424);

  CreateNewWindow (windowClass,
			 attributes,
			 &contentBounds,
			 &myWindow);

  SetWindowTitleWithCFString (myWindow, windowTitle);
  CFRelease(windowTitle);
  ShowWindow(myWindow);

  InstallWindowEventHandler(myWindow,
			    NewEventHandlerUPP (MyWindowEventHandler),
			    GetEventTypeCount(eventList),
			    eventList, myWindow, NULL);
  return myWindow;
}

OSStatus MyAGLReportError (void) {
    GLenum err = aglGetError();

    if (err == AGL_NO_ERROR)
        return noErr;
    else
        return (OSStatus) err;
}

OSStatus MySetWindowAsDrawableObject  (WindowRef window)
{

    OSStatus err = noErr;

    Rect rectPort;

    GLint attributes[] =  { AGL_RGBA,
                        AGL_DOUBLEBUFFER, 
                        AGL_DEPTH_SIZE, 24, 
                        AGL_NONE };

    AGLPixelFormat myAGLPixelFormat;

    myAGLPixelFormat = aglChoosePixelFormat (NULL, 0, attributes);

    err = MyAGLReportError ();

    if (myAGLPixelFormat) {
        myAGLContext = aglCreateContext (myAGLPixelFormat, NULL);

        err = MyAGLReportError ();
    }

    if (! aglSetDrawable (myAGLContext, GetWindowPort (window)))
            err = MyAGLReportError ();

    if (!aglSetCurrentContext (myAGLContext))
            err = MyAGLReportError ();

    return err;

}

int main () {
  MenuRef menu;
  EventLoopTimerRef nextFrameTimer;

  WindowRef window = CreateMyWindow();
  MySetWindowAsDrawableObject(window);

  CreateNewMenu(1, 0, &menu);
  SetMenuTitleWithCFString(menu, CFSTR("Emulation"));
  InsertMenuItemTextWithCFString(menu, CFSTR("Run"), 0, 0, YUI_COMMAND_RUN);
  InsertMenuItemTextWithCFString(menu, CFSTR("Pause"), 1, 0, YUI_COMMAND_PAUSE);
  InsertMenuItemTextWithCFString(menu, CFSTR("Resume"), 2, kMenuItemAttrHidden, YUI_COMMAND_RESUME);
  InsertMenuItemTextWithCFString(menu, CFSTR("Show CPU status"), 3, 0, YUI_COMMAND_SHOW_CPU);
  InsertMenuItemTextWithCFString(menu, CFSTR("Hide CPU status"), 4, kMenuItemAttrHidden, YUI_COMMAND_HIDE_CPU);
  InsertMenuItemTextWithCFString(menu, CFSTR("Hide NBG0"), 5, 0, YUI_COMMAND_TOGGLE_NBG0);
  InsertMenuItemTextWithCFString(menu, CFSTR("Show NBG0"), 6, kMenuItemAttrHidden, YUI_COMMAND_TOGGLE_NBG0);
  InsertMenuItemTextWithCFString(menu, CFSTR("Hide NBG1"), 7, 0, YUI_COMMAND_TOGGLE_NBG1);
  InsertMenuItemTextWithCFString(menu, CFSTR("Show NBG1"), 8, kMenuItemAttrHidden, YUI_COMMAND_TOGGLE_NBG1);
  InsertMenuItemTextWithCFString(menu, CFSTR("Hide NBG2"), 9, 0, YUI_COMMAND_TOGGLE_NBG2);
  InsertMenuItemTextWithCFString(menu, CFSTR("Show NBG2"), 10, kMenuItemAttrHidden, YUI_COMMAND_TOGGLE_NBG2);
  InsertMenuItemTextWithCFString(menu, CFSTR("Hide NBG3"), 11, 0, YUI_COMMAND_TOGGLE_NBG3);
  InsertMenuItemTextWithCFString(menu, CFSTR("Show NBG3"), 12, kMenuItemAttrHidden, YUI_COMMAND_TOGGLE_NBG3);
  InsertMenuItemTextWithCFString(menu, CFSTR("Hide RBG0"), 13, 0, YUI_COMMAND_TOGGLE_RBG0);
  InsertMenuItemTextWithCFString(menu, CFSTR("Show RBG0"), 14, kMenuItemAttrHidden, YUI_COMMAND_TOGGLE_RBG0);

  SetMenuItemCommandKey(menu, 6, 0, '1');
  SetMenuItemCommandKey(menu, 7, 0, '1');
  SetMenuItemCommandKey(menu, 8, 0, '2');
  SetMenuItemCommandKey(menu, 9, 0, '2');
  SetMenuItemCommandKey(menu, 10, 0, '3');
  SetMenuItemCommandKey(menu, 11, 0, '3');
  SetMenuItemCommandKey(menu, 12, 0, '4');
  SetMenuItemCommandKey(menu, 13, 0, '4');
  SetMenuItemCommandKey(menu, 14, 0, '5');
  SetMenuItemCommandKey(menu, 15, 0, '5');

  InsertMenu(menu, 0);
  DrawMenuBar();

  EnableMenuCommand(NULL, kHICommandPreferences);

  read_settings();

  RunApplicationEventLoop();

  return 0;
}

void YuiErrorMsg(const char * string) {
	printf("%s\n", string);
}

void YuiVideoResize(unsigned int w, unsigned int h, int isfullscreen) {
}

void YuiSetVideoAttribute(int type, int val) {
}

int YuiSetVideoMode(int width, int height, int bpp, int fullscreen) {
	return 0;
}

void YuiSwapBuffers(void) {
  aglSwapBuffers(myAGLContext);
}
