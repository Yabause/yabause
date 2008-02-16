#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "../include/os.h"

#if defined(_WIN32)
# include <io.h>	/* Needed by _findfirst(), _findnext(), etc */
#else
# include <unistd.h>
# include <sys/time.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#if !defined(USE_GETDENTS) && !defined(_WIN32)
# include <dirent.h>
#endif

#if defined(__linux__)
# include <linux/limits.h>

# ifdef USE_GETDENTS
#  include <linux/dirent.h>   
# endif
#endif

#include "../include/string.h"
#include "../include/disk.h"


int FILEHASEXTENSION(const char *filename);
int ISPATHABSOLUTE(const char *path);
int NUMDIRCONTENTS(const char *path);

int COMPARE_PARENT_PATHS(const char *path, const char *parent);

int ISPATHDIR(const char *path);
int ISLPATHDIR(const char *path);
int ISPATHEXECUTABLE(const char *path);

int rmkdir(const char *path, mode_t mode);

char *PathSubHome(const char *path);
char **GetDirEntNames2(const char *parent, int *total);
char **GetDirEntNames(const char *parent);
char *ChangeDirRel(const char *cpath, const char *npath);
void StripAbsolutePath(char *path);
void StripParentPath(char *path, const char *parent);
char *PrefixPaths(const char *parent, const char *child);
char *GetAllocLinkDest(const char *link);
const char *GetParentDir(const char *path);
void StripPath(char *path);
void SimplifyPath(char *path);

char *CompletePath(char *path, int *status);

int FileCountLines(const char *filename);
int DirHasSubDirs(const char *path);

int CopyObject(
	const char *target, const char *source,
	int (*confirm_func)(const char *, const char *)
);


#define MAX(a,b)        (((a) > (b)) ? (a) : (b))
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#define CLIP(a,l,h)     (MIN(MAX((a),(l)),(h)))
#define STRISEMPTY(s)	(((s) != NULL) ? (*(s) == '\0') : 1)
#define STRLEN(s)	(((s) != NULL) ? (int)strlen(s) : 0)
#define STRDUP(s)       (((s) != NULL) ? strdup(s) : NULL)


/*
 *	Checks if there is a '.' character in the file name.
 *
 *	Skips the very first '.' character if any.
 */
int FILEHASEXTENSION(const char *filename)
{
	if(STRISEMPTY(filename))
		return(0);

	filename++;		/* Skip the first char */

	if(strchr(filename, '.') != NULL)
	    return(1);
	else
	    return(0);
}

/*
 *	Checks if the specified path is an absolute path.
 */
int ISPATHABSOLUTE(const char *path)
{
#if defined(_WIN32)
	if(path == NULL)
		return(0);

	/* Has "X:" drive notation? */
	if((path[0] != '\0') ? (path[1] == ':') : 0)
	{
		/* Has drive notation, check if first char past the drive
		 * notation is a DIR_DELIMINATOR
		 */
		return(path[2] == DIR_DELIMINATOR);
	}
	else
	{
		/* No drive notation, check if first char is a '\\' */
		while(ISBLANK(*path))
			path++;
		return(*path == DIR_DELIMINATOR);
	}
#else
	if(path == NULL)
		return(0);

	while(ISBLANK(*path))
		path++;

	return(*path == DIR_DELIMINATOR);
#endif
}

/*
 *	Returns the number of entries in a dir.
 */
int NUMDIRCONTENTS(const char *path)
{
#if defined(_WIN32)
	int i, total = 0;
	const char *s;
	char **strv = GetDirEntNames(path);
	if(strv == NULL)
		return(total);

	for( i = 0; strv[i] != NULL; i++ )
	{
		s = strv[i];

		/* Skip special notations */
		if(!strcmp(s, ".") || !strcmp(s, ".."))
		{
			free(strv[i]);
			continue;
		}

		total++;		/* Count this directory entry */

		free(strv[i]);
	}

	free(strv);

	return(total);
#else
	DIR *dir;
	int total = 0;
	const char *s;
	struct dirent *de;

	if(path == NULL)
		return(total);

	/* Open directory for obtaining list of entries */
	dir = opendir(path);
	if(dir == NULL)
		return(total);

	/* Begin iterating through each directory entry */
	while(1 )
	{
		/* Get next directory entry */
		de = readdir(dir);
		if(de == NULL)
			break;

		/* Get pointer to directory entry name */
		s = de->d_name;

		/* Skip special notations */
		if(!strcmp(s, ".") || !strcmp(s, ".."))
			continue;

		total++;	/* Count this directory entry */
	}

	/* Close directory */
	closedir(dir);

	return(total);
#endif
}

/*
 *      Returns 1 if path has the parents of parent.
 *
 *      "/usr/X11/bin/xplay" "/usr/X11/bin"
 *      will return true. 
 *
 *      Spaces must be stripped from both!
 */
int COMPARE_PARENT_PATHS(const char *path, const char *parent)
{
	int len_path = STRLEN(path),
				   len_parent = STRLEN(parent);
	if((len_path <= 0) || (len_parent <= 0))
		return(0);

	/* Cannot compare if paths are not absolute */
	if(*path != DIR_DELIMINATOR )
		return(0);
	if(*parent != DIR_DELIMINATOR )
		return(0);

	/* Reduce length of parent for tailing DIR_DELIMINATOR chars */
	while(len_parent > 0)
	{
		if(parent[len_parent - 1] == DIR_DELIMINATOR )
			len_parent--;
		else
			break;
	}

	/* Definately not if parent is longer then path */
	if(len_parent > len_path )
		return(0);

	/* Compare, but just to the length of the parent */
	if(strncmp(path, parent, len_parent))
		return(0);
	else
		return(1);
}

int ISPATHDIR(const char *path)
{
	struct stat stat_buf;


	if(STRISEMPTY(path))
		return(0);

	/* Check if path exists */
	if(stat(path, &stat_buf))
		return(0);

	return((S_ISDIR(stat_buf.st_mode)) ? 1 : 0);
}

int ISLPATHDIR(const char *path)
{
#if defined(_WIN32)
	return(ISPATHDIR(path));
#else
	struct stat stat_buf;

	if(STRISEMPTY(path))
		return(0);

	/* Check if exists (do not follow links) */
	if(lstat(path, &stat_buf))
		return(0);

	return((S_ISDIR(stat_buf.st_mode)) ? 1 : 0);
#endif
}

int ISPATHEXECUTABLE(const char *path)
{
#if defined(_WIN32)
	char *s;

	if(STRISEMPTY(path))
		return(0);

	s = strrchr(path, '.');
	if(s == NULL)
		return(0);
	else
		s++;

	/* Check known MSW extensions for being executeable */
	if(!strcasecmp(s, "exe"))
		return(1);
	else if(!strcasecmp(s, "bat"))
		return(1);
	else if(!strcasecmp(s, "com"))
		return(1);
	else
		return(0);
#else
	struct stat stat_buf;


	if(STRISEMPTY(path))
		return(0);

	/* Get stats */
	if(stat(path, &stat_buf))
		return(0);


	if((S_IXUSR & stat_buf.st_mode) |
		(S_IXGRP & stat_buf.st_mode) |
		(S_IXOTH & stat_buf.st_mode)
	  )
		return(1);
	else
		return(0);
#endif
}

/*
 *	Creates the directory specified by path and any parent paths that
 *	do not exist.
 *
 *	Returns non-zero on error or 0 on success.
 */
int rmkdir(const char *path, mode_t m)
{
	char *s, *full_path;
	struct stat stat_buf;

	if(STRISEMPTY(path))
	{
#ifdef EINVAL
	    errno = EINVAL;
#endif
	    return(-1);
	}

	/* Need to get full absolute path */
	if(ISPATHABSOLUTE(path))
	{
	    full_path = STRDUP(path);
	}
	else
	{
	    char cwd[PATH_MAX];

	    /* Not an absolute path
	     *
	     * Get the current working directory and prefix it to the
	     * specified path
	     */
	    if(getcwd(cwd, sizeof(cwd)) != NULL)
		cwd[sizeof(cwd) - 1] = '\0';
	    else
		return(-1);

	    full_path = STRDUP(PrefixPaths(cwd, path));
	}
	if(full_path == NULL)
	{
#ifdef ENOMEM
	    errno = ENOMEM;
#endif
	    return(-3);
	}

	/* Begin creating each directory compoent by iterating through
	 * each path compoent in full_path
	 */

	/* Seek s past the toplevel deliminator in full_path */
#if defined(_WIN32)
	s = strchr(full_path, DIR_DELIMINATOR);
	if(s != NULL)
	    s++;
	else
	    s = full_path;
#else
	s = full_path + 1;
#endif
	while(s != NULL)
	{
	    /* Seek s to the next path compoent and temporarly set
	     * its deliminator to null
	     */
	    s = strchr(s, DIR_DELIMINATOR);
	    if(s != NULL)
		*s = '\0';

	    /* Does this directory need to be created? */
	    if(stat(full_path, &stat_buf) != 0)
	    {
		const int error_code = errno;
		if(error_code == ENOENT)
		{
#if defined(_WIN32)
		    if(mkdir(full_path))
		    {
			const int error_code = errno;
			free(full_path);
			errno = error_code;
			return(-1);
		    }
#else
		    if(mkdir(full_path, m))
		    {
			const int error_code = errno;
			free(full_path);
			errno = error_code;
			return(-1);
		    }
#endif
		}
		else
		{
		    free(full_path);
		    errno = error_code;
		    return(-1);
		}
	    }

	    /* Set back the directory deliminator and seek s to the
	     * next directory compoent
	     */
	    if(s != NULL)
	    {
		*s = DIR_DELIMINATOR;
		s++;
	    }
	}

	free(full_path);

	return(0);
}

/*
 *	Replaces any occurances of "~" in path with value of the
 *	HOME enviroment variable.
 *
 *	Returns a statically allocated string.
 */
char *PathSubHome(const char *path)
{
#if defined(_WIN32)
	static char rtn_path[PATH_MAX];

	if(STRISEMPTY(path))
		return(NULL);

	strncpy(rtn_path, path, sizeof(rtn_path));
	rtn_path[sizeof(rtn_path) - 1] = '\0';

	return(rtn_path);
#else
	static char rtn_path[PATH_MAX];

	if(STRISEMPTY(path))
		return(NULL);

	/* If the first character of the path is a '~' character then
	 * we need to substitute it with the user's home directory.
	 */
	if(*path == '~' )
	{
		size_t m = sizeof(rtn_path);
		const char *src = path + 1;
		const char *home = getenv("HOME");

		/* If no home dir defined then assume toplevel */
		if(home == NULL)
			home = "/";

		/* Reset return path */
		*rtn_path = '\0';

		/* Append home directory to return path */
		if(m > 0)
			strncat(rtn_path, home, m);

		/* Append source path to return path */
		m -= STRLEN(rtn_path);
		if(m > 0)
			strncat(rtn_path, src, m);

		rtn_path[sizeof(rtn_path) - 1] = '\0';
	} else
	{
		strncpy(rtn_path, path, sizeof(rtn_path));
		rtn_path[sizeof(rtn_path) - 1] = '\0';
	}

	return(rtn_path);
#endif
}


/*
 *	Returns a pointer to an array of pointers to strings.
 *	Last pointer points to NULL.
 *
 *	Returns NULL on error.   All contents are dynamically
 *	allocated and MUST BE FREED by calling program.
 */
