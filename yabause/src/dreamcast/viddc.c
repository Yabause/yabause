/*  Copyright 2003-2006 Guillaume Duhamel
    Copyright 2004-2006 Lawrence Sebald
    Copyright 2004-2006 Theo Berkau

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "viddc.h"
#include "debug.h"
#include "vdp2.h"

#include <dc/video.h>
#include <dc/pvr.h>
#include <time.h>
#include <assert.h>

#include <stdio.h>

//static inline uint16 SAT2YAB1(uint16 tmp)	{
//	asm("swap.b		%0, %0" : "=r"(tmp) : "0"(tmp));
//	return tmp;
//}

#define SAT2YAB1(temp)	((temp & 0x8000) | (temp & 0x1F) << 10 | (temp & 0x3E0) | (temp & 0x7C00) >> 10)
#define SAT2YAB32(alpha, temp)	(alpha << 24 | (temp & 0x1F) << 3 | (temp & 0x3E0) << 6 | (temp & 0x7C00) << 9)

/* 24-bit color, unimplemented for now */
#define SAT2YAB2(dot1, dot2)	0

#define COLOR_ADDt(b)		(b > 0xFF ? 0xFF : (b < 0 ? 0 : b))
#define COLOR_ADDb(b1,b2)	COLOR_ADDt((signed) (b1) + (b2))

#define COLOR_ADD(l,r,g,b)	COLOR_ADDb((l & 0xFF), r) | \
                (COLOR_ADDb((l >> 8) & 0xFF, g) << 8) | \
                (COLOR_ADDb((l >> 16) & 0xFF, b) << 16) | \
                (l & 0xFF000000)

static pvr_init_params_t pvr_params =   {
    /* Enable Opaque, Translucent, and Punch-Thru polygons with binsize 16 */
    { PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16, PVR_BINSIZE_0, PVR_BINSIZE_16 },
    /* 512KB Vertex Buffer */
    512 * 1024,
    /* DMA Enabled */
    1,
    /* FSAA Disabled */
    0
};

struct sprite_info  {
    float uf, vf;
    int w, h;
};

static struct sprite_info cur_spr;

/* Polygon Headers */
static pvr_poly_hdr_t op_poly_hdr;
static pvr_poly_hdr_t tr_poly_hdr;
static pvr_poly_ic_hdr_t tr_sprite_hdr;
static pvr_poly_ic_hdr_t pt_sprite_hdr;

/* DMA Vertex Buffers 256KB Each */
static uint8 vbuf_opaque[1024 * 256] __attribute__((aligned(32)));
static uint8 vbuf_translucent[1024 * 256] __attribute__((aligned(32)));
static uint8 vbuf_punchthru[1024 * 256] __attribute__((aligned(32)));

/* Priority levels, sprites drawn last get drawn on top */
static float priority_levels[8];

/* Texture space for VDP1 sprites */
static pvr_ptr_t tex_space;
static uint32 cur_addr;

/* Misc parameters */
static int vdp1cor = 0;
static int vdp1cog = 0;
static int vdp1cob = 0;

static int vdp2disptoggle = 0xFF;
static int nbg0priority = 0;
static int nbg1priority = 0;
static int nbg2priority = 0;
static int nbg3priority = 0;
static int rbg0priority = 0;

static int power_of_two(int num)    {
    int ret = 8;

    while(ret < num)
        ret <<= 1;

    return ret;
}

static u32 Vdp2ColorRamGetColor32(u32 colorindex, int alpha)    {
    switch(Vdp2Internal.ColorMode)  {
        case 0:
        case 1:
        {
            u32 tmp;
            colorindex <<= 1;
            tmp = T2ReadWord(Vdp2ColorRam, colorindex & 0xFFF);
            return SAT2YAB32(alpha, tmp);
        }
        case 2:
        {
            u32 tmp1, tmp2;
            colorindex <<= 2;
            colorindex &= 0xFFF;
            tmp1 = T2ReadWord(Vdp2ColorRam, colorindex);
            tmp2 = T2ReadWord(Vdp2ColorRam, colorindex+2);
            //return SAT2YAB2(alpha, tmp1, tmp2);
            return 0; /* FIXME */
        }
        default:
            break;
    }

    return 0;
}

