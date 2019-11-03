
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

#include <fstream>
#include <sstream>
#include <time.h>
#include <stdlib.h>  
#include <stdio.h>  
#define CURL_STATICLIB
#include "curl/curl.h"
#include "ygl.h"
#include "../config.h"
#include "test_framework.h"
#include "webinterface/src/picojson.h"
#include "memory.h"

#ifdef _WINDOWS
#include <direct.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif


using namespace std;

TestFramework::TestFramework():
  state_(IDLE)
{
  mtx_ = YabThreadCreateMutex();
}

int TestFramework::load( const string & title, const string & path){

  title_ = title;
  target_test_id_ = -1;
  git_hash_ = GIT_SHA1;

  time_t t = time(nullptr);
  const tm* lt = localtime(&t);
  std::stringstream s;
  s << "20";
  s << lt->tm_year - 100;
  s << "-";
  s << lt->tm_mon + 1;
  s << "-";
  s << lt->tm_mday;
  s << " ";
  s << lt->tm_hour << ":" << lt->tm_min << ":" << lt->tm_sec;
  date_ = s.str();

  s.str("");
  s.clear(stringstream::goodbit);

  s << "20";
  s << lt->tm_year - 100;
  s << "_";
  s << lt->tm_mon + 1;
  s << "_";
  s << lt->tm_mday;
  s << "_";
  s << lt->tm_hour << "_" << lt->tm_min << "_" << lt->tm_sec;
  base_dir_ = s.str();

  ifstream inputStream;
  string thisLine;
  inputStream.open(path.c_str());
  if (!inputStream.is_open())
  {
    cerr << "cannot open file!" << endl;
    return - 1;
  }

  stringstream sstream;
  while (getline(inputStream, thisLine)) {
    sstream << thisLine;
  }
  inputStream.close();
  cout << "finish opening file!" << endl;
  
  picojson::value v;
  picojson::parse(v, sstream);

  picojson::object& all = v.get<picojson::object>();
  picojson::array& array = all["test_suite"].get<picojson::array>();
  for (picojson::array::iterator it = array.begin(); it != array.end(); it++)
  {
    picojson::object& tmpObject = it->get<picojson::object>();
    TestItem tmpitem;
    
    if (tmpObject["title"].is<string>()) {
      tmpitem.title_ = tmpObject["title"].get<string>();
    }
    else {
      cout << "title is not found";
      return -1;
    }

    if (tmpObject["program_path"].is<string>()) {
      tmpitem.program_path_ = tmpObject["program_path"].get<string>();
    }
    else {
      cout << "program_path is not found";
      return -1;
    }
    
    if (tmpObject["state"].is<string>()) {
      tmpitem.state_path_ = tmpObject["state"].get<string>();
    }
    else {
      tmpitem.state_path_ = "";
    }

    string start_address;
    if (tmpObject["start_address"].is<string>() ) {
      start_address = tmpObject["start_address"].get<string>();
    }
    if (!start_address.empty()) {
      tmpitem.start_address_ = std::stoul(start_address, nullptr, 16);
    }
    else {
      tmpitem.start_address_ = 0x06004000;
    }

    picojson::array& frame_point_array = tmpObject["check_frame_count"].get<picojson::array>();
    for (picojson::array::iterator itx = frame_point_array.begin(); itx != frame_point_array.end(); itx++) {
      TestReultItem r;
      picojson::object& jitem = itx->get<picojson::object>();

      if(jitem["frame"].is<double>()) {
        r.check_frame_counts_ = (u32)jitem["frame"].get<double>();
      }
      else {
        cout << "frame is not found";
        return -1;
      }

      if (jitem["name"].is<string>()) {
        r.name_ = jitem["name"].get<string>();
      }
      else {
        cout << "name is not found";
        return -1;
      }

      tmpitem.results.push_back(r);
    }
    test_items_.push_back(tmpitem);
  }

  if (test_items_.size() == 0) {
    return -1;
  }
  
  current_test_ = 0;
  current_point_ = 0;
  frame_count_ = 0;
#if defined(_WINDOWS)
  _mkdir(base_dir_.c_str());
#else
  mkdir(base_dir_.c_str(),0700);
#endif

  if (MappedMemoryLoadExec(test_items_[0].program_path_.c_str(), test_items_[0].start_address_) != 0) {
    cout << "Fail to start test" << endl;
    return -1;
  }
  state_ = WAIT_FOR_CAPTURE_FRAME;


  return 0;
}

void TestFramework::setTarget(const string & target) {
  for (int i = 0; i < test_items_.size(); i++) {
    if (test_items_[i].title_ == target) {
      target_test_id_ = i;
      current_test_ = target_test_id_;
    }
  }
}

