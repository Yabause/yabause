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

#ifndef CDBASE_HH
#define CDBASE_HH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class CDInterface {
public:
	CDInterface() {}
	CDInterface(const char *) {}
	virtual ~CDInterface() {} 
	virtual long readTOC(unsigned long *TOC) = 0;
	virtual int getStatus() = 0;
	virtual bool readSectorFAD(unsigned long FAD, void *buffer) = 0;
	
	virtual const char *deviceName() = 0;
private:
	virtual int init(const char *) = 0;
	virtual int deinit() = 0;
};

class DummyCDDrive : public CDInterface {
public:
	DummyCDDrive();
	~DummyCDDrive();
	long readTOC(unsigned long *TOC);
	int getStatus();
	bool readSectorFAD(unsigned long FAD, void *buffer);

	const char *deviceName();

private:
	int init(const char *);
	int deinit();
};

class ISOCDDrive : public CDInterface {
public:
	ISOCDDrive(const char *iso);
	virtual ~ISOCDDrive();

	long readTOC(unsigned long *TOC);
	int getStatus();
	bool readSectorFAD(unsigned long FAD, void *buffer);
	const char *deviceName();
	
private:
	int init(const char *iso);
	int deinit();
	
	FILE *_isoFile;
	int _fileSize;
};

#endif