static uint16 Vdp2ColorRamGetColor(u32 colorindex)   {
    u16 tmp;

    switch(Vdp2Internal.ColorMode)  {
        case 0:
        case 1:
        {
            colorindex <<= 1;
            tmp = T2ReadWord(Vdp2ColorRam, colorindex & 0xFFF);
            return SAT2YAB1(tmp);
        }
        case 2:
        {
            u16 tmp2;
            colorindex <<= 2;
            colorindex &= 0xFFF;
            tmp = T2ReadWord(Vdp2ColorRam, colorindex);
            tmp2 = T2ReadWord(Vdp2ColorRam, colorindex+2);
            //return SAT2YAB2(alpha, tmp1, tmp2);
            return 0; /* FIXME */
        }
        default:
            break;
    }
    
    return 0;
}

static int Vdp1ReadTexture(vdp1cmd_struct *cmd, pvr_poly_ic_hdr_t *hdr) {
    u32 charAddr = (cmd->CMDSRCA * 8) & 0x7FFFF;
    uint16 dot, dot2;
    int queuepos = 0;
    uint32 *store_queue;
    uint32 cur_base;
    u8 SPD = ((cmd->CMDPMOD & 0x40) != 0);

    int wi = power_of_two(cur_spr.w);
    int he = power_of_two(cur_spr.h);

    cur_base = cur_addr;

    VDP1LOG("Making new sprite %08X\n", charAddr);

    /* Set up both Store Queues for transfer to VRAM */
    QACR0 = 0x00000004;
    QACR1 = 0x00000004;

    switch((cmd->CMDPMOD >> 3) & 0x07)  {
        case 1:
        {
            // 4 bpp LUT mode
            u16 temp;
            u32 colorLut = cmd->CMDCOLR * 8;
            u16 i, j;
            
            for(i = 0; i < cur_spr.h; ++i)  {
                store_queue = (uint32 *) (0xE0000000 | 
                                          (cur_addr & 0x03FFFFE0));

                for(j = 0; j < cur_spr.w; j += 2)    {
                    dot = T1ReadByte(Vdp1Ram, charAddr & 0x7FFFF);

                    if (((dot & 0xF) == 0) && !SPD) dot2 = 0;
                    else    {
                        temp = T1ReadWord(Vdp1Ram, ((dot & 0xF) * 2 + 
                                                    colorLut) & 0x7FFFF);

                        if (temp & 0x8000)
                            dot2 = SAT2YAB1(temp);
                        else
                            dot2 = Vdp2ColorRamGetColor(temp);                     
                    }
                    
                    if (((dot >> 4) == 0) && !SPD)  ;
                    else    {
                        temp = T1ReadWord(Vdp1Ram, ((dot >> 4) * 2 + colorLut) &
                                          0x7FFFF);
                        if (temp & 0x8000)
                            dot = SAT2YAB1(temp);
                        else
                            dot = Vdp2ColorRamGetColor(temp);
                    }

                    ++charAddr;

                    store_queue[queuepos++] = dot | (dot2 << 16);

                    if(queuepos == 8)   {
                        asm("pref @%0" : : "r"(store_queue));
                        queuepos = 0;
                        store_queue += 8;
                    }
                }

                if(queuepos)    {
                    asm("pref @%0" : : "r"(store_queue));
                    queuepos = 0;
                }

                cur_addr += wi * 2;
            }
            break;
        }
        case 5:
        {
            // 16 bpp Bank mode
            u16 i, j;

            for(i = 0; i < cur_spr.h; ++i)  {
                store_queue = (uint32 *) (0xE0000000 | 
                                          (cur_addr & 0x03FFFFE0));

                for(j = 0; j < cur_spr.w; j += 2)  {
                    dot = T1ReadWord(Vdp1Ram, charAddr);
                    dot2 = T1ReadWord(Vdp1Ram, charAddr + 2);
                    charAddr = (charAddr + 4) & 0x7FFFF;

                    if((dot == 0) && !SPD)  ;
                    else
                        dot = SAT2YAB1(dot);

                    if((dot2 == 0) && !SPD) ;
                    else
                        dot2 = SAT2YAB1(dot2);

                    store_queue[queuepos++] = dot | (dot2 << 16);

                    if(queuepos == 8)   {
                        asm("pref @%0" : : "r"(store_queue));
                        queuepos = 0;
                        store_queue += 8;
                    }

                }

                if(queuepos)    {
                    asm("pref @%0" : : "r"(store_queue));
                    queuepos = 0;
                }

                cur_addr += wi * 2;
            }
            break;
        }

        default:
            printf("Sprite type not handled yet.... %x\n", (cmd->CMDPMOD >> 3) & 0x07);
            VDP1LOG("Unimplemented sprite color mode: %X\n", (cmd->CMDPMOD >> 3) & 0x7);
            return 0;
    }

    cur_spr.uf = (float) cur_spr.w / wi;
    cur_spr.vf = (float) cur_spr.h / he;

    hdr->mode2 &= (~(PVR_TA_PM2_USIZE_MASK | PVR_TA_PM2_VSIZE_MASK));

    switch (wi) {
        case 8:     break;
        case 16:    hdr->mode2 |= (1 << PVR_TA_PM2_USIZE_SHIFT); break;
        case 32:    hdr->mode2 |= (2 << PVR_TA_PM2_USIZE_SHIFT); break;
        case 64:    hdr->mode2 |= (3 << PVR_TA_PM2_USIZE_SHIFT); break;
        case 128:   hdr->mode2 |= (4 << PVR_TA_PM2_USIZE_SHIFT); break;
        case 256:   hdr->mode2 |= (5 << PVR_TA_PM2_USIZE_SHIFT); break;
        case 512:   hdr->mode2 |= (6 << PVR_TA_PM2_USIZE_SHIFT); break;
        case 1024:  hdr->mode2 |= (7 << PVR_TA_PM2_USIZE_SHIFT); break;
        default:    assert_msg(0, "Invalid texture U size"); break;
    }

    switch (he) {
        case 8:     break;
        case 16:    hdr->mode2 |= (1 << PVR_TA_PM2_VSIZE_SHIFT); break;
        case 32:    hdr->mode2 |= (2 << PVR_TA_PM2_VSIZE_SHIFT); break;
        case 64:    hdr->mode2 |= (3 << PVR_TA_PM2_VSIZE_SHIFT); break;
        case 128:   hdr->mode2 |= (4 << PVR_TA_PM2_VSIZE_SHIFT); break;
        case 256:   hdr->mode2 |= (5 << PVR_TA_PM2_VSIZE_SHIFT); break;
        case 512:   hdr->mode2 |= (6 << PVR_TA_PM2_VSIZE_SHIFT); break;
        case 1024:  hdr->mode2 |= (7 << PVR_TA_PM2_VSIZE_SHIFT); break;
        default:    assert_msg(0, "Invalid texture V size"); break;
    }

    hdr->mode3 = ((cur_base & 0x00FFFFF8) >> 3) | (PVR_TXRFMT_NONTWIDDLED);
    return 1;
}

