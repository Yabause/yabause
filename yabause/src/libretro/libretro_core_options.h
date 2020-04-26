#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */

struct retro_core_option_definition option_defs_us[] = {
   {
      "kronos_force_hle_bios",
      "Force HLE BIOS",
      "Use at your own risk, we only provide support for real BIOS. Requires a restart.",
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "kronos_videoformattype",
      "Video format",
      "Select a video format for output. Requires a restart.",
      {
         { "auto", NULL },
         { "NTSC", NULL },
         { "PAL", NULL },
         { NULL, NULL},
      },
      "auto"
   },
   {
      "kronos_skipframe",
      "Frameskip",
      "Skip rendering of X frames out of X+1",
      {
         { "0", "No skipping" },
         { "1", "Skip rendering of 1 frames out of 2" },
         { "2", "Skip rendering of 2 frames out of 3" },
         { "3", "Skip rendering of 3 frames out of 4" },
         { "4", "Skip rendering of 4 frames out of 5" },
         { "5", "Skip rendering of 5 frames out of 6" },
         { NULL, NULL },
      },
      "0"
   },
   {
      "kronos_sh2coretype",
      "SH-2 cpu core",
      "Select an interpreter for the SH-2 cpus, 'old' is the old interpreter from yabause (we don't provide support for this one). Requires a restart.",
      {
         { "kronos", NULL },
         { "old", NULL },
         { NULL, NULL},
      },
      "kronos"
   },
#if !defined(_OGLES3_)
   {
      "kronos_opengl_version",
      "OpenGL version",
      "Select the highest OpenGL version compatible with your hardware. Requires a restart.",
      {
         { "3.3", "OpenGL 3.3 (minimum version)" },
         { "4.2", "OpenGL 4.2 (add support for gpu tesselation)" },
         { "4.3", "OpenGL 4.3 (add support for compute shaders)" },
         { "4.5", "OpenGL 4.5 (fix texture issues on some specific games)" },
         { NULL, NULL},
      },
      "3.3"
   },
   {
      "kronos_videocoretype",
      "Video renderer",
      "Change the renderer. Requires a restart.",
      {
         { "opengl", "OpenGL (requires OpenGL 3.3+)" },
         { "opengl_cs", "OpenGL CS (requires OpenGL 4.3+)" },
         { NULL, NULL},
      },
      "perspective_correction"
   },
#endif
   {
      "kronos_use_beetle_saves",
      "Share saves with beetle",
      "Use the same path as beetle-saturn for savefiles. Requires a restart.",
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "kronos_addon_cartridge",
      "Addon Cartridge",
      "Select the addon cartridge. Requires a restart.",
      {
         { "none", NULL },
         { "1M_extended_ram", NULL },
         { "4M_extended_ram", NULL },
         { "16M_extended_ram", NULL },
         { "512K_backup_ram", NULL },
         { "1M_backup_ram", NULL },
         { "2M_backup_ram", NULL },
         { "4M_backup_ram", NULL },
         { "auto", NULL },
         { NULL, NULL},
      },
      "auto"
   },
   {
      "kronos_multitap_port1",
      "6Player Adaptor on Port 1",
      "Enable multitap in port 1",
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "kronos_multitap_port2",
      "6Player Adaptor on Port 2",
      "Enable multitap in port 2 (it is known for causing issues)",
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "kronos_resolution_mode",
      "Internal Resolution",
      "Modify rendering resolution. Requires a restart.",
      {
         { "original", NULL },
         { "480p", NULL },
         { "720p", NULL },
         { "1080p", NULL },
         { "4k", NULL },
         { NULL, NULL},
      },
      "original"
   },
   {
      "kronos_polygon_mode",
      "Polygon Mode",
      "Select method for applying textures on polygon, gpu_tesselation requires OpenGL 4.2+, OpenGL CS renderer doesn't need this.",
      {
         { "perspective_correction", NULL },
         { "gpu_tesselation", NULL },
         { "cpu_tesselation", NULL },
         { NULL, NULL},
      },
      "perspective_correction"
   },
   {
      "kronos_meshmode",
      "Improved mesh",
      "Replace meshed transparency by real transparency.",
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "kronos_use_cs",
      "RBG Compute shaders",
      "Use compute shaders for upscaling RBG, requires OpenGL 4.3+.",
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "kronos_wireframe_mode",
      "Wireframe mode",
      "Wireframe mode, requires OpenGL CS renderer.",
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "kronos_service_enabled",
      "ST-V Service/Test Buttons",
      "Enable Service/Test for ST-V, to enter cabinet settings.",
      {
         { "disabled", NULL },
         { "enabled", NULL },
         { NULL, NULL},
      },
      "disabled"
   },
   {
      "kronos_stv_favorite_region",
      "ST-V Favorite Region",
      "Choose favorite bios region for STV.",
      {
         { "EU", NULL },
         { "US", NULL },
         { "JP", NULL },
         { "TW", NULL },
         { NULL, NULL},
      },
      "EU"
   },
   { NULL, NULL, NULL, {{0}}, NULL },
};

