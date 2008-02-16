#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <time.h>
#include "../include/os.h"

#if defined(_WIN32)
/* Nothing */
#else
# include <sys/time.h>
#endif

#include "../include/cfgfmt.h"
#include "../include/cs.h"
#include "../include/string.h"


/* Line Checking */
int strlinelen(const char *s);
int strlongestline(const char *s);
int strlines(const char *s);

/* Comparing */
#ifdef NEED_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#endif
#ifdef NEED_STRCASESTR
char *strcasestr(const char *haystack, const char *needle);
#endif
int strpfx(const char *s, const char *pfx);
int strcasepfx(const char *s, const char *pfx);

/* Editing */
char *strinsstr(char *s, int i, const char *s2);
char *strinschr(char *s, int i, char c);
char *strdelchrs(char *s, int i, int n);
char *strdelchr(char *s, int i);
void strtoupper(char *s);
void strtolower(char *s);
char *strcatalloc(char *orig, const char *new_str);
void substr(char *s, const char *token, const char *val);
char *strsub(const char *fmt, const char *token, const char *val);
#ifndef _WIN32
void strset(char *s, char c, int n);
#endif
#ifndef _WIN32
void strpad(char *s, int n);
#endif
void straddflag(char *s, const char *flag, char operation, int len);
void strstriplead(char *s);
void strstriptail(char *s);
void strstrip(char *s);

/* Lists */
char **strlistinsert(char **strv, int *strc, const char *s, int i);
char **strlistappend(char **strv, int *strc, const char *s);
char **strlistdelete(char **strv, int *strc, int i);
char **strlistcopy(const char **strv, int strc);
void strlistfree(char **strv, int strc);

/* Sorting */
static int SORT(const void *a, const void *b);
char **StringQSort(char **strings, int nitems);

/* Decorating */
char *StringTailSpaces(char *s, int len);
void StringShortenFL(char *s, int limit);

/* Configuration Checking */
int StringIsYes(const char *s);
int StringIsComment(const char *s, char c);
char *StringCfgParseParm(const char *s);
char *StringCfgParseValue(const char *s);

/* Value Parsing */
int StringParseStdColor(
	const char *s,
	u_int8_t *r_rtn, u_int8_t *g_rtn, u_int8_t *b_rtn
);
int StringParseIP(
	const char *s,
	u_int8_t *c1, u_int8_t *c2, u_int8_t *c3, u_int8_t *c4
);

/* ShipWars protocol string parsing */
int StringGetNetCommand(const char *str);
char *StringGetNetArgument(const char *str);

/* Time Formatting */
char *StringCurrentTimeFormat(const char *format);
char *StringTimeFormat(const char *format, time_t seconds);
char *StringFormatTimePeriod(time_t seconds);


#define ATOI(s)         (((s) != NULL) ? atoi(s) : 0)
#define ATOL(s)         (((s) != NULL) ? atol(s) : 0)
#define ATOF(s)         (((s) != NULL) ? atof(s) : 0.0f)
#define STRDUP(s)       (((s) != NULL) ? strdup(s) : NULL)

#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)     (MIN(MAX((a),(l)),(h)))
#define STRLEN(s)       (((s) != NULL) ? strlen(s) : 0)
#define STRISEMPTY(s)   (((s) != NULL) ? (*(s) == '\0') : 1)


/*
 *	Returns the length in bytes, of the line s.
 *	The end point character must be either a '\n', '\r', or
 *	'\0'.
 */
int strlinelen(const char *s)
{
	int i = 0;


	if(s == NULL)
	    return(0);

	while((*s != '\0') &&
	      (*s != '\n') &&
	      (*s != '\r')
	)
	{
	    i++;
	    s++;
	}

	return(i);
}


/*
 *	Returns the length of the longest line in string s.
 */
int strlongestline(const char *s)
{
	int n, longest = 0;


	if(s == NULL)
	    return(longest);

	while(1)
	{
	    n = strlinelen(s);

	    if(n > longest)
		longest = n;

	    s += n;

	    if(*s == '\0')
		break;

	    s++;
	}

	return(longest);
}

/*
 *	Returns the number of '\n' or '\r' characters + 1
 *	in string s.
 *
 *	If the first character is '\0' in string s or string s
 *	is NULL, then 0 will be returned.
 */
int strlines(const char *s)
{
	int lines = 0;

	if(s == NULL)
	    return(lines);

	if(*s == '\0')
	    return(lines);

	lines++;	/* Count first line */

	/* Iterate through string, counting number of lines */
	while(*s != '\0')
	{
	    if((*s == '\n') || (*s == '\r'))
		lines++;
	    s++;
	}

	return(lines);
}

#ifdef NEED_STRCASECMP
/*
 *	Works just like the UNIX strcasecmp(). Returns 1 for no match
 *	and 0 for match.
 */