static u8 Vdp1ReadPriority(vdp1cmd_struct *cmd) {
    u8 SPCLMD = Vdp2Regs->SPCTL;
    u8 sprite_register;
    u8 *sprprilist = (u8 *)&Vdp2Regs->PRISA;

    if ((SPCLMD & 0x20) && (cmd->CMDCOLR & 0x8000)) {
        // RGB data, use register 0
        return Vdp2Regs->PRISA & 0x07;
    }
    else    {
        u8 sprite_type = SPCLMD & 0x0F;
        switch(sprite_type) {
            case 0:
                sprite_register = ((cmd->CMDCOLR & 0x8000) | (~cmd->CMDCOLR & 0x4000)) >> 14;
                return sprprilist[sprite_register ^ 1] & 0x07;
                break;
            case 1:
                sprite_register = ((cmd->CMDCOLR & 0xC000) | (~cmd->CMDCOLR & 0x2000)) >> 13;
                return sprprilist[sprite_register ^ 1] & 0x07;
                break;
            case 3:
                sprite_register = ((cmd->CMDCOLR & 0x4000) | (~cmd->CMDCOLR & 0x2000)) >> 13;
                return sprprilist[sprite_register ^ 1] & 0x07;
                break;
            case 4:
                sprite_register = ((cmd->CMDCOLR & 0x4000) | (~cmd->CMDCOLR & 0x2000)) >> 13;
                return sprprilist[sprite_register ^ 1] & 0x07;
                break;
            case 5:
                sprite_register = ((cmd->CMDCOLR & 0x6000) | (~cmd->CMDCOLR & 0x1000)) >> 12;
                return sprprilist[sprite_register ^ 1] & 0x07;
                break;
            case 6:
                sprite_register = ((cmd->CMDCOLR & 0x6000) | (~cmd->CMDCOLR & 0x1000)) >> 12;
                return sprprilist[sprite_register ^ 1] & 0x07;
                break;
            case 7:
                sprite_register = ((cmd->CMDCOLR & 0x6000) | (~cmd->CMDCOLR & 0x1000)) >> 12;
                return sprprilist[sprite_register ^ 1] & 0x07;
                break;
            default:
                VDP1LOG("sprite type %d not implemented\n", sprite_type);
                return 0x07;
                break;
        }
    }
}

