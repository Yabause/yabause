/******************************** -*- C -*- ****************************
 *
 *	Platform-independent layer (Sparc version)
 *
 ***********************************************************************/


/***********************************************************************
 *
 * Copyright 2000, 2001, 2002 Free Software Foundation, Inc.
 * Written by Paolo Bonzini.
 *
 * This file is part of GNU lightning.
 *
 * GNU lightning is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1, or (at your option)
 * any later version.
 * 
 * GNU lightning is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with GNU lightning; see the file COPYING.LESSER; if not, write to the
 * Free Software Foundation, 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 ***********************************************************************/


#ifndef __lightning_core_h
#define __lightning_core_h

#define JIT_R_NUM		3
#define JIT_V_NUM		6
#define JIT_R(i)		((i) ? _Rl((i) - 1) : _Rg(2))
#define JIT_V(i)		_Rl((i)+2)

#define JIT_BIG			_Rg(1)	/* %g1 used to make 32-bit operands */
#define JIT_BIG2		_Ro(7)	/* %o7 used to make 32-bit compare operands */
#define JIT_SP			_Ro(6)
#define JIT_RZERO		_Rg(0)
#define JIT_RET			_Ri(0)

/* Delay slot scheduling: jmp generates branches with annulled delay
 * slots; we toggle the annul bit if we can fill the slot.  CALLs and
 * cond. branches have a different meaning for the annul bit, so we
 * automatically generate a NOP and eventually copy the delay insn onto
 * it.  Delay slots in RET are already used for RESTORE, so we don't
 * schedule them.
 *
 *	,--- _jit.x.pc
 * insn X       X				before
 * cmp  branch  insn  X	   X			after (branch)
 *		      `--- _jit.x.pc			
 * call insn    insn X				after (call)
 *		`--- _jit.x.pc			
 */

struct jit_local_state {
  int	nextarg_put;	/* Next %o reg. to be written */
  int	nextarg_get;	/* Next %i reg. to be read */
  jit_insn delay;
};

#define jit_fill_delay_after(branch) (_jitl.delay = *--_jit.x.pc, 					 \
	((branch) == _jit.x.pc					  /* check if NOP was inserted */		 \
		? (_jit.x.pc[-1] ^= 1<<29)			  /* no if branch, toggle annul bit  */	 \
		: (_jit.x.pc[-1] = _jitl.delay)),			  /* yes if call, replace NOP with delay insn */ \
	*_jit.x.pc = _jitl.delay, _jit.x.pc - 1)			  /* return addr of delay insn */

/* If possible, use the `small' instruction (rs, imm, rd)
 * else load imm into %l6 and use the `big' instruction (rs, %l6, rd)
 * jit_chk_imm2 uses %l7 instead of %l6 to avoid conflicts when using delay slots
 */
#define jit_chk_imm(imm, small, big)		(_siP(13,(imm)) ? (small) : (SETir((imm), JIT_BIG),  (big)) )
#define jit_chk_imm2(imm, small, big)		(_siP(13,(imm)) ? (small) : (SETir((imm), JIT_BIG2), (big)) )

/* Helper macros for branches */
#define jit_branchi(rs, is, jmp, nop)		(jit_chk_imm2(is, CMPri(rs, is), CMPrr(rs, JIT_BIG2)), jmp, nop, _jit.x.pc - 1)
#define jit_branchr(s1, s2, jmp, nop)		(		  CMPrr(s1, s2), 		       jmp, nop, _jit.x.pc - 1)

/* Helper macros for boolean tests -- delay slot sets d to 1;
 * taken branch leaves it to 1, not-taken branch resets it to 0 */
#define jit_booli(d, rs, is, jmp)		(jit_chk_imm (is, CMPri(rs, is), CMPrr(rs, JIT_BIG)), jmp, MOVir(1, (d)), MOVir(0, (d)))
#define jit_boolr(d, s1, s2, jmp)		(		  CMPrr(s1, s2), 		      jmp, MOVir(1, (d)), MOVir(0, (d)))

