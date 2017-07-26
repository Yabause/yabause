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
; ESI  <- Address of SysReg
; EDI  <- Address of GenReg
; EIP
; ESP
; EBP
; EFL
;

bits 32

section .data

section .code

%macro opfunc 1
	global _x86_%1
	_x86_%1:
	global x86_%1
	x86_%1:
%endmacro

%macro opfuncend 1
	x86_%1_end:
	global %1_size
	%1_size dw (x86_%1_end - x86_%1)
	global _%1_size
	_%1_size dw %1_size
%endmacro

%macro opdesc 6
	x86_%1_end:
	global %1_size
	global _%1_size
	%1_size dw (x86_%1_end - x86_%1)
	_%1_size dw (x86_%1_end - x86_%1)
	global %1_src
	global _%1_src
	%1_src db %2
	_%1_src db %2
	global %1_dest
	global _%1_dest
	%1_dest db %3
	_%1_dest db %3
	global %1_off1
	global _%1_off1
	%1_off1 db %4
	_%1_off1 db %4
	global %1_imm
	global _%1_imm
	%1_imm db %5
	_%1_imm db %5
	global %1_off3
	global _%1_off3
	%1_off3 db %6
	_%1_off3 db %6
%endmacro

%define GEN_REG edi
%define CTRL_REG ebx
%define SYS_REG esi
%define PC esp+4

%define SCRATCH1 ebp

%macro GET_R 1
	mov %1, GEN_REG         
	add %1,byte 0x7F    
%endmacro

%macro GET_BYTE_IMM 1      
	or %1, 0x7F    
%endmacro

%macro GET_R_ID 2
	mov %2, %1
	shl %2, byte 2
	add %2, GEN_REG              
%endmacro

%macro GET_R0 1
	mov %1, GEN_REG              
%endmacro

%macro SET_MACH 1
 	mov dword [SYS_REG], %1
%endmacro

%macro SET_MACL 1
	mov dword [SYS_REG+4], %1
%endmacro

%macro GET_MACH 1
	mov %1, dword [SYS_REG]
%endmacro

%macro GET_MACL 1
	mov %1, dword [SYS_REG+4]
%endmacro

%macro GET_PR 1
	mov %1, dword [SYS_REG+8]
%endmacro

%macro SET_PR 1
	mov dword [SYS_REG+8], %1
%endmacro

%macro SET_VBR 1
	mov dword [CTRL_REG+8], %1
%endmacro

%macro GET_SR 1
	mov %1, dword [CTRL_REG]
%endmacro

%macro GET_GBR 1
	mov %1, dword [CTRL_REG+4]
%endmacro

%macro SET_GBR 1
	mov dword [CTRL_REG+4], %1
%endmacro

%macro GET_VBR 1
	mov %1, dword [CTRL_REG+8]
%endmacro

%macro CLEAR_T 0
	and dword [CTRL_REG], 0x3F2
%endmacro

%macro SET_T 0
	or dword [CTRL_REG], 0x1
%endmacro

%macro SET_T_R 1
	or dword [CTRL_REG], %1
%endmacro

%macro GET_T 1
	mov %1, dword [CTRL_REG]
	and %1, 0x1
%endmacro

%macro CLEAR_Q 0
	and  dword [CTRL_REG],0x2F3
%endmacro

%macro SET_Q 0
	or dword [CTRL_REG], 0x100
%endmacro

%macro GET_Q 1
	mov  %1,[CTRL_REG]
	shr  %1,8
	and  %1,1
%endmacro

%macro GET_M 1
	mov  %1,[CTRL_REG]
	shr  %1,9
	and  %1,1
%endmacro

%macro TEST_IS_Q 0
	bt dword [CTRL_REG],0x8
%endmacro

%macro CLEAR_M 0
	and  dword [CTRL_REG],0x1F3
%endmacro

%macro SET_M 0
	or  dword [CTRL_REG],0x200
%endmacro

%macro TEST_IS_M 0
	bt dword [CTRL_REG],0x9 ; M == 1 ?
%endmacro

%macro TEST_IS_S 0
	bt dword [CTRL_REG],0x1
%endmacro

%macro TEST_IS_T 0
	bt dword [CTRL_REG],0x0
%endmacro

%macro SET_SR 1
	and %1, 0x3F3
	mov dword [CTRL_REG], %1
%endmacro

;Memory Functions
;extern DebugEachClock, DebugDelayClock

%macro CALL_FUNC 1
  mov eax, [SYS_REG + 24 + %1*4]
  call eax  
%endmacro

%macro CALL_GETMEM_BYTE 0
  CALL_FUNC 0  
%endmacro

%macro CALL_GETMEM_WORD 0
  CALL_FUNC 1  
%endmacro

%macro CALL_GETMEM_LONG 0
  CALL_FUNC 2 
%endmacro

%macro CALL_SETMEM_BYTE 0
  CALL_FUNC 3 
%endmacro

%macro CALL_SETMEM_WORD 0
  CALL_FUNC 4 
%endmacro

%macro CALL_SETMEM_LONG 0
  CALL_FUNC 5 
%endmacro

%macro CALL_EACH_CLOCK 0
  CALL_FUNC 6 
%endmacro

%macro CALL_CHECK_INT 0
  CALL_FUNC 7 
%endmacro

%macro PUSHAD 0
        pushad           ;1
%endmacro

%macro POPAD 0
        popad
%endmacro

%macro START 1
	global %1
	global _%1
	%1:
	_%1:
%endmacro

%macro END 1
	%1_end:
	global %1_size
	global _%1_size
	%1_size dw (%1_end - %1)
	_%1_size dw (%1_end - %1)
%endmacro

;=====================================================
; Basic
;=====================================================

;-----------------------------------------------------
; Begining of block
; SR => [ebx]
; GenReg => edi
; SysReg => esi
; Size = 27 Bytes
START prologue
PUSHAD
mov GEN_REG,[esp+36]
mov CTRL_REG,GEN_REG
add CTRL_REG,byte 64
mov SYS_REG,CTRL_REG
add SYS_REG,byte 12  
mov eax, SYS_REG        ;2
add eax,byte 12
push dword [eax]           ;3 (PC)
push dword 00      ;4 (JumpAddr) 
END prologue

;-------------------------------------------------------
; normal part par instruction
;Size = 7 Bytes
START seperator_normal
add dword [PC], byte 2   ;3 PC += 2
add dword [SYS_REG+16], byte 1 ;4 Clock += 1
END  seperator_normal

START check_interrupt
CALL_CHECK_INT
pop eax         ;1
pop eax
mov [SYS_REG+12], eax   
POPAD
ret             ;1
END check_interrupt

;------------------------------------------------------
; Delay slot part par instruction
;Size = 17 Bytes
START seperator_delay_slot
test dword [esp], 0xFFFFFFFF ; 7
jnz   .continue               ; 2
add dword [PC], byte 2      ; 3 PC += 2
add dword [SYS_REG+16], byte 1    ; 4 Clock += 1
pop  eax 
pop eax
mov [SYS_REG+12], eax                    ; 1
POPAD
ret                          ; 1
.continue:
mov  eax, dword [esp]             ; 3
sub  eax,byte 2            ; 3
mov  dword [PC],eax             ; 2
END seperator_delay_slot


;------------------------------------------------------
; End part of delay slot
;Size = 24 Bytes
START seperator_delay_after
add dword [PC], byte 2   ;3 PC += 2
add dword [SYS_REG+16], byte 1 ;4 Clock += 1
pop  eax                  ; 1
pop eax
mov [SYS_REG+12], eax   
POPAD
ret                       ; 1
END seperator_delay_after

;-------------------------------------------------------
; End of block
; Size = 3 Bytes
START epilogue
pop eax         ;1
pop eax
mov [SYS_REG+12], eax   
POPAD
ret             ;1
END epilogue

