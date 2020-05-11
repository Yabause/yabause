
#include <sys/stat.h>

#include "cs0.h"
#include "cs2.h"
#include "db.h"

#define NB_GAMES_DB 31

static GameDB GameDBList[NB_GAMES_DB] = {
   { "T-3105G", 0, CART_DRAM8MBIT, NULL }, // Real Bout Garou Densetsu
   { "T-3119G", 0, CART_DRAM8MBIT, NULL }, // Real Bout Garou Densetsu Special
   { "T-3116G", 0, CART_DRAM8MBIT, NULL }, // Samurai Spirits - Amakusa Kourin
   { "T-3104G", 0, CART_DRAM8MBIT, NULL }, // Samurai Spirits - Zankurou Musouken
   { "T-3108G", 0, CART_DRAM8MBIT, NULL }, // The King of Fighters '96
   { "T-3121G", 0, CART_DRAM8MBIT, NULL }, // The King of Fighters '97
   { "T-1515G", 0, CART_DRAM8MBIT, NULL }, // Waku Waku 7
   { "T-1521G", 0, CART_DRAM32MBIT, NULL }, // Astra Superstars
   { "T-9904G", 0, CART_DRAM32MBIT, NULL }, // Magical Night Dreams - Cotton 2
   { "T-1217G", 0, CART_DRAM32MBIT, NULL }, // Cyberbots - Fullmetal Madness
   { "T-1245G", 0, CART_DRAM32MBIT, NULL }, // Dungeons & Dragons Collection - Shadow over Mystara
   { "GS-9107", 0, CART_DRAM32MBIT, NULL }, // Fighter's History Dynamite
   { "T-1248G", 0, CART_DRAM32MBIT, NULL }, // Final Fight Revenge
   { "T-20109G", 0, CART_DRAM32MBIT, NULL }, // Friends: Seishun no Kagayaki
   { "T-14411G", 0, CART_DRAM32MBIT, NULL }, // Groove On Fight: Gouketsuji Ichizoku 3
   { "T-7032H-50VV1.000", 0, CART_DRAM32MBIT, NULL }, // Marvel Super Heroes
   { "T-1214H", 0, CART_DRAM32MBIT, NULL }, // Marvel Super Heroes
   { "T-1521G", 0, CART_DRAM32MBIT, NULL }, // Marvel Super Heroes
   { "T-1238G", 0, CART_DRAM32MBIT, NULL }, // Marvel Super Heroes vs. Street Fighter
   { "T-3111G", 0, CART_DRAM32MBIT, NULL }, // Metal Slug
   { "T-22205G", 0, CART_DRAM32MBIT, NULL }, // Noël 3
   { "T-20114G", 0, CART_DRAM32MBIT, NULL }, // Pia Carrot e Youkoso!! 2
   { "T-1230G", 0, CART_DRAM32MBIT, NULL }, // Pocket Fighter
   { "T-1246G", 0, CART_DRAM32MBIT, NULL }, // Street Fighter Alpha 3
   { "T-16510G", 0, CART_DRAM32MBIT, NULL }, // Super Real Mahjong P7
   { "T-1229G", 0, CART_DRAM32MBIT, NULL }, // Vampire Savior
   { "T-1226G", 0, CART_DRAM32MBIT, NULL }, // X-Men vs. Street Fighter
   // the following rom filenames are based on MAME, themselves based on stickers/labels on the cart's chip
   { "MK-81088", 0, CART_ROM16MBIT, "mpr-18811-mx.ic1" }, // The King of Fighters '95
   { "T-3101G", 0, CART_ROM16MBIT, "mpr-18811-mx.ic1" }, // The King of Fighters '95
   { "T-13308G", 0, CART_ROM16MBIT, "mpr-19367-mx.ic1" }, // Ultraman: Hikari no Kyojin Densetsu
   // this game is a prototype without game_code
   { NULL, 0x3939393939393939, CART_DRAM128MBIT, NULL } // Heart of Darkness
};

static int does_file_exist(const char *filename)
{
   struct stat st;
   int result = stat(filename, &st);
   return result == 0;
}

void DBLookup(int* const cart_type, const char** cart_path, const char * support_dir)
{
#ifdef _WIN32
   char slash = '\\';
#else
   char slash = '/';
#endif
   Cs2GetIP(1);
   const char* game_code = Cs2GetCurrentGmaecode();
   const u64 game_id = Cs2GetGameId();
   for (int i=0; i<NB_GAMES_DB; i++) {
      // Heart of Darkness has no game_code, but i lack information on game_id's uniqueness, so a false positive is not impossible
      if ((GameDBList[i].game_code != NULL && game_code != NULL && strcmp(GameDBList[i].game_code, game_code) == 0)
      || (GameDBList[i].game_id != 0 && game_id != 0 && GameDBList[i].game_id == game_id)) {
         if (GameDBList[i].filename != NULL && support_dir != NULL) {
            char rompath[512];
            snprintf(rompath, sizeof(rompath), "%s%c%s", support_dir, slash, GameDBList[i].filename);
            if (does_file_exist(rompath)) *cart_path = strdup(rompath);
         }
         *cart_type = GameDBList[i].cart_type;
         return;
      }
   }
}