/* Helper macros for division
 * The architecture specifies that there must be 3 instructions between *
 * a y register write and a use of it for correct results. */
#define jit_prepare_y(rs, is)		(SRArir(rs, 31, JIT_BIG), WRri(JIT_BIG, _y), NOP(), NOP(), NOP(), _jit.x.pc -= jit_immsize(is))
#define jit_clr_y(rs, is)		(			  WRri(0,	_y), NOP(), NOP(), NOP(), _jit.x.pc -= jit_immsize(is))

#define jit_modr(jit_div, jit_mul, d, s1, s2)   \
        (jit_div (JIT_BIG, s1, s2),             \
         jit_mul (JIT_BIG, JIT_BIG, s2),        \
         jit_subr_i (d, s1, JIT_BIG))

#define jit_modi(jit_divi, jit_muli, jit_divr, jit_mulr, d, rs, is)     \
        (_siP(13,(imm))                                                 \
         ? (jit_divi (JIT_BIG, rs, is),                                 \
            jit_muli (JIT_BIG, JIT_BIG, is),                            \
            jit_subr_i (d, rs, JIT_BIG))                                \
         : (SETir ((is), JIT_BIG2),                                     \
            jit_modr (jit_divr, jit_mulr, d, rs, JIT_BIG2)))

/* How many instruction are needed to put imm in a register.  */
#define jit_immsize(imm)	(!(imm) ? 0 :			\
				(!_siP((imm), 13) && ((imm) & 0x3ff)  ? 2 : 1))


/* branch instructions return the address of the *delay* instruction -- this
 * is just a helper macro that makes jit_patch more readable.
 */
#define jit_patch_(jump_pc,pv)						\
	(*jump_pc &= ~_MASK(22),					\
	 *jump_pc |= ((_jit_UL((pv)) - _jit_UL(jump_pc)) >> 2) & _MASK(22))

#define jit_patch_set(sethi_pc, or_pc, dest)			\
	(*(sethi_pc) &= ~_MASK(22), *(sethi_pc) |= _HI(dest),	\
	 *(or_pc) &= ~_MASK(13), *(or_pc) |= _LO(dest))		\

#define jit_patch_movi(movi_pc, val)				\
	jit_patch_set((movi_pc) - 2, (movi_pc) - 1, (val))