;------------------------------------------------------
; Jump part
; Size = 17 Bytes
START PageFlip
test dword [esp], 0xFFFFFFFF ; 7
jz   .continue               ; 2
mov  eax,dword [esp]               ; 3
mov  dword [PC],eax               ; 2
pop  eax                     ; 1
pop eax
mov [SYS_REG+12], eax   
POPAD
ret                          ; 1
.continue: 
END PageFlip

;-------------------------------------------------------
; normal part par instruction( for debug build )
;Size = 24 Bytes
START seperator_d_normal
add dword [SYS_REG+16],byte 1 ;4 Clock += 1
;mov  eax,DebugEachClock ;5
;call eax                 ;2
test eax, 0x01           ;5 finish 
jz  NEXT_D_INST          ;2
pop eax                  ;1 
pop eax
mov [SYS_REG+12], eax   
POPAD
ret                      ;1
NEXT_D_INST:
add dword [PC],byte 2   ;3 PC += 2
END seperator_d_normal

;------------------------------------------------------
; Delay slot part par instruction( for debug build )
;Size = 34 Bytes
START seperator_d_delay
;mov  eax,DebugDelayClock ;5
;call eax                   ;2
test dword [esp], 0xFFFFFFFF ; 7
jnz   .continue               ; 2
add dword [PC], byte 2      ; 3 PC += 2
add dword [SYS_REG+16], byte 1    ; 4 Clock += 1
pop  eax                     ; 1
pop eax
mov [SYS_REG+12], eax   
POPAD
ret                          ; 1
.continue:
mov  eax,[esp]             ; 3
sub  eax,byte 2            ; 3
mov  [edx],eax             ; 2
END seperator_d_delay

;=================
;Begin x86 Opcodes
;=================

opfunc CLRT
CLEAR_T
opdesc CLRT,  0xFF,0xFF,0xFF,0xFF,0xFF

opfunc CLRMAC
and dword [SYS_REG], 0   ;4
and dword [SYS_REG+4], 0 ;4
opdesc CLRMAC,	0xFF,0xFF,0xFF,0xFF,0xFF

opfunc NOP
nop
opdesc NOP,		0xFF,0xFF,0xFF,0xFF,0xFF

opfunc DIV0U
and dword [CTRL_REG],0x0f2
opdesc DIV0U,	0xFF,0xFF,0xFF,0xFF,0xFF

opfunc SETT
SET_T
opdesc SETT,	0xFF,0xFF,0xFF,0xFF,0xFF

opfunc SLEEP
pop eax             ;1
pop eax
mov [SYS_REG+12], eax   
POPAD
ret                 ;1
opdesc SLEEP,	0xFF,0xFF,0xFF,0xFF,0xFF

opfunc SWAP_W
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
rol eax,16          ;2
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;2
opdesc SWAP_W,	4,15,0xff,0xff,0xff

opfunc SWAP_B
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
xchg ah,al          ;2
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;2
opdesc SWAP_B,	4,14,0xff,0xff,0xff

opfunc TST
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
GET_R SCRATCH1
CLEAR_T
test dword [SCRATCH1],eax      ;2
jne end_tst
SET_T
end_tst:
opdesc TST,	4,12,0xff,0xff,0xff

opfunc TSTI
GET_R0 SCRATCH1
mov eax,dword [SCRATCH1]       ;2
CLEAR_T
test al,$00        ;5  Imidiate Val
jne end_tsti
SET_T
end_tsti:
opdesc TSTI,	0xff,0xff,0xff,12,0xff


opfunc ANDI
GET_R0 SCRATCH1
xor eax, eax
GET_BYTE_IMM al
and dword [SCRATCH1],eax ;3
opdesc ANDI,	0xff,0xff,0xff,5,0xff

opfunc XORI
GET_R0 SCRATCH1
xor eax, eax
GET_BYTE_IMM al
xor dword [SCRATCH1],eax ;3
opdesc XORI,	0xff,0xff,0xff,5,0xff

opfunc ORI
GET_R0 SCRATCH1
xor eax,eax         ;2
GET_BYTE_IMM al
or dword [SCRATCH1],eax ;3
opdesc ORI,	0xff,0xff,0xff,5,0xff

opfunc CMP_EQ_IMM
mov eax, [GEN_REG]            ;2
CLEAR_T
cmp eax, byte 0x7F ;4
jne .continue
SET_T
.continue:
opdesc CMP_EQ_IMM,	0xff,0xff,0xff,10,0xff

opfunc XTRCT
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
GET_R SCRATCH1
shl eax,16          ;3
shr dword [SCRATCH1],16  ;3
or dword [SCRATCH1],eax        ;2
opdesc XTRCT,	4,12,0xff,0xff,0xff

opfunc ADD
GET_R SCRATCH1
mov dword eax,[SCRATCH1]
GET_R SCRATCH1
add dword [SCRATCH1],eax       ;2
opdesc ADD,		4,12,0xff,0xff,0xff

opfunc ADDC
GET_R SCRATCH1
mov dword eax,[SCRATCH1]
GET_R SCRATCH1
bts dword [CTRL_REG],0   ;4
adc [SCRATCH1],eax       ;3
jnc	ADDC_NO_CARRY   ;2
SET_T
jmp ADDC_END        ;2
ADDC_NO_CARRY:
CLEAR_T
ADDC_END:
opdesc ADDC,	4,12,0xff,0xff,0xff


; add with overflow check
opfunc ADDV
GET_R SCRATCH1
mov dword eax,[SCRATCH1]
GET_R SCRATCH1
CLEAR_T
add dword [SCRATCH1],eax
jno	 NO_OVER_FLO      ;2
SET_T
NO_OVER_FLO:
opdesc ADDV, 4,12,0xff,0xff,0xff

opfunc SUBC
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;3
GET_R SCRATCH1
bts dword [CTRL_REG],0
sbb dword [SCRATCH1],eax       ;3
jnc	non_carry       ;2
SET_T
jmp SUBC_END        ;2
non_carry:
CLEAR_T
SUBC_END:
opdesc SUBC,	4,12,0xff,0xff,0xff



opfunc SUB
GET_R SCRATCH1
mov dword eax,[SCRATCH1]
GET_R SCRATCH1
sub dword [SCRATCH1],eax       ;2
opdesc SUB,		4,12,0xff,0xff,0xff

opfunc NOT
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
not eax             ;2
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;2
opdesc NOT,		4,14,0xff,0xff,0xff

opfunc NEG
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
neg eax             ;2
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;2
opdesc NEG,		4,14,0xff,0xff,0xff

opfunc NEGC
GET_R ebp
mov ecx,[ebp]             ;3
neg ecx                   ;2
GET_R SCRATCH1
mov dword [SCRATCH1],ecx             ;3
GET_SR eax
and dword eax,1           ;5
sub dword [SCRATCH1],eax             ;3
CLEAR_T
cmp ecx,0                 ;5
jna NEGC_NOT_LESS_ZERO    ;2
SET_T
NEGC_NOT_LESS_ZERO:
cmp dword [SCRATCH1],ecx             ;3  
jna NEGC_NOT_LESS_OLD     ;2
SET_T
NEGC_NOT_LESS_OLD:
opdesc NEGC,	4,14,0xff,0xff,0xff

opfunc EXTUB
GET_R SCRATCH1
mov eax, dword [SCRATCH1]  
and dword eax,0x000000ff  ;5
GET_R SCRATCH1
mov dword [SCRATCH1],eax
opdesc EXTUB,	4,17,0xff,0xff,0xff

opfunc EXTU_W
GET_R SCRATCH1
mov eax, dword [SCRATCH1]   
and dword eax,0xffff;5
GET_R SCRATCH1
mov dword [SCRATCH1],eax
opdesc EXTU_W,	4,17,0xff,0xff,0xff

