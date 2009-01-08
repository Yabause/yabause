/*  Copyright 2007 Guillaume Duhamel

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

#include "m68kc68k.h"
#include "c68k/c68k.h"

void M68KC68KInit(void) {
	C68k_Init(&C68K, NULL); // not sure if I need the int callback or not
}

void M68KC68KDeInit(void) {
}

void M68KC68KReset(void) {
	C68k_Reset(&C68K);
}

s32 FASTCALL M68KC68KExec(s32 cycle) {
	return C68k_Exec(&C68K, cycle);
}

u32 M68KC68KGetDReg(u32 num) {
	return C68k_Get_DReg(&C68K, num);
}

u32 M68KC68KGetAReg(u32 num) {
	return C68k_Get_AReg(&C68K, num);
}

u32 M68KC68KGetPC(void) {
	return C68k_Get_PC(&C68K);
}

u32 M68KC68KGetSR(void) {
	return C68k_Get_SR(&C68K);
}

u32 M68KC68KGetUSP(void) {
	return C68k_Get_USP(&C68K);
}

u32 M68KC68KGetMSP(void) {
	return C68k_Get_MSP(&C68K);
}

void M68KC68KSetDReg(u32 num, u32 val) {
	C68k_Set_DReg(&C68K, num, val);
}

void M68KC68KSetAReg(u32 num, u32 val) {
	C68k_Set_AReg(&C68K, num, val);
}

void M68KC68KSetPC(u32 val) {
	C68k_Set_PC(&C68K, val);
}

void M68KC68KSetSR(u32 val) {
	C68k_Set_SR(&C68K, val);
}

void M68KC68KSetUSP(u32 val) {
	C68k_Set_USP(&C68K, val);
}

void M68KC68KSetMSP(u32 val) {
	C68k_Set_MSP(&C68K, val);
}

void M68KC68KSetFetch(u32 low_adr, u32 high_adr, pointer fetch_adr) {
	C68k_Set_Fetch(&C68K, low_adr, high_adr, fetch_adr);
}

void FASTCALL M68KC68KSetIRQ(s32 level) {
	C68k_Set_IRQ(&C68K, level);
}

void M68KC68KSetReadB(M68K_READ *Func) {
	C68k_Set_ReadB(&C68K, Func);
}

void M68KC68KSetReadW(M68K_READ *Func) {
	C68k_Set_ReadW(&C68K, Func);
}

void M68KC68KSetWriteB(M68K_WRITE *Func) {
	C68k_Set_WriteB(&C68K, Func);
}

void M68KC68KSetWriteW(M68K_WRITE *Func) {
	C68k_Set_WriteW(&C68K, Func);
}

M68K_struct M68KC68K = {
	1,
	"C68k Interface",
	M68KC68KInit,
	M68KC68KDeInit,
	M68KC68KReset,
	M68KC68KExec,
	M68KC68KGetDReg,
	M68KC68KGetAReg,
	M68KC68KGetPC,
	M68KC68KGetSR,
	M68KC68KGetUSP,
	M68KC68KGetMSP,
	M68KC68KSetDReg,
	M68KC68KSetAReg,
	M68KC68KSetPC,
	M68KC68KSetSR,
	M68KC68KSetUSP,
	M68KC68KSetMSP,
	M68KC68KSetFetch,
	M68KC68KSetIRQ,
	M68KC68KSetReadB,
	M68KC68KSetReadW,
	M68KC68KSetWriteB,
	M68KC68KSetWriteW
};
