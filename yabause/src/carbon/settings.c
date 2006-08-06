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
#include "settings.h"

#define		TAB_ID 	 	128
#define		TAB_SIGNATURE	'tabs'

void SelectItemOfTabControl(ControlRef tabControl)
{
    ControlRef userPane;
    ControlID controlID;
    UInt16 i;
    int tabList[] = {129, 130, 131, 132};
    int lastTabIndex;

    lastTabIndex = GetControlValue(tabControl);
    controlID.signature = TAB_SIGNATURE;

    for (i = 1; i < 5; i++)
    {
        controlID.id = tabList[i - 1];
        GetControlByID(GetControlOwner(tabControl), &controlID, &userPane);
       
        if (i == lastTabIndex) {
            EnableControl(userPane);
            SetControlVisibility(userPane, true, true);
        } else {
            SetControlVisibility(userPane, false, false);
            DisableControl(userPane);
        }
    }
    
    Draw1Control(tabControl);
}

pascal OSStatus TabEventHandler(EventHandlerCallRef inHandlerRef, EventRef inEvent, void *inUserData)
{
    ControlRef control;
    
    GetEventParameter(inEvent, kEventParamDirectObject, typeControlRef,
	NULL, sizeof(ControlRef), NULL, &control );
    
    SelectItemOfTabControl(control);
    
    return eventNotHandledErr;
}

void InstallTabHandler(WindowRef window)
{
    EventTypeSpec	controlSpec = { kEventClassControl, kEventControlHit };
    ControlRef 		tabControl;
    ControlID 		controlID;

    controlID.signature = TAB_SIGNATURE;
    controlID.id = TAB_ID;
    GetControlByID(window, &controlID, &tabControl);

    InstallControlEventHandler(tabControl,
                        NewEventHandlerUPP( TabEventHandler ),
                        1, &controlSpec, 0, NULL);

    SelectItemOfTabControl(tabControl); 
}

CFStringRef get_settings(WindowRef window, int i) {
	ControlID id;
	ControlRef control;
	CFStringRef s;

	id.signature = 'conf';
	id.id = i;
	GetControlByID(window, &id, &control);
	GetControlData(control, kControlEditTextPart,
		kControlEditTextCFStringTag, sizeof(CFStringRef), &s, NULL);

	return s;
}

void set_settings(WindowRef window, int i, CFStringRef s) {
	ControlID id;
	ControlRef control;

	if (s) {
		id.signature = 'conf';
		id.id = i;
		GetControlByID(window, &id, &control);
		SetControlData(control, kControlEditTextPart,
			kControlEditTextCFStringTag, sizeof(CFStringRef), &s);
	}
}

void save_settings(WindowRef window) {
	int i;
	CFStringRef s;

	CFPreferencesSetAppValue(CFSTR("BiosPath"), get_settings(window, 1),
		kCFPreferencesCurrentApplication);
	CFPreferencesSetAppValue(CFSTR("CDROMDrive"), get_settings(window, 2),
		kCFPreferencesCurrentApplication);

	i = 0;
	while(key_config[i].name) {
		s = get_settings(window, 31 + i);
		CFPreferencesSetAppValue(
			CFStringCreateWithCString(0, key_config[i].name, 0),
			s, kCFPreferencesCurrentApplication);
		key_config[i].key = CFStringGetIntValue(s);
		i++;
	}

	CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

void load_settings(WindowRef window) {
	int i;

	set_settings(window, 1, CFPreferencesCopyAppValue(CFSTR("BiosPath"),
		kCFPreferencesCurrentApplication));
	set_settings(window, 2, CFPreferencesCopyAppValue(CFSTR("CDROMDrive"),
		kCFPreferencesCurrentApplication));

	i = 0;
	while(key_config[i].name) {
		set_settings(window, 31 + i, CFPreferencesCopyAppValue(
			CFStringCreateWithCString(0, key_config[i].name, 0),
			kCFPreferencesCurrentApplication));
		i++;
	}
}

OSStatus SettingsWindowEventHandler (EventHandlerCallRef myHandler, EventRef theEvent, void* userData)
{
  OSStatus result = eventNotHandledErr;

  switch (GetEventKind (theEvent))
    {
    case kEventWindowClose:
      {
	WindowRef window;
        GetEventParameter(theEvent, kEventParamDirectObject, typeWindowRef,
	  0, sizeof(typeWindowRef), 0, &window);

	save_settings(window);

        DisposeWindow(window);
      }
      result = noErr;
      break;

    }
 
  return (result);
}

OSStatus KeyConfigHandler(EventHandlerCallRef h, EventRef event, void* data) {
	UInt32 key;
	CFStringRef s;
	Str255 tmp;
        GetEventParameter(event, kEventParamKeyCode,
		typeUInt32, NULL, sizeof(UInt32), NULL, &key);
	NumToString(key, tmp);
	s = CFStringCreateWithPascalString(0, tmp, 0);
	SetControlData(data, kControlEditTextPart,
		kControlEditTextCFStringTag, sizeof(CFStringRef), &s);
	Draw1Control(data);

	return noErr;
}

WindowRef CreateSettingsWindow() {

  WindowRef myWindow;
  IBNibRef nib;

  EventTypeSpec eventList[] = {
    { kEventClassWindow, kEventWindowClose }
  };

  CreateNibReference(CFSTR("preferences"), &nib);
  CreateWindowFromNib(nib, CFSTR("Dialog"), &myWindow);

  load_settings(myWindow);

  InstallTabHandler(myWindow);
  {
    int i;
    ControlRef control;
    ControlID id;
    EventTypeSpec elist[] = {
      { kEventClassKeyboard, kEventRawKeyDown },
      { kEventClassKeyboard, kEventRawKeyUp }
    };

    id.signature = 'conf';
    i = 0;
    while(key_config[i].name) {
      id.id = 31 + i;
      GetControlByID(myWindow, &id, &control);

      InstallControlEventHandler(control, NewEventHandlerUPP(KeyConfigHandler),
	GetEventTypeCount(elist), elist, control, NULL);
      i++;
    }
  }

  ShowWindow(myWindow);

  InstallWindowEventHandler(myWindow,
			    NewEventHandlerUPP (SettingsWindowEventHandler),
			    GetEventTypeCount(eventList),
			    eventList, myWindow, NULL);

  return myWindow;
}
