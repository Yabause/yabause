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

#ifndef _TEST_FRAMEWORK_
#define _TEST_FRAMEWORK_

#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "frameprofile.h"
#include "threads.h"
#include <string>
#include <vector>

using std::string;
using std::vector;

/*
  test file foramt
  {
    test_suite: [
      {
        program_path:"./normal_scroll/window_controll/sl_coff.coff",
        start_address:"0x06004000",
        check_frame_count:[ 32,64,28 ]
      },
      {
        program_path:"./normal_scroll/window_controll/sl_coff.coff",
        start_address:"0x06004000",
        check_frame_count:[ 32,64,128 ]
      }
    ]
  }


*/

/*

{
"date" : "2017-01-01 16:00:00",
"title" : "yaba sanshiro",
"git" : "aaaaaaa",
"result" : [
{
"title" : "5.3 Line Scroll",
"results" : [
{ "image" : "filename.png", "performance" : 14666 },
{ "image" : "filename.png", "performance" : 14666 },
{ "image" : "filename.png", "performance" : 14666 }
]
}
]
}
*/

class TestReultItem {
public:
  string name_ = "";
  u32 check_frame_counts_ = 0;
  string image_ = "";
  double performance_ = 0.0;
};

class TestItem{
public:
  string title_;
  string program_path_;
  string state_path_;
  u32 start_address_;
  TestItem(){
    program_path_ = "";
    state_path_ = "";
    start_address_ = 0x06004000;
  }
  vector<TestReultItem> results;

};



typedef int (*SAVE_SCREENSHOT)(const char * save_filename);

class TestFramework {
public:
  enum eState{
    IDLE,
    WAIT_FOR_CAPTURE_FRAME,
    CAPTURE_FRAME,
    FRAME_CAPTURED,
    FINISHED
  };
  TestFramework();
  vector<TestItem> test_items_;
  

  eState state_;

  string date_;
  string title_;
  string git_hash_;

  u32 frame_count_ = 0;
  u32 current_test_  = 0;
  u32 current_point_ = 0;
  u64 pre_time_ = 0;
  int load( const string & title,  const string & path);
  int step_in_main_thread();
  int step_in_draw_thread();
  void onStartFrame();

  void setSaveScreenShotCallback(SAVE_SCREENSHOT callback ) {
    save_screenshot = callback;
  }

  void setTarget(const string & target);

protected:
  YabMutex * mtx_ = nullptr;
  bool is_check_point_ = false;
  string base_dir_;
  void onCheckPoint();
  void onFinidhed();
  static size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s);
  int sendImageFile(const string & filename, int & id);
  static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp);
  int sendTestResult(const char * data);
  SAVE_SCREENSHOT save_screenshot = nullptr;
  int target_test_id_ = -1;
};


#endif // _TEST_FRAMEWORK_