#ifndef TIMER_HH
#define TIMER_HH

class SuperH;

class Timer {
private:
  static SuperH *sh;
public:
  static void initSuperH(SuperH *);
  static void wait(unsigned long);
  void waitHBlankIN(void);
  void waitHBlankOUT(void);
  void waitVBlankIN(void);
  void waitVBlankOUT(void);

  template<class C, void (C::*)(void), int I> static int call(void * c);
};

template<class C, void (C::*function)(void), int I> int Timer::call(void * c) {
	wait(I);
	(((C *) c)->*function)();
}

#endif
