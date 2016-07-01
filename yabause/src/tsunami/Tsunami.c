/* 
   Tsunami.c
*/
/* 
   The MIT License (MIT)
   
   Copyright (c) 2014 James A. McCombe

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.   
*/

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#if _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#if _WIN32

#else
#include <pthread.h>
#endif

#include "Tsunami.h"

#if TSUNAMI_ENABLE

/* ================================================================================ */
/* Tsunami Internal Logging */
/* Tsunami maintains a compact internal log of the value changes over time in
   memory.  It stores this in a circular buffer of a user-defined size.  This 
   allows for a fixed sliding window of time to be captured.  Once the application
   finishes the timeline, it will dump this internal buffer to a VCD file on disk
   so that the data can be viewed using external tools.                             */
typedef enum _TsunamiLogEntryType {
	TsunamiLogEntryType_NewTime = 0,
	TsunamiLogEntryType_ChangeValue,
} TsunamiLogEntryType;

typedef struct _TsunamiLogEntry {
	TsunamiLogEntryType  type;
	
	union {
		struct {
			uint64_t         time;
		} new_time;

		struct {
			TsunamiVariable *var;
			uint64_t         value;
		} change_value;
	};

} TsunamiLogEntry;
/* ================================================================================ */

/* ================================================================================ */
/* Structure - A Tsunami "timeline" context.                                        */
struct _TsunamiTimeline {
	
	char *name;        /* Name of the timeline */
	char *filename;    /* VCD filename to write the timeline to on Finish() */

	struct {
		TsunamiLogEntry *entry;             /* Log storage buffer              */
		uint32_t         entry_alloc_count; /* Number of log entries allocated */
		uint32_t         entry_start_index; /* Start of circular buffer        */
		uint32_t         entry_last_index;  /* End of the circular buffer      */
		uint32_t         entry_count;       /* Number of active entries        */
	} log;
	
	TsunamiVariable var_root;  /* Variables - This is a hierarchy of nodes */

	uint64_t        starting_usec; /* Microseconds at timeline start */
	uint64_t        clk;           /* Current time slice             */
	uint32_t        next_uid;      /* Variable unique ID allocator   */
	
	struct _TsunamiTimeline *next; /* Next timeline */
};

/* ================================================================================ */
/* ================================================================================ */
/* Structure - Overall Tsunami context                                              */
typedef struct _TsunamiContext {
	TsunamiTimeline *timeline_head;
	uint32_t         timeline_count;

#if _WIN32

#else
	pthread_mutex_t  mutex; /* TODO: Make this platform specific */
#endif

} TsunamiContext;

/* Globally defined Tsunami context */
TsunamiContext  _tsunami_context;
static uint32_t _tsunami_context_is_initialised = 0;
/* ================================================================================ */

/* ================================================================================ */
TsunamiLogEntry *TsunamiGetLogWritePtrAndAdvance(TsunamiTimeline *timeline)
{
	TsunamiLogEntry *entry = (timeline->log.entry + timeline->log.entry_last_index);	
	
	timeline->log.entry_last_index = ((timeline->log.entry_last_index + 1) % timeline->log.entry_alloc_count);
	if (timeline->log.entry_count == timeline->log.entry_alloc_count)
		timeline->log.entry_start_index = ((timeline->log.entry_start_index + 1) % timeline->log.entry_alloc_count);
	else
		timeline->log.entry_count ++;

	return entry;
}

void TsunamiDumpSignals_Traverse(FILE            *output_vcd_file, 
								 TsunamiTimeline *timeline, 
								 TsunamiVariable *var, 
								 uint32_t         depth)
{
	uint32_t i;

	while (var) {
		for (i = 0; i < depth; i ++)
			fprintf(output_vcd_file, "   ");

		if (var->head) {
			fprintf(output_vcd_file, "$scope module %s $end\n", var->node_name);			
		} else {
			fprintf(output_vcd_file, "$var wire %i %s %s [%d:0] $end\n", var->size, var->uid, var->node_name, var->size);
		}
		
		if (var->head) {
			TsunamiDumpSignals_Traverse(output_vcd_file,
										timeline, 
										var->head, 
										(depth + 1));
			for (i = 0; i < depth; i ++)
				fprintf(output_vcd_file, "   ");	
			fprintf(output_vcd_file, "$upscope $end\n");		
		}

		var = var->next;
	}
}

