/* ================================================================================ */
/*                                                                                 	*/
/*                         ` ` `                                                   	*/
/*                      ...... ` ` `  `               `                            	*/
/*                   ,/////\\\\\   ` `       `                                      */
/*                 ,///      `\\\\ 	` `                                             */
/*               ,////  ,       `\\\                                                */
/*             ,///   /	,,`,   	 `\\\\           `                                  */
/*            ///  	/,,/  ,`,` `   ````                     `                       */
/*           /,,  /,/  /,`/	`/``                                                    */
/*	\__..._//, 	   ___________                                .__    `              */
/*     /////// /,..\__    ___/_______ __  ____ _____    _____ |__|                  */
/*	// 	   	 ,,,,    |    | /  ___/  |  \/    \\__  \  /     \|  |                  */
/*  \_____....,	     |    | \___ \|  |  /   |  \/ __ \|  Y Y  \  |                  */
/*                   |____|/____  >____/|___|  (____  /__|_|  /__|        `         */
/*                              \/           \/     \/      \/                      */
/*                                                                                  */
/* ================================================================================ */
/* 
   Tsunami.h
   
   Defines the public interface to Tsunami
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

/* ================================================================================ */
/* Master enable switch                                                             */
/* By disabling here, Tsunami disappears from your software entirely through
   empty stub macros replacing the real Tsunami APIs                                */

#ifndef TSUNAMI_ENABLE
#define TSUNAMI_ENABLE      1
#endif

/* ================================================================================ */

/* Required includes */
#include "TsunamiPrivate.h"

/* Constants */
#define TSUNAMI_DEFAULT_LOGSIZE        (1 * 1024 * 1024)

#ifdef __cplusplus
extern "C" {
#endif

#if TSUNAMI_ENABLE

/* ================================================================================ */
/* Public Tsunami API                                                               */
/* ================================================================================ */

/* TsunamiInitialise():
   
   Initialises Tsunami's internal context.  This must be called before everything else
   and would typically be placed at the earliest reasonable place in your client application */
void TsunamiInitialise(void);

/* TsunamiStartTimeline():
	   
   Tsunami allows for multiple "timelines", each time line containing the change in values
   of variables in your client application over time.  This function creates a new timeline 

   If this is called and an existing timeline with the same name is found, that previous timeline
   will be flushed its VCD file and new empty timeline started with the same name.
*/
void TsunamiStartTimeline(const char *timeline_name,    /* NULL terminate timeline name                    */
						  const char *filename,         /* Full path to write output VCD file to           */
						  uint32_t    log_size_bytes);  /* Size in bytes of the internal circular 
														   buffer used to log value changes within Tsunami.
														   Use TSUNAMI_DEFAULT_LOGSIZE if you don't care.  */

/* TsunamiFlushTimeline():

   This causes a timeline to write its internal circular buffer to the
   file specified on the call to TsunamiStartTimeline().  The timeline
   remains active after this call.
   
   This must be called at least once otherwise the output VCD file will remain empty.
   
   You can call this multiple times if you want your application to provide updates
   to the VCD file over time.
*/
void TsunamiFlushTimeline(const char *timeline_name);

/* TsunamiAdvanceTimeline():

   This advances a timeline to the next time increment.  When using this 
   function, the time increment doesn't correspond to real-time. */
void TsunamiAdvanceTimeline(const char *timeline_name);

/* TsunamiUpdateTimelineToRealtime():

   This advances a timeline to the current real-time in microseconds
   since the timeline was started.  This can be useful when using
   Tsunami for real-time analysis and debugging. 
*/
void TsunamiUpdateTimelineToRealtime(const char *timeline_name);

/* ================================================================================ */
/* Timeline value setting macros.

   These macros update the current value of a named entry in a timeline.
   These macros have several common arguments:

   _value_            : 32-bit integer value in your client application.
   _timeline_name_    : Name of the timeline this value should be recoreded into.
   _value_name_format : Name of the value.  This can use printf() style format strings
                        and be followed by an optional variable length argument list.

   TsunamiSetValue():
   This sets the current value of a named entry in the timeline.  The entry will retain
   this value forever onwards until its changed.

   TsunamiIncrementValue():
   This adds "_increment_" to the existing value of the named entry in the timeline.  The
   entry will retain its value forever onwards until its changed.

   TsunamiPulseValue():
   This sets the current value of a named entry in the timeline.  The entry will have this
   value only for the current time step and will be reset to 0 afterwards.  This is useful
   to indicate that a value exists only the current time step, i.e. perhaps a boolean
   value indicating that an event occured on that particular timestep.
   
   TsunamiSetRange():
   This creates a single value in the VCD file with value "_range_".  It is useful as a presentation
   tool in the waveform viewer when viewing signals as analogue curves.  This causes the waveform
   viewer to set the Y axis scale the curve between 0 and "_range_" which can help with certain
   visualisations.
*/
	
#if _WIN32

#define TsunamiSetValue(_value_, _size_, _timeline_name_, _value_name_format_, ...) \
	TsunamiSetValue_Base_Internal(0, 0, 0, _value_, _size_, _timeline_name_, _value_name_format_, __VA_ARGS__)
#define TsunamiIncrementValue(_increment_, _timeline_name_, _value_name_format_, ...) \
	TsunamiSetValue_Base_Internal(1, 0, 0, _increment_, _timeline_name_, _value_name_format_, __VA_ARGS__)
#define TsunamiPulseValue(_value_, _size_, _timeline_name_, _value_name_format_, ...) \
	TsunamiSetValue_Base_Internal(0, 1, 0, _value_, _size_, _timeline_name_, _value_name_format_, __VA_ARGS__)
#define TsunamiSetRange(_range_, _timeline_name_, _value_name_format_, ...) \
	TsunamiSetValue_Base_Internal(0, 0, 1, _range_, _timeline_name_, _value_name_format_, __VA_ARGS__)	
#else
	
#define TsunamiSetValue(_value_, _size_, _timeline_name_, _value_name_format_, args...) \
	TsunamiSetValue_Base_Internal(0, 0, 0, _value_, _size_, _timeline_name_, _value_name_format_, ##args)
#define TsunamiIncrementValue(_increment_, _timeline_name_, _value_name_format_, args...) \
	TsunamiSetValue_Base_Internal(1, 0, 0, _increment_, _timeline_name_, _value_name_format_, ##args)
#define TsunamiPulseValue(_value_, _size_, _timeline_name_, _value_name_format_, args...) \
	TsunamiSetValue_Base_Internal(0, 1, 0, _value_, _size_, _timeline_name_, _value_name_format_, ##args)
#define TsunamiSetRange(_range_, _timeline_name_, _value_name_format_, args...) \
	TsunamiSetValue_Base_Internal(0, 0, 1, _range_, _timeline_name_, _value_name_format_, ##args)	

#endif

/* ================================================================================ */

#endif /* TSUNAMI_ENABLE */

#ifdef __cplusplus
}
#endif
