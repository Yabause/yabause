
#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include <sys/types.h>
#include <dirent.h> 
#else
#include <windows.h>
#endif

#include "stv.h"
#include "cs0.h"
#include "junzip.h"
#include "yabause.h"
#include "zlib/zlib.h"

#define LOGSTV 
//YuiMsg
#define ROTATED 1

#define NB_STV_GAMES 95

GameLink availableGames[NB_STV_GAMES];
GameLink biosLink;
int loadGames(char* path);
int copyFile(JZFile *zip, void* data);
int copyBios(JZFile *zip, void* id);

const u8 fhBoxerNV[0x80]={
    0x53,0x45,0x47,0x41,0xff,0xff,0xff,0xff,0x68,0x5c,0xff,0xff,0x00,0x00,0x00,0x02,
    0x01,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x08,0x08,0xfc,0x00,0xca,0x01,0x56,
    0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0x68,0x5c,0xff,0xff,0x00,0x00,0x00,0x02,0x01,0x00,0x01,0x01,
    0x00,0x00,0x00,0x00,0x00,0x08,0x08,0xfc,0x00,0xca,0x01,0x56,0x00,0x00,0x00,0x00,
    0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

const u8 ShienryuNV[0x80]={
    0x53,0x45,0x47,0x41,0x3b,0xe2,0x5e,0x09,0x5e,0x09,0x00,0x00,0x00,0x00,0x00,0x02,
    0x01,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x08,0x18,0xfd,0x18,0x01,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0x5e,0x09,0x00,0x00,0x00,0x00,0x00,0x02,0x01,0x00,0x01,0x01,
    0x00,0x00,0x00,0x00,0x00,0x08,0x18,0xfd,0x18,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

const Game BiosList = 
{"STV Bios",
    0x0,
    0,
    NULL,
    {
        BIOS_BLOB, "epr-20091.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr20091.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr-19730.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr19730.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr-17951a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr17951a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr-17740a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr17740a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr-17740.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr17740.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr-17954a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr17954a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr-17952a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr17952a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr-19854.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr19854.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr-17953a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr17953a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr-17742a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "epr17742a.ic8", 0x000000, 0x080000,
        BIOS_BLOB, "stv110.bin", 0x000000, 0x080000,
        BIOS_BLOB, "stv1061.bin", 0x000000, 0x080000,
        GAME_END, "", 0, 0
    },
};
	
const Game GameList[NB_STV_GAMES]={
  {"Astra SuperStars (J 980514 V1.002)",
    0x052e2901,
    0,
    NULL,
    {
	HEADER_BLOB, "epr20825.13",    0x0000000, 0x0100000,
    GAME_BYTE_BLOB, "epr20825.13",    0x0200000, 0x0100000,
    GAME_BYTE_BLOB, "epr20825.13",    0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr20827.2",     0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20828.3",     0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20829.4",     0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20830.5",     0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20831.6",     0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20826.1",     0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20832.8",     0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20833.9",     0x2000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Baku Baku Animal (J 950407 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB,    "fpr17969.13",   0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "fpr17969.13",   0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "fpr17969.13",   0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr17970.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr17971.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr17972.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr17973.5",    0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Batman Forever (JUE 960507 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB, "350-mpa1.u19",    0x0000000, 0x0100000,
    GAME_BYTE_BLOB, "350-mpa1.u19", 0x0200000, 0x0100000,	
	HEADER_BLOB, "350-mpa1.u16", 0x0000000, 0x0100000,
    GAME_BYTE_BLOB, "350-mpa1.u16", 0x0200000, 0x0100000,
    GAME_WORD_BLOB, "gfx0.u1",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "gfx1.u3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "gfx2.u5",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "gfx3.u8",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "gfx4.u12",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "gfx5.u15",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "gfx6.u18",    0x1c00000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Choro Q Hyper Racing 5 (J 981230 V1.000)",
    0,
    0,
    NULL,
    {
	GAME_WORD_BLOB,    "ic22.bin",   0x0200000, 0x200000,
	GAME_WORD_BLOB, "ic24.bin",   0x0400000, 0x200000,
	GAME_WORD_BLOB, "ic26.bin",   0x0600000, 0x200000,
	GAME_WORD_BLOB, "ic28.bin",    0x0800000, 0x200000,
	GAME_WORD_BLOB, "ic30.bin",    0x0a00000, 0x200000,
	GAME_WORD_BLOB, "ic32.bin",    0x0c00000, 0x200000,
	GAME_WORD_BLOB, "ic34.bin",    0x0e00000, 0x200000,
	GAME_WORD_BLOB, "ic36.bin",    0x1000000, 0x200000,
        GAME_END, "", 0, 0
    },
  },
   {"Columns '97 (JET 961209 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB,    "fpr19553.13",  0x000000, 0x100000,
	GAME_BYTE_BLOB, "fpr19553.13",  0x200000, 0x100000,
	GAME_BYTE_BLOB, "fpr19553.13",  0x300000, 0x100000,
	GAME_WORD_BLOB, "mpr19554.2",   0x400000, 0x400000,
	GAME_WORD_BLOB, "mpr19555.3",   0x800000, 0x400000,
        GAME_END, "", 0, 0
    },
  }, 
  {"Cotton 2 (JUET 970902 V1.000)",
    0x0,
    0,
    NULL,
    {
	GAME_WORD_BLOB, "mpr20122.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr20117.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20118.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20119.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20120.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20121.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20116.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20123.8",    0x1c00000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Cotton Boomerang (JUET 980709 V1.000)",
    0x0,
    0,
    NULL,
    {
	GAME_WORD_BLOB, "mpr21075.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr21070.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr21071.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr21072.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr21073.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr21074.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr21069.1",    0x1800000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"DaeJeon! SanJeon SuJeon (AJTUE 990412 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB, "ic11",  0x0000000, 0x0200000,
	GAME_WORD_BLOB, "ic13",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "ic14",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic15",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "ic16",    0x0a00000, 0x0200000, 
	GAME_WORD_BLOB, "ic17",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "ic18",    0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "ic19",    0x1000000, 0x0200000,
	GAME_WORD_BLOB, "ic20",    0x1200000, 0x0200000,
	GAME_WORD_BLOB, "ic21",    0x1400000, 0x0200000,
	GAME_WORD_BLOB, "ic22",    0x1600000, 0x0200000,
	GAME_WORD_BLOB, "ic12",    0x1800000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Danchi de Hanafuda (J 990607 V1.400)",
    0,
    0,
    NULL,
    {
	GAME_WORD_BLOB, "mpr21974.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr21970.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr21971.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr21972.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr21973.5",    0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },

  {"Danchi de Quiz: Okusan Yontaku Desuyo! (J 001128 V1.200)",
    0,
    0,
    NULL,
    {
    GAME_WORD_BLOB, "ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "ic30",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "ic32",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "ic34",    0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "ic36",    0x1000000, 0x0200000,
	GAME_WORD_BLOB, "ic23",    0x1200000, 0x0200000,
	GAME_WORD_BLOB, "ic25",    0x1400000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Decathlete (JUET 960709 V1.001)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB, "epr18967a.13",    0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "epr18967a.13", 0x0200000, 0x0100000,
    GAME_BYTE_BLOB, "epr18967a.13", 0x0300000, 0x0100000,
    GAME_WORD_BLOB, "mpr18968.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18969.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18970.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18971.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr18972.6",    0x1400000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Die Hard Arcade (UET 960515 V1.000)",
    0,
    0,
    NULL,
    {
	BIOS_BLOB, "epr-17952a.ic8",   0x0000000, 0x0080000,
	HEADER_BLOB, "fpr19119.13",    0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "fpr19119.13", 0x0200000, 0x0100000,
    GAME_BYTE_BLOB, "fpr19119.13", 0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr19115.2",  0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr19116.3",  0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr19117.4",  0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr19118.5",  0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Dynamite Deka (J 960515 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB,    "fpr19114.13", 0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "fpr19114.13", 0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "fpr19114.13", 0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr19115.2",  0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr19116.3",  0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr19117.4",  0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr19118.5",  0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Ejihon Tantei Jimusyo (J 950613 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB,    "epr18137.13",  0x0000000, 0x0080000,
	HEADER_BLOB,    "epr18137.13",  0x0100000, 0x0080000,
	GAME_BYTE_BLOB, "epr18137.13",  0x0200000, 0x0080000,
	GAME_BYTE_BLOB, "epr18137.13",  0x0300000, 0x0080000,
	GAME_WORD_BLOB, "mpr18138.2",   0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18139.3",   0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18140.4",   0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18141.5",   0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr18142.6",   0x1400000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Final Fight Revenge (JUET 990714 V1.000)",
    0x0524ac01,
    0,
    NULL,
    {
	GAME_WORD_BLOB, "opr21872.7",   0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr21873.2",   0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr21874.3",   0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr21875.4",   0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr21876.5",   0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr21877.6",   0x1400000, 0x0400000,
	GAME_WORD_BLOB, "opr21878.1",   0x1800000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Final Fight Revenge (JUET 990930 V1.100)",
    0x0524ac01,
    0,
    NULL,
    {
	HEADER_BLOB,    "ffr110.ic35",  0x0000000, 0x0100000,
	GAME_WORD_BLOB, "opr21872.7",   0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr21873.2",   0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr21874.3",   0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr21875.4",   0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr21876.5",   0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr21877.6",   0x1400000, 0x0400000,
	GAME_WORD_BLOB, "opr21878.1",   0x1800000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Funky Head Boxers (JUETBKAL 951218 V1.000)",
    0x0524ac01,
    0,
    fhBoxerNV,
    {
	HEADER_BLOB,    "fr18541a.13",   0x0000000, 0x0100000,
	GAME_WORD_BLOB, "mpr18538.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr18533.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18534.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18535.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18536.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr18537.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18532.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18539.8",    0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18540.9",    0x2000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Golden Axe - The Duel (JUETL 950117 V1.000)",
   0,
   0,
   NULL,
   {
	HEADER_BLOB,   "epr17766.13",    0x0000000, 0x0080000,
	HEADER_BLOB,   "epr17766.13",    0x0100000, 0x0080000,
	GAME_BYTE_BLOB,"epr17766.13",    0x0200000, 0x0080000,
	GAME_BYTE_BLOB,"epr17766.13",    0x0300000, 0x0080000,
	GAME_WORD_BLOB, "mpr17768.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr17769.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr17770.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr17771.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr17772.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr17767.1",    0x1800000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Guardian Force (JUET 980318 V0.105)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "mpr20844.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr20839.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20840.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20841.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20842.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20843.6",    0x1400000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Groove on Fight - Gouketsuji Ichizoku 3 (J 970416 V1.001)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "mpr19820.7",    0x0200000, 0x0100000,
	GAME_WORD_BLOB, "mpr19815.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr19816.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr19817.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr19818.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr19819.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr19814.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr19821.8",    0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr19822.9",    0x2000000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Karaoke Quiz Intro Don Don! (J 960213 V1.000)",
   0,
   0,
   NULL,
   {
	HEADER_BLOB,    "epr18937.13", 0x0000000, 0x0080000,
	HEADER_BLOB,    "epr18937.13", 0x0100000, 0x0080000,
	GAME_WORD_BLOB, "mpr18944.7",  0x0200000, 0x0100000,
	GAME_WORD_BLOB, "mpr18939.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18940.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18941.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18942.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr18943.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18938.1",    0x1800000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Maru-Chan de Goo! (J 971216 V1.000)",
   0,
   0,
   NULL,
   {
	HEADER_BLOB,    "epr20416.13", 0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "epr20416.13",  0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "epr20416.13",  0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr20417.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20418.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20419.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20420.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20421.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20422.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20423.8",    0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20443.9",    0x2000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Mausuke no Ojama the World (J 960314 V1.000)",
   0,
   0,
   NULL,
   {
	HEADER_BLOB,    "ic13.bin", 0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "ic13.bin",  0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "ic13.bin",  0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mcj-00.2",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "mcj-01.3",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "mcj-02.4",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "mcj-03.5",    0x1000000, 0x0200000,
	GAME_WORD_BLOB, "mcj-04.6",    0x1400000, 0x0200000,
	GAME_WORD_BLOB, "mcj-05.1",    0x1800000, 0x0200000,
	GAME_WORD_BLOB, "mcj-06.8",    0x1c00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Microman Battle Charge (J 990326 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "ic22", 0x0200000, 0x200000,
	GAME_WORD_BLOB, "ic24", 0x0400000, 0x200000,
	GAME_WORD_BLOB, "ic26", 0x0600000, 0x200000,
	GAME_WORD_BLOB, "ic28", 0x0800000, 0x200000,
	GAME_WORD_BLOB, "ic30", 0x0a00000, 0x200000,
	GAME_WORD_BLOB, "ic32", 0x0c00000, 0x200000,
	GAME_WORD_BLOB, "ic34", 0x0e00000, 0x200000,
	GAME_WORD_BLOB, "ic36", 0x1200000, 0x200000,
        GAME_END, "", 0, 0
    },
  },
  {"Name Club (J 960315 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "ic22",  0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24",  0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic26",  0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "ic28",  0x0e00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Name Club Ver.2 (J 960315 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "nclubv2.ic22",  0x0200000, 0x0200000,
	GAME_WORD_BLOB, "nclubv2.ic24",  0x0600000, 0x0200000,
	GAME_WORD_BLOB, "nclubv2.ic26",  0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "nclubv2.ic28",  0x0e00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Name Club Ver.3 (J 970723 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "ic22",  0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24",  0x0400000, 0x0200000,
	GAME_WORD_BLOB, "ic26",  0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic28",  0x0800000, 0x0200000,
	GAME_WORD_BLOB, "ic30",  0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "ic32",  0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "ic34",  0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "ic36",  0x1000000, 0x0200000,
	GAME_WORD_BLOB, "ic23",  0x1200000, 0x0200000,
	GAME_WORD_BLOB, "ic25",  0x1400000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Othello Shiyouyo (J 980423 V1.002)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "mpr20967.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr20963.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20964.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20965.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20966.5",    0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Pebble Beach - The Great Shot (JUE 950913 V0.990)",
   0,
   0,
   NULL,
   {
	HEADER_BLOB,    "epr18852.13",   0x0000000, 0x0080000,
	HEADER_BLOB,    "epr18852.13",   0x0100000, 0x0080000,
	GAME_BYTE_BLOB, "epr18852.13",   0x0200000, 0x0080000,
	GAME_BYTE_BLOB, "epr18852.13",   0x0300000, 0x0080000,
	GAME_WORD_BLOB, "mpr18853.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18854.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18855.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18856.5",    0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Princess Clara Daisakusen (J 960910 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "mpr19337.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr19333.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr19334.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr19335.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr19336.5",    0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 (U 970921 V1.000)",
   0,
   0,
   NULL,
   {
	BIOS_BLOB, "epr-17952a.ic8",   0x0000000, 0x0080000,
	GAME_WORD_BLOB, "pclub2_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclub2_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclub2_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclub2_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclub2_ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Vol. 3 (U 990310 V1.000)",
   0,
   0,
   NULL,
   {
	BIOS_BLOB, "epr-17952a.ic8",   0x0000000, 0x0080000,
	GAME_WORD_BLOB, "pclub2v3_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclub2v3_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclub2v3_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclub2v3_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclub2v3_ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Vol. 6 Winter (J 961210 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclbvol6w_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclbvol6w_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclbvol6w_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclbvol6w_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclbvol6w_ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Vol. 6 Winter (J 970121 V1.200)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "ic22.bin",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24.bin",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic26.bin",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic28.bin",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic30.bin",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Vol. 7 Spring (J 970313 V1.100)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclub2v7.ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclub2v7.ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclub2v7.ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclub2v7.ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclub2v7.ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Vol. 8 Summer (J 970616 V1.100)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "u22.bin",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "u24.bin",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "u26.bin",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "u28.bin",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "u30.bin",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "u32.bin",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "u34.bin",    0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "u36.bin",    0x1000000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Vol. 9 Autumn (J V1.100)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "ic22.bin",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24.bin",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "ic26.bin",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic28.bin",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "ic30.bin",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "ic32.bin",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "ic34.bin",    0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "ic36.bin",    0x1000000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 '97 Winter Ver (J 971017 V1.100, set 1)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "prc297wi_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic30",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic32",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic34",    0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic36",    0x1000000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic23",    0x1200000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic25",    0x1400000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic27",    0x1600000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 '97 Winter Ver (J 971017 V1.100, set 2)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclb297w_ic22_alt",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic30",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic32",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic34",    0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic36",    0x1000000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic23",    0x1200000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic25",    0x1400000, 0x0200000,
	GAME_WORD_BLOB, "prc297wi_ic27",    0x1600000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 '98 Spring Ver (J 971017 V1.100)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "prc298sp_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "prc298sp_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "prc298sp_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "prc298sp_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "prc298sp_ic30",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "prc298sp_ic32",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "prc298sp_ic34",    0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "prc298sp_ic36",    0x1000000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 '98 Autumn Ver (J 980827 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "prc298au_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "prc298au_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "prc298au_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "prc298au_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "prc298au_ic30",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "prc298au_ic32",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "prc298au_ic34",    0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "prc298au_ic36",    0x1000000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 '98 Summer Ver (J 980603 V1.100)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclb298s_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclb298s_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclb298s_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclb298s_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclb298s_ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 2000 Summer (J 000509 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "ic22.bin",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24.bin",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "ic26.bin",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic28.bin",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "ic30.bin",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Earth Limited Kobe (Print Club Custom) (J 970808 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "ic22.bin",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24.bin",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "ic26.bin",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic28.bin",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "ic30.bin",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "ic32.bin",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "ic34.bin",    0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "ic36.bin",    0x1000000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Felix The Cat (Rev. A) (J 970415 V1.100)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclub2fc_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclub2fc_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclub2fc_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclub2fc_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclub2fc_ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 (U 970921 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclub2kc_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclub2kc_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclub2kc_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclub2kc_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclub2kc_ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
   {"Print Club LoveLove (J 970421 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclblove.ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclblove.ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclblove.ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclblove.ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclblove.ic30",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "pclblove.ic32",    0x0c00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
   {"Print Club LoveLove Ver 2 (J 970825 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "ic30",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "ic32",    0x0c00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
   {"Print Club Olive (J 980717 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclubol_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclubol_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclubol_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclubol_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclubol_ic30",    0x0a00000, 0x0200000,
	GAME_WORD_BLOB, "pclubol_ic32",    0x0c00000, 0x0200000,
	GAME_WORD_BLOB, "pclubol_ic34",    0x0e00000, 0x0200000,
	GAME_WORD_BLOB, "pclubol_ic36",    0x1000000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Pepsiman (J 970618 V1.100)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclb2psi_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclb2psi_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclb2psi_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclb2psi_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclb2psi_ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club Pokemon B (U 991126 V1.000)",
   0,
   0,
   NULL,
   {
	BIOS_BLOB, "epr-17952a.ic8",   0x0000000, 0x0080000,
	GAME_WORD_BLOB, "pclubpok_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclubpok_ic22",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclubpok_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclubpok_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclubpok_ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Puffy (J V1.100)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclb2puf.ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclb2puf.ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclb2puf.ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclb2puf.ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclb2puf.ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
   },
  },
   {"Print Club 2 Warner Bros (J 970228 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclb2wb_ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclb2wb_ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclb2wb_ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclb2wb_ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclb2wb_ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
   {"Print Club Winnie-the-Pooh Vol. 2 (J 971218 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "ic22.bin",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24.bin",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "ic26.bin",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic28.bin",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "ic30.bin",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
   {"Print Club Winnie-the-Pooh Vol. 3 (J 980406 V1.000)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "ic22.bin",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24.bin",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "ic26.bin",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "ic28.bin",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "ic30.bin",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
   {"Print Club Yoshimoto V2 (J 970422 V1.100)",
   0,
   0,
   NULL,
   {
	GAME_WORD_BLOB, "pclbyov2.ic22",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "pclbyov2.ic24",    0x0400000, 0x0200000,
	GAME_WORD_BLOB, "pclbyov2.ic26",    0x0600000, 0x0200000,
	GAME_WORD_BLOB, "pclbyov2.ic28",    0x0800000, 0x0200000,
	GAME_WORD_BLOB, "pclbyov2.ic30",    0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Print Club 2 Vol. 6 Winter (J 961210 V1.000)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "pclbvol6w_ic22",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "pclbvol6w_ic24",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "pclbvol6w_ic26",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "pclbvol6w_ic28",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "pclbvol6w_ic30",    0x0a00000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 Vol. 6 Winter (J 970121 V1.200)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "ic22.bin",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "ic24.bin",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "ic26.bin",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "ic28.bin",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "ic30.bin",    0x0a00000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 Vol. 7 Spring (J 970313 V1.100)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "pclub2v7.ic22",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2v7.ic24",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2v7.ic26",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2v7.ic28",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2v7.ic30",    0x0a00000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 Vol. 8 Summer (J 970616 V1.100)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "u22.bin",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "u24.bin",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "u26.bin",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "u28.bin",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "u30.bin",    0x0a00000, 0x0200000,
 	GAME_WORD_BLOB, "u32.bin",    0x0c00000, 0x0200000,
 	GAME_WORD_BLOB, "u34.bin",    0x0e00000, 0x0200000,
 	GAME_WORD_BLOB, "u36.bin",    0x1000000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 Vol. 9 Autumn (J V1.100)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "ic22.bin",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "ic24.bin",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "ic26.bin",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "ic28.bin",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "ic30.bin",    0x0a00000, 0x0200000,
 	GAME_WORD_BLOB, "ic32.bin",    0x0c00000, 0x0200000,
 	GAME_WORD_BLOB, "ic34.bin",    0x0e00000, 0x0200000,
 	GAME_WORD_BLOB, "ic36.bin",    0x1000000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 '97 Winter Ver (J 971017 V1.100, set 1)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "prc297wi_ic22",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic24",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic26",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic28",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic30",    0x0a00000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic32",    0x0c00000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic34",    0x0e00000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic36",    0x1000000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic23",    0x1200000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic25",    0x1400000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic27",    0x1600000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 '97 Winter Ver (J 971017 V1.100, set 2)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "pclb297w_ic22_alt",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic24",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic26",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic28",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic30",    0x0a00000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic32",    0x0c00000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic34",    0x0e00000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic36",    0x1000000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic23",    0x1200000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic25",    0x1400000, 0x0200000,
 	GAME_WORD_BLOB, "prc297wi_ic27",    0x1600000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 '98 Spring Ver (J 971017 V1.100)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "prc298sp_ic22",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "prc298sp_ic24",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "prc298sp_ic26",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "prc298sp_ic28",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "prc298sp_ic30",    0x0a00000, 0x0200000,
 	GAME_WORD_BLOB, "prc298sp_ic32",    0x0c00000, 0x0200000,
 	GAME_WORD_BLOB, "prc298sp_ic34",    0x0e00000, 0x0200000,
 	GAME_WORD_BLOB, "prc298sp_ic36",    0x1000000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 '98 Autumn Ver (J 980827 V1.000)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "prc298au_ic22",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "prc298au_ic24",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "prc298au_ic26",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "prc298au_ic28",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "prc298au_ic30",    0x0a00000, 0x0200000,
 	GAME_WORD_BLOB, "prc298au_ic32",    0x0c00000, 0x0200000,
 	GAME_WORD_BLOB, "prc298au_ic34",    0x0e00000, 0x0200000,
 	GAME_WORD_BLOB, "prc298au_ic36",    0x1000000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 '98 Summer Ver (J 980603 V1.100)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "pclb298s_ic22",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "pclb298s_ic24",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "pclb298s_ic26",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "pclb298s_ic28",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "pclb298s_ic30",    0x0a00000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 2000 Summer (J 000509 V1.000)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "ic22.bin",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "ic24.bin",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "ic26.bin",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "ic28.bin",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "ic30.bin",    0x0a00000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club Kome Kome Club (J 970203 V1.000)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "pclub2kc_ic22",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2kc_ic24",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2kc_ic26",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2kc_ic28",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2kc_ic30",    0x0a00000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 Felix The Cat (Rev. A) (J 970415 V1.100)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "pclub2fc_ic22",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2fc_ic24",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2fc_ic26",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2fc_ic28",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "pclub2fc_ic30",    0x0a00000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
   {"Print Club 2 Puffy (J V1.100)",
    0,
    0,
    NULL,
    {
 	GAME_WORD_BLOB, "pclb2puf.ic22",    0x0200000, 0x0200000,
 	GAME_WORD_BLOB, "pclb2puf.ic24",    0x0400000, 0x0200000,
 	GAME_WORD_BLOB, "pclb2puf.ic26",    0x0600000, 0x0200000,
 	GAME_WORD_BLOB, "pclb2puf.ic28",    0x0800000, 0x0200000,
 	GAME_WORD_BLOB, "pclb2puf.ic30",    0x0a00000, 0x0200000,
         GAME_END, "", 0, 0
     },
   },
  {"Pro Mahjong Kiwame S (J 951020 V1.208)",
   0,
   0,
   NULL,
   {
	HEADER_BLOB,   "epr18737.13",    0x0000000, 0x0080000,
	GAME_BYTE_BLOB, "epr18737.13",    0x0100000, 0x0080000,
	GAME_WORD_BLOB, "mpr18738.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18739.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18740.4",    0x0c00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Puyo Puyo Sun (J 961115 V0.001)",
   0,
   0,
   NULL,
   {
	HEADER_BLOB, "epr19531.13",    0x0000000, 0x0080000,
	HEADER_BLOB, "epr19531.13",    0x0100000, 0x0080000,
	GAME_BYTE_BLOB, "epr19531.13", 0x0200000, 0x0080000,
	GAME_BYTE_BLOB, "epr19531.13", 0x0300000, 0x0080000,
	GAME_WORD_BLOB, "mpr19533.2",  0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr19534.3",  0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr19535.4",  0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr19536.5",  0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr19537.6",  0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr19532.1",  0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr19538.8",  0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr19539.9",  0x2000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Puzzle & Action: Sando-R (J 951114 V1.000)",
   0,
   0,
   NULL,
   {
        HEADER_BLOB,    "sando-r.13",  0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "sando-r.13",  0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "sando-r.13",  0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr18635.8",  0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18635.8",  0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18636.9",  0x2000000, 0x0400000,
	GAME_WORD_BLOB, "mpr18636.9",  0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18637.10", 0x2400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18637.10", 0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18638.11", 0x2800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18638.11", 0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Puzzle & Action: Treasure Hunt (JUET 970901 V2.00E)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB, "th-ic7_2.stv",    0x0200000, 0x0080000,
	HEADER_BLOB, "th-ic7_1.stv",    0x0200001, 0x0080000,
	GAME_WORD_BLOB, "th-e-2.ic2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "th-e-3.ic3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "th-e-4.ic4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "th-e-5.ic5",    0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Radiant Silvergun (JUET 980523 V1.000)",
    0x05272d01,
    0,
    NULL,
    {
	GAME_WORD_BLOB, "mpr20958.7",   0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr20959.2",   0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20960.3",   0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20961.4",   0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20962.5",   0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Sakura Taisen - Hanagumi Taisen Columns (J 971007 V1.010)",
    0,
    0,
    NULL,
    {
	GAME_WORD_BLOB, "mpr20143.7",    0x0200000, 0x0100000,
	GAME_WORD_BLOB, "mpr20138.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20139.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20140.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20141.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20142.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20137.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20144.8",    0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20145.9",    0x2000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20146.10",   0x2400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20147.11",   0x2800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20148.12",   0x2c00000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Sea Bass Fishing (JUET 971110 V0.001)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB, "seabassf.13",    0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "seabassf.13",    0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "seabassf.13",    0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr20551.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20552.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20553.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20554.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20555.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20550.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20556.8",   0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20557.9",   0x2000000, 0x0400000,

        GAME_END, "", 0, 0
    },
  },
  {"Shanghai - The Great Wall / Shanghai Triple Threat (JUE 950623 V1.005)",
    0,
    0,
    NULL,
    {
	GAME_WORD_BLOB, "mpr18341.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr18341.7",    0x0400000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Shienryu (JUET 961226 V1.000)",
    0,
    ROTATED,
    ShienryuNV,
    {
	GAME_WORD_BLOB, "mpr19631.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr19632.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr19633.3",    0x0800000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Soukyugurentai / Terra Diver (JUET 960821 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB,    "fpr19188.13",  0x0100000, 0x0100000,
	GAME_BYTE_BLOB, "fpr19188.13",  0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "fpr19188.13",   0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr19189.2",   0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr19190.3",   0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr19191.4",   0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr19192.5",   0x1000000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Steep Slope Sliders (JUET 981110 V1.000)",
    0x052b6901,
    0,
    NULL,
    {
	HEADER_BLOB,    "epr21488.13",  0x0000000, 0x0080000,
	HEADER_BLOB,    "epr21488.13",  0x0100000, 0x0080000,
	GAME_BYTE_BLOB, "epr21488.13",  0x0200000, 0x0080000,
	GAME_BYTE_BLOB, "epr21488.13",   0x0300000, 0x0080000,
	GAME_WORD_BLOB, "mpr21489.2",   0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr21490.3",   0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr21491.4",   0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr21492.5",   0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr21493.6",   0x1400000, 0x0400000,	
        GAME_END, "", 0, 0
    },
  },
  {"Stress Busters (J 981020 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB,    "epr-21300a.ic13",  0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "epr-21300a.ic13",  0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "epr-21300a.ic13",  0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr-21290.ic2",   0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr-21291.ic3",   0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr-21292.ic4",   0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr-21293.ic5",   0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr-21294.ic6",   0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr-21289.ic1",   0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr-21296.ic8",   0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr-21297.ic9",   0x2000000, 0x0400000,
	GAME_WORD_BLOB, "mpr-21298.ic10",  0x2400000, 0x0400000,
	GAME_WORD_BLOB, "mpr-21299.ic11",  0x2800000, 0x0400000,		
        GAME_END, "", 0, 0
    },
  },
  {"Suiko Enbu / Outlaws of the Lost Dynasty (JUETL 950314 V2.001)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB,    "fpr17834.13",  0x0100000, 0x0100000,
	GAME_BYTE_BLOB, "fpr17834.13",  0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "fpr17834.13",  0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr17836.2",   0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr17837.3",   0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr17838.4",   0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr17839.5",   0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr17840.6",   0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr17835.1",   0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr17841.8",   0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr17842.9",   0x2000000, 0x0400000,	
        GAME_END, "", 0, 0
    },
  },
  {"Super Major League (U 960108 V1.000)",
    0,
    0,
    NULL,
    {
	BIOS_BLOB, "epr-17952a.ic8",   0x0000000, 0x0080000,
	HEADER_BLOB,    "epr18777.13",  0x0000000, 0x0080000,
	HEADER_BLOB,    "epr18777.13",  0x0100000, 0x0080000,
	GAME_BYTE_BLOB, "epr18777.13",  0x0200000, 0x0080000,
	GAME_BYTE_BLOB, "epr18777.13",   0x0300000, 0x0080000,
	GAME_WORD_BLOB, "mpr18778.8",   0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18779.9",   0x2000000, 0x0400000,
	GAME_WORD_BLOB, "mpr18780.10",   0x2400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18781.11",   0x2800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18782.12",   0x2c00000, 0x0200000,	
        GAME_END, "", 0, 0
    },
  },
  {"Taisen Tanto-R Sashissu!! (J 980216 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB, "epr20542.13",    0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "epr20542.13",   0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "epr20542.13",   0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr20544.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20545.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20546.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20547.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20548.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20543.1",    0x1800000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Technical Bowling (J 971212 V1.000)",
    0x05200913,
    0,
    NULL,
    {
	GAME_WORD_BLOB, "ic22",   0x0200000, 0x0200000,
	GAME_WORD_BLOB, "ic24",   0x0300000, 0x0200000,
	GAME_WORD_BLOB, "ic26",   0x0400000, 0x0200000,
	GAME_WORD_BLOB, "ic28",   0x0800000, 0x0200000,
	GAME_WORD_BLOB, "ic30",   0x0a00000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Tecmo World Cup '98 (JUET 980410 V1.000)",
    0x05200913,
    0,
    NULL,
    {
	HEADER_BLOB, "epr20819.24",    0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "epr20819.24",   0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "epr20819.24",   0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr20821.12",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20822.13",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20823.14",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20824.15",    0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Tecmo World Soccer '98 (JUET 980410 V1.000)",
    0x05200913,
    0,
    NULL,
    {
	HEADER_BLOB, "epr-20820.ic24",    0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "epr-20820.ic24",   0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "epr-20820.ic24",   0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr20821.12",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20822.13",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20823.14",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20824.15",    0x1000000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Touryuu Densetsu Elan-Doree / Elan Doree - Legend of Dragoon (JUET 980922 V1.006)",
    0x05226d41,
    0,
    NULL,
    {
	GAME_WORD_BLOB, "mpr21307.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr21301.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr21302.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr21303.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr21304.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr21305.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr21306.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr21308.8",    0x1c00000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Virtua Fighter Kids (JUET 960319 V0.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB, "fpr18914.13",    0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "fpr18914.13",    0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "fpr18914.13",    0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr18916.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18917.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr18918.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18915.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18919.8",    0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr18920.9",    0x2000000, 0x0400000,
	GAME_WORD_BLOB, "mpr18921.10",    0x2400000, 0x0400000,
	GAME_WORD_BLOB, "mpr18922.11",    0x2800000, 0x0400000,
	GAME_WORD_BLOB, "mpr18923.12",    0x2c00000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Virtua Fighter Remix (JUETBKAL 950428 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB, "epr17944.13",    0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "epr17944.13",    0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "epr17944.13",    0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr17946.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr17947.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr17948.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr17949.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr17950.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr17945.1",    0x1800000, 0x0200000,
        GAME_END, "", 0, 0
    },
  },
  {"Virtual Mahjong 2 - My Fair Lady (J 980608 V1.000)",
    0x05226d41,
    0,
    NULL,
    {
	GAME_WORD_BLOB, "mpr21000.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr20995.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20996.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20997.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20998.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20999.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20994.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr21001.8",    0x1c00000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Winter Heat (JUET 971012 V1.000)",
    0x05226d41,
    0,
    NULL,
    {
	HEADER_BLOB, "fpr20108.13",    0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "fpr20108.13",    0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "fpr20108.13",    0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr20110.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20111.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20112.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20113.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20114.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20109.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20115.8",    0x1c00000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Zen Nippon Pro-Wrestling Featuring Virtua (J 971123 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB,   "epr20398.13",    0x0000000, 0x0100000,
	GAME_BYTE_BLOB, "epr20398.13",   0x0200000, 0x0100000,
	GAME_BYTE_BLOB, "epr20398.13",   0x0300000, 0x0100000,
	GAME_WORD_BLOB, "mpr20400.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20401.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20402.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20403.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20404.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20399.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20405.8",    0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20406.9",    0x2000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20407.10",   0x2400000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
  {"Zenkoku Seifuku Bishoujo Grand Prix Find Love (J 971212 V1.000)",
    0,
    0,
    NULL,
    {
	HEADER_BLOB,   "epr20424.13",    0x0000000, 0x0100000,
	GAME_WORD_BLOB, "mpr20431.7",    0x0200000, 0x0200000,
	GAME_WORD_BLOB, "mpr20426.2",    0x0400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20427.3",    0x0800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20428.4",    0x0c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20429.5",    0x1000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20430.6",    0x1400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20425.1",    0x1800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20432.8",    0x1c00000, 0x0400000,
	GAME_WORD_BLOB, "mpr20433.9",    0x2000000, 0x0400000,
	GAME_WORD_BLOB, "mpr20434.10",   0x2400000, 0x0400000,
	GAME_WORD_BLOB, "mpr20435.11",   0x2800000, 0x0400000,
	GAME_WORD_BLOB, "mpr20436.12",   0x2c00000, 0x0400000,
        GAME_END, "", 0, 0
    },
  },
};


u8 hasBios = 0;
u8 fileFound[NB_STV_GAMES][MAX_GAME_FILES];
u8 biosFound[MAX_GAME_FILES];

typedef struct {
    char* filename;
    int gameId;
    int bios;
} rominfo;

int processBios(JZFile *zip,void *input) {
    JZFileHeader header;
    char filename[1024];
    int j;
    rominfo* info = (rominfo*) input;

   if ((info != NULL) && (info->gameId != -1)) return copyBios(zip, input);


    if(jzReadLocalFileHeader(zip, &header, filename, sizeof(filename))) {
        LOGSTV("Couldn't read local file header!\n");
        return -1;
    }

    //LOGSTV("%s, %d / %d bytes at offset %08X\n", filename, header.compressedSize, header.uncompressedSize, header.offset);
    j=0;
    while(BiosList.blobs[j].type != GAME_END) {
      if (strncmp(BiosList.blobs[j].filename, filename, 1024) == 0) {
        //Compatible file found
        biosFound[j] = 1;
      }
      j++;
    }

    return 0;
}

int processFile(JZFile *zip,void *input) {
    JZFileHeader header;
    char filename[1024];
    int i,j;
    rominfo* info = (rominfo*) input;

    if (info != NULL)
		if (info->gameId != -1)  return copyFile(zip, input);

    if(jzReadLocalFileHeader(zip, &header, filename, sizeof(filename))) {
        LOGSTV("Couldn't read local file header!\n");
        return -1;
    }

    //LOGSTV("%s, %d / %d bytes at offset %08X\n", filename, header.compressedSize, header.uncompressedSize, header.offset);
    for (i=0; i<NB_STV_GAMES; i++) {
      j=0;
      while(GameList[i].blobs[j].type != GAME_END) {
        if (strncmp(GameList[i].blobs[j].filename, filename, 1024) == 0) {
          //Compatible file found
          fileFound[i][j] = 1;
        }
        j++;
      }
    }

    return 0;
}
int biosloaded = 0xFF;

int copyBios(JZFile *zip, void* id) {
    JZFileHeader header;
    char filename[1024];
    u8* data;
    unsigned int i,j, dataAvailable;
    rominfo* info = (rominfo*)id;
    int gameId = -1;
    if (info != NULL) gameId = info->gameId;

    if(jzReadLocalFileHeader(zip, &header, filename, sizeof(filename))) {
        LOGSTV("Couldn't read local file header!\n");
        return -1;
    }

    if((data = (u8*)malloc(header.uncompressedSize)) == NULL) {
        LOGSTV("Couldn't allocate memory!\n");
        return -1;
    }

    LOGSTV("copyBios %s\n", filename);

    i=0;
    dataAvailable = 0;
    while(biosLink.entry->blobs[i].type != GAME_END) {
      if (strncmp(biosLink.entry->blobs[i].filename, filename, 1024) == 0) {
        if (dataAvailable == 0) {
          dataAvailable = (jzReadData(zip, &header, data) == Z_OK);
        }
        if(dataAvailable != 0) {
          if (info->bios == 1) {
            biosFound[i] = 1;
          } else {
            fileFound[gameId][i] = 1;
          }
          switch (biosLink.entry->blobs[i].type) {
            case BIOS_BLOB:
              if (biosloaded > i) {
                LOGSTV("Load bios %s\n", filename);
                for (j=0; j<biosLink.entry->blobs[i].length;j++) {
                  T1WriteByte(BiosRom, biosLink.entry->blobs[i].offset+j, data[j]);
                }
                biosloaded = i;
              }
              break;
          }
        }
      }
      i++;
    }

    free(data);

    return 0;
}

int copyFile(JZFile *zip, void* id) {
    JZFileHeader header;
    char filename[1024];
    u8* data;
    int i,j, dataAvailable;
	rominfo* info = (rominfo*)id;
	int gameId = -1;
	if (info != NULL) gameId = info->gameId;

    if(jzReadLocalFileHeader(zip, &header, filename, sizeof(filename))) {
        LOGSTV("Couldn't read local file header!\n");
        return -1;
    }

    if((data = (u8*)malloc(header.uncompressedSize)) == NULL) {
        LOGSTV("Couldn't allocate memory!\n");
        return -1;
    }

    i=0;
    dataAvailable = 0;
    while(availableGames[gameId].entry->blobs[i].type != GAME_END) {
      if (strncmp(availableGames[gameId].entry->blobs[i].filename, filename, 1024) == 0) {
        LOGSTV("copyFile %s\n", filename);
        if (dataAvailable == 0) {
          dataAvailable = (jzReadData(zip, &header, data) == Z_OK);
        }
        if(dataAvailable != 0) {
          fileFound[gameId][i] = 1;
          switch (availableGames[gameId].entry->blobs[i].type) {
            case BIOS_BLOB:
              if (biosloaded > i) {
                LOGSTV("Load bios %s\n", filename);
                for (j=0; j<availableGames[gameId].entry->blobs[i].length;j++) {
                  T1WriteByte(BiosRom, availableGames[gameId].entry->blobs[i].offset+j, data[j]);
                }
                biosloaded = i;
              }
              break;
            case GAME_WORD_BLOB:
              for (j=0; j<availableGames[gameId].entry->blobs[i].length;j++) {
                T2WriteByte(CartridgeArea->rom, availableGames[gameId].entry->blobs[i].offset+j, data[j]);
              }
              break;
            case HEADER_BLOB:
              for (j=0; j<availableGames[gameId].entry->blobs[i].length;j++) {
                T1WriteByte(CartridgeArea->rom, availableGames[gameId].entry->blobs[i].offset+(2*j+1), data[j]);
                T1WriteByte(CartridgeArea->rom, availableGames[gameId].entry->blobs[i].offset+(2*j), 0);
              }
              break;
            case GAME_BYTE_BLOB:
              for (j=0; j<availableGames[gameId].entry->blobs[i].length;j++) {
                T1WriteByte(CartridgeArea->rom, availableGames[gameId].entry->blobs[i].offset+j, data[j]);
              }
              break;
          }
        } else LOGSTV("Error : No data read from %s\n", filename);
      }
      i++;
    }

    free(data);

    return 0;
}

int recordCallback(JZFile *zip, int idx, JZFileHeader *header, char *filename, void *user_data) {
    long offset;
    rominfo* info = (rominfo*)user_data;
    offset = zip->tell(zip); // store current position

    if(zip->seek(zip, header->offset, SEEK_SET)) {
        LOGSTV("Cannot seek in zip file!\n");
        return 0; // abort
    }

    LOGSTV("%s\n", filename);
    char *last = strrchr(info->filename, '/');
    if (last != NULL) {
      if (strcmp(last+1, "stvbios.zip") == 0) {
        processBios(zip, user_data); // alters file offset
      } else {
        processFile(zip, user_data); // alters file offset
      }
    } else {
      processFile(zip, user_data); // alters file offset
    }

    zip->seek(zip, offset, SEEK_SET); // return to position

    return 1; // continue
}

void updateGameList(const char* file, int *nbGames){
  FILE *fp;
  JZEndRecord endRecord;
  JZFile *zip;
  int i, j;
  u8 isASTVGame, isBiosFound, isBlobFound;
  rominfo info;
  info.filename = file;
  info.gameId = -1;
  info.bios = 0;

  memset(fileFound, 0x0, NB_STV_GAMES*MAX_GAME_FILES);

  if(!(fp = fopen(file, "rb"))) {
        LOGSTV("Couldn't open \"%s\"!\n", file);
        return;
  }

  zip = jzfile_from_stdio_file(fp);

  if(jzReadEndRecord(zip, &endRecord)) {
    LOGSTV("Couldn't read ZIP file end record.\n");
    goto endClose;
  }

  if(jzReadCentralDirectory(zip, &endRecord, recordCallback, &info)) {
    LOGSTV("Couldn't read ZIP file central record.\n");
    goto endClose;
  }

  j=0;
  if (!hasBios) {
    while(BiosList.blobs[j].type != GAME_END) {
      if (BiosList.blobs[j].type == BIOS_BLOB) {
        hasBios |= biosFound[j];
        biosLink.entry = &BiosList;
        strncpy(biosLink.path, file, 1024);
        break;
      }
      j++;
    }
  }
  for (i=0; i<NB_STV_GAMES; i++) {
    isASTVGame = 1;
    isBiosFound = 0;
    isBlobFound = 1;
    j=0;
    while(GameList[i].blobs[j].type != GAME_END) {
      if (GameList[i].blobs[j].type == BIOS_BLOB) {
        isBiosFound |= fileFound[i][j];
      } else {
        isBlobFound &= fileFound[i][j];
      }
      j++;
    }
    isASTVGame = isBlobFound & (isBiosFound | hasBios);
    if (isASTVGame == 1) {
      //Add the filename as a Game
      int found = 0;
      for (j=0; j<NB_STV_GAMES; j++) {
        if (availableGames[j].entry == &GameList[i]) {
          found = 1;
          break;
        }
      }
      if (found == 0) {
        availableGames[*nbGames].entry = &GameList[i];
        strncpy(availableGames[*nbGames].path, file, 1024);
        (*nbGames)++;
        break;
      }
    }
  }

endClose:
    zip->close(zip);

}

int loadBios(){
  FILE *fp;
  JZEndRecord endRecord;
  JZFile *zip;
  int i = 0;
  u8 isBiosFound = 0;
  rominfo info;
  info.filename = biosLink.path;
  info.gameId = 0;
  info.bios = 1;
  memset(fileFound, 0x0, NB_STV_GAMES*MAX_GAME_FILES);
  if(!(fp = fopen(biosLink.path, "rb"))) {
        LOGSTV("Couldn't open bios\"%s\"!\n", biosLink.path);
        return 0;
  }

  zip = jzfile_from_stdio_file(fp);

  if(jzReadEndRecord(zip, &endRecord)) {
    LOGSTV("Couldn't read ZIP file end record.\n");
  } else {
    if(jzReadCentralDirectory(zip, &endRecord, recordCallback, &info)) {
      LOGSTV("Couldn't read ZIP file central record.\n");
    }
  }
  zip->close(zip);
  while(biosLink.entry->blobs[i].type != GAME_END) {
    if (biosLink.entry->blobs[i].type == BIOS_BLOB) {
      isBiosFound |= biosFound[i];
    } 
    i++;
  }
  return isBiosFound;
}

int loadGame(int gameId){
  FILE *fp;
  JZEndRecord endRecord;
  JZFile *zip;
  int i = 0;
  u8 isBiosFound = 0;
  u8 isBlobFound = 1;
  u8 hasBios = 0;
  rominfo info;
  LOGSTV("loadGame");

  info.filename = availableGames[gameId].path;
  info.gameId = gameId;
  info.bios = 0;
  hasBios = loadBios();
  biosloaded = 0xFF;

  LOGSTV("Loading game[%d] %s from %s\n", gameId, availableGames[gameId].entry->name, availableGames[gameId].path);
  memset(fileFound, 0x0, NB_STV_GAMES*MAX_GAME_FILES);

  if(!(fp = fopen(availableGames[gameId].path, "rb"))) {
        LOGSTV("Couldn't open \"%s\"!\n", availableGames[gameId].path);
        return -1;
  }

  zip = jzfile_from_stdio_file(fp);

  if(jzReadEndRecord(zip, &endRecord)) {
    LOGSTV("Couldn't read ZIP file end record.\n");
  } else {
    if(jzReadCentralDirectory(zip, &endRecord, recordCallback, &info)) {
      LOGSTV("Couldn't read ZIP file central record.\n");
    }
  }
  zip->close(zip);

  while(availableGames[gameId].entry->blobs[i].type != GAME_END) {
    if (availableGames[gameId].entry->blobs[i].type == BIOS_BLOB) {
      isBiosFound |= fileFound[gameId][i];
    } else {
      isBlobFound &= fileFound[gameId][i];
    }
    i++;
  }
  if (isBlobFound & (isBiosFound|hasBios)) {
    LOGSTV("%s has been sucessfully loaded\n", availableGames[gameId].entry->name);
    eeprom_start(availableGames[gameId].entry->eeprom);
    cyptoSetKey(availableGames[gameId].entry->key);
    yabsys.isRotated = availableGames[gameId].entry->rotated;
    return 0;
  }
  return -1;
}

#ifndef WIN32
int STVGetRomList(const char* path, int force){
//List number of files in directory
  DIR *d;
  FILE *fp;
  int i, nbGames = 0;
  char savefile[1024];
  memset(availableGames, 0x0, sizeof(GameLink)*NB_STV_GAMES);
  snprintf(savefile, 1024, "%s/gamelist.save", path);
  if (force == 0) {
    nbGames = loadGames(savefile);
    if (nbGames != 0) return nbGames;
  }
  struct dirent *dir;
  d = opendir(path);
  if (d) {
    //Force a detection of the bios first
    unsigned int len = strlen(path)+strlen("/")+strlen("stvbios.zip")+1;
    unsigned char *file = malloc(len);
    snprintf(file, len, "%s/stvbios.zip",path);
    updateGameList(file, &nbGames);
    free(file);
    while ((dir = readdir(d)) != NULL) {
      if (dir->d_type == DT_REG)
      {
        char *dot = strrchr(dir->d_name, '.');
        if (dot && !strcmp(dot, ".zip")) {
          len = strlen(path)+strlen("/")+strlen(dir->d_name)+1;
          file = malloc(len);
          snprintf(file, len, "%s/%s",path, dir->d_name);
          updateGameList(file, &nbGames);
          free(file);
        }
      }
    }
    closedir(d);
    fp = fopen(savefile, "w");
    if (biosLink.entry != NULL) {
      fprintf(fp, "%s,%s\n", biosLink.entry->name, biosLink.path);
    }
    for (i=0; i<nbGames; i++) {
      fprintf(fp, "%s,%s\n", availableGames[i].entry->name, availableGames[i].path);
    }
    fclose(fp);
  }
  return nbGames;
}
#else
int STVGetRomList(const char* path, int force){
//List number of files in directory
  FILE *fp;
  int i, nbGames = 0;
  char savefile[1024];
  char pathfile[1024];
  memset(availableGames, 0x0, sizeof(GameLink)*NB_STV_GAMES);
  snprintf(savefile, 1024, "%s/gamelist.save", path);
  snprintf(pathfile, 1024, "%s/*.zip", path);
  if (force == 0) {
    nbGames = loadGames(savefile);
    if (nbGames != 0) return nbGames;
  }
  HANDLE hFind;
  WIN32_FIND_DATAA FindFileData;
  //Force a detection of the bios first
  unsigned int len = strlen(path) + strlen("/") + strlen("stvbios.zip") + 1;
  unsigned char *file = malloc(len);
  snprintf(file, len, "%s/stvbios.zip", path);
  updateGameList(file, &nbGames);
  free(file);
  if((hFind = FindFirstFileA(pathfile, &FindFileData)) != INVALID_HANDLE_VALUE){
    do{
      unsigned int len = strlen(path)+strlen("/")+strlen(FindFileData.cFileName)+1;
      unsigned char *file = malloc(len);
      snprintf(file, len, "%s/%s",path, FindFileData.cFileName);
	  LOGSTV(file);
      updateGameList(file, &nbGames);
      free(file);
    }while(FindNextFileA(hFind, &FindFileData));
    FindClose(hFind);
    fp = fopen(savefile, "w");
	if (biosLink.entry != NULL) {
		fprintf(fp, "%s,%s\n", biosLink.entry->name, biosLink.path);
	}
    for (i=0; i<nbGames; i++) {
      fprintf(fp, "%s,%s\n", availableGames[i].entry->name, availableGames[i].path);
    }
    fclose(fp);
  }
  return nbGames;
}
#endif

char* getSTVGameName(int id) {
  return availableGames[id].entry->name;
}

int loadGames(char* path) {
  int i, nbGames = 0;
  char* field;
  char gameName[1024];
  char gamePath[1024];
  FILE *fp;
  fp = fopen(path, "r");
  if (fp == NULL) return 0;
  for(;;) {
    i=0;
    field = &gameName[0];
    while((field[i] = fgetc(fp)) != EOF) {
      if (field[i] == '\n') {
        field[i] = '\0';
        break;
      }
      else {
        if (field[i] == ',') {
          field[i] = '\0';
          field = &gamePath[0];
          i = 0;
        } else {
          i++;
        }
      }
    }
    if (field[i] == EOF) break;
    LOGSTV("Scan new game %s %s!!!\n", gameName, gamePath);
    if (strncmp(gameName,BiosList.name,1024)==0) {
      biosLink.entry = &BiosList;
      strncpy(biosLink.path, gamePath, 1024);
    } else {
      for (i=0; i<NB_STV_GAMES; i++) {
        if (strncmp(gameName,GameList[i].name,1024)==0) {
          availableGames[nbGames].entry = &GameList[i];
          strncpy(availableGames[nbGames].path, gamePath, 1024);
          LOGSTV("Rebuild %s from %s\n", gameName, gamePath);
          nbGames++;
          break;
        }
      }
    }
  }
  fclose(fp);
  return nbGames;
}

int STVInit(int id, char* path){
  cryptoReset();
  if (CartridgeArea->carttype != CART_ROMSTV) return 0;
  int nbGames = STVGetRomList(path, 0);
  if (nbGames == 0) return -1;
  if (nbGames <= id) return -1;
  if (loadGame(id) == 0) {
    yabsys.isSTV = 1;
    return 0;
  }
  return -1;
}
