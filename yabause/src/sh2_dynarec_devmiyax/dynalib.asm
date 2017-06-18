; Copyright 2017 devMiyax(smiyaxdev@gmail.com)
; 
; This file is part of Yabause.
; 
; Yabause is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
; 
; Yabause is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with Yabause; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

; EAX  
; EBX  <- Address of CtrlReg
; ECX  
; EDX  <- Address of PC
; ESI  <- Address of SysReg
; EDI  <- Address of GenReg
; EIP
; ESP
; EBP
; EFL
;

bits 32

section .data
struc tagSH2
	GenReg:  resd 16 
	CtrlReg:  resd 4
	SysReg:  resd 3 
endstruc
section .code

;Memory Functions
extern _memGetByte, _memGetWord, _memGetLong
extern _memSetByte, _memSetWord, _memSetLong
extern _EachClock, _DelayEachClock, _DebugEachClock, _DebugDelayClock

%macro opfunc 1
	global _x86_%1
	_x86_%1:
%endmacro

%macro opdesc 7
	global _%1_size
	_%1_size dw %2
	global _%1_src
	_%1_src db %3
	global _%1_dest
	_%1_dest db %4
	global _%1_off1
	_%1_off1 db %5
	global _%1_imm
	_%1_imm db %6
	global _%1_off3
	_%1_off3 db %7
%endmacro

%macro ctrlreg_load 1	;5
	mov ebp,edi            ; 2
	add ebp,byte 64+(%1*4) ; 3
%endmacro

%macro getflag 1	;7
	lahf
	shr ax,8+%1
	and eax,byte 1
%endmacro

;=====================================================
; Basic
;=====================================================

;-----------------------------------------------------
; Begining of block
; SR => [ebx]
; PC => [edx]
; GenReg => edi
; SysReg => esi
; Size = 27 Bytes
global _prologue
_prologue:
pushad             ;1
push dword 00      ;4 (JumpAddr)
mov ebp,[esp+36+4] ;4
mov edi,ebp        ;2 (GenReg)
add ebp,byte 64    ;3
mov ebx,ebp        ;2 (SR)
add ebp,byte 12    ;3
mov esi,ebp        ;2 (SysReg)
mov edx,esi        ;2
add edx,byte 12    ;3 (PC)


;-------------------------------------------------------
; normal part par instruction
;Size = 7 Bytes
global _seperator_normal
_seperator_normal:
add dword [edx], byte 2   ;3 PC += 2
add dword [edx+4], byte 1 ;4 Clock += 1

;------------------------------------------------------
; Delay slot part par instruction
;Size = 17 Bytes
global _seperator_delay_slot
_seperator_delay_slot:
test dword [esp], 0xFFFFFFFF ; 7
jnz   .continue               ; 2
add dword [edx], byte 2      ; 3 PC += 2
add dword [edx+4], byte 1    ; 4 Clock += 1
pop  eax                     ; 1
popad                        ; 1
ret                          ; 1
.continue
mov  eax,[esp]             ; 3
sub  eax,byte 2            ; 3
mov  [edx],eax             ; 2



;------------------------------------------------------
; End part of delay slot
;Size = 24 Bytes
global _seperator_delay_after
_seperator_delay_after:
add dword [edx], byte 2   ;3 PC += 2
add dword [edx+4], byte 1 ;4 Clock += 1
pop  eax                  ; 1
popad                     ; 1
ret                       ; 1

;-------------------------------------------------------
; End of block
; Size = 3 Bytes
global _epilogue
_epilogue:
pop eax         ;1
popad           ;1
ret             ;1

;------------------------------------------------------
; Jump part
; Size = 17 Bytes
global _PageFlip
_PageFlip:
test dword [esp], 0xFFFFFFFF ; 7
jz   .continue               ; 2
mov  eax,[esp]               ; 3
mov  [edx],eax               ; 2
pop  eax                     ; 1
popad                        ; 1
ret                          ; 1
.continue


;-------------------------------------------------------
; normal part par instruction( for debug build )
;Size = 24 Bytes
global _seperator_d_normal
_seperator_d_normal:
add dword [edx+4],byte 1 ;4 Clock += 1
mov  eax,_DebugEachClock ;5
call eax                 ;2
test eax, 0x01           ;5 finish 
jz  NEXT_D_INST          ;2
pop eax                  ;1 
popad                    ;1
ret                      ;1
NEXT_D_INST:
add dword [edx],byte 2   ;3 PC += 2

;------------------------------------------------------
; Delay slot part par instruction( for debug build )
;Size = 34 Bytes
global _seperator_d_delay
_seperator_d_delay:
mov  eax,_DebugDelayClock ;5
call eax                   ;2
test dword [esp], 0xFFFFFFFF ; 7
jnz   .continue               ; 2
add dword [edx], byte 2      ; 3 PC += 2
add dword [edx+4], byte 1    ; 4 Clock += 1
pop  eax                     ; 1
popad                        ; 1
ret                          ; 1
.continue
mov  eax,[esp]             ; 3
sub  eax,byte 2            ; 3
mov  [edx],eax             ; 2

;=================
;Begin x86 Opcodes
;=================

opdesc CLRT,	3,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc CLRT
and dword [ebx],byte 0xfe

opdesc CLRMAC,	7,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc CLRMAC
and dword [esi],byte 0   ;4
and dword [esi+4],byte 0 ;4

opdesc NOP,		1,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc NOP
nop

opdesc DIV0U,	6,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc DIV0U
and dword [ebx],0xfffffcfe

opdesc SETT,	3,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc SETT
or [ebx],byte 1

opdesc SLEEP,	14,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc SLEEP
add dword [edx+4],byte 1   ;4 
mov  eax,_EachClock ;5
call eax            ;2
pop eax             ;1
popad               ;1
ret                 ;1


opdesc SWAP_W,	19,4,15,0xff,0xff,0xff
opfunc SWAP_W
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
rol eax,16          ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;2

opdesc SWAP_B,	18,4,14,0xff,0xff,0xff
opfunc SWAP_B
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
xchg ah,al          ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;2


opdesc TST,	29,4,12,0xff,0xff,0xff
opfunc TST
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
test [ebp],eax      ;2
getflag 6           ;7
and dword [ebx],byte 0xfe ;3
or [ebx],eax        ;2

opdesc TSTI,	23,0xff,0xff,0xff,6,0xff
opfunc TSTI
mov  ebp,edi        ;2
mov  eax,[ebp]      ;3
test eax,$00        ;5  Imidiate Val
getflag 6           ;7
and dword [ebx],byte 0xfe ;3
or [ebx],eax        ;2


opdesc ANDI,	9,0xff,0xff,0xff,5,0xff
opfunc ANDI
mov ebp,edi         ;2
xor eax,eax         ;2
mov al,byte $00     ;2
and dword [ebp],eax ;3

opdesc XORI,	9,0xff,0xff,0xff,5,0xff
opfunc XORI
mov ebp,edi         ;2
xor eax,eax         ;2
mov al,byte $00     ;2
xor dword [ebp],eax ;3

opdesc ORI,	9,0xff,0xff,0xff,5,0xff
opfunc ORI
mov ebp,edi         ;2
xor eax,eax         ;2
mov al,byte $00     ;2
or dword [ebp],eax  ;3

opdesc CMP_EQ_IMM,	25,0xff,0xff,0xff,8,0xff
opfunc CMP_EQ_IMM
mov ebp,edi         ;2
mov eax,[ebp]       ;3
cmp dword [ebp],byte $00 ;4
getflag 6           ;7
and dword [ebx], 0xFFFFFFE ;6
or [ebx],eax        ;2 

opdesc XTRCT,	23,4,12,0xff,0xff,0xff
opfunc XTRCT
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
shl eax,16          ;3
shr dword [ebp],16  ;3
or [ebp],eax        ;2

opdesc ADD,		16,4,12,0xff,0xff,0xff
opfunc ADD
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
add [ebp],eax       ;2

opdesc ADDC,	33,4,12,0xff,0xff,0xff
opfunc ADDC
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
mov ebp,edi         ;2
add ebp,byte $00    ;3
bts dword [ebx],0   ;4
adc [ebp],eax       ;3
jnc	ADDC_NO_CARRY   ;2
or [ebx], byte 1    ;3
jmp ADDC_END        ;2
ADDC_NO_CARRY:
and dword [ebx],byte 0xfe ;3
ADDC_END:


; add with overflow check
opdesc ADDV, 24,4,12,0xff,0xff,0xff
opfunc ADDV
mov  ebp,edi          ;2
add  ebp,byte $00     ;3
mov  eax,[ebp]        ;3
mov  ebp,edi          ;2
add  ebp,byte $00     ;3
and  [ebx], byte 0xFE ;3
add  [ebp],eax        ;3
jno	 NO_OVER_FLO      ;2
or   [ebx], byte 01   ;3
NO_OVER_FLO

