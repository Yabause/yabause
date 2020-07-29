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

/*

 ## File structure

 - [Gamcode]_[YYYY_MM_DD_HH_MM_SS]
     - record.json
     - [framenumber].png
     - backup.bin


 ## Scheme of record.json

  - records
    - frame
    - port1
    - port2
  - screenshots
    - frame
  - setting
    - isUseBios

*/

#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>


#include "bios.h"
#include "json/json.h"
#include "yabause.h"
#include "smpc.h"
#include "cs2.h"
#include "threads.h"
#include "debug.h"

#include "PlayRecorder.h"


#define LOGTAG "PlayRecorder"
using namespace std;
using namespace Json;
using std::unique_ptr;
using std::ostringstream;

extern "C" {
  extern PortData_struct PORTDATA1;
  extern PortData_struct PORTDATA2;
  void YabauseReset(void);
}

PlayRecorder::PlayRecorder() {
  mode_ = IDLE;
  scindex_ = 0;
  index_ = 0;
  take_screenshot = false;
  basedir = "./";
  f_takeScreenshot = nullptr;
}

PlayRecorder * PlayRecorder::instance = NULL;

 void PlayRecorder::PerKeyRecordInit() {
    for (int i = 0; i < PORTDATA1.size; i++) {
      PORTDATA1Pre.data[i] = PORTDATA1.data[i];
    }
    for (int i = 0; i < PORTDATA2.size; i++) {
      PORTDATA2Pre.data[i] = PORTDATA2.data[i];
    }
 }

#include <filesystem>
#ifdef _WINDOWS
namespace fs = experimental::filesystem;
extern "C" int YabMakeCleanDir(const char * dirname) {
  fs::remove_all(dirname);
  if (fs::create_directories(dirname) == false) {
    printf("Fail to create %s\n", dirname);
    return -1;
  }
  return 0;
}
#else
namespace fs = std::filesystem;
#endif


 int PlayRecorder::startRocord() {
    
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << Cs2GetCurrentGmaecode();
    //ss << std::put_time(std::localtime(&t_c), "_%Y_%m_%d_%H_%M_%S");
    std::string dirname = basedir + ss.str();

    try {
      YabMakeCleanDir(dirname.c_str());
      const char * backup = YabauseThread_getBackupPath();
      std::string dst_path = dirname + "/backup.bin";
      YabCopyFile(backup,dst_path.c_str());
    }
    catch (std::exception e) {
      return -1;
    }
     
    record_.clear();
    if (YabauseThread_IsUseBios() == 0) {
      record_["setting"]["isUseBios"] = true;
    }
    else {
      record_["setting"]["isUseBios"] = false;
    }

    std::ostringstream ssdate;
    ssdate << std::put_time(std::localtime(&t_c), "_%Y_%m_%d_%H_%M_%S");
    string date = ssdate.str();
    record_["setting"]["date"] = date;

    mode_ = RECORDING;
    dirname_ = dirname;
    PerKeyRecordInit();
    YabauseReset();
    take_screenshot = false;
    return 0;
 }

int PlayRecorder::stopRocord() {
  if (mode_ != RECORDING) return -1;
  
  Json::Value item;

  // Terminator
  item["port1"] = Json::Value(Json::arrayValue);
  for (int i = 0; i < PORTDATA1.size; i++) {
    item["port1"].append(PORTDATA1.data[i]);
  }

  item["port2"] = Json::Value(Json::arrayValue);
  for (int i = 0; i < PORTDATA2.size; i++) {
    item["port2"].append(PORTDATA2.data[i]);
  }
  item["frame"] = current_frame;
  record_["records"].append(item);

  ofstream record_file;
  string fname = dirname_ + "/record.json";
  record_file.open(fname);
  using namespace Json;
  StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "   ";  // or whatever you like
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  writer->write(record_, &record_file);
  record_file << std::endl;  // add lf and flush
  record_file.close();
  mode_ = IDLE;
  return 0;
}

int PlayRecorder::startPlay( const char * recorddir, bool clodboot, yabauseinit_struct *init ) {

  YabauseThread_resetPlaymode();

  ifstream record_file;
  string fname = string(recorddir) + "/record.json";
  try {
    record_.clear();
    record_file.open(fname);
    Json::CharReaderBuilder rbuilder;
    rbuilder["collectComments"] = false;
    std::string errs;
    bool ok = Json::parseFromStream(rbuilder, record_file, &record_, &errs);
    record_file.close();
    if (!ok) {
      printf("Fail to read %s\n", fname.c_str());
      return -1;
    }
  }catch( exception e)
  { 
    LOG(e.what());
    return -1;
  }
  index_ = 0;
  scindex_ = 0;

  if (record_["setting"]["isUseBios"].asBool() == false) {
    YabauseThread_setUseBios(0);
  }
  else {
    YabauseThread_setUseBios(-1);
  }

  string date = record_["setting"]["date"].asString();
  int y, m, d, h, M, s;
  tm ymd;
  sscanf(date.c_str(), "_%d_%d_%d_%d_%d_%d", &y,&m,&d,&h,&M,&s);
  ymd.tm_year = y - 1900;
  ymd.tm_mon = m - 1;
  ymd.tm_mday = d;
  ymd.tm_hour = h;
  ymd.tm_min = M;
  ymd.tm_sec = s;
  this->start_time = mktime(&ymd);

  string fnameback = string(recorddir) + "/backup.bin";
  dirname_ = recorddir;
  dirname_ = dirname_ + "out";
  printf("Dir is %s\n", dirname_.c_str());
  YabMakeCleanDir(dirname_.c_str());
  fnameback_test = dirname_ + "/backup.bin.new";
  YabCopyFile(fnameback.c_str(), fnameback_test.c_str());
  YabauseThread_setBackupPath(fnameback_test.c_str());
  mode_ = PLAYING;
  if (clodboot) {
    YabauseThread_coldBoot();
  }
  if( init ){
    init->buppath = fnameback_test.c_str();
  }
  return 0;
}

