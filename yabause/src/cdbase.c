/*  Copyright 2004-2005 Theo Berkau
    Copyright 2005 Joost Peters
    
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

#include <string.h>
#include <assert.h>
#include "cdbase.h"

//////////////////////////////////////////////////////////////////////////////

// Contains the Dummy and ISO CD Interfaces

CDInterface DummyCD = {
CDCORE_DUMMY,
"Dummy CD Drive",
DummyCDInit,
DummyCDDeInit,
DummyCDGetStatus,
DummyCDReadTOC,
DummyCDReadSectorFAD
};

CDInterface ISOCD = {
CDCORE_ISO,
"ISO-File Virtual Drive",
ISOCDInit,
ISOCDDeInit,
ISOCDGetStatus,
ISOCDReadTOC,
ISOCDReadSectorFAD
};

//////////////////////////////////////////////////////////////////////////////
// Dummy Interface
//////////////////////////////////////////////////////////////////////////////

int DummyCDInit(const char *cdrom_name)
{
	// Initialization function. cdrom_name can be whatever you want it to be.
	// Obviously with some ports(e.g. the dreamcast port) you probably won't
	// even use it.
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int DummyCDDeInit()
{
	// Cleanup function. Enough said.
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int DummyCDGetStatus()
{
	// This function is called periodically to see what the status of the
	// drive is.
	//
	// Should return one of the following values:
	// 0 - CD Present, disc spinning
	// 1 - CD Present, disc not spinning
	// 2 - CD not present
	// 3 - Tray open
	//
	// If you really don't want to bother too much with this function, just
	// return status 0. Though it is kind of nice when the bios's cd player,
	// etc. recognizes when you've ejected the tray and popped in another disc.

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

s32 DummyCDReadTOC(u32 *TOC)
{
	// The format of TOC is as follows:
	// TOC[0] - TOC[98] are meant for tracks 1-99. Each entry has the following
	// format:
	// bits 0 - 23: track FAD address
	// bits 24 - 27: track addr
	// bits 28 - 31: track ctrl
	//
	// Any Unused tracks should be set to 0xFFFFFFFF
	//
	// TOC[99] - Point A0 information 
	// Uses the following format:
	// bits 0 - 7: PFRAME(should always be 0)
	// bits 7 - 15: PSEC(Program area format: 0x00 - CDDA or CDROM, 0x10 - CDI, 0x20 - CDROM-XA)
	// bits 16 - 23: PMIN(first track's number)
	// bits 24 - 27: first track's addr
	// bits 28 - 31: first track's ctrl
	//
	// TOC[100] - Point A1 information
	// Uses the following format:
	// bits 0 - 7: PFRAME(should always be 0)
	// bits 7 - 15: PSEC(should always be 0)
	// bits 16 - 23: PMIN(last track's number)
	// bits 24 - 27: last track's addr
	// bits 28 - 31: last track's ctrl
	//
	// TOC[101] - Point A2 information
	// Uses the following format:
	// bits 0 - 23: leadout FAD address
	// bits 24 - 27: leadout's addr
	// bits 28 - 31: leadout's ctrl
	//
	// Special Note: To convert from LBA/LSN to FAD, add 150.

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int DummyCDReadSectorFAD(u32 FAD, void * buffer)
{
	// This function is supposed to read exactly 1 -RAW- 2352-byte sector at
	// the specified FAD address to buffer. Should return true if successful,
	// false if there was an error.
	//
	// Special Note: To convert from FAD to LBA/LSN, minus 150.
	//
	// The whole process needed to be changed since I need more control over
	// sector detection, etc. Not to mention it means less work for the porter
	// since they only have to implement raw sector reading as opposed to
	// implementing mode 1, mode 2 form1/form2, -and- raw sector reading.

	return 1;
}

//////////////////////////////////////////////////////////////////////////////
// ISO Interface
//////////////////////////////////////////////////////////////////////////////

static const s8 syncHdr[12] = { 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };
static FILE *isofile=NULL;
static int isofilesize=0;
static int bytesPerSector = 0;

//////////////////////////////////////////////////////////////////////////////

int ISOCDInit(const char * iso) {
        if (!(isofile = fopen(iso, "rb")))
                return -1;

        fseek(isofile, 0, SEEK_END);
        isofilesize = ftell(isofile);
	
	if (0 == (isofilesize % 2048)) {
		bytesPerSector = 2048;
	} else if (0 == (isofilesize % 2352)) {
		bytesPerSector = 2352;
	} else {
		printf("Unsupported CD image!\n");
		return -1;
	}
	
        return 0;
}

//////////////////////////////////////////////////////////////////////////////

int ISOCDDeInit() {
        fclose(isofile);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int ISOCDGetStatus() {
        return isofile != NULL ? 0 : 2;
}

//////////////////////////////////////////////////////////////////////////////

s32 ISOCDReadTOC(u32 * TOC) {
        if (isofile) {
		memset(TOC, 0xFF, 0xCC * 2);

		TOC[99] = 0x41010000;
		TOC[100] = 0x01010000;
                TOC[101] = (0x41 << 24) | (isofilesize / bytesPerSector);       //this isn't fully correct, but it does the job for now.
      
		return (0xCC * 2);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

int ISOCDReadSectorFAD(u32 FAD, void *buffer) {
        int sector = FAD - 150;
	
        assert(isofile);
	
        memset(buffer, 0, 2352);
        
	if ((sector * bytesPerSector) >= isofilesize) {
		printf("Warning: Trying to read beyond end of CD image! (sector: %d)\n", sector);
		return 0;
	}
	
	fseek(isofile, sector * bytesPerSector, SEEK_SET);
	
	if (2048 == bytesPerSector) {
		memcpy(buffer, syncHdr, 12);
		fread((char *)buffer + 0x10, bytesPerSector, 1, isofile);
	} else { //2352
		fread(buffer, bytesPerSector, 1, isofile);
	}
	
	return 1;
}

//////////////////////////////////////////////////////////////////////////////