opdesc SUBC,	33,4,12,0xff,0xff,0xff
opfunc SUBC
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
mov ebp,edi         ;2
add ebp,byte $00    ;3
bts dword [ebx],0   ;4
sbb [ebp],eax       ;3
jnc	non_carry       ;2
or [ebx], byte 1    ;3
jmp SUBC_END        ;2
non_carry:
and dword [ebx],byte 0xfe ;3
SUBC_END:




opdesc SUB,		16,4,12,0xff,0xff,0xff
opfunc SUB
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
sub [ebp],eax       ;2


opdesc NOT,		18,4,14,0xff,0xff,0xff
opfunc NOT
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
not eax             ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;2

opdesc NEG,		18,4,14,0xff,0xff,0xff
opfunc NEG
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
neg eax             ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;2

opdesc NEGC,	50,4,14,0xff,0xff,0xff
opfunc NEGC
mov ebp,edi               ;2
add ebp,byte $00          ;3
mov ecx,[ebp]             ;3
neg ecx                   ;2
mov ebp,edi               ;2
add ebp,byte $00          ;3
mov [ebp],ecx             ;3
mov eax,[ebx]             ;2
and dword eax,1           ;5
sub [ebp],eax             ;3
and dword [ebx],byte 0xfe ;3
cmp ecx,0                 ;5
jna NEGC_NOT_LESS_ZERO    ;2
or [ebx], byte 1          ;2
NEGC_NOT_LESS_ZERO:
cmp [ebp],ecx             ;3  
jna NEGC_NOT_LESS_OLD     ;2
or  [ebx], byte 1         ;2
NEGC_NOT_LESS_OLD:

opdesc EXTUB,	21,4,17,0xff,0xff,0xff
opfunc EXTUB
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
and dword eax,0x000000ff  ;5
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;3

opdesc EXTU_W,	21,4,17,0xff,0xff,0xff
opfunc EXTU_W
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
and dword eax,0xffff;5
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;3

opdesc EXTS_B,	19,4,15,0xff,0xff,0xff
opfunc EXTS_B
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
cbw                 ;2
cwde                ;1
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;3

opdesc EXTS_W,	17,4,13,0xff,0xff,0xff
opfunc EXTS_W
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
cwde                ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;2

;Store Register Opcodes
;----------------------

opdesc STC_SR_MEM,	23,0xff,6,0xff,0xff,0xff
opfunc STC_SR_MEM
push dword [ebx]        ;2
mov  ebp,edi            ;2
add  ebp,byte $00       ;3
sub  dword [ebp],byte 4 ;4
push dword [ebp]        ;3
mov  eax,_memSetLong    ;5
call eax                ;2
pop  eax                ;1
pop  eax                ;1

opdesc STC_GBR_MEM,	30,0xff,13,0xff,0xff,0xff
opfunc STC_GBR_MEM
mov  ebp,edi            ;2
add  ebp,byte 68        ;3
mov  eax,[ebp]          ;3
push eax                ;1
mov  ebp,edi            ;2
add  ebp,byte $00       ;3
sub  dword [ebp],byte 4 ;4
push dword [ebp]        ;3
mov  eax,_memSetLong    ;5
call eax                ;2
pop  eax                ;1
pop  eax                ;1

opdesc STC_VBR_MEM,	30,0xff,13,0xff,0xff,0xff
opfunc STC_VBR_MEM
mov  ebp,edi            ;2
add  ebp,byte 72        ;3
mov  eax,[ebp]          ;3
push eax                ;1
mov  ebp,edi            ;2
add  ebp,byte $00       ;3
sub  dword [ebp],byte 4 ;4
push dword [ebp]        ;3
mov  eax,_memSetLong    ;5
call eax                ;2
pop  eax                ;1
pop  eax                ;1


;------------------------------

opdesc MOVBL,	28,4,20,0xff,0xff,0xff
opfunc MOVBL
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
push eax            ;1
mov eax,_memGetByte ;5
call eax            ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
cbw                 ;1
cwde                ;1
mov [ebp],eax       ;3
pop eax             ;1


opdesc MOVWL,		26,4,20,0xff,0xff,0xff
opfunc MOVWL
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
push eax            ;1
mov eax,_memGetWord ;5
call eax            ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
cwde                ;1
mov [ebp],eax       ;3
pop eax             ;1

opdesc MOVL_MEM_REG,		25,4,20,0xff,0xff,0xff
opfunc MOVL_MEM_REG
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
push eax            ;1
mov eax,_memGetLong ;5
call eax            ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;3
pop eax             ;1

opdesc MOVBP,	31,4,23,0xff,0xff,0xff
opfunc MOVBP
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
inc dword [ebp]     ;3
push eax            ;1
mov eax,_memGetByte ;5
call eax            ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
cbw                 ;1
cwde                ;1
mov [ebp],eax       ;3
pop eax             ;1


opdesc MOVWP,	30,4,24,0xff,0xff,0xff
opfunc MOVWP
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
add dword [ebp],byte 2 ;4
push eax            ;1
mov eax,_memGetWord ;5
call eax            ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
cwde                ;1
mov [ebp],eax       ;3
pop eax             ;1

opdesc MOVLP,	29,4,24,0xff,0xff,0xff
opfunc MOVLP
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
add dword [ebp],byte 4 ;4
push eax            ;1
mov eax,_memGetLong ;5
call eax            ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;3
pop eax             ;1


opdesc MOVW_A,	38,0xff,4,0xff,17,0xff
opfunc MOVW_A
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[edx]       ;2
and eax,byte 0xfc   ;3
push eax            ;1
and eax,byte 0      ;3
mov al,byte $00     ;3
shl eax,byte 2      ;3
add eax,dword [esp] ;2
add eax,byte 4      ;3
push edx            ;1
push eax            ;1
mov eax,_memGetWord ;3
call eax            ;3
pop edx             ;1
cwde                ;1
mov [ebp],eax       ;2
pop eax             ;1

opdesc MOVL_A,	37,0xff,4,0xff,17,0xff
opfunc MOVL_A
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[edx]       ;2
and eax,byte 0xfc   ;3
push eax            ;1
and eax,byte 0      ;3
mov al,byte $00     ;3
shl eax,byte 2      ;3
add eax,dword [esp] ;2
add eax,byte 4      ;3
push edx            ;1
push eax            ;1
mov eax,_memGetLong ;3
call eax            ;3
pop edx             ;1
mov [ebp],eax       ;2
pop eax             ;1

opdesc MOVI,	13,0xff,4,0xff,9,0xff
opfunc MOVI
mov ebp,edi         ;2
add ebp,byte $00    ;3
xor eax,eax         ;2
or eax,byte 00      ;3
mov [ebp],eax       ;3

;----------------------

opdesc MOVBL0,	30,4,22,0xff,0xff,0xff
opfunc MOVBL0
mov ebp,edi          ;2
add ebp,byte $00     ;3
mov eax,[ebp]        ;3
add eax,[edi]        ;2
push eax             ;1
mov  eax,_memGetByte ;5
call eax             ;2
mov ebp,edi          ;2
add ebp,byte $00     ;3
cbw                  ;1
cwde                 ;1
mov [ebp],eax        ;3
pop  eax             ;1

opdesc MOVWL0,	28,4,22,0xff,0xff,0xff
opfunc MOVWL0
mov ebp,edi          ;2
add ebp,byte $00     ;3
mov eax,[ebp]        ;3
add eax,[edi]        ;2
push eax             ;1
mov  eax,_memGetWord ;5
call eax             ;2
mov ebp,edi          ;2
add ebp,byte $00     ;3
cwde                 ;1
mov [ebp],eax        ;3
pop  eax             ;1

opdesc MOVLL0,	27,4,22,0xff,0xff,0xff
opfunc MOVLL0
mov ebp,edi          ;2
add ebp,byte $00     ;3
mov eax,[ebp]        ;3
add eax,[edi]        ;2
push eax             ;1
mov  eax,_memGetLong ;5
call eax             ;2
mov ebp,edi          ;2
add ebp,byte $00     ;3
mov [ebp],eax        ;3
pop  eax             ;1

opdesc MOVT,		15,0xff,4,0xff,0xff,0xff
opfunc MOVT
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebx]       ;2
and dword eax,1     ;5
mov [ebp],eax       ;3

opdesc MOVBS0,	28,4,12,0xff,0xff,0xff
opfunc MOVBS0
mov  ebp,edi         ;2
add  ebp,byte $00    ;3 m(4..7)
push dword [ebp]     ;3
mov  ebp,edi         ;2
add  ebp,byte $00    ;3 n(8..11)
mov  eax,[ebp]       ;3
add  eax,[edi]       ;2
push eax             ;1
mov  eax,_memSetByte ;5
call eax             ;2
pop  eax             ;1
pop  eax             ;1

