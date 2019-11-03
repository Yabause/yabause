/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef _INPUTMANAGER_H_
#define _INPUTMANAGER_H_

#include <SDL.h>
#include <vector>
#include <map>
#include <string>

class InputConfig;
class YWindow;

class MenuScreen;

using std::string;

struct MenuInput {
	uint32_t select_button_ = -1;
	SDL_JoystickID select_device_ = -1;
};

typedef std::map<std::string, int *> pre_input_map;
typedef std::map<int,int> AnalogType;

//you should only ever instantiate one of these, by the way
class InputManager
{
private:
	InputManager();

	static InputManager* mInstance;

	static const int DEADZONE = 23000;

	void loadDefaultKBConfig();

	std::map<SDL_JoystickID, SDL_Joystick*> mJoysticks;
	std::map<SDL_JoystickID, InputConfig*> mInputConfigs;
	InputConfig* mKeyboardInputConfig;

	std::map<SDL_JoystickID, int*> mPrevAxisValues;
	std::map<SDL_JoystickID, uint8_t*> mHatValues;

	bool initialized() const;

	void addJoystickByDeviceIndex(int id);
	void removeJoystickByJoystickID(SDL_JoystickID id);
	bool loadInputConfig(InputConfig* config); // returns true if successfully loaded, false if not (or didn't exist)
	MenuScreen * menu_layer_ = nullptr;

  int convertFromEmustationFile( const std::string & fname );
  pre_input_map joymap_;

public:
	virtual ~InputManager();

	static InputManager* getInstance();

	void writeDeviceConfig(InputConfig* config);
	static std::string getConfigPath();

	void init( const std::string & fname );
	void deinit();

	int getNumJoysticks();
	int getButtonCountByDevice(int deviceId);
	int getNumConfiguredDevices();

	std::string getDeviceGUIDString(int deviceId);

	InputConfig* getInputConfigByDevice(int deviceId);

	int handleJoyEvents(void);
	bool parseEvent(const SDL_Event& ev );

	int handleJoyEventsMenu(void);
	bool parseEventMenu(const SDL_Event& ev );

	void setMenuLayer( MenuScreen * menu_layer );


	uint32_t showmenu_ = 0;
	void setToggleMenuEventCode( uint32_t type ){ showmenu_ = type; }

	void setGamePadomode( int user, int mode );
	int getCurrentPadMode( int user );
	std::string config_fname_;

	void updateConfig();

	void saveInputConfig( const std::string & player , const std::string & key , const std::string & type, int id , int value);

	std::vector<MenuInput> menu_inputs_;

	AnalogType _analogType;

public:
	static void genJoyString( string & out, SDL_JoystickID id, const string & name, const string & guid );
	
};

#endif
