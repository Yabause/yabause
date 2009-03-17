/*  src/psp/psp-video.c: PSP video interface module
    Copyright 2009 Andrew Church
    Based on src/vidogl.c by Guillaume Duhamel and others

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "../yabause.h"
#include "../vdp1.h"
#include "../vdp2.h"
#include "../vidshared.h"

#include "common.h"
#include "display.h"
#include "texcache.h"
#include "psp-video.h"

/*************************************************************************/

/* Interface function declarations (must come before interface definition) */

static int psp_video_init(void);
static void psp_video_deinit(void);
static void psp_video_resize(unsigned int width, unsigned int height,
                             int fullscreen);
static int psp_video_is_fullscreen(void);
static void psp_video_debug_message(char *format, ...);

static int psp_vdp1_reset(void);
static void psp_vdp1_draw_start(void);
static void psp_vdp1_draw_end(void);
static void psp_vdp1_normal_sprite_draw(void);
static void psp_vdp1_scaled_sprite_draw(void);
static void psp_vdp1_distorted_sprite_draw(void);
static void psp_vdp1_polygon_draw(void);
static void psp_vdp1_polyline_draw(void);
static void psp_vdp1_line_draw(void);
static void psp_vdp1_user_clipping(void);
static void psp_vdp1_system_clipping(void);
static void psp_vdp1_local_coordinate(void);

static int psp_vdp2_reset(void);
static void psp_vdp2_draw_start(void);
static void psp_vdp2_draw_end(void);
static void psp_vdp2_draw_screens(void);
static void psp_vdp2_set_resolution(u16 TVMD);
static void FASTCALL psp_vdp2_set_priority_NBG0(int priority);
static void FASTCALL psp_vdp2_set_priority_NBG1(int priority);
static void FASTCALL psp_vdp2_set_priority_NBG2(int priority);
static void FASTCALL psp_vdp2_set_priority_NBG3(int priority);
static void FASTCALL psp_vdp2_set_priority_RBG0(int priority);

/*-----------------------------------------------------------------------*/

/* Module interface definition */

VideoInterface_struct VIDPSP = {
    .id                   = VIDCORE_PSP,
    .Name                 = "PSP Video Interface",
    .Init                 = psp_video_init,
    .DeInit               = psp_video_deinit,
    .Resize               = psp_video_resize,
    .IsFullscreen         = psp_video_is_fullscreen,
    .OnScreenDebugMessage = psp_video_debug_message,

    .Vdp1Reset            = psp_vdp1_reset,
    .Vdp1DrawStart        = psp_vdp1_draw_start,
    .Vdp1DrawEnd          = psp_vdp1_draw_end,
    .Vdp1NormalSpriteDraw = psp_vdp1_normal_sprite_draw,
    .Vdp1ScaledSpriteDraw = psp_vdp1_scaled_sprite_draw,
    .Vdp1DistortedSpriteDraw = psp_vdp1_distorted_sprite_draw,
    .Vdp1PolygonDraw      = psp_vdp1_polygon_draw,
    .Vdp1PolylineDraw     = psp_vdp1_polyline_draw,
    .Vdp1LineDraw         = psp_vdp1_line_draw,
    .Vdp1UserClipping     = psp_vdp1_user_clipping,
    .Vdp1SystemClipping   = psp_vdp1_system_clipping,
    .Vdp1LocalCoordinate  = psp_vdp1_local_coordinate,

    .Vdp2Reset            = psp_vdp2_reset,
    .Vdp2DrawStart        = psp_vdp2_draw_start,
    .Vdp2DrawEnd          = psp_vdp2_draw_end,
    .Vdp2DrawScreens      = psp_vdp2_draw_screens,
    .Vdp2SetResolution    = psp_vdp2_set_resolution,
    .Vdp2SetPriorityNBG0  = psp_vdp2_set_priority_NBG0,
    .Vdp2SetPriorityNBG1  = psp_vdp2_set_priority_NBG1,
    .Vdp2SetPriorityNBG2  = psp_vdp2_set_priority_NBG2,
    .Vdp2SetPriorityNBG3  = psp_vdp2_set_priority_NBG3,
    .Vdp2SetPriorityRBG0  = psp_vdp2_set_priority_RBG0,
};

/*************************************************************************/

/**** Exported data ****/

/* Color table generated from VDP2 color RAM */
__attribute__((aligned(64))) uint16_t global_clut_16[0x800];
__attribute__((aligned(64))) uint32_t global_clut_32[0x800];

/*-----------------------------------------------------------------------*/

/**** Internal data ****/

/*----------------------------------*/

/* Displayed width and height */
static int disp_width, disp_height;

/* Scale (right-shift) applied to X and Y coordinates */
static int disp_xscale, disp_yscale;

/* Flag indicating whether this is the second field of an interlaced frame
 * (which will not be drawn) */
static int interlaced_field_2;

/* Flag indicating whether graphics should be drawn this frame */
static int draw_graphics;

/* Background priorities (NBG0, NBG1, NBG2, NBG3, RBG0) */
static int bg_priority[5];
enum {BG_NBG0 = 0, BG_NBG1, BG_NBG2, BG_NBG3, BG_RBG0};

/* VDP1 color component offset values (-0xFF...+0xFF) */
static int32_t vdp1_rofs, vdp1_gofs, vdp1_bofs;

/*----------------------------------*/

/* Rendering data for sprites, polygons, and lines (a copy of all
 * parameters except priority passed to vdp1_render_queue() */
typedef struct VDP1RenderData_ {
    uint32_t texture_key;
    int translucent;
    int primitive;
    int vertex_type;
    int count;
    const void *indices;
    const void *vertices;
} VDP1RenderData;

/* VDP1 render queues (one for each priority level) */
typedef struct VDP1RenderQueue_ {
    VDP1RenderData *queue;  // Array of entries (dynamically expanded)
    int size;               // Size of queue array, in entries
    int len;                // Number of entries currently in array
} VDP1RenderQueue;
static VDP1RenderQueue vdp1_queue[8];

/* Amount to expand a queue's array when it gets full */
#define VDP1_QUEUE_EXPAND_SIZE  1000

/*----------------------------------*/

/* Vertex types */
typedef struct VertexUVXYZ_ {
    int16_t u, v;
    int16_t x, y, z;
} VertexUVXYZ;

/*************************************************************************/

/*
 * Macros to work through the Saturn's remarkably nested tile layout (used
 * in map drawing routines).  Use like:
 *
 * TILE_LOOP_BEGIN(tilesize) {
 *     ...  // Draw a single tile based on "info" and "vertices"
 * } TILE_LOOP_END;
 *
 * where "tilesize" is 8 or 16 (info->patternwh * 8).
 */

#define TILE_LOOP_BEGIN(tilesize)  \
    int x1start = info->x;                                              \
    int y1;                                                             \
    for (y1 = 0; y1 < info->mapwh && info->y < info->drawh; y1++) {     \
        int y2start = info->y;                                          \
        info->x = x1start;                                              \
        int x1;                                                         \
        for (x1 = 0; x1 < info->mapwh && info->x < info->draww; x1++) { \
            info->PlaneAddr(info, (y1 * info->mapwh) + x1);             \
            int x2start = info->x;                                      \
            info->y = y2start;                                          \
            int y2;                                                     \
            for (y2 = 0; y2 < info->planeh; y2++) {                     \
                int y3start = info->y;                                  \
                info->x = x2start;                                      \
                int x2;                                                 \
                for (x2 = 0; x2 < info->planew; x2++) {                 \
                    int x3start = info->x;                              \
                    info->y = y3start;                                  \
                    int y3;                                             \
                    for (y3 = 0; y3 < info->pagewh;                     \
                         y3++, info->y += (tilesize)                    \
                    ) {                                                 \
                        if (UNLIKELY(info->y <= -(tilesize)             \
                                  || info->y >= info->drawh)) {         \
                            info->addr += info->patterndatasize * 2     \
                                         * info->pagewh;                \
                            continue;                                   \
                        }                                               \
                        info->x = x3start;                              \
                        int x3;                                         \
                        for (x3 = 0; x3 < info->pagewh;                 \
                             x3++, info->x += (tilesize),               \
                                   info->addr += info->patterndatasize * 2 \
                        ) {                                             \
                            if (UNLIKELY(info->x <= -(tilesize)         \
                                      || info->x >= info->draww)) {     \
                                continue;                               \
                            }                                           \
                            vdp2_calc_pattern_address(info);

#define TILE_LOOP_END \
                        }  /* Inner (pattern) X */              \
                    }  /* Inner (pattern) Y */                  \
                }  /* Middle (page) X */                        \
            }  /* Middle (page) Y */                            \
        }  /* Outer (plane) X */                                \
    }  /* Outer (plane) Y */

/*-----------------------------------------------------------------------*/

/* Additional macros used by tile map drawing routines */

/*----------------------------------*/

/* Declare tiledatasize as the size of a single tile's data in bytes */
#define GET_TILEDATASIZE \
    int tiledatasize;                                           \
    switch (info->colornumber) {                                \
        case 0:  /*  4bpp */ tiledatasize = 8*8/2; break;       \
        case 1:  /*  8bpp */ tiledatasize = 8*8*1; break;       \
        case 2:  /* 16bpp */ tiledatasize = 8*8*2; break;       \
        case 3:  /* 16bpp */ tiledatasize = 8*8*2; break;       \
        case 4:  /* 32bpp */ tiledatasize = 8*8*4; break;       \
        default: DMSG("Bad tile pixel type %d", info->colornumber); \
                 tiledatasize = 0; break;                        \
    }

/* Allocate memory for "vertspertile" vertices per tile */
#define GET_VERTICES(vertspertile)                              \
    int tilew = info->draww / 8 + 2;  /* +2 to handle partial-tile scroll */ \
    int tileh = info->drawh / 8 + 2;                            \
    int nvertices = tilew * tileh * (vertspertile);             \
    VertexUVXYZ *vertices = sceGuGetMemory(sizeof(*vertices) * nvertices); \
    if (!vertices) {                                            \
        DMSG("Failed to get vertex memory (%d bytes)",          \
             sizeof(*vertices) * nvertices);                    \
        return;                                                 \
    }

/* Start a display sublist so we're not writing to uncached memory and
 * triggering the GU every single time through the loop.  Ideally we'd
 * use a library like sceGu but that allowed the user to manually
 * choose when to commit data.  "cmdspertile" indicates the maximum number
 * of commands that will be written per tile. */
#define GET_LIST(cmdspertile)                                           \
    int maxcmds = tilew * tileh * (cmdspertile) + 7;                    \
    uint32_t *list = sceGuGetMemory(4 * maxcmds);                       \
    if (!list) {                                                        \
        DMSG("Failed to get list memory (%d bytes)", 4 * maxcmds);      \
        return;                                                         \
    }                                                                   \
    /* sceGuXXX() functions insist on using the uncached pointer, so    \
     * create the lists ourselves */                                    \
    /* sceGuStart(GU_CALL, list); */                                    \
    uint32_t *listptr = (uint32_t *)((uintptr_t)list & 0x3FFFFFFF);     \
    vertices = (VertexUVXYZ *)((uintptr_t)vertices & 0x3FFFFFFF);