opfunc EXTS_B
GET_R SCRATCH1
mov eax, dword [SCRATCH1]
cbw                 ;2
cwde                ;1
GET_R SCRATCH1
mov dword [SCRATCH1],eax
opdesc EXTS_B,	4,15,0xff,0xff,0xff

opfunc EXTS_W
GET_R SCRATCH1
mov eax, dword [SCRATCH1]    
cwde                ;2
GET_R SCRATCH1
mov dword [SCRATCH1],eax
opdesc EXTS_W,	4,13,0xff,0xff,0xff

;Store Register Opcodes
;----------------------

opfunc STC_SR_MEM
GET_R SCRATCH1
sub  dword [SCRATCH1],byte 4 ;4
push dword [CTRL_REG]
push dword [SCRATCH1]
CALL_SETMEM_LONG
pop  eax                ;1
pop  eax                ;1
opdesc STC_SR_MEM,	0xff,4,0xff,0xff,0xff

opfunc STC_GBR_MEM
GET_R SCRATCH1
sub  dword [SCRATCH1],byte 4 ;4
GET_GBR eax
push eax                ;1
push dword [SCRATCH1]        ;3
CALL_SETMEM_LONG
pop  eax                ;1
pop  eax                ;1
opdesc STC_GBR_MEM,	0xff,4,0xff,0xff,0xff

opfunc STC_VBR_MEM
GET_R SCRATCH1
sub  dword [SCRATCH1],byte 4 ;4
GET_VBR eax
push eax                ;1
push dword [SCRATCH1]        ;3
CALL_SETMEM_LONG
pop  eax                ;1
pop  eax                ;1
opdesc STC_VBR_MEM,	0xff,4,0xff,0xff,0xff

;------------------------------

opfunc MOVBL
GET_R SCRATCH1
push dword [SCRATCH1] 
CALL_GETMEM_BYTE
GET_R SCRATCH1
cbw                 ;1
cwde                ;1
mov dword [SCRATCH1],eax       ;3
pop eax             ;1
opdesc MOVBL,	4,17,0xff,0xff,0xff

opfunc MOVWL
GET_R SCRATCH1
push dword [SCRATCH1]       ;3
CALL_GETMEM_WORD
GET_R SCRATCH1
cwde                ;1
mov dword [SCRATCH1],eax       ;3
pop eax             ;1
opdesc MOVWL,		4,17,0xff,0xff,0xff

opfunc MOVL_MEM_REG
GET_R SCRATCH1
push dword [SCRATCH1]       ;3
CALL_GETMEM_LONG
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;3
pop eax             ;1
opdesc MOVL_MEM_REG,		4,17,0xff,0xff,0xff

opfunc MOVBP
GET_R SCRATCH1
mov eax, [SCRATCH1]       ;3
push eax
CALL_GETMEM_BYTE
cbw                 ;1
cwde                ;1
GET_R edx
cmp SCRATCH1,edx
je continue_movbp
inc dword [SCRATCH1]     ;3
continue_movbp:
mov dword [edx],eax       ;3
pop eax             ;1
opdesc MOVBP,	4,21,0xff,0xff,0xff


opfunc MOVWP
GET_R SCRATCH1
push dword [SCRATCH1]       ;3
CALL_GETMEM_WORD
cwde                ;1
GET_R edx
cmp SCRATCH1,edx
je continue_movwp
add dword [SCRATCH1],byte 2
continue_movwp:
mov dword [edx],eax       ;3
pop eax
opdesc MOVWP,	4,18,0xff,0xff,0xff

opfunc MOVLP
GET_R SCRATCH1
push dword [SCRATCH1]       ;3
CALL_GETMEM_LONG
GET_R edx
mov dword [edx],eax       ;3
cmp SCRATCH1,edx
je end_movlp
add dword [SCRATCH1],byte 4
end_movlp:
pop eax
opdesc MOVLP,	4,17,0xff,0xff,0xff

opfunc MOVI
GET_R SCRATCH1
xor eax,eax         ;2
or eax,byte 00      ;3
mov dword [SCRATCH1],eax       ;3
opdesc MOVI,	0xff,4,0xff,9,0xff

;----------------------

opfunc MOVBL0
GET_R SCRATCH1
mov eax,[SCRATCH1]        ;3
add eax,[GEN_REG]        ;2
push eax             ;1
CALL_GETMEM_BYTE
GET_R SCRATCH1
cbw                  ;1
cwde                 ;1
mov dword [SCRATCH1],eax        ;3
pop  eax             ;1
opdesc MOVBL0,	4,20,0xff,0xff,0xff

opfunc MOVWL0
GET_R SCRATCH1
mov eax,dword [SCRATCH1]        ;3
add eax,dword [GEN_REG]        ;2
push eax             ;1
CALL_GETMEM_WORD
GET_R SCRATCH1
cwde                 ;1
mov dword [SCRATCH1],eax        ;3
pop  eax             ;1
opdesc MOVWL0,	4,20,0xff,0xff,0xff

opfunc MOVLL0
GET_R SCRATCH1
mov eax,dword [SCRATCH1]        ;3
add eax,dword [GEN_REG]        ;2
push eax             ;1
CALL_GETMEM_LONG
GET_R SCRATCH1
mov dword [SCRATCH1],eax        ;3
pop  eax             ;1
opdesc MOVLL0,	4,20,0xff,0xff,0xff

opfunc MOVT
GET_R SCRATCH1
GET_T eax
mov dword [SCRATCH1],eax       ;3
opdesc MOVT,		0xff,4,0xff,0xff,0xff

opfunc MOVBS0
GET_R SCRATCH1
push dword [SCRATCH1]     ;3
GET_R SCRATCH1
mov  eax,dword [SCRATCH1]       ;3
add  eax,dword [GEN_REG]       ;2
push eax             ;1
CALL_SETMEM_BYTE
pop  eax             ;1
pop  eax             ;1
opdesc MOVBS0,	4,12,0xff,0xff,0xff

opfunc MOVWS0
GET_R SCRATCH1
push dword [SCRATCH1]     ;3
GET_R SCRATCH1
mov  eax,dword [SCRATCH1]       ;3
add  eax,[GEN_REG]       ;2
push eax             ;1
CALL_SETMEM_WORD
pop  eax             ;1
pop  eax             ;1
opdesc MOVWS0,	4,12,0xff,0xff,0xff

opfunc MOVLS0
GET_R SCRATCH1
push dword [SCRATCH1]     ;3
GET_R SCRATCH1
mov  eax,dword [SCRATCH1]       ;3
add  eax,[GEN_REG]       ;2
push eax             ;1
CALL_SETMEM_LONG
pop  eax             ;1
pop  eax             ;1
opdesc MOVLS0,	4,12,0xff,0xff,0xff

;===========================================================================
;Verified Opcodes
;===========================================================================

opfunc DT
GET_R SCRATCH1
CLEAR_T
dec dword [SCRATCH1]     ;3
cmp dword [SCRATCH1],byte 0 ;4
jne .continue       ;2
SET_T
.continue:
opdesc DT,		0xff,4,0xff,0xff,0xff

opfunc CMP_PZ
GET_R SCRATCH1
CLEAR_T
cmp dword [SCRATCH1],byte 0    ;4
jl .continue              ;2
SET_T
.continue:
opdesc CMP_PZ,	0xff,4,0xff,0xff,0xff

opfunc CMP_PL
GET_R SCRATCH1
CLEAR_T
cmp dword [SCRATCH1], 0          ;7
jle .continue               ;2
SET_T
.continue:
opdesc CMP_PL,	0xff,4,0xff,0xff,0xff