#define	jit_arg_c()			(_jitl.nextarg_get++)
#define	jit_arg_i()			(_jitl.nextarg_get++)
#define	jit_arg_l()			(_jitl.nextarg_get++)
#define	jit_arg_p()			(_jitl.nextarg_get++)
#define	jit_arg_s()			(_jitl.nextarg_get++)
#define	jit_arg_uc()			(_jitl.nextarg_get++)
#define	jit_arg_ui()			(_jitl.nextarg_get++)
#define	jit_arg_ul()			(_jitl.nextarg_get++)
#define	jit_arg_us()			(_jitl.nextarg_get++)
#define jit_addi_i(d, rs, is)		jit_chk_imm((is), ADDrir((rs), (is), (d)), ADDrrr((rs), JIT_BIG, (d)))
#define jit_addr_i(d, s1, s2)				  ADDrrr((s1), (s2), (d))
#define jit_addci_i(d, rs, is)		jit_chk_imm((is), ADDCCrir((rs), (is), (d)), ADDCCrrr((rs), JIT_BIG, (d)))
#define jit_addcr_i(d, s1, s2)				  ADDCCrrr((s1), (s2), (d))
#define jit_addxi_i(d, rs, is)		jit_chk_imm((is), ADDXCCrir((rs), (is), (d)), ADDXCCrrr((rs), JIT_BIG, (d)))
#define jit_addxr_i(d, s1, s2)				  ADDXCCrrr((s1), (s2), (d))
#define jit_andi_i(d, rs, is)		jit_chk_imm((is), ANDrir((rs), (is), (d)), ANDrrr((rs), JIT_BIG, (d)))
#define jit_andr_i(d, s1, s2)				  ANDrrr((s1), (s2), (d))
#define jit_beqi_i(label, rs, is)	jit_branchi((rs), (is), BEi((label)), NOP() )
#define jit_beqr_i(label, s1, s2)	jit_branchr((s1), (s2), BEi((label)), NOP() )
#define jit_bgei_i(label, rs, is)	jit_branchi((rs), (is), BGEi((label)), NOP() )
#define jit_bgei_ui(label, rs, is)	jit_branchi((rs), (is), BGEUi((label)), NOP() )
#define jit_bger_i(label, s1, s2)	jit_branchr((s1), (s2), BGEi((label)), NOP() )
#define jit_bger_ui(label, s1, s2)	jit_branchr((s1), (s2), BGEUi((label)), NOP() )
#define jit_bgti_i(label, rs, is)	jit_branchi((rs), (is), BGi((label)), NOP() )
#define jit_bgti_ui(label, rs, is)	jit_branchi((rs), (is), BGUi((label)), NOP() )
#define jit_bgtr_i(label, s1, s2)	jit_branchr((s1), (s2), BGi((label)), NOP() )
#define jit_bgtr_ui(label, s1, s2)	jit_branchr((s1), (s2), BGUi((label)), NOP() )
#define jit_blei_i(label, rs, is)	jit_branchi((rs), (is), BLEi((label)), NOP() )
#define jit_blei_ui(label, rs, is)	jit_branchi((rs), (is), BLEUi((label)), NOP() )
#define jit_bler_i(label, s1, s2)	jit_branchr((s1), (s2), BLEi((label)), NOP() )
#define jit_bler_ui(label, s1, s2)	jit_branchr((s1), (s2), BLEUi((label)), NOP() )
#define jit_blti_i(label, rs, is)	jit_branchi((rs), (is), BLi((label)), NOP() )
#define jit_blti_ui(label, rs, is)	jit_branchi((rs), (is), BLUi((label)), NOP() )
#define jit_bltr_i(label, s1, s2)	jit_branchr((s1), (s2), BLi((label)), NOP() )
#define jit_bltr_ui(label, s1, s2)	jit_branchr((s1), (s2), BLUi((label)), NOP() )
#define jit_bnei_i(label, rs, is)	jit_branchi((rs), (is), BNEi((label)), NOP() )
#define jit_bner_i(label, s1, s2)	jit_branchr((s1), (s2), BNEi((label)), NOP() )
#define jit_bmsi_i(label, rs, is)	(jit_chk_imm((is), BTSTir((is), (rs)), BTSTrr((rs), JIT_BIG)), BNEi((label)), NOP(), _jit.x.pc - 1)
#define jit_bmci_i(label, rs, is)	(jit_chk_imm((is), BTSTir((is), (rs)), BTSTrr((rs), JIT_BIG)), BEi((label)), NOP(),  _jit.x.pc - 1)
#define jit_bmsr_i(label, s1, s2)	(		   BTSTrr((s1), (s2)),			       BNEi((label)), NOP(), _jit.x.pc - 1)
#define jit_bmcr_i(label, s1, s2)	(		   BTSTrr((s1), (s2)),			       BEi((label)), NOP(),  _jit.x.pc - 1)
#define jit_boaddi_i(label, rs, is)	(jit_chk_imm((is), ADDCCrir((rs), (is), (rs)), ADDCCrrr((rs), JIT_BIG, (rs))), BVSi((label)), NOP(), _jit.x.pc - 1)
#define jit_bosubi_i(label, rs, is)	(jit_chk_imm((is), SUBCCrir((rs), (is), (rs)), SUBCCrrr((rs), JIT_BIG, (rs))), BVSi((label)), NOP(), _jit.x.pc - 1)
#define jit_boaddr_i(label, s1, s2)	(		   ADDCCrrr((s1), (s2), (s1)),			         BVSi((label)), NOP(), _jit.x.pc - 1)
#define jit_bosubr_i(label, s1, s2)	(		   SUBCCrrr((s1), (s2), (s1)),			         BVSi((label)), NOP(), _jit.x.pc - 1)
#define jit_boaddi_ui(label, rs, is)	(jit_chk_imm((is), ADDCCrir((rs), (is), (rs)), ADDCCrrr((rs), JIT_BIG, (rs))), BCSi((label)), NOP(), _jit.x.pc - 1)
#define jit_bosubi_ui(label, rs, is)	(jit_chk_imm((is), SUBCCrir((rs), (is), (rs)), SUBCCrrr((rs), JIT_BIG, (rs))), BCSi((label)), NOP(), _jit.x.pc - 1)
#define jit_boaddr_ui(label, s1, s2)	(		   ADDCCrrr((s1), (s2), (s1)),			         BCSi((label)), NOP(), _jit.x.pc - 1)
#define jit_bosubr_ui(label, s1, s2)	(		   SUBCCrrr((s1), (s2), (s1)),			         BCSi((label)), NOP(), _jit.x.pc - 1)
#define jit_calli(label)		(CALLi(label), NOP(), _jit.x.pc - 1)
#define jit_callr(reg)			(CALLx((reg), 0), NOP())

