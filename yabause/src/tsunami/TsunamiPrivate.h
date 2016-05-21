/*
  TsunamiPrivate.h
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

#pragma once

#if TSUNAMI_ENABLE

/* Required includes */
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Structure - Internal representation of a variable/signal */
typedef struct _TsunamiVariable {
	char           *full_name;
	char           *node_name;
	uint32_t        full_name_len;
	uint32_t        node_name_len;
	char            uid  [16];
   int size;

	uint32_t        is_defined;
	uint32_t        is_pulse;
	uint64_t        last_value;
	uint64_t        range;              /* Maximum this value can be.  If present,
										   this is placed at the end of the trace
										   to allow tools to draw analogue graphs
										   will proper scale.                     */

	struct _TsunamiVariable *next;      /* Sibling link                   */
	struct _TsunamiVariable *head;      /* Children head                  */
	struct _TsunamiVariable *list_next; /* Overall flat list next pointer */
} TsunamiVariable;

/* Structure - Timeline */
typedef struct _TsunamiTimeline TsunamiTimeline;

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================================ */
/* Tsunami Internal APIs */
TsunamiTimeline *TsunamiFindTimeline_Internal(const char *timeline_name);
TsunamiVariable *TsunamiFindVariable_Internal(TsunamiTimeline *timeline, const char *name);
void             TsunamiSetValue_Internal(TsunamiTimeline *timeline, TsunamiVariable *var, uint64_t value);
void             TsunamiSetRange_Internal(TsunamiTimeline *timeline, TsunamiVariable *var, uint64_t range);

#define TsunamiSetValue_Base_Internal2(_inc_, _pulse_, _setrange_, _value_, _size_, _timeline_name_, _variable_name_) \
	{																	\
		static TsunamiTimeline *timeline = NULL;						\
		static TsunamiVariable *var      = NULL;						\
		if (!timeline)													\
			timeline = TsunamiFindTimeline_Internal((_timeline_name_));	\
		if (!timeline) {												\
			printf("TSUNAMI: Attempted to set value on unknown timeline \"%s\"\n", _timeline_name_); \
		} else {														\
			if (!var)													\
				var = TsunamiFindVariable_Internal(timeline, (_variable_name_)); \
			if (strcmp((_variable_name_), var->full_name))				\
				var = TsunamiFindVariable_Internal(timeline, (_variable_name_)); \
			if (_inc_)													\
				TsunamiSetValue_Internal(timeline, var, (var->last_value + (_value_))); \
			else if (_setrange_)										\
				TsunamiSetRange_Internal(timeline, var, (_value_));		\
			else {														\
				TsunamiSetValue_Internal(timeline, var, (uint64_t) (_value_)); \
				var->is_pulse = (_pulse_);								\
            var->size = (_size_);  \
			}															\
		}																\
	}

#if _WIN32

#define TsunamiSetValue_Base_Internal(_inc_, _pulse_, _setrange_, _value_, _size_, _timeline_name_, _value_format_, ...) \
	{																	\
		static char             variable_name [1024];					\
		sprintf(variable_name, (_value_format_), __VA_ARGS__);			\
		TsunamiSetValue_Base_Internal2(_inc_, _pulse_, _setrange_, _value_, _size_, _timeline_name_, variable_name); \
	}

#else

#define TsunamiSetValue_Base_Internal(_inc_, _pulse_, _setrange_, _value_, _size_, _timeline_name_, _value_format_, args...) \
	{																	\
		static char             variable_name [1024];					\
		sprintf(variable_name, (_value_format_), ##args);				\
		TsunamiSetValue_Base_Internal2(_inc_, _pulse_, _setrange_, _value_, _size_, _timeline_name_, variable_name); \
	}

#endif
/* ================================================================================ */	

#ifdef __cplusplus
}
#endif

#else /* TSUNAMI_ENABLE */

#define TsunamiInitialise()                            ((void)0)
#define TsunamiStartTimeline(_t_, _f_, _l_)            ((void)0)
#define TsunamiFlushTimeline(_t_)                      ((void)0)
#define TsunamiAdvanceTimeline(_t_)                    ((void)0)
#define TsunamiUpdateTimelineToRealtime(_t_)           ((void)0)
#define TsunamiSetValue(_v_, _t_, _vf_, ...)           ((void)0)
#define TsunamiIncrementValue(_i_, _t_, _vf_, ...)     ((void)0)
#define TsunamiPulseValue(_v_, _t_, _vf_, ...)         ((void)0)
#define TsunamiSetRange(_r_, _t_, _vf_, ...)           ((void)0)

#endif