opfunc CMP_EQ
GET_R SCRATCH1
mov eax,[SCRATCH1]       ;2
GET_R SCRATCH1
CLEAR_T
cmp [SCRATCH1],eax       ;3
jne .continue       ;2  
SET_T
.continue:
opdesc CMP_EQ,	4,12,0xff,0xff,0xff

opfunc CMP_HS
GET_R SCRATCH1
mov eax,[SCRATCH1]       ;3
GET_R SCRATCH1
CLEAR_T
cmp [SCRATCH1],eax       ;3
jb .continue        ;2
SET_T
.continue:
opdesc CMP_HS,	4,12,0xff,0xff,0xff

opfunc CMP_HI
GET_R SCRATCH1
mov eax,[SCRATCH1]       ;2
GET_R SCRATCH1
CLEAR_T
cmp [SCRATCH1],eax       ;3
jbe .continue       ;2
SET_T 
.continue:
opdesc CMP_HI,	4,12,0xff,0xff,0xff

opfunc CMP_GE
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
GET_R SCRATCH1
CLEAR_T
cmp dword [SCRATCH1],eax       ;3
jl .continue        ;2
SET_T 
.continue:
opdesc CMP_GE,	4,12,0xff,0xff,0xff

opfunc CMP_GT
GET_R SCRATCH1
mov eax,[SCRATCH1]       ;2
GET_R SCRATCH1
CLEAR_T
cmp [SCRATCH1],eax       ;3
jle .continue       ;2
SET_T
.continue:
opdesc CMP_GT,	4,12,0xff,0xff,0xff

opfunc ROTR
GET_R ebp
mov eax,[ebp]       ;2
and eax,byte 1      ;3
CLEAR_T
SET_T_R eax
shr dword [ebp],1   ;2
shl eax,byte 31     ;3
or dword [ebp],eax        ;2
opdesc ROTR,	0xff,4,0xff,0xff,0xff

opfunc ROTCR
GET_R ebp
mov eax,[ebx]       ;2
and dword eax,1     ;5
shl eax,byte 31     ;3
mov ecx,[ebp]       ;2
and dword ecx,1     ;5
CLEAR_T
or [ebx],ecx        ;2
shr dword [ebp],byte 1 ;4
or [ebp],eax        ;2
opdesc ROTCR,	0xff,4,0xff,0xff,0xff

opfunc ROTL
GET_R ebp
mov eax,[ebp]             ;2
shr eax,byte 31           ;3
CLEAR_T
SET_T_R eax
shl dword [ebp],byte 1    ;4
or [ebp],eax              ;2
opdesc ROTL,	0xff,4,0xff,0xff,0xff

opfunc ROTCL
GET_R ebp
mov eax,[ebp]       ;2
shl dword [ebp],byte 1     ;3
shr eax,byte 31     ;3
and eax,byte 1     ;3
TEST_IS_T
jnc continue_rotcl
or dword [ebp],byte 1
continue_rotcl:
CLEAR_T
SET_T_R eax
opdesc ROTCL,	0xff,4,0xff,0xff,0xff

opfunc SHL
GET_R ebp
mov eax,[ebp]       ;3
shr eax,byte 31     ;3
CLEAR_T
or [ebx],eax        ;2
shl dword [ebp],byte 1 ;4
opdesc SHL,		0xff,4,0xff,0xff,0xff

opfunc SHLR
GET_R ebp
mov eax,[ebp]               ;3
and dword eax,1             ;5
CLEAR_T
or [ebx],eax                ;2
shr dword [ebp],byte 1      ;4
opdesc SHLR,	0xff,4,0xff,0xff,0xff

opfunc SHAR
GET_R ebp
mov eax,[ebp]       ;2
and dword eax,1     ;5
CLEAR_T
or  [ebx],eax          ;2
sar dword [ebp],byte 1 ;4
opdesc SHAR,	0xff,4,0xff,0xff,0xff


opfunc SHLL2
GET_R ebp
shl dword [ebp],byte 2 ;4
opdesc SHLL2,	0xff,4,0xff,0xff,0xff

opfunc SHLR2
GET_R ebp
shr dword [ebp],byte 2 ;4
opdesc SHLR2,	0xff,4,0xff,0xff,0xff

opfunc SHLL8
GET_R ebp
shl dword[ebp],byte 8  ;4
opdesc SHLL8,	0xff,4,0xff,0xff,0xff

opfunc SHLR8
GET_R ebp
shr dword [ebp],byte 8 ;4
opdesc SHLR8,	0xff,4,0xff,0xff,0xff


opfunc SHLL16
GET_R ebp
shl dword [ebp],byte 16 ;4
opdesc SHLL16,	0xff,4,0xff,0xff,0xff

opfunc SHLR16
GET_R ebp
shr dword [ebp],byte 16 ;4
opdesc SHLR16,	0xff,4,0xff,0xff,0xff

opfunc AND
GET_R ebp
mov eax,[ebp]       ;2
GET_R ebp
and [ebp],eax       ;2
opdesc AND,		4,12,0xff,0xff,0xff

opfunc OR
GET_R ebp
mov eax,[ebp]       ;2
GET_R ebp
or [ebp],eax        ;2
opdesc OR,		4,12,0xff,0xff,0xff

opfunc XOR
GET_R ebp
mov eax,[ebp]       ;2
GET_R ebp
xor [ebp],eax       ;3
opdesc XOR,		4,12,0xff,0xff,0xff

opfunc ADDI
GET_R ebp
add dword [ebp],byte $00 ;4
opdesc ADDI,	0xff,4,0xff,8,0xff

opfunc AND_B
GET_R0 ebp
GET_GBR edx
add edx, dword [ebp]
push edx
CALL_GETMEM_BYTE
and al,byte $7F      ;2
pop  edx              ;1
push eax              ;1 data
push edx              ;1 addr
CALL_SETMEM_BYTE
pop  eax              ;1
pop  eax              ;1
opdesc AND_B,	0xff,0xff,0xff,15,0xff

opfunc OR_B
GET_R0 ebp
GET_GBR edx
add edx, dword [ebp]
push edx
CALL_GETMEM_BYTE
or   al,byte $00      ;2
pop  edx              ;1
push eax              ;1 data
push edx              ;1 addr
CALL_SETMEM_BYTE
pop  eax              ;1
pop  eax              ;1
opdesc OR_B,	0xff,0xff,0xff,15,0xff

opfunc XOR_B
GET_R0 ebp
GET_GBR edx
add edx, dword [ebp]
push edx
CALL_GETMEM_BYTE
xor al,byte $7F      ;2
pop  edx              ;1
push eax              ;1 data
push edx              ;1 addr
CALL_SETMEM_BYTE
pop  eax              ;1
pop  eax              ;1
opdesc XOR_B,	0xff,0xff,0xff,15,0xff

opfunc TST_B
CLEAR_T
GET_GBR eax
add  eax, dword [GEN_REG] ;2 Add R[0]
push eax              ;1
CALL_GETMEM_BYTE
test al, $00
jne .continue         ;2
SET_T
.continue:
pop eax               ;1
opdesc TST_B,	0xff,0xff,0xff,18,0xff

;Jump Opcodes
;------------

opfunc JMP
GET_R ebp
mov eax,[ebp]       ;2
mov [esp],eax       ;3
opdesc JMP,		0xff,4,0xff,0xff,0xff

opfunc JSR
mov eax, dword [PC]       ;2
add eax,byte 4      ;3
SET_PR eax
GET_R ebp
mov eax,dword [ebp]       ;3
mov dword [esp],eax       ;3
opdesc JSR,		0xff,14,0xff,0xff,0xff

opfunc BRA
mov ax, 0x0ABF     ;3
bt ax, 11        ;4
jnc .continue       ;2
or ax,0xf000   ;5
.continue:
cwde
shl eax, 1
add eax,byte 4      ;3
add eax,dword [PC] ;2
mov dword [esp],eax       ;3
opdesc BRA,		0xff,0xff,0xff,0xff,2

