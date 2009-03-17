/*  src/psp/display.c: PSP display management functions
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

#include "common.h"
#include "display.h"
#include "sys.h"

/*************************************************************************/

/* Effective display size */
static unsigned int display_width, display_height;

/* Display mode (pixel format) and bits per pixel */
static uint8_t display_mode;
static uint8_t display_bpp;

/* Currently displayed (front) buffer index */
static uint8_t displayed_surface;

/* Work (back) buffer index */
static uint8_t work_surface;

/* Pointers into VRAM */
static void *surfaces[2];       // Display buffers
static uint16_t *depth_buffer;  // 3D depth buffer
static uint8_t *vram_spare_ptr; // Spare VRAM (above depth buffer)
static uint8_t *vram_top;       // Top of VRAM

/* Display list */
/* Size note:  A single VDP2 background layer, if at resolution 704x512,
 * can take up to 500k of display list memory to render. */
static uint8_t __attribute__((aligned(64))) display_list[3*1024*1024];

/* Semaphore flag to indicate whether there is a buffer swap pending; while
 * true, the main thread must not access any other variables */
static int swap_pending;

/*************************************************************************/

/* Local routine declarations */

__attribute__((noreturn))
    static int buffer_swap_thread(SceSize args, void *argp);

/*************************************************************************/
/*************************************************************************/

/**
 * display_init:  Initialize the PSP display.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Nonzero on success, zero on error
 */
int display_init(void)
{
    /* Have we already initialized? */
    static int initted = 0;
    if (initted) {
        return 1;
    }

    /* Clear out VRAM */
    memset(sceGeEdramGetAddr(), 0, sceGeEdramGetSize());
    sceKernelDcacheWritebackAll();

    /* Set display mode */
    int32_t res = sceDisplaySetMode(0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (res < 0) {
        DMSG("sceDisplaySetMode() failed: %s", psp_strerror(res));
        return 0;
    }
    display_width = DISPLAY_WIDTH;
    display_height = DISPLAY_HEIGHT;
    display_mode = PSP_DISPLAY_PIXEL_FORMAT_8888;
    display_bpp = 32;

    /* Initialize VRAM pointers */
    uint8_t *vram_addr = sceGeEdramGetAddr();
    uint32_t vram_size = sceGeEdramGetSize();
    const uint32_t frame_size =
        DISPLAY_STRIDE * DISPLAY_HEIGHT * (display_bpp/8);
    int i;
    for (i = 0; i < lenof(surfaces); i++) {
        surfaces[i] = vram_addr + i*frame_size;
    }
    depth_buffer = (uint16_t *)(vram_addr + lenof(surfaces)*frame_size);
    vram_spare_ptr = (uint8_t *)(depth_buffer + DISPLAY_STRIDE*DISPLAY_HEIGHT);
    vram_top = vram_addr + vram_size;
    displayed_surface = 0;
    work_surface = 1;
    swap_pending = 0;

    /* Set the currently-displayed buffer */
    sceDisplaySetFrameBuf(surfaces[displayed_surface], DISPLAY_STRIDE,
                          display_mode, PSP_DISPLAY_SETBUF_IMMEDIATE);

    /* Set up the GU library */
    sceGuInit();
    sceGuStart(GU_DIRECT, display_list);
    sceGuDispBuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT,
                    surfaces[displayed_surface], DISPLAY_STRIDE);
    sceGuFinish();
    sceGuSync(0, 0);

    /* Success */
    initted = 1;
    return 1;
}

/*-----------------------------------------------------------------------*/

/**
 * display_set_size:  Set the effective display size.  The display will
 * be centered on the PSP's screen.
 *
 * This routine should not be called while drawing a frame (i.e. between
 * display_begin_frame() and display_end_frame()).
 *
 * [Parameters]
 *      width: Effective display width (pixels)
 *     height: Effective display height (pixels)
 * [Return value]
 *     None
 */
void display_set_size(int width, int height)
{
    display_width = width;
    display_height = height;
}

/*-----------------------------------------------------------------------*/

/**
 * display_disp_buffer:  Return a pointer to the current displayed (front)
 * buffer.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Pointer to the current work buffer
 */
uint32_t *display_disp_buffer(void)
{
    return (uint32_t *)surfaces[displayed_surface]
           + ((DISPLAY_HEIGHT - display_height) / 2) * DISPLAY_STRIDE
           + ((DISPLAY_WIDTH  - display_width ) / 2);
}

/*-----------------------------------------------------------------------*/

/**
 * display_work_buffer:  Return a pointer to the current work (back)
 * buffer.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Pointer to the current work buffer
 */
uint32_t *display_work_buffer(void)
{
    return (uint32_t *)surfaces[work_surface]
           + ((DISPLAY_HEIGHT - display_height) / 2) * DISPLAY_STRIDE
           + ((DISPLAY_WIDTH  - display_width ) / 2);
}

/*-----------------------------------------------------------------------*/

