/*  Copyright 2007 Theo Berkau

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

#include "core.h"
#include "sh2core.h"
#include "yabause.h"

typedef struct
{
  u8 magic[2];
  u16 numsections;
  u32 timedate;
  u32 symtabptr;
  u32 numsymtabs;
  u16 optheader;
  u16 flags;
} coff_header_struct;

typedef struct
{
  u8 magic[2];
  u16 versionstamp;
  u32 textsize;
  u32 datasize;
  u32 bsssize;
  u32 entrypoint;
  u32 textaddr;
  u32 dataaddr;
} aout_header_struct;

typedef struct
{
  s8 name[8];
  u32 physaddr;
  u32 virtaddr;
  u32 sectionsize;
  u32 sectionptr;
  u32 relptr;
  u32 linenoptr;
  u16 numreloc;
  u16 numlineno;
  u32 flags;
} section_header_struct;


#define WordSwap(x) x = ((x & 0xFF00) >> 8) + ((x & 0x00FF) << 8);
#define DoubleWordSwap(x) x = (((x & 0xFF000000) >> 24) + \
                              ((x & 0x00FF0000) >> 8) + \
                              ((x & 0x0000FF00) << 8) + \
                              ((x & 0x000000FF) << 24));

//////////////////////////////////////////////////////////////////////////////

int MappedMemoryLoadCoff(const char *filename)
{
   coff_header_struct coff_header;
   aout_header_struct aout_header;
   section_header_struct *section_headers=NULL;
   FILE *fp;
   u8 *buffer;
   u32 i, j;

   if ((fp = fopen(filename, "rb")) == NULL)
      return -1;

   fread((void *)&coff_header, sizeof(coff_header), 1, fp);
#ifndef WORDS_BIGENDIAN
   WordSwap(coff_header.numsections);
   DoubleWordSwap(coff_header.timedate);
   DoubleWordSwap(coff_header.timedate);
   DoubleWordSwap(coff_header.symtabptr);
   DoubleWordSwap(coff_header.numsymtabs);
   WordSwap(coff_header.optheader);
   WordSwap(coff_header.flags);
#endif

   if (coff_header.magic[0] != 0x05 || coff_header.magic[1] != 0x00 ||
       coff_header.optheader != sizeof(aout_header))
   {
      // Not SH COFF or is missing the optional header
      fclose(fp);
      return -1;
   }

   fread((void *)&aout_header, sizeof(aout_header), 1, fp);
#ifndef WORDS_BIGENDIAN
   WordSwap(aout_header.versionstamp);
   DoubleWordSwap(aout_header.textsize);
   DoubleWordSwap(aout_header.datasize);
   DoubleWordSwap(aout_header.bsssize);
   DoubleWordSwap(aout_header.entrypoint);
   DoubleWordSwap(aout_header.textaddr);
   DoubleWordSwap(aout_header.dataaddr);
#endif

   // Read in each section header
   if ((section_headers = (section_header_struct *)malloc(sizeof(section_header_struct) * coff_header.numsections)) == NULL)
   {
      fclose(fp);
      return -2;
   }

   // read in section headers
   for (i = 0; i < coff_header.numsections; i++)
   {
      fread((void *)&section_headers[i], sizeof(section_header_struct), 1, fp);
#ifndef WORDS_BIGENDIAN
      DoubleWordSwap(section_headers[i].physaddr);
      DoubleWordSwap(section_headers[i].virtaddr);
      DoubleWordSwap(section_headers[i].sectionsize);
      DoubleWordSwap(section_headers[i].sectionptr);
      DoubleWordSwap(section_headers[i].relptr);
      DoubleWordSwap(section_headers[i].linenoptr);
      WordSwap(section_headers[i].numreloc);
      WordSwap(section_headers[i].numlineno);
      DoubleWordSwap(section_headers[i].flags);
#endif
   }

   YabauseResetNoLoad();

   // Setup the vector table area, etc.
   YabauseSpeedySetup();

   // Read in sections, load them to ram
   for (i = 0; i < coff_header.numsections; i++)
   {
      if (section_headers[i].sectionsize == 0 ||
          section_headers[i].sectionptr == 0)
         // Skip to the next section
         continue;
      
      if ((buffer = (u8 *)malloc(section_headers[i].sectionsize)) == NULL)
      {
         fclose(fp);
         free(section_headers);
         return -2;
      }

      fseek(fp, section_headers[i].sectionptr, SEEK_SET);
      fread((void *)buffer, 1, section_headers[i].sectionsize, fp);

      for (j = 0; j < section_headers[i].sectionsize; j++)
         MappedMemoryWriteByte(section_headers[i].physaddr+j, buffer[j]);

      free(buffer);
   }

   MSH2->regs.PC = aout_header.entrypoint;
   return 0;
}


//////////////////////////////////////////////////////////////////////////////

int MappedMemoryLoadElf(const char *filename)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