static int VIDDCInit(void)  {
    pvr_poly_cxt_t op_poly_cxt, tr_poly_cxt;
    pvr_sprite_cxt_t pt_sprite_cxt, tr_sprite_cxt;

    vid_set_mode(DM_320x240, PM_RGB565);

    if(pvr_init(&pvr_params))   {
        fprintf(stderr, "VIDDCInit() - error initializing PVR\n");
        return -1;
    }

    pvr_set_vertbuf(PVR_LIST_OP_POLY, vbuf_opaque, 1024 * 256);
    pvr_set_vertbuf(PVR_LIST_TR_POLY, vbuf_translucent, 1024 * 256);
    pvr_set_vertbuf(PVR_LIST_PT_POLY, vbuf_punchthru, 1024 * 256);

    tex_space = pvr_mem_malloc(1024 * 1024 * 2);
    cur_addr = (uint32)tex_space;

    sq_set(tex_space, 0xFF, 1024 * 1024 * 2);

    pvr_poly_cxt_col(&op_poly_cxt, PVR_LIST_OP_POLY);
    pvr_poly_cxt_col(&tr_poly_cxt, PVR_LIST_TR_POLY);

    op_poly_cxt.gen.culling = PVR_CULLING_NONE;
    tr_poly_cxt.gen.culling = PVR_CULLING_NONE;

    pvr_poly_compile(&op_poly_hdr, &op_poly_cxt);
    pvr_poly_compile(&tr_poly_hdr, &tr_poly_cxt);

    pvr_sprite_cxt_txr(&tr_sprite_cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555 |
                       PVR_TXRFMT_NONTWIDDLED, 1024, 1024, tex_space,
                       PVR_FILTER_NONE);
    pvr_sprite_cxt_txr(&pt_sprite_cxt, PVR_LIST_PT_POLY, PVR_TXRFMT_ARGB1555 |
                       PVR_TXRFMT_NONTWIDDLED, 1024, 1024, tex_space,
                       PVR_FILTER_NONE);

    pt_sprite_cxt.gen.culling = PVR_CULLING_NONE;
    tr_sprite_cxt.gen.culling = PVR_CULLING_NONE;

    pvr_sprite_compile(&tr_sprite_hdr, &tr_sprite_cxt);
    pvr_sprite_compile(&pt_sprite_hdr, &pt_sprite_cxt);

    priority_levels[0] = 0.0f;
    priority_levels[1] = 1.0f;
    priority_levels[2] = 2.0f;
    priority_levels[3] = 3.0f;
    priority_levels[4] = 4.0f;
    priority_levels[5] = 5.0f;
    priority_levels[6] = 6.0f;
    priority_levels[7] = 7.0f;

    return 0;
}

static void VIDDCDeInit(void)   {
    pvr_set_vertbuf(PVR_LIST_OP_POLY, NULL, 0);
    pvr_set_vertbuf(PVR_LIST_TR_POLY, NULL, 0);
    pvr_set_vertbuf(PVR_LIST_PT_POLY, NULL, 0);

    pvr_mem_free(tex_space);

    pvr_shutdown();
    vid_set_mode(DM_640x480, PM_RGB565);
}

static void VIDDCResize(unsigned int w, unsigned int h, int unused) {
}

static int VIDDCIsFullscreen(void)  {
    return 1;
}

static int VIDDCVdp1Reset(void) {
    return 0;
}

static void VIDDCVdp1DrawStart(void)    {
    if(Vdp2Regs->CLOFEN & 0x40)    {
        // color offset enable
        if(Vdp2Regs->CLOFSL & 0x40)    {
            // color offset B
            vdp1cor = Vdp2Regs->COBR & 0xFF;
            if(Vdp2Regs->COBR & 0x100)
                vdp1cor |= 0xFFFFFF00;
            
            vdp1cog = Vdp2Regs->COBG & 0xFF;
            if(Vdp2Regs->COBG & 0x100)
                vdp1cog |= 0xFFFFFF00;
            
            vdp1cob = Vdp2Regs->COBB & 0xFF;
            if(Vdp2Regs->COBB & 0x100)
                vdp1cob |= 0xFFFFFF00;
        }
        else    {
            // color offset A
            vdp1cor = Vdp2Regs->COAR & 0xFF;
            if(Vdp2Regs->COAR & 0x100)
                vdp1cor |= 0xFFFFFF00;
            
            vdp1cog = Vdp2Regs->COAG & 0xFF;
            if(Vdp2Regs->COAG & 0x100)
                vdp1cog |= 0xFFFFFF00;
            
            vdp1cob = Vdp2Regs->COAB & 0xFF;
            if(Vdp2Regs->COAB & 0x100)
                vdp1cob |= 0xFFFFFF00;
        }
    }
    else // color offset disable
        vdp1cor = vdp1cog = vdp1cob = 0;

    cur_addr = (uint32) tex_space;
}

