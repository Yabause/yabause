/*  Copyright 2004 Guillaume Duhamel
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

#ifndef LINUX_CD_HH
#define LINUX_CD_HH

#include "../cdbase.hh"

class LinuxCDDrive : public CDInterface {
public:
	LinuxCDDrive(const char *cdrom_name);
	virtual ~LinuxCDDrive();

	long readTOC(unsigned long *TOC);
	int getStatus();
	bool readSectorFAD(unsigned long FAD, void *buffer);
	const char *deviceName();
	
private:
	int init(const char *iso);
	int deinit();
	
	int hCDROM;
};

#endif
