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

#ifndef SATURN_HH
#define SATURN_HH

#ifndef _arch_dreamcast
using namespace std;

#include <iostream>
#include <iomanip>

#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <fstream>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

template<unsigned long A, unsigned long B> unsigned long div(void) {
  return A / B;
}

template<unsigned long A> unsigned long div(unsigned long b) {
  return b / A;
}

template<unsigned long A, unsigned long B>
class Div {
private:
  unsigned long q;
  unsigned long r;
  unsigned long p;
  unsigned long i;
public:
  Div(void);
  unsigned long nextValue(void);
};

template<unsigned long A, unsigned long B>
Div<A, B>::Div(void) {
  q = A / B;
  r = A % B;
  p = 0;
  i = 0;
}

template<unsigned long A, unsigned long B>
unsigned long Div<A, B>::nextValue(void) {
  unsigned long d = 0;
  if (((((long long) p) * B) / r) == i) {
    p++;
    d = 1;
  }
  i++;
  if (i == B) i = 0;
  return q + d;
}

#endif