void TsunamiDumpRanges_Traverse(FILE            *output_vcd_file,
								TsunamiTimeline *timeline, 
								TsunamiVariable *var, 
								uint32_t         depth)
{
	while (var) {
		if ((!var->head) && (var->range)) {
			uint32_t i;
			fprintf(output_vcd_file, "#%llu\n", (unsigned long long) timeline->clk);
			fprintf(output_vcd_file, "b");
			for (i = 0; i < 32; i ++) 
				fprintf(output_vcd_file, "%c", (int) ('0' + ((var->range >> (63 - i)) & 0x1)));
			fprintf(output_vcd_file, " %s\n", var->uid);			
		} else {
			TsunamiDumpRanges_Traverse(output_vcd_file, 
									   timeline, 
									   var->head, 
									   (depth + 1));			
		}
			
		var = var->next;
	}
}

TsunamiTimeline *TsunamiFindTimeline_Internal(const char *timeline_name)
{
	TsunamiContext  *ctx      = &_tsunami_context;	
	TsunamiTimeline *timeline = ctx->timeline_head;
	
	while (timeline) {
		if (!strcmp(timeline_name, timeline->name))
			return timeline;
		timeline = timeline->next;
	}
	
	return NULL;
}

void TsunamiLock_Internal(TsunamiContext *ctx)
{
#if _WIN32

#else
	pthread_mutex_lock(&ctx->mutex);
#endif
}

void TsunamiUnlock_Internal(TsunamiContext *ctx)
{
#if _WIN32

#else
	pthread_mutex_unlock(&ctx->mutex);
#endif
}

uint64_t TsunamiGetMicroseconds(void)
{
#if _WIN32
   LARGE_INTEGER current;
   LARGE_INTEGER freq;
   QueryPerformanceFrequency(&freq);
   QueryPerformanceCounter(&current);

   current.QuadPart *= 1000000;
   current.QuadPart /= freq.QuadPart;

   return (uint64_t)current.QuadPart;
#else
	struct timeval t;

	gettimeofday(&t, NULL);

	return ((t.tv_sec * 1000000) + t.tv_usec);
#endif

}

void TsunamiTerminateSignalHandler(int signal)
{
	TsunamiContext  *ctx      = &_tsunami_context;
	TsunamiTimeline *timeline = ctx->timeline_head;

	printf("TSUNAMI: Caught signal, flushing all outstanding timelines...\n");

	/* Finish all outstanding timelines */
	while (timeline) {
		TsunamiFlushTimeline(timeline->name);
		timeline = timeline->next;
	}
	
	exit(1);
}

void TsunamiInitialise(void)
{
	if (!_tsunami_context_is_initialised) {
		TsunamiContext *ctx = &_tsunami_context;
		memset(ctx, 0, sizeof(TsunamiContext));
		
		/* Create mutex */
#if _WIN32

#else
		pthread_mutex_init(&ctx->mutex, NULL);
#endif

		/* Install signal handlers for abnormal application termination */
		//signal(SIGHUP,  &TsunamiTerminateSignalHandler);
		//signal(SIGILL,  &TsunamiTerminateSignalHandler);
		signal(SIGABRT, &TsunamiTerminateSignalHandler);
		signal(SIGFPE,  &TsunamiTerminateSignalHandler);
		//signal(SIGKILL, &TsunamiTerminateSignalHandler);
		//signal(SIGBUS,  &TsunamiTerminateSignalHandler);
		signal(SIGSEGV, &TsunamiTerminateSignalHandler);
		signal(SIGTERM, &TsunamiTerminateSignalHandler);
		signal(SIGINT,  &TsunamiTerminateSignalHandler);
		
		_tsunami_context_is_initialised = 1;

		printf("TSUNAMI: Initialised and installed termination signal handlers\n");
	}
}