static void VIDDCVdp1DrawEnd(void)  {
    priority_levels[0] = 0.0f;
    priority_levels[1] = 1.0f;
    priority_levels[2] = 2.0f;
    priority_levels[3] = 3.0f;
    priority_levels[4] = 4.0f;
    priority_levels[5] = 5.0f;
    priority_levels[6] = 6.0f;
    priority_levels[7] = 7.0f;
}

static void VIDDCVdp1NormalSpriteDraw(void) {
    int x, y, num;
    u8 z;
    vdp1cmd_struct cmd;
    pvr_sprite_txr_t sprite;
    pvr_list_t list;

    Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

    x = Vdp1Regs->localX + cmd.CMDXA;
    y = Vdp1Regs->localY + cmd.CMDYA;
    cur_spr.w = ((cmd.CMDSIZE >> 8) & 0x3F) << 3;
    cur_spr.h = cmd.CMDSIZE & 0xFF;

    if ((cmd.CMDPMOD & 0x07) == 0x03) {
        tr_sprite_hdr.a = 0.5f;
        list = PVR_LIST_TR_POLY;
        num = Vdp1ReadTexture(&cmd, &tr_sprite_hdr);

        if(num == 0)
            return;
        else
            pvr_list_prim(PVR_LIST_TR_POLY, &tr_sprite_hdr,
                          sizeof(pvr_poly_ic_hdr_t));
    }
    else    {
        pt_sprite_hdr.a = 1.0f;
        num = Vdp1ReadTexture(&cmd, &pt_sprite_hdr);
        list = PVR_LIST_PT_POLY;
        
        if(num == 0)
            return;
        else
            pvr_list_prim(PVR_LIST_PT_POLY, &pt_sprite_hdr,
                          sizeof(pvr_poly_ic_hdr_t));
    }

    z = Vdp1ReadPriority(&cmd);

    sprite.flags = PVR_CMD_VERTEX_EOL;
    sprite.ax = x;
    sprite.ay = y;
    sprite.az = priority_levels[z];

    sprite.bx = x + cur_spr.w;
    sprite.by = y;
    sprite.bz = priority_levels[z];

    sprite.cx = x + cur_spr.w;
    sprite.cy = y + cur_spr.h;
    sprite.cz = priority_levels[z];

    sprite.dx = x;
    sprite.dy = y + cur_spr.h;

    sprite.auv = PVR_PACK_16BIT_UV(((cmd.CMDCTRL & 0x0010) ? cur_spr.uf : 0.0f),
                                  ((cmd.CMDCTRL & 0x0020) ? cur_spr.vf : 0.0f));
    sprite.buv = PVR_PACK_16BIT_UV(((cmd.CMDCTRL & 0x0010) ? 0.0f : cur_spr.uf),
                                  ((cmd.CMDCTRL & 0x0020) ? cur_spr.vf : 0.0f));
    sprite.cuv = PVR_PACK_16BIT_UV(((cmd.CMDCTRL & 0x0010) ? 0.0f : cur_spr.uf),
                                  ((cmd.CMDCTRL & 0x0020) ? 0.0f : cur_spr.vf));
    pvr_list_prim(list, &sprite, sizeof(sprite));

    priority_levels[z] += 0.000001f;
}