int strcasecmp(const char *s1, const char *s2)
{
	if((s1 == NULL) || (s2 == NULL))
	    return(1);		/* False */

	while(*s1 && *s2)
	{
	    if(toupper((int)(*s1)) != toupper((int)(*s2)))
		return(1);	/* False */

	    s1++;
	    s2++;
	}
	if(*s1 == *s2)
	    return(0);		/* True */
	else
	    return(1);		/* False */
}
#endif

#ifdef NEED_STRCASESTR
/*
 *	Case insensitive version of strstr(). Returns the pointer to
 *	needle in haystack if found or NULL on no match.
 */
char *strcasestr(const char *haystack, const char *needle)
{
	const char *sh, *sn;

	if((haystack == NULL) || (needle == NULL))
	    return(NULL);

	sh = haystack;
	while(*sh != '\0')
	{
	    sn = needle;	/* Get starting position of needle */

	    /* Character in needle and haystack at same position the
	     * same (case insensitive)
	     */
	    if(toupper((int)(*sh)) == toupper((int)(*sn)))
	    {
		const char *sh_needle_start = sh;

		sh++;
		sn++;

		/* Continue iterating through haystack */
		while(*sh != '\0')
		{
		    /* End of needle? */
		    if(*sn == '\0')
		    {
			return((char *)sh_needle_start);
		    }
		    /* Characters differ (case insensitive)? */
		    else if(toupper((int)(*sh)) != toupper((int)(*sn)))
		    {
			sh++;
			break;
		    }
		    /* Characters are still matching */
		    else
		    {
			sh++;
			sn++;
		    }
		}
		/* If the end of the needle has been reached then we
		 * got a match
		 */
		if(*sn == '\0')
		    return((char *)sh_needle_start);
	    }
	    else
	    {
		sh++; 
	    }
	}

	return(NULL);
}
#endif	/* NEED_STRCASESTR */

/*
 *	Checks if the string specified pfx is a prefix of the string
 *	specified by s.
 */
int strpfx(const char *s, const char *pfx)
{
	if((s == NULL) || STRISEMPTY(pfx))
	    return(0);

	/* Iterate through string and pfx to see if all characters
	 * match up to the end of pfx
	 */
	while(*pfx != '\0')
	{
	    if(*s != *pfx)
		return(0);

	    s++;
	    pfx++;
	}

	return(1);
}

int strcasepfx(const char *s, const char *pfx)
{
	if((s == NULL) || STRISEMPTY(pfx))
	    return(0);

	/* Iterate through string and pfx to see if all characters
	 * match up to the end of pfx
	 */
	while(*pfx != '\0')
	{
	    if(toupper((int)(*s)) != toupper((int)(*pfx)))
		return(0);

	    s++;
	    pfx++;  
	}

	return(1);
}

/*
 *	Inserts string s2 at position i in string s.
 *
 *	Returns the new base pointer of s due to realloc().
 */
char *strinsstr(char *s, int i, const char *s2)
{
	int slen, dlen = STRLEN(s2);
	char *ps, *pt, *pend;
	const char *ps2;

	if(dlen <= 0)
	    return(s);

	if(s == NULL)
	    s = STRDUP("");

	slen = STRLEN(s);

	/* Append? */
	if(i < 0)
	    i = slen;
	else if(i > slen)
	    i = slen;

	/* Increase allocation of string s */
	slen += dlen;
	s = (char *)realloc(s, (slen + 1) * sizeof(char));
	if(s == NULL)
	    return(s);

	/* Shift string s */
	for(pt = s + slen,
	    ps = pt - dlen,
	    pend = s + i;
	    ps >= pend;
	    *pt-- = *ps--
	);

	/* Insert string s2 into string s */
	for(pt = s + i,
	    ps2 = s2,
	    pend = pt + dlen;
	    pt < pend;
	    *pt++ = *ps2++
	);

	return(s);
}

/*
 *	Inserts character c at position i in string s.
 *
 *	Returns the new base pointer of s due to realloc().
 */
char *strinschr(char *s, int i, char c)
{
	int slen;
	char *ps, *pt, *pend;

	if(s == NULL)
	    s = STRDUP("");

	slen = STRLEN(s);

	/* Append? */
	if(i < 0)
	    i = slen;
	else if(i > slen)
	    i = slen;

	/* Increase allocation of string s */
	slen++;
	s = (char *)realloc(s, (slen + 1) * sizeof(char));
	if(s == NULL)
	    return(s);

	/* Shift string s */
	for(pt = s + slen,
	    ps = pt - 1,
	    pend = s + i;
	    ps >= pend;
	    *pt-- = *ps--
	);

	/* Insert character c into string s */
	s[i] = c;

	return(s);
}

