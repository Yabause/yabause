#ifndef __STV_INCLUDE_H__
#define __STV_INCLUDE_H__

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif
int STVGetRomList(const char *dir, int force);
char* getSTVGameName(int id);
int STVGetSingle(const char *pathfile, const char *biospath, int* id);
int STVSingleInit(const char *gamepath, const char *biospath, const char *eepromdir, int stv_favorite_region);
int STVInit(int id, const char *path, const char *eepromdir, int stv_favorite_region);
int STVDeInit();
#ifdef __cplusplus
}
#endif

#define MAX_GAME_FILES 40

#define MAX_LENGTH_FILENAME 128
#define MAX_LENGTH_FILEPATH 1024

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

typedef enum{
  STV_REGION_EU = 1,
  STV_REGION_US = 2,
  STV_REGION_JP = 4,
  STV_REGION_TW = 8,
  STV_DEBUG = 16, // Not used
  STV_DEV = 32 // Not used
} biosRegion;

typedef struct FileEntry_s{
  blobType type;
  char filename[MAX_LENGTH_FILENAME];
  u32 offset;
  u32 length;
  u32 crc32;
} FileEntry;

typedef struct Game_s{
  char* romset;
  char* parent;
  char* name;
  u32 regions;
  u32 key;
  u8 rotated;
  void (*init)(void);
  const u8* eeprom;
  FileEntry blobs[MAX_GAME_FILES];
  inputType input;
} Game;

typedef struct GameLink_s{
  const Game* entry;
  char path[MAX_LENGTH_FILEPATH];
} GameLink;

typedef struct BiosFileEntry_s{
  blobType type;
  biosRegion region;
  char filename[MAX_LENGTH_FILENAME];
  u32 offset;
  u32 length;
  u32 crc32;
} BiosFileEntry;

typedef struct Bios_s{
  char* romset;
  char* name;
  BiosFileEntry blobs[MAX_GAME_FILES];
} Bios;

typedef struct BiosLink_s{
  const Bios* entry;
  char path[MAX_LENGTH_FILEPATH];
} BiosLink;

#endif
