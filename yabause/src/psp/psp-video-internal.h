/*  src/psp/psp-video-internal.h: Internal header for PSP video module
    Copyright 2009-2010 Andrew Church

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

#ifndef PSP_VIDEO_INTERNAL_H
#define PSP_VIDEO_INTERNAL_H

/*************************************************************************/
/******************* Map drawing routine declarations ********************/
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
extern void vdp2_draw_bitmap(vdp2draw_struct *info,
                             const clipping_struct *clip);

/**
 * vdp2_draw_bitmap_t8:  Draw an 8-bit indexed graphics layer bitmap.
 *
 * [Parameters]
 *     info: Graphics layer data
 *     clip: Clipping window data
 * [Return value]
 *     None
 */
extern void vdp2_draw_bitmap_t8(vdp2draw_struct *info,
                                const clipping_struct *clip);

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
extern void vdp2_draw_map_rotated(vdp2draw_struct *info,
                                  const clipping_struct *clip);

/*----------------------------------*/

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
extern void vdp2_draw_map_8x8(vdp2draw_struct *info,
                              const clipping_struct *clip);

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
extern void vdp2_draw_map_8x8_t8(vdp2draw_struct *info,
                                 const clipping_struct *clip);

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
extern void vdp2_draw_map_16x16(vdp2draw_struct *info,
                                const clipping_struct *clip);

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
extern void vdp2_draw_map_16x16_t8(vdp2draw_struct *info,
                                   const clipping_struct *clip);

/*************************************************************************/
/**************** Other utility routines and declarations ****************/
/*************************************************************************/

/* Displayed width and height */
extern unsigned int disp_width, disp_height;

/* Scale (right-shift) applied to X and Y coordinates */
extern unsigned int disp_xscale, disp_yscale;

/*************************************************************************/

/**
 * vdp2_calc_pattern_address, vdp2_calc_pattern_address_8x8,
 * vdp2_calc_pattern_address_16x16:  Calculate the address and associated
 * data of the next pattern.  The _8x8 and _16x16 versions are for use when
 * the size of the pattern (8x8 or 16x16 pixels) is known at calling time.
 *
 * [Parameters]
 *     info: Graphics layer data
 * [Return value]
 *     None
 */
static inline void vdp2_calc_pattern_address(vdp2draw_struct *info)
{
    if (info->patterndatasize == 1) {
        const uint16_t data = T1ReadWord(Vdp2Ram, info->addr);
        /* We don't support per-pixel priority, so don't waste time
         * parsing this bit */
        //info->specialfunction = (info->supplementdata >> 9) & 1;
        if (info->colornumber == 0) {  // 4bpp
            info->paladdr = ((data >> 12) & 0xF)
                          | ((info->supplementdata >> 1) & 0x70);
        } else {  // >=8bpp
            info->paladdr = (data >> 8) & 0x70;
        }
        if (!info->auxmode) {
            info->flipfunction = (data >> 10) & 0x3;
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
        const uint32_t data = T1ReadLong(Vdp2Ram, info->addr);
        info->charaddr = data & 0x7FFF;
        info->flipfunction = data >> 30;
        info->paladdr = (data >> 16) & 0x007F;
        /* Ignored, as above */
        //info->specialfunction = (data1 >> 29) & 1;
    }

    /* We don't support 1MB of VDP2 RAM, so always mask off the high bit */
    //if (!(Vdp2Regs->VRSIZE & 0x8000)) {
        info->charaddr &= 0x3FFF;
    //}

    info->charaddr <<= 5;
}

/*----------------------------------*/

static inline void vdp2_calc_pattern_address_8x8(vdp2draw_struct *info)
{
    if (info->patterndatasize == 1) {
        const uint16_t data = T1ReadWord(Vdp2Ram, info->addr);
        //info->specialfunction = (info->supplementdata >> 9) & 1;
        if (info->colornumber == 0) {  // 4bpp
            info->paladdr = ((data >> 12) & 0xF)
                          | ((info->supplementdata >> 1) & 0x70);
        } else {  // >=8bpp
            info->paladdr = (data >> 8) & 0x70;
        }
        if (!info->auxmode) {
            info->flipfunction = (data >> 10) & 0x3;
            info->charaddr = (info->supplementdata & 0xF) << 10
                           | (data & 0x3FF);
        } else {
            info->flipfunction = 0;
            info->charaddr = (info->supplementdata & 0xC) << 10
                           | (data & 0xFFF);
        }
    } else {  // patterndatasize == 2
        const uint32_t data = T1ReadLong(Vdp2Ram, info->addr);
        info->charaddr = data & 0x3FFF;
        info->flipfunction = data >> 30;
        info->paladdr = (data >> 16) & 0x007F;
        /* Ignored, as above */
        //info->specialfunction = (data1 >> 29) & 1;
    }

    info->charaddr <<= 5;
}

/*----------------------------------*/

static inline void vdp2_calc_pattern_address_16x16(vdp2draw_struct *info)
{
    if (info->patterndatasize == 1) {
        const uint16_t data = T1ReadWord(Vdp2Ram, info->addr);
        //info->specialfunction = (info->supplementdata >> 9) & 1;
        if (info->colornumber == 0) {  // 4bpp
            info->paladdr = ((data >> 12) & 0xF)
                          | ((info->supplementdata >> 1) & 0x70);
        } else {  // >=8bpp
            info->paladdr = (data >> 8) & 0x70;
        }
        if (!info->auxmode) {
            info->flipfunction = (data >> 10) & 0x3;
            info->charaddr = (info->supplementdata & 0xF) << 10
                           | (data & 0x3FF) << 2
                           | (info->supplementdata & 0x3);
        } else {
            info->flipfunction = 0;
            info->charaddr = (info->supplementdata & 0xC) << 10
                           | (data & 0xFFF) << 2
                           | (info->supplementdata & 0x3);
        }
    } else {  // patterndatasize == 2
        const uint32_t data = T1ReadLong(Vdp2Ram, info->addr);
        info->charaddr = data & 0x3FFF;
        info->flipfunction = data >> 30;
        info->paladdr = (data >> 16) & 0x007F;
        /* Ignored, as above */
        //info->specialfunction = (data1 >> 29) & 1;
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
 *     Allocated color table pointer
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
/*************************************************************************/

#endif  // PSP_VIDEO_INTERNAL_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
