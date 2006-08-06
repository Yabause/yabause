/*  Copyright 2004 Stephane Dallongeville
    Copyright 2004-2006 Theo Berkau
    Copyright 2006 Guillaume Duhamel

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

////////////////////////////////////////////////////////////////
// Custom Sound Processor

// note: model2 scsp is mapped to 0x100000~0x100ee4 of the space, but seems to have additional hw ports ($40a~$410)
// note: it seems that the user interrupt is used by the sound driver to reset the subsystem

//--------------------------------------------------------------
//
// Common Control Register (CCR)
//
//	  $+00      $+01
// $400 ---- --12 3333 4444 1:MEM4MB memory size 2:DAC18B dac for digital output 3:VER version number 4:MVOL	-
// $402 ---- ---1 1222 2222 1:RBL ring buffer length 2:RBP lead address								-
// $404 ---1 2345 6666 6666 1:MOFULL out fifo full 2:MOEMP empty 3:MIOVF overflow 4:MIFULL in 5:MIEMP 6:MIBUF	*
// $406 ---- ---- 1111 1111 1:MOBUF midi output data buffer										*
// $408 1111 1222 2--- ---- 1:MSLC monitor slot 2:CA call address									*!
// $40a ---- ---- ---- ----															-
// $40c ---- ---- ---- ----															-
// $40e ---- ---- ---- ----															-
// $410 ---- ---- ---- ----															-
// $412 1111 1111 1111 111- 1:DMEAL transfer start address (sound)								*
// $414 1111 2222 2222 222- 1:DMEAH transfer start address hi2:DRGA start register address (dsp)			*
// $416 -123 4444 4444 444- 1:DGATE transfer gate 0 clear 2:DDIR direction 3:DEXE start 4:DTLG data count		*
// $418 ---- -111 2222 2222 1:TACTL timer a prescalar control 2:TIMA timer a count data					-
// $41a ---- -111 2222 2222 1:TBCTL timer b prescalar control 2:TIMB timer b count data					-
// $41c ---- -111 2222 2222 2:TCCTL timer c prescalar control 2:TIMC timer c count data					-
// $41e ---- -111 1111 1111 1:SCIEB allow sound cpu interrupt									-
// $420 ---- -111 1111 1111 1:SCIPD request sound cpu interrupt									-
// $422 ---- -111 1111 1111 1:SCIRE reset sound cpu interrupt									-
// $424 ---- ---- 1111 1111 1:SCILV0 sound cpu interrupt level bit0								-
// $426 ---- ---- 1111 1111 1:SCILV1 sound cpu interrupt level bit1								-
// $428 ---- ---- 1111 1111 1:SCILV2 sound cpu interrupt level bit2								-
// $42a ---- -111 1111 1111 1:MCIEB allow main cpu interrupt									-
// $42c ---- -111 1111 1111 1:MCIPD request main cpu interrupt									-
// $42e ---- -111 1111 1111 1:MCIRE reset main cpu interrupt									-
//
//--------------------------------------------------------------
//
// Individual Slot Register (ISR)
//
//	 $+00      $+01
// $00 ---1 2334 4556 7777 1:KYONEX 2:KYONB 3:SBCTL 4:SSCTL 5:LPCTL 6:PCM8B 7:SA start address
// $02 1111 1111 1111 1111 1:SA start address
// $04 1111 1111 1111 1111 1:LSA loop start address
// $06 1111 1111 1111 1111 1:LEA loop end address
// $08 1111 1222 2234 4444 1:D2R decay 2 rate 2:D1R decay 1 rate 3:EGHOLD eg hold mode 4:AR attack rate
// $0a -122 2233 3334 4444 1:LPSLNK loop start link 2:KRS key rate scaling 3:DL decay level 4:RR release rate
// $0c ---- --12 3333 3333 1:STWINH stack write inhibit 2:SDIR sound direct 3:TL total level
// $0e 1111 2222 2233 3333 1:MDL modulation level 2:MDXSL modulation input x 3:MDYSL modulation input y
// $10 -111 1-22 2222 2222 1:OCT octave 2:FNS frequency number switch
// $12 1222 2233 4445 5666 1:LFORE 2:LFOF 3:PLFOWS 4:PLFOS 5:ALFOWS 6:ALFOS
// $14 ---- ---- -111 1222 1:ISEL input select 2:OMXL input mix level
// $16 1112 2222 3334 4444 1:DISDL 2:DIPAN 3:EFSDL 4:EFPAN
//
//--------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "c68k/c68k.h"
#include "debug.h"
#include "error.h"
#include "memory.h"
#include "scu.h"
#include "yabause.h"
#include "scsp.h"

////////////////////////////////////////////////////////////////

#ifndef PI
#define PI 3.14159265358979323846
#endif

#define SCSP_FREQ			44100				// SCSP frequency

#define SCSP_RAM_SIZE		0x080000			// SCSP RAM size
#define SCSP_RAM_MASK		(SCSP_RAM_SIZE - 1)

#define SCSP_MIDI_IN_EMP	0x01				// MIDI flags
#define SCSP_MIDI_IN_FUL	0x02
#define SCSP_MIDI_IN_OVF	0x04
#define SCSP_MIDI_OUT_EMP	0x08
#define SCSP_MIDI_OUT_FUL	0x10

#define SCSP_ENV_RELEASE	0				// Enveloppe phase
#define SCSP_ENV_SUBSTAIN	1
#define SCSP_ENV_DECAY		2
#define SCSP_ENV_ATTACK		3

#define SCSP_FREQ_HB		19				// Freq counter int part
#define SCSP_FREQ_LB		10				// Freq counter float part

#define SCSP_ENV_HB		10				// Env counter int part
#define SCSP_ENV_LB		10				// Env counter float part

#define SCSP_LFO_HB		10				// LFO counter int part
#define SCSP_LFO_LB		10				// LFO counter float part

#define SCSP_ENV_LEN		(1 << SCSP_ENV_HB)	// Env table len
#define SCSP_ENV_MASK		(SCSP_ENV_LEN - 1)	// Env table mask

#define SCSP_FREQ_LEN		(1 << SCSP_FREQ_HB)	// Freq table len
#define SCSP_FREQ_MASK		(SCSP_FREQ_LEN - 1)	// Freq table mask

#define SCSP_LFO_LEN		(1 << SCSP_LFO_HB)	// LFO table len
#define SCSP_LFO_MASK		(SCSP_LFO_LEN - 1)	// LFO table mask

#define SCSP_ENV_AS		0							// Env Attack Start
#define SCSP_ENV_DS		(SCSP_ENV_LEN << SCSP_ENV_LB)			// Env Decay Start
#define SCSP_ENV_AE		(SCSP_ENV_DS - 1)					// Env Attack End
#define SCSP_ENV_DE		(((2 * SCSP_ENV_LEN) << SCSP_ENV_LB) - 1)	// Env Decay End

#define SCSP_ATTACK_R		(u32) (8 * 44100)
#define SCSP_DECAY_R		(u32) (12 * SCSP_ATTACK_R)

////////////////////////////////////////////////////////////////

typedef struct slot_t
{
	u8	swe;			// stack write enable
	u8	sdir;			// sound direct
	u8	pcm8b;		// PCM sound format

	u8	sbctl;		// source bit control
	u8	ssctl;		// sound source control
	u8	lpctl;		// loop control

	u8	key;			// KEY_ state
	u8	keyx;			// still playing regardless the KEY_ state (hold, decay)

	s8	*buf8;		// sample buffer 8 bits
	s16	*buf16;		// sample buffer 16 bits

	u32	fcnt;			// phase counter
	u32	finc;			// phase step adder
	u32	finct;		// non ajusted phase step

	s32	ecnt;			// enveloppe counter
	s32	einc;			// enveloppe current step adder
	s32	einca;		// enveloppe step adder for attack
	s32	eincd;		// enveloppe step adder for decay 1
	s32	eincs;		// enveloppe step adder for decay 2
	s32	eincr;		// enveloppe step adder for release
	s32	ecmp;			// enveloppe compare to raise next phase
	u32	ecurp;		// enveloppe current phase (attack / decay / release ...)

	void	(*enxt)(struct slot_t *);	// enveloppe function pointer for next phase event

	u32	lfocnt;		// lfo counter
        s32     lfoinc;         // lfo step adder

	u32	sa;			// start address
	u32	lsa;			// loop start address
	u32	lea;			// loop end address

	s32	tl;			// total level
	s32	sl;			// substain level

	s32	ar;			// attack rate
	s32	dr;			// decay rate
	s32	sr;			// substain rate
	s32	rr;			// release rate

	s32	*arp;			// attack rate table pointer
	s32	*drp;			// decay rate table pointer
	s32	*srp;			// substain rate table pointer
	s32	*rrp;			// release rate table pointer

	u32	krs;			// key rate scale

	s32	*lfofmw;		// lfo frequency modulation waveform pointer
	s32	*lfoemw;		// lfo enveloppe modulation waveform pointer
	u8	lfofms;		// lfo frequency modulation sensitivity
	u8	lfoems;		// lfo enveloppe modulation sensitivity
	u8	fsft;			// frequency shift (used for freq lfo)

	u8	mdl;			// modulation level
	u8	mdx;			// modulation source X 
	u8	mdy;			// modulation source Y

	u8	imxl;			// input sound level
	u8	disll;		// direct sound level left
	u8	dislr;		// direct sound level right
	u8	efsll;		// effect sound level left
	u8	efslr;		// effect sound level right

	u8	eghold;		// eg type enveloppe hold
	u8	lslnk;		// loop start link (start D1R when start loop adr is reached)

	u8	dirt14;		// 4 bytes alignement...
	u8	dirt15;		// 4 bytes alignement...
	u8	dirt16;		// 4 bytes alignement...
} slot_t;

typedef struct scsp_t
{
        u32     mem4b;                  // 4mbit memory
	u32	mvol;			// master volume

	u32	rbl;			// ring buffer lenght
	u32	rbp;			// ring buffer address (pointer)

	u32	mslc;			// monitor slot
	u32	ca;			// call address

	u32	dmea;			// dma memory address start
	u32	drga;			// dma register address start
	u32	dmfl;			// dma flags (direction / gate 0 ...)
	u32	dmlen;		// dma transfert len

	u8	midinbuf[4];	// midi in buffer
	u8	midoutbuf[4];	// midi out buffer
	u8	midincnt;		// midi in buffer size
	u8	midoutcnt;		// midi out buffer size
	u8	midflag;		// midi flag (empty, full, overflow ...)
	u8	midflag2;		// midi flag 2 (here only for alignement)

	s32	timacnt;		// timer A counter
	u32	timasd;		// timer A step diviser
	s32	timbcnt;		// timer B counter
	u32	timbsd;		// timer B step diviser
	s32	timccnt;		// timer C counter
	u32	timcsd;		// timer C step diviser

	u32	scieb;		// allow sound cpu interrupt
	u32	scipd;		// pending sound cpu interrupt

	u32	scilv0;		// IL0 M68000 interrupt pin state
	u32	scilv1;		// IL1 M68000 interrupt pin state
	u32	scilv2;		// IL2 M68000 interrupt pin state

	u32	mcieb;		// allow main cpu interrupt
	u32	mcipd;		// pending main cpu interrupt

	u8	*scsp_ram;		// scsp ram pointer
	void	(*mintf)(void);	// main cpu interupt function pointer
	void	(*sintf)(u32);	// sound cpu interrupt function pointer

	s32		stack[32 * 2];	// two last generation slot output (SCSP STACK)
	slot_t	slot[32];		// 32 slots
} scsp_t;

////////////////////////////////////////////////////////////////

static s32		scsp_env_table[SCSP_ENV_LEN * 2];	// enveloppe curve table (attack & decay)

static s32		scsp_lfo_sawt_e[SCSP_LFO_LEN];	// lfo sawtooth waveform for enveloppe
static s32		scsp_lfo_squa_e[SCSP_LFO_LEN];	// lfo square waveform for enveloppe
static s32		scsp_lfo_tri_e[SCSP_LFO_LEN];		// lfo triangle waveform for enveloppe
static s32		scsp_lfo_noi_e[SCSP_LFO_LEN];		// lfo noise waveform for enveloppe

static s32		scsp_lfo_sawt_f[SCSP_LFO_LEN];	// lfo sawtooth waveform for frequency
static s32		scsp_lfo_squa_f[SCSP_LFO_LEN];	// lfo square waveform for frequency
static s32		scsp_lfo_tri_f[SCSP_LFO_LEN];		// lfo triangle waveform for frequency
static s32		scsp_lfo_noi_f[SCSP_LFO_LEN];		// lfo noise waveform frequency

static s32		scsp_attack_rate[0x40 + 0x20];	// enveloppe step for attack
static s32		scsp_decay_rate[0x40 + 0x20];		// enveloppe step for decay
static s32		scsp_null_rate[0x20];			// null enveloppe step

static s32		scsp_lfo_step[32];			// directly give the lfo counter step

static u8		scsp_reg[0x1000];

static u8		* scsp_isr;
static u8		* scsp_ccr;
static u8		* scsp_dcr;

static s32		* scsp_bufL;
static s32		* scsp_bufR;
static u32		scsp_buf_len;
static u32		scsp_buf_pos;

static scsp_t   scsp;                                           // SCSP structure


////////////////////////////////////////////////////////////////

static void scsp_env_null_next(slot_t *slot);
static void scsp_release_next(slot_t *slot);
static void scsp_substain_next(slot_t *slot);
static void scsp_decay_next(slot_t *slot);
static void scsp_attack_next(slot_t *slot);

////////////////////////////////////////////////////////////////
// Misc

static int scsp_round(double val)
{
	if ((val - ((double) (int) val)) > 0.5) return (int) (val + 1);
	else return (int) val;
}


////////////////////////////////////////////////////////////////
// Interrupts

static void scsp_main_interrupt(u32 id)
{
//	if (scsp.mcipd & id) return;
	
//      if (id != 0x400) SCSPLOG("scsp main interrupt %.4X\n", id);

	scsp.mcipd |= id;

	if (scsp.mcieb & id)
	{
                SCSPLOG("scsp main interrupt accepted %.4X\n", id);

		if (scsp.mintf != NULL) scsp.mintf();
	}
}

static void scsp_sound_interrupt(u32 id)
{
	u32 level;
	
//	if (scsp.scipd & id) return;

//      SCSPLOG("scsp sound interrupt %.4X\n", id);

	scsp.scipd |= id;
	
	if (scsp.scieb & id)
	{
		level = 0;
                if (id > 0x80) id = 0x80;

		if (scsp.scilv0 & id) level |= 1;
		if (scsp.scilv1 & id) level |= 2;
		if (scsp.scilv2 & id) level |= 4;

                if (id == 0x8) SCSPLOG("scsp sound interrupt accepted %.2X lev=%d\n", id, level);

		if (scsp.sintf != NULL) scsp.sintf(level);
	}


}

////////////////////////////////////////////////////////////////
// Direct Memory Access

static void scsp_dma(void)
{
	if (scsp.dmfl & 0x20)
	{
		// dsp -> scsp_ram
                SCSPLOG("scsp dma: scsp_ram(%08lx) <- reg(%08lx) * %08lx\n", scsp.dmea, scsp.drga, scsp.dmlen);
	}
	else
	{
		// scsp_ram -> dsp
                SCSPLOG("scsp dma: scsp_ram(%08lx) -> reg(%08lx) * %08lx\n", scsp.dmea, scsp.drga, scsp.dmlen);
	}

	scsp_ccr[0x16 ^ 3] &= 0xE0;

	scsp_sound_interrupt(0x10);
	scsp_main_interrupt(0x10);
}

////////////////////////////////////////////////////////////////
// Key ON/OFF event handler

static void scsp_slot_keyon(slot_t *slot)
{
	// key need to be released before being pressed ;)
	if (slot->ecurp == SCSP_ENV_RELEASE)
	{
                SCSPLOG("key on slot %d. 68K PC = %08X slot->sa = %08X slot->lsa = %08X slot->lea = %08X\n", slot - &(scsp.slot[0]), C68k_Get_PC(&C68K), slot->sa, slot->lsa, slot->lea >> SCSP_FREQ_LB);

		// set buffer, loop start/end address of the slot
		if (slot->pcm8b)
                {
			slot->buf8 = (s8*) &(scsp.scsp_ram[slot->sa]);
			if ((slot->sa + (slot->lea >> SCSP_FREQ_LB)) > SCSP_RAM_MASK)
			{
				slot->lea = (SCSP_RAM_MASK - slot->sa) << SCSP_FREQ_LB;
			}
		}
		else
		{
			slot->buf16 = (s16*) &(scsp.scsp_ram[slot->sa & ~1]);
			if ((slot->sa + (slot->lea >> (SCSP_FREQ_LB - 1))) > SCSP_RAM_MASK)
			{
				slot->lea = (SCSP_RAM_MASK - slot->sa) << (SCSP_FREQ_LB - 1);
			}
		}

		slot->fcnt = 0;				// we reset frequency counter
		slot->ecnt = SCSP_ENV_AS;		// we reset enveloppe counter (probably wrong ... convert decay to attack)
		slot->einc = slot->einca;		// enveloppe counter step is attack step
		slot->ecmp = SCSP_ENV_AE;		// limit to reach for next event (Attack End)
		slot->ecurp = SCSP_ENV_ATTACK;	// current enveloppe phase is attack
		slot->enxt = scsp_attack_next;	// function pointer to next event
	}
}

static void scsp_slot_keyoff(slot_t *slot)
{
	// key need to be pressed before being released ;)
	if (slot->ecurp != SCSP_ENV_RELEASE)
	{
                SCSPLOG("key off slot %d\n", slot - &(scsp.slot[0]));

		// if we still are in attack phase at release time, convert attack to decay
		if (slot->ecurp == SCSP_ENV_ATTACK) slot->ecnt = SCSP_ENV_DE - slot->ecnt;
		slot->einc = slot->eincr;
		slot->ecmp = SCSP_ENV_DE;		
		slot->ecurp = SCSP_ENV_RELEASE;
		slot->enxt = scsp_release_next;
	}
}

static void scsp_slot_keyonoff(void)
{
	slot_t *slot;

	for(slot = &(scsp.slot[0]); slot < &(scsp.slot[32]); slot++)
	{
		if (slot->key) scsp_slot_keyon(slot);
		else scsp_slot_keyoff(slot);
	}
}

////////////////////////////////////////////////////////////////
// Enveloppe Events Handler
//
// Max EG level = 0x3FF      /|\
//                          / | \
//                         /  |  \_____
// Min EG level = 0x000 __/   |  |    |\___
//                        A   D1 D2   R
//

static void scsp_env_null_next(slot_t *slot)
{
	// only to prevent null call pointer...
}

static void scsp_release_next(slot_t *slot)
{
	// end of release happened, update to process the next phase...

	slot->ecnt = SCSP_ENV_DE;
	slot->einc = 0;
	slot->ecmp = SCSP_ENV_DE + 1;
	slot->enxt = scsp_env_null_next;
}

static void scsp_substain_next(slot_t *slot)
{
	// end of subtain happened, update to process the next phase...

	slot->ecnt = SCSP_ENV_DE;
	slot->einc = 0;
	slot->ecmp = SCSP_ENV_DE + 1;
	slot->enxt = scsp_env_null_next;
}

static void scsp_decay_next(slot_t *slot)
{
	// end of decay happened, update to process the next phase...

	slot->ecnt = slot->sl;
	slot->einc = slot->eincs;
	slot->ecmp = SCSP_ENV_DE;
	slot->ecurp = SCSP_ENV_SUBSTAIN;
	slot->enxt = scsp_substain_next;
}

static void scsp_attack_next(slot_t *slot)
{
	// end of attack happened, update to process the next phase...

	slot->ecnt = SCSP_ENV_DS;
	slot->einc = slot->eincd;
	slot->ecmp = slot->sl;
	slot->ecurp = SCSP_ENV_DECAY;
	slot->enxt = scsp_decay_next;
}

////////////////////////////////////////////////////////////////
// Slot Access

void scsp_slot_set_b(u32 s, u32 a, u8 d)
{
	slot_t *slot = &(scsp.slot[s]);

        SCSPLOG("slot %d : reg %.2X = %.2X\n", s, a & 0x1F, d);

	scsp_isr[a ^ 3] = d;

	switch(a & 0x1F){

        case 0x00: // KX/KB/SBCTL/SSCTL(high bit)
		slot->key = (d >> 3) & 1;
		slot->sbctl = (d >> 1) & 3;
		slot->ssctl = (slot->ssctl & 1) + ((d & 1) << 1);

		if (d & 0x10) scsp_slot_keyonoff();
		return;

        case 0x01: // SSCTL(low bit)/LPCTL/8B/SA(highest 4 bits)
		slot->ssctl = (slot->ssctl & 2) + ((d >> 7) & 1);
		slot->lpctl = (d >> 5) & 3;
		slot->pcm8b = d & 0x10;
		slot->sa = (slot->sa & 0x0FFFF) + ((d & 0xF) << 16);
		slot->sa &= SCSP_RAM_MASK;
		return;

        case 0x02: // SA(next highest byte)
		slot->sa = (slot->sa & 0xF00FF) + (d << 8);
		slot->sa &= SCSP_RAM_MASK;
		return;

        case 0x03: // SA(low byte)
		slot->sa = (slot->sa & 0xFFF00) + d;
		slot->sa &= SCSP_RAM_MASK;
		return;

        case 0x04: // LSA(high byte)
		slot->lsa = (slot->lsa & (0x00FF << SCSP_FREQ_LB)) + (d << (8 + SCSP_FREQ_LB));
		return;

        case 0x05: // LSA(low byte)
		slot->lsa = (slot->lsa & (0xFF00 << SCSP_FREQ_LB)) + (d << SCSP_FREQ_LB);
		return;

        case 0x06: // LEA(high byte)
		slot->lea = (slot->lea & (0x00FF << SCSP_FREQ_LB)) + (d << (8 + SCSP_FREQ_LB));
		return;

        case 0x07: // LEA(low byte)
		slot->lea = (slot->lea & (0xFF00 << SCSP_FREQ_LB)) + (d << SCSP_FREQ_LB);
		return;

        case 0x08: // D2R/D1R(highest 3 bits)
		slot->sr = (d >> 3) & 0x1F;
		slot->dr = (slot->dr & 0x03) + ((d & 7) << 2);

		if (slot->sr) slot->srp = &scsp_decay_rate[slot->sr << 1];
		else slot->srp = &scsp_null_rate[0];
		if (slot->dr) slot->drp = &scsp_decay_rate[slot->dr << 1];
		else slot->drp = &scsp_null_rate[0];

		slot->eincs = slot->srp[(14 - slot->fsft) >> slot->krs];
		slot->eincd = slot->drp[(14 - slot->fsft) >> slot->krs];
		return;

        case 0x09: // D1R(lowest 2 bits)/EGHOLD/AR
		slot->dr = (slot->dr & 0x1C) + ((d >> 6) & 3);
		slot->eghold = d & 0x20;
		slot->ar = d & 0x1F;

		if (slot->dr) slot->drp = &scsp_decay_rate[slot->dr << 1];
		else slot->drp = &scsp_null_rate[0];
		if (slot->ar) slot->arp = &scsp_attack_rate[slot->ar << 1];
		else slot->arp = &scsp_null_rate[0];

		slot->eincd = slot->drp[(14 - slot->fsft) >> slot->krs];
		slot->einca = slot->arp[(14 - slot->fsft) >> slot->krs];
		return;

        case 0x0A: // LPSLNK/KRS/DL(highest 2 bits)
		slot->lslnk = d & 0x40;
		slot->krs = (d >> 2) & 0xF;
		if (slot->krs == 0xF) slot->krs = 4;
		else slot->krs >>= 2;
		slot->sl &= 0xE0 << SCSP_ENV_LB;
		slot->sl += (d & 3) << (8 + SCSP_ENV_LB);
		slot->sl += SCSP_ENV_DS;				// adjusted for enveloppe compare (ecmp)
		return;

        case 0x0B: // DL(lowest 3 bits)/RR
		slot->sl &= 0x300 << SCSP_ENV_LB;
		slot->sl += (d & 0xE0) << SCSP_ENV_LB;
		slot->sl += SCSP_ENV_DS;				// adjusted for enveloppe compare (ecmp)
		slot->rr = d & 0x1F;

		if (slot->rr) slot->rrp = &scsp_decay_rate[slot->rr << 1];
		else slot->rrp = &scsp_null_rate[0];

		slot->eincr = slot->rrp[(14 - slot->fsft) >> slot->krs];
		return;

        case 0x0C: // STWINH/SDIR
		slot->sdir = d & 2;
		slot->swe = d & 1;
		return;

        case 0x0D: // TL
		slot->tl = (d & 0xFF) << 2;				// adjusted for enveloppe substract
		return;

        case 0x0E: // MDL/MDXSL(highest 4 bits)
		slot->mdl = (d >> 4) & 0xF;				// need to adjust for correct shift
		slot->mdx = (slot->mdx & 3) + ((d & 0xF) << 2);
		return;

        case 0x0F: // MDXSL(lowest 2 bits)/MDYSL
		slot->mdx = (slot->mdx & 0x3C) + ((d >> 6) & 3);
		slot->mdy = d & 0x3F;
		return;

        case 0x10: // OCT/FNS(highest 2 bits)
		if (d & 0x40) slot->fsft = 23 - ((d >> 3) & 0xF);
		else slot->fsft = ((d >> 3) & 7) ^ 7;
		slot->finct = (slot->finct & 0x7F80) + ((d & 3) << (8 + 7));
		slot->finc = (0x20000 + slot->finct) >> slot->fsft;
		return;

        case 0x11: // FNS(low byte)
		slot->finct = (slot->finct & 0x18000) + (d << 7);
		slot->finc = (0x20000 + slot->finct) >> slot->fsft;
		return;

        case 0x12: // LFORE/LFOF/PLFOWS
		if (d & 0x80)
		{
			slot->lfoinc = -1;
			return;
		}
		else if (slot->lfoinc == -1) slot->lfocnt = 0;

		slot->lfoinc = scsp_lfo_step[(d >> 2) & 0x1F];

		switch(d & 3){
		case 0:
			slot->lfofmw = scsp_lfo_sawt_f;
			return;

		case 1:
			slot->lfofmw = scsp_lfo_squa_f;
			return;

		case 2:
			slot->lfofmw = scsp_lfo_tri_f;
			return;

		case 3:
			slot->lfofmw = scsp_lfo_noi_f;
			return;
		}

        case 0x13: // PLFOS/ALFOWS/ALFOS
		if ((d >> 5) & 7) slot->lfofms = ((d >> 5) & 7) + 7;
		else slot->lfofms = 31;
		if (d & 7) slot->lfoems = ((d & 7) ^ 7) + 4;
		else slot->lfoems = 31;

		switch((d >> 3) & 3){
		case 0:
			slot->lfoemw = scsp_lfo_sawt_e;
			return;

		case 1:
			slot->lfoemw = scsp_lfo_squa_e;
			return;

		case 2:
			slot->lfoemw = scsp_lfo_tri_e;
			return;

		case 3:
			slot->lfoemw = scsp_lfo_noi_e;
			return;
		}

        case 0x15: // ISEL/OMXL
		if (d & 7) slot->imxl = ((d & 7) ^ 7) + SCSP_ENV_HB;
		else slot->imxl = 31;
		return;

        case 0x16: // DISDL/DIPAN
		if (d & 0xE0)
		{
			// adjusted for enveloppe calculation
			// some inaccuracy in panning though...
			slot->dislr = slot->disll = (((d >> 5) & 7) ^ 7) + SCSP_ENV_HB;
			if (d & 0x10)
			{
				if ((d & 0xF) == 0xF) slot->disll = 31;
				else slot->disll += (d >> 1) & 7;
			}
			else
			{
				if ((d & 0xF) == 0xF) slot->dislr = 31;
				else slot->dislr += (d >> 1) & 7;
			}
		}
		else slot->dislr = slot->disll = 31;
		return;

        case 0x17: // EFSDL/EFPAN
		if (d & 0xE0)
		{
			slot->efslr = slot->efsll = (((d >> 5) & 7) ^ 7) + SCSP_ENV_HB;
			if (d & 0x10)
			{
				if ((d & 0xF) == 0xF) slot->efsll = 31;
				else slot->efsll += (d >> 1) & 7;
			}
			else
			{
				if ((d & 0xF) == 0xF) slot->efslr = 31;
				else slot->efslr += (d >> 1) & 7;
			}
		}
		else slot->efslr = slot->efsll = 31;
		return;
	}
}

void scsp_slot_set_w(u32 s, s32 a, u16 d)
{
	slot_t *slot = &(scsp.slot[s]);

        SCSPLOG("slot %d : reg %.2X = %.4X\n", s, a & 0x1E, d);

	*(u16 *)&scsp_isr[a ^ 2] = d;

	switch((a >> 1) & 0xF){

        case 0x0: // KYONEX/KYONB/SBCTL/SSCTL/LPCTL/PCM8B/SA(highest 4 bits)
		slot->key = (d >> 11) & 1;
		slot->sbctl = (d >> 9) & 3;
		slot->ssctl = (d >> 7) & 3;
		slot->lpctl = (d >> 5) & 3;
		slot->pcm8b = d & 0x10;
		slot->sa = (slot->sa & 0x0FFFF) | ((d & 0xF) << 16);
		slot->sa &= SCSP_RAM_MASK;

		if (d & 0x1000) scsp_slot_keyonoff();
		return;

        case 0x1: // SA(low word)
		slot->sa = (slot->sa & 0xF0000) | d;
		slot->sa &= SCSP_RAM_MASK;
		return;

        case 0x2: // LSA
		slot->lsa = d << SCSP_FREQ_LB;
		return;

        case 0x3: // LEA
		slot->lea = d << SCSP_FREQ_LB;
		return;

        case 0x4: // D2R/D1R/EGHOLD/AR
		slot->sr = (d >> 11) & 0x1F;
		slot->dr = (d >> 6) & 0x1F;
		slot->eghold = d & 0x20;
		slot->ar = d & 0x1F;

		if (slot->sr) slot->srp = &scsp_decay_rate[slot->sr << 1];
		else slot->srp = &scsp_null_rate[0];
		if (slot->dr) slot->drp = &scsp_decay_rate[slot->dr << 1];
		else slot->drp = &scsp_null_rate[0];
		if (slot->ar) slot->arp = &scsp_attack_rate[slot->ar << 1];
		else slot->arp = &scsp_null_rate[0];

		slot->einca = slot->arp[(14 - slot->fsft) >> slot->krs];
		slot->eincd = slot->drp[(14 - slot->fsft) >> slot->krs];
		slot->eincs = slot->srp[(14 - slot->fsft) >> slot->krs];
		return;

        case 0x5: // LPSLNK/KRS/DL/RR
		slot->lslnk = (d >> 8) & 0x40;
		slot->krs = (d >> 10) & 0xF;
		if (slot->krs == 0xF) slot->krs = 4;
		else slot->krs >>= 2;
		slot->sl = ((d & 0x3E0) << SCSP_ENV_LB) + SCSP_ENV_DS;	// adjusted for enveloppe compare (ecmp)
		slot->rr = d & 0x1F;

		if (slot->rr) slot->rrp = &scsp_decay_rate[slot->rr << 1];
		else slot->rrp = &scsp_null_rate[0];

		slot->eincr = slot->rrp[(14 - slot->fsft) >> slot->krs];
		return;

        case 0x6: // STWINH/SDIR
		slot->sdir = (d >> 8) & 2;
		slot->swe = (d >> 8) & 1;
		slot->tl = (d & 0xFF) << 2;				// adjusted for enveloppe substract
		return;

        case 0x7: // MDL/MDXSL/MDYSL
		slot->mdl = (d >> 12) & 0xF;				// need to adjust for correct shift
		slot->mdx = (d >> 6) & 0x3F;
		slot->mdy = d & 0x3F;
		return;

        case 0x8: // OCT/FNS
		if (d & 0x4000) slot->fsft = 23 - ((d >> 11) & 0xF);
		else slot->fsft = (((d >> 11) & 7) ^ 7);
		slot->finc = ((0x400 + (d & 0x3FF)) << 7) >> slot->fsft;
		return;

        case 0x9: // LFORE/LFOF/PLFOWS/PLFOS/ALFOWS/ALFOS
		if (d & 0x8000)
		{
			slot->lfoinc = -1;
			return;
		}
		else if (slot->lfoinc == -1) slot->lfocnt = 0;

		slot->lfoinc = scsp_lfo_step[(d >> 10) & 0x1F];
		if ((d >> 5) & 7) slot->lfofms = ((d >> 5) & 7) + 7;
		else slot->lfofms = 31;
		if (d & 7) slot->lfoems = ((d & 7) ^ 7) + 4;
		else slot->lfoems = 31;

		switch((d >> 8) & 3){
		case 0:
			slot->lfofmw = scsp_lfo_sawt_f;
			break;

		case 1:
			slot->lfofmw = scsp_lfo_squa_f;
			break;

		case 2:
			slot->lfofmw = scsp_lfo_tri_f;
			break;

		case 3:
			slot->lfofmw = scsp_lfo_noi_f;
			break;
		}

		switch((d >> 3) & 3){
		case 0:
			slot->lfoemw = scsp_lfo_sawt_e;
			return;

		case 1:
			slot->lfoemw = scsp_lfo_squa_e;
			return;

		case 2:
			slot->lfoemw = scsp_lfo_tri_e;
			return;

		case 3:
			slot->lfoemw = scsp_lfo_noi_e;
			return;
		}

        case 0xA: // ISEL/OMXL
		if (d & 7) slot->imxl = ((d & 7) ^ 7) + SCSP_ENV_HB;
		else slot->imxl = 31;
		return;

        case 0xB: // DISDL/DIPAN/EFSDL/EFPAN
		if (d & 0xE000)
		{
			// adjusted fr enveloppe calculation
			// some accuracy lose for panning here...
			slot->dislr = slot->disll = (((d >> 13) & 7) ^ 7) + SCSP_ENV_HB;
			if (d & 0x1000)
			{
				if ((d & 0xF00) == 0xF00) slot->disll = 31;
				else slot->disll += (d >> 9) & 7;
			}
			else
			{
				if ((d & 0xF00) == 0xF00) slot->dislr = 31;
				else slot->dislr += (d >> 9) & 7;
			}
		}
		else slot->dislr = slot->disll = 31;

		if (d & 0xE0)
		{
			slot->efslr = slot->efsll = (((d >> 5) & 7) ^ 7) + SCSP_ENV_HB;
			if (d & 0x10)
			{
				if ((d & 0xF) == 0xF) slot->efsll = 31;
				else slot->efsll += (d >> 1) & 7;
			}
			else
			{
				if ((d & 0xF) == 0xF) slot->efslr = 31;
				else slot->efslr += (d >> 1) & 7;
			}
		}
		else slot->efslr = slot->efsll = 31;
		return;
	}
}

u8 scsp_slot_get_b(u32 s, u32 a)
{
//        slot_t *slot = &(scsp.slot[s]);

	a &= 0x1F;

        SCSPLOG("r_b slot %d : reg %.2X\n", s, a);

	if (a == 0x00) return scsp_isr[a ^ 3] & 0xEF;

	return scsp_isr[a ^ 3];
}

u16 scsp_slot_get_w(u32 s, u32 a)
{
//        slot_t *slot = &(scsp.slot[s]);

	a = (a >> 1) & 0xF;

        SCSPLOG("r_w slot %d : reg %.2X\n", s, a * 2);

	if (a == 0x00) return *(u16 *)&scsp_isr[a ^ 2] & 0xEFFF;

	return *(u16 *)&scsp_isr[a ^ 2];
}

////////////////////////////////////////////////////////////////
// SCSP Access

void scsp_set_b(u32 a, u8 d)
{
//      if (a != 0x41D) SCSPLOG("scsp : reg %.2X = %.2X\n", a & 0x3F, d);
        if ((a != 0x408) && (a != 0x41D)) SCSPLOG("scsp : reg %.2X = %.2X\n", a & 0x3F, d);
//      SCSPLOG("scsp : reg %.2X = %.2X\n", a & 0x3F, d);

	scsp_ccr[a ^ 3] = d;

	switch(a & 0x3F){
        case 0x00: // MEM4MB/DAC18B
                scsp.mem4b = (d >> 1) & 0x1;
                if (scsp.mem4b)
                   C68k_Set_Fetch(&C68K, 0x000000, 0x080000, (pointer)SoundRam);
                else
                {
                   C68k_Set_Fetch(&C68K, 0x000000, 0x040000, (pointer)SoundRam);
                   C68k_Set_Fetch(&C68K, 0x040000, 0x080000, (pointer)SoundRam);
                   C68k_Set_Fetch(&C68K, 0x080000, 0x0C0000, (pointer)SoundRam);
                   C68k_Set_Fetch(&C68K, 0x0C0000, 0x100000, (pointer)SoundRam);
                }
                return;

        case 0x01: // VER/MVOL
		scsp.mvol = d & 0xF;
		return;

        case 0x02: // RBL(high bit)
		scsp.rbl = (scsp.rbl & 1) + ((d & 1) << 1);
		return;

        case 0x03: // RBL(low bit)/RBP
		scsp.rbl = (scsp.rbl & 2) + ((d >> 7) & 1);
		scsp.rbp = (d & 0x7F) * (4 * 1024 * 2);
		return;

        case 0x07: // MOBUF
		scsp_midi_out_send(d);
		return;

        case 0x08: // MSLC/CA
		scsp.mslc = (d >> 3) & 0x1F;
		return;

        case 0x12: // DMEAL(high byte)
		scsp.dmea = (scsp.dmea & 0x700FE) + (d << 8);
		return;

        case 0x13: // DMEAL(low byte)
		scsp.dmea = (scsp.dmea & 0x7FF00) + (d & 0xFE);
		return;

        case 0x14: // DMEAH(high byte)
		scsp.dmea = (scsp.dmea & 0xFFFE) + ((d & 0x70) << 12);
		scsp.drga = (scsp.drga & 0xFE) + ((d & 0xF) << 8);
		return;

        case 0x15: // DMEAH(low byte)
		scsp.drga = (scsp.drga & 0xF00) + (d & 0xFE);
		return;

        case 0x16: // DGATE/DDIR/DEXE/DTLG(upper 4 bits)
		scsp.dmlen = (scsp.dmlen & 0xFE) + ((d & 0xF) << 8);
		if ((scsp.dmfl = d & 0xF0) & 0x10) scsp_dma();
		return;

        case 0x17: // DTLG(lower byte)
		scsp.dmlen = (scsp.dmlen & 0xF00) + (d & 0xFE);
		return;

        case 0x18: // TACTL
		scsp.timasd = d & 7;
		return;

        case 0x19: // TIMA
		scsp.timacnt = d << 8;
		return;

        case 0x1A: // TBCTL
		scsp.timbsd = d & 7;
		return;

        case 0x1B: // TIMB
		scsp.timbcnt = d << 8;
		return;

        case 0x1C: // TCCTL
		scsp.timcsd = d & 7;
		return;

        case 0x1D: // TIMC
		scsp.timccnt = d << 8;
		return;

        case 0x1E: // SCIEB(high byte)
		scsp.scieb = (scsp.scieb & 0xFF) + (d << 8);
		return;

        case 0x1F: // SCIEB(low byte)
		scsp.scieb = (scsp.scieb & 0x700) + d;
		return;

        case 0x21: // SCIPD(low byte)
		if (d & 0x20) scsp_sound_interrupt(0x20);
		return;

        case 0x22: // SCIRE(high byte)
		scsp.scipd &= ~(d << 8);
		return;

        case 0x23: // SCIRE(low byte)
		scsp.scipd &= ~(u32)d;
		return;

        case 0x25: // SCILV0
		scsp.scilv0 = d;
		return;

        case 0x27: // SCILV1
		scsp.scilv1 = d;
		return;

        case 0x29: // SCILV2
		scsp.scilv2 = d;
		return;

        case 0x2A: // MCIEB(high byte)
		scsp.mcieb = (scsp.mcieb & 0xFF) + (d << 8);
		return;

        case 0x2B: // MCIEB(low byte)
		scsp.mcieb = (scsp.mcieb & 0x700) + d;
		return;

        case 0x2D: // MCIPD(low byte)
		if (d & 0x20) scsp_main_interrupt(0x20);
		return;

        case 0x2E: // MCIRE(high byte)
		scsp.mcipd &= ~(d << 8);
		return;

        case 0x2F: // MCIRE(low byte)
		scsp.mcipd &= ~(u32)d;
		return;
	}
}

void scsp_set_w(u32 a, u16 d)
{
        if ((a != 0x418) && (a != 0x41A) && (a != 0x422)) SCSPLOG("scsp : reg %.2X = %.4X\n", a & 0x3E, d);
//      SCSPLOG("scsp : reg %.2X = %.4X\n", a & 0x3E, d);

	*(u16 *)&scsp_ccr[a ^ 2] = d;

	switch((a >> 1) & 0x1F){

        case 0x00: // MEM4MB/DAC18B/VER/MVOL
                scsp.mem4b = (d >> 9) & 0x1;                
		scsp.mvol = d & 0xF;
                if (scsp.mem4b)
                   C68k_Set_Fetch(&C68K, 0x000000, 0x080000, (pointer)SoundRam);
                else
                {
                   C68k_Set_Fetch(&C68K, 0x000000, 0x040000, (pointer)SoundRam);
                   C68k_Set_Fetch(&C68K, 0x040000, 0x080000, (pointer)SoundRam);
                   C68k_Set_Fetch(&C68K, 0x080000, 0x0C0000, (pointer)SoundRam);
                   C68k_Set_Fetch(&C68K, 0x0C0000, 0x100000, (pointer)SoundRam);
                }
		return;

        case 0x01: // RBL/RBP
		scsp.rbl = (d >> 7) & 3;
		scsp.rbp = (d & 0x7F) * (4 * 1024 * 2);
		return;

        case 0x03: // MOBUF
		scsp_midi_out_send(d & 0xFF);
		return;

        case 0x04: // MSLC/CA
		scsp.mslc = (d >> 11) & 0x1F;
		scsp.ca = (d >> 7) & 0xF;
		return;

        case 0x09: // DMEAL
		scsp.dmea = (scsp.dmea & 0x70000) + (d & 0xFFFE);
		return;

        case 0x0A: // DMEAH/DRGA
		scsp.dmea = (scsp.dmea & 0xFFFE) + ((d & 0x7000) << 4);
		scsp.drga = d & 0xFFE;
		return;

        case 0x0B: // DGATE/DDIR/DEXE/DTLG
		scsp.dmlen = d & 0xFFE;
		if ((scsp.dmfl = ((d >> 8) & 0xF0)) & 0x10) scsp_dma();
		return;

        case 0x0C: // TACTL/TIMA
		scsp.timasd = (d >> 8) & 7;
		scsp.timacnt = (d & 0xFF) << 8;
		return;

        case 0x0D: // TBCTL/TIMB
		scsp.timbsd = (d >> 8) & 7;
		scsp.timbcnt = (d & 0xFF) << 8;
		return;

        case 0x0E: // TCCTL/TIMC
		scsp.timcsd = (d >> 8) & 7;
		scsp.timccnt = (d & 0xFF) << 8;
		return;

        case 0x0F: // SCIEB
		scsp.scieb = d;
		return;

        case 0x10: // SCIPD
		if (d & 0x20) scsp_sound_interrupt(0x20);
		return;

        case 0x11: // SCIRE
		scsp.scipd &= ~d;
		return;

        case 0x12: // SCILV0
		scsp.scilv0 = d;
		return;

        case 0x13: // SCILV1
		scsp.scilv1 = d;
		return;

        case 0x14: // SCILV2
		scsp.scilv2 = d;
		return;

        case 0x15: // MCIEB
		scsp.mcieb = d;
		return;

        case 0x16: // MCIPD
		if (d & 0x20) scsp_main_interrupt(0x20);
		return;

        case 0x18: // MCIRE
		scsp.mcipd &= ~d;
		return;
	}
}

u8 scsp_get_b(u32 a)
{
	a &= 0x3F;

//      if (a != 0x21) SCSPLOG("r_b scsp : reg %.2X\n", a);
        if ((a != 0x09) && (a != 0x21)) SCSPLOG("r_b scsp : reg %.2X\n", a);
//      if (a == 0x09) SCSPLOG("r_b scsp 09 = %.2X\n", ((scsp.slot[scsp.mslc].fcnt >> (SCSP_FREQ_LB + 12)) & 0x1) << 7);
//      SCSPLOG("r_b scsp : reg %.2X\n", a);

	switch(a){

        case 0x01: // VER/MVOL
		scsp_ccr[a ^ 3] &= 0x0F;
		break;

        case 0x04: // Midi flags register
		return scsp.midflag;

        case 0x05: // MIBUF
		return scsp_midi_in_read();

        case 0x07: // MOBUF
		return scsp_midi_out_read();

        case 0x08: // MSLC/CA(highest 3 bits)
		return ((scsp.slot[scsp.mslc].fcnt >> (SCSP_FREQ_LB + 12)) & 0xE) >> 1;

        case 0x09: // CA(lowest bit)
		return ((scsp.slot[scsp.mslc].fcnt >> (SCSP_FREQ_LB + 12)) & 0x1) << 7;

        case 0x1E: // SCIEB(high byte)
                return (scsp.scieb >> 8);

        case 0x1F: // SCIEB(low byte)
                return scsp.scieb;

        case 0x20: // SCIPD(high byte)
                return (scsp.scipd >> 8);

        case 0x21: // SCIPD(low byte)
                return scsp.scipd;

        case 0x2C: // MCIPD(high byte)
                return (scsp.mcipd >> 8);

        case 0x2D: // MCIPD(low byte)
                return scsp.mcipd;
	}

	return scsp_ccr[a ^ 3];
}

u16 scsp_get_w(u32 a)
{
	a = (a >> 1) & 0x1F;

        if (a != 0x10) SCSPLOG("r_w scsp : reg %.2X\n", a * 2);

	switch(a){

        case 0x00: // MEM4MB/DAC18B/VER/MVOL
		*(u16 *)&scsp_ccr[a ^ 2] &= 0xFF0F;
		break;

        case 0x02: // Midi flags/MIBUF
		return (scsp.midflag << 8) | scsp_midi_in_read();

        case 0x03: // MOBUF
		return scsp_midi_out_read();

        case 0x04: // MSLC/CA
                return ((scsp.slot[scsp.mslc].fcnt >> (SCSP_FREQ_LB + 12)) & 0xF) << 7;

        case 0x0F: // SCIEB
                return scsp.scieb;

        case 0x10: // SCIPD
                return scsp.scipd;

        case 0x16: // MCIPD
                return scsp.mcipd;
	}

	return *(u16 *)&scsp_ccr[a ^ 2];
}

////////////////////////////////////////////////////////////////
// Synth Slot
//
//      SCSPLOG("outL=%.8X bufL=%.8X disll=%d\n", outL, scsp_bufL[scsp_buf_pos], slot->disll);

////////////////////////////////////////////////////////////////

#ifdef WORDS_BIGENDIAN
#define SCSP_GET_OUT_8B \
   out = (s32) slot->buf8[(slot->fcnt >> SCSP_FREQ_LB)];
#else
#define SCSP_GET_OUT_8B \
   out = (s32) slot->buf8[(slot->fcnt >> SCSP_FREQ_LB) ^ 1];
#endif

#define SCSP_GET_OUT_16B        \
                out = (s32) slot->buf16[slot->fcnt >> SCSP_FREQ_LB];

#define SCSP_GET_ENV		\
		env = scsp_env_table[slot->ecnt >> SCSP_ENV_LB] - slot->tl;

#define SCSP_GET_ENV_LFO	\
		env = (scsp_env_table[slot->ecnt >> SCSP_ENV_LB] - slot->tl) - (slot->lfoemw[(slot->lfocnt >> SCSP_LFO_LB) & SCSP_LFO_MASK] >> slot->lfoems);

#define SCSP_OUT_8B_L		\
		if ((out) && (env > 0))							\
		{										\
			out *= env;								\
			scsp_bufL[scsp_buf_pos] += out >> (slot->disll - 8);	\
		}

#define SCSP_OUT_8B_R		\
		if ((out) && (env > 0))							\
		{										\
			out *= env;								\
			scsp_bufR[scsp_buf_pos] += out >> (slot->dislr - 8);	\
		}

#define SCSP_OUT_8B_LR		\
		if ((out) && (env > 0))							\
		{										\
			out *= env;								\
			scsp_bufL[scsp_buf_pos] += out >> (slot->disll - 8);	\
			scsp_bufR[scsp_buf_pos] += out >> (slot->dislr - 8);	\
		}

#define SCSP_OUT_16B_L		\
		if ((out) && (env > 0))						\
		{									\
			out *= env;							\
			scsp_bufL[scsp_buf_pos] += out >> slot->disll;	\
		}

#define SCSP_OUT_16B_R		\
		if ((out) && (env > 0))						\
		{									\
			out *= env;							\
			scsp_bufR[scsp_buf_pos] += out >> slot->dislr;	\
		}

#define SCSP_OUT_16B_LR		\
		if ((out) && (env > 0))						\
		{									\
			out *= env;							\
			scsp_bufL[scsp_buf_pos] += out >> slot->disll;	\
			scsp_bufR[scsp_buf_pos] += out >> slot->dislr;	\
		}

#define SCSP_UPDATE_PHASE	\
		if ((slot->fcnt += slot->finc) > slot->lea)		\
		{									\
			if (slot->lpctl) slot->fcnt = slot->lsa;		\
			else								\
			{								\
				slot->ecnt = SCSP_ENV_DE;			\
				return;						\
			}								\
		}

#define SCSP_UPDATE_PHASE_LFO	\
		slot->fcnt += slot->finc + ((slot->lfofmw[(slot->lfocnt >> SCSP_LFO_LB) & SCSP_LFO_MASK] << slot->lfofms) >> slot->fsft);	\
		if ((slot->fcnt += slot->finc) > slot->lea)		\
		{									\
			if (slot->lpctl) slot->fcnt = slot->lsa;		\
			else								\
			{								\
				slot->ecnt = SCSP_ENV_DE;			\
				return;						\
			}								\
		}

#define SCSP_UPDATE_ENV		\
		if ((slot->ecnt += slot->einc) >= slot->ecmp)	\
		{								\
			slot->enxt(slot);					\
			if (slot->ecnt >= SCSP_ENV_DE) return;	\
		}

#define SCSP_UPDATE_LFO		\
		slot->lfoinc += slot->lfocnt;

////////////////////////////////////////////////////////////////

static void scsp_slot_update_null(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
                SCSP_GET_ENV

		SCSP_UPDATE_PHASE
                SCSP_UPDATE_ENV
	}
}

////////////////////////////////////////////////////////////////
// Normal 8 bits

static void scsp_slot_update_8B_L(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		// env = [0..0x3FF] - slot->tl
                SCSP_GET_OUT_8B
		SCSP_GET_ENV

		// don't waste time if no sound...
		SCSP_OUT_8B_L

		// calculate new frequency (phase) counter and enveloppe counter
		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

static void scsp_slot_update_8B_R(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV

		SCSP_OUT_8B_R

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

static void scsp_slot_update_8B_LR(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
                SCSP_GET_OUT_8B
		SCSP_GET_ENV

		SCSP_OUT_8B_LR

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

////////////////////////////////////////////////////////////////
// Enveloppe LFO modulation 8 bits

static void scsp_slot_update_E_8B_L(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_L

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_E_8B_R(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_R

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_E_8B_LR(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_LR

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Frequency LFO modulation 8 bits

static void scsp_slot_update_F_8B_L(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV

		SCSP_OUT_8B_L

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_8B_R(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV

		SCSP_OUT_8B_R

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_8B_LR(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV

		SCSP_OUT_8B_LR

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Enveloppe & Frequency LFO modulation 8 bits

static void scsp_slot_update_F_E_8B_L(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_L

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_E_8B_R(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_R

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_E_8B_LR(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_8B
		SCSP_GET_ENV_LFO

		SCSP_OUT_8B_LR

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Normal 16 bits

static void scsp_slot_update_16B_L(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_L

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

static void scsp_slot_update_16B_R(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_R

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

static void scsp_slot_update_16B_LR(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_LR

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
	}
}

////////////////////////////////////////////////////////////////
// Enveloppe LFO modulation 16 bits

static void scsp_slot_update_E_16B_L(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_L

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_E_16B_R(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_R

                SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_E_16B_LR(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_LR

		SCSP_UPDATE_PHASE
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Frequency LFO modulation 16 bits

static void scsp_slot_update_F_16B_L(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_L

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_16B_R(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_R

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_16B_LR(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV

		SCSP_OUT_16B_LR

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Enveloppe & Frequency LFO modulation 16 bits

static void scsp_slot_update_F_E_16B_L(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_L

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_E_16B_R(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_R

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

static void scsp_slot_update_F_E_16B_LR(slot_t *slot)
{
	s32 out, env;

	for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
	{
		SCSP_GET_OUT_16B
		SCSP_GET_ENV_LFO

		SCSP_OUT_16B_LR

		SCSP_UPDATE_PHASE_LFO
		SCSP_UPDATE_ENV
		SCSP_UPDATE_LFO
	}
}

////////////////////////////////////////////////////////////////
// Update functions

static void (*scsp_slot_update_p[2][2][2][2][2])(slot_t *slot) =
{
	// NO FMS
	{	// NO EMS
		{	// 8 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_8B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_8B_L,
					// RIGHT
					scsp_slot_update_8B_LR
				},
			},
			// 16 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_16B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_16B_L,
					// RIGHT
					scsp_slot_update_16B_LR
				},
			}
		},
		// EMS
		{	// 8 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_E_8B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_E_8B_L,
					// RIGHT
					scsp_slot_update_E_8B_LR
				},
			},
			// 16 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_E_16B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_E_16B_L,
					// RIGHT
					scsp_slot_update_E_16B_LR
				},
			}
		}
	},
	// FMS
	{	// NO EMS
		{	// 8 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_F_8B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_F_8B_L,
					// RIGHT
					scsp_slot_update_F_8B_LR
				},
			},
			// 16 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_F_16B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_F_16B_L,
					// RIGHT
					scsp_slot_update_F_16B_LR
				},
			}
		},
		// EMS
		{	// 8 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_F_E_8B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_F_E_8B_L,
					// RIGHT
					scsp_slot_update_F_E_8B_LR
				},
			},
			// 16 BITS
			{	// NO LEFT
				{	// NO RIGHT
					scsp_slot_update_null,
					// RIGHT
					scsp_slot_update_F_E_16B_R
				},
				// LEFT
				{	// NO RIGHT
					scsp_slot_update_F_E_16B_L,
					// RIGHT
					scsp_slot_update_F_E_16B_LR
				},
			}
		}
	}
};

void scsp_update(s32 *bufL, s32 *bufR, u32 len)
{
	slot_t *slot;

	scsp_bufL = bufL;
	scsp_bufR = bufR;

	for(slot = &(scsp.slot[0]); slot < &(scsp.slot[32]); slot++)
	{
		if (slot->ecnt >= SCSP_ENV_DE) continue;	// enveloppe null...

		if (slot->ssctl)
                {
                   // Still not correct, but at least this fixes games
                   // that rely on Call Address information
                   scsp_buf_len = len;
                   scsp_buf_pos = 0;

                   for(; scsp_buf_pos < scsp_buf_len; scsp_buf_pos++)
                   {
                      if ((slot->fcnt += slot->finc) > slot->lea)
                      {
                         if (slot->lpctl) slot->fcnt = slot->lsa;
                         else
                         {
                            slot->ecnt = SCSP_ENV_DE;
                            break;
                         }
                      }
                   }

                   continue; // not yet supported!
                }

		scsp_buf_len = len;
		scsp_buf_pos = 0;

		// take effect sound volume if no direct sound volume...
		if ((slot->disll == 31) && (slot->dislr == 31))
		{
			slot->disll = slot->efsll;
			slot->dislr = slot->efslr;
		}

//              SCSPLOG("update : VL=%d  VR=%d CNT=%.8X STEP=%.8X\n", slot->disll, slot->dislr, slot->fcnt, slot->finc);

		scsp_slot_update_p[(slot->lfofms == 31)?0:1][(slot->lfoems == 31)?0:1][(slot->pcm8b == 0)?1:0][(slot->disll == 31)?0:1][(slot->dislr == 31)?0:1](slot);
	}
}

void scsp_update_timer(u32 len)
{
	if (scsp.timacnt != 0xFF00)
	{
		scsp.timacnt += len << (8 - scsp.timasd);
		if (scsp.timacnt >= 0xFF00)
		{
			scsp_sound_interrupt(0x40);
			scsp_main_interrupt(0x40);
			scsp.timacnt = 0xFF00;
		}
	}

	if (scsp.timbcnt != 0xFF00)
	{
		scsp.timbcnt += len << (8 - scsp.timbsd);
		if (scsp.timbcnt >= 0xFF00)
		{
			scsp_sound_interrupt(0x80);
			scsp_main_interrupt(0x80);
			scsp.timbcnt = 0xFF00;
		}
	}

	if (scsp.timccnt != 0xFF00)
	{
		scsp.timccnt += len << (8 - scsp.timcsd);
		if (scsp.timccnt >= 0xFF00)
		{
			scsp_sound_interrupt(0x100);
			scsp_main_interrupt(0x100);
			scsp.timccnt = 0xFF00;
		}
	}

	// 1F interrupt can't be accurate here...
	if (len)
	{
		scsp_sound_interrupt(0x400);
		scsp_main_interrupt(0x400);
	}
}

////////////////////////////////////////////////////////////////
// MIDI

void scsp_midi_in_send(u8 data)
{
	if (scsp.midflag & SCSP_MIDI_IN_EMP)
	{
		scsp_sound_interrupt(0x8);
		scsp_main_interrupt(0x8);
	}

	scsp.midflag &= ~SCSP_MIDI_IN_EMP;

	if (scsp.midincnt > 3)
	{
		scsp.midflag |= SCSP_MIDI_IN_OVF;
		return;
	}

	scsp.midinbuf[scsp.midincnt++] = data;

	if (scsp.midincnt > 3) scsp.midflag |= SCSP_MIDI_IN_FUL;
}

void scsp_midi_out_send(u8 data)
{
	scsp.midflag &= ~SCSP_MIDI_OUT_EMP;

	if (scsp.midoutcnt > 3) return;

	scsp.midoutbuf[scsp.midoutcnt++] = data;

	if (scsp.midoutcnt > 3) scsp.midflag |= SCSP_MIDI_OUT_FUL;
}

u8 scsp_midi_in_read(void)
{
	u8 data;

	scsp.midflag &= ~(SCSP_MIDI_IN_OVF | SCSP_MIDI_IN_FUL);

	if (scsp.midincnt > 0)
	{
		if (scsp.midincnt > 1)
		{
			scsp_sound_interrupt(0x8);
			scsp_main_interrupt(0x8);
		}
		else scsp.midflag |= SCSP_MIDI_IN_EMP;

		data = scsp.midinbuf[0];

		switch((--scsp.midincnt) & 3)
		{
			case 1:
				scsp.midinbuf[0] = scsp.midinbuf[1];
				break;

			case 2:
				scsp.midinbuf[0] = scsp.midinbuf[1];
				scsp.midinbuf[1] = scsp.midinbuf[2];
				break;

			case 3:
				scsp.midinbuf[0] = scsp.midinbuf[1];
				scsp.midinbuf[1] = scsp.midinbuf[2];
				scsp.midinbuf[2] = scsp.midinbuf[3];
				break;
		}

		return data;
	}

	return 0xFF;
}

u8 scsp_midi_out_read(void)
{
	u8 data;

	scsp.midflag &= ~SCSP_MIDI_OUT_FUL;

	if (scsp.midoutcnt > 0)
	{
		if (scsp.midoutcnt == 1)
		{
			scsp.midflag |= SCSP_MIDI_OUT_EMP;
			scsp_sound_interrupt(0x200);
			scsp_main_interrupt(0x200);
		}

		data = scsp.midoutbuf[0];

		switch(--scsp.midoutcnt & 3)
		{
			case 1:
				scsp.midoutbuf[0] = scsp.midoutbuf[1];
				break;

			case 2:
				scsp.midoutbuf[0] = scsp.midoutbuf[1];
				scsp.midoutbuf[1] = scsp.midoutbuf[2];
				break;

			case 3:
				scsp.midoutbuf[0] = scsp.midoutbuf[1];
				scsp.midoutbuf[1] = scsp.midoutbuf[2];
				scsp.midoutbuf[2] = scsp.midoutbuf[3];
				break;
		}

		return data;
	}

	return 0xFF;
}

////////////////////////////////////////////////////////////////
// Access

void FASTCALL scsp_w_b(u32 a, u8 d)
{
	a &= 0xFFF;

	if (a < 0x400)
	{
		scsp_slot_set_b(a >> 5, a, d);
		return;
	}
	else if (a < 0x600)
	{
		if (a < 0x440)
		{
			scsp_set_b(a, d);
			return;
		}
	}
	else if (a < 0x700)
	{

	}
	else if (a < 0xee4)
	{
		a &= 0x3ff;
		scsp_dcr[a ^ 3] = d;
	}

        SCSPLOG("WARNING: scsp w_b to %08lx w/ %02x\n", a, d);
}

////////////////////////////////////////////////////////////////

void FASTCALL scsp_w_w(u32 a, u16 d)
{
	if (a & 1)
	{
                SCSPLOG("ERROR: scsp w_w misaligned : %.8X\n", a);
	}

	a &= 0xFFE;

	if (a < 0x400)
	{
		scsp_slot_set_w(a >> 5, a, d);
		return;
	}
	else if (a < 0x600)
	{
		if (a < 0x440)
		{
			scsp_set_w(a, d);
			return;
		}
	}
	else if (a < 0x700)
	{

	}
	else if (a < 0xee4)
	{
		a &= 0x3ff;
		*(u16 *)&scsp_dcr[a ^ 2] = d;
	}

        SCSPLOG("WARNING: scsp w_w to %08lx w/ %04x\n", a, d);
}

////////////////////////////////////////////////////////////////

void FASTCALL scsp_w_d(u32 a, u32 d)
{
	if (a & 3)
	{
                SCSPLOG("ERROR: scsp w_d misaligned : %.8X\n", a);
	}

	a &= 0xFFC;

	if (a < 0x400)
	{
		scsp_slot_set_w(a >> 5, a + 0, d >> 16);
		scsp_slot_set_w(a >> 5, a + 2, d & 0xFFFF);
		return;
	}
	else if (a < 0x600)
	{
		if (a < 0x440)
		{
			scsp_set_w(a + 0, d >> 16);
			scsp_set_w(a + 2, d & 0xFFFF);
			return;
		}
	}
	else if (a < 0x700)
	{

	}
	else if (a < 0xee4)
	{
		a &= 0x3ff;
		*(u32 *)&scsp_dcr[a] = d;
	}

        SCSPLOG("WARNING: scsp w_d to %08lx w/ %08lx\n", a, d);
}

////////////////////////////////////////////////////////////////

u8 FASTCALL scsp_r_b(u32 a)
{
	a &= 0xFFF;

	if (a < 0x400)
	{
		return scsp_slot_get_b(a >> 5, a);
	}
	else if (a < 0x600)
	{
		if (a < 0x440) return scsp_get_b(a);
	}
	else if (a < 0x700)
	{

	}
	else if (a < 0xee4)
	{

	}

        SCSPLOG("WARNING: scsp r_b to %08lx\n", a);

	return 0;
}

////////////////////////////////////////////////////////////////

u16 FASTCALL scsp_r_w(u32 a)
{
	if (a & 1)
	{
                SCSPLOG("ERROR: scsp r_w misaligned : %.8X\n", a);
	}

	a &= 0xFFE;

	if (a < 0x400)
	{
		return scsp_slot_get_w(a >> 5, a);
	}
	else if (a < 0x600)
	{
		if (a < 0x440) return scsp_get_w(a);
	}
	else if (a < 0x700)
	{

	}
	else if (a < 0xee4)
	{

	}

        SCSPLOG("WARNING: scsp r_w to %08lx\n", a);

	return 0;
}

////////////////////////////////////////////////////////////////

u32 FASTCALL scsp_r_d(u32 a)
{
	if (a & 3)
	{
                SCSPLOG("ERROR: scsp r_d misaligned : %.8X\n", a);
	}

	a &= 0xFFC;

	if (a < 0x400)
	{
		return (scsp_slot_get_w(a >> 5, a + 0) << 16) + scsp_slot_get_w(a >> 5, a + 1);
	}
	else if (a < 0x600)
	{
		if (a < 0x440) return (scsp_get_w(a + 0) << 16) + scsp_get_w(a + 1);
	}
	else if (a < 0x700)
	{

	}
	else if (a < 0xee4)
	{

	}

        SCSPLOG("WARNING: scsp r_d to %08lx\n", a);

	return 0;
}

////////////////////////////////////////////////////////////////
// Interface

void scsp_shutdown(void)
{

}

void scsp_reset(void)
{
	slot_t *slot;

	memset(scsp_reg,		0, 0x1000);

        scsp.mem4b              = 0;
	scsp.mvol		= 0;
	scsp.rbl		= 0;
	scsp.rbp		= 0;
	scsp.mslc		= 0;
	scsp.ca		= 0;

	scsp.dmea		= 0;
	scsp.drga		= 0;
	scsp.dmfl		= 0;
	scsp.dmlen		= 0;

	scsp.midincnt	= 0;
	scsp.midoutcnt	= 0;
	scsp.midflag	= SCSP_MIDI_IN_EMP | SCSP_MIDI_OUT_EMP;
	scsp.midflag2	= 0;

	scsp.timacnt	= 0xFF00;
	scsp.timbcnt	= 0xFF00;
	scsp.timccnt	= 0xFF00;
	scsp.timasd		= 0;
	scsp.timbsd		= 0;
	scsp.timcsd		= 0;

	scsp.mcieb		= 0;
	scsp.mcipd		= 0;
	scsp.scieb		= 0;
	scsp.scipd		= 0;
	scsp.scilv0		= 0;
	scsp.scilv1		= 0;
	scsp.scilv2		= 0;

	for(slot = &(scsp.slot[0]); slot < &(scsp.slot[32]); slot++)
	{
		memset(slot, 0, sizeof(slot_t));
		slot->ecnt = SCSP_ENV_DE;		// slot off
		slot->dislr = slot->disll = 31;	// direct level sound off
		slot->efslr = slot->efsll = 31;	// effect level sound off
                slot->buf8 = (s8*)&(scsp.scsp_ram[1]);
                slot->buf16 = (s16*)&(scsp.scsp_ram[0]);
	}
}

void scsp_init(u8 *scsp_ram, void (*sint_hand)(u32), void (*mint_hand)(void))
{
	u32 i, j;
	double x;

	scsp_shutdown();

	scsp_isr		= &scsp_reg[0x0000];
	scsp_ccr		= &scsp_reg[0x0400];
	scsp_dcr		= &scsp_reg[0x0700];

	scsp.scsp_ram	= scsp_ram;
	scsp.sintf		= sint_hand;
	scsp.mintf		= mint_hand;

	for(i = 0; i < SCSP_ENV_LEN; i++)
	{
		// Attack Curve (x^4 ?)
		x = pow(((double) (SCSP_ENV_MASK - i) / (double) (SCSP_ENV_LEN)), 4);
		x *= (double) SCSP_ENV_LEN;
		scsp_env_table[i] = SCSP_ENV_MASK - (s32) x;

		// Decay curve (x = linear)
		x = pow(((double) (i) / (double) (SCSP_ENV_LEN)), 1);
		x *= (double) SCSP_ENV_LEN;
		scsp_env_table[i + SCSP_ENV_LEN] = SCSP_ENV_MASK - (s32) x;
	}

	for(i = 0, j = 0; i < 32; i++)
	{
		j += 1 << (i >> 2);
		// lfo freq
		x = 172.3 / (double) (j);
		// converting lfo freq in lfo step
		scsp_lfo_step[31 - i] = scsp_round(x * ((double) (SCSP_LFO_LEN) / (double) (SCSP_FREQ)) * (double) (1 << SCSP_LFO_LB));
	}

	for(i = 0; i < SCSP_LFO_LEN; i++)
	{
		scsp_lfo_sawt_e[i] = SCSP_LFO_MASK - i;
		if (i < (SCSP_LFO_LEN / 2)) scsp_lfo_squa_e[i] = SCSP_LFO_MASK;
		else scsp_lfo_squa_e[i] = 0;
		if (i < (SCSP_LFO_LEN / 2)) scsp_lfo_tri_e[i] = SCSP_LFO_MASK - (i * 2);
		else scsp_lfo_tri_e[i] = (i - (SCSP_LFO_LEN / 2)) * 2;
		scsp_lfo_noi_e[i] = rand() & SCSP_LFO_MASK;

		scsp_lfo_sawt_f[i] = i - (SCSP_LFO_LEN / 2);
		if (i < (SCSP_LFO_LEN / 2)) scsp_lfo_squa_f[i] = 0 - (SCSP_LFO_LEN / 2);
		else scsp_lfo_squa_f[i] = SCSP_LFO_MASK - (SCSP_LFO_LEN / 2);
		if (i < (SCSP_LFO_LEN / 2)) scsp_lfo_tri_f[i] = (i * 2) - (SCSP_LFO_LEN / 2);
		else scsp_lfo_tri_f[i] = (SCSP_LFO_MASK - ((i - (SCSP_LFO_LEN / 2)) * 2)) - (SCSP_LFO_LEN / 2);
		scsp_lfo_noi_f[i] = scsp_lfo_noi_e[i] - (SCSP_LFO_LEN / 2);
	}

	for(i = 0; i < 4; i++)
	{
		scsp_attack_rate[i] = 0;
		scsp_decay_rate[i] = 0;
	}

	for(i = 0; i < 60; i++)
	{
		x = 1.0 + ((i & 3) * 0.25);				// bits 0-1 : x1.00, x1.25, x1.50, x1.75
		x *= (double) (1 << ((i >> 2)));			// bits 2-5 : shift bits (x2^0 - x2^15)
		x *= (double) (SCSP_ENV_LEN << SCSP_ENV_LB);	// adjust for table scsp_env_table

		scsp_attack_rate[i + 4] = scsp_round(x / (double) SCSP_ATTACK_R);
		scsp_decay_rate[i + 4] = scsp_round(x / (double) SCSP_DECAY_R);

		if (scsp_attack_rate[i + 4] == 0) scsp_attack_rate[i + 4] = 1;
		if (scsp_decay_rate[i + 4] == 0) scsp_decay_rate[i + 4] = 1;
	}

	scsp_attack_rate[63] = SCSP_ENV_AE;
	scsp_decay_rate[61] = scsp_decay_rate[60];
	scsp_decay_rate[62] = scsp_decay_rate[60];
	scsp_decay_rate[63] = scsp_decay_rate[60];

	for(i = 64; i < 96; i++)
	{
		scsp_attack_rate[i] = scsp_attack_rate[63];
		scsp_decay_rate[i] = scsp_decay_rate[63];
		scsp_null_rate[i - 64] = 0;
	}

	for(i = 0; i < 96; i++)
	{
                SCSPLOG("attack rate[%d] = %.8X -> %.8X\n", i, scsp_attack_rate[i], scsp_attack_rate[i] >> SCSP_ENV_LB);
                SCSPLOG("decay rate[%d] = %.8X -> %.8X\n", i, scsp_decay_rate[i], scsp_decay_rate[i] >> SCSP_ENV_LB);
	}

	scsp_reset();
}

//////////////////////////////////////////////////////////////////////////////
// Yabause specific
//////////////////////////////////////////////////////////////////////////////

u8 *SoundRam;
ScspInternal *ScspInternalVars;
static SoundInterface_struct *SNDCore=NULL;
extern SoundInterface_struct *SNDCoreList[];

struct sounddata {
  u32 *data32;
} scspchannel[2];

static u32 scspsoundlen;

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL c68k_byte_read(const u32 adr) {
  if (adr < 0x100000)
     return T2ReadByte(SoundRam, adr & 0x7FFFF);
  else
     return scsp_r_b(adr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL c68k_byte_write(const u32 adr, u32 data) {
  if (adr < 0x100000)
     T2WriteByte(SoundRam, adr & 0x7FFFF, data);
  else
     scsp_w_b(adr, data);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL c68k_word_read(const u32 adr) {
  if (adr < 0x100000)
     return T2ReadWord(SoundRam, adr & 0x7FFFF);
  else
     return scsp_r_w(adr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL c68k_word_write(const u32 adr, u32 data) {
  if (adr < 0x100000)
     T2WriteWord(SoundRam, adr & 0x7FFFF, data);
  else
     scsp_w_w(adr, data);
}

//////////////////////////////////////////////////////////////////////////////

void c68k_interrupt_handler(u32 level) {
   // send interrupt to 68k
   C68k_Set_IRQ(&C68K, (s32)level);
}

//////////////////////////////////////////////////////////////////////////////

void scu_interrupt_handler(void) {
  // send interrupt to scu
  ScuSendSoundRequest();
}

//////////////////////////////////////////////////////////////////////////////

u8 FASTCALL SoundRamReadByte(u32 addr) {
   addr &= 0xFFFFF;

   // If mem4b is set, mirror ram every 256k
   if (scsp.mem4b == 0)
      addr &= 0x3FFFF;
   else if (addr > 0x7FFFF)
         return 0xFF;

   return T2ReadByte(SoundRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SoundRamWriteByte(u32 addr, u8 val) {
   addr &= 0xFFFFF;

   // If mem4b is set, mirror ram every 256k
   if (scsp.mem4b == 0)
      addr &= 0x3FFFF;
   else if (addr > 0x7FFFF)
      return;

   T2WriteByte(SoundRam, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u16 FASTCALL SoundRamReadWord(u32 addr) {
   addr &= 0xFFFFF;

   if (scsp.mem4b == 0)
      addr &= 0x3FFFF;
   else if (addr > 0x7FFFF)
      return 0xFFFF;

   return T2ReadWord(SoundRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SoundRamWriteWord(u32 addr, u16 val) {
   addr &= 0xFFFFF;

   // If mem4b is set, mirror ram every 256k
   if (scsp.mem4b == 0)
      addr &= 0x3FFFF;
   else if (addr > 0x7FFFF)
      return;

   T2WriteWord(SoundRam, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

u32 FASTCALL SoundRamReadLong(u32 addr) {
   addr &= 0xFFFFF;

   // If mem4b is set, mirror ram every 256k
   if (scsp.mem4b == 0)
      addr &= 0x3FFFF;
   else if (addr > 0x7FFFF)
      return 0xFFFFFFFF;

   return T2ReadLong(SoundRam, addr);
}

//////////////////////////////////////////////////////////////////////////////

void FASTCALL SoundRamWriteLong(u32 addr, u32 val) {
   addr &= 0xFFFFF;

   // If mem4b is set, mirror ram every 256k
   if (scsp.mem4b == 0)
      addr &= 0x3FFFF;
   else if (addr > 0x7FFFF)
      return;

   T2WriteLong(SoundRam, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

int ScspInit(int coreid) {
   int i;

   if ((SoundRam = T2MemoryInit(0x80000)) == NULL)
      return -1;

   if ((ScspInternalVars = (ScspInternal *)calloc(1, sizeof(ScspInternal))) == NULL)
      return -1;

   C68k_Init(&C68K, NULL); // not sure if I need the int callback or not

   C68k_Set_ReadB(&C68K, (C68K_READ *)c68k_byte_read);
   C68k_Set_ReadW(&C68K, (C68K_READ *)c68k_word_read);
   C68k_Set_WriteB(&C68K, (C68K_WRITE *)c68k_byte_write);
   C68k_Set_WriteW(&C68K, (C68K_WRITE *)c68k_word_write);

   C68k_Set_Fetch(&C68K, 0x000000, 0x040000, (pointer)SoundRam);
   C68k_Set_Fetch(&C68K, 0x040000, 0x080000, (pointer)SoundRam);
   C68k_Set_Fetch(&C68K, 0x080000, 0x0C0000, (pointer)SoundRam);
   C68k_Set_Fetch(&C68K, 0x0C0000, 0x100000, (pointer)SoundRam);

   yabsys.IsM68KRunning = 0;

   scsp_init(SoundRam, &c68k_interrupt_handler, &scu_interrupt_handler);
   ScspInternalVars->scsptiming1 = 0;
   ScspInternalVars->scsptiming2 = 0;

   for (i = 0; i < MAX_BREAKPOINTS; i++)
      ScspInternalVars->codebreakpoint[i].addr = 0xFFFFFFFF;
   ScspInternalVars->numcodebreakpoints = 0;
   ScspInternalVars->BreakpointCallBack = NULL;
   ScspInternalVars->inbreakpoint = 0;

   // Allocate enough memory for each channel buffer(may have to change)
   scspsoundlen = 44100 / 60; // assume it's NTSC timing

   if ((scspchannel[0].data32 = (u32 *)calloc(scspsoundlen, sizeof(u32))) == NULL)
      return -1;

   if ((scspchannel[1].data32 = (u32 *)calloc(scspsoundlen, sizeof(u32))) == NULL)
      return -1;

   // So which core do we want?
   if (coreid == SNDCORE_DEFAULT)
      coreid = 0; // Assume we want the first one

   // Go through core list and find the id
   for (i = 0; SNDCoreList[i] != NULL; i++)
   {
      if (SNDCoreList[i]->id == coreid)
      {
         // Set to current core
         SNDCore = SNDCoreList[i];
         break;
      }
   }

   if (SNDCore == NULL)
      return -1;

   if (SNDCore->Init() == -1)
   {
      // Since it failed, instead of it being fatal, we'll just use the dummy
      // core instead

      // This might be helpful though.
      YabSetError(YAB_ERR_CANNOTINIT, (void *)SNDCore->Name);

      SNDCore = &SNDDummy;
      SNDCore->Init();
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void ScspDeInit(void) {
   if (scspchannel[0].data32)
      free(scspchannel[0].data32);

   if (scspchannel[1].data32)
      free(scspchannel[1].data32);

   if (SNDCore)
      SNDCore->DeInit();

   scsp_shutdown();

   if (SoundRam)
      T2MemoryDeInit(SoundRam);
}

//////////////////////////////////////////////////////////////////////////////

void M68KReset(void) {
   C68k_Reset(&C68K);
}

//////////////////////////////////////////////////////////////////////////////

void ScspReset(void) {
   scsp_reset();
}

//////////////////////////////////////////////////////////////////////////////

int ScspChangeVideoFormat(int type) {
   scspsoundlen = 44100 / (type ? 50 : 60);

   if (scspchannel[0].data32)
      free(scspchannel[0].data32);

   if (scspchannel[1].data32)
      free(scspchannel[1].data32);

   // Reallocate the sound buffers
   if ((scspchannel[0].data32 = (u32 *)calloc(scspsoundlen, sizeof(u32))) == NULL)
      return -1;

   if ((scspchannel[1].data32 = (u32 *)calloc(scspsoundlen, sizeof(u32))) == NULL)
      return -1;

   SNDCore->ChangeVideoFormat(type ? 50 : 60);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void M68KExec(u32 cycles) {
   int i;
   if (yabsys.IsM68KRunning)
   {
      if (ScspInternalVars->numcodebreakpoints == 0)
      {
         C68k_Exec(&C68K, (u32)((float)cycles / 2.5)); // almost correct
      }
      else
      {
         u32 cyclestoexec=(u32)((float)cycles / 2.5);
         u32 cyclesexecuted=0;

         for (;;)
         {
            // Make sure it isn't one of our breakpoints
            for (i=0; i < ScspInternalVars->numcodebreakpoints; i++) {
               if ((C68k_Get_PC(&C68K) == ScspInternalVars->codebreakpoint[i].addr) && ScspInternalVars->inbreakpoint == 0) {
                  ScspInternalVars->inbreakpoint = 1;
                  if (ScspInternalVars->BreakpointCallBack)
                     ScspInternalVars->BreakpointCallBack(ScspInternalVars->codebreakpoint[i].addr);
                     ScspInternalVars->inbreakpoint = 0;
               }
            }

            // execute instructions individually
            cyclesexecuted += C68k_Exec(&C68K, 1);
          
            if (cyclesexecuted >= cyclestoexec) break;
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void M68KStep() {
   C68k_Exec(&C68K, 1);
}

//////////////////////////////////////////////////////////////////////////////

void ScspConvert32uto16s(s32 *srcL, s32 *srcR, s16 *dst, u32 len) {
   u32 i;

   for (i = 0; i < len; i++)
   {
      // Left Channel
      if (*srcL > 0x7FFF) *dst = 0x7FFF;
      else if (*srcL < -0x8000) *dst = -0x8000;
      else *dst = *srcL;
      srcL++;
      dst++;
      // Right Channel
      if (*srcR > 0x7FFF) *dst = 0x7FFF;
      else if (*srcR < -0x8000) *dst = -0x8000;
      else *dst = *srcR;
      srcR++;
      dst++;
   } 
}

//////////////////////////////////////////////////////////////////////////////

void ScspExec() {
   u32 audiosize;

   scsp_update_timer((u32)(ScspInternalVars->scsptiming2 + 2.7947)); // I should really be using integers, but oh well
   ScspInternalVars->scsptiming2 = (ScspInternalVars->scsptiming2 + 2.7947) - (float)((u32)(ScspInternalVars->scsptiming2 + 2.7947)); 
   ScspInternalVars->scsptiming1++;

   if (ScspInternalVars->scsptiming1 >= 263)
   {
      ScspInternalVars->scsptiming1 -= 263;
      ScspInternalVars->scsptiming2 = 0;
   }

   if ((audiosize = SNDCore->GetAudioSpace()))
   {
      if (audiosize > (scspsoundlen))
         audiosize = scspsoundlen;
      memset(scspchannel[0].data32, 0, sizeof(u32) * audiosize);
      memset(scspchannel[1].data32, 0, sizeof(u32) * audiosize);

      scsp_update((s32 *)scspchannel[0].data32, (s32 *)scspchannel[1].data32, audiosize);
      SNDCore->UpdateAudio(scspchannel[0].data32, (s32 *)scspchannel[1].data32, audiosize);
   }
}

//////////////////////////////////////////////////////////////////////////////

void M68KGetRegisters(m68kregs_struct *regs) {
   int i;

   if (regs != NULL) {
      for (i = 0; i < 8; i++) {
         regs->D[i] = C68k_Get_DReg(&C68K, i);
         regs->A[i] = C68k_Get_AReg(&C68K, i);
      }

      regs->SR = C68k_Get_SR(&C68K);
      regs->PC = C68k_Get_PC(&C68K);
   }
}

//////////////////////////////////////////////////////////////////////////////

void M68KSetRegisters(m68kregs_struct *regs) {
   int i;

   if (regs != NULL) {
      for (i = 0; i < 8; i++) {
         C68k_Set_DReg(&C68K, i, regs->D[i]);
         C68k_Set_AReg(&C68K, i, regs->A[i]);
      }

      C68k_Set_SR(&C68K, regs->SR);
      C68k_Set_PC(&C68K, regs->PC);
   }
}

//////////////////////////////////////////////////////////////////////////////

void ScspMuteAudio() {
   SNDCore->MuteAudio();
}

//////////////////////////////////////////////////////////////////////////////

void ScspUnMuteAudio() {
   SNDCore->UnMuteAudio();
}

//////////////////////////////////////////////////////////////////////////////

void M68KSetBreakpointCallBack(void (*func)(u32)) {
   ScspInternalVars->BreakpointCallBack = func;
}

//////////////////////////////////////////////////////////////////////////////

int M68KAddCodeBreakpoint(u32 addr) {
  if (ScspInternalVars->numcodebreakpoints < MAX_BREAKPOINTS) {
     ScspInternalVars->codebreakpoint[ScspInternalVars->numcodebreakpoints].addr = addr;
     ScspInternalVars->numcodebreakpoints++;

     return 0;
  }

  return -1;
}

//////////////////////////////////////////////////////////////////////////////

void M68KSortCodeBreakpoints() {
  int i, i2;
  u32 tmp;

  for (i = 0; i < (MAX_BREAKPOINTS-1); i++)
  {
     for (i2 = i+1; i2 < MAX_BREAKPOINTS; i2++)
     {
        if (ScspInternalVars->codebreakpoint[i].addr == 0xFFFFFFFF &&
            ScspInternalVars->codebreakpoint[i2].addr != 0xFFFFFFFF)
        {
           tmp = ScspInternalVars->codebreakpoint[i].addr;
           ScspInternalVars->codebreakpoint[i].addr = ScspInternalVars->codebreakpoint[i2].addr;
           ScspInternalVars->codebreakpoint[i2].addr = tmp;
        }
     }
  }
}

//////////////////////////////////////////////////////////////////////////////

int M68KDelCodeBreakpoint(u32 addr) {
   int i;
   if (ScspInternalVars->numcodebreakpoints > 0) {
      for (i = 0; i < ScspInternalVars->numcodebreakpoints; i++) {
         if (ScspInternalVars->codebreakpoint[i].addr == addr)
         {
            ScspInternalVars->codebreakpoint[i].addr = 0xFFFFFFFF;
            M68KSortCodeBreakpoints();
            ScspInternalVars->numcodebreakpoints--;
            return 0;
         }
      }
   }

   return -1;
}

//////////////////////////////////////////////////////////////////////////////

m68kcodebreakpoint_struct *M68KGetBreakpointList() {
  return ScspInternalVars->codebreakpoint;
}

//////////////////////////////////////////////////////////////////////////////

void M68KClearCodeBreakpoints() {
   int i;
   for (i = 0; i < MAX_BREAKPOINTS; i++)
      ScspInternalVars->codebreakpoint[i].addr = 0xFFFFFFFF;

   ScspInternalVars->numcodebreakpoints = 0;
}

//////////////////////////////////////////////////////////////////////////////

int SoundSaveState(FILE *fp)
{
//   int i;
//   u32 temp;
   int offset;

   offset = StateWriteHeader(fp, "SCSP", 1);

/*
   // Save 68k registers first
   fwrite((void *)&is68kOn, 1, 1, fp);

   for (i = 0; i < 8; i++)
   {
      temp = C68k_Get_DReg(&C68K, i);
      fwrite((void *)&temp, 4, 1, fp);
   }

   for (i = 0; i < 8; i++)
   {
      temp =  C68k_Get_AReg(&C68K, i);
      fwrite((void *)&temp, 4, 1, fp);
   }

   temp = C68k_Get_SR(&C68K);
   fwrite((void *)&temp, 4, 1, fp);
   temp = C68k_Get_PC(&C68K);
   fwrite((void *)&temp, 4, 1, fp);

   // Now for the SCSP registers
   fwrite((void *)scsp_reg, 0x1000, 1, fp);

   // Lastly, sound ram
   fwrite((void *)sram->getBuffer(), 0x80000, 1, fp);
*/

   return StateFinishHeader(fp, offset);
}

