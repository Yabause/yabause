/*  Copyright 2004 Theo Berkau

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
int hCDROM;

int GetBSDPath( io_iterator_t mediaIterator, char *bsdPath, CFIndex maxPathSize )
{
    io_object_t nextMedia;
    *bsdPath = '\0';
    
    nextMedia = IOIteratorNext( mediaIterator );
    if (nextMedia) {
        CFTypeRef bsdPathAsCFString;
        bsdPathAsCFString = IORegistryEntryCreateCFProperty(nextMedia, CFSTR(kIOBSDNameKey), kCFAllocatorDefault, 0);
        if (bsdPathAsCFString) {
            size_t devPathLength;
            strcpy(bsdPath, "/dev/r");
            devPathLength = strlen(bsdPath);
            CFStringGetCString(bsdPathAsCFString,  bsdPath + devPathLength, maxPathSize - devPathLength, kCFStringEncodingASCII);
            CFRelease(bsdPathAsCFString);
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
		
	GetBSDPath(mediaIterator, cdPath, sizeof(cdPath));
	
	if ((hCDROM = open(cdPath, O_RDONLY | O_NONBLOCK)) == -1)
		return -1;
	printf("CDInit OK\n");
	return 0;
}

int CDDeInit()
{
	if (hCDROM != -1) {
		close(hCDROM);
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
   return 0;
}

unsigned long CDReadSector(unsigned long lba, unsigned long size, void *buffer)
{
   return 0;
}