/**
 * display_depth_buffer:  Return a pointer to the 3D depth buffer.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Pointer to the 3D depth buffer
 */
uint16_t *display_depth_buffer(void)
{
    return depth_buffer;
}

/*-----------------------------------------------------------------------*/

/**
 * display_spare_vram:  Return a pointer to the VRAM spare area.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Pointer to the VRAM spare area
 */
void *display_spare_vram(void)
{
    return vram_spare_ptr;
}

/*-----------------------------------------------------------------------*/

/**
 * display_spare_vram_size:  Return the size of the VRAM spare area.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     Size of the VRAM spare area, in bytes
 */
uint32_t display_spare_vram_size(void)
{
    return vram_top - vram_spare_ptr;
}

/*************************************************************************/

/**
 * display_begin_frame:  Begin processing for a frame.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void display_begin_frame(void)
{
    sceKernelDelayThread(0);  // Seems to be needed for the buffer swap to work
    while (swap_pending) {
        sceKernelDelayThread(100);  // 0.1ms
    }

    sceGuStart(GU_DIRECT, display_list);

    /* Register the various display pointers */
    sceGuDrawBuffer(GU_PSM_8888, display_work_buffer(), DISPLAY_STRIDE);
    sceGuDepthBuffer(depth_buffer, DISPLAY_STRIDE);

    /* Clear the work surface and depth buffer */
    sceGuDisable(GU_SCISSOR_TEST);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    /* Set up drawing area parameters */
    sceGuViewport(2048, 2048, display_width, display_height);
    sceGuOffset(2048 - display_height/2, 2048 - display_height/2);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, display_width, display_height);
    sceGuEnable(GU_SCISSOR_TEST);

}

/*-----------------------------------------------------------------------*/

/**
 * display_end_frame:  End processing for the current frame, then swap the
 * displayed buffer and work buffer.  The current frame will not actually
 * be shown until the next vertical blank.
 *
 * [Parameters]
 *     None
 * [Return value]
 *     None
 */
void display_end_frame(void)
{
    sceGuFinish();
    swap_pending = 1;
    /* Give the new thread a slightly higher priority than us, or else it
     * won't actually get a chance to run */
    if (sys_start_thread("YabauseBufferSwapThread", buffer_swap_thread,
                         sceKernelGetThreadCurrentPriority() - 1,
                         0x1000, 0, NULL) < 0) {
        DMSG("Failed to start buffer swap thread");
        swap_pending = 0;
        /* Do it ourselves */
        sceGuSync(0, 0);
        sceDisplaySetFrameBuf(surfaces[work_surface], DISPLAY_STRIDE,
                              display_mode, PSP_DISPLAY_SETBUF_NEXTFRAME);
        displayed_surface = work_surface;
        work_surface = (work_surface + 1) % lenof(surfaces);
        sceDisplayWaitVblankStart();
    }
}

/*************************************************************************/

/**
 * display_blit:  Draw an image to the display.  The image must be in
 * native 32bpp format.
 *
 * [Parameters]
 *           src: Source image data pointer
 *     src_width: Width of the source image, in pixels
 *    src_height: Height of the source image, in pixels
 *    src_stride: Line length of source image, in pixels
 *        dest_x: X coordinate at which to display image
 *        dest_y: Y coordinate at which to display image
 * [Return value]
 *     None
 */
void display_blit(const void *src, int src_width, int src_height,
                  int src_stride, int dest_x, int dest_y)
{
#if 1  // Method 1: DMA the data into VRAM (fastest)

    /* Pointer must be 64-byte aligned, so if it's not, adjust the pointer
     * and add an offset to the X coordinate */
    const unsigned int xofs = ((uintptr_t)src & 0x3F) / 4;
    const uint32_t * const src_aligned = (const uint32_t *)src - xofs;

    sceGuCopyImage(GU_PSM_8888, xofs, 0, src_width, src_height, src_stride,
                   src_aligned, dest_x, dest_y, DISPLAY_STRIDE,
                   display_work_buffer());

#elif 1  // Method 2: Draw as a texture (slower, but more general)

    /* Pointer must be 64-byte aligned, so if it's not, adjust the pointer
     * and add an offset to the X coordinate */
    const unsigned int xofs = ((uintptr_t)src & 0x3F) / 4;
    const uint32_t * const src_aligned = (const uint32_t *)src - xofs;

    /* Set up the vertex array (it's faster to draw multiple vertical
     * strips than try to blit the whole thing at once) */
    const int nstrips = (src_width + 15) / 16;
    struct {uint16_t u, v, x, y, z;} *vertices;
    vertices = sceGuGetMemory(sizeof(*vertices) * (2*nstrips));
    if (!vertices) {
        DMSG("Failed to get vertex memory (%d bytes)",
             sizeof(*vertices) * nstrips);
        return;
    }
    int i, x;
    for (i = 0, x = 0; x < src_width; i += 2, x += 16) {
        int thiswidth = 16;
        if (x+16 > src_width) {
            thiswidth = src_width - x;
        }
        vertices[i+0].u = xofs + x;
        vertices[i+0].v = 0;
        vertices[i+0].x = dest_x + x;
        vertices[i+0].y = dest_y;
        vertices[i+0].z = 0;
        vertices[i+1].u = xofs + x + thiswidth;
        vertices[i+1].v = src_height;
        vertices[i+1].x = dest_x + x + thiswidth;
        vertices[i+1].y = dest_y + src_height;
        vertices[i+1].z = 0;
    }

    /* Draw the array */
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
    /* Always use a 512x512 texture size (the GE can only handle sizes that
     * are powers of 2, and the size itself doesn't seem to affect speed) */
    sceGuTexImage(0, 512, 512, src_stride, src_aligned);
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuDrawArray(GU_SPRITES,
                   GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                   2*nstrips, NULL, vertices);
    sceGuDisable(GU_TEXTURE_2D);

#else  // Method 3: Copy using the CPU (slowest)

    const uint32_t *src32 = (const uint32_t *)src;
    uint32_t *dest32 = display_work_buffer() + dest_y*DISPLAY_STRIDE + dest_x;
    int y;
    for (y = 0; y < src_height;
         y++, src32 += src_stride, dest32 += DISPLAY_STRIDE
    ) {
        memcpy(dest32, src32, src_width*4);
    }
    sceKernelDcacheWritebackAll();

#endif
}

