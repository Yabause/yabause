/*  Copyright 2017 devMiyax(smiyaxdev@gmail.com)

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


#include <stdio.h>
#include <string.h>
#include <malloc.h> 
#include <stdint.h>
#include <core.h>

#include "sh2core.h"
#include "debug.h"
#include "yabause.h"
#include "bios.h"

#include "DynarecSh2.h"
#include "opcodes.h"
#if defined(WEBINTERFACE)
#define DEBUG_CPU
#endif
//#define DEBUG_CPU
//#define BUILD_INFO
//#define LOG printf

CompileBlocks * CompileBlocks::instance_ = NULL;
DynarecSh2 * DynarecSh2::CurrentContext = NULL;

#if defined(ARCH_IS_LINUX)
#include <unistd.h> // chaceflush

#if !defined(ANDROID)
void cacheflush(uintptr_t begin, uintptr_t end, int flag )
{ 
    const int syscall = 0xf0002;
      __asm __volatile (
     "mov   r0, %0\n"      
     "mov   r1, %1\n"
     "mov   r7, %2\n"
     "mov     r2, #0x0\n"
     "svc     0x00000000\n"
     :
     : "r" (begin), "r" (end), "r" (syscall)
     : "r0", "r1", "r7"
    );
}
#endif

#endif

i_desc opcode_list[] =
{
  { ZERO_F,	"clrt",						0xffff, 0x8,	0, sh2_CLRT},
  { ZERO_F,   "clrmac",					0xffff, 0x28,	0, sh2_CLRMAC},
  { ZERO_F,   "div0u",					0xffff, 0x19,	0, sh2_DIV0U},
  { ZERO_F,   "nop",						0xffff, 0x9,	0, sh2_NOP},
  { ZERO_F,   "rte",						0xffff, 0x2b,	0, sh2_RTE},
  { ZERO_F,   "rts",						0xffff, 0xb,	0, sh2_RTS},
  { ZERO_F,   "sett",						0xffff, 0x18,	0, sh2_SETT},
  { ZERO_F,   "sleep",					0xffff, 0x1b,	0, sh2_SLEEP},
  { N_F,      "cmp/pl r%d",				0xf0ff, 0x4015,	0, sh2_CMP_PL},
  { N_F,      "cmp/pz r%d",				0xf0ff, 0x4011,	0, sh2_CMP_PZ},
  { N_F,      "dt r%d",					0xf0ff, 0x4010,	0, sh2_DT},
  { N_F,      "movt r%d",					0xf0ff, 0x0029,	0, sh2_MOVT},
  { N_F,      "rotl r%d",					0xf0ff, 0x4004,	0, sh2_ROTL},
  { N_F,      "rotr r%d",					0xf0ff, 0x4005,	0, sh2_ROTR},
  { N_F,      "rotcl r%d",				0xf0ff, 0x4024,	0, sh2_ROTCL},
  { N_F,      "rotcr r%d",				0xf0ff, 0x4025,	0, sh2_ROTCR},
  { N_F,      "shal r%d",					0xf0ff, 0x4020,	0, sh2_SHL},
  { N_F,      "shar r%d",					0xf0ff, 0x4021,	0, sh2_SHAR},
  { N_F,      "shll r%d",					0xf0ff, 0x4000,	0, sh2_SHL},
  { N_F,      "shlr r%d",					0xf0ff, 0x4001,	0, sh2_SHLR},
  { N_F,      "shll2 r%d",				0xf0ff, 0x4008,	0, sh2_SHLL2},
  { N_F,      "shlr2 r%d",				0xf0ff, 0x4009,	0, sh2_SHLR2},
  { N_F,      "shll8 r%d",				0xf0ff, 0x4018,	0, sh2_SHLL8},
  { N_F,      "shlr8 r%d",				0xf0ff, 0x4019,	0, sh2_SHLR8},
  { N_F,      "shll16 r%d",				0xf0ff, 0x4028,	0, sh2_SHLL16},
  { N_F,      "shlr16 r%d",				0xf0ff, 0x4029,	0, sh2_SHLL16},
  { N_F,      "stc sr, r%d",				0xf0ff, 0x0002,	0, sh2_STC_SR},
  { N_F,      "stc gbr, r%d",				0xf0ff, 0x0012,	0, sh2_STC_GBR},
  { N_F,      "stc vbr, r%d",				0xf0ff, 0x0022,	0, sh2_STC_VBR},
  { N_F,      "sts mach, r%d",			0xf0ff, 0x000a,	0, sh2_STS_MACH},
  { N_F,      "sts macl, r%d",			0xf0ff, 0x001a,	0, sh2_STS_MACL},
  { N_F,      "sts pr, r%d",				0xf0ff, 0x002a,	0, sh2_STS_PR},
  { N_F,      "tas.b @r%d",				0xf0ff, 0x401b,	0, sh2_TAS_B},
  { N_F,      "stc.l sr, @-r%d",			0xf0ff, 0x4003,	0, sh2_STC_SR_DEC},
  { N_F,      "stc.l gbr, @-r%d",			0xf0ff, 0x4013,	0, sh2_STC_GBR_DEC},
  { N_F,      "stc.l vbr, @-r%d",			0xf0ff, 0x4023,	0, sh2_STC_VBR_DEC},
  { N_F,      "sts.l mach, @-r%d",		0xf0ff, 0x4002,	0, sh2_STS_MACH_DEC},
  { N_F,      "sts.l macl, @-r%d",		0xf0ff, 0x4012,	0, sh2_STS_MACL_DEC},
  { N_F,      "sts.l pr, @-r%d",			0xf0ff, 0x4022,	0, sh2_STS_PR_DEC},
  { M_F,      "ldc r%d, sr",				0xf0ff, 0x400e,	0, sh2_LDC_SR},
  { M_F,      "ldc r%d, gbr",				0xf0ff, 0x401e,	0, sh2_LDC_GBR},
  { M_F,      "ldc r%d, vbr",				0xf0ff, 0x402e,	0, sh2_LDC_VBR},
  { M_F,      "lds r%d, mach",			0xf0ff, 0x400a,	0, sh2_LDS_MACH},
  { M_F,      "lds r%d, macl",			0xf0ff, 0x401a,	0, sh2_LDS_MACL},
  { M_F,      "lds r%d, pr",				0xf0ff, 0x402a,	0, sh2_LDS_PR},
  { M_F,      "jmp @r%d",					0xf0ff, 0x402b,	0, sh2_JMP},
  { M_F,      "jsr @r%d",					0xf0ff, 0x400b,	0, sh2_JSR}, 
  { M_F,      "ldc.l @r%d+, sr",			0xf0ff, 0x4007,	0, sh2_LDC_SR_INC},
  { M_F,      "ldc.l @r%d+, gbr",			0xf0ff, 0x4017,	0, sh2_LDC_GBR_INC},
  { M_F,      "ldc.l @r%d+, vbr",			0xf0ff, 0x4027,	0, sh2_LDC_VBR_INC},
  { M_F,      "lds.l @r%d+, mach",		0xf0ff, 0x4006,	0, sh2_LDS_MACH_INC},
  { M_F,      "lds.l @r%d+, macl",		0xf0ff, 0x4016,	0, sh2_LDS_MACL_INC},
  { M_F,      "lds.l @r%d+, pr",			0xf0ff, 0x4026,	0, sh2_LDS_PR_INC},
  { M_F,      "braf r%d",					0xf0ff, 0x0023,	0, sh2_BRAF},
  { M_F,      "bsrf r%d",					0xf0ff, 0x0003,	0, sh2_BSRF},
  { NM_F,     "add r%d, r%d",				0xf00f, 0x300c,	0, sh2_ADD},
  { NM_F,     "addc r%d, r%d",			0xf00f, 0x300e,	0, sh2_ADDC},
  { NM_F,     "addv r%d, r%d",			0xf00f, 0x300f,	0, sh2_ADDV},
  { NM_F,     "and r%d, r%d",				0xf00f, 0x2009,	0, sh2_AND},
  { NM_F,     "cmp/eq r%d, r%d",			0xf00f, 0x3000,	0, sh2_CMP_EQ},
  { NM_F,     "cmp/hs r%d, r%d",			0xf00f, 0x3002,	0, sh2_CMP_HS},
  { NM_F,     "cmp/ge r%d, r%d",			0xf00f, 0x3003,	0, sh2_CMP_GE},
  { NM_F,     "cmp/hi r%d, r%d",			0xf00f, 0x3006,	0, sh2_CMP_HI},
  { NM_F,     "cmp/gt r%d, r%d",			0xf00f, 0x3007,	0, sh2_CMP_GT},
  { NM_F,     "cmp/str r%d, r%d",			0xf00f, 0x200c,	0, sh2_CMP_STR},
  { NM_F,     "div1 r%d, r%d",			0xf00f, 0x3004,	0, sh2_DIV1},
  { NM_F,     "div0s r%d, r%d",			0xf00f, 0x2007,	0, sh2_DIV0S},
  { NM_F,     "dmuls.l r%d, r%d",			0xf00f, 0x300d,	0, sh2_DMULS_L},
  { NM_F,     "dmulu.l r%d, r%d",			0xf00f, 0x3005,	0, sh2_DMULU_L},
  { NM_F,     "exts.b r%d, r%d",			0xf00f, 0x600e,	0, sh2_EXTS_B},
  { NM_F,     "exts.w r%d, r%d",			0xf00f, 0x600f,	0, sh2_EXTS_W},
  { NM_F,     "extu.b r%d, r%d",			0xf00f, 0x600c,	0, sh2_EXTU_B},
  { NM_F,     "extu.w r%d, r%d",			0xf00f, 0x600d,	0, sh2_EXTU_W},
  { NM_F,     "mov r%d, r%d",				0xf00f, 0x6003,	0, sh2_MOVR},
  { NM_F,     "mul.l r%d, r%d",			0xf00f, 0x0007,	0, sh2_MUL_L},
  { NM_F,     "muls.w r%d, r%d",			0xf00f, 0x200f,	0, sh2_MULS},
  { NM_F,     "mulu.w r%d, r%d",			0xf00f, 0x200e,	0, sh2_MULU},
  { NM_F,     "neg r%d, r%d",				0xf00f, 0x600b,	0, sh2_NEG},
  { NM_F,     "negc r%d, r%d",			0xf00f, 0x600a,	0, sh2_NEGC},
  { NM_F,     "not r%d, r%d",				0xf00f, 0x6007,	0, sh2_NOT},
  { NM_F,     "or r%d, r%d",				0xf00f, 0x200b,	0, sh2_OR},
  { NM_F,     "sub r%d, r%d",				0xf00f, 0x3008,	0, sh2_SUB},
  { NM_F,     "subc r%d, r%d",			0xf00f, 0x300a,	0, sh2_SUBC},
  { NM_F,     "subv r%d, r%d",			0xf00f, 0x300b,	0, sh2_SUBV},
  { NM_F,     "swap.b r%d, r%d",			0xf00f, 0x6008,	0, sh2_SWAP_B},
  { NM_F,     "swap.w r%d, r%d",			0xf00f, 0x6009,	0, sh2_SWAP_W},
  { NM_F,     "tst r%d, r%d",				0xf00f, 0x2008,	0, sh2_TST},
  { NM_F,     "xor r%d, r%d",				0xf00f, 0x200a,	0, sh2_XOR},
  { NM_F,     "xtrct r%d, r%d",			0xf00f, 0x200d,	0, sh2_XTRCT},
  { NM_F,     "mov.b r%d, @r%d",			0xf00f, 0x2000,	0, sh2_MOVB},
  { NM_F,     "mov.w r%d, @r%d",			0xf00f, 0x2001,	0, sh2_MOVW},
  { NM_F,     "mov.l r%d, @r%d",			0xf00f, 0x2002,	0, sh2_MOVL},
  { NM_F,     "mov.b @r%d, r%d",			0xf00f, 0x6000,	0, sh2_MOVB_MEM},
  { NM_F,     "mov.w @r%d, r%d",			0xf00f, 0x6001,	0, sh2_MOVW_MEM},
  { NM_F,     "mov.l @r%d, r%d",			0xf00f, 0x6002,	0, sh2_MOVL_MEM},
  { NM_F,     "mac.l @r%d+, @r%d+",		0xf00f, 0x000f,	0, sh2_MAC_L},
  { NM_F,     "mac.w @r%d+, @r%d+",		0xf00f, 0x400f,	0, sh2_MAC_W},
  { NM_F,     "mov.b @r%d+, r%d",			0xf00f, 0x6004,	0, sh2_MOVB_INC},
  { NM_F,     "mov.w @r%d+, r%d",			0xf00f, 0x6005,	0, sh2_MOVW_INC},
  { NM_F,     "mov.l @r%d+, r%d",			0xf00f, 0x6006,	0, sh2_MOVL_INC},
  { NM_F,     "mov.b r%d, @-r%d",			0xf00f, 0x2004,	0, sh2_MOVB_DEC},
  { NM_F,     "mov.w r%d, @-r%d",			0xf00f, 0x2005,	0, sh2_MOVW_DEC},
  { NM_F,     "mov.l r%d, @-r%d",			0xf00f, 0x2006,	0, sh2_MOVL_DEC},
  { NM_F,     "mov.b r%d, @(r0, r%d)",	0xf00f, 0x0004,	0, sh2_MOVB_R0},
  { NM_F,     "mov.w r%d, @(r0, r%d)",	0xf00f, 0x0005,	0, sh2_MOVW_R0},
  { NM_F,     "mov.l r%d, @(r0, r%d)",	0xf00f, 0x0006,	0, sh2_MOVL_R0},
  { NM_F,     "mov.b @(r0, r%d), r%d",	0xf00f, 0x000c,	0, sh2_MOVB_R0_MEM},
  { NM_F,     "mov.w @(r0, r%d), r%d",	0xf00f, 0x000d,	0, sh2_MOVW_R0_MEM},
  { NM_F,     "mov.l @(r0, r%d), r%d",	0xf00f, 0x000e,	0, sh2_MOVL_R0_MEM},
  { MD_F,     "mov.b @(0x%03X, r%d), r0",	0xff00, 0x8400,	0, sh2_MOVB_DISP_R0},
  { MD_F,     "mov.w @(0x%03X, r%d), r0", 0xff00, 0x8500,	0, sh2_MOVW_DISP_R0},
  { ND4_F,    "mov.b r0, @(0x%03X, r%d)", 0xff00, 0x8000,	0, sh2_MOVB_R0_DISP},
  { ND4_F,    "mov.w r0, @(0x%03X, r%d)", 0xff00, 0x8100,	0, sh2_MOVW_R0_DISP},
  { NMD_F,    "mov.l r%d, @(0x%03X, r%d)",0xf000, 0x1000,	0, sh2_MOVL_DISP_MEM},
  { NMD_F,    "mov.l @(0x%03X, r%d), r%d",0xf000, 0x5000,	0, MOVLL4},
  { D_F,      "mov.b r0, @(0x%03X, gbr)",	0xff00, 0xc000,	1, sh2_MOVB_R0_GBR},
  { D_F,      "mov.w r0, @(0x%03X, gbr)",	0xff00, 0xc100,	2, sh2_MOVW_R0_GBR},
  { D_F,      "mov.l r0, @(0x%03X, gbr)",	0xff00, 0xc200,	4, sh2_MOVL_R0_GBR},
  { D_F,      "mov.b @(0x%03X, gbr), r0",	0xff00, 0xc400,	1, sh2_MOVB_GBR_R0},
  { D_F,      "mov.w @(0x%03X, gbr), r0",	0xff00, 0xc500,	2, sh2_MOVW_GBR_R0},
  { D_F,      "mov.l @(0x%03X, gbr), r0",	0xff00, 0xc600,	4, sh2_MOVL_GBR_R0},
  { D_F,      "mova @(0x%03X, pc), r0",	0xff00, 0xc700,	4, sh2_MOVA},
  { D_F,      "bf 0x%08X",				0xff00, 0x8b00,	4, sh2_BF},
  { D_F,      "bf/s 0x%08X",				0xff00, 0x8f00,	5, sh2_BF_S},
  { D_F,      "bt 0x%08X",				0xff00, 0x8900,	5, sh2_BT},
  { D_F,      "bt/s 0x%08X",				0xff00, 0x8d00,	5, sh2_BT_S},
  { D12_F,    "bra 0x%08X",				0xf000, 0xa000,	5, sh2_BRA},
  { D12_F,    "bsr 0x%08X",				0xf000, 0xb000,	0, sh2_BSR},
  { ND8_F,    "mov.w @(0x%03X, pc), r%d", 0xf000, 0x9000,	0, sh2_MOV_DISP_W},
  { ND8_F,    "mov.l @(0x%03X, pc), r%d",	0xf000, 0xd000,	2, sh2_MOV_DISP_L},
  { I_F,      "and.b #0x%02X, @(r0, gbr)",0xff00, 0xcd00,	4, sh2_AND_B},
  { I_F,      "or.b #0x%02X, @(r0, gbr)", 0xff00, 0xcf00,	0, sh2_OR_B},
  { I_F,      "tst.b #0x%02X, @(r0, gbr)",0xff00, 0xcc00,	0, sh2_TST_B},
  { I_F,      "xor.b #0x%02X, @(r0, gbr)",0xff00, 0xce00,	0, sh2_XOR_B},
  { I_F,      "and #0x%02X, r0",			0xff00, 0xc900,	0, sh2_ANDI},
  { I_F,      "cmp/eq #0x%02X, r0",		0xff00, 0x8800,	0, sh2_CMP_EQ_IMM},
  { I_F,      "or #0x%02X, r0",			0xff00, 0xcb00,	0, sh2_ORI},
  { I_F,      "tst #0x%02X, r0",			0xff00, 0xc800,	0, sh2_TST_R0},
  { I_F,      "xor #0x%02X, r0",			0xff00, 0xca00,	0, sh2_XORI},
  { I_F,      "trapa #0x%X",				0xff00, 0xc300,	0, sh2_TRAPA},
  { NI_F,     "add #0x%02X, r%d",			0xf000, 0x7000,	0, sh2_ADDI},
  { NI_F,     "mov #0x%02X, r%d",			0xf000, 0xe000,	0, sh2_MOVI},
  { 0,        NULL,						0,      0,		0, 0}
};


void DumpInstX( int i, u32 pc, u16 op  )
{
   LOG( "%08X : ", pc );

  if (opcode_list[i].format == ZERO_F)
    LOG("%s", opcode_list[i].mnem);
  else if (opcode_list[i].format == N_F)
    LOG(opcode_list[i].mnem, (op >> 8) & 0xf);
  else if (opcode_list[i].format == M_F)
    LOG(opcode_list[i].mnem, (op >> 8) & 0xf);
  else if (opcode_list[i].format == NM_F)
    LOG(opcode_list[i].mnem, (op >> 4) & 0xf, (op >> 8) & 0xf);
  else if (opcode_list[i].format == MD_F)
  {
    if (op & 0x100)
      LOG(opcode_list[i].mnem, (op & 0xf) * 2, (op >> 4) & 0xf);
    else
      LOG(opcode_list[i].mnem, op & 0xf, (op >> 4) & 0xf);
  }
  else if (opcode_list[i].format == ND4_F)
  {
    if (op & 0x100)
      LOG(opcode_list[i].mnem, (op & 0xf) * 2, (op >> 4) & 0xf);
    else
      LOG(opcode_list[i].mnem, (op & 0xf), (op >> 4) & 0xf);
  }
  else if (opcode_list[i].format == NMD_F)
  {
    if ((op & 0xf000) == 0x1000)
      LOG(opcode_list[i].mnem, (op >> 4) & 0xf, (op & 0xf) * 4, (op >> 8) & 0xf);
    else
      LOG(opcode_list[i].mnem, (op & 0xf) * 4, (op >> 4) & 0xf, (op >> 8) & 0xf);
  }
  else if (opcode_list[i].format == D_F)
  {
    if (opcode_list[i].dat <= 4)
    {
      if ((op & 0xff00) == 0xc700)
      {
        LOG(opcode_list[i].mnem, (op & 0xff) * opcode_list[i].dat + 4);
//				LOG("  ; 0x%08X", (op & 0xff) * opcode_list[i].dat + 4 + PC);
      }
      else
        LOG(opcode_list[i].mnem, (op & 0xff) * opcode_list[i].dat);
    }
    else
    {
      if (op & 0x80)  /* sign extend */
        LOG(opcode_list[i].mnem, (((op & 0xff) + 0xffffff00) * 2) + pc + 4);
      else
        LOG(opcode_list[i].mnem, ((op & 0xff) * 2) + pc + 4);
    }        
  }
  else if (opcode_list[i].format == D12_F)
  {
    if (op & 0x800)         /* sign extend */
      LOG(opcode_list[i].mnem, ((op & 0xfff) + 0xfffff000) * 2 + pc + 4);
    else
      LOG(opcode_list[i].mnem, (op & 0xfff) * 2 + pc + 4);
  }
  else if (opcode_list[i].format == ND8_F)
  {
    if ((op & 0xf000) == 0x9000)    /* .W */
    {
      LOG(opcode_list[i].mnem, (op & 0xff) * opcode_list[i].dat + 4, (op >> 8) & 0xf);
      //LOG("  ; 0x%08X", (op & 0xff) * opcode_list[i].dat + 4 + pc);
    }
    else  /* .L */
    {
      LOG(opcode_list[i].mnem, (op & 0xff) * opcode_list[i].dat + 4, (op >> 8) & 0xf);
      //LOG("  ; 0x%08X", (op & 0xff) * opcode_list[i].dat + 4 + ((pc) & 0xfffffffc));
    }
  }
  else if (opcode_list[i].format == I_F)
    LOG(opcode_list[i].mnem, op & 0xff);
  else if (opcode_list[i].format == NI_F)
    LOG(opcode_list[i].mnem, op & 0xff, (op >> 8) & 0xf);
  else
  {
    LOG("unrecognized\n");
    return;
  }

  LOG( "\n");
  return;
}