/* Initialize variables for 8-bit indexed tile palette handling.  There can
 * be up to 128 different palettes, selectable on a per-tile basis, so to
 * save time, we only create (on the fly) those which are actually used. */
#define INIT_T8_PALETTE                                         \
    uint32_t *palettes[128];                                    \
    memset(palettes, 0, sizeof(palettes));                      \
    int cur_palette = -1;  /* So it's always set the first time */ \
    /* sceGuClutMode(GU_PSM_8888, 0, 0xFF, 0); */               \
    *listptr++ = 0xC500FF03;

/* Set the texture pixel format for 8-bit indexed tiles. 16-byte-wide
 * textures are effectively swizzled already, so set the swizzled flag for
 * whatever speed boost it gives us. */
#define INIT_T8_TEXTURE                                         \
    /* sceGuTexMode(GU_PSM_T8, 0, 0, 1); */                     \
    *listptr++ = 0xC2000001;                                    \
    *listptr++ = 0xC3000005;

/* Set the vertex type for 8-bit indexed tiles */
#define INIT_T8_VERTEX                                          \
    *listptr++ = 0x12000000                                     \
               | (GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D);

/* Initialize the work buffer pointers for 8-bit indexed tiles */
#define INIT_T8_WORK_BUFFER                                     \
    uint32_t * const work_buffer_0 = display_work_buffer();     \
    uint32_t * const work_buffer_1 = work_buffer_0 + DISPLAY_STRIDE;

/*----------------------------------*/

/* Declare flip_* and priority with the proper values */
#define GET_FLIP_PRI                                            \
    const int flip_u0 = (info->flipfunction & 1) << 3;          \
    const int flip_u1 = flip_u0 ^ 8;                            \
    const int flip_v0 = (info->flipfunction & 2) << 2;          \
    const int flip_v1 = flip_v0 ^ 8;                            \
    int priority;                                               \
    if (info->specialprimode == 1) {                            \
        priority = (info->priority & ~1) | info->specialfunction; \
    } else {                                                    \
        priority = info->priority;                              \
    }

/* Declare flip_* and priority for 8-bit indexed tiles */
static const int flip_t8_u[4][4] =
    { {0,8,8,16}, {8,0,16,8}, {8,16,0,8}, {16,8,8,0} };
#define GET_FLIP_PRI_T8                                         \
    const int flip_u0 = flip_t8_u[info->flipfunction][0];       \
    const int flip_u1 = flip_t8_u[info->flipfunction][1];       \
    const int flip_u2 = flip_t8_u[info->flipfunction][2];       \
    const int flip_u3 = flip_t8_u[info->flipfunction][3];       \
    const int flip_v0 = (info->flipfunction & 2) << 1;          \
    const int flip_v1 = flip_v0 ^ 4;                            \
    int priority;                                               \
    if (info->specialprimode == 1) {                            \
        priority = (info->priority & ~1) | info->specialfunction; \
    } else {                                                    \
        priority = info->priority;                              \
    }

/* Update the current palette for an 8-bit indexed tile, if necessary */
#define UPDATE_T8_PALETTE                                               \
    if (info->paladdr != cur_palette) {                                 \
        cur_palette = info->paladdr;                                    \
        if (UNLIKELY(!palettes[cur_palette])) {                         \
            palettes[cur_palette] = vdp2_gen_t8_clut(                   \
                info->coloroffset, info->paladdr<<4,                    \
                info->transparencyenable, info->cor, info->cog, info->cob \
            );                                                          \
        }                                                               \
        if (LIKELY(palettes[cur_palette])) {                            \
            /* sceGuClutLoad(256/8, clut); */                           \
            const uint32_t * const clut = palettes[cur_palette];        \
            *listptr++ = 0xB0000000 | ((uintptr_t)clut & 0x00FFFFFF);   \
            *listptr++ = 0xB1000000 | ((uintptr_t)clut & 0x3F000000) >> 8; \
            *listptr++ = 0xC4000020;                                    \
        }                                                               \
    }

/* Define 2 vertices for a generic 8x8 tile */
#define SET_VERTICES(tilex,tiley,xsize,ysize)                   \
    vertices[0].u = flip_u0;                                    \
    vertices[0].v = flip_v0;                                    \
    vertices[0].x = (tilex);                                    \
    vertices[0].y = (tiley);                                    \
    vertices[0].z = 0;                                          \
    vertices[1].u = flip_u1;                                    \
    vertices[1].v = flip_v1;                                    \
    vertices[1].x = (tilex) + (xsize);                          \
    vertices[1].y = (tiley) + (ysize);                          \
    vertices[1].z = 0;

/* Define 2 vertices for the even lines of an 8-bit indexed 8x8 tile */
#define SET_VERTICES_T8_EVEN(tilex,tiley,xsize,ysize)           \
    vertices[0].u = flip_u0;                                    \
    vertices[0].v = yofs + flip_v0;                             \
    vertices[0].x = (tilex);                                    \
    vertices[0].y = (tiley) / 2;                                \
    vertices[0].z = 0;                                          \
    vertices[1].u = flip_u1;                                    \
    vertices[1].v = yofs + flip_v1;                             \
    vertices[1].x = (tilex) + (xsize);                          \
    vertices[1].y = ((tiley) + (ysize)) / 2;                    \
    vertices[1].z = 0;

/* Define 2 vertices for the odd lines of an 8-bit indexed 8x8 tile */
#define SET_VERTICES_T8_ODD(tilex,tiley,xsize,ysize)            \
    vertices[2].u = flip_u2;                                    \
    vertices[2].v = flip_v0;                                    \
    vertices[2].x = (tilex);                                    \
    vertices[2].y = (tiley) / 2;                                \
    vertices[2].z = 0;                                          \
    vertices[3].u = flip_u3;                                    \
    vertices[3].v = flip_v1;                                    \
    vertices[3].x = (tilex) + (xsize);                          \
    vertices[3].y = ((tiley) + (ysize)) / 2;                    \
    vertices[3].z = 0;

/* Load the texture pointer for an 8-bit indexed 8x8 tile */
#define LOAD_T8_TILE                                            \
    /* sceGuTexImage(0, 512, 512, 16, src); */                  \
    *listptr++ = 0xA0000000 | ((uintptr_t)src & 0x00FFFFFF);    \
    *listptr++ = 0xA8000000 | ((uintptr_t)src & 0x3F000000) >> 8 | 16; \
    *listptr++ = 0xB8000909;                                    \
    *listptr++ = 0xCB000000;

/* Set the even-lines work buffer for 8-bit indexed 8x8 tiles */
#define SET_T8_BUFFER_0                                                    \
    /* sceGuDrawBuffer(GU_PSM_8888, work_buffer_0, DISPLAY_STRIDE*2); */   \
    *listptr++ = 0x9C000000 | ((uintptr_t)work_buffer_0 & 0x00FFFFFF);     \
    *listptr++ = 0x9D000000 | ((uintptr_t)work_buffer_0 & 0x3F000000) >> 8 \
                            | (DISPLAY_STRIDE*2);

/* Draw the even lines of an 8-bit indexed 8x8 tile */
#define RENDER_T8_EVEN \
    /* sceGuDrawArray(GU_SPRITES,                                       \
     *                GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, \
     *                2, NULL, vertices); */                            \
    *listptr++ = 0x01000000 | ((uintptr_t)vertices & 0x00FFFFFF);       \
    *listptr++ = 0x10000000 | ((uintptr_t)vertices & 0x3F000000) >> 8;  \
    *listptr++ = 0x04060002;

/* Set the odd-lines work buffer for 8-bit indexed 8x8 tiles */
#define SET_T8_BUFFER_1                                                    \
    /* sceGuDrawBuffer(GU_PSM_8888, work_buffer_1, DISPLAY_STRIDE*2); */   \
    *listptr++ = 0x9C000000 | ((uintptr_t)work_buffer_1 & 0x00FFFFFF);     \
    *listptr++ = 0x9D000000 | ((uintptr_t)work_buffer_1 & 0x3F000000) >> 8 \
                            | (DISPLAY_STRIDE*2);

/* Draw the odd lines of an 8-bit indexed 8x8 tile */
#define RENDER_T8_ODD \
    /* sceGuDrawArray(GU_SPRITES,                                       \
     *                GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D, \
     *                2, NULL, vertices+2); */                          \
    *listptr++ = 0x01000000 | ((uintptr_t)(vertices+2) & 0x00FFFFFF);   \
    *listptr++ = 0x10000000 | ((uintptr_t)(vertices+2) & 0x3F000000) >> 8; \
    *listptr++ = 0x04060002;

/*************************************************************************/

/**** Local function declarations ****/

static void vdp1_draw_lines(vdp1cmd_struct *cmd, int poly);
static void vdp1_draw_quad(vdp1cmd_struct *cmd, int textured);
static uint32_t vdp1_get_cmd_color(vdp1cmd_struct *cmd);
static uint32_t vdp1_get_cmd_color_pri(vdp1cmd_struct *cmd, int *priority_ret);
static uint32_t vdp1_process_sprite_color(uint16_t color16, int *priority_ret);
static uint32_t vdp1_cache_sprite_texture(
    vdp1cmd_struct *cmd, int width, int height, int *priority_ret);
static inline void vdp1_queue_render(
    int priority, uint32_t texture_key, int translucent, int primitive,
    int vertex_type, int count, const void *indices, const void *vertices);
static void vdp1_run_queue(int priority);

static inline void vdp2_get_color_offsets(uint16_t mask, int32_t *rofs_ret,
                                          int32_t *gofs_ret, int32_t *bofs_ret);

static void vdp2_draw_bg(void);
static void vdp2_draw_graphics(int layer);
static void vdp2_draw_map_8x8(vdp2draw_struct *info, clipping_struct *clip);
static void vdp2_draw_map_8x8_t8(vdp2draw_struct *info, clipping_struct *clip);
static void vdp2_draw_map_16x16(vdp2draw_struct *info, clipping_struct *clip);
static void vdp2_draw_map_16x16_t8(vdp2draw_struct *info,
                                   clipping_struct *clip);
static inline void vdp2_calc_pattern_address(vdp2draw_struct *info);
static inline uint32_t *vdp2_gen_t8_clut(
    int color_base, int color_ofs, int transparent,
    int rofs, int gofs, int bofs);
static void vdp2_draw_bitmap(vdp2draw_struct *info, clipping_struct *clip);
static void vdp2_draw_bitmap_t8(vdp2draw_struct *info, clipping_struct *clip);

/*************************************************************************/
/********************** General interface functions **********************/
/*************************************************************************/

/**
 * psp_video_init:  Initialize the peripheral interface.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Zero on success, negative on error
 */
