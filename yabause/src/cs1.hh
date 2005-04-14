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

#ifndef CS1_HH
#define CS1_HH

#include "memory.hh"

class Cs1 : public Dummy {
  int cartid;
  int carttype;
  char *cartfile;
  Memory *sramarea;
public:
  Cs1(char *file, int type);
  ~Cs1(void);

  unsigned char getByte(unsigned long);
  unsigned short getWord(unsigned long);
  unsigned long getLong(unsigned long);
  void setByte(unsigned long addr, unsigned char val);
  void setWord(unsigned long addr, unsigned short val);
  void setLong(unsigned long addr, unsigned long val);

  int saveState(FILE *fp);
  int loadState(FILE *fp, int version, int size);
};

#endif