opdesc MOVWS0,	28,4,12,0xff,0xff,0xff
opfunc MOVWS0
mov  ebp,edi         ;2
add  ebp,byte $00    ;3 m(4..7)
push dword [ebp]     ;3
mov  ebp,edi         ;2
add  ebp,byte $00    ;3 n(8..11)
mov  eax,[ebp]       ;3
add  eax,[edi]       ;2
push eax             ;1
mov  eax,_memSetWord ;5
call eax             ;2
pop  eax             ;1
pop  eax             ;1

opdesc MOVLS0,	28,4,12,0xff,0xff,0xff
opfunc MOVLS0
mov  ebp,edi         ;2
add  ebp,byte $00    ;3 m(4..7)
push dword [ebp]     ;3
mov  ebp,edi         ;2
add  ebp,byte $00    ;3 n(8..11)
mov  eax,[ebp]       ;3
add  eax,[edi]       ;2
push eax             ;1
mov  eax,_memSetLong ;5
call eax             ;2
pop  eax             ;1
pop  eax             ;1

;===========================================================================
;Verified Opcodes
;===========================================================================

opdesc DT,		20,0xff,4,0xff,0xff,0xff
opfunc DT
mov ebp,edi         ;2
add ebp,byte $00    ;3
and dword [ebx],byte 0xfe ;3
dec dword [ebp]     ;3
cmp dword [ebp],byte 0 ;4
jne .continue       ;2
or dword [ebx],byte 1 ;3
.continue

opdesc CMP_PZ,	17,0xff,4,0xff,0xff,0xff
opfunc CMP_PZ
mov ebp,edi               ;2
add ebp,byte $00          ;3
and dword [ebx],byte 0xfe ;3
cmp dword [ebp],byte 0    ;4
jl .continue              ;2
or [ebx], byte 1          ;3
.continue

opdesc CMP_PL,	23,0xff,4,0xff,0xff,0xff
opfunc CMP_PL
mov ebp,edi                 ;2
add ebp,byte $00            ;3
and dword [ebx], 0xFFFFFFFe ;6
cmp dword [ebp], 0          ;7
jle .continue               ;2
or [ebx], byte 1            ;3 Set T Flg
.continue

opdesc CMP_EQ,	24,4,12,0xff,0xff,0xff
opfunc CMP_EQ
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
and dword [ebx],byte 0xfe ;3
cmp [ebp],eax       ;3
jne .continue       ;2  
or [ebx],byte 1     ;3 [edp] == eax
.continue

opdesc CMP_HS,	24,4,12,0xff,0xff,0xff
opfunc CMP_HS
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
mov ebp,edi         ;2
add ebp,byte $00    ;3
and dword [ebx],byte 0xfe ;6
cmp [ebp],eax       ;3
jb .continue        ;2
or [ebx],byte 1     ;3 
.continue

opdesc CMP_HI,	24,4,12,0xff,0xff,0xff
opfunc CMP_HI
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
and dword [ebx],byte 0xfe ;3
cmp [ebp],eax       ;3
jbe .continue       ;2
or [ebx],byte 1     ;3 
.continue

opdesc CMP_GE,	24,4,12,0xff,0xff,0xff
opfunc CMP_GE
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
and dword [ebx],byte 0xfe ;3
cmp [ebp],eax       ;3
jl .continue        ;2
or [ebx],byte 1     ;3 
.continue

opdesc CMP_GT,	24,4,12,0xff,0xff,0xff
opfunc CMP_GT
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
and dword [ebx],byte 0xfe ;3
cmp [ebp],eax       ;3
jle .continue       ;2
or [ebx],byte 1     ;3 
.continue

opdesc ROTL,	23,0xff,4,0xff,0xff,0xff
opfunc ROTL
mov ebp,edi               ;2
add ebp,byte $00          ;3
mov eax,[ebp]             ;2
shr eax,byte 31           ;3
and dword [ebx],byte 0xfe ;3
or [ebx],eax              ;2
shl dword [ebp],byte 1    ;4
or [ebp],eax              ;2

opdesc ROTR,	25,0xff,4,0xff,0xff,0xff
opfunc ROTR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
and eax,byte 1      ;3
and dword [ebx],byte 0xfe ;3
or [ebx],eax        ;2
shr dword [ebp],1   ;2
shl eax,byte 31     ;3
or [ebp],eax        ;2

opdesc ROTCL,	30,0xff,4,0xff,0xff,0xff
opfunc ROTCL
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebx]       ;2
and dword eax,1     ;5
mov ecx,[ebp]       ;2
shr ecx,byte 31     ;3
and dword [ebx],byte 0xfe ;3
or [ebx],ecx        ;2
shl dword [ebp],byte 1 ;4
or [ebp],eax        ;2


opdesc ROTCR,	36,0xff,4,0xff,0xff,0xff
opfunc ROTCR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebx]       ;2
and dword eax,1     ;5
shl eax,byte 31     ;3
mov ecx,[ebp]       ;2
and dword ecx,1     ;5
and dword [ebx],byte 0xfe ;3
or [ebx],ecx        ;2
shr dword [ebp],byte 1 ;4
or [ebp],eax        ;2

opdesc SHL,		20,0xff,4,0xff,0xff,0xff
opfunc SHL
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
shr eax,byte 31     ;3
and dword [ebx],byte 0xfe ;3
or [ebx],eax        ;2
shl dword [ebp],byte 1 ;4

opdesc SHLR,	25,0xff,4,0xff,0xff,0xff
opfunc SHLR
mov ebp,edi                 ;2
add ebp,byte $00            ;3
mov eax,[ebp]               ;3
and dword eax,1             ;5
and dword [ebx], 0xFFFFFFFE ;6
or [ebx],eax                ;2
shr dword [ebp],byte 1      ;4

;opdesc SHAR,	33,0xff,4,0xff,0xff,0xff
;opfunc SHAR
;mov ebp,edi         ;2
;add ebp,byte $00    ;3
;mov eax,[ebp]       ;2
;and dword eax,1     ;5
;and dword [ebx],byte 0xfe ;3
;or [ebx],eax        ;2
;mov eax,[ebp]       ;2
;and eax,0xffx80000000  ;5
;shr dword [ebp],byte 1 ;4
;or [ebp],eax        ;2

opdesc SHAR,	22,0xff,4,0xff,0xff,0xff
opfunc SHAR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
and dword eax,1     ;5
and dword [ebx],byte 0xfe ;3
or  [ebx],eax          ;2
sar dword [ebp],byte 1 ;4



opdesc SHLL2,	9,0xff,4,0xff,0xff,0xff
opfunc SHLL2
mov ebp,edi         ;2
add ebp,byte $00    ;3
shl dword [ebp],byte 2 ;4

opdesc SHLR2,	9,0xff,4,0xff,0xff,0xff
opfunc SHLR2
mov ebp,edi         ;2
add ebp,byte $00    ;3
shr dword [ebp],byte 2 ;4

opdesc SHLL8,	9,0xff,4,0xff,0xff,0xff
opfunc SHLL8
mov ebp,edi         ;2
add ebp,byte $00    ;3
shl dword[ebp],byte 8  ;4

opdesc SHLR8,	9,0xff,4,0xff,0xff,0xff
opfunc SHLR8
mov ebp,edi         ;2
add ebp,byte $00    ;3
shr dword [ebp],byte 8 ;4

opdesc SHLL16,	9,0xff,4,0xff,0xff,0xff
opfunc SHLL16
mov ebp,edi         ;2
add ebp,byte $00    ;3
shl dword [ebp],byte 16 ;4

opdesc SHLR16,	9,0xff,4,0xff,0xff,0xff
opfunc SHLR16
mov ebp,edi         ;2
add ebp,byte $00    ;3
shr dword [ebp],byte 16 ;4

opdesc AND,		16,4,12,0xff,0xff,0xff
opfunc AND
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
and [ebp],eax       ;2

opdesc OR,		16,4,12,0xff,0xff,0xff
opfunc OR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
or [ebp],eax        ;2

opdesc XOR,		16,4,12,0xff,0xff,0xff
opfunc XOR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte $00    ;3
xor [ebp],eax       ;3

opdesc ADDI,	9,0xff,4,0xff,8,0xff
opfunc ADDI
mov ebp,edi         ;2
add ebp,byte $00    ;3
add dword [ebp],byte $00 ;4


opdesc AND_B,	35,0xff,0xff,0xff,21,0xff
opfunc AND_B
push edx            ;1
ctrlreg_load 1      ;5
mov eax,[ebp]       ;3
add eax, dword [edi];3
push eax            ;1
mov eax, _memGetByte;5
call eax            ;2
pop ebp             ;1
and al,byte $00     ;2
push eax            ;1
push ebp            ;1
mov eax, _memSetByte;5
call eax            ;2
add esp, byte 8     ;3
pop edx             ;1