void TsunamiStartTimeline(const char *timeline_name, const char *filename, uint32_t log_size_bytes)
{
	TsunamiContext  *ctx      = &_tsunami_context;
	
	TsunamiLock_Internal(ctx);
	{
		TsunamiTimeline *timeline = TsunamiFindTimeline_Internal(timeline_name);
		
		if (timeline) {
			printf("TSUNAMI: Found existing timeline for \"%s\", flushing its contents and restarting it from scratch\n",
				   timeline_name);
			TsunamiUnlock_Internal(ctx);
			TsunamiFlushTimeline(timeline_name);
			TsunamiLock_Internal(ctx);
			timeline = NULL;
		}

		printf("TSUNAMI: Starting timeline \"%s\", logging into %u byte buffer to be written to VCD file \"%s\"\n",
			   timeline_name, (unsigned int) log_size_bytes, filename);

		/* Create a new timeline */
		timeline           = ((TsunamiTimeline *) calloc(sizeof(TsunamiTimeline), 1));
		timeline->name     = ((char *) malloc(sizeof(char) * (strlen(timeline_name) + 1)));
		strcpy(timeline->name, timeline_name);
		timeline->filename = ((char *) malloc(sizeof(char) * (strlen(filename) + 1)));
		strcpy(timeline->filename, filename);

		/* Add it to the linked list of timelines */
		if (!ctx->timeline_count) {
			ctx->timeline_head  = timeline;
		} else {
			timeline->next = ctx->timeline_head;
			ctx->timeline_head = timeline;
		}
		ctx->timeline_count ++;

		/* Setup the timeline */
		timeline->log.entry_alloc_count = (log_size_bytes / sizeof(TsunamiLogEntry));
		timeline->log.entry             = ((TsunamiLogEntry *) calloc(sizeof(TsunamiLogEntry), timeline->log.entry_alloc_count));
		
		/* Put in a zero timestamp into the change log to start off */
		{
			TsunamiLogEntry *entry = TsunamiGetLogWritePtrAndAdvance(timeline);
			entry->type            = TsunamiLogEntryType_NewTime;
			entry->new_time.time   = 0;
		}

		/* Record start microseconds for timeline (used for real-time mode) */
		timeline->starting_usec = TsunamiGetMicroseconds();
	}
	TsunamiUnlock_Internal(ctx);
}

void TsunamiFlushTimeline(const char *timeline_name)
{
	TsunamiContext *ctx = &_tsunami_context;

	TsunamiLock_Internal(ctx);
	{
		TsunamiTimeline *timeline = TsunamiFindTimeline_Internal(timeline_name);

		if (!timeline) {
			printf("TSUNAMI: Attempted to flush unknown timeline \"%s\"\n", timeline_name);
		} else {
			FILE *output_vcd_file = fopen(timeline->filename, "wb");
			if (output_vcd_file) {

				/* Write a simple timeline */
				fprintf(output_vcd_file, "$timescale 1us $end\n");

				/* Dump signal hierarchy */
				TsunamiDumpSignals_Traverse(output_vcd_file,
											timeline, 
											timeline->var_root.head, 
											0);

				/* Dump change deltas */
				{
					TsunamiLogEntry *entry;
					uint32_t         i, j, k;
					uint32_t         found_first_time = 0;

					for (i = 0, j = timeline->log.entry_start_index; i < timeline->log.entry_count; i ++) {
			
						entry = (timeline->log.entry + j);

						/* The first entry in the file should be a time stamp so wait for it */
						if (!found_first_time && (entry->type != TsunamiLogEntryType_NewTime))
							goto skip;

						switch (entry->type) {
						case TsunamiLogEntryType_NewTime:
							fprintf(output_vcd_file, "#%llu\n", (unsigned long long) entry->new_time.time);
							found_first_time = 1;
							break;
						case TsunamiLogEntryType_ChangeValue:
							fprintf(output_vcd_file, "b");
							for (k = 0; k < entry->change_value.var->size; k ++)
								fprintf(output_vcd_file, "%c", (int) ('0' + ((entry->change_value.value >> ((entry->change_value.var->size -1) - k)) & 0x1)));
							fprintf(output_vcd_file, " %s\n", entry->change_value.var->uid);
							break;
						}
			
					skip:
						j = ((j + 1) % timeline->log.entry_alloc_count);
					}
				}
		
				/* Dump ranges (for signals which have ranges set) */
				TsunamiDumpRanges_Traverse(output_vcd_file,
										   timeline, 
										   timeline->var_root.head, 
										   0);

				fclose(output_vcd_file);

				printf("TSUNAMI: Flushed timeline \"%s\" and written to VCD file \"%s\"\n",
					   timeline_name, timeline->filename);
			} else {
				printf("TSUNAMI: Failed to open VCD file \"%s\" for writing\n",
					   timeline->filename);
			}
		}
	}
	TsunamiUnlock_Internal(ctx);
}

