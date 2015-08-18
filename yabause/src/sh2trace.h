/*  src/sh2trace.h: SH-2 tracing header for debugging
    Copyright 2009 Andrew Church

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef SH2TRACE_H
#define SH2TRACE_H

#include "core.h"

void SetInsTracing(int toggle);

extern FASTCALL u64 sh2_cycle_count(void);
extern FASTCALL void sh2_trace_add_cycles(s32 cycles);
extern FASTCALL void sh2_trace_writeb(u32 address, u32 value);
extern FASTCALL void sh2_trace_writew(u32 address, u32 value);
extern FASTCALL void sh2_trace_writel(u32 address, u32 value);
extern FASTCALL void sh2_trace(SH2_struct *state, u32 address);

#endif  // SH2TRACE_H