opdesc OR_B,	36,0xff,0xff,0xff,22,0xff
opfunc OR_B
push edx
mov  ebp,edi          ;2
add  ebp,byte 68      ;3
mov  eax, [ebp]       ;3 Get GBR
add  eax, dword [edi] ;2 ADD R0
mov  edx, eax         ;2 Save Dist addr
push eax              ;1
mov  eax, _memGetByte ;5
call eax              ;2
or   al,byte $00      ;2
push eax              ;1 data
push edx              ;1 addr
mov  eax, _memSetByte ;5
call eax              ;2
pop  edx              ;1
pop  eax              ;1
pop  eax              ;1
pop  edx              ;1


opdesc XOR_B,	36,0xff,0xff,0xff,22,0xff
opfunc XOR_B
push edx
mov  ebp,edi          ;2
add  ebp,byte 68      ;3
mov  eax, [ebp]       ;3 Get GBR
add  eax, dword [edi] ;2 ADD R0
mov  edx, eax         ;2 Save Dist addr
push eax              ;1
mov  eax, _memGetByte ;5
call eax              ;2
xor   al,byte $00      ;2
push eax              ;1 data
push edx              ;1 addr
mov  eax, _memSetByte ;5
call eax              ;2
pop  edx              ;1
pop  eax              ;1
pop  eax              ;1
pop  edx              ;1



opdesc TST_B,	34,0xff,0xff,0xff,25,0xff
opfunc TST_B
ctrlreg_load 1        ;5
mov  eax,[ebp]        ;3 Get GBR
add  eax, dword [edi] ;2 Add R[0]
push eax              ;1
mov  eax, _memGetByte ;5
call eax              ;2
and  dword [ebx],0xFFFFFFFE ;6
and  al,byte $00      ;2
cmp  al,0             ;2
jne .continue         ;2
or dword [ebx],byte 1 ;3
.continue
pop eax               ;1

;Jump Opcodes
;------------

opdesc JMP,		11,0xff,4,0xff,0xff,0xff
opfunc JMP
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov [esp],eax       ;3

opdesc JSR,		19,0xff,12,0xff,0xff,0xff
opfunc JSR
mov eax,[edx]       ;2
add eax,byte 4      ;3
mov [esi+8],eax     ;3
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
mov [esp],eax       ;3

opdesc BRA,		29,0xff,0xff,0xff,0xff,5
opfunc BRA
and eax,byte 00     ;3
mov ax,0            ;4
shl eax,byte 1      ;3
cmp ax,0xfff        ;4
jle .continue       ;2
or eax,0xfffff000   ;5
.continue
add eax,byte 4      ;3
add eax,dword [edx] ;2
mov [esp],eax       ;3

opdesc BSR,		37,0xff,0xff,0xff,0xff,13
opfunc BSR
mov eax,[edx]       ;2
add eax,byte 4      ;3
mov [esi+8],eax     ;3
and eax,byte 00     ;3
mov ax,0            ;4
shl eax,byte 1      ;3
cmp ax,0xfff        ;4
jle .continue       ;2
or eax,0xfffff000   ;5
.continue
add eax,byte 4      ;3
add eax,dword [edx] ;2
mov [esp],eax       ;3

opdesc BSRF,		24,0xff,4,0xff,0xff,0xff
opfunc BSRF
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[edx]       ;2
add eax,byte 4      ;3
mov [esi+8],eax     ;3
mov eax,[edx]       ;3
add eax,dword [ebp] ;3
add eax,byte 4      ;3
mov [esp],eax       ;3

opdesc BRAF,		16,0xff,4,0xff,0xff,0xff
opfunc BRAF
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[edx]       ;2
add eax,dword [ebp] ;3
add eax,byte 4      ;4
mov [esp],eax       ;3


opdesc RTS,			6,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc RTS
mov eax,[esi+8]     ;3
mov [esp],eax       ;3

opdesc RTE,			46,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc RTE
mov  ebp,edi            ;2
add  ebp,byte 60        ;3
push dword [ebp]        ;3
mov  eax,_memGetLong    ;5
call eax                ;2
mov  dword [esp+4],eax  ;4  Get PC
add  dword [ebp],byte 4 ;4
push dword [ebp]        ;3
mov  eax,_memGetLong    ;5  
call eax                ;2
and  eax,0x000003f3     ;5  Get SR
mov  [ebx],eax          ;2
add  dword [ebp],byte 4 ;4
pop  eax                ;1
pop  eax                ;1

opdesc TRAPA,	      75,0xFF,0xFF,0xFF,50,0xFF
opfunc TRAPA
mov  ebp,edi          ;2
add  ebp,byte 60      ;3
sub  dword [ebp],4    ;7
push dword [ebx]      ;2 SR
push dword [ebp]      ;3
mov  eax, _memSetLong ;5
call eax              ;2
sub  dword [ebp],4    ;7
mov  eax,[edx]        ;3 PC
add  eax,byte 2       ;3
push eax              ;1
push dword [ebp]      ;2
mov  eax, _memSetLong ;5
call eax              ;2
xor  eax,eax          ;2
mov  al,byte $00      ;2 Get Imm
shl  eax,2            ;3
add  eax,[ebx+8]      ;3 ADD VBR
push eax              ;1
mov  eax, _memGetLong ;5
call eax              ;2
mov  [edx],eax        ;3
sub  [edx],byte 2     ;3
pop  eax              ;1
pop  eax              ;1
pop  eax              ;1
pop  eax              ;1
pop  eax              ;1



opdesc BT,		30,0xFF,0xFF,0xFF,18,0xFF
opfunc BT
and [esp],dword 0   ;7
bt dword [ebx],0    ;4
jnc .continue       ;2
and eax,byte 00     ;3
or  eax,byte 00     ;3
shl eax,byte 1      ;3
add eax,byte 4      ;3
add eax,dword [edx] ;2
mov [esp],eax       ;3
.continue

opdesc BF,		30,0xFF,0xFF,0xFF,18,0xFF
opfunc BF
and [esp],dword 0   ;7
bt dword [ebx],0    ;4
jc .continue        ;2
and eax,byte 00     ;3
or  eax,byte 00     ;3
shl eax,byte 1      ;3
add eax,byte 4      ;3
add eax,dword [edx] ;2
mov [esp],eax       ;3
.continue

opdesc BF_S,		23,0xFF,0xFF,0xFF,11,0xFF
opfunc BF_S
bt dword [ebx],0    ;4
jc .continue        ;2
and eax,byte 00     ;3
or eax,byte 00      ;3
shl eax,byte 1      ;3
add eax,byte 4      ;3
add eax,dword [edx] ;2
mov [esp],eax       ;3
.continue


;Store/Load Opcodes
;------------------

opdesc STC_SR,	10,0xFF,4,0xFF,0xFF,0xFF
opfunc STC_SR
mov ebp,edi
add ebp,byte $00
mov eax,[ebx]
mov [ebp],eax

opdesc STC_GBR,	18,0xFF,4,0xFF,0xFF,0xFF
opfunc STC_GBR
mov ebp,edi         ;2
add ebp,byte $00    ;3
push ebp            ;1
ctrlreg_load 1      ;5
mov eax,[ebp]       ;2
pop ebp             ;1
mov [ebp],eax       ;2

opdesc STC_VBR,	18,0xFF,4,0xFF,0xFF,0xFF
opfunc STC_VBR
mov ebp,edi
add ebp,byte $00
push ebp
ctrlreg_load 2
mov eax,[ebp]
pop ebp
mov [ebp],eax

opdesc STS_MACH, 10,0xFF,4,0xFF,0xFF,0xFF
opfunc STS_MACH
mov ebp,edi       ;2
add ebp,byte $00  ;3
mov eax,[esi]     ;2
mov [ebp],eax     ;2

opdesc STS_MACH_DEC,	25,0xFF,4,0xFF,0xFF,0xFF
opfunc STS_MACH_DEC
mov ebp,edi       ;2
add ebp,byte $00  ;3
sub dword [ebp],byte 4 ;3
mov eax,[esi]     ;2
push eax          ;1
mov eax,[ebp]     ;2
push eax          ;1
mov eax,_memSetLong ;5
call eax          ;2
pop eax ;1
pop eax ;1

opdesc STS_MACL, 11,0xFF,4,0xFF,0xFF,0xFF
opfunc STS_MACL
mov ebp,edi       ;2
add ebp,byte $00  ;3
mov eax,[esi+4]   ;3
mov [ebp],eax     ;2

opdesc STS_MACL_DEC,	26,0xFF,4,0xFF,0xFF,0xFF
opfunc STS_MACL_DEC
mov ebp,edi       ;2
add ebp,byte $00  ;3
sub dword [ebp],byte 4 ;3
mov eax,[esi+4]   ;3
push eax          ;1
mov eax,[ebp]     ;2
push eax          ;1
mov eax,_memSetLong ;5
call eax          ;2
pop eax ;1
pop eax ;1


