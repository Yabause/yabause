/*  Copyright 2008 Theo Berkau

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

// This file is a result of the crap that is libogc and their broken
// stdio functions

#include <stdio.h>
#include <string.h>
#include <sys/iosupport.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <ogc/lwp_queue.h>
#include <sdcard.h>

#define MAX_FD 64
#define FD_MASK (MAX_FD-1)

typedef struct 
{
   lwp_node node;
   u32 offset;
} _sd_file;

static struct
{
   sd_file *fp;
} fddata[MAX_FD];

void freefd(int fd)
{
   fddata[fd & FD_MASK].fp = NULL;
}

int findfreefd()
{
   int i;

   for (i = 0; i < MAX_FD; i++)
   {
      if (fddata[i].fp == NULL)
         return i;
   }

   return -1;
}

sd_file *fetchsdfile(int fd)
{
   return fddata[fd & FD_MASK].fp;
}

int sdcardopen(struct _reent *r, void *fileStruct, const char *path,int flags,int mode)
{
   char modetext[3];
   sd_file *fp=NULL;
   int fd = findfreefd();

   if (fd == -1)
      return -1;

   if(flags==O_WRONLY)
      strcpy(modetext, "wb");
   else
      strcpy(modetext, "rb");

   if ((fp = SDCARD_OpenFile(path, modetext)) == NULL)
      return -1;

   fddata[fd].fp = fp;
   return 0;
}

int sdcardclose(struct _reent *r,int fd)
{
   sd_file *fp=(sd_file *)fd;

   if ((fp = fetchsdfile(fd)) != NULL)
   {
      freefd((int)fp);
      return SDCARD_CloseFile(fp);
   }

   return -1;
}

int sdcardwrite(struct _reent *r,int fd,const char *ptr,int len)
{
   sd_file *fp;

   if ((fp = fetchsdfile(fd)) != NULL)
      return SDCARD_WriteFile(fp, ptr, len);
   return -1;
}

int sdcardread(struct _reent *r,int fd,char *ptr,int len)
{
   sd_file *fp;

   if ((fp = fetchsdfile(fd)) != NULL)
      return SDCARD_ReadFile(fp, ptr, len);
   return -1;
}

int sdcardseek(struct _reent *r,int fd,int pos,int dir)
{
   sd_file *fp;
   int ret = -1;

   if ((fp = fetchsdfile(fd)) != NULL)
   {
      if ((ret = SDCARD_SeekFile(fp, pos, dir)) == SDCARD_ERROR_READY)
      {
         // Me shakes my fist at libogc people
         ret = ((_sd_file *)fp)->offset;
      }
   }

   return ret;
}

int sdcardfstat(struct _reent *r,int fd,struct stat *st)
{
   sd_file *fp = NULL;
   SDSTAT sdstat;

   if ((fp = fetchsdfile(fd)) != NULL)
   {
      if(SDCARD_GetStats(fp, &sdstat) == SDCARD_ERROR_READY)
      {
         st->st_size = sdstat.size;
         st->st_ino = sdstat.ino;
         st->st_dev = sdstat.dev;
         st->st_blksize = sdstat.blk_sz;
         st->st_blocks = sdstat.blk_cnt;

         st->st_mode = 0;

         if(sdstat.attr & SDCARD_ATTR_DIR)
            st->st_mode |= S_IFDIR;

         if(sdstat.attr & SDCARD_ATTR_RONLY)
            st->st_mode |= S_IRUSR | S_IXUSR |
                           S_IRGRP | S_IXGRP |
                           S_IROTH | S_IXOTH;
         else
            st->st_mode |= S_IRWXU | S_IRWXG | S_IRWXO;

         return 0;
      }
   }

   return -1;
}

const devoptab_t dotab_sdcard0 = {
"dev0", // name
0, // structSize
sdcardopen, // open_r
sdcardclose, // close_r
sdcardwrite, // write_r
sdcardread, // read_r
sdcardseek, // seek_r
sdcardfstat, // fstat_r
NULL, // stat_r
NULL, // link_r
NULL, // unlink_r
NULL, // chdir_r
NULL, // rename_r
NULL, // mkdir_r
0, // dirStateSize
NULL, // diropen_r
NULL, // dirreset_r
NULL, // dirnext_r
NULL, // dirclose_r
NULL  // statvfs_r
};

const devoptab_t dotab_sdcard1 = {
"dev1", // name
0, // structSize
sdcardopen, // open_r
sdcardclose, // close_r
sdcardwrite, // write_r
sdcardread, // read_r
sdcardseek, // seek_r
sdcardfstat, // fstat_r
NULL, // stat_r
NULL, // link_r
NULL, // unlink_r
NULL, // chdir_r
NULL, // rename_r
NULL, // mkdir_r
0, // dirStateSize
NULL, // diropen_r
NULL, // dirreset_r
NULL, // dirnext_r
NULL, // dirclose_r
NULL  // statvfs_r
};

void CARDIO_Init()
{
   int i;
   for (i = 0; i < MAX_FD; i++)
      fddata[i].fp = NULL;
   AddDevice(&dotab_sdcard0);
   AddDevice(&dotab_sdcard1);
}

int _open_r _PARAMS ((struct _reent *r, const char *file, int flags, int mode))
{
   int device=-1;
   int fd;

   if ((device = FindDevice(file)) >= 0)
   {
      if (devoptab_list[device]->open_r &&
          (fd = devoptab_list[device]->open_r(r, NULL, file, flags, mode)) != -1)
         return ((fd << 16) | (device & 0xFFFF));
   }
   else
      r->_errno = ENOSYS;

   return -1;
}

int _close_r _PARAMS ((struct _reent *r, int fd))
{
   int device = fd & 0xFFFF;
   if (fd != -1 && devoptab_list[device]->close_r)
      return devoptab_list[device]->close_r(r, fd >> 16);
   return -1;
}

_ssize_t _write_r _PARAMS ((struct _reent *r, int fd, void *ptr, size_t len))
{
   int device = fd & 0xFFFF;
   if (fd != -1 && devoptab_list[device]->write_r)
      return devoptab_list[device]->write_r(r, fd >> 16, ptr, len);
   return -1;
}

_ssize_t _read_r _PARAMS ((struct _reent *r, int fd, void *ptr, size_t len))
{
   int device = fd & 0xFFFF;
   if (fd != -1 && devoptab_list[device]->read_r)
      return devoptab_list[device]->read_r(r, fd >> 16, ptr, len);
   return -1;
}

_off_t _lseek_r _PARAMS ((struct _reent *r, int fd, _off_t pos, int dir))
{
   int device = fd & 0xFFFF;
   if (fd != -1 && devoptab_list[device]->seek_r)
      return devoptab_list[device]->seek_r(r, fd >> 16, pos, dir);
   return -1;
}

int _fstat_r _PARAMS ((struct _reent *r, int fd, struct stat *st))
{
   int device = fd & 0xFFFF;
   if (fd != -1 && devoptab_list[device]->fstat_r)
      return devoptab_list[device]->fstat_r(r, fd >> 16, st);
   return -1;
}

