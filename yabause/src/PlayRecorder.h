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

#include "BackupManager.h"
#include "bios.h"


#include "json/json.h"
#include <memory>
#include <fstream>
#include <iostream>
#include <sstream>
#include <functional>
#include "smpc.h"
#include <time.h>

using namespace std;
using namespace Json;
using std::unique_ptr;
using std::ostringstream;


class PlayRecorder {
private:
  PlayRecorder();
  static PlayRecorder * instance;

  PortData_struct PORTDATA1Pre;
  PortData_struct PORTDATA2Pre;
  u32 index_;
  u32 scindex_;
  Json::Value record_;
  u32 current_frame;
  std::string dirname_;
  bool take_screenshot;
  u64 start_time;
  string fnameback_test;

  string basedir;
  
public:
  enum eStatus {
    IDLE = -1,
    RECORDING = 0,
    PLAYING = 1
  };
  static PlayRecorder * getInstance() {
    if (PlayRecorder::instance == NULL) {
      PlayRecorder::instance = new PlayRecorder();
    }
    return PlayRecorder::instance;
  }
  void PerKeyRecordInit();
  eStatus getStatus() {
    return mode_;
  }
  int startRocord();
  int stopRocord();
  int startPlay( const char * record_dir, bool clodboot, yabauseinit_struct *init);
  int proc(u32 framecount);
  void PerKeyRecord(u32 frame, Json::Value & recordArray);
  void PerKeyPlay(Json::Value & item);
  void getVirtualTime(time_t * t);

  std::function<void(const char *)> f_takeScreenshot;

  void takeShot();

  void setBaseDir( const char * dir){ basedir = dir; }

private: 
  eStatus mode_;  

};

