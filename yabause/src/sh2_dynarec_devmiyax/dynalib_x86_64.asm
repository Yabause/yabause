;        Copyright 2019 devMiyax(smiyaxdev@gmail.com)
;
;This file is part of YabaSanshiro.
;
;        YabaSanshiro is free software; you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation; either version 2 of the License, or
;(at your option) any later version.
;
;YabaSanshiro is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.
;
;        You should have received a copy of the GNU General Public License
;along with YabaSanshiro; if not, write to the Free Software
;Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

; https://learn.microsoft.com/ja-jp/windows-hardware/drivers/debugger/x64-architecture
; RAX  揮発性 戻り値
; RBX  <- [CtrlReg] 不揮発性
; RCX  揮発性 第一引数
; RDX  <- 揮発性 第二引数
; RSI  <- [SysReg] 不揮発性
; RDI  <- [GenReg] 不揮発性
; RBP  不揮発性
; RSP  不揮発性
; R8 揮発性 第３引数
; R9 揮発性 第４引数
; R10 揮発性
; R11 揮発性
; R12 [PC] 不揮発性  
; R13 不揮発性  
; R14 [Jump Address] 不揮発性 ここのアドレスが入っている場合、ジャンプする  
; R15 不揮発性

;最初の 4 つの整数またはポインター パラメーターは 、rcx、 rdx、 r8、r9 レジスタ で 渡されます。
;最初の 4 つの浮動小数点パラメーターは、最初の 4 つの SSE レジスタ xmm0xmm3 で-渡されます。
;呼び出し元は、レジスタに渡される引数の領域をスタックに予約します。 呼び出された関数は、この領域を使用して、レジスタの内容をスタックに書き込む可能性があります。
;追加の引数はスタックに渡されます。
;rax レジスタでは整数またはポインターの戻り値が 返 され、浮動小数点の戻り値は xmm0 で返されます。
;rax、rcx、rdx、r8r11-は揮発性です。
;rbx、rbp、rdi、rb、r12r15- は不揮発性です。



bits 64

section .data
struc tagSH2
	GenReg:  resd 16 
	CtrlReg:  resd 4
	SysReg:  resd 3 
endstruc

extern memGetByte, memGetWord, memGetLong
extern memSetByte, memSetWord, memSetLong
extern EachClock, DelayEachClock, DebugEachClock, DebugDelayClock

section .code

;Memory Functions

%macro pushaq 0
    push rbx
    push rbp
    push rdi	
    push rsi
	push r12
	push r14
	sub rsp,64
%endmacro # pushaq


%macro popaq 0
	add RSP,64
    pop r14
	pop r12
	pop rsi
	pop rdi
	pop rbp
    pop rbx
%endmacro # popaq


%macro opfunc 1
	global x86_%1
	x86_%1:
%endmacro

%macro opdesc 7
	global %1_size
	%1_size dw %2
	global %1_src
	%1_src db %3
	global %1_dest
	%1_dest db %4
	global %1_off1
	%1_off1 db %5
	global %1_imm
	%1_imm db %6
	global %1_off3
	%1_off3 db %7
%endmacro

%macro ctrlreg_load 1	;5
	mov rbp,rdi            ; 2
	add rbp,byte 64+(%1*4) ; 3
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
; SR => [rbx]
; PC => [edx]
; GenReg => edi
; SysReg => esi
; Size = 27 Bytes
global prologue
prologue:
pushaq
mov r14, 00      ;4 (JumpAddr)
mov rdi,rcx        ;2 (GenReg)
add rcx,byte 64    ;3
mov rbx,rcx        ;2 (SR)
add rcx,byte 12    ;3
mov rsi,rcx        ;2 (SysReg)
mov r12,rsi        ;2
add r12,byte 12    ;3 (PC)


;-------------------------------------------------------
; normal part par instruction
;Size = 7 Bytes
global seperator_normal
seperator_normal:
add dword [r12], byte 2   ;3 PC += 2
add dword [r12+4], byte 1 ;4 Clock += 1

;------------------------------------------------------
; Delay slot part par instruction
;Size = 17 Bytes
global seperator_delay_slot
seperator_delay_slot:
test dword r14d, 0xFFFFFFFF ; 7
jnz   .continue               ; 2
add dword [r12], byte 2      ; 3 PC += 2
add dword [r12+4], byte 1    ; 4 Clock += 1
popaq                        ; 1
ret                          ; 1
.continue
mov  eax,r14d             ; 3
sub  eax,byte 2            ; 3
mov  [r12],eax             ; 2



;------------------------------------------------------
; End part of delay slot
;Size = 24 Bytes
global seperator_delay_after
seperator_delay_after:
add dword [r12], byte 2   ;3 PC += 2
add dword [r12+4], byte 1 ;4 Clock += 1
popaq                     ; 1
ret                       ; 1

;-------------------------------------------------------
; End of block
; Size = 3 Bytes
global epilogue
epilogue:
popaq           ;1
ret             ;1

;------------------------------------------------------
; Jump part
; Size = 17 Bytes
global PageFlip
PageFlip:
test r14d, 0xFFFFFFFF ; 7
jz   .continue               ; 2
mov  eax,r14d               ; 3
mov  [r12],eax               ; 2
popaq                        ; 1
ret                          ; 1
.continue


;-------------------------------------------------------
; normal part par instruction( for debug build )
;Size = 24 Bytes
global seperator_d_normal
seperator_d_normal:
add dword [r12+4],byte 1 ;4 Clock += 1
mov  rax,DebugEachClock ;5
call rax                 ;2
test eax, 0x01           ;5 finish 
jz  NEXT_D_INST          ;2
popaq                    ;1
ret                      ;1
NEXT_D_INST:
add dword [r12],byte 2   ;3 PC += 2

;------------------------------------------------------
; Delay slot part par instruction( for debug build )
;Size = 34 Bytes
global seperator_d_delay
seperator_d_delay:
mov  rax,DebugDelayClock ;5
call rax                   ;2
test r14d, 0xFFFFFFFF ; 7
jnz   .continue               ; 2
add dword [r12], byte 2      ; 3 PC += 2
add dword [r12+4], byte 1    ; 4 Clock += 1
popaq                        ; 1
ret                          ; 1
.continue
mov  eax,r14d             ; 3
sub  eax,byte 2            ; 3
mov  [r12],eax             ; 2

;=================
;Begin x86 Opcodes
;=================

opdesc CLRT,	3,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc CLRT
and dword [rbx],byte 0xfe

opdesc CLRMAC,	7,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc CLRMAC
and dword [rsi],byte 0   ;4
and dword [rsi+4],byte 0 ;4

opdesc NOP,		1,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc NOP
nop

opdesc DIV0U,	7,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc DIV0U
and dword [rbx],0xfffffcfe

opdesc SETT,	4,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc SETT
or [rbx],byte 1

opdesc SLEEP,	34,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc SLEEP
add dword [r12+4],byte 1   ;4 
mov  rax,EachClock ;5
call rax            ;2
popaq               ;1
ret                 ;1


opdesc SWAP_W,	23,6,19,0xff,0xff,0xff
opfunc SWAP_W
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
rol eax,16          ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;2

opdesc SWAP_B,	22,6,18,0xff,0xff,0xff
opfunc SWAP_B
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
xchg ah,al          ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;2


opdesc TST,	33,6,16,0xff,0xff,0xff
opfunc TST
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
test dword [rbp],eax      ;2
getflag 6           ;7
and dword [rbx],byte 0xfe ;3
or dword [rbx],eax        ;2

opdesc TSTI,	24,0xff,0xff,0xff,7,0xff
opfunc TSTI
mov  rbp,rdi        ;2
mov  eax,[rbp]      ;3
test eax,$00        ;5  Imidiate Val
getflag 6           ;7
and dword [rbx],byte 0xfe ;3
or [rbx],eax        ;2


opdesc ANDI,	10,0xff,0xff,0xff,6,0xff
opfunc ANDI
mov rbp,rdi         ;2
xor eax,eax         ;2
mov al,byte $00     ;2
and dword [rbp],eax ;3

opdesc XORI,	10,0xff,0xff,0xff,6,0xff
opfunc XORI
mov rbp,rdi         ;2
xor eax,eax         ;2
mov al,byte $00     ;2
xor dword [rbp],eax ;3

opdesc ORI,	10,0xff,0xff,0xff,6,0xff
opfunc ORI
mov rbp,rdi         ;2
xor eax,eax         ;2
mov al,byte $00     ;2
or dword [rbp],eax  ;3