#ifdef USE_GETDENTS
char **GetDirEntNames2(const char *parent, int *total)
{
	int x, bytes_read, fd;

	struct dirent *dirp_origin;
	struct dirent *dirp_pos;
	char *dirp_raw;
	long dirp_memsize;
	long next_dirent;

	int total_dnames;
	char **dname;


	if(total != NULL)
		*total = 0;

	if(parent == NULL)
		return(NULL);

	/* Check if parent is a directory */
	if(!ISPATHDIR(parent))
		return(NULL);


	/* Open parent */
	fd = open(parent, O_RDONLY);
	if(fd < 0)
		return(NULL);


	/* Begin reading dirs */
	next_dirent = 0;
	total_dnames = 0;
	dname = NULL;
	dirp_origin = NULL;
	dirp_memsize = sizeof(struct dirent);
	while(1 )
	{
		/* Reopen file descriptor */
		close(fd); fd = -1;
		fd = open(parent, O_RDONLY);
		if(fd < 0)
			break;

		/* Increment number of directory entry names gotten */
		total_dnames++;

		/* Allocate more directory entry name pointers */
		dname = (char **)realloc(dname, total_dnames * sizeof(char *));

		/* Allocate more dent pointers */
		dirp_origin = (struct dirent *)realloc(dirp_origin,
											   dirp_memsize);
		/* Reset to prevent segfault */
		memset(dirp_origin, 0, dirp_memsize);

		/* Get more directory entries */
		bytes_read = getdents((unsigned int)fd, dirp_origin,
							  dirp_memsize);
		if(bytes_read <= 0)
			break;

		/* Get raw FAT position pointer */
		dirp_raw = (char *)dirp_origin;
		dirp_pos = (struct dirent *)(&dirp_raw[next_dirent]);
		if(dirp_pos->d_reclen == 0)
			break;

		/* Get next dir entry position */
		next_dirent += dirp_pos->d_reclen;
		dirp_memsize += dirp_pos->d_reclen;	/* More memory needed */

		/* Allocate memory for new directory entry name */
		dname[total_dnames - 1] = (char *)calloc(
												1,
												(STRLEN(dirp_pos->d_name) + 1) * sizeof(char)
												);
		strcpy(dname[total_dnames - 1], dirp_pos->d_name);
	}

	/* Free dents pointer */
	free(dirp_origin); dirp_origin = NULL;

	/* Close the directory */
	if(fd > -1 )
	{
		close(fd);
		fd = -1;
	}

	/* Last entry for directory entry names must be NULL */
	if(total_dnames > 0)
		dname[total_dnames - 1] = NULL;
	else
		dname[0] = NULL;

	/* Update total return */
	if(total != NULL)
		*total = total_dnames;

	return(dname);
}

#else /* USE_GETDENTS */

char **GetDirEntNames2(const char *parent, int *total)
{
#if defined(_WIN32)
	long ffh;		/* Find file handle */
	struct _finddata_t d;	/* Find data return structure */
	char prev_cwd[PATH_MAX];


	if(total != NULL)
		*total = 0;

	/* Record previous current working dir */
	if(getcwd(prev_cwd, sizeof(prev_cwd)) != NULL)
		prev_cwd[sizeof(prev_cwd) - 1] = '\0';
	else
		*prev_cwd = '\0';

	/* Change to parent dir */
	if(parent != NULL)
		chdir(parent);

	/* Find first file in specified directory */
	ffh = _findfirst("*", &d);
	if(ffh == -1L )
	{
		chdir(prev_cwd);
		return(NULL);
	} else
	{
		/* Found first file in directory */
		int i = 0;
		char **rtn_names = (char **)malloc(sizeof(char *));


		if(rtn_names == NULL)
		{
			_findclose(ffh);
			chdir(prev_cwd);
			return(NULL);
		}

		rtn_names[i] = strdup(d.name);
		i++;

		/* Find additional (if any) files in directory */
		while(_findnext(ffh, &d) == 0)
		{
			rtn_names = (char **)realloc(rtn_names, (i + 1) * sizeof(char *));
			if(rtn_names == NULL)
			{
				_findclose(ffh);
				chdir(prev_cwd);
				return(NULL);
			}

			rtn_names[i] = strdup(d.name);
			i++;
		}

		/* Close find file handle and its resources */
		_findclose(ffh);
		chdir(prev_cwd);

		/* Allocate last pointer to be NULL */
		rtn_names = (char **)realloc(rtn_names, (i + 1) * sizeof(char *));
		if(rtn_names != NULL)
		{
			rtn_names[i] = NULL;
			if(total != NULL)
				*total = i;
		}

		return(rtn_names);
	}
#else
	int i;
	DIR *dir;
	struct dirent *de;
	char **rtn_names;


	if(total != NULL)
		*total = 0;

	if(parent == NULL)
		return(NULL);

	/* Open directory for obtaining list of entries */
	dir = opendir(parent);
	if(dir == NULL)
		return(NULL);

	/* Begin iterating through each directory entry */
	rtn_names = NULL;
	for( i = 0; 1; i++ )
	{
		/* Allocate more pointers in directory entry names array */
		rtn_names = (char **)realloc(
									rtn_names,
									(i + 1) * sizeof(char *)
									);
		if(rtn_names == NULL)
		{
			closedir(dir);
			return(NULL);
		}

		/* Get next directory entry */
		de = (struct dirent *)readdir(dir);

		/* End of directory entries? If so then break, leaving the
		 * newly allocated pointer index undefined where it will be
		 * set to NULL just outside the loop.
		*/
		if(de == NULL)
			break;

		/* Copy directory entry name to directory names array */
		rtn_names[i] = strdup(de->d_name);
	}

	/* Close directory */
	closedir(dir);

	/* Set last entry to NULL */
	rtn_names[i] = NULL;

	/* Update total */
	if(total != NULL)
		*total = i;

	return(rtn_names);
#endif	/* NOT _WIN32 */
}
#endif /* USE_GETDENTS */