int PlayRecorder::proc(u32 framecount) {

  current_frame = framecount;

  if (mode_ == RECORDING) {
    PerKeyRecord(framecount, record_);
    if (take_screenshot) {
      take_screenshot = false;
      std::string fname;
      ostringstream ss;
      ss << dirname_ << "/" << current_frame << ".png";
      fname = ss.str();
      f_takeScreenshot(fname.c_str());
      record_["screenshots"].append(current_frame);
    }
    return 0;
  }

  if (mode_ == PLAYING) {
    if (framecount == 1) {
      SmpcSetClockSync(1, (u32)this->start_time);
    }

    if (index_ >= record_["records"].size()) {
      mode_ = IDLE;
      printf("Test is finished\n");
      exit(0);
      return -1; // Finish
    }

    if (framecount == record_["records"][index_]["frame"].asUInt()) {
      PerKeyPlay(record_["records"][index_]);
      index_++;
     }

    if (framecount == record_["screenshots"][scindex_].asUInt()) {
      if( f_takeScreenshot ){
        std::string fname;
        ostringstream ss;
        ss << dirname_ << "/" << current_frame << ".png";
        fname = ss.str();
        printf("Take screen shot for %s\n", fname.c_str());
        f_takeScreenshot(fname.c_str());
      } 
      scindex_++;
    }

  }
  return 0;
}

 
void PlayRecorder::PerKeyRecord(u32 frame, Json::Value & recordArray) {
    Json::Value item;
    int change = 0;

    for (int i = 0; i < PORTDATA1.size; i++) {
      if (PORTDATA1.data[i] != PORTDATA1Pre.data[i]) {
        change += 1;
      }
      PORTDATA1Pre.data[i] = PORTDATA1.data[i];
    }

    for (int i = 0; i < PORTDATA2.size; i++) {
      if (PORTDATA2.data[i] != PORTDATA2Pre.data[i]) {
        change += 1;
      }
      PORTDATA2Pre.data[i] = PORTDATA2.data[i];
    }

    if (change == 0) {
      return;
    }

    item["port1"] = Json::Value(Json::arrayValue);
    for (int i = 0; i < PORTDATA1.size; i++) {
      item["port1"].append(PORTDATA1.data[i]);
    }

    item["port2"] = Json::Value(Json::arrayValue);
    for (int i = 0; i < PORTDATA2.size; i++) {
      item["port2"].append(PORTDATA2.data[i]);
    }
    item["frame"] = frame;
    recordArray["records"].append(item);
}

void PlayRecorder::PerKeyPlay(Json::Value & item) {
    for (int i = 0; i < item["port1"].size(); i++) {
      PORTDATA1.data[i] = (u8)item["port1"][i].asUInt();
    }
    PORTDATA1.size = item["port1"].size();

    for (int i = 0; i < item["port2"].size(); i++) {
      PORTDATA2.data[i] = (u8)item["port2"][i].asUInt();
    }
    PORTDATA2.size = item["port2"].size();
}

void PlayRecorder::takeShot() {
  take_screenshot = true; // take screen shot on nextframe
}

void PlayRecorder::getVirtualTime(time_t * t) {
  u64 ctime = this->start_time + ((u64)this->current_frame * 1001 / 60000);
  *t = ctime;
}

extern "C" {
  void PlayRecorder_proc( u32 framecount ) {
    PlayRecorder * p = PlayRecorder::getInstance();
    p->proc(framecount);
  }

  int PlayRecorder_getStatus() {
    PlayRecorder * p = PlayRecorder::getInstance();
    return p->getStatus();
  }

  int PlayRecorder_getVirtualTime(time_t * t ) {
    PlayRecorder * p = PlayRecorder::getInstance();
    p->getVirtualTime(t);
    return 0;
  }


  void PlayRecorder_setPlayMode( const char * dir, yabauseinit_struct *init  ) {
    PlayRecorder * p = PlayRecorder::getInstance();
    p->startPlay(dir,false, init);
  }

}