opfunc BSR
mov eax,dword [PC]       ;2
add eax,byte 4      ;3
SET_PR eax
xor edx, edx
or dx,word 0x0ABE   ;3
bt edx, 11        ;4
jnc .continue       ;2
or edx,0xfffff000   ;5
.continue:
shl edx,byte 1      ;3
add edx,eax ;2
mov dword [esp],edx  
opdesc BSR,		0xff,0xff,0xff,0xff,15

opfunc BSRF
GET_R ebp
mov eax,dword [PC]       ;2
add eax,byte 4      ;3
SET_PR eax
mov eax,dword [PC]       ;3
add eax,dword [ebp] ;3
add eax,byte 4      ;3
mov dword [esp],eax       ;3
opdesc BSRF,		0xff,4,0xff,0xff,0xff

opfunc BRAF
GET_R ebp
mov eax,dword [PC]       ;2
add eax,dword [ebp] ;3
add eax,byte 4      ;4
mov dword [esp],eax       ;3
opdesc BRAF,		0xff,4,0xff,0xff,0xff


opfunc RTS
GET_PR eax
mov [esp],eax       ;3
opdesc RTS,			0xFF,0xFF,0xFF,0xFF,0xFF

opfunc RTE
GET_R_ID 15,ebp
push dword [ebp]        ;3
CALL_GETMEM_LONG
mov  dword [esp+4],eax  ;4  Get PC
add  dword [ebp],byte 4 ;4
push dword [ebp]        ;3
CALL_GETMEM_LONG
and  eax,0x000003f3     ;5  Get SR
mov  [ebx],eax          ;2
add  dword [ebp],byte 4 ;4
pop  eax                ;1
pop  eax                ;1
opdesc RTE,			0xFF,0xFF,0xFF,0xFF,0xFF

opfunc TRAPA
GET_R_ID 15,ebp
sub  dword [ebp],4    ;7
push dword [CTRL_REG]      ;2 SR
push dword [ebp]      ;3
CALL_SETMEM_LONG
sub  dword [ebp],4    ;7
mov  eax,dword [PC+8]        ;3 PC
add  eax,byte 2       ;3
push eax              ;1
push dword [ebp]      ;2
CALL_SETMEM_LONG
xor  eax,eax          ;2
GET_BYTE_IMM al
shl  eax,2            ;3
add  eax,[CTRL_REG+8]      ;3 ADD VBR
push eax              ;1
CALL_GETMEM_LONG       ;3
pop  edx              ;1
pop  edx              ;1
pop  edx              ;1
pop  edx              ;1
pop  edx              ;1
mov  dword [esp],eax 
opdesc TRAPA,	      0xFF,0xFF,0xFF,47,0xFF

opfunc BT
xor eax,eax     ;3
TEST_IS_T
jnc .continue        ;2
GET_BYTE_IMM al
cbw
cwde
shl eax,byte 1      ;3
add eax,byte 4      ;3
add eax,dword [PC] ;2
mov dword [esp],eax       ;3
.continue:
opdesc BT,		0xFF,0xFF,0xFF,9,0xFF

opfunc BT_S
xor eax,eax     ;3
TEST_IS_T
jnc .continue        ;2
GET_BYTE_IMM al
cbw
cwde
shl eax, byte 1      ;3
.continue:
add eax, byte 4      ;3
add eax, dword [PC] ;2
mov dword [esp], eax
opdesc BT_S,		0xFF,0xFF,0xFF,9,0xFF

opfunc BF
xor eax,eax     ;3
TEST_IS_T
jc .continue        ;2
GET_BYTE_IMM al
cbw
cwde
shl eax,byte 1      ;3
add eax,byte 4      ;3
add eax,dword [PC] ;2
mov dword [esp],eax       ;3
.continue:
opdesc BF,		0xFF,0xFF,0xFF,9,0xFF

opfunc BF_S
xor eax,eax     ;3
TEST_IS_T
jc .continue        ;2
GET_BYTE_IMM al
cbw
cwde
shl eax, byte 1      ;3
.continue:
add eax, byte 4      ;3
add eax, dword [PC] ;2
mov dword [esp], eax
opdesc BF_S,		0xFF,0xFF,0xFF,9,0xFF

;Store/Load Opcodes
;------------------

opfunc STC_SR
GET_R ebp
mov eax,[ebx]
mov [ebp],eax
opdesc STC_SR,	0xFF,4,0xFF,0xFF,0xFF

opfunc STC_GBR
GET_R ebp
GET_GBR eax
mov dword [ebp],eax
opdesc STC_GBR,	0xFF,4,0xFF,0xFF,0xFF

opfunc STC_VBR
GET_R ebp
GET_VBR eax
mov dword [ebp],eax
opdesc STC_VBR,	0xFF,4,0xFF,0xFF,0xFF

opfunc STS_MACH
GET_R ebp
GET_MACH eax     ;2
mov [ebp],eax     ;2
opdesc STS_MACH, 0xFF,4,0xFF,0xFF,0xFF

opfunc STS_MACH_DEC
GET_R ebp
sub dword [ebp],byte 4 ;3
GET_MACH eax 
push eax          ;1
mov eax,[ebp]     ;2
push eax          ;1
CALL_SETMEM_LONG
pop eax ;1
pop eax ;1
opdesc STS_MACH_DEC,	0xFF,4,0xFF,0xFF,0xFF

opfunc STS_MACL
GET_R ebp
GET_MACL eax     ;2
mov [ebp],eax     ;2
opdesc STS_MACL, 0xFF,4,0xFF,0xFF,0xFF

opfunc STS_MACL_DEC
GET_R ebp
sub dword [ebp],byte 4 ;3
GET_MACL eax
push eax          ;1
mov eax,[ebp]     ;2
push eax          ;1
CALL_SETMEM_LONG
pop eax ;1
pop eax ;1
opdesc STS_MACL_DEC,	0xFF,4,0xFF,0xFF,0xFF

opfunc LDC_SR
GET_R ebp
mov eax,dword [ebp]       ;2
SET_SR eax
opdesc LDC_SR,	0xFF,4,0xFF,0xFF,0xFF

opfunc LDC_SR_INC
GET_R ebp
push dword [ebp]       ;2
CALL_GETMEM_LONG
SET_SR eax ;3
add dword [ebp],byte 4 ;3
pop eax             ;1
opdesc LDC_SR_INC,	0xFF,4,0xFF,0xFF,0xFF

opfunc LDCGBR
GET_R ebp
mov eax,dword [ebp]       ;3
SET_GBR eax
opdesc LDCGBR,	0xff,4,0xFF,0xFF,0xFF

opfunc LDC_GBR_INC
GET_R SCRATCH1
mov eax,dword [SCRATCH1]          ;2
add dword [SCRATCH1],byte 4 ;3
push eax                ;1
CALL_GETMEM_LONG
SET_GBR eax ;3
pop eax                ;1
opdesc LDC_GBR_INC,	0xFF,4,0xFF,0xFF,0xFF

opfunc LDC_VBR
GET_R ebp
mov eax,[ebp]       ;2
SET_VBR eax
opdesc LDC_VBR,	0xFF,4,0xFF,0xFF,0xFF

opfunc LDC_VBR_INC
GET_R ebp
mov  eax,dword [ebp]          ;2
push eax                ;1
CALL_GETMEM_LONG
SET_VBR eax ;3
add  dword [ebp],byte 4 ;3
pop  eax                ;1
opdesc LDC_VBR_INC,	0xFF,4,0xFF,0xFF,0xFF

opfunc STS_PR
GET_R ebp
GET_PR eax     ;2
mov dword [ebp],eax       ;3
opdesc STS_PR,		0xFF,4,0xFF,0xFF,0xFF

