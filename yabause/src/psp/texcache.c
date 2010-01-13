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

#include "common.h"

#include "../vdp1.h"
#include "../vdp2.h"

#include "display.h"
#include "gu.h"
#include "psp-video.h"
#include "texcache.h"

/*************************************************************************/
/************************* Configuration options *************************/
/*************************************************************************/

/**
 * TEXCACHE_SIZE:  Number of entries in the texture cache.  This value does
 * not have any direct impact on processing speed, but each entry requires
 * 24 bytes of memory.
 *
 * Note that a single background layer at maximum resolution (704x512)
 * using flipped 16x16 tiles requires (88+4)*(64+4) = 6256 textures.
 */
#ifndef TEXCACHE_SIZE
# define TEXCACHE_SIZE 30000
#endif

/**
 * TEXCACHE_HASH_SIZE:  Number of hash table slots in the texture cache.
 * A larger value increases the fixed setup time for each frame, but can
 * reduce time consumed in traversing hash table collision chains.  This
 * value should always be a prime number.
 */
#ifndef TEXCACHE_HASH_SIZE
# define TEXCACHE_HASH_SIZE 1009
#endif

/**
 * CLUT_ENTRIES:  Number of entries per CLUT (color lookup table) cache
 * slot.  The CLUT cache stores generated tables in the slot designated by
 * the upper 7 bits of the base color index; this value sets how many
 * different CLUTs (i.e. with different color offsets, transparency flags,
 * or color_ofs values) will be cached for any given base color index.
 *
 * If all entries for a given CLUT cache slot are filled, any color set
 * that does not match a cached set will cause a new CLUT to be generated
 * for each texture using that color set.
 */
#ifndef CLUT_ENTRIES
# define CLUT_ENTRIES 4
#endif

/*************************************************************************/
/****************************** Local data *******************************/
/*************************************************************************/

/* Structure for cached texture data */

typedef struct TexInfo_ TexInfo;
struct TexInfo_ {
    TexInfo *next;      // Collision chain pointer
    uint32_t key;       // Texture key value (HASHKEY_TEXTURE_*(...))
    void *vram_address; // Address of texture in VRAM
    void *clut_address; // Address of color table in VRAM (NULL if none)
    uint16_t clut_size; // Number of entries in CLUT table
    uint16_t stride;    // Line length in pixels
    uint16_t pixfmt;    // Texture pixel format (GU_PSM_*)
    uint16_t width;     // Texture width (always a power of 2)
    uint16_t height;    // Texture height (always a power of 2)
};

/* Structure for cached color lookup table data */

typedef struct CLUTInfo_ CLUTInfo;
struct CLUTInfo_ {
    uint16_t size;      // Number of colors (0 = unused entry)
    uint8_t color_ofs;  // color_ofs value ORed into pixel
    uint32_t color_set; // (rofs&0x1FF)<<0 | (gofs&0x1FF)<<9 | (bofs&0x1FF)<<18
                        // | (transparent ? 1<<27 : 0)
    void *clut_address; // CLUT data pointer
};

/*************************************************************************/

/* Pointer to next available and top VRAM addresses */
uint8_t *vram_next, *vram_top;

/* Cached texture hash table and associated data buffer */
static TexInfo *tex_table[TEXCACHE_HASH_SIZE];
static TexInfo tex_buffer[TEXCACHE_SIZE];
static int tex_buffer_nextfree;

/* Cached color lookup tables, indexed by upper 7 bits of global color number*/
static CLUTInfo clut_cache[128][CLUT_ENTRIES];

/* Hash functions */
#define HASHKEY_TEXTURE_SPRITE(CMDSRCA,CMDPMOD,CMDCOLR) \
    ((CMDSRCA) | ((CMDPMOD>>3 & 7) >= 5 ? 0 : (CMDCOLR&0xFF)<<16) \
               | ((CMDPMOD>>3 & 15) << 27) | 0x80000000)
#define HASHKEY_TEXTURE_TILE(address,pixfmt,color_base,color_ofs,transparent) \
    ((address>>3) | (pixfmt >= 3 ? 0 : ((color_ofs|color_base)&0x7FF)<<16) \
                  | (pixfmt<<27) | (transparent<<30) )
#define HASH_TEXTURE(key)  ((key) % TEXCACHE_HASH_SIZE)

/*************************************************************************/

/* Local routine declarations */

static TexInfo *alloc_texture(uint32_t key, uint32_t hash);
static TexInfo *find_texture(uint32_t key, unsigned int *hash_ret);
static void load_texture(const TexInfo *tex);
static void *alloc_vram(uint32_t size);