int TestFramework::step_in_main_thread( ) {
  
  YabThreadLock(mtx_);
  if (current_test_ >= test_items_.size()) {
    YabThreadUnLock(mtx_);
    return 0;
  }

  if (frame_count_ == 10 && test_items_[current_test_].state_path_ != "" ) {
    YabLoadState(test_items_[current_test_].state_path_.c_str());
  }

  if (FRAME_CAPTURED == state_) {
    state_ = WAIT_FOR_CAPTURE_FRAME;
    if (current_point_ >= test_items_[current_test_].results.size()) {
      current_point_ = 0;
      cout << "finished" << endl;
      if (target_test_id_ != -1) {
        current_test_ = target_test_id_;
      }
      else {
        current_test_++;
      }
      if (current_test_ >= test_items_.size()) {
        onFinidhed();
        state_ = FINISHED;
        return state_;
      }
      else {
        cout << "loading" << test_items_[current_test_].program_path_.c_str() << endl;
        MappedMemoryLoadExec(test_items_[current_test_].program_path_.c_str(), test_items_[0].start_address_);
        frame_count_ = 0;
      }
    }
  }else if(state_ == WAIT_FOR_CAPTURE_FRAME )
    if (test_items_[current_test_].results[current_point_].check_frame_counts_ <= frame_count_) {
      cout << "Check point: " << test_items_[current_test_].results[current_point_].check_frame_counts_ << endl;
      state_ = CAPTURE_FRAME;
  }
  else {
    // ??????
  }

  frame_count_++;
  YabThreadUnLock(mtx_);
  return 0;
}

int TestFramework::step_in_draw_thread() {
  YabThreadLock(mtx_);
  if (CAPTURE_FRAME == state_) {
    if (target_test_id_ != -1) {
      current_point_++;
      state_ = FRAME_CAPTURED;
      YabThreadUnLock(mtx_);
      return 0;
    }
    glFinish();
    u64 now_time = YabauseGetTicks() * 1000000 / yabsys.tickfreq;
    double difftime;
    if (now_time > pre_time_) {
      difftime = now_time - pre_time_;
    } else {
      difftime = now_time + (ULLONG_MAX - pre_time_);
    }
    std::ostringstream s;
    s << base_dir_ << "/" << current_test_ << "_" << current_point_ << ".png";
    string fname = s.str();
    if (save_screenshot != nullptr) {
      save_screenshot(fname.c_str());
    }
    TestReultItem ritem;
    ritem.image_ = fname.c_str();
    ritem.performance_ = difftime;
    test_items_[current_test_].results[current_point_].image_ = fname.c_str();
    test_items_[current_test_].results[current_point_].performance_ = difftime;
    current_point_++;
    state_ = FRAME_CAPTURED;
  }
  else {
    // ?????
  }
  YabThreadUnLock(mtx_);
  return 0;
}

void TestFramework::onStartFrame() {
  pre_time_ = YabauseGetTicks() * 1000000 / yabsys.tickfreq;
}

void TestFramework::onFinidhed() {
  //std::ofstream s("result.json", std::ofstream::binary);
  std::ostringstream s;
  s << "{" << endl;
  s << "\"date\":\"" << date_ << "\",";
  s << "\"title\":\"" << title_ << "\",";
  s << "\"git\":\"" << git_hash_ << "\"," << endl;
  s << "\"result\":[" << endl;
  for (int i = 0; i < test_items_.size(); i++) {
    s << "{";
    s << "\"title\":\"" << test_items_[i].title_ << "\",";
    s << "\"results\":[" << endl;
    for (int j = 0; j < test_items_[i].results.size(); j++) {

      int id;
      if (sendImageFile(test_items_[i].results[j].image_, id) != 0) {
        return;
      }
      s << " { \"name\":\"" << test_items_[i].results[j].name_ << "\", \"image\": " << id << ", \"performance\":" << test_items_[i].results[j].performance_ << " }";
      if (j != test_items_[i].results.size() - 1) {
        s << "," << endl;
      }
      else {
        s << "]" <<endl;
      }
    }
    s << "}";
    if (i != test_items_.size() - 1) {
      s << "," << endl;
    }
    else {
      s << "]" << endl;
    }
  }
  s << "}" << endl;
  //s.close();
  string sendstr = s.str();
  cout << sendstr;
  sendTestResult(sendstr.c_str());
}



