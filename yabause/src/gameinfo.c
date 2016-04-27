/*  Copyright 2016 Guillaume Duhamel

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

#include <stdio.h>
#include <string.h>

#include "gameinfo.h"
#include "cdbase.h"
#include "cs2.h"

extern ip_struct * cdip;

int GameInfoFromPath(const char * filename, GameInfo * info)
{
   if (cdip != NULL) return 0;

   Cs2Init(0, CDCORE_ISO, filename, NULL, NULL, NULL);
   Cs2GetIP(1);

   memcpy(info, cdip, sizeof(GameInfo));

   Cs2DeInit();

   return 1;
}

int LoadStateSlotScreenshotStream(FILE * fp, int * outputwidth, int * outputheight, u32 ** buffer)
{
   int version, chunksize;
   int totalsize;
   size_t fread_result = 0;

   fseek(fp, 0x14, SEEK_SET);

   if (StateCheckRetrieveHeader(fp, "CART", &version, &chunksize) != 0)
      return -1;
   fseek(fp, chunksize, SEEK_CUR);

   if (StateCheckRetrieveHeader(fp, "CS2 ", &version, &chunksize) != 0)
      return -1;
   fseek(fp, chunksize, SEEK_CUR);

   if (StateCheckRetrieveHeader(fp, "MSH2", &version, &chunksize) != 0)
      return -1;
   fseek(fp, chunksize, SEEK_CUR);

   if (StateCheckRetrieveHeader(fp, "SSH2", &version, &chunksize) != 0)
      return -1;
   fseek(fp, chunksize, SEEK_CUR);

   if (StateCheckRetrieveHeader(fp, "SCSP", &version, &chunksize) != 0)
      return -1;
   fseek(fp, chunksize, SEEK_CUR);

   if (StateCheckRetrieveHeader(fp, "SCU ", &version, &chunksize) != 0)
      return -1;
   fseek(fp, chunksize, SEEK_CUR);

   if (StateCheckRetrieveHeader(fp, "SMPC", &version, &chunksize) != 0)
      return -1;
   fseek(fp, chunksize, SEEK_CUR);

   if (StateCheckRetrieveHeader(fp, "VDP1", &version, &chunksize) != 0)
      return -1;
   fseek(fp, chunksize, SEEK_CUR);

   if (StateCheckRetrieveHeader(fp, "VDP2", &version, &chunksize) != 0)
      return -1;
   fseek(fp, chunksize, SEEK_CUR);

   if (StateCheckRetrieveHeader(fp, "OTHR", &version, &chunksize) != 0)
      return -1;

   fseek(fp, 0x210000, SEEK_CUR);

   fseek(fp, sizeof(int) * 9, SEEK_CUR);

   fread_result = fread((void *) outputwidth, sizeof(int), 1, fp);
   fread_result = fread((void *) outputheight, sizeof(int), 1, fp);

   totalsize = *outputwidth * *outputheight * sizeof(u32);

   *buffer = malloc(totalsize);

   fread_result = fread(*buffer, totalsize, 1, fp);

   return 0;
}

int LoadStateSlotScreenshot(const char * dirpath, const char * itemnum, int slot, int * outputwidth, int * outputheight, u32 ** buffer)
{
   char filename[512];
   FILE * fp;
   int status;

   sprintf(filename, "%s/%s_%03d.yss", dirpath, itemnum, slot);

   fp = fopen(filename, "r");
   if (fp == NULL)
      return -1;

   status = LoadStateSlotScreenshotStream(fp, outputwidth, outputheight, buffer);

   fclose(fp);

   return status;
}