static int psp_video_init(void)
{
    /* Set some reasonable defaults */
    disp_width = 320;
    disp_height = 224;
    disp_xscale = 0;
    disp_yscale = 0;

    return 0;
}

/*-----------------------------------------------------------------------*/

/**
 * psp_video_deinit:  Shut down the peripheral interface.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_video_deinit(void)
{
    /* We don't implement shutting down, so nothing to do */
}

/*************************************************************************/

/**
 * psp_video_resize:  Resize the display window.  A no-op on PSP.
 *
 * [Parameters]
 *          width: New window width
 *         height: New window height
 *     fullscreen: Nonzero to use fullscreen mode, else zero
 * [Return value]
 *     None
 */
static void psp_video_resize(unsigned int width, unsigned int height,
                             int fullscreen)
{
}

/*************************************************************************/

/**
 * psp_video_is_fullscreen:  Return whether the display is currently in
 * fullscreen mode.  Always returns true (nonzero) on PSP.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Nonzero if in fullscreen mode, else zero
 */
static int psp_video_is_fullscreen(void)
{
    return 1;
}

/*************************************************************************/

/**
 * psp_video_debug_message:  Display a debug message on the screen.
 *
 * [Parameters]
 *     format: printf()-style format string
 * [Return value]
 *     None
 */
static void psp_video_debug_message(char *format, ...)
{
    // FIXME: not implemented yet
}

/*************************************************************************/
/******************* VDP1-specific interface functions *******************/
/*************************************************************************/

/**
 * psp_vdp1_reset:  Reset the VDP1 state.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Unknown (always zero)
 */
static int psp_vdp1_reset(void)
{
    /* Nothing to do */
    return 0;
}

/*************************************************************************/

/**
 * psp_vdp1_draw_start:  Prepare for VDP1 drawing.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_draw_start(void)
{
    if (interlaced_field_2) {
        return;
    }

    /* Clear out all the rendering queues (just to be sure) */
    int priority;
    for (priority = 0; priority < 8; priority++) {
        vdp1_queue[priority].len = 0;
    }

    /* Get the color offsets */
    vdp2_get_color_offsets(1<<6, &vdp1_rofs, &vdp1_gofs, &vdp1_bofs);
}

/*************************************************************************/


/**
 * psp_vdp1_draw_end:  Finish VDP1 drawing.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_draw_end(void)
{
    /* Nothing to do */
}

/*************************************************************************/


/**
 * psp_vdp1_normal_sprite_draw:  Draw an unscaled rectangular sprite.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_normal_sprite_draw(void)
{
    if (interlaced_field_2) {
        return;
    }

    vdp1cmd_struct cmd;
    Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

    int width  = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
    int height = cmd.CMDSIZE & 0xFF;
    cmd.CMDXB = cmd.CMDXA + width; cmd.CMDYB = cmd.CMDYA;
    cmd.CMDXC = cmd.CMDXA;         cmd.CMDYC = cmd.CMDYA + height;
    cmd.CMDXD = cmd.CMDXA + width; cmd.CMDYD = cmd.CMDYA + height;

    vdp1_draw_quad(&cmd, 1);
}

/*************************************************************************/

/**
 * psp_vdp1_scaled_sprite_draw:  Draw a scaled rectangular sprite.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_scaled_sprite_draw(void)
{
    if (interlaced_field_2) {
        return;
    }

    vdp1cmd_struct cmd;
    Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

    if (!(cmd.CMDCTRL & 0x0F00)) {
        /* Direct specification of the size */
        cmd.CMDXC++;           cmd.CMDYC++;
        cmd.CMDXB = cmd.CMDXC; cmd.CMDYB = cmd.CMDYA;
        cmd.CMDXD = cmd.CMDXA; cmd.CMDYD = cmd.CMDYC;
    } else {
        /* Scale around a particular point (left/top, center, right/bottom) */
        int new_w = cmd.CMDXB + 1;
        int new_h = cmd.CMDYB + 1;
        if ((cmd.CMDCTRL & 0x300) == 0x200) {
            cmd.CMDXA -= cmd.CMDXB / 2;
        } else if ((cmd.CMDCTRL & 0x300) == 0x300) {
            cmd.CMDXA -= cmd.CMDXB;
        }
        if ((cmd.CMDCTRL & 0xC00) == 0x800) {
            cmd.CMDYA -= cmd.CMDYB / 2;
        } else if ((cmd.CMDCTRL & 0xC00) == 0xC00) {
            cmd.CMDYA -= cmd.CMDYB;
        }
        cmd.CMDXB = cmd.CMDXA + new_w; cmd.CMDYB = cmd.CMDYA;
        cmd.CMDXC = cmd.CMDXA;         cmd.CMDYB = cmd.CMDYA + new_h;
        cmd.CMDXD = cmd.CMDXA + new_w; cmd.CMDYD = cmd.CMDYA + new_h;
    }

    vdp1_draw_quad(&cmd, 1);
}

/*************************************************************************/

/**
 * psp_vdp1_distorted_sprite_draw:  Draw a sprite on an arbitrary
 * quadrilateral.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_distorted_sprite_draw(void)
{
    if (interlaced_field_2) {
        return;
    }

    vdp1cmd_struct cmd;
    Vdp1ReadCommand(&cmd, Vdp1Regs->addr);
    vdp1_draw_quad(&cmd, 1);
}

/*************************************************************************/

/**
 * psp_vdp1_polygon_draw:  Draw an untextured quadrilateral.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_polygon_draw(void)
{
    if (interlaced_field_2) {
        return;
    }

    vdp1cmd_struct cmd;
    Vdp1ReadCommand(&cmd, Vdp1Regs->addr);
    vdp1_draw_quad(&cmd, 0);
}

/*************************************************************************/

/**
 * psp_vdp1_polyline_draw:  Draw four connected lines.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_polyline_draw(void)
{
    if (interlaced_field_2) {
        return;
    }

    vdp1cmd_struct cmd;
    Vdp1ReadCommand(&cmd, Vdp1Regs->addr);
    vdp1_draw_lines(&cmd, 1);
}

/*************************************************************************/

/**
 * psp_vdp1_line_draw:  Draw a single line.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_line_draw(void)
{
    if (interlaced_field_2) {
        return;
    }

    vdp1cmd_struct cmd;
    Vdp1ReadCommand(&cmd, Vdp1Regs->addr);
    vdp1_draw_lines(&cmd, 0);
}

/*************************************************************************/

/**
 * psp_vdp1_user_clipping:  Set the user clipping coordinates.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_user_clipping(void)
{
    Vdp1Regs->userclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
    Vdp1Regs->userclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
    Vdp1Regs->userclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
    Vdp1Regs->userclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
}

/*************************************************************************/

/**
 * psp_vdp1_system_clipping:  Set the system clipping coordinates.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_system_clipping(void)
{
    Vdp1Regs->systemclipX1 = 0;
    Vdp1Regs->systemclipY1 = 0;
    Vdp1Regs->systemclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
    Vdp1Regs->systemclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
}

/*************************************************************************/

/**
 * psp_vdp1_local_coordinate:  Set coordinate offset values used in drawing
 * primitives.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp1_local_coordinate(void)
{
    Vdp1Regs->localX = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xC);
    Vdp1Regs->localY = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0xE);
}

/*************************************************************************/
/******************* VDP2-specific interface functions *******************/
/*************************************************************************/

/**
 * psp_vdp2_reset:  Reset the VDP2 state.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Unknown (always zero)
 */
static int psp_vdp2_reset(void)
{
    /* Nothing to do */
    return 0;
}

/*************************************************************************/

/**
 * psp_vdp2_draw_start:  Begin drawing a video frame.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp2_draw_start(void)
{
    /* First check whether this is the second field of an interlaced frame.
     * If so, we already drew everything last field, so we just copy the
     * frame this time around. */
    if (interlaced_field_2) {
        display_begin_frame();
        display_blit(display_disp_buffer(), disp_width >> disp_xscale,
                     disp_height >> disp_yscale, DISPLAY_STRIDE, 0, 0);
        return;
    }

    /* Load the global color lookup tables from VDP2 color RAM */
    const uint16_t *cram = (const uint16_t *)Vdp2ColorRam;
    if (Vdp2Internal.ColorMode == 2) {  // 32-bit color table
        int i;
        for (i = 0; i < 0x400; i++) {
            uint16_t xb = cram[i*2+0];
            uint16_t gr = cram[i*2+1];
            uint16_t color16 = 0x8000 | (xb<<7 & 0x7C00)
                                      | (gr<<2 & 0x3E0) | (gr>>3 & 0x1F);
            uint32_t color32 = 0xFF000000 | xb<<16 | gr;
            global_clut_16[i] = color16;
            global_clut_16[i+0x400] = color16;
            global_clut_32[i] = color32;
            global_clut_32[i+0x400] = color32;
        }
    } else {  // 16-bit color table
        int i;
        for (i = 0; i < 0x800; i++) {
            uint16_t color16 = 0x8000 | cram[i];
            uint32_t color32 = 0xFF000000 | (color16 & 0x7C00) << 9
                                          | (color16 & 0x03E0) << 6
                                          | (color16 & 0x001F) << 3;
            global_clut_16[i] = color16;
            global_clut_32[i] = color32;
        }
    }

    /* Start a new frame */
    display_begin_frame();
    texcache_reset();

    /* Initialize the render state */
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuEnable(GU_BLEND);  // We treat everything as alpha-enabled

    /* Reset the draw-graphics flag (will be set by draw_screens() if
     * graphics are active) */
    draw_graphics = 0;
}

/*************************************************************************/

/**
 * psp_vdp2_draw_end:  Finish drawing a video frame.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp2_draw_end(void)
{
    if (!interlaced_field_2) {

        /* Draw all graphics by priority */
        int priority;
        for (priority = 0; priority < 8; priority++) {
            /* Draw background graphics first */
            if (draw_graphics && priority > 0) {
                if (bg_priority[BG_NBG3] == priority) {
                    vdp2_draw_graphics(BG_NBG3);
                }
                if (bg_priority[BG_NBG2] == priority) {
                    vdp2_draw_graphics(BG_NBG2);
                }
                if (bg_priority[BG_NBG1] == priority) {
                    vdp2_draw_graphics(BG_NBG1);
                }
                if (bg_priority[BG_NBG0] == priority) {
                    vdp2_draw_graphics(BG_NBG0);
                }
                if (bg_priority[BG_RBG0] == priority) {
                    vdp2_draw_graphics(BG_RBG0);
                }
            }
            /* Then draw sprites on top */
            vdp1_run_queue(priority);
            /* And clear the rendering queue */
            vdp1_queue[priority].len = 0;
        }

    }  // if (!interlaced_field_2)

    display_end_frame();

    if (disp_height > 272) {  // Interlaced
        /* Switch fields */
        interlaced_field_2 = !interlaced_field_2;
    } else {  // Not interlaced
        interlaced_field_2 = 0;
    }
}

/*************************************************************************/

