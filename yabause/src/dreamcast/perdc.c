/*  Copyright 2005 Lawrence Sebald

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

int PERDCInit(void);
void PERDCDeInit(void);
int PERDCHandleEvents(void);
void PERDCNothing(void);

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
	if(YabauseExec() != 0)
		return -1;

	return 0;
}

void PERDCNothing(void)	{
}
