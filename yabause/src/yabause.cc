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

#include "superh.hh"
#include "smpc.hh"
#include "cs2.hh"
#include "intc.hh"
#include "vdp1.hh"
#include "vdp2.hh"
#if HAVE_LIBCURSES
#include "monitor.hh"
#endif
#include <sys/types.h>
#include "cmdline.h"

int main(int argc, char **argv) {
#if DEBUG
  cerr << hex;
#endif

#ifndef _arch_dreamcast
  gengetopt_args_info args_info;

  if (cmdline_parser(argc, argv, &args_info) != 0) {
    cmdline_parser_print_help();
    return 1;
  }
#endif

  SDL_Init(SDL_INIT_EVENTTHREAD);

#ifndef _arch_dreamcast
  SaturnMemory *mem = new SaturnMemory(args_info.bios_arg, args_info.bin_arg);
#else
  SaturnMemory *mem = new SaturnMemory("/cd/saturn.bin", NULL);
#endif

  bool stop = false;

  if (args_info.debug_given) {
#if HAVE_LIBCURSES
    monitor(mem->getMasterSH());
#endif
  }
  else {
    SDL_Event event;
    while (!stop) {
      if (SDL_PollEvent(&event)) {
	switch(event.type) {
	case SDL_KEYDOWN:
	  switch(event.key.keysym.sym) {
	  case SDLK_q: stop = true;
		       break;
	  case SDLK_p: cerr << "pause" << endl;
		       mem->getMasterSH()->pause();
		       break;
	  case SDLK_r: cerr << "run" << endl;
		       mem->getMasterSH()->run();
		       break;
	  }
	}
      }
      else {
	SDL_Delay(1);
      }
    }
  }

#if DEBUG
  cerr << "stopping yabause\n";
#endif

  delete(mem);

  SDL_Quit();
}