char **GetDirEntNames(const char *parent)
{
	return(GetDirEntNames2(parent, NULL));
}


/*
 *	Performs a `change directory' operation on the given path
 *	statements cpath and npath.
 *
 *	cpath is treated as your current directory and npath is
 *	treated as the operation.
 *
 *	Returns NULL on error or an allocated NULL terminated string on
 *	success.  The calling function must free the returned pointer.
 */
char *ChangeDirRel(const char *cpath, const char *npath)
{
	int len;
	char *rtn_str, *s;


	/* If both cpath and npath are NULL, return allocated cwd */
	if((cpath == NULL) &&
		(npath == NULL)
	  )
	{
		len = PATH_MAX;
		rtn_str = (char *)malloc((len + 1) * sizeof(char));
		if(rtn_str == NULL)
			return(NULL);

		if(getcwd(rtn_str, len) == NULL)
		{
			free(rtn_str);
			return(NULL);
		} else
		{
			rtn_str[len] = '\0';
			return(rtn_str);
		}
	}
	/* If only npath is NULL, return allocated cpath */
	if((cpath != NULL) && (npath == NULL))
	{
		len = STRLEN(cpath);
		rtn_str = (char *)malloc((len + 1) * sizeof(char));
		if(rtn_str == NULL)
			return(NULL);

		strncpy(rtn_str, cpath, len);
		rtn_str[len] = '\0';

		return(rtn_str);
	}
	/* If only cpath is NULL, return allocated cwd */
	if((npath != NULL) && (cpath == NULL))
	{
		len = PATH_MAX;
		rtn_str = (char *)malloc((len + 1) * sizeof(char));
		if(rtn_str == NULL)
			return(NULL);

		if(getcwd(rtn_str, len) == NULL)
		{
			free(rtn_str); 
			return(NULL);
		} else
		{
			rtn_str[len] = '\0';
			return(rtn_str);
		}
	}


	/* If cpath is not absolute, then return cwd */
	if(!ISPATHABSOLUTE(cpath))
	{
		len = PATH_MAX;
		rtn_str = (char *)malloc((len + 1) * sizeof(char));
		if(rtn_str == NULL)
			return(NULL);

		if(getcwd(rtn_str, len) == NULL)
		{
			free(rtn_str);
			return(NULL);
		} else
		{
			rtn_str[len] = '\0';
			return(rtn_str);
		}
	}

	/* If npath is ".", then just return cpath */
	if(!strcmp(npath, "."))
	{
		len = STRLEN(cpath);
		rtn_str = (char *)malloc((len + 1) * sizeof(char));
		if(rtn_str == NULL)
			return(NULL);

		strncpy(rtn_str, cpath, len);
		rtn_str[len] = '\0';

		return(rtn_str);
	}

	/* If npath is an absolute directory, then change to it */
	if(ISPATHABSOLUTE(npath))
	{
		len = STRLEN(npath);
		rtn_str = (char *)malloc((len + 1) * sizeof(char));
		if(rtn_str == NULL)
			return(NULL);

		strncpy(rtn_str, npath, len);
		rtn_str[len] = '\0';

		return(rtn_str);
	}


	/* npath is a relative notation */

	/* Prefix paths */
	s = PrefixPaths(cpath, npath);
	if(s == NULL)
		return(NULL);

	/* Allocate return string */
	len = STRLEN(s);
	rtn_str = (char *)malloc((len + 1) * sizeof(char));
	if(rtn_str == NULL)
		return(NULL);

	strncpy(rtn_str, s, len);
	rtn_str[len] = '\0';


	/* Simplify return path, this removes any "whatever/.."s */
	SimplifyPath(rtn_str);


	return(rtn_str);
}

/*
 *	Strips the specified absolute path of its absolute path,
 *	leaving it with just the final destination name.
 */
void StripAbsolutePath(char *path)
{
	char *s1, *s2;

	if(path == NULL)
		return;

	if(*path != DIR_DELIMINATOR )
		return;

	s1 = path;

	/* Seek to end */
	while(*s1 != '\0' )
		s1++;

	/* Seek back, skipping tailing DIR_DELIMINATOR chars */
	s1--;
	if(s1 < path )
		s1 = path;
	while((s1 > path) && (*s1 == DIR_DELIMINATOR))
		s1--;

	/* Seek back untill a DIR_DELIMINATOR char */
	if(s1 < path )
		s1 = path;
	while((s1 > path) && (*s1 != DIR_DELIMINATOR))
		s1--;

	s1++;	   /* Go forward one char */

	if(s1 < path )
		s1 = path;

	/* Copy child to beginning */
	s2 = path;
	while(*s1 != '\0' )
		*s2++ = *s1++;
	*s2 = '\0';

	/* Make sure the path contains atleast toplevel */
	if(*path == '\0' )
	{
		path[0] = DIR_DELIMINATOR;
		path[1] = '\0';
	}
}

/*
 *	Checks if parent is under the specified path, if it is then it
 *	will be removed.
 *
 *	Example:
 *
 *	path = "/var/spool/mail"
 *	parent = "/var"
 *
 *	Then path becomes "spool/mail"
 */
void StripParentPath(char *path, const char *parent)
{
	char *s;

	if((path == NULL) || (parent == NULL))
		return;

	/* Both given paths must be absolute */
	if(!ISPATHABSOLUTE(path) || !ISPATHABSOLUTE(parent))
		return;

	if(!COMPARE_PARENT_PATHS(path, parent))
		return;

	/* Remove parent from path */
	substr(path, parent, "");

	/* Need to remove any prefixing DIR_DELIMINATOR characters */
	while(*path == DIR_DELIMINATOR )
	{
		s = path;
		while(*s != '\0')
		{
			*s = *(s + 1);
			++s;
		}
	}

	/* If path is empty, it implies the parent and path were
	 * the same
	 */
	if(*path == '\0' )
		strcpy(path, parent);
}

