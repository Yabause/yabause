/*
	       UNIX Standard Configuration File Format

	For reading and parsing the standard UNIX configuration
	file format.

	The format is:

	    <parameter>=<value>

	Spaces are allowed between the = character.
	All tailing and leading spaces will be removed.

	Lines beginning with a `#' character are consider comments.

 */


#ifndef CFGFMT_H
#define CFGFMT_H


/*
 *	Comment character:
 */
#ifndef UNIXCFG_COMMENT_CHAR
# define UNIXCFG_COMMENT_CHAR		'#'
#endif


/*
 *    Parameter and value delimiter:
 */
#ifndef CFG_PARAMETER_DELIMITER
    #define CFG_PARAMETER_DELIMITER	'='
#endif


/*
 *    Maximum length of Parameter:
 */
#ifndef CFG_PARAMETER_MAX
    #define CFG_PARAMETER_MAX		256
#endif

/*
 *    Maximum length of Value:
 */
#ifndef CFG_VALUE_MAX
    #define CFG_VALUE_MAX		1024
#endif

/*
 *    Maximum length of one line:
 */
#ifndef CFG_STRING_MAX
    #define CFG_STRING_MAX		CFG_PARAMETER_MAX + CFG_VALUE_MAX + 5
#endif




#endif /* CFGFMT_H */
