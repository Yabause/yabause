#ifndef ERROR_H
#define ERROR_H

#define YAB_ERR_UNKNOWN                 0
#define YAB_ERR_FILENOTFOUND            1
#define YAB_ERR_MEMORYALLOC             2
#define YAB_ERR_FILEREAD                3
#define YAB_ERR_FILEWRITE               4
#define YAB_ERR_CANNOTINIT              5

#define YAB_ERR_SH2INVALIDOPCODE        6
#define YAB_ERR_SH2READ                 7
#define YAB_ERR_SH2WRITE                8

#define YAB_ERR_SDL                     9

void YabSetError(int type, void *extra);
#endif