/**
 * psp_vdp2_draw_screens:  Draw the VDP2 background and graphics layers.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void psp_vdp2_draw_screens(void)
{
    if (interlaced_field_2) {
        return;
    }

    /* Draw the background color(s) */
    vdp2_draw_bg();

    /* Flag the background graphics to be drawn */
    draw_graphics = 1;
}

/*************************************************************************/

/**
 * psp_vdp2_set_resolution:  Change the resolution of the Saturn display.
 *
 * [Parameters]
 *     TVMD: New value of the VDP2 TVMD register
 * [Return value]
 *     None
 */
static void psp_vdp2_set_resolution(u16 TVMD)
{
    /* Set the display width from bits 0-1 */
    disp_width = (TVMD & 1) ? 352 : 320;
    if (TVMD & 2) {
        disp_width *= 2;
    }

    /* Set the display height from bits 4-5.  Note that 0x30 is an invalid
     * value for these bits and should not occur in practice; valid heights
     * are 0x00=224, 0x10=240, and (for PAL) 0x20=256 */
    disp_height = 224 + (TVMD & 0x30);
    if ((TVMD & 0xC0) == 0xC0) {
        disp_height *= 2;  // Interlaced mode
    }

    /* Hi-res or interlaced displays won't fit on the PSP screen, so cut
     * everything in half when using them */
    disp_xscale = (disp_width  > 352);
    disp_yscale = (disp_height > 256);

    /* Reset to interlaced field 1 */
    interlaced_field_2 = 0;

    /* Update the GE display parameters */
    display_set_size(disp_width >> disp_xscale, disp_height >> disp_yscale);
}

/*************************************************************************/

/**
 * psp_vdp2_set_priority_{NBG[0-3],RGB0}:  Set the priority of the given
 * background graphics layer.
 *
 * [Parameters]
 *     priority: Priority to set
 * [Return value]
 *     None
 */
static void FASTCALL psp_vdp2_set_priority_NBG0(int priority)
{
    bg_priority[BG_NBG0] = priority;
}

static void FASTCALL psp_vdp2_set_priority_NBG1(int priority)
{
    bg_priority[BG_NBG1] = priority;
}

static void FASTCALL psp_vdp2_set_priority_NBG2(int priority)
{
    bg_priority[BG_NBG2] = priority;
}

static void FASTCALL psp_vdp2_set_priority_NBG3(int priority)
{
    bg_priority[BG_NBG3] = priority;
}

static void FASTCALL psp_vdp2_set_priority_RBG0(int priority)
{
    bg_priority[BG_RBG0] = priority;
}

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * vdp1_draw_lines:  Draw one or four lines based on the given VDP1 command.
 *
 * [Parameters]
 *      cmd: VDP1 command pointer
 *     poly: Nonzero = draw four connected lines, zero = draw a single line
 * [Return value]
 *     None
 */
static void vdp1_draw_lines(vdp1cmd_struct *cmd, int poly)
{
    /* Get the line color and priority */
    // FIXME: vidogl.c suggests that the priority processing done for
    // sprites and polygons is not done here; is that correct?
    const uint32_t color32 = vdp1_get_cmd_color(cmd);
    const int priority = Vdp2Regs->PRISA & 0x7;

    /* Set up the vertex array */
    int nvertices = poly ? 5 : 2;
    struct {uint32_t color; int16_t x, y, z, pad;} *vertices;
    vertices = sceGuGetMemory(sizeof(*vertices) * nvertices);
    if (!vertices) {
        DMSG("Failed to get vertex memory (%d bytes)",
             sizeof(*vertices) * nvertices);
        return;
    }
    vertices[0].color = color32;
    vertices[0].x = (cmd->CMDXA + Vdp1Regs->localX) >> disp_xscale;
    vertices[0].y = (cmd->CMDYA + Vdp1Regs->localY) >> disp_yscale;
    vertices[0].z = 0;
    vertices[1].color = color32;
    vertices[1].x = (cmd->CMDXB + Vdp1Regs->localX) >> disp_xscale;
    vertices[1].y = (cmd->CMDYB + Vdp1Regs->localY) >> disp_xscale;
    vertices[1].z = 0;
    if (poly) {
        vertices[2].color = color32;
        vertices[2].x = (cmd->CMDXC + Vdp1Regs->localX) >> disp_xscale;
        vertices[2].y = (cmd->CMDYC + Vdp1Regs->localY) >> disp_yscale;
        vertices[2].z = 0;
        vertices[3].color = color32;
        vertices[3].x = (cmd->CMDXD + Vdp1Regs->localX) >> disp_xscale;
        vertices[3].y = (cmd->CMDYD + Vdp1Regs->localY) >> disp_yscale;
        vertices[3].z = 0;
        vertices[4] = vertices[0];
    }

    /* Queue the line(s) */
    vdp1_queue_render(priority, 0, 0, GU_LINE_STRIP,
                      GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                      nvertices, NULL, vertices);
}

/*************************************************************************/

/**
 * vdp1_draw_quad:  Draw a quadrilateral based on the given VDP1 command.
 *
 * [Parameters]
 *          cmd: VDP1 command pointer
 *     textured: Nonzero if the quadrilateral is textured (i.e. a sprite)
 * [Return value]
 *     None
 */
static void vdp1_draw_quad(vdp1cmd_struct *cmd, int textured)
{
    /* Get the width, height, and flip arguments for sprites (unused for
     * untextured polygons) */
    int width, height, flip_h, flip_v;
    if (textured) {
        width  = ((cmd->CMDSIZE >> 8) & 0x3F) * 8;
        height = cmd->CMDSIZE & 0xFF;
        if (width == 0 || height < 2) {
            return;
        }
        flip_h = cmd->CMDCTRL & 0x10;  // Flip horizontally?
        flip_v = cmd->CMDCTRL & 0x20;  // Flip vertically?
    } else {
        width = height = flip_h = flip_v = 0;
    }


    /* Get the polygon color and priority, and load the texture if it's
     * a sprite */
    uint32_t color32;
    int priority;
    uint32_t texture_key;
    if (textured) {
        color32 = 0xFFFFFFFF;
        texture_key = vdp1_cache_sprite_texture(cmd, width, height, &priority);
    } else {
        color32 = vdp1_get_cmd_color_pri(cmd, &priority);
        texture_key = 0;
    }

    /*
     * The Saturn allows the vertices of quadrilaterals to be specified in
     * any order.  On the PSP, though, we only have triangles to work with,
     * so we have to know in which order the points should be connected to
     * form a proper quadrilateral.  (If we get it backwards, the two
     * triangles passed to the GE overlap, and 1/4 of the polygon is not
     * rendered, sort of like Pac-Man.)
     *
     * For convex polygons, we can determine the order of vertices by
     * checking the slopes of lines from one vertex to each of the others.
     * Except in the case of a degenerate quadrilateral (i.e. triangle,
     * line, or point), one slope will be strictly between the others;
     * this is the diagonal, and the other two are edges of the polygon.
     *
     * Here, we find and order the orientations of the line segments AB,
     * AC, and AD (going by the vertex names in the command structure).
     * There are six possible orders of these three segments; we rearrange
     * vertices as necessary to ensure that AD is the diagonal, since BC
     * (the opposite diagonal) is the edge shared between the two triangles
     * we render:
     *     AB, AC, AD --> swap D and C
     *     AB, AD, AC --> (no change)
     *     AC, AB, AD --> swap D and B
     *     AC, AD, AB --> (no change)
     *     AD, AB, AC --> swap D and B
     *     AD, AC, AB --> swap D and C
     * Since we do not perform face culling, we don't worry about whether
     * the resulting triangles are specified in clockwise or counter-
     * clockwise order.
     *
     * If someone passes us a concave quadrilateral, we gleefully lose.
     */

    /* Prepare local variables to receive the coordinates */
    int32_t A_x, A_y, B_x, B_y, C_x, C_y, D_x, D_y;

    /* Compute the cross products AB x AC, AB x AD, AC x AD to determine
     * the directions (positive = clockwise, negative = counterclockwise)
     * between each pair of line segments */
    int32_t AB_x = cmd->CMDXB - cmd->CMDXA, AB_y = cmd->CMDYB - cmd->CMDYA;
    int32_t AC_x = cmd->CMDXC - cmd->CMDXA, AC_y = cmd->CMDYC - cmd->CMDYA;
    int32_t AD_x = cmd->CMDXD - cmd->CMDXA, AD_y = cmd->CMDYD - cmd->CMDYA;
    int32_t ABxAC = AB_x*AC_y - AB_y*AC_x;
    int32_t ABxAD = AB_x*AD_y - AB_y*AD_x;
    int32_t ACxAD = AC_x*AD_y - AC_y*AD_x;
    int ABxAC_sign = (ABxAC < 0);
    int ABxAD_sign = (ABxAD < 0);
    int ACxAD_sign = (ACxAD < 0);

    /* Assign point coordinates to local variables, possibly switching
     * around the order of the vertices */
    A_x = cmd->CMDXA; A_y = cmd->CMDYA;
    if (ABxAC_sign != ABxAD_sign) {
        /* AB is between AC and AD, so swap D with B */
        B_x = cmd->CMDXD; B_y = cmd->CMDYD;
        C_x = cmd->CMDXC; C_y = cmd->CMDYC;
        D_x = cmd->CMDXB; D_y = cmd->CMDYB;
    } else if (ABxAC_sign == ACxAD_sign) {
        /* AC is between AB and AD, so swap D with C */
        B_x = cmd->CMDXB; B_y = cmd->CMDYB;
        C_x = cmd->CMDXD; C_y = cmd->CMDYD;
        D_x = cmd->CMDXC; D_y = cmd->CMDYC;
    } else {
        /* AD is between AB and AC, so we're fine as is */
        B_x = cmd->CMDXB; B_y = cmd->CMDYB;
        C_x = cmd->CMDXC; C_y = cmd->CMDYC;
        D_x = cmd->CMDXD; D_y = cmd->CMDYD;
    }

    /* Set up the vertex array.  For simplicity, we assign both the color
     * and U/V coordinates regardless of whether the polygon is textured or
     * not; the GE is fast enough that it can handle all the processing in
     * time. */

    // FIXME: Do we need to adjust U/V for the vertex reordering above?

    struct {int16_t u, v; uint32_t color; int16_t x, y, z, pad;} *vertices;
    vertices = sceGuGetMemory(sizeof(*vertices) * 4);
    if (!vertices) {
        DMSG("Failed to get vertex memory (%d bytes)", sizeof(*vertices) * 4);
        return;
    }
    vertices[0].u = (flip_h ? width : 0);
    vertices[0].v = (flip_v ? height : 0);
    vertices[0].color = color32;
    vertices[0].x = (A_x + Vdp1Regs->localX) >> disp_xscale;
    vertices[0].y = (A_y + Vdp1Regs->localY) >> disp_yscale;
    vertices[0].z = 0;
    vertices[1].u = (flip_h ? 0 : width);
    vertices[1].v = (flip_v ? height : 0);
    vertices[1].color = color32;
    vertices[1].x = (B_x + Vdp1Regs->localX) >> disp_xscale;
    vertices[1].y = (B_y + Vdp1Regs->localY) >> disp_yscale;
    vertices[1].z = 0;
    vertices[2].u = (flip_h ? width : 0);
    vertices[2].v = (flip_v ? 0 : height);
    vertices[2].color = color32;
    vertices[2].x = (C_x + Vdp1Regs->localX) >> disp_xscale;
    vertices[2].y = (C_y + Vdp1Regs->localY) >> disp_yscale;
    vertices[2].z = 0;
    vertices[3].u = (flip_h ? 0 : width);
    vertices[3].v = (flip_v ? 0 : height);
    vertices[3].color = color32;
    vertices[3].x = (D_x + Vdp1Regs->localX) >> disp_xscale;
    vertices[3].y = (D_y + Vdp1Regs->localY) >> disp_yscale;
    vertices[3].z = 0;

    /* Queue the draw operation */
    vdp1_queue_render(priority, texture_key, (cmd->CMDPMOD & 7) == 3,
                      GU_TRIANGLE_STRIP, GU_TEXTURE_16BIT | GU_COLOR_8888
                                       | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                      4, NULL, vertices);
}

