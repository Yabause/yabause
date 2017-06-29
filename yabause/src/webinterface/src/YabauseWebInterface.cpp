/*  Copyright 2017 devMiyax(smiyaxdev@gmail.com)

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "CivetServer.h"
#include <stdio.h>
#include <cstring>
#include <string>

#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "frameprofile.h"
#include "../sh2_dynarec_devmiyax/DynarecSh2.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <cstdio>
#include <map>
#include <stdlib.h>
#include <cstring>
#include <cctype>
#include <ctype.h>
#include <iomanip>
#include <sstream>
#include <map>

#include "picojson.h"

using std::map;
using std::ostringstream;
using std::string;
using std::vector;
using std::cout;
using std::endl;



#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

extern SH2_struct *MSH2;
extern SH2_struct *SSH2;

class ExampleHandler : public CivetHandler
{
public:
  bool
    handleGet(CivetServer *server, struct mg_connection *conn)
  {
    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: "
      "text/html\r\nConnection: close\r\n\r\n");

    mg_printf(conn, "<html><body>\r\n");
    mg_printf(conn,
      "<h2>This is an example text from a C++ handler</h2>\r\n");
    mg_printf(conn,"<p>To see Master CPU Execute Statics <a href=\"execute_statics/?cpu=0\">click here</a></p>\r\n");
    mg_printf(conn,"<p>To see Slave CPU Execute Statics <a href=\"execute_statics/?cpu=1\">click here</a></p>\r\n");
    mg_printf(conn,
      "<p>To DisAssemble code "
      "<a href=\"disassemble?from=0x06000000&to=0x06000040\">click here</a></p>\r\n");
    mg_printf(conn,
      "<p>memory"
      "<a href=\"memory?from=0x06000000&type=0&size=1024\">click here</a></p>\r\n");
    mg_printf(conn,
      "<p>To see a page from the A/B handler <a "
      "href=\"a/b\">click here</a></p>\r\n");
    mg_printf(conn,
      "<p>To see a page from the *.foo handler <a "
      "href=\"xy.foo\">click here</a></p>\r\n");
    mg_printf(conn,
      "<p>To see a page from the WebSocket handler <a "
      "href=\"ws\">click here</a></p>\r\n");
    mg_printf(conn, "</body></html>\r\n");
    return true;
  }
};

vector<string> split(const string& s, const string& delim, const bool keep_empty = true)
{
  vector<string> result;
  if (delim.empty()) {
    result.push_back(s);
    return result;
  }
  string::const_iterator substart = s.begin(), subend;
  while (true) {
    subend = search(substart, s.end(), delim.begin(), delim.end());
    string temp(substart, subend);
    if (keep_empty || !temp.empty()) {
      result.push_back(temp);
    }
    if (subend == s.end()) {
      break;
    }
    substart = subend + delim.size();
  }
  return result;
}

int HexToInt(std::string hex) {
  int a = 0;
  int tmp = 0;
  int step = 0;
  auto c = hex.end();
  c--;
  while (c != hex.begin()) {
    switch (*c) {
    case '0': tmp = 0; break;
    case '1': tmp = 1; break;
    case '2': tmp = 2; break;
    case '3': tmp = 3; break;
    case '4': tmp = 4; break;
    case '5': tmp = 5; break;
    case '6': tmp = 6; break;
    case '7': tmp = 7; break;
    case '8': tmp = 8; break;
    case '9': tmp = 9; break;
    case 'a': tmp = 10; break;
    case 'A': tmp = 10; break;
    case 'b': tmp = 11; break;
    case 'B': tmp = 11; break;
    case 'c': tmp = 12; break;
    case 'C': tmp = 12; break;
    case 'd': tmp = 13; break;
    case 'D': tmp = 13; break;
    case 'e': tmp = 14; break;
    case 'E': tmp = 14; break;
    case 'f': tmp = 15; break;
    case 'F': tmp = 15; break;
    default: tmp = 0;
    }
    
    // 0,16,256,4096
    a += tmp*(1<<(step*4));
    step++;
    c--;
  }
  return a;
}

std::map<std::string, std::string>& map_pairs(const char* character, std::map<std::string, std::string>& Elements)
{
  string test;
  string key;
  string value;
  vector<string>::iterator it;  
  string character_string = character;
  vector<string> words;

  //Elements.insert(std::pair<string,string>("0","0"));
  //cout << "########## SPLITTING STRING INTO SMALLER CHUNKS ################## " << endl;
  words = split(character_string, "&");
  //cout << "########## GENERATING KEY VALUE PAIRS ################## " << endl;
  for (it = words.begin(); it != words.end(); ++it)
  {
    test = *it;
    cout << "string:" << test << endl;

    const string::size_type pos_eq = test.find('=');
    if (pos_eq != string::npos)
    {
      //Assign Key and Value Respectively
      key = test.substr(0, pos_eq);
      value = test.substr(pos_eq + 1);
      //cout << "key = " << key << " and value = " << value << endl;
      Elements.insert(std::pair<string, string>(key, value));
    }

  }

  return Elements;
}




class Resume : public CivetHandler
{
public:

  int DynarecSh2Resume(int cpuid) {
    DynarecSh2* pctx = NULL;
    if (cpuid == 0) {
      pctx = ((DynarecSh2*)MSH2->ext);
    }
    else if (cpuid == 1) {
      pctx = ((DynarecSh2*)SSH2->ext);
    }
    else {
      return -1;
    }

    if (pctx) {
      return pctx->Resume();
    }
    return -1;
  }

  bool
    handleGet(CivetServer *server, struct mg_connection *conn)
  {
    const  mg_request_info *  rq = mg_get_request_info(conn);
    if (rq->query_string == NULL) {
      mg_printf(conn, "Failed!");
      return true;
    }
    std::map<std::string, std::string> query_map;
    map_pairs(rq->query_string, query_map);

    int cpuid = atoi(query_map["cpu"].c_str());
    DynarecSh2Resume(cpuid);
    return true;
  }
};


class ExecuteStatics : public CivetHandler
{
public:

  int DynarecSh2GetCurrentStatics(int cpuid, MapCompileStatics & buf) {
    DynarecSh2* pctx = NULL;
    if (cpuid == 0) {
      pctx = ((DynarecSh2*)MSH2->ext);
    }
    else if (cpuid == 1) {
      pctx = ((DynarecSh2*)SSH2->ext);
    }
    else {
      return -1;
    }

    if (pctx) {
      return pctx->GetCurrentStatics(buf);
    }
    return -1;
  }

  bool
    handleGet(CivetServer *server, struct mg_connection *conn)
  {
    MapCompileStatics cpu_exeute_statics;

    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: "
      "application/json\r\nConnection: close\r\n"
      "Access-Control-Allow-Origin: *\r\n\r\n");

   const  mg_request_info *  rq = mg_get_request_info(conn);
   if (rq->query_string == NULL) {
     mg_printf(conn, "Failed!");
     return true;
   }
   std::map<std::string, std::string> query_map;
   map_pairs(rq->query_string, query_map);
   
   int cpuid = atoi(query_map["cpu"].c_str());
    
    DynarecSh2GetCurrentStatics(cpuid, cpu_exeute_statics);
    string message_buf;

    // Generate JSON String
    /*
    {
      "CpuStatics" :
      [ 
        {"start":"0x1111","end":0xFFFF","count":100,"time":100},
        {"start":"0x1111","end":0xFFFF","count":100,"time":100}
      ]
    }
    */
    message_buf = "{\"CpuStatics\" :[";
    auto node = cpu_exeute_statics.begin();
    while (node != cpu_exeute_statics.end()) {
      std::ostringstream s;
      s << "{\"start\":\"0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << node->first << "\", ";
      s << "\"end\":\"0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex  << node->second.end_addr << "\", ";
      s << "\"count\":" << std::dec << node->second.count << ",";
      s << "\"time\":" << std::dec << node->second.time << "}";
      message_buf += s.str();
      node++;
      if (node != cpu_exeute_statics.end()) {
        message_buf += ",\n";
      }
    }
    message_buf += "]}";
    mg_printf(conn, message_buf.c_str());

    return true;
  }
};


