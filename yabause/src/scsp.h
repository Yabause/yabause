/*  Copyright 2004 Stephane Dallongeville
    Copyright 2004-2006 Theo Berkau

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

// Quick hack so nobody else needs to know we're using a different header file
#ifdef USE_SCSP2
#include "scsp2.h"  // defines SCSP_H
#endif

#ifndef SCSP_H
#define SCSP_H

#include "core.h"
#include "memory.h"

#define SNDCORE_DEFAULT -1
#define SNDCORE_DUMMY   0
#define SNDCORE_WAV     10 // should really be 1, but I'll probably break people's stuff

#define SCSP_MUTE_SYSTEM    1
#define SCSP_MUTE_USER      2

typedef struct
{
   int id;
   const char *Name;
   int (*Init)(void);
   void (*DeInit)(void);
   int (*Reset)(void);
   int (*ChangeVideoFormat)(int vertfreq);
   void (*UpdateAudio)(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples);
   u32 (*GetAudioSpace)(void);
   void (*MuteAudio)(void);
   void (*UnMuteAudio)(void);
   void (*SetVolume)(int volume);
#ifdef USE_SCSPMIDI
	int (*MidiChangePorts)(int inport, int outport);
	u8 (*MidiIn)(int *isdata);
	int (*MidiOut)(u8 data);
#endif
} SoundInterface_struct;

typedef struct
{
   u32 D[8];
   u32 A[8];
   u32 SR;
   u32 PC;
} m68kregs_struct;

typedef struct
{
  u32 addr;
} m68kcodebreakpoint_struct;

#define MAX_BREAKPOINTS 10

typedef struct
{
  u32 scsptiming1;
  u32 scsptiming2;  // 16.16 fixed point

  m68kcodebreakpoint_struct codebreakpoint[MAX_BREAKPOINTS];
  int numcodebreakpoints;
  void (*BreakpointCallBack)(u32);
  int inbreakpoint;
} ScspInternal;

extern SoundInterface_struct SNDDummy;
extern SoundInterface_struct SNDWave;
extern u8 *SoundRam;
extern int use_new_scsp;

u8 FASTCALL SoundRamReadByte(u32 addr);
u16 FASTCALL SoundRamReadWord(u32 addr);
u32 FASTCALL SoundRamReadLong(u32 addr);
void FASTCALL SoundRamWriteByte(u32 addr, u8 val);
void FASTCALL SoundRamWriteWord(u32 addr, u16 val);
void FASTCALL SoundRamWriteLong(u32 addr, u32 val);

u8 FASTCALL Sh2SoundRamReadByte(SH2_struct *sh, u32 addr);
u16 FASTCALL Sh2SoundRamReadWord(SH2_struct *sh, u32 addr);
u32 FASTCALL Sh2SoundRamReadLong(SH2_struct *sh, u32 addr);
void FASTCALL Sh2SoundRamWriteByte(SH2_struct *sh, u32 addr, u8 val);
void FASTCALL Sh2SoundRamWriteWord(SH2_struct *sh, u32 addr, u16 val);
void FASTCALL Sh2SoundRamWriteLong(SH2_struct *sh, u32 addr, u32 val);

int ScspInit(int coreid);
int ScspChangeSoundCore(int coreid);
void ScspDeInit(void);
void M68KStart(void);
void M68KStop(void);
void ScspReset(void);
int ScspChangeVideoFormat(int type);
void M68KExec(s32 cycles);
void ScspExec(void);
void ScspConvert32uto16s(s32 *srcL, s32 *srcR, s16 *dst, u32 len);
void ScspReceiveCDDA(const u8 *sector);
void ScspReceiveMpeg (const u8 *samples, int len);
int SoundSaveState(void ** stream);
int SoundLoadState(const void * stream, int version, int size);
void ScspSlotDebugStats(u8 slotnum, char *outstring);
void ScspCommonControlRegisterDebugStats(char *outstring);
int ScspSlotDebugSaveRegisters(u8 slotnum, const char *filename);
u32 ScspSlotDebugAudio (u32 *workbuf, s16 *buf, u32 len);
void ScspSlotResetDebug(u8 slotnum);
int ScspSlotDebugAudioSaveWav(u8 slotnum, const char *filename);
void ScspMuteAudio(int flags);
void ScspUnMuteAudio(int flags);
void ScspSetVolume(int volume);


u8 FASTCALL ScspReadByte(u32 addr);
void FASTCALL ScspWriteByte(u32 addr, u8 val);
u16 FASTCALL ScspReadWord(u32 addr);
void FASTCALL ScspWriteWord(u32 addr, u16 val);
u32 FASTCALL ScspReadLong(u32 addr);
void FASTCALL ScspWriteLong(u32 addr, u32 val);

u8 FASTCALL Sh2ScspReadByte(SH2_struct *sh, u32 addr);
void FASTCALL Sh2ScspWriteByte(SH2_struct *sh, u32 addr, u8 val);
u16 FASTCALL Sh2ScspReadWord(SH2_struct *sh, u32 addr);
void FASTCALL Sh2ScspWriteWord(SH2_struct *sh, u32 addr, u16 val);
u32 FASTCALL Sh2ScspReadLong(SH2_struct *sh, u32 addr);
void FASTCALL Sh2ScspWriteLong(SH2_struct *sh, u32 addr, u32 val);

void scsp_init(u8 *scsp_ram, void (*sint_hand)(u32), void (*mint_hand)(void));
void scsp_shutdown(void);
void scsp_reset(void);

void scsp_midi_in_send(u8 data);
void scsp_midi_out_send(u8 data);
u8 scsp_midi_in_read(void);
u8 scsp_midi_out_read(void);
void scsp_update(s32 *bufL, s32 *bufR, u32 len);
void scsp_update_monitor(void);
void scsp_update_timer(u32 len);

u32 FASTCALL c68k_byte_read(const u32 adr);
u32 FASTCALL c68k_word_read(const u32 adr);

void M68KStep(void);
void M68KSync(void);
void M68KWriteNotify(u32 address, u32 size);
void M68KGetRegisters(m68kregs_struct *regs);
void M68KSetRegisters(m68kregs_struct *regs);
void M68KSetBreakpointCallBack(void (*func)(u32));
int M68KAddCodeBreakpoint(u32 addr);
void M68KSortCodeBreakpoints(void);
int M68KDelCodeBreakpoint(u32 addr);
m68kcodebreakpoint_struct *M68KGetBreakpointList(void);
void M68KClearCodeBreakpoints(void);

void scsp_debug_instrument_get_data(int i, u32 * sa, int * is_muted);
void scsp_debug_instrument_set_mute(u32 sa, int mute);
void scsp_debug_instrument_clear();
void scsp_debug_get_envelope(int chan, int * env, int * state);
void scsp_debug_set_mode(int mode);
void scsp_set_use_new(int which);
void new_scsp_exec(s32 cycles);

extern int use_new_scsp;
#endif