static void VIDDCVdp1ScaledSpriteDraw(void) {
    vdp1cmd_struct cmd;
    s16 rw = 0, rh = 0;
    s16 x, y;
    u8 z;
    pvr_sprite_txr_t sprite;
    pvr_list_t list;
    int num;

    Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

    x = cmd.CMDXA + Vdp1Regs->localX;
    y = cmd.CMDYA + Vdp1Regs->localY;
    cur_spr.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
    cur_spr.h = cmd.CMDSIZE & 0xFF;

    if((cmd.CMDPMOD & 0x07) == 0x03)    {
        tr_sprite_hdr.a = 0.5f;
        list = PVR_LIST_TR_POLY;
        num = Vdp1ReadTexture(&cmd, &tr_sprite_hdr);

        if(num == 0)
            return;
        else
            pvr_list_prim(PVR_LIST_TR_POLY, &tr_sprite_hdr,
                          sizeof(pvr_poly_ic_hdr_t));
    }
    else    {
        pt_sprite_hdr.a = 1.0f;
        num = Vdp1ReadTexture(&cmd, &pt_sprite_hdr);
        list = PVR_LIST_PT_POLY;

        if(num == 0)
            return;
        else
            pvr_list_prim(PVR_LIST_PT_POLY, &pt_sprite_hdr,
                          sizeof(pvr_poly_ic_hdr_t));
    }

    // Setup Zoom Point
    switch ((cmd.CMDCTRL & 0xF00) >> 8) {
        case 0x0: // Only two coordinates
            rw = cmd.CMDXC - x + Vdp1Regs->localX + 1;
            rh = cmd.CMDYC - y + Vdp1Regs->localY + 1;
            break;
        case 0x5: // Upper-left
            rw = cmd.CMDXB + 1;
            rh = cmd.CMDYB + 1;
            break;
        case 0x6: // Upper-Center
            rw = cmd.CMDXB;
            rh = cmd.CMDYB;
            x = x - rw / 2;
            ++rw;
            ++rh;
            break;
        case 0x7: // Upper-Right
            rw = cmd.CMDXB;
            rh = cmd.CMDYB;
            x = x - rw;
            ++rw;
            ++rh;
            break;
        case 0x9: // Center-left
            rw = cmd.CMDXB;
            rh = cmd.CMDYB;
            y = y - rh / 2;
            ++rw;
            ++rh;
            break;
        case 0xA: // Center-center
            rw = cmd.CMDXB;
            rh = cmd.CMDYB;
            x = x - rw / 2;
            y = y - rh / 2;
            ++rw;
            ++rh;
            break;
        case 0xB: // Center-right
            rw = cmd.CMDXB;
            rh = cmd.CMDYB;
            x = x - rw;
            y = y - rh / 2;
            ++rw;
            ++rh;
            break;
        case 0xD: // Lower-left
            rw = cmd.CMDXB;
            rh = cmd.CMDYB;
            y = y - rh;
            ++rw;
            ++rh;
            break;
        case 0xE: // Lower-center
            rw = cmd.CMDXB;
            rh = cmd.CMDYB;
            x = x - rw / 2;
            y = y - rh;
            ++rw;
            ++rh;
            break;
        case 0xF: // Lower-right
            rw = cmd.CMDXB;
            rh = cmd.CMDYB;
            x = x - rw;
            y = y - rh;
            ++rw;
            ++rh;
            break;
        default:
            break;
    }

    z = Vdp1ReadPriority(&cmd);

    sprite.flags = PVR_CMD_VERTEX_EOL;
    sprite.ax = x;
    sprite.ay = y;
    sprite.az = priority_levels[z];

    sprite.bx = x + rw;
    sprite.by = y;
    sprite.bz = priority_levels[z];

    sprite.cx = x + rw;
    sprite.cy = y + rh;
    sprite.cz = priority_levels[z];

    sprite.dx = x;
    sprite.dy = y + rh;

    sprite.auv = PVR_PACK_16BIT_UV(((cmd.CMDCTRL & 0x0010) ? cur_spr.uf : 0.0f),
                                  ((cmd.CMDCTRL & 0x0020) ? cur_spr.vf : 0.0f));
    sprite.buv = PVR_PACK_16BIT_UV(((cmd.CMDCTRL & 0x0010) ? 0.0f : cur_spr.uf),
                                  ((cmd.CMDCTRL & 0x0020) ? cur_spr.vf : 0.0f));
    sprite.cuv = PVR_PACK_16BIT_UV(((cmd.CMDCTRL & 0x0010) ? 0.0f : cur_spr.uf),
                                  ((cmd.CMDCTRL & 0x0020) ? 0.0f : cur_spr.vf));
    pvr_list_prim(list, &sprite, sizeof(sprite));

    priority_levels[z] += 0.000001f;
}

