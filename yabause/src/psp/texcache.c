/*  src/psp/texcache.c: PSP texture cache management
    Copyright 2009 Andrew Church

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

#include "common.h"
#include "display.h"
#include "texcache.h"
#include "psp-video.h"

/*************************************************************************/

/* Structures for cached data */

typedef struct TexInfo_ TexInfo;
struct TexInfo_ {
    TexInfo *next;                // Collision chain pointer
    uint32_t key;                 // Texture key value (HASHKEY_TEXTURE_*)
    const uint8_t *vram_address;  // Address of texture in VRAM
    uint16_t stride;              // Line length in pixels
    uint16_t pixfmt;              // Pixel format (GU_PSM_*)
};

/*-----------------------------------------------------------------------*/

/* Pointer to next available and top VRAM addresses */
uint8_t *vram_next, *vram_top;

/* Cached texture hash table and associated data buffer */
static TexInfo *tex_table[PSP_TEXCACHE_SIZE];
static TexInfo tex_buffer[PSP_TEXCACHE_SIZE];
static int tex_buffer_nextfree;

/* Hash functions */
#define HASHKEY_TEXTURE_SPRITE(CMDSRCA,CMDPMOD,CMDCOLR) \
    ((CMDSRCA) | ((CMDPMOD>>3 & 7) >= 5 ? 0 : (CMDCOLR&0xFF)<<16) \
               | ((CMDPMOD>>3 & 15) << 27) | 0x80000000)
#define HASHKEY_TEXTURE_TILE(address,pixfmt,color_base,color_ofs,transparent) \
    ((address>>3) | (pixfmt >= 3 ? 0 : ((color_ofs|color_base)&0x7FF)<<16) \
                  | (pixfmt<<27) | (transparent<<30) )
#define HASH_TEXTURE(key)  ((key) % PSP_TEXCACHE_SIZE)

/*-----------------------------------------------------------------------*/

/* Local routine declarations */

/* Force GCC to inline these, to save the expense of register save/restore */
__attribute__((always_inline)) static inline void cache_texture_t4(
    TexInfo *tex, const uint8_t *src, uint32_t color_base, uint32_t color_ofs,
    int width, int height, int stride,
    int rofs, int gofs, int bofs, int transparent);
__attribute__((always_inline)) static inline void cache_texture_t8(
    TexInfo *tex, const uint8_t *src, uint8_t pixmask,
    uint32_t color_base, uint32_t color_ofs,
    int width, int height, int stride,
    int rofs, int gofs, int bofs, int transparent);
__attribute__((always_inline)) static inline void cache_texture_t16(
    TexInfo *tex, const uint8_t *src, uint32_t color_base,
    int width, int height, int stride,
    int rofs, int gofs, int bofs, int transparent);
__attribute__((always_inline)) static inline void cache_texture_16(
    TexInfo *tex, const uint8_t *src, int width, int height, int stride,
    int rofs, int gofs, int bofs, int transparent);
__attribute__((always_inline)) static inline void cache_texture_32(
    TexInfo *tex, const uint8_t *src, int width, int height, int stride,
    int rofs, int gofs, int bofs, int transparent);

/*************************************************************************/
/************************** Interface routines ***************************/
/*************************************************************************/

/**
 * texcache_reset:  Reset the texture cache, and set the global color
 * lookup table pointer and length.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void texcache_reset(void)
{
    memset(tex_table, 0, sizeof(tex_table));
    tex_buffer_nextfree = 0;
    vram_next = display_spare_vram();
    vram_top = vram_next + display_spare_vram_size();
    sceGuTexFlush();
}

/*************************************************************************/

/**
 * texcache_cache_sprite:  Cache the given sprite texture if it has not
 * been cached already.  Returns a texture key for later use in loading the
 * texture.
 *
 * [Parameters]
 *     CMDSRCA, CMDPMOD, CMDCOLR: Values of like-named fields in VDP1 command
 *                 width, height: Size of texture (in pixels)
 *              rofs, gofs, bofs: Color offset values for texture
 * [Return value]
 *     Texture key (zero on error)
 */