size_t TestFramework::CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)
{
  size_t newLength = size*nmemb;
  size_t oldLength = s->size();
  try
  {
    s->resize(oldLength + newLength);
  }
  catch (std::bad_alloc &e)
  {
    //handle memory problem
    return 0;
  }

  std::copy((char*)contents, (char*)contents + newLength, s->begin() + oldLength);
  return size*nmemb;
}

struct WriteThis {
  const char *readptr;
  int sizeleft;
};

size_t TestFramework::read_callback(void *dest, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *wt = (struct WriteThis *)userp;
  size_t buffer_size = size*nmemb;

  if (wt->sizeleft) {
    /* copy as much as possible from the source to the destination */
    size_t copy_this_much = wt->sizeleft;
    if (copy_this_much > buffer_size)
      copy_this_much = buffer_size;
    memcpy(dest, wt->readptr, copy_this_much);

    wt->readptr += copy_this_much;
    wt->sizeleft -= copy_this_much;
    return copy_this_much; /* we copied this many bytes */
  }

  return 0;                          /* no more data left to deliver */
}

int TestFramework::sendTestResult( const char * data)
{
  CURL *curl;
  CURLcode res;

  struct WriteThis pooh;

  pooh.readptr = data;
  pooh.sizeleft = strlen(data);

  curl = curl_easy_init();
  if (curl) {
    /* First set the URL that is about to receive our POST. */
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3000/api/v1/testsession/upload");

    /* Now specify we want to POST data */
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    /* we want to use our own read function */
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

    /* pointer to pass to our read function */
    curl_easy_setopt(curl, CURLOPT_READDATA, &pooh);

    /* get verbose debug output please */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    struct curl_slist *list = NULL;
    list = curl_slist_append(list, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    /*
    If you use POST to a HTTP 1.1 server, you can send data without knowing
    the size before starting the POST if you use chunked encoding. You
    enable this by adding a header like "Transfer-Encoding: chunked" with
    CURLOPT_HTTPHEADER. With HTTP 1.0 or without chunked transfer, you must
    specify the size in the request.
    */
#ifdef USE_CHUNKED
    {
      struct curl_slist *chunk = NULL;

      chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      /* use curl_slist_free_all() after the *perform() call to free this
      list again */
    }
#else
    /* Set the expected POST size. If you want to POST large amounts of data,
    consider CURLOPT_POSTFIELDSIZE_LARGE */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (curl_off_t)pooh.sizeleft);
#endif

#ifdef DISABLE_EXPECT
    /*
    Using POST with HTTP 1.1 implies the use of a "Expect: 100-continue"
    header.  You can disable this header with CURLOPT_HTTPHEADER as usual.
    NOTE: if you want chunked transfer too, you need to combine these two
    since you can only set one list of headers with CURLOPT_HTTPHEADER. */

    /* A less good option would be to enforce HTTP 1.0, but that might also
    have other implications. */
    {
      struct curl_slist *chunk = NULL;

      chunk = curl_slist_append(chunk, "Expect:");
      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      /* use curl_slist_free_all() after the *perform() call to free this
      list again */
    }
#endif

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  return 0;
}



int TestFramework::sendImageFile(const string & filename, int & id ) {

  CURL *curl;
  CURLcode res;

  struct curl_httppost *formpost = NULL;
  struct curl_httppost *lastptr = NULL;
  struct curl_slist *headerlist = NULL;
  static const char buf[] = "Expect:";

  curl_global_init(CURL_GLOBAL_ALL);

  // set up the header
  curl_formadd(&formpost,
    &lastptr,
    CURLFORM_COPYNAME, "cache-control:",
    CURLFORM_COPYCONTENTS, "no-cache",
    CURLFORM_END);

  curl_formadd(&formpost,
    &lastptr,
    CURLFORM_COPYNAME, "content-type:",
    CURLFORM_COPYCONTENTS, "multipart/form-data",
    CURLFORM_END);

  curl_formadd(&formpost, &lastptr,
    CURLFORM_COPYNAME, "image",  // <--- the (in this case) wanted file-Tag!
    CURLFORM_CONTENTTYPE, "image/png",
    CURLFORM_FILE, filename.c_str(),
    CURLFORM_END);

  curl = curl_easy_init();

  headerlist = curl_slist_append(headerlist, buf);
  if (curl) {

    std::string s;
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:3000/api/v1/screenshot/upload/");
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
        curl_easy_strerror(res));
      return -1;
    }

    curl_easy_cleanup(curl);
    curl_formfree(formpost);
    curl_slist_free_all(headerlist);

    picojson::value v;
    picojson::parse(v, s);

    id = (int)(v.get<picojson::object>()["id"].get<double>());
  }
  return 0;
}

