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

#ifndef CS0_HH
#define CS0_HH

#include "memory.hh"

#define CART_NONE               0
#define CART_PAR                1
#define CART_BACKUPRAM4MBIT     2
#define CART_BACKUPRAM8MBIT     3
#define CART_BACKUPRAM16MBIT    4
#define CART_BACKUPRAM32MBIT    5
#define CART_DRAM8MBIT          6
#define CART_DRAM32MBIT         7
#define CART_NETLINK            8

class Cs0 : public Dummy {
private:
  int carttype;
  Memory *biosarea;
  Memory *dramarea;
public:
  Cs0(char *file, int type);
  ~Cs0(void);
  void setByte(unsigned long addr, unsigned char val);
  unsigned char getByte(unsigned long addr);
  void setWord(unsigned long addr, unsigned short val);
  unsigned short getWord(unsigned long addr);
  void setLong(unsigned long addr, unsigned long val);
  unsigned long getLong(unsigned long addr);

  int saveState(FILE *fp);
  int loadState(FILE *fp, int version, int size);
};

#endif