class DissAssemble : public CivetHandler
{
public:
  bool
    handleGet(CivetServer *server, struct mg_connection *conn)
  {
    std::string st;
    const  mg_request_info *  rq = mg_get_request_info(conn);

    if (rq->query_string == NULL) {
      mg_printf(conn, "Failed!");
      return true;
    }

    std::map<std::string, std::string> query_map;
    map_pairs(rq->query_string, query_map);

    u32 from = HexToInt(query_map["from"]);
    u32 to = HexToInt(query_map["to"]);

    picojson::value v1;
    picojson::object  o;
    char linebuf[128];

    v1.set<picojson::object>(picojson::object());
    v1.get<picojson::object>()["code"].set<picojson::array>(picojson::array());

    for (u32 i = from; i < (to + 2); i += 2) {
      SH2Disasm(i, memGetWord(i), 0, NULL, linebuf);
      v1.get<picojson::object>()["code"].get<picojson::array>().push_back(picojson::value(linebuf));
    }

    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: "
      "application/json\r\nConnection: close\r\n"
      "Access-Control-Allow-Origin: *\r\n\r\n");

    mg_printf(conn, v1.serialize().c_str() );

    return true;
  }
};


class GetMemory : public CivetHandler
{
public:
  bool
    handleGet(CivetServer *server, struct mg_connection *conn)
  {
    std::string st;
    const  mg_request_info *  rq = mg_get_request_info(conn);

    if (rq->query_string == NULL) {
      mg_printf(conn, "Failed!");
      return true;
    }

    std::map<std::string, std::string> query_map;
    map_pairs(rq->query_string, query_map);

    u32 from = HexToInt(query_map["from"]);
    u32 type = atoi(query_map["type"].c_str());
    u32 size = atoi(query_map["size"].c_str());

    vector<u32> vbuf;

    picojson::value v1;
    picojson::object  o;

    v1.set<picojson::object>(picojson::object());
    v1.get<picojson::object>()["type"] = picojson::value((double)type);
    v1.get<picojson::object>()["start"] = picojson::value((double)from);
    v1.get<picojson::object>()["memory"].set<picojson::array>(picojson::array());

    switch (type) {
    case 0: // byte
    {
      for (u32 i = from; i < from + size; i ++) {
        v1.get<picojson::object>()["memory"].get<picojson::array>().push_back(picojson::value((double)memGetByte(i)));
      }
    }
      break;
    case 1: // word
    {
      for (u32 i = from; i < from+size; i += 2) {
        v1.get<picojson::object>()["memory"].get<picojson::array>().push_back(picojson::value((double)memGetWord(i)));
      }
    }
    break;
    case 2: // dword
    {
      {
        for (u32 i = from; i < from + size; i += 4) {
          v1.get<picojson::object>()["memory"].get<picojson::array>().push_back(picojson::value((double)memGetLong(i)));
        }
      }
    }
    break;
    default:
      return false;
      break;
    }

    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: "
      "application/json\r\nConnection: close\r\n"
      "Access-Control-Allow-Origin: *\r\n\r\n");

