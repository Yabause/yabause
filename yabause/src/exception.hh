/*  Copyright 2003 Guillaume Duhamel

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

#ifndef EXCEPTION_HH
#define EXCEPTION_HH

#include "saturn.hh"

class Exception {
protected:
	char exc[255];
public:
#ifndef _arch_dreamcast
	friend ostream& operator<<(ostream&, const Exception&);
#endif
};

class FileNotFound : public Exception {
public:
	FileNotFound(const char *);
};

class BadMemoryAccess : public Exception {
private:
	unsigned long addr;
public:
	BadMemoryAccess(unsigned long);
	unsigned long getAddress(void) const;
};

class UnimplementedOpcode : public Exception {
public:
	UnimplementedOpcode(const char *);
};

class IllegalOpcode : public Exception {
private:
	unsigned short _opcode;
	unsigned long _address;
public:
	IllegalOpcode(unsigned short, unsigned long);
	unsigned short opcode(void);
	unsigned long address(void);
};

#endif
