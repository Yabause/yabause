#include "Preference.h"
#include <iostream>
#include <fstream>
#include <sstream>

#define LOG printf

Preference::Preference( const std::string & filename ){

  std::string input_trace_filename;

  int index = filename.find_last_of("/\\");
  if( index != string::npos ){
    input_trace_filename = filename.substr(index+1);
  }else{
    input_trace_filename = filename;
  }

  std::string home_dir = getenv("HOME");
  home_dir += "/.yabasanshiro/";

  this->filename = home_dir + input_trace_filename + ".config";

  LOG("Preference: opening %s\n", this->filename.c_str() );

  try{
    std::ifstream fin( this->filename );
    fin >> j;
    fin.close();
  }catch ( json::exception& e ){
  }
} 

Preference::~Preference(){
  std::ofstream out(this->filename);
  out << j.dump(2);
  out.close();  
}   

int Preference::getInt( const std::string & key , int defaultv = 0  ){
  try {
    LOG("Preference: getInt %s:%d\n", key.c_str() ,j[key].get<int>() );
    return j[key].get<int>();
  }catch ( json::type_error & e ){
  }

  LOG("Preference: fail to getInt %s\n", key.c_str());
  return defaultv;
}

bool Preference::getBool( const std::string & key , bool defaultv = false ){
  try {
    LOG("Preference: getBool %s:%d\n", key.c_str(),j[key].get<bool>() );
    return j[key].get<bool>();
  }catch ( json::type_error & e ){
  }

  LOG("Preference: fail to getBool %s\n", key.c_str());
  return defaultv;
}

void Preference::setInt( const std::string & key , int value ){
  j[key] = value;
  std::ofstream out(this->filename);
  out << j.dump(2);
  out.close();  
}

void Preference::setBool( const std::string & key , bool value ){
  j[key] = value;
  std::ofstream out(this->filename);
  out << j.dump(2);
  out.close();     
}
