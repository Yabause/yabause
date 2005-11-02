/*  Copyright 2005 Fabien Coulon

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

/* This file is for managing configuration file of Yabause : $HOME/.yabause
   ---------------------------------------------------------------------------- */

#ifndef _YUI_CONF_H_
#define _YUI_CONF_H_

#include <gtk/gtk.h>

void GtkYuiConfInit();
char* GtkYuiGetString( const char* name );
void GtkYuiSetString( const char* name, const char* value );
int GtkYuiGetInt( const char* name, int deflt );
void GtkYuiSetInt( const char* name, int value );
void GtkYuiStore();

#endif
