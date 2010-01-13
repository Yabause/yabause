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

#include "common.h"

#include "../vdp1.h"
#include "../vdp2.h"
#include "../vidshared.h"

#include "config.h"
#include "display.h"
#include "font.h"
#include "gu.h"
#include "texcache.h"
#include "psp-video.h"

/*************************************************************************/
/************************* Interface definition **************************/
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
/************************* Global and local data *************************/
/*************************************************************************/

/**** Exported data ****/

/* Color table generated from VDP2 color RAM */
__attribute__((aligned(64))) uint16_t global_clut_16[0x800];
__attribute__((aligned(64))) uint32_t global_clut_32[0x800];

/*-----------------------------------------------------------------------*/

/**** Internal data ****/

/*----------------------------------*/

/* Pending infoline text (malloc()ed, or NULL if none) and color */
static char *infoline_text;
static uint32_t infoline_color;

/*----------------------------------*/

/* Displayed width and height */
static unsigned int disp_width, disp_height;

/* Scale (right-shift) applied to X and Y coordinates */
static unsigned int disp_xscale, disp_yscale;

/* Total number of frames to skip before we draw the next one */
static unsigned int frames_to_skip;

/* Number of frames skipped so far since we drew the last one */
static unsigned int frames_skipped;

/* Current average frame rate (rolling average) */
static float average_fps;

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

/* Coordinate transformation parameters for rotated graphics layers */
typedef struct RotationParams_ RotationParams;
struct RotationParams_ {
    /* Transformation parameters read from VDP2 RAM (arranged for easy
     * VFPU loading, in case that ever becomes useful) */
    __attribute__((aligned(16))) float Xst;
    float Yst, Zst, pad0w;
    float deltaXst, deltaYst, pad1z, pad1w;
    float deltaX, deltaY, pad2z, pad2w;
    float A, B, C, pad3w;
    float D, E, F, pad4w;
    float Px, Py, Pz, pad5w;
    float Cx, Cy, Cz, pad6w;
    float Mx, My, pad7z, pad7w;
    float kx, ky, pad8z, pad8w;  // May be updated in coefficient table mode

    /* Computed transformation parameters */
    float Xp, Yp, pad9z, pad9w;  // May be updated in coefficient table mode
    float mat11, mat12, mat13, mat1_pad;
    float mat21, mat22, mat23, mat2_pad;

    /* Coefficient table parameters read from VDP2 RAM */
    float KAst;
    float deltaKAst;
    float deltaKAx;

    /* Coefficient table base address and flags */
    uint32_t coeftbladdr;
    uint8_t coefenab;
    uint8_t coefmode;
    uint8_t coefdatasize;  // Size of a single coefficient in bytes (2 or 4)
    uint8_t coefdatashift; // log2(coefdatasize)

    /* Miscellaneous parameters */
    uint8_t screenover;  // FIXME: What is this for?
};

/*----------------------------------*/

/* Vertex types */
typedef struct VertexUVXYZ_ {
    int16_t u, v;
    int16_t x, y, z;
} VertexUVXYZ;

/*************************************************************************/

/**** Local function declarations ****/

static void vdp1_draw_lines(vdp1cmd_struct *cmd, int poly);
static void vdp1_draw_quad(vdp1cmd_struct *cmd, int textured);
static uint32_t vdp1_convert_color(uint16_t color16, int textured,
                                   unsigned int CMDPMOD);
static uint32_t vdp1_get_cmd_color(vdp1cmd_struct *cmd);
static uint32_t vdp1_get_cmd_color_pri(vdp1cmd_struct *cmd, int textured,
                                       int *priority_ret);
static uint16_t vdp1_process_sprite_color(uint16_t color16, int *priority_ret);
static uint32_t vdp1_cache_sprite_texture(
    vdp1cmd_struct *cmd, int width, int height, int *priority_ret);
static inline void vdp1_queue_render(
    int priority, uint32_t texture_key, int primitive,
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
static void vdp2_draw_map_rotated(vdp2draw_struct *info,
                                  clipping_struct *clip);
static inline void vdp2_calc_pattern_address(vdp2draw_struct *info);
static inline uint32_t *vdp2_gen_t8_clut(
    int color_base, int color_ofs, int transparent,
    int rofs, int gofs, int bofs);

static void vdp2_draw_bitmap(vdp2draw_struct *info, clipping_struct *clip);
static void vdp2_draw_bitmap_t8(vdp2draw_struct *info, clipping_struct *clip);

static void vdp2_draw_map_rotated(vdp2draw_struct *info,
                                  clipping_struct *clip);
static void get_rotation_parameters(vdp2draw_struct *info, int which,
                                    RotationParams *param_ret);
static int get_rotation_coefficient(RotationParams *param, uint32_t address);
static void transform_coordinates(RotationParams *param, int x, int y,
                                  float *srcx_ret, float *srcy_ret);
static uint32_t rotation_get_pixel_t4(vdp2draw_struct *info,
                                      unsigned int pixelnum);
static uint32_t rotation_get_pixel_t8(vdp2draw_struct *info,
                                      unsigned int pixelnum);
static uint32_t rotation_get_pixel_t16(vdp2draw_struct *info,
                                       unsigned int pixelnum);
static uint32_t rotation_get_pixel_16(vdp2draw_struct *info,
                                      unsigned int pixelnum);
static uint32_t rotation_get_pixel_32(vdp2draw_struct *info,
                                      unsigned int pixelnum);
static uint32_t rotation_get_pixel_t4_adjust(vdp2draw_struct *info,
                                             unsigned int pixelnum);
static uint32_t rotation_get_pixel_t8_adjust(vdp2draw_struct *info,
                                             unsigned int pixelnum);
static uint32_t rotation_get_pixel_t16_adjust(vdp2draw_struct *info,
                                              unsigned int pixelnum);
static uint32_t rotation_get_pixel_16_adjust(vdp2draw_struct *info,
                                             unsigned int pixelnum);
static uint32_t rotation_get_pixel_32_adjust(vdp2draw_struct *info,
                                             unsigned int pixelnum);

static void *pspGuGetMemoryMerge(uint32_t size);

/*************************************************************************/
/************************** Tile-drawing macros **************************/
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
                                          * info->pagewh;               \
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
    /* We add 4 here to handle up to 15 pixels of partial tiles \
     * on each edge of the display area */                      \
    const int tilew = info->draww / 8 + 4;                      \
    const int tileh = info->drawh / 8 + 4;                      \
    const int nvertices = tilew * tileh * (vertspertile);       \
    VertexUVXYZ *vertices = pspGuGetMemoryMerge(sizeof(*vertices) * nvertices);

/* Initialize variables for 8-bit indexed tile palette handling.  There can
 * be up to 128 different palettes, selectable on a per-tile basis, so to
 * save time, we only create (on the fly) those which are actually used. */
#define INIT_T8_PALETTE                                         \
    uint32_t *palettes[128];                                    \
    memset(palettes, 0, sizeof(palettes));                      \
    int cur_palette = -1;  /* So it's always set the first time */ \
    guClutMode(GU_PSM_8888, 0, 0xFF, 0);

/* Set the texture pixel format for 8-bit indexed tiles. 16-byte-wide
 * textures are effectively swizzled already, so set the swizzled flag for
 * whatever speed boost it gives us. */
#define INIT_T8_TEXTURE                                         \
    guTexMode(GU_PSM_T8, 0, 0, 1);

/* Set the vertex type for 8-bit indexed tiles */
#define INIT_T8_VERTEX                                          \
    guVertexFormat(GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D);

/* Initialize the work buffer pointers for 8-bit indexed tiles */
#define INIT_T8_WORK_BUFFER                                     \
    uint32_t * const work_buffer_0 = display_work_buffer();     \
    uint32_t * const work_buffer_1 = work_buffer_0 + DISPLAY_STRIDE;

/* Initialize a variable for tracking empty tiles (initialize with an
 * impossible address so it matches nothing by default) */
#define INIT_EMPTY_TILE                                         \
    uint32_t empty_tile_addr = 1;

/*----------------------------------*/

/* Check whether this is a known empty tile, and skip the loop body if so */
static const uint8_t CHECK_EMPTY_TILE_bppshift_lut[8] = {2,3,4,4,5,5,5,5};
#define CHECK_EMPTY_TILE(tilesize)                              \
    if (info->charaddr == empty_tile_addr) {                    \
        continue;                                               \
    }                                                           \
    if (info->transparencyenable && empty_tile_addr == 1) {     \
        const uint32_t bppshift =                               \
            CHECK_EMPTY_TILE_bppshift_lut[info->colornumber];   \
        const uint32_t tile_nwords = ((tilesize)*(tilesize)/32) << bppshift; \
        empty_tile_addr = info->charaddr;                       \
        const uint32_t *ptr = (const uint32_t *)&Vdp2Ram[info->charaddr]; \
        const uint32_t *top = ptr + tile_nwords;                \
        for (; ptr < top; ptr++) {                              \
            if (*ptr != 0) {                                    \
                empty_tile_addr = 1;                            \
                break;                                          \
            }                                                   \
        }                                                       \
        if (empty_tile_addr != 1) {                             \
            /* The tile was empty, so we don't need to draw anything */ \
            continue;                                           \
        }                                                       \
    }

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
#define UPDATE_T8_PALETTE                                       \
    if (info->paladdr != cur_palette) {                         \
        cur_palette = info->paladdr;                            \
        if (UNLIKELY(!palettes[cur_palette])) {                 \
            palettes[cur_palette] = vdp2_gen_t8_clut(           \
                info->coloroffset, info->paladdr<<4,            \
                info->transparencyenable, info->cor, info->cog, info->cob \
            );                                                  \
        }                                                       \
        if (LIKELY(palettes[cur_palette])) {                    \
            const uint32_t * const clut = palettes[cur_palette];\
            guClutLoad(256/8, clut);                            \
        }                                                       \
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
    guTexFlush();                                               \
    guTexImage(0, 512, 512, 16, src);

/* Set the even-lines work buffer for 8-bit indexed 8x8 tiles */
#define SET_T8_BUFFER_0                                         \
    guDrawBuffer(GU_PSM_8888, work_buffer_0, DISPLAY_STRIDE*2);

/* Draw the even lines of an 8-bit indexed 8x8 tile */
#define RENDER_T8_EVEN \
    guVertexPointer(vertices);                                  \
    guDrawPrimitive(GU_SPRITES, 2);

/* Set the odd-lines work buffer for 8-bit indexed 8x8 tiles */
#define SET_T8_BUFFER_1                                         \
    guDrawBuffer(GU_PSM_8888, work_buffer_1, DISPLAY_STRIDE*2);

/* Draw the odd lines of an 8-bit indexed 8x8 tile */
#define RENDER_T8_ODD \
    guVertexPointer(vertices+2);                                \
    guDrawPrimitive(GU_SPRITES, 2);

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

    /* Always draw the first frame */
    frames_to_skip = 0;
    frames_skipped = 0;

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
    /* Not implemented */
}

/*************************************************************************/
/********************* PSP-only interface functions **********************/
/*************************************************************************/

/**
 * psp_video_infoline:  Display an information line on the bottom of the
 * screen.  The text will be displayed for one frame only; call this
 * function every frame to keep the text visible.
 *
 * [Parameters]
 *     color: Text color (0xAABBGGRR)
 *      text: Text string
 * [Return value]
 *     None
 */
void psp_video_infoline(uint32_t color, const char *text)
{
    infoline_text = strdup(text);
    if (UNLIKELY(!infoline_text)) {
        DMSG("Failed to strdup(%s)", text);
    }
    infoline_color = color;
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
    if (frames_skipped < frames_to_skip) {
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
    if (frames_skipped < frames_to_skip) {
        return;
    }

    vdp1cmd_struct cmd;
    Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

    int width  = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
    int height = cmd.CMDSIZE & 0xFF;
    cmd.CMDXB = cmd.CMDXA + width; cmd.CMDYB = cmd.CMDYA;
    cmd.CMDXC = cmd.CMDXA + width; cmd.CMDYC = cmd.CMDYA + height;
    cmd.CMDXD = cmd.CMDXA;         cmd.CMDYD = cmd.CMDYA + height;

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
    if (frames_skipped < frames_to_skip) {
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
        cmd.CMDXC = cmd.CMDXA + new_w; cmd.CMDYC = cmd.CMDYA + new_h;
        cmd.CMDXD = cmd.CMDXA;         cmd.CMDYD = cmd.CMDYA + new_h;
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
    if (frames_skipped < frames_to_skip) {
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
    if (frames_skipped < frames_to_skip) {
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
    if (frames_skipped < frames_to_skip) {
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
    if (frames_skipped < frames_to_skip) {
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
    /* If we're skipping this frame, we don't do anything, not even start
     * a new output frame (because that forces a VBlank sync, which may
     * waste time if the previous frame completed quickly) */
    if (frames_skipped < frames_to_skip) {
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
    display_set_size(disp_width >> disp_xscale, disp_height >> disp_yscale);
    display_begin_frame();
    texcache_reset();

    /* Initialize the render state */
    guTexFilter(GU_NEAREST, GU_NEAREST);
    guTexWrap(GU_CLAMP, GU_CLAMP);
    guTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    guBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    guEnable(GU_BLEND);  // We treat everything as alpha-enabled

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
    if (frames_skipped >= frames_to_skip) {

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

        /* Always compute average FPS (even if we're not showing it), so
         * the value is accurate as soon as the display is turned on.
         * We use a rolling average that decays by 50% every second. */
        unsigned int frame_length = display_last_frame_length();
        if (frame_length == 0) {
            frame_length = 1;  // Just in case (avoid division by 0)
        }
        unsigned int frame_count = 1 + frames_skipped;
        const float fps = (frame_count*60.0f) / frame_length;
        if (!average_fps) {
            /* When first starting up, just set the average to the first
             * frame's frame rate. */
            average_fps = fps;
        } else {
            const float weight = powf(2.0f, -(1/fps));
            average_fps = (average_fps * weight) + (fps * (1-weight));
        }
        if (config_get_show_fps()) {
            unsigned int show_fps = iroundf(fps*10);
            if (show_fps > 600) {
                /* FPS may momentarily exceed 60.0 due to timing jitter,
                 * but we never show more than 60.0. */
                show_fps = 600;
            }
            unsigned int show_avg = iroundf(average_fps*10);
            if (show_avg > 600) {
                show_avg = 600;
            }
            font_printf((disp_width >> disp_xscale) - 2, 2, 1, 0xAAFF8040,
                        "FPS: %2d.%d (%2d.%d)", show_avg/10, show_avg%10,
                        show_fps/10, show_fps%10);
        }

        if (infoline_text) {
            font_printf((disp_width >> disp_xscale) / 2,
                        (disp_height >> disp_yscale) - FONT_HEIGHT - 2, 0,
                        infoline_color, "%s", infoline_text);
            free(infoline_text);
            infoline_text = NULL;
        }

        display_end_frame();

    }  // if (frames_to_skip >= frames_skipped )

    if (frames_skipped < frames_to_skip) {
        frames_skipped++;
    } else {
        frames_skipped = 0;
        if (config_get_frameskip_auto()) {
            // FIXME: auto frame skipping not yet implemented
            frames_to_skip = 0;
        } else {
            frames_to_skip = config_get_frameskip_num();
        }
        if ((Vdp2Regs->BGON & Vdp2External.disptoggle & (1 << BG_RBG0))
         && config_get_enable_rotate()
        ) {
            frames_to_skip += 1 + frames_to_skip;
        }
        if (disp_height > 272 && frames_to_skip == 0
         && config_get_frameskip_interlace()
        ) {
            frames_to_skip = 1;
        }
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
    if (frames_skipped < frames_to_skip) {
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
}

/*************************************************************************/

/**
 * psp_vdp2_set_priority_{NBG[0-3],RBG0}:  Set the priority of the given
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
    vertices = pspGuGetMemoryMerge(sizeof(*vertices) * nvertices);
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
    vdp1_queue_render(priority, 0, GU_LINE_STRIP,
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

    int priority;
    uint32_t color32 = vdp1_get_cmd_color_pri(cmd, textured, &priority);
    uint32_t texture_key;
    if (textured) {
        texture_key = vdp1_cache_sprite_texture(cmd, width, height, &priority);
        if (UNLIKELY(!texture_key)) {
            DMSG("WARNING: failed to cache texture for A=(%d,%d) B=(%d,%d)"
                 " C=(%d,%d) D=(%d,%d)",
                 cmd->CMDXA + Vdp1Regs->localX, cmd->CMDYA + Vdp1Regs->localY,
                 cmd->CMDXB + Vdp1Regs->localX, cmd->CMDYB + Vdp1Regs->localY,
                 cmd->CMDXC + Vdp1Regs->localX, cmd->CMDYC + Vdp1Regs->localY,
                 cmd->CMDXD + Vdp1Regs->localX, cmd->CMDYD + Vdp1Regs->localY);
        }
    } else {
        texture_key = 0;
    }

    /* We don't support mesh shading; treat it as half-alpha instead */

    if (cmd->CMDPMOD & 0x100) {  // Mesh shading bit
        const unsigned int alpha = color32 >> 24;
        color32 = ((alpha+1)/2) << 24 | (color32 & 0x00FFFFFF);
    }

    /* If it's a Gouraud-shaded polygon, pick up the four corner colors */

    uint32_t color_A, color_B, color_C, color_D;
    if (cmd->CMDPMOD & 4) {  // Gouraud shading bit
        const uint32_t alpha = color32 & 0xFF000000;
        uint16_t color16;
        color16 = T1ReadWord(Vdp1Ram, (cmd->CMDGRDA<<3) + 0);
        color_A = alpha | (color16 & 0x7C00) << 9
                        | (color16 & 0x03E0) << 6
                        | (color16 & 0x001F) << 3;
        color16 = T1ReadWord(Vdp1Ram, (cmd->CMDGRDA<<3) + 2);
        color_B = alpha | (color16 & 0x7C00) << 9
                        | (color16 & 0x03E0) << 6
                        | (color16 & 0x001F) << 3;
        color16 = T1ReadWord(Vdp1Ram, (cmd->CMDGRDA<<3) + 4);
        color_C = alpha | (color16 & 0x7C00) << 9
                        | (color16 & 0x03E0) << 6
                        | (color16 & 0x001F) << 3;
        color16 = T1ReadWord(Vdp1Ram, (cmd->CMDGRDA<<3) + 6);
        color_D = alpha | (color16 & 0x7C00) << 9
                        | (color16 & 0x03E0) << 6
                        | (color16 & 0x001F) << 3;
    } else {
        color_A = color_B = color_C = color_D = color32;
    }

    /* Set up the vertex array using a strip of 2 triangles.  The Saturn
     * coordinate order is A,B,C,D clockwise around the texture, so we flip
     * around C and D in our vertex array.  For simplicity, we assign both
     * the color and U/V coordinates regardless of whether the polygon is
     * textured or not; the GE is fast enough that it can handle all the
     * processing in time. */

    struct {int16_t u, v; uint32_t color; int16_t x, y, z, pad;} *vertices;
    vertices = pspGuGetMemoryMerge(sizeof(*vertices) * 4);
    vertices[0].u = (flip_h ? width : 0);
    vertices[0].v = (flip_v ? height : 0);
    vertices[0].color = color_A;
    vertices[0].x = (cmd->CMDXA + Vdp1Regs->localX) >> disp_xscale;
    vertices[0].y = (cmd->CMDYA + Vdp1Regs->localY) >> disp_yscale;
    vertices[0].z = 0;
    vertices[1].u = (flip_h ? 0 : width);
    vertices[1].v = (flip_v ? height : 0);
    vertices[1].color = color_B;
    vertices[1].x = (cmd->CMDXB + Vdp1Regs->localX) >> disp_xscale;
    vertices[1].y = (cmd->CMDYB + Vdp1Regs->localY) >> disp_yscale;
    vertices[1].z = 0;
    vertices[2].u = (flip_h ? width : 0);
    vertices[2].v = (flip_v ? 0 : height);
    vertices[2].color = color_D;
    vertices[2].x = (cmd->CMDXD + Vdp1Regs->localX) >> disp_xscale;
    vertices[2].y = (cmd->CMDYD + Vdp1Regs->localY) >> disp_yscale;
    vertices[2].z = 0;
    vertices[3].u = (flip_h ? 0 : width);
    vertices[3].v = (flip_v ? 0 : height);
    vertices[3].color = color_C;
    vertices[3].x = (cmd->CMDXC + Vdp1Regs->localX) >> disp_xscale;
    vertices[3].y = (cmd->CMDYC + Vdp1Regs->localY) >> disp_yscale;
    vertices[3].z = 0;

    /* Queue the draw operation */

    vdp1_queue_render(priority, texture_key,
                      GU_TRIANGLE_STRIP, GU_TEXTURE_16BIT | GU_COLOR_8888
                                       | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                      4, NULL, vertices);
}

/*************************************************************************/

/**
 * vdp1_convert_color:  Convert a VDP1 16-bit color value and pixel mode to
 * a 32-bit color value.  Helper function for vdp1_get_cmd_color() and
 * vdp1_get_cmd_color_pri().
 *
 * [Parameters]
 *      color16: 16-bit color value
 *     textured: Nonzero if a textured polygon command, else zero
 *      CMDPMOD: Value of CMDPMOD field in VDP1 command
 * [Return value]
 *     32-bit color value
 */
static uint32_t vdp1_convert_color(uint16_t color16, int textured,
                                   unsigned int CMDPMOD)
{
    uint32_t color32;
    if (textured) {
        color32 = 0xFFFFFF;
    } else if (color16 == 0) {
        color32 = adjust_color_16_32(0x0000, vdp1_rofs, vdp1_gofs, vdp1_bofs);
        return color32 & 0x00FFFFFF;  // Transparent regardless of CMDPMOD
    } else if (color16 & 0x8000) {
        color32 = adjust_color_16_32(color16, vdp1_rofs, vdp1_gofs, vdp1_bofs);
    } else {
        color32 = adjust_color_32_32(global_clut_32[color16 & 0x7FF],
                                     vdp1_rofs, vdp1_gofs, vdp1_bofs);
    }

    switch (CMDPMOD & 7) {
      default: // Impossible, but avoid a "function may not return" warning
      case 1:  // Shadow
        return 0x80000000;
      case 4 ... 7:  // Gouraud shading (handled separately)
      case 0:  // Replace
        return 0xFF000000 | color32;
      case 2:  // 50% luminance
        /* Clever, quick way to divide each component by 2 in one step
         * (borrowed from vidsoft.c) */
        return 0xFF000000 | ((color32 & 0xFEFEFE) >> 1);
      case 3:  // 50% transparency
        return 0x80000000 | color32;
    }
}

/*-----------------------------------------------------------------------*/

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
    return vdp1_convert_color(cmd->CMDCOLR, 0, cmd->CMDPMOD);
}

/*-----------------------------------------------------------------------*/

/**
 * vdp1_get_cmd_color_pri:  Return the 32-bit color value and priority
 * specified by a VDP1 polygon command.
 *
 * [Parameters]
 *              cmd: VDP1 command pointer
 *         textured: Nonzero if the polygon is textured, else zero
 *     priority_ret: Pointer to variable to receive priority value
 * [Return value]
 *     32-bit color value
 */
static uint32_t vdp1_get_cmd_color_pri(vdp1cmd_struct *cmd, int textured,
                                       int *priority_ret)
{
    uint16_t color16 = cmd->CMDCOLR;
    if (cmd->CMDCOLR & 0x8000) {
        *priority_ret = Vdp2Regs->PRISA & 7;
    } else {
        *priority_ret = 0;  // Default if not set by SPCTL
        vdp1_process_sprite_color(color16, priority_ret);
    }
    return vdp1_convert_color(color16, textured, cmd->CMDPMOD);
}

/*-----------------------------------------------------------------------*/

/**
 * vdp1_process_sprite_color:  Return the color index mask and priority
 * value selected by the given color register value and the VDP2 SPCTL
 * register.  Derived from vidshared.h:Vdp1ProcessSpritePixel(), but
 * ignores the unused colorcalc and shadow parameters.
 *
 * [Parameters]
 *          color16: 16-bit color register value
 *     priority_ret: Pointer to variable to receive priority value
 * [Return value]
 *     Mask to apply to CMDCOLR register
 */
static uint16_t vdp1_process_sprite_color(uint16_t color16, int *priority_ret)
{
    switch (Vdp2Regs->SPCTL & 0x0F) {
      case  0:
        *priority_ret = (color16 & 0xC000) >> 14;
        return 0x7FF;
      case  1:
        *priority_ret = (color16 & 0xE000) >> 13;
        return 0x7FF;
      case  2:
        *priority_ret = (color16 & 0x8000) >> 15;
        return 0x7FF;
      case  3:
        *priority_ret = (color16 & 0x6000) >> 13;
        return 0x7FF;
      case  4:
        *priority_ret = (color16 & 0x7000) >> 12;
        return 0x3FF;
      case  5:
        *priority_ret = (color16 & 0x7000) >> 12;
        return 0x7FF;
      case  6:
        *priority_ret = (color16 & 0x7000) >> 12;
        return 0x3FF;
      case  7:
        *priority_ret = (color16 & 0x7000) >> 12;
        return 0x1FF;
      case  8:
        *priority_ret = (color16 & 0x0080) >>  7;
        return 0x7F;
      case  9:
        *priority_ret = (color16 & 0x0080) >>  7;
        return 0x3F;
      case 10:
        *priority_ret = (color16 & 0x00C0) >>  6;
        return 0x3F;
      case 11:
        *priority_ret = 0;
        return 0x3F;
      case 12:
        *priority_ret = (color16 & 0x0080) >>  7;
        return 0xFF;
      case 13:
        *priority_ret = (color16 & 0x0080) >>  7;
        return 0xFF;
      case 14:
        *priority_ret = (color16 & 0x00C0) >>  6;
        return 0xFF;
      case 15:
        *priority_ret = 0;
        return 0xFF;
      default:  // Impossible, but avoid a "may not return a value" warning
        *priority_ret = 0;
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
    uint16_t CMDCOLR = cmd->CMDCOLR;
    int regnum = 0;  // Default value
    if (!((Vdp2Regs->SPCTL & 0x20) && (cmd->CMDCOLR & 0x8000))) {
        uint16_t color16 = cmd->CMDCOLR;
        int is_lut = 1;
        if ((cmd->CMDPMOD>>3 & 7) == 1) {
            /* Indirect T4 texture; see whether the first pixel references
             * a LUT or uses raw RGB values */
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
            CMDCOLR &= vdp1_process_sprite_color(color16, &regnum);
        }
    }
    *priority_ret = ((uint8_t *)&Vdp2Regs->PRISA)[regnum] & 0x7;

    /* Cache the texture data and return the key */
    
    return texcache_cache_sprite(cmd->CMDSRCA, cmd->CMDPMOD, CMDCOLR,
                                 width, height, vdp1_rofs, vdp1_gofs,
                                 vdp1_bofs);
}

/*************************************************************************/

/**
 * vdp1_queue_render:  Queue a render operation from a VDP1 command.
 *
 * [Parameters]
 *        priority: Saturn display priority (0-7)
 *     texture_key: Texture key for sprites, zero for untextured operations
 *       primitive,
 *     vertex_type,
 *           count,
 *         indices,
 *        vertices: Parameters to pass to guDrawArray()
 * [Return value]
 *     None
 */
static inline void vdp1_queue_render(
    int priority, uint32_t texture_key, int primitive,
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
    int in_texture_mode;  // Remember which mode we're in
    VDP1RenderData *entry = vdp1_queue[priority].queue;
    VDP1RenderData * const queue_top = entry + vdp1_queue[priority].len;

    if (vdp1_queue[priority].len == 0) {
        return;  // Nothing to do
    }

    guShadeModel(GU_SMOOTH);
    guAmbientColor(0xFFFFFFFF);
    guTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    if (config_get_smooth_textures()) {
        guTexFilter(GU_LINEAR, GU_LINEAR);
    }
    if (entry->texture_key) {
        guEnable(GU_TEXTURE_2D);
        in_texture_mode = 1;
    } else {
        guDisable(GU_TEXTURE_2D);
        in_texture_mode = 0;
    }
    for (; entry < queue_top; entry++) {
        if (entry->texture_key) {
            texcache_load_sprite(entry->texture_key);
            if (!in_texture_mode) {
                guEnable(GU_TEXTURE_2D);
                in_texture_mode = 1;
            }
        } else {
            if (in_texture_mode) {
                guDisable(GU_TEXTURE_2D);
                in_texture_mode = 0;
            }
        }
        guDrawArray(entry->primitive, entry->vertex_type,
                    entry->count, entry->indices, entry->vertices);
    }
    if (in_texture_mode) {
        guDisable(GU_TEXTURE_2D);
    }
    if (config_get_smooth_textures()) {
        guTexFilter(GU_NEAREST, GU_NEAREST);
    }
    guShadeModel(GU_FLAT);

    guCommit();
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

    int rofs, gofs, bofs;
    vdp2_get_color_offsets(1<<6, &rofs, &gofs, &bofs);

    struct {uint32_t color; int16_t x, y, z, pad;} *vertices;

    if (Vdp2Regs->BKTAU & 0x8000) {
        /* Distinct color for each line */
        int num_vertices, y;
        if (disp_height > 272) {
            /* For interlaced screens, we average the colors of each two
             * adjacent lines */
            num_vertices = 2*(disp_height/2);
            vertices = pspGuGetMemoryMerge(sizeof(*vertices) * num_vertices);
            for (y = 0; y+1 < disp_height; y += 2, address += 4) {
                uint16_t rgb0 = T1ReadWord(Vdp2Ram, address);
                uint32_t r0 = (rgb0 & 0x001F) << 3;
                uint32_t g0 = (rgb0 & 0x03E0) >> 2;
                uint32_t b0 = (rgb0 & 0x7C00) >> 7;
                uint16_t rgb1 = T1ReadWord(Vdp2Ram, address);
                uint32_t r1 = (rgb1 & 0x001F) << 3;
                uint32_t g1 = (rgb1 & 0x03E0) >> 2;
                uint32_t b1 = (rgb1 & 0x7C00) >> 7;
                uint32_t color = bound(((r0+r1+1)/2) + rofs, 0, 255) <<  0
                               | bound(((g0+g1+1)/2) + gofs, 0, 255) <<  8
                               | bound(((b0+b1+1)/2) + bofs, 0, 255) << 16
                               | 0xFF000000;
                vertices[y+0].color = color;
                vertices[y+0].x = 0;
                vertices[y+0].y = y/2;
                vertices[y+0].z = 0;
                vertices[y+1].color = color;
                vertices[y+1].x = disp_width >> disp_xscale;
                vertices[y+1].y = y/2;
                vertices[y+1].z = 0;
            }
        } else {
            num_vertices = 2*disp_height;
            vertices = pspGuGetMemoryMerge(sizeof(*vertices) * num_vertices);
            for (y = 0; y < disp_height; y++, address += 2) {
                uint16_t rgb = T1ReadWord(Vdp2Ram, address);
                uint32_t r = bound(((rgb & 0x001F) << 3) + rofs, 0, 255);
                uint32_t g = bound(((rgb & 0x03E0) >> 2) + gofs, 0, 255);
                uint32_t b = bound(((rgb & 0x7C00) >> 7) + bofs, 0, 255);
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
        guDrawArray(GU_LINES,
                    GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                    num_vertices, NULL, vertices);
        guCommit();
    } else {
        /* Single color for the whole screen */
        vertices = pspGuGetMemoryMerge(sizeof(*vertices) * 2);
        uint16_t rgb = T1ReadWord(Vdp2Ram, address);
        uint32_t r = bound(((rgb & 0x001F) << 3) + rofs, 0, 255);
        uint32_t g = bound(((rgb & 0x03E0) >> 2) + gofs, 0, 255);
        uint32_t b = bound(((rgb & 0x7C00) >> 7) + bofs, 0, 255);
        vertices[0].color = 0xFF000000 | r | g<<8 | b<<16;
        vertices[0].x = 0;
        vertices[0].y = 0;
        vertices[0].z = 0;
        vertices[1].color = 0xFF000000 | r | g<<8 | b<<16;
        vertices[1].x = disp_width >> disp_xscale;
        vertices[1].y = disp_height >> disp_yscale;
        vertices[1].z = 0;
        guDrawArray(GU_SPRITES,
                    GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                    2, NULL, vertices);
        guCommit();
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

    /* Is this background layer enabled? */
    if (!(Vdp2Regs->BGON & Vdp2External.disptoggle & (1 << layer))) {
        return;
    }

    /* Check whether we should smooth the graphics */
    const int smooth_hires =
        (disp_width > 352 || disp_height > 272) && config_get_smooth_hires();

    /* Find out whether it's a bitmap or not */
    switch (layer) {
      case BG_NBG0: info.isbitmap = Vdp2Regs->CHCTLA & 0x0002; break;
      case BG_NBG1: info.isbitmap = Vdp2Regs->CHCTLA & 0x0200; break;
      case BG_RBG0: info.isbitmap = Vdp2Regs->CHCTLB & 0x0200; break;
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

    /* Extract rotation information for RBG0 */
    if (layer == BG_RBG0) {
        switch (Vdp2Regs->RPMD & 3) {
          case 0:
            info.rotatenum = 0;
            info.rotatemode = 0;
            break;
          case 1:
            info.rotatenum = 1;
            info.rotatemode = 0;
            break;
          case 2:
            info.rotatenum = 1; // FIXME: Quick hack for Panzer Dragoon Saga
            info.rotatemode = 1;
            break;
          case 3:
            info.rotatenum = 0;
            info.rotatemode = 2;
            break;
        }
    }

    /* Determine tilemap/bitmap size and display offset */
    if (info.isbitmap) {
        if (layer == BG_RBG0) {
            ReadBitmapSize(&info, Vdp2Regs->CHCTLB >> 10, 0x3);
            info.charaddr = ((Vdp2Regs->MPOFR >> (4*info.rotatenum)) & 7) << 17;
            info.paladdr = (Vdp2Regs->BMPNB & 0x7) << 4;
        } else {
            ReadBitmapSize(&info, Vdp2Regs->CHCTLA >> (2 + layer*8), 0x3);
            info.charaddr = ((Vdp2Regs->MPOFN >> (4*layer)) & 7) << 17;
            info.paladdr = ((Vdp2Regs->BMPNA >> (8*layer)) & 7) << 4;
        }
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
          case BG_RBG0:
            /* Coordinate rotation is handled separately; nothing to do here */
            break;
          default:
            DMSG("info.isbitmap set for invalid layer %d", layer);
            return;
        }
    } else {
        if (layer == BG_RBG0) {
            info.mapwh = 4;
            ReadPlaneSize(&info, Vdp2Regs->PLSZ >> (8 + 4*info.rotatenum));
        } else {
            info.mapwh = 2;
            ReadPlaneSize(&info, Vdp2Regs->PLSZ >> (2*layer));
        }
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
        case BG_RBG0: if (info.rotatenum == 0) {
                          info.PlaneAddr = (void *)Vdp2ParameterAPlaneAddr;
                      } else {
                          info.PlaneAddr = (void *)Vdp2ParameterBPlaneAddr;
                      }
                      break;
        default:      DMSG("No PlaneAddr for layer %d", layer); return;
    }
    info.patternpixelwh = 8 * info.patternwh;
    info.draww = (int)((float)(disp_width  >> disp_xscale) / info.coordincx);
    info.drawh = (int)((float)(disp_height >> disp_yscale) / info.coordincy);

    /* Select a rendering function based on the tile layout and format */
    void (*draw_map_func)(vdp2draw_struct *info, clipping_struct *clip);
    if (layer == BG_RBG0) {
        draw_map_func = &vdp2_draw_map_rotated;
    } else if (info.isbitmap) {
        switch (layer) {
          case BG_NBG0:
            if ((Vdp2Regs->SCRCTL & 7) == 7) {
                DMSG("WARNING: line scrolling not supported");
            }
            /* fall through */
          case BG_NBG1:
            if (info.colornumber == 1 && !smooth_hires) {
                draw_map_func = &vdp2_draw_bitmap_t8;
            } else {
                draw_map_func = &vdp2_draw_bitmap;
            }
            break;
          default:
            DMSG("info.isbitmap set for invalid layer %d", layer);
            return;
        }
    } else if (info.patternwh == 2) {
        if (info.colornumber == 1 && !smooth_hires) {
            draw_map_func = &vdp2_draw_map_16x16_t8;
        } else {
            draw_map_func = &vdp2_draw_map_16x16;
        }
    } else {
        if (info.colornumber == 1 && !smooth_hires) {
            draw_map_func = &vdp2_draw_map_8x8_t8;
        } else {
            draw_map_func = &vdp2_draw_map_8x8;
        }
    }

    /* Set up for rendering */
    guEnable(GU_TEXTURE_2D);
    guAmbientColor(info.alpha<<24 | 0xFFFFFF);
    if (smooth_hires) {
        guTexFilter(GU_LINEAR, GU_LINEAR);
    }

    /* Render the graphics */
    (*draw_map_func)(&info, clip);

    /* All done */
    if (smooth_hires) {
        guTexFilter(GU_NEAREST, GU_NEAREST);
    }
    guAmbientColor(0xFFFFFFFF);
    guDisable(GU_TEXTURE_2D);
    guCommit();
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
    /* Allocate vertex memory and perform other initialization */
    GET_VERTICES(2);
    INIT_EMPTY_TILE;

    /* Loop through tiles */
    TILE_LOOP_BEGIN(8) {
        CHECK_EMPTY_TILE(8);
        GET_FLIP_PRI;
        SET_VERTICES(info->x * info->coordincx, info->y * info->coordincy,
                     8 * info->coordincx, 8 * info->coordincy);
        texcache_load_tile(info->charaddr, info->colornumber,
                           info->transparencyenable,
                           info->coloroffset, info->paladdr << 4,
                           info->cor, info->cog, info->cob);
        guDrawArray(GU_SPRITES,
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
    /* Check the current screen mode; if we're in interlaced mode, we can
     * cheat by treating each 8x8 tile as a 16x4 texture and drawing only
     * the left or right half, thus omitting alternate lines for free. */
    const int interlaced = (disp_height > 272);

    /* Allocate vertex memory and perform other initialization.  Note that
     * we need 2 sprites to draw each tile if we're not optimizing
     * interlaced graphics. */
    GET_VERTICES(interlaced ? 2 : 4);
    INIT_T8_PALETTE;
    INIT_T8_TEXTURE;
    INIT_T8_VERTEX;
    INIT_T8_WORK_BUFFER;
    INIT_EMPTY_TILE;

    /* Loop through tiles */
    TILE_LOOP_BEGIN(8) {
        CHECK_EMPTY_TILE(8);
        GET_FLIP_PRI_T8;
        UPDATE_T8_PALETTE;

        /* Set up vertices and draw the tile */
        const uint8_t *src = &Vdp2Ram[info->charaddr];
        int yofs = ((uintptr_t)src & 63) / 16;
        src = (const uint8_t *)((uintptr_t)src & ~63);
        SET_VERTICES_T8_EVEN(info->x * info->coordincx,
                             info->y * info->coordincy,
                             8 * info->coordincx, 8 * info->coordincy);
        if (!interlaced) {
            SET_VERTICES_T8_ODD(info->x * info->coordincx,
                                info->y * info->coordincy,
                                8 * info->coordincx, 8 * info->coordincy);
        } else {
            /* We don't modify the work buffer stride in this case, so double
             * the Y coordinates (which were set assuming a doubled stride) */
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
    } TILE_LOOP_END;

    if (!interlaced) {
        /* Restore stride to normal */
        guDrawBuffer(GU_PSM_8888, work_buffer_0, DISPLAY_STRIDE);
    }
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

    /* Allocate vertex memory and perform other initialization */
    GET_VERTICES(2);
    INIT_EMPTY_TILE;

    /* Loop through tiles */
    TILE_LOOP_BEGIN(16) {
        CHECK_EMPTY_TILE(16);
        GET_FLIP_PRI;

        /* Draw four consecutive tiles in a 2x2 pattern */
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
            guDrawArray(GU_SPRITES,
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
    /* Check the current screen mode */
    const int interlaced = (disp_height > 272);

    /* Allocate vertex memory and perform other initialization */
    GET_VERTICES(interlaced ? 2 : 4);
    INIT_T8_PALETTE;
    INIT_T8_TEXTURE;
    INIT_T8_VERTEX;
    INIT_T8_WORK_BUFFER;
    INIT_EMPTY_TILE;

    /* Loop through tiles */
    TILE_LOOP_BEGIN(16) {
        CHECK_EMPTY_TILE(16);
        GET_FLIP_PRI_T8;
        UPDATE_T8_PALETTE;

        const uint8_t *src = &Vdp2Ram[info->charaddr];
        int yofs = ((uintptr_t)src & 63) / 16;
        src = (const uint8_t *)((uintptr_t)src & ~63);
        int tilenum;
        for (tilenum = 0; tilenum < 4; tilenum++, src += 8*8*1) {
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
                vertices += 2;
            }
        }
    } TILE_LOOP_END;

    if (!interlaced) {
        guDrawBuffer(GU_PSM_8888, work_buffer_0, DISPLAY_STRIDE);
    }
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
 *            color_ofs: Color offset (ORed with pixel value)
 *          transparent: Nonzero if color index 0 should be transparent
 *     rofs, gofs, bofs: Red/green/blue adjustment values
 * [Return value]
 *     Allocated color table pointer (NULL on failure to allocate memory)
 */
static inline uint32_t *vdp2_gen_t8_clut(
    int color_base, int color_ofs, int transparent,
    int rofs, int gofs, int bofs)
{
    uint32_t *clut = pspGuGetMemoryMerge(256*4 + 60);
    clut = (uint32_t *)(((uintptr_t)clut + 63) & -64);  // Must be aligned
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
    /* Set up vertices */
    VertexUVXYZ *vertices = pspGuGetMemoryMerge(sizeof(*vertices) * 2);
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
    guDrawArray(GU_SPRITES,
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
    /* Set up vertices */
    // FIXME: this will break with a final (clipped) width above 512;
    // need to split the texture in half in that case
    VertexUVXYZ *vertices = pspGuGetMemoryMerge(sizeof(*vertices) * 2);
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
    guClutMode(GU_PSM_8888, 0, 0xFF, 0);
    void *ptr = vdp2_gen_t8_clut(
        info->coloroffset, info->paladdr<<4,
        info->transparencyenable, info->cor, info->cog, info->cob
    );
    if (LIKELY(ptr)) {
        guClutLoad(256/8, ptr);
    }

    /* Draw the bitmap */
    guTexMode(GU_PSM_8888, 0, 0, 0);
    guTexMode(GU_PSM_T8, 0, 0, 0);
    guTexImage(0, 512, 512, info->cellw, &Vdp2Ram[info->charaddr & 0x7FFFF]);
    guDrawArray(GU_SPRITES,
                GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                2, NULL, vertices);
}

/*************************************************************************/

/**
 * INIT_CALC_PIXELNUM:  Precompute values used by the CALC_PIXELNUM macro.
 */
#define INIT_CALC_PIXELNUM                                              \
    const int srcx_mask = info->isbitmap                                \
                          ? info->cellw - 1                             \
                          : (8 * 64 * info->planew * 4) - 1;            \
    const int srcy_mask = info->isbitmap                                \
                          ? info->cellh - 1                             \
                          : (8 * 64 * info->planeh * 4) - 1;            \
    const int plane_xshift = 3 + 6 + info->planew_bits;                 \
    const int plane_yshift = 3 + 6 + info->planeh_bits;                 \
    const int plane_xmask  = (1 << plane_xshift) - 1;                   \
    const int plane_ymask  = (1 << plane_yshift) - 1;                   \
    const int page_shift   = 3 + 6;                                     \
    const int page_mask    = (1 << page_shift) - 1;                     \
    const int tile_shift   = (info->patternwh==2) ? 4 : 3;              \
    const int tile_mask    = (1 << tile_shift) - 1;                     \
    /* Remember the last tile seen, to save the time of looking it up   \
     * while we're on nearby pixels */                                  \
    int last_tilex = -1, last_tiley = -1

/*----------------------------------*/

/**
 * CALC_PIXELNUM:  Calculate the pixel index associated with the
 * transformed coordinates (srcx,srcy), store it in the variable
 * "pixelnum", and update the "info" structure's fields as necessary for
 * the associated tile (if the graphics layer is in tilemap mode).
 *
 * This is implemented as a macro so it can make use of precomputed
 * constant values within the calling routine.
 */
#define CALC_PIXELNUM  do {                                             \
    srcx &= srcx_mask;                                                  \
    srcy &= srcy_mask;                                                  \
    if (info->isbitmap) {                                               \
        pixelnum = srcy << info->cellw_bits | srcx;                     \
    } else {                                                            \
        const int tilex = srcx >> tile_shift;                           \
        const int tiley = srcy >> tile_shift;                           \
        if (tilex != last_tilex || tiley != last_tiley) {               \
            last_tilex = tilex;                                         \
            last_tiley = tiley;                                         \
            const int planenum = 4 * (srcy >> plane_yshift)             \
                               + (srcx >> plane_xshift);                \
            info->PlaneAddr(info, planenum);                            \
            const int pagex = (srcx & plane_xmask) >> page_shift;       \
            const int pagey = (srcy & plane_ymask) >> page_shift;       \
            const int pagenum = pagey << info->planew_bits | pagex;     \
            const int page_tilex = (srcx & page_mask) >> tile_shift;    \
            const int page_tiley = (srcy & page_mask) >> tile_shift;    \
            const int page_tilenum =                                    \
                page_tiley << (page_shift-tile_shift) | page_tilex;     \
            const int tilenum =                                         \
                pagenum << (2*(page_shift-tile_shift)) | page_tilenum;  \
            info->addr += tilenum << (info->patterndatasize_bits + 1);  \
            vdp2_calc_pattern_address(info);                            \
        }                                                               \
        srcx &= tile_mask;                                              \
        srcy &= tile_mask;                                              \
        if (info->flipfunction & 1) {                                   \
            srcx ^= tile_mask;                                          \
        }                                                               \
        if (info->flipfunction & 2) {                                   \
            srcy ^= tile_mask;                                          \
        }                                                               \
        if (tile_shift == 4) {                                          \
            pixelnum = (srcy & 8) << (7-3)                              \
                     | (srcx & 8) << (6-3)                              \
                     | (srcy & 7) << 3                                  \
                     | (srcx & 7);                                      \
        } else {                                                        \
            pixelnum = srcy << 3 | srcx;                                \
        }                                                               \
    }                                                                   \
} while (0)

/*----------------------------------*/

/**
 * vdp2_draw_map_rotated:  Draw a rotated graphics layer.
 *
 * [Parameters]
 *     info: Graphics layer data
 *     clip: Clipping window data
 * [Return value]
 *     None
 */
static void vdp2_draw_map_rotated(vdp2draw_struct *info,
                                  clipping_struct *clip)
{
    /* See if rotated graphics are enabled in the first place */
    if (!config_get_enable_rotate()) {
        return;
    }

    /* Parse rotation parameters */
    static __attribute__((aligned(16))) RotationParams param_set[2];
    get_rotation_parameters(info, 0, &param_set[0]);
    get_rotation_parameters(info, 1, &param_set[1]);

    /* Select the initial parameter set (depending on the rotation mode,
     * it may be changed later) */
    RotationParams *param = &param_set[info->rotatenum];

    /*
     * If the parameter set uses a fixed transformation matrix that doesn't
     * actually perform any rotation, we can just call the regular map
     * drawing functions with appropriately scaled parameters:
     *
     *     [srcX]   (  [screenX])   [kx]   [Xp]
     *     [srcY] = (M [screenY]) * [ky] + [Yp]
     *              (  [   1   ])
     *
     *     [srcX]   ([mat11    0    mat13] [screenX])   [kx]   [Xp]
     *     [srcY] = ([  0    mat22  mat23] [screenY]) * [ky] + [Yp]
     *              (                      [   1   ])
     *
     *     srcX = (mat11*screenX + mat13) * kx + Xp
     *     srcY = (mat22*screenY + mat23) * ky + Yp
     *
     *     srcX = (mat11*kx)*screenX + (mat13*kx + Xp)
     *     srcY = (mat22*ky)*screenY + (mat23*ky + Yp)
     *
     *     (srcX - (mat13*kx + Xp)) / (mat11*kx) = screenX
     *     (srcY - (mat23*ky + Yp)) / (mat22*ky) = screenX
     *
     *     screenX = (1/(mat11*kx))*srcX - ((mat13*kx + Xp) / (mat11*kx))
     *     screenY = (1/(mat22*ky))*srcX - ((mat23*ky + Yp) / (mat22*ky))
     */

    if (!param->coefenab && info->rotatemode == 0
     && param->mat12 == 0.0f && param->mat21 == 0.0f
    ) {
        const float xmul = 1/(param->mat11 * param->kx);
        const float ymul = 1/(param->mat22 * param->ky);
        info->coordincx = xmul;
        info->coordincy = xmul;
        info->x = -((param->mat13 * param->kx) + param->Xp) * xmul;
        info->y = -((param->mat23 * param->ky) + param->Yp) * ymul;
        void (*draw_func)(vdp2draw_struct *info, clipping_struct *clip);
        if (info->isbitmap) {
            draw_func = &vdp2_draw_bitmap;
        } else if (info->patternwh == 2) {
            draw_func = &vdp2_draw_map_16x16;
        } else {
            draw_func = &vdp2_draw_map_8x8;
        }
        return (*draw_func)(info, clip);
    }

    /*
     * There's rotation and/or distortion going on, so we'll have to render
     * the image manually.  (Sadly, the PSP doesn't have shaders, so we
     * can't translate the coefficients into a texture coordinate map and
     * render that way.)
     */

    /* Precalculate tilemap/bitmap coordinate masks and shift counts */

    INIT_CALC_PIXELNUM;

    /* Choose a pixel-read function based on the pixel format */

    const int need_adjust =
        (info->alpha != 0xFF) || info->cor || info->cog || info->cob;
    uint32_t (*get_pixel)(vdp2draw_struct *, unsigned int);
    switch (info->colornumber) {
      case 0:
        get_pixel = need_adjust ? rotation_get_pixel_t4_adjust
                                : rotation_get_pixel_t4;
        break;
      case 1:
        get_pixel = need_adjust ? rotation_get_pixel_t8_adjust
                                : rotation_get_pixel_t8;
        break;
      case 2:
        get_pixel = need_adjust ? rotation_get_pixel_t16_adjust
                                : rotation_get_pixel_t16;
        break;
      case 3:
        get_pixel = need_adjust ? rotation_get_pixel_16_adjust
                                : rotation_get_pixel_16;
        break;
      case 4:
        get_pixel = need_adjust ? rotation_get_pixel_32_adjust
                                : rotation_get_pixel_32;
        break;
      default:
        DMSG("Invalid pixel format %d", info->colornumber);
        return;
    }

    /* Allocate a buffer for drawing the texture.  Note that we swizzle
     * the texture for faster drawing, since it's allocated in system
     * RAM (which the GE is slow at accessing). */

    uint32_t *pixelbuf = guGetMemory(disp_width * disp_height * 4 + 60);
    pixelbuf = (uint32_t *)(((uintptr_t)pixelbuf + 63) & -64);

    /* Render all pixels */

    float coef_dy = param->deltaKAst;
    float coef_y = 0;
    unsigned int y;

    for (y = 0; y < disp_height; y++, coef_y += coef_dy) {

        /* FIXME: This is needed to get the Panzer Dragoon Saga name entry
         * screen's background to display properly, but I have no clue
         * whether it's the Right Thing To Do.  I'm using the clip window
         * based on vidsoft.c comments, but it could also be just a
         * top-half/bottom-half-of-screen thing. */
        if (info->rotatemode == 2) {
            if (param == &param_set[0] && y >= (clip[0].yend + 1) / 2) {
                param = &param_set[1];
                info->PlaneAddr = (void *)Vdp2ParameterBPlaneAddr;
                coef_dy = param->deltaKAst;
                coef_y = param->deltaKAst * y;
            }
        }

        const uint32_t coef_base =
            param->coeftbladdr + (ifloorf(coef_y) << param->coefdatashift);
        uint32_t * const dest_base =
            pixelbuf + (y/8)*(disp_width*8) + (y%8)*4;

        if (!param->coefenab) {

            /* Constant parameters for the whole screen (FIXME: we could
             * draw this in hardware if we had code to rotate vertices) */
            float srcx_f, srcy_f;
            transform_coordinates(param, 0, y, &srcx_f, &srcy_f);
            const float delta_srcx = param->mat11 * param->kx;
            const float delta_srcy = param->mat21 * param->ky;
            unsigned int x;
            for (x = 0; x < disp_width;
                 x++, srcx_f += delta_srcx, srcy_f += delta_srcy
            ) {
                uint32_t *dest = dest_base + (x/4)*32 + x%4;
                int srcx = ifloorf(srcx_f);
                int srcy = ifloorf(srcy_f);
                unsigned int pixelnum;
                CALC_PIXELNUM;
                *dest = (*get_pixel)(info, pixelnum);
            }

        } else if (param->deltaKAx == 0) {

            /* One coefficient for the whole row */
            if (!get_rotation_coefficient(param, coef_base)) {
                /* Empty row */
                uint32_t *dest = dest_base;
                unsigned int x;
                for (x = 0; x < disp_width; x += 4, dest += 32) {
                    dest[0] = dest[1] = dest[2] = dest[3] = 0;
                }
                continue;
            }
            float srcx_f, srcy_f;
            transform_coordinates(param, 0, y, &srcx_f, &srcy_f);
            const float delta_srcx = param->mat11 * param->kx;
            const float delta_srcy = param->mat21 * param->ky;
            unsigned int x;
            for (x = 0; x < disp_width;
                 x++, srcx_f += delta_srcx, srcy_f += delta_srcy
            ) {
                uint32_t *dest = dest_base + (x/4)*32 + x%4;
                int srcx = ifloorf(srcx_f);
                int srcy = ifloorf(srcy_f);
                unsigned int pixelnum;
                CALC_PIXELNUM;
                *dest = (*get_pixel)(info, pixelnum);
            }

        } else {  // param->coefenab && param->deltaKAx != 0

            /* Multiple coefficients per row */
            const float coef_dx = param->deltaKAx;
            float coef_x = 0;
            int last_coef_x = -1;
            int empty_pixel = 0;
            unsigned int x;
            for (x = 0; x < disp_width; x++, coef_x += coef_dx) {
                uint32_t *dest = dest_base + (x/4)*32 + x%4;
                if (ifloorf(coef_x) != last_coef_x) {
                    last_coef_x = ifloorf(coef_x);
                    const uint32_t coef_addr =
                        coef_base + (last_coef_x << param->coefdatashift);
                    empty_pixel = !get_rotation_coefficient(param, coef_addr);
                }
                if (empty_pixel) {
                    *dest = 0;
                } else {
                    float srcx_f, srcy_f;
                    transform_coordinates(param, x, y, &srcx_f, &srcy_f);
                    int srcx = ifloorf(srcx_f);
                    int srcy = ifloorf(srcy_f);
                    unsigned int pixelnum;
                    CALC_PIXELNUM;
                    *dest = (*get_pixel)(info, pixelnum);
                }
            }

        }  // if (param->coefenab)

    }  // for (y = 0; y < disp_height; y++, coef_y += coef_dy)

    /* Set up vertices for optimized GE drawing */

    const unsigned int nverts = (disp_width / 16) * 2;
    VertexUVXYZ *vertices = guGetMemory(sizeof(*vertices) * nverts);
    VertexUVXYZ *vptr;
    unsigned int x;
    for (x = 0, vptr = vertices; x < disp_width; x += 16, vptr += 2) {
        vptr[0].u = x;
        vptr[0].v = 0;
        vptr[0].x = x >> disp_xscale;
        vptr[0].y = 0;
        vptr[0].z = 0;
        vptr[1].u = x + 16;
        vptr[1].v = disp_height;
        vptr[1].x = (x + 16) >> disp_xscale;
        vptr[1].y = disp_height >> disp_yscale;
        vptr[1].z = 0;
    }

    /* Send the texture to the GE */

    guTexFlush();
    guTexMode(GU_PSM_8888, 0, 0, 1 /*swizzled*/);
    if (disp_width <= 512) {
        guTexImage(0, 512, 512, disp_width, pixelbuf);
        guDrawArray(GU_SPRITES,
                    GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                    nverts, NULL, vertices);
    } else {
        guTexImage(0, 512, 512, disp_width, pixelbuf);
        guDrawArray(GU_SPRITES,
                    GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                    nverts/2, NULL, vertices);
        for (x = disp_width/2, vptr = vertices + nverts/2; x < disp_width;
             x += 16, vptr += 2
        ) {
            vptr[0].u = x - disp_width/2;
            vptr[1].u = (x + 16) - disp_width/2;
        }
        guTexImage(0, 512, 512, disp_width, pixelbuf + (disp_width/2) * 8);
        guDrawArray(GU_SPRITES,
                    GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                    nverts/2, NULL, vertices + nverts/2);
    }
    guCommit();
}

/*-----------------------------------------------------------------------*/

/**
 * get_rotation_parameters:  Retrieve the rotation parameters for a rotated
 * graphics layer and perform necessary precalculations.
 *
 * [Parameters]
 *          info: Graphics layer data
 *         which: Which parameter set to retrieve (0 or 1)
 *     param_ret: Pointer to structure to receive parsed parameters
 * [Return value]
 *     None
 */
static void get_rotation_parameters(vdp2draw_struct *info, int which,
                                    RotationParams *param_ret)
{
    uint32_t addr = (Vdp2Regs->RPTA.all << 1) & 0x7FF7C;
    unsigned int KTCTL, KTAOF;
    if (which == 0) {
        KTCTL = Vdp2Regs->KTCTL & 0xFF;
        KTAOF = Vdp2Regs->KTAOF & 0xFF;
        param_ret->screenover = (Vdp2Regs->PLSZ >> 10) & 3;
    } else {
        addr |= 0x80;
        KTCTL = (Vdp2Regs->KTCTL >> 8) & 0xFF;
        KTAOF = (Vdp2Regs->KTAOF >> 8) & 0xFF;
        param_ret->screenover = (Vdp2Regs->PLSZ >> 14) & 3;
    }
    param_ret->coefenab = KTCTL & 1;
    param_ret->coefdatashift = (KTCTL & 2) ? 1 : 2;
    param_ret->coefdatasize = 1 << param_ret->coefdatashift;
    param_ret->coefmode = (KTCTL >> 2) & 3;
    param_ret->coeftbladdr = ((KTAOF & 7) << param_ret->coefdatashift) << 16;

    #define GET_SHORT(nbits) \
        (addr += 2, ((int16_t)T1ReadWord(Vdp2Ram,addr-2) \
                      << (16-nbits)) >> (16-nbits))
    #define GET_SIGNED_FLOAT(nbits) \
        (addr += 4, ((((int32_t)T1ReadLong(Vdp2Ram,addr-4) \
                      << (32-nbits)) >> (32-nbits)) & ~0x3F) / 65536.0f)
    #define GET_UNSIGNED_FLOAT(nbits) \
        (addr += 4, (((uint32_t)T1ReadLong(Vdp2Ram,addr-4) \
                      & (0xFFFFFFFFU >> (32-nbits))) & ~0x3F) / 65536.0f)

    param_ret->Xst      = GET_SIGNED_FLOAT(29);
    param_ret->Yst      = GET_SIGNED_FLOAT(29);
    param_ret->Zst      = GET_SIGNED_FLOAT(29);
    param_ret->deltaXst = GET_SIGNED_FLOAT(19);
    param_ret->deltaYst = GET_SIGNED_FLOAT(19);
    param_ret->deltaX   = GET_SIGNED_FLOAT(19);
    param_ret->deltaY   = GET_SIGNED_FLOAT(19);
    param_ret->A        = GET_SIGNED_FLOAT(20);
    param_ret->B        = GET_SIGNED_FLOAT(20);
    param_ret->C        = GET_SIGNED_FLOAT(20);
    param_ret->D        = GET_SIGNED_FLOAT(20);
    param_ret->E        = GET_SIGNED_FLOAT(20);
    param_ret->F        = GET_SIGNED_FLOAT(20);
    param_ret->Px       = GET_SHORT(14);
    param_ret->Py       = GET_SHORT(14);
    param_ret->Pz       = GET_SHORT(14);
    addr += 2;
    param_ret->Cx       = GET_SHORT(14);
    param_ret->Cy       = GET_SHORT(14);
    param_ret->Cz       = GET_SHORT(14);
    addr += 2;
    param_ret->Mx       = GET_SIGNED_FLOAT(30);
    param_ret->My       = GET_SIGNED_FLOAT(30);
    param_ret->kx       = GET_SIGNED_FLOAT(24);
    param_ret->ky       = GET_SIGNED_FLOAT(24);
    if (param_ret->coefenab) {
        param_ret->KAst      = GET_UNSIGNED_FLOAT(32);
        param_ret->deltaKAst = GET_SIGNED_FLOAT(26);
        param_ret->deltaKAx  = GET_SIGNED_FLOAT(26);
        param_ret->coeftbladdr +=
            ifloorf(param_ret->KAst) * param_ret->coefdatasize;
    }

    #undef GET_SHORT
    #undef GET_SIGNED_FLOAT
    #undef GET_UNSIGNED_FLOAT

    /*
     * The coordinate transformation performed for rotated graphics layers
     * works out to the following:
     *
     *     [srcX]   (  [screenX])   [kx]   [Xp]
     *     [srcY] = (M [screenY]) * [ky] + [Yp]
     *              (  [   1   ])
     *
     * where the "*" operator is multiplication by components (not matrix
     * multiplication), and M is the 3x2 constant matrix product:
     *
     *         [A  B  C] [deltaX  deltaXst  (Xst - Px)]
     *     M = [D  E  F] [deltaY  deltaYst  (Yst - Py)]
     *                   [  0        0      (Zst - Pz)]
     */

    param_ret->mat11 = param_ret->A * param_ret->deltaX
                     + param_ret->B * param_ret->deltaY;
    param_ret->mat12 = param_ret->A * param_ret->deltaXst
                     + param_ret->B * param_ret->deltaYst;
    param_ret->mat13 = param_ret->A * (param_ret->Xst - param_ret->Px)
                     + param_ret->B * (param_ret->Yst - param_ret->Py)
                     + param_ret->C * (param_ret->Zst - param_ret->Pz);
    param_ret->mat21 = param_ret->D * param_ret->deltaX
                     + param_ret->E * param_ret->deltaY;
    param_ret->mat22 = param_ret->D * param_ret->deltaXst
                     + param_ret->E * param_ret->deltaYst;
    param_ret->mat23 = param_ret->D * (param_ret->Xst - param_ret->Px)
                     + param_ret->E * (param_ret->Yst - param_ret->Py)
                     + param_ret->F * (param_ret->Zst - param_ret->Pz);
    param_ret->Xp    = param_ret->A * (param_ret->Px - param_ret->Cx)
                     + param_ret->B * (param_ret->Py - param_ret->Cy)
                     + param_ret->C * (param_ret->Pz - param_ret->Cz)
                     + param_ret->Cx
                     + param_ret->Mx;
    param_ret->Yp    = param_ret->D * (param_ret->Px - param_ret->Cx)
                     + param_ret->E * (param_ret->Py - param_ret->Cy)
                     + param_ret->F * (param_ret->Pz - param_ret->Cz)
                     + param_ret->Cy
                     + param_ret->My;
}

/*----------------------------------*/

/**
 * get_rotation_coefficient:  Retrieve a single rotation coefficient for a
 * rotated graphics layer.
 *
 * [Parameters]
 *       param: Rotation parameter structure
 *     address: Address in VDP2 RAM of coefficient
 * [Return value]
 *     Zero if this pixel is blank, else nonzero
 */
static int get_rotation_coefficient(RotationParams *param, uint32_t address)
{
    float value;
    if (param->coefdatasize == 2) {
        const int16_t data = T1ReadWord(Vdp2Ram, address & 0x7FFFE);
        if (data < 0) {
            return 0;
        }
        value = (float)((data << 1) >> 1) / 1024.0f;
    } else {
        const int32_t data = T1ReadLong(Vdp2Ram, address & 0x7FFFC);
        if (data < 0) {
            return 0;
        }
        value = (float)((data << 8) >> 8) / 65536.0f;
    }
    switch (param->coefmode) {
      case 0: param->kx = param->ky = value; break;
      case 1: param->kx =             value; break;
      case 2:             param->ky = value; break;
      case 3: param->Xp = value * 256.0f;    break;
    }
    return 1;
}

/*----------------------------------*/

/**
 * transform_coordinates:  Transform screen coordinates to source
 * coordinates based on the current rotation parameters.
 *
 * [Parameters]
 *                  param: Rotation parameter structure
 *                   x, y: Screen coordinates
 *     srcx_ret, srcy_ret: Pointers to variables to receive source coordinates
 * [Return value]
 *     None
 */
static void transform_coordinates(RotationParams *param, int x, int y,
                                  float *srcx_ret, float *srcy_ret)
{
    *srcx_ret = (param->mat11*x + param->mat12*y + param->mat13)
                 * param->kx + param->Xp;
    *srcy_ret = (param->mat21*x + param->mat22*y + param->mat23)
                 * param->ky + param->Yp;
}

/*-----------------------------------------------------------------------*/

/**
 * rotation_get_pixel_*:  Retrieve a pixel from tiles or bitmaps of various
 * pixel formats.  info->charaddr is assumed to contain the offset of the
 * tile or bitmap in VDP2 RAM.
 *
 * [Parameters]
 *         info: Graphics layer data
 *     pixelnum: Index of pixel to retrieve (y*w+x)
 * [Return value]
 *     Pixel value as 0xAABBGGRR
 */

/*----------------------------------*/

static uint32_t rotation_get_pixel_t4(vdp2draw_struct *info,
                                      unsigned int pixelnum)
{
    const uint32_t address = info->charaddr + pixelnum/2;
    /* For speed, we assume the tile/bitmap won't wrap around the end of
     * VDP2 RAM */
    const uint8_t *ptr = (const uint8_t *)(Vdp2Ram + address);
    const uint8_t byte = *ptr;
    const uint8_t pixel = (pixelnum & 1) ? byte & 0x0F : byte >> 4;
    if (!pixel && info->transparencyenable) {
        return 0x00000000;
    }
    const unsigned int colornum =
        info->coloroffset + (info->paladdr<<4 | pixel);
    return global_clut_32[colornum & 0x7FF];
}

/*----------------------------------*/

static uint32_t rotation_get_pixel_t8(vdp2draw_struct *info,
                                      unsigned int pixelnum)
{
    const uint32_t address = info->charaddr + pixelnum;
    const uint8_t *ptr = (const uint8_t *)(Vdp2Ram + address);
    const uint8_t pixel = *ptr;
    if (!pixel && info->transparencyenable) {
        return 0x00000000;
    }
    const unsigned int colornum =
        info->coloroffset + (info->paladdr<<4 | pixel);
    return global_clut_32[colornum & 0x7FF];
}

/*----------------------------------*/

static uint32_t rotation_get_pixel_t16(vdp2draw_struct *info,
                                       unsigned int pixelnum)
{
    const uint32_t address = info->charaddr + pixelnum*2;
    const uint8_t *ptr = (const uint8_t *)(Vdp2Ram + address);
    const uint8_t pixel = ptr[0]<<8 | ptr[1];
    if (!pixel && info->transparencyenable) {
        return 0x00000000;
    }
    const unsigned int colornum = info->coloroffset + pixel;
    return global_clut_32[colornum & 0x7FF];
}

/*----------------------------------*/

static uint32_t rotation_get_pixel_16(vdp2draw_struct *info,
                                      unsigned int pixelnum)
{
    const uint32_t address = info->charaddr + pixelnum*2;
    const uint8_t *ptr = (const uint8_t *)(Vdp2Ram + address);
    const uint16_t pixel = ptr[0]<<8 | ptr[1];
    if (!(pixel & 0x8000) && info->transparencyenable) {
        return 0x00000000;
    }
    return 0xFF000000
         | (pixel & 0x7C00) << 9
         | (pixel & 0x03E0) << 6
         | (pixel & 0x001F) << 3;
}

/*----------------------------------*/

static uint32_t rotation_get_pixel_32(vdp2draw_struct *info,
                                      unsigned int pixelnum)
{
    const uint32_t address = info->charaddr + pixelnum*2;
    const uint8_t *ptr = (const uint8_t *)(Vdp2Ram + address);
    const uint16_t pixel = ptr[0]<<24 | ptr[1]<<16 | ptr[2]<<8 | ptr[3];
    if (!(pixel & 0x80000000) && info->transparencyenable) {
        return 0x00000000;
    }
    return 0xFF000000 | pixel;
}

/*-----------------------------------------------------------------------*/

/**
 * rotation_get_pixel_*_adjust:  Retrieve a pixel from tiles or bitmaps of
 * various pixel formats, and apply alpha and color adjustments.
 * info->charaddr is assumed to contain the offset of the tile or bitmap in
 * VDP2 RAM.
 *
 * [Parameters]
 *         info: Graphics layer data
 *     pixelnum: Index of pixel to retrieve (y*w+x)
 * [Return value]
 *     Pixel value as 0xAABBGGRR
 */

/*----------------------------------*/

static uint32_t rotation_get_pixel_t4_adjust(vdp2draw_struct *info,
                                             unsigned int pixelnum)
{
    const uint32_t address = info->charaddr + pixelnum/2;
    /* For speed, we assume the tile/bitmap won't wrap around the end of
     * VDP2 RAM */
    const uint8_t *ptr = (const uint8_t *)(Vdp2Ram + address);
    const uint8_t byte = *ptr;
    const uint8_t pixel = (pixelnum & 1) ? byte & 0x0F : byte >> 4;
    if (!pixel && info->transparencyenable) {
        return 0x00000000;
    }
    const unsigned int colornum =
        info->coloroffset + (info->paladdr<<4 | pixel);
    return (adjust_color_32_32(global_clut_32[colornum & 0x7FF],
                               info->cor, info->cog, info->cob) & 0xFFFFFF)
         | info->alpha << 24;
}

/*----------------------------------*/

static uint32_t rotation_get_pixel_t8_adjust(vdp2draw_struct *info,
                                             unsigned int pixelnum)
{
    const uint32_t address = info->charaddr + pixelnum;
    const uint8_t *ptr = (const uint8_t *)(Vdp2Ram + address);
    const uint8_t pixel = *ptr;
    if (!pixel && info->transparencyenable) {
        return 0x00000000;
    }
    const unsigned int colornum =
        info->coloroffset + (info->paladdr<<4 | pixel);
    return (adjust_color_32_32(global_clut_32[colornum & 0x7FF],
                               info->cor, info->cog, info->cob) & 0xFFFFFF)
         | info->alpha << 24;
}

/*----------------------------------*/

static uint32_t rotation_get_pixel_t16_adjust(vdp2draw_struct *info,
                                              unsigned int pixelnum)
{
    const uint32_t address = info->charaddr + pixelnum*2;
    const uint8_t *ptr = (const uint8_t *)(Vdp2Ram + address);
    const uint8_t pixel = ptr[0]<<8 | ptr[1];
    if (!pixel && info->transparencyenable) {
        return 0x00000000;
    }
    const unsigned int colornum = info->coloroffset + pixel;
    return (adjust_color_32_32(global_clut_32[colornum & 0x7FF],
                               info->cor, info->cog, info->cob) & 0xFFFFFF)
         | info->alpha << 24;
}

/*----------------------------------*/

static uint32_t rotation_get_pixel_16_adjust(vdp2draw_struct *info,
                                             unsigned int pixelnum)
{
    const uint32_t address = info->charaddr + pixelnum*2;
    const uint8_t *ptr = (const uint8_t *)(Vdp2Ram + address);
    const uint16_t pixel = ptr[0]<<8 | ptr[1];
    if (!(pixel & 0x8000) && info->transparencyenable) {
        return 0x00000000;
    }
    return (adjust_color_16_32(pixel,
                               info->cor, info->cog, info->cob) & 0xFFFFFF)
         | info->alpha << 24;
}

/*----------------------------------*/

static uint32_t rotation_get_pixel_32_adjust(vdp2draw_struct *info,
                                             unsigned int pixelnum)
{
    const uint32_t address = info->charaddr + pixelnum*2;
    const uint8_t *ptr = (const uint8_t *)(Vdp2Ram + address);
    const uint16_t pixel = ptr[0]<<24 | ptr[1]<<16 | ptr[2]<<8 | ptr[3];
    if (!(pixel & 0x80000000) && info->transparencyenable) {
        return 0x00000000;
    }
    return (adjust_color_32_32(pixel,
                               info->cor, info->cog, info->cob) & 0xFFFFFF)
         | info->alpha << 24;
}

/*************************************************************************/
/*************************************************************************/

/* Last block allocated with pspGuGetMemoryMerge() */
static void *merge_last_ptr;
static uint32_t merge_last_size;

/*-----------------------------------------------------------------------*/

/**
 * pspGuGetMemoryMerge:  Acquire a block of memory from the GE display
 * list.  Similar to scGuGetMemory(), but if the most recent display list
 * operation was also a pspGuGetMemoryMerge() call, merge the two blocks
 * together to avoid long chains of jump instructions in the display list.
 *
 * [Parameters]
 *     size: Size of block to allocate, in bytes
 * [Return value]
 *     Allocated block
 */
static void *pspGuGetMemoryMerge(uint32_t size)
{
    /* Make sure size is 32-bit aligned */
    size = (size + 3) & -4;

    /* Start off by allocating the block normally.  Ideally, we'd check
     * first whether the current list pointer is immediately past the last
     * block allocated, but since there's apparently no interface for
     * either getting the current pointer or deleting the last instruction,
     * we're out of luck and can't save the 8 bytes taken by the jump, even
     * if we end up not needing it. */
    void *ptr = guGetMemory(size);

    /* If the pointer we got back is equal to the end of the previously
     * allocated block plus 8 bytes (2 GU instructions), we can merge. */
    if ((uint8_t *)ptr == (uint8_t *)merge_last_ptr + merge_last_size + 8) {
        /* Make sure the instruction before the last block really is a
         * jump instruction before we update it. */
        uint32_t *jump_ptr = (uint32_t *)merge_last_ptr - 1;
        if (*jump_ptr >> 24 == 0x08) {
            void *block_end = (uint8_t *)ptr + size;
            *jump_ptr = 0x08<<24 | ((uintptr_t)block_end & 0xFFFFFF);
            merge_last_size = (uint8_t *)block_end - (uint8_t *)merge_last_ptr;
            return ptr;
        }
    }

    /* We couldn't merge, so reset the last-block-allocated variables and
     * return the block we allocated above. */
    merge_last_ptr = ptr;
    merge_last_size = size;
    return ptr;
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