#define opdesc(op, y, c, d)	x86op_desc(x86_##op, &op##_size, &op##_src, &op##_dest, &op##_off1, &op##_imm, &op##_off3, y, c, d)

#define opNULL			x86op_desc(0,0,0,0,0,0,0,0,0,0)

#if _WINDOWS
#define PROLOGSIZE		     27    
#define EPILOGSIZE		      3
#define SEPERATORSIZE	     10
#define SEPERATORSIZE_NORMAL 7
#define SEPERATORSIZE_DEBUG  24
#define SEPERATORSIZE_DELAY  7
#define SEPERATORSIZE_DELAY_SLOT  27
#define SEPERATORSIZE_DELAY_AFTER  10 
#define SEPERATORSIZE_DELAYD 34
#define DELAYJUMPSIZE	     17
#define DALAY_CLOCK_OFFSET 6
#define NORMAL_CLOCK_OFFSET 6
#define NORMAL_CLOCK_OFFSET_DEBUG 3
#else
#define PROLOGSIZE		     16    
#define SEPERATORSIZE_NORMAL 8
#define NORMAL_CLOCK_OFFSET 4
#define NORMAL_CLOCK_OFFSET_DEBUG 4
#define SEPERATORSIZE_DEBUG  36
#define SEPERATORSIZE_DELAY_SLOT  36
#define SEPERATORSIZE_DELAY_AFTER  20
#define DALAY_CLOCK_OFFSET 8
#define SEPERATORSIZE_DELAYD 60
#define EPILOGSIZE		      12
#define DELAYJUMPSIZE	     32
#endif


