/*  Copyright 2004-2005 Lucas Newman
    Copyright 2004-2005 Theo Berkau
    Copyright 2005 Weston Yager
    Copyright 2006 Guillaume Duhamel

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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <paths.h>
#include <sys/param.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMediaBSDClient.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IOCDMedia.h>
#include <CoreFoundation/CoreFoundation.h>

#include "cdbase.h"

#include "macosx-new/cd.h"

CDInterface ArchCD = {
CDCORE_ARCH,
"MacOSX CD Drive",
MacOSXCDInit,
MacOSXCDDeInit,
MacOSXCDGetStatus,
MacOSXCDReadTOC,
MacOSXCDReadSectorFAD
};

int hCDROM;
char *BSDDeviceName = NULL;

int MacOSXCDInit(const char *cdrom_name)
{	
	if ((hCDROM = open(cdrom_name, O_RDONLY)) == -1)
	{
			return -1;
	}
	
	BSDDeviceName = malloc(strlen(cdrom_name)+1);
	strcpy(BSDDeviceName, cdrom_name);
	
	return 0;
}

int MacOSXCDDeInit() 
{
	if (hCDROM != -1) 
	{
		free(BSDDeviceName);
		BSDDeviceName = NULL;
		close(hCDROM);
	}
	
	return 0;
}

CDTOC *GetTOCFromCDPath(const char *BSDDeviceName)
{
	CFMutableDictionaryRef  classesToMatch;
	io_iterator_t mediaIterator;
	io_object_t media;
	CDTOC * TOC;

	classesToMatch = IOServiceMatching(kIOCDMediaClass); 
	CFDictionarySetValue(classesToMatch, CFSTR(kIOMediaEjectableKey),
		kCFBooleanTrue); 
	IOServiceGetMatchingServices(kIOMasterPortDefault,
		classesToMatch, &mediaIterator);    

	media = IOIteratorNext(mediaIterator);
	
	if(media)
	{
		CFDataRef TOCData = IORegistryEntryCreateCFProperty(media, CFSTR(kIOCDMediaTOCKey), kCFAllocatorDefault, 0);
		TOC = malloc(CFDataGetLength(TOCData));
		CFDataGetBytes(TOCData,CFRangeMake(0,CFDataGetLength(TOCData)),(UInt8 *)TOC);
		CFRelease(TOCData);
		IOObjectRelease(media);
	}
	
	return TOC;
}

long MacOSXCDReadTOC(unsigned long *TOC) 
{
  	int add150 = 150, tracks = 0;
	u_char track;
	int i, fad = 0;
	CDTOC *cdTOC = GetTOCFromCDPath(BSDDeviceName);
	CDTOCDescriptor *pTrackDescriptors;
	pTrackDescriptors = cdTOC->descriptors;

	memset(TOC, 0xFF, 0xCC * 2);

	/* Convert TOC to Saturn format */
	for( i = 3; i < CDTOCGetDescriptorCount(cdTOC); i++ ) {
        	track = pTrackDescriptors[i].point;
		fad = CDConvertMSFToLBA(pTrackDescriptors[i].p) + add150;
		if ((track > 99) || (track < 1))
			continue;
		TOC[i-3] = (pTrackDescriptors[i].control << 28 | pTrackDescriptors[i].adr << 24 | fad);
		tracks++;
	}

	/* First */
	TOC[99] = pTrackDescriptors[0].control << 28 | pTrackDescriptors[0].adr << 24 | 1 << 16;
	/* Last */
	TOC[100] = pTrackDescriptors[1].control << 28 | pTrackDescriptors[1].adr << 24 | tracks << 16;
	/* Leadout */
	TOC[101] = pTrackDescriptors[2].control << 28 | pTrackDescriptors[2].adr << 24 | CDConvertMSFToLBA(pTrackDescriptors[2].p) + add150;

	{
		int i;
		for(i = 0;i < 102;i++) {
			printf("%x\n", TOC[i]);
		}
	}
			
	//free(cdTOC); Looks like this is not need, will look into that.
	return (0xCC * 2);
}

int MacOSXCDGetStatus(void) 
{
	// 0 - CD Present, disc spinning
	// 1 - CD Present, disc not spinning
	// 2 - CD not present
	// 3 - Tray open
	// see ../windows/cd.cc for more info

	//Return that disc is present and spinning.  2 and 3 can't happen in the mac port, i don't understand what "not spinning" is supposed to say
	return 0;
}

int MacOSXCDReadSectorFAD(unsigned long FAD, void *buffer) 
{
	//int blockSize;
	int blockSize = 2352;
	
	if (hCDROM != -1) 
	{
		//ioctl(hCDROM, DKIOCGETBLOCKSIZE, &blockSize);
		if (pread(hCDROM, buffer, blockSize, (FAD - 150) * blockSize))
			return true;
	}
	
	return false;
}