/*************************************************************************/

/**
 * vdp1_get_cmd_color:  Return the 32-bit color value specified by a VDP1
 * line command.
 *
 * [Parameters]
 *     cmd: VDP1 command pointer
 * [Return value]
 *     32-bit color value
 */
static uint32_t vdp1_get_cmd_color(vdp1cmd_struct *cmd)
{
    uint32_t alpha;
    if ((cmd->CMDPMOD & 7) == 3) {
        alpha = 0x80;
    } else {
        alpha = 0xFF;
    }
    const uint16_t color16 = cmd->CMDCOLR;
    uint32_t color32;
    if (color16 == 0) {
        color32 = 0;
    } else if (color16 & 0x8000) {
        color32 = adjust_color_16_32(color16, vdp1_rofs, vdp1_gofs, vdp1_bofs);
    } else {
        color32 = adjust_color_32_32(global_clut_32[color16 & 0x7FF],
                                     vdp1_rofs, vdp1_gofs, vdp1_bofs);
    }
    return color32 | alpha<<24;
}

/*-----------------------------------------------------------------------*/

/**
 * vdp1_get_cmd_color_pri:  Return the 32-bit color value and priority
 * specified by a VDP1 polygon command.
 *
 * [Parameters]
 *              cmd: VDP1 command pointer
 *     priority_ret: Pointer to variable to receive priority value
 * [Return value]
 *     32-bit color value
 */
static uint32_t vdp1_get_cmd_color_pri(vdp1cmd_struct *cmd, int *priority_ret)
{
    uint16_t color16 = cmd->CMDCOLR;
    if (cmd->CMDCOLR & 0x8000) {
        *priority_ret = Vdp2Regs->PRISA & 7;
    } else {
        *priority_ret = 0;  // Default if not set by SPCTL
        vdp1_process_sprite_color(color16, priority_ret);
    }
    uint32_t alpha;
    if ((cmd->CMDPMOD & 7) == 3) {
        alpha = 0x80;
    } else {
        alpha = 0xFF;
    }
    uint32_t color32;
    if (color16 == 0) {
        color32 = 0;
    } else if (color16 & 0x8000) {
        color32 = adjust_color_16_32(color16, vdp1_rofs, vdp1_gofs, vdp1_bofs);
    } else {
        color32 = adjust_color_32_32(global_clut_32[color16 & 0x7FF],
                                     vdp1_rofs, vdp1_gofs, vdp1_bofs);
    }
    return color32 | alpha<<24;
}

/*-----------------------------------------------------------------------*/

/**
 * vdp1_process_sprite_color:  Return the RGB color and priority value
 * selected by the given color register value and the VDP2 SPCTL register.
 * Derived from vidshared.h:Vdp1ProcessSpritePixel, but ignores the unused
 * colorcalc and shadow parameters and performs the color table lookup to
 * return a 32-bit color value.
 *
 * [Parameters]
 *          color16: 16-bit color register value
 *     priority_ret: Pointer to variable to receive priority value
 * [Return value]
 *     32-bit color value
 */
static uint32_t vdp1_process_sprite_color(uint16_t color16, int *priority_ret)
{
    switch (Vdp2Regs->SPCTL & 0x0F) {
      case  0:
        *priority_ret = (color16 & 0xC000) >> 14;
        return global_clut_32[color16 & 0x7FF];
      case  1:
        *priority_ret = (color16 & 0xE000) >> 13;
        return global_clut_32[color16 & 0x7FF];
      case  2:
        *priority_ret = (color16 & 0x8000) >> 15;
        return global_clut_32[color16 & 0x7FF];
      case  3:
        *priority_ret = (color16 & 0x6000) >> 13;
        return global_clut_32[color16 & 0x7FF];
      case  4:
        *priority_ret = (color16 & 0x7000) >> 12;
        return global_clut_32[color16 & 0x3FF];
      case  5:
        *priority_ret = (color16 & 0x7000) >> 12;
        return global_clut_32[color16 & 0x7FF];
      case  6:
        *priority_ret = (color16 & 0x7000) >> 12;
        return global_clut_32[color16 & 0x3FF];
      case  7:
        *priority_ret = (color16 & 0x7000) >> 12;
        return global_clut_32[color16 & 0x1FF];
      case  8:
        *priority_ret = (color16 & 0x0080) >>  7;
        return global_clut_32[color16 & 0x7F];
      case  9:
        *priority_ret = (color16 & 0x0080) >>  7;
        return global_clut_32[color16 & 0x3F];
      case 10:
        *priority_ret = (color16 & 0x00C0) >>  6;
        return global_clut_32[color16 & 0x3F];
      case 11:
        *priority_ret = 0;
        return global_clut_32[color16 & 0x3F];
      case 12:
        *priority_ret = (color16 & 0x0080) >>  7;
        return global_clut_32[color16 & 0xFF];
      case 13:
        *priority_ret = (color16 & 0x0080) >>  7;
        return global_clut_32[color16 & 0xFF];
      case 14:
        *priority_ret = (color16 & 0x00C0) >>  6;
        return global_clut_32[color16 & 0xFF];
      case 15:
        *priority_ret = 0;
        return global_clut_32[color16 & 0xFF];
      default:
        *priority_ret = 0; /* impossible */
        return 0;
    }
}

/*************************************************************************/

/**
 * vdp1_cache_sprite_texture:  Cache the sprite texture designated by the
 * given VDP1 command.
 *
 * [Parameters]
 *              cmd: VDP1 command pointer
 *            width: Sprite width (pixels; passed in to avoid recomputation)
 *           height: Sprite height (pixels; passed in to avoid recomputation)
 *     priority_ret: Pointer to variable to receive priority value
 * [Return value]
 *     Cached texture key, or zero on error
 */
static uint32_t vdp1_cache_sprite_texture(
    vdp1cmd_struct *cmd, int width, int height, int *priority_ret)
{
    int regnum = 0;  // Default value
    if (!((Vdp2Regs->SPCTL & 0x20) && (cmd->CMDCOLR & 0x8000))) {
        uint16_t color16 = cmd->CMDCOLR;
        int is_lut = 1;
        if ((cmd->CMDPMOD>>3 & 7) == 1) {
            uint32_t addr = cmd->CMDSRCA << 3;
            uint8_t pixel = T1ReadByte(Vdp1Ram, addr);
            uint32_t colortable = cmd->CMDCOLR << 3;
            uint16_t value = T1ReadWord(Vdp1Ram, (pixel>>4)*2 + colortable);
            if (value & 0x8000) {
                is_lut = 0;
            } else {
                color16 = value;
            }
        }
        if (is_lut) {
            vdp1_process_sprite_color(color16, &regnum);
        }
    }
    *priority_ret = ((uint8_t *)&Vdp2Regs->PRISA)[regnum] & 0x7;

    /* Cache the texture data and return the key */
    
    return texcache_cache_sprite(cmd->CMDSRCA, cmd->CMDPMOD, cmd->CMDCOLR,
                                 width, height, vdp1_rofs, vdp1_gofs,
                                 vdp1_bofs);

    /* FIXME: Mesh and gouraud shading are currently unsupported (see
     * vidogl.c for details on these) */
}

/*************************************************************************/

/**
 * vdp1_queue_render:  Queue a render operation from a VDP1 command.
 *
 * [Parameters]
 *        priority: Saturn display priority (0-7)
 *     texture_key: Texture key for sprites, zero for untextured operations
 *     translucent: Nonzero to render at 50% opacity, zero for 100% opacity
 *       primitive,
 *     vertex_type,
 *           count,
 *         indices,
 *        vertices: Parameters to pass to sceGuDrawArray()
 * [Return value]
 *     None
 */
static inline void vdp1_queue_render(
    int priority, uint32_t texture_key, int translucent, int primitive,
    int vertex_type, int count, const void *indices, const void *vertices)
{
    /* Expand the queue if necessary */
    if (UNLIKELY(vdp1_queue[priority].len >= vdp1_queue[priority].size)) {
        const int newsize = vdp1_queue[priority].size + VDP1_QUEUE_EXPAND_SIZE;
        VDP1RenderData * const newqueue = realloc(vdp1_queue[priority].queue,
                                                  newsize * sizeof(*newqueue));
        if (UNLIKELY(!newqueue)) {
            DMSG("Failed to expand priority %d queue to %d entries",
                 priority, newsize);
            return;
        }
        vdp1_queue[priority].queue = newqueue;
        vdp1_queue[priority].size  = newsize;
    }

    /* Record the data passed in */
    const int index = vdp1_queue[priority].len++;
    VDP1RenderData * const entry = &vdp1_queue[priority].queue[index];
    entry->texture_key = texture_key;
    entry->translucent = translucent;
    entry->primitive   = primitive;
    entry->vertex_type = vertex_type;
    entry->count       = count;
    entry->indices     = indices;
    entry->vertices    = vertices;
}

/*-----------------------------------------------------------------------*/

/**
 * vdp1_run_queue:  Run the rendering queue for the given priority level.
 *
 * [Parameters]
 *     priority: Priority level to run
 * [Return value]
 *     None
 */