/*-----------------------------------------------------------------------*/

/**
 * display_blit_scaled:  Scale and draw an image to the display.  The image
 * must be in native 32bpp format.
 *
 * [Parameters]
 *            src: Source image data pointer
 *      src_width: Width of the source image, in pixels
 *     src_height: Height of the source image, in pixels
 *     src_stride: Line length of source image, in pixels
 *         dest_x: X coordinate at which to display image
 *         dest_y: Y coordinate at which to display image
 *     dest_width: Width of the displayed image, in pixels
 *    dest_height: Height of the displayed image, in pixels
 * [Return value]
 *     None
 */
void display_blit_scaled(const void *src, int src_width, int src_height,
                         int src_stride, int dest_x, int dest_y,
                         int dest_width, int dest_height)
{
    /* Pointer must be 64-byte aligned, so if it's not, adjust the pointer
     * and add an offset to the X coordinate */
    const unsigned int xofs = ((uintptr_t)src & 0x3F) / 4;
    const uint32_t * const src_aligned = (const uint32_t *)src - xofs;

    /* Set up the vertex array (4 vertices for a 2-triangle strip) */
    struct {uint16_t u, v, x, y, z;} *vertices;
    vertices = sceGuGetMemory(sizeof(*vertices) * 4);
    if (!vertices) {
        DMSG("Failed to get vertex memory (%d bytes)", sizeof(*vertices) * 4);
        return;
    }
    vertices[0].u = xofs;
    vertices[0].v = 0;
    vertices[0].x = dest_x;
    vertices[0].y = dest_y;
    vertices[0].z = 0;
    vertices[1].u = xofs;
    vertices[1].v = src_height;
    vertices[1].x = dest_x;
    vertices[1].y = dest_y + dest_height;
    vertices[1].z = 0;
    vertices[2].u = xofs + src_width;
    vertices[2].v = 0;
    vertices[2].x = dest_x + dest_width;
    vertices[2].y = dest_y;
    vertices[2].z = 0;
    vertices[3].u = xofs + src_width;
    vertices[3].v = src_height;
    vertices[3].x = dest_x + dest_width;
    vertices[3].y = dest_y + dest_height;
    vertices[3].z = 0;

    /* Draw the array */
    sceGuEnable(GU_TEXTURE_2D);
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuTexWrap(GU_CLAMP, GU_CLAMP);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
    sceGuTexImage(0, 512, 512, src_stride, src_aligned);
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuDrawArray(GU_TRIANGLE_STRIP,
                   GU_TEXTURE_16BIT | GU_VERTEX_16BIT | GU_TRANSFORM_2D,
                   4, NULL, vertices);
    sceGuDisable(GU_TEXTURE_2D);
}

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * buffer_swap_thread:  Call sceGuSync(), swap the display and work
 * surfaces, wait for the following vertical blank, and clear the
 * "swap_pending" variable to false.  Designed to run as a background
 * thread while the main emulation proceeds.
 *
 * [Parameters]
 *     args, argp: Thread argument size and pointer (unused)
 * [Return value]
 *     Does not return
 */
static int buffer_swap_thread(SceSize args, void *argp)
{
    sceGuSync(0, 0);
    sceDisplaySetFrameBuf(surfaces[work_surface], DISPLAY_STRIDE,
                          display_mode, PSP_DISPLAY_SETBUF_NEXTFRAME);
    displayed_surface = work_surface;
    work_surface = (work_surface + 1) % lenof(surfaces);
    sceDisplayWaitVblankStart();
    swap_pending = 0;
    sceKernelExitDeleteThread(0);
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