uint32_t texcache_cache_sprite(uint16_t CMDSRCA, uint16_t CMDPMOD,
                               uint16_t CMDCOLR, int width, int height,
                               int rofs, int gofs, int bofs)
{
    /* See if the texture is already cached */

    const uint32_t tex_key = HASHKEY_TEXTURE_SPRITE(CMDSRCA, CMDPMOD, CMDCOLR);
    const uint32_t tex_hash = HASH_TEXTURE(tex_key);
    TexInfo *tex;
    for (tex = tex_table[tex_hash]; tex != NULL; tex = tex->next) {
        if (tex->key == tex_key) {
            return tex_key;
        }
    }

    /* It's not cached, so cache it */

    if (tex_buffer_nextfree >= lenof(tex_buffer)) {
        DMSG("Texture buffer full, can't cache");
        return 0;
    }
    uint32_t vram_needed = width * height * 4;
    if (vram_next + vram_needed > vram_top) {
        DMSG("VRAM buffer full, can't cache");
        return 0;
    }
    tex = &tex_buffer[tex_buffer_nextfree++];
    tex->next = tex_table[tex_hash];
    tex_table[tex_hash] = tex;
    tex->key = tex_key;
    tex->vram_address = vram_next;
    vram_next += vram_needed;
    if ((uintptr_t)vram_next & 0x3F) {  // Make sure it stays aligned
        vram_next += 0x40 - ((uintptr_t)vram_next & 0x3F);
    }

    const int pixfmt = CMDPMOD>>3 & 7;
    if (UNLIKELY((CMDSRCA << 3) + (pixfmt<=1 ? width/2 : pixfmt<=4 ? width : width*2) * height > 0x80000)) {
        DMSG("%dx%d texture at 0x%X extends past end of VDP1 RAM"
             " and will be incorrectly drawn", width, height, CMDSRCA << 3);
    }

    const uint8_t *src = Vdp1Ram + (CMDSRCA << 3);
    uint32_t color_base = (Vdp2Regs->CRAOFB & 0x0070) << 4;
    switch (pixfmt) {
      case 0:
        cache_texture_t4(tex, src, color_base, CMDCOLR, width, height, width,
                         rofs, gofs, bofs, !(CMDPMOD & 0x40));
        break;
      case 1:
        DMSG("FIXME: pixel format 1 not yet implemented");
        return 0;
      case 2:
        cache_texture_t8(tex, src, 0x3F, color_base, CMDCOLR,
                         width, height, width,
                         rofs, gofs, bofs, !(CMDPMOD & 0x40));
        break;
      case 3:
        cache_texture_t8(tex, src, 0x7F, color_base, CMDCOLR,
                         width, height, width,
                         rofs, gofs, bofs, !(CMDPMOD & 0x40));
        break;
      case 4:
        cache_texture_t8(tex, src, 0xFF, color_base, CMDCOLR,
                         width, height, width,
                         rofs, gofs, bofs, !(CMDPMOD & 0x40));
        break;
      default:
        DMSG("unsupported pixel mode %d, assuming 16-bit", pixfmt);
        /* fall through to... */
      case 5:
        cache_texture_16(tex, src, width, height, width, rofs, gofs, bofs,
                         !(CMDPMOD & 0x40));
        break;
    }

    return tex_key;
}

/*-----------------------------------------------------------------------*/

/**
 * texcache_load_sprite:  Load the sprite texture indicated by the given
 * texture key.
 *
 * [Parameters]
 *     key: Texture key returned by texcache_cache_sprite
 * [Return value]
 *     None
 */
void texcache_load_sprite(uint32_t key)
{
    const uint32_t tex_hash = HASH_TEXTURE(key);
    TexInfo *tex;
    for (tex = tex_table[tex_hash]; tex != NULL; tex = tex->next) {
        if (tex->key == key) {
            break;
        }
    }
    if (UNLIKELY(!tex)) {
        DMSG("No texture found for key %08X", (int)key);
        return;
    }

    sceGuTexImage(0, 512, 512, tex->stride, tex->vram_address);
    sceGuTexMode(tex->pixfmt, 0, 0, 0);
}

/*-----------------------------------------------------------------------*/

/**
 * texcache_load_tile:  Load the specified tile texture into the GE
 * registers for drawing.  If the texture is not cached, caches it first.
 *
 * [Parameters]
 *              address: Tile data address within VDP2 RAM
 *               pixfmt: Tile pixel format
 *          transparent: Nonzero if index 0 or alpha 0 should be transparent
 *           color_base: Color table base (for indexed formats)
 *            color_ofs: Color table offset (for indexed formats)
 *     rofs, gofs, bofs: Color offset values for texture
 * [Return value]
 *     None
 */