opdesc CMP_EQ_IMM,	26,0xff,0xff,0xff,9,0xff
opfunc CMP_EQ_IMM
mov rbp,rdi         ;2
mov eax,[rbp]       ;3
cmp dword [rbp],byte $00 ;4
getflag 6           ;7
and dword [rbx], 0xFFFFFFE ;6
or [rbx],eax        ;2 

opdesc XTRCT,	27,6,16,0xff,0xff,0xff
opfunc XTRCT
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
shl eax,16          ;3
shr dword [rbp],16  ;3
or [rbp],eax        ;2

opdesc ADD,		20,6,16,0xff,0xff,0xff
opfunc ADD
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
add [rbp],eax       ;2

opdesc ADDC,	37,6,16,0xff,0xff,0xff
opfunc ADDC
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;3
mov rbp,rdi         ;2
add rbp,byte $00    ;3
bts dword [rbx],0   ;4
adc [rbp],eax       ;3
jnc	ADDC_NO_CARRY   ;2
or [rbx], byte 1    ;3
jmp ADDC_END        ;2
ADDC_NO_CARRY:
and dword [rbx],byte 0xfe ;3
ADDC_END:


; add with overflow check
opdesc ADDV, 28,6,16,0xff,0xff,0xff
opfunc ADDV
mov  rbp,rdi          ;2
add  rbp,byte $00     ;3
mov  eax,[rbp]        ;3
mov  rbp,rdi          ;2
add  rbp,byte $00     ;3
and  [rbx], byte 0xFE ;3
add  [rbp],eax        ;3
jno	 NO_OVER_FLO      ;2
or   [rbx], byte 01   ;3
NO_OVER_FLO

opdesc SUBC,	37,6,16,0xff,0xff,0xff
opfunc SUBC
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;3
mov rbp,rdi         ;2
add rbp,byte $00    ;3
bts dword [rbx],0   ;4
sbb [rbp],eax       ;3
jnc	non_carry       ;2
or [rbx], byte 1    ;3
jmp SUBC_END        ;2
non_carry:
and dword [rbx],byte 0xfe ;3
SUBC_END:


opdesc SUB,		20,6,16,0xff,0xff,0xff
opfunc SUB
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
sub [rbp],eax       ;2


opdesc NOT,		22,6,18,0xff,0xff,0xff
opfunc NOT
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
not eax             ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;2

opdesc NEG,		22,6,18,0xff,0xff,0xff
opfunc NEG
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
neg eax             ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;2

opdesc NEGC,	54,6,18,0xff,0xff,0xff
opfunc NEGC
mov rbp,rdi               ;2
add rbp,byte $00          ;3
mov ecx,[rbp]             ;3
neg ecx                   ;2
mov rbp,rdi               ;2
add rbp,byte $00          ;3
mov [rbp],ecx             ;3
mov eax,[rbx]             ;2
and dword eax,1           ;5
sub [rbp],eax             ;3
and dword [rbx],byte 0xfe ;3
cmp ecx,0                 ;5
jna NEGC_NOT_LESS_ZERO    ;2
or [rbx], byte 1          ;2
NEGC_NOT_LESS_ZERO:
cmp [rbp],ecx             ;3  
jna NEGC_NOT_LESS_OLD     ;2
or  [rbx], byte 1         ;2
NEGC_NOT_LESS_OLD:

opdesc EXTUB,	25,6,21,0xff,0xff,0xff
opfunc EXTUB
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;3
and dword eax,0x000000ff  ;5
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;3

opdesc EXTU_W,	25,6,21,0xff,0xff,0xff
opfunc EXTU_W
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;3
and dword eax,0xffff;5
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;3

opdesc EXTS_B,	23,6,19,0xff,0xff,0xff
opfunc EXTS_B
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
cbw                 ;2
cwde                ;1
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;3

opdesc EXTS_W,	21,6,17,0xff,0xff,0xff
opfunc EXTS_W
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
cwde                ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;2

;Store Register Opcodes
;----------------------

opdesc STC_SR_MEM,	28,0xff,6,0xff,0xff,0xff
opfunc STC_SR_MEM
mov  rbp,rdi            ;2
add  rbp,byte $00       ;3
mov  edx, [rbx]        ;2
sub  dword [rbp],byte 4 ;4
mov  ecx, [rbp]        ;3
mov  rax,memSetLong    ;5
call rax                ;2


opdesc STC_GBR_MEM,	36,0xff,16,0xff,0xff,0xff
opfunc STC_GBR_MEM
mov  rbp,rdi            ;2
add  rbp,byte 68        ;3
mov  edx,[rbp]          ;3
mov  rbp,rdi            ;2
add  rbp,byte $00       ;3
sub  dword [rbp],byte 4 ;4
mov  ecx, [rbp]        ;3
mov  rax,memSetLong    ;5
call rax                ;2


opdesc STC_VBR_MEM,	36,0xff,16,0xff,0xff,0xff
opfunc STC_VBR_MEM
mov  rbp,rdi            ;2
add  rbp,byte 72        ;3
mov  edx,[rbp]          ;3
mov  rbp,rdi            ;2
add  rbp,byte $00       ;3
sub  dword [rbp],byte 4 ;4
mov  ecx, [rbp]        ;3
mov  rax,memSetLong    ;5
call rax                ;2


;------------------------------

opdesc MOVBL,	35,6,28,0xff,0xff,0xff
opfunc MOVBL
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov ecx,[rbp]       ;3
mov rax,memGetByte ;5
call rax            ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
cbw                 ;1
cwde                ;1
mov [rbp],eax       ;3


opdesc MOVWL,		33,6,28,0xff,0xff,0xff
opfunc MOVWL
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov ecx,[rbp]       ;3
mov rax,memGetWord ;5
call rax            ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
cwde                ;1
mov [rbp],eax       ;3


opdesc MOVL_MEM_REG,		32,6,28,0xff,0xff,0xff
opfunc MOVL_MEM_REG
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov ecx,[rbp]       ;3
mov rax,memGetLong ;5
call rax            ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;3


opdesc MOVBP,	38,6,31,0xff,0xff,0xff
opfunc MOVBP
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov ecx,[rbp]       ;3
inc dword [rbp]     ;3
mov rax,memGetByte ;5
call rax            ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
cbw                 ;1
cwde                ;1
mov [rbp],eax       ;3

opdesc MOVWP,	37,6,32,0xff,0xff,0xff
opfunc MOVWP
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov ecx,[rbp]       ;3
add dword [rbp],byte 2 ;4
mov rax,memGetWord ;5
call rax            ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
cwde                ;1
mov [rbp],eax       ;3


opdesc MOVLP,	36,6,32,0xff,0xff,0xff
opfunc MOVLP
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov ecx,[rbp]       ;3
add dword [rbp],byte 4 ;4
mov rax,memGetLong ;5
call rax            ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;3

;?????? need to check
opdesc MOVW_A, 47,0xff,6,0xff,16,0xff
opfunc MOVW_A
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov ecx,[r12]       ;2
and ecx,byte 0xfc   ;3
mov r13d,ecx
and ecx,byte 0      ;3
mov cl,byte $00     ;3
shl ecx,byte 2      ;3
add ecx,r13d		 ;2
add ecx,byte 4      ;3
mov rax,memGetWord ;3
call rax            ;3
cwde                ;1
mov [rbp],eax       ;2


opdesc MOVL_A,	51,0xff,6,0xff,16,0xff
opfunc MOVL_A
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[r12]       ;2
and eax,byte 0xfc   ;3
mov r13d, eax
and eax,byte 0      ;3
mov al,byte $00     ;3
shl eax,byte 2      ;3
add eax, r13d       ;2
add eax,byte 4      ;3
mov edx, r12d        ;1
mov ecx, eax        ;1
mov rax,memGetLong  ;3
call rax            ;3
mov [rbp],eax       ;2

opdesc MOVI,	15,0xff,6,0xff,11,0xff
opfunc MOVI
mov rbp,rdi         ;2
add rbp,byte $00    ;3
xor eax,eax         ;2
or eax,byte 00      ;3
mov [rbp],eax       ;3

;----------------------

