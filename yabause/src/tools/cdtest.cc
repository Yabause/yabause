/*******************************************************************************
  CDTEST - Yabause CD interface tester

  (c) Copyright 2004 Theo Berkau(cwx@softhome.net)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*******************************************************************************/

// This program is designed to be linked with any port's cd.cc file.
// example: g++ windows\cd.cc tools\cdtest.cc -o cdtest.exe

// Once it's compiled, run it with the cdrom path as your argument
// example: cdtest g:

// SPECIAL NOTE: You need to use the Sega Boot CD as your test cd to have
// accurate test results. I hope to change this in the future.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../cd.hh"

#define PROG_NAME "CDTEST"
#define VER_NAME "1.00"
#define COPYRIGHT_YEAR "2004"

int testspassed=0;

unsigned char cdbuffer[2352];
unsigned long cdTOC[102];

unsigned long bootcdTOC[102] = {
0x41000096,
0x010002F2,
0x010004B4,
0x01000678,
0x0100083C,
0x01000A00,
0x01000BC4,
0x01000D88,
0x01000F4C,
0x01001110,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0xFFFFFFFF,
0x41010000,
0x010A0000,
0x0100123F
};

//////////////////////////////////////////////////////////////////////////////

void ProgramUsage()
{
   printf("%s v%s - by Cyber Warrior X (c)%s\n", PROG_NAME, VER_NAME, COPYRIGHT_YEAR);
   printf("usage: %s <drive pathname as specified in cd.cc>\n", PROG_NAME);
   exit (1);
}

//////////////////////////////////////////////////////////////////////////////

void cleanup(void)
{
   if (CDDeInit() != 0)
   {
      printf("CDDeInit error: Unable to deinitialize cdrom\n");
      exit(1);
   }
   else testspassed++;

   printf("Test Score: %d/11 \n", testspassed);
}

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
   char *cdrom_name;
   unsigned long f_size=0;
   int status;
   char syncheader[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                           0xFF, 0xFF, 0xFF, 0x00};
   FILE *fp;

   atexit(cleanup);

   if (argc != 2)
   {
      ProgramUsage();
   }

   printf("%s v%s - by Cyber Warrior X(c)%s\n", PROG_NAME, VER_NAME, COPYRIGHT_YEAR);

   cdrom_name = argv[1];

   if (CDInit(cdrom_name) != 0)
   {
      printf("CDInit error: Unable to initialize cdrom\n");
      exit(1);
   }
   else testspassed++;

   // Let's make sure we're returning the proper status
   status = CDGetStatus();

   if (status == 0 || status == 1)
   {
      testspassed++;

      if (CDReadToc(cdTOC) != (0xCC * 2))
      {
         printf("CDReadToc error: return value was not the correct size\n");
      }
      else testspassed++;

      // Make sure TOC matches Sega boot CD
      if (memcmp(cdTOC, bootcdTOC, 0xCC * 2) != 0)
      {
         printf("CDReadToc error: TOC data doesn't match Sega Boot CD's. If you did indeed test with the Sega Boot CD, check your code again.\n");
      }
      else testspassed++;

      // Read sector 0
      if (CDReadSectorFAD(150, cdbuffer) != true)
      {
         printf("CDReadSectorFAD error: Reading of LBA 0(FAD 150) returned false\n");
      }
      else testspassed++;

      // Check cdbuffer to make sure contents are correct
      if (memcmp(syncheader, cdbuffer, 12) != 0)
      {
         printf("CDReadSectorFAD error: LBA 0(FAD 150) read is missing sync header\n");
      }
      else testspassed++;

      // look for "SEGA SEGASATURN"
      if (memcmp("SEGA SEGASATURN", cdbuffer+0x10, 15) != 0)
      {
         printf("CDReadSectorFAD error: LBA 0(FAD 150)'s sector contents were not as expected\n");
      }
      else testspassed++;

      // Read sector 16(I figured it makes a good random test sector)
      if (CDReadSectorFAD(166, cdbuffer) != true)
      {
         printf("CDReadSectorFAD error: Reading of LBA 16(FAD 166) returned false\n");
      }
      else testspassed++;

      // Check cdbuffer to make sure contents are correct
      if (memcmp(syncheader, cdbuffer, 12) != 0)
      {
         printf("CDReadSectorFAD error: LBA 16(FAD 166) read is missing sync header\n");
      }
      else testspassed++;

      // look for "CD001"
      if (memcmp("CD001", cdbuffer+0x11, 5) != 0)
      {
         printf("CDReadSectorFAD error: LBA 16(FAD 166)'s sector contents were not as expected\n");
      }
      else testspassed++;
   }
   else
   {
      printf("CDGetStatus error: Can't continue the rest of the test - There's either no CD present or the tray is open\n");
      exit(1);
   }
}