void texcache_load_tile(uint32_t address, int pixfmt, int transparent,
                        uint16_t color_base, uint16_t color_ofs,
                        int rofs, int gofs, int bofs)
{
    /* See if the texture is already cached */

    const uint32_t tex_key = HASHKEY_TEXTURE_TILE(address, pixfmt, color_base,
                                                  color_ofs, transparent);
    const uint32_t tex_hash = HASH_TEXTURE(tex_key);
    TexInfo *tex;
    for (tex = tex_table[tex_hash]; tex != NULL; tex = tex->next) {
        if (tex->key == tex_key) {
            break;
        }
    }

    /* If it's not cached, do the caching now */

    if (!tex) {
        if (tex_buffer_nextfree >= lenof(tex_buffer)) {
            DMSG("Texture buffer full, can't cache");
            return;
        }
        uint32_t vram_needed = 8*8*4;
        if (vram_next + vram_needed > vram_top) {
            DMSG("VRAM buffer full, can't cache");
            return;
        }
        tex = &tex_buffer[tex_buffer_nextfree++];
        tex->next = tex_table[tex_hash];
        tex_table[tex_hash] = tex;
        tex->key = tex_key;
        tex->vram_address = vram_next;
        vram_next += vram_needed;
        if ((uintptr_t)vram_next & 0x3F) {  // Make sure it stays aligned
            vram_next += 0x40 - ((uintptr_t)vram_next & 0x3F);
        }

        if (UNLIKELY(address + (pixfmt==0 ? 8/2 : pixfmt==1 ? 8*1 : pixfmt<=3 ? 8*2 : 8*4) * 8 > 0x80000)) {
            DMSG("8x8 texture at 0x%X extends past end of VDP2 RAM"
                 " and will be incorrectly drawn", address);
        }

        const uint8_t *src = Vdp2Ram + address;
        switch (pixfmt) {
          case 0:
            cache_texture_t4(tex, src, color_base, color_ofs,
                             8, 8, 8, rofs, gofs, bofs, transparent);
            break;
          case 1:
            cache_texture_t8(tex, src, 0xFF, color_base, color_ofs,
                             8, 8, 8, rofs, gofs, bofs, transparent);
            break;
          case 2:
            cache_texture_t16(tex, src,color_base,
                              8, 8, 8, rofs, gofs, bofs, transparent);
            break;
          case 3:
            cache_texture_16(tex, src, 8, 8, 8, rofs, gofs, bofs, transparent);
            break;
          case 4:
            cache_texture_32(tex, src, 8, 8, 8, rofs, gofs, bofs, transparent);
            break;
        }
    }  // if (!tex)

    /* Load the texture data */
    sceGuTexImage(0, 512, 512, tex->stride, tex->vram_address);
    sceGuTexMode(tex->pixfmt, 0, 0, 0);
    if (tex->pixfmt == GU_PSM_T8) {
        sceGuClutMode(GU_PSM_8888, 0, 0xFF, 0);
        sceGuClutLoad(256/8, &global_clut_32[color_base + color_ofs]);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * texcache_load_bitmap:  Load the specified bitmap texture into the GE
 * registers for drawing.  If the texture is not cached, caches it first.
 *
 * [Parameters]
 *              address: Bitmap data address within VDP2 RAM
 *        width, height: Size of texture (in pixels)
 *               stride: Line size of source data (in pixels)
 *               pixfmt: Bitmap pixel format
 *          transparent: Nonzero if index 0 or alpha 0 should be transparent
 *           color_base: Color table base (for indexed formats)
 *            color_ofs: Color table offset (for indexed formats)
 *     rofs, gofs, bofs: Color offset values for texture
 * [Return value]
 *     None
 */
void texcache_load_bitmap(uint32_t address, int width, int height,
                          int stride, int pixfmt, int transparent,
                          uint16_t color_base, uint16_t color_ofs,
                          int rofs, int gofs, int bofs)
{
    /* NOTE: This is essentially a generalized version of
     * texcache_load_tile() */

    int outstride = (width + 3) & ~3;  // Stride must be a multiple of 16 bytes

    /* See if the texture is already cached */

    const uint32_t tex_key = HASHKEY_TEXTURE_TILE(address, pixfmt, color_base,
                                                  color_ofs, transparent);
    const uint32_t tex_hash = HASH_TEXTURE(tex_key);
    TexInfo *tex;
    for (tex = tex_table[tex_hash]; tex != NULL; tex = tex->next) {
        if (tex->key == tex_key) {
            break;
        }
    }

    /* If it's not cached, do the caching now */

    if (!tex) {
        if (tex_buffer_nextfree >= lenof(tex_buffer)) {
            DMSG("Texture buffer full, can't cache");
            return;
        }
        uint32_t vram_needed = outstride * height * 4;
        if (vram_next + vram_needed > vram_top) {
            DMSG("VRAM buffer full, can't cache");
            return;
        }
        tex = &tex_buffer[tex_buffer_nextfree++];
        tex->next = tex_table[tex_hash];
        tex_table[tex_hash] = tex;
        tex->key = tex_key;
        tex->vram_address = vram_next;
        vram_next += vram_needed;
        if ((uintptr_t)vram_next & 0x3F) {  // Make sure it stays aligned
            vram_next += 0x40 - ((uintptr_t)vram_next & 0x3F);
        }

        if (UNLIKELY(address + (pixfmt==0 ? width/2 : pixfmt==1 ? width*1 : pixfmt<=3 ? width*2 : width*4) * height > 0x80000)) {
            DMSG("%dx%d texture at 0x%X extends past end of VDP2 RAM"
                 " and will be incorrectly drawn", width, height, address);
        }

        const uint8_t *src = Vdp2Ram + address;
        switch (pixfmt) {
          case 0:
            cache_texture_t4(tex, src, color_base, color_ofs,
                             outstride, height, stride,
                             rofs, gofs, bofs, transparent);
            break;
          case 1:
            cache_texture_t8(tex, src, 0xFF, color_base, color_ofs,
                             outstride, height, stride,
                             rofs, gofs, bofs, transparent);
            break;
          case 2:
            cache_texture_t16(tex, src, color_base, outstride, height, stride,
                              rofs, gofs, bofs, transparent);
            break;
          case 3:
            cache_texture_16(tex, src, outstride, height, stride,
                             rofs, gofs, bofs, transparent);
            break;
          case 4:
            cache_texture_32(tex, src, outstride, height, stride,
                             rofs, gofs, bofs, transparent);
            break;
        }
    }  // if (!tex)

    /* Load the texture data */
    sceGuTexImage(0, 512, 512, tex->stride, tex->vram_address);
    sceGuTexMode(tex->pixfmt, 0, 0, 0);
    if (tex->pixfmt == GU_PSM_T8) {
        sceGuClutMode(GU_PSM_8888, 0, 0xFF, 0);
        sceGuClutLoad(256/8, &global_clut_32[color_base + color_ofs]);
    }
}

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * cache_texture_t4:  Cache a 4-bit indexed texture.
 *
 * [Parameters]
 *                  tex: TexInfo structure for texture
 *                  src: Pointer to VDP1/VDP2 texture data
 *           color_base: Base color index
 *            color_ofs: Color index offset or'd together with pixel
 *        width, height: Size of texture (in pixels)
 *               stride: Line size of source data (in pixels)
 *     rofs, gofs, bofs: Color offset values for texture
 *          transparent: Nonzero if pixel value 0 should be transparent
 * [Return value]
 *     None
 */
static inline void cache_texture_t4(
    TexInfo *tex, const uint8_t *src, uint32_t color_base, uint32_t color_ofs,
    int width, int height, int stride,
    int rofs, int gofs, int bofs, int transparent)
{
    tex->stride = width;
    tex->pixfmt = GU_PSM_8888;
    uint32_t *dest = (uint32_t *)tex->vram_address;
    const uint32_t *clut_32 = &global_clut_32[color_base];
    if (rofs || gofs || bofs) {
        int y;
        for (y = 0; y < height; y++, src += (stride - width) / 2) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src++, dest += 2) {
                uint8_t pixel;
                uint32_t color;
                pixel = *src >> 4;
                if (transparent && !pixel) {
                    dest[0] = 0;
                } else {
                    color = clut_32[pixel | color_ofs];
                    dest[0] = adjust_color_32_32(color, rofs, gofs, bofs);
                }
                pixel = *src & 0x0F;
                if (transparent && !pixel) {
                    dest[1] = 0;
                } else {
                    color = clut_32[pixel | color_ofs];
                    dest[1] = adjust_color_32_32(color, rofs, gofs, bofs);
                }
            }
        }
    } else if (width != stride) {
        int y;
        for (y = 0; y < height; y++, src += (stride - width) / 2) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 2, dest += 4) {
                uint8_t byte0 = src[0];
                uint8_t byte1 = src[1];
                uint8_t pixel;
                pixel = byte0 >> 4;
                dest[0] = (transparent && !pixel) ? 0 : clut_32[pixel | color_ofs];
                pixel = byte0 & 0x0F;
                dest[1] = (transparent && !pixel) ? 0 : clut_32[pixel | color_ofs];
                pixel = byte1 >> 4;
                dest[2] = (transparent && !pixel) ? 0 : clut_32[pixel | color_ofs];
                pixel = byte1 & 0x0F;
                dest[3] = (transparent && !pixel) ? 0 : clut_32[pixel | color_ofs];
            }
        }
    } else {
        uint32_t *dest_top = dest + (width*height);
        for (; dest < dest_top; src += 2, dest += 4) {
            uint8_t byte0 = src[0];
            uint8_t byte1 = src[1];
            uint8_t pixel;
            pixel = byte0 >> 4;
            dest[0] = (transparent && !pixel) ? 0 : clut_32[pixel | color_ofs];
            pixel = byte0 & 0x0F;
            dest[1] = (transparent && !pixel) ? 0 : clut_32[pixel | color_ofs];
            pixel = byte1 >> 4;
            dest[2] = (transparent && !pixel) ? 0 : clut_32[pixel | color_ofs];
            pixel = byte1 & 0x0F;
            dest[3] = (transparent && !pixel) ? 0 : clut_32[pixel | color_ofs];
        }
    }
    sceKernelDcacheWritebackRange(tex->vram_address, width*height*4);
}

