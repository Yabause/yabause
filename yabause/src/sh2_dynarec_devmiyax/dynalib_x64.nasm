; Copyright 2017 FCare(francois.caron.perso@gmail.com)
; 
; This file is part of Yabause.
; 
; Yabause is free software; you can rrdistribute it and/or modify
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
;

; MACH   [GEN_REG, #(16+3+0)*4 ]   [r14]  
; MACL   [GEN_REG, #(16+3+1)*4 ]   [r14, #(1)*4]  
; PR     [GEN_REG, #(16+3+2)*4 ]   [r14, #(2)*4]
; PC     [GEN_REG, #(16+3+3)*4 ]  
; COUNT  [GEN_REG, #(16+3+4)*4 ]  
; ICOUNT [GEN_REG, #(16+3+5)*4 ]  
; SR     [GEN_REG, #(16+0)*4 ]  
; GBR     [GEN_REG, #(16+1)*4 ]  
; VBR     [GEN_REG, #(16+2)*4 ]  
; getmembyte = [GEN_REG, #((16+3+6)*4 + 0 *8) + 4 // Alignment] ;
; getmemword = [GEN_REG, #((16+3+6)*4 + 1 *8) + 4 // Alignment] ;
; getmemlong = [GEN_REG, #((16+3+6)*4 + 2 *8) + 4 // Alignment] ;
; setmembyte = [GEN_REG, #((16+3+6)*4 + 3 *8) + 4 // Alignment] ;
; setmemword = [GEN_REG, #((16+3+6)*4 + 4 *8) + 4 // Alignment] ;
; setmemlong = [GEN_REG, #((16+3+6)*4 + 5 *8) + 4 // Alignment] ;

bits 64

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

%define GEN_REG r12
%define CTRL_REG GEN_REG+16*4
%define SYS_REG CTRL_REG+3*4
%define MACH SYS_REG
%define MACL SYS_REG+4
%define PR SYS_REG+2*4
%define PC SYS_REG+3*4
%define R_0 GEN_REG
%define R_15 GEN_REG+15*4
%define GBR CTRL_REG+4

%define SCRATCH1 rbp
%define SCRATCH2 rbx

%macro GET_R 1
	mov %1, GEN_REG         
	add %1,byte 0x7F    
%endmacro

%macro GET_BYTE_IMM 1   
	mov %1, 0x7F    
%endmacro

%macro GET_SIGNED_BYTE_IMM 1 
	xor %1, %1  
	or %1, 0x7F    
%endmacro

%macro GET_R_ID 2
	mov %2, %1
	shl %2, byte 2
	add %2, GEN_REG              
%endmacro

%macro SET_MACH 1
 	mov dword [MACH], %1
%endmacro

%macro SET_MACL 1
	mov dword [MACL], %1
%endmacro

%macro GET_MACH 1
	mov %1, dword [MACH]
%endmacro

%macro GET_MACL 1
	mov %1, dword [MACL]
%endmacro

%macro GET_PR 1
	mov %1, dword [PR]
%endmacro

%macro SET_PR 1
	mov dword [PR], %1
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
	btr dword [CTRL_REG], 0x0
%endmacro

%macro SET_T 0
	or dword [CTRL_REG], 0x1
%endmacro

%macro SET_T_R 2
	CLEAR_T
	bt %1,%2
	jnc .end
	SET_T
	.end:
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
  mov r8, [SYS_REG + 28 + %1*8]
  call r8  
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
        push SCRATCH2           ;1
        push SCRATCH1           ;1
        push GEN_REG           ;1
%endmacro

%macro POPAD 0
	pop GEN_REG           ;1
	pop SCRATCH1           ;1
	pop SCRATCH2           ;1
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
; u32 GenReg[16] ->r12 (m_pDynaSh2)
; u32 CtrlReg[3]
; u32 SysReg[6];
; PC => SysReg[3]
; SR => CtrlReg[0]

; r12  <- Address of GenReg


; Size = 27 Bytes
START prologue
PUSHAD
mov GEN_REG,rdi        ;GEN_REG = m_pDynaSh2 
END prologue

;-------------------------------------------------------
; normal part par instruction
;Size = 9 Bytes
START seperator_normal
add dword [PC], byte 2   ;3 PC += 2
END  seperator_normal

START check_interrupt
CALL_CHECK_INT
END check_interrupt

;------------------------------------------------------
; Delay slot part par instruction
;Size = 40 Bytes
START seperator_delay_slot
sub dword [PC], byte 2                       ; 1
END seperator_delay_slot


;------------------------------------------------------
; End part of delay slot
;Size = 19 Bytes
START seperator_delay_after
add dword [PC], byte 2   ;
END seperator_delay_after

;-------------------------------------------------------
; End of block
; Size = 10 Bytes
START epilogue
POPAD
ret             ;1
END epilogue

;------------------------------------------------------
; Jump part
; Size = 27 Bytes
START PageFlip                       ; 1
END PageFlip

;-------------------------------------------------------
; normal part par instruction( for debug build )
;Size = 39 Bytes
START seperator_d_normal
add dword [PC+4], byte 1 ;4 Clock += 1
;mov  rax,DebugEachClock ;5
;call rax                 ;2
test rax, 0x01           ;5 finish 
jz  NEXT_D_INST          ;2
POPAD
ret                      ;1
NEXT_D_INST:
add dword [PC], byte 2   ;3 PC += 2
END seperator_d_normal

;------------------------------------------------------
; Delay slot part par instruction( for debug build )
;Size = 52 Bytes
START seperator_d_delay
;mov  rax,DebugDelayClock ;5
;call rax                   ;2
test dword [rsp], 0xFFFFFFFF ; 7
jnz   .continue               ; 2
add dword [PC], byte 2   ;3 PC += 2
POPAD
ret                          ; 1
.continue:
mov  eax,dword [rsp]             ; 3
sub  eax,byte 2            ; 3
mov  dword [rdx],eax             ; 2
END seperator_d_delay

;=================
;Begin x86 Opcodes
;=================

opfunc CLRT
CLEAR_T
opdesc CLRT,  0xFF,0xFF,0xFF,0xFF,0xFF

opfunc CLRMAC
and qword [MACH], 0   ;4
opdesc CLRMAC,	0xFF,0xFF,0xFF,0xFF,0xFF

opfunc NOP
opdesc NOP,		0xFF,0xFF,0xFF,0xFF,0xFF

opfunc DIV0U
and dword [CTRL_REG],0x0f2
opdesc DIV0U,	0xFF,0xFF,0xFF,0xFF,0xFF

opfunc SETT
SET_T
opdesc SETT,	0xFF,0xFF,0xFF,0xFF,0xFF

opfunc SLEEP
POPAD
ret                 ;1
opdesc SLEEP,	0xFF,0xFF,0xFF,0xFF,0xFF

opfunc SWAP_W
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
rol eax,16          ;2
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;2
opdesc SWAP_W,	6,19,0xff,0xff,0xff

opfunc SWAP_B
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
xchg ah,al          ;2
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;2
opdesc SWAP_B,	6,18,0xff,0xff,0xff

opfunc TST
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
GET_R SCRATCH1
CLEAR_T
test dword [SCRATCH1],eax      ;2
jne end_tst
SET_T
end_tst:
opdesc TST,	6,16,0xff,0xff,0xff

opfunc TSTI
mov eax,dword [R_0]       ;2
CLEAR_T
test al,$00        ;5  Imidiate Val
jne end_tsti
SET_T
end_tsti:
opdesc TSTI,	0xff,0xff,0xff,12,0xff


opfunc ANDI
and dword [R_0],dword 0x000000ff ;3
opdesc ANDI,	0xff,0xff,0xff,4,0xff

opfunc XORI
xor byte [R_0],byte 0x7f ;3
opdesc XORI,	0xff,0xff,0xff,4,0xff

opfunc ORI
or byte [R_0],byte 0x7f ;3
opdesc ORI,	0xff,0xff,0xff,4,0xff

opfunc CMP_EQ_IMM
mov eax, [GEN_REG]            ;2
CLEAR_T
cmp eax, byte 0x7F ;4
jne .continue
SET_T
.continue:
opdesc CMP_EQ_IMM,     0xff,0xff,0xff,13,0xff


opfunc XTRCT
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
GET_R SCRATCH1
shl eax,16          ;3
shr dword [SCRATCH1],16  ;3
or dword [SCRATCH1],eax        ;2
opdesc XTRCT,	6,16,0xff,0xff,0xff

opfunc ADD
GET_R SCRATCH1
mov dword eax,[SCRATCH1]
GET_R SCRATCH1
add dword [SCRATCH1],eax       ;2
opdesc ADD,		6,16,0xff,0xff,0xff

opfunc ADDC
GET_R SCRATCH1
mov dword eax,[SCRATCH1]
GET_R SCRATCH1
mov dword ebx,[SCRATCH1]
TEST_IS_T
CLEAR_T
jnc ADD_REG
add ebx, 1
jnc ADD_REG
SET_T
ADD_REG:
add ebx,eax       ;3  rn = rm +rn (with carry)
jnc ADDC_NO_CARRY   ;2
SET_T
ADDC_NO_CARRY:
mov dword [SCRATCH1], ebx
opdesc ADDC,	6,16,0xff,0xff,0xff


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
opdesc ADDV, 6,16,0xff,0xff,0xff

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
opdesc SUBC,	6,16,0xff,0xff,0xff



opfunc SUB
GET_R SCRATCH1
mov dword eax,[SCRATCH1]
GET_R SCRATCH1
sub dword [SCRATCH1],eax       ;2
opdesc SUB,		6,16,0xff,0xff,0xff

opfunc NOT
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
not eax             ;2
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;2
opdesc NOT,		6,18,0xff,0xff,0xff

opfunc NEG
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
neg eax             ;2
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;2
opdesc NEG,		6,18,0xff,0xff,0xff

opfunc NEGC
GET_R SCRATCH1
mov ecx,[SCRATCH1]             ;3
neg ecx                   ;2
GET_R SCRATCH1
mov dword [SCRATCH1],ecx             ;3
GET_T eax
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
opdesc NEGC,	6,18,0xff,0xff,0xff

opfunc EXTUB
GET_R SCRATCH1
mov eax, dword [SCRATCH1]  
and dword eax,0x000000ff  ;5
GET_R SCRATCH1
mov dword [SCRATCH1],eax
opdesc EXTUB,	6,21,0xff,0xff,0xff

opfunc EXTU_W
GET_R SCRATCH1
mov eax, dword [SCRATCH1]   
and dword eax,0xffff;5
GET_R SCRATCH1
mov dword [SCRATCH1],eax
opdesc EXTU_W,	6,21,0xff,0xff,0xff

opfunc EXTS_B
GET_R SCRATCH1
mov eax, dword [SCRATCH1]
cbw                 ;2
cwde                ;1
GET_R SCRATCH1
mov dword [SCRATCH1],eax
opdesc EXTS_B,	6,19,0xff,0xff,0xff

opfunc EXTS_W
GET_R SCRATCH1
mov eax, dword [SCRATCH1]    
cwde                ;2
GET_R SCRATCH1
mov dword [SCRATCH1],eax
opdesc EXTS_W,	6,17,0xff,0xff,0xff

;Store Register Opcodes
;----------------------

opfunc STC_SR_MEM
GET_R SCRATCH1
sub  dword [SCRATCH1],byte 4 ;4
mov edi, dword[SCRATCH1]
GET_SR esi
CALL_SETMEM_LONG
opdesc STC_SR_MEM,	0xff,6,0xff,0xff,0xff

opfunc STC_GBR_MEM
GET_R SCRATCH1
sub  dword [SCRATCH1],byte 4 ;4
GET_GBR esi
mov edi, dword[SCRATCH1]
CALL_SETMEM_LONG
opdesc STC_GBR_MEM,	0xff,6,0xff,0xff,0xff

opfunc STC_VBR_MEM
GET_R SCRATCH1
sub  dword [SCRATCH1],byte 4 ;4
mov edi, dword[SCRATCH1]
GET_VBR esi
CALL_SETMEM_LONG
opdesc STC_VBR_MEM,	0xff,6,0xff,0xff,0xff


;------------------------------

opfunc MOVBL
GET_R SCRATCH1
mov edi,dword [SCRATCH1]       ;3
CALL_GETMEM_BYTE
GET_R SCRATCH1
cbw                 ;1
cwde                ;1
mov dword [SCRATCH1],eax       ;3
opdesc MOVBL,	6,24,0xff,0xff,0xff

opfunc MOVWL
GET_R SCRATCH1
mov edi,dword [SCRATCH1]       ;3
CALL_GETMEM_WORD
GET_R SCRATCH1
cwde                ;1
mov dword [SCRATCH1],eax       ;3
opdesc MOVWL,		6,24,0xff,0xff,0xff

opfunc MOVL_MEM_REG
GET_R SCRATCH1
mov edi, dword [SCRATCH1]       ;3
CALL_GETMEM_LONG
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;3
opdesc MOVL_MEM_REG,	6,24,0xff,0xff,0xff

opfunc MOVBP
GET_R SCRATCH1
mov edi,dword [SCRATCH1]       ;3
CALL_GETMEM_BYTE
cbw                 ;1
cwde                ;1
GET_R SCRATCH2
cmp SCRATCH1,SCRATCH2
je continue_movbp
inc dword [SCRATCH1]     ;3
continue_movbp:
mov dword [SCRATCH2],eax       ;3
opdesc MOVBP,	6,27,0xff,0xff,0xff


opfunc MOVWP
GET_R SCRATCH1
mov edi,dword [SCRATCH1]       ;3
CALL_GETMEM_WORD
cwde                ;1
GET_R SCRATCH2
cmp SCRATCH1,SCRATCH2
je continue_movwp
add dword [SCRATCH1],byte 2
continue_movwp:
mov dword [SCRATCH2],eax       ;3
opdesc MOVWP,	6,25,0xff,0xff,0xff

opfunc MOVLP
GET_R SCRATCH1
mov edi, dword [SCRATCH1]       ;3
CALL_GETMEM_LONG
GET_R SCRATCH2
mov dword [SCRATCH2],eax       ;3
cmp SCRATCH2, SCRATCH1
je end_movlp
add dword [SCRATCH1],byte 4 ;4
end_movlp:
opdesc MOVLP,	6,24,0xff,0xff,0xff

opfunc MOVI
GET_R SCRATCH1
GET_SIGNED_BYTE_IMM eax
mov dword [SCRATCH1],eax       ;3
opdesc MOVI,	0xff,6,0xff,11,0xff

;----------------------

opfunc MOVBL0
GET_R SCRATCH1
mov edi,dword [SCRATCH1]        ;3
add edi,dword [R_0]
CALL_GETMEM_BYTE
GET_R SCRATCH1
cbw                  ;1
cwde                 ;1
mov dword [SCRATCH1],eax        ;3
opdesc MOVBL0,	6,28,0xff,0xff,0xff

opfunc MOVWL0
GET_R SCRATCH1
mov edi,dword [SCRATCH1]        ;3
add edi,dword [R_0]        ;2
CALL_GETMEM_WORD
GET_R SCRATCH1
cwde                 ;1
mov dword [SCRATCH1],eax        ;3
opdesc MOVWL0,	6,28,0xff,0xff,0xff

opfunc MOVLL0
GET_R SCRATCH1
mov edi,dword [SCRATCH1]        ;3
add edi,dword [R_0]        ;2
CALL_GETMEM_LONG
GET_R SCRATCH1
mov dword [SCRATCH1],eax        ;3
opdesc MOVLL0,	6,28,0xff,0xff,0xff

opfunc MOVT
GET_R SCRATCH1
GET_T eax
mov dword [SCRATCH1],eax       ;3
opdesc MOVT,		0xff,6,0xff,0xff,0xff

opfunc MOVBS0
GET_R SCRATCH1
mov edi,dword [SCRATCH1]        ;3
add edi,dword [R_0]        ;2
GET_R SCRATCH1
mov  esi,dword [SCRATCH1]       ;3
CALL_SETMEM_BYTE
opdesc MOVBS0,	20,6,0xff,0xff,0xff

opfunc MOVWS0
GET_R SCRATCH1
mov edi,dword [SCRATCH1]        ;3
add edi,dword [R_0]        ;2
GET_R SCRATCH1
mov  esi,dword [SCRATCH1]       ;3
CALL_SETMEM_WORD         ;1
opdesc MOVWS0,	20,6,0xff,0xff,0xff

opfunc MOVLS0
GET_R SCRATCH1
mov edi,dword [SCRATCH1]        ;3
add edi,dword [R_0]        ;2
GET_R SCRATCH1
mov  esi,dword [SCRATCH1]       ;3
CALL_SETMEM_LONG
opdesc MOVLS0,	20,6,0xff,0xff,0xff

;===========================================================================
;Verified Opcodes
;===========================================================================

opfunc DT
GET_R SCRATCH1
CLEAR_T
dec dword [SCRATCH1]     ;3
jne .continue       ;2
SET_T
.continue:
opdesc DT,		0xff,6,0xff,0xff,0xff

opfunc CMP_PZ
GET_R SCRATCH1
CLEAR_T
cmp dword [SCRATCH1],byte 0    ;4
jl .continue              ;2
SET_T
.continue:
opdesc CMP_PZ,	0xff,6,0xff,0xff,0xff

opfunc CMP_PL
GET_R SCRATCH1
CLEAR_T
cmp dword [SCRATCH1], 0          ;7
jle .continue               ;2
SET_T
.continue:
opdesc CMP_PL,	0xff,6,0xff,0xff,0xff

opfunc CMP_EQ
GET_R SCRATCH1
mov eax,[SCRATCH1]       ;2
GET_R SCRATCH1
CLEAR_T
cmp [SCRATCH1],eax       ;3
jne .continue       ;2  
SET_T
.continue:
opdesc CMP_EQ,	6,16,0xff,0xff,0xff

opfunc CMP_HS
GET_R SCRATCH1
mov eax,[SCRATCH1]       ;3
GET_R SCRATCH1
CLEAR_T
cmp [SCRATCH1],eax       ;3
jb .continue        ;2
SET_T
.continue:
opdesc CMP_HS,	6,16,0xff,0xff,0xff

opfunc CMP_HI
GET_R SCRATCH1
mov eax,[SCRATCH1]       ;2
GET_R SCRATCH1
CLEAR_T
cmp [SCRATCH1],eax       ;3
jbe .continue       ;2
SET_T 
.continue:
opdesc CMP_HI,	6,16,0xff,0xff,0xff

opfunc CMP_GE
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
GET_R SCRATCH1
CLEAR_T
cmp dword [SCRATCH1],eax       ;3
jl .continue        ;2
SET_T  
.continue:
opdesc CMP_GE,	6,16,0xff,0xff,0xff

opfunc CMP_GT
GET_R SCRATCH1
mov eax,[SCRATCH1]       ;2
GET_R SCRATCH1
CLEAR_T
cmp [SCRATCH1],eax       ;3
jle .continue       ;2
SET_T
.continue:
opdesc CMP_GT,	6,16,0xff,0xff,0xff

opfunc ROTR
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
SET_T_R eax, 0
ror dword [SCRATCH1], 1
opdesc ROTR,	0xff,6,0xff,0xff,0xff

opfunc ROTCR
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
shr dword [SCRATCH1],byte 1     ;3
TEST_IS_T
jnc continue_rotcr
bts dword [SCRATCH1], 31
continue_rotcr:  ;2
SET_T_R eax, 0
opdesc ROTCR,	0xff,6,0xff,0xff,0xff

opfunc ROTL
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
rol dword [SCRATCH1], 1
SET_T_R eax, 31
opdesc ROTL,	0xff,6,0xff,0xff,0xff

opfunc ROTCL
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
shl dword [SCRATCH1],byte 1     ;3
TEST_IS_T
jnc continue_rotcl
or dword [SCRATCH1],byte 1
continue_rotcl:
SET_T_R eax,31
opdesc ROTCL,	0xff,6,0xff,0xff,0xff

opfunc SHL
GET_R SCRATCH1
mov eax, dword [SCRATCH1]
shl dword [SCRATCH1], byte 1
CLEAR_T
bt eax,byte 31     ;3
jnc cont_shl
SET_T
cont_shl:
opdesc SHL,		0xff,6,0xff,0xff,0xff

opfunc SHLR
GET_R SCRATCH1
mov eax, dword [SCRATCH1]
shr dword [SCRATCH1], byte 1
and dword eax,1             ;5
CLEAR_T
test eax, 1
je cont_shlr
SET_T
cont_shlr:
opdesc SHLR,	0xff,6,0xff,0xff,0xff

opfunc SHAR
GET_R SCRATCH1
mov eax,dword [SCRATCH1]
shr dword [SCRATCH1], byte 1
CLEAR_T
bt eax, 0
jnc cont_shar
SET_T
cont_shar:
bt eax, 31
jnc .end
bts dword [SCRATCH1], 31
.end:
opdesc SHAR,	0xff,6,0xff,0xff,0xff


opfunc SHLL2
GET_R SCRATCH1
shl dword [SCRATCH1],byte 2 ;4
opdesc SHLL2,	0xff,6,0xff,0xff,0xff

opfunc SHLR2
GET_R SCRATCH1
shr dword [SCRATCH1],byte 2 ;4
opdesc SHLR2,	0xff,6,0xff,0xff,0xff

opfunc SHLL8
GET_R SCRATCH1
shl dword [SCRATCH1],byte 8 ;4
opdesc SHLL8,	0xff,6,0xff,0xff,0xff

opfunc SHLR8
GET_R SCRATCH1
shr dword [SCRATCH1],byte 8 ;4
opdesc SHLR8,	0xff,6,0xff,0xff,0xff


opfunc SHLL16
GET_R SCRATCH1
shl dword [SCRATCH1],byte 16 ;4
opdesc SHLL16,	0xff,6,0xff,0xff,0xff

opfunc SHLR16
GET_R SCRATCH1
shr dword [SCRATCH1],byte 16 ;4
opdesc SHLR16,	0xff,6,0xff,0xff,0xff

opfunc AND
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
GET_R SCRATCH1
and dword [SCRATCH1],eax       ;2
opdesc AND,		6,16,0xff,0xff,0xff

opfunc OR
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
GET_R SCRATCH1
or dword [SCRATCH1],eax       ;2
opdesc OR,		6,16,0xff,0xff,0xff

opfunc XOR
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
GET_R SCRATCH1
xor dword[SCRATCH1],eax       ;3
opdesc XOR,		6,16,0xff,0xff,0xff

opfunc ADDI
GET_R SCRATCH1
add dword [SCRATCH1],byte 0x7F ;4
opdesc ADDI,	0xff,6,0xff,10,0xff

opfunc AND_B
GET_GBR ebx
add  ebx, dword [GEN_REG]
mov edi, ebx
CALL_GETMEM_BYTE
and al, $7F     ;2
mov edi, ebx
mov esi, eax
CALL_SETMEM_BYTE
opdesc AND_B,	0xff,0xff,0xff,20,0xff

opfunc OR_B
GET_GBR ebx
add  ebx, dword [R_0]
mov edi, ebx
CALL_GETMEM_BYTE
or al, $00      ;2
mov edi, ebx
mov esi, eax
CALL_SETMEM_BYTE
opdesc OR_B,	0xff,0xff,0xff,20,0xff

opfunc XOR_B
GET_GBR ebx
add  ebx, dword [R_0]
mov edi, ebx
CALL_GETMEM_BYTE
xor al, 0xFF      ;2
mov edi, ebx
mov esi, eax
CALL_SETMEM_BYTE
opdesc XOR_B,	0xff,0xff,0xff,20,0xff

opfunc TST_B
CLEAR_T
GET_GBR ebx
add  ebx, dword [R_0]
mov edi, ebx
CALL_GETMEM_BYTE
test al, $00
jne .continue         ;2
SET_T
.continue:
opdesc TST_B,	0xff,0xff,0xff,27,0xff

;Jump Opcodes
;------------

opfunc JMP
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
mov dword [PC],eax       ;3
opdesc JMP,		0xff,6,0xff,0xff,0xff

opfunc JSR
mov eax, dword [PC]       ;2
add eax,byte 4      ;3
SET_PR eax
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;3
mov dword [PC],eax       ;3
opdesc JSR,		0xff,19,0xff,0xff,0xff

opfunc BRA
mov ax, 0x0ABF     ;3
bt ax, 11        ;4
jnc .continue       ;2
or ax,0xf000   ;5
.continue:
cwde
shl eax, 1
add eax,byte 4      ;3
add dword [PC],eax ;2      ;3
opdesc BRA,		0xff,0xff,0xff,0xff,2

opfunc BSR
mov ebx,dword [PC]       ;2
add ebx,byte 4      ;3
SET_PR ebx
mov ax, 0x0ABE   ;3
bt ax, 11        ;4
jnc .continue       ;2
or ax,0xf000   ;5
.continue:
cwde
shl eax, 1      ;3
add eax,ebx ;2
mov dword [PC],eax       ;3
opdesc BSR,		0xff,0xff,0xff,0xff,15

opfunc BSRF
GET_R SCRATCH1
mov eax,dword [PC]       ;2
add eax,byte 4      ;3
SET_PR eax
add eax,dword [SCRATCH1] ;3
mov dword [PC],eax       ;3
opdesc BSRF,		0xff,6,0xff,0xff,0xff

opfunc BRAF
GET_R SCRATCH1
mov eax,dword [PC]       ;2
add eax,dword [SCRATCH1] ;3
add eax,byte 4      ;4
mov dword [PC],eax       ;3
opdesc BRAF,		0xff,6,0xff,0xff,0xff


opfunc RTS
GET_PR eax     ;3
mov dword [PC],eax       ;3
opdesc RTS,			0xFF,0xFF,0xFF,0xFF,0xFF

opfunc RTE
mov edi, dword [R_15]        ;3
CALL_GETMEM_LONG
mov dword [PC], eax
add dword [R_15], byte 4
mov edi, dword [R_15]
CALL_GETMEM_LONG 
SET_SR eax
add dword [R_15], byte 4
opdesc RTE,			0xFF,0xFF,0xFF,0xFF,0xFF

opfunc TRAPA
sub  dword [R_15], byte 4    ;7
mov edi, dword [R_15]        ;3
GET_SR esi
CALL_SETMEM_LONG
sub  dword [R_15], byte 4    ;7
mov  esi,dword [PC]        ;3 PC
add  esi,byte 2       ;3
mov edi, dword [R_15]        ;3
CALL_SETMEM_LONG
GET_BYTE_IMM eax
shl  eax,2            ;3
add  eax, dword [CTRL_REG+8]      ;3 ADD VBR
mov  edi,eax
CALL_GETMEM_LONG
mov  dword [PC],eax        ;3
sub  dword [PC], byte 2        ;3
opdesc TRAPA,	      0xFF,0xFF,0xFF,58,0xFF

opfunc BT
TEST_IS_T
jnc .continue        ;2
GET_SIGNED_BYTE_IMM eax
shl eax,byte 1      ;3
add eax,byte 2      ;3
add dword [PC], eax ;2
.continue:
opdesc BT,		0xFF,0xFF,0xFF,13,0xFF

opfunc BT_S
TEST_IS_T
jnc .continue        ;2       ;2
GET_SIGNED_BYTE_IMM eax
shl eax, byte 1      ;3
add dword [PC], eax ;2
.continue:
add dword [PC], byte 4 ;2
opdesc BT_S,		0xFF,0xFF,0xFF,13,0xFF

opfunc BF
TEST_IS_T
jc .continue        ;2
GET_SIGNED_BYTE_IMM eax
shl eax, byte 1      ;3
add eax, byte 2      ;3
add dword [PC], eax ;2
.continue:
opdesc BF,		0xFF,0xFF,0xFF,13,0xFF

opfunc BF_S
TEST_IS_T
jc .continue        ;2       ;2
GET_SIGNED_BYTE_IMM eax
shl eax, byte 1      ;3
add dword [PC], eax ;2
.continue:
add dword [PC], byte 4      ;3
opdesc BF_S,		0xFF,0xFF,0xFF,13,0xFF

;Store/Load Opcodes
;------------------

opfunc STC_SR
GET_R SCRATCH1
GET_SR eax     ;2
mov dword [SCRATCH1],eax     ;2
opdesc STC_SR,	0xFF,6,0xFF,0xFF,0xFF

opfunc STC_GBR
GET_R SCRATCH1
GET_GBR eax
mov dword [SCRATCH1],eax
opdesc STC_GBR,	0xFF,6,0xFF,0xFF,0xFF

opfunc STC_VBR
GET_R SCRATCH1
GET_VBR eax
mov dword [SCRATCH1],eax
opdesc STC_VBR,	0xFF,6,0xFF,0xFF,0xFF

opfunc STS_MACH
GET_R SCRATCH1
GET_MACH eax     ;2
mov dword [SCRATCH1],eax     ;2
opdesc STS_MACH, 0xFF,6,0xFF,0xFF,0xFF

opfunc STS_MACH_DEC
GET_R SCRATCH1
sub dword [SCRATCH1],byte 4 ;3
GET_MACH esi 
mov edi,dword [SCRATCH1]     ;2
CALL_SETMEM_LONG
opdesc STS_MACH_DEC,	0xFF,6,0xFF,0xFF,0xFF

opfunc STS_MACL
GET_R SCRATCH1
GET_MACL eax     ;2
mov dword [SCRATCH1],eax     ;2
opdesc STS_MACL, 0xFF,6,0xFF,0xFF,0xFF

opfunc STS_MACL_DEC
GET_R SCRATCH1
sub dword [SCRATCH1],byte 4 ;3
GET_MACL esi 
mov edi,dword [SCRATCH1]     ;2
CALL_SETMEM_LONG
opdesc STS_MACL_DEC,	0xFF,6,0xFF,0xFF,0xFF

opfunc LDC_SR
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
SET_SR eax
opdesc LDC_SR,	0xFF,6,0xFF,0xFF,0xFF

opfunc LDC_SR_INC
GET_R SCRATCH1
mov edi,dword [SCRATCH1]       ;2
CALL_GETMEM_LONG
SET_SR eax ;3
add dword [SCRATCH1],byte 4 ;3
opdesc LDC_SR_INC,	0xFF,6,0xFF,0xFF,0xFF

opfunc LDCGBR
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;3
SET_GBR eax
opdesc LDCGBR,	0xff,6,0xFF,0xFF,0xFF

opfunc LDC_GBR_INC
GET_R SCRATCH1
mov edi,dword [SCRATCH1]       ;2
add dword [SCRATCH1],byte 4 ;3
CALL_GETMEM_LONG
SET_GBR eax ;3
opdesc LDC_GBR_INC,	0xFF,6,0xFF,0xFF,0xFF

opfunc LDC_VBR
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
SET_VBR eax
opdesc LDC_VBR,	0xFF,6,0xFF,0xFF,0xFF

opfunc LDC_VBR_INC
GET_R SCRATCH1
mov edi,dword [SCRATCH1]       ;2
CALL_GETMEM_LONG
SET_VBR eax ;3
add dword [SCRATCH1],byte 4 ;3
opdesc LDC_VBR_INC,	0xFF,6,0xFF,0xFF,0xFF

opfunc STS_PR
GET_R SCRATCH1
GET_PR eax     ;2
mov dword [SCRATCH1],eax     ;2
opdesc STS_PR,		0xFF,6,0xFF,0xFF,0xFF

opfunc STSMPR
GET_R SCRATCH1
sub  dword [SCRATCH1],byte 4 ;4
mov edi, dword[SCRATCH1]
GET_PR esi
CALL_SETMEM_LONG
opdesc STSMPR,	0xFF,6,0xFF,0xFF,0xFF

opfunc LDS_PR
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
SET_PR eax
opdesc LDS_PR,		0xFF,6,0xFF,0xFF,0xFF

opfunc LDS_PR_INC
GET_R SCRATCH1
mov edi,dword [SCRATCH1]       ;3
CALL_GETMEM_LONG
SET_PR eax
add dword [SCRATCH1],byte 4 ;4
opdesc LDS_PR_INC,	0xFF,6,0xFF,0xFF,0xFF

opfunc LDS_MACH
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
SET_MACH eax
opdesc LDS_MACH,		0xFF,6,0xFF,0xFF,0xFF

opfunc LDS_MACH_INC
GET_R SCRATCH1
mov edi,dword [SCRATCH1]          ;2
CALL_GETMEM_LONG
SET_MACH eax        ;2
add dword [SCRATCH1],byte 4 ;3
opdesc LDS_MACH_INC,	0xFF,6,0xFF,0xFF,0xFF

opfunc LDS_MACL
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;2
SET_MACL eax
opdesc LDS_MACL,		0xFF,6,0xFF,0xFF,0xFF


opfunc LDS_MACL_INC
GET_R SCRATCH1
mov edi,dword [SCRATCH1]          ;2
CALL_GETMEM_LONG
SET_MACL eax        ;2
add dword [SCRATCH1],byte 4 ;3
opdesc LDS_MACL_INC,	0xFF,6,0xFF,0xFF,0xFF

;Mov Opcodes
;-----------

opfunc MOVA
mov ebx,[PC]       ;2
add ebx,byte 4      ;3
and bl, 0xfc   ;3
GET_BYTE_IMM eax
shl eax,byte 2      ;3
add eax, ebx ;2
mov dword [R_0],eax       ;2
opdesc MOVA,	0xFF,0xFF,0xFF,12,0xFF


opfunc MOVWI
GET_R SCRATCH1
GET_BYTE_IMM edi
shl edi,byte 1       ;1
add edi,[PC]       ;2 
add edi,byte 4      ;3
CALL_GETMEM_WORD
cwde                ;1
mov dword [SCRATCH1], eax       ;3
opdesc MOVWI,	0xFF,6,0xFF,8,0xFF


opfunc MOVLI
GET_R SCRATCH1
GET_BYTE_IMM eax
shl eax,byte 2       ;3
mov edi,dword [PC]       ;2
add edi,byte 4      ;3
and edi,0xFFFFFFFC  ;6
add edi,eax         ;2            ;1
CALL_GETMEM_LONG
mov dword [SCRATCH1],eax       ;3
opdesc MOVLI,       0xFF,6,0xFF,8,0xFF

opfunc MOVBL4
GET_R SCRATCH1
GET_BYTE_IMM edi
add  edi,dword [SCRATCH1]        ;3
CALL_GETMEM_BYTE
cbw                   ;1  Sign extension byte -> word
cwde                  ;1  Sign extension qword -> dword
mov  dword [R_0],eax        ;3
opdesc MOVBL4, 6,0xFF,8,0xFF,0xFF


opfunc MOVWL4
GET_R SCRATCH1
GET_BYTE_IMM edi
shl  edi, byte 1       ;3  << 1
add  edi,dword [SCRATCH1]        ;2
CALL_GETMEM_WORD
cwde                  ;2  sign 
mov  dword [R_0],eax        ;3
opdesc MOVWL4, 6,0xFF,8,0xFF,0xFF

opfunc MOVLL4
GET_R SCRATCH1
GET_BYTE_IMM edi
shl  edi, byte 2       ;3  << 2
add  edi,dword [SCRATCH1]        ;2
CALL_GETMEM_LONG
GET_R SCRATCH1
mov  dword [SCRATCH1],eax        ;3
opdesc MOVLL4, 6,32,8,0xFF,0xFF

 
opfunc MOVBS4
GET_R SCRATCH1
mov esi, dword [R_0]     ;3
GET_BYTE_IMM edi
add edi,dword [SCRATCH1]       ;3  Add Disp value
CALL_SETMEM_BYTE
opdesc MOVBS4,	6,0xFF,12,0xFF,0xFF

opfunc MOVWS4
mov esi, dword [R_0]     ;3
GET_R SCRATCH1
GET_BYTE_IMM edi
shl edi,byte 1      ;3  Shift Left
add edi,dword [SCRATCH1]       ;3  Add Disp value
CALL_SETMEM_WORD
opdesc MOVWS4,	10,0xFF,12,0xFF,0xFF


opfunc MOVLS4
GET_R SCRATCH1
mov esi, dword [SCRATCH1]     ;3
GET_BYTE_IMM edi
shl edi,byte 2  
GET_R SCRATCH1
add edi, dword [SCRATCH1]     ;3
CALL_SETMEM_LONG
opdesc MOVLS4,	6,24,11,0xFF,0xFF

 
opfunc MOVBLG
GET_BYTE_IMM edi
add edi, dword [GBR]
CALL_GETMEM_BYTE
cbw                    ;1
cwde                   ;1
mov  dword [R_0],eax         ;3
opdesc MOVBLG,    0xFF,0xFF,0xFF,1,0xFF


opfunc MOVWLG
GET_BYTE_IMM edi
shl  edi,byte 1         ;3  Shift left 2
add edi, dword [GBR]
CALL_GETMEM_WORD
cwde                   ;1
mov  dword [R_0],eax         ;3
opdesc MOVWLG,    0xFF,0xFF,0xFF,1,0xFF


opfunc MOVLLG
GET_BYTE_IMM edi
shl  edi,byte 2         ;3  Shift left 2
add edi, dword [GBR]
CALL_GETMEM_LONG
mov  dword [R_0],eax         ;3
opdesc MOVLLG,    0xFF,0xFF,0xFF,1,0xFF


opfunc MOVBSG
mov esi, dword [R_0]     ;3
GET_BYTE_IMM edi
add edi, dword [GBR]
CALL_SETMEM_BYTE
opdesc MOVBSG,    0xFF,0xFF,0xFF,5,0xFF

opfunc MOVWSG
mov esi, dword [R_0]
GET_BYTE_IMM edi
shl  edi,byte 1       ;3  Shift left 2
add edi, dword [GBR]
CALL_SETMEM_WORD
opdesc MOVWSG,    0xFF,0xFF,0xFF,5,0xFF

opfunc MOVLSG
mov esi, dword [R_0]     ;3
GET_BYTE_IMM edi
shl edi,byte 2       ;3  Shift left 2
add edi, dword [GBR]
CALL_SETMEM_LONG
opdesc MOVLSG,    0xFF,0xFF,0xFF,5,0xFF


opfunc MOVBS
GET_R SCRATCH1
mov esi, dword [SCRATCH1]
GET_R SCRATCH1
mov edi, dword [SCRATCH1]
CALL_SETMEM_BYTE
opdesc MOVBS,	6,16,0xFF,0xFF,0xFF


opfunc MOVWS
GET_R SCRATCH1
mov esi, dword [SCRATCH1]
GET_R SCRATCH1
mov edi, dword [SCRATCH1]
CALL_SETMEM_WORD
opdesc MOVWS,	6,16,0xFF,0xFF,0xFF

opfunc MOVLS
GET_R SCRATCH1
mov esi, dword [SCRATCH1]
GET_R SCRATCH1
mov edi, dword [SCRATCH1]
CALL_SETMEM_LONG
opdesc MOVLS,	6,16,0xFF,0xFF,0xFF


opfunc MOVR
GET_R SCRATCH1
mov eax,dword [SCRATCH1]       ;3
GET_R SCRATCH1
mov dword [SCRATCH1],eax       ;3
opdesc MOVR,		6,16,0xFF,0xFF,0xFF

opfunc MOVBM
GET_R SCRATCH1 
mov esi, dword [SCRATCH1]
GET_R SCRATCH1 
sub  dword [SCRATCH1],byte 1  ;4
mov edi, dword [SCRATCH1]
CALL_SETMEM_BYTE
opdesc MOVBM,		6,16,0xFF,0xFF,0xFF

opfunc MOVWM
GET_R SCRATCH1 
mov esi, dword [SCRATCH1]
GET_R SCRATCH1 
sub  dword [SCRATCH1],byte 2  ;4
mov edi, dword [SCRATCH1]
CALL_SETMEM_WORD
opdesc MOVWM,		6,16,0xFF,0xFF,0xFF

opfunc MOVLM
GET_R SCRATCH1  
mov esi,dword [SCRATCH1]         ;3 Set data
GET_R SCRATCH1
sub  dword [SCRATCH1],byte 4  ;4
mov edi, dword [SCRATCH1]         ;3 Set addr
CALL_SETMEM_LONG
opdesc MOVLM,		6,16,0xFF,0xFF,0xFF

;------------- added ------------------

opfunc TAS
GET_R SCRATCH1
mov edi, dword [SCRATCH1]         ;3
CALL_GETMEM_BYTE
CLEAR_T
test eax,eax             ;3
jne  NOT_ZERO            ;2
SET_T
NOT_ZERO:              
or   al, 0x80        ;3
mov  esi, eax
mov  edi, dword [SCRATCH1]          ;3
CALL_SETMEM_BYTE
opdesc TAS,  0xFF,6,0xFF,0xFF,0xFF


; sub with overflow check
opfunc SUBV
GET_R SCRATCH1
mov  eax,dword [SCRATCH1]        ;3
GET_R SCRATCH1
CLEAR_T
sub  dword [SCRATCH1],eax        ;3 R[n] = R[n] - R[m]
jno	 NO_OVER_FLOS     ;2
SET_T
NO_OVER_FLOS:
opdesc SUBV, 6,16,0xFF,0xFF,0xFF


; string cmp
opfunc CMPSTR
GET_R SCRATCH1
mov  eax,[SCRATCH1]        ;3
GET_R SCRATCH1
mov  ecx,[SCRATCH1]        ;3
xor  eax,ecx          ;2  
CLEAR_T
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
SET_T
ENDPROC:
opdesc CMPSTR, 6,16,0xFF,0xFF,0xFF

;-------------------------------------------------------------
;div0s 
opfunc DIV0S
GET_R SCRATCH1
CLEAR_Q
mov  eax, 0                  ;5 Zero Clear rax     
bt dword [SCRATCH1],31  ;7 Test sign
jnc   continue                ;2 if ZF = 1 then goto NO_SIGN
SET_Q
inc  eax                     ;1 
continue:
GET_R SCRATCH1
CLEAR_M
bt dword [SCRATCH1],31  ;7 Test sign
jnc   continue2               ;2 if ZF = 1 then goto NO_SIGN
SET_M
inc  eax                     ;1
continue2:
CLEAR_T
test eax, 1                  ;5 if( Q != M ) SetT(1)
je  continue3                ;2
SET_T
continue3:
opdesc DIV0S, 45,6,0xFF,0xFF,0xFF
 

;===============================================================
; DIV1   1bit Divid operation
; 
; size = 69 + 135 + 132 + 38 = 374 
;===============================================================
opfunc DIV1

; 69
GET_R r8                    ;2 r8 = @R[n]
GET_SR ecx		    ;ecx = old SR
mov  eax,dword [r8]         ;3 R[n]

bt eax,31          ;5 
jnc   NOZERO                  ;2
SET_Q
jmp  CONTINUE                ;3
NOZERO:
CLEAR_Q

CONTINUE:

; sh2i->R[n] |= (DWORD)(sh2i->i_get_T())
GET_SR eax  
and  eax, 0x01               ;5
shl  dword [r8], byte 1     ;3
or   dword [r8],eax         ;3

;Get R[n],R[m]
mov  eax,dword [r8]               ;3 R[n]
GET_R SCRATCH1
mov  r9d,dword [SCRATCH1]               ;3 R[m]

;switch( old_q )
bt ecx,8          ;6 old_q == 1 ?
jc   Q_FLG_TMP              ;2

;----------------------------------------------------------
; 8 + 62 + 3 + 62 = 135
NQ_FLG: 

bt ecx,9          ;6 M == 1 ?
jc  NQ_M_FLG                ;2

	;--------------------------------------------------
	; 62
	NQ_NM_FLG:  
	  sub dword [r8],r9d         ;3 sh2i->R[n] -= sh2i->R[m]
    
	  TEST_IS_Q
	  jc NQ_NM_Q_FLG                  ;2

	  NQ_NM_NQ_FLG:
	  cmp dword [r8],eax        ;3 tmp1 = (sh2i->R[n]>tmp0);
	  jna NQ_NM_NQ_00_FLG  ;2

		  NQ_NM_NQ_01_FLG:
		  SET_Q
		  jmp END_DIV1                 ;3

		  NQ_NM_NQ_00_FLG:
		  CLEAR_Q
		  jmp END_DIV1                 ;3  
  
	  NQ_NM_Q_FLG:
	  cmp dword [r8],eax        ;3 tmp1 = (sh2i->R[n]>tmp0);
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
	  add  dword [r8],r9d        ; sh2i->R[n] += sh2i->R[m]  
	  TEST_IS_Q
	  jc NQ_M_Q_FLG

	  NQ_M_NQ_FLG:
	  cmp dword [r8],eax         ; tmp1 = (sh2i->R[n]<tmp0);
	  jnb NQ_M_NQ_00_FLG

		  NQ_M_NQ_01_FLG:
		  CLEAR_Q
		  jmp END_DIV1

		  NQ_M_NQ_00_FLG:
		  SET_Q
		  jmp END_DIV1


	  NQ_M_Q_FLG:
	  cmp dword [r8],eax                    ; tmp1 = (sh2i->R[n]<tmp0);
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


bt ecx,9 ; M == 1 ?
jc  Q_M_FLG

	;--------------------------------------------------
	Q_NM_FLG:
	  add dword [r8],r9d         ; sh2i->R[n] += sh2i->R[m]
	  TEST_IS_Q
	  jc Q_NM_Q_FLG

	  Q_NM_NQ_FLG:
	  cmp dword [r8],eax      ; tmp1 = (sh2i->R[n]<tmp0);
	  ja Q_NM_NQ_00_FLG

		  Q_NM_NQ_01_FLG:
		  SET_Q
		  jmp END_DIV1

		  Q_NM_NQ_00_FLG:
		  CLEAR_Q
		  jmp END_DIV1
  
	  Q_NM_Q_FLG:
	  cmp dword [r8],eax      ; tmp1 = (sh2i->R[n]<tmp0);
	  ja Q_NM_NQ_10_FLG  

		  Q_NM_NQ_11_FLG:
		  CLEAR_Q
		  jmp END_DIV1

		  Q_NM_NQ_10_FLG:
		  SET_Q
		  jmp END_DIV1

	;----------------------------------------------------  
	Q_M_FLG:
	  sub dword [r8],r9d      ; sh2i->R[n] -= sh2i->R[m]  
	  TEST_IS_Q 
	  jc Q_M_Q_FLG

	  Q_M_NQ_FLG:
	  cmp dword [r8],eax      ; tmp1 = (sh2i->R[n]>tmp0);
	  jb Q_M_NQ_00_FLG

		  Q_M_NQ_01_FLG:
		  CLEAR_Q
		  jmp END_DIV1

		  Q_M_NQ_00_FLG:
		  SET_Q
		  jmp END_DIV1
 
	  Q_M_Q_FLG:
	  cmp dword [r8],eax      ; tmp1 = (sh2i->R[n]>tmp0);
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
GET_M r9d
CLEAR_T
cmp  eax, r9d                  ;2
jne  NO_Q_M                   ;2
SET_T
NO_Q_M:
opdesc DIV1, 65,6,0xFF,0xFF,0xFF

;======================================================
; end of DIV1
;======================================================

;------------------------------------------------------------
;dmuls
opfunc DMULS
GET_R SCRATCH1
mov  eax,dword [SCRATCH1]    ;3
GET_R SCRATCH1
mov  edx,dword [SCRATCH1]    ;3  
imul edx                ;2
SET_MACH edx  ;2 store MACH             
SET_MACL eax  ;3 store MACL   
opdesc DMULS, 6,16,0xFF,0xFF,0xFF

;------------------------------------------------------------
;dmulu 32bit -> 64bit Mul
opfunc DMULU
GET_R SCRATCH1
mov  eax,dword [SCRATCH1]    ;3
GET_R SCRATCH1
mov  edx,dword [SCRATCH1]    ;3  
mul  edx                ;2
SET_MACH edx  ;2 store MACH             
SET_MACL eax  ;3 store MACL     
opdesc DMULU, 6,16,0xFF,0xFF,0xFF

;--------------------------------------------------------------
; mull 32bit -> 32bit Multip
opfunc MULL
GET_R SCRATCH1
mov  eax,dword [SCRATCH1]    ;3
GET_R SCRATCH1
mov  edx,dword [SCRATCH1]    ;3  
imul edx                ;2
SET_MACL eax  ;3 store MACL  
opdesc MULL, 6,16,0xFF,0xFF,0xFF

;--------------------------------------------------------------
; muls 16bit -> 32 bit Multip
opfunc MULS
GET_R SCRATCH1
mov  ax,word [SCRATCH1]     ;3
GET_R SCRATCH1
mov  dx,word [SCRATCH1]     ;3  
imul dx                ;2
shl  edx, byte 16      ;3
add  dx, ax            ;2
SET_MACL edx ;3 store MACL   
opdesc MULS, 6,17,0xFF,0xFF,0xFF

;--------------------------------------------------------------
; mulu 16bit -> 32 bit Multip
opfunc MULU
GET_R SCRATCH1
mov  ax,word [SCRATCH1]     ;3
GET_R SCRATCH1
mov  dx,word [SCRATCH1]     ;3  
mul dx                ;2
shl  edx, byte 16      ;3
add  dx, ax            ;2
SET_MACL edx ;3 store MACL   
opdesc MULU, 6,17,0xFF,0xFF,0xFF

;--------------------------------------------------------------
; MACL   ans = 32bit -> 64 bit MUL
;        (MACH << 32 + MACL)  + ans 
;------------------------------------------------------------- 
opfunc MAC_L
GET_R SCRATCH1
mov  edi,dword [SCRATCH1]          ;3
CALL_GETMEM_LONG
mov  edx,eax                  ;2
add  dword [SCRATCH1], 4           ;7 R[n] += 4
GET_R SCRATCH1
mov  edi,dword [SCRATCH1]          ;3
CALL_GETMEM_LONG
add  dword [SCRATCH1], 4           ;7 R[m] += 4 
GET_MACL ecx            ;3 load macl
imul edx                      ;2 eax <- low, edx <- high
GET_MACH edi               ;3 load mach
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
opdesc MAC_L, 6,30,0xFF,0xFF,0xFF


;--------------------------------------------------------------
; MACW   ans = 32bit -> 64 bit MUL
;        (MACH << 32 + MACL)  + ans 
;-------------------------------------------------------------
opfunc MAC_W
GET_R SCRATCH1
mov  edi,dword [SCRATCH1]          ;3
CALL_GETMEM_WORD
movsx  edx,ax                 ;3
add  dword [SCRATCH1], 2           ;7 R[n] += 2
GET_R SCRATCH1
mov  edi,dword [SCRATCH1]          ;3
CALL_GETMEM_WORD
add  dword [SCRATCH1], 2           ;7 R[m] += 2
cwde                          ;1 Sigin extention
imul edx                      ;2 rax <- low, rdx <- high
TEST_IS_S
jnc   MACW_NO_S_FLG                 ;2 if( S == 0 ) goto 'no sign proc'

MACW_S_FLG:
  add dword [MACL],eax   ;3 MACL = ansL + MACL
  jno NO_OVERFLO
  js  FU 
  SEI:
    mov dword [MACL],0x80000000 ; min value
	or  dword [MACH],1
	jmp END_MACW

  FU:
    mov dword [MACL],0x7FFFFFFF ; max value
	or dword [MACH],1
	jmp END_MACW

  NO_OVERFLO:
  jmp END_MACW

MACW_NO_S_FLG:
  add dword [MACL],eax         ;3 MACL = ansL + MACL
  jnc MACW_NO_CARRY             ;2 Check Carry
  inc edx                       ;1
MACW_NO_CARRY: 
  add dword [MACH],edx           ;2 MACH = ansH + MACH

END_MACW:
opdesc MAC_W, 6,31,0xFF,0xFF,0xFF
  
 
end:
