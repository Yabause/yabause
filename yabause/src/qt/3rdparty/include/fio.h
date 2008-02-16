#ifndef FIO_H
#define FIO_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int FPending(FILE *fp);

extern FILE *FOpen(const char *path, const char *mode);
extern void FClose(FILE *fp);

extern void FSeekNextLine(FILE *fp);
extern void FSeekPastSpaces(FILE *fp);
extern void FSeekPastChar(FILE *fp, char c);

extern int FSeekToParm(FILE *fp, const char *parm, char comment, char delimiator);
extern char *FSeekNextParm(FILE *fp, char *buf, char comment, char delim);

extern int FGetValuesI(FILE *fp, int *value, int nvalues);
extern int FGetValuesL(FILE *fp, long *value, int nvalues);
extern int FGetValuesF(FILE *fp, double *value, int nvalues);
extern char *FGetString(FILE *fp);
extern char *FGetStringLined(FILE *fp);
extern char *FGetStringLiteral(FILE *fp);

extern char *FReadNextLineAlloc(FILE *fp, char comment);

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" char *FReadNextLineAllocCount(
	FILE *fp,
	char comment,
	int *line_count
);
#else
extern char *FReadNextLineAllocCount(
	FILE *fp,
	char comment,
	int *line_count
);
#endif


#ifdef __cplusplus
}
#endif

#endif	/* FIO_H */

