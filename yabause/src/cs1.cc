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

#include "cs1.hh"

Cs1::Cs1(void) : Dummy(0xFFFFFF) {} //Memory(0x1000000) {}

unsigned char Cs1::getByte(unsigned long addr) { // merci satonem !!!
	return 0xFF;
}

unsigned short Cs1::getWord(unsigned long addr) { // merci satonem !!!
	return 0xFFFF;
}

unsigned long Cs1::getLong(unsigned long addr) { // merci satonem !!!
	return 0xFFFFFFFF;
}
