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

unsigned short buttonbits = 0xFFFF;

void keyDown(int key)
{
  switch (key) {
	case SDLK_f:
		buttonbits &= 0x7FFF;
		cerr << "Right" << endl;
		break;
	case SDLK_s:
		buttonbits &= 0xBFFF;
		cerr << "Left" << endl;
		break;
	case SDLK_d:
		buttonbits &= 0xDFFF;
		cerr << "Down" << endl;
		break;
	case SDLK_e:
		buttonbits &= 0xEFFF;
		cerr << "Up" << endl;
		break;
	case SDLK_j:
		buttonbits &= 0xF7FF;
		cerr << "Start" << endl;
		break;
	case SDLK_k:
		buttonbits &= 0xFBFF;
		cerr << "A" << endl;
		break;
	case SDLK_m:
		buttonbits &= 0xFDFF;
		cerr << "C" << endl;
		break;
	case SDLK_l:
		buttonbits &= 0xFEFF;
		cerr << "B" << endl;
		break;
	case SDLK_z:
		buttonbits &= 0xFF7F;
		cerr << "Right Trigger" << endl;
		break;
	case SDLK_u:
		buttonbits &= 0xFFBF;
		cerr << "X" << endl;
		break;
	case SDLK_i:
		buttonbits &= 0xFFDF;
		cerr << "Y" << endl;
		break;
	case SDLK_o:
		buttonbits &= 0xFFEF;
		cerr << "Z" << endl;
		break;
	case SDLK_x:
		buttonbits &= 0xFFF7;
		cerr << "LTrig" << endl;
		break;
	default:
		break;
  }
}

void keyUp(int key)
{
  switch (key) {
	case SDLK_f:
		buttonbits |= ~0x7FFF;
		break;
	case SDLK_s:
		buttonbits |= ~0xBFFF;
		break;
	case SDLK_d:
		buttonbits |= ~0xDFFF;
		break;
	case SDLK_e:
		buttonbits |= ~0xEFFF;
		break;
	case SDLK_j:
		buttonbits |= ~0xF7FF;
		break;
	case SDLK_k:
		buttonbits |= ~0xFBFF;
		break;
	case SDLK_m:
		buttonbits |= ~0xFDFF;
		break;
	case SDLK_l:
		buttonbits |= ~0xFEFF;
		break;
	case SDLK_z:
		buttonbits |= ~0xFF7F;
		break;
	case SDLK_u:
		buttonbits |= ~0xFFBF;
		break;
	case SDLK_i:
		buttonbits |= ~0xFFDF;
		break;
	case SDLK_o:
		buttonbits |= ~0xFFEF;
		break;
	case SDLK_x:
		buttonbits |= ~0xFFF7;
		break;
	default:
		break;
  }
}

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
		case SDLK_q:
			stop = true;
			break;
		case SDLK_p:
			cerr << "pause" << endl;
			mem->getMasterSH()->pause();
			break;
		case SDLK_r:
			cerr << "run" << endl;
			mem->getMasterSH()->run();
			break;
		default:
			keyDown(event.key.keysym.sym);
			break;
	  }
	  break;
	case SDL_KEYUP:
	  switch(event.key.keysym.sym) {
		case SDLK_q:
		case SDLK_p:
		case SDLK_r:
			break;
		default:
			keyUp(event.key.keysym.sym);
			break;
	  }
	  break;
	default:
		break;
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