/*************************************************************************/

/**
 * cache_texture_t8:  Cache an 8-bit indexed texture.
 *
 * [Parameters]
 *                  tex: TexInfo structure for texture
 *                  src: Pointer to VDP1/VDP2 texture data
 *              pixmask: Pixel data mask
 *           color_base: Base color index
 *            color_ofs: Color index offset or'd together with pixel
 *        width, height: Size of texture (in pixels)
 *               stride: Line size of source data (in pixels)
 *     rofs, gofs, bofs: Color offset values for texture
 *          transparent: Nonzero if pixel value 0 should be transparent
 * [Return value]
 *     None
 */
static inline void cache_texture_t8(
    TexInfo *tex, const uint8_t *src, uint8_t pixmask,
    uint32_t color_base, uint32_t color_ofs,
    int width, int height, int stride,
    int rofs, int gofs, int bofs, int transparent)
{
    tex->stride = width;
    tex->pixfmt = GU_PSM_8888;
    uint32_t *dest = (uint32_t *)tex->vram_address;
    const uint32_t *clut_32 = &global_clut_32[color_base];
    if (rofs || gofs || bofs) {
        int y;
        for (y = 0; y < height; y++, src += (stride - width)) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src++, dest++) {
                uint8_t pixel = *src & pixmask;
                if (transparent && !pixel) {
                    *dest = 0;
                } else {
                    uint32_t color = clut_32[pixel | color_ofs];
                    *dest = adjust_color_32_32(color, rofs, gofs, bofs);
                }
            }
        }
    } else if (width != stride) {
        int y;
        for (y = 0; y < height; y++, src += (stride - width)) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 4, dest += 4) {
                uint8_t pixel0 = src[0] & pixmask;
                uint8_t pixel1 = src[1] & pixmask;
                uint8_t pixel2 = src[2] & pixmask;
                uint8_t pixel3 = src[3] & pixmask;
                dest[0] = (transparent && !pixel0) ? 0 :
                    clut_32[pixel0 | color_ofs];
                dest[1] = (transparent && !pixel1) ? 0 :
                    clut_32[pixel1 | color_ofs];
                dest[2] = (transparent && !pixel2) ? 0 :
                    clut_32[pixel2 | color_ofs];
                dest[3] = (transparent && !pixel3) ? 0 :
                    clut_32[pixel3 | color_ofs];
            }
        }
    } else {
        uint32_t *dest_top = dest + (width*height);
        for (; dest < dest_top; src += 4, dest += 4) {
            uint8_t pixel0 = src[0] & pixmask;
            uint8_t pixel1 = src[1] & pixmask;
            uint8_t pixel2 = src[2] & pixmask;
            uint8_t pixel3 = src[3] & pixmask;
            dest[0] = (transparent && !pixel0) ? 0 :
                clut_32[pixel0 | color_ofs];
            dest[1] = (transparent && !pixel1) ? 0 :
                clut_32[pixel1 | color_ofs];
            dest[2] = (transparent && !pixel2) ? 0 :
                clut_32[pixel2 | color_ofs];
            dest[3] = (transparent && !pixel3) ? 0 :
                clut_32[pixel3 | color_ofs];
        }
    }
    sceKernelDcacheWritebackRange(tex->vram_address, width*height*4);
}