#define jit_divi_i(d, rs, is)		(jit_prepare_y((rs), 0x12345678), SETir((is), JIT_BIG), SDIVrrr((rs), JIT_BIG, (d)) )
#define jit_divi_ui(d, rs, is)		(jit_clr_y((rs),     0x12345678), SETir((is), JIT_BIG), UDIVrrr((rs), JIT_BIG, (d)) )
#define jit_divr_i(d, s1, s2)		(jit_prepare_y((s1), 0), 				SDIVrrr((s1), (s2), (d)))
#define jit_divr_ui(d, s1, s2)		(jit_clr_y((s1),     0), 				UDIVrrr((s1), (s2), (d)))
#define jit_eqi_i(d, rs, is)		jit_chk_imm((is), \
  (SUBCCrir((rs), (is), (d)), ADDXCCrir((d), -1, JIT_BIG), SUBXrir(0,-1,(d))),\
  jit_eqr_i(d, rs, JIT_BIG))
#define jit_eqr_i(d, s1, s2)		  (SUBCCrrr((s1), (s2), (d)), ADDXCCrir((d), -1, JIT_BIG), SUBXrir(0,-1,(d)))
#define jit_nei_i(d, rs, is)		jit_chk_imm((is), \
  (SUBCCrir((rs), (is), (d)), ADDXCCrir((d), -1, JIT_BIG), ADDXrrr(0,0,(d))),\
  jit_ner_i(d, rs, JIT_BIG))
