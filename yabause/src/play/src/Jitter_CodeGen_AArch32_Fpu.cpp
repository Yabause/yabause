#include "Jitter_CodeGen_AArch32.h"

using namespace Jitter;

void CCodeGen_AArch32::LoadMemoryFpSingleInRegister(CTempRegisterContext& tempRegContext, CAArch32Assembler::SINGLE_REGISTER reg, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REL_SINGLE:
		LoadRelativeFpSingleInRegister(tempRegContext, reg, symbol);
		break;
	case SYM_FP_TMP_SINGLE:
		LoadTemporaryFpSingleInRegister(tempRegContext, reg, symbol);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::StoreRegisterInMemoryFpSingle(CTempRegisterContext& tempRegContext, CSymbol* symbol, CAArch32Assembler::SINGLE_REGISTER reg)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REL_SINGLE:
		StoreRelativeFpSingleInRegister(tempRegContext, symbol, reg);
		break;
	case SYM_FP_TMP_SINGLE:
		StoreTemporaryFpSingleInRegister(tempRegContext, symbol, reg);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::LoadRelativeFpSingleInRegister(CTempRegisterContext& tempRegContext, CAArch32Assembler::SINGLE_REGISTER reg, CSymbol* symbol)
{
	assert(symbol->m_type == SYM_FP_REL_SINGLE);
	if((symbol->m_valueLow / 4) < 0x100)
	{
		m_assembler.Vldr(reg, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_valueLow));
	}
	else
	{
		auto offsetRegister = tempRegContext.Allocate();
		LoadConstantInRegister(offsetRegister, symbol->m_valueLow);
		m_assembler.Add(offsetRegister, offsetRegister, g_baseRegister);
		m_assembler.Vldr(reg, offsetRegister, CAArch32Assembler::MakeImmediateLdrAddress(0));
		tempRegContext.Release(offsetRegister);
	}
}

void CCodeGen_AArch32::StoreRelativeFpSingleInRegister(CTempRegisterContext& tempRegContext, CSymbol* symbol, CAArch32Assembler::SINGLE_REGISTER reg)
{
	assert(symbol->m_type == SYM_FP_REL_SINGLE);
	if((symbol->m_valueLow / 4) < 0x100)
	{
		m_assembler.Vstr(reg, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_valueLow));
	}
	else
	{
		auto offsetRegister = tempRegContext.Allocate();
		LoadConstantInRegister(offsetRegister, symbol->m_valueLow);
		m_assembler.Add(offsetRegister, offsetRegister, g_baseRegister);
		m_assembler.Vstr(reg, offsetRegister, CAArch32Assembler::MakeImmediateLdrAddress(0));
		tempRegContext.Release(offsetRegister);
	}
}

void CCodeGen_AArch32::LoadTemporaryFpSingleInRegister(CTempRegisterContext& tempRegContext, CAArch32Assembler::SINGLE_REGISTER reg, CSymbol* symbol)
{
	assert(symbol->m_type == SYM_FP_TMP_SINGLE);
	auto offset = symbol->m_stackLocation + m_stackLevel;
	if((offset / 4) < 0x100)
	{
		m_assembler.Vldr(reg, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel));
	}
	else
	{
		auto offsetRegister = tempRegContext.Allocate();
		LoadConstantInRegister(offsetRegister, offset);
		m_assembler.Add(offsetRegister, offsetRegister, CAArch32Assembler::rSP);
		m_assembler.Vldr(reg, offsetRegister, CAArch32Assembler::MakeImmediateLdrAddress(0));
		tempRegContext.Release(offsetRegister);
	}
}

void CCodeGen_AArch32::StoreTemporaryFpSingleInRegister(CTempRegisterContext& tempRegContext, CSymbol* symbol, CAArch32Assembler::SINGLE_REGISTER reg)
{
	assert(symbol->m_type == SYM_FP_TMP_SINGLE);
	auto offset = symbol->m_stackLocation + m_stackLevel;
	if((offset / 4) < 0x100)
	{
		m_assembler.Vstr(reg, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel));
	}
	else
	{
		auto offsetRegister = tempRegContext.Allocate();
		LoadConstantInRegister(offsetRegister, offset);
		m_assembler.Add(offsetRegister, offsetRegister,CAArch32Assembler::rSP);
		m_assembler.Vstr(reg, offsetRegister, CAArch32Assembler::MakeImmediateLdrAddress(0));
		tempRegContext.Release(offsetRegister);
	}
}