opdesc MOVBL0,	37,6,30,0xff,0xff,0xff
opfunc MOVBL0
mov rbp,rdi          ;2
add rbp,byte $00     ;3
mov ecx,[rbp]        ;3
add ecx,[rdi]        ;2
mov  rax,memGetByte ;5
call rax             ;2
mov rbp,rdi          ;2
add rbp,byte $00     ;3
cbw                  ;1
cwde                 ;1
mov [rbp],eax        ;3

opdesc MOVWL0,	35,6,30,0xff,0xff,0xff
opfunc MOVWL0
mov rbp,rdi          ;2
add rbp,byte $00     ;3
mov ecx,[rbp]        ;3
add ecx,[rdi]        ;2
mov rax,memGetWord ;5
call rax             ;2
mov rbp,rdi          ;2
add rbp,byte $00     ;3
cwde                 ;1
mov [rbp],eax        ;3


opdesc MOVLL0,	34,6,30,0xff,0xff,0xff
opfunc MOVLL0
mov rbp,rdi          ;2
add rbp,byte $00     ;3
mov ecx,[rbp]        ;3
add ecx,[rdi]        ;2
mov  rax,memGetLong ;5
call rax             ;2
mov rbp,rdi          ;2
add rbp,byte $00     ;3
mov [rbp],eax        ;3


opdesc MOVT,		17,0xff,6,0xff,0xff,0xff
opfunc MOVT
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbx]       ;2
and dword eax,1     ;5
mov [rbp],eax       ;3

opdesc MOVBS0,	34,6,16,0xff,0xff,0xff
opfunc MOVBS0
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3 m(4..7)
mov  edx,[rbp]       ;3
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3 n(8..11)
mov  ecx,[rbp]       ;3
add  ecx,[rdi]       ;2
mov  rax,memSetByte  ;5
call rax             ;2


opdesc MOVWS0, 34,6,16,0xff,0xff,0xff
opfunc MOVWS0
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3 m(4..7)
mov  edx, [rbp]     ;3
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3 n(8..11)
mov  ecx,[rbp]       ;3
add  ecx,[rdi]       ;2
mov  rax,memSetWord ;5
call rax             ;2

opdesc MOVLS0,	34,6,16,0xff,0xff,0xff
opfunc MOVLS0
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3 m(4..7)
mov  edx, [rbp]     ;3
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3 n(8..11)
mov  ecx,[rbp]       ;3
add  ecx,[rdi]       ;2
mov  rax,memSetLong ;5
call rax             ;2

;===========================================================================
;Verified Opcodes
;===========================================================================

opdesc DT,		25,0xff,6,0xff,0xff,0xff
opfunc DT
mov rbp,rdi         ;2
add rbp,byte $00    ;3
and dword [rbx],byte 0xfe ;3
dec dword [rbp]     ;3
cmp dword [rbp],byte 0 ;4
jne .continue       ;2
or dword [rbx], 1 ;3
.continue

opdesc CMP_PZ,	22,0xff,6,0xff,0xff,0xff
opfunc CMP_PZ
mov rbp,rdi               ;2
add rbp,byte $00          ;3
and dword [rbx],byte 0xfe ;3
cmp dword [rbp],byte 0    ;4
jl .continue              ;2
or dword [rbx], 1          ;3
.continue

opdesc CMP_PL,	28,0xff,6,0xff,0xff,0xff
opfunc CMP_PL
mov rbp,rdi                 ;2
add rbp,byte $00            ;3
and dword [rbx], 0xFFFFFFFe ;6
cmp dword [rbp], 0          ;7
jle .continue               ;2
or dword [rbx],  1          ;3 Set T Flg
.continue

opdesc CMP_EQ,	31,6,16,0xff,0xff,0xff
opfunc CMP_EQ
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
and dword [rbx],byte 0xfe ;3
cmp [rbp],eax       ;3
jne .continue       ;2  
or dword [rbx], 1     ;3 [edp] == eax
.continue

opdesc CMP_HS,	34,6,16,0xff,0xff,0xff
opfunc CMP_HS
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;3
mov rbp,rdi         ;2
add rbp,byte $00    ;3
and dword [rbx], 0xFFFFFFFe ;6
cmp dword [rbp],eax       ;3
jb .continue        ;2
or dword [rbx], 1     ;3 
.continue

opdesc CMP_HI,	34,6,16,0xff,0xff,0xff
opfunc CMP_HI
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
and dword [rbx], 0xFFFFFFFe ;6
cmp [rbp],eax       ;3
jbe .continue       ;2
or dword [rbx], 1     ;3 
.continue

opdesc CMP_GE,	34,6,16,0xff,0xff,0xff
opfunc CMP_GE
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
and dword [rbx], 0xFFFFFFFe ;6
cmp [rbp],eax       ;3
jl .continue        ;2
or dword [rbx], 1     ;3 
.continue

opdesc CMP_GT,	34,6,16,0xff,0xff,0xff
opfunc CMP_GT
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
and dword [rbx], 0xFFFFFFFe ;6
cmp [rbp],eax       ;3
jle .continue       ;2
or dword [rbx], 1     ;3 
.continue

opdesc ROTL,	28,0xff,6,0xff,0xff,0xff
opfunc ROTL
mov rbp,rdi               ;2
add rbp,byte $00          ;3
mov eax,[rbp]             ;2
shr eax,byte 31           ;3
and dword [rbx], 0xFFFFFFFe ;6
or [rbx],eax              ;2
shl dword [rbp],byte 1    ;4
or [rbp],eax              ;2

opdesc ROTR,	30,0xff,6,0xff,0xff,0xff
opfunc ROTR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
and eax,byte 1      ;3
and dword [rbx], 0xFFFFFFFe ;6
or [rbx],eax        ;2
shr dword [rbp],1   ;2
shl eax,byte 31     ;3
or [rbp],eax        ;2

opdesc ROTCL,	35,0xff,6,0xff,0xff,0xff
opfunc ROTCL
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbx]       ;2
and dword eax,1     ;5
mov ecx,[rbp]       ;2
shr ecx,byte 31     ;3
and dword [rbx], 0xFFFFFFFe ;6
or  dword [rbx],ecx        ;2
shl dword [rbp],byte 1 ;4
or  dword [rbp],eax        ;2


opdesc ROTCR,	38,0xff,6,0xff,0xff,0xff
opfunc ROTCR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbx]       ;2
and dword eax,1     ;5
shl eax,byte 31     ;3
mov ecx,[rbp]       ;2
and dword ecx,1     ;5
and dword [rbx],byte 0xfe ;3
or [rbx],ecx        ;2
shr dword [rbp],byte 1 ;4
or [rbp],eax        ;2

opdesc SHL,		22,0xff,6,0xff,0xff,0xff
opfunc SHL
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;3
shr eax,byte 31     ;3
and dword [rbx],byte 0xfe ;3
or [rbx],eax        ;2
shl dword [rbp],byte 1 ;4

opdesc SHLR,	27,0xff,6,0xff,0xff,0xff
opfunc SHLR
mov rbp,rdi                 ;2
add rbp,byte $00            ;3
mov eax,[rbp]               ;3
and dword eax,1             ;5
and dword [rbx], 0xFFFFFFFE ;6
or [rbx],eax                ;2
shr dword [rbp],byte 1      ;4

;opdesc SHAR,	33,0xff,6,0xff,0xff,0xff
;opfunc SHAR
;mov rbp,rdi         ;2
;add rbp,byte $00    ;3
;mov eax,[rbp]       ;2
;and dword eax,1     ;5
;and dword [rbx],byte 0xfe ;3
;or [rbx],eax        ;2
;mov eax,[rbp]       ;2
;and eax,0xffx80000000  ;5
;shr dword [rbp],byte 1 ;4
;or [rbp],eax        ;2

opdesc SHAR,	24,0xff,6,0xff,0xff,0xff
opfunc SHAR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
and dword eax,1     ;5
and dword [rbx],byte 0xfe ;3
or  [rbx],eax          ;2
sar dword [rbp],byte 1 ;4



opdesc SHLL2,	11,0xff,6,0xff,0xff,0xff
opfunc SHLL2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
shl dword [rbp],byte 2 ;4

opdesc SHLR2,	11,0xff,6,0xff,0xff,0xff
opfunc SHLR2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
shr dword [rbp],byte 2 ;4

opdesc SHLL8,	11,0xff,6,0xff,0xff,0xff
opfunc SHLL8
mov rbp,rdi         ;2
add rbp,byte $00    ;3
shl dword[rbp],byte 8  ;4