    mg_printf(conn, v1.serialize().c_str());

    return true;
  }
};

class GetFrameprofile : public CivetHandler
{
public:
  bool
    handleGet(CivetServer *server, struct mg_connection *conn)
  {
    std::string st;
    vector<u32> vbuf;

    picojson::value v1;

    ProfileInfo * pfp = NULL;
    int size;

    v1.set<picojson::object>(picojson::object());
    v1.get<picojson::object>()["profile"].set<picojson::array>(picojson::array());


    FrameGetLastProfile( &pfp,&size);

    u32 intime = 0;
    u32 extime = 0;
    for (int i = 0; i < size; i++) {
      if (i > 0) {
        intime = pfp[i].time - pfp[i - 1].time;
        extime += intime;
      }
      picojson::value v2;
      v2.set<picojson::object>(picojson::object());
      v2.get<picojson::object>()["event"] = picojson::value(pfp[i].event);
      v2.get<picojson::object>()["time"] = picojson::value((double)intime);
      v1.get<picojson::object>()["profile"].get<picojson::array>().push_back(v2);
    }

    if (pfp != NULL) {
      free((void*)pfp);
    }


    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: "
      "application/json\r\nConnection: close\r\n"
      "Access-Control-Allow-Origin: *\r\n\r\n");

    mg_printf(conn, v1.serialize().c_str());

    return true;
  }
};


class ResumeFrameprofile : public CivetHandler
{
public:
  bool
    handleGet(CivetServer *server, struct mg_connection *conn)
  {
    picojson::value v1;
    FrameResume();
    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: "
      "text/text\r\nConnection: close\r\n"
      "Access-Control-Allow-Origin: *\r\n\r\n");

    mg_printf(conn, "OK");

    return true;
  }
};


class ConnectionTest : public CivetHandler
{
public:
  bool
    handleGet(CivetServer *server, struct mg_connection *conn)
  {
    picojson::value v1;

    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: "
      "text/text\r\nConnection: close\r\n"
      "Access-Control-Allow-Origin: *\r\n\r\n");

    mg_printf(conn, "OK");

    return true;
  }
};

CivetServer * server = nullptr;
ExampleHandler h_ex;
ExecuteStatics h_execute_statics;
DissAssemble h_disassemble;
Resume h_resume;
GetMemory h_getMemory;
GetFrameprofile h_frame_profile;
ResumeFrameprofile h_resume_frame;
ConnectionTest h_test;

#define DOCUMENT_ROOT "."
#define PORT "8081"
#define EXAMPLE_URI "/"
#define EXIT_URI "/exit"
bool exitNow = false;

extern "C" {

int YabStartHttpServer() {

  if (server != nullptr) return 0;

  const char *options[] = {
    "document_root", DOCUMENT_ROOT, "listening_ports", PORT, 0 };

  std::vector<std::string> cpp_options;
  for (int i = 0; i<(sizeof(options) / sizeof(options[0]) - 1); i++) {
    cpp_options.push_back(options[i]);
  }

  server = new CivetServer(cpp_options); // <-- C++ style start
  
  server->addHandler(EXAMPLE_URI, h_ex);
  server->addHandler("/execute_statics", h_execute_statics);
  server->addHandler("/disassemble", h_disassemble);
  server->addHandler("/resume", h_resume);
  server->addHandler("/memory", h_getMemory);
  server->addHandler("/frame", h_frame_profile);
  server->addHandler("/resume_frame", h_resume_frame);
  server->addHandler("/test", h_test);

  return 0;
}

}