/*
 *	Deletes the specified number of characters at position i in
 *	string s.
 *
 *	Returns the new base pointer of s due to realloc().
 */
char *strdelchrs(char *s, int i, int n)
{
	int slen;
	char *ps, *pt, *pend;

	if((s == NULL) || (n <= 0))
	    return(s);

	slen = STRLEN(s);
	if(slen <= 0)
	    return(s);

	if((i < 0) || (i >= slen))
	    return(s);

	/* Sanitize number of characters to delete */
	if((i + n) > slen)
	    n = slen - i;

	/* Shift string s */
	for(pt = s + i,
	    ps = pt + n,
	    pend = s + slen;
	    ps <= pend;
	    *pt++ = *ps++
	);

	/* Decrease allocation of string s */
	slen -= n;
	if(slen > 0)
	{
	    s = (char *)realloc(s, (slen + 1) * sizeof(char));
	    if(s == NULL)
		return(s);
	}
	else
	{
	    s = (char *)realloc(s, 1 * sizeof(char));
	    if(s == NULL)
		return(s);

	    *s = '\0';
	}
	return(s);
}

/*
 *	Deletes one character at position i in string s.
 *
 *	Returns the new base pointer of s due to realloc().
 */
char *strdelchr(char *s, int i)
{
	return(strdelchrs(s, i, 1));
}

/*
 *	Sets all characters in string s to upper.
 */
void strtoupper(char *s)
{
	if(s == NULL)
	    return;

	for(; *s != '\0'; s++)
	    *s = (char)toupper((int)(*s));
}

/*
 *      Sets all characters in string s to lower.
 */
void strtolower(char *s)
{
	if(s == NULL)
	    return;

	for(; *s != '\0'; s++)
	    *s = (char)tolower((int)(*s));
}

/*
 *	Takes the given string orig and reallocates it with enough
 *	memory to hold itself and the new string plus a null terminating
 *	byte at the end.
 *
 *	Returns the new pointer which needs to be free()'ed by the calling
 *	function.
 *
 *	The given pointer orig should never be referenced again after this
 *	call.
 */
char *strcatalloc(char *orig, const char *new_str)
{
	int orig_len, new_len, rtn_len;
	char *new_rtn = NULL;


	/* If new string is NULL, then return original pointer since
	 * there is nothing to do
	 */
	if(new_str == NULL)
	    return(orig);

	/* Calculate lengths of original and new */
	orig_len = (orig != NULL) ? strlen(orig) : 0;
	new_len = strlen(new_str);
	rtn_len = orig_len + new_len;
	if(rtn_len < 0)
	    rtn_len = 0;

	/* Reallocate string */
	new_rtn = (char *)realloc(
	    orig,
	    (rtn_len + 1) * sizeof(char)
	);
	if(new_rtn != NULL)
	{
	    /* If original string was NULL then it means a new string
	     * has been allocated so we need to set the first character
	     * to null in order to strcat() properly
	     */
	    if(orig == NULL)
		*new_rtn = '\0';

	    /* Cat new string to the new return string */
	    strcat(new_rtn, new_str);
	}

	return(new_rtn);
}

/*
 *	Substitutes all occurances of string token in string s
 *	with string val.  String s must have enough capacity for
 *	all substitutions.
 *
 *	Example: substr("Hi there %name!", "%name", "Kattie")
 *	Turns into: "Hi there Kattie!"
 */
void substr(char *s, const char *token, const char *val)
{
	int i, tl, vl;
	char *strptr1, *strptr2, *strptr3, *strptr4;


	if((s == NULL) ||
	   (token == NULL)
	)
	    return;

	if(val == NULL)
	    val = "";

	/* Token string must not be empty */
	if(*token == '\0')
	    return;

	/* Token and value strings may not have the same content */
	if(!strcmp(val, token))
	    return;

	/* Get lengths of token and value strings */
	tl = strlen(token);
	vl = strlen(val);

	/* Set strptr1 to begining of string s */
	strptr1 = s;

	/* Begin substituting */
	while(1)
	{
	    /* Seek next instance of token string */
	    strptr1 = strstr(strptr1, token);
	    if(strptr1 == NULL)
		break;

	    /* Calculate end pointer of strptr1 */
	    i = strlen(strptr1);
	    strptr2 = strptr1 + i;

	    if(tl > vl)
	    {
		/* Token string is longer than value string */

		/* Calculate starting pointer positions */
		strptr3 = strptr1 + vl;
		strptr4 = strptr1 + tl;

		/* Shift tailing portion of string */
		while(strptr4 <= strptr2)
		    *strptr3++ = *strptr4++;
	    }
	    else if(tl < vl)
	    {
		/* Token string is less than value string */

		/* Calculate starting pointer positions */
		strptr3 = strptr2;
		strptr4 = strptr2 + vl - tl;

		/* Shift tailing portion of string */
		while(strptr3 > strptr1) 
		    *strptr4-- = *strptr3--;
	    }

	    memcpy(strptr1, val, vl);

	    /* Increment strptr1 past length of value */
	    strptr1 = strptr1 + vl;
	}
}

