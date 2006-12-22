/*  Copyright 2005-2006 Lawrence Sebald

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

#include "dreamcast/perdc.h"
#include "yabause.h"
#include "yui.h"
#include "vdp2.h"

#include <dc/maple.h>
#include <dc/maple/controller.h>

int PERDCInit(void);
void PERDCDeInit(void);
int PERDCHandleEvents(void);

extern u16 buttonbits;

PerInterface_struct PERDC = {
	PERCORE_DC,
	"Dreamcast Input Interface",
	PERDCInit,
	PERDCDeInit,
	PERDCHandleEvents
};

int PERDCInit(void)	{
	return 0;
}

void PERDCDeInit(void)	{
}

int PERDCHandleEvents(void)	{
    maple_device_t *dev;

    dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
    if(dev != NULL) {
        cont_state_t *state = (cont_state_t *) maple_dev_status(dev);

        if(state != NULL)   {
            if(state->buttons & CONT_DPAD_UP)
                buttonbits &= 0xEFFF;
            else
                buttonbits |= 0x1000;

            if(state->buttons & CONT_DPAD_DOWN)
                buttonbits &= 0xDFFF;
            else
                buttonbits |= 0x2000;

            if(state->buttons & CONT_DPAD_RIGHT)
                buttonbits &= 0x7FFF;
            else
                buttonbits |= 0x8000;

            if(state->buttons & CONT_DPAD_LEFT)
                buttonbits &= 0xBFFF;
            else
                buttonbits |= 0x4000;

            if(state->buttons & CONT_START)
                buttonbits &= 0xF7FF;
            else
                buttonbits |= 0x0800;

            if(state->buttons & CONT_A)
                buttonbits &= 0xFBFF;
            else
                buttonbits |= 0x0400;

            if(state->buttons & CONT_B)
                buttonbits &= 0xFEFF;
            else
                buttonbits |= 0x0100;

            if(state->buttons & CONT_X)
                buttonbits &= 0xFFBF;
            else
                buttonbits |= 0x0040;

            if(state->buttons & CONT_Y)
                buttonbits &= 0xFFDF;
            else
                buttonbits |= 0x0020;

            if(state->rtrig > 20)
                buttonbits &= 0xFF7F;
            else
                buttonbits |= 0x0080;

            if(state->ltrig > 20)
                buttonbits &= 0xFFF7;
            else
                buttonbits |= 0x0008;

            if(state->joyx > 20)
                buttonbits &= 0xFDFF;
            else
                buttonbits |= 0x0200;

            if(state->joyy > 20)
                buttonbits &= 0xFFEF;
            else
                buttonbits |= 0x0010;

        }
    }

	if(YabauseExec() != 0)
		return -1;

	return 0;
}