;opdesc LDC_SR,	9,0xFF,4,0xFF,0xFF,0xFF
;opfunc LDC_SR
;mov ebp,edi
;add ebp,byte $00
;mov eax,[ebp]
;mov [ebx],eax

opdesc LDC_SR,	21,0xFF,4,0xFF,0xFF,0xFF
opfunc LDC_SR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
and eax,0x000003f3  ;5
mov ebp,edi         ;2
add ebp,byte 64     ;3
mov [ebp],eax       ;2


opdesc LDC_SR_INC,	28,0xFF,4,0xFF,0xFF,0xFF
opfunc LDC_SR_INC
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
push eax            ;1
mov eax,_memGetLong ;5
call eax            ;2
and eax,0x3f3       ;5
mov dword [ebx],eax ;3
add dword [ebp],byte 4 ;3
pop eax             ;1

opdesc LDCGBR,	16,0xff,4,0xFF,0xFF,0xFF
opfunc LDCGBR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
mov ebp,edi         ;2
add ebp,byte 68     ;3
mov [ebp],eax       ;3

opdesc LDC_GBR_INC,	29,0xFF,4,0xFF,0xFF,0xFF
opfunc LDC_GBR_INC
mov  ebp,edi            ;2
add  ebp,byte $00       ;3
mov  eax,[ebp]          ;2
add  dword [ebp],byte 4 ;3
push eax                ;1
mov  eax,_memGetLong    ;5
call eax                ;2
mov  ebp,edi            ;2
add  ebp,byte 68        ;3
mov  dword [ebp],eax    ;3
pop  eax                ;1


opdesc LDC_VBR,	16,0xFF,4,0xFF,0xFF,0xFF
opfunc LDC_VBR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov ebp,edi         ;2
add ebp,byte 72     ;3
mov [ebp],eax       ;2

opdesc LDC_VBR_INC,	29,0xFF,4,0xFF,0xFF,0xFF
opfunc LDC_VBR_INC
mov  ebp,edi            ;2
add  ebp,byte $00       ;3
mov  eax,[ebp]          ;2
add  dword [ebp],byte 4 ;3
push eax                ;1
mov  eax,_memGetLong    ;5
call eax                ;2
mov  ebp,edi            ;2
add  ebp,byte 72        ;3
mov  dword [ebp],eax    ;3
pop  eax                ;1

opdesc STS_PR,		11,0xFF,4,0xFF,0xFF,0xFF
opfunc STS_PR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[esi+8]     ;3
mov [ebp],eax       ;3

opdesc STSMPR,	26,0xFF,4,0xFF,0xFF,0xFF
opfunc STSMPR
mov ebp,edi       ;2
add ebp,byte $00  ;3
sub dword [ebp],byte 4 ;3
mov eax,[esi+8]   ;3
push eax          ;1
mov eax,[ebp]     ;2
push eax          ;1
mov eax,_memSetLong ;5
call eax          ;2
pop eax           ;1
pop eax           ;1

opdesc LDS_PR,		11,0xFF,4,0xFF,0xFF,0xFF
opfunc LDS_PR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov [esi+8],eax     ;4

opdesc LDS_PR_INC,	24,0xFF,4,0xFF,0xFF,0xFF
opfunc LDS_PR_INC
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
push eax            ;1
mov eax,_memGetLong ;5
call eax            ;2
mov [esi+8],eax     ;3
add dword [ebp],byte 4 ;4
pop eax             ;1

opdesc LDS_MACH,		10,0xFF,4,0xFF,0xFF,0xFF
opfunc LDS_MACH
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov [esi],eax       ;2

opdesc LDS_MACH_INC,	23,0xFF,4,0xFF,0xFF,0xFF
opfunc LDS_MACH_INC
mov ebp,edi            ;2
add ebp,byte $00       ;3
mov eax,[ebp]          ;2
push eax               ;1
mov eax,_memGetLong    ;5
call eax               ;2
mov [esi],eax          ;2
add dword [ebp],byte 4 ;3
pop eax                ;1

opdesc LDS_MACL,		11,0xFF,4,0xFF,0xFF,0xFF
opfunc LDS_MACL
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;2
mov [esi+4],eax     ;2


opdesc LDS_MACL_INC,	24,0xFF,4,0xFF,0xFF,0xFF
opfunc LDS_MACL_INC
mov ebp,edi            ;2
add ebp,byte $00       ;3
mov eax,[ebp]          ;2
push eax               ;1
mov eax,_memGetLong    ;5
call eax               ;2
mov [esi+4],eax        ;3
add dword [ebp],byte 4 ;3
pop eax                ;1

;Mov Opcodes
;-----------

opdesc MOVA,	25,0xFF,0xFF,0xFF,14,0xFF
opfunc MOVA
mov ebp,edi         ;2
mov eax,[edx]       ;2
add eax,byte 4      ;3
and eax,byte 0xfc   ;3
push eax            ;1
xor eax,eax         ;2
mov al,byte $00     ;3
shl eax,byte 2      ;3
add eax,dword [esp] ;2
mov [ebp],eax       ;2
pop eax             ;1


opdesc MOVWI,	31,0xFF,4,0xFF,8,0xFF
opfunc MOVWI
mov ebp,edi         ;2
add ebp,byte $00    ;3
xor eax,eax         ;2
mov al,00           ;2
shl ax,byte 1       ;1
add eax,[edx]       ;2 
add eax,byte 4      ;3
push eax            ;1
mov eax,_memGetWord ;5
call eax            ;2
cwde                ;1
mov [ebp],eax       ;3
pop eax             ;1


opdesc MOVLI,       40,0xFF,4,0xFF,8,0xFF
opfunc MOVLI
mov ebp,edi         ;2
add ebp,byte $00    ;3
xor eax,eax         ;2
mov al,00           ;2
shl ax,byte 2       ;3
push ecx            ;1
mov ecx,[edx]       ;2
add ecx,byte 4      ;3
and ecx,0xFFFFFFFC  ;6
add eax,ecx         ;2 
push eax            ;1
mov eax,_memGetLong ;5
call eax            ;2
mov [ebp],eax       ;3
pop eax             ;1
pop ecx             ;1

opdesc MOVBL4, 29,4,0xFF,8,0xFF,0xFF
opfunc MOVBL4
mov  ebp,edi          ;2  Get R0 adress
add  ebp,byte $00     ;3
xor  eax,eax          ;2  Clear Eax
mov  al,$00           ;2  Get Disp
add  eax,[ebp]        ;3
push eax              ;1  Set Func
mov  eax,_memGetByte  ;5
call eax              ;2
mov  ebp,edi          ;2  Get R0 adress
cbw                   ;1  Sign extension byte -> word
cwde                  ;1  Sign extension word -> dword
mov  [ebp],eax        ;3
pop  eax              ;1


opdesc MOVWL4, 31,4,0xFF,8,0xFF,0xFF
opfunc MOVWL4
mov  ebp,edi          ;2  Get R0 adress
add  ebp,byte $00     ;3
xor  eax,eax          ;2  Clear Eax
mov  al,$00           ;2  Get Disp
shl  ax, byte 1       ;3  << 1
add  eax,[ebp]        ;2
push eax              ;1  Set Func
mov  eax,_memGetWord  ;5
call eax              ;2
mov  ebp,edi          ;2  Get R0 adress
cwde                  ;2  sign 
mov  [ebp],eax        ;3
pop  eax              ;1


opdesc MOVLL4, 33,4,28,8,0xFF,0xFF
opfunc MOVLL4
mov  ebp,edi          ;2  Get R0 adress
add  ebp,byte $00     ;3
xor  eax,eax          ;2  Clear Eax
mov  al,$00           ;2  Get Disp
shl  ax, byte 2       ;3  << 2
add  eax,[ebp]        ;2
push eax              ;1  Set Func
mov  eax,_memGetLong  ;5
call eax              ;2
mov  ebp,edi          ;2  Get R0 adress
add  ebp,byte $00     ;3
mov  [ebp],eax        ;3
pop  eax              ;1
 
opdesc MOVBS4,	29,7,0xFF,15,0xFF,0xFF
opfunc MOVBS4
mov ebp,edi         ;2  Get R0 address
push dword [ebp]    ;3  Set to Func
add ebp,byte $00    ;3  Get Imidiate value R[n]
and eax,00000000h   ;5  clear eax
or  eax,byte $00    ;3  Get Disp value
add eax,[ebp]       ;3  Add Disp value
push eax            ;1  Set to Func
mov eax,_memSetByte ;5
call eax            ;2  Call Func
pop eax             ;1
pop eax             ;1