/*
 *	Substitutes a format string containing tokens for values.
 *
 *	The fmt specifies the format string.
 *
 *	The token specifies the token string.
 *
 *	The val specifies the value string to replace each token
 *	string encountered in the format string.
 *
 *	Returns a dynamically allocated string describing the
 *	substituted format string or NULL on error.
 */
char *strsub(const char *fmt, const char *token, const char *val)
{
	const int	token_len = STRLEN(token),
			val_len = STRLEN(val);
	const char *fmt_token;
	char *ts = NULL;
	int t_len = 0;

	if(fmt == NULL)
	    return(NULL);

	if(val == NULL)
	    val = "";

	if(token_len <= 0)
	    return(STRDUP(fmt));

	while(1)
	{
	    /* Seek next token position in the format string */
	    fmt_token = strstr(fmt, token);
	    if(fmt_token != NULL)
	    {
		/* Found next token, copy the format string leading up
		 * to the token to the target string and then copy the
		 * value to the target string
		 */
		const int fmt_len = (int)(fmt_token - fmt);

		/* Increase target string allocation and copy
		 * presegment of the format string
		 */
		if(fmt_len > 0)
		{
		    ts = (char *)realloc(
			ts, (t_len + fmt_len) * sizeof(char)
		    );
		    if(ts == NULL)
			return(NULL);
		    memcpy(ts + t_len, fmt, fmt_len * sizeof(char));
		    t_len += fmt_len;
		}

		/* Seek format string past this token */
		fmt = (const char *)(fmt_token + token_len);

		/* Increase target string allocation and copy
		 * the value string
		 */
		if(val_len > 0)
		{
		    ts = (char *)realloc(
			ts, (t_len + val_len) * sizeof(char)
		    );
		    if(ts == NULL)
			return(NULL);
		    memcpy(ts + t_len, val, val_len * sizeof(char));
		    t_len += val_len;
		}

		/* No need to null terminate the target string, it will
		 * be null terminated when no more tokens are found
		 */
	    }
	    else
	    {
		/* No more tokens, copy the remaining segment of the
		 * format string to the target string
		 */
		const int fmt_len = strlen(fmt);

		/* Increase target string allocation and copy the
		 * remaining segment of the format string plus a null
		 * terminating byte
		 */
		ts = (char *)realloc(
		    ts, (t_len + fmt_len + 1) * sizeof(char)
		);
		if(ts == NULL)
		    return(NULL);
		if(fmt_len > 0)
		{
		    memcpy(ts + t_len, fmt, fmt_len * sizeof(char));
		    t_len += fmt_len;
		}

		ts[t_len] = '\0';

		break;
	    }
	}

	return(ts);
}

#if !defined(_WIN32)
/*
 *	Sets the first n characters of string s to be the
 *	value of c.  A null character will be tacked on at the end.
 *
 *	String s must have enough storage for n characters plus the
 *	null terminating character tacked on at the end.
 */
void strset(char *s, char c, int n)
{
	int i;

	if(s == NULL)
	    return;

	for(i = 0; i < n; i++)
	    s[i] = c;

	s[i] = '\0';
}
#endif

#if !defined(_WIN32)
/*
 *	Same as strset(), except always sets the first n characters
 *	of string s to the ' ' character. Tacks on a null terminating
 *	character at the end.
 */
void strpad(char *s, int n)
{
	strset(s, ' ', n);
}
#endif

/*
 *      Concatonates flag string, putting in the operation character
 *      if there is one or more flags already existing in s.
 * 
 *      "Close | Open" "Inventory" '|'
 * 
 *      Becomes:
 * 
 *      "Close | Open | Inventory"
 *	Dan S: Renaming arg 3 from "operator" to "operation", "operator" is a C+ key word.
 */
void straddflag(char *s, const char *flag, char operation, int len)
{
	int s_len, flag_len;

	if(STRISEMPTY(s) || STRISEMPTY(flag) || (len <= 0))
	    return;

	s_len = STRLEN(s);
	flag_len = STRLEN(flag);

	/* Put operation after last flag if there is one */
	if((s_len > 0) &&
	   ((len - s_len) > 3)
	)
	{
	    int i;

	    i = s_len;
  
	    s[i] = ' '; i++;
	    s[i] = operation; i++;
	    s[i] = ' '; i++;
	    s[i] = '\0';
   
	    s_len += 3;
	}
 
	/* Append flag string */
	if((len - s_len - 1) > 0)
	    strncat(s, flag, len - s_len - 1);

	s[len - 1] = '\0';
}