static void vdp1_run_queue(int priority)
{
    int in_texture_mode = 0;  // Remember which mode we're in
    VDP1RenderData *entry = vdp1_queue[priority].queue;
    VDP1RenderData * const queue_top = entry + vdp1_queue[priority].len;

    /* Set up the list ourselves to save time */
    int maxcmds = vdp1_queue[priority].len * 15 + 4;
    uint32_t *list = sceGuGetMemory(4 * maxcmds);
    if (!list) {
        DMSG("Failed to get list memory (%d bytes)", 4 * maxcmds);
        return;
    }
    sceGuStart(GU_CALL, list);

    for (; entry < queue_top; entry++) {
        if (entry->texture_key) {
            texcache_load_sprite(entry->texture_key);
            if (!in_texture_mode) {
                sceGuEnable(GU_TEXTURE_2D);
                if (entry->translucent) {
                    sceGuAmbientColor(0x80FFFFFF);
                }
                in_texture_mode = 1;
            }
        } else if (in_texture_mode) {
            sceGuAmbientColor(0xFFFFFFFF);
            sceGuDisable(GU_TEXTURE_2D);
            in_texture_mode = 0;
        }
        sceGuDrawArray(entry->primitive, entry->vertex_type,
                       entry->count, entry->indices, entry->vertices);
    }
    if (in_texture_mode) {
        sceGuAmbientColor(0xFFFFFFFF);
        sceGuDisable(GU_TEXTURE_2D);
    }

    sceGuFinish();
    sceGuCallList(list);
}

/*************************************************************************/
/*************************************************************************/

/**
 * vdp2_get_color_offset:  Calculate the color offsets to use for the
 * specified CLOFEN/CLOFSL bit.
 *
 * [Parameters]
 *         mask: 1 << bit number to check
 *     rofs_ret: Pointer to variable to store red offset in
 *     gofs_ret: Pointer to variable to store green offset in
 *     bofs_ret: Pointer to variable to store blue offset in
 * [Return value]
 *     None
 */
static inline void vdp2_get_color_offsets(uint16_t mask, int32_t *rofs_ret,
                                          int32_t *gofs_ret, int32_t *bofs_ret)
{
    if (Vdp2Regs->CLOFEN & mask) {  // CoLor OFfset ENable
        /* Offsets are 9-bit signed values */
        if (Vdp2Regs->CLOFSL & mask) {  // CoLor OFfset SeLect
            *rofs_ret = ((int32_t)Vdp2Regs->COBR << 23) >> 23;
            *gofs_ret = ((int32_t)Vdp2Regs->COBG << 23) >> 23;
            *bofs_ret = ((int32_t)Vdp2Regs->COBB << 23) >> 23;
        } else {
            *rofs_ret = ((int32_t)Vdp2Regs->COAR << 23) >> 23;
            *gofs_ret = ((int32_t)Vdp2Regs->COAG << 23) >> 23;
            *bofs_ret = ((int32_t)Vdp2Regs->COAB << 23) >> 23;
        }
    } else {
        /* No color offset */
        *rofs_ret = *gofs_ret = *bofs_ret = 0;
    }
}

/*************************************************************************/

/**
 * vdp2_draw_bg:  Draw the screen background.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
static void vdp2_draw_bg(void)
{
    uint32_t address = ((Vdp2Regs->BKTAU & 7) << 16 | Vdp2Regs->BKTAL) << 1;
    if (!(Vdp2Regs->VRSIZE & 0x8000)) {
        address &= 0x7FFFF;
    }

    struct {uint32_t color; int16_t x, y, z, pad;} *vertices;

    if (Vdp2Regs->BKTAU & 0x8000) {
        /* Distinct color for each line */
        int num_vertices, y;
        if (disp_height > 272) {
            /* For interlaced screens, we average the colors of each two
             * adjacent lines */
            num_vertices = 2*(disp_height/2);
            vertices = sceGuGetMemory(sizeof(*vertices) * num_vertices);
            if (!vertices) {
                DMSG("Failed to get vertex memory (%d bytes)",
                     sizeof(*vertices) * (2*disp_height));
                return;
            }
            for (y = 0; y+1 < disp_height; y += 2, address += 4) {
                uint16_t rgb0 = T1ReadWord(Vdp2Ram, address);
                uint32_t r0 = (rgb0 & 0x001F) << 3;
                uint32_t g0 = (rgb0 & 0x03E0) >> 2;
                uint32_t b0 = (rgb0 & 0x7C00) >> 7;
                uint16_t rgb1 = T1ReadWord(Vdp2Ram, address);
                uint32_t r1 = (rgb1 & 0x001F) << 3;
                uint32_t g1 = (rgb1 & 0x03E0) >> 2;
                uint32_t b1 = (rgb1 & 0x7C00) >> 7;
                uint32_t color = ((r0+r1+1)/2) | ((g0+g1+1)/2) <<  8
                                               | ((b0+b1+1)/2) << 16;
                vertices[y+0].color = 0xFF000000 | color;
                vertices[y+0].x = 0;
                vertices[y+0].y = y/2;
                vertices[y+0].z = 0;
                vertices[y+1].color = 0xFF000000 | color;
                vertices[y+1].x = disp_width >> disp_xscale;
                vertices[y+1].y = y/2;
                vertices[y+1].z = 0;
            }
        } else {
            num_vertices = 2*disp_height;
            vertices = sceGuGetMemory(sizeof(*vertices) * num_vertices);
            if (!vertices) {
                DMSG("Failed to get vertex memory (%d bytes)",
                     sizeof(*vertices) * (2*disp_height));
                return;
            }
            for (y = 0; y < disp_height; y++, address += 2) {
                uint16_t rgb = T1ReadWord(Vdp2Ram, address);
                uint32_t r = (rgb & 0x001F) << 3;
                uint32_t g = (rgb & 0x03E0) >> 2;
                uint32_t b = (rgb & 0x7C00) >> 7;
                vertices[y*2+0].color = 0xFF000000 | r | g<<8 | b<<16;
                vertices[y*2+0].x = 0;
                vertices[y*2+0].y = y;
                vertices[y*2+0].z = 0;
                vertices[y*2+1].color = 0xFF000000 | r | g<<8 | b<<16;
                vertices[y*2+1].x = disp_width >> disp_xscale;
                vertices[y*2+1].y = y;
                vertices[y*2+1].z = 0;
            }
        }
        sceGuDrawArray(GU_LINES,
                       GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                       num_vertices, NULL, vertices);
    } else {
        /* Single color for the whole screen */
        vertices = sceGuGetMemory(sizeof(*vertices) * 2);
        if (!vertices) {
            DMSG("Failed to get vertex memory (%d bytes)",
                 sizeof(*vertices) * 2);
            return;
        }
        uint16_t rgb = T1ReadWord(Vdp2Ram, address);
        uint32_t r = (rgb & 0x001F) << 3;
        uint32_t g = (rgb & 0x03E0) >> 2;
        uint32_t b = (rgb & 0x7C00) >> 7;
        vertices[0].color = 0xFF000000 | r | g<<8 | b<<16;
        vertices[0].x = 0;
        vertices[0].y = 0;
        vertices[0].z = 0;
        vertices[1].color = 0xFF000000 | r | g<<8 | b<<16;
        vertices[1].x = disp_width >> disp_xscale;
        vertices[1].y = disp_height >> disp_yscale;
        vertices[1].z = 0;
        sceGuDrawArray(GU_SPRITES,
                       GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                       2, NULL, vertices);
    }
}

/*************************************************************************/

/**
 * vdp2_draw_graphics:  Draw a single VDP2 background graphics layer.
 *
 * [Parameters]
 *     layer: Background graphics layer (BG_* constant)
 * [Return value]
 *     None
 */