opfunc STSMPR
GET_R ebp
sub dword [ebp],byte 4 ;3
GET_PR eax     ;2
push eax          ;1
push dword [ebp]
CALL_SETMEM_LONG
pop eax           ;1
pop eax           ;1
opdesc STSMPR,	0xFF,4,0xFF,0xFF,0xFF

opfunc LDS_PR
GET_R ebp
mov eax,[ebp]       ;2
SET_PR eax
opdesc LDS_PR,		0xFF,4,0xFF,0xFF,0xFF

opfunc LDS_PR_INC
GET_R ebp
push dword [ebp]            ;1
CALL_GETMEM_LONG
SET_PR eax
add dword [ebp],byte 4 ;4
pop eax             ;1
opdesc LDS_PR_INC,	0xFF,4,0xFF,0xFF,0xFF

opfunc LDS_MACH
GET_R ebp
mov eax,[ebp]       ;2
SET_MACH eax
opdesc LDS_MACH,		0xFF,4,0xFF,0xFF,0xFF

opfunc LDS_MACH_INC
GET_R ebp
mov eax,[ebp]          ;2
push eax               ;1
CALL_GETMEM_LONG
SET_MACH eax
add dword [ebp],byte 4 ;3
pop eax                ;1
opdesc LDS_MACH_INC,	0xFF,4,0xFF,0xFF,0xFF

opfunc LDS_MACL
GET_R ebp
mov eax,[ebp]       ;2
SET_MACL eax
opdesc LDS_MACL,		0xFF,4,0xFF,0xFF,0xFF

opfunc LDS_MACL_INC
GET_R ebp
mov eax,[ebp]          ;2
push eax               ;1
CALL_GETMEM_LONG
SET_MACL eax
add dword [ebp],byte 4 ;3
pop eax                ;1
opdesc LDS_MACL_INC,	0xFF,4,0xFF,0xFF,0xFF

;Mov Opcodes
;-----------

opfunc MOVA
GET_R0 ebp
mov eax,[PC]       ;2
and eax, 0xfffffffc   ;3
add eax,byte 4      ;3
push eax            ;1
xor eax,eax         ;2
GET_BYTE_IMM al
shl eax,byte 2      ;3
add eax,dword [esp] ;2
mov dword [ebp],eax       ;2
pop eax             ;1
opdesc MOVA,	0xFF,0xFF,0xFF,16,0xFF

opfunc MOVWI
GET_R ebp
xor eax,eax         ;2
GET_BYTE_IMM al
shl eax,byte 1       ;1
add eax,[PC]       ;2 
add eax,byte 4      ;3
push eax            ;1
CALL_GETMEM_WORD
cwde                ;1
mov dword [ebp],eax       ;3
pop eax             ;1
opdesc MOVWI,	0xFF,4,0xFF,8,0xFF

opfunc MOVLI
GET_R ebp
xor eax,eax         ;2
GET_BYTE_IMM al
shl eax,byte 2       ;3
mov edx,dword [PC]       ;23
and edx,0xFFFFFFFC  ;6
add edx,byte 4      ;
add eax,edx         ;2 
push eax            ;1
CALL_GETMEM_LONG
mov dword [ebp],eax       ;3
pop eax             ;1
opdesc MOVLI,       0xFF,4,0xFF,8,0xFF

opfunc MOVBL4
GET_R ebp
xor  eax,eax          ;2  Clear Eax
mov  al,$00           ;2  Get Disp
add  eax,[ebp]        ;3
push eax              ;1  Set Func
CALL_GETMEM_BYTE
cbw                   ;1  Sign extension byte -> word
cwde                  ;1  Sign extension word -> dword
mov  [GEN_REG],eax        ;3
pop  eax              ;1
opdesc MOVBL4, 4,0xFF,8,0xFF,0xFF

opfunc MOVWL4
GET_R ebp
xor  eax,eax          ;2  Clear Eax
mov  al,$00           ;2  Get Disp
shl  ax, byte 1       ;3  << 1
add  eax,[ebp]        ;2
push eax              ;1  Set Func
CALL_GETMEM_WORD
GET_R0 ebp
cwde                  ;2  sign 
mov  [ebp],eax        ;3
pop  eax              ;1
opdesc MOVWL4, 4,0xFF,8,0xFF,0xFF

opfunc MOVLL4
GET_R ebp
xor  eax,eax          ;2  Clear Eax
GET_BYTE_IMM al
shl  ax, byte 2       ;3  << 2
add  eax,dword [ebp]        ;2
push eax              ;1  Set Func
CALL_GETMEM_LONG
GET_R ebp
mov dword [ebp],eax        ;3
pop  eax              ;1
opdesc MOVLL4, 4,26,8,0xFF,0xFF
 
opfunc MOVBS4
GET_R0 ebp
push dword [ebp]    ;3  Set to Func
add ebp,byte $00    ;3  Get Imidiate value R[n]
and eax,00000000h   ;5  clear eax
or  eax,byte $00    ;3  Get Disp value
add eax,dword [ebp]       ;3  Add Disp value
push eax            ;1  Set to Func
CALL_SETMEM_BYTE
pop eax             ;1
pop eax             ;1
opdesc MOVBS4,	7,0xFF,13,0xFF,0xFF

opfunc MOVWS4
mov ebp,edi         ;2  Get R0 address
push dword [ebp]    ;3  Set to Func
add ebp,byte $00    ;3  Get Imidiate value R[n]
and eax,00000000h   ;5  clear eax
or  eax,byte $00    ;3  Get Disp value
shl eax,byte 1      ;3  Shift Left
add eax,[ebp]       ;3  Add Disp value
push eax            ;1  Set to Func
CALL_SETMEM_WORD
pop eax             ;1
pop eax             ;1
opdesc MOVWS4,	7,0xFF,13,0xFF,0xFF

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
CALL_SETMEM_LONG
pop  eax             ;1
pop  eax             ;1
opdesc MOVLS4,	4,12,20,0xFF,0xFF
 
opfunc MOVBLG
mov  ebp,edi           ;2  Get R0 adress
xor  eax,eax           ;2  clear eax
mov  al,00             ;2  Get Imidiate Value
add  eax,dword [ebp+68];3  GBR + IMM( Adress for Get Value )
push eax               ;1
CALL_GETMEM_BYTE
cbw                    ;1
cwde                   ;1
mov  [ebp],eax         ;3
pop  eax               ;1
opdesc MOVBLG,    0xFF,0xFF,0xFF,5,0xFF

opfunc MOVWLG
mov  ebp,edi           ;2  Get R0 adress
xor  eax,eax           ;2  clear eax
mov  al,00             ;2  Get Imidiate Value
shl  ax,byte 1         ;3  Shift left 2
add  eax,dword [ebp+68];3  GBR + IMM( Adress for Get Value )
push eax               ;1
CALL_GETMEM_WORD
cwde                   ;1
mov  [ebp],eax         ;3
pop  eax               ;1
opdesc MOVWLG,    0xFF,0xFF,0xFF,5,0xFF

opfunc MOVLLG
mov  ebp,edi           ;2  Get R0 adress
xor  eax,eax           ;2  clear eax
mov  al,00             ;5  Get Imidiate Value
shl  ax,byte 2         ;3  Shift left 2
add  eax,dword [ebp+68];3  GBR + IMM( Adress for Get Value )
push eax               ;1
CALL_GETMEM_LONG
mov  [ebp],eax         ;3
pop  eax               ;1
opdesc MOVLLG,    0xFF,0xFF,0xFF,5,0xFF

