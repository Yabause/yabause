/*
	  Operating System Specific and `Make Up' Definations


	Most of these definations are assumptions, compiled in
	if the system is missing any ANSI C standard definations.

	In ADDITION there are program specific definations here
	for system level values.

 */


#ifndef OS_H
#define OS_H

#include <limits.h>

#if defined(__linux__) || defined(__FreeBSD__)
# include <paths.h>
#endif

/* *******************************************************************
 *
 *                         Terminal sizes
 *
 *	These are not standards but some default must be set for
 *	them.
 */
#ifndef STD_TERMINAL_COLUMS
# define STD_TERMINAL_COLUMS	80
#endif

#ifndef STD_TERMINAL_ROWS
# define STD_TERMINAL_ROWS 	23	/* or should this be 25? */
#endif




/* *******************************************************************
 *
 *                          Object Paths
 *
 *	These should be defined for UNIXes, standard UNIX paths.
 */
#ifndef __PATH_ETC_INET
# define __PATH_ETC_INET	"/etc"
#endif

#ifndef _PATH_DEVNULL
# define _PATH_DEVNULL		"/dev/null"
#endif

#if !defined(_AIX) && !defined(__FreeBSD__) && !defined(__NetBSD__)
# ifndef _PATH_MAILDIR
#  define _PATH_MAILDIR		"/var/spool/mail"
# endif
#endif	/* _AIX */

/* Apparently VI has become the standard editor for UNIX */
#if !defined(_PATH_VI) && !defined(WIN32) && !defined(__hpux)
//# define _PATH_VI		"/usr/bin/vi"
# include <paths.h>
#endif


/*
 *	Directories (UNIX standard):
 *
 *	Note: tailing slash should exist under rule of standard.
 */
#ifndef _PATH_ETC
# define _PATH_ETC		"/etc/"
#endif

#ifndef _PATH_DEV
# define _PATH_DEV		"/dev/"
#endif

#ifndef _PATH_TMP
# define _PATH_TMP		"/tmp/"
#endif

#ifndef _PATH_VARRUN
# define _PATH_VARRUN		"/var/run/"
#endif

#ifndef _PATH_VARTMP
# define _PATH_VARTMP		"/var/tmp/"
#endif


/* *******************************************************************
 *
 *                         Size limits
 *
 *	Limits based on Linux i386.
 *
 *	YOUR SYSTEM SHOULD ALREADY HAVE THESE DEFINED UNDER
 *	ANSI C STANDARD!
 */
/*
#ifndef NR_OPEN
# define NR_OPEN	256	
#endif
*/

#ifndef NGROUPS_MAX
# define NGROUPS_MAX	32	/* supplemental group IDs are available */
#endif

#ifndef ARG_MAX
# define ARG_MAX	131072    /* # bytes of args + environ for exec() */
#endif

#ifndef CHILD_MAX
# define CHILD_MAX	999    /* no limit :-) */
#endif

#ifndef OPEN_MAX
# define OPEN_MAX	256    /* # open files a process may have */
#endif

#ifndef LINK_MAX
# define LINK_MAX	127    /* # links a file may have */
#endif

#ifndef MAX_CANON
# define MAX_CANON	255    /* size of the canonical input queue */
#endif

#ifndef MAX_INPUT
# define MAX_INPUT	255    /* size of the type-ahead buffer */
#endif

#ifndef NAME_MAX
# define NAME_MAX	255	/* # chars in a file name */
#endif

#ifndef PATH_MAX
# define PATH_MAX	1024	/* # chars in a path name */
#endif

#ifndef PIPE_BUF
# define PIPE_BUF	4096	/* # bytes in atomic write to a pipe */
#endif

#ifndef CMD_MAX
# define CMD_MAX	ARG_MAX	/* Same as ARG_MAX */
#endif


/*
 *      Network limits (not ANSI C standard, but BSD standard).
 */
#ifndef HOST_NAME_MAX
# define HOST_NAME_MAX          128
#endif

#ifndef MAX_URL_LEN
# define MAX_URL_LEN            1024
#endif



/*
 *                           Bit Types Makeups
 */

