/*
        Copyright 2019 devMiyax(smiyaxdev@gmail.com)

This file is part of YabaSanshiro.

        YabaSanshiro is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

YabaSanshiro is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
along with YabaSanshiro; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h> 
#include <stdint.h>
#include "core.h"
#include "sh2core.h"
#include "debug.h"
#include "yabause.h"

#include <atomic>

using std::atomic;

atomic<u64> m68k_counter(0);
atomic<u64> m68k_counter_done(0);

const u64 MAX_SCSP_COUNTER = (u64)(44100 * 256 / 60) << SCSP_FRACTIONAL_BITS;

extern "C" {
  void SyncCPUtoSCSP();
  extern u64 g_m68K_dec_cycle;

  void setM68kCounter(u64 counter) {
    m68k_counter = counter;
  }

  void setM68kDoneCounter(u64 counter) {
    m68k_counter_done = counter;
  }

  u64 getM68KCounter() {
    return m68k_counter;
  }

  void syncM68K() {
    int timeout = 0;
/*
    m68k_counter += (2 << SCSP_FRACTIONAL_BITS);
    g_m68K_dec_cycle += (2 << SCSP_FRACTIONAL_BITS);
    if (m68k_counter >= MAX_SCSP_COUNTER) {
      SyncCPUtoSCSP();
    }
*/
    u64 a = (m68k_counter >> SCSP_FRACTIONAL_BITS);
    u64 b = m68k_counter_done;
    //LOG("[CPU] INC m68k_counter %lld/%lld", a, b);
    while ( (m68k_counter >> SCSP_FRACTIONAL_BITS) > m68k_counter_done) { timeout++; };
  }

}

