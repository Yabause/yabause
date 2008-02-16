/*
	  Disk IO, Directory Listing, and Path Parsing/Manipulations
*/


#ifndef DISK_H
#define DISK_H

#include <sys/types.h>
#include "os.h"

#if defined(_WIN32)
# include <direct.h>
# define mode_t unsigned short
# define S_ISREG(m) ((m) & S_IFREG)
# define S_ISDIR(m) ((m) & S_IFDIR)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Path deliminator */
#ifndef DIR_DELIMINATOR
# ifdef _WIN32
#  define DIR_DELIMINATOR	'\\'
# else
#  define DIR_DELIMINATOR	'/'
# endif
#endif

#if defined(_WIN32)
# define PATH_SEP_CHAR		'\\'
# define PATH_SEP_STR		"\\"
# define CWD_STR		".\\"
#else
# define PATH_SEP_CHAR		'/'
# define PATH_SEP_STR		"/"
# define CWD_STR		"./"
#endif


extern int FILEHASEXTENSION(const char *filename);
extern int ISPATHABSOLUTE(const char *path);
extern int NUMDIRCONTENTS(const char *path);

extern int COMPARE_PARENT_PATHS(const char *path, const char *parent);

extern int ISPATHDIR(const char *path);
extern int ISLPATHDIR(const char *path);
extern int ISPATHEXECUTABLE(const char *path);

extern int rmkdir(const char *path, mode_t m);

extern char *PathSubHome(const char *path);
extern char **GetDirEntNames2(const char *parent, int *total);
extern char **GetDirEntNames(const char *parent);
extern char *ChangeDirRel(const char *cpath, const char *npath);
extern void StripAbsolutePath(char *path);
extern void StripParentPath(char *path, const char *parent);

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" char *PrefixPaths(const char *parent, const char *child);
#else
extern char *PrefixPaths(const char *parent, const char *child);
#endif

extern char *GetAllocLinkDest(const char *link);
extern const char *GetParentDir(const char *path);

#define COMPLETE_PATH_SUCCESS		0
#define COMPLETE_PATH_NONE		-1
#define COMPLETE_PATH_AMBIGUOUS		-2
#define COMPLETE_PATH_PARTIAL		-3
extern char *CompletePath(char *path, int *status);

extern int FileCountLines(const char *filename);
extern int DirHasSubDirs(const char *path);
extern void StripPath(char *path);
extern void SimplifyPath(char *path);

extern int CopyObject(
	const char *target, const char *source,
	int (*comferm_func)(const char *, const char *)
);


#ifdef __cplusplus
}
#endif

#endif /* DISK_H */
