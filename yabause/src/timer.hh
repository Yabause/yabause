#ifndef TIMER_HH
#define TIMER_HH

class SuperH;

class Timer {
private:
  static SuperH *sh;
public:
  static void initSuperH(SuperH *);
  void wait(unsigned long);
  void waitHBlankIN(void);
  void waitHBlankOUT(void);
  void waitVBlankIN(void);
  void waitVBlankOUT(void);
};

#endif