//////////////////////////////////////////////////////////////////////////////

int SoundLoadState(FILE *fp, int version, int size)
{
/*
   int i;
   u32 temp;

   // Read 68k registers first
   fread((void *)&is68kOn, 1, 1, fp);

   for (i = 0; i < 8; i++) {
      fread((void *)&temp, 4, 1, fp);
      C68k_Set_DReg(&C68K, i, temp);
   }

   for (i = 0; i < 8; i++) {
      fread((void *)&temp, 4, 1, fp);
      C68k_Set_AReg(&C68K, i, temp);
   }

   fread((void *)&temp, 4, 1, fp);
   C68k_Set_SR(&C68K, temp);
   fread((void *)&temp, 4, 1, fp);
   C68k_Set_PC(&C68K, temp);

   // Now for the SCSP registers
   fread((void *)scsp_reg, 0x1000, 1, fp);

   // Lastly, sound ram
   fread((void *)sram->getBuffer(), 0x80000, 1, fp);
*/
   return size;
}

//////////////////////////////////////////////////////////////////////////////

// Terrible, but I'm not sure how to do the equivalent in inline
#define AddString(s, r...) \
   sprintf(s, ## r); \
   s += strlen(s)

//////////////////////////////////////////////////////////////////////////////

void ScspSlotDebugStats(u8 slotnum, char *outstring)
{
   AddString(outstring, "Sound Source = ");
   switch (scsp.slot[slotnum].ssctl)
   {
      case 0:
      {
         AddString(outstring, "External DRAM data\r\n");
         break;
      }
      case 1:
      {
         AddString(outstring, "Internal(Noise)\r\n");
         break;
      }
      case 2:
      {
         AddString(outstring, "Internal(0's)\r\n");
         break;
      }
      default:
      {
         AddString(outstring, "Invalid setting\r\n");
         break;
      }
   }
   AddString(outstring, "Source bit = ");
   switch(scsp.slot[slotnum].sbctl)
   {
      case 0:
      {
         AddString(outstring, "No bit reversal\r\n");
         break;
      }
      case 1:
      {
         AddString(outstring, "Reverse other bits\r\n");
         break;
      }
      case 2:
      {
         AddString(outstring, "Reverse sign bit\r\n");
         break;
      }
      case 3:
      {
         AddString(outstring, "Reverse sign and other bits\r\n");
         break;
      }
   }

   // Loop Control
   AddString(outstring, "Loop Mode = ");
   switch (scsp.slot[slotnum].lpctl)
   {
      case 0:
      {
         AddString(outstring, "Off\r\n");
         break;
      }
      case 1:
      {
         AddString(outstring, "Normal\r\n");
         break;
      }
      case 2:
      {
         AddString(outstring, "Reverse\r\n");
         break;
      }
      case 3:
      {
         AddString(outstring, "Alternative\r\n");
         break;
      }
   }
   // PCM8B
   if (scsp.slot[slotnum].pcm8b)
   {
      AddString(outstring, "8-bit samples\r\n");
   }
   else
   {
      AddString(outstring, "16-bit samples\r\n");
   }

   AddString(outstring, "Start Address = %05X\r\n", scsp.slot[slotnum].sa);
   AddString(outstring, "Loop Start Address = %04X\r\n", scsp.slot[slotnum].lsa >> SCSP_FREQ_LB);
   AddString(outstring, "Loop End Address = %04X\r\n", scsp.slot[slotnum].lea >> SCSP_FREQ_LB);
   AddString(outstring, "Decay 1 Rate = %d\r\n", scsp.slot[slotnum].dr);
   AddString(outstring, "Decay 2 Rate = %d\r\n", scsp.slot[slotnum].sr);
   if (scsp.slot[slotnum].eghold)
   {
      AddString(outstring, "EG Hold Enabled\r\n");
   }
   AddString(outstring, "Attack Rate = %d\r\n", scsp.slot[slotnum].ar);

   if (scsp.slot[slotnum].lslnk)
   {
      AddString(outstring, "Loop Start Link Enabled\r\n");
   }

   if (scsp.slot[slotnum].krs != 0)
   {
      AddString(outstring, "Key rate scaling = %d\r\n", scsp.slot[slotnum].krs);
   }

//   AddString(outstring, "Decay Level = \r\n");
   AddString(outstring, "Release Rate = %d\r\n", scsp.slot[slotnum].rr);

   if (scsp.slot[slotnum].swe)
   {
      AddString(outstring, "Stack Write Inhibited\r\n");
   }

   if (scsp.slot[slotnum].sdir)
   {
      AddString(outstring, "Sound Direct Enabled\r\n");
   }

   AddString(outstring, "Total Level = %d\r\n", scsp.slot[slotnum].tl);
//   AddString(outstring, "Modulation Level = \r\n");
//   AddString(outstring, "Modulation Input X = \r\n");
//   AddString(outstring, "Modulation Input Y = \r\n");
//   AddString(outstring, "Octave = \r\n");
//   AddString(outstring, "Frequency Number Switch = \r\n");
//   AddString(outstring, "LFO Reset = \r\n");
//   AddString(outstring, "LFO Frequency = \r\n");
//   AddString(outstring, "LFO Frequency modulation waveform = \r\n");
//   AddString(outstring, "LFO Frequency modulation level = \r\n");
//   AddString(outstring, "LFO Amplitude modulation waveform = \r\n");
//   AddString(outstring, "LFO Amplitude modulation level = \r\n");
//   AddString(outstring, "Input Select = \r\n");
//   AddString(outstring, "Input mix level = \r\n");
   AddString(outstring, "Direct data send level: left = %d, right = %d\r\n", scsp.slot[slotnum].disll, scsp.slot[slotnum].dislr);
//   AddString(outstring, "Direct data fixed position = \r\n");
//   AddString(outstring, "Effect data send level = \r\n");
//   AddString(outstring, "Effect data fixed position = \r\n");
}

//////////////////////////////////////////////////////////////////////////////
// Dummy Sound Interface
//////////////////////////////////////////////////////////////////////////////

int SNDDummyInit();
void SNDDummyDeInit();
int SNDDummyReset();
int SNDDummyChangeVideoFormat(int vertfreq);
void SNDDummyUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples);
u32 SNDDummyGetAudioSpace();
void SNDDummyMuteAudio();
void SNDDummyUnMuteAudio();

