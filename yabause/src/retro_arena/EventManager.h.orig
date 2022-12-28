#pragma once

#if defined(ARCH_IS_LINUX)
#include <unistd.h>
#include <sys/resource.h>
#include <errno.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL_opengles2.h>
#elif defined(_WINDOWS)
#include <direct.h>
#include <SDL.h>
#endif

#include <map>
#include <functional>

using std::string;

struct Event {
  uint32_t type;
  std::function<void( int code, void * data1, void * data2 )> func;
};


class EventManager{

protected:
  static EventManager * instance;

public:
  static EventManager * getInstance() {
    if (instance == nullptr) {
      instance = new EventManager();
    }
    return instance;
  }

protected:
    std::map< uint32_t, string >  etos;
    std::map< string, Event >  m;

public:
    void setEvent( string ev ,  const std::function<void(int code, void * data1, void * data2)> &callback ){

      Event event;
      event.type = SDL_RegisterEvents(1);
      event.func = callback;
      m.insert_or_assign( ev, event);
      etos.insert_or_assign( event.type, ev);
    }

    void procEvent(uint32_t ev, int code = 0, void * data = nullptr ) {
      if (etos.find(ev) != etos.end()) {
        m[etos[ev]].func(code, data, nullptr);
      }
    }

    void postEvent(string ev) {
      if (m.find(ev) != m.end()) {
        SDL_Event event = {};
        event.type = m[ev].type;
        event.user.code = 0;
        event.user.data1 = 0;
        event.user.data2 = 0;
        SDL_PushEvent(&event);
      }
    }

    void postEvent(string ev,void * data) {
      if (m.find(ev) != m.end()) {
        SDL_Event event = {};
        event.type = m[ev].type;
        event.user.code = 0;
        event.user.data1 = data;
        event.user.data2 = 0;
        SDL_PushEvent(&event);
      }
    }

    void postEvent(string ev, uint32_t code) {
      if (m.find(ev) != m.end()) {
        SDL_Event event = {};
        event.type = m[ev].type;
        event.user.code = code;
        event.user.data1 = nullptr;
        event.user.data2 = nullptr;
        SDL_PushEvent(&event);
      }
    }

};