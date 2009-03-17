/*  src/psp/texcache.h: PSP texture cache management header
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

#ifndef PSP_TEXCACHE_H
#define PSP_TEXCACHE_H

/*************************************************************************/

/**
 * PSP_TEXCACHE_SIZE:  Number of entries in the texture cache.  Should be a
 * prime number.
 */
#ifndef PSP_TEXCACHE_SIZE
# define PSP_TEXCACHE_SIZE 1009
#endif

/**
 * PSP_CLUTCACHE_SIZE:  Number of entries in the color lookup table cache.
 * Should be a prime number.
 */
#ifndef PSP_CLUTCACHE_SIZE
# define PSP_CLUTCACHE_SIZE 1009
#endif

/*************************************************************************/

/**
 * texcache_reset:  Reset the texture cache.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
extern void texcache_reset(void);

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
extern uint32_t texcache_cache_sprite(uint16_t CMDSRCA, uint16_t CMDPMOD,
                                      uint16_t CMDCOLR, int width, int height,
                                      int rofs, int gofs, int bofs);

/**
 * texcache_load_sprite:  Load the sprite texture indicated by the given
 * texture key.
 *
 * [Parameters]
 *     key: Texture key returned by texcache_cache_sprite
 * [Return value]
 *     None
 */
extern void texcache_load_sprite(uint32_t key);

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
extern void texcache_load_tile(uint32_t address, int pixfmt, int transparent,
                               uint16_t color_base, uint16_t color_ofs,
                               int rofs, int gofs, int bofs);

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
extern void texcache_load_bitmap(uint32_t address, int width, int height,
                                 int stride, int pixfmt, int transparent,
                                 uint16_t color_base, uint16_t color_ofs,
                                 int rofs, int gofs, int bofs);

/*************************************************************************/

#endif  // PSP_TEXCACHE_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
