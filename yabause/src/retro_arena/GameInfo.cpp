#include "GameInfo.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "common.h"

const std::string WHITESPACE = " \n\r\t\f\v";

std::string ltrim(const std::string &s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string &s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s) {
  return rtrim(ltrim(s));
}


string GameInfo::siginString = "";

shared_ptr<GameInfo> GameInfo::genGameInfoFromBuf(const string & filePath, const char * header) {
    int startIndex = -1;
    const char * startPoint = "SEGA ";
    for (int i = 0; i < strlen(header); i++) {
      if (header[i + 0] == startPoint[0] &&
        header[i + 1] == startPoint[1] &&
        header[i + 2] == startPoint[2] &&
        header[i + 3] == startPoint[3] &&
        header[i + 4] == startPoint[4]
        ) {
        startIndex = i;
        break;
      }
    }
    if (startIndex == -1) return nullptr;

    shared_ptr<GameInfo> ginfo(new GameInfo);
    ginfo->filePath = filePath;

    string tmph = string(&header[startIndex]);

    ginfo->gameTitle = tmph.substr(0x60,0x70);
    ginfo->gameTitle = trim(ginfo->gameTitle);

    ginfo->makerId = tmph.substr(0x10, 0x10);
    ginfo->makerId = trim(ginfo->makerId);

    ginfo->productNumnber = tmph.substr(0x20, 0xA);
    ginfo->productNumnber = trim(ginfo->productNumnber);

    ginfo->version = tmph.substr(0x2A, 0x10);
    ginfo->version = trim(ginfo->version);

    ginfo->releaseDate = tmph.substr(0x30, 0x8);
    ginfo->releaseDate = trim(ginfo->releaseDate);

    ginfo->deviceInformation = tmph.substr(0x38, 0x8);
    ginfo->deviceInformation = trim(ginfo->deviceInformation);

    ginfo->area = tmph.substr(0x40, 0xA);
    ginfo->area = trim(ginfo->area);

    ginfo->inputDevice = tmph.substr(0x50, 0x10);
    ginfo->inputDevice = trim(ginfo->inputDevice);

    ginfo->downloadBoxArt();

    return ginfo;
};


shared_ptr<GameInfo> GameInfo::genGameInfoFromISo(const string & filePath) {
    FILE * fp = fopen(filePath.c_str(), "rb");
    if (fp == nullptr) {
      return nullptr;
    }
    char buf[256];
    fread(buf, 1, 256, fp);
    fclose(fp);
    return genGameInfoFromBuf(filePath, buf);
}

shared_ptr<GameInfo> GameInfo::genGameInfoFromCHD(const string & filePath) {

    chd_file *chd;
    const int len = 256;
    char buf[len];

    chd_error error = chd_open(filePath.c_str(), CHD_OPEN_READ, NULL, &chd);
    if (error != CHDERR_NONE) {
      return nullptr;
    }
    const chd_header * header = chd_get_header(chd);
    if (header == NULL) {
      return nullptr;
    }

    char * hunk_buffer = (char*)malloc(header->hunkbytes);
    chd_read(chd, 0, hunk_buffer);

    //char header_buff[256];
    memcpy(buf, &hunk_buffer[16], len);
    buf[len - 1] = 0;
    free(hunk_buffer);
    chd_close(chd);
    return genGameInfoFromBuf(filePath, buf);
  }


shared_ptr<GameInfo> GameInfo::genGameInfoFromCUE(const string & filePath) {

    long size;
    char *temp_buffer;
    int matched = 0;

    FILE * iso_file = fopen(filePath.c_str(), "rb");

    fseek(iso_file, 0, SEEK_END);
    size = ftell(iso_file);
    if (size <= 0)
    {
      return nullptr;
    }

    fseek(iso_file, 0, SEEK_SET);

    // Allocate buffer with enough space for reading cue
    if ((temp_buffer = (char *)calloc(size, 1)) == NULL)
      return nullptr;

    fseek(iso_file, 0, SEEK_SET);
    matched = fscanf(iso_file, "FILE \"%[^\"]\" %*s\r\n", temp_buffer);
    if (matched == EOF) {
      return nullptr;
    }

    fs::path path = filePath;
    fs::path apath = absolute(path);
    fs::path ppath = apath.parent_path();
    ppath.append(temp_buffer);

    shared_ptr<GameInfo> tmp = genGameInfoFromISo(ppath.string());
    if (tmp == nullptr) {
      free(temp_buffer);
      return tmp;
    }
    tmp->filePath = filePath;
    return tmp;

}

#include<iostream>
#include<fstream>
using namespace std;

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;