/*
 *	Returns a statically allocated string containing the
 *	prefixed paths of the parent and child.
 *
 *	If child is absolute, then the statically allocated string
 *	will contain the child path.
 *
 *	The parent path must be absolute.
 */
char *PrefixPaths(const char *parent, const char *child)
{
	char *s, *s_end;
	const char *s2;
	static char rtn_path[PATH_MAX];

	if((parent == NULL) || (child == NULL) || (parent == child))
		return("/");

	/* If child is absolute, copy child and return */
	if(*child == DIR_DELIMINATOR )
	{
		strncpy(rtn_path, child, sizeof(rtn_path));
		rtn_path[sizeof(rtn_path) - 1] = '\0';
		return(rtn_path);
	}

	/* Get the end of rtn_path */
	s_end = rtn_path + sizeof(rtn_path);

	/* Copy the parent to rtn_path */
	strncpy(rtn_path, parent, sizeof(rtn_path));
	rtn_path[sizeof(rtn_path) - 1] = '\0';

	/* Seek s to end of parent path in rtn_path */
	s = rtn_path;
	while(*s != '\0' )
		s++;

	/* Insert deliminator to rtn_path */
	if(s > rtn_path )
	{
		/* Does the parent portion of rtn_path does not already
		 * have a tailing deliminator?
		 */
		if(*(s - 1) != DIR_DELIMINATOR )
		{
			*s++ = DIR_DELIMINATOR;
		}
	} else
	{
		s = rtn_path;
	}

	/* Copy child */
	s2 = child;
	while((s < s_end) && (*s2 != '\0'))
		*s++ = *s2++;
	if(s < s_end )
		*s = '\0';
	else
		rtn_path[sizeof(rtn_path) - 1] = '\0';

	/* Make sure rtn_path contains atleast "/" */
	if(*rtn_path == '\0' )
	{
		rtn_path[0] = DIR_DELIMINATOR;
		rtn_path[1] = '\0';
	}

	return(rtn_path);
}


/*
 *	Returns an allocated string containing the link's destination
 *	or NULL on error or if link is not really a link.
 *
 *	The pointer returned must be free()ed at by the
 *	calling function.
 */
char *GetAllocLinkDest(const char *link)
{
#if defined(_WIN32)
	return(NULL);
#else
	int bytes_read;
	char *dest;
	struct stat lstat_buf;

	if(link == NULL)
		return(NULL);
	if(lstat(link, &lstat_buf))
		return(NULL);
#ifdef S_ISLNK
	if(!S_ISLNK(lstat_buf.st_mode))
		return(NULL);
#endif

	/* Allocate link destination buffer */
	dest = (char *)calloc(1, (PATH_MAX + NAME_MAX + 1) * sizeof(char));
	if(dest == NULL)
		return(NULL);

	/* Read link's destination */
	bytes_read = readlink(link, dest, PATH_MAX + NAME_MAX);
	if(bytes_read <= 0)
	{
		/* Empty dest buffer, return an allocated but empty string */
		dest[0] = '\0';
		return(dest);
	}

	if(bytes_read > PATH_MAX + NAME_MAX )
		bytes_read = PATH_MAX + NAME_MAX;
	if(bytes_read < 0)
		bytes_read = 0;

	/* Null terminate */
	dest[bytes_read] = '\0';

	return(dest);
#endif	/* NOT _WIN32 */
}


/*
 *	Completes the path.
 *
 *	The path specifies the string describing the path to complete,
 *	this string may be modified by this function. The path must
 *	be an absolute path. In the case of an empty path, the path
 *	will be returned describing toplevel.
 *
 *	If status is not NULL then *status will be set to one of
 *	the COMPLETE_PATH_* status codes:
 *
 *	COMPLETE_PATH_SUCCESS		Completed path fully to a valid
 *					object.
 *	COMPLETE_PATH_NONE		General error or not completed.
 *	COMPLETE_PATH_AMBIGUOUS		Completed path fully to a valid
 *					object but there are other
 *					objects a with similar name.
 *	COMPLETE_PATH_PARTIAL		Path partially completed.
 *
 *	Returns the reallocated path on success or NULL on error.
 */
