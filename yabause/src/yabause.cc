/*  Copyright 2003 Guillaume Duhamel
    Copyright 2004 Theo Berkau

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
#include <sys/types.h>
#include "yui.hh"

unsigned short buttonbits = 0xFFFF;

void keyDown(int key)
{
  switch (key) {
        case SDLK_RIGHT:
		buttonbits &= 0x7FFF;
#ifdef DEBUG
		cerr << "Right" << endl;
#endif
		break;
        case SDLK_LEFT:
		buttonbits &= 0xBFFF;
#ifdef DEBUG
		cerr << "Left" << endl;
#endif
		break;
        case SDLK_DOWN:
		buttonbits &= 0xDFFF;
#ifdef DEBUG
		cerr << "Down" << endl;
#endif
		break;
        case SDLK_UP:
		buttonbits &= 0xEFFF;
#ifdef DEBUG
		cerr << "Up" << endl;
#endif
		break;
	case SDLK_j:
		buttonbits &= 0xF7FF;
#ifdef DEBUG
		cerr << "Start" << endl;
#endif
		break;
	case SDLK_k:
		buttonbits &= 0xFBFF;
#ifdef DEBUG
		cerr << "A" << endl;
#endif
		break;
	case SDLK_m:
		buttonbits &= 0xFDFF;
#ifdef DEBUG
		cerr << "C" << endl;
#endif
		break;
	case SDLK_l:
		buttonbits &= 0xFEFF;
#ifdef DEBUG
		cerr << "B" << endl;
#endif
		break;
	case SDLK_z:
		buttonbits &= 0xFF7F;
#ifdef DEBUG
		cerr << "Right Trigger" << endl;
#endif
		break;
	case SDLK_u:
		buttonbits &= 0xFFBF;
#ifdef DEBUG
		cerr << "X" << endl;
#endif
		break;
	case SDLK_i:
		buttonbits &= 0xFFDF;
#ifdef DEBUG
		cerr << "Y" << endl;
#endif
		break;
	case SDLK_o:
		buttonbits &= 0xFFEF;
#ifdef DEBUG
		cerr << "Z" << endl;
#endif
		break;
	case SDLK_x:
		buttonbits &= 0xFFF7;
#ifdef DEBUG
		cerr << "LTrig" << endl;
#endif
		break;
	default:
		break;
  }
}

void keyUp(int key)
{
  switch (key) {
        case SDLK_RIGHT:
		buttonbits |= ~0x7FFF;
		break;
        case SDLK_LEFT:
		buttonbits |= ~0xBFFF;
		break;
        case SDLK_DOWN:
		buttonbits |= ~0xDFFF;
		break;
        case SDLK_UP:
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

int handleEvents(SaturnMemory *mem) {
    SDL_Event event;
      if (SDL_PollEvent(&event)) {
	switch(event.type) {
	case SDL_QUIT:
		yui_quit();
		break;
	case SDL_KEYDOWN:
	  switch(event.key.keysym.sym) {
                case SDLK_F1:
                        mem->vdp2_3->toggleFPS();
                        break;

                case SDLK_1:
                        ((NBG0 *)(mem->vdp2_3->getNBG0()))->toggleDisplay();
                        break;

                case SDLK_2:
                        ((NBG1 *)(mem->vdp2_3->getNBG1()))->toggleDisplay();
			break;

                case SDLK_3:
                        ((NBG2 *)(mem->vdp2_3->getNBG2()))->toggleDisplay();
			break;

                case SDLK_4:
                        ((NBG3 *)(mem->vdp2_3->getNBG3()))->toggleDisplay();
			break;

                case SDLK_5:
                        ((RBG0 *)(mem->vdp2_3->getRBG0()))->toggleDisplay();
			break;

                case SDLK_6:
                        mem->vdp1_2->toggleDisplay();
			break;

		case SDLK_h:
			yui_hide_show();
			break;
			
		case SDLK_q:
			yui_quit();
			break;
		case SDLK_p:
#ifdef DEBUG
			cerr << "Pause" << endl;
#endif
			break;
		case SDLK_r:
#ifdef DEBUG
			cerr << "Run" << endl;
#endif
			break;
#ifdef DEBUG
		case SDLK_v:
			break;
#endif
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
	try {
		mem->synchroStart();
	}
	catch (Exception e) {
		cerr << e << endl;
                cerr << *mem->getCurrentSH() << endl;
		exit(1);
	}
      }
      return 1;
}

int main(int argc, char **argv) {
	//This function is deprecated according to Sam Latinga - it only works on X11
	//SDL_Init(SDL_INIT_EVENTTHREAD);

	yui_init((int (*)(void*)) &handleEvents);

	SDL_Quit();
        return 0;
}