opdesc SHLR8,	11,0xff,6,0xff,0xff,0xff
opfunc SHLR8
mov rbp,rdi         ;2
add rbp,byte $00    ;3
shr dword [rbp],byte 8 ;4

opdesc SHLL16,	11,0xff,6,0xff,0xff,0xff
opfunc SHLL16
mov rbp,rdi         ;2
add rbp,byte $00    ;3
shl dword [rbp],byte 16 ;4

opdesc SHLR16,	11,0xff,6,0xff,0xff,0xff
opfunc SHLR16
mov rbp,rdi         ;2
add rbp,byte $00    ;3
shr dword [rbp],byte 16 ;4

opdesc AND,		20,6,16,0xff,0xff,0xff
opfunc AND
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
and [rbp],eax       ;2

opdesc OR,		20,6,16,0xff,0xff,0xff
opfunc OR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
or [rbp],eax        ;2

opdesc XOR,		20,6,16,0xff,0xff,0xff
opfunc XOR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte $00    ;3
xor [rbp],eax       ;3

opdesc ADDI,	11,0xff,6,0xff,10,0xff
opfunc ADDI
mov rbp,rdi         ;2
add rbp,byte $00    ;3
add dword [rbp],byte $00 ;4


opdesc AND_B,	42,0xff,0xff,0xff,26,0xff
opfunc AND_B
ctrlreg_load 1      ;5 GBR to rbp
mov ecx,[rbp]       ;3
add ecx, dword [rdi];3
push rcx
mov rax, memGetByte;5
call rax            ;2
and al,byte $00     ;2
pop rcx             ;  第一引数アドレス
mov edx,eax         ;3   第２引数値
mov rax, memSetByte;5
call rax            ;2



opdesc OR_B,	42,0xff,0xff,0xff,26,0xff
opfunc OR_B
ctrlreg_load 1      ;5 GBR to rbp
mov  ecx, [rbp]       ;3 Get GBR
add  ecx, dword [rdi] ;2 ADD R0
push rcx         ;2 Save Dist addr
mov  rax, memGetByte ;5
call rax              ;2
or   al,byte $00      ;2
pop  rcx             ;  第一引数アドレス
mov  edx,eax
mov  rax, memSetByte ;5
call rax              ;2


opdesc XOR_B,	42,0xff,0xff,0xff,26,0xff
opfunc XOR_B
ctrlreg_load 1      ;5 GBR to rbp
mov  ecx, [rbp]       ;3 Get GBR
add  ecx, dword [rdi] ;2 ADD R0
push rcx         ;2 Save Dist addr
mov  rax, memGetByte ;5
call rax              ;2
xor  al,byte $00      ;2
pop  rcx             ;  第一引数アドレス
mov  edx,eax
mov  rax, memSetByte ;5
call rax              ;2



opdesc TST_B,	40,0xff,0xff,0xff,32,0xff
opfunc TST_B
ctrlreg_load 1        ;5
mov  ecx,[rbp]        ;3 Get GBR
add  ecx, dword [edi] ;2 Add R[0]
mov  rax, memGetByte ;5
call rax              ;2
and  dword [rbx],0xFFFFFFFE ;6
and  al,byte $00      ;2
cmp  al,0             ;2
jne .continue         ;2
or dword [rbx],byte 1 ;3
.continue

;Jump Opcodes
;------------

opdesc JMP,		13,0xff,6,0xff,0xff,0xff
opfunc JMP
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov r14d, eax       ;3

opdesc JSR,		23,0xff,16,0xff,0xff,0xff
opfunc JSR
mov eax,[r12]       ;2
add eax,byte 4      ;3
mov [rsi+8],eax     ;3
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;3
mov r14d, eax       ;3

opdesc BRA,		31,0xff,0xff,0xff,0xff,5
opfunc BRA
and eax,byte 00     ;3
mov ax,0            ;4
shl eax,byte 1      ;3
cmp ax,0xfff        ;4
jle .continue       ;2
or eax,0xfffff000   ;5
.continue
add eax,byte 4      ;3
add eax,dword [r12] ;2
mov r14d, eax       ;3

opdesc BSR,		41,0xff,0xff,0xff,0xff,15
opfunc BSR
mov eax,[r12]       ;2
add eax,byte 4      ;3
mov [rsi+8],eax     ;3
and eax,byte 00     ;3
mov ax,0            ;4
shl eax,byte 1      ;3
cmp ax,0xfff        ;4
jle .continue       ;2
or eax,0xfffff000   ;5
.continue
add eax,byte 4      ;3
add eax,dword [r12] ;2
mov r14d, eax       ;3

opdesc BSRF,		30,0xff,6,0xff,0xff,0xff
opfunc BSRF
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[r12]       ;2
add eax,byte 4      ;3
mov [rsi+8],eax     ;3
mov eax,[r12]       ;3
add eax,dword [rbp] ;3
add eax,byte 4      ;3
mov r14d, eax       ;3

opdesc BRAF,		20,0xff,6,0xff,0xff,0xff
opfunc BRAF
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[r12]       ;2
add eax,dword [rbp] ;3
add eax,byte 4      ;4
mov r14d, eax       ;3


opdesc RTS,			6,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc RTS
mov eax,[rsi+8]     ;3
mov r14d, eax       ;3

opdesc RTE,			64,0xFF,0xFF,0xFF,0xFF,0xFF
opfunc RTE
mov  rbp,rdi            ;2
add  rbp,byte 60        ;3
mov  ecx, [rbp]        ;3
mov  rax,memGetLong    ;5
call rax                ;2
mov  r13d,eax  ;4  Set PC
add  dword [rbp], 4 ;4
mov  ecx, [rbp]        ;3
mov  rax,memGetLong    ;5  
call rax                ;2
and  eax,0x000003f3     ;5  Get SR
mov  [rbx],eax          ;2
add  dword [rbp], 4 ;4
mov  r14d,r13d  ;4  Set PC

opdesc TRAPA,	      92,0xFF,0xFF,0xFF,63,0xFF
opfunc TRAPA
mov  rbp,rdi          ;2
add  rbp,byte 60      ;3
sub  dword [rbp],4    ;7
mov edx, [rbx]      ;2 SR
mov ecx, [rbp]      ;3
mov  rax, memSetLong ;5
call rax              ;2
sub  dword [rbp],4    ;7
mov  edx,[r12]        ;3 PC
add  edx,byte 2       ;3
mov ecx, [rbp]      ;2
mov  rax, memSetLong ;5
call rax              ;2
xor  ecx,ecx          ;2
mov  cl,byte $00      ;2 Get Imm
shl  ecx,2            ;3
add  ecx,[ebx+8]      ;3 ADD VBR
mov  rax, memGetLong ;5
call rax              ;2
mov  [r12],eax        ;3
sub  dword [r12],byte 2     ;3



opdesc BT,		32,0xFF,0xFF,0xFF,18,0xFF
opfunc BT
and r14d,dword 0   ;7
bt dword [rbx],0    ;4
jnc .continue       ;2
and eax,byte 00     ;3
or  eax,byte 00     ;3
shl eax,byte 1      ;3
add eax,byte 4      ;3
add eax,dword [r12] ;2
mov r14d,eax       ;3
.continue

opdesc BF,		32,0xFF,0xFF,0xFF,18,0xFF
opfunc BF
and r14d,dword 0   ;7
bt dword [rbx],0    ;4
jc .continue        ;2
and eax,byte 00     ;3
or  eax,byte 00     ;3
shl eax,byte 1      ;3
add eax,byte 4      ;3
add eax,dword [r12] ;2
mov r14d,eax       ;3
.continue

opdesc BF_S,		25,0xFF,0xFF,0xFF,11,0xFF
opfunc BF_S
bt dword [rbx],0    ;4
jc .continue        ;2
and eax,byte 00     ;3
or eax,byte 00      ;3
shl eax,byte 1      ;3
add eax,byte 4      ;3
add eax,dword [r12] ;2
mov r14d,eax       ;3
.continue


;Store/Load Opcodes
;------------------

opdesc STC_SR,	12,0xFF,6,0xFF,0xFF,0xFF
opfunc STC_SR
mov rbp,rdi
add rbp,byte $00
mov eax,[rbx]
mov [rbp],eax

opdesc STC_GBR,	24,0xFF,6,0xFF,0xFF,0xFF
opfunc STC_GBR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov r13,rbp            ;1
ctrlreg_load 1      ;5
mov eax,[rbp]       ;2
mov [r13],eax       ;2