char *CompletePath(char *path, int *status)
{
#if 1
/* Version 2 */
	int i, nmatches;
	const char *s;
	char *name, *parent, *matched_name, **strv;

	/* If the path is an empty string then set it to describe
	 * toplevel
	 */
	if(STRISEMPTY(path))
	{
	    if(status != NULL)
		*status = COMPLETE_PATH_SUCCESS;
	    free(path);
#if defined(_WIN32)
	    path = STRDUP("C:\\");
#else
	    path = STRDUP("/");
#endif
	    return(path);
	}

	/* If the path has the home directory prefix then substitute
	 * it when the actual home directory
	 */
	if(*path == '~')
	{
	    int len;
	    char *s;
	    const char	*path2 = path + 1,
			*home = getenv("HOME");
	    if(home == NULL)
#if defined(_WIN32)
		home = "\\";
#else
		home = "/";
#endif
	    len = STRLEN(home) + 1 + STRLEN(path2);
	    s = (char *)malloc((len + 1) * sizeof(char));
	    if(s == NULL)
	    {
		if(status != NULL)
		    *status = COMPLETE_PATH_NONE;
		errno = ENOMEM;
		return(path);
	    }
	    strcpy(s, home);
	    if(*path2 != DIR_DELIMINATOR)
	    {
#if defined(_WIN32)
		strcat(s, "\\");
#else
		strcat(s, "/");
#endif
	    }
	    strcat(s, path2);
	    free(path);
	    path = s;
	}

	/* The path to complete must be an absolute path */
	if(!ISPATHABSOLUTE(path))
	{
	    if(status != NULL)
		*status = COMPLETE_PATH_NONE;
	    errno = EINVAL;
	    return(path);
	}

	/* Get the name of the last path segment, this is what we
	 * need to complete
	 */
	s = strrchr(path, DIR_DELIMINATOR);
	if(s != NULL)
	    s++;
	else
	    s = "";
	name = STRDUP(s);
	if(name == NULL)
	{
	    if(status != NULL)
		*status = COMPLETE_PATH_NONE;
	    errno = ENOMEM;
	    return(path);
	}

	/* Get the parent path
	 *
	 * If the name is NULL then that implies the path itself is
	 * the parent, otherwise get the parent of the path
	 */
	if(STRISEMPTY(name))
	{
	    parent = STRDUP(path);
	    StripPath(parent);		/* Need to strip tailing deliminators */
	}
	else
	{
	    parent = STRDUP(GetParentDir(path));
	}
	if(parent == NULL)
	{
	    if(status != NULL)
		*status = COMPLETE_PATH_NONE;
	    free(name);
	    errno = ENOMEM;
	    return(path);
	}

	/* Get the listing of object names in the parent path */
	strv = GetDirEntNames(parent);
	if(strv == NULL)
	{
	    /* No object names means no matches */
	    if(status != NULL)
		*status = COMPLETE_PATH_NONE;
	    free(name);
	    free(parent);
	    errno = ENOENT;
	    return(path);
	}

	/* Begin searching for match */
	nmatches = 0;
	matched_name = NULL;
	for(i = 0; strv[i] != NULL; i++)
	{
	    s = strv[i];

	    /* Skip special directory notations */
	    if(!strcmp(s, ".") ||
	       !strcmp(s, "..")
	    )
	    {
		free(strv[i]);
		continue;
	    }

	    /* If the specified name is empty then always match */
	    if(STRISEMPTY(name))
	    {
		nmatches++;

		/* Was there already a match? */
		if(matched_name != NULL)
		{
		    /* Shorten the matched name to the number of
		     * characters that coencide with this object's
		     * name
		     */
		    const char *src = s;
		    char *tar = matched_name;
		    while(*tar != '\0')
		    {
			if(*tar != *src)
			{
			    *tar = '\0';
			    break;
			}
			tar++;
			src++;
		    }
		}
		else
		{
		    matched_name = STRDUP(s);
		}
	    }

	    /* Does this object's name partially match the specified
	     * name?
	     */
#if defined(_WIN32)
	    if(strcasepfx(s, name))
#else
	    if(strpfx(s, name))
#endif
	    {
		nmatches++;

		/* Was there already a match? */
		if(matched_name != NULL)
		{
		    /* Shorten the matched name to the number of
		     * characters that coencide with this object's
		     * name
		     */
		    const char *src = s;
		    char *tar = matched_name;
		    while(*tar != '\0')
		    {
			if(*tar != *src)
			{
			    *tar = '\0';
			    break;
			}
			tar++;
			src++;
		    }
		}
		else
		{
		    matched_name = STRDUP(s);
		}
	    }

	    free(strv[i]);
	}
	free(strv);

	/* Got match? */
	if(matched_name != NULL)
	{
	    /* Update the path */
	    free(path);
	    path = STRDUP(PrefixPaths(parent, matched_name));
	    if(path == NULL)
	    {
		if(status != NULL)
		    *status = COMPLETE_PATH_NONE;
		free(name);
		free(parent);
		free(matched_name);
		errno = ENOMEM;
		return(NULL);
	    }

	    free(matched_name);

	    /* If exactly one name was matched and it is a directory
	     * then tack on a tailing directory deliminator to the
	     * path
	     */
	    if(nmatches == 1)
	    {
		if(ISPATHDIR(path))
		    path = strinschr(path, -1, DIR_DELIMINATOR);
	    }
	    if(path == NULL)
	    {
		if(status != NULL)
		    *status = COMPLETE_PATH_NONE;
		free(name);
		free(parent);
		errno = ENOMEM;
		return(NULL);
	    }

	    /* Set the status */
	    if(status != NULL)
	    {
		if(nmatches > 1)
		{
		    struct stat stat_buf;
		    if(!lstat(path, &stat_buf))
			*status = COMPLETE_PATH_AMBIGUOUS;
		    else
			*status = COMPLETE_PATH_PARTIAL;
		}
		else
		{
		    *status = COMPLETE_PATH_SUCCESS;
		}
	    }
	}
	else
	{
	    /* No matches */
	    if(status != NULL)
		*status = COMPLETE_PATH_NONE;
	}

	free(name);
	free(parent);

	return(path);
#else
	const char *cur_child;

	if(!ISPATHABSOLUTE(path))
	{
		if(status != NULL)
			*status = COMPLETE_PATH_NONE;
		return(NULL);
	}

	cur_child = strrchr(path, DIR_DELIMINATOR);
	if(cur_child != NULL)
		cur_child++;

	if(!STRISEMPTY(cur_child))
	{
		/* Get parent directory and listing of all child objects
		 * to complete the path
		 */
		int i, strc, matches = 0;
		char *name, **matches_list = NULL;
		char **strv = GetDirEntNames2(GetParentDir(path), &strc);

		/* Get match list containing all object names that have
		 * cur_child as their prefix and then delete the object
		 * names list
		 */
		for( i = 0; i < strc; i++ )
		{
			name = strv[i];
			if(name == NULL)
				continue;

			if(!strcmp(name, ".") ||
			   !strcmp(name, "..")
			)
			{
				free(name);
				continue;
			}

			if(strpfx(name, cur_child))
			{
				/* Append this name to the matches list */
				int n = matches;
				matches = n + 1;
				matches_list = (char **)realloc(
					matches_list,
					matches * sizeof(char *)
				);
				matches_list[n] = STRDUP(name);
			}

			free(name);
		}
		free(strv);

		/* Got exactly one match? */
		if(matches == 1 )
		{
			char *new_path = STRDUP(PrefixPaths(
				GetParentDir(path),
				matches_list[0]
			));
			free(path);
			path = new_path;
			if(ISPATHDIR(path))
				path = strcatalloc(path, "/");
			if(status != NULL)
				*status = COMPLETE_PATH_SUCCESS;
		}
		/* Got multiple matches? */
		else if(matches > 1 )
		{
			/* Iterate through multiple matches and get len to be
			 * the number of the most common characters in all of
			 * the matches
			 */
			int n, k, len = 0;
			const char *ss, *st;
			for( i = 0; i < matches; i++ )
			{
				ss = matches_list[i];
				if(ss == NULL)
					continue;

				for( n = 0; n < matches; n++ )
				{
					if(n == i )
						continue;

					st = matches_list[n];
					if(st == NULL)
						continue;

					/* Calculate the number of common characters
					 * k in both name strings
					 */
					for( k = 0; ss[k] != '\0'; k++ )
					{
						if(ss[k] != st[k] )
							break;
					}

					/* Record length if it is longer than the
					 * previous length
					 */
					if((len > 0) ? (k < len) : 1)
						len = k;
				}
			}

			/* Is the length of the common characters shorter than
			 * or equal to the child's name?
			 */
			if(len <= STRLEN(cur_child))
			{
				if(status != NULL)
					*status = COMPLETE_PATH_AMBIGUOUS;
			}
			else
			{
				char *new_path,
				*new_child = STRDUP(matches_list[0]);
				new_child[len] = '\0';
				new_path = STRDUP(PrefixPaths(
				    GetParentDir(path), new_child
				));
				free(path);
				path = new_path;
				free(new_child);
				if(status != NULL)
					*status = COMPLETE_PATH_PARTIAL;
			}
		}
		/* No matches */
		else
		{
			if(status != NULL)
				*status = COMPLETE_PATH_NONE;
		}

		/* Delete the matched names list */
		for( i = 0; i < matches; i++ )
			free(matches_list[i]);
		free(matches_list);
	} else {
		/* Child name is empty so get listing of object names and
		 * only set match if there is exactly one object
		 */
		int i, strc, matches = 0;
		char *name, *match = NULL;
		char **strv = GetDirEntNames2(path, &strc);

		for( i = 0; i < strc; i++ )
		{
			name = strv[i];
			if(name == NULL)
				continue;

			if(!strcmp(name, ".") ||  
			   !strcmp(name, "..")
			)
			{
				free(name);
				continue;
			}

			if(match == NULL)
				match = STRDUP(name);
			matches++;

			free(name);
		}
		free(strv);

		/* Got exactly one match? */
		if(matches == 1 )
		{
			char *new_path = STRDUP(PrefixPaths(
				path, match
			));
			free(path);
			path = new_path;
			if(ISPATHDIR(path))
				path = strcatalloc(path, "/");
			if(status != NULL)
				*status = COMPLETE_PATH_SUCCESS;
		}
		else
		{
			if(status != NULL)
				*status = COMPLETE_PATH_AMBIGUOUS;
		}

		/* Delete the matched name */
		free(match);
	}

	return(path);
#endif
}


/*
 *	Returns the number of lines in the text file.
 */
int FileCountLines(const char *filename)
{
	int c, lines = 0;
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL)
	    return(lines);

	/* Start counting the lines */
	c = fgetc(fp);
	while(c != EOF)
	{
	    if((c == '\n') || (c == '\r'))
		lines++;

	    c = fgetc(fp);
	}

	fclose(fp);

	return(lines);
}


/*
 *	Returns true if the path is a directory and contains
 *	subdirectories, otherwise it returns false.
 *
 *	The given path must be an absolute path.
 */
int DirHasSubDirs(const char *path)
{
	const char *name, *child_path;
	DIR *dir;
	struct dirent *dent;
	struct stat stat_buf;

	if(STRISEMPTY(path))
	    return(0);

	/* Open the directory */
	dir = opendir(path);
	if(dir == NULL)
	    return(0);

	/* Check if there is a subdirectory */
	for(dent = readdir(dir); dent != NULL; dent = readdir(dir))
	{
	    name = dent->d_name;

	    /* Skip special notations */
	    if(!strcmp(name, ".") ||
	       !strcmp(name, "..")
	    )
		continue;

	    /* Get the full path to this object */
	    child_path = PrefixPaths(path, name);
	    if(child_path == NULL)
		continue;

	    /* Does this object lead to a directory? */
	    if(!stat(child_path, &stat_buf))
	    {
		if(S_ISDIR(stat_buf.st_mode))
		{
		    /* Close the directory and return true */
		    closedir(dir);
		    return(1);
		}
	    }
	}

	/* Close the directory */
	closedir(dir);

	return(0);
}


/*
 *	Gets the parent.
 *
 *	The path specifies the string describing the absolute path.
 *
 *	Returns a statically allocated string describing the parent
 *	directory of the specified path or NULL on error.
 */
const char *GetParentDir(const char *path)
{
	char *s;
	static char parent[PATH_MAX];

	if(path == NULL)
	    return(NULL);

	strncpy(parent, path, sizeof(parent));
	parent[sizeof(parent) - 1] = '\0';

	/* Seek to last deliminator character (which may be a tailing
	 * deliminator)
	 */
	s = strrchr(parent, DIR_DELIMINATOR);
	if(s == NULL)
	    return(parent);

	/* Strip any tailing deliminators */
#if defined(_WIN32)
	while(s >= (parent + 3))
#else
	while(s >= (parent + 1))
#endif
	{
	    if(*(s + 1) != '\0' )
		break;

	    *s = '\0';
	    s = strrchr(parent, DIR_DELIMINATOR);
	}

	/* Get the parent */
#if defined(_WIN32)
	if(s >= (parent + 3))
	    *s = '\0';
	else
	    parent[3] = '\0';
#else
	if(s >= (parent + 1))
	    *s = '\0';
	else
	    parent[1] = '\0';
#endif

	return(parent);
}