#define MININSTRSIZE    3
#define MAXINSTRSIZE	416
#define MAXJUMPSIZE		46

#define SAFEPAGESIZE	MAXBLOCKSIZE - MAXINSTRSIZE - SEPERATORSIZE_DELAY_SLOT - SEPERATORSIZE_DELAY_AFTER - SEPERATORSIZE_NORMAL - EPILOGSIZE
#define MAXINSTRCNT		MAXBLOCKSIZE/(MININSTRSIZE+SEPERATORSIZE_NORMAL)
int instrSize[NUMOFBLOCKS][MAXINSTRCNT];

#define opinit(x)	extern const unsigned short x##_size; \
                    extern const unsigned char x##_src, x##_dest, x##_off1, x##_imm, x##_off3; \
            void x86_##x();


extern "C" {

opinit(CLRT);
opinit(CLRMAC);
opinit(NOP);
opinit(DIV0U);
opinit(SETT);
opinit(SLEEP);
opinit(STS_MACH);
opinit(STS_MACL);
opinit(STS_MACH_DEC);
opinit(STS_MACL_DEC);
opinit(STS_PR);
opinit(STC_SR);
opinit(STC_VBR);
opinit(STC_GBR);
opinit(LDS_PR);
opinit(LDS_MACH);
opinit(LDS_MACL);
opinit(LDS_PR_INC);
opinit(LDS_MACH_INC);
opinit(LDS_MACL_INC);
opinit(LDC_SR);
opinit(LDC_VBR);
opinit(LDCGBR);
opinit(LDC_SR_INC);
opinit(LDC_VBR_INC);
opinit(LDC_GBR_INC);
opinit(STC_SR_MEM);
opinit(STC_VBR_MEM);
opinit(STC_GBR_MEM);
opinit(STSMPR);
opinit(CMP_EQ);
opinit(CMP_HI);
opinit(CMP_GE);
opinit(CMP_HS);
opinit(CMP_GT);
opinit(DT);
opinit(CMP_PL);
opinit(CMP_PZ);
opinit(ROTL);
opinit(ROTR);
opinit(ROTCL);
opinit(ROTCR);
opinit(SHL);
opinit(SHLR);
opinit(SHAR);
opinit(SHLL2);
opinit(SHLR2);
opinit(SHLL8);
opinit(SHLR8);
opinit(SHLL16);
opinit(SHLR16);
opinit(AND);
opinit(OR);
opinit(XOR);
opinit(NOT);
opinit(XTRCT);
opinit(ADD);
opinit(ADDC);
opinit(SUB);
opinit(SUBC);
opinit(NEG);
opinit(NEGC);
opinit(TST);
opinit(TSTI);
opinit(AND_B);
opinit(OR_B);
opinit(TST_B);
opinit(XOR_B);
opinit(ADDI);
opinit(ANDI);
opinit(ORI);
opinit(XORI);
opinit(MOVI);
opinit(CMP_EQ_IMM);
opinit(SWAP_B);
opinit(SWAP_W);
opinit(EXTUB);
opinit(EXTU_W);
opinit(EXTS_B);
opinit(EXTS_W);
opinit(BT)
opinit(BF);
opinit(BF_S);
opinit(JMP);
opinit(JSR);
opinit(BRA);
opinit(BSR);
opinit(BSRF);
opinit(BRAF);
opinit(RTE);
opinit(RTS);
opinit(MOVA);
opinit(MOVT);
opinit(MOVBL);
opinit(MOVWL);
opinit(MOVL_MEM_REG);
opinit(MOVBS);
opinit(MOVWS);
opinit(MOVLS);
opinit(MOVWI);
opinit(MOVLI);
opinit(MOVBS4);
opinit(MOVWS4);
opinit(MOVLS4);
opinit(MOVR);
opinit(MOVBP);
opinit(MOVWP);
opinit(MOVLP);
opinit(MOVBL0);
opinit(MOVWL0);
opinit(MOVLL0);
opinit(MOVBS0);
opinit(MOVWS0);
opinit(MOVLS0);
opinit(MOVBSG);
opinit(MOVLSG);
opinit(MOVWL4);  // 0x8500
opinit(MOVLL4);
opinit(MOVLM);
opinit(TAS);
opinit(ADDV);
opinit(CMPSTR);
opinit(DIV0S);
opinit(DMULS);
opinit(DMULU);
opinit(MULL);
opinit(MULS);
opinit(MULU);
opinit(SUBV);
opinit(MAC_L);
opinit(MOVBM);
opinit(MOVWM);
opinit(MOVBL4);
opinit(MOVWSG);
opinit(MOVBLG);
opinit(MOVWLG);
opinit(MOVLLG);
opinit(TRAPA);
opinit(DIV1);
opinit(MAC_W);


void prologue(void);
void epilogue(void);
void seperator(void);
void seperator_normal(void);
void seperator_delay(void);
void seperator_delay_slot(void);
void seperator_delay_after(void);

void seperator_d_normal(void);
void seperator_d_delay(void);

void PageJump(void); // jumps to a different page
void PageFlip(void); // "flips" the page

extern x86op_desc asm_list[];

}