static void VIDDCVdp1DistortedSpriteDraw(void)  {
    vdp1cmd_struct cmd;
    u8 z;
    pvr_sprite_txr_t sprite;
    pvr_list_t list;
    int num;

    Vdp1ReadCommand(&cmd, Vdp1Regs->addr);

    cur_spr.w = ((cmd.CMDSIZE >> 8) & 0x3F) * 8;
    cur_spr.h = cmd.CMDSIZE & 0xFF;

    if((cmd.CMDPMOD & 0x7) == 0x3) {
        tr_sprite_hdr.a = 0.5f;
        list = PVR_LIST_TR_POLY;
        num = Vdp1ReadTexture(&cmd, &tr_sprite_hdr);

        if(num == 0)
            return;
        else
            pvr_list_prim(PVR_LIST_TR_POLY, &tr_sprite_hdr,
                          sizeof(pvr_poly_ic_hdr_t));
    }
    else    {
        pt_sprite_hdr.a = 1.0f;
        num = Vdp1ReadTexture(&cmd, &pt_sprite_hdr);
        list = PVR_LIST_PT_POLY;

        if(num == 0)
            return;
        else
            pvr_list_prim(PVR_LIST_PT_POLY, &pt_sprite_hdr,
                          sizeof(pvr_poly_ic_hdr_t));
    }

    z = Vdp1ReadPriority(&cmd);

    sprite.flags = PVR_CMD_VERTEX_EOL;
    sprite.ax = cmd.CMDXA + Vdp1Regs->localX;
    sprite.ay = cmd.CMDYA + Vdp1Regs->localY;
    sprite.az = priority_levels[z];

    sprite.bx = cmd.CMDXB + Vdp1Regs->localX + 1;
    sprite.by = cmd.CMDYB + Vdp1Regs->localY;
    sprite.bz = priority_levels[z];

    sprite.cx = cmd.CMDXC + Vdp1Regs->localX + 1;
    sprite.cy = cmd.CMDYC + Vdp1Regs->localY + 1;
    sprite.cz = priority_levels[z];

    sprite.dx = cmd.CMDXD + Vdp1Regs->localX;
    sprite.dy = cmd.CMDYD + Vdp1Regs->localY + 1;

    sprite.auv = PVR_PACK_16BIT_UV(((cmd.CMDCTRL & 0x0010) ? cur_spr.uf : 0.0f),
                                  ((cmd.CMDCTRL & 0x0020) ? cur_spr.vf : 0.0f));
    sprite.buv = PVR_PACK_16BIT_UV(((cmd.CMDCTRL & 0x0010) ? 0.0f : cur_spr.uf),
                                  ((cmd.CMDCTRL & 0x0020) ? cur_spr.vf : 0.0f));
    sprite.cuv = PVR_PACK_16BIT_UV(((cmd.CMDCTRL & 0x0010) ? 0.0f : cur_spr.uf),
                                  ((cmd.CMDCTRL & 0x0020) ? 0.0f : cur_spr.vf));
    pvr_list_prim(list, &sprite, sizeof(sprite));

    priority_levels[z] += 0.000001f;
}

static void VIDDCVdp1PolygonDraw(void)  {
    s16 X[4];
    s16 Y[4];
    u16 color;
    u16 CMDPMOD;
    u8 alpha, z;
    pvr_vertex_t vert;
    pvr_list_t list;

    X[0] = Vdp1Regs->localX + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C);
    Y[0] = Vdp1Regs->localY + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E);
    X[1] = Vdp1Regs->localX + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x10);
    Y[1] = Vdp1Regs->localY + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x12);
    X[2] = Vdp1Regs->localX + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
    Y[2] = Vdp1Regs->localY + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
    X[3] = Vdp1Regs->localX + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x18);
    Y[3] = Vdp1Regs->localY + T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x1A);

    color = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x06);
    CMDPMOD = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x04);

    /* Don't bother rendering completely transparent polygons */
    if(!(color & 0x8000) && !(CMDPMOD & 0x0040))    {
        return;
    }
    else if(!(color & 0x8000))   {
        /* Hack to make it so that giant polygons don't clobber the rest of
           the output... */
        if(X[0] == 0 && X[3] == 0 && X[1] == 319 && X[2] == 319 && Y[0] == 0 &&
           Y[1] == 0 && Y[2] == 255 && Y[3] == 255) {
            return;
        }
    }

    if ((CMDPMOD & 0x0007) == 0x0003) {
        alpha = 0x80;
        list = PVR_LIST_TR_POLY;
        pvr_list_prim(PVR_LIST_TR_POLY, &tr_poly_hdr, sizeof(pvr_poly_hdr_t));
    }
    else    {
        alpha = 0xFF;
        list = PVR_LIST_OP_POLY;
        pvr_list_prim(PVR_LIST_OP_POLY, &op_poly_hdr, sizeof(pvr_poly_hdr_t));
    }

    if(color & 0x8000)  {
        vert.argb = COLOR_ADD(SAT2YAB32(alpha, color), vdp1cor, vdp1cog,
                              vdp1cob);
    }
    else    {
        vert.argb = COLOR_ADD(Vdp2ColorRamGetColor32(color, alpha), vdp1cor, 
                              vdp1cog, vdp1cob);
    }

    z = Vdp2Regs->PRISA & 0x07;

    vert.flags = PVR_CMD_VERTEX;
    vert.u = 0.0f;
    vert.v = 0.0f;
    vert.oargb = 0;
    vert.z = priority_levels[z];

    vert.x = X[3];
    vert.y = Y[3];
    pvr_list_prim(list, &vert, sizeof(vert));

    vert.x = X[0];
    vert.y = Y[0];
    pvr_list_prim(list, &vert, sizeof(vert));

    vert.x = X[2];
    vert.y = Y[2];
    pvr_list_prim(list, &vert, sizeof(vert));

    vert.flags = PVR_CMD_VERTEX_EOL;
    vert.x = X[1];
    vert.y = Y[1];
    pvr_list_prim(list, &vert, sizeof(vert));

    priority_levels[z] += 0.000001f;
}