/*
 *	Strips leading blank characters.
 */
void strstriplead(char *s)
{
	char *s2 = s;

	if(STRISEMPTY(s))
	    return;

	while(ISBLANK(*s2))
	    s2++;

	if(s2 == s)
	    return;

	while(*s2 != '\0')
	    *s++ = *s2++;
	*s = *s2;
}

/*
 *	Strips tailing blank characters.
 */
void strstriptail(char *s)
{
	char *s2;

	if(STRISEMPTY(s))
	    return; 

	s2 = s + STRLEN(s) - 1;
	while(s2 >= s)
	{
	    if(!ISBLANK(*s2))
		break;
	    s2--;
	}
	s2++;
	*s2 = '\0';
}

/*
 *	Strips leading and tailing blank characters.
 */
void strstrip(char *s)
{
	strstriplead(s);
	strstriptail(s);
}


/*
 *	Inserts string s into the string list at index i.
 */
char **strlistinsert(char **strv, int *strc, const char *s, int i)
{
	if(strc == NULL)
	    return(NULL);

	/* Append? */
	if(i < 0)
	{
	    i = MAX(*strc, 0);
	    *strc = i + 1;
	    strv = (char **)realloc(strv, (*strc) * sizeof(char *));
	    if(strv == NULL)
	    {
		*strc = 0;
		return(strv);
	    }
	    strv[i] = STRDUP(s);
	}
	else
	{
	    int n = MAX(*strc, 0);
	    *strc = n + 1;   
	    strv = (char **)realloc(strv, (*strc) * sizeof(char *));
	    if(strv == NULL)
	    {
		*strc = 0;
		return(strv);
	    }

	    if(i > n)
		i = n;

	    for(; n > i; n--)
		strv[n] = strv[n - 1];

	    strv[i] = STRDUP(s);
	}

	return(strv);
}

/*
 *      Appends string s to the string list.
 */
char **strlistappend(char **strv, int *strc, const char *s)
{
	return(strlistinsert(strv, strc, s, -1));
}

/*
 *      Deletes the string at index i in the string list.
 */
char **strlistdelete(char **strv, int *strc, int i)
{
	int n;

	if((i < 0) || (i >= *strc))
	    return(strv);

	/* Reduce total */
	*strc = (*strc) - 1;

	/* Delete string */
	free(strv[i]);

	/* Shift pointers */
	for(n = i; n < *strc; n++)
	    strv[n] = strv[n + 1];

	/* Reduce allocation of string list pointer array */
	if(*strc > 0)
	{
	    strv = (char **)realloc(strv, (*strc) * sizeof(char *));
	    if(strv == NULL)
	    {
		*strv = 0;
	    }
	}
	else
	{
	    free(strv);
	    strv = NULL;
	    *strc = 0;
	}

	return(strv);
}

/*
 *	Coppies the string list.
 */
char **strlistcopy(const char **strv, int strc)
{
	char **strv2 = (char **)(
	    (strc > 0) ? malloc(strc * sizeof(char *)) : NULL
	);
	if(strv2 != NULL)
	{
	    int i;
	    for(i = 0; i < strc; i++)
		strv2[i] = STRDUP(strv[i]);
	}
	return(strv2);
}


/*
 *	Deletes the string list.
 */
void strlistfree(char **strv, int strc)
{
	int i;

	for(i = 0; i < strc; i++)
	    free(strv[i]);
	free(strv);
}


/*
 *	Used by StringQSort().
 */
static int SORT(const void *a, const void *b)
{
	char *x, *y;
	x = *((char **)a);
	y = *((char **)b);
	return(strcmp(x, y));
}

/*
 *	Sorts given string array strings using the qsort() method.
 *
 *	Returns NULL on error and strings on success.
 */
char **StringQSort(char **strings, int nitems)
{
	if((strings == NULL) || (nitems <= 0))
	    return(NULL);

	qsort(strings, nitems, sizeof(char *), SORT);

	return(strings);
}


/*
 *	Puts space characters at the end of the given string's current
 *	contents up to the specified length len.
 *
 *	The allocation of string must be len + 1.
 */
char *StringTailSpaces(char *s, int len)
{
	int i, prev_len;

	if(s == NULL)
	    return(s);

	s[len] = '\0';
	prev_len = STRLEN(s);

	for(i = prev_len; i < len; i++)
	    s[i] = ' ';

	return(s);
}


/*
 *      Shortens string to number of characters specified by limit.
 *
 *      If the limit is greater than 3 then "..." is prepended to s.
 *
 *      If length of string is <= to limit, then nothing is done.
 *
 *	Example (limit=10):
 *
 *	"Hello there kitty!" becomes "... kitty!"
 */
