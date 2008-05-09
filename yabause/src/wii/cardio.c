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
#include <stdlib.h>
#include <string.h>
#include <sys/iosupport.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <ogc/lwp_queue.h>
#include "wiisd/tff.h"

#define MAX_FD 64
#define FD_MASK (MAX_FD-1)

typedef struct 
{
   lwp_node node;
   u32 offset;
} _sd_file;

static struct
{
   FIL *fp;
} fddata[MAX_FD];

FATFS sdcard;

void freefd(int fd)
{
   free(fddata[fd & FD_MASK].fp);
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

FIL *fetchsdfile(int fd)
{
   return fddata[fd & FD_MASK].fp;
}

int sdcardopen(struct _reent *r, void *fileStruct, const char *path,int flags,int mode)
{
   int modeflags;
   FIL *fp;
   int fd = findfreefd();
   int ret;

   if (fd == -1)
      return -1;

   if ((fp = malloc(sizeof(FIL))) == NULL)
      return -1;

   if(flags==O_WRONLY)
      modeflags = FA_CREATE_ALWAYS | FA_WRITE;
   else
      modeflags = FA_READ;

   if ((ret = f_open(fp, path, modeflags)) != FR_OK)
      return -1;

   fddata[fd].fp = fp;
   return fd;
}

int sdcardclose(struct _reent *r,int fd)
{
   FIL *fp;

   if ((fp = fetchsdfile(fd)) != NULL)
   {
      f_close(fp);
      freefd(fd);
      return 0;
   }

   return -1;
}

int sdcardwrite(struct _reent *r,int fd,const char *ptr,int len)
{
   FIL *fp;
   int offset=0;
   WORD byteswritten;

   if ((fp = fetchsdfile(fd)) != NULL)
   {
      // Maximum write size is 64k
      while (len > 65000)
      {
         if (f_write(fp, ptr + offset, 65000, &byteswritten) != FR_OK)
             return -1;
         offset += byteswritten;
         len -= byteswritten;
      }

      // Write the rest
      if (f_write(fp, ptr + offset, len, &byteswritten) == FR_OK)
         return len;
   }
   return -1;
}

int sdcardread(struct _reent *r,int fd,char *ptr,int len)
{
   FIL *fp;
   int offset=0;
   WORD bytesread;

   if ((fp = fetchsdfile(fd)) != NULL)
   {
      // Maximum read size is 64k
      while (len > 65000)
      {
         if (f_read(fp, (BYTE *)ptr + offset, 65000, &bytesread) != FR_OK)
             return -1;
         offset += bytesread;
         len -= bytesread;
      }

      // Write the rest
      if (f_read(fp, (BYTE *)ptr + offset, len, &bytesread) == FR_OK)
         return len;
   }
   return -1;
}

int sdcardseek(struct _reent *r,int fd,int pos,int dir)
{
   FIL *fp;
   int ret = -1;

   if ((fp = fetchsdfile(fd)) != NULL)
   {
      switch (dir)
      {
         case SEEK_SET:
            if (f_lseek(fp, pos) == FR_OK)
               return fp->fptr;
            break;
         case SEEK_CUR:
            if (f_lseek(fp, fp->fptr+pos) == FR_OK)
               return fp->fptr; 
            break;
         case SEEK_END:
            if (f_lseek(fp, fp->fsize+pos) == FR_OK)
               return fp->fptr;             
            break;
         default: break;
      }
   }

   return ret;
}

int sdcardfstat(struct _reent *r,int fd,struct stat *st)
{
   FIL *fp = NULL;
//   FILINFO sdstat;

   if ((fp = fetchsdfile(fd)) != NULL)
   {
      {
         st->st_size = fp->fsize;
#if 0
         st->st_ino = sdstat.ino;
         st->st_dev = sdstat.dev;
         st->st_blksize = sdstat.blk_sz;
         st->st_blocks = sdstat.blk_cnt;
#endif
         st->st_mode = 0;

         if(fp->flag & FA_READ)
            st->st_mode |= S_IRUSR | S_IXUSR |
                           S_IRGRP | S_IXGRP |
                           S_IROTH | S_IXOTH;
         if(fp->flag & FA_WRITE)
            st->st_mode |= S_IRWXU | S_IRWXG | S_IRWXO;
         return 0;
      }
   }
   return -1;
}

const devoptab_t dotab_sdcard = {
"sd", // name
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
   setDefaultDevice(AddDevice(&dotab_sdcard));
   f_mount(0, &sdcard);
}

int _open_r _PARAMS ((struct _reent *r, const char *file, int flags, int mode))
{
   int device=-1;
   int fd;

   if ((device = FindDevice(file)) >= 0)
   {
      if (devoptab_list[device]->open_r &&
          (fd = devoptab_list[device]->open_r(r, NULL, file, flags, mode)) != -1)
         return ((fd << 4) | (device & 0xF));
   }
   else
      r->_errno = ENOSYS;

   return -1;
}

int _close_r _PARAMS ((struct _reent *r, int fd))
{
   int device = fd & 0xF;
   if (fd != -1 && devoptab_list[device]->close_r)
      return devoptab_list[device]->close_r(r, fd >> 4);
   return -1;
}

_ssize_t _write_r _PARAMS ((struct _reent *r, int fd, void *ptr, size_t len))
{
   int device = fd & 0xF;
   if (fd != -1 && devoptab_list[device]->write_r)
      return devoptab_list[device]->write_r(r, fd >> 4, ptr, len);
   return -1;
}

_ssize_t _read_r _PARAMS ((struct _reent *r, int fd, void *ptr, size_t len))
{
   int device = fd & 0xF;
   if (fd != -1 && devoptab_list[device]->read_r)
      return devoptab_list[device]->read_r(r, fd >> 4, ptr, len);
   return -1;
}

_off_t _lseek_r _PARAMS ((struct _reent *r, int fd, _off_t pos, int dir))
{
   int device = fd & 0xF;
   if (fd != -1 && devoptab_list[device]->seek_r)
      return devoptab_list[device]->seek_r(r, fd >> 4, pos, dir);
   return -1;
}

int _fstat_r _PARAMS ((struct _reent *r, int fd, struct stat *st))
{
   int device = fd & 0xF;
   if (fd != -1 && devoptab_list[device]->fstat_r)
      return devoptab_list[device]->fstat_r(r, fd >> 4, st);
   return -1;
}