x86op_desc asm_list[] =
{
  opdesc(CLRT,0,1,0),       
  opdesc(CLRMAC,0,1,0),
  opdesc(DIV0U,0,1,0),
  opdesc(NOP,0,1,0),
  opdesc(RTE,4,4,0),
  opdesc(RTS,4,2,0),
  opdesc(SETT,0,1,0),
  opdesc(SLEEP,0,3,0),
  opdesc(CMP_PL,0,1,0),
  opdesc(CMP_PZ,0,1,0),
  opdesc(DT,0,1,1),
  opdesc(MOVT,0,1,0),
  opdesc(ROTL,0,1,0),
  opdesc(ROTR,0,1,0),
  opdesc(ROTCL,0,1,0),
  opdesc(ROTCR,0,1,0),
  opdesc(SHL,0,1,0),
  opdesc(SHAR,0,1,0),
  opdesc(SHL,0,1,0),
  opdesc(SHLR,0,1,0),
  opdesc(SHLL2,0,1,0),
  opdesc(SHLR2,0,1,0),
  opdesc(SHLL8,0,1,0),
  opdesc(SHLR8,0,1,0),
  opdesc(SHLL16,0,1,0),
  opdesc(SHLR16,0,1,0),
  opdesc(STC_SR,0xFF,1,1),
  opdesc(STC_GBR,0xFF,1,1),
  opdesc(STC_VBR,0xFF,1,1),
  opdesc(STS_MACH,0xFF,1,0),
  opdesc(STS_MACL,0xFF,1,0),
  opdesc(STS_PR,0xFF,1,0),
  opdesc(TAS,0,4,1),         // 0x401b
  opdesc(STC_SR_MEM,0xFF,2,1),
  opdesc(STC_GBR_MEM,0xFF,2,1),
  opdesc(STC_VBR_MEM,0xFF,2,1),
  opdesc(STS_MACH_DEC,0xFF,2,1),
  opdesc(STS_MACL_DEC,0xFF,2,1),
  opdesc(STSMPR,0xFF,1,1),     // 0x4022
  opdesc(LDC_SR,0xFF,1,0),
  opdesc(LDCGBR,0xFF,1,0),
  opdesc(LDC_VBR,0xFF,1,0),
  opdesc(LDS_MACH,0xFF,1,0),
  opdesc(LDS_MACL,0xFF,1,0),
  opdesc(LDS_PR,0xFF,1,0),
  opdesc(JMP, 3,2,0),
  opdesc(JSR, 4,2,0),
  opdesc(LDC_SR_INC,0xFF,3,0),  
  opdesc(LDC_GBR_INC,0xFF,3,0),
  opdesc(LDC_VBR_INC,0xFF,3,0),
  opdesc(LDS_MACH_INC,0xFF,1,0),
  opdesc(LDS_MACL_INC,0xFF,1,0),
  opdesc(LDS_PR_INC,0xFF,1,0),
  opdesc(BRAF,4,2,0),
  opdesc(BSRF,4,2,0),
  opdesc(ADD,0,1,1),
  opdesc(ADDC,0,1,1),
  opdesc(ADDV,0,1,0),  // 0x300F
  opdesc(AND,0,1,0),
  opdesc(CMP_EQ,0,1,0),
  opdesc(CMP_HS,0,1,0),
  opdesc(CMP_GE,0,1,0),
  opdesc(CMP_HI,0,1,0),
  opdesc(CMP_GT,0,1,0),
  opdesc(CMPSTR,0,1,0), // 0x200C
  opdesc(DIV1,0,1,0),   // 0x3004
  opdesc(DIV0S,0,1,0),  // 0x2007
  opdesc(DMULS,0,4,0),  // 0x300D
  opdesc(DMULU,0,4,0),  // 0x3005
  opdesc(EXTS_B,0,1,0),
  opdesc(EXTS_W,0,1,0),
  opdesc(EXTUB,0,1,0),
  opdesc(EXTU_W,0,1,0),
  opdesc(MOVR,0,1,0),  // 0x6003
  opdesc(MULL,0,4,0),  // 0x0007
  opdesc(MULS,0,3,0),  // 0x200f
  opdesc(MULU,0,3,0),  // 0x200e
  opdesc(NEG,0,1,0),
  opdesc(NEGC,0,1,0),
  opdesc(NOT,0,1,0),
  opdesc(OR,0,1,0),
  opdesc(SUB,0,1,0),
  opdesc(SUBC,0,1,0),
  opdesc(SUBV,0,1,0),  // 0x300B
  opdesc(SWAP_B,0,1,0),
  opdesc(SWAP_W,0,1,0),
  opdesc(TST,0,1,0),
  opdesc(XOR,0,1,0),
  opdesc(XTRCT,0,1,0),
  opdesc(MOVBS,0,1,1),
  opdesc(MOVWS,0,1,1),
  opdesc(MOVLS,0,1,1),
  opdesc(MOVBL,0,1,0), // 6000
  opdesc(MOVWL,0,1,0), // 6001
  opdesc(MOVL_MEM_REG,0,1,0),
  opdesc(MAC_L,0,3,0),  // 0x000f
  opdesc(MAC_W,0,3,0),  // 0x400f
  opdesc(MOVBP,0,1,0),  // 6004
  opdesc(MOVWP,0,1,0),  // 6005
  opdesc(MOVLP,0,1,1),  // 6006
  opdesc(MOVBM,0,1,1),  // 0x2004,
  opdesc(MOVWM,0,1, 0),  // 0x2005,
  opdesc(MOVLM,0,1,1),  // 0x2006
  opdesc(MOVBS0,0,1,1), // 0x0004
  opdesc(MOVWS0,0,1,1), // 0x0005
  opdesc(MOVLS0,0,1,1), // 0x0006
  opdesc(MOVBL0,0,1, 0), // 0x000C
  opdesc(MOVWL0,0,1, 0), // 0x000D 
  opdesc(MOVLL0,0,1, 0), // 0x000E
  opdesc(MOVBL4,0,1,1), // 0x8400
  opdesc(MOVWL4,0,1, 0), // 0x8500
  opdesc(MOVBS4,0,1, 0), // 0x8000
  opdesc(MOVWS4,0,1,1), // 0x8100
  opdesc(MOVLS4,0,1,1), // 0x1000
  opdesc(MOVLL4,0,1, 0), // 0x5000 ,1
  opdesc(MOVBSG,0,1,1), // 0xC000
  opdesc(MOVWSG,0,1,1), // 0xc100
  opdesc(MOVLSG,0,1,1), // 0xC200
  opdesc(MOVBLG,0,1, 0), // 0xC400
  opdesc(MOVWLG,0,1, 0), // 0xC500
  opdesc(MOVLLG,0,1, 0), // 0xC600
  opdesc(MOVA,0,1, 0),
  opdesc(BF,1,3, 0),
  opdesc(BF_S,3,2, 0),
  opdesc(BT,1,3, 0),
  opdesc(BT,3,2, 0),
  opdesc(BRA,2,2, 0),
  opdesc(BSR,2,2, 0),
  opdesc(MOVWI,0,1, 0),
  opdesc(MOVLI,0, 1, 0),
  opdesc(AND_B,0,3,1),
  opdesc(OR_B,0,3,1),
  opdesc(TST_B,0, 1, 0),
  opdesc(XOR_B,0,3,1),
  opdesc(ANDI,0,1, 0),
  opdesc(CMP_EQ_IMM,0,1, 0),
  opdesc(ORI,0,1, 0),
  opdesc(TSTI,0,1, 0),  // C800
  opdesc(XORI,0,1, 0),
  opdesc(TRAPA,5,8,1), // 0xc300
  opdesc(ADDI,0,1, 0),
  opdesc(MOVI,0,1, 0),
  opNULL
}; 

