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

#if HAVE_LIBCURSES

#include "superh.hh"
#include <curses.h>
#include "sh2d.hh"
#include "monitor.hh"

Frame::Frame(unsigned int a, unsigned b, unsigned int c, unsigned int d) {
  _left = a;
  _width = b;
  _up = c;
  _height = d;
}

void Frame::draw(void) {
  int i;
  mvaddch(_up, _left, ACS_ULCORNER);
  for(i = 1;i < _width;i++) addch(ACS_HLINE);
  addch(ACS_URCORNER);

  for(i = 1;i < _height - 1;i++) mvaddch(_up + i, _left, ACS_VLINE);
  for(i = 1;i < _height - 1;i++) mvaddch(_up + i, _left + _width, ACS_VLINE);

  mvaddch(_up + _height - 1, _left, ACS_LLCORNER);
  for(i = 1;i < _width;i++) addch(ACS_HLINE);
  addch(ACS_LRCORNER);
}

void monitor(SuperH *proc) {
  char chaine[200];
  int arg_pos = 3;
  char arg[11] = "0x1";
  unsigned long arg_val = 1;

  initscr();
  cbreak();
  noecho();
  nonl();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);

  int c;
  int ymax = 25;
  int xmax = 80;
  getmaxyx(stdscr,ymax, xmax);
  int largeur = (xmax / 12) * 12;
  int deca = (xmax - largeur)/2;

  Frame frame1(deca - 1, largeur + 1, 1, 9);
  frame1.draw();

  Frame cadre2(deca + 2 * largeur / 3 - 1, largeur/3 + 1, 10, 3);
  cadre2.draw();

  Frame cadre3(deca - 1, 2 * largeur / 3 - 2, 10, 3);
  cadre3.draw();

  mvprintw(14,deca,"(0-F) set arg : ");
  mvprintw(16,deca,"(p)ause");
  mvprintw(17,deca,"(q)uit");
  mvprintw(18,deca,"(r)un");
  mvprintw(19,deca,"(s)tep one instruction");
 
  while((c = getch()) != 'q') {

    if (((c >= '0') && (c <= '9')) ||
	((c >= 'a') && (c <= 'f')) ||
	((c >= 'A') && (c <= 'F'))) {
      sprintf(arg, "%x%c", arg_val, c);
      sscanf(arg, "%x", &arg_val);
    }

    if (c == KEY_BACKSPACE) {
      arg_val /= 16;
    }

    if (c == 'm') {
      mvprintw(14, deca + 30, "Memory dump");
      for(int i = 0;i < 5;i++) {
        try {
          mvprintw(15 + i, deca + 30, "%8X %8X", arg_val, proc->getMemory()->getLong(arg_val));
	}
        catch(BadMemoryAccess ma) {
	  mvprintw(15 + i, deca + 30, "bad memory access");
        }
	arg_val += 4;
      }
    }
    
    if (c == 'p') proc->pause();

    if (c == 'r') proc->run();

    if (c == 's') proc->step();
    
    // AFFICHAGE
    for(int i = 0;i < 4;i++)
      for(int j = 0;j < 4;j++) {
	mvprintw(i + 2, deca + j * largeur/4 + 1, "R%d", 4 * i  + j);
	mvprintw(i+2, deca+(j+1) * largeur/4 - 11, "%10X", proc->R[4 * i + j]);
      }

    int l5 = ((largeur - 4) / 5) * 5;
    int r5 = largeur - l5;
    mvprintw(6,deca + 1,"SR");
    mvprintw(6,deca + r5 + 1,"M");
    mvprintw(6,deca + l5/5 + r5 - 2,"%1X",proc->SR.partie.M);
    mvprintw(6,deca + l5/5 + r5 + 1,"Q");
    mvprintw(6,deca + 2 * l5/5 + r5 - 2,"%1X",proc->SR.partie.Q);
    mvprintw(6,deca + 2 * l5/5 + r5 + 1,"I");
    mvprintw(6,deca + 3 * l5/5 + r5 - 2,"%1X",proc->SR.partie.I);
    mvprintw(6,deca + 3 * l5/5 + r5 + 1,"S");
    mvprintw(6,deca + 4 * l5/5 + r5 - 2,"%1X",proc->SR.partie.S);
    mvprintw(6,deca + 4 * l5/5 + r5 + 1,"T");
    mvprintw(6,deca + l5 + r5 - 2,"%1X",proc->SR.partie.T);

    mvprintw(7,deca + 1,"GBR");
    mvprintw(7,deca + largeur/3 - 11,"%10X",proc->GBR);
    mvprintw(7,deca + largeur/3 + 1,"MACH");
    mvprintw(7,deca + 2 * largeur/3 - 11,"%10X",proc->MACH);
    mvprintw(7,deca + 2 * largeur/3 + 1,"PR");
    mvprintw(7,deca + largeur - 11,"%10X",proc->PR);

    mvprintw(8,deca + 1,"VBR");
    mvprintw(8,deca + largeur/3 - 11,"%10X",proc->VBR);
    mvprintw(8,deca + largeur/3 + 1,"MACL");
    mvprintw(8,deca + 2 * largeur/3 - 11,"%10X",proc->MACL);
    mvprintw(8,deca + 2 * largeur/3 + 1,"PC");
    mvprintw(8,deca + largeur - 11,"%10X",proc->PC);

    mvprintw(11,deca + 2 * largeur/3 + 1,"@(delay) ");
    mvprintw(11,deca + largeur - 11,"%10X",proc->instruction);
    SH2Disasm(proc->_delai - 4, proc->instruction, 0, chaine);
    mvprintw(11, deca + 1, "                                  ");
    mvprintw(11, deca + 1, chaine);

    if (proc->_delai) {
      mvprintw(11,deca + 2 * largeur/3 + 1,"@(delay) ");
      mvprintw(11,deca + largeur - 11,"%10X",proc->instruction);
      SH2Disasm(proc->_delai - 4, proc->instruction, 0, chaine);
      mvprintw(11, deca + 1, "                                  ");
      mvprintw(11, deca + 1, chaine);
    }
    else {
      mvprintw(11,deca + 2 * largeur/3 + 1,"@(PC - 4)");
      mvprintw(11,deca + largeur - 11,"%10X",proc->instruction);
      SH2Disasm(proc->PC - 4, proc->instruction, 0, chaine);
      mvprintw(11, deca + 1, "                                  ");
      mvprintw(11, deca + 1, chaine);
    }

    //mvprintw(14, deca + 16, "\n");
    mvprintw(14, deca + 16, "%10X", arg_val);

    //proc->microsleep(100000);
  }
  endwin();

}

#endif
