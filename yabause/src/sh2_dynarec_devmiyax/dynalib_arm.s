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
  http://www.peter-cockerell.net/aalp/html/ch-3.html

  union {
    struct {
      u32 T:1;
      u32 S:1;
      u32 reserved0:2;
      u32 I:4;
      u32 Q:1;
      u32 M:1;
      u32 reserved1:22;
    } part;
    u32 all;
  } SR;


*/

/*
;Memory Functions
extern _memGetByte, _memGetWord, _memGetLong
extern _memSetByte, _memSetWord, _memSetLong
extern _EachClock, _DelayEachClock, _DebugEachClock, _DebugDelayClock
*/
 
.arch armv7-a
 
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

.macro CALL_EACHCLOCK
  STR_PC r8
  ldr r10 , [r7, #(16+3+6+6)*4 ]  
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
.align  4

//-----------------------------------------------------
// Begining of block
// r7   <- Address of GenReg
// r8   <- PC
// r9   <- Clock Counter
.global prologue
prologue:
push {r4-r10, lr}   // push regs
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
pop {r4-r10, pc} // pop regs and resturn
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
pop {r4-r10, pc} // pop regs and resturn
.size seperator_delay_after, .-seperator_delay_after // 20


//-------------------------------------------------------
// End of block
.global epilogue
epilogue:
STR_PC r8     // store PC to memory
STR_COUNT r9  // store COUNTER to memory
pop {r4-r10, pc}  // pop regs and resturn
.size	epilogue, .-epilogue // 12

//-----------------------------------------------------
// Jump part 20
.global PageFlip
PageFlip:
cmn r0, #1 // 7
bne PageFlip.jmp     // 2
STR_PC r8     // store PC to memory
STR_COUNT r9  // store COUNTER to memory
pop {r4-r10, pc} // pop regs and resturn
PageFlip.jmp:
STR_PC r0
STR_COUNT r9  // store COUNTER to memory
pop {r4-r10, pc} // pop regs and resturn

.size	PageFlip, .-PageFlip // 22

//-------------------------------------------------------
// normal part par instruction( for debug build )
.global seperator_d_normal
seperator_d_normal:
add r8, #2    // PC += 2
add r9, #1    // Clock += 1  
STR_PC r8     // store to memory
STR_COUNT r9  // store to memory
CALL_EACHCLOCK
tst r0, #1
bne seperator_d_normal.continue
pop {r4-r10, pc} // pop regs and resturn
seperator_d_normal.continue:

//------------------------------------------------------
// Delay slot part par instruction( for debug build )
.global seperator_d_delay
seperator_d_delay:
push {r0,r1}
STR_PC r8     // store to memory
STR_COUNT r9  // store to memory
CALL_EACHCLOCK
pop {r0,r1}
cmn r0, #1 // Check need to branch
bne seperator_d_delay.continue  
add r8, #2    // PC += 2
STR_PC r8
add r9, #1    // Clock += 1  
STR_COUNT r9
pop {r4-r10, pc} // pop regs and resturn
seperator_d_delay.continue:
mov r8, r0    // copy jump addr
sub r8, #2    // PC -= 2
.size seperator_delay_slot, .-seperator_delay_slot // 40


opdesc CLRT, 12,0xff,0xff,0xff,0xff,0xff
opfunc CLRT
LDR_SR r0
bic    r0, #1
STR_SR r0

opdesc CLRMAC,24,0xff,0xff,0xff,0xff,0xff
opfunc CLRMAC
LDR_MACH r0
mov      r0, #0
STR_MACH r0
LDR_MACL r0
mov      r0, #0
STR_MACL r0

opdesc NOP,		4,0xff,0xff,0xff,0xff,0xff
opfunc NOP
nop

opdesc SETT,	12,0xff,0xff,0xff,0xff,0xff
opfunc SETT
LDR_SR r0
orr r0 , #1
STR_SR r0

opdesc SLEEP,	4,0xff,0xff,0xff,0xff,0xff
opfunc SLEEP
sub r8, #2

opdesc SWAP_W,	20,0,4,0xff,0xff,0xff
opfunc SWAP_W
mov r0, #0 // m
mov r1, #0 // n
ldr r2, [r7, r0]
mov r2, r2, ror #16
str r2, [r7, r1]

opdesc SWAP_B,	(10*4),0,4,0xff,0xff,0xff
opfunc SWAP_B
mov r0, #0 // m
mov r1, #0 // n
ldr r0, [r7, r0]
mov r2, r0, asl #8
mov r3, r0, asl #16
uxth  r2, r2
mov r0, r0, lsr #16
orr r3, r2, r3, lsr #24
orr r3, r3, r0, asl #16
str r3, [r7, r1]
 



REV r3, r2
str r3, [r7, r1]

opdesc TST,	36,0,4,0xff,0xff,0xff
opfunc TST
mov r0, #0 // m
mov r1, #0 // n
LDR_SR r2
ldr     r1, [r7, r1]
ldr     r3, [r7, r0]
tst     r1, r3
orreq   r2, r2, #1
bicne   r2, r2, #1
STR_SR r2


opdesc TSTI,	28,0xff,0xff,0xff,0,0xff
opfunc TSTI
mov     r0, #0   // n
LDR_SR  r1
ldr     r3, [r7] // R[0]
tst     r0, r3
orreq   r0, r1, #1
bicne   r0, r1, #1
STR_SR  r0

opdesc ANDI,	16,0xff,0xff,0xff,0,0xff
opfunc ANDI
mov  r0, #0
ldr  r3, [r7]
and  r0, r0, r3
str  r0, [r7]

opdesc XORI,	16,0xff,0xff,0xff,0,0xff
opfunc XORI
mov  r0, #0
ldr  r3, [r7]
eor  r0, r3, r0
str  r0, [r7]

opdesc ORI,	16,0xff,0xff,0xff,0,0xff
opfunc ORI
mov  r0, #0
ldr  r3, [r7]
orr  r0, r0, r3
str  r0, [r7]

opdesc XTRCT,	28,0,4,0xff,0xff,0xff
opfunc XTRCT
mov r0, #0 // m
mov r1, #0 // n
ldr     r3, [r7, r1]
ldr     r0, [r7, r0]
mov     r3, r3, lsr #16
orr     r3, r3, r0, asl #16
str     r3, [r7, r1]

opdesc ADD,		24,0,4,0xff,0xff,0xff
opfunc ADD
mov r1, #0 // b
mov r0, #0 // c
ldr r1, [r7, r1]
ldr r3, [r7, r0]
add r3, r3, r1
str r3, [r7, r0]

opdesc ADDC,	52,0,4,0xff,0xff,0xff
opfunc ADDC
mov r0, #0 // source
mov r1, #0 // dest
ldr r2, [r7, r0] // r2 = R[source]
LDR_SR r0        // r0 = SR
ldr r3, [r7, r1] // r3 = R[dest]
adds r2, r3       // r2+r3
and r3, r0, #1   // r3 = r0 & 1
orrcs r4, r0, #1 // check carry
biccc r4, r0, #1 // check not carry
adds r2, r3
orrcs r4, r0, #1 // check carry
STR_SR r4
str r2, [r7, r1]

opdesc ADDV, 40,0,4,0xff,0xff,0xff
opfunc ADDV
mov r0, #0 // source
mov r1, #0 // dest
ldr r2, [r7, r0] // r2 = R[source]
LDR_SR r0        // r0 = SR
ldr r3, [r7, r1] // r3 = R[dest]
adds r2, r3       // r2+r3
orrvs r4, r0, #1 // check overflow
bicvc r4, r0, #1 // check not overflow
STR_SR r4
str r2, [r7, r1]

opdesc SUBV, 40,0,4,0xff,0xff,0xff
opfunc SUBV
mov r0, #0 // source
mov r1, #0 // dest
ldr r2, [r7, r0] // r2 = R[source]
LDR_SR r0        // r0 = SR
ldr r3, [r7, r1] // r3 = R[dest]
subs r2, r3       // r2+r3
orrvs r4, r0, #1 // check overflow
bicvc r4, r0, #1 // check not overflow
STR_SR r4
str r2, [r7, r1]


opdesc SUBC,	52,0,4,0xff,0xff,0xff
opfunc SUBC
mov r0, #0 // source
mov r1, #0 // dest
ldr r2, [r7, r0] // r2 = R[source]  
LDR_SR r0        // r0 = SR
ldr r3, [r7, r1] // r3 = R[dest]
subs r3, r2       // r3-r2
and r2, r0, #1   // r3 = r0 & 1
orrcc r4, r0, #1 // check carry
biccs r4, r0, #1 // check not carry
subs r3, r2      // r3 = r3 -T
orrcc r4, r0, #1 // check carry
STR_SR r4
str r3, [r7, r1]


opdesc SUB,		24,0,4,0xff,0xff,0xff
opfunc SUB
mov r1, #0 // b
mov r0, #0 // c
ldr r1, [r7, r1]
ldr r3, [r7, r0]
sub r3, r3, r1
str r3, [r7, r0]


opdesc NOT,		20,0,4,0xff,0xff,0xff
opfunc NOT
mov r0, #0 // m
mov r1, #0 // n
ldr  r3, [r7, r0]
mvn  r3, r3
str  r3, [r7, r1]

opdesc NEG,		20,0,4,0xff,0xff,0xff
opfunc NEG
mov r0, #0 // m
mov r1, #0 // n
ldr  r3, [r7, r0]
rsb  r3, r3, #0
str  r3, [r7, r1]

opdesc NEGC,	48,0,4,0xff,0xff,0xff
opfunc NEGC
mov r0, #0 // source
mov r1, #0 // dest
ldr r2, [r7, r0] // r2 = R[source]  
LDR_SR r0        // r0 = SR
ldr r3, [r7, r1] // r3 = R[dest]
rsb r2, r3, #0   // r2 = 0 - r3
and r3, r0, #1   // r3 = r0 & 1
orrcs r4, r0, #1 // check carry
biccc r4, r0, #1 // check not carry
sub r2, r3
STR_SR r4
str r2, [r7, r1]

opdesc EXTUB,	16,0,4,0xff,0xff,0xff
opfunc EXTUB
mov r0, #0 // m
mov r1, #0 // n
ldrb    r3, [r7, r0]    @ zero_extendqisi2
str     r3, [r7, r1]

opdesc EXTU_W,	20,0,4,0xff,0xff,0xff
opfunc EXTU_W
mov r0, #0 // m
mov r1, #0 // n
ldr  r3, [r7, r0]
uxth r3, r3
str  r3, [r7, r1]

opdesc EXTS_B,	20,0,4,0xff,0xff,0xff
opfunc EXTS_B
mov r0, #0 // m
mov r1, #0 // n
ldr  r3, [r7, r0]
sxtb r3, r3
str  r3, [r7, r1]

opdesc EXTS_W,	20,0,4,0xff,0xff,0xff
opfunc EXTS_W
mov r0, #0 // m
mov r1, #0 // n
ldr  r3, [r7, r0]
sxth r3, r3
str  r3, [r7, r1]

//Store Register Opcodes
//----------------------

opdesc STC_SR_MEM,	28,0xff,0,0xff,0xff,0xff
opfunc STC_SR_MEM
mov r2, #0  // n
ldr r0, [r7, r2] // R[n]
sub r0, #4       // 
str r0, [r7, r2] // R[n] -= 4
LDR_SR r1
CALL_SETMEM_LONG


opdesc STC_GBR_MEM,	28,0xff,0,0xff,0xff,0xff
opfunc STC_GBR_MEM
mov r2, #0  // n
ldr r0, [r7, r2] // R[n]
sub r0, #4       // 
str r0, [r7, r2] // R[n] -= 4
LDR_GBR r1
CALL_SETMEM_LONG

opdesc STC_VBR_MEM,	28,0xff,0,0xff,0xff,0xff
opfunc STC_VBR_MEM
mov r2, #0  // n
ldr r0, [r7, r2] // R[n]
sub r0, #4       // 
str r0, [r7, r2] // R[n] -= 4
LDR_VBR r1
CALL_SETMEM_LONG

opdesc MOVBL,	28,0,4,0xff,0xff,0xff
opfunc MOVBL
mov  r0, #0  // m
mov  r4, #0  // n
ldr  r0, [r7, r0] // r0 = R[m] 
CALL_GETMEM_BYTE
sxtb r0, r0
str  r0, [r7, r4]


opdesc MOVWL,		28,0,4,0xff,0xff,0xff
opfunc MOVWL
mov  r0, #0  // m
mov  r4, #0  // n
ldr  r0, [r7, r0] // r0 = R[m] 
CALL_GETMEM_WORD
sxth r0, r0
str  r0, [r7, r4]


opdesc MOVL_MEM_REG, 28,0,4,0xff,0xff,0xff
opfunc MOVL_MEM_REG
mov r1,  #0  // c
mov r0,  #0  // b
mov r5, r0
ldr r0, [r7, r1]
CALL_GETMEM_LONG
str r0, [r7, r5]


opdesc MOVBP,	52,0,4,0xff,0xff,0xff
opfunc MOVBP
mov  r0, #0 // m
mov  r1, #0 // n
mov  r5, r0 
ldr  r0, [r7, r0]
mov  r6, r1
CALL_GETMEM_BYTE
sxtb r0, r0
str  r0, [r7, r6]
cmp  r6, r5
ldrne  r3, [r7, r5]
addne  r3, r3, #1
strne  r3, [r7, r5]


opdesc MOVWP,	52,0,4,0xff,0xff,0xff
opfunc MOVWP
mov  r0, #0 // m
mov  r1, #0 // n
mov  r5, r0 
ldr  r0, [r7, r0]
mov  r6, r1
CALL_GETMEM_WORD
sxth r0, r0
str  r0, [r7, r6]
cmp  r6, r5
ldrne  r3, [r7, r5]
addne  r3, r3, #2
strne  r3, [r7, r5]



opdesc MOVLP,	48,0,4,0xff,0xff,0xff
opfunc MOVLP
mov  r0, #0 // m
mov  r1, #0 // n
mov  r5, r0 
ldr  r0, [r7, r0]
mov  r6, r1
CALL_GETMEM_LONG
cmp  r6, r5
str  r0, [r7, r6]
ldrne  r3, [r7, r5]
addne  r3, r3, #4
strne  r3, [r7, r5]

opdesc MOVI,	16,0xff,0,0xff,4,0xff
opfunc MOVI
mov r1, #0  // n
mov r0, #0  // i
sxtb r0, r0 // (signed char)i
str  r0, [r7, r1] // R[n] = (int)i


//----------------------

opdesc MOVBL0,	40,0,4,0xff,0xff,0xff
opfunc MOVBL0
mov r1,  #0  
mov r0 , #0  
mov r5, r0
ldr r0, [r7, r1]
ldr r1, [r7]
add r0, r1
CALL_GETMEM_BYTE
sxtb r0,r0
str r0, [r7, r5]


opdesc MOVWL0,	40,0,4,0xff,0xff,0xff
opfunc MOVWL0
mov r1,  #0  
mov r0 , #0  
mov r5, r0
ldr r0, [r7, r1]
ldr r1, [r7]
add r0, r1
CALL_GETMEM_WORD
sxth r0,r0
str r0, [r7, r5]


opdesc MOVLL0,	36,0,4,0xff,0xff,0xff
opfunc MOVLL0
mov r1,  #0  
mov r0 , #0  
mov r5, r0
ldr r0, [r7, r1]
ldr r1, [r7]
add r0, r1
CALL_GETMEM_LONG
str r0, [r7, r5]


opdesc MOVT,	16,0xff,0x0,0xff,0xff,0xff 
opfunc MOVT
mov r0,  #0  
LDR_SR r1
and r1, #1
str r1, [r7, r0];

opdesc MOVBS0,	24,0x0,0x4,0xff,0xff,0xff
opfunc MOVBS0
ldr r1, [r7, #0]
ldr r0, [r7, #0]
ldr r2, [r7]
add r0, r0, r2
CALL_SETMEM_BYTE


opdesc MOVWS0,	24,0x0,0x4,0xff,0xff,0xff
opfunc MOVWS0
ldr r1, [r7, #0]
ldr r0, [r7, #0]
ldr r2, [r7]
add r0, r0, r2
CALL_SETMEM_WORD

opdesc MOVLS0,	24,0x0,0x4,0xff,0xff,0xff
opfunc MOVLS0
ldr r1, [r7, #0]
ldr r0, [r7, #0]
ldr r2, [r7]
add r0, r0, r2
CALL_SETMEM_LONG

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

opdesc CMP_PZ,	24,0xff,0,0xff,0xff,0xff
opfunc CMP_PZ
ldr     r0, [r7, #0]
LDR_SR  r1
cmp     r0, #0
orrge   r0, r1, #1
biclt   r0, r1, #1
STR_SR  r0

opdesc CMP_PL,	24,0xff,0,0xff,0xff,0xff
opfunc CMP_PL
ldr     r0, [r7, #0]
LDR_SR  r1
cmp     r0, #0
orrgt   r0, r1, #1
bicle   r0, r1, #1
STR_SR  r0


opdesc CMP_EQ,	28,0,4,0xff,0xff,0xff
opfunc CMP_EQ
ldr     r3, [r7, #0]
ldr     r0, [r7, #0]
LDR_SR  r1
cmp     r0, r3
orreq   r0, r1, #1
bicne   r0, r1, #1
STR_SR  r0

opdesc CMP_HS,	28,0,4,0xff,0xff,0xff
opfunc CMP_HS
ldr     r3, [r7, #0]
ldr     r0, [r7, #0]
LDR_SR  r1
cmp     r0, r3
orrcs   r0, r1, #1
biccc   r0, r1, #1
STR_SR  r0


opdesc CMP_HI,	28,0,4,0xff,0xff,0xff
opfunc CMP_HI
ldr     r3, [r7, #0]
ldr     r0, [r7, #0]
LDR_SR  r1
cmp     r0, r3
orrhi   r0, r1, #1
bicls   r0, r1, #1
STR_SR  r0


opdesc CMP_GE,	28,0,4,0xff,0xff,0xff
opfunc CMP_GE
ldr     r3, [r7, #0]
ldr     r0, [r7, #0]
LDR_SR  r1
cmp     r0, r3
orrge   r0, r1, #1
biclt   r0, r1, #1
STR_SR  r0


opdesc CMP_GT,	28,0,4,0xff,0xff,0xff
opfunc CMP_GT
ldr     r3, [r7, #0]
ldr     r0, [r7, #0]
LDR_SR  r1
cmp     r0, r3
orrgt   r0, r1, #1
bicle   r0, r1, #1
STR_SR  r0

opdesc CMP_EQ_IMM,	32,0xff,0xff,0xff,0,0xff
opfunc CMP_EQ_IMM
mov r0, #0 // imm
sxtb r0,r0
LDR_SR  r1
ldr     r3, [r7]
cmp     r3, r0
orreq   r0, r1, #1
bicne   r0, r1, #1
STR_SR  r0

// string cmp
opdesc CMPSTR, (29*4),0,4,0xff,0xff,0xff
opfunc CMPSTR
mov r0, #0 // m
mov r1, #0 // n
ldr r1, [r7, r1] // R[n]
ldr r3, [r7, r0] // R[m]
eor r3, r3, r1   // temp= R[n]^R[m];
movs r1, r3, lsr #24 // r1 = temp>>24
mov  r0, r3, asr #16 // r0 = temp>>16
movne   r1, #1       
moveq   r1, #0
tst     r0, #255
moveq   r1, #0
andne   r1, r1, #1
cmp     r1, #0
beq     CMPSTR.L2
mov     r1, r3, asr #8
tst     r1, #255
movne   r1, #1
moveq   r1, #0
tst     r3, #255
moveq   r3, #0
andne   r3, r1, #1
cmp     r3, #0
beq     CMPSTR.L2
LDR_SR  r2
bic     r0, r2, #1
b CMPSTR.FINISH
CMPSTR.L2:
LDR_SR  r2
orr     r0, r2, #1
CMPSTR.FINISH:
STR_SR  r0


//http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0489fj/CIHDDCIF.html

opdesc ROTL,	(9*4),0xff,0,0xff,0xff,0xff
opfunc ROTL
mov r0, #0   // n
LDR_SR r1    // r1 = SR
ldr r2, [r7, r0]
lsls r2, r2, #1       // r2 <<= 1  
orrcs r1, #0x01  // if( C==1 ) T=1;
orrcs r2, #0x01  // if( C==1 ) r2 |= 1;
biccc r1, #0x01  // if( C==0 ) T=0;
str r2, [r7, r0]
STR_SR r1 


opdesc ROTR,	32,0xff,0,0xff,0xff,0xff
opfunc ROTR
mov r0, #0   // n
LDR_SR r1    // r1 = SR
ldr r2, [r7, r0]
rors r2, #1       // r2 >>= 1  
orrcs r1, #0x01  // if( C==1 ) T=1;
biccc r1, #0x01  // if( C==0 ) T=0;
str r2, [r7, r0]
STR_SR r1 

opdesc ROTCL,	40,0xff,0,0xff,0xff,0xff
opfunc ROTCL
mov r0, #0   // n
LDR_SR r1    // r1 = SR
and r3, r1 , #1 // r3 = SR.T
ldr r2, [r7, r0]
lsls r2, r2, #1       // r2 <<= 1  
biccc r1, r1, #1  // if( C==0 ) T=0;
orrcs r1,  #1  // if( C==1 ) T=1;
orr r2, r3
str r2, [r7, r0]
STR_SR r1 


opdesc ROTCR,	44,0xff,0,0xff,0xff,0xff
opfunc ROTCR
mov r0, #0   // n
LDR_SR r1    // r1 = SR
and r3, r1 , #1 // r3 = SR.T
lsl r3, #31     // 
ldr r2, [r7, r0]
lsrs r2, #1       // r2 <<= 1  
orrcs r1, #0x01  // if( C==1 ) T=1;
biccc r1, #0x01  // if( C==0 ) T=0;
orr r2, r3
str r2, [r7, r0]
STR_SR r1 

opdesc SHL,		32,0xff,0,0xff,0xff,0xff
opfunc SHL
mov r0, #0 
LDR_SR r1
ldr r2, [r7, r0]
lsls r2, #1       // r2 <<= 1  
orrcs r1, #0x01  // if( C==1 ) T=1;
biccc r1, #0x01  // if( C==0 ) T=0;
str r2, [r7, r0]
STR_SR r1 


opdesc SHLR,	32,0xff,0,0xff,0xff,0xff
opfunc SHLR
mov r0, #0 
LDR_SR r1
ldr r2, [r7, r0]
lsrs r2, #1  // r2 >>= 1
orrcs r1, #0x01  // if( C==1 ) T=1;
biccc r1, #0x01  // if( C==0 ) T=0;
str r2, [r7, r0]
STR_SR r1 

opdesc SHAR,	(8*4),0xff,0,0xff,0xff,0xff
opfunc SHAR
mov r0, #0 
LDR_SR r1
ldr r2, [r7, r0]
asrs r2, #1  // r2 >>= 1
orrcs r1, #0x01  // if( C==1 ) T=1;
biccc r1, #0x01  // if( C==0 ) T=0;
str r2, [r7, r0]
STR_SR r1 


opdesc SHLL2,	16,0xff,0,0xff,0xff,0xff
opfunc SHLL2
mov r0, #0 
ldr r2, [r7, r0]
mov r2, r2, lsl #2
str r2, [r7, r0]

opdesc SHLR2,	16,0xff,0,0xff,0xff,0xff
opfunc SHLR2
mov r0, #0 
ldr r2, [r7, r0]
mov r2, r2, lsr #2
str r2, [r7, r0]

opdesc SHLL8,	16,0xff,0,0xff,0xff,0xff
opfunc SHLL8
mov r0, #0 
ldr r2, [r7, r0]
mov r2, r2, lsl #8
str r2, [r7, r0]

opdesc SHLR8,	16,0xff,0,0xff,0xff,0xff
opfunc SHLR8
mov r0, #0 
ldr r2, [r7, r0]
mov r2, r2, lsr #8
str r2, [r7, r0]


opdesc SHLL16,	16,0xff,0,0xff,0xff,0xff
opfunc SHLL16
mov r0, #0 
ldr r2, [r7, r0]
mov r2, r2, lsl #16
str r2, [r7, r0]

opdesc SHLR16,	16,0xff,0,0xff,0xff,0xff
opfunc SHLR16
mov r0, #0 
ldr r2, [r7, r0]
mov r2, r2, lsr #16
str r2, [r7, r0]


opdesc AND,		24,0,4,0xff,0xff,0xff
opfunc AND
mov r0, #0
mov r1, #0
ldr r3, [r7, r1]
ldr r0, [r7, r0]
and r3, r3, r0
str r3, [r7, r1]

opdesc OR,		24,0,4,0xff,0xff,0xff
opfunc OR
mov r0, #0
mov r1, #0
ldr r3, [r7, r1]
ldr r0, [r7, r0]
orr r3, r3, r0
str r3, [r7, r1]


opdesc XOR,		24,0,4,0xff,0xff,0xff
opfunc XOR
mov r2, #0  // get INSTRUCTION_C
mov r3, #0  // get INSTRUCTION_B
ldr r0,[r7,r2]   
ldr r1,[r7,r3]   
eor r0,r1        // xor  
str r0,[r7,r3]   // R[B] = xor

opdesc ADDI,	20,0xff,4,0xff,0,0xff
opfunc ADDI
mov r0, #0 // r0 = cd
mov r1, #0 // r1 = b
ldr     r3, [r7, r1]
sxtab   r0, r3, r0
str     r0, [r7, r1]


opdesc AND_B,	44,0xff,0xff,0xff,0,0xff
opfunc AND_B
mov     r0, #0     // source
LDR_GBR r5         // GBR
ldr     r6, [r7]   // R[0]
mov     r4, r0     // 
add     r0, r5, r6 // GBR+R0
CALL_GETMEM_BYTE
and     r1, r0, r4  // temp &= source
add     r0, r5, r6  // GBR+R0
CALL_SETMEM_BYTE


opdesc OR_B,	44,0xff,0xff,0xff,0,0xff
opfunc OR_B
mov     r0, #0     // source
LDR_GBR r5         // GBR
ldr     r6, [r7]   // R[0]
mov     r4, r0     // 
add     r0, r5, r6 // GBR+R0
CALL_GETMEM_BYTE
orr     r1, r0, r4  // temp |= source
add     r0, r5, r6  // GBR+R0
CALL_SETMEM_BYTE


opdesc XOR_B,	44,0xff,0xff,0xff,0,0xff
opfunc XOR_B
mov     r0, #0     // source
LDR_GBR r5         // GBR
ldr     r6, [r7]   // R[0]
mov     r4, r0     // 
add     r0, r5, r6 // GBR+R0
CALL_GETMEM_BYTE
eor     r1, r0, r4  // temp ^= source
add     r0, r5, r6  // GBR+R0
CALL_SETMEM_BYTE


opdesc TST_B,	(10*4),0xff,0xff,0xff,20,0xff
opfunc TST_B
LDR_GBR r0
ldr     r1, [r7]
add     r0, r0, r1 
CALL_GETMEM_BYTE    // MappedMemoryReadByte( GBR+R[0] );
tst     r0, #0      // temp&=i;
LDR_SR  r1
orreq   r1, r1, #1
bicne   r1, r1, #1
STR_SR  r1


// Jump Opcodes
//------------

opdesc JMP,		8,0xff,0,0xff,0xff,0xff
opfunc JMP
mov r0, #0
ldr r0, [r7,r0]

opdesc JSR,		16,0xff,0,0xff,0xff,0xff
opfunc JSR
mov r0, #0
add r1, r8, #4
STR_PR r1
ldr r0, [r7,r0]

opdesc BRA,		36,0xFF,0xFF,0xFF,0xFF,0
opfunc BRA
mov r0, #0 // for arm5
mov r1, #0
lsl r0, #8
orr r0, r1
tst     r0, #2048
mvnne   r0, r0, asl #20
mvnne   r0, r0, asr #20
add     r0, r8, r0, asl #1
add     r0, r0, #4

opdesc BSR,		(13*4),0xFF,0xFF,0xFF,0xFF,0
opfunc BSR
mov r0, #0 // for arm5
mov r2, #0
lsl r0, #8
orr r0, r2
mov r1, r8 
add r1, r1, #4   // PC += 4
STR_PR r1    // PR(SysReg[2]) = PC
uxth r3, r0
tst  r3, #2048
mvnne r3, r3, asl #20
mvnne r3, r3, lsr #20
sxthne r0, r3
add r0, r1, r0, asl #1

/*
mov r1,r8          // Load PC
add r1, #4         // PC += 4
STR_PR r1          // PR(SysReg[2]) = PC
mov r0, #0         // get disp from instruction
tst r0, #0x0800    // if(disp&0x800)
beq BSR.continue
mvn r2, #0
and r2, #0xF000
orr r0, r2         // disp |= 0xFFFFF000
BSR.continue:
lsl r0, #1         // disp << 1
add r0, r1         // PC = PC+disp
*/

opdesc BSRF,		28,0xff,0,0xff,0xff,0xff  // ToDo:
opfunc BSRF
mov r0, #0 // m
ldr r0, [r7, r0]
mov r1, r8
add r1, #4
STR_PR r1
add r0, r8
add r0, #4


opdesc BRAF,		16,0xff,0,0xff,0xff,0xff
opfunc BRAF
mov r0, #0 // m
ldr r0, [r7, r0]
add r0, r0, #4
add r0, r0, r8

opdesc RTS,			4,0xff,0xff,0xff,0xff,0xff
opfunc RTS
LDR_PR r0

opdesc RTE,			68,0xff,0xff,0xff,0xff,0xff
opfunc RTE
ldr     r0, [r7, #60]
CALL_GETMEM_LONG
mov     r5, r0 // PC
ldr     r0, [r7, #60]
add     r0, r0, #4
str     r0, [r7, #60]
CALL_GETMEM_LONG
ldr     r3, [r7, #60]
add     r3, r3, #4
str     r3, [r7, #60]
bic     r0, r0, #12
mov     r0, r0, asl #22
mov     r0, r0, lsr #22 // SR & 0x000003F3
STR_SR  r0
mov     r0, r5

opdesc TRAPA,	  (17*4),0xff,0xff,0xff,(13*4),0xff
opfunc TRAPA
ldr r0, [r7, #60]  // r0 = R[15]
sub r0, #4         // r0 -= 4
LDR_SR r1          // r1 = SR
CALL_SETMEM_LONG   // MappedMemoryWriteLong(R[15],SR);
ldr r0, [r7, #60]  // r0 = R[15]
sub r0, #8         // r0 -= 8
str r0, [r7, #60]  // R[15] = r0
mov r1, r8         // r1 = PC
add r1, #2         // r1 += 2
CALL_SETMEM_LONG   // MappedMemoryWriteLong(R[15],PC + 2);
LDR_VBR r0         // VBR
mov r1, #0         // r1 = imm
add r0, r0, r1, asl #2 // PC = MappedMemoryReadLong(VBR+(imm<<2));
CALL_GETMEM_LONG


opdesc BT,		32,0xff,0xff,0xff,0,0xff
opfunc BT
mov r0, #0 // disp
sxtb r0,r0
LDR_SR r1
mov r2, r8 // PC
tst     r1, #1
addne   r0, r2, r0, asl #1
addne   r0, r0, #4
mvneq   r0, #0

opdesc BF,		32,0xff,0xff,0xff,0,0xff
opfunc BF
mov r0, #0 // disp
sxtb r0,r0
LDR_SR r1
mov r2, r8 // PC
tst     r1, #1
addeq   r0, r2, r0, asl #1
addeq   r0, r0, #4
mvnne   r0, #0

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

opdesc STC_SR,	8,0xff,4,0xff,0xff,0xff
opfunc STC_SR
LDR_SR r1
str r1, [r7, #0 ] // R[n] = SR

opdesc STC_GBR,	8,0xff,4,0xff,0xff,0xff
opfunc STC_GBR
LDR_GBR r1
str r1, [r7, #0 ] // R[n] = GBR

opdesc STC_VBR,	8,0xff,4,0xff,0xff,0xff
opfunc STC_VBR
LDR_VBR r1
str r1, [r7, #0 ] // R[n] = VBR

opdesc STS_MACH, 8,0xff,4,0xff,0xff,0xff
opfunc STS_MACH
LDR_MACH r1
str r1, [r7, #0 ] // R[n] = MACL


opdesc STS_MACH_DEC, 28,0xff,0,0xff,0xff,0xff
opfunc STS_MACH_DEC
mov r2, #0
ldr r0, [r7, r2 ]
sub r0, #4
str r0, [r7, r2 ]
LDR_MACH r1
CALL_SETMEM_LONG


opdesc STS_MACL, 8,0xff,4,0xff,0xff,0xff
opfunc STS_MACL
LDR_MACL r1
str r1, [r7, #0 ] // R[n] = MACL

opdesc STS_MACL_DEC, 28,0xff,0,0xff,0xff,0xff
opfunc STS_MACL_DEC
mov r2, #0
ldr r0, [r7, r2 ]
sub r0, #4
str r0, [r7, r2 ]
LDR_MACL r1
CALL_SETMEM_LONG

opdesc LDC_SR,	24,0xff,0,0xff,0xff,0xff
opfunc LDC_SR
mov     r1, #0  // m
ldr     r0, [r7, r1] // r5 = R[m] 
bic     r0, r0, #12
mov     r0, r0, asl #22
mov     r0, r0, lsr #22  // SR = r0 & 0x000003f3;
STR_SR  r0

opdesc LDC_SR_INC,	(12*4),0xff,0,0xff,0xff,0xff
opfunc LDC_SR_INC
mov     r1, #0  // m
ldr     r5, [r7, r1] // r5 = R[m] 
mov     r4, r1
mov     r0, r5
CALL_GETMEM_LONG
bic     r0, r0, #12
mov     r0, r0, asl #22
mov     r0, r0, lsr #22  // SR = r0 & 0x000003f3;
STR_SR  r0
add     r5, r5, #4
str     r5, [r7, r4]


opdesc LDCGBR,	12,0xff,0,0xff,0xff,0xff
opfunc LDCGBR
mov     r0, #0  // m
ldr     r0, [r7, r0] // r5 = R[m] 
STR_GBR  r0
 

opdesc LDC_GBR_INC,	(7*4),0xff,0,0xff,0xff,0xff
opfunc LDC_GBR_INC
mov r2, #0 // m
ldr r0, [r7, r2 ]
add r1, r0 , #4
str r1, [r7, r2 ]
CALL_GETMEM_LONG
STR_GBR r0 


opdesc LDC_VBR,	12,0xff,0,0xff,0xff,0xff
opfunc LDC_VBR
mov     r0, #0  // m
ldr     r0, [r7, r0] // r5 = R[m] 
STR_VBR  r0

opdesc LDC_VBR_INC,	(7*4),0xff,0,0xff,0xff,0xff
opfunc LDC_VBR_INC
mov r2, #0 // m
ldr r0, [r7, r2 ]
add r1, r0 , #4
str r1, [r7, r2 ]
CALL_GETMEM_LONG
STR_VBR r0 


opdesc STS_PR,		12,0xFF,0,0xFF,0xff,0xff
opfunc STS_PR
mov r0, #0
LDR_PR  r1 
str     r1, [r7, r0]

opdesc STSMPR,	28,0xff,0,0xff,0xff,0xff
opfunc STSMPR
mov r4, #0
ldr r0, [r7, r4 ]
sub r0, #4
str r0, [r7, r4 ]
LDR_PR r1
CALL_SETMEM_LONG

opdesc LDS_PR,		12,0xff,0,0xff,0xff,0xff
opfunc LDS_PR
mov r0, #0 // b
ldr r0, [r7, r0]
STR_PR r0

opdesc LDS_PR_INC,	28,0xff,0,0xff,0xff,0xff
opfunc LDS_PR_INC
mov r2, #0 // m
ldr r0, [r7, r2 ]
add r1, r0 , #4
str r1, [r7, r2 ]
CALL_GETMEM_LONG
STR_PR r0       // MACH = ReadLong(sh->regs.R[m]);


opdesc LDS_MACH,		12,0xff,0,0xff,0xff,0xff
opfunc LDS_MACH
mov r0, #0 // b
ldr r0, [r7, r0]
STR_MACH r0


opdesc LDS_MACH_INC,	28,0xff,0,0xff,0xff,0xff
opfunc LDS_MACH_INC
mov r2, #0 // m
ldr r0, [r7, r2 ]
add r1, r0 , #4
str r1, [r7, r2 ]
CALL_GETMEM_LONG
STR_MACH r0       // MACH = ReadLong(sh->regs.R[m]);


opdesc LDS_MACL,		12,0xff,0,0xff,0xff,0xff
opfunc LDS_MACL
mov r0, #0 // b
ldr r0, [r7, r0]
STR_MACL r0


opdesc LDS_MACL_INC,	28,0xff,0,0xff,0xff,0xff
opfunc LDS_MACL_INC
mov r2, #0 // m
ldr r0, [r7, r2 ]
add r1, r0 , #4
str r1, [r7, r2 ]
CALL_GETMEM_LONG
STR_MACL r0       // MACH = ReadLong(sh->regs.R[m]);

//Mov Opcodes
//-----------

opdesc MOVA,	24,0xff,0xff,0xff,0,0xff
opfunc MOVA
mov     r1, #0  // disp
mov     r2, r8  // PC
add     r2, r2, #4
bic     r2, r2, #3
add     r2, r2, r1, asl #2
str     r2, [r7]


opdesc MOVWI,	40,0xff,0,0xff,4,0xff
opfunc MOVWI
mov r4, #0 // n
mov r2, #0 // disp
mov r1, r8 // GET PC
add r1, r1, r2, asl #1 // PC += (disp << 1)
add r0, r1, #4  // PC += 4
CALL_GETMEM_WORD
sxth r0,r0
str r0, [r7, r4] // R[n] = readval

opdesc MOVLI,  36,0xff,0,0xff,4,0xff
opfunc MOVLI
mov r4, #0 // n
mov r2, #0 // disp
mov r1, r8 // GET PC
add r1, r1, #4
bic r1, r1, #3
add r0, r1, r2, asl #2 // read addr
CALL_GETMEM_LONG
str r0, [r7, r4] // R[n] = readval


opdesc MOVBL4, 28,0,0xff,4,0xff,0xff
opfunc MOVBL4
ldr r2, [r7, #0 ]
mov r1, #0 // disp
add r0, r2, r1
CALL_GETMEM_BYTE
sxtb r0,r0
str  r0, [r7, #0]

opdesc MOVWL4, 28,0,0xff,4,0xff,0xff
opfunc MOVWL4
ldr r2, [r7, #0]
mov r1, #0 // disp
add r0, r2, r1, asl #1
CALL_GETMEM_WORD
sxth r0,r0
str  r0, [r7, #0]

opdesc MOVLL4, 24,0,20,4,0xff,0xff
opfunc MOVLL4
ldr r2, [r7, #0]
mov r1, #0 // disp
add r0, r2, r1, asl #2
CALL_GETMEM_LONG
str r0, [r7, #0]

opdesc MOVBS4,	28,0,0xff,4,0xff,0xff
opfunc MOVBS4
mov r0, #0 // n
mov r1, #0 // disp
ldr r2, [r7, r0]
add r0, r2, r1
ldr r1, [r7]
CALL_SETMEM_BYTE

opdesc MOVWS4,	28,0,0xff,4,0xff,0xff
opfunc MOVWS4
mov r0, #0 // n
mov r1, #0 // disp
ldr r2, [r7, r0]
add r0, r2, r1, asl #1
ldr r1, [r7]
CALL_SETMEM_WORD

opdesc MOVLS4,	36,0,8,4,0xff,0xff
opfunc MOVLS4
mov r0, #0 // m
mov r1, #0 // disp
mov r2, #0 // n
ldr r2, [r7, r2 ]
ldr r3, [r7, r0 ]
add r0, r2, r1, asl #2
mov r1, r3
CALL_SETMEM_LONG

opdesc MOVBLG,    28,0xff,0xff,0xff,0,0xff
opfunc MOVBLG
mov r0, #0
LDR_GBR r1
add     r0, r1, r0
CALL_GETMEM_BYTE
sxtb r0,r0
str  r0, [r7]

opdesc MOVWLG,     28,0xff,0xff,0xff,0,0xff
opfunc MOVWLG
mov r0, #0
LDR_GBR r1
add     r0, r1, r0, asl #1
CALL_GETMEM_WORD
sxth r0,r0
str  r0, [r7]

opdesc MOVLLG,    24,0xff,0xff,0xff,0,0xff
opfunc MOVLLG
mov r0, #0
LDR_GBR r1
add     r0, r1, r0, asl #2
CALL_GETMEM_LONG
str     r0, [r7]

opdesc MOVBSG,    24,0xff,0xff,0xff,0,0xff
opfunc MOVBSG
mov r0, #0 // disp
LDR_GBR r1 // GBR
add     r0, r1
ldr     r1, [r7]
CALL_SETMEM_BYTE

opdesc MOVWSG,    24,0xff,0xff,0xff,0,0xff
opfunc MOVWSG
mov r0, #0 // disp
LDR_GBR r1 // GBR
add     r0, r1, r0, asl #1
ldr     r1, [r7]
CALL_SETMEM_WORD

opdesc MOVLSG,    24,0xff,0xff,0xff,0,0xff
opfunc MOVLSG
mov r0, #0 // disp
LDR_GBR r1 // GBR
add     r0, r1, r0, asl #2
ldr     r1, [r7]
CALL_SETMEM_LONG

opdesc MOVBS,	24,4,0,0xff,0xff,0xff
opfunc MOVBS
mov r0, #0 // b
mov r1, #0 // c
ldr  r0, [r7, r0]
ldrb  r1, [r7, r1]
CALL_SETMEM_BYTE // 2cyclte

        
opdesc MOVWS,	(6*4),4,0,0xff,0xff,0xff
opfunc MOVWS
mov r0, #0 // b
mov r1, #0 // c
ldr  r0, [r7, r0]
ldr  r1, [r7, r1]
CALL_SETMEM_WORD // 2cyclte

opdesc MOVLS,	24,4,0,0xff,0xff,0xff
opfunc MOVLS
mov r0, #0 // b
mov r1, #0 // c
ldr  r0, [r7, r0]
ldr  r1, [r7, r1]
CALL_SETMEM_LONG // 2cyclte

opdesc MOVR,		16,4,0,0xff,0xff,0xff
opfunc MOVR
mov r0, #0 // b
mov r1, #0 // c
ldr  r1, [r7, r1]
str  r1, [r7, r0]


opdesc MOVBM,		32,0,4,0xff,0xff,0xff
opfunc MOVBM
mov r2, #0 // m
mov r3, #0 // n
ldr  r0, [r7, r3] // addr R[n]
ldrb r1, [r7, r2] // val  R[m]
sub  r0, #1
str  r0, [r7, r3] // R[n] -= 1
CALL_SETMEM_BYTE // 2cyclte

opdesc MOVWM,		32,0,4,0xff,0xff,0xff
opfunc MOVWM
mov r2, #0 // m
mov r3, #0 // n
ldr  r0, [r7, r3] // addr R[n]
ldrh r1, [r7, r2] // val  R[m]
sub  r0, #2
str  r0, [r7, r3] // R[n] -= 2
CALL_SETMEM_WORD // 2cyclte

opdesc MOVLM,		32,0,4,0xff,0xff,0xff
opfunc MOVLM
mov r2, #0 // m
mov r3, #0 // n
ldr  r0, [r7, r3] // addr R[n]
ldr  r1, [r7, r2] // val  R[m]
sub  r0, #4
str  r0, [r7, r3] // R[n] -= 4
CALL_SETMEM_LONG // 2cyclte

opdesc TAS,  (17*4),0xff,0,0xff,0xff,0xff
opfunc TAS
mov     r0, #0 // n
LDR_SR  r1   // SR
mov     r5, r0
mov     r4, r1
ldr     r0, [r7, r0]
CALL_GETMEM_BYTE
cmp     r0, #0
mvn     r1, r0, asl #25
orreq   r4, r4, #1
mvn     r1, r1, lsr #25
bicne   r4, r4, #1
ldr     r0, [r7, r5]
STR_SR  r4
uxtb    r1, r1
CALL_SETMEM_BYTE
       

opdesc DIV0U,	16,0xff,0xff,0xff,0xff,0xff
opfunc DIV0U
LDR_SR r0
//and r0,0xfffffcfe
bic     r0, r0, #768
bic     r0, r0, #1
STR_SR r0

opdesc DIV0S, 60,0,4,0xff,0xff,0xff
opfunc DIV0S
ldr  r2, [r7, #0 ] // m
ldr  r3, [r7, #0 ] // n
cmp  r3, #0
LDR_SR  r3
bicge   r3, r3, #256
orrlt   r3, r3, #256
cmp     r2, #0
bicge   r3, r3, #512
orrlt   r3, r3, #512
mov     r2, r3, asr #8
eor     r2, r2, r3, asr #9
tst     r2, #1
orrne   r3, r3, #1
biceq   r3, r3, #1
STR_SR     r3

opdesc DIV1, (64*4),0,4,0xff,0xff,0xff
opfunc DIV1
mov r0, #0 // m
mov r1, #0 // n
  LDR_SR r3
	ldr	r4, [r7, r1]
	cmp	r4, #0
	ubfx	r2, r3, #8, #1
	bicge	r3, r3, #256
	orrlt	r3, r3, #256
	cmp	r2, #1
	and	r2, r3, #1
	orr	r2, r2, r4, asl #1
	str	r2, [r7, r1]
	ldr	r0, [r7, r0]
	bne	DIV1.L41
	tst	r3, #512
	bne	DIV1.L21
	add	r0, r2, r0
	str	r0, [r7, r1]
	cmp	r0, r2
	movcs	r2, #0
	movcc	r2, #1
	tst	r3, #256
	bne	DIV1.L34
	b	DIV1.L25
DIV1.L41:
	tst	r3, #512
	beq	DIV1.L42
	add	r0, r2, r0
	str	r0, [r7, r1]
	cmp	r0, r2
	movcs	r2, #0
	movcc	r2, #1
	tst	r3, #256
	bne	DIV1.L25
DIV1.L34:
	cmp	r2, #0
	bne	DIV1.L27
DIV1.L31:
	orr	r3, r3, #256
DIV1.L16:
	mov	r2, r3, asr #8
	eor	r2, r2, r3, asr #9
	tst	r2, #1
	orreq	r3, r3, #1
	bicne	r3, r3, #1
	STR_SR r3
	b DIV1.FINISH
DIV1.L42:
	rsb	r0, r0, r2
	str	r0, [r7, r1]
	cmp	r0, r2
	movls	r2, #0
	movhi	r2, #1
	tst	r3, #256
	bne	DIV1.L34
DIV1.L25:
	cmp	r2, #0
	bne	DIV1.L31
DIV1.L27:
	bic	r3, r3, #256
	b	DIV1.L16
DIV1.L21:
	rsb	r0, r0, r2
	str	r0, [r7, r1]
	cmp	r0, r2
	movls	r2, #0
	movhi	r2, #1
	tst	r3, #256
	bne	DIV1.L25
	b	DIV1.L34
DIV1.FINISH:



//------------------------------------------------------------
//dmuls
opdesc DMULS, 20,0,4,0xff,0xff,0xff
opfunc DMULS
ldr     r3, [r7, #0]
ldr     r2, [r7, #0]
smull   r0, r1, r3, r2
STR_MACH r1
STR_MACL r0

//------------------------------------------------------------
//dmulu 32bit -> 64bit Mul
opdesc DMULU, 20,0,4,0xff,0xff,0xff
opfunc DMULU
ldr     r3, [r7, #0]
ldr     r2, [r7, #0]
umull   r0, r1, r3, r2
STR_MACH r1
STR_MACL r0

//--------------------------------------------------------------
// mull 32bit -> 32bit Multip
opdesc MULL, 16,0,4,0xff,0xff,0xff
opfunc MULL
ldr     r2, [r7, #0]
ldr     r0, [r7, #0]
mul     r1, r0, r2
STR_MACL r1

//--------------------------------------------------------------
// muls 16bit -> 32 bit Multip
opdesc MULS, 16,0,4,0xff,0xff,0xff
opfunc MULS
ldr     r2, [r7, #0]
ldr     r0, [r7, #0]
smulbb  r0, r2, r0
STR_MACL r0


//--------------------------------------------------------------
// mulu 16bit -> 32 bit Multip
opdesc MULU, 24,0,4,0xff,0xff,0xff
opfunc MULU
ldr     r2, [r7, #0]
ldr     r0, [r7, #0]
uxth    r2, r2
uxth    r0, r0
mul     r1, r0, r2
STR_MACL r1
//CALL_EACHCLOCK

//--------------------------------------------------------------
// MACL   ans = 32bit -> 64 bit MUL
//        (MACH << 32 + MACL)  + ans 
//-------------------------------------------------------------
opdesc MAC_L, ((52)*4),0,4,0xff,0xff,0xff 
opfunc MAC_L
mov r0, #0
mov r1, #0
mov r6, r0 
ldr r0, [r7, r1]
mov r4, r1
CALL_GETMEM_LONG
ldr r3, [r7, r4]
add r3, r3, #4
str r3, [r7, r4]
mov r4, #0
mov r5, r0 
ldr r0, [r7, r6]
CALL_GETMEM_LONG
ldr r2, [r7, r6]
add r2, r2, #4
str r2, [r7, r6]
LDR_MACL r1
mov r3,r5
LDR_MACH r5
LDR_SR r2       
orr     r6, r4, r1
smull   r4, r1, r0, r3
adds    r6, r6, r4
adc     ip, r5, r1

tst     r2, #2
beq     MAC_L.NO_S
        mov     r5, #0
        mov     r4, #32768
        adds    r5, r5, r6
        movt    r4, 65535
        adc     r4, r4, ip
        mvn     r2, #65536
        cmp     r4, r2
        mvn     r3, #0
        cmpeq   r5, r3
        bhi     MAC_L.NO_S
        cmp     r0, #0
        sbcs    r2, r1, #0
        mov     r2, #32768
        movw    r1, #32767
        movt    r2, 65535
        mvnge   r4, #0
        movge   r2, r1
        b       MAC_L.FINISH
MAC_L.NO_S:
  mov     r4, r6
  mov     r2, ip
MAC_L.FINISH:
  STR_MACL  r4
  STR_MACH  r2
//CALL_EACHCLOCK

//--------------------------------------------------------------
// MACW   ans = 32bit -> 64 bit MUL
//        (MACH << 32 + MACL)  + ans 
//-------------------------------------------------------------
opdesc MAC_W, (42*4),0,4,0xff,0xff,0xff
opfunc MAC_W
  mov r0, #0  // m
  mov r1, #0  // n
	mov	r5, r0
	ldr	r0, [r7, r0]
	mov	r6, r1
	CALL_GETMEM_WORD
	ldr	r3, [r7, r5]
	add	r3, r3, #2
	str	r3, [r7, r5]
	uxth	r5, r0
	ldr	r0, [r7, r6]
	CALL_GETMEM_WORD
  LDR_MACL r2
  LDR_SR lr
	ldr	r1, [r7, r6]
	mov	r3, r2, asr #31
	add	r1, r1, #2
	str	r1, [r7, r6]
	smulbb	r0, r5, r0
	adds	r2, r2, r0
	adc	r3, r3, r0, asr #31
	tst	lr, #2
	beq	MAC_W.L2
	adds	r6, r2, #-2147483648
	mvn	r5, #1
	sbc	r12, r3, #0
	mvn	r4, #0
	cmp	r12, r5
	cmpeq	r6, r4
	bhi	MAC_W.L4
	cmp	r0, #0
	ldr	r1, [r3]
	movlt	r2, #-2147483648
	mvnge	r2, #-2147483648
	orr	r1, r1, #1
	STR_MACH r1
MAC_W.L4:
	STR_MACL r2
	b MAC_W.FINISH
MAC_W.L2:
  STR_MACL r2
  STR_MACH r3
MAC_W.FINISH:


  
  
  