Block *CompileBlocks::Init(Block *dynaCode)
{
  dynaCode = (Block*)ALLOCATE(sizeof(Block)*NUMOFBLOCKS);
  memset((void*)dynaCode, 0, sizeof(Block)*NUMOFBLOCKS);

  memset(LookupTable, 0, sizeof(LookupTable));
  memset(LookupTableRom, 0, sizeof(LookupTableRom));
  memset(LookupTableLow, 0, sizeof(LookupTableLow));
  memset(LookupTableC, 0, sizeof(LookupTableC));

  blockCount = 0;
  LastMakeBlock = 0;

  g_CompleBlock = dynaCode;
  return dynaCode;
}

int CompileBlocks::opcodeIndex(u16 op)
{
  register int i = 0;
  while (((op & opcode_list[i].mask) != opcode_list[i].bits) && opcode_list[i].mnem != 0) i++;
  return i;
}

void CompileBlocks::FindOpCode(u16 opcode, u8 * instindex)
{
  *instindex = opcodeIndex(opcode);
  return;
}

void CompileBlocks::BuildInstructionList()
{
  u32 optest;
  memset(dsh2_instructions, 0, sizeof(u8)*MAX_INSTSIZE);
  for (optest = 0; optest <= 0xFFFF; optest++) {
    FindOpCode(optest, &dsh2_instructions[optest]);
  }
}

Block * CompileBlocks::CompileBlock(u32 pc, addrs * ParentT = NULL)
{
  compile_count_++;

  LastMakeBlock++;
  if (LastMakeBlock >= NUMOFBLOCKS) {
    LastMakeBlock = 0;
  }

  blockCount = LastMakeBlock;
  
  if (g_CompleBlock[blockCount].b_addr != 0x00) {
    switch (g_CompleBlock[blockCount].b_addr & 0x0FF00000) {
    case 0x00000000:
      if (yabsys.emulatebios) {
        return NULL;
      }
      else {
        LookupTableRom[ (g_CompleBlock[blockCount].b_addr&0x000FFFFF)>>1  ] = NULL;
      }
      break;
    case 0x00200000:
      LookupTableLow[ (g_CompleBlock[blockCount].b_addr&0x000FFFFF)>>1 ] = NULL;
      break;
    case 0x06000000:
      /*case 0x06100000:*/
      LookupTable[ (g_CompleBlock[blockCount].b_addr & 0x000FFFFF)>>1 ] = NULL;
      break;
    default:
      if ((g_CompleBlock[blockCount].b_addr & 0xFF000000) == 0xC0000000) {
        LookupTableC[ (g_CompleBlock[blockCount].b_addr & 0x000FFFFF)>>1   ] = NULL;
      }
      break;
    }
  }

  g_CompleBlock[blockCount].b_addr = pc;

  if (EmmitCode(&g_CompleBlock[blockCount], ParentT) != 0) {
    return NULL;
  }

  return &g_CompleBlock[blockCount];
}

void CompileBlocks::ShowStatics() {
  LOG("Compile %d/%d\n", compile_count_, exec_count_);
  compile_count_ = 0;
  exec_count_ = 0;
}

void CompileBlocks::opcodePass(x86op_desc *op, u16 opcode, u8 *ptr)
{
  // multiply source and dest regions by 4 (size of register) 

  if (*(op->src) != 0xFF) // C
    *(ptr + *(op->src)) = (u8)(((opcode >> 4) & 0xf) << 2);

  if (*(op->dest) != 0xFF) // B
    *(ptr + *(op->dest)) = (u8)(((opcode >> 8) & 0xf) << 2);

  if (*(op->off1) != 0xFF)
    *(ptr + *(op->off1)) = (u8)(opcode & 0xf);

  if (*(op->imm) != 0xFF)
    *(ptr + *(op->imm)) = (u8)(opcode & 0xff);

#if _WINDOWS
  if (*(op->off3) != 0xFF)
    *(u16*)(ptr + *(op->off3)) = (u16)(opcode & 0xfff);
#else  
  if (*(op->off3) != 0xFF) {
    *(ptr + *(op->off3)) = (u8)((opcode >> 8) & 0x0f);
    *(ptr + *(op->off3) + 4) = (u8)(opcode & 0xff);
  }
#endif  
}


