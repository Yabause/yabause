#ifndef DEBUG_H
#define DEBUG_H

#include "core.h"
#include <stdio.h>

typedef enum { DEBUG_STRING, DEBUG_STREAM , DEBUG_STDOUT, DEBUG_STDERR } DebugOutType;

typedef struct {
	DebugOutType output_type;
	union {
		FILE * stream;
		char * string;
	} output;
	char * name;
} Debug;

Debug * DebugInit(const char *, DebugOutType, char *);
void DebugDeInit(Debug *);

void DebugChangeOutput(Debug *, DebugOutType, char *);

void DebugPrintf(Debug *, const char *, u32, const char *, ...);

extern Debug * MainLog;

void LogStart(void);
void LogStop(void);

#ifdef DEBUG
#define LOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG(...)
#endif

#ifdef CDDEBUG
#define CDLOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define CDLOG(...)
#endif

#ifdef SCSP_DEBUG
#define SCSPLOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define SCSPLOG(...)
#endif

#ifdef VDP1_DEBUG
#define VDP1LOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define VDP1LOG(...)
#endif

#ifdef VDP2_DEBUG
#define VDP2LOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define VDP2LOG(...)
#endif

#ifdef SMPC_DEBUG
#define SMPCLOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define SMPCLOG(...)
#endif

#endif
