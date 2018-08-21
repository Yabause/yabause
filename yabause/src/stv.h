#ifndef __STV_INCLUDE_H__
#define __STV_INCLUDE_H__

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif
int STVGetRomList(const char* dir, int force);
char* getSTVGameName(int id);
int STVInit(int id, char* path);
#ifdef __cplusplus
}
#endif

#define MAX_GAME_FILES 40

typedef enum{
  BIOS_BLOB,
  HEADER_BLOB,
  GAME_BYTE_BLOB,
  GAME_WORD_BLOB,
  GAME_END
} blobType;

typedef struct FileEntry_s{
  blobType type;
  char* filename;
  u32 offset;
  u32 length;
} FileEntry;

typedef struct Game_s{
 char* name;
 u32 key;
 u8 rotated;
 u8* eeprom;
 FileEntry blobs[MAX_GAME_FILES];
} Game;

typedef struct GameLink_s{
  Game* entry;
  char path[1024];
} GameLink;

#endif
