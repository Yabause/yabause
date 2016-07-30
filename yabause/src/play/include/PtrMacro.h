/*

	Framework - PtrMacro.h
	File Version 1.0.000
	Various pointer related macros

*/

#ifndef _PTRMACRO_H_
#define _PTRMACRO_H_

#ifndef NULL
#define NULL 0
#endif

#define FREEPTR(p)		\
	if((p) != NULL)		\
	{					\
		free((p));		\
		(p) = NULL;		\
	}

#define DELETEPTR(p)	\
	if((p) != NULL)		\
	{					\
		delete (p);		\
		(p) = NULL;		\
	}

#endif