opdesc STC_VBR,	24,0xFF,6,0xFF,0xFF,0xFF
opfunc STC_VBR
mov rbp,rdi
add rbp,byte $00
mov r13,rbp            ;1
ctrlreg_load 2
mov eax,[rbp]
mov [r13],eax

opdesc STS_MACH, 12,0xFF,6,0xFF,0xFF,0xFF
opfunc STS_MACH
mov rbp,rdi       ;2
add rbp,byte $00  ;3
mov eax,[rsi]     ;2
mov [rbp],eax     ;2

opdesc STS_MACH_DEC,	28,0xFF,6,0xFF,0xFF,0xFF
opfunc STS_MACH_DEC
mov rbp,rdi       ;2
add rbp,byte $00  ;3
sub dword [rbp],byte 4 ;3
mov edx,[rsi]     ;2
mov ecx,[rbp]     ;2
mov rax,memSetLong ;5
call rax          ;2


opdesc STS_MACL, 13,0xFF,6,0xFF,0xFF,0xFF
opfunc STS_MACL
mov rbp,rdi       ;2
add rbp,byte $00  ;3
mov eax,[rsi+4]   ;3
mov [rbp],eax     ;2

opdesc STS_MACL_DEC,	29,0xFF,6,0xFF,0xFF,0xFF
opfunc STS_MACL_DEC
mov rbp,rdi       ;2
add rbp,byte $00  ;3
sub dword [rbp],byte 4 ;3
mov edx,[rsi+4]   ;3
mov ecx,[rbp]     ;2
mov rax,memSetLong ;5
call rax          ;2


;opdesc LDC_SR,	9,0xFF,6,0xFF,0xFF,0xFF
;opfunc LDC_SR
;mov rbp,rdi
;add rbp,byte $00
;mov eax,[rbp]
;mov [rbx],eax

opdesc LDC_SR,	25,0xFF,6,0xFF,0xFF,0xFF
opfunc LDC_SR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
and eax,0x000003f3  ;5
mov rbp,rdi         ;2
add rbp,byte 64     ;3
mov [rbp],eax       ;2


opdesc LDC_SR_INC,	34,0xFF,6,0xFF,0xFF,0xFF
opfunc LDC_SR_INC
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov rcx,[rbp]       ;2
mov rax,memGetLong ;5
call rax            ;2
and eax,0x3f3       ;5
mov dword [rbx],eax ;3
add dword [rbp],byte 4 ;3


opdesc LDCGBR,	20,0xff,6,0xFF,0xFF,0xFF
opfunc LDCGBR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;3
mov rbp,rdi         ;2
add rbp,byte 68     ;3
mov [rbp],eax       ;3

opdesc LDC_GBR_INC,	36,0xFF,6,0xFF,0xFF,0xFF
opfunc LDC_GBR_INC
mov  rbp,rdi            ;2
add  rbp,byte $00       ;3
mov  ecx,[rbp]          ;2
add  dword [rbp],byte 4 ;3
mov  rax,memGetLong    ;5
call rax                ;2
mov  rbp,rdi            ;2
add  rbp,byte 68        ;3
mov  dword [rbp],eax    ;3



opdesc LDC_VBR,	20,0xFF,6,0xFF,0xFF,0xFF
opfunc LDC_VBR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov rbp,rdi         ;2
add rbp,byte 72     ;3
mov [rbp],eax       ;2

opdesc LDC_VBR_INC,	36,0xFF,6,0xFF,0xFF,0xFF
opfunc LDC_VBR_INC
mov  rbp,rdi            ;2
add  rbp,byte $00       ;3
mov  ecx,[rbp]          ;2
add  dword [rbp],byte 4 ;3
mov  rax,memGetLong    ;5
call rax                ;2
mov  rbp,rdi            ;2
add  rbp,byte 72        ;3
mov  dword [rbp],eax    ;3


opdesc STS_PR,		13,0xFF,6,0xFF,0xFF,0xFF
opfunc STS_PR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rsi+8]     ;3
mov [rbp],eax       ;3

opdesc STSMPR,	29,0xFF,6,0xFF,0xFF,0xFF
opfunc STSMPR
mov rbp,rdi       ;2
add rbp,byte $00  ;3
sub dword [rbp],byte 4 ;3
mov edx,[rsi+8]   ;3
mov ecx,[rbp]     ;2
mov rax,memSetLong ;5
call rax          ;2


opdesc LDS_PR,		13,0xFF,6,0xFF,0xFF,0xFF
opfunc LDS_PR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov [rsi+8],eax     ;4

opdesc LDS_PR_INC,	29,0xFF,6,0xFF,0xFF,0xFF
opfunc LDS_PR_INC
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov ecx,[rbp]       ;3
mov rax,memGetLong ;5
call rax            ;2
mov [rsi+8],eax     ;3
add dword [rbp],byte 4 ;4


opdesc LDS_MACH,		12,0xFF,6,0xFF,0xFF,0xFF
opfunc LDS_MACH
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov [rsi],eax       ;2

opdesc LDS_MACH_INC,	31,0xFF,6,0xFF,0xFF,0xFF
opfunc LDS_MACH_INC
mov rbp,rdi            ;2
add rbp,byte $00       ;3
mov ecx,[rbp]          ;2
mov rax,memGetLong    ;5
call rax               ;2
mov [rsi],eax          ;2
add dword [rbp],  4 ;3

opdesc LDS_MACL,		13,0xFF,6,0xFF,0xFF,0xFF
opfunc LDS_MACL
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;2
mov [rsi+4],eax     ;2


opdesc LDS_MACL_INC,	29,0xFF,6,0xFF,0xFF,0xFF
opfunc LDS_MACL_INC
mov rbp,rdi            ;2
add rbp,byte $00       ;3
mov ecx,[rbp]          ;2
mov rax,memGetLong    ;5
call rax               ;2
mov [rsi+4],eax        ;3
add dword [rbp],byte 4 ;3


;Mov Opcodes
;-----------

opdesc MOVA,	25,0xFF,0xFF,0xFF,16,0xFF
opfunc MOVA
mov eax,[r12]       ;2
add eax,byte 4      ;3
and eax,byte 0xfc   ;3
mov r13d,eax
xor eax,eax         ;2
mov al,byte $00     ;3
shl eax,byte 2      ;3
add eax,r13d        ;2
mov [rdi],eax       ;2



opdesc MOVWI,	38,0xFF,6,0xFF,10,0xFF
opfunc MOVWI
mov rbp,rdi         ;2
add rbp,byte $00    ;3
xor ecx,ecx         ;2
mov cl,00           ;2
shl cx,byte 1       ;1
add ecx,[r12]       ;2 
add ecx,byte 4      ;3
mov rax,memGetWord ;5
call rax            ;2
cwde                ;1
mov [rbp],eax       ;3


opdesc MOVLI,       46,0xFF,6,0xFF,11,0xFF
opfunc MOVLI
mov rbp,rdi         ;2
add rbp,byte $00    ;3
xor rax,rax         ;2
mov al,00           ;2
shl ax,byte 2       ;3
mov ecx,[r12]       ;2
add ecx,byte 4      ;3
and ecx,0xFFFFFFFC  ;6
add ecx,eax         ;2 
mov rax,memGetLong ;5
call rax            ;2
mov [rbp],eax       ;3


opdesc MOVBL4, 35,6,0xFF,10,0xFF,0xFF
opfunc MOVBL4
mov  rbp,rdi          ;2  Get R0 adress
add  rbp,byte $00     ;3
xor  ecx,ecx          ;2  Clear Eax
mov  cl,$00           ;2  Get Disp
add  ecx,[rbp]        ;3
mov  rax,memGetByte  ;5
call rax              ;2
mov  rbp,rdi          ;2  Get R0 adress
cbw                   ;1  Sign extension byte -> word
cwde                  ;1  Sign extension word -> dword
mov  [rbp],eax        ;3


opdesc MOVWL4, 37,6,0xFF,10,0xFF,0xFF
opfunc MOVWL4
mov  rbp,rdi          ;2  Get R0 adress
add  rbp,byte $00     ;3
xor  ecx,ecx          ;2  Clear Eax
mov  cl,$00           ;2  Get Disp
shl  cx, byte 1       ;3  << 1
add  ecx,[rbp]        ;2
mov  rax,memGetWord  ;5
call rax              ;2
mov  rbp,rdi          ;2  Get R0 adress
cwde                  ;2  sign 
mov  [rbp],eax        ;3


