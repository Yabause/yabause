/*
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

; r0  ; not keeped on function call
; r1  ; not keeped on function call  
; r2  ; not keeped on function call
; r3  ; not keeped on function call
; r4  
; r5  
; r6
; r7   <- Address of GenReg
; r8   <- PC
; r9   <- ClockCounter
; r10  <- 
; r11     ; scrach
; r12 ip
; r13 sp
; r14 lr  ; scrach
; r15 pc

  u32 GenReg[16];
  u32 CtrlReg[3];
  u32 SysReg[6];

  MACH   [r7, #(16+3+0)*4 ]  
  MACL   [r7, #(16+3+1)*4 ]  
  PR     [r7, #(16+3+2)*4 ]  
  PC     [r7, #(16+3+3)*4 ]  
  COUNT  [r7, #(16+3+4)*4 ]  
  ICOUNT [r7, #(16+3+5)*4 ]  
  SR     [r7, #(16+0)*4 ]  
  GBR     [r7, #(16+1)*4 ]  
  VBR     [r7, #(16+2)*4 ]  
  getmembyte = [r7, #(16+3+6+0)*4 ] ;
  getmemword = [r7, #(16+3+6+1)*4 ] ;
  getmemlong = [r7, #(16+3+6+2)*4 ] ;
  setmembyte = [r7, #(16+3+6+3)*4 ] ;
  setmemword = [r7, #(16+3+6+4)*4 ] ;
  setmemlong = [r7, #(16+3+6+5)*4 ] ;

  http://www.mztn.org/slasm/arm07.html
  http://qiita.com/edo_m18/items/a7c747c5bed600dca977

*/

/*
;Memory Functions
extern _memGetByte, _memGetWord, _memGetLong
extern _memSetByte, _memSetWord, _memSetLong
extern _EachClock, _DelayEachClock, _DebugEachClock, _DebugDelayClock
*/
 
 
.macro opfunc name
.section .text 
  .global x86_\name
  .type	x86_\name, %function
  x86_\name:
.endm