void TsunamiAdvanceTimeline_Internal(TsunamiTimeline *timeline)
{

	TsunamiLogEntry *entry    = TsunamiGetLogWritePtrAndAdvance(timeline);	

	/* Write a new time token into the log */
	entry->type          = TsunamiLogEntryType_NewTime;
	entry->new_time.time = timeline->clk;

	/* Reset pulse variables */
	{
		TsunamiVariable *var = timeline->var_root.list_next;
		
		while (var) {
			if (var->is_pulse) { 
				TsunamiSetValue_Internal(timeline, var, 0);
				var->last_value = 0;
			}
			var = var->list_next;
		}
	}
}

void TsunamiAdvanceTimeline(const char *timeline_name)
{
	TsunamiTimeline *timeline = TsunamiFindTimeline_Internal(timeline_name);

	if (!timeline) {
		printf("TSUNAMI: Attempted to flush unknown timeline \"%s\"\n", timeline_name);
		return;
	}

	/* Advance clock */
	timeline->clk ++;

	TsunamiAdvanceTimeline_Internal(timeline);
}

void TsunamiUpdateTimelineToRealtime(const char *timeline_name)
{
	TsunamiTimeline *timeline = TsunamiFindTimeline_Internal(timeline_name);

	if (!timeline) {
		printf("TSUNAMI: Attempted to flush unknown timeline \"%s\"\n", timeline_name);
		return;
	}

	/* Set the clock to real-time microseconds since timeline start-up */
	timeline->clk = (TsunamiGetMicroseconds() - timeline->starting_usec);

	TsunamiAdvanceTimeline_Internal(timeline);
}

TsunamiVariable *TsunamiFindVariable_Traverse_Internal(TsunamiTimeline *timeline, const char *name, uint32_t name_len, TsunamiVariable *parent_var)
{
	TsunamiVariable *var;
	uint32_t         i, len;

	/* A NULL var suggests starting traversal at root */
	if (!parent_var)
		parent_var = &timeline->var_root;

	/* Extract the name of this scope */
	len = name_len;
	for (i = 0; i < len; i ++) {
		if (name[i] == '.')
			break;
	}
	len = i;
	
	/* Search this level for a matching scope name */
	var = parent_var->head;
	while (var) {
		if (var->node_name_len == len) {
			if (!strncmp(name, var->node_name, len))
				break;
		}
		var = var->next;
	}

	/* If we failed to find a match at this level, return NULL since
		this variable doesn't exist in the tree */
	if (!var)
		return NULL;

	/* Descend another level if needed */
	if (var->head)
		return TsunamiFindVariable_Traverse_Internal(timeline, (name + len + 1), (name_len - len - 1), var);
	
	return var;
}