/*************************************************************************/

/**
 * cache_texture_t16:  Cache a 16-bit indexed texture.
 *
 * [Parameters]
 *                  tex: TexInfo structure for texture
 *                  src: Pointer to VDP1/VDP2 texture data
 *           color_base: Base color index
 *        width, height: Size of texture (in pixels)
 *               stride: Line size of source data (in pixels)
 *     rofs, gofs, bofs: Color offset values for texture
 *          transparent: Nonzero if pixel value 0 should be transparent
 * [Return value]
 *     None
 */
static inline void cache_texture_t16(
    TexInfo *tex, const uint8_t *src, uint32_t color_base,
    int width, int height, int stride,
    int rofs, int gofs, int bofs, int transparent)
{
    tex->stride = width;
    tex->pixfmt = GU_PSM_8888;
    uint32_t *dest = (uint32_t *)tex->vram_address;
    const uint32_t *clut_32 = &global_clut_32[color_base];
    if (rofs || gofs || bofs) {
        int y;
        for (y = 0; y < height; y++, src += (stride - width) * 2) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 2, dest++) {
                uint16_t pixel = src[0]<<8 | src[1];
                if (transparent && !pixel) {
                    *dest = 0;
                } else {
                    *dest = adjust_color_32_32(clut_32[pixel & 0x7FF],
                                               rofs, gofs, bofs);
                }
            }
        }
    } else if (width != stride) {
        int y;
        for (y = 0; y < height; y++, src += (stride - width) * 2) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 4, dest += 2) {
                uint16_t pixel0 = src[0]<<8 | src[1];
                uint16_t pixel1 = src[2]<<8 | src[3];
                dest[0] = (transparent && !pixel0) ? 0 : clut_32[pixel0 & 0x7FF];
                dest[1] = (transparent && !pixel1) ? 0 : clut_32[pixel1 & 0x7FF];
            }
        }
    } else {
        uint32_t *dest_top = dest + (width*height);
        for (; dest < dest_top; src += 4, dest += 2) {
            uint16_t pixel0 = src[0]<<8 | src[1];
            uint16_t pixel1 = src[2]<<8 | src[3];
            dest[0] = (transparent && !pixel0) ? 0 : clut_32[pixel0 & 0x7FF];
            dest[1] = (transparent && !pixel1) ? 0 : clut_32[pixel1 & 0x7FF];
        }
    }
    sceKernelDcacheWritebackRange(tex->vram_address, width*height*4);
}

