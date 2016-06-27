/*  Copyright 2016 James Laird-Wah

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

/*! \file yab_tsunami.c
    \brief Yabause Tsunami library integration
*/

#include "Tsunami.h"

#define TSUNAMI_FILENAME "c:/yabause/cdb.vcd"

void tsunami_init(void) {
   TsunamiInitialise();
   TsunamiStartTimeline("CDB", TSUNAMI_FILENAME, 1048576 * 512);
}

void tsunami_log_value(char * name, int value, int size) {
   TsunamiUpdateTimelineToRealtime("CDB");
   TsunamiSetValue(value, size, "CDB", "root.%s", name);
}

void tsunami_log_pulse(char * name, int value) {
   TsunamiUpdateTimelineToRealtime("CDB");
   TsunamiPulseValue(value, 1, "CDB", "root.%s", name);
}

void tsunami_flush()
{
   TsunamiFlushTimeline("CDB");
}

void tsunami_init_dummy(void) {
}

void tsunami_log_value_dummy(char * name, int value, int size) {
}

void tsunami_log_pulse_dummy(char * name, int value) {
}

void tsunami_flush_dummy() {}