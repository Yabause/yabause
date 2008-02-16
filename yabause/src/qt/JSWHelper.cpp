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
