#include "timer.hh"
#include "superh.hh"

SuperH *Timer::sh;

void Timer::initSuperH(SuperH *superh) {
  sh = superh;
}

void Timer::wait(unsigned long nbusec) {
  sh->microsleep(nbusec);
}

void Timer::waitHBlankIN(void) {
  sh->waitHBlankIN();
}

void Timer::waitHBlankOUT(void) {
  sh->waitHBlankOUT();
}

void Timer::waitVBlankIN(void) {
  sh->waitVBlankIN();
}

void Timer::waitVBlankOUT(void) {
  sh->waitVBlankOUT();
}