template <typename FPUOP>
void CCodeGen_AArch32::Emit_Fpu_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	CTempRegisterContext tempRegisterContext;

	LoadMemoryFpSingleInRegister(tempRegisterContext, CAArch32Assembler::s0, src1);
	((m_assembler).*(FPUOP::OpReg()))(CAArch32Assembler::s1, CAArch32Assembler::s0);
	StoreRegisterInMemoryFpSingle(tempRegisterContext, dst, CAArch32Assembler::s1);
}

template <typename FPUOP>
void CCodeGen_AArch32::Emit_Fpu_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	CTempRegisterContext tempRegisterContext;

	LoadMemoryFpSingleInRegister(tempRegisterContext, CAArch32Assembler::s0, src1);
	LoadMemoryFpSingleInRegister(tempRegisterContext, CAArch32Assembler::s1, src2);
	((m_assembler).*(FPUOP::OpReg()))(CAArch32Assembler::s2, CAArch32Assembler::s0, CAArch32Assembler::s1);
	StoreRegisterInMemoryFpSingle(tempRegisterContext, dst, CAArch32Assembler::s2);
}

template <typename FPUMDOP>
void CCodeGen_AArch32::Emit_FpuMd_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	CTempRegisterContext tempRegisterContext;

	LoadMemoryFpSingleInRegister(tempRegisterContext, CAArch32Assembler::s0, src1);
	LoadMemoryFpSingleInRegister(tempRegisterContext, CAArch32Assembler::s4, src2);
	((m_assembler).*(FPUMDOP::OpReg()))(CAArch32Assembler::q2, CAArch32Assembler::q0, CAArch32Assembler::q1);
	StoreRegisterInMemoryFpSingle(tempRegisterContext, dst, CAArch32Assembler::s8);
}

void CCodeGen_AArch32::Emit_Fp_Rcpl_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	CTempRegisterContext tempRegisterContext;

	LoadMemoryFpSingleInRegister(tempRegisterContext, CAArch32Assembler::s0, src1);
	m_assembler.Vrecpe_F32(CAArch32Assembler::q1, CAArch32Assembler::q0);
	m_assembler.Vrecps_F32(CAArch32Assembler::q2, CAArch32Assembler::q1, CAArch32Assembler::q0);
	m_assembler.Vmul_F32(CAArch32Assembler::s4, CAArch32Assembler::s4, CAArch32Assembler::s8);
	StoreRegisterInMemoryFpSingle(tempRegisterContext, dst, CAArch32Assembler::s4);
}

void CCodeGen_AArch32::Emit_Fp_Rsqrt_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	CTempRegisterContext tempRegisterContext;

	LoadMemoryFpSingleInRegister(tempRegisterContext, CAArch32Assembler::s0, src1);
	m_assembler.Vrsqrte_F32(CAArch32Assembler::q1, CAArch32Assembler::q0);
	m_assembler.Vmul_F32(CAArch32Assembler::s8, CAArch32Assembler::s0, CAArch32Assembler::s4);
	m_assembler.Vrsqrts_F32(CAArch32Assembler::q3, CAArch32Assembler::q2, CAArch32Assembler::q1);
	m_assembler.Vmul_F32(CAArch32Assembler::s4, CAArch32Assembler::s4, CAArch32Assembler::s12);
	StoreRegisterInMemoryFpSingle(tempRegisterContext, dst, CAArch32Assembler::s4);
}

void CCodeGen_AArch32::Emit_Fp_Cmp_AnyMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	CTempRegisterContext tempRegisterContext;

	auto tmpReg = tempRegisterContext.Allocate();
	auto dstReg = PrepareSymbolRegisterDef(dst, tmpReg);

	m_assembler.Mov(dstReg, CAArch32Assembler::MakeImmediateAluOperand(0, 0));
	LoadMemoryFpSingleInRegister(tempRegisterContext, CAArch32Assembler::s0, src1);
	LoadMemoryFpSingleInRegister(tempRegisterContext, CAArch32Assembler::s1, src2);
	m_assembler.Vcmp_F32(CAArch32Assembler::s0, CAArch32Assembler::s1);
	m_assembler.Vmrs(CAArch32Assembler::rPC);	//Move to general purpose status register
	switch(statement.jmpCondition)
	{
	case Jitter::CONDITION_AB:
		m_assembler.MovCc(CAArch32Assembler::CONDITION_GT, dstReg, CAArch32Assembler::MakeImmediateAluOperand(1, 0));
		break;
	case Jitter::CONDITION_BE:
		m_assembler.MovCc(CAArch32Assembler::CONDITION_LS, dstReg, CAArch32Assembler::MakeImmediateAluOperand(1, 0));
		break;
	case Jitter::CONDITION_BL:
		m_assembler.MovCc(CAArch32Assembler::CONDITION_MI, dstReg, CAArch32Assembler::MakeImmediateAluOperand(1, 0));
		break;
	case Jitter::CONDITION_EQ:
		m_assembler.MovCc(CAArch32Assembler::CONDITION_EQ, dstReg, CAArch32Assembler::MakeImmediateAluOperand(1, 0));
		break;
	default:
		assert(0);
		break;
	}

	CommitSymbolRegister(dst, dstReg);
	tempRegisterContext.Release(tmpReg);
}