void StringShortenFL(char *s, int limit)
{
	int ol;

	if(STRISEMPTY(s))
	    return;

	if(limit < 0)
	{
	    if(*s != '\0')
		*s = '\0';
	    return;
	}

	ol = STRLEN(s);
	if(ol > limit)
	{
	    char	*s1 = s,
			*s2 = &s[ol - limit];

	    while(*s2 != '\0')
		*s1++ = *s2++;

	    if(limit >= 3)
	    {
		s1 = s;
		s2 = &s[3];
		while(s1 < s2)
		    *s1++ = '.';
	   }

	    s[limit] = '\0';
	}
}


/*
 *	Checks if the string is a "yes" (and standard variations
 *	accepted and checked as well).
 */
int StringIsYes(const char *s)
{
	if(STRISEMPTY(s))
	    return(0);

	while(ISBLANK(*s))
	    s++;

	/* Is first char a number from 0 to 9 (for "0" or "1")? */
	if(isdigit(*s))
	{
	    if(*s != '0')
		return(1);
	    else
		return(0);
	}
	/* Is first char a 'o'? (for "on" or "off") */
	else if(toupper(s[0]) == 'O')
	{
	    /* Check second char, is it an 'n'? */
	    if(toupper(s[1]) == 'N')
		return(1);
	    else
		return(0);
	}
	/* All else check first char for "yes" or "no" */
	else
	{
	    if(toupper(s[0]) == 'Y')
	        return(1);
	    else
	        return(0);
	}
}

/*
 *	Returns true if string is a comment in accordance with
 *	the UNIX configuration file format.
 *
 *	The comment character c should be
 *	(but does not have to be) UNIXCFG_COMMENT_CHAR.
 */
int StringIsComment(const char *s, char c)
{
	if(STRISEMPTY(s))
	    return(0);

	while(ISBLANK(*s))
	    s++;

	return((*s == c) ? 1 : 0);
}

/*
 *	Returns the parameter section of string which should comform to
 *	the standard configuration format of "<parameter>=<value>" as
 *	a statically allocated string or NULL on error.
 */
char *StringCfgParseParm(const char *s)
{
	int x, y, got_parm_start;
	static char parameter[CFG_PARAMETER_MAX];


	if(s == NULL)
	    return(NULL);

	if((*s == '\0') || (*s == '\r') || (*s == '\n'))
	    return(NULL);

	/* Is comment? */
	if(StringIsComment(s, UNIXCFG_COMMENT_CHAR))
	    return(NULL);

	/* Begin fetching parameter from string */
	got_parm_start = 0;
	for(x = 0, y = 0;
	    (x < CFG_STRING_MAX) && (y < CFG_PARAMETER_MAX);
	    x++
	)
	{
	    /* Skip newline escape sequences */
	    if((s[x] == '\\') &&
	       ((x + 1) < CFG_STRING_MAX)
	    )
	    {
		if((s[x + 1] == '\n') || (s[x + 1] == '\r'))
	        {
		    x++;
	            continue;
	        }
	    }

	    /* Skip other escape sequences */
	    if(s[x] == '\\')
	    {   
		x++;
		if(x >= CFG_STRING_MAX)
		    break;
	    }


	    /* End on NULL, new line, or delimiter */
	    if((s[x] == '\0') ||
	       (s[x] == '\r') ||
	       (s[x] == '\n') || 
	       (s[x] == CFG_PARAMETER_DELIMITER)
	    )
	    {
		parameter[y] = '\0';
		break;
	    }   

	    if(got_parm_start == 0)
	    {
		if((s[x] == ' ') ||
		   (s[x] == '\t')
		)
		    continue;
		else
		    got_parm_start = 1;
	    }

	    parameter[y] = s[x];
	    y++;
	}

	/* Null terminate parameter */  
	parameter[sizeof(parameter) - 1] = '\0';
	strstrip(parameter);

	return(parameter);
}

/*
 *      Returns the value section of string which should comform to
 *      the standard configuration format of "<parameter>=<value>".
 *
 *      The returned string will never be NULL and will be striped of
 *      tailing or leading spaces.
 */