int CompileBlocks::EmmitCode(Block *page, addrs * ParentT )
{
  int i, j, jmp = 0, count = 0;
  u16 op, temp;
  u32 addr = page->b_addr;
  u32 start_addr = page->b_addr;
  u8 *ptr, *startptr;
  u32 instruction_counter = 0;
  u32 write_memory_counter = 0;
  u32 calsize;

  startptr = ptr = page->code;
  i = 0;
  j = 0;
  count = 0;  
  memset((void*)ptr,0,sizeof(char)*MAXBLOCKSIZE);
  memcpy((void*)ptr, (void*)prologue, PROLOGSIZE);
  ptr += PROLOGSIZE;
  int MaxSize = 0;

  void * nomal_seperator;
  u32 nomal_seperator_size;
  u8 nomal_seperator_counter_offset;
  void * delay_seperator;
  u32 delay_seperator_size;
  u8 delayslot_seperator_counter_offset;

  if (0x06006094 == start_addr) {
    int a = 0;
  }

  if (debug_mode_) {
    nomal_seperator = (void*)seperator_d_normal;
    nomal_seperator_size = SEPERATORSIZE_DEBUG;
    nomal_seperator_counter_offset = NORMAL_CLOCK_OFFSET_DEBUG;
    delay_seperator = (void*)seperator_d_delay;
    delay_seperator_size = SEPERATORSIZE_DELAYD;
    delayslot_seperator_counter_offset = DALAY_CLOCK_OFFSET;
  }
  else {
    nomal_seperator = (void*)seperator_normal;
    nomal_seperator_size = SEPERATORSIZE_NORMAL;
    nomal_seperator_counter_offset = NORMAL_CLOCK_OFFSET;
    delay_seperator = (void*)seperator_delay_slot;
    delay_seperator_size = SEPERATORSIZE_DELAY_SLOT;
    delayslot_seperator_counter_offset = DALAY_CLOCK_OFFSET;
  }
  
  page->flags = 0;
  
#ifdef BUILD_INFO  
  LOG("*********** start block *************\n");
#endif

  //MaxSize = MAXBLOCKSIZE - MAXINSTRSIZE- delay_seperator_size - SEPERATORSIZE_DELAY_AFTER - nomal_seperator_size - EPILOGSIZE;
  //while (ptr - startptr < MaxSize) {
  while (1) {
    // translate the opcode and insert code
    op = memGetWord(addr);
#ifdef SET_DIRTY
    if (ParentT) {
      u32 keepaddr = adress_mask(addr);
      ParentT[keepaddr].push_back(adress_mask(start_addr));
      ParentT[keepaddr].unique();
    }
#endif

    i = dsh2_instructions[op];
    if (asm_list[i].func == 0) {
      return -1;  // bad instruction code
    }

    // CheckSize
    u8 delay = asm_list[i].delay;
    if ( delay == 0 || delay == 0xFF) {
      calsize  = (ptr - startptr) + *asm_list[i].size + nomal_seperator_size + EPILOGSIZE;
    }else if(delay == 1 || delay == 5) {
      calsize = (ptr - startptr) + *asm_list[i].size + nomal_seperator_size + DELAYJUMPSIZE + EPILOGSIZE;
    } else {
      u32 op2 = memGetWord(addr+2);
      u32 delayop = dsh2_instructions[op2];
      calsize = (ptr - startptr) + *asm_list[i].size + *asm_list[delayop].size + delay_seperator_size + SEPERATORSIZE_DELAY_AFTER + EPILOGSIZE;
    }

    if (calsize >= MAXBLOCKSIZE) {
      break; // no space is available
    }

    if (0x1b == op) { // SLEEP
      page->flags |= BLOCK_LOOP;
    }

    // Inifinity Loop Detection
    if (count == 0 && (op & 0xF00F) == 0x6000) {
      u32 loopcheck = memGetLong(addr + 2);
      if ((loopcheck & 0xFF00FFFF) == 0xC80089FC) { // test, bf
        page->flags |= BLOCK_LOOP;
      }
    }

#ifdef BUILD_INFO  
    LOG("compiling %08X, 0x%04X @ 0x%08X\n", startptr, op, addr);
#endif    

    addr += 2;

#ifdef BUILD_INFO
    DumpInstX( i, addr-2, op  );
#endif

    instruction_counter++;
    asm_list[i].build_count++;
    write_memory_counter += asm_list[i].write_count;

    // Regular Opcode ( No Delay Branch )
    if (asm_list[i].delay == 0) { 
      memcpy((void*)ptr, (void*)(asm_list[i].func), *(asm_list[i].size));
      memcpy((void*)(ptr + *(asm_list[i].size)), (void*)nomal_seperator, nomal_seperator_size);
      instrSize[blockCount][count++] = *(asm_list[i].size) + nomal_seperator_size;
      opcodePass(&asm_list[i], op, ptr);
      u8 * counterpos = ptr + *(asm_list[i].size) + nomal_seperator_counter_offset;
      *counterpos = asm_list[i].cycle;
      ptr += *(asm_list[i].size) + nomal_seperator_size;
    }

    // No Intrupt Func ToDo: Never end block these functions
    else if (asm_list[i].delay == 0xFF ) { 
      memcpy((void*)ptr, (void*)(asm_list[i].func), *(asm_list[i].size));
      memcpy((void*)(ptr + *(asm_list[i].size)), (void*)nomal_seperator, nomal_seperator_size);
      instrSize[blockCount][count++] = *(asm_list[i].size) + nomal_seperator_size;
      opcodePass(&asm_list[i], op, ptr);
      ptr += *(asm_list[i].size) + nomal_seperator_size;
    }

    // Normal Jump
    else if (asm_list[i].delay == 1 || asm_list[i].delay == 5 ) { 
      memcpy((void*)ptr, (void*)(asm_list[i].func), *(asm_list[i].size));
      memcpy((void*)(ptr + *(asm_list[i].size)), (void*)nomal_seperator, nomal_seperator_size);
      memcpy((void*)(ptr + *(asm_list[i].size)+ nomal_seperator_size), (void*)PageFlip, DELAYJUMPSIZE);
      instrSize[blockCount][count++] = *(asm_list[i].size) + nomal_seperator_size + DELAYJUMPSIZE;
      opcodePass(&asm_list[i], op, ptr);
      u8 * counterpos = ptr + *(asm_list[i].size) + nomal_seperator_counter_offset;
      *counterpos = asm_list[i].cycle;
      ptr += *(asm_list[i].size) + nomal_seperator_size + DELAYJUMPSIZE;
    }

    // Jmp With Delay Operation
    else { 

      u8 cycle = asm_list[i].cycle;
      memcpy((void*)ptr, (void*)(asm_list[i].func), *(asm_list[i].size));
      memcpy((void*)(ptr + *(asm_list[i].size)), (void*)delay_seperator, delay_seperator_size);
      instrSize[blockCount][count++] = *(asm_list[i].size) + delay_seperator_size;
      opcodePass(&asm_list[i], op, ptr);
      ptr += *(asm_list[i].size) + delay_seperator_size;

      // Get NExt instruction
      temp = memGetWord(addr);
#ifdef SET_DIRTY
      if (ParentT) {
        u32 keepaddr = adress_mask(addr);
        ParentT[keepaddr].push_back(adress_mask(start_addr));
        ParentT[keepaddr].unique();
      }
#endif
      addr += 2;
      j = opcodeIndex(temp);
#ifdef BUILD_INFO
      DumpInstX( j, addr-2, temp  );
#endif
      if (asm_list[j].func == 0) {
        LOG("Unimplemented Opcode (0x%4x) at 0x%8x\n", temp, addr-2);
        return -1;
        break;
      }
      
      asm_list[j].build_count++;
      write_memory_counter += asm_list[j].write_count;

      cycle += asm_list[j].cycle;
      memcpy((void*)ptr, (void*)(asm_list[j].func), *(asm_list[j].size));
      memcpy((void*)(ptr + *(asm_list[j].size)), (void*)seperator_delay_after, SEPERATORSIZE_DELAY_AFTER);
      instrSize[blockCount][count++] = *(asm_list[j].size) + SEPERATORSIZE_DELAY_AFTER;
      opcodePass(&asm_list[j], temp, ptr);
      u8 * counterpos = ptr + *(asm_list[j].size) + delayslot_seperator_counter_offset;
      *counterpos = cycle;
      ptr += *(asm_list[j].size) + SEPERATORSIZE_DELAY_AFTER;
    }

    if (asm_list[i].delay != 0xFF && asm_list[i].delay != 0x00) {
      // Loop Detectator
      u32 jumppc = 0xFFFFFFFF;
      //immediate w/o delay branch
      if (asm_list[i].delay == 1) {
        jumppc = addr + ((signed char)(op & 0xff) << 1) + 2;

      //offset3
      }
      else if (asm_list[i].delay == 2) {
        temp = (op & 0xfff) << 1;
        if (temp & 0x1000)
          temp |= 0xfffff000;
        jumppc = addr + ((signed)(op & 0xfff) << 1);

      //immediate
      }
      else if (asm_list[i].delay == 3) {
        jumppc = addr + ((signed char)(op & 0xff) << 1);
      }

      // jump to inside and no write is happend
      if (jumppc >= start_addr &&  jumppc < addr ) {
        if (write_memory_counter == 0) {
          page->flags |= BLOCK_LOOP;
#ifdef BUILD_INFO 
              LOG("InfinityLoop block %08X 0x%04X  from 0x%08X to 0x%08X\n", start_addr, op, addr - 2, jumppc);
#endif
        }
      }
      else if (jumppc < start_addr && write_memory_counter == 0 ) {

        Block * tmp = NULL; 
        if ( (jumppc&0x0FF00000) == 0x06000000 && (start_addr & 0x0FF00000) == 0x06000000) {
          tmp = LookupTable[(jumppc & 0x000FFFFF) >> 1];
        }else if ((jumppc & 0x0FF00000) == 0x00200000 && (start_addr & 0x0FF00000) == 0x00200000) {
          tmp = LookupTableLow[(jumppc & 0x000FFFFF) >> 1];
        }
        else if ((jumppc & 0x0FF00000) == 0x00000000 && (start_addr & 0x0FF00000) == 0x00000000) {
          tmp = LookupTableRom[(jumppc & 0x000FFFFF) >> 1];
        }
        if (tmp != NULL && (tmp->flags&BLOCK_WRITE) == 0 && (tmp->e_addr+2) == page->b_addr ) {
          page->flags |= BLOCK_LOOP;
        }

      }
      break;
    }
  }
  page->e_addr = addr-2;
  memcpy((void*)ptr, (void*)epilogue, EPILOGSIZE);
  ptr += EPILOGSIZE;

  if (write_memory_counter > 0) {
    page->flags |= BLOCK_WRITE;
  }

#ifdef BUILD_INFO 
  LOG("*********** end block size = %08X *************\n", (ptr - startptr));
#endif 

#if defined(ARCH_IS_LINUX)
  cacheflush((uintptr_t)page->code,(uintptr_t)ptr,0);
#endif

#if 0 // Dump code
  char fname[64];
#if defined(ANDROID)
  sLOG(fname,"/mnt/sdcard/yabause/%08X.bin",start_addr);
#else
  sLOG(fname,"%08X.bin",start_addr);
#endif
   FILE * fp = fopen(fname, "wb");
  if(fp){
    fwrite(page->code, sizeof(char), (uintptr_t)ptr - (uintptr_t)page->code, fp);
    fclose(fp);
  }
#endif

  return 0;
}

