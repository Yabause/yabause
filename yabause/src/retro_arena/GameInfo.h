#pragma once
#include "chd.h"
#include <string>
using std::string;

#include <memory>
using std::shared_ptr;

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include <sqlite3.h>

class GameInfo {
public:
  string filePath = "";
  string imageUrl = "";

  string gameTitle;
  string productNumnber;
  string makerId;
  string version;
  string releaseDate;
  string deviceInformation;
  string area;
  string inputDevice;

  int boxart;


  static shared_ptr<GameInfo> genGameInfoFromBuf(const string & filePath, const char * header);
  static shared_ptr<GameInfo> genGameInfoFromISo(const string & filePath);
  static shared_ptr<GameInfo> genGameInfoFromCHD(const string & filePath);
  static shared_ptr<GameInfo> genGameInfoFromCUE(const string & filePath);

  static string siginString;

private:
  void downloadBoxArt();

};

class GameInfoManager {
public:
  GameInfoManager();
  ~GameInfoManager();
  void clearAll();
  int insert( const shared_ptr<GameInfo> game );
  int getAll(std::vector<shared_ptr<GameInfo>> & games);
protected:
  sqlite3 *db;

};