/*
 *	Strips tailing deliminator characters found in the given path.
 */
void StripPath(char *path)
{
	char *s;


	if(path == NULL)
		return;

	s = strrchr(path, DIR_DELIMINATOR);
	if(s != NULL)
	{
		while((*(s + 1) == '\0') &&
			   (s > path)
			 )
		{
			if(*s != DIR_DELIMINATOR )
				break;

			*s = '\0';
			s--;
		}
	}
}


/*
 *	Simplifies the path.
 *
 *	Removes any tailing deliminators and reduces any occurances
 *	of "/.." or "/.".
 *
 *	The path specifies the string describing the path to
 *	simplify. The path must be an absolute path.
 */
void SimplifyPath(char *path)
{
	char was_absolute_path;
	char *s;

	if(STRISEMPTY(path))
	    return;

	was_absolute_path = ISPATHABSOLUTE(path);

	/* Seek s to first path compoent */
	for(s = path; *s == DIR_DELIMINATOR; s++);

	while(*s != '\0')
	{
		/* This compoent is a parent directory notation? */
		if(strpfx(s, "../") || !strcmp(s, ".."))
		{
			/* Seek s2 to start of previous path compoent (if any) */
			char *s2 = s - 1;
			while((s2 >= path) ? (*s2 == DIR_DELIMINATOR) : 0)
				s2--;
			while((s2 >= path) ? (*s2 != DIR_DELIMINATOR) : 0)
				s2--;
			s2++;

			/* Seek s to next compoent */
			while((*s != '\0') && (*s != DIR_DELIMINATOR))
				s++;
			while(*s == DIR_DELIMINATOR)
				s++;

			if(s2 < s)
			{
				/* Shift s position to s2 position */
				char *s3 = s;
				while(*s3 != '\0')
					*s2++ = *s3++;
				*s2 = *s3;
			}

			s = path;		/* Reset s position */
			while(*s == DIR_DELIMINATOR)
				s++;
		}
		/* This compoent is a current directory notation? */
		else if(strpfx(s, "./") || !strcmp(s, "."))
		{
			char *s2 = s;

			/* Seek s to next compoent */
			while((*s != '\0') && (*s != DIR_DELIMINATOR))
				s++;
			while(*s == DIR_DELIMINATOR)
				s++;

			if(s2 < s )
			{
				/* Shift s position to s2 position */
				char *s3 = s;
				while(*s3 != '\0')
					*s2++ = *s3++;
				*s2 = *s3;
			}

			s = path;				/* Reset s position */
			while(*s == DIR_DELIMINATOR)
				s++;
		}
		else
		{
			/* Seek s to next compoent */
			while((*s != '\0') && (*s != DIR_DELIMINATOR))
				s++;
			while(*s == DIR_DELIMINATOR )
				s++;
		}
	}

	/* Strip tailing deliminators (s should already be at the
	 * end of path)
	 */
	while(s > (path + 1))
	{
		s--;
		if(*s == DIR_DELIMINATOR)
			*s = '\0';
		else
			break;
	}

	/* Restore the absolute path as needed */
	if(was_absolute_path)
	{
		/* Resulted in empty string? */
		if(*path == '\0')
			strcpy(path, "/");
	}
}


/*
 *	Coppies object source to target.
 *
 *	If target exists and confirm_func is not NULL, then
 *	confirm_func will be called, given the target and
 *	source (respectivly) and return is checked.
 *
 *	Return of any true value from confirm_func will mean overwrite,
 *	and return of 0 from confirm_func will mean do not overwrite.
 */
int CopyObject(
	const char *target, const char *source,
	int (*confirm_func)(const char *, const char *)
)
{
	size_t blksize;
	FILE *tar_fp, *src_fp;
	struct stat stat_buf;

	if((target == NULL) || (source == NULL))
	    return(-2);

	/* Check if target exists */
#if defined(_WIN32)
	if(!stat(target, &stat_buf))
#else
	if(!lstat(target, &stat_buf))
#endif
	{
	    if(confirm_func != NULL)
	    {
		/* Overwrite? */
		if(!confirm_func(target, source))
		    return(-3);
	    }
	}

	/* Open target for writing and source for reading */
	tar_fp = fopen(target, "wb");
	if(tar_fp == NULL)
	    return(-1);
	src_fp = fopen(source, "rb");
	if(src_fp == NULL)
	{
	    fclose(tar_fp);
	    return(-1);
	}

	/* Get block size of the source */
	if(fstat(fileno(src_fp), &stat_buf))
	    blksize = 0l;
	else
	    blksize = stat_buf.st_blksize;
	if(blksize > 0l)
	{
	    /* Copy block by block */
	    void *buf = malloc(blksize * sizeof(char));
	    if(buf != NULL)
	    {
		size_t len = fread(buf, sizeof(char), blksize, src_fp);
		while(len > 0)
		{
		    if(fwrite(buf, sizeof(char), len, tar_fp) < len)
			break;

		    len = fread(buf, sizeof(char), blksize, src_fp);
		}
		free(buf);
	    }
	}
	else
	{
	    /* Block size not specified, copy byte by byte instead */
	    int c = fgetc(src_fp);
	    while(c != EOF)
	    {
		if(fputc(c, tar_fp) == EOF)
		    break;

		c = fgetc(src_fp);
	    }
	}

	/* Close target and source */
	fclose(tar_fp);
	fclose(src_fp);

	return(0);
}