/* RETRO_LANGUAGE_JAPANESE */

/* RETRO_LANGUAGE_FRENCH */

/* RETRO_LANGUAGE_SPANISH */

/* RETRO_LANGUAGE_GERMAN */

/* RETRO_LANGUAGE_ITALIAN */

/* RETRO_LANGUAGE_DUTCH */

/* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */

/* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */

/* RETRO_LANGUAGE_RUSSIAN */

/* RETRO_LANGUAGE_KOREAN */

/* RETRO_LANGUAGE_CHINESE_TRADITIONAL */

/* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */

/* RETRO_LANGUAGE_ESPERANTO */

/* RETRO_LANGUAGE_POLISH */

/* RETRO_LANGUAGE_VIETNAMESE */

/* RETRO_LANGUAGE_ARABIC */

/* RETRO_LANGUAGE_GREEK */

/* RETRO_LANGUAGE_TURKISH */

/*
 ********************************
 * Language Mapping
 ********************************
*/

struct retro_core_option_definition *option_defs_intl[RETRO_LANGUAGE_LAST] = {
   option_defs_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,           /* RETRO_LANGUAGE_JAPANESE */
   NULL,           /* RETRO_LANGUAGE_FRENCH */
   NULL,           /* RETRO_LANGUAGE_SPANISH */
   NULL,           /* RETRO_LANGUAGE_GERMAN */
   NULL,           /* RETRO_LANGUAGE_ITALIAN */
   NULL,           /* RETRO_LANGUAGE_DUTCH */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,           /* RETRO_LANGUAGE_RUSSIAN */
   NULL,           /* RETRO_LANGUAGE_KOREAN */
   NULL,           /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,           /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,           /* RETRO_LANGUAGE_ESPERANTO */
   NULL,           /* RETRO_LANGUAGE_POLISH */
   NULL,           /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,           /* RETRO_LANGUAGE_ARABIC */
   NULL,           /* RETRO_LANGUAGE_GREEK */
   NULL,           /* RETRO_LANGUAGE_TURKISH */
};

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should only be called inside retro_set_environment().
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb)
{
   unsigned version = 0;

   if (!environ_cb)
      return;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && (version == 1))
   {
      struct retro_core_options_intl core_options_intl;
      unsigned language = 0;

      core_options_intl.us    = option_defs_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
          (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = option_defs_intl[language];

      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_intl);
   }
   else
   {
      size_t i;
      size_t num_options               = 0;
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine number of options */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      /* Allocate arrays */
      variables  = (struct retro_variable *)calloc(num_options + 1, sizeof(struct retro_variable));
      values_buf = (char **)calloc(num_options, sizeof(char *));

      if (!variables || !values_buf)
         goto error;

      /* Copy parameters from option_defs_us array */
      for (i = 0; i < num_options; i++)
      {
         const char *key                        = option_defs_us[i].key;
         const char *desc                       = option_defs_us[i].desc;
         const char *default_value              = option_defs_us[i].default_value;
         struct retro_core_option_value *values = option_defs_us[i].values;
         size_t buf_len                         = 3;
         size_t default_index                   = 0;

         values_buf[i] = NULL;

         if (desc)
         {
            size_t num_values = 0;

            /* Determine number of values */
            while (true)
            {
               if (values[num_values].value)
               {
                  /* Check if this is the default value */
                  if (default_value)
                     if (strcmp(values[num_values].value, default_value) == 0)
                        default_index = num_values;

                  buf_len += strlen(values[num_values].value);
                  num_values++;
               }
               else
                  break;
            }

            /* Build values string */
            if (num_values > 0)
            {
               size_t j;

               buf_len += num_values - 1;
               buf_len += strlen(desc);

               values_buf[i] = (char *)calloc(buf_len, sizeof(char));
               if (!values_buf[i])
                  goto error;

               strcpy(values_buf[i], desc);
               strcat(values_buf[i], "; ");

               /* Default value goes first */
               strcat(values_buf[i], values[default_index].value);

               /* Add remaining values */
               for (j = 0; j < num_values; j++)
               {
                  if (j != default_index)
                  {
                     strcat(values_buf[i], "|");
                     strcat(values_buf[i], values[j].value);
                  }
               }
            }
         }

         variables[i].key   = key;
         variables[i].value = values_buf[i];
      }
      
      /* Set variables */
      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);

error:

      /* Clean up */
      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
