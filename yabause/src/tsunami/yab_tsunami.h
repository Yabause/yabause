/*  Copyright 2003-2005 Guillaume Duhamel
    Copyright 2005 Theo Berkau

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef YAB_TSUNAMI_H
#define YAB_TSUNAMI_H

//#define ENABLE_TSUNAMI

void tsunami_init_dummy(void);
void tsunami_log_value_dummy(char * name, int value, int size);
void tsunami_log_pulse_dummy(char * name, int value);
void tsunami_flush_dummy();

#ifdef ENABLE_TSUNAMI
void tsunami_init(void);
void tsunami_log_value(char * name, int value, int size);
void tsunami_log_pulse(char * name, int value);
void tsunami_flush();
#else
#define tsunami_init tsunami_init_dummy
#define tsunami_log_value tsunami_log_value_dummy
#define tsunami_log_pulse tsunami_log_pulse_dummy
#define tsunami_flush tsunami_flush_dummy
#endif

#endif