/* Solaris & HPUX */
#if defined(__SOLARIS__) || defined(__hpux)

/* Robin Lee Powell: rlpowell at solect.com
 * Solaris apparently has uint*_t defined instead of u_int*_t defined.
 */

#include <sys/types.h>

# ifdef uint8_t
#  define u_int8_t	uint8_t
# endif
# ifndef u_int8_t
#  define u_int8_t	unsigned char
# endif

# ifdef uint16_t
#  define u_int16_t	uint16_t
# endif
# ifndef u_int16_t
#  define u_int16_t	unsigned short
# endif

# ifdef uint32_t
#  define u_int32_t	uint32_t
# endif
# ifndef u_int32_t
#  define u_int32_t     unsigned int
# endif

# ifdef uint64_t
#  define u_int64_t	uint64_t
# endif
# ifndef u_int64_t
#  define u_int64_t	unsigned long long
# endif

# ifndef int64_t
#  define int64_t	long long
# endif

/* Win32 */
#elif defined(WIN32)

#include <sys/types.h>

#ifndef u_int8_t
# define u_int8_t		unsigned char
#endif
#ifndef int8_t
# define int8_t			char
#endif
#ifndef u_int16_t
# define u_int16_t		unsigned short
#endif
#ifndef int16_t
# define int16_t		short
#endif
#ifndef u_int32_t
# define u_int32_t		unsigned int
#endif
#ifndef int32_t
# define int32_t		int
#endif
#ifndef u_int64_t
# define u_int64_t		unsigned long
#endif
#ifndef int64_t
# define int64_t		long
#endif

#endif


/* Bit types still not defined? */
#if !defined(__BIT_TYPES_DEFINED__) && !defined(WIN32) && !defined(__hpux)
//#define __BIT_TYPES_DEFINED__ Dan S: Multiple declaration problem.
#ifndef __FreeBSD__

// typedef char			int8_t;  Dan S: multiple declaration problem.
typedef unsigned char           u_int8_t;
typedef short                   int16_t;
typedef unsigned short          u_int16_t;
typedef int                     int32_t;
typedef unsigned int            u_int32_t;

# if __GNUC__ >= 2

#  ifndef int64_t
typedef long long               int64_t;
#  endif

#  ifndef u_int64_t
typedef unsigned long long      u_int64_t;
#  endif

# endif	/* __GNUC__ >= 2 */

#endif	/* __FreeBSD__ */
#endif	/* __BIT_TYPES_DEFINED__ */


/* *******************************************************************
 *
 *                   stat() structure sizes
 *
 *	These are data types for the members of the struct stat
 *	used in stat().
 *
 *	Although this is SVID, AT&T, POSIX, X/OPEN, and BSD 4.3
 *	standard, apparently it is still undefined on some UNIXes
 *	(bad distribution?)
 */
/* UNCOMMENT THIS CHUNK IF YOU GET PARSE ERRORS FOR ANY OF THESE TYPES
typedef unsigned short	dev_t;
typedef unsigned long	ino_t;
typedef unsigned short	mode_t;
typedef unsigned short	nlink_t;
typedef long		off_t;
typedef int		pid_t;
typedef unsigned short	uid_t;
typedef unsigned short	gid_t;
typedef unsigned int	size_t;
typedef int		ssize_t;
typedef int		ptrdiff_t;
typedef long		time_t;
typedef long		clock_t;
typedef int		daddr_t;
typedef char *		caddr_t;
*/

#ifdef __hpux
#define setlinebuf(file) setvbuf(file, (char *)NULL, _IOLBF, 0)
#endif


/*
 *	Codes for socket IO function shutdown().
 */
#ifndef SHUT_RD
# define SHUT_RD	0
#endif

#ifndef SHUT_WR
# define SHUT_WR	1
#endif

#ifndef SHUT_RDWR
# define SHUT_RDWR	2
#endif



/*
 *	Function declarations catchers:
 *
 *	Some typical functions arn't prototyped, like isblank()
 *	(which is a GNU extension), so prototype them here.
 */
/* extern int isblank(int c); */


#endif /* OS_H */