TsunamiVariable *TsunamiAddVariable_Internal(TsunamiTimeline *timeline, 
											 const char *full_name, uint32_t full_name_len,
											 const char *name,      uint32_t name_len,
											 TsunamiVariable *parent_var)
{
	TsunamiVariable *var;
	uint32_t         i, node_name_len, is_leaf = 0;
	uint32_t         uid;

	/* A NULL var suggests starting traversal at root */
	if (!parent_var)
		parent_var = &timeline->var_root;
	
	/* Extract the name of this scope */
	node_name_len = name_len;
	for (i = 0; i < node_name_len; i ++) {
		if (name[i] == '.')
			break;
	}
	if (i == node_name_len)
		is_leaf = 1;
	node_name_len = i;
	
	/* Search this level for a matching scope name */
	var = parent_var->head;
	while (var) {
		if (var->node_name_len == node_name_len) {
			if (!strncmp(name, var->node_name, node_name_len))
				break;
		}
		var = var->next;
	}
	
	/* If we failed to find a match at this level, create this scope and
		continue descent */
	if (!var) {
		/* Create a new variable */
		var = ((TsunamiVariable *) calloc(sizeof(TsunamiVariable), 1));

		/* Assign node name of this scope */
		var->node_name = ((char *) calloc(sizeof(char), (node_name_len + 1)));
		strncpy(var->node_name, name, node_name_len);
		var->node_name_len = node_name_len;

		/* If its a leaf, assign the actual attributes of the variable */
		if (is_leaf) {

			/* Assign full name */
			var->full_name = ((char *) calloc(sizeof(char), (full_name_len + 1)));
			strncpy(var->full_name, full_name, full_name_len);
			var->full_name_len = full_name_len;

			/* Assign the type of the signal and initialise the "last" value */
			var->last_value = 0;

			/* Generate a new UID */
			uid = timeline->next_uid ++;
			/* Convert the UID into an ASCII string (upto 5 chars long) */
			for (i = 0; i < 5; i ++) {
				var->uid[i] = ((uid >> (i * 5) & 0x1f));
				if ((i > 0) && (!var->uid[i]))
					break;
				var->uid[i] += 65;
			}
			var->uid[i] = 0;
		}

		/* Add to linked list */
		var->next        = parent_var->head;
		parent_var->head = var;
		if (timeline->var_root.list_next)
			var->list_next          = timeline->var_root.list_next;
		timeline->var_root.list_next = var;
	}
	
	/* Descend another level if needed */
	if (!is_leaf)
		return TsunamiAddVariable_Internal(timeline, full_name, full_name_len,
										   (name + node_name_len + 1), (name_len - node_name_len - 1), var);
	
	return var;
}

TsunamiVariable *TsunamiFindVariable_Internal(TsunamiTimeline *timeline, const char *name)
{
	TsunamiVariable *var      = NULL;	
	uint32_t         name_len;

	/* Find variable */
	name_len = (uint32_t) strlen(name);
	var      = TsunamiFindVariable_Traverse_Internal(timeline, name, name_len, NULL);

	/* Create a new variable, if existing one was not found */
	if (!var)
		var = TsunamiAddVariable_Internal(timeline, name, name_len, name, name_len, NULL);
	
	return var;
}

void TsunamiSetValue_Internal(TsunamiTimeline *timeline, TsunamiVariable *var, uint64_t value)
{
	/* Only write the variable if the data has changed for this signal */ 
	if ((var->last_value != value) || (!var->is_defined)) {	

		TsunamiLogEntry *entry = TsunamiGetLogWritePtrAndAdvance(timeline);

		entry->type               = TsunamiLogEntryType_ChangeValue;
		entry->change_value.var   = var;
		entry->change_value.value = value;

		var->last_value = value;
		var->is_defined = 1;
	}
}

void TsunamiSetRange_Internal(TsunamiTimeline *timeline, TsunamiVariable *var, uint64_t range)
{
	var->range = range;
}

#endif /* TSUNAMI_ENABLE */
