/*  Copyright 2004 Lucas Newman
	Copyright 2004 Theo Berkau

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
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOCDTypes.h>
#include <CoreFoundation/CoreFoundation.h>

char cdPath[1024];
int hCDROM = -1;
CDTOC *cdTOC = NULL;

int getCDPath( io_iterator_t mediaIterator, char *devPath, CFIndex maxPathSize )
{
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
            strcpy(devPath, "/dev/r");
            devPathLength = strlen(devPath);
            CFStringGetCString(devPathAsCFString,  devPath + devPathLength, maxPathSize - devPathLength, kCFStringEncodingASCII);
            CFRelease(devPathAsCFString);
			
			if((IORegistryEntryCreateCFProperties(nextMedia, &properties, kCFAllocatorDefault, kNilOptions)) != 0)
				perror("IORegistryEntryCreateCFProperties Failed!");
			
			if((data = (CFDataRef) CFDictionaryGetValue(properties, CFSTR(kIOCDMediaTOCKey))) != NULL) {
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
}

int CDInit(char *cdrom_name)
{
	io_iterator_t mediaIterator;
	mach_port_t masterPort;
	CFMutableDictionaryRef classesToMatch;
	
	if (IOMasterPort(MACH_PORT_NULL, &masterPort) != 0)
		printf("IOMasterPort Failed!\n");
		
	classesToMatch = IOServiceMatching(kIOCDMediaClass); 

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

int CDDeInit()
{
	if (hCDROM != -1) {
		close(hCDROM);
		free((void*)cdTOC);
		printf("CDDeInit OK\n");
	}
	return 0;
}

bool CDIsCDPresent()
{
	if (cdPath[0] != '\0')
		return 1;
	else
		return 0;
}

long CDReadToc(unsigned long *TOC)
{
	int tracks = 0;
	u_char track;
	int i, lba = 0; 
	CDTOCDescriptor *pTrackDescriptors;
	pTrackDescriptors = cdTOC->descriptors;
	
	memset(TOC, 0xFF, 0xCC * 2);
	
	/* First three entries are first, last, and lead out - track one starts at [3] */
	TOC[0] = (pTrackDescriptors[3].control << 28 |
		pTrackDescriptors[3].adr << 24 | CDConvertMSFToLBA(pTrackDescriptors[3].p));
		
	/* Convert TOC to Saturn format */
	for( i = 4; i < CDTOCGetDescriptorCount(cdTOC); i++ ) {
        track = pTrackDescriptors[i].point;
		lba = CDConvertMSFToLBA(pTrackDescriptors[i].p);
		
        if ((track > 99) || (track < 1))
            continue;
		TOC[i-3] = (pTrackDescriptors[i].control << 28 | 
			pTrackDescriptors[i].adr << 24 | lba);
		tracks++;
    }
	
	/* Do first, last, and lead out */
	TOC[99] = (pTrackDescriptors[0].control << 28 | 
			pTrackDescriptors[0].adr << 24 | 1 << 16);
	TOC[100] = (pTrackDescriptors[1].control << 28 | 
			pTrackDescriptors[1].adr << 24 | tracks << 16);
	TOC[101] = (pTrackDescriptors[2].control << 28 | 
			pTrackDescriptors[2].adr << 24 | CDConvertMSFToLBA(pTrackDescriptors[2].p));
	
	return (0xCC * 2);
}

unsigned long CDReadSector(unsigned long lba, unsigned long size, void *buffer)
{
    int blockSize;
	
	if (hCDROM != -1) {
		ioctl(hCDROM, DKIOCGETBLOCKSIZE, &blockSize);
		return pread(hCDROM, buffer, blockSize, lba * blockSize);
	}
    return 0;
}