/*  Copyright 2006 Guillaume Duhamel

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

#define YUI_COMMAND_RUN		1
#define YUI_COMMAND_PAUSE 	2

AGLContext  myAGLContext = NULL;
yabauseinit_struct yinit;
YuiAction key_config[] = {
	{ 0 , "Up", PerUpPressed, PerUpReleased },
	{ 0 , "Right", PerRightPressed, PerRightReleased },
	{ 0 , "Down", PerDownPressed, PerDownReleased },
	{ 0 , "Left", PerLeftPressed, PerLeftReleased },
	{ 0 , "Right trigger", PerRTriggerPressed, PerRTriggerReleased },
	{ 0 , "Left trigger", PerLTriggerPressed, PerLTriggerReleased },
	{ 0 , "Start", PerStartPressed, PerStartReleased },
	{ 0 , "A", PerAPressed, PerAReleased },
	{ 0 , "B", PerBPressed, PerBReleased },
	{ 0 , "C", PerCPressed, PerCReleased },
	{ 0 , "X", PerXPressed, PerXReleased },
	{ 0 , "Y", PerYPressed, PerYReleased },
	{ 0 , "Z", PerZPressed, PerZReleased },
	{ 0, 0, 0, 0 }
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
NULL
};

VideoInterface_struct *VIDCoreList[] = {
&VIDDummy,
&VIDOGL,
NULL
};

void YuiIdle(EventLoopTimerRef a, void * b) {
	int i;
	for(i = 0;i < 20;i++) {
		YabauseExec();
	}
}

void read_settings(void) {
	int i;
	CFStringRef s;
	yinit.percoretype = PERCORE_DUMMY;
	yinit.sh2coretype = SH2CORE_INTERPRETER;
	yinit.vidcoretype = VIDCORE_OGL;
	yinit.sndcoretype = SNDCORE_DUMMY;
	yinit.cdcoretype = CDCORE_ARCH;
	yinit.carttype = CART_NONE;
	yinit.regionid = 0;

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
	yinit.mpegpath = 0;
	yinit.cartpath = 0;

	i = 0;
	while(key_config[i].name) {
		s = CFPreferencesCopyAppValue(
			CFStringCreateWithCString(0, key_config[i].name, 0),
			kCFPreferencesCurrentApplication);
		if (s)
			key_config[i].key = CFStringGetIntValue(s);
		i++;
	}
}

void YuiRun(void) {
	EventLoopTimerUPP myFrameUPP = NewEventLoopTimerUPP(YuiIdle);

	read_settings();
	YabauseInit(&yinit);

	InstallEventLoopTimer(GetCurrentEventLoop(), kEventDurationNoWait,
		kEventDurationMillisecond, myFrameUPP, NULL, NULL);
}

OSStatus MyWindowEventHandler (EventHandlerCallRef myHandler, EventRef theEvent, void* userData)
{
  switch(GetEventClass(theEvent)) {
    case kEventClassWindow:
      switch (GetEventKind (theEvent)) {
        case kEventWindowClose:
 
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
            QuitApplicationEventLoop();
            break;
          case YUI_COMMAND_RUN:
	    YuiRun();
            break;
          case YUI_COMMAND_PAUSE:
            printf("pause\n");
            break;
          default:
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
          i = 0;
          while(key_config[i].name) {
            if (key_config[i].key == key) {
              key_config[i].press();
            }
            i++;
          }
          break;
        case kEventRawKeyUp:
          GetEventParameter(theEvent, kEventParamKeyCode,
            typeUInt32, NULL, sizeof(UInt32), NULL, &key);
          i = 0;
          while(key_config[i].name) {
            if (key_config[i].key == key) {
              key_config[i].release();
            }
            i++;
          }
          break;
      }
      break;
    }
 
  return (noErr);
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

    //AGLContext  myAGLContext = NULL;

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
  InsertMenu(menu, 0);
  DrawMenuBar();

  EnableMenuCommand(NULL, kHICommandPreferences);

/*
  {
    CFStringRef s;
    int i, j = 0;
    while(key_config[j].name) {
      s = CFStringCreateWithCString(0, key_config[j].name, 0);
      for(i=0;i < CFStringGetLength(s);i++) {
        printf("%c", CFStringGetCharacterAtIndex(s, i));
      }
      printf("\n");
      j++;
    }
  }
*/

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
