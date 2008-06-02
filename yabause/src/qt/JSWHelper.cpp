/*	Copyright 2008 Filipe Azevedo <pasnox@gmail.com>

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
#include "JSWHelper.h"

JSWHelperAttributes* JSWHelper::joysticks()
{ return new JSWHelperAttributes(); }

const char* JSWHelper::errorString( int s )
{
	switch ( s )
	{
		case JSBadValue:
			return "Invalid value encountered while initializing joystick.";
			break;
		case JSNoAccess:
			return "Cannot access resources, check permissions and file locations.";
			break;
		case JSNoBuffers:
			return "Insufficient memory to initialize.";
			break;
		default:
			return "Error occured while initializing.";
			break;
	}
}
