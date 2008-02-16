/*
                              String Utilities

	These string functions go on top of the string functions in
	string.h.
 */

#ifndef STRING_H
#define STRING_H	/* Not to be confused with standard _STRING_H */

#include <sys/types.h>
#include "../include/os.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Checks if c is a space or tab character */
#ifndef ISBLANK
# define ISBLANK(c)	(((c) == ' ') || ((c) == '\t'))
#endif

/* Checks if c is a space, tab, vertical tab, form-feed, carrage
 * return or newline character
 */
#ifndef ISSPACE
# define ISSPACE(c)	(			\
	((c) == ' ') || ((c) == '\t') ||	\
	((c) == '\v') || ((c) == '\f') ||	\
	((c) == '\r') || ((c) == '\n')  	\
)
#endif


/* Line Checking */
extern int strlinelen(const char *s);
extern int strlongestline(const char *s);
extern int strlines(const char *s);

/* Comparing */
#ifdef NEED_STRCASECMP
extern int strcasecmp(const char *s1, const char *s2);
#endif
#ifdef NEED_STRCASESTR
extern char *strcasestr(const char *haystack, const char *needle);
#endif
extern int strpfx(const char *s, const char *pfx);
extern int strcasepfx(const char *s, const char *pfx);

/* Editing */
extern char *strinsstr(char *s, int i, const char *s2);
extern char *strinschr(char *s, int i, char c);
extern char *strdelchrs(char *s, int i, int n);
extern char *strdelchr(char *s, int i);
extern void strtoupper(char *s);
extern void strtolower(char *s);
extern char *strcatalloc(char *orig, const char *new_str);
extern void substr(char *s, const char *token, const char *val);
extern char *strsub(const char *fmt, const char *token, const char *val);
#ifndef WIN32
extern void strset(char *s, char c, int n);
#endif
extern void strpad(char *s, int n);
extern void straddflag(char *s, const char *flag, char operation, int len);
extern void strstriplead(char *s);
extern void strstriptail(char *s);
extern void strstrip(char *s);

/* Lists */
extern char **strlistinsert(char **strv, int *strc, const char *s, int i);
#define strlistins strlistinsert
extern char **strlistappend(char **strv, int *strc, const char *s);
#define strlistapp strlistappend
extern char **strlistdelete(char **strv, int *strc, int i);
#define strlistdel strlistdelete
extern char **strlistcopy(const char **strv, int strc);
extern void strlistfree(char **strv, int strc);

/* Sorting */
extern char **StringQSort(char **strings, int nitems);

extern char *StringTailSpaces(char *s, int len);
extern void StringShortenFL(char *s, int limit);

/* UNIX configruation file format string handling */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" int StringIsYes(const char *s);
extern "C" int StringIsComment(const char *s, char c);
extern "C" char *StringCfgParseParm(const char *s);
extern "C" char *StringCfgParseValue(const char *s);
#else
extern int StringIsYes(const char *s);
extern int StringIsComment(const char *s, char c);
extern char *StringCfgParseParm(const char *s);
extern char *StringCfgParseValue(const char *s);
#endif

/* Misc string parsing */
extern int StringParseStdColor(
	const char *s,
	u_int8_t *r_rtn,
	u_int8_t *g_rtn,
	u_int8_t *b_rtn
);
extern int StringParseIP(
	const char *s,
	u_int8_t *c1,
	u_int8_t *c2,
	u_int8_t *c3,
	u_int8_t *c4
);

/* ShipWars CyberSpace network protocol string functions */
extern int StringGetNetCommand(const char *str);
extern char *StringGetNetArgument(const char *str);

/* Time string formatting */
extern char *StringCurrentTimeFormat(const char *format);
extern char *StringTimeFormat(const char *format, time_t seconds);
extern char *StringFormatTimePeriod(time_t seconds);


#ifdef __cplusplus
}
#endif

#endif /* STRING_H */
