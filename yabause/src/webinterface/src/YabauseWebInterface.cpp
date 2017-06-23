

#include "CivetServer.h"
#include <cstring>
#include <string>

#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
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
#include <sstream>      // std::ostringstream

using std::ostringstream;
using std::string;

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define DOCUMENT_ROOT "."
#define PORT "8081"
#define EXAMPLE_URI "/"
#define EXIT_URI "/exit"
bool exitNow = false;


int DynarecSh2GetCurrentStatics( int cpuid, string & buf);
void DynarecSh2GetDisasmebleString(string & out, unsigned int  from, unsigned int to);

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
      "<p>To see a page from the A/B handler <a "
      "href=\"a/b\">click here</a></p>\r\n");
    mg_printf(conn,
      "<p>To see a page from the *.foo handler <a "
      "href=\"xy.foo\">click here</a></p>\r\n");
    mg_printf(conn,
      "<p>To see a page from the WebSocket handler <a "
      "href=\"ws\">click here</a></p>\r\n");
    mg_printf(conn,
      "<p>To exit <a href=\"%s\">click here</a></p>\r\n",
      EXIT_URI);
    mg_printf(conn, "</body></html>\r\n");
    return true;
  }
};

#include <map>
using std::map;

void QueryParamToMap(char * qurty, map<string, string>) {

}


using namespace std;

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


class ExecuteStatics : public CivetHandler
{
public:
  bool
    handleGet(CivetServer *server, struct mg_connection *conn)
  {
    MapCompileStatics cpu_exeute_statics;

    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: "
      "text/json\r\nConnection: close\r\n\r\n");

   const  mg_request_info *  rq = mg_get_request_info(conn);

   if (rq->query_string == NULL) {
     mg_printf(conn, "Failed!");
     return true;
   }

   std::map<std::string, std::string> query_map;
   map_pairs(rq->query_string, query_map);
   
   int cpuid = std::stoi(query_map["cpu"]);
    
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
      s << "\"end\":\"0x" << node->second.end_addr << "\", ";
      s << "\"count\":" << std::dec << node->second.count << ",";
      s << "\"time\":" << std::dec << node->second.time << "}";
      message_buf += s.str();
      node++;
      if (node != cpu_exeute_statics.end()) {
        message_buf += ",\n";
      }
    }
    message_buf += "]}";
/*
    {
      "sources" :
      [

      {
        "file" : "C:/work/EagleXX/build/CMakeFiles/glfw"
      },
*/

    //mg_printf(conn, "<html><body>\r\n");
    mg_printf(conn, message_buf.c_str());
    //mg_printf(conn, "</body></html>\r\n");

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

    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: "
      "text/plain\r\nConnection: close\r\n\r\n");

    const  mg_request_info *  rq = mg_get_request_info(conn);

    if (rq->query_string == NULL) {
      mg_printf(conn, "Failed!");
      return true;
    }

    std::map<std::string, std::string> query_map;
    map_pairs(rq->query_string, query_map);

    int from = HexToInt(query_map["from"]);
    int to = HexToInt(query_map["to"]);

    DynarecSh2GetDisasmebleString(st,from,to);

    mg_printf(conn, st.c_str());

    return true;
  }
};


CivetServer * server;
ExampleHandler h_ex;
ExecuteStatics h_execute_statics;
DissAssemble h_disassemble;

extern "C" {

int YabStartHttpServer() {

  const char *options[] = {
    "document_root", DOCUMENT_ROOT, "listening_ports", PORT, 0 };

  std::vector<std::string> cpp_options;
  for (int i = 0; i<(sizeof(options) / sizeof(options[0]) - 1); i++) {
    cpp_options.push_back(options[i]);
  }

  //int val = HexToInt(string("0x06000000"));

  // CivetServer server(options); // <-- C style start
  server = new CivetServer(cpp_options); // <-- C++ style start

  
  server->addHandler(EXAMPLE_URI, h_ex);
  server->addHandler("/execute_statics", h_execute_statics);
  server->addHandler("/disassemble", h_disassemble);

  return 0;
}

}