/* Force GCC to inline the texture/CLUT caching functions, to save the
 * expense of register save/restore and optimize constant cases
 * (particularly for tile loading) */

__attribute__((always_inline)) static inline int cache_sprite(
    TexInfo *tex, uint16_t CMDSRCA, uint16_t CMDPMOD, uint16_t CMDCOLR,
    unsigned int width, unsigned int height, int rofs, int gofs, int bofs);
__attribute__((always_inline)) static inline int cache_tile(
    TexInfo *tex, uint32_t address, unsigned int width, unsigned int height,
    unsigned int stride, int pixfmt, int transparent,
    uint16_t color_base, uint16_t color_ofs, int rofs, int gofs, int bofs);

__attribute__((always_inline)) static inline int cache_texture_t4(
    TexInfo *tex, const uint8_t *src,
    unsigned int width, unsigned int height, unsigned int stride);
__attribute__((always_inline)) static inline int cache_texture_t8(
    TexInfo *tex, const uint8_t *src, uint8_t pixmask,
    unsigned int width, unsigned int height, unsigned int stride);
__attribute__((always_inline)) static inline int cache_texture_t16(
    TexInfo *tex, const uint8_t *src, uint32_t color_base,
    unsigned int width, unsigned int height, unsigned int stride,
    int rofs, int gofs, int bofs, int transparent);
__attribute__((always_inline)) static inline int cache_texture_16(
    TexInfo *tex, const uint8_t *src,
    unsigned int width, unsigned int height, unsigned int stride,
    int rofs, int gofs, int bofs, int transparent);
__attribute__((always_inline)) static inline int cache_texture_32(
    TexInfo *tex, const uint8_t *src,
    unsigned int width, unsigned int height, unsigned int stride,
    int rofs, int gofs, int bofs, int transparent);

__attribute__((always_inline)) static inline int gen_clut(
    TexInfo *tex, unsigned int size, uint32_t color_base, uint8_t color_ofs,
    int rofs, int gofs, int bofs, int transparent);
__attribute__((always_inline)) static inline int gen_clut_t4ind(
    TexInfo *tex, const uint8_t *color_lut, int rofs,
    int gofs, int bofs, int transparent);

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

    unsigned int i;
    for (i = 0; i < lenof(clut_cache); i++) {
        unsigned int j;
        for (j = 0; j < lenof(clut_cache[i]); j++) {
            clut_cache[i][j].size = 0;
        }
    }

    vram_next = display_spare_vram();
    vram_top = vram_next + display_spare_vram_size();
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
                               uint16_t CMDCOLR,
                               unsigned int width, unsigned int height,
                               int rofs, int gofs, int bofs)
{
    const uint32_t tex_key = HASHKEY_TEXTURE_SPRITE(CMDSRCA, CMDPMOD, CMDCOLR);
    uint32_t tex_hash;
    if (find_texture(tex_key, &tex_hash)) {
        /* Already cached, so just return the key */
        return tex_key;
    }

    TexInfo *tex = alloc_texture(tex_key, tex_hash);
    if (UNLIKELY(!tex)) {
        DMSG("Texture buffer full, can't cache");
        return 0;
    }
    if (UNLIKELY(!cache_sprite(tex, CMDSRCA, CMDPMOD, CMDCOLR,
                               width, height, rofs, gofs, bofs))) {
        return 0;
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
    TexInfo *tex = find_texture(key, NULL);
    if (UNLIKELY(!tex)) {
        DMSG("No texture found for key %08X", (int)key);
        return;
    }

    load_texture(tex);
}

/*-----------------------------------------------------------------------*/

/**
 * texcache_load_tile:  Load the specified 8x8 tile texture into the GE
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
    const uint32_t tex_key = HASHKEY_TEXTURE_TILE(address, pixfmt, color_base,
                                                  color_ofs, transparent);
    uint32_t tex_hash;
    TexInfo *tex = find_texture(tex_key, &tex_hash);

    if (!tex) {
        tex = alloc_texture(tex_key, tex_hash);
        if (UNLIKELY(!tex)) {
            DMSG("Texture buffer full, can't cache");
            return;
        }
        if (UNLIKELY(!cache_tile(tex, address, 8, 8, 8, pixfmt, transparent,
                                 color_base, color_ofs, rofs, gofs, bofs))) {
            return;
        }
    }  // if (!tex)

    /* Load the texture data (and color table, if appropriate) */
    load_texture(tex);
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
void texcache_load_bitmap(uint32_t address, unsigned int width,
                          unsigned int height, unsigned int stride,
                          int pixfmt, int transparent,
                          uint16_t color_base, uint16_t color_ofs,
                          int rofs, int gofs, int bofs)
{
    const uint32_t tex_key = HASHKEY_TEXTURE_TILE(address, pixfmt, color_base,
                                                  color_ofs, transparent);
    uint32_t tex_hash;
    TexInfo *tex = find_texture(tex_key, &tex_hash);

    if (!tex) {
        tex = alloc_texture(tex_key, tex_hash);
        if (UNLIKELY(!tex)) {
            DMSG("Texture buffer full, can't cache");
            return;
        }
        if (UNLIKELY(!cache_tile(tex, address, width, height, stride,
                                 pixfmt, transparent,
                                 color_base, color_ofs, rofs, gofs, bofs))) {
            return;
        }
    }  // if (!tex)

    /* Load the texture data (and color table, if appropriate) */
    load_texture(tex);
}

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * alloc_texture:  Allocate a new texture entry and insert it into the hash
 * table.  The entry's data fields (other than those for hash table
 * management) are _not_ initialized.
 *
 * [Parameters]
 *      key: Hash key for texture
 *     hash: Hash index for texture (== HASH_TEXTURE(key), passed in
 *              separately so precomputed values can be reused)
 * [Return value]
 *     Allocated texture entry, or NULL on failure (table full)
 */