opdesc MOVWS4,	32,7,0xFF,15,0xFF,0xFF
opfunc MOVWS4
mov ebp,edi         ;2  Get R0 address
push dword [ebp]    ;3  Set to Func
add ebp,byte $00    ;3  Get Imidiate value R[n]
and eax,00000000h   ;5  clear eax
or  eax,byte $00    ;3  Get Disp value
shl eax,byte 1      ;3  Shift Left
add eax,[ebp]       ;3  Add Disp value
push eax            ;1  Set to Func
mov eax,_memSetWord ;5
call eax            ;2  Call Func
pop eax             ;1
pop eax             ;1


opdesc MOVLS4,	36,4,12,20,0xFF,0xFF
opfunc MOVLS4
mov  ebp,edi         ;2 
add  ebp,byte $00    ;3 Get m 4..7
push dword [ebp]     ;3
mov  ebp,edi         ;2
add  ebp,byte $00    ;3 Get n 8..11
push dword [ebp]     ;3
xor  eax,eax         ;2
or   eax,byte 0      ;3 Get Disp 0..3
shl  eax,byte 2      ;3
add  dword [esp],eax ;2
mov  eax,_memSetLong ;5
call eax             ;2
pop  eax             ;1
pop  eax             ;1

 
opdesc MOVBLG,    24,0xFF,0xFF,0xFF,5,0xFF
opfunc MOVBLG
mov  ebp,edi           ;2  Get R0 adress
xor  eax,eax           ;2  clear eax
mov  al,00             ;2  Get Imidiate Value
add  eax,dword [ebp+68];3  GBR + IMM( Adress for Get Value )
push eax               ;1
mov  eax,_memGetByte   ;5
call eax               ;2
cbw                    ;1
cwde                   ;1
mov  [ebp],eax         ;3
pop  eax               ;1


opdesc MOVWLG,    26,0xFF,0xFF,0xFF,5,0xFF
opfunc MOVWLG
mov  ebp,edi           ;2  Get R0 adress
xor  eax,eax           ;2  clear eax
mov  al,00             ;2  Get Imidiate Value
shl  ax,byte 1         ;3  Shift left 2
add  eax,dword [ebp+68];3  GBR + IMM( Adress for Get Value )
push eax               ;1
mov  eax,_memGetWord   ;5
call eax               ;2
cwde                   ;1
mov  [ebp],eax         ;3
pop  eax               ;1


opdesc MOVLLG,    25,0xFF,0xFF,0xFF,5,0xFF
opfunc MOVLLG
mov  ebp,edi           ;2  Get R0 adress
xor  eax,eax           ;2  clear eax
mov  al,00             ;5  Get Imidiate Value
shl  ax,byte 2         ;3  Shift left 2
add  eax,dword [ebp+68];3  GBR + IMM( Adress for Get Value )
push eax               ;1
mov  eax,_memGetLong   ;5
call eax               ;2
mov  [ebp],eax         ;3
pop  eax               ;1


opdesc MOVBSG,    26,0xFF,0xFF,0xFF,8,0xFF
opfunc MOVBSG
mov  ebp,edi         ;2  Get R0 adress
push dword [ebp]     ;3
xor  eax,eax         ;2  Clear Eax
mov  al,00           ;5  Get Imidiate Value
mov  ecx,edi         ;2  Get GBR adress
add  ecx,byte 68     ;3 
add  eax,dword [ecx] ;2  GBR + IMM( Adress for Get Value )
push eax             ;1
mov  eax,_memSetByte ;5
call eax             ;2
pop  eax             ;1
pop  eax             ;1

opdesc MOVWSG,    30,0xFF,0xFF,0xFF,8,0xFF
opfunc MOVWSG
mov  ebp,edi         ;2  Get R0 adress
push dword [ebp]     ;3
xor  eax,eax         ;2  Clear Eax
mov  al,00           ;5  Get Imidiate Value
shl  ax,byte 1       ;3  Shift left 2
mov  ecx,edi         ;2  Get GBR adress
add  ecx,byte 68     ;3 
add  eax,dword [ecx] ;2  GBR + IMM( Adress for Get Value )
push eax             ;1
mov  eax,_memSetWord ;5
call eax             ;2
pop  eax             ;1
pop  eax             ;1

opdesc MOVLSG,    30,0xFF,0xFF,0xFF,8,0xFF
opfunc MOVLSG
mov  ebp,edi         ;2  Get R0 adress
push dword [ebp]     ;3
xor  eax,eax         ;2  Clear Eax
mov  al,00           ;5  Get Imidiate Value
shl  ax,byte 2       ;3  Shift left 2
mov  ecx,edi         ;2  Get GBR adress
add  ecx,byte 68     ;3 
add  eax,dword [ecx] ;2  GBR + IMM( Adress for Get Value )
push eax             ;1
mov  eax,_memSetLong ;5
call eax             ;2
pop  eax             ;1
pop  eax             ;1


opdesc MOVBS,	25,4,12,0xFF,0xFF,0xFF
opfunc MOVBS
mov  ebp,edi         ;2
add  ebp,byte $00    ;3
push dword [ebp]     ;3
mov  ebp,edi         ;2
add  ebp,byte $00    ;3
push dword [ebp]     ;3
mov  eax,_memSetByte ;5
call eax             ;2
pop  eax             ;1
pop  eax             ;1


opdesc MOVWS,	25,4,12,0xFF,0xFF,0xFF
opfunc MOVWS
mov  ebp,edi          ;2
add  ebp,byte $00     ;3
push dword [ebp]      ;3
mov  ebp,edi          ;2
add  ebp,byte $00     ;3
push dword [ebp]      ;3
mov  eax,_memSetWord  ;5
call eax              ;2
pop  eax              ;1
pop  eax              ;1

opdesc MOVLS,	25,4,12,0xFF,0xFF,0xFF
opfunc MOVLS
mov  ebp,edi         ;2
add  ebp,byte $00    ;3
push dword [ebp]     ;3
mov  ebp,edi         ;2
add  ebp,byte $00    ;3
push dword [ebp]     ;3
mov  eax,_memSetLong ;5
call eax             ;2
pop  eax             ;1
pop  eax             ;1


opdesc MOVR,		16,4,12,0xFF,0xFF,0xFF
opfunc MOVR
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov eax,[ebp]       ;3
mov ebp,edi         ;2
add ebp,byte $00    ;3
mov [ebp],eax       ;3

opdesc MOVBM,		29,4,12,0xFF,0xFF,0xFF
opfunc MOVBM
mov  ebp,edi             ;2
add  ebp,byte $00        ;3  
push dword [ebp]         ;3 Set data
mov  ebp,edi             ;2
add  ebp,byte $00        ;3
sub  dword [ebp],byte 1  ;4
push dword [ebp]         ;3 Set addr
mov  eax,_memSetByte     ;5
call eax                 ;2
pop  eax                 ;1
pop  eax                 ;1

opdesc MOVWM,		29,4,12,0xFF,0xFF,0xFF
opfunc MOVWM
mov  ebp,edi             ;2
add  ebp,byte $00        ;3  
push dword [ebp]         ;3 Set data
mov  ebp,edi             ;2
add  ebp,byte $00        ;3
sub  dword [ebp],byte 2  ;4
push dword [ebp]         ;3 Set addr
mov  eax,_memSetWord     ;5
call eax                 ;2
pop  eax                 ;1
pop  eax                 ;1



opdesc MOVLM,		29,4,12,0xFF,0xFF,0xFF
opfunc MOVLM
mov  ebp,edi             ;2
add  ebp,byte $00        ;3  
push dword [ebp]         ;3 Set data
mov  ebp,edi             ;2
add  ebp,byte $00        ;3
sub  dword [ebp],byte 4  ;4
push dword [ebp]         ;3 Set addr
mov  eax,_memSetLong     ;5
call eax                 ;2
pop  eax                 ;1
pop  eax                 ;1

;------------- added ------------------

opdesc TAS,  46,0xFF,4,0xFF,0xFF,0xFF
opfunc TAS
mov  ebp,edi             ;2
add  ebp,byte $00        ;3
push dword [ebp]         ;3
mov  eax,_memGetByte     ;5
call eax                 ;2
and  eax,0x000000FF      ;5
and  [ebx], byte 0xFE    ;3
test eax,eax             ;3
jne  NOT_ZERO            ;2
or   [ebx], byte 01      ;3
NOT_ZERO:              
or   al, byte 0x80        ;3
push eax                  ;1 
push dword [ebp]          ;3
mov  eax,_memSetByte      ;5
call eax                  ;2
pop eax                   ;1
pop eax                   ;1
pop eax                   ;1


