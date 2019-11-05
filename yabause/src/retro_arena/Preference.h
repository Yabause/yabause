/*  Copyright 2018 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

Yabause is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Yabause is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <string>
using std::string;

#include <nlohmann/json.hpp>
using json = nlohmann::json;


class Preference {
public:
    Preference( const string & filename );
    ~Preference();
    int getInt( const string & key, int defaultv );
    bool getBool( const string & key, bool defaultv );
    void setInt( const string & key, int value );
    void setBool( const string & key, bool value );

protected:
  string filename;
  json j;

};