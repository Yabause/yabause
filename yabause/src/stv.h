#ifndef __STV_INCLUDE_H__
#define __STV_INCLUDE_H__

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif
int STVGetRomList(const char *dir, int force);
char* getSTVGameName(int id);
int STVGetSingle(const char *pathfile, const char *biospath, int* id);
int STVSingleInit(const char *gamepath, const char *biospath, const char *eepromdir);
int STVInit(int id, const char *path);
int STVDeInit();
#ifdef __cplusplus
}
#endif

#define MAX_GAME_FILES 40

// Only STV & STV6B are properly hooked at the moment, the others might require tweaks
typedef enum{
  STV,
  STV6B,
  BATMANFR,
  CRITCRSH,
  STVMP,
  MICROMBC,
  MYFAIRLD,
  PATOCAR,
  VMAHJONG
} inputType;

typedef enum{
  BIOS_BLOB,
  HEADER_BLOB,
  GAME_BYTE_BLOB,
  GAME_WORD_BLOB,
  EEPROM_BLOB,
  GAME_END
} blobType;

typedef struct FileEntry_s{
  blobType type;
  char* filename;
  u32 offset;
  u32 length;
} FileEntry;

typedef struct Game_s{
  char* romset;
  char* parent;
  char* name;
  u32 key;
  u8 rotated;
  void (*init)(void);
  const u8* eeprom;
  FileEntry blobs[MAX_GAME_FILES];
  inputType input;
} Game;

typedef struct GameLink_s{
  const Game* entry;
  char path[1024];
} GameLink;

#endif