static TexInfo *alloc_texture(uint32_t key, uint32_t hash)
{
    if (UNLIKELY(tex_buffer_nextfree >= lenof(tex_buffer))) {
        return NULL;
    }
    TexInfo *tex = &tex_buffer[tex_buffer_nextfree++];
    tex->next = tex_table[hash];
    tex_table[hash] = tex;
    tex->key = key;
    return tex;
}
/*-----------------------------------------------------------------------*/

/**
 * find_texture:  Return the texture corresponding to the given texture
 * key, or NULL if no such texture is cached.
 *
 * [Parameters]
 *          key: Texture hash key
 *     hash_ret: Pointer to variable to receive hash index, or NULL if unneeded
 * [Return value]
 *     Cached texture, or NULL if none
 */
static TexInfo *find_texture(uint32_t key, unsigned int *hash_ret)
{
    const uint32_t hash = HASH_TEXTURE(key);
    if (hash_ret) {
        *hash_ret = hash;
    }

    TexInfo *tex;
    for (tex = tex_table[hash]; tex != NULL; tex = tex->next) {
        if (tex->key == key) {
            return tex;
        }
    }
    return NULL;
}

/*-----------------------------------------------------------------------*/

/**
 * load_texture:  Load the given texture into the GE texture registers.
 *
 * [Parameters]
 *     tex: Texture to load
 * [Return value]
 *     None
 */
static void load_texture(const TexInfo *tex)
{
    if (tex->clut_address) {
        guClutMode(GU_PSM_8888, 0, tex->clut_size-1, 0);
        guClutLoad(tex->clut_size/8, tex->clut_address);
    }
    guTexFlush();
    guTexMode(tex->pixfmt, 0, 0, 0);
    guTexImage(0, tex->width, tex->height, tex->stride, tex->vram_address);
}

/*************************************************************************/

/**
 * alloc_vram:  Allocate memory from the spare VRAM area.  All allocated
 * VRAM will be automatically freed after the frame has been drawn.
 *
 * [Parameters]
 *     size: Amount of memory to allocate, in bytes
 * [Return value]
 *     Pointer to allocated memory, or NULL on failure (out of memory)
 */
static void *alloc_vram(uint32_t size)
{
    if (vram_next + size > vram_top) {
        return NULL;
    }
    void *ptr = vram_next;
    vram_next += size;
    if ((uintptr_t)vram_next & 0x3F) {  // Make sure it stays aligned
        vram_next += 0x40 - ((uintptr_t)vram_next & 0x3F);
    }
    return (void *)((uint32_t)ptr | 0x40000000);
}

/*************************************************************************/

/**
 * cache_sprite:  Cache a texture from sprite (VDP2) memory.  Implements
 * caching code for texcache_cache_sprite().
 *
 * [Parameters]
 *                           tex: TexInfo structure for caching texture
 *     CMDSRCA, CMDPMOD, CMDCOLR: Values of like-named fields in VDP1 command
 *                 width, height: Size of texture (in pixels)
 *              rofs, gofs, bofs: Color offset values for texture
 * [Return value]
 *     Nonzero on success, zero on error
 */