static void vdp2_draw_graphics(int layer)
{
    vdp2draw_struct info;
    clipping_struct clip[2];

    /* Is background enabled? */
    if (!(Vdp2Regs->BGON & Vdp2External.disptoggle & (1 << layer))) {
        return;
    }
    if (layer == BG_RBG0) {
        DMSG("RBG0 not yet supported");
        return;
    }

    /* Find out whether it's a bitmap or not */
    switch (layer) {
      case BG_NBG0: info.isbitmap = Vdp2Regs->CHCTLA & 0x0002; break;
      case BG_NBG1: info.isbitmap = Vdp2Regs->CHCTLA & 0x0200; break;
      default:      info.isbitmap = 0; break;
    }

    /* Determine color-related data */
    info.transparencyenable = !(Vdp2Regs->BGON & (0x100 << layer));
    /* FIXME: specialprimode is not actually supported by the map drawing
     * functions */
    info.specialprimode = (Vdp2Regs->SFPRMD >> (2*layer)) & 3;
    switch (layer) {
      case BG_NBG0:
        info.colornumber = (Vdp2Regs->CHCTLA & 0x0070) >>  4;
        break;
      case BG_NBG1:
        info.colornumber = (Vdp2Regs->CHCTLA & 0x3000) >> 12;
        break;
      case BG_NBG2:
        info.colornumber = (Vdp2Regs->CHCTLB & 0x0002) >>  1;
        break;
      case BG_NBG3:
        info.colornumber = (Vdp2Regs->CHCTLB & 0x0020) >>  5;
        break;
      case BG_RBG0:
        info.colornumber = (Vdp2Regs->CHCTLB & 0x7000) >> 12;
        break;
    }
    if (Vdp2Regs->CCCTL & (1 << layer)) {
        const uint8_t *ptr = (const uint8_t *)&Vdp2Regs->CCRNA;
        info.alpha = ((~ptr[layer] & 0x1F) << 3) + 7;
    } else {
        info.alpha = 0xFF;
    }
    if (layer == BG_RBG0) {
        info.coloroffset = (Vdp2Regs->CRAOFB & 7) << 8;
    } else {
        info.coloroffset = ((Vdp2Regs->CRAOFA >> (4*layer)) & 7) << 8;
    }
    vdp2_get_color_offsets(1 << layer, (int32_t *)&info.cor,
                           (int32_t *)&info.cog, (int32_t *)&info.cob);

    /* Determine tilemap/bitmap size and display offset */
    if (info.isbitmap) {
        ReadBitmapSize(&info, Vdp2Regs->CHCTLA >> (2 + layer*8), 0x3);
        info.charaddr = ((Vdp2Regs->MPOFN >> (4*layer)) & 7) << 16;
        info.paladdr = (Vdp2Regs->BMPNA >> (8*layer)) & 7;
        info.flipfunction = 0;
        info.specialfunction = 0;
        switch (layer) {
          case BG_NBG0:
            info.x = - ((Vdp2Regs->SCXIN0 & 0x7FF) % info.cellw);
            info.y = - ((Vdp2Regs->SCYIN0 & 0x7FF) % info.cellh);
            break;
          case BG_NBG1:
            info.x = - ((Vdp2Regs->SCXIN1 & 0x7FF) % info.cellw);
            info.y = - ((Vdp2Regs->SCYIN1 & 0x7FF) % info.cellh);
            break;
          default:
            DMSG("info.isbitmap set for invalid layer %d", layer);
            return;
        }
    } else {
        info.mapwh = (layer <= BG_NBG3 ? 2 : 4);
        ReadPlaneSize(&info, Vdp2Regs->PLSZ >> (2*layer));
        switch (layer) {
          case BG_NBG0:
            info.x = - ((Vdp2Regs->SCXIN0 & 0x7FF) % (512 * info.planew));
            info.y = - ((Vdp2Regs->SCYIN0 & 0x7FF) % (512 * info.planeh));
            ReadPatternData(&info, Vdp2Regs->PNCN0, Vdp2Regs->CHCTLA & 0x0001);
            break;
          case BG_NBG1:
            info.x = - ((Vdp2Regs->SCXIN1 & 0x7FF) % (512 * info.planew));
            info.y = - ((Vdp2Regs->SCYIN1 & 0x7FF) % (512 * info.planeh));
            ReadPatternData(&info, Vdp2Regs->PNCN1, Vdp2Regs->CHCTLA & 0x0100);
            break;
          case BG_NBG2:
            info.x = - ((Vdp2Regs->SCXN2 & 0x07FF) % (512 * info.planew));
            info.y = - ((Vdp2Regs->SCYN2 & 0x07FF) % (512 * info.planeh));
            ReadPatternData(&info, Vdp2Regs->PNCN2, Vdp2Regs->CHCTLB & 0x0001);
            break;
          case BG_NBG3:
            info.x = - ((Vdp2Regs->SCXN3 & 0x07FF) % (512 * info.planew));
            info.y = - ((Vdp2Regs->SCYN3 & 0x07FF) % (512 * info.planeh));
            ReadPatternData(&info, Vdp2Regs->PNCN3, Vdp2Regs->CHCTLB & 0x0010);
            break;
          case BG_RBG0:
            ReadPatternData(&info, Vdp2Regs->PNCR,  Vdp2Regs->CHCTLB & 0x0100);
            break;
        }
    }

    /* Determine coordinate scaling */
    // FIXME: scaled graphics may be distorted because integers are used
    // for vertex coordinates
    switch (layer) {
      case BG_NBG0:
        info.coordincx = 65536.0f / (Vdp2Regs->ZMXN0.all & 0x7FF00 ?: 65536);
        info.coordincy = 65536.0f / (Vdp2Regs->ZMYN0.all & 0x7FF00 ?: 65536);
        break;
      case BG_NBG1:
        info.coordincx = 65536.0f / (Vdp2Regs->ZMXN1.all & 0x7FF00 ?: 65536);
        info.coordincy = 65536.0f / (Vdp2Regs->ZMYN1.all & 0x7FF00 ?: 65536);
        break;
      default:
        info.coordincx = info.coordincy = 1;
        break;
    }
    if (disp_xscale == 1) {
        info.coordincx /= 2;
    }
    if (disp_yscale == 1) {
        info.coordincy /= 2;
    }

    /* Get clipping data */
    info.wctl = ((uint8_t *)&Vdp2Regs->WCTLA)[layer];
    clip[0].xstart = 0; clip[0].xend = disp_width;
    clip[0].ystart = 0; clip[0].yend = disp_height;
    clip[1].xstart = 0; clip[1].xend = disp_width;
    clip[1].ystart = 0; clip[1].yend = disp_height;
    ReadWindowData(info.wctl, clip);

    info.priority = bg_priority[layer];
    switch (layer) {
        case BG_NBG0: info.PlaneAddr = (void *)Vdp2NBG0PlaneAddr; break;
        case BG_NBG1: info.PlaneAddr = (void *)Vdp2NBG1PlaneAddr; break;
        case BG_NBG2: info.PlaneAddr = (void *)Vdp2NBG2PlaneAddr; break;
        case BG_NBG3: info.PlaneAddr = (void *)Vdp2NBG3PlaneAddr; break;
        default:      return;
    }
    info.patternpixelwh = 8 * info.patternwh;
    info.draww = (int)((float)(disp_width  >> disp_xscale) / info.coordincx);
    info.drawh = (int)((float)(disp_height >> disp_yscale) / info.coordincy);

    /* Select a rendering function based on the tile layout and format */
    void (*draw_map_func)(vdp2draw_struct *info, clipping_struct *clip);
    if (info.isbitmap) {
        switch (layer) {
          case BG_NBG0:
            if ((Vdp2Regs->SCRCTL & 7) == 7) {
                DMSG("WARNING: line scrolling not supported");
            }
            /* fall through */
          case BG_NBG1:
            switch (info.colornumber) {
              case 1:
                draw_map_func = &vdp2_draw_bitmap_t8;
                break;
              default:
                draw_map_func = &vdp2_draw_bitmap;
                break;
            }
            break;
          default:
            DMSG("info.isbitmap set for invalid layer %d", layer);
            return;
        }
    } else if (info.patternwh == 2) {
        if (info.colornumber == 1) {
            draw_map_func = &vdp2_draw_map_16x16_t8;
        } else {
            draw_map_func = &vdp2_draw_map_16x16;
        }
    } else {
        if (info.colornumber == 1) {
            draw_map_func = &vdp2_draw_map_8x8_t8;
        } else {
            draw_map_func = &vdp2_draw_map_8x8;
        }
    }

    /* Set up for rendering */
    sceGuEnable(GU_TEXTURE_2D);
    sceGuAmbientColor(info.alpha<<24 | 0xFFFFFF);

    /* Render the graphics */
    (*draw_map_func)(&info, clip);

    /* All done */
    sceGuAmbientColor(0xFFFFFFFF);
    sceGuDisable(GU_TEXTURE_2D);
}

/*-----------------------------------------------------------------------*/

/**
 * vdp2_draw_map_8x8:  Draw a graphics layer composed of 8x8 patterns of
 * any format.
 *
 * [Parameters]
 *     info: Graphics layer data
 *     clip: Clipping window data
 * [Return value]
 *     None
 */
static void vdp2_draw_map_8x8(vdp2draw_struct *info, clipping_struct *clip)
{
    /* Allocate vertex memory */
    GET_VERTICES(2);

    /* Loop through tiles */
    TILE_LOOP_BEGIN(8) {
        GET_FLIP_PRI;
        SET_VERTICES(info->x * info->coordincx, info->y * info->coordincy,
                     8 * info->coordincx, 8 * info->coordincy);
        texcache_load_tile(info->charaddr, info->colornumber,
                           info->transparencyenable,
                           info->coloroffset, info->paladdr << 4,
                           info->cor, info->cog, info->cob);
        sceGuDrawArray(GU_SPRITES,
                       GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                       2, NULL, vertices);
        vertices += 2;
    } TILE_LOOP_END;
}

/*-----------------------------------------------------------------------*/

/**
 * vdp2_draw_map_8x8_t8:  Draw a graphics layer composed of 8-bit indexed
 * color 8x8 patterns.
 *
 * [Parameters]
 *     info: Graphics layer data
 *     clip: Clipping window data
 * [Return value]
 *     None
 */
static void vdp2_draw_map_8x8_t8(vdp2draw_struct *info, clipping_struct *clip)
{
    /* Allocate vertex memory and perform other initialization */
    GET_VERTICES(4);  // Need 2 sprites to draw each tile
    GET_LIST(17);
    INIT_T8_PALETTE;
    INIT_T8_TEXTURE;
    INIT_T8_VERTEX;
    INIT_T8_WORK_BUFFER;

    /* Loop through tiles */
    TILE_LOOP_BEGIN(8) {
        GET_FLIP_PRI_T8;
        UPDATE_T8_PALETTE;

        /* Set up vertices and draw the tile */
        const uint8_t *src = &Vdp2Ram[info->charaddr];
        int yofs = ((uintptr_t)src & 63) / 16;
        src = (const uint8_t *)((uintptr_t)src & ~63);
        SET_VERTICES_T8_EVEN(info->x * info->coordincx,
                             info->y * info->coordincy,
                             8 * info->coordincx, 8 * info->coordincy);
        SET_VERTICES_T8_ODD(info->x * info->coordincx,
                            info->y * info->coordincy,
                             8 * info->coordincx, 8 * info->coordincy);
        LOAD_T8_TILE;
        SET_T8_BUFFER_0;
        RENDER_T8_EVEN;
        SET_T8_BUFFER_1;
        RENDER_T8_ODD;
        vertices += 4;
    } TILE_LOOP_END;

    /* Restore stride to normal */
    //sceGuDrawBuffer(GU_PSM_8888, work_buffer_0, DISPLAY_STRIDE);
    *listptr++ = 0x9C000000 | ((uintptr_t)work_buffer_0 & 0x00FFFFFF);
    *listptr++ = 0x9D000000 | ((uintptr_t)work_buffer_0 & 0x3F000000) >> 8
                            | DISPLAY_STRIDE;

    /* Run the list */
    //sceGuFinish();
    *listptr = 0x0B000000;
    sceKernelDcacheWritebackAll();
    sceGuCallList(list);
}

/*-----------------------------------------------------------------------*/

/**
 * vdp2_draw_map_16x16:  Draw a graphics layer composed of 16x16 patterns
 * of any format.
 *
 * [Parameters]
 *     info: Graphics layer data
 *     clip: Clipping window data
 * [Return value]
 *     None
 */
static void vdp2_draw_map_16x16(vdp2draw_struct *info, clipping_struct *clip)
{
    /* Determine tile data size */
    GET_TILEDATASIZE;

    /* Allocate vertex memory */
    GET_VERTICES(2);

    /* Loop through tiles */
    TILE_LOOP_BEGIN(16) {
        GET_FLIP_PRI;

        /* Draw 4 consecutive tiles in a 2x2 pattern */
        int tilenum;
        for (tilenum = 0; tilenum < 4; tilenum++) {
            const int tilex = info->x + (8 * (tilenum % 2));
            const int tiley = info->y + (8 * (tilenum / 2));
            SET_VERTICES(tilex * info->coordincx, tiley * info->coordincy,
                         8 * info->coordincx, 8 * info->coordincy);
            texcache_load_tile(info->charaddr, info->colornumber,
                               info->transparencyenable,
                               info->coloroffset, info->paladdr << 4,
                               info->cor, info->cog, info->cob);
            sceGuDrawArray(GU_SPRITES,
                           GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                           2, NULL, vertices);
            vertices += 2;
            info->charaddr += tiledatasize;
        }
    } TILE_LOOP_END;
}

/*-----------------------------------------------------------------------*/

/**
 * vdp2_draw_map_16x16_t8:  Draw a graphics layer composed of 8-bit indexed
 * color 16x16 patterns.
 *
 * [Parameters]
 *     info: Graphics layer data
 *     clip: Clipping window data
 * [Return value]
 *     None
 */