opfunc MOVBSG
mov  ebp,edi         ;2  Get R0 adress
push dword [ebp]     ;3
xor  eax,eax         ;2  Clear Eax
mov  al,00           ;5  Get Imidiate Value
mov  ecx,edi         ;2  Get GBR adress
add  ecx,byte 68     ;3 
add  eax,dword [ecx] ;2  GBR + IMM( Adress for Get Value )
push eax             ;1
CALL_SETMEM_BYTE
pop  eax             ;1
pop  eax             ;1
opdesc MOVBSG,    0xFF,0xFF,0xFF,8,0xFF

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
CALL_SETMEM_WORD
pop  eax             ;1
pop  eax             ;1
opdesc MOVWSG,    0xFF,0xFF,0xFF,8,0xFF

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
CALL_SETMEM_LONG
pop  eax             ;1
pop  eax             ;1
opdesc MOVLSG,    0xFF,0xFF,0xFF,8,0xFF

opfunc MOVBS
mov  ebp,edi         ;2
add  ebp,byte $00    ;3
push dword [ebp]     ;3
mov  ebp,edi         ;2
add  ebp,byte $00    ;3
push dword [ebp]     ;3
CALL_SETMEM_BYTE
pop  eax             ;1
pop  eax             ;1
opdesc MOVBS,	4,12,0xFF,0xFF,0xFF

opfunc MOVWS
mov  ebp,edi          ;2
add  ebp,byte $00     ;3
push dword [ebp]      ;3
mov  ebp,edi          ;2
add  ebp,byte $00     ;3
push dword [ebp]      ;3
CALL_SETMEM_WORD
pop  eax              ;1
pop  eax              ;1
opdesc MOVWS,	4,12,0xFF,0xFF,0xFF

opfunc MOVLS
mov  ebp,edi         ;2
add  ebp,byte $00    ;3
push dword [ebp]     ;3
mov  ebp,edi         ;2
add  ebp,byte $00    ;3
push dword [ebp]     ;3
CALL_SETMEM_LONG
pop  eax             ;1
pop  eax             ;1
opdesc MOVLS,	4,12,0xFF,0xFF,0xFF

opfunc MOVR
GET_R ebp
mov eax,[ebp]       ;3
GET_R ebp
mov [ebp],eax       ;3
opdesc MOVR,	4,12,0xFF,0xFF,0xFF

opfunc MOVBM
mov  ebp,edi             ;2
add  ebp,byte $00        ;3  
push dword [ebp]         ;3 Set data
mov  ebp,edi             ;2
add  ebp,byte $00        ;3
sub  dword [ebp],byte 1  ;4
push dword [ebp]         ;3 Set addr
CALL_SETMEM_BYTE
pop  eax                 ;1
pop  eax                 ;1
opdesc MOVBM,	4,12,0xFF,0xFF,0xFF

opfunc MOVWM
mov  ebp,edi             ;2
add  ebp,byte $00        ;3  
push dword [ebp]         ;3 Set data
mov  ebp,edi             ;2
add  ebp,byte $00        ;3
sub  dword [ebp],byte 2  ;4
push dword [ebp]         ;3 Set addr
CALL_SETMEM_WORD
pop  eax                 ;1
pop  eax                 ;1
opdesc MOVWM,	4,12,0xFF,0xFF,0xFF

opfunc MOVLM
mov  ebp,edi             ;2
add  ebp,byte $00        ;3  
push dword [ebp]         ;3 Set data
mov  ebp,edi             ;2
add  ebp,byte $00        ;3
sub  dword [ebp],byte 4  ;4
push dword [ebp]         ;3 Set addr
CALL_SETMEM_LONG
pop  eax                 ;1
pop  eax                 ;1
opdesc MOVLM,	4,12,0xFF,0xFF,0xFF
;------------- added ------------------

opfunc TAS
mov  ebp,edi             ;2
add  ebp,byte $00        ;3
push dword [ebp]         ;3
CALL_GETMEM_BYTE
and  eax,0x000000FF      ;5
and  [ebx], byte 0xFE    ;3
test eax,eax             ;3
jne  NOT_ZERO            ;2
or   [ebx], byte 01      ;3
NOT_ZERO:              
or   al, byte 0x80        ;3
push eax                  ;1 
push dword [ebp]          ;3
CALL_SETMEM_BYTE
pop eax                   ;1
pop eax                   ;1
pop eax                   ;1
opdesc TAS,  0xFF,4,0xFF,0xFF,0xFF

; sub with overflow check
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
NO_OVER_FLOS:
opdesc SUBV, 4,12,0xFF,0xFF,0xFF


; string cmp
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
opdesc CMPSTR, 4,12,0xFF,0xFF,0xFF

;-------------------------------------------------------------
;div0s 
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
opdesc DIV0S, 36,4,0xFF,0xFF,0xFF

;===============================================================
; DIV1   1bit Divid operation
; 
; size = 69 + 135 + 132 + 38 = 374 
;===============================================================
opfunc DIV1

; 69
GET_R ebp
GET_SR ecx		    ;ecx = old SR
mov  eax,dword [ebp]

test eax,0x80000000          ;5 
je   NOZERO                  ;2
SET_Q
jmp  CONTINUE                ;3
NOZERO:
CLEAR_Q

CONTINUE:

; sh2i->R[n] |= (DWORD)(sh2i->i_get_T())
GET_SR eax  
and  eax, 0x01               ;5
shl  dword [ebp], byte 1     ;3
or   dword [ebp], eax               ;3

;Get R[n],R[m]
mov  eax,dword [ebp]               ;3 R[n]
GET_R edx
mov  edx,dword [edx]               ;3 R[m]

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
	  sub [ebp],edx         ;3 sh2i->R[n] -= sh2i->R[m]
    
	  TEST_IS_Q
	  jc NQ_NM_Q_FLG                  ;2

	  NQ_NM_NQ_FLG:
	  cmp [ebp],eax        ;3 tmp1 = (sh2i->R[n]>tmp0);
	  jna NQ_NM_NQ_00_FLG  ;2

		  NQ_NM_NQ_01_FLG:
		  SET_Q
		  jmp END_DIV1                 ;3

		  NQ_NM_NQ_00_FLG:
		  CLEAR_Q
		  jmp END_DIV1                 ;3  
  
	  NQ_NM_Q_FLG:
	  cmp [ebp],eax        ;3 tmp1 = (sh2i->R[n]>tmp0);
	  jna NQ_NM_NQ_10_FLG  ;2

		  NQ_NM_NQ_11_FLG:
		  CLEAR_Q
		  jmp END_DIV1                 ;3

		  NQ_NM_NQ_10_FLG:
		  SET_Q
		  jmp END_DIV1                 ;3

Q_FLG_TMP:
jmp Q_FLG; 3

	;----------------------------------------------------  
	NQ_M_FLG:
	  add  [ebp],edx        ; sh2i->R[n] += sh2i->R[m]  
	  TEST_IS_Q
	  jc NQ_M_Q_FLG

	  NQ_M_NQ_FLG:
	  cmp dword [ebp],eax         ; tmp1 = (sh2i->R[n]<tmp0);
	  jnb NQ_M_NQ_00_FLG

		  NQ_M_NQ_01_FLG:
		  CLEAR_Q
		  jmp END_DIV1

		  NQ_M_NQ_00_FLG:
		  SET_Q
		  jmp END_DIV1


	  NQ_M_Q_FLG:
	  cmp dword [ebp],eax                    ; tmp1 = (sh2i->R[n]<tmp0);
	  jnb NQ_M_NQ_10_FLG

		  NQ_M_NQ_11_FLG:
		  SET_Q
		  jmp END_DIV1

		  NQ_M_NQ_10_FLG:
		  CLEAR_Q
		  jmp END_DIV1

;------------------------------------------------------
; 8 + 62 + 62 = 132
Q_FLG:   