opdesc MOVLL4, 40,6,36,10,0xFF,0xFF
opfunc MOVLL4
mov  rbp,rdi          ;2  Get R0 adress
add  rbp,byte $00     ;3
xor  ecx,ecx          ;2  Clear Eax
mov  cl,$00           ;2  Get Disp
shl  cx, byte 2       ;3  << 2
add  ecx,[rbp]        ;2
mov  rax,memGetLong  ;5
call rax              ;2
mov  rbp,rdi          ;2  Get R0 adress
add  rbp,byte $00     ;3
mov  [rbp],eax        ;3

 
opdesc MOVBS4,	35,9,0xFF,18,0xFF,0xFF
opfunc MOVBS4
mov rbp,rdi         ;2  Get R0 address
mov edx, [rbp]      ;3  Set to Func
add rbp,byte $00    ;3  Get Imidiate value R[n]
and ecx,00000000h   ;5  clear eax
or  ecx,byte $00    ;3  Get Disp value
add rcx,[rbp]       ;3  Add Disp value
mov rax,memSetByte  ;5
call rax            ;2  Call Func

opdesc MOVWS4,	37,9,0xFF,18,0xFF,0xFF
opfunc MOVWS4
mov rbp,rdi         ;2  Get R0 address
mov edx, [rbp]      ;3  Set to Func
add rbp,byte $00    ;3  Get Imidiate value R[n]
and ecx,00000000h   ;5  clear eax
or  ecx,byte $00    ;3  Get Disp value
shl ecx,byte 1      ;3  Shift Left
add ecx,[rbp]       ;3  Add Disp value
mov rax,memSetWord ;5
call rax            ;2  Call Func


opdesc MOVLS4,	42,6,16,24,0xFF,0xFF
opfunc MOVLS4
mov  rbp,rdi         ;2 
add  rbp,byte $00    ;3 Get m 4..7
mov  edx, [rbp]     ;3
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3 Get n 8..11
mov  ecx, [rbp]     ;3
xor  eax,eax         ;2
or   eax,byte 0      ;3 Get Disp 0..3
shl  eax,byte 2      ;3
add  ecx,eax ;2
mov  rax,memSetLong ;5
call rax             ;2

 
opdesc MOVBLG,    28,0xFF,0xFF,0xFF,6,0xFF
opfunc MOVBLG
mov  rbp,rdi           ;2  Get R0 adress
xor  ecx,ecx           ;2  clear eax
mov  cl,00             ;2  Get Imidiate Value
add  ecx,dword [rbp+68];3  GBR + IMM( Adress for Get Value )
mov  rax,memGetByte   ;5
call rax               ;2
cbw                    ;1
cwde                   ;1
mov  [rbp],eax         ;3


opdesc MOVWLG,    30,0xFF,0xFF,0xFF,6,0xFF
opfunc MOVWLG
mov  rbp,rdi           ;2  Get R0 adress
xor  ecx,ecx           ;2  clear eax
mov  cl,00             ;2  Get Imidiate Value
shl  cx,byte 1         ;3  Shift left 2
add  ecx,dword [rbp+68];3  GBR + IMM( Adress for Get Value )
mov  rax,memGetWord   ;5
call rax               ;2
cwde                   ;1
mov  [rbp],eax         ;3


opdesc MOVLLG,    29,0xFF,0xFF,0xFF,6,0xFF
opfunc MOVLLG
mov  rbp,rdi           ;2  Get R0 adress
xor  ecx,ecx           ;2  clear eax
mov  cl,00             ;5  Get Imidiate Value
shl  cx,byte 2         ;3  Shift left 2
add  ecx,dword [rbp+68];3  GBR + IMM( Adress for Get Value )
mov  rax,memGetLong   ;5
call rax               ;2
mov  [rbp],eax         ;3


opdesc MOVBSG,    32,0xFF,0xFF,0xFF,9,0xFF
opfunc MOVBSG
mov  rbp,rdi         ;2  Get R0 adress
mov  edx,[rbp]       ;3
xor  ecx,ecx         ;2  Clear Eax
mov  cl,00           ;5  Get Imidiate Value
mov  rbp,rdi         ;2  Get GBR adress
add  rbp,byte 68     ;3 
add  ecx,dword [rbp] ;2  GBR + IMM( Adress for Get Value )
mov  rax,memSetByte ;5
call rax             ;2

opdesc MOVWSG,    36,0xFF,0xFF,0xFF,9,0xFF
opfunc MOVWSG
mov  rbp,rdi         ;2  Get R0 adress
mov  edx,[rbp]       ;3
xor  ecx,ecx         ;2  Clear Eax
mov  cl,00           ;5  Get Imidiate Value
shl  cx,byte 1       ;3  Shift left 2
mov  rbp,rdi         ;2  Get GBR adress
add  rbp,byte 68     ;3 
add  ecx,dword [rbp] ;2  GBR + IMM( Adress for Get Value )
mov  rax,memSetWord ;5
call rax             ;2


opdesc MOVLSG,    36,0xFF,0xFF,0xFF,9,0xFF
opfunc MOVLSG
mov  rbp,rdi         ;2  Get R0 adress
mov  edx, [rbp]     ;3
xor  ecx,ecx         ;2  Clear Eax
mov  cl,00           ;5  Get Imidiate Value
shl  cx,byte 2       ;3  Shift left 2
mov  rbp,rdi         ;2  Get GBR adress
add  rbp,byte 68     ;3 
add  ecx,dword [rbp] ;2  GBR + IMM( Adress for Get Value )
mov  rax,memSetLong ;5
call rax             ;2


opdesc MOVBS,	32,6,16,0xFF,0xFF,0xFF
opfunc MOVBS
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3
mov  edx, [rbp]     ;3
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3
mov  ecx, [rbp]     ;3
mov  rax,memSetByte ;5
call rax             ;2


opdesc MOVWS,	32,6,16,0xFF,0xFF,0xFF
opfunc MOVWS
mov  rbp,rdi          ;2
add  rbp,byte $00     ;3
mov  edx, [rbp]      ;3
mov  rbp,rdi          ;2
add  rbp,byte $00     ;3
mov  ecx, [rbp]      ;3
mov  rax,memSetWord  ;5
call rax              ;2


opdesc MOVLS,	32,6,16,0xFF,0xFF,0xFF
opfunc MOVLS
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3
mov edx, [rbp]     ;3
mov  rbp,rdi         ;2
add  rbp,byte $00    ;3
mov ecx, [rbp]     ;3
mov  rax,memSetLong ;5
call rax             ;2


opdesc MOVR,		20,6,16,0xFF,0xFF,0xFF
opfunc MOVR
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov eax,[rbp]       ;3
mov rbp,rdi         ;2
add rbp,byte $00    ;3
mov [rbp],eax       ;3

opdesc MOVBM,		36,6,16,0xFF,0xFF,0xFF
opfunc MOVBM
mov  rbp,rdi             ;2
add  rbp,byte $00        ;3  
mov  edx, [rbp]         ;3 Set data
mov  rbp,rdi             ;2
add  rbp,byte $00        ;3
sub  dword [rbp],byte 1  ;4
mov  ecx, [rbp]         ;3 Set addr
mov  rax,memSetByte     ;5
call rax                 ;2


opdesc MOVWM,		36,6,16,0xFF,0xFF,0xFF
opfunc MOVWM
mov  rbp,rdi             ;2
add  rbp,byte $00        ;3  
mov  edx, [rbp]         ;3 Set data
mov  rbp,rdi             ;2
add  rbp,byte $00        ;3
sub  dword [rbp],byte 2  ;4
mov  ecx, [rbp]         ;3 Set addr
mov  rax,memSetWord     ;5
call rax                 ;2


opdesc MOVLM,		36,6,16,0xFF,0xFF,0xFF
opfunc MOVLM
mov  rbp,rdi             ;2
add  rbp,byte $00        ;3  
mov  edx, [rbp]         ;3 Set data
mov  rbp,rdi             ;2
add  rbp,byte $00        ;3
sub  dword [rbp],byte 4  ;4
mov  ecx, [rbp]         ;3 Set addr
mov  rax,memSetLong     ;5
call rax                 ;2

;------------- added ------------------