.macro CALL_GETMEM_BYTE 
  ldr r10 , [r7, #(16+3+6+0)*4 ]  
  blx r10
.endm  

.macro CALL_GETMEM_WORD 
  ldr r10 , [r7, #(16+3+6+1)*4 ]  
  blx r10
.endm  

.macro CALL_GETMEM_LONG
  ldr r10 , [r7, #(16+3+6+2)*4 ]  
  blx r10
.endm  


.macro CALL_SETMEM_BYTE 
  ldr r10 , [r7, #(16+3+6+3)*4 ]  
  blx r10
.endm  

.macro CALL_SETMEM_WORD 
  ldr r10 , [r7, #(16+3+6+4)*4 ]  
  blx r10
.endm  

.macro CALL_SETMEM_LONG
  ldr r10 , [r7, #(16+3+6+5)*4 ]  
  blx r10
.endm  


.macro LDR_MACH reg
  ldr \reg , [r7, #(16+3+0)*4 ]  
.endm  

.macro LDR_MACL reg
  ldr \reg , [r7, #(16+3+1)*4 ]  
.endm  

.macro LDR_PR reg
   ldr \reg , [r7, #(16+3+2)*4 ] 
.endm

.macro LDR_PC reg
   ldr \reg , [r7, #(16+3+3)*4 ] 
.endm

.macro LDR_COUNT reg
   ldr \reg , [r7, #(16+3+4)*4 ] 
.endm

.macro LDR_SR reg
   ldr \reg , [r7, #(16+0)*4 ] 
.endm

.macro LDR_GBR reg
   ldr \reg , [r7, #(16+1)*4 ] 
.endm

.macro LDR_VBR reg
   ldr \reg , [r7, #(16+2)*4 ] 
.endm

.macro STR_MACH reg
  str \reg , [r7, #(16+3+0)*4 ]  
.endm  

.macro STR_MACL reg
  str \reg , [r7, #(16+3+1)*4 ]  
.endm  

.macro STR_PR reg
  str \reg , [r7, #(16+3+2)*4 ] 
.endm

.macro STR_PC reg
  str \reg , [r7, #(16+3+3)*4 ] 
.endm

.macro STR_COUNT reg
  str \reg , [r7, #(16+3+4)*4 ] 
.endm

.macro STR_SR reg
  str \reg , [r7, #(16+0)*4 ] 
.endm

.macro STR_GBR reg
  str \reg , [r7, #(16+1)*4 ] 
.endm

.macro STR_VBR reg
  str \reg , [r7, #(16+2)*4 ] 
.endm


.macro opdesc name,b,c,d,e,f,g
.section .data
  .global \name\()_size
  \name\()_size: .long \b
  .global \name\()_src
   \name\()_src: .long \c
  .global \name\()_dest
   \name\()_dest: .long \d
  .global \name\()_off1
   \name\()_off1: .long \e
  .global \name\()_imm
   \name\()_imm: .long \f
  .global \name\()_off3
   \name\()_off3: .long \g
.endm  

.macro ctrlreg_load a     // 5
  add r0,r10, (\a*4)      // 2
  ldr	r0, [r0]
.endm

//=====================================================
// Basic
//=====================================================


.text
.align  2

//-----------------------------------------------------
// Begining of block
// r7   <- Address of GenReg
// r8   <- PC
// r9   <- Clock Counter
.global prologue
prologue:
stmfd  sp!, {r0-r10, lr}   // push regs
mov r7, r0      // GenReg( r0 has adress of m_pDynaSh2)
LDR_PC r8       // PC
LDR_COUNT r9    // ClockCounter
.size	prologue, .-prologue // 16


//-----------------------------------------------------
// normal part par instruction 
.global seperator_normal
seperator_normal:
add r8, #2    // 3 PC += 2
add r9, #1    // 4 Clock += 1  
.size	seperator_normal, .-seperator_normal // 8

//----------------------------------------------------- 
// Delay slot part par instruction
.global seperator_delay_slot
seperator_delay_slot:
cmn r0, #1 // Check need to branch
bne continue  
add r8, #2    // PC += 2
STR_PC r8
add r9, #1    // Clock += 1  
STR_COUNT r9
ldmfd  sp!, {r0-r10, pc} // pop regs and resturn
continue:
mov r8, r0    // copy jump addr
sub r8, #2    // PC -= 2
.size seperator_delay_slot, .-seperator_delay_slot // 40
  

//-----------------------------------------------------
// Delay slot part par instruction
.global seperator_delay_after
seperator_delay_after:
add r8, #2    // PC += 2
STR_PC r8     // store to memory
add r9, #1    // Clock += 1  
STR_COUNT r9  // store to memory
ldmfd  sp!, {r0-r10, pc} // pop regs and resturn
.size seperator_delay_after, .-seperator_delay_after // 20


//-------------------------------------------------------
// End of block
.global epilogue
epilogue:
STR_PC r8     // store PC to memory
STR_COUNT r9  // store COUNTER to memory
ldmfd  sp!, {r0-r10, pc} // pop regs and resturn
.size	epilogue, .-epilogue // 12

//-----------------------------------------------------
// Jump part
.global PageFlip
PageFlip:
mvn r1, #0 // load 0xFFFFFF 
tst r0, r1 // 7
bne PageFlip.continue     // 2
STR_PC r0
ldmfd  sp!, {r0-r10, pc} // pop regs and resturn
PageFlip.continue:
.size	PageFlip, .-PageFlip // 22

//-------------------------------------------------------
// normal part par instruction( for debug build )
.global seperator_d_normal
seperator_d_normal:
bl	SH2HandleBreakpoints

//------------------------------------------------------
// Delay slot part par instruction( for debug build )
.global seperator_d_delay
seperator_d_delay:


opdesc CLRT,3,0,0,0,0,0
opfunc CLRT

opdesc CLRMAC,7,0,0,0,0,0
opfunc CLRMAC

opdesc NOP,		4,0xff,0xff,0xff,0xff,0xff
opfunc NOP
nop

opdesc DIV0U,	6,0,0,0,0,0
opfunc DIV0U

opdesc SETT,	3,0,0,0,0,0
opfunc SETT

opdesc SLEEP,	14,0,0,0,0,0
opfunc SLEEP

opdesc SWAP_W,	19,4,15,0,0,0
opfunc SWAP_W

opdesc SWAP_B,	18,4,14,0,0,0
opfunc SWAP_B

opdesc TST,	29,4,12,0,0,0
opfunc TST


opdesc TSTI,	23,0,0,0,6,0
opfunc TSTI


opdesc ANDI,	9,0,0,0,5,0
opfunc ANDI

opdesc XORI,	9,0,0,0,5,0
opfunc XORI

opdesc ORI,	9,0,0,0,5,0
opfunc ORI

opdesc CMP_EQ_IMM,	25,0,0,0,8,0
opfunc CMP_EQ_IMM

opdesc XTRCT,	23,4,12,0,0,0
opfunc XTRCT

opdesc ADD,		16,4,12,0,0,0
opfunc ADD

opdesc ADDC,	33,4,12,0,0,0
opfunc ADDC

opdesc ADDV, 24,4,12,0,0,0
opfunc ADDV

opdesc SUBC,	33,4,12,0,0,0
opfunc SUBC

opdesc SUB,		16,4,12,0,0,0
opfunc SUB

opdesc NOT,		18,4,14,0,0,0
opfunc NOT

opdesc NEG,		18,4,14,0,0,0
opfunc NEG

opdesc NEGC,	50,4,14,0,0,0
opfunc NEGC

opdesc EXTUB,	21,4,17,0,0,0
opfunc EXTUB

opdesc EXTU_W,	21,4,17,0,0,0
opfunc EXTU_W

opdesc EXTS_B,	19,4,15,0,0,0
opfunc EXTS_B

opdesc EXTS_W,	17,4,13,0,0,0
opfunc EXTS_W

//Store Register Opcodes
//----------------------

opdesc STC_SR_MEM,	23,0,6,0,0,0
opfunc STC_SR_MEM

opdesc STC_GBR_MEM,	30,0,13,0,0,0
opfunc STC_GBR_MEM

opdesc STC_VBR_MEM,	30,0,13,0,0,0
opfunc STC_VBR_MEM


opdesc MOVBL,	28,0,4,0xff,0xff,0xff
opfunc MOVBL
mov  r0, #0  // m
mov  r4, #0  // n
ldr  r0, [r7, r0, asl #2] // r0 = R[m] 
CALL_GETMEM_BYTE
sxtb r0, r0
str  r0, [r7, r4, asl #2]


opdesc MOVWL,		26,4,20,0,0,0
opfunc MOVWL

opdesc MOVL_MEM_REG,		25,4,20,0,0,0
opfunc MOVL_MEM_REG

opdesc MOVBP,	31,4,23,0,0,0
opfunc MOVBP

opdesc MOVWP,	30,4,24,0,0,0
opfunc MOVWP

opdesc MOVLP,	56,0,4,0xff,0xff,0xff
opfunc MOVLP
mov  r0, #0 // m
mov  r1, #0 // n
mov  r5, r0 
ldr  r0, [r7, r0]
mov  r6, r1
CALL_GETMEM_LONG
cmp  r6, r5
str  r0, [r7, r6]
bne  MOVLP.continue   
ldr  r3, [r7, r5]
add  r3, r3, #4
str  r3, [r7, r5]
MOVLP.continue:


opdesc MOVW_A,	38,0,4,0,17,0
opfunc MOVW_A

opdesc MOVL_A,	37,0,4,0,17,0
opfunc MOVL_A

opdesc MOVI,	16,0xff,0,0xff,4,0xff
opfunc MOVI
mov r1, #0  // n
mov r0, #0  // i
sxtb r0, r0 // (signed char)i
str  r0, [r7, r1] // R[n] = (int)i


//----------------------

opdesc MOVBL0,	30,4,22,0,0,0
opfunc MOVBL0

opdesc MOVWL0,	28,4,22,0,0,0
opfunc MOVWL0

opdesc MOVLL0,	27,4,22,0,0,0
opfunc MOVLL0

opdesc MOVT,		15,0,4,0,0,0
opfunc MOVT

opdesc MOVBS0,	28,4,12,0,0,0
opfunc MOVBS0

opdesc MOVWS0,	28,4,12,0,0,0
opfunc MOVWS0

opdesc MOVLS0,	28,4,12,0,0,0
opfunc MOVLS0

//===========================================================================
//Verified Opcodes
//===========================================================================

opdesc DT,		36,0xff,0,0xff,0xff,0xff
opfunc DT
mov     r0, #0            // n
LDR_SR  r1            // r1 = SR
ldr     r3, [r7, r0] // r3 = R[n]
sub     r3, r3, #1   // r3--
cmp     r3, #0   
str     r3, [r7, r0] // R[n] = r3
orreq   r0, r1, #1   // if( R[n] == 0 ) SR |= 0x01
bicne   r0, r1, #1   // if( R[n] != 0 ) SR &= ^0x01;
STR_SR  r0           // SR = r0		


opdesc CMP_PZ,	17,0,4,0,0,0
opfunc CMP_PZ

opdesc CMP_PL,	23,0,4,0,0,0
opfunc CMP_PL

opdesc CMP_EQ,	24,4,12,0,0,0
opfunc CMP_EQ

opdesc CMP_HS,	24,4,12,0,0,0
opfunc CMP_HS

opdesc CMP_HI,	24,4,12,0,0,0
opfunc CMP_HI

opdesc CMP_GE,	24,4,12,0,0,0
opfunc CMP_GE

opdesc CMP_GT,	24,4,12,0,0,0
opfunc CMP_GT

opdesc ROTL,	23,0,4,0,0,0
opfunc ROTL

opdesc ROTR,	25,0,4,0,0,0
opfunc ROTR

opdesc ROTCL,	30,0,4,0,0,0
opfunc ROTCL

opdesc ROTCR,	36,0,4,0,0,0
opfunc ROTCR

opdesc SHL,		20,0,4,0,0,0
opfunc SHL

opdesc SHLR,	25,0,4,0,0,0
opfunc SHLR

opdesc SHAR,	22,0,4,0,0,0
opfunc SHAR

opdesc SHLL2,	9,0,4,0,0,0
opfunc SHLL2

opdesc SHLR2,	9,0,4,0,0,0
opfunc SHLR2

opdesc SHLL8,	9,0,4,0,0,0
opfunc SHLL8

opdesc SHLR8,	9,0,4,0,0,0
opfunc SHLR8

opdesc SHLL16,	9,0,4,0,0,0
opfunc SHLL16

opdesc SHLR16,	9,0,4,0,0,0
opfunc SHLR16

opdesc AND,		16,4,12,0,0,0
opfunc AND

opdesc OR,		16,4,12,0,0,0
opfunc OR

opdesc XOR,		24,0,4,0xff,0xff,0xff
opfunc XOR
mov r2, #0  // get INSTRUCTION_B
mov r3, #0  // get INSTRUCTION_C
ldr r0,[r7,r2]   
ldr r1,[r7,r3]   
eor r0,r1        // xor  
str r0,[r7,r2]   // R[B] = xor

opdesc ADDI,	20,0xff,4,0xff,0,0xff
opfunc ADDI
mov r0, #0 // r0 = cd
mov r1, #0 // r1 = b
ldr     r3, [r7, r1]
sxtab   r0, r3, r0
str     r0, [r7, r1]


opdesc AND_B,	35,0,0,0,21,0
opfunc AND_B

opdesc OR_B,	36,0,0,0,22,0
opfunc OR_B

opdesc XOR_B,	36,0,0,0,22,0
opfunc XOR_B

opdesc TST_B,	34,0,0,0,25,0
opfunc TST_B

// Jump Opcodes
//------------

opdesc JMP,		11,0,4,0,0,0
opfunc JMP

opdesc JSR,		19,0,12,0,0,0
opfunc JSR

opdesc BRA,		29,0,0,0,0,5
opfunc BRA

opdesc BSR,		48,0xFF,0xFF,0xFF,0xFF,16
opfunc BSR
mov r1,r8          // Load PC
add r1, #4         // PC += 4
STR_PR r1          // PR(SysReg[2]) = PC
and r0, #0         // clear
add r0, #0         // get disp from instruction
tst r0, #0x0800    // if(disp&0x800)
beq BSR.continue
mvn r2, #0
and r2, #0xF000
orr r0, r2         // disp |= 0xFFFFF000
BSR.continue:
lsl r0, #1         // disp << 1
add r0, r1         // PC = PC+disp


opdesc BSRF,		24,0,4,0,0,0
opfunc BSRF
add r0, r1         // PC = PC+disp

opdesc BRAF,		16,0,4,0,0,0
opfunc BRAF

opdesc RTS,			4,0xff,0xff,0xff,0xff,0xff
opfunc RTS
LDR_PR r0

opdesc RTE,			46,0,0,0,0,0
opfunc RTE

opdesc TRAPA,	      75,0,0,0,50,0
opfunc TRAPA

opdesc BT,		30,0,0,0,18,0
opfunc BT

opdesc BF,		30,0,0,0,18,0
opfunc BF

opdesc BF_S,		32,0xFF,0xFF,0xFF,0,0xFF
opfunc BF_S
mov     r0, #0      // r0 = disp
mov     r1, r8      // r1 = PC 
LDR_SR  r2          // r2 = SR
tst     r2, #1  //
sxtbeq  r0,r0
addeq   r0, r1, r0, asl #1
addeq   r0, r0, #4
mvnne   r0, #0


//Store/Load Opcodes
//------------------

opdesc STC_SR,	10,0,4,0,0,0
opfunc STC_SR

opdesc STC_GBR,	18,0,4,0,0,0
opfunc STC_GBR

opdesc STC_VBR,	18,0,4,0,0,0
opfunc STC_VBR

opdesc STS_MACH, 10,0,4,0,0,0
opfunc STS_MACH

opdesc STS_MACH_DEC,	25,0,4,0,0,0
opfunc STS_MACH_DEC

opdesc STS_MACL, 11,0,4,0,0,0
opfunc STS_MACL

opdesc STS_MACL_DEC,	26,0,4,0,0,0
opfunc STS_MACL_DEC

opdesc LDC_SR,	21,0,4,0,0,0
opfunc LDC_SR


opdesc LDC_SR_INC,	44,0xff,0,0xff,0xff,0xff
opfunc LDC_SR_INC
mov     r1, #0  // m
ldr     r5, [r7, r1] // r5 = R[m] 
mov     r0, r5
CALL_GETMEM_LONG
bic     r0, r0, #12
mov     r0, r0, asl #22
mov     r0, r0, lsr #22  // SR = r0 & 0x000003f3;
STR_SR  r0
add     r5, r5, #4
str     r5, [r7, r1]


opdesc LDCGBR,	16,0,4,0,0,0
opfunc LDCGBR

opdesc LDC_GBR_INC,	44,0xff,0,0xff,0xff,0xff
opfunc LDC_GBR_INC
mov     r1, #0  // m
ldr     r5, [r7, r1] // r5 = R[m] 
mov     r0, r5
CALL_GETMEM_LONG
bic     r0, r0, #12
mov     r0, r0, asl #22
mov     r0, r0, lsr #22  // SR = r0 & 0x000003f3;
STR_GBR  r0
add     r5, r5, #4
str     r5, [r7, r1]


opdesc LDC_VBR,	16,0,4,0,0,0
opfunc LDC_VBR

opdesc LDC_VBR_INC,	44,0xff,0,0xff,0xff,0xff
opfunc LDC_VBR_INC
mov     r1, #0  // m
ldr     r5, [r7, r1] // r5 = R[m] 
mov     r0, r5
CALL_GETMEM_LONG
bic     r0, r0, #12
mov     r0, r0, asl #22
mov     r0, r0, lsr #22  // SR = r0 & 0x000003f3;
STR_VBR  r0
add     r5, r5, #4
str     r5, [r7, r1]


opdesc STS_PR,		12,0xFF,0,0xFF,0xff,0xff
opfunc STS_PR
mov r0, #0
LDR_PR  r1
str     r1, [r7, r0]

opdesc STSMPR,	26,0,4,0,0,0
opfunc STSMPR

opdesc LDS_PR,		11,0,4,0,0,0
opfunc LDS_PR

opdesc LDS_PR_INC,	24,0,4,0,0,0
opfunc LDS_PR_INC

opdesc LDS_MACH,		10,0,4,0,0,0
opfunc LDS_MACH

opdesc LDS_MACH_INC,	23,0,4,0,0,0
opfunc LDS_MACH_INC

opdesc LDS_MACL,		11,0,4,0,0,0
opfunc LDS_MACL

opdesc LDS_MACL_INC,	24,0,4,0,0,0
opfunc LDS_MACL_INC

//Mov Opcodes
//-----------

opdesc MOVA,	28,0xff,0xff,0xff,0,0xff
opfunc MOVA
mov     r1, #0  // disp
ldr     r0, [r7]
mov     r2, r8  // PC
add     r1, r1, #4
bic     r1, r1, #3
add     r0, r0, r1, asl #2
str     r0, [r7]
		

opdesc MOVWI,	31,0,4,0,8,0
opfunc MOVWI


opdesc MOVLI,       36,0xff,0,0xff,4,0xff
opfunc MOVLI
mov r4, #0 // n
mov r2, #0 // disp
mov r1, r8 // GET PC
add r1, r1, #4
bic r1, r1, #3
add r0, r1, r2, asl #2 // read addr
CALL_GETMEM_LONG
str r0, [r7, r4] // R[n] = readval



opdesc MOVBL4, 29,4,0,8,0,0
opfunc MOVBL4


opdesc MOVWL4, 31,4,0,8,0,0
opfunc MOVWL4


opdesc MOVLL4, 33,4,28,8,0,0
opfunc MOVLL4
 
opdesc MOVBS4,	29,7,0,15,0,0
opfunc MOVBS4

opdesc MOVWS4,	32,7,0,15,0,0
opfunc MOVWS4

opdesc MOVLS4,	36,4,12,20,0,0
opfunc MOVLS4
 
opdesc MOVBLG,    24,0,0,0,5,0
opfunc MOVBLG

opdesc MOVWLG,    26,0,0,0,5,0
opfunc MOVWLG

opdesc MOVLLG,    25,0,0,0,5,0
opfunc MOVLLG

opdesc MOVBSG,    26,0,0,0,8,0
opfunc MOVBSG

opdesc MOVWSG,    30,0,0,0,8,0
opfunc MOVWSG

opdesc MOVLSG,    30,0,0,0,8,0
opfunc MOVLSG

opdesc MOVBS,	24,4,0,0xff,0xff,0xff
opfunc MOVBS
mov r0, #0 // b
mov r1, #0 // c
ldr  r0, [r7, r0]
ldrb  r1, [r7, r1]
CALL_SETMEM_BYTE // 2cyclte

        
opdesc MOVWS,	28,4,0,0xff,0xff,0xff
opfunc MOVWS
mov r0, #0 // b
mov r1, #0 // c
ldr  r0, [r7, r0]
ldr  r1, [r7, r1]
sxth r1,r1
CALL_SETMEM_WORD // 2cyclte


opdesc MOVLS,	24,4,0,0xff,0xff,0xff
opfunc MOVLS
mov r0, #0 // b
mov r1, #0 // c
ldr  r0, [r7, r0]
ldr  r1, [r7, r1]
CALL_SETMEM_LONG // 2cyclte



opdesc MOVR,		16,4,12,0,0,0
opfunc MOVR

opdesc MOVBM,		29,4,12,0,0,0
opfunc MOVBM

opdesc MOVWM,		29,4,12,0,0,0
opfunc MOVWM

opdesc MOVLM,		29,4,12,0,0,0
opfunc MOVLM

opdesc TAS,  46,0,4,0,0,0
opfunc TAS

opdesc SUBV, 24,4,12,0,0,0
opfunc SUBV

// string cmp
opdesc CMPSTR, 60,4,12,0,0,0
opfunc CMPSTR

//-------------------------------------------------------------
//div0s 
opdesc DIV0S, 78,36,4,0,0,0
opfunc DIV0S


//===============================================================
// DIV1   1bit Divid operation
// 
// size = 69 + 135 + 132 + 38 = 374 
//===============================================================
opdesc DIV1, 410,62,7,0,0,0
opfunc DIV1

//------------------------------------------------------------
//dmuls
opdesc DMULS, 27,6,14,0,0,0
opfunc DMULS

//------------------------------------------------------------
//dmulu 32bit -> 64bit Mul
opdesc DMULU, 27,6,14,0,0,0
opfunc DMULU

//--------------------------------------------------------------
// mull 32bit -> 32bit Multip
opdesc MULL, 25,6,14,0,0,0
opfunc MULL

//--------------------------------------------------------------
// muls 16bit -> 32 bit Multip
opdesc MULS, 38,6,17,0,0,0
opfunc MULS

//--------------------------------------------------------------
// mulu 16bit -> 32 bit Multip
opdesc MULU, 38,6,17,0,0,0
opfunc MULU

//--------------------------------------------------------------
// MACL   ans = 32bit -> 64 bit MUL
//        (MACH << 32 + MACL)  + ans 
//-------------------------------------------------------------
opdesc MAC_L, 145,5,30,0,0,0 
opfunc MAC_L


//--------------------------------------------------------------
// MACW   ans = 32bit -> 64 bit MUL
//        (MACH << 32 + MACL)  + ans 
//-------------------------------------------------------------
opdesc MAC_W, 120,5,31,0,0,0
opfunc MAC_W