void GameInfo::downloadBoxArt() {

  string home;
  getHomeDir(home);
  string fname = this->productNumnber + ".PNG";

  // ファイルの存在確認
  std::ifstream file(home + "/BOXART/" + fname);
  if (file.is_open()) {
    std::cout << fname << " is exists" << std::endl;
    imageUrl = home + "/BOXART/" + fname;
    file.close();
    return;
  }

  if (!fs::exists(home + "/BOXART/")) {
    fs::create_directory(home + "/BOXART/");
    //std::cout << dirname << " を作成しました。" << std::endl;
  }
  else {
    //std::cout << dirname << " は既に存在します。" << std::endl;
  }


  if (siginString == "") {
    std::ifstream reading_file;
    std::string filename = "boxart.txt";
    reading_file.open(filename, std::ios::in);
    std::string reading_line_buffer;
    while (std::getline(reading_file, reading_line_buffer)) {
      siginString = reading_line_buffer;
    }
  }

  // ない場合はネットから取得する
  httplib::Client cli("https://d3edktb2n8l35b.cloudfront.net");
  string path = "/BOXART/" + fname + "?" + siginString;
  imageUrl = "";
  if (auto res = cli.Get(path.c_str())) {
    if (res->status == 200) {
      //std::cout << res->body << std::endl;
      ofstream wf(home + "/BOXART/" + fname, ios::out | ios::binary);
      wf.write((char *)res->body.data(), res->body.length());
      wf.close();
      imageUrl = home + "/BOXART/" + fname;
      std::cout << "Success!" << std::endl;
    }
    else {
      imageUrl = "missing.png";
      std::cout << "Fail: to get " << fname  << res->status << std::endl;
    }
  }
  else {
    auto err = res.error();
    imageUrl = "missing.png";
    //std::cout << err << std::endl;
    std::cout << "Fail: to get " << fname << res->status << std::endl;
  }

}


GameInfoManager::GameInfoManager() {
  
  char* err = NULL;

  string home;
  getHomeDir(home);


  home += "/games.db";

  int ret = sqlite3_open(home.c_str(), &this->db);
  if (ret != SQLITE_OK) {
    printf("FILE Open Error \n");
    return;
  }

  ret = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS Games(id INTEGER PRIMARY KEY AUTOINCREMENT,"\
    "filePath TEXT,"\
    "imageUrl TEXT,"\
    "gameTitle TEXT,"\
    "productNumnber TEXT,"\
    "makerId TEXT,"\
    "version TEXT,"\
    "releaseDate TEXT,"\
    "deviceInformation TEXT,"\
    "area TEXT,"\
    "inputDevice TEXT)" , NULL, NULL, &err);

  if (ret != SQLITE_OK) {
    printf("Execution Err \n");
    sqlite3_close(db);
    sqlite3_free(err);
    return;
  }

}


GameInfoManager::~GameInfoManager() {
  sqlite3_close(db);
}


void GameInfoManager::clearAll() {
  string sql = "delete from Games;";

  // SQL文を実行する
  char *err_msg = NULL;
  if (sqlite3_exec(db, sql.c_str(), NULL, NULL, &err_msg) != SQLITE_OK)
  {
    // SQL文の実行に失敗した場合
    fprintf(stderr, "Error: %s\n", err_msg);
    sqlite3_free(err_msg);
    return ;
  }
}

int GameInfoManager::insert(const shared_ptr<GameInfo> game ) {
  char* err = NULL;
  string sql = "insert into Games(filePath,imageUrl,gameTitle,productNumnber,makerId,version,releaseDate,deviceInformation,area,inputDevice) values(";
  sql += "'" + game->filePath + "',";
  sql += "'" + game->imageUrl + "',";
  sql += "'" + game->gameTitle + "',";
  sql += "'" + game->productNumnber + "',";
  sql += "'" + game->makerId + "',";
  sql += "'" + game->version + "',";
  sql += "'" + game->releaseDate + "',";
  sql += "'" + game->deviceInformation + "',";
  sql += "'" + game->area + "',";
  sql += "'" + game->inputDevice + "');";
  int ret = sqlite3_exec(db, sql.c_str(), NULL, NULL, &err);
  if (ret != SQLITE_OK) {
    printf("fail to insert:  %s \n", err);
    return -1;
  }
  return 0;
}

int GameInfoManager::getAll(std::vector<shared_ptr<GameInfo>> & games) {
  games.clear();
  string sql = "select * from Games;";
  // SQL文を実行する
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
  {
    // SQL文の実行に失敗した場合
    fprintf(stderr, "Error: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return 1;
  }

  // 結果を取得する
  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    shared_ptr<GameInfo> tmp(new GameInfo);
    // 列を取得する
    tmp->filePath = (const char *)sqlite3_column_text(stmt, 1);
    tmp->imageUrl = (const char *)sqlite3_column_text(stmt, 2);
    tmp->gameTitle = (const char *)sqlite3_column_text(stmt, 3);
    tmp->productNumnber = (const char *)sqlite3_column_text(stmt, 4);
    tmp->makerId = (const char *)sqlite3_column_text(stmt, 5);
    tmp->version = (const char *)sqlite3_column_text(stmt, 6);
    tmp->releaseDate = (const char *)sqlite3_column_text(stmt, 7);
    tmp->deviceInformation = (const char *)sqlite3_column_text(stmt, 8);
    tmp->area = (const char *)sqlite3_column_text(stmt, 9);
    tmp->inputDevice = (const char *)sqlite3_column_text(stmt, 10);

    games.push_back(tmp);

  }

  sqlite3_finalize(stmt);

  return 0;
}