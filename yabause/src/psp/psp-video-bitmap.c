/*  src/psp/psp-video-bitmap.c: Bitmapped background graphics handling for
                                PSP video module
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

#include "common.h"

#include "../vidshared.h"

#include "gu.h"
#include "psp-video.h"
#include "psp-video-internal.h"
#include "texcache.h"

/*************************************************************************/
/************************** Interface functions **************************/
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
void vdp2_draw_bitmap(vdp2draw_struct *info, const clipping_struct *clip)
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
void vdp2_draw_bitmap_t8(vdp2draw_struct *info, const clipping_struct *clip)
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