static inline int cache_sprite(
    TexInfo *tex, uint16_t CMDSRCA, uint16_t CMDPMOD, uint16_t CMDCOLR,
    unsigned int width, unsigned int height, int rofs, int gofs, int bofs)
{
    const uint32_t address = CMDSRCA << 3;
    const int pixfmt = CMDPMOD>>3 & 7;
    const int transparent = !(CMDPMOD & 0x40);
    uint32_t color_base = (Vdp2Regs->CRAOFB & 0x0070) << 4;

    if (UNLIKELY(address + (pixfmt<=1 ? width/2 : pixfmt<=4 ? width : width*2) * height > 0x80000)) {
        DMSG("%dx%d texture at 0x%X extends past end of VDP1 RAM"
             " and will be incorrectly drawn", width, height, address);
    }

    tex->width  = 1 << (32 - __builtin_clz(width-1));
    tex->height = 1 << (32 - __builtin_clz(height-1));

    const uint8_t *src = Vdp1Ram + address;
    switch (pixfmt) {
      case 0:
        return gen_clut(tex, 16,
                        color_base + (CMDCOLR & 0x7F0), CMDCOLR & 0x00F,
                        rofs, gofs, bofs, transparent)
            && cache_texture_t4(tex, src, width, height, width);
      case 1:
        return gen_clut_t4ind(tex, Vdp1Ram + (CMDCOLR << 3),
                              rofs, gofs, bofs, transparent)
            && cache_texture_t4(tex, src, width, height, width);
      case 2:
        return gen_clut(tex, 64,
                        color_base + (CMDCOLR & 0x7C0), CMDCOLR & 0x03F,
                        rofs, gofs, bofs, transparent)
            && cache_texture_t8(tex, src, 0x3F, width, height, width);
      case 3:
        return gen_clut(tex, 128,
                        color_base + (CMDCOLR & 0x780), CMDCOLR & 0x07F,
                        rofs, gofs, bofs, transparent)
            && cache_texture_t8(tex, src, 0x7F, width, height, width);
      case 4:
        return gen_clut(tex, 256,
                        color_base + (CMDCOLR & 0x700), CMDCOLR & 0x0FF,
                        rofs, gofs, bofs, transparent)
            && cache_texture_t8(tex, src, 0xFF, width, height, width);
      default:
        DMSG("unsupported pixel mode %d, assuming 16-bit", pixfmt);
        /* fall through to... */
      case 5:
        return cache_texture_16(tex, src, width, height, width,
                                rofs, gofs, bofs, transparent);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * cache_tile:  Cache a texture from tile (VDP2) memory.  Implements common
 * code for texcache_load_tile() and texcache_load_bitmap().
 *
 * [Parameters]
 *                  tex: TexInfo structure for caching texture
 *              address: Bitmap data address within VDP2 RAM
 *        width, height: Size of texture (in pixels)
 *               stride: Line size of source data (in pixels)
 *               pixfmt: Bitmap pixel format
 *          transparent: Nonzero if index 0 or alpha 0 should be transparent
 *           color_base: Color table base (for indexed formats)
 *            color_ofs: Color table offset (for indexed formats)
 *     rofs, gofs, bofs: Color offset values for texture
 * [Return value]
 *     Nonzero on success, zero on error
 */
static inline int cache_tile(
    TexInfo *tex, uint32_t address, unsigned int width, unsigned int height,
    unsigned int stride, int pixfmt, int transparent,
    uint16_t color_base, uint16_t color_ofs, int rofs, int gofs, int bofs)
{
    if (UNLIKELY(address + (pixfmt==0 ? width/2 : pixfmt==1 ? width*1 : pixfmt<=3 ? width*2 : width*4) * height > 0x80000)) {
        DMSG("%dx%d texture at 0x%X extends past end of VDP2 RAM"
             " and will be incorrectly drawn", width, height, address);
    }

    tex->width  = 1 << (32 - __builtin_clz(width-1));
    tex->height = 1 << (32 - __builtin_clz(height-1));

    const uint8_t *src = Vdp2Ram + address;
    switch (pixfmt) {
      case 0:
        return gen_clut(tex, 16,
                        color_base + (color_ofs & 0x7F0), color_ofs & 0x00F,
                        rofs, gofs, bofs, transparent)
            && cache_texture_t4(tex, src, width, height, stride);
      case 1:
        return gen_clut(tex, 256,
                        color_base + (color_ofs & 0x700), color_ofs & 0x0FF,
                        rofs, gofs, bofs, transparent)
            && cache_texture_t8(tex, src, 0xFF, width, height, stride);
      case 2:
        return cache_texture_t16(tex, src, color_base, width, height,
                                 stride, rofs, gofs, bofs, transparent);
      case 3:
        return cache_texture_16(tex, src, width, height, stride,
                                rofs, gofs, bofs, transparent);
      case 4:
        return cache_texture_32(tex, src, width, height, stride,
                                rofs, gofs, bofs, transparent);
      default:
        DMSG("Invalid tile pixel format %d", pixfmt);
        return 0;
    }
}

/*************************************************************************/
/*************************************************************************/

/**
 * cache_texture_t4:  Cache a 4-bit indexed texture.
 *
 * [Parameters]
 *               tex: TexInfo structure for texture
 *               src: Pointer to VDP1/VDP2 texture data
 *     width, height: Size of texture (in pixels)
 *            stride: Line size of source data (in pixels)
 * [Return value]
 *     Nonzero on success, zero on error
 */
static inline int cache_texture_t4(
    TexInfo *tex, const uint8_t *src,
    unsigned int width, unsigned int height, unsigned int stride)
{
    /* If the texture width or height aren't powers of 2, we add an extra
     * 1-pixel border on the right and bottom edges to avoid graphics
     * glitches resulting from the GE trying to read one pixel beyond the
     * edge of the texture data. */
    const int outwidth  = width  + (width  == tex->width  ? 0 : 1);
    const int outheight = height + (height == tex->height ? 0 : 1);

    tex->pixfmt = GU_PSM_T4;
    tex->stride = (outwidth+31) & -32;
    tex->vram_address = alloc_vram((tex->stride/2) * outheight);
    if (UNLIKELY(!tex->vram_address)) {
        DMSG("VRAM buffer full, can't cache");
        return 0;
    }

    uint8_t *dest = (uint8_t *)tex->vram_address;
    if (stride != tex->stride) {
        int y;
        for (y = 0; y < height; y++, dest += tex->stride/2 - stride/2) {
            uint8_t *dest_top = dest + stride/2;
            for (; dest < dest_top; src += 4, dest += 4) {
                const uint32_t src_word = *(const uint32_t *)src;
                *(uint32_t *)dest = ((src_word & 0x0F0F0F0F) << 4)
                                  | ((src_word >> 4) & 0x0F0F0F0F);
            }
            if (outwidth > width) {
                *dest = dest[-1] >> 4;  // Copy last pixel
            }
        }
    } else {
        uint8_t *dest_top = dest + (height * (tex->stride/2));
        for (; dest < dest_top; src += 4, dest += 4) {
            const uint32_t src_word = *(const uint32_t *)src;
            *(uint32_t *)dest = ((src_word & 0x0F0F0F0F) << 4)
                              | ((src_word >> 4) & 0x0F0F0F0F);
        }
    }
    if (outheight > height) {
        memcpy(dest, dest - tex->stride/2, tex->stride/2);
    }

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * cache_texture_t8:  Cache an 8-bit indexed texture.
 *
 * [Parameters]
 *               tex: TexInfo structure for texture
 *               src: Pointer to VDP1/VDP2 texture data
 *           pixmask: Pixel data mask
 *     width, height: Size of texture (in pixels)
 *            stride: Line size of source data (in pixels)
 * [Return value]
 *     Nonzero on success, zero on error
 */
static inline int cache_texture_t8(
    TexInfo *tex, const uint8_t *src, uint8_t pixmask,
    unsigned int width, unsigned int height, unsigned int stride)
{
    const int outwidth  = width  + (width  == tex->width  ? 0 : 1);
    const int outheight = height + (height == tex->height ? 0 : 1);

    tex->pixfmt = GU_PSM_T8;
    tex->stride = (outwidth+15) & -16;
    tex->vram_address = alloc_vram(tex->stride * outheight);
    if (UNLIKELY(!tex->vram_address)) {
        DMSG("VRAM buffer full, can't cache");
        return 0;
    }

    uint8_t *dest = (uint8_t *)tex->vram_address;
    if (pixmask != 0xFF) {
        const uint32_t pixmask32 = pixmask * 0x01010101;
        if (stride == 8) {  // Optimize this case (dest->stride == 16)
            int y;
            for (y = 0; y < height; y++, src += 8, dest += 16) {
                const uint32_t pix0_3 = *(const uint32_t *)(src+0);
                const uint32_t pix4_7 = *(const uint32_t *)(src+4);
                *(uint32_t *)(dest+0) = pix0_3 & pixmask32;
                *(uint32_t *)(dest+4) = pix4_7 & pixmask32;
            }
        } else {
            int y;
            for (y = 0; y < height; y++, dest += tex->stride - stride) {
                int x;
                for (x = 0; x < stride; x += 8, src += 8, dest += 8) {
                    const uint32_t pix0_3 = *(const uint32_t *)(src+0);
                    const uint32_t pix4_7 = *(const uint32_t *)(src+4);
                    *(uint32_t *)(dest+0) = pix0_3 & pixmask32;
                    *(uint32_t *)(dest+4) = pix4_7 & pixmask32;
                }
            }
        }
    } else if (stride == 8) {
        int y;
        for (y = 0; y < height; y++, src += 8, dest += 16) {
            *(uint32_t *)(dest+0) = *(const uint32_t *)(src+0);
            *(uint32_t *)(dest+4) = *(const uint32_t *)(src+4);
        }
    } else if (stride != tex->stride) {
        int y;
        for (y = 0; y < height; y++, src += stride, dest += tex->stride) {
            memcpy(dest, src, width);
            if (outwidth > width) {
                dest[width] = dest[width-1];
            }
        }
    } else {
        memcpy(dest, src, stride * height);
    }
    if (outheight > height) {
        memcpy(dest, dest - tex->stride, tex->stride);
    }

    return 1;
}

/*-----------------------------------------------------------------------*/

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
 *     Nonzero on success, zero on error
 */
static inline int cache_texture_t16(
    TexInfo *tex, const uint8_t *src, uint32_t color_base,
    unsigned int width, unsigned int height, unsigned int stride,
    int rofs, int gofs, int bofs, int transparent)
{
    const int outwidth  = width  + (width  == tex->width  ? 0 : 1);
    const int outheight = height + (height == tex->height ? 0 : 1);

    tex->pixfmt = GU_PSM_8888;
    tex->stride = (outwidth+3) & -4;
    tex->vram_address = alloc_vram(tex->stride * outheight * 4);
    if (UNLIKELY(!tex->vram_address)) {
        DMSG("VRAM buffer full, can't cache");
        return 0;
    }
    tex->clut_address = NULL;

    uint32_t *dest = (uint32_t *)tex->vram_address;
    const uint32_t *clut_32 = &global_clut_32[color_base];
    if (rofs || gofs || bofs) {
        const uint32_t trans_pixel = 0x00000000
                                   | (rofs>=0 ? rofs<< 0 : 0)
                                   | (gofs>=0 ? gofs<< 8 : 0)
                                   | (bofs>=0 ? bofs<<16 : 0);
        int y;
        for (y = 0; y < height;
             y++, src += (stride - width) * 2, dest += tex->stride - width
        ) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 2, dest++) {
                uint16_t pixel = src[0]<<8 | src[1];
                if (transparent && !pixel) {
                    *dest = trans_pixel;
                } else {
                    *dest = adjust_color_32_32(clut_32[pixel & 0x7FF],
                                               rofs, gofs, bofs);
                }
            }
            if (outwidth > width) {
                *dest = dest[-1];
            }
        }
    } else if (width != stride || width != tex->stride) {
        int y;
        for (y = 0; y < height;
             y++, src += (stride - width) * 2, dest += tex->stride - width
        ) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 4, dest += 2) {
                uint16_t pixel0 = src[0]<<8 | src[1];
                uint16_t pixel1 = src[2]<<8 | src[3];
                dest[0] = (transparent && !pixel0) ? 0 : clut_32[pixel0 & 0x7FF];
                dest[1] = (transparent && !pixel1) ? 0 : clut_32[pixel1 & 0x7FF];
            }
            if (outwidth > width) {
                *dest = dest[-1];
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
    if (outheight > height) {
        memcpy(dest, dest - tex->stride, tex->stride * 4);
    }

    return 1;
}

/*-----------------------------------------------------------------------*/

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
 *     Nonzero on success, zero on error
 */
static inline int cache_texture_16(
    TexInfo *tex, const uint8_t *src,
    unsigned int width, unsigned int height, unsigned int stride,
    int rofs, int gofs, int bofs, int transparent)
{
    const int outwidth  = width  + (width  == tex->width  ? 0 : 1);
    const int outheight = height + (height == tex->height ? 0 : 1);

    tex->pixfmt = GU_PSM_8888;
    tex->stride = (outwidth+3) & -4;
    tex->vram_address = alloc_vram(tex->stride * outheight * 4);
    if (UNLIKELY(!tex->vram_address)) {
        DMSG("VRAM buffer full, can't cache");
        return 0;
    }
    tex->clut_address = NULL;

    uint32_t *dest = (uint32_t *)tex->vram_address;
    if (rofs || gofs || bofs) {
        const uint32_t trans_pixel = 0x00000000
                                   | (rofs>=0 ? rofs<< 0 : 0)
                                   | (gofs>=0 ? gofs<< 8 : 0)
                                   | (bofs>=0 ? bofs<<16 : 0);
        int y;
        for (y = 0; y < height;
             y++, src += (stride - width) * 2, dest += tex->stride - width
        ) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 2, dest++) {
                uint16_t pixel = src[0]<<8 | src[1];
                *dest = (transparent && !pixel) ? trans_pixel :
                    adjust_color_16_32(pixel, rofs, gofs, bofs);
            }
            if (outwidth > width) {
                *dest = dest[-1];
            }
        }
    } else if (width != stride || width != tex->stride) {
        int y;
        for (y = 0; y < height;
             y++, src += (stride - width) * 2, dest += tex->stride - width
        ) {
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
            if (outwidth > width) {
                *dest = dest[-1];
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
    if (outheight > height) {
        memcpy(dest, dest - tex->stride, tex->stride * 4);
    }

    return 1;
}

/*-----------------------------------------------------------------------*/

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
 *     Nonzero on success, zero on error
 */
static inline int cache_texture_32(
    TexInfo *tex, const uint8_t *src,
    unsigned int width, unsigned int height, unsigned int stride,
    int rofs, int gofs, int bofs, int transparent)
{
    const int outwidth  = width  + (width  == tex->width  ? 0 : 1);
    const int outheight = height + (height == tex->height ? 0 : 1);

    tex->pixfmt = GU_PSM_8888;
    tex->stride = (outwidth+3) & -4;
    tex->vram_address = alloc_vram(tex->stride * outheight * 4);
    if (UNLIKELY(!tex->vram_address)) {
        DMSG("VRAM buffer full, can't cache");
        return 0;
    }
    tex->clut_address = NULL;

    uint32_t *dest = (uint32_t *)tex->vram_address;
    if (rofs || gofs || bofs) {
        const uint32_t trans_pixel = 0x00000000
                                   | (rofs>=0 ? rofs<< 0 : 0)
                                   | (gofs>=0 ? gofs<< 8 : 0)
                                   | (bofs>=0 ? bofs<<16 : 0);
        int y;
        for (y = 0; y < height;
             y++, src += (stride - width) * 4, dest += tex->stride - width
        ) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 4, dest++) {
                int8_t a = src[0]; //Signed so we can check the high bit easily
                uint8_t r = src[3], g = src[2], b = src[1];
                *dest = (transparent && a >= 0) ? trans_pixel :
                    0xFF000000 | bound(r+rofs,0,255) <<  0
                               | bound(g+gofs,0,255) <<  8
                               | bound(b+bofs,0,255) << 16;
            }
            if (outwidth > width) {
                *dest = dest[-1];
            }
        }
    } else if (width != stride) {
        int y;
        for (y = 0; y < height;
             y++, src += (stride - width) * 4, dest += tex->stride - width
        ) {
            uint32_t *dest_top = dest + width;
            for (; dest < dest_top; src += 4, dest++) {
                int8_t a = src[0];
                uint8_t r = src[3], g = src[2], b = src[1];
                *dest = (transparent && a >= 0) ? 0 :
                    0xFF000000 | r<<0 | g<<8 | b<<16;
            }
            if (outwidth > width) {
                *dest = dest[-1];
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
    if (outheight > height) {
        memcpy(dest, dest - tex->stride, tex->stride * 4);
    }

    return 1;
}

/*************************************************************************/
/*************************************************************************/

/**
 * gen_clut:  Generate a color lookup table for a 4-bit or 8-bit indexed
 * texture.
 *
 * [Parameters]
 *                 size: Number of color entries to generate
 *           color_base: Base color index
 *            color_ofs: Color index offset ORed together with pixel
 *     rofs, gofs, bofs: Color offset values for texture
 *          transparent: Nonzero if pixel value 0 should be transparent
 * [Return value]
 *     Nonzero on success, zero on error
 */
static inline int gen_clut(
    TexInfo *tex, unsigned int size, uint32_t color_base, uint8_t color_ofs,
    int rofs, int gofs, int bofs, int transparent)
{
    tex->clut_size = size;

    if (!transparent && !color_ofs && !rofs && !gofs && !bofs) {
        /* There are no changes to apply to the palette, so just use the
         * global CLUT directly. */
        tex->clut_address = &global_clut_32[color_base + color_ofs];
        return 1;
    }

    const uint32_t color_set = (rofs & 0x1FF) <<  0
                             | (gofs & 0x1FF) <<  9
                             | (bofs & 0x1FF) << 18
                             | (transparent ? 1<<27 : 0);
    const unsigned int cache_slot = color_base >> 4;
    unsigned int cache_entry;
    for (cache_entry = 0; cache_entry < CLUT_ENTRIES; cache_entry++) {
        if (clut_cache[cache_slot][cache_entry].size == 0) {
            /* This entry is empty, so we'll generate a palette and store
             * it here. */
            clut_cache[cache_slot][cache_entry].size = size;
            clut_cache[cache_slot][cache_entry].color_ofs = color_ofs;
            clut_cache[cache_slot][cache_entry].color_set = color_set;
            break;
        } else if (clut_cache[cache_slot][cache_entry].size == size
                && clut_cache[cache_slot][cache_entry].color_ofs == color_ofs
                && clut_cache[cache_slot][cache_entry].color_set == color_set){
            /* Found a match, so return it. */
            tex->clut_address =
                clut_cache[cache_slot][cache_entry].clut_address;
            return 1;
        }
    }
    if (UNLIKELY(cache_entry >= CLUT_ENTRIES)) {
        DMSG("Warning: no free entries for CLUT cache slot 0x%02X", cache_slot);
    }

    tex->clut_address = alloc_vram(size*4);
    if (UNLIKELY(!tex->clut_address)) {
        DMSG("VRAM buffer full, can't cache CLUT");
        return 0;
    }
    if (cache_entry < CLUT_ENTRIES) {
        clut_cache[cache_slot][cache_entry].clut_address = tex->clut_address;
    }

    uint32_t *dest = (uint32_t *)tex->clut_address;
    int i;
    if (transparent) {
        /* Apply the color offset values even though it's transparent;
         * this prevents dark rims around interpolated textures when
         * positive color offsets are applied. */
        *dest++ = 0x00000000
                | (rofs>=0 ? rofs<< 0 : 0)
                | (gofs>=0 ? gofs<< 8 : 0)
                | (bofs>=0 ? bofs<<16 : 0);
        i = 1;
    } else {
        i = 0;
    }
    const uint32_t *clut_32 = &global_clut_32[color_base];
    if (rofs || gofs || bofs) {
        for (; i < size; i++, dest++) {
            uint32_t color = clut_32[i | color_ofs];
            *dest = adjust_color_32_32(color, rofs, gofs, bofs);
        }
    } else {
        for (; i < size; i++, dest++) {
            *dest = clut_32[i | color_ofs];
        }
    }

    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * gen_clut_t4ind:  Generate a color lookup table for a 4-bit indirect
 * indexed texture.
 *
 * [Parameters]
 *            color_lut: Pointer to VDP1 color lookup table
 *     rofs, gofs, bofs: Color offset values for texture
 *          transparent: Nonzero if pixel value 0 should be transparent
 * [Return value]
 *     Nonzero on success, zero on error
 */
static inline int gen_clut_t4ind(
    TexInfo *tex, const uint8_t *color_lut,
    int rofs, int gofs, int bofs, int transparent)
{
    tex->clut_size = 16;

    tex->clut_address = alloc_vram(16*4);
    if (UNLIKELY(!tex->clut_address)) {
        DMSG("VRAM buffer full, can't cache CLUT");
        return 0;
    }

    uint32_t *dest = (uint32_t *)tex->clut_address;
    int i;
    if (transparent) {
        *dest++ = 0x00000000
                | (rofs>=0 ? rofs<< 0 : 0)
                | (gofs>=0 ? gofs<< 8 : 0)
                | (bofs>=0 ? bofs<<16 : 0);
        i = 1;
    } else {
        i = 0;
    }
    const uint32_t *clut_32 = global_clut_32;
    if (rofs || gofs || bofs) {
        for (; i < 16; i++, dest++) {
            const uint16_t color16 =
                (uint16_t)color_lut[i*2]<<8 | color_lut[i*2+1];
            if (color16 & 0x8000) {
                *dest = adjust_color_16_32(color16, rofs, gofs, bofs);
            } else {
                *dest = adjust_color_32_32(clut_32[color16], rofs, gofs, bofs);
            }
        }
    } else {
        for (; i < 16; i++, dest++) {
            const uint16_t color16 =
                (uint16_t)color_lut[i*2]<<8 | color_lut[i*2+1];
            if (color16 & 0x8000) {
                *dest = 0xFF000000 | (color16 & 0x7C00) << 9
                                   | (color16 & 0x03E0) << 6
                                   | (color16 & 0x001F) << 3;
            } else {
                *dest = clut_32[color16];
            }
        }
    }

    return 1;
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