static void vdp2_draw_map_16x16_t8(vdp2draw_struct *info,
                                   clipping_struct *clip)
{
    /* Are we interlaced? */
    const int interlaced = (disp_height > 272);

    /* Set stuff up */
    GET_VERTICES(interlaced ? 2 : 4);
    GET_LIST(17);
    INIT_T8_PALETTE;
    INIT_T8_TEXTURE;
    INIT_T8_VERTEX;
    INIT_T8_WORK_BUFFER;

    /* Loop through tiles */
    TILE_LOOP_BEGIN(16) {
        GET_FLIP_PRI_T8;
        UPDATE_T8_PALETTE;

        const uint8_t *src = &Vdp2Ram[info->charaddr];
        int yofs = ((uintptr_t)src & 63) / 16;
        src = (const uint8_t *)((uintptr_t)src & ~63);
        int tilenum;
        for (tilenum = 0; tilenum < 4; tilenum++) {
            const int tilex = info->x + (8 * ((tilenum % 2) ^ (info->flipfunction & 1)));
            const int tiley = info->y + (8 * ((tilenum / 2) ^ ((info->flipfunction & 2) >> 1)));
            SET_VERTICES_T8_EVEN(tilex * info->coordincx,
                                 tiley * info->coordincy,
                                 8 * info->coordincx, 8 * info->coordincy);
            if (!interlaced) {
                SET_VERTICES_T8_ODD(tilex * info->coordincx,
                                    tiley * info->coordincy,
                                    8 * info->coordincx, 8 * info->coordincy);
            } else {
                vertices[0].y *= 2;
                vertices[1].y *= 2;
            }

            LOAD_T8_TILE;
            if (!interlaced) {
                SET_T8_BUFFER_0;
            }
            RENDER_T8_EVEN;
            if (!interlaced) {
                SET_T8_BUFFER_1;
                RENDER_T8_ODD;
                vertices += 4;
            } else {
                /* Interlaced, so drop odd lines of tile */
                vertices += 2;
            }

            src += 8*8*1;
        }
    } TILE_LOOP_END;

    if (!interlaced) {
        /* Restore stride to normal */
        *listptr++ = 0x9C000000 | ((uintptr_t)work_buffer_0 & 0x00FFFFFF);
        *listptr++ = 0x9D000000 | ((uintptr_t)work_buffer_0 & 0x3F000000) >> 8
                                | DISPLAY_STRIDE;
    }

    /* Run the list */
    *listptr = 0x0B000000;
    sceKernelDcacheWritebackAll();
    sceGuCallList(list);
}

/*-----------------------------------------------------------------------*/

/**
 * vdp2_calc_pattern_address:  Calculate the address and associated data of
 * the next pattern.
 *
 * [Parameters]
 *     info: Graphics layer data
 * [Return value]
 *     None
 */
static inline void vdp2_calc_pattern_address(vdp2draw_struct *info)
{
    if (info->patterndatasize == 1) {
        uint16_t data = T1ReadWord(Vdp2Ram, info->addr);
        info->specialfunction = (info->supplementdata >> 9) & 1;
        if (info->colornumber == 0) {  // 4bpp
            info->paladdr = (data & 0xF000) >> 12
                          | (info->supplementdata & 0xE0) >> 1;
        } else {  // >=8bpp
            info->paladdr = (data & 0x7000) >> 8;
        }
        if (!info->auxmode) {
            info->flipfunction = (data & 0xC00) >> 10;
            if (info->patternwh == 1) {
                info->charaddr = (info->supplementdata & 0x1F) << 10
                               | (data & 0x3FF);
            } else {
                info->charaddr = (info->supplementdata & 0x1C) << 10
                               | (data & 0x3FF) << 2
                               | (info->supplementdata & 0x3);
            }
        } else {
            info->flipfunction = 0;
            if (info->patternwh == 1) {
                info->charaddr = (info->supplementdata & 0x1C) << 10
                               | (data & 0xFFF);
            } else {
                info->charaddr = (info->supplementdata & 0x10) << 10
                               | (data & 0xFFF) << 2
                               | (info->supplementdata & 0x3);
            }
        }
    } else {  // patterndatasize == 2
        uint16_t data1 = T1ReadWord(Vdp2Ram, info->addr+0);
        uint16_t data2 = T1ReadWord(Vdp2Ram, info->addr+2);
        info->charaddr = data2 & 0x7FFF;
        info->flipfunction = (data1 & 0xC000) >> 14;
        info->paladdr = data1 & 0x007F;
        info->specialfunction = (data1 & 0x2000) >> 13;
    }

    if (!(Vdp2Regs->VRSIZE & 0x8000)) {
        info->charaddr &= 0x3FFF;
    }
    info->charaddr <<= 5;
}

/*-----------------------------------------------------------------------*/

/**
 * vdp2_gen_t8_clut:  Generate a 256-color color table for an 8-bit indexed
 * tile given the base color index and color offset values.  Helper routine
 * for the T8 map drawing functions.
 *
 * [Parameters]
 *           color_base: Base color index
 *            color_ofs: Color offset (OR'd with pixel value)
 *          transparent: Nonzero if color index 0 should be transparent
 *     rofs, gofs, bofs: Red/green/blue adjustment values
 * [Return value]
 *     Allocated color table pointer (NULL on failure to allocate memory)
 */
static inline uint32_t *vdp2_gen_t8_clut(
    int color_base, int color_ofs, int transparent,
    int rofs, int gofs, int bofs)
{
    uint32_t *clut = sceGuGetMemory(256*4 + 60);
    if (UNLIKELY(!clut)) {
        DMSG("Failed to allocate memory for palette");
        return NULL;
    }
    clut = (uint32_t *)(((uintptr_t)clut + 63) & 0x3FFFFFC0);
    int i;
    for (i = 0; i < 256; i++) {
        clut[i] = adjust_color_32_32(
            global_clut_32[color_base + (color_ofs | i)], rofs, gofs, bofs
        );
    }
    if (transparent) {
        clut[0] = 0x00000000;
    }
    return clut;
}

/*************************************************************************/

/**
 * vdp2_draw_bitmap:  Draw a graphics layer bitmap.
 *
 * [Parameters]
 *     info: Graphics layer data
 *     clip: Clipping window data
 * [Return value]
 *     None
 */
static void vdp2_draw_bitmap(vdp2draw_struct *info, clipping_struct *clip)
{
#if 0  // FIXME: these are not currently used
    /* Determine first whether the bitmap wraps around */
    int wrap_w = (info->x + info->cellw < disp_width);
    int wrap_h = (info->y + info->cellh < disp_height);
#endif

    /* Set up vertices */
    VertexUVXYZ *vertices = sceGuGetMemory(sizeof(*vertices) * 2);
    if (!vertices) {
        DMSG("Failed to get vertex memory (%d bytes)", sizeof(*vertices) * 2);
        return;
    }
    vertices[0].u = 0;
    vertices[0].v = 0;
    vertices[0].x = info->x * info->coordincx;
    vertices[0].y = info->y * info->coordincy;
    vertices[0].z = 0;
    vertices[1].u = info->cellw;
    vertices[1].v = info->cellh;
    vertices[1].x = (info->x + info->cellw) * info->coordincx;
    vertices[1].y = (info->y + info->cellh) * info->coordincy;
    vertices[1].z = 0;

    /* FIXME: only very basic clipping processing at the moment; see
     * vidsoft.c for more details on how this works */
    if ((info->wctl & 0x3) == 0x3) {
        vertices[0].x = clip[0].xstart;
        vertices[0].y = clip[0].ystart;
        vertices[1].x = clip[0].xend + 1;
        vertices[1].y = clip[0].yend + 1;
        vertices[1].u = (vertices[1].x - vertices[0].x) / info->coordincx;
        vertices[1].v = (vertices[1].y - vertices[0].y) / info->coordincy;
        /* Offset the bitmap address appropriately */
        const int bpp = (info->colornumber==4 ? 32 :
                         info->colornumber>=2 ? 16 :
                         info->colornumber==1 ?  8 : 4);
        const int xofs = clip[0].xstart - info->x;
        const int yofs = clip[0].ystart - info->y;
        info->charaddr += (yofs * info->cellw + xofs) * bpp / 8;
    }

    /* Draw the bitmap */
    texcache_load_bitmap(
        info->charaddr,
        vertices[1].u - vertices[0].u, vertices[1].v - vertices[0].v,
        info->cellw, info->colornumber, info->transparencyenable,
        info->coloroffset, info->paladdr << 4, info->cor, info->cog, info->cob
    );
    sceGuDrawArray(GU_SPRITES,
                   GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                   2, NULL, vertices);
}

/*-----------------------------------------------------------------------*/

/**
 * vdp2_draw_bitmap_t8:  Draw an 8-bit indexed graphics layer bitmap.
 *
 * [Parameters]
 *     info: Graphics layer data
 *     clip: Clipping window data
 * [Return value]
 *     None
 */
static void vdp2_draw_bitmap_t8(vdp2draw_struct *info, clipping_struct *clip)
{
#if 0  // FIXME: these are not currently used
    /* Determine first whether the bitmap wraps around */
    int wrap_w = (info->x + info->cellw < disp_width);
    int wrap_h = (info->y + info->cellh < disp_height);
#endif

    /* Set up vertices */
    // FIXME: this will break with a final (clipped) width above 512;
    // need to split the texture in half in that case
    VertexUVXYZ *vertices = sceGuGetMemory(sizeof(*vertices) * 2);
    if (!vertices) {
        DMSG("Failed to get vertex memory (%d bytes)", sizeof(*vertices) * 2);
        return;
    }
    vertices[0].u = 0;
    vertices[0].v = 0;
    vertices[0].x = info->x * info->coordincx;
    vertices[0].y = info->y * info->coordincy;
    vertices[0].z = 0;
    vertices[1].u = info->cellw;
    vertices[1].v = info->cellh;
    vertices[1].x = (info->x + info->cellw) * info->coordincx;
    vertices[1].y = (info->y + info->cellh) * info->coordincy;
    vertices[1].z = 0;

    /* FIXME: only very basic clipping processing at the moment; see
     * vidsoft.c for more details on how this works */
    if ((info->wctl & 0x3) == 0x3) {
        vertices[0].x = clip[0].xstart;
        vertices[0].y = clip[0].ystart;
        vertices[1].x = clip[0].xend + 1;
        vertices[1].y = clip[0].yend + 1;
        vertices[1].u = (vertices[1].x - vertices[0].x) / info->coordincx;
        vertices[1].v = (vertices[1].y - vertices[0].y) / info->coordincy;
        /* Offset the bitmap address appropriately */
        const int bpp = (info->colornumber==4 ? 32 :
                         info->colornumber>=2 ? 16 :
                         info->colornumber==1 ?  8 : 4);
        const int xofs = clip[0].xstart - info->x;
        const int yofs = clip[0].ystart - info->y;
        info->charaddr += (yofs * info->cellw + xofs) * bpp / 8;
    }

    /* Set up the color table */
    sceGuClutMode(GU_PSM_8888, 0, 0xFF, 0);
    void *ptr = vdp2_gen_t8_clut(
        info->coloroffset, info->paladdr<<4,
        info->transparencyenable, info->cor, info->cog, info->cob
    );
    if (LIKELY(ptr)) {
        sceGuClutLoad(256/8, ptr);
    }

    /* Draw the bitmap */
    sceGuTexMode(GU_PSM_T8, 0, 0, 0);
    sceGuTexImage(0, 512, 512, info->cellw, &Vdp2Ram[info->charaddr & 0x7FFFF]);
    sceGuDrawArray(GU_SPRITES,
                   GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                   2, NULL, vertices);
}

/*************************************************************************/
/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