/*************************************************************************/

/**
 * cache_texture_16:  Cache a 16-bit RGB555 texture.
 *
 * [Parameters]
 *                  tex: TexInfo structure for texture
 *                  src: Pointer to VDP1/VDP2 texture data
 *        width, height: Size of texture (in pixels)
 *               stride: Line size of source data (in pixels)
 *     rofs, gofs, bofs: Color offset values for texture
 *          transparent: Nonzero if alpha-0 pixels should be transparent
 * [Return value]
 *     None
 */
static inline void cache_texture_16(
    TexInfo *tex, const uint8_t *src, int width, int height, int stride,
    int rofs, int gofs, int bofs, int transparent)
{
    tex->stride = width;
    tex->pixfmt = GU_PSM_8888;
    uint32_t *dest = (uint32_t *)tex->vram_address;
    if (rofs || gofs || bofs) {
        int y;
        for (y = 0; y < height; y++, src += (stride - width) * 2) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 2, dest++) {
                uint16_t pixel = src[0]<<8 | src[1];
                *dest = (transparent && !pixel) ? 0 :
                    adjust_color_16_32(pixel, rofs, gofs, bofs);
            }
        }
    } else if (width != stride) {
        int y;
        for (y = 0; y < height; y++, src += (stride - width) * 2) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 4, dest += 2) {
                uint16_t pixel0 = src[0]<<8 | src[1];
                uint16_t pixel1 = src[2]<<8 | src[3];
                dest[0] = (transparent && !(pixel0>>15)) ? 0 :
                    0xFF000000 | (pixel0 & 0x7C00) << 9
                               | (pixel0 & 0x03E0) << 6
                               | (pixel0 & 0x001F) << 3;
                dest[1] = (transparent && !(pixel1>>15)) ? 0 :
                    0xFF000000 | (pixel1 & 0x7C00) << 9
                               | (pixel1 & 0x03E0) << 6
                               | (pixel1 & 0x001F) << 3;
            }
        }
    } else {
        uint32_t *dest_top = dest + (width*height);
        for (; dest < dest_top; src += 4, dest += 2) {
            uint16_t pixel0 = src[0]<<8 | src[1];
            uint16_t pixel1 = src[2]<<8 | src[3];
            dest[0] = (transparent && !(pixel0>>15)) ? 0 :
                0xFF000000 | (pixel0 & 0x7C00) << 9
                           | (pixel0 & 0x03E0) << 6
                           | (pixel0 & 0x001F) << 3;
            dest[1] = (transparent && !(pixel1>>15)) ? 0 :
                0xFF000000 | (pixel1 & 0x7C00) << 9
                           | (pixel1 & 0x03E0) << 6
                           | (pixel1 & 0x001F) << 3;
        }
    }
    sceKernelDcacheWritebackRange(tex->vram_address, width*height*4);
}