char *StringCfgParseValue(const char *s)
{
	int x, y, got_value;
	static char value[CFG_VALUE_MAX];


	*value = '\0';

	if(s == NULL)
	    return(value);

	if((*s == '\0') || (*s == '\r') || (*s == '\n'))
	    return(value);

	/* Is s a comment? */
	if(StringIsComment(s, UNIXCFG_COMMENT_CHAR))
	    return(value);

	/* Does not have a delimiter? */
	if(strchr(s, CFG_PARAMETER_DELIMITER) == NULL)
	    return(value);


	/* Begin fetching value from string */
	got_value = 0;
	for(x = 0, y = 0;
	    (x < CFG_STRING_MAX) && (y < CFG_VALUE_MAX);
	    x++
	)
	{
	    /* Skip newline escape sequences */
	    if((s[x] == '\\') &&
	       ((x + 1) < CFG_STRING_MAX)
	    )
	    {
		if((s[x + 1] == '\n') || (s[x + 1] == '\r'))
		{
		    x++;
		    continue;
		}
	    }

	    /* Skip other escape sequences */
	    if(s[x] == '\\')
	    {
		x++;
		if(x >= CFG_STRING_MAX)
		    break;
	    }

	    /* Stop on newline or NULL */
	    if((s[x] == '\0') || 
	       (s[x] == '\r') ||
	       (s[x] == '\n')
	    )
	    {
		value[y] = '\0';
		break;
	    }
	
	    if(got_value == 0)
	    {
		if(s[x] == CFG_PARAMETER_DELIMITER)
		{
		    got_value = 1;
		    continue;
		}
		else
		{
		    continue;
		}
	    }
		
	
	    value[y] = s[x];
	    y++;
	}

	/* Null terminate value */
	value[sizeof(value) - 1] = '\0';
	strstrip(value);

	return(value);
}

/*
 *	Parses a standard color string "#rrggbb" where rr, gg,
 *	and bb are in hexidecimal notation.
 *
 *	Returns 0 on success, -1 on general error, and
 *	-2 for incomplete or ambiguous.
 */
int StringParseStdColor(
	const char *s,
	u_int8_t *r_rtn,
	u_int8_t *g_rtn,
	u_int8_t *b_rtn
)
{
	int i, r = 0, g = 0, b = 0;

	if(s == NULL)
	    return(-1);

	/* Red */
	while((*s == '#') || ISBLANK(*s))
	    s++;
	if(!*s)
	    return(-2);

	i = 0;  
	while(isxdigit(*s) && (i < 2))
	{   
	    if(isdigit(*s))
		r = (r << 4) + (*s - '0');
	    else
		r = (r << 4) + (tolower(*s) - 'a' + 10);

	    i++; s++;
	}
	if(r_rtn != NULL)
	    *r_rtn = (u_int8_t)r;

	/* Green */
	i = 0;
	while(isxdigit(*s) && (i < 2))
	{
	    if(isdigit(*s))
		g = (g << 4) + (*s - '0');
	    else  
		g = (g << 4) + (tolower(*s) - 'a' + 10);

	    i++; s++;
	}
	if(g_rtn != NULL)
	    *g_rtn = (u_int8_t)g;

	/* Blue */
	i = 0;
	while(isxdigit(*s) && (i < 2))
	{
	    if(isdigit(*s))
		b = (b << 4) + (*s - '0');
	    else
		b = (b << 4) + (tolower(*s) - 'a' + 10);

	    i++; s++;
	}
	if(b_rtn != NULL)
	    *b_rtn = (u_int8_t)b;

	return(0);
}

/*
 *	Parse IP address.
 *
 *      Returns 0 on success, -1 on general error, and
 *      -2 for incomplete or ambiguous.
 */
int StringParseIP(
	const char *s,
	u_int8_t *c1, u_int8_t *c2, u_int8_t *c3, u_int8_t *c4 
)
{
	const char *cstrptr;
	char *strptr;
	char ls[4];


	if(s == NULL)
	    return(-1);

	while(ISBLANK(*s))
	    s++;

	if(*s == '\0')
	    return(-2);

	/* First number */
	if(c1 != NULL)
	{
	    strncpy(ls, s, 4); ls[3] = '\0';

	    strptr = strchr(ls, '.');
	    if(strptr != NULL)
		(*strptr) = '\0';

	    (*c1) = (u_int8_t)atoi(ls);
	}
	cstrptr = strchr(s, '.');
	if(cstrptr == NULL)
	    return(-2);
	s = cstrptr + 1;

	/* Second number */
	if(c2 != NULL)
	{
	    strncpy(ls, s, 4); ls[3] = '\0';

	    strptr = strchr(ls, '.');
	    if(strptr != NULL) 
		(*strptr) = '\0';

	    (*c2) = (u_int8_t)atoi(ls);
	}
	cstrptr = strchr(s, '.');
	if(cstrptr == NULL)
	    return(-2);
	s = cstrptr + 1;

	/* Third number */
	if(c3 != NULL)
	{
	    strncpy(ls, s, 4); ls[3] = '\0';

	    strptr = strchr(ls, '.');
	    if(strptr != NULL)
		(*strptr) = '\0';

	    (*c3) = (u_int8_t)atoi(ls);
	}
	cstrptr = strchr(s, '.');
	if(cstrptr == NULL)
	    return(-2);
	s = cstrptr + 1;

	/* Fourth number */
	if(c4 != NULL)
	{
	    strncpy(ls, s, 4); ls[3] = '\0';

	    strptr = strchr(ls, ' ');	/* Last, look for a space */
	    if(strptr != NULL)
		(*strptr) = '\0';

	    (*c4) = (u_int8_t)atoi(ls);
	}

	return(0);
}


