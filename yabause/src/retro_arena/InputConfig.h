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

#ifndef _INPUTCONFIG_H_
#define _INPUTCONFIG_H_

#include <map>
#include <vector>
#include <string>
#include <SDL.h>
#include <sstream>
#include "pugixml/pugixml.hpp"

#define DEVICE_KEYBOARD -1

enum InputType
{
  TYPE_AXIS,
  TYPE_BUTTON,
  TYPE_HAT,
  TYPE_KEY,
  TYPE_COUNT
};

struct Input
{
public:
  int device;
  InputType type;
  int id;
  int value;
  bool configured;

  Input()
  {
    device = DEVICE_KEYBOARD;
    configured = false;
    id = -1;
    value = -999;
    type = TYPE_COUNT;
  }

  Input(int dev, InputType t, int i, int val, bool conf) : device(dev), type(t), id(i), value(val), configured(conf)
  {
  }

  std::string getHatDir(int val)
  {
    if(val & SDL_HAT_UP)
      return "up";
    else if(val & SDL_HAT_DOWN)
      return "down";
    else if(val & SDL_HAT_LEFT)
      return "left";
    else if(val & SDL_HAT_RIGHT)
      return "right";
    return "neutral?";
  }

  std::string string()
  {
    std::stringstream stream;
    switch(type)
    {
      case TYPE_BUTTON:
        stream << "Button " << id;
        break;
      case TYPE_AXIS:
        stream << "Axis " << id << (value > 0 ? "+" : "-");
        break;
      case TYPE_HAT:
        stream << "Hat " << id << " " << getHatDir(value);
        break;
      case TYPE_KEY:
        stream << "Key " << SDL_GetKeyName((SDL_Keycode)id);
        break;
      default:
        stream << "Input to string error";
        break;
    }

    return stream.str();
  }
};

class InputConfig
{
public:
  InputConfig(int deviceId, const std::string& deviceName, const std::string& deviceGUID);

  void clear();
  void mapInput(const std::string& name, Input input);
  void unmapInput(const std::string& name); // unmap all Inputs mapped to this name

  inline int getDeviceId() const { return mDeviceId; };
  inline const std::string& getDeviceName() { return mDeviceName; }
  inline const std::string& getDeviceGUIDString() { return mDeviceGUID; }

  //Returns true if Input is mapped to this name, false otherwise.
  bool isMappedTo(const std::string& name, Input input);

  //Returns a list of names this input is mapped to.
  std::vector<std::string> getMappedTo(Input input);

  void loadFromXML(pugi::xml_node root);
  void writeToXML(pugi::xml_node parent);

  bool isConfigured();

  // Returns true if there is an Input mapped to this name, false otherwise.
  // Writes Input mapped to this name to result if true.
  bool getInputByName(const std::string& name, Input* result);

private:
  std::map<std::string, Input> mNameMap;
  const int mDeviceId;
  const std::string mDeviceName;
  const std::string mDeviceGUID;
};

#endif
