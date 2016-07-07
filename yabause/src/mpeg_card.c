/*  Copyright 2014-2016 James Laird-Wah
    Copyright 2004-2006, 2013 Theo Berkau

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

/*! \file mpeg_card.c
    \brief Mpeg card implementation
*/

#include "core.h"
#include "sh7034.h"
#include "debug.h"
#include "assert.h"
#include "error.h"
#include "assert.h"
#include "vidshared.h"
#include "titan/titan.h"

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_MPEG
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#define BUFFER_SIZE 4096

struct YabCodec
{
  AVCodec *codec;
  AVCodecContext *context;
  AVFrame *frame;
  AVPacket packet;
  struct SwsContext *sws_context;
  int is_audio;

  u8 buffer[BUFFER_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
  
  FILE * file;
};

struct YabMpegState
{
   struct YabCodec video;
   struct YabCodec audio;
   int inited;
}yab_mpeg = {0};

void yab_mpeg_do_frame(struct YabCodec * c);
void yab_mpeg_init();
void yab_mpeg_play_file(struct YabCodec * c, char * filename);
u32 pixels[704*480] = {0};
u8* out_buf[4];
int out_linesize[4];
extern pixel_t *dispbuffer;
void ScspReceiveMpeg (const u8 *samples, int len);
#endif

void TitanPutPixel(int priority, s32 x, s32 y, u32 color, int linescreen, vdp2draw_struct* info);

/////////////////////////////////////////////////////////////////////

void mpeg_reg_debug_print();

//a100000
//write
//FEDC BA98 7654 3210
//.ccc .yyy iiii h.d.
//d : 0 == display on, 1 == display off
//h : 0 == HD, 1 == standard def (mpeg set mode)
//i : interpolation (set to opposite of command, 0xf - command_value)
//y : y gain
//c : c gain

//read
//FEDC BA98 7654 3210
//.... ...o .... fpdc
// c == video decoding
// d == display
// p == pause
// f == freeze
// o == output prep complete

//a100002
//FEDC BA98 7654 3210
//.... .... .... .ooo
//.... .... .ppp ppp.
//o : display window offset x (not sure how many bits)
//p: display window pos x (seems to occupy the same bits, modal?) (not sure how many bits)

//a100004
//FEDC BA98 7654 3210
//.... .... .... .ooo
//.... .... .ppp ppp.
//o : display window offset y
//p: display window pos y

//a100006

//when writing
//FEDC BA98 7654 3210
//.... ..xx xxxx xxx.
//x : frame buffer window pos x (not sure how many bits total)

//a100008
//FEDC BA98 7654 3210
//.... ..yy yyyy yyy.
//y : frame buffer window pos y

//a10000e
//FEDC BA98 7654 3210
//.... .... .... xxxx
//x : zoom rate x (not sure how many bits total)

//a100010
//FEDC BA98 7654 3210
//.... .... .... yyyy
//y : zoom rate y (not sure how many bits total)

//a100012
//FEDC BA98 7654 3210
//cccc cccc cccc cccc
//c : border color (rgb555?)

//a100014
//FEDC BA98 7654 3210
//.... .... .s.. fpot
//t : 0 == decode timing mode is host, 1 == decode timing mode is vsync
//o : 0 == output to host, 1 == output to vdp2
//p : seems pause related. writing 0 in cr2 of set decoding method sets it to 0
//f : seems freeze related. write 0 in cr4 of set decoding method to set it to 0
//s : 0 == still picture, 1 == video (mpeg set mode)

//a10001c
//FEDC BA98 7654 3210
//hhhh wwww ..b. ....
//b : blur on/off (todo recheck)
//w : mosaic width 0xf == off, 0-9 settings
//h : mosaic height 0xf == off, 0-9 settings

//a100022
//FEDC BA98 7654 3210
//fff. .... .... ....
//f : frame type (sent in status info)
//    1 : i frame
//    2 : p frame
//    3 : b frame
//    4 : d frame

//a10003e
//FEDC BA98 7654 3210
//..i. .... .... ....
// i : 0 == not interlaced 1 == interlaced

struct MpegCard
{
   u16 reg_00;
   u16 window_offset_x;//0x02
   u16 window_offset_y;//0x04
   u16 window_size_x;//0x06
   u16 window_size_y;//0x08
   u16 framebuffer_x;//0x0a
   u16 framebuffer_y;//0x0c
   u16 zoom_x;//0x0e
   u16 zoom_y;//0x10
   u16 border_color;//0x12
   u16 reg_14;
   u16 reg_18;
   u16 reg_1a;
   u16 mosaic_blur;//0x1c
   u16 reg_1e;
   u16 reg_20;
   u16 reg_22;
   u16 reg_32;
   u16 reg_34;
   u16 reg_3e;
   u16 reg_80000;
   u16 reg_80008;
   