static void VIDDCVdp1PolylineDraw(void) {
}

static void VIDDCVdp1LineDraw(void) {
}

static void VIDDCVdp1UserClipping(void) {
    Vdp1Regs->userclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C);
    Vdp1Regs->userclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E);
    Vdp1Regs->userclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
    Vdp1Regs->userclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
}

static void VIDDCVdp1SystemClipping(void)   {
    Vdp1Regs->systemclipX1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C);
    Vdp1Regs->systemclipY1 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E);
    Vdp1Regs->systemclipX2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x14);
    Vdp1Regs->systemclipY2 = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x16);
}

static void VIDDCVdp1LocalCoordinate(void)  {
    Vdp1Regs->localX = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0C);
    Vdp1Regs->localY = T1ReadWord(Vdp1Ram, Vdp1Regs->addr + 0x0E);
}

static int VIDDCVdp2Reset(void) {
    return 0;
}

static void VIDDCVdp2DrawStart(void)    {
    pvr_wait_ready();
    pvr_scene_begin();
}

static void VIDDCVdp2DrawEnd(void)  {
    pvr_scene_finish();
}

static void VIDDCVdp2DrawScreens(void)  {
}

static void VIDDCVdp2SetResolution(u16 TVMD)    {
}

static void VIDDCVdp2SetPriorityNBG0(int priority)  {
    nbg0priority = priority;
}

static void VIDDCVdp2SetPriorityNBG1(int priority)  {
    nbg1priority = priority;
}

static void VIDDCVdp2SetPriorityNBG2(int priority)  {
    nbg2priority = priority;
}

static void VIDDCVdp2SetPriorityNBG3(int priority)  {
    nbg3priority = priority;
}

static void VIDDCVdp2SetPriorityRBG0(int priority)  {
    rbg0priority = priority;
}

static void VIDDCVdp2ToggleDisplayNBG0(void)    {
    vdp2disptoggle ^= 0x01;
}

static void VIDDCVdp2ToggleDisplayNBG1(void)    {
    vdp2disptoggle ^= 0x02;
}

static void VIDDCVdp2ToggleDisplayNBG2(void)    {
    vdp2disptoggle ^= 0x04;
}

static void VIDDCVdp2ToggleDisplayNBG3(void)    {
    vdp2disptoggle ^= 0x08;
}

static void VIDDCVdp2ToggleDisplayRBG0(void)    {
    vdp2disptoggle ^= 0x10;
}

VideoInterface_struct VIDDC = {
    VIDCORE_DC,
    "Dreamcast PVR Video Interface",
    VIDDCInit,
    VIDDCDeInit,
    VIDDCResize,
    VIDDCIsFullscreen,
    VIDDCVdp1Reset,
    VIDDCVdp1DrawStart,
    VIDDCVdp1DrawEnd,
    VIDDCVdp1NormalSpriteDraw,
    VIDDCVdp1ScaledSpriteDraw,
    VIDDCVdp1DistortedSpriteDraw,
    VIDDCVdp1PolygonDraw,
    VIDDCVdp1PolylineDraw,
    VIDDCVdp1LineDraw,
    VIDDCVdp1UserClipping,
    VIDDCVdp1SystemClipping,
    VIDDCVdp1LocalCoordinate,
    VIDDCVdp2Reset,
    VIDDCVdp2DrawStart,
    VIDDCVdp2DrawEnd,
    VIDDCVdp2DrawScreens,
    VIDDCVdp2SetResolution,
    VIDDCVdp2SetPriorityNBG0,
    VIDDCVdp2SetPriorityNBG1,
    VIDDCVdp2SetPriorityNBG2,
    VIDDCVdp2SetPriorityNBG3,
    VIDDCVdp2SetPriorityRBG0,
    VIDDCVdp2ToggleDisplayNBG0,
    VIDDCVdp2ToggleDisplayNBG1,
    VIDDCVdp2ToggleDisplayNBG2,
    VIDDCVdp2ToggleDisplayNBG3,
    VIDDCVdp2ToggleDisplayRBG0
};
