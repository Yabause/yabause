/*  Copyright 2004-2005 Lucas Newman
    Copyright 2004-2005 Theo Berkau

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
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMediaBSDClient.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#include <CoreFoundation/CoreFoundation.h>

#include "cd.hh"


MacOSXCDDrive::MacOSXCDDrive(const char *cdrom_name) {
	cdTOC = NULL;
	init(cdrom_name);
}

MacOSXCDDrive::~MacOSXCDDrive() {
	deinit();
}

const char *MacOSXCDDrive::deviceName() { return "MacOSX CD drive"; }

int MacOSXCDDrive::init(const char *cdrom_name) {
	io_iterator_t mediaIterator;
	mach_port_t masterPort;
	CFMutableDictionaryRef classesToMatch;
	
	if (IOMasterPort(MACH_PORT_NULL, &masterPort) != 0)
		printf("IOMasterPort Failed!\n");
		
	classesToMatch = IOServiceMatching("IOCDMedia"); 

	if (classesToMatch == NULL)
		printf("IOServiceMatching returned a NULL dictionary.\n");
	else
		CFDictionarySetValue(classesToMatch, CFSTR(kIOMediaEjectableKey), kCFBooleanTrue);
	
	if (IOServiceGetMatchingServices(masterPort, classesToMatch, &mediaIterator) != 0)
		printf("IOServiceGetMatchingServices Failed!\n");
		
	getCDPath(mediaIterator, cdPath, sizeof(cdPath));
	if ((hCDROM = open(cdPath, O_RDONLY)) == -1)
		return -1;
	printf("CDInit OK\n");
	return 0;
}

int MacOSXCDDrive::deinit() {
	if (hCDROM != NULL) {
		close(hCDROM);
		free((void*)cdTOC);
		printf("CDDeInit OK\n");
	}
	return 0;
}


int MacOSXCDDrive::getCDPath( io_iterator_t mediaIterator, char *devPath, CFIndex maxPathSize ) {
    io_object_t nextMedia;
    CFMutableDictionaryRef properties;
    CFDataRef data;
    *devPath = '\0';
    
    nextMedia = IOIteratorNext( mediaIterator );
    if (nextMedia) {
	CFTypeRef devPathAsCFString;
        devPathAsCFString = IORegistryEntryCreateCFProperty(nextMedia, CFSTR(kIOBSDNameKey), kCFAllocatorDefault, kNilOptions);
        if (devPathAsCFString) {
            size_t devPathLength;
            strcpy(devPath, "/dev/");
			
            devPathLength = strlen(devPath);
            CFStringGetCString((CFStringRef)devPathAsCFString, devPath + devPathLength, maxPathSize - devPathLength, kCFStringEncodingASCII);
            CFRelease(devPathAsCFString);
			
			if((IORegistryEntryCreateCFProperties(nextMedia, &properties, kCFAllocatorDefault, kNilOptions)) != 0)
				perror("IORegistryEntryCreateCFProperties Failed!");
			
			if((data = (CFDataRef) CFDictionaryGetValue(properties, CFSTR("TOC"))) != NULL) {
				CFRange range;
				CFIndex buf_len;

				buf_len = CFDataGetLength( data ) + 1;
				range = CFRangeMake( 0, buf_len );

				if((cdTOC = (CDTOC *)malloc( buf_len )) != NULL)
				{
					CFDataGetBytes( data, range, (u_char *)cdTOC );
				}
			}
			else {
				perror("CFDictionaryGetValue Failed!");
			}
			CFRelease(properties);
        }
        IOObjectRelease(nextMedia);
    }
    return 0;
}

long MacOSXCDDrive::readTOC(unsigned long *TOC) {
  	int add150 = 150, tracks = 0;
	u_char track;
	int i, fad = 0; 
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
	TOC[100] = pTrackDescriptors[1].adr << 24 | (tracks + 1) << 16;
	/* Leadout */
	TOC[101] = pTrackDescriptors[2].adr << 24 | fad;
			
	//hhhack for single track discs
	TOC[1] = pTrackDescriptors[2].adr << 24 | fad + add150;
	
	return (0xCC * 2);
}

int MacOSXCDDrive::getStatus(void) {
	// 0 - CD Present, disc spinning
	// 1 - CD Present, disc not spinning
	// 2 - CD not present
	// 3 - Tray open
	// see ../windows/cd.cc for more info

	//Return that disc is present and spinning.  Fix this eventually.
	return 0;
}

bool MacOSXCDDrive::readSectorFAD(unsigned long FAD, void *buffer) {
	int blockSize;
	
	if (hCDROM != -1) {
		ioctl(hCDROM, DKIOCGETBLOCKSIZE, &blockSize);
		if (pread(hCDROM, buffer, blockSize, (FAD - 150) * blockSize))
			return true;
	}
	return false;
}