   u16 ram[0x40000];
   u32 ram_ptr;
}mpeg_card;

#define RAM_MASK 0x3ffff

u32 mpeg_framebuffer[704 * 480] = { 0 };

void mpeg_card_write_word(u32 addr, u16 data)
{
   if ((addr & 0xfffff) != 0x34 && (addr & 0xfffff) != 0x36)
   {
  //    CDLOG("mpeg lsi ww %08x, %04x\n", addr, data);
      //mpeg_reg_debug_print();
   }
   switch (addr & 0xfffff)
   {
   case 0:
      mpeg_card.reg_00 = data;
      return;
   case 2:
      mpeg_card.window_offset_x = data;
      return;
   case 4:
      mpeg_card.window_offset_y = data;
      return;
   case 6:
      mpeg_card.window_size_x = data;
      return;
   case 8:
      mpeg_card.window_size_y = data;
      return;
   case 0xa:
      mpeg_card.framebuffer_x = data;
      return;
   case 0xc:
      mpeg_card.framebuffer_y = data;
      return;
   case 0xe:
      mpeg_card.zoom_x = data;
      return;
   case 0x10:
      mpeg_card.zoom_y = data;
      return;
   case 0x12:
      mpeg_card.border_color = data;
      return;
   case 0x14:
      mpeg_card.reg_14 = data;
      return;
   case 0x18:
      mpeg_card.reg_18 = data;
      return;
   case 0x1a:
      mpeg_card.reg_1a = data;
      return;
   case 0x1c:
      mpeg_card.mosaic_blur = data;
      return;
   case 0x1e:
      mpeg_card.reg_1e = data;
      return;
   case 0x20:
      mpeg_card.reg_20 = data;
      return;
   case 0x22:
      mpeg_card.reg_22 = data;
      return;
   case 0x30:
      mpeg_card.ram_ptr = data << 2;
      return;
   case 0x32:
      mpeg_card.reg_32 = data;
      return;
   case 0x34:
      mpeg_card.reg_34 = data;
      return;
   case 0x36:
      mpeg_card.ram_ptr &= RAM_MASK;
      mpeg_card.ram[mpeg_card.ram_ptr++] = data;
      return;
   case 0x3e:
      mpeg_card.reg_3e = data;
      return;
   case 0x80000:
      mpeg_card.reg_80000 = data;
      return;
   case 0x80008:
      mpeg_card.reg_80008 = data;
      return;
   }

   assert(0);
}

u16 mpeg_card_read_word(u32 addr)
{
 //  if ((addr & 0xfffff) != 0x34 && (addr & 0xfffff) != 0x36)
 //     CDLOG("mpeg lsi rw %08x %08x\n", addr, SH1->regs.PC);
   switch (addr & 0xfffff)
   {
   case 0:
      //0x10 needs to be clear for the video interrupt to set mpcm
      //get status wants 1 << 8 set, to clear "output prep" bit in mpeg video status
      //get interrupt wants 1 << 13 set, to clear "sequence end detected" interrupt bit
      return (mpeg_card.reg_00 & ~0x10) | (1 << 8) | (1 << 13);
   case 2:
      return mpeg_card.window_offset_x;//probably not right
   case 0x34:
      return 1;//return mpeg_card.reg_34 | 5;//first bit is always set?
   case 0x36:
      mpeg_card.ram_ptr &= RAM_MASK;
      return mpeg_card.ram[mpeg_card.ram_ptr++];
   case 0x80000:
      return mpeg_card.reg_80000;
   case 0x80008:
      return mpeg_card.reg_80008;
   }
//   assert(0);

   return 0;
}

void set_mpeg_audio_data_transfer_irq()
{
   sh1_assert_tioca(0);
}

void set_mpeg_video_data_transfer_irq()
{
   sh1_assert_tioca(1);
}

void set_mpeg_audio_irq()
{
   sh1_assert_tioca(2);
}

const u16 mosaic_masks[] = {
   0xfffe,//0
   0xfffc,//1
   0xfff8,//2
   0xfff0,//3
   0xffe0,//4
   0xffc0,//5
   0xff80,//6
   0xff00,//7
   0xfe00,//8
   0xfc00,//9
   0xf800,//a
   0xf000,//b
   0xe000,//c
   0xc000,//d
   0x8000,//e
   0xffff,//f (off)
};

void mpeg_render()
{
   int x = 0; int  y = 0;
   vdp2draw_struct info = { 0 };
   int priority = (Vdp2Regs->PRINA >> 8) & 7;

   //offset from a center value or fixed point?
   int window_x_offset = (mpeg_card.window_offset_x - 0x96) >> 1;
   int window_y_offset = (mpeg_card.window_offset_y - 0x26) >> 1;

   int framebuffer_x_offset = mpeg_card.framebuffer_x >> 1;
   int framebuffer_y_offset = mpeg_card.framebuffer_y >> 1;

   //start framebuffer pos from offset
   float fb_x = framebuffer_x_offset;
   float fb_y = framebuffer_y_offset;

   int window_last_x = window_x_offset + (mpeg_card.window_size_x >> 1);
   int window_last_y = window_y_offset + (mpeg_card.window_size_y >> 1);

   //zoom
   float fb_x_inc = 1;
   float fb_y_inc = 1;

   int mosaic_x = (mpeg_card.mosaic_blur >> 8) & 0xf;
   int mosaic_y = (mpeg_card.mosaic_blur >> 12) & 0xf;

   info.titan_which_layer = TITAN_NBG1;

   if (window_x_offset < 0)
      window_x_offset = 0;

   if (window_y_offset < 0)
      window_y_offset = 0;

   for (y = window_y_offset; y < window_last_y; y++)//in output coordinates
   {
      fb_x = framebuffer_x_offset;

      for (x = window_x_offset; x < window_last_x; x++)
      {
         u32 pixel = 0;
         int x_val = ((int)fb_x) & mosaic_masks[mosaic_x];
         int y_val = ((int)fb_y) & mosaic_masks[mosaic_y];

         u32 offset = (y_val * 704) + x_val;

         if (offset < (704 * 480))
         {
            pixel = mpeg_framebuffer[offset];

            if(x < 704 && y < 480)
               TitanPutPixel(priority, x, y, pixel, 0, &info);
         }
         fb_x+= fb_x_inc;
      }
      fb_y+= fb_y_inc;
   }
}

void set_mpeg_video_irq()
{
   sh1_assert_tiocb(2);
#ifdef HAVE_MPEG
   yab_mpeg_do_frame(&yab_mpeg.video);
   yab_mpeg_do_frame(&yab_mpeg.audio);
#else
   if(Vdp2Regs->EXTEN & 1)//exbg enabled
      mpeg_render();
#endif
}

void mpeg_card_set_all_irqs()
{
   set_mpeg_audio_data_transfer_irq();
   set_mpeg_video_data_transfer_irq();

   //triggers jump to null pointer, get hardware info shows mpeg present without it
   //set_mpeg_audio_irq();
   set_mpeg_video_irq();
}

void mpeg_card_init()
{
   int x = 0; int  y = 0;
   memset(&mpeg_card, 0, sizeof(struct MpegCard));
   memset(&mpeg_framebuffer, 0, sizeof(u32) * 704*480);
   for (y = 0; y < 480; y++)
   {
      for (x = 0; x < 704; x++)
      {
         mpeg_framebuffer[(y * 704) + x] = 0xff00ff00;//green
      }
   }

   for (y = 0; y < 240; y++)
   {
      for (x = 8; x < 320; x++)
      {
         mpeg_framebuffer[(y * 704) + x] = 0xff0000ff;//red
      }
   }

   mpeg_card.reg_34 = 1;
#ifdef HAVE_MPEG
   yab_mpeg_init();
#endif
}

void mpeg_reg_debug_print()
{
   if((mpeg_card.reg_00 >> 1) & 1)
      CDLOG("Display disabled\n");
   else
      CDLOG("Display enabled\n");

   CDLOG("Interpolation %01X\n", 0xf - ((mpeg_card.reg_00 >> 4) & 0xf));

   CDLOG("Window x %d\n", mpeg_card.window_size_x >> 1);

   CDLOG("Window y %d\n", mpeg_card.window_size_y >> 1);

   CDLOG("Mosaic x %d\n", (mpeg_card.mosaic_blur >> 8) & 0xf);

   CDLOG("Mosaic y %d\n", (mpeg_card.mosaic_blur >> 12) & 0xf);
   
   if ((mpeg_card.mosaic_blur >> 5) & 1)
      CDLOG("Blur disabled\n");
   else
      CDLOG("Blur enabled\n");
}

/////////////////////////////////////////////////////////////////////
#ifdef HAVE_MPEG
void yab_mpeg_init_codec(struct YabCodec * c, int type)
{
  c->codec = avcodec_find_decoder(type);

  if(!c->codec)
  {
     YabErrorMsg("couldn't find decoder");
     return;
  }

  c->context = avcodec_alloc_context3(c->codec);

  if(!c->context)
  {
     YabErrorMsg("couldn't allocate context");
     return;
  }

  if (avcodec_open2(c->context, c->codec, NULL) < 0)
  {
     YabErrorMsg("couldn't open codec");
     return;
  }

  av_init_packet(&c->packet);

  c->packet.data = c->buffer;
}

void yab_mpeg_deinit_codec(struct YabCodec * c)
{
  avcodec_close(c->context);
}

void yab_mpeg_init()
{
  int ret = 0;
  av_register_all();
  memset(&yab_mpeg,0,sizeof(struct YabMpegState));
  yab_mpeg_init_codec(&yab_mpeg.video,AV_CODEC_ID_MPEG1VIDEO);
  yab_mpeg_init_codec(&yab_mpeg.audio,AV_CODEC_ID_MP2);
  
  yab_mpeg_play_file(&yab_mpeg.video, "/home/d/yab-f/yabause/yabauseut/src/buildcd/M2TEST/MOVIE.m1v");
  yab_mpeg_play_file(&yab_mpeg.audio, "/home/d/yab-f/yabause/yabauseut/src/buildcd/M2TEST/MOVIE.MP2");

  yab_mpeg.audio.is_audio = 1;
  yab_mpeg.inited = 1;

  yab_mpeg.audio.context->channels = 2;
  yab_mpeg.audio.context->sample_rate = 44100;

  ret = av_image_alloc(out_buf,out_linesize, 320, 240,PIX_FMT_RGB32, 1);

  if(!ret)
    assert(0);
}

void write_frame_to_video_buffer(struct YabCodec * c)
{
  int x = 0, y = 0;
  int out_width = 320;
  int out_height = 240;

  int mpeg_width = c->frame->linesize[0];
  u8 * mpeg_buf = c->frame->data[0];

  //convert yuv to rgb with sws
  if(!c->sws_context)
  {
    c->sws_context = sws_getContext(
      //source width, height, format
      c->context->width,
      c->context->height,
      c->context->pix_fmt,
      //dest width, height, format
      out_width,
      out_height,
      PIX_FMT_RGB32,
      //flags
      SWS_BILINEAR,
      //source/dest filters
      NULL,
      NULL,
      //param
      NULL
    );
  }

  sws_scale(
    c->sws_context,
    (u8 const * const *)c->frame->data,
    c->frame->linesize,
    0,
    c->context->height,
    out_buf,
    out_linesize);

  memcpy(pixels, out_buf[0], 320*240*4);

  for(y = 0; y < out_height; y++)
  {
    for(x = 0; x < out_width; x++)
    {
       dispbuffer[(y*out_width) + x] = pixels[(y * mpeg_width) + x];
    }
  }
}

void write_sound(struct YabCodec * c)
{
   int size = av_get_bytes_per_sample(c->context->sample_fmt);
   ScspReceiveMpeg(c->frame->data[0],size);
}

void yab_mpeg_do_frame(struct YabCodec * c)
{
  int got_frame = 0;
  int num_tries = 0;

  if(!yab_mpeg.inited)
  {
     yab_mpeg_init();
  }

  c->packet.size = fread(c->buffer, 1, BUFFER_SIZE, c->file);

  while(c->packet.size > 0) 
  {
    int length = 0;

    if(!c->frame)
    {
      c->frame = av_frame_alloc();
      if(!c->frame)
      {
        YabErrorMsg("couldn't allocate frame");
        return;
      }
    }

    if(!c->is_audio)
      length = avcodec_decode_video2(c->context, c->frame, &got_frame, &c->packet);
    else
      length = avcodec_decode_audio4(c->context, c->frame, &got_frame, &c->packet);
    
    if(length < 0)
    {
      return;
    }

    if(num_tries > 8)
    {
       return;
    }
    if(got_frame)
    {
      if(!c->is_audio)
        write_frame_to_video_buffer(c);
      else
        write_sound(c);
      break;
    }
    num_tries++;
  }
}

void yab_mpeg_do_video_frame()
{
  yab_mpeg_do_frame(&yab_mpeg.video);
  yab_mpeg_do_frame(&yab_mpeg.audio);
}

void yab_mpeg_play_file(struct YabCodec * c, char * filename)
{
  c->file = fopen(filename, "rb");

  if(!c->file)
  {
    YabErrorMsg("couldn't open file");
    assert(0);
    return;
  }
}
#endif