opdesc TAS,  56,0xFF,6,0xFF,0xFF,0xFF
opfunc TAS
mov  rbp,rdi             ;2
add  rbp,byte $00        ;3
mov  rcx, [rbp]         ;3
push rcx 
mov  rax,memGetByte     ;5
call rax                 ;2
and  eax,0x000000FF      ;5
and  [rbx], byte 0xFE    ;3
test eax,eax             ;3
jne  NOT_ZERO            ;2
or   [rbx], byte 01      ;3
NOT_ZERO:              
or   al, byte 0x80        ;3
mov  edx, eax                  ;1 
pop  rcx          ;3
mov  rax,memSetByte      ;5
call rax                  ;2


; sub with overflow check
opdesc SUBV, 28,6,16,0xFF,0xFF,0xFF
opfunc SUBV
mov  rbp,rdi          ;2
add  rbp,byte $00     ;3 m 4...7
mov  eax,[rbp]        ;3
mov  rbp,rdi          ;2
add  rbp,byte $00     ;3 n 8...11
and  dword [rbx], byte 0xFE ;3
sub  dword [rbp],eax        ;3 R[n] = R[n] - R[m]
jno	 NO_OVER_FLOS     ;2
or   dword [rbx], byte 01   ;3
NO_OVER_FLOS


; string cmp
opdesc CMPSTR, 64,6,16,0xFF,0xFF,0xFF
opfunc CMPSTR
mov  rbp,rdi          ;2
add  rbp,byte $00     ;3
mov  eax,[rbp]        ;3
mov  rbp,rdi          ;2
add  rbp,byte $00     ;3
mov  ecx,[rbp]        ;3
xor  eax,ecx          ;2  
and  [rbx], byte 0xFE ;3  Clear T flag
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
or   [rbx], byte 01   ;3 T flg ON
ENDPROC:

;-------------------------------------------------------------
;div0s 
opdesc DIV0S, 82,39,6,0xFF,0xFF,0xFF
opfunc DIV0S
mov  rbp,rdi                 ;2 SetQ
add  rbp,byte $00            ;3 8..11
and  dword [rbx],0xFFFFFEFF  ;6 Clear Q Flg

mov  eax, 0                  ;5 Zero Clear eax     

test dword [rbp],0x80000000  ;7 Test sign
je   continue                ;2 if ZF = 1 then goto NO_SIGN
or   dword [rbx],0x00000100  ;6 Set Q Flg
inc  eax                     ;1 

continue:
mov  ebp,edi                 ;2 SetM
add  ebp,byte $00            ;3 4..7
and  dword [rbx],0xFFFFFDFF  ;6 Clear M Flg
test dword [rbp],0x80000000  ;7 Test sign
je   continue2               ;2 if ZF = 1 then goto NO_SIGN
or   dword [rbx],0x00000200  ;6  Set M Flg
inc  eax                     ;1

continue2:
and  dword [rbx],0xFFFFFFFE  ;6 Clear T Flg
test eax, 1                  ;5 if( Q != M ) SetT(1)
je  continue3                ;2
or   dword [rbx],0x00000001  ;6 Set T Flg
continue3:
 

;===============================================================
; DIV1   1bit Divid operation
; 
; size = 69 + 135 + 132 + 38 = 374 
;===============================================================
opdesc DIV1, 428,65,6,0xFF,0xFF,0xFF
opfunc DIV1

; 69
mov  rbp,rdi                 ;2 Get R[0] Adress 
xor  eax,eax                 ;2 Clear esi
mov  al, byte 00             ;3 save n(8-11)
mov  r15d,eax                 ;2
add  rbp,r15                 ;2 Get R[n]
mov  eax,[rbp]               ;3 R[n]
mov  ecx,[rbx]               ;2 SR

test eax,0x80000000          ;5 
je   NOZERO                  ;2
or   dword [rbx],0x00000100  ;6 Set Q Flg
jmp  CONTINUE                ;3
NOZERO:
and  dword [rbx],0xFFFFFEFF  ;6 Clear Q Flg

CONTINUE:

; sh2i->R[n] |= (DWORD)(sh2i->i_get_T())
mov  eax,[rbx]               ;2    
and  eax,0x01                ;5
shl  dword [rbp], byte 1     ;3
or   [rbp],eax               ;3