SoundInterface_struct SNDDummy = {
SNDCORE_DUMMY,
"Dummy Sound Interface",
SNDDummyInit,
SNDDummyDeInit,
SNDDummyReset,
SNDDummyChangeVideoFormat,
SNDDummyUpdateAudio,
SNDDummyGetAudioSpace,
SNDDummyMuteAudio,
SNDDummyUnMuteAudio
};

//////////////////////////////////////////////////////////////////////////////

int SNDDummyInit()
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDDummyDeInit()
{
}

//////////////////////////////////////////////////////////////////////////////

int SNDDummyReset()
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int SNDDummyChangeVideoFormat(int vertfreq)
{
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDDummyUpdateAudio(u32 *leftchanbuffer, u32 *rightchanbuffer, u32 num_samples)
{
}

//////////////////////////////////////////////////////////////////////////////

u32 SNDDummyGetAudioSpace()
{
   /* A "hack" to get dummy sound core working enough
    * so videos are not "freezing". Values have been
    * found by experiments... I don't have a clue why
    * they are working ^^;
    */
   static int i = 0;
   i++;
   if (i == 55) {
	i = 0;
   	return 85;
   } else {
        return 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

void SNDDummyMuteAudio()
{
}

//////////////////////////////////////////////////////////////////////////////

void SNDDummyUnMuteAudio()
{
}

//////////////////////////////////////////////////////////////////////////////

