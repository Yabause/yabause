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

#include "exception.hh"

#ifndef _arch_dreamcast
/* No exceptions on the dreamcast.... */
ostream& operator<<(ostream& os, const Exception& e) {
	return os << e.exc;
}

FileNotFound::FileNotFound(const char *filename) {
	sprintf(exc, "File not found: %s", filename);
}

BadMemoryAccess::BadMemoryAccess(unsigned long ul) {
  addr = ul;
  sprintf(exc, "Bad memory access: %8X", ul);
}

unsigned long BadMemoryAccess::getAddress(void) const {
  return addr;
}

UnimplementedOpcode::UnimplementedOpcode(const char *opcode) {
	strcpy(exc, opcode);
}

IllegalOpcode::IllegalOpcode(char *cpu_name, unsigned short o, unsigned long a) {
  _opcode = o;
  _address = a;
  sprintf(exc, "%s Illegal opcode: %8X @ %8X", cpu_name, o, a);
}

unsigned short IllegalOpcode::opcode(void) {
  return _opcode;
}

unsigned long IllegalOpcode::address(void) {
	  return _address;
}
#endif