;Get R[n],R[m]
mov  eax,[rbp]               ;3 R[n]
mov  rbp,rdi                 ;2  
add  rbp,byte $00            ;3 4...7
mov  r13d,[rbp]               ;3 R[m]

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
	  mov rbp,rdi           ;2
	  add rbp,r15           ;3
	  sub dword [rbp],r13d         ;3 sh2i->R[n] -= sh2i->R[m]
    
	  test dword [rbx],0x00000100      ;6 Q == 1 
	  jne NQ_NM_Q_FLG                  ;2

	  NQ_NM_NQ_FLG:
	  cmp [rbp],eax        ;3 tmp1 = (sh2i->R[n]>tmp0);
	  jna NQ_NM_NQ_00_FLG  ;2

		  NQ_NM_NQ_01_FLG:
		  or   dword [rbx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1                 ;3

		  NQ_NM_NQ_00_FLG:
		  and  dword [rbx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1                 ;3  
  
	  NQ_NM_Q_FLG:
	  cmp [rbp],eax        ;3 tmp1 = (sh2i->R[n]>tmp0);
	  jna NQ_NM_NQ_10_FLG  ;2

		  NQ_NM_NQ_11_FLG:
		  and  dword [rbx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1                 ;3

		  NQ_NM_NQ_10_FLG:
		  or   dword [rbx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1                 ;3

Q_FLG_TMP:
jmp Q_FLG; 3

	;----------------------------------------------------  
	NQ_M_FLG:
	  mov rbp,rdi           ;
	  add rbp,r15           ;

	  add  dword [rbp],r13d        ; sh2i->R[n] += sh2i->R[m]  
	  test dword [rbx],0x00000100 ; Q == 1 
	  jne NQ_M_Q_FLG

	  NQ_M_NQ_FLG:
	  cmp [rbp],eax         ; tmp1 = (sh2i->R[n]<tmp0);
	  jnb NQ_M_NQ_00_FLG

		  NQ_M_NQ_01_FLG:
		  and  dword [rbx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1

		  NQ_M_NQ_00_FLG:
		  or   dword [rbx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1


	  NQ_M_Q_FLG:
	  cmp [rbp],eax                    ; tmp1 = (sh2i->R[n]<tmp0);
	  jnb NQ_M_NQ_10_FLG

		  NQ_M_NQ_11_FLG:
		  or   dword [rbx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1

		  NQ_M_NQ_10_FLG:
		  and  dword [rbx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1

;------------------------------------------------------
; 8 + 62 + 62 = 132
Q_FLG:   


test ecx,0x00000200 ; M == 1 ?
jne  Q_M_FLG

	;--------------------------------------------------
	Q_NM_FLG:
	  mov rbp,rdi           ;
	  add rbp,r15           ;
	  add dword [rbp],r13d         ; sh2i->R[n] += sh2i->R[m]
	  test dword [rbx],0x00000100 ; Q == 1 
	  jne Q_NM_Q_FLG

	  Q_NM_NQ_FLG:
	  cmp [rbp],eax      ; tmp1 = (sh2i->R[n]<tmp0);
	  ja Q_NM_NQ_00_FLG

		  Q_NM_NQ_01_FLG:
		  or   dword [rbx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1

		  Q_NM_NQ_00_FLG:
		  and  dword [rbx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1
  
	  Q_NM_Q_FLG:
	  cmp [rbp],eax      ; tmp1 = (sh2i->R[n]<tmp0);
	  ja Q_NM_NQ_10_FLG  

		  Q_NM_NQ_11_FLG:
		  and  dword [rbx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1

		  Q_NM_NQ_10_FLG:
		  or   dword [rbx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1

	;----------------------------------------------------  
	Q_M_FLG:
	  mov rbp,rdi        ;
	  add rbp,r15        ;
	  sub dword [rbp],r13d      ; sh2i->R[n] -= sh2i->R[m]  
	  test dword [rbx],0x00000100 ; Q == 1 
	  jne Q_M_Q_FLG

	  Q_M_NQ_FLG:
	  cmp [rbp],eax      ; tmp1 = (sh2i->R[n]>tmp0);
	  jb Q_M_NQ_00_FLG

		  Q_M_NQ_01_FLG:
		  and  dword [rbx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1

		  Q_M_NQ_00_FLG:
		  or   dword [rbx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1
 
	  Q_M_Q_FLG:
	  cmp [rbp],eax      ; tmp1 = (sh2i->R[n]>tmp0);
	  jb Q_M_NQ_10_FLG

		  Q_M_NQ_11_FLG:
		  or   dword [rbx],0x00000100  ;6 Set Q Flg
		  jmp END_DIV1

		  Q_M_NQ_10_FLG:
		  and  dword [rbx],0xFFFFFEFF  ;6 Clear Q Flg
		  jmp END_DIV1


;---------------------------------------------------
; size = 38
END_DIV1 

;sh2i->i_set_T( (sh2i->i_get_Q() == sh2i->i_get_M()) );

mov  eax,[rbx]                ;2 Get Q
shr  eax,8                    ;3
and  eax,1                    ;5 
mov  r13d,[rbx]                ;2 Get M
shr  r13d,9                    ;3    
and  r13d,1                    ;6
and  dword [rbx], 0xFFFFFFFE  ;6 Set T Flg
cmp  eax,r13d                  ;2
jne  NO_Q_M                   ;2
or   dword [rbx], 0x00000001  ;6 Set T Flg
NO_Q_M:



;======================================================
; end of DIV1
;======================================================

;------------------------------------------------------------
;dmuls
opdesc DMULS, 27,6,16,0xFF,0xFF,0xFF
opfunc DMULS
mov  rbp,rdi            ;2  
add  rbp,byte $00       ;3 4..7
mov  eax,dword [rbp]    ;3
mov  rbp,rdi            ;2 SetQ
add  rbp,byte $00       ;3 8..11
mov  edx,dword [rbp]    ;3  
imul edx                ;2
mov  dword [rsi]  ,edx  ;2 store MACH             
mov  dword [rsi+4],eax  ;3 store MACL   

;------------------------------------------------------------
;dmulu 32bit -> 64bit Mul
opdesc DMULU, 27,6,16,0xFF,0xFF,0xFF
opfunc DMULU
mov  rbp,rdi            ;2  
add  rbp,byte $00       ;3 4..7
mov  eax,dword [rbp]    ;3
mov  rbp,rdi            ;2 SetQ
add  rbp,byte $00       ;3 8..11
mov  edx,dword [rbp]    ;3  
mul  edx                ;2
mov  dword [rsi]  ,edx  ;2 store MACH             
mov  dword [rsi+4],eax  ;3 store MACL   

;--------------------------------------------------------------
; mull 32bit -> 32bit Multip
opdesc MULL, 25,6,16,0xFF,0xFF,0xFF
opfunc MULL
mov  rbp,rdi            ;2  
add  rbp,byte $00       ;3 4..7
mov  eax,dword [rbp]    ;3
mov  rbp,rdi            ;2
add  rbp,byte $00       ;3 8..11
mov  edx,dword [rbp]    ;3  
imul edx                ;2
mov  dword [rsi+4],eax  ;3 store MACL   

;--------------------------------------------------------------
; muls 16bit -> 32 bit Multip
opdesc MULS, 38,6,19,0xFF,0xFF,0xFF
opfunc MULS
mov  rbp,rdi           ;2  
add  rbp,byte $00      ;3 4..7
xor  eax,eax           ;2
mov  ax,word [rbp]     ;3
mov  rbp,rdi           ;2
add  rbp,byte $00      ;3 8..11
xor  edx,edx           ;2
mov  dx,word [rbp]     ;3  
imul dx                ;2
shl  edx, byte 16      ;3
add  dx, ax            ;2
mov  dword [rsi+4],edx ;3 store MACL   

;--------------------------------------------------------------
; mulu 16bit -> 32 bit Multip
opdesc MULU, 38,6,19,0xFF,0xFF,0xFF
opfunc MULU
mov  rbp,rdi           ;2  
add  rbp,byte $00      ;3 4..7
xor  eax,eax           ;2
mov  ax,word [rbp]     ;3
mov  rbp,rdi           ;2
add  rbp,byte $00      ;3 8..11
xor  edx,edx           ;2
mov  dx,word [rbp]     ;3  
mul  dx                ;2
shl  edx, byte 16      ;3
add  dx, ax            ;2
mov  dword [rsi+4],edx ;3 store MACL   

;--------------------------------------------------------------
; MACL   ans = 32bit -> 64 bit MUL
;        (MACH << 32 + MACL)  + ans 
;-------------------------------------------------------------
opdesc MAC_L, 160,6,38,0xFF,0xFF,0xFF 
opfunc MAC_L
mov  rbp,rdi                  ;2  
add  rbp,byte $00             ;3 4..7
mov  ecx,dword [rbp]          ;3
mov  rax,memGetLong          ;5
call rax                      ;2
mov  r13d,eax                  ;2
add  dword [rbp], 4           ;7 R[n] += 4
mov  rbp,rdi                  ;2 
add  rbp,byte $00             ;3 8..11
mov  ecx,dword [rbp]          ;3
mov  rax,memGetLong          ;5 
call rax                      ;2
add  dword [rbp], 4           ;7 R[m] += 4 
xor  ecx,ecx                  ;2
or   ecx,[rsi+4]              ;3 load macl
imul r13d                      ;2 eax <- low, edx <- high
mov  r13d,[rsi]                ;3 load mach
add  ecx,eax                  ;3 sum = a+b;
adc  r13d,edx                  ;2
test dword [rbx], 0x00000002  ;6 check S flg
je   END_PROC                 ;2 if( S == 0 ) goto 'no sign proc'
cmp  r13d,7FFFh                ;6
jb   END_PROC                 ;2 
ja   COMP_MIN                ;2
cmp  ecx,0FFFFFFFFh           ;3 
jbe   END_PROC                ;2  = 88

COMP_MIN:
cmp   r13d,0FFFF8000h          ;6
ja   END_PROC                 ;2 
jb   CHECK_AAA                ;2 
test ecx,ecx                  ;2
jae  END_PROC                ;2
CHECK_AAA:
test edx,edx                ;2
jg   MAXMIZE                  ;2
jl   MINIMIZE                 ;2
test eax,eax                  ;3
jae MAXMIZE                   ;2
MINIMIZE:
xor ecx,ecx                   ;2
mov r13d,0FFFF8000h            ;5
jmp END_PROC                 ;2
MAXMIZE:
or   ecx,0FFFFFFFFh           ;3 sum = 0x00007FFFFFFFFFFFULL;
mov  r13d,7FFFh                ;5
END_PROC:
mov  [rsi],r13d         ; 3
mov  [rsi+4],ecx       ; 3



;--------------------------------------------------------------
; MACW   ans = 32bit -> 64 bit MUL
;        (MACH << 32 + MACL)  + ans 
;-------------------------------------------------------------
opdesc MAC_W, 133,6,39,0xFF,0xFF,0xFF
opfunc MAC_W
mov  rbp,rdi                  ;2  
add  rbp,byte $00             ;3 4..7
mov  ecx,dword [rbp]          ;3
mov  rax,memGetWord          ;5
call rax                      ;2
movsx  r13d,ax                 ;3
add  dword [rbp], 2           ;7 R[n] += 2
mov  rbp,rdi                  ;2 
add  rbp,byte $00             ;3 8..11
mov  ecx,dword [rbp]          ;3
mov  rax,memGetWord          ;5 
call rax                      ;2
add  dword [rbp], 2           ;7 R[m] += 2
cwde                          ;1 Sigin extention
imul r13d                      ;2 eax <- low, edx <- high
test dword [rbx], 0x00000002  ;6 check S flg
je   MACW_NO_S_FLG                 ;2 if( S == 0 ) goto 'no sign proc'

MACW_S_FLG:
  add dword [rsi+4],eax   ;3 MACL = ansL + MACL
  jno NO_OVERFLO
  js  FU 
  SEI:
    mov dword [rsi+4],0x80000000 ; min value
	or  dword [rsi],1
	jmp END_MACW

  FU:
    mov dword [rsi+4],0x7FFFFFFF ; max value
	or dword [rsi],1
	jmp END_MACW

  NO_OVERFLO:
  jmp END_MACW

MACW_NO_S_FLG:
  add dword [rsi+4],eax         ;3 MACL = ansL + MACL
  jnc MACW_NO_CARRY             ;2 Check Carry
  inc r13d                       ;1
MACW_NO_CARRY: 
  add dword [rsi],r13d           ;2 MACH = ansH + MACH

END_MACW:
  
END_ALL:
  nop

end
