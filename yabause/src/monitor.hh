#if HAVE_LIBCURSES
#ifndef MONITOR_HH
#define MONITOR_HH

#include "superh.hh"

class Frame {
private:
  unsigned int _left;
  unsigned int _width;
  unsigned int _up;
  unsigned int _height;
public:
  Frame(unsigned int, unsigned int, unsigned int, unsigned int);
  void draw(void);
};

void monitor(SuperH *);

#endif
#endif
