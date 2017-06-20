

#include "CivetServer.h"
#include <cstring>
#include <string>

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
      "<p>To see a page from the A handler with a parameter "
      "<a href=\"a?param=1\">click here</a></p>\r\n");
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
    std::string st;

    mg_printf(conn,
      "HTTP/1.1 200 OK\r\nContent-Type: "
      "text/plain\r\nConnection: close\r\n\r\n");

   const  mg_request_info *  rq = mg_get_request_info(conn);
   std::map<std::string, std::string> query_map;
   map_pairs(rq->query_string, query_map);
   
   int cpuid = std::stoi(query_map["cpu"]);
    
    DynarecSh2GetCurrentStatics(cpuid,st);

    mg_printf(conn, st.c_str());

    return true;
  }
};


CivetServer * server;
ExampleHandler h_ex;
ExecuteStatics h_execute_statics;

extern "C" {

int YabStartHttpServer() {

  const char *options[] = {
    "document_root", DOCUMENT_ROOT, "listening_ports", PORT, 0 };

  std::vector<std::string> cpp_options;
  for (int i = 0; i<(sizeof(options) / sizeof(options[0]) - 1); i++) {
    cpp_options.push_back(options[i]);
  }

  // CivetServer server(options); // <-- C style start
  server = new CivetServer(cpp_options); // <-- C++ style start

  
  server->addHandler(EXAMPLE_URI, h_ex);
  server->addHandler("/execute_statics", h_execute_statics);

  return 0;
}

}