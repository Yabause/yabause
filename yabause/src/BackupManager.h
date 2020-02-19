
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

#include <string>
#include <cstdint>
#include "bios.h"

using std::string;


class BackupManager {

protected:
  BackupManager();
public:  
  static BackupManager * instance_;
  static BackupManager * getInstance(){
    if( instance_==NULL ) instance_ = new BackupManager();
    return instance_; 
  }

  virtual ~BackupManager();
  int getDevicelist( string & jsonstr );
  int getFilelist( int deviceid, string & jsonstr );
  int deletefile( int index );
  int getFile( int index, string & jsonstr );
  int putFile( const string & jsonstr );
  int copy( int target_device, int file_index );

protected:  
  uint32_t currentbupdevice_ = 0;
  deviceinfo_struct* devices_ = NULL;
  int32_t numbupdevices_ = 0;
  int32_t current_device_ = -1;
  saveinfo_struct* saves_ = NULL;
  int32_t numsaves_ = 0;

};