DynarecSh2::DynarecSh2() {
  m_pDynaSh2     = new tagSH2;
  m_pDynaSh2->getmembyte = (uintptr_t)memGetByte;
  m_pDynaSh2->getmemword = (uintptr_t)memGetWord;
  m_pDynaSh2->getmemlong = (uintptr_t)memGetLong;
  m_pDynaSh2->setmembyte = (uintptr_t)memSetByte;
  m_pDynaSh2->setmemword = (uintptr_t)memSetWord;
  m_pDynaSh2->setmemlong = (uintptr_t)memSetLong;
  m_pDynaSh2->eachclock = (uintptr_t)DebugEachClock;
  
  m_pCompiler = CompileBlocks::getInstance();
  m_ClockCounter = 0;
  m_IntruptTbl.clear();
  m_bIntruptSort = true;
  pre_cnt_ = 0;
  pre_exe_count_ = 0;
  interruput_chk_cnt_ = 0;
  interruput_cnt_ = 0;
  pre_PC_ = 0;
  ctx_ = NULL;
  mtx_ = YabThreadCreateMutex();
  logenable_ = false;
}

DynarecSh2::~DynarecSh2(){
  YabThreadFreeMutex(mtx_);
}

void DynarecSh2::ResetCPU(){
  memset((void*)m_pDynaSh2->GenReg, 0, sizeof(u32) * 16);
  memset((void*)m_pDynaSh2->CtrlReg, 0, sizeof(u32) * 3);
  memset((void*)m_pDynaSh2->SysReg, 0, sizeof(u32) * 6);

  m_pDynaSh2->CtrlReg[0] = 0x000000;  // SR
  m_pDynaSh2->CtrlReg[2] = 0x000000; // VBR
  m_pDynaSh2->SysReg[3] = memGetLong(m_pDynaSh2->CtrlReg[2]);
  m_pDynaSh2->GenReg[15] = memGetLong(m_pDynaSh2->CtrlReg[2] + 4);
  m_pDynaSh2->SysReg[4] = 0;
  m_pDynaSh2->SysReg[5] = 0;
  pre_cnt_ = 0;
  pre_exe_count_ = 0;
  interruput_chk_cnt_ = 0;
  interruput_cnt_ = 0;
  m_IntruptTbl.clear();
}

void DynarecSh2::ExecuteCount( u32 Count ) {
  
  u32 targetcnt = 0;
  
  if (Count > pre_exe_count_) {
    targetcnt = m_pDynaSh2->SysReg[4] + Count - pre_exe_count_;
  }
  else {
    pre_exe_count_ = pre_exe_count_-Count;
    return;
  }

  // Overflow
  if (targetcnt < m_pDynaSh2->SysReg[4]){
    targetcnt = Count + (0xFFFFFFFF - m_pDynaSh2->SysReg[4]) + 1;
    m_pDynaSh2->SysReg[4] = 0;
  }


  if ((GET_SR() & 0xF0) < GET_ICOUNT()) {
    this->CheckInterupt();
  }

  while (GET_COUNT() < targetcnt) {
    if (Execute() == IN_INFINITY_LOOP ) {
      SET_COUNT(targetcnt);
      loopskip_cnt_++;
    }
    CurrentSH2->cycles = GET_COUNT();
  }

  //if (Count == 1) {
  //  one_step_ = true;
  //  pre_exe_count_ = 0;
  //}
  //else {
  //  one_step_ = false;
  pre_exe_count_ = m_pDynaSh2->SysReg[4] - targetcnt;
  //}
}

int DynarecSh2::CheckOneStep() {
  if (one_step_ == 1) {
    m_pDynaSh2->SysReg[3] += 2;
    return 1;
  }
  return 0;
}

void DynarecSh2::Undecoded(){

//  LOG("Undecoded %08X", GET_PC());
  // Save regs.SR on stack
  GetGenRegPtr()[15] -= 4;
  memSetLong(GetGenRegPtr()[15], GET_SR());

  // Save regs.PC on stack
  GetGenRegPtr()[15] -= 4;
  memSetLong(GetGenRegPtr()[15], GET_PC()+2);


  // What caused the exception? The delay slot or a general instruction?
  // 4 for General Instructions, 6 for delay slot
  u32 vectnum = 4; //  Fix me

  // Jump to Exception service routine
  u32 newpc = memGetLong(GET_VBR() + (vectnum << 2));
  SET_PC(newpc);

  return;
}