#define jit_ner_i(d, s1, s2)		  (SUBCCrrr((s1), (s2), (d)), ADDXCCrir((d), -1, JIT_BIG), ADDXrrr(0,0,(d)))
#define jit_gei_i(d, rs, is)		jit_booli ((d), (rs), (is), BGEi(_jit.x.pc + 3) )
#define jit_gei_ui(d, rs, is)		jit_booli ((d), (rs), (is), BGEUi(_jit.x.pc + 3))
#define jit_ger_i(d, s1, s2)		jit_boolr ((d), (s1), (s2), BGEi(_jit.x.pc + 3) )
#define jit_ger_ui(d, s1, s2)		jit_boolr ((d), (s1), (s2), BGEUi(_jit.x.pc + 3))
#define jit_gti_i(d, rs, is)		jit_booli ((d), (rs), (is), BGi(_jit.x.pc + 3) )
#define jit_gti_ui(d, rs, is)		jit_booli ((d), (rs), (is), BGUi(_jit.x.pc + 3) )
#define jit_gtr_i(d, s1, s2)		jit_boolr ((d), (s1), (s2), BGi(_jit.x.pc + 3)  )
#define jit_gtr_ui(d, s1, s2)		jit_boolr ((d), (s1), (s2), BGUi(_jit.x.pc + 3) )
#define jit_hmuli_i(d, rs, is)		(jit_muli_i (JIT_BIG, (rs), (is)), RDir (_y, (d)))
#define jit_hmuli_ui(d, rs, is)		(jit_muli_ui(JIT_BIG, (rs), (is)), RDir (_y, (d)))
#define jit_hmulr_i(d, s1, s2)		(jit_mulr_i (JIT_BIG, (s1), (s2)), RDir (_y, (d)))
#define jit_hmulr_ui(d, s1, s2)		(jit_mulr_ui(JIT_BIG, (s1), (s2)), RDir (_y, (d)))
#define jit_jmpi(label)			(BA_Ai((label)), _jit.x.pc)
#define jit_jmpr(reg)			(JMPx(JIT_RZERO, (reg)), NOP(), _jit.x.pc - 1)
#define jit_ldxi_c(d, rs, is)		jit_chk_imm((is), LDSBmr((rs), (is), (d)), LDSBxr((rs), JIT_BIG, (d)))
#define jit_ldxi_i(d, rs, is)		jit_chk_imm((is), LDSWmr((rs), (is), (d)), LDSWxr((rs), JIT_BIG, (d)))
#define jit_ldxi_s(d, rs, is)		jit_chk_imm((is), LDSHmr((rs), (is), (d)), LDSHxr((rs), JIT_BIG, (d)))
#define jit_ldxi_uc(d, rs, is)		jit_chk_imm((is), LDUBmr((rs), (is), (d)), LDUBxr((rs), JIT_BIG, (d)))
#define jit_ldxi_us(d, rs, is)		jit_chk_imm((is), LDUHmr((rs), (is), (d)), LDUHxr((rs), JIT_BIG, (d)))
#define jit_ldxr_c(d, s1, s2)				  LDSBxr((s1), (s2), (d))
#define jit_ldxr_i(d, s1, s2)				  LDSWxr((s1), (s2), (d))
#define jit_ldxr_s(d, s1, s2)				  LDSHxr((s1), (s2), (d))
#define jit_ldxr_uc(d, s1, s2)				  LDUBxr((s1), (s2), (d))
#define jit_ldxr_us(d, s1, s2)				  LDUHxr((s1), (s2), (d))
#define jit_lei_i(d, rs, is)		jit_booli ((d), (rs), (is), BLEi(_jit.x.pc + 3) )
#define jit_lei_ui(d, rs, is)		jit_booli ((d), (rs), (is), BLEUi(_jit.x.pc + 3))
#define jit_ler_i(d, s1, s2)		jit_boolr ((d), (s1), (s2), BLEi(_jit.x.pc + 3) )
#define jit_ler_ui(d, s1, s2)		jit_boolr ((d), (s1), (s2), BLEUi(_jit.x.pc + 3))
#define jit_lshi_i(d, rs, is)		SLLrir((rs), (is), (d))
#define jit_lshr_i(d, r1, r2)		SLLrrr((r1), (r2), (d))
#define jit_lti_i(d, rs, is)		jit_booli ((d), (rs), (is), BLi(_jit.x.pc + 3)  )
#define jit_lti_ui(d, rs, is)		jit_booli ((d), (rs), (is), BLUi(_jit.x.pc + 3) )
#define jit_ltr_i(d, s1, s2)		jit_boolr ((d), (s1), (s2), BLi(_jit.x.pc + 3)  )
#define jit_ltr_ui(d, s1, s2)		jit_boolr ((d), (s1), (s2), BLUi(_jit.x.pc + 3) )
#define jit_modi_i(d, rs, is)           jit_modi(jit_divi_i, jit_muli_i, jit_divr_i, jit_mulr_i, (d), (rs), (is))
#define jit_modi_ui(d, rs, is)          jit_modi(jit_divi_ui, jit_muli_ui, jit_divr_ui, jit_mulr_ui, (d), (rs), (is))
#define jit_modr_i(d, s1, s2)           jit_modr(jit_divr_i, jit_mulr_i, (d), (s1), (s2))
#define jit_modr_ui(d, s1, s2)          jit_modr(jit_divr_ui, jit_mulr_ui, (d), (s1), (s2))
#define jit_movi_i(d, is)		SETir((is), (d))
#define jit_movi_p(d, is)		(SETir2(_HI((is)), _LO((is)), (d)), _jit.x.pc)
#define jit_movr_i(d, rs)		MOVrr((rs), (d))
#define jit_muli_i(d, rs, is)		jit_chk_imm((is), SMULrir((rs), (is), (d)), SMULrrr((rs), JIT_BIG, (d)))
#define jit_muli_ui(d, rs, is)		jit_chk_imm((is), UMULrir((rs), (is), (d)), UMULrrr((rs), JIT_BIG, (d)))
#define jit_mulr_i(d, s1, s2)				  SMULrrr((s1), (s2), (d))
#define jit_mulr_ui(d, s1, s2)				  UMULrrr((s1), (s2), (d))
#define jit_nop()			NOP()
#define jit_ori_i(d, rs, is)		jit_chk_imm((is), ORrir((rs), (is), (d)), ORrrr((rs), JIT_BIG, (d)))
#define jit_orr_i(d, s1, s2)				  ORrrr((s1), (s2), (d))
#define jit_patch_at(delay_pc, pv)	jit_patch_ (((delay_pc) - 1) , (pv))
#define jit_popr_i(rs)			(LDmr(JIT_SP, 0, (rs)), ADDrir(JIT_SP, 8, JIT_SP))
#define jit_prepare_i(num)		(_jitl.nextarg_put += (num))
#define jit_prolog(numargs)		(SAVErir(JIT_SP, -120, JIT_SP), _jitl.nextarg_get = _Ri(0))
#define jit_pushr_i(rs)			(STrm((rs), JIT_SP, -8), SUBrir(JIT_SP, 8, JIT_SP))
#define jit_pusharg_i(rs)		(--_jitl.nextarg_put, MOVrr((rs), _Ro(_jitl.nextarg_put)))
#define jit_ret()			(RET(), RESTORE())
#define jit_retval_i(rd)		MOVrr(_Ro(0), (rd))
#define jit_rshi_i(d, rs, is)		SRArir((rs), (is), (d))
#define jit_rshi_ui(d, rs, is)		SRLrir((rs), (is), (d))
#define jit_rshr_i(d, r1, r2)		SRArrr((r1), (r2), (d))
#define jit_rshr_ui(d, r1, r2)		SRLrrr((r1), (r2), (d))
#define jit_stxi_c(id, rd, rs)		jit_chk_imm((id), STBrm((rs), (rd), (id)), STBrx((rs), (rd), JIT_BIG))
#define jit_stxi_i(id, rd, rs)		jit_chk_imm((id), STWrm((rs), (rd), (id)), STWrx((rs), (rd), JIT_BIG))
#define jit_stxi_s(id, rd, rs)		jit_chk_imm((id), STHrm((rs), (rd), (id)), STHrx((rs), (rd), JIT_BIG))
#define jit_stxr_c(d1, d2, rs)				  STBrx((rs), (d1), (d2))
#define jit_stxr_i(d1, d2, rs)				  STWrx((rs), (d1), (d2))
#define jit_stxr_s(d1, d2, rs)				  STHrx((rs), (d1), (d2))
#define jit_subr_i(d, s1, s2)				  SUBrrr((s1), (s2), (d))
#define jit_subcr_i(d, s1, s2)				  SUBCCrrr((s1), (s2), (d))
#define jit_subxi_i(d, rs, is)		jit_chk_imm((is), SUBXCCrir((rs), (is), (d)), SUBXCCrrr((rs), JIT_BIG, (d)))
#define jit_subxr_i(d, s1, s2)				  SUBXCCrrr((s1), (s2), (d))
#define jit_xori_i(d, rs, is)		jit_chk_imm((is), XORrir((rs), (is), (d)), XORrrr((rs), JIT_BIG, (d)))
#define jit_xorr_i(d, s1, s2)				  XORrrr((s1), (s2), (d))

#endif /* __lightning_core_h */
