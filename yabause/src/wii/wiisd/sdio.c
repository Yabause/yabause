/*
 *  Copyright (C) 2008 svpe, #wiidev at efnet
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
// pretty much everything here has been reversed from the twilight hack elf loader
// my implementation is *really* bad and will fail fail in random situations.
// it should not be used anywhere else. i recommend waiting for the libogc update.
// you have been warned....

#include <gccore.h>
#include <ogc/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static s32 sd_fd = -1;

static u32 status __attribute__((aligned(32)));

s32 sd_send_cmd(u32 cmd, u32 type, u32 resp, u32 arg, u32 blocks, u32 bsize, u32 addr)
{
	static u32 request[9] __attribute__((aligned(32)));
	static u32 reply[4] __attribute__((aligned(32)));
	u32 r;

	memset(request, 0, sizeof(request));
	memset(reply, 0, sizeof(reply));

	request[0] = cmd;
	request[1] = type;
	request[2] = resp;
	request[3] = arg;
	request[4] = blocks;
	request[5] = bsize;
	request[6] = addr;
	request[7] = 0;
	request[8] = 0;

	r = IOS_Ioctl(sd_fd, 7, (u8 *)request, 36, (u8 *)reply, 0x10);
	printf("sd_send_cmd(%x, %x, %x, %x, %x, %x, %x) = %d", cmd, type, resp, arg, blocks, bsize, addr, r);
	printf("  -> %x %x %x %x\n", reply[0], reply[1], reply[2], reply[3]); // TODO: add some argument for this reply

	return r;
}

s32 sd_reset()
{
	s32 r;
	
	r = IOS_Ioctl(sd_fd, 4, 0, 0, (u8 *)&status, 4);
	printf("sd_reset(): r = %d; status = %d\n", r, status);
	return r;
}

s32 sd_select()
{
	s32 r;

	r = sd_send_cmd(7, 3, 2, status & 0xFFFF0000, 0, 0, 0);
	printf("sd_select(): r = %d\n", r);
	return r;
}

s32 sd_deselect()
{
	s32 r;
	r = sd_send_cmd(7, 3, 2, 0, 0, 0, 0);
	printf("sd_deselect(): r = %d\n", r);
	return r;
}

s32 sd_set_blocklen(u32 len)
{
	s32 r;

	r = sd_send_cmd(0x10, 3, 1, len, 0, 0, 0);
	printf("sd_set_blocklen(%x) = %d\n", len, r);
	return r;
}

u8 sd_get_hcreg()
{
	s32 r;
	static u32 data __attribute__((aligned(32)));
	static u32 query[6] __attribute__((aligned(32)));

	memset(&data, 0, 4);
	memset(query, 0, 0x18);

	query[0] = 0x28;
	query[3] = 1;
	query[4] = 0;
	
	r = IOS_Ioctl(sd_fd, 2, (u8 *)query, 0x18, (u8 *)&data, 4);
	printf("sd_get_hcreg() = %d; r = %d\n", data & 0xFF, r);
	return data & 0xFF;
}

s32 sd_set_hcreg(u8 value)
{
	s32 r;
	static u32 query[6] __attribute__((aligned(32)));

	memset(query, 0, 0x18);

	query[0] = 0x28;
	query[3] = 1;
	query[4] = value;
	
	r = IOS_Ioctl(sd_fd, 1, (u8 *)query, 0x18, 0, 0);
	printf("sd_set_hcreg(%d) = %d\n", value, r);
	return r;
}

s32 sd_set_buswidth(u8 w)
{
	s32 r;
	u8 reg;

	r = sd_send_cmd(0x37, 3, 1, status & 0xFFFF0000, 0, 0, 0);
	if(r < 0)
		return r;
	r = sd_send_cmd(6, 3, 1, (w == 4 ? 2 : 0), 0, 0, 0);
	if(r < 0)
		return r;
	
	reg = sd_get_hcreg();
	
	reg &= ~2;
	if(w == 4)
		reg |= 2;
	return sd_set_hcreg(reg);
}

s32 sd_clock()
{
	s32 r;
	static u32 c __attribute__((aligned(32)));

	c = 1;
	r = IOS_Ioctl(sd_fd, 6, &c, 4, 0, 0);
	printf("sd_clock() = %d\n", r);
	return r;
}

s32 sd_read(u32 n, u8 *buf)
{
	s32 r;
	static u8 buffer[0x200] __attribute__((aligned(32)));
	static u32 query[9] __attribute__((aligned(32)));
	static u32 res[4] __attribute__((aligned(32)));

	static ioctlv v[3] __attribute__((aligned(32)));

//	printf("sd_read(%d) called\n", n);

	memset(buffer, 0xAA, 0x200);  // why is this buffer filled with 0xAA? is this really needed?
	memset(query, 0, 0x24);
	memset(res, 0, 0x10);

	query[0] = 0x12;
	query[1] = 3;
	query[2] = 1;
	query[3] = n * 0x200;	// arg
	query[4] = 1;		// block_count
	query[5] = 0x200;	// sector size
	query[6] = (u32)buffer;	// buffer
	query[7] = 1;		// ?
	query[8] = 0;		// ?

	v[0].data = (u32 *)query;
	v[0].len = 0x24;
	v[1].data =(u32 *)buffer;
	v[1].len = 0x200;
	v[2].data = (u32 *)res;
	v[2].len = 0x10;

	// FIXME: is this really needed? twilight hack loader does it.
	DCFlushRange(buffer, 0x200);
	DCInvalidateRange(buffer, 0x200);

	r = IOS_Ioctlv(sd_fd, 7, 2, 1, v);

	if(r != 0)
	{
		printf("sd_read() = %d\n", r);
		printf("  %x %x %x %x\n", res[0], res[1], res[2], res[3]);
		return r;
	}

	memcpy(buf, buffer, 0x200);

	return 0;
}

s32 sd_write(u32 n, const u8 *buf)
{
       s32 r;
       static u8 buffer[0x200] __attribute__((aligned(32)));
       static u32 query[9] __attribute__((aligned(32)));
       static u32 res[4] __attribute__((aligned(32)));

       static ioctlv v[3] __attribute__((aligned(32)));

//     printf("sd_write(%d) called\n", n);

       memcpy(buffer, buf, 0x200);
       memset(query, 0, 0x24);
       memset(res, 0, 0x10);

       query[0] = 0x19;
       query[1] = 3;
       query[2] = 1;
       query[3] = n * 0x200;   // arg
       query[4] = 1;           // block_count
       query[5] = 0x200;       // sector size
       query[6] = (u32)buffer; // buffer
       query[7] = 1;           // ?
       query[8] = 0;           // ?

       v[0].data = (u32 *)query;
       v[0].len = 0x24;
       v[1].data =(u32 *)buffer;
       v[1].len = 0x200;
       v[2].data = (u32 *)res;
       v[2].len = 0x10;

       r = IOS_Ioctlv(sd_fd, 7, 2, 1, v);

       if(r != 0)
       {
               printf("sd_write() = %d\n", r);
               printf("  %x %x %x %x\n", res[0], res[1], res[2], res[3]);
               return r;
       }

       return 0;
}

s32 sd_init()
{
	s32 r;

	if(sd_fd > 0)
	{
		printf("sd_init() called more than once. using old sd_fd: %d\n", sd_fd);
		return 0;
	}

	sd_fd = IOS_Open("/dev/sdio/slot0", 0);
	printf("sd_fd = %d\n", sd_fd);
	if(sd_fd < 0)
		return sd_fd;
	

	// TODO: close sd_fd on failure and do proper error check here
	r = sd_reset();
	if(r < 0)
		return r;

	sd_select();
	r = sd_set_blocklen(0x200);
	if(r < 0)
		return sd_fd;
	r = sd_set_buswidth(4);
	if(r < 0)
		return sd_fd;
	r = sd_clock();
	if(r < 0)
		return sd_fd;

	return 0;

}

s32 sd_deinit()
{
	sd_deselect();
	return IOS_Close(sd_fd);
}