int DynarecSh2::Execute(){

  Block * pBlock = NULL;

  m_pCompiler->exec_count_++;

  switch( GET_PC() & 0x0FF00000 )
  {
    
  // ROM
  case 0x00000000:
    if (yabsys.emulatebios){
      BiosHandleFunc(ctx_);
      return IN_INFINITY_LOOP;
    }
    pBlock = m_pCompiler->LookupTableRom[(GET_PC() & 0x000FFFFF) >> 1];
    if( pBlock == NULL )
    {
      pBlock = m_pCompiler->CompileBlock(GET_PC());
      if (pBlock == NULL) {
        Undecoded();
        return IN_INFINITY_LOOP;
      }
      m_pCompiler->LookupTableRom[(GET_PC() & 0x000FFFFF) >> 1] = pBlock;
    }
    break;

  // Low Memory
  case 0x00200000:
    pBlock = m_pCompiler->LookupTableLow[(GET_PC() & 0x000FFFFF) >> 1];
    if( pBlock == NULL )
    {
      pBlock = m_pCompiler->CompileBlock(GET_PC());
      if (pBlock == NULL) {
        Undecoded();
        return IN_INFINITY_LOOP;
      }
      m_pCompiler->LookupTableLow[(GET_PC() & 0x000FFFFF) >> 1] = pBlock;
    }
    break;

  // High Memory
  case 0x06000000:
  /*case 0x06100000:*/

    pBlock = m_pCompiler->LookupTable[ (GET_PC() & 0x000FFFFF)>>1 ];
    if( pBlock == NULL )
    {
      pBlock = m_pCompiler->CompileBlock(GET_PC(), m_pCompiler->LookupParentTable);
      if (pBlock == NULL) {
        Undecoded();
        return IN_INFINITY_LOOP;
      }
      m_pCompiler->LookupTable[ (GET_PC() & 0x000FFFFF)>>1 ] = pBlock;
    } 
    break;

  // Cache
  default:
    if( (GET_PC() & 0xFF000000) == 0xC0000000 )
    {
      pBlock = m_pCompiler->LookupTableC[ (GET_PC() & 0x000FFFFF)>>1 ];
      if( pBlock == NULL )
      {
        pBlock = m_pCompiler->CompileBlock(GET_PC());
        m_pCompiler->LookupTableC[ (GET_PC()&0x000FFFFF)>>1 ] = pBlock;
        if (pBlock == NULL) {
           Undecoded();
           return IN_INFINITY_LOOP;
        }
      } 
    }else{
      pBlock = m_pCompiler->CompileBlock(GET_PC());
      if (pBlock == NULL) {
        Undecoded();
        return IN_INFINITY_LOOP;
      }
    }
    break;  
   }
    
#if 0
    static FILE * fp = NULL;
    char fname[64];
    sprintf(fname,"/mnt/sdcard/yabause/intlog.txt");
    if( fp == NULL ) {
        fp = fopen(fname, "w");
    }
    if(fp){
        fprintf(fp,"\n---dynaExecute %08X----\n", GET_PC());
        fflush(fp);
    }
#endif
  u32 prepc  = GET_PC();
  //if(yabsys.frame_count == 1000){
 //   logenable_ = true;
  //}
  //if( logenable_ )
  //  LOG("[%s] dynaExecute start %08X", (is_slave_ == false) ? "M" : "S", GET_PC());
#if defined(DEBUG_CPU)
  if (statics_trigger_ == COLLECTING) {
    u64 pretime = YabauseGetTicks();
    ((dynaFunc)((void*)(pBlock->code)))(m_pDynaSh2);
    compie_statics_[prepc].count++;
    compie_statics_[prepc].time += YabauseGetTicks() - pretime;
    compie_statics_[prepc].end_addr = pBlock->e_addr;
  }
  else {
    ((dynaFunc)((void*)(pBlock->code)))(m_pDynaSh2);
  }
#else
  ((dynaFunc)((void*)(pBlock->code)))(m_pDynaSh2);
#endif

  if (!m_pCompiler->debug_mode_ && (pBlock->flags&BLOCK_LOOP)) {
    return IN_INFINITY_LOOP;
  }
  return 0;
}

bool operator < (const dIntcTbl & data1 , const dIntcTbl & data2 )
{
  return data1.level > data2.level;
} 
bool operator == (const dIntcTbl & data1 , const dIntcTbl & data2 )
{
  return ( data1.Vector == data2.Vector );
}

void DynarecSh2::AddInterrupt( u8 Vector, u8 level )
{
  // Ignore Timer0 and Timer1 when masked
  if ((Vector == 67 || Vector == 68) && level <= ((m_pDynaSh2->CtrlReg[0] >> 4) & 0x0F)){
    return;
  }

  dIntcTbl tmp;
  tmp.Vector = Vector;
  tmp.level  = level;

  YabThreadLock(mtx_);
  m_bIntruptSort = false;
  m_IntruptTbl.push_back(tmp);
  m_IntruptTbl.unique(); 

  //printf("AddInterrupt v:%d l:%d\n", Vector, level );



  if( m_IntruptTbl.size() > 1 ) {
    m_IntruptTbl.sort();
  }
  m_bIntruptSort = true;
  m_pDynaSh2->SysReg[5] = m_IntruptTbl.begin()->level<<4;
  YabThreadUnLock(mtx_);
}


int DynarecSh2::CheckInterupt(){

  interruput_chk_cnt_++;

  if( m_IntruptTbl.size() == 0 ) {
    return 0;
  }

  //LOG("CheckInterupt %d\n", m_IntruptTbl.size() );
    
  YabThreadLock(mtx_);  
  dlstIntct::iterator pos = m_IntruptTbl.begin();
  if( InterruptRutine((*pos).Vector, (*pos).level ) != 0 ) {
    m_IntruptTbl.pop_front();
    if( m_IntruptTbl.size() != 0 ) {
      m_pDynaSh2->SysReg[5] = m_IntruptTbl.begin()->level<<4;
    }else{
      m_pDynaSh2->SysReg[5] = 0x0000;
    }
    YabThreadUnLock(mtx_);
    return 1;
  }
  YabThreadUnLock(mtx_);
  return 0;
}

int DynarecSh2::InterruptRutine(u8 Vector, u8 level)
{
  if (((u32)level) > ((m_pDynaSh2->CtrlReg[0] >> 4) & 0x0F)) {

    u32 prepc = m_pDynaSh2->SysReg[3];

    interruput_cnt_++;
    m_pDynaSh2->GenReg[15] -= 4;
    memSetLong(m_pDynaSh2->GenReg[15], m_pDynaSh2->CtrlReg[0]);
    m_pDynaSh2->GenReg[15] -= 4;
    memSetLong(m_pDynaSh2->GenReg[15], m_pDynaSh2->SysReg[3]);
    m_pDynaSh2->CtrlReg[0] |= ((u32)(level << 4) & 0x000000F0);
    m_pDynaSh2->SysReg[3] = memGetLong(m_pDynaSh2->CtrlReg[2] + (((u32)Vector) << 2));
#if defined(DEBUG_CPU)
//    LOG("**** [%s] Exception vecnum=%u, PC=%08X to %08X, level=%08X\n", (is_slave_==false)?"M":"S", Vector, prepc, m_pDynaSh2->SysReg[3], level);
#endif
    return 1;
  }
  return 0; 
}


int DynarecSh2GetDisasmebleString(string & out, u32 from, u32 to) {
  char linebuf[128];
  if (from > to) return -1;
  for (u32 i = from; i < (to+2); i += 2) {
    SH2Disasm(i, memGetWord(i), 0, NULL, linebuf);
    out += linebuf;
    out += "\n";
  }
  return 0;
}

int DynarecSh2::Resume() {
  statics_trigger_ = NORMAL;
  return 0;
}

void DynarecSh2::ShowStatics(){
#if defined(DEBUG_CPU)
  LOG("\nExec cnt %d loopskip_cnt_ = %d, interruput_chk_cnt_ = %d, interruput_cnt_ = %d\n", GET_COUNT() - pre_cnt_, loopskip_cnt_, interruput_chk_cnt_, interruput_cnt_ );
  pre_cnt_ = GET_COUNT();
  interruput_chk_cnt_ = 0;
  interruput_cnt_ = 0;
  loopskip_cnt_ = 0;

  switch (statics_trigger_) {
  case NORMAL:
    break;
  case REQUESTED:
    statics_trigger_ = COLLECTING;
    break;
  case COLLECTING:
    statics_trigger_ = FINISHED;
    while (FINISHED == statics_trigger_) {
      YabThreadUSleep(10000);
    }
    break;
  case FINISHED:
    break;
  }
#endif
}

int DynarecSh2::GetCurrentStatics(MapCompileStatics & buf){
#if !defined(DEBUG_CPU)
//  buf = "Not Debug Mode\n";
#else

  if (statics_trigger_ != NORMAL && statics_trigger_ != FINISHED) return -1;

  statics_trigger_ = REQUESTED;
  while (statics_trigger_!= FINISHED) {
    YabThreadUSleep(10000);
  }
  
  buf = compie_statics_;
  compie_statics_.clear();
#endif
  return 0;
}

void DynarecSh2::ShowCompileInfo(){
  int i = 0;
  while (asm_list[i].func != NULL) {
    LOG("%s: %d\n", opcode_list[i].mnem, asm_list[i].build_count);
    i++;
  }
}

void DynarecSh2::ResetCompileInfo() {
  int i = 0;
  while (asm_list[i].func != NULL) {
    asm_list[i].build_count = 0;
    i++;
  }
}
