#ifndef LIBRETRO_CORE_OPTIONS_INTL_H__
#define LIBRETRO_CORE_OPTIONS_INTL_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1500 && _MSC_VER < 1900)
/* https://support.microsoft.com/en-us/kb/980263 */
#pragma execution_character_set("utf-8")
#pragma warning(disable:4566)
#endif

#include <libretro.h>

/*
 ********************************
 * VERSION: 2.0
 ********************************
 *
 * - 2.0: Add support for core options v2 interface
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/
/* RETRO_LANGUAGE_AR */

#define YABAUSE_FRAMESKIP_LABEL_AR NULL
#define YABAUSE_FRAMESKIP_INFO_0_AR "تمكين frameskip، سيساعد في الأداء."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_AR NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_AR NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_AR NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_AR NULL
#define OPTION_VAL_NONE_AR "لاشيء"
#define OPTION_VAL_1M_RAM_AR NULL
#define OPTION_VAL_4M_RAM_AR NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_AR NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_AR "تمكين النقر المتعدد في المنفذ 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_AR NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_AR "تمكين النقر المتعدد في المنفذ 2"
#define YABAUSE_NUMTHREADS_LABEL_AR NULL
#define YABAUSE_NUMTHREADS_INFO_0_AR NULL

struct retro_core_option_v2_category option_cats_ar[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ar[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_AR,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_AR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_AR,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_AR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_AR,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_AR,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_AR },
         { "1M_ram", OPTION_VAL_1M_RAM_AR },
         { "4M_ram", OPTION_VAL_4M_RAM_AR },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_AR,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_AR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_AR,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_AR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_AR,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_AR,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ar = {
   option_cats_ar,
   option_defs_ar
};

/* RETRO_LANGUAGE_AST */

#define YABAUSE_FRAMESKIP_LABEL_AST NULL
#define YABAUSE_FRAMESKIP_INFO_0_AST "Activa'l saltu de cuadros, habría ayudar al rindimientu."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_AST NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_AST NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_AST NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_AST NULL
#define OPTION_VAL_NONE_AST NULL
#define OPTION_VAL_1M_RAM_AST NULL
#define OPTION_VAL_4M_RAM_AST NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_AST NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_AST NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_AST NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_AST NULL
#define YABAUSE_NUMTHREADS_LABEL_AST NULL
#define YABAUSE_NUMTHREADS_INFO_0_AST NULL

struct retro_core_option_v2_category option_cats_ast[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ast[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_AST,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_AST,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_AST,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_AST,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_AST,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_AST,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_AST },
         { "1M_ram", OPTION_VAL_1M_RAM_AST },
         { "4M_ram", OPTION_VAL_4M_RAM_AST },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_AST,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_AST,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_AST,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_AST,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_AST,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_AST,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ast = {
   option_cats_ast,
   option_defs_ast
};

/* RETRO_LANGUAGE_CA */

#define YABAUSE_FRAMESKIP_LABEL_CA NULL
#define YABAUSE_FRAMESKIP_INFO_0_CA NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_CA NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_CA NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_CA NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_CA NULL
#define OPTION_VAL_NONE_CA NULL
#define OPTION_VAL_1M_RAM_CA NULL
#define OPTION_VAL_4M_RAM_CA NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_CA NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_CA NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_CA NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_CA NULL
#define YABAUSE_NUMTHREADS_LABEL_CA NULL
#define YABAUSE_NUMTHREADS_INFO_0_CA NULL

struct retro_core_option_v2_category option_cats_ca[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ca[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_CA,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_CA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_CA,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_CA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_CA,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_CA,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_CA },
         { "1M_ram", OPTION_VAL_1M_RAM_CA },
         { "4M_ram", OPTION_VAL_4M_RAM_CA },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_CA,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_CA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_CA,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_CA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_CA,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_CA,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ca = {
   option_cats_ca,
   option_defs_ca
};

/* RETRO_LANGUAGE_CHS */

#define YABAUSE_FRAMESKIP_LABEL_CHS "跳帧"
#define YABAUSE_FRAMESKIP_INFO_0_CHS "启用跳帧功能，此选项应能提升性能。"
#define YABAUSE_FORCE_HLE_BIOS_LABEL_CHS NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_CHS NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_CHS NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_CHS NULL
#define OPTION_VAL_NONE_CHS "无"
#define OPTION_VAL_1M_RAM_CHS NULL
#define OPTION_VAL_4M_RAM_CHS NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_CHS NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_CHS NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_CHS NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_CHS NULL
#define YABAUSE_NUMTHREADS_LABEL_CHS NULL
#define YABAUSE_NUMTHREADS_INFO_0_CHS NULL

struct retro_core_option_v2_category option_cats_chs[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_chs[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_CHS,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_CHS,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_CHS,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_CHS,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_CHS,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_CHS,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_CHS },
         { "1M_ram", OPTION_VAL_1M_RAM_CHS },
         { "4M_ram", OPTION_VAL_4M_RAM_CHS },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_CHS,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_CHS,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_CHS,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_CHS,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_CHS,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_CHS,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_chs = {
   option_cats_chs,
   option_defs_chs
};

/* RETRO_LANGUAGE_CHT */

#define YABAUSE_FRAMESKIP_LABEL_CHT NULL
#define YABAUSE_FRAMESKIP_INFO_0_CHT NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_CHT NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_CHT NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_CHT NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_CHT NULL
#define OPTION_VAL_NONE_CHT "無"
#define OPTION_VAL_1M_RAM_CHT NULL
#define OPTION_VAL_4M_RAM_CHT NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_CHT NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_CHT NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_CHT NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_CHT NULL
#define YABAUSE_NUMTHREADS_LABEL_CHT NULL
#define YABAUSE_NUMTHREADS_INFO_0_CHT NULL

struct retro_core_option_v2_category option_cats_cht[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_cht[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_CHT,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_CHT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_CHT,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_CHT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_CHT,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_CHT,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_CHT },
         { "1M_ram", OPTION_VAL_1M_RAM_CHT },
         { "4M_ram", OPTION_VAL_4M_RAM_CHT },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_CHT,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_CHT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_CHT,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_CHT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_CHT,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_CHT,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_cht = {
   option_cats_cht,
   option_defs_cht
};

/* RETRO_LANGUAGE_CS */

#define YABAUSE_FRAMESKIP_LABEL_CS NULL
#define YABAUSE_FRAMESKIP_INFO_0_CS NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_CS NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_CS NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_CS NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_CS NULL
#define OPTION_VAL_NONE_CS NULL
#define OPTION_VAL_1M_RAM_CS NULL
#define OPTION_VAL_4M_RAM_CS NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_CS NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_CS NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_CS NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_CS NULL
#define YABAUSE_NUMTHREADS_LABEL_CS NULL
#define YABAUSE_NUMTHREADS_INFO_0_CS NULL

struct retro_core_option_v2_category option_cats_cs[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_cs[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_CS,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_CS,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_CS,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_CS,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_CS,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_CS,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_CS },
         { "1M_ram", OPTION_VAL_1M_RAM_CS },
         { "4M_ram", OPTION_VAL_4M_RAM_CS },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_CS,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_CS,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_CS,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_CS,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_CS,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_CS,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_cs = {
   option_cats_cs,
   option_defs_cs
};

/* RETRO_LANGUAGE_CY */

#define YABAUSE_FRAMESKIP_LABEL_CY NULL
#define YABAUSE_FRAMESKIP_INFO_0_CY "Galluogi frameskip, dylai helpu gyda pherfformiad."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_CY NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_CY NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_CY NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_CY NULL
#define OPTION_VAL_NONE_CY "dim"
#define OPTION_VAL_1M_RAM_CY NULL
#define OPTION_VAL_4M_RAM_CY NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_CY NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_CY "Galluogi multitap ym mhorthladd 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_CY NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_CY "Galluogi multitap ym mhorthladd 2"
#define YABAUSE_NUMTHREADS_LABEL_CY NULL
#define YABAUSE_NUMTHREADS_INFO_0_CY NULL

struct retro_core_option_v2_category option_cats_cy[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_cy[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_CY,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_CY,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_CY,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_CY,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_CY,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_CY,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_CY },
         { "1M_ram", OPTION_VAL_1M_RAM_CY },
         { "4M_ram", OPTION_VAL_4M_RAM_CY },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_CY,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_CY,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_CY,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_CY,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_CY,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_CY,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_cy = {
   option_cats_cy,
   option_defs_cy
};

/* RETRO_LANGUAGE_DA */

#define YABAUSE_FRAMESKIP_LABEL_DA NULL
#define YABAUSE_FRAMESKIP_INFO_0_DA "Aktivér frameskip, det bør hjælpe på ydelsen."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_DA NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_DA NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_DA NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_DA NULL
#define OPTION_VAL_NONE_DA NULL
#define OPTION_VAL_1M_RAM_DA NULL
#define OPTION_VAL_4M_RAM_DA NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_DA NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_DA "Aktivér multitap i port 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_DA NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_DA "Aktivér multitap i port 2"
#define YABAUSE_NUMTHREADS_LABEL_DA NULL
#define YABAUSE_NUMTHREADS_INFO_0_DA NULL

struct retro_core_option_v2_category option_cats_da[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_da[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_DA,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_DA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_DA,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_DA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_DA,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_DA,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_DA },
         { "1M_ram", OPTION_VAL_1M_RAM_DA },
         { "4M_ram", OPTION_VAL_4M_RAM_DA },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_DA,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_DA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_DA,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_DA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_DA,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_DA,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_da = {
   option_cats_da,
   option_defs_da
};

/* RETRO_LANGUAGE_DE */

#define YABAUSE_FRAMESKIP_LABEL_DE NULL
#define YABAUSE_FRAMESKIP_INFO_0_DE "Frameskip aktivieren, sollte die Leistung verbessern."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_DE NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_DE NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_DE NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_DE NULL
#define OPTION_VAL_NONE_DE "keine"
#define OPTION_VAL_1M_RAM_DE NULL
#define OPTION_VAL_4M_RAM_DE NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_DE NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_DE "Aktiviere Multitap in Port 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_DE NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_DE "Aktiviere Multitap in Port 2"
#define YABAUSE_NUMTHREADS_LABEL_DE NULL
#define YABAUSE_NUMTHREADS_INFO_0_DE NULL

struct retro_core_option_v2_category option_cats_de[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_de[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_DE,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_DE,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_DE,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_DE,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_DE,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_DE,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_DE },
         { "1M_ram", OPTION_VAL_1M_RAM_DE },
         { "4M_ram", OPTION_VAL_4M_RAM_DE },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_DE,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_DE,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_DE,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_DE,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_DE,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_DE,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_de = {
   option_cats_de,
   option_defs_de
};

/* RETRO_LANGUAGE_EL */

#define YABAUSE_FRAMESKIP_LABEL_EL NULL
#define YABAUSE_FRAMESKIP_INFO_0_EL "Ενεργοποίηση frameskip, θα πρέπει να βοηθήσει με τις επιδόσεις."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_EL NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_EL NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_EL NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_EL NULL
#define OPTION_VAL_NONE_EL "κανείς"
#define OPTION_VAL_1M_RAM_EL NULL
#define OPTION_VAL_4M_RAM_EL NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_EL NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_EL "Ενεργοποίηση κιτ πολλαπλών παιχτών στη θύρα 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_EL NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_EL "Ενεργοποίηση κιτ πολλαπλών παιχτών στη θύρα 2"
#define YABAUSE_NUMTHREADS_LABEL_EL NULL
#define YABAUSE_NUMTHREADS_INFO_0_EL NULL

struct retro_core_option_v2_category option_cats_el[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_el[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_EL,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_EL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_EL,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_EL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_EL,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_EL,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_EL },
         { "1M_ram", OPTION_VAL_1M_RAM_EL },
         { "4M_ram", OPTION_VAL_4M_RAM_EL },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_EL,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_EL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_EL,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_EL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_EL,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_EL,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_el = {
   option_cats_el,
   option_defs_el
};

/* RETRO_LANGUAGE_EO */

#define YABAUSE_FRAMESKIP_LABEL_EO NULL
#define YABAUSE_FRAMESKIP_INFO_0_EO NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_EO NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_EO NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_EO NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_EO NULL
#define OPTION_VAL_NONE_EO NULL
#define OPTION_VAL_1M_RAM_EO NULL
#define OPTION_VAL_4M_RAM_EO NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_EO NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_EO NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_EO NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_EO NULL
#define YABAUSE_NUMTHREADS_LABEL_EO NULL
#define YABAUSE_NUMTHREADS_INFO_0_EO NULL

struct retro_core_option_v2_category option_cats_eo[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_eo[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_EO,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_EO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_EO,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_EO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_EO,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_EO,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_EO },
         { "1M_ram", OPTION_VAL_1M_RAM_EO },
         { "4M_ram", OPTION_VAL_4M_RAM_EO },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_EO,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_EO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_EO,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_EO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_EO,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_EO,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_eo = {
   option_cats_eo,
   option_defs_eo
};

/* RETRO_LANGUAGE_ES */

#define YABAUSE_FRAMESKIP_LABEL_ES "Omisión de fotogramas"
#define YABAUSE_FRAMESKIP_INFO_0_ES "Activa la omisión de fotogramas. Debería mejorar el rendimiento."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_ES "Forzar BIOS HLE (es necesario reiniciar)"
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_ES "Utiliza esta opción por tu cuenta y riesgo, ya que produce más problemas de los que resuelve."
#define YABAUSE_ADDON_CARTRIDGE_LABEL_ES "Cartucho de expansión (es necesario reiniciar)"
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_ES "Selecciona el cartucho de expansión."
#define OPTION_VAL_NONE_ES "Ninguno"
#define OPTION_VAL_1M_RAM_ES "RAM de 1 MB"
#define OPTION_VAL_4M_RAM_ES "RAM de 4 MB"
#define YABAUSE_MULTITAP_PORT1_LABEL_ES "Adaptador para seis jugadores en el puerto 1"
#define YABAUSE_MULTITAP_PORT1_INFO_0_ES "Activa un multitap en el puerto 1."
#define YABAUSE_MULTITAP_PORT2_LABEL_ES "Adaptador para seis jugadores en el puerto 2"
#define YABAUSE_MULTITAP_PORT2_INFO_0_ES "Activa un multitap en el puerto 2."
#define YABAUSE_NUMTHREADS_LABEL_ES "Número de hilos (es necesario reiniciar)"
#define YABAUSE_NUMTHREADS_INFO_0_ES "Este valor puede mejorar el rendimiento en función de tu CPU."

struct retro_core_option_v2_category option_cats_es[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_es[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_ES,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_ES,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_ES,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_ES,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_ES,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_ES,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_ES },
         { "1M_ram", OPTION_VAL_1M_RAM_ES },
         { "4M_ram", OPTION_VAL_4M_RAM_ES },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_ES,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_ES,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_ES,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_ES,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_ES,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_ES,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_es = {
   option_cats_es,
   option_defs_es
};

/* RETRO_LANGUAGE_FA */

#define YABAUSE_FRAMESKIP_LABEL_FA NULL
#define YABAUSE_FRAMESKIP_INFO_0_FA "پرش فریم را فعال کنید، باید به عملکرد کمک کند."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_FA NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_FA NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_FA NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_FA NULL
#define OPTION_VAL_NONE_FA NULL
#define OPTION_VAL_1M_RAM_FA NULL
#define OPTION_VAL_4M_RAM_FA NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_FA NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_FA NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_FA NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_FA NULL
#define YABAUSE_NUMTHREADS_LABEL_FA NULL
#define YABAUSE_NUMTHREADS_INFO_0_FA NULL

struct retro_core_option_v2_category option_cats_fa[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_fa[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_FA,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_FA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_FA,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_FA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_FA,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_FA,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_FA },
         { "1M_ram", OPTION_VAL_1M_RAM_FA },
         { "4M_ram", OPTION_VAL_4M_RAM_FA },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_FA,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_FA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_FA,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_FA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_FA,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_FA,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_fa = {
   option_cats_fa,
   option_defs_fa
};

/* RETRO_LANGUAGE_FI */

#define YABAUSE_FRAMESKIP_LABEL_FI "Kuvanohitus"
#define YABAUSE_FRAMESKIP_INFO_0_FI "Ota käyttöön kuvan ohitus, jonka pitäisi auttaa suorituskyvyssä."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_FI "Pakota HLE BIOS (Uudelleenkäynnistys vaaditaan)"
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_FI "Käytä omalla riskilläsi, tämä aiheuttaa enemmän ongelmia kuin ratkaisee niitä."
#define YABAUSE_ADDON_CARTRIDGE_LABEL_FI "Lisäosa kasetti (Uudelleenkäynnistys vaaditaan)"
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_FI "Valitse lisäosa kasetti."
#define OPTION_VAL_NONE_FI "ei mitään"
#define OPTION_VAL_1M_RAM_FI NULL
#define OPTION_VAL_4M_RAM_FI NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_FI "6 pelajaan adapteri 1 portissa"
#define YABAUSE_MULTITAP_PORT1_INFO_0_FI "Ota \"multitap\" käyttöön 1 portissa"
#define YABAUSE_MULTITAP_PORT2_LABEL_FI "6 pelajaan adapteri 2 portissa"
#define YABAUSE_MULTITAP_PORT2_INFO_0_FI "Ota \"multitap\" käyttöön 2 portissa"
#define YABAUSE_NUMTHREADS_LABEL_FI "Säikeiden määrä (Uudelleenkäynnistys vaaditaan)"
#define YABAUSE_NUMTHREADS_INFO_0_FI "Riippuen prosessoristasi, tämän arvon muuttaminen voi auttaa suorituskyvyssä."

struct retro_core_option_v2_category option_cats_fi[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_fi[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_FI,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_FI,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_FI,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_FI,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_FI,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_FI,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_FI },
         { "1M_ram", OPTION_VAL_1M_RAM_FI },
         { "4M_ram", OPTION_VAL_4M_RAM_FI },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_FI,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_FI,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_FI,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_FI,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_FI,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_FI,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_fi = {
   option_cats_fi,
   option_defs_fi
};

/* RETRO_LANGUAGE_FR */

#define YABAUSE_FRAMESKIP_LABEL_FR "Saut d'images"
#define YABAUSE_FRAMESKIP_INFO_0_FR "Activer le saut d'images, cela devrait améliorer les performances."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_FR "Forcer le BIOS HLE (Redémarrage requis)"
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_FR "Utiliser à vos risques et périls, cette option cause plus de problèmes qu'elle n'en résout."
#define YABAUSE_ADDON_CARTRIDGE_LABEL_FR "Cartouche de mémoire supplémentaire (Redémarrage requis)"
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_FR "Sélectionner la cartouche de mémoire supplémentaire."
#define OPTION_VAL_NONE_FR "aucune"
#define OPTION_VAL_1M_RAM_FR NULL
#define OPTION_VAL_4M_RAM_FR NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_FR "Adaptateur 6 joueurs sur le port 1"
#define YABAUSE_MULTITAP_PORT1_INFO_0_FR "Active le multitap dans le port 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_FR "Adaptateur 6 joueurs sur le port 2"
#define YABAUSE_MULTITAP_PORT2_INFO_0_FR "Active le multitap dans le port 2"
#define YABAUSE_NUMTHREADS_LABEL_FR "Nombre de fils d'exécution (Redémarrage requis)"
#define YABAUSE_NUMTHREADS_INFO_0_FR "Selon votre processeur, changer cette valeur peut améliorer les performances."

struct retro_core_option_v2_category option_cats_fr[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_fr[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_FR,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_FR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_FR,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_FR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_FR,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_FR,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_FR },
         { "1M_ram", OPTION_VAL_1M_RAM_FR },
         { "4M_ram", OPTION_VAL_4M_RAM_FR },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_FR,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_FR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_FR,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_FR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_FR,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_FR,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_fr = {
   option_cats_fr,
   option_defs_fr
};

/* RETRO_LANGUAGE_GL */

#define YABAUSE_FRAMESKIP_LABEL_GL NULL
#define YABAUSE_FRAMESKIP_INFO_0_GL NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_GL NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_GL NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_GL NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_GL NULL
#define OPTION_VAL_NONE_GL "ningún"
#define OPTION_VAL_1M_RAM_GL NULL
#define OPTION_VAL_4M_RAM_GL NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_GL NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_GL NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_GL NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_GL NULL
#define YABAUSE_NUMTHREADS_LABEL_GL NULL
#define YABAUSE_NUMTHREADS_INFO_0_GL NULL

struct retro_core_option_v2_category option_cats_gl[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_gl[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_GL,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_GL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_GL,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_GL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_GL,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_GL,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_GL },
         { "1M_ram", OPTION_VAL_1M_RAM_GL },
         { "4M_ram", OPTION_VAL_4M_RAM_GL },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_GL,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_GL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_GL,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_GL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_GL,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_GL,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_gl = {
   option_cats_gl,
   option_defs_gl
};

/* RETRO_LANGUAGE_HE */

#define YABAUSE_FRAMESKIP_LABEL_HE NULL
#define YABAUSE_FRAMESKIP_INFO_0_HE NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_HE NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_HE NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_HE NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_HE NULL
#define OPTION_VAL_NONE_HE NULL
#define OPTION_VAL_1M_RAM_HE NULL
#define OPTION_VAL_4M_RAM_HE NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_HE NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_HE NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_HE NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_HE NULL
#define YABAUSE_NUMTHREADS_LABEL_HE NULL
#define YABAUSE_NUMTHREADS_INFO_0_HE NULL

struct retro_core_option_v2_category option_cats_he[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_he[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_HE,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_HE,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_HE,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_HE,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_HE,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_HE,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_HE },
         { "1M_ram", OPTION_VAL_1M_RAM_HE },
         { "4M_ram", OPTION_VAL_4M_RAM_HE },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_HE,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_HE,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_HE,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_HE,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_HE,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_HE,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_he = {
   option_cats_he,
   option_defs_he
};

/* RETRO_LANGUAGE_HU */

#define YABAUSE_FRAMESKIP_LABEL_HU NULL
#define YABAUSE_FRAMESKIP_INFO_0_HU NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_HU NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_HU NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_HU NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_HU NULL
#define OPTION_VAL_NONE_HU NULL
#define OPTION_VAL_1M_RAM_HU NULL
#define OPTION_VAL_4M_RAM_HU NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_HU NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_HU NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_HU NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_HU NULL
#define YABAUSE_NUMTHREADS_LABEL_HU NULL
#define YABAUSE_NUMTHREADS_INFO_0_HU NULL

struct retro_core_option_v2_category option_cats_hu[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_hu[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_HU,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_HU,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_HU,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_HU,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_HU,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_HU,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_HU },
         { "1M_ram", OPTION_VAL_1M_RAM_HU },
         { "4M_ram", OPTION_VAL_4M_RAM_HU },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_HU,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_HU,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_HU,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_HU,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_HU,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_HU,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_hu = {
   option_cats_hu,
   option_defs_hu
};

/* RETRO_LANGUAGE_ID */

#define YABAUSE_FRAMESKIP_LABEL_ID NULL
#define YABAUSE_FRAMESKIP_INFO_0_ID "Aktifkan frameskip agar dapat membantu performa nya."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_ID NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_ID NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_ID NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_ID NULL
#define OPTION_VAL_NONE_ID NULL
#define OPTION_VAL_1M_RAM_ID NULL
#define OPTION_VAL_4M_RAM_ID NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_ID NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_ID "Aktifkan fitur multitap di port 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_ID NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_ID "Aktifkan fitur multitap di port 2"
#define YABAUSE_NUMTHREADS_LABEL_ID NULL
#define YABAUSE_NUMTHREADS_INFO_0_ID NULL

struct retro_core_option_v2_category option_cats_id[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_id[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_ID,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_ID,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_ID,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_ID,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_ID,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_ID,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_ID },
         { "1M_ram", OPTION_VAL_1M_RAM_ID },
         { "4M_ram", OPTION_VAL_4M_RAM_ID },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_ID,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_ID,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_ID,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_ID,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_ID,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_ID,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_id = {
   option_cats_id,
   option_defs_id
};

/* RETRO_LANGUAGE_IT */

#define YABAUSE_FRAMESKIP_LABEL_IT "Salta fotogramma"
#define YABAUSE_FRAMESKIP_INFO_0_IT "Abilita frameskip, dovrebbe aiutare con le prestazioni."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_IT "Forza HLE BIOS (Riavvio Richiesto)"
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_IT "Utilizzare a proprio rischio, sta causando più problemi di quanto risolva."
#define YABAUSE_ADDON_CARTRIDGE_LABEL_IT "Cartuccia Componente (Riavvio Richiesto)"
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_IT "Selezionare la cartuccia aggiuntiva."
#define OPTION_VAL_NONE_IT "nessuno"
#define OPTION_VAL_1M_RAM_IT NULL
#define OPTION_VAL_4M_RAM_IT NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_IT "Adattatore 6Player sulla porta 1"
#define YABAUSE_MULTITAP_PORT1_INFO_0_IT "Abilita multitap nella porta 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_IT "Adattatore 6Player sulla porta 2"
#define YABAUSE_MULTITAP_PORT2_INFO_0_IT "Abilita multitap nella porta 2"
#define YABAUSE_NUMTHREADS_LABEL_IT "Numero di thread (Riavvio Richiesto)"
#define YABAUSE_NUMTHREADS_INFO_0_IT "A seconda della CPU, cambiare questo valore può aiutare con le prestazioni."

struct retro_core_option_v2_category option_cats_it[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_it[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_IT,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_IT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_IT,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_IT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_IT,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_IT,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_IT },
         { "1M_ram", OPTION_VAL_1M_RAM_IT },
         { "4M_ram", OPTION_VAL_4M_RAM_IT },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_IT,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_IT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_IT,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_IT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_IT,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_IT,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_it = {
   option_cats_it,
   option_defs_it
};

/* RETRO_LANGUAGE_JA */

#define YABAUSE_FRAMESKIP_LABEL_JA "フレームスキップ"
#define YABAUSE_FRAMESKIP_INFO_0_JA "フレームスキップを有効にすると, パフォーマンス向上に役立ちます."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_JA "HLE BIOSを強制する (再起動が必要)"
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_JA "自己責任で使用してください。改善よりも、問題発生の方が多いです。"
#define YABAUSE_ADDON_CARTRIDGE_LABEL_JA "アドオンカートリッジ (再起動が必要)"
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_JA "アドオンカートリッジを選択してください。"
#define OPTION_VAL_NONE_JA "なし"
#define OPTION_VAL_1M_RAM_JA NULL
#define OPTION_VAL_4M_RAM_JA NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_JA "ポート1でマルチターミナル6を使用する"
#define YABAUSE_MULTITAP_PORT1_INFO_0_JA "ポート1でマルチタップを有効にする"
#define YABAUSE_MULTITAP_PORT2_LABEL_JA "ポート2でマルチターミナル6を使用する"
#define YABAUSE_MULTITAP_PORT2_INFO_0_JA "ポート2でマルチタップを有効にする"
#define YABAUSE_NUMTHREADS_LABEL_JA "スレッド数 (再起動が必要)"
#define YABAUSE_NUMTHREADS_INFO_0_JA "CPUによっては、この値を変更するとパフォーマンス向上に役立ちます。"

struct retro_core_option_v2_category option_cats_ja[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ja[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_JA,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_JA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_JA,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_JA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_JA,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_JA,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_JA },
         { "1M_ram", OPTION_VAL_1M_RAM_JA },
         { "4M_ram", OPTION_VAL_4M_RAM_JA },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_JA,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_JA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_JA,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_JA,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_JA,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_JA,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ja = {
   option_cats_ja,
   option_defs_ja
};

/* RETRO_LANGUAGE_KO */

#define YABAUSE_FRAMESKIP_LABEL_KO "프레임 스킵"
#define YABAUSE_FRAMESKIP_INFO_0_KO "프레임 건너뛰기 사용, 성능에 도움이 됩니다."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_KO "HLE BIOS 강제 사용 (재시작 필요)"
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_KO "이 옵션을 사용하면 해결되는 문제보다 발생하는 문제가 더 많으니, 본인 책임 하에 사용하십시오."
#define YABAUSE_ADDON_CARTRIDGE_LABEL_KO "확장 카트리지 (재시작 필요)"
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_KO "확장 카트리지를 선택합니다."
#define OPTION_VAL_NONE_KO "없음"
#define OPTION_VAL_1M_RAM_KO NULL
#define OPTION_VAL_4M_RAM_KO NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_KO "1번 포트에 6인 플레이 어댑터 사용"
#define YABAUSE_MULTITAP_PORT1_INFO_0_KO "포트 1에서 멀티탭을 사용합니다"
#define YABAUSE_MULTITAP_PORT2_LABEL_KO "2번 포트에 6인 플레이 어댑터 사용"
#define YABAUSE_MULTITAP_PORT2_INFO_0_KO "포트 2에서 멀티탭을 사용합니다"
#define YABAUSE_NUMTHREADS_LABEL_KO "스레드 수 (재시작 필요)"
#define YABAUSE_NUMTHREADS_INFO_0_KO "CPU에 따라 값을 변경하면 성능에 도움을 줄 수 있습니다."

struct retro_core_option_v2_category option_cats_ko[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ko[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_KO,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_KO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_KO,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_KO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_KO,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_KO,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_KO },
         { "1M_ram", OPTION_VAL_1M_RAM_KO },
         { "4M_ram", OPTION_VAL_4M_RAM_KO },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_KO,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_KO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_KO,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_KO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_KO,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_KO,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ko = {
   option_cats_ko,
   option_defs_ko
};

/* RETRO_LANGUAGE_MT */

#define YABAUSE_FRAMESKIP_LABEL_MT NULL
#define YABAUSE_FRAMESKIP_INFO_0_MT NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_MT NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_MT NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_MT NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_MT NULL
#define OPTION_VAL_NONE_MT NULL
#define OPTION_VAL_1M_RAM_MT NULL
#define OPTION_VAL_4M_RAM_MT NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_MT NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_MT NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_MT NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_MT NULL
#define YABAUSE_NUMTHREADS_LABEL_MT NULL
#define YABAUSE_NUMTHREADS_INFO_0_MT NULL

struct retro_core_option_v2_category option_cats_mt[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_mt[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_MT,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_MT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_MT,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_MT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_MT,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_MT,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_MT },
         { "1M_ram", OPTION_VAL_1M_RAM_MT },
         { "4M_ram", OPTION_VAL_4M_RAM_MT },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_MT,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_MT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_MT,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_MT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_MT,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_MT,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_mt = {
   option_cats_mt,
   option_defs_mt
};

/* RETRO_LANGUAGE_NL */

#define YABAUSE_FRAMESKIP_LABEL_NL NULL
#define YABAUSE_FRAMESKIP_INFO_0_NL NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_NL NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_NL NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_NL NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_NL NULL
#define OPTION_VAL_NONE_NL "geen"
#define OPTION_VAL_1M_RAM_NL NULL
#define OPTION_VAL_4M_RAM_NL NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_NL NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_NL NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_NL NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_NL NULL
#define YABAUSE_NUMTHREADS_LABEL_NL NULL
#define YABAUSE_NUMTHREADS_INFO_0_NL NULL

struct retro_core_option_v2_category option_cats_nl[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_nl[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_NL,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_NL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_NL,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_NL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_NL,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_NL,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_NL },
         { "1M_ram", OPTION_VAL_1M_RAM_NL },
         { "4M_ram", OPTION_VAL_4M_RAM_NL },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_NL,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_NL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_NL,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_NL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_NL,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_NL,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_nl = {
   option_cats_nl,
   option_defs_nl
};

/* RETRO_LANGUAGE_NO */

#define YABAUSE_FRAMESKIP_LABEL_NO NULL
#define YABAUSE_FRAMESKIP_INFO_0_NO NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_NO NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_NO NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_NO NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_NO NULL
#define OPTION_VAL_NONE_NO NULL
#define OPTION_VAL_1M_RAM_NO NULL
#define OPTION_VAL_4M_RAM_NO NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_NO NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_NO NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_NO NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_NO NULL
#define YABAUSE_NUMTHREADS_LABEL_NO NULL
#define YABAUSE_NUMTHREADS_INFO_0_NO NULL

struct retro_core_option_v2_category option_cats_no[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_no[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_NO,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_NO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_NO,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_NO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_NO,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_NO,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_NO },
         { "1M_ram", OPTION_VAL_1M_RAM_NO },
         { "4M_ram", OPTION_VAL_4M_RAM_NO },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_NO,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_NO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_NO,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_NO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_NO,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_NO,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_no = {
   option_cats_no,
   option_defs_no
};

/* RETRO_LANGUAGE_OC */

#define YABAUSE_FRAMESKIP_LABEL_OC NULL
#define YABAUSE_FRAMESKIP_INFO_0_OC NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_OC NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_OC NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_OC NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_OC NULL
#define OPTION_VAL_NONE_OC NULL
#define OPTION_VAL_1M_RAM_OC NULL
#define OPTION_VAL_4M_RAM_OC NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_OC NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_OC NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_OC NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_OC NULL
#define YABAUSE_NUMTHREADS_LABEL_OC NULL
#define YABAUSE_NUMTHREADS_INFO_0_OC NULL

struct retro_core_option_v2_category option_cats_oc[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_oc[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_OC,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_OC,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_OC,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_OC,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_OC,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_OC,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_OC },
         { "1M_ram", OPTION_VAL_1M_RAM_OC },
         { "4M_ram", OPTION_VAL_4M_RAM_OC },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_OC,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_OC,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_OC,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_OC,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_OC,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_OC,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_oc = {
   option_cats_oc,
   option_defs_oc
};

/* RETRO_LANGUAGE_PL */

#define YABAUSE_FRAMESKIP_LABEL_PL "Pomijanie klatek"
#define YABAUSE_FRAMESKIP_INFO_0_PL "Włącz pomijanie klatek, powinno pomóc w wydajności."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_PL "Wymuś BIOS HLE (wymagany restart)"
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_PL NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_PL NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_PL NULL
#define OPTION_VAL_NONE_PL "brak"
#define OPTION_VAL_1M_RAM_PL NULL
#define OPTION_VAL_4M_RAM_PL NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_PL NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_PL "Włącz multitap w porcie 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_PL NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_PL "Włącz multitap w porcie 2"
#define YABAUSE_NUMTHREADS_LABEL_PL NULL
#define YABAUSE_NUMTHREADS_INFO_0_PL NULL

struct retro_core_option_v2_category option_cats_pl[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_pl[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_PL,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_PL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_PL,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_PL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_PL,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_PL,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_PL },
         { "1M_ram", OPTION_VAL_1M_RAM_PL },
         { "4M_ram", OPTION_VAL_4M_RAM_PL },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_PL,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_PL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_PL,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_PL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_PL,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_PL,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_pl = {
   option_cats_pl,
   option_defs_pl
};

/* RETRO_LANGUAGE_PT_BR */

#define YABAUSE_FRAMESKIP_LABEL_PT_BR "Pulo de quadro"
#define YABAUSE_FRAMESKIP_INFO_0_PT_BR "Habilita o pulo de quadros, que pode melhorar o desempenho."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_PT_BR "Impor a BIOS HLE (requer reinício)"
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_PT_BR "Use por sua conta e risco, isto causa mais problemas do que resolve."
#define YABAUSE_ADDON_CARTRIDGE_LABEL_PT_BR "Cartucho de expansão (requer reinício)"
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_PT_BR "Seleciona o cartucho da expansão."
#define OPTION_VAL_NONE_PT_BR "nenhum"
#define OPTION_VAL_1M_RAM_PT_BR "RAM de 1 MB"
#define OPTION_VAL_4M_RAM_PT_BR "RAM de 4 MB"
#define YABAUSE_MULTITAP_PORT1_LABEL_PT_BR "Adaptador para seis jogadores na porta 1"
#define YABAUSE_MULTITAP_PORT1_INFO_0_PT_BR "Habilita o multitap na porta 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_PT_BR "Adaptador para seis jogadores na porta 2"
#define YABAUSE_MULTITAP_PORT2_INFO_0_PT_BR "Habilita o multitap na porta 2"
#define YABAUSE_NUMTHREADS_LABEL_PT_BR "Número de threads (requer reinício)"
#define YABAUSE_NUMTHREADS_INFO_0_PT_BR "Dependendo da sua CPU, alterar esse valor pode ajudar no desempenho."

struct retro_core_option_v2_category option_cats_pt_br[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_pt_br[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_PT_BR,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_PT_BR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_PT_BR,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_PT_BR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_PT_BR,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_PT_BR,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_PT_BR },
         { "1M_ram", OPTION_VAL_1M_RAM_PT_BR },
         { "4M_ram", OPTION_VAL_4M_RAM_PT_BR },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_PT_BR,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_PT_BR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_PT_BR,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_PT_BR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_PT_BR,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_PT_BR,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_pt_br = {
   option_cats_pt_br,
   option_defs_pt_br
};

/* RETRO_LANGUAGE_PT_PT */

#define YABAUSE_FRAMESKIP_LABEL_PT_PT NULL
#define YABAUSE_FRAMESKIP_INFO_0_PT_PT NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_PT_PT NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_PT_PT NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_PT_PT NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_PT_PT NULL
#define OPTION_VAL_NONE_PT_PT "nenhum"
#define OPTION_VAL_1M_RAM_PT_PT NULL
#define OPTION_VAL_4M_RAM_PT_PT NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_PT_PT NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_PT_PT NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_PT_PT NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_PT_PT NULL
#define YABAUSE_NUMTHREADS_LABEL_PT_PT NULL
#define YABAUSE_NUMTHREADS_INFO_0_PT_PT NULL

struct retro_core_option_v2_category option_cats_pt_pt[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_pt_pt[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_PT_PT,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_PT_PT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_PT_PT,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_PT_PT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_PT_PT,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_PT_PT,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_PT_PT },
         { "1M_ram", OPTION_VAL_1M_RAM_PT_PT },
         { "4M_ram", OPTION_VAL_4M_RAM_PT_PT },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_PT_PT,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_PT_PT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_PT_PT,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_PT_PT,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_PT_PT,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_PT_PT,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_pt_pt = {
   option_cats_pt_pt,
   option_defs_pt_pt
};

/* RETRO_LANGUAGE_RO */

#define YABAUSE_FRAMESKIP_LABEL_RO NULL
#define YABAUSE_FRAMESKIP_INFO_0_RO NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_RO NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_RO NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_RO NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_RO NULL
#define OPTION_VAL_NONE_RO NULL
#define OPTION_VAL_1M_RAM_RO NULL
#define OPTION_VAL_4M_RAM_RO NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_RO NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_RO NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_RO NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_RO NULL
#define YABAUSE_NUMTHREADS_LABEL_RO NULL
#define YABAUSE_NUMTHREADS_INFO_0_RO NULL

struct retro_core_option_v2_category option_cats_ro[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ro[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_RO,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_RO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_RO,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_RO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_RO,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_RO,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_RO },
         { "1M_ram", OPTION_VAL_1M_RAM_RO },
         { "4M_ram", OPTION_VAL_4M_RAM_RO },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_RO,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_RO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_RO,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_RO,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_RO,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_RO,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ro = {
   option_cats_ro,
   option_defs_ro
};

/* RETRO_LANGUAGE_RU */

#define YABAUSE_FRAMESKIP_LABEL_RU "Пропуск кадров"
#define YABAUSE_FRAMESKIP_INFO_0_RU "Включает пропуск кадров для повышения производительности."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_RU "Использовать HLE BIOS (требуется перезапуск)"
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_RU "Включение опции даёт больше проблем, чем преимуществ. Используйте с осторожностью."
#define YABAUSE_ADDON_CARTRIDGE_LABEL_RU "Картридж расширения (требуется перезапуск)"
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_RU "Выбор картриджа расширения."
#define OPTION_VAL_NONE_RU "нет"
#define OPTION_VAL_1M_RAM_RU "1 МБ ОЗУ"
#define OPTION_VAL_4M_RAM_RU "4 МБ ОЗУ"
#define YABAUSE_MULTITAP_PORT1_LABEL_RU "Адаптер для 6 игроков в порте 1"
#define YABAUSE_MULTITAP_PORT1_INFO_0_RU "Подключает Multitap для порта 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_RU "Адаптер для 6 игроков в порте 2"
#define YABAUSE_MULTITAP_PORT2_INFO_0_RU "Подключает Multitap для порта 2"
#define YABAUSE_NUMTHREADS_LABEL_RU "Число потоков (требуется перезапуск)"
#define YABAUSE_NUMTHREADS_INFO_0_RU "В зависимости от процессора, изменение настройки может улучшить производительность."

struct retro_core_option_v2_category option_cats_ru[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_ru[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_RU,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_RU,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_RU,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_RU,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_RU,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_RU,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_RU },
         { "1M_ram", OPTION_VAL_1M_RAM_RU },
         { "4M_ram", OPTION_VAL_4M_RAM_RU },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_RU,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_RU,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_RU,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_RU,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_RU,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_RU,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_ru = {
   option_cats_ru,
   option_defs_ru
};

/* RETRO_LANGUAGE_SI */

#define YABAUSE_FRAMESKIP_LABEL_SI NULL
#define YABAUSE_FRAMESKIP_INFO_0_SI NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_SI NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_SI NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_SI NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_SI NULL
#define OPTION_VAL_NONE_SI NULL
#define OPTION_VAL_1M_RAM_SI NULL
#define OPTION_VAL_4M_RAM_SI NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_SI NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_SI NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_SI NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_SI NULL
#define YABAUSE_NUMTHREADS_LABEL_SI NULL
#define YABAUSE_NUMTHREADS_INFO_0_SI NULL

struct retro_core_option_v2_category option_cats_si[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_si[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_SI,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_SI,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_SI,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_SI,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_SI,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_SI,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_SI },
         { "1M_ram", OPTION_VAL_1M_RAM_SI },
         { "4M_ram", OPTION_VAL_4M_RAM_SI },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_SI,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_SI,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_SI,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_SI,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_SI,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_SI,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_si = {
   option_cats_si,
   option_defs_si
};

/* RETRO_LANGUAGE_SK */

#define YABAUSE_FRAMESKIP_LABEL_SK NULL
#define YABAUSE_FRAMESKIP_INFO_0_SK "Povoliť preskočenie snímkov, malo by pomôcť s výkonmi."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_SK NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_SK NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_SK NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_SK NULL
#define OPTION_VAL_NONE_SK "žiadne"
#define OPTION_VAL_1M_RAM_SK NULL
#define OPTION_VAL_4M_RAM_SK NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_SK NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_SK "Povoliť zariadenie pre viacerých hráčov na porte 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_SK NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_SK "Povoliť zariadenie pre viacerých hráčov na porte 2"
#define YABAUSE_NUMTHREADS_LABEL_SK NULL
#define YABAUSE_NUMTHREADS_INFO_0_SK NULL

struct retro_core_option_v2_category option_cats_sk[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_sk[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_SK,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_SK,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_SK,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_SK,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_SK,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_SK,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_SK },
         { "1M_ram", OPTION_VAL_1M_RAM_SK },
         { "4M_ram", OPTION_VAL_4M_RAM_SK },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_SK,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_SK,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_SK,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_SK,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_SK,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_SK,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_sk = {
   option_cats_sk,
   option_defs_sk
};

/* RETRO_LANGUAGE_SR */

#define YABAUSE_FRAMESKIP_LABEL_SR NULL
#define YABAUSE_FRAMESKIP_INFO_0_SR NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_SR NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_SR NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_SR NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_SR NULL
#define OPTION_VAL_NONE_SR NULL
#define OPTION_VAL_1M_RAM_SR NULL
#define OPTION_VAL_4M_RAM_SR NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_SR NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_SR NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_SR NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_SR NULL
#define YABAUSE_NUMTHREADS_LABEL_SR NULL
#define YABAUSE_NUMTHREADS_INFO_0_SR NULL

struct retro_core_option_v2_category option_cats_sr[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_sr[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_SR,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_SR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_SR,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_SR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_SR,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_SR,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_SR },
         { "1M_ram", OPTION_VAL_1M_RAM_SR },
         { "4M_ram", OPTION_VAL_4M_RAM_SR },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_SR,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_SR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_SR,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_SR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_SR,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_SR,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_sr = {
   option_cats_sr,
   option_defs_sr
};

/* RETRO_LANGUAGE_SV */

#define YABAUSE_FRAMESKIP_LABEL_SV NULL
#define YABAUSE_FRAMESKIP_INFO_0_SV "Aktivera frameskip, det bör hjälpa till med prestandan."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_SV NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_SV NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_SV NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_SV NULL
#define OPTION_VAL_NONE_SV "ingen"
#define OPTION_VAL_1M_RAM_SV NULL
#define OPTION_VAL_4M_RAM_SV NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_SV NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_SV "Aktivera multitap i port 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_SV NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_SV "Aktivera multitap i port 2"
#define YABAUSE_NUMTHREADS_LABEL_SV NULL
#define YABAUSE_NUMTHREADS_INFO_0_SV NULL

struct retro_core_option_v2_category option_cats_sv[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_sv[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_SV,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_SV,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_SV,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_SV,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_SV,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_SV,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_SV },
         { "1M_ram", OPTION_VAL_1M_RAM_SV },
         { "4M_ram", OPTION_VAL_4M_RAM_SV },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_SV,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_SV,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_SV,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_SV,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_SV,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_SV,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_sv = {
   option_cats_sv,
   option_defs_sv
};

/* RETRO_LANGUAGE_TR */

#define YABAUSE_FRAMESKIP_LABEL_TR "Kare Atlama"
#define YABAUSE_FRAMESKIP_INFO_0_TR "Kare atlamayı etkinleştirir, performansa yardımcı olması gerekir."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_TR "HLE BIOS'u zorla (Yeniden Başlatılmalı)"
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_TR "Kullanmak kendi sorumluluğunuzdadır, çözdüğünden daha fazla soruna neden olabilir."
#define YABAUSE_ADDON_CARTRIDGE_LABEL_TR "Kartuş Eklentisi (Yeniden Başlatılmalı)"
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_TR "Ek kartuşu seçin."
#define OPTION_VAL_NONE_TR "yok"
#define OPTION_VAL_1M_RAM_TR NULL
#define OPTION_VAL_4M_RAM_TR NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_TR "1. Port üstünde 6Player Adaptörü"
#define YABAUSE_MULTITAP_PORT1_INFO_0_TR "1. porttaki multitap'ı etkinleştir"
#define YABAUSE_MULTITAP_PORT2_LABEL_TR "2. Port üstünde 6Player Adaptörü"
#define YABAUSE_MULTITAP_PORT2_INFO_0_TR "2. porttaki multitap'ı etkinleştir"
#define YABAUSE_NUMTHREADS_LABEL_TR "İş Parçacığı Sayısı (Yeniden Başlatılmalı)"
#define YABAUSE_NUMTHREADS_INFO_0_TR "CPU çeşidine bağlı olarak, bu değeri değiştirmek performansa yardımcı olabilir."

struct retro_core_option_v2_category option_cats_tr[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_tr[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_TR,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_TR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_TR,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_TR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_TR,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_TR,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_TR },
         { "1M_ram", OPTION_VAL_1M_RAM_TR },
         { "4M_ram", OPTION_VAL_4M_RAM_TR },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_TR,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_TR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_TR,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_TR,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_TR,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_TR,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_tr = {
   option_cats_tr,
   option_defs_tr
};

/* RETRO_LANGUAGE_UK */

#define YABAUSE_FRAMESKIP_LABEL_UK "Пропуск кадру"
#define YABAUSE_FRAMESKIP_INFO_0_UK "Увімкнути пропуск кадрів, це має покращити продуктивність."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_UK NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_UK NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_UK NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_UK NULL
#define OPTION_VAL_NONE_UK "немає"
#define OPTION_VAL_1M_RAM_UK NULL
#define OPTION_VAL_4M_RAM_UK NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_UK NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_UK "Увімкнути multitap для портів 1"
#define YABAUSE_MULTITAP_PORT2_LABEL_UK NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_UK "Увімкнути multitap для портів 2"
#define YABAUSE_NUMTHREADS_LABEL_UK NULL
#define YABAUSE_NUMTHREADS_INFO_0_UK NULL

struct retro_core_option_v2_category option_cats_uk[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_uk[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_UK,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_UK,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_UK,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_UK,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_UK,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_UK,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_UK },
         { "1M_ram", OPTION_VAL_1M_RAM_UK },
         { "4M_ram", OPTION_VAL_4M_RAM_UK },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_UK,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_UK,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_UK,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_UK,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_UK,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_UK,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_uk = {
   option_cats_uk,
   option_defs_uk
};

/* RETRO_LANGUAGE_VAL */

#define YABAUSE_FRAMESKIP_LABEL_VAL NULL
#define YABAUSE_FRAMESKIP_INFO_0_VAL NULL
#define YABAUSE_FORCE_HLE_BIOS_LABEL_VAL NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_VAL NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_VAL NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_VAL NULL
#define OPTION_VAL_NONE_VAL NULL
#define OPTION_VAL_1M_RAM_VAL NULL
#define OPTION_VAL_4M_RAM_VAL NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_VAL NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_VAL NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_VAL NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_VAL NULL
#define YABAUSE_NUMTHREADS_LABEL_VAL NULL
#define YABAUSE_NUMTHREADS_INFO_0_VAL NULL

struct retro_core_option_v2_category option_cats_val[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_val[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_VAL,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_VAL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_VAL,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_VAL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_VAL,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_VAL,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_VAL },
         { "1M_ram", OPTION_VAL_1M_RAM_VAL },
         { "4M_ram", OPTION_VAL_4M_RAM_VAL },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_VAL,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_VAL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_VAL,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_VAL,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_VAL,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_VAL,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_val = {
   option_cats_val,
   option_defs_val
};

/* RETRO_LANGUAGE_VN */

#define YABAUSE_FRAMESKIP_LABEL_VN "Bỏ qua khung hình"
#define YABAUSE_FRAMESKIP_INFO_0_VN "Kích hoạt frameskip, nó có thể giúp cải thiện hiệu suất."
#define YABAUSE_FORCE_HLE_BIOS_LABEL_VN NULL
#define YABAUSE_FORCE_HLE_BIOS_INFO_0_VN NULL
#define YABAUSE_ADDON_CARTRIDGE_LABEL_VN NULL
#define YABAUSE_ADDON_CARTRIDGE_INFO_0_VN NULL
#define OPTION_VAL_NONE_VN NULL
#define OPTION_VAL_1M_RAM_VN NULL
#define OPTION_VAL_4M_RAM_VN NULL
#define YABAUSE_MULTITAP_PORT1_LABEL_VN NULL
#define YABAUSE_MULTITAP_PORT1_INFO_0_VN NULL
#define YABAUSE_MULTITAP_PORT2_LABEL_VN NULL
#define YABAUSE_MULTITAP_PORT2_INFO_0_VN NULL
#define YABAUSE_NUMTHREADS_LABEL_VN NULL
#define YABAUSE_NUMTHREADS_INFO_0_VN NULL

struct retro_core_option_v2_category option_cats_vn[] = {
   { NULL, NULL, NULL },
};
struct retro_core_option_v2_definition option_defs_vn[] = {
   {
      "yabause_frameskip",
      YABAUSE_FRAMESKIP_LABEL_VN,
      NULL,
      YABAUSE_FRAMESKIP_INFO_0_VN,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_force_hle_bios",
      YABAUSE_FORCE_HLE_BIOS_LABEL_VN,
      NULL,
      YABAUSE_FORCE_HLE_BIOS_INFO_0_VN,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_addon_cartridge",
      YABAUSE_ADDON_CARTRIDGE_LABEL_VN,
      NULL,
      YABAUSE_ADDON_CARTRIDGE_INFO_0_VN,
      NULL,
      NULL,
      {
         { "none", OPTION_VAL_NONE_VN },
         { "1M_ram", OPTION_VAL_1M_RAM_VN },
         { "4M_ram", OPTION_VAL_4M_RAM_VN },
         { NULL, NULL},
      },
      "none"
   },
   {
      "yabause_multitap_port1",
      YABAUSE_MULTITAP_PORT1_LABEL_VN,
      NULL,
      YABAUSE_MULTITAP_PORT1_INFO_0_VN,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "yabause_multitap_port2",
      YABAUSE_MULTITAP_PORT2_LABEL_VN,
      NULL,
      YABAUSE_MULTITAP_PORT2_INFO_0_VN,
      NULL,
      NULL,
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
#ifdef HAVE_THREADS
   {
      "yabause_numthreads",
      YABAUSE_NUMTHREADS_LABEL_VN,
      NULL,
      YABAUSE_NUMTHREADS_INFO_0_VN,
      NULL,
      NULL,
      {
         { "1", NULL },
         { "2", NULL },
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "32", NULL },
         { NULL, NULL},
      },
      "4"
   },
#endif
   { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};
struct retro_core_options_v2 options_vn = {
   option_cats_vn,
   option_defs_vn
};


#ifdef __cplusplus
}
#endif

#endif