/*************************************************************************/

/**
 * cache_texture_32:  Cache a 32-bit ARGB1888 texture.
 *
 * [Parameters]
 *                  tex: TexInfo structure for texture
 *                  src: Pointer to VDP1/VDP2 texture data
 *        width, height: Size of texture (in pixels)
 *               stride: Line size of source data (in pixels)
 *     rofs, gofs, bofs: Color offset values for texture
 *          transparent: Nonzero if alpha-0 pixels should be transparent
 * [Return value]
 *     None
 */
static inline void cache_texture_32(
    TexInfo *tex, const uint8_t *src, int width, int height, int stride,
    int rofs, int gofs, int bofs, int transparent)
{
    tex->stride = width;
    tex->pixfmt = GU_PSM_8888;
    uint32_t *dest = (uint32_t *)tex->vram_address;
    if (rofs || gofs || bofs) {
        int y;
        for (y = 0; y < height; y++, src += (stride - width) * 4) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 4, dest++) {
                int8_t a = src[0];
                uint8_t r = src[3], g = src[2], b = src[1];
                *dest = (transparent && a >= 0) ? 0 :
                    0xFF000000 | bound(r+rofs,0,255) <<  0
                               | bound(g+gofs,0,255) <<  8
                               | bound(b+bofs,0,255) << 16;
            }
        }
    } else if (width != stride) {
        int y;
        for (y = 0; y < height; y++, src += (stride - width) * 4) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 4, dest++) {
                int8_t a = src[0];
                uint8_t r = src[3], g = src[2], b = src[1];
                *dest = (transparent && a >= 0) ? 0 :
                    0xFF000000 | r<<0 | g<<8 | b<<16;
            }
        }
    } else {
        uint32_t *dest_top = dest + (width*height);
        for (; dest < dest_top; src += 4, dest++) {
            int8_t a = src[0];
            uint8_t r = src[3], g = src[2], b = src[1];
            *dest = (transparent && a >= 0) ? 0 :
                0xFF000000 | r<<0 | g<<8 | b<<16;
        }
    }
    sceKernelDcacheWritebackRange(tex->vram_address, width*height*4);
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