/*
 *	ShipWars CS protocol command parsing.
 */
int StringGetNetCommand(const char *str)
{
	char *strptr;
	static char cmd_str[CS_DATA_MAX_LEN];

	if(str == NULL)
	    return(-1);

	strncpy(cmd_str, str, CS_DATA_MAX_LEN);
	cmd_str[CS_DATA_MAX_LEN - 1] = '\0';
	
	/* Get command */   
	strptr = strchr(cmd_str, ' ');
	if(strptr != NULL)
	    *strptr = '\0';

	return(atoi(cmd_str));
}

/*
 *	ShipWars CS protocol argument parsing.
 *
 *	Returns the argument from str with spaces stripped.
 *	This function never returns NULL.
 */
char *StringGetNetArgument(const char *str)
{
	char *strptr;
	static char arg[CS_DATA_MAX_LEN];

	if(str == NULL)
	    return("");

	strncpy(arg, str, CS_DATA_MAX_LEN);
	arg[CS_DATA_MAX_LEN - 1] = '\0';
	    
	/* Get argument */
	strptr = strchr(arg, ' ');
	if(strptr != NULL)
	{
	    strptr += 1;
	    strstrip(strptr);
	    return(strptr);
	}

	return("");
}


#ifndef MAX_TIME_STR
# define MAX_TIME_STR	256
#endif

/*
 *	Returns a formatted time string in accordance with given format
 *	format and returns the statically allocated string.
 *
 *	This function will never return NULL.
 */
char *StringCurrentTimeFormat(const char *format)
{
	size_t len;
	time_t current;
	struct tm *tm_ptr;
	static char s[MAX_TIME_STR];


	if(format == NULL)
	    return("");
	if((*format) == '\0')
	    return("");

	/* Get current time */
	time(&current);
	tm_ptr = localtime(&current);
	if(tm_ptr == NULL)
	    return("");

	/* Format time string */
	len = strftime(s, sizeof(s), format, tm_ptr);
	if(len >= sizeof(s))
	    len = sizeof(s) - 1;
	if(len < 0)
	    len = 0;
	/* Null terminate */
	s[len] = '\0';

	return(s);
}

/*
 *	Returns statically allocated string containing formatted
 *	values.
 */
char *StringTimeFormat(const char *format, time_t seconds)
{
	size_t len;
	struct tm *tm_ptr;
	static char s[MAX_TIME_STR];

	*s = '\0';

	if(STRISEMPTY(format))
	    return(s);

	tm_ptr = localtime(&seconds);
	if(tm_ptr == NULL)
	    return(s);

	/* Format time string */
	len = strftime(s, sizeof(s), format, tm_ptr);
	if(len >= sizeof(s))
	    len = sizeof(s) - 1;
	if(len < 0)
	    len = 0;
	/* Null terminate */
	s[len] = '\0';

	return(s);
}

/*
 *	Returns a statically allocated string containing a verbose
 *	statement of the delta time seconds.
 */
char *StringFormatTimePeriod(time_t seconds)
{
	static char s[MAX_TIME_STR];

	*s = '\0';

	if(seconds < 60)
	{
	    sprintf(
		s, "%ld sec%s",
		seconds,
		((seconds > 1) ? "s" : "")
	    );
	}
	else if(seconds < 3600)
	{
	    seconds = seconds / 60;
	    sprintf(
		s, "%ld min%s",
		seconds,
		((seconds > 1) ? "s" : "")
	    );
	}
	else if(seconds < 86400)
	{
	    seconds = seconds / 3600;
	    sprintf(
		s, "%ld hour%s",
		seconds,
		((seconds > 1) ? "s" : "")
	    );
	}
	else if(seconds < (7 * 86400))
	{
	    seconds = seconds / 86400;
	    sprintf(
		s, "%ld day%s",
		seconds,
		((seconds != 1) ? "s" : "")
	    );
	}
	else if(seconds < (28 * 86400))
	{
	    seconds = seconds / (7 * 86400);
	    sprintf(
		s, "%ld week%s",
		seconds,
		((seconds != 1) ? "s" : "")
	    );
	}
        else if(seconds < (365 * 86400))
        {
            seconds = seconds / (28 * 86400);
            sprintf(
                s, "%ld month%s",
                seconds,
                ((seconds != 1) ? "s" : "")
            );
        }     
	else
	{
	    seconds = seconds / (365 * 86400);
	    sprintf(
		s, "%ld year%s",
		seconds,
		((seconds != 1) ? "s" : "")
	    );
	}     


	s[sizeof(s) - 1] = '\0';

	return(s);
}