; sub with overflow check
opdesc SUBV, 24,4,12,0xFF,0xFF,0xFF
opfunc SUBV
mov  ebp,edi          ;2
add  ebp,byte $00     ;3 m 4...7
mov  eax,[ebp]        ;3
mov  ebp,edi          ;2
add  ebp,byte $00     ;3 n 8...11
and  [ebx], byte 0xFE ;3
sub  [ebp],eax        ;3 R[n] = R[n] - R[m]
jno	 NO_OVER_FLOS     ;2
or   [ebx], byte 01   ;3
NO_OVER_FLOS


; string cmp
opdesc CMPSTR, 60,4,12,0xFF,0xFF,0xFF
opfunc CMPSTR
mov  ebp,edi          ;2
add  ebp,byte $00     ;3
mov  eax,[ebp]        ;3
mov  ebp,edi          ;2
add  ebp,byte $00     ;3
mov  ecx,[ebp]        ;3
xor  eax,ecx          ;2  
and  [ebx], byte 0xFE ;3  Clear T flag
mov  ecx,eax          ;2  
shr  ecx,byte 24      ;3 1Byte Check
test cl,cl            ;1
je  HIT_BYTE          ;2
mov  ecx,eax          ;2  
shr  ecx,byte 16       ;3 1Byte Check
test cl,cl            ;2
je  HIT_BYTE         ;2
mov  ecx,eax          ;2  
shr  ecx,byte 8       ;3 1Byte Check
test cl,cl            ;2
je  HIT_BYTE         ;2
test al,al            ;2
je  HIT_BYTE         ;2
jmp  ENDPROC          ;3
HIT_BYTE:
or   [ebx], byte 01   ;3 T flg ON
ENDPROC:

;-------------------------------------------------------------
;div0s 
opdesc DIV0S, 78,36,4,0xFF,0xFF,0xFF
opfunc DIV0S
mov  ebp,edi                 ;2 SetQ
add  ebp,byte $00            ;3 8..11
and  dword [ebx],0xFFFFFEFF  ;6 Clear Q Flg

mov  eax, 0                  ;5 Zero Clear eax     

test dword [ebp],0x80000000  ;7 Test sign
je   continue                ;2 if ZF = 1 then goto NO_SIGN
or   dword [ebx],0x00000100  ;6 Set Q Flg
inc  eax                     ;1 

continue:
mov  ebp,edi                 ;2 SetM
add  ebp,byte $00            ;3 4..7
and  dword [ebx],0xFFFFFDFF  ;6 Clear M Flg
test dword [ebp],0x80000000  ;7 Test sign
je   continue2               ;2 if ZF = 1 then goto NO_SIGN
or   dword [ebx],0x00000200  ;6  Set M Flg
inc  eax                     ;1

continue2:
and  dword [ebx],0xFFFFFFFE  ;6 Clear T Flg
test eax, 1                  ;5 if( Q != M ) SetT(1)
je  continue3                ;2
or   dword [ebx],0x00000001  ;6 Set T Flg
continue3:
 

;===============================================================
; DIV1   1bit Divid operation
; 
; size = 69 + 135 + 132 + 38 = 374 
;===============================================================
opdesc DIV1, 410,62,7,0xFF,0xFF,0xFF
opfunc DIV1

; 69
push edx                     ;1
push esi                     ;1
mov  ebp,edi                 ;2 Get R[0] Adress 
xor  eax,eax                 ;2 Clear esi
mov  al, byte 00             ;3 save n(8-11)
mov  esi,eax                 ;2
add  ebp,esi                 ;2 Get R[n]
mov  eax,[ebp]               ;3 R[n]
mov  ecx,[ebx]               ;2 SR

test eax,0x80000000          ;5 
je   NOZERO                  ;2
or   dword [ebx],0x00000100  ;6 Set Q Flg
jmp  CONTINUE                ;3
NOZERO:
and  dword [ebx],0xFFFFFEFF  ;6 Clear Q Flg

CONTINUE:

; sh2i->R[n] |= (DWORD)(sh2i->i_get_T())
mov  eax,[ebx]               ;2    
and  eax,0x01                ;5
shl  dword [ebp], byte 1     ;3
or   [ebp],eax               ;3

;Get R[n],R[m]
mov  eax,[ebp]               ;3 R[n]
mov  ebp,edi                 ;2  
add  ebp,byte $00            ;3 4...7
mov  edx,[ebp]               ;3 R[m]

;switch( old_q )
test ecx,0x00000100          ;6 old_q == 1 ?
jne   Q_FLG_TMP              ;2

;----------------------------------------------------------
; 8 + 62 + 3 + 62 = 135
NQ_FLG: 