test ecx,0x200 ; M == 1 ?
jne  Q_M_FLG

	;--------------------------------------------------
	Q_NM_FLG:
	  add dword [ebp],edx         ; sh2i->R[n] += sh2i->R[m]
	  TEST_IS_Q
	  jc Q_NM_Q_FLG

	  Q_NM_NQ_FLG:
	  cmp dword [ebp],eax      ; tmp1 = (sh2i->R[n]<tmp0);
	  ja Q_NM_NQ_00_FLG

		  Q_NM_NQ_01_FLG:
		  SET_Q
		  jmp END_DIV1

		  Q_NM_NQ_00_FLG:
		  CLEAR_Q
		  jmp END_DIV1
  
	  Q_NM_Q_FLG:
	  cmp dword [ebp],eax      ; tmp1 = (sh2i->R[n]<tmp0);
	  ja Q_NM_NQ_10_FLG  

		  Q_NM_NQ_11_FLG:
		  CLEAR_Q
		  jmp END_DIV1

		  Q_NM_NQ_10_FLG:
		  SET_Q
		  jmp END_DIV1

	;----------------------------------------------------  
	Q_M_FLG:
	  sub [ebp],edx      ; sh2i->R[n] -= sh2i->R[m]  
	  TEST_IS_Q 
	  jc Q_M_Q_FLG

	  Q_M_NQ_FLG:
	  cmp dword [ebp],eax      ; tmp1 = (sh2i->R[n]>tmp0);
	  jb Q_M_NQ_00_FLG

		  Q_M_NQ_01_FLG:
		  CLEAR_Q
		  jmp END_DIV1

		  Q_M_NQ_00_FLG:
		  SET_Q
		  jmp END_DIV1
 
	  Q_M_Q_FLG:
	  cmp dword [ebp],eax      ; tmp1 = (sh2i->R[n]>tmp0);
	  jb Q_M_NQ_10_FLG

		  Q_M_NQ_11_FLG:
		  SET_Q
		  jmp END_DIV1

		  Q_M_NQ_10_FLG:
		  CLEAR_Q
		  jmp END_DIV1


;---------------------------------------------------
; size = 38
END_DIV1:

;sh2i->i_set_T( (sh2i->i_get_Q() == sh2i->i_get_M()) );

GET_Q eax
GET_M edx
CLEAR_T
cmp  eax, edx
jne  NO_Q_M                   ;2
SET_T
NO_Q_M:
opdesc DIV1, 50,4,0xFF,0xFF,0xFF

;======================================================
; end of DIV1
;======================================================

;------------------------------------------------------------
;dmuls
opfunc DMULS
mov  ecx,edx            ;2 Save PC
mov  ebp,edi            ;2  
add  ebp,byte $00       ;3 4..7
mov  eax,dword [ebp]    ;3
mov  ebp,edi            ;2 SetQ
add  ebp,byte $00       ;3 8..11
mov  edx,dword [ebp]    ;3  
imul edx                ;2
SET_MACH edx  ;2 store MACH             
SET_MACL eax  ;3 store MACL  
mov  edx,ecx            ;2
opdesc DMULS, 6,14,0xFF,0xFF,0xFF

;------------------------------------------------------------
;dmulu 32bit -> 64bit Mul
opfunc DMULU
mov  ecx,edx            ;2 Save PC
mov  ebp,edi            ;2  
add  ebp,byte $00       ;3 4..7
mov  eax,dword [ebp]    ;3
mov  ebp,edi            ;2 SetQ
add  ebp,byte $00       ;3 8..11
mov  edx,dword [ebp]    ;3  
mul  edx                ;2
SET_MACH edx  ;2 store MACH             
SET_MACL eax  ;3 store MACL      
mov  edx,ecx            ;2
opdesc DMULU, 6,14,0xFF,0xFF,0xFF

;--------------------------------------------------------------
; mull 32bit -> 32bit Multip
opfunc MULL
mov  ecx,edx            ;2 Save PC
mov  ebp,edi            ;2  
add  ebp,byte $00       ;3 4..7
mov  eax,dword [ebp]    ;3
mov  ebp,edi            ;2
add  ebp,byte $00       ;3 8..11
mov  edx,dword [ebp]    ;3  
imul edx                ;2
SET_MACL eax  ;3 store MACL 
mov  edx,ecx            ;2
opdesc MULL, 6,14,0xFF,0xFF,0xFF

;--------------------------------------------------------------
; muls 16bit -> 32 bit Multip
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
SET_MACL edx ;3 store MACL  
mov  edx,ecx            ;2
opdesc MULS,	6,17,0xFF,0xFF,0xFF

;--------------------------------------------------------------
; mulu 16bit -> 32 bit Multip
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
SET_MACL edx ;3 store MACL   
mov  edx,ecx            ;2
opdesc MULU, 6,17,0xFF,0xFF,0xFF

;--------------------------------------------------------------
; MACL   ans = 32bit -> 64 bit MUL
;        (MACH << 32 + MACL)  + ans 
;------------------------------------------------------------- 
opfunc MAC_L
GET_R ebp
push dword [ebp]          ;3
add  dword [ebp], 4           ;7 R[n] += 4
CALL_GETMEM_LONG
push eax                  ;2
GET_R ebp
push dword [ebp]          ;3                      ;1
add  dword [ebp], 4           ;7 R[m] += 4 
CALL_GETMEM_LONG
pop edx
pop edx
push edi                      ;1 Save GenReg
GET_MACL ecx            ;3 load macl
imul edx                      ;2 eax <- low, edx <- high
GET_MACH edi
add  ecx,eax                  ;3 sum = a+b;
adc  edi,edx                  ;2
TEST_IS_S
jnc   END_PROC                 ;2 if( S == 0 ) goto 'no sign proc'
cmp  edi,7FFFh                ;6
jb   END_PROC                 ;2 
ja   COMP_MIN                ;2
cmp  ecx,0FFFFFFFFh           ;3 
jbe  END_PROC                ;2  = 88

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
SET_MACH edi
SET_MACL ecx
pop  edi                      ; 1 Restore GenReg
pop  eax                      ;1
opdesc MAC_L, 4,22,0xFF,0xFF,0xFF

;--------------------------------------------------------------
; MACW   ans = 32bit -> 64 bit MUL
;        (MACH << 32 + MACL)  + ans 
;-------------------------------------------------------------
opfunc MAC_W
GET_R ebp
push dword [ebp]
add  dword [ebp], 2           ;7 R[n] += 2
CALL_GETMEM_WORD
movsx  edx,ax
push edx
GET_R ebp
push dword [ebp]          ;3
add  dword [ebp], 2           ;7 R[m] += 2
CALL_GETMEM_WORD
pop edx
pop edx
cwde                          ;1 Sigin extention
imul edx                      ;2 eax <- low, edx <- high
TEST_IS_S
jnc   MACW_NO_S_FLG                 ;2 if( S == 0 ) goto 'no sign proc'

MACW_S_FLG:
  add dword [SYS_REG+4],eax   ;3 MACL = ansL + MACL
  jno NO_OVERFLO
  js  FU 
  SEI:
    mov dword [SYS_REG+4],0x80000000 ; min value
	or  dword [SYS_REG],1
	jmp END_MACW

  FU:
    mov dword [SYS_REG+4],0x7FFFFFFF ; max value
	or dword [SYS_REG],1
	jmp END_MACW

  NO_OVERFLO:
  jmp END_MACW

MACW_NO_S_FLG:
  add dword [SYS_REG+4],eax         ;3 MACL = ansL + MACL
  jnc MACW_NO_CARRY             ;2 Check Carry
  inc edx                       ;1
MACW_NO_CARRY: 
  add dword [SYS_REG],edx           ;2 MACH = ansH + MACH

END_MACW:
pop  eax                        ;1
opdesc MAC_W, 4,25,0xFF,0xFF,0xFF
  
 
end:
