
#include <string>
#include <cstdint>
#include "bios.h"

using std::string;


class BackupManager {

public:  
  BackupManager();
  ~BackupManager();

  int getDevicelist( string & jsonstr );
  int getFilelist( int deviceid, string & jsonstr );
  int deletefile( int index );
  int getFile( int index, string & jsonstr );
  int putFile( const string & jsonstr );

protected:  
  uint32_t currentbupdevice_ = 0;
  deviceinfo_struct* devices_ = NULL;
  int32_t numbupdevices_ = 0;
  saveinfo_struct* saves_ = NULL;
  int32_t numsaves_ = 0;

};