void CCodeGen_AArch32::Emit_Fp_Mov_MemSRelI32(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REL_INT32);

	CTempRegisterContext tempRegisterContext;

	auto dstReg = CAArch32Assembler::s0;
	auto src1Reg = CAArch32Assembler::s1;

	m_assembler.Vldr(src1Reg, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(src1->m_valueLow));
	m_assembler.Vcvt_F32_S32(dstReg, src1Reg);
	StoreRegisterInMemoryFpSingle(tempRegisterContext, dst, dstReg);
}

void CCodeGen_AArch32::Emit_Fp_ToIntTrunc_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	CTempRegisterContext tempRegisterContext;

	auto dstReg = CAArch32Assembler::s0;
	auto src1Reg = CAArch32Assembler::s1;

	LoadMemoryFpSingleInRegister(tempRegisterContext, src1Reg, src1);
	m_assembler.Vcvt_S32_F32(dstReg, src1Reg);
	StoreRegisterInMemoryFpSingle(tempRegisterContext, dst, dstReg);
}

void CCodeGen_AArch32::Emit_Fp_LdCst_TmpCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_TMP_SINGLE);
	assert(src1->m_type == SYM_CONSTANT);

	LoadConstantInRegister(CAArch32Assembler::r0, src1->m_valueLow);
	m_assembler.Str(CAArch32Assembler::r0, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel));
}

CCodeGen_AArch32::CONSTMATCHER CCodeGen_AArch32::g_fpuConstMatchers[] = 
{
	{ OP_FP_ADD,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_AArch32::Emit_Fpu_MemMemMem<FPUOP_ADD>		},
	{ OP_FP_SUB,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_AArch32::Emit_Fpu_MemMemMem<FPUOP_SUB>		},
	{ OP_FP_MUL,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_AArch32::Emit_Fpu_MemMemMem<FPUOP_MUL>		},
	{ OP_FP_DIV,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_AArch32::Emit_Fpu_MemMemMem<FPUOP_DIV>		},

	{ OP_FP_CMP,			MATCH_ANY,					MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_AArch32::Emit_Fp_Cmp_AnyMemMem				},

	{ OP_FP_MIN,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_AArch32::Emit_FpuMd_MemMemMem<FPUMDOP_MIN>	},
	{ OP_FP_MAX,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,	&CCodeGen_AArch32::Emit_FpuMd_MemMemMem<FPUMDOP_MAX>	},

	{ OP_FP_RCPL,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_AArch32::Emit_Fp_Rcpl_MemMem					},
	{ OP_FP_SQRT,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_AArch32::Emit_Fpu_MemMem<FPUOP_SQRT>			},
	{ OP_FP_RSQRT,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_AArch32::Emit_Fp_Rsqrt_MemMem					},

	{ OP_FP_ABS,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_AArch32::Emit_Fpu_MemMem<FPUOP_ABS>			},
	{ OP_FP_NEG,			MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_AArch32::Emit_Fpu_MemMem<FPUOP_NEG>			},

	{ OP_MOV,				MATCH_MEMORY_FP_SINGLE,		MATCH_RELATIVE_FP_INT32,	MATCH_NIL,				&CCodeGen_AArch32::Emit_Fp_Mov_MemSRelI32				},
	{ OP_FP_TOINT_TRUNC,	MATCH_MEMORY_FP_SINGLE,		MATCH_MEMORY_FP_SINGLE,		MATCH_NIL,				&CCodeGen_AArch32::Emit_Fp_ToIntTrunc_MemMem			},

	{ OP_FP_LDCST,			MATCH_TEMPORARY_FP_SINGLE,	MATCH_CONSTANT,				MATCH_NIL,				&CCodeGen_AArch32::Emit_Fp_LdCst_TmpCst					},

	{ OP_MOV,				MATCH_NIL,					MATCH_NIL,					MATCH_NIL,				NULL												},
};