test ecx,0x00000200          ;6 M == 1 ?
jne  NQ_M_FLG                ;2

	;--------------------------------------------------
	; 62
	NQ_NM_FLG:  
	  mov ebp,edi           ;2
	  add ebp,esi           ;3
	  sub [ebp],edx         ;3 sh2i->R[n] -= sh2i->R[m]
    
	  test dword [ebx],0x00000100      ;6 Q == 1 
	  jne NQ_NM_Q_FLG                  ;2

	  NQ_NM_NQ_FLG:
	  cmp [ebp],eax        ;3 tmp1 = (sh2i->R[n]>tmp0);
	  jna NQ_NM_NQ_00_FLG  ;2

		  NQ_NM_NQ_01_FLG:
		  or   dword [ebx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1                 ;3

		  NQ_NM_NQ_00_FLG:
		  and  dword [ebx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1                 ;3  
  
	  NQ_NM_Q_FLG:
	  cmp [ebp],eax        ;3 tmp1 = (sh2i->R[n]>tmp0);
	  jna NQ_NM_NQ_10_FLG  ;2

		  NQ_NM_NQ_11_FLG:
		  and  dword [ebx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1                 ;3

		  NQ_NM_NQ_10_FLG:
		  or   dword [ebx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1                 ;3

Q_FLG_TMP:
jmp Q_FLG; 3

	;----------------------------------------------------  
	NQ_M_FLG:
	  mov ebp,edi           ;
	  add ebp,esi           ;

	  add  [ebp],edx        ; sh2i->R[n] += sh2i->R[m]  
	  test dword [ebx],0x00000100 ; Q == 1 
	  jne NQ_M_Q_FLG

	  NQ_M_NQ_FLG:
	  cmp [ebp],eax         ; tmp1 = (sh2i->R[n]<tmp0);
	  jnb NQ_M_NQ_00_FLG

		  NQ_M_NQ_01_FLG:
		  and  dword [ebx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1

		  NQ_M_NQ_00_FLG:
		  or   dword [ebx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1


	  NQ_M_Q_FLG:
	  cmp [ebp],eax                    ; tmp1 = (sh2i->R[n]<tmp0);
	  jnb NQ_M_NQ_10_FLG

		  NQ_M_NQ_11_FLG:
		  or   dword [ebx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1

		  NQ_M_NQ_10_FLG:
		  and  dword [ebx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1

;------------------------------------------------------
; 8 + 62 + 62 = 132
Q_FLG:   


test ecx,0x00000200 ; M == 1 ?
jne  Q_M_FLG

	;--------------------------------------------------
	Q_NM_FLG:
	  mov ebp,edi           ;
	  add ebp,esi           ;
	  add [ebp],edx         ; sh2i->R[n] += sh2i->R[m]
	  test dword [ebx],0x00000100 ; Q == 1 
	  jne Q_NM_Q_FLG

	  Q_NM_NQ_FLG:
	  cmp [ebp],eax      ; tmp1 = (sh2i->R[n]<tmp0);
	  ja Q_NM_NQ_00_FLG

		  Q_NM_NQ_01_FLG:
		  or   dword [ebx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1

		  Q_NM_NQ_00_FLG:
		  and  dword [ebx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1
  
	  Q_NM_Q_FLG:
	  cmp [ebp],eax      ; tmp1 = (sh2i->R[n]<tmp0);
	  ja Q_NM_NQ_10_FLG  

		  Q_NM_NQ_11_FLG:
		  and  dword [ebx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1

		  Q_NM_NQ_10_FLG:
		  or   dword [ebx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1

	;----------------------------------------------------  
	Q_M_FLG:
	  mov ebp,edi        ;
	  add ebp,esi        ;
	  sub [ebp],edx      ; sh2i->R[n] -= sh2i->R[m]  
	  test dword [ebx],0x00000100 ; Q == 1 
	  jne Q_M_Q_FLG

	  Q_M_NQ_FLG:
	  cmp [ebp],eax      ; tmp1 = (sh2i->R[n]>tmp0);
	  jb Q_M_NQ_00_FLG

		  Q_M_NQ_01_FLG:
		  and  dword [ebx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1

		  Q_M_NQ_00_FLG:
		  or   dword [ebx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1
 
	  Q_M_Q_FLG:
	  cmp [ebp],eax      ; tmp1 = (sh2i->R[n]>tmp0);
	  jb Q_M_NQ_10_FLG

		  Q_M_NQ_11_FLG:
		  or   dword [ebx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1

		  Q_M_NQ_10_FLG:
		  and  dword [ebx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1


;---------------------------------------------------
; size = 38
END_DIV1 

;sh2i->i_set_T( (sh2i->i_get_Q() == sh2i->i_get_M()) );

mov  eax,[ebx]                ;2 Get Q
shr  eax,8                    ;3
and  eax,1                    ;5 
mov  edx,[ebx]                ;2 Get M
shr  edx,9                    ;3    
and  edx,1                    ;6
and  dword [ebx], 0xFFFFFFFE  ;6 Set T Flg
cmp  eax,edx                  ;2
jne  NO_Q_M                   ;2
or   dword [ebx], 0x00000001  ;6 Set T Flg
NO_Q_M:
pop esi ;1
pop edx  ;1

;======================================================
; end of DIV1
;======================================================

;------------------------------------------------------------
;dmuls
opdesc DMULS, 27,6,14,0xFF,0xFF,0xFF
opfunc DMULS
mov  ecx,edx            ;2 Save PC
mov  ebp,edi            ;2  
add  ebp,byte $00       ;3 4..7
mov  eax,dword [ebp]    ;3
mov  ebp,edi            ;2 SetQ
add  ebp,byte $00       ;3 8..11
mov  edx,dword [ebp]    ;3  
imul edx                ;2
mov  dword [esi]  ,edx  ;2 store MACH             
mov  dword [esi+4],eax  ;3 store MACL   
mov  edx,ecx            ;2

;------------------------------------------------------------
;dmulu 32bit -> 64bit Mul
opdesc DMULU, 27,6,14,0xFF,0xFF,0xFF
opfunc DMULU
mov  ecx,edx            ;2 Save PC
mov  ebp,edi            ;2  
add  ebp,byte $00       ;3 4..7
mov  eax,dword [ebp]    ;3
mov  ebp,edi            ;2 SetQ
add  ebp,byte $00       ;3 8..11
mov  edx,dword [ebp]    ;3  
mul  edx                ;2
mov  dword [esi]  ,edx  ;2 store MACH             
mov  dword [esi+4],eax  ;3 store MACL   
mov  edx,ecx            ;2

;--------------------------------------------------------------
; mull 32bit -> 32bit Multip
opdesc MULL, 25,6,14,0xFF,0xFF,0xFF
opfunc MULL
mov  ecx,edx            ;2 Save PC
mov  ebp,edi            ;2  
add  ebp,byte $00       ;3 4..7
mov  eax,dword [ebp]    ;3
mov  ebp,edi            ;2
add  ebp,byte $00       ;3 8..11
mov  edx,dword [ebp]    ;3  
imul edx                ;2
mov  dword [esi+4],eax  ;3 store MACL   
mov  edx,ecx            ;2

;--------------------------------------------------------------
; muls 16bit -> 32 bit Multip
opdesc MULS, 38,6,17,0xFF,0xFF,0xFF
opfunc MULS
mov  ecx,edx           ;2 Save PC
mov  ebp,edi           ;2  
add  ebp,byte $00      ;3 4..7
xor  eax,eax           ;2
mov  ax,word [ebp]     ;3
mov  ebp,edi           ;2
add  ebp,byte $00      ;3 8..11
xor  edx,edx           ;2
mov  dx,word [ebp]     ;3  
imul dx                ;2
shl  edx, byte 16      ;3
add  dx, ax            ;2
mov  dword [esi+4],edx ;3 store MACL   
mov  edx,ecx            ;2

;--------------------------------------------------------------
; mulu 16bit -> 32 bit Multip
opdesc MULU, 38,6,17,0xFF,0xFF,0xFF
opfunc MULU
mov  ecx,edx           ;2 Save PC
mov  ebp,edi           ;2  
add  ebp,byte $00      ;3 4..7
xor  eax,eax           ;2
mov  ax,word [ebp]     ;3
mov  ebp,edi           ;2
add  ebp,byte $00      ;3 8..11
xor  edx,edx           ;2
mov  dx,word [ebp]     ;3  
mul  dx                ;2
shl  edx, byte 16      ;3
add  dx, ax            ;2
mov  dword [esi+4],edx ;3 store MACL   
mov  edx,ecx            ;2

;--------------------------------------------------------------
; MACL   ans = 32bit -> 64 bit MUL
;        (MACH << 32 + MACL)  + ans 
;-------------------------------------------------------------
opdesc MAC_L, 145,5,30,0xFF,0xFF,0xFF 
opfunc MAC_L
push edx                      ;1 Save PC
mov  ebp,edi                  ;2  
add  ebp,byte $00             ;3 4..7
mov  eax,dword [ebp]          ;3
push eax                      ;1
mov  eax,_memGetLong          ;5
call eax                      ;2
mov  edx,eax                  ;2
add  dword [ebp], 4           ;7 R[n] += 4
mov  ebp,edi                  ;2 
add  ebp,byte $00             ;3 8..11
mov  eax,dword [ebp]          ;3
push eax                      ;1
mov  eax,_memGetLong          ;5 
call eax                      ;2
add  dword [ebp], 4           ;7 R[m] += 4 
push edi                      ;1 Save GenReg
xor  ecx,ecx                  ;2
or   ecx,[esi+4]              ;3 load macl
imul edx                      ;2 eax <- low, edx <- high
mov  edi,[esi]                ;3 load mach
add  ecx,eax                  ;3 sum = a+b;
adc  edi,edx                  ;2
test dword [ebx], 0x00000002  ;6 check S flg
je   END_PROC                 ;2 if( S == 0 ) goto 'no sign proc'
cmp  edi,7FFFh                ;6
jb   END_PROC                 ;2 
ja   COMP_MIN                ;2
cmp  ecx,0FFFFFFFFh           ;3 
jbe   END_PROC                ;2  = 88

COMP_MIN:
cmp   edi,0FFFF8000h          ;6
ja   END_PROC                 ;2 
jb   CHECK_AAA                ;2 
test ecx,ecx                  ;2
jae  END_PROC                ;2
CHECK_AAA:
test edx,edx                  ;2
jg   MAXMIZE                  ;2
jl   MINIMIZE                 ;2
test eax,eax                  ;3
jae MAXMIZE                   ;2
MINIMIZE:
xor ecx,ecx                   ;2
mov edi,0FFFF8000h            ;5
jmp END_PROC                 ;2
MAXMIZE:
or   ecx,0FFFFFFFFh           ;3 sum = 0x00007FFFFFFFFFFFULL;
mov  edi,7FFFh                ;5
END_PROC:
mov         [esi],edi         ; 3
pop  edi                      ; 1 Restore GenReg
mov         [esi+4],ecx       ; 3
pop  eax                      ;1
pop  eax                      ;1
pop  edx                      ;1 = 42 



;--------------------------------------------------------------
; MACW   ans = 32bit -> 64 bit MUL
;        (MACH << 32 + MACL)  + ans 
;-------------------------------------------------------------
opdesc MAC_W, 120,5,31,0xFF,0xFF,0xFF
opfunc MAC_W
push edx                      ;1 Save PC
mov  ebp,edi                  ;2  
add  ebp,byte $00             ;3 4..7
mov  eax,dword [ebp]          ;3
push eax                      ;1
mov  eax,_memGetWord          ;5
call eax                      ;2
movsx  edx,ax                 ;3
add  dword [ebp], 2           ;7 R[n] += 2
mov  ebp,edi                  ;2 
add  ebp,byte $00             ;3 8..11
mov  eax,dword [ebp]          ;3
push eax                      ;1
mov  eax,_memGetWord          ;5 
call eax                      ;2
add  dword [ebp], 2           ;7 R[m] += 2
cwde                          ;1 Sigin extention
imul edx                      ;2 eax <- low, edx <- high
test dword [ebx], 0x00000002  ;6 check S flg
je   MACW_NO_S_FLG                 ;2 if( S == 0 ) goto 'no sign proc'

MACW_S_FLG:
  add dword [esi+4],eax   ;3 MACL = ansL + MACL
  jno NO_OVERFLO
  js  FU 
  SEI:
    mov dword [esi+4],0x80000000 ; min value
	or  dword [esi],1
	jmp END_MACW

  FU:
    mov dword [esi+4],0x7FFFFFFF ; max value
	or dword [esi],1
	jmp END_MACW

  NO_OVERFLO:
  jmp END_MACW

MACW_NO_S_FLG:
  add dword [esi+4],eax         ;3 MACL = ansL + MACL
  jnc MACW_NO_CARRY             ;2 Check Carry
  inc edx                       ;1
MACW_NO_CARRY: 
  add dword [esi],edx           ;2 MACH = ansH + MACH

END_MACW:
pop  eax                        ;1
pop  eax                        ;1
pop  edx                        ;1

  
 
end
