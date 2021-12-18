
//#define CPPHTTPLIB_OPENSSL_SUPPORT
#define CPPHTTPLIB_ZLIB_SUPPORT
#include "httplib.h"

#include <stdio.h>
#include "cdbase.h"
#include "yabause.h"
#include "threads.h"
#include <string>
using std::string;

#include <vector>
using std::vector;

#include <mutex>
using std::mutex;

#include <unordered_map>
using std::unordered_map;

extern "C"
{

  static int WebCDInit(const char *);
  static void WebCDDeInit(void);
  static s32 WebCDReadTOC(u32 *);
  static int WebCDGetStatus(void);
  static int WebCDReadSectorFAD(u32, void *);
  static void WebCDReadAheadFAD(u32);
  static void WebCDSetStatus(int status);

  CDInterface WebApiCD = {
      CDCORE_WEBAPI,
      "Web Api CD Driver",
      WebCDInit,
      WebCDDeInit,
      WebCDGetStatus,
      WebCDReadTOC,
      WebCDReadSectorFAD,
      WebCDReadAheadFAD,
      WebCDSetStatus,
  };
}

const int sectorSize = 2448;

class WdbCD
{
protected:
  static WdbCD *instance;
  WdbCD() {}

  string base = "http://192.168.11.5:8080";
  string session = "";

  std::mutex transMutex;

  unordered_map<u32, char[sectorSize]> memCache;

public:
  virtual ~WdbCD() {}
  virtual int init() = 0;
  virtual void deInit() = 0;
  virtual s32 readToc(u32 *) = 0;
  virtual int getStatus() = 0;
  virtual int readSectorFAD(u32 addr, void *buf) = 0;
  virtual int setStatus(int status) = 0;
};

WdbCD *WdbCD::instance = nullptr;

class WebCDHttplib : WdbCD
{

protected:
  WebCDHttplib()
  {
  }

public:
  static WdbCD *getInstance()
  {
    if (instance == NULL)
      instance = new WebCDHttplib();
    return instance;
  }

  virtual ~WebCDHttplib() {}
  virtual int init()
  {

    yprintf("WebCDHttplib init start");

    if (session != "")
    {
      deInit();
    }

    httplib::Client cli(base.c_str());
    string param = "/games/open?id=33";
    auto res = cli.Get(param.c_str());
    session = res->body;

    yprintf("WebCDHttplib init %s", session.c_str());

    /*
    if (auto res = cli.Get(param.c_str())) {
      if (res->status == 200) {
        session = res->body;
      }
    }
    else {
      auto err = res.error();

    }
*/
    return 0;
  }

  virtual void deInit()
  {
    httplib::Client cli(base.c_str());
    string param = "/games/close?session=" + session;
    auto res = cli.Get(param.c_str());

    session = "";
  }

  virtual s32 readToc(u32 *toc)
  {

    if (session == "")
      return 0xFFFFFFFF;

    httplib::Client cli(base.c_str());
    string param = "/games/toc?session=" + session;

    vector<char> rtnbuf;
    int status = 200;

    auto res = cli.Get(
        param.c_str(), httplib::Headers(),
        [&](const httplib::Response &response)
        {
          std::lock_guard<std::mutex> guard(transMutex);
          status = response.status;
          return true;
        },
        [&](const char *data, size_t data_length)
        {
          for (int i = 0; i < data_length; i++)
          {
            std::lock_guard<std::mutex> guard(transMutex);
            std::cout << data_length;
            rtnbuf.push_back(data[i]);
          }
          return true; // return 'false' if you want to cancel the request.
        });

    while (1)
    {
      {
        std::lock_guard<std::mutex> guard(transMutex);
        if (!(rtnbuf.size() < (0xCC * 2) && status == 200))
        {
          break;
        }
      }
      YabNanosleep(100000);
    }

    memcpy(toc, rtnbuf.data(), 0xCC * 2);
    return rtnbuf.size();
  }

  virtual int getStatus()
  {
    if (session == "")
      return -1;

    httplib::Client cli(base.c_str());
    string param = "/games/status?session=" + session;
    auto res = cli.Get(param.c_str());

    if (res->status != 200)
    {
      return -1;
    }

    return std::stoi(res->body);
  }

  virtual int readSectorFAD(u32 addr, void *buf)
  {

    if (session == "")
      return 0xFFFFFFFF;

    /*
    auto pos = memCache.find(addr);
    if (pos != memCache.end()) {
      memcpy(buf, pos->second, sectorSize);
    }
*/
    httplib::Client cli(base.c_str());
    string param = "/games/read?session=" + session + "&fad=" + std::to_string(addr);

    vector<char> rtnbuf;
    int status = 200;

    httplib::Headers headers = {
        {"content-type", "application/octet-stream"},
        {"Accept-Encoding", "gzip"}};

    auto res = cli.Get(
        param.c_str(), headers,
        [&](const httplib::Response &response)
        {
          std::lock_guard<std::mutex> guard(transMutex);
          status = response.status;
          return true;
        },
        [&](const char *data, size_t data_length)
        {
          std::lock_guard<std::mutex> guard(transMutex);
          std::cout << data_length;
          for (int i = 0; i < data_length; i++)
          {
            rtnbuf.push_back(data[i]);
          }
          return true;
        });

    while (1)
    {
      {
        std::lock_guard<std::mutex> guard(transMutex);
        if (!(rtnbuf.size() < sectorSize && status == 200))
        {
          break;
        }
      }
      YabNanosleep(100000);
    }

    /*
    memcpy(memCache[addr], rtnbuf.data(), sectorSize);
    if (memCache.size() > 32) {
      auto pos = memCache.begin();
      memCache.erase(pos);
    }
*/
    memcpy(buf, rtnbuf.data(), sectorSize);

    return rtnbuf.size();
  }

  virtual int setStatus(int status)
  {
    return 0;
  }
};

extern "C"
{

  int WebCDInit(const char *)
  {
    WdbCD *instance = (WdbCD *)WebCDHttplib::getInstance();
    return instance->init();
  }

  void WebCDDeInit(void)
  {
    WdbCD *instance = (WdbCD *)WebCDHttplib::getInstance();
    instance->deInit();
  }

  s32 WebCDReadTOC(u32 *toc)
  {
    WdbCD *instance = (WdbCD *)WebCDHttplib::getInstance();
    return instance->readToc(toc);
  }

  int WebCDGetStatus(void)
  {
    WdbCD *instance = (WdbCD *)WebCDHttplib::getInstance();
    return instance->getStatus();
  }

  int WebCDReadSectorFAD(u32 addr, void *buf)
  {
    WdbCD *instance = (WdbCD *)WebCDHttplib::getInstance();
    return instance->readSectorFAD(addr, buf);
  }

  void WebCDReadAheadFAD(u32 fad)
  {
    return;
  }

  void WebCDSetStatus(int status)
  {
    WdbCD *instance = (WdbCD *)WebCDHttplib::getInstance();
    instance->setStatus(status);
  }
}