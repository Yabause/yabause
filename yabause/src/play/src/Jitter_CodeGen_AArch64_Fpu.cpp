#include "Jitter_CodeGen_AArch64.h"

using namespace Jitter;

void CCodeGen_AArch64::LoadMemoryFpSingleInRegister(CAArch64Assembler::REGISTERMD reg, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REL_SINGLE:
		m_assembler.Ldr_1s(reg, g_baseRegister, symbol->m_valueLow);
		break;
	case SYM_FP_TMP_SINGLE:
		m_assembler.Ldr_1s(reg, CAArch64Assembler::xSP, symbol->m_stackLocation);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch64::StoreRegisterInMemoryFpSingle(CSymbol* symbol, CAArch64Assembler::REGISTERMD reg)
{
	switch(symbol->m_type)
	{
	case SYM_FP_REL_SINGLE:
		m_assembler.Str_1s(reg, g_baseRegister, symbol->m_valueLow);
		break;
	case SYM_FP_TMP_SINGLE:
		m_assembler.Str_1s(reg, CAArch64Assembler::xSP, symbol->m_stackLocation);
		break;
	default:
		assert(false);
		break;
	}
}

template <typename FPUOP>
void CCodeGen_AArch64::Emit_Fpu_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	
	LoadMemoryFpSingleInRegister(src1Reg, src1);
	((m_assembler).*(FPUOP::OpReg()))(dstReg, src1Reg);
	StoreRegisterInMemoryFpSingle(dst, dstReg);
}

template <typename FPUOP>
void CCodeGen_AArch64::Emit_Fpu_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	auto src2Reg = GetNextTempRegisterMd();
	
	LoadMemoryFpSingleInRegister(src1Reg, src1);
	LoadMemoryFpSingleInRegister(src2Reg, src2);
	((m_assembler).*(FPUOP::OpReg()))(dstReg, src1Reg, src2Reg);
	StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Cmp_AnyMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = GetNextTempRegisterMd();
	auto src2Reg = GetNextTempRegisterMd();

	LoadMemoryFpSingleInRegister(src1Reg, src1);
	LoadMemoryFpSingleInRegister(src2Reg, src2);
	m_assembler.Fcmp_1s(src1Reg, src2Reg);
	Cmp_GetFlag(dstReg, statement.jmpCondition);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Rcpl_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	auto oneReg = GetNextTempRegisterMd();
	
	m_assembler.Fmov_1s(oneReg, 0x70);	//Loads 1.0f
	LoadMemoryFpSingleInRegister(src1Reg, src1);
	m_assembler.Fdiv_1s(dstReg, oneReg, src1Reg);
	StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Rsqrt_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();
	auto oneReg = GetNextTempRegisterMd();
	
	m_assembler.Fmov_1s(oneReg, 0x70);	//Loads 1.0f
	LoadMemoryFpSingleInRegister(src1Reg, src1);
	m_assembler.Fsqrt_1s(src1Reg, src1Reg);
	m_assembler.Fdiv_1s(dstReg, oneReg, src1Reg);
	StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_Mov_MemSRelI32(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REL_INT32);

	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();

	m_assembler.Ldr_1s(src1Reg, g_baseRegister, src1->m_valueLow);
	m_assembler.Scvtf_1s(dstReg, src1Reg);
	StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_ToIntTrunc_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = GetNextTempRegisterMd();
	auto src1Reg = GetNextTempRegisterMd();

	LoadMemoryFpSingleInRegister(src1Reg, src1);
	m_assembler.Fcvtzs_1s(dstReg, src1Reg);
	StoreRegisterInMemoryFpSingle(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Fp_LdCst_TmpCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_FP_TMP_SINGLE);
	assert(src1->m_type == SYM_CONSTANT);

	auto tmpReg = GetNextTempRegister();
	
	LoadConstantInRegister(tmpReg, src1->m_valueLow);
	m_assembler.Str(tmpReg, CAArch64Assembler::xSP, dst->m_stackLocation);
}

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_fpuConstMatchers[] =
{
	{ OP_FP_ADD,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_ADD>    },
	{ OP_FP_SUB,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_SUB>    },
	{ OP_FP_MUL,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_MUL>    },
	{ OP_FP_DIV,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_DIV>    },

	{ OP_FP_CMP,            MATCH_ANY,                    MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fp_Cmp_AnyMemMem            },

	{ OP_FP_MIN,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_MIN>    },
	{ OP_FP_MAX,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_MEMORY_FP_SINGLE,    &CCodeGen_AArch64::Emit_Fpu_MemMemMem<FPUOP_MAX>    },

	{ OP_FP_RCPL,           MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 &CCodeGen_AArch64::Emit_Fp_Rcpl_MemMem              },
	{ OP_FP_SQRT,           MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 &CCodeGen_AArch64::Emit_Fpu_MemMem<FPUOP_SQRT>      },
	{ OP_FP_RSQRT,          MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 &CCodeGen_AArch64::Emit_Fp_Rsqrt_MemMem             },

	{ OP_FP_ABS,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 &CCodeGen_AArch64::Emit_Fpu_MemMem<FPUOP_ABS>       },
	{ OP_FP_NEG,            MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 &CCodeGen_AArch64::Emit_Fpu_MemMem<FPUOP_NEG>       },

	{ OP_MOV,               MATCH_MEMORY_FP_SINGLE,       MATCH_RELATIVE_FP_INT32,    MATCH_NIL,                 &CCodeGen_AArch64::Emit_Fp_Mov_MemSRelI32           },
	{ OP_FP_TOINT_TRUNC,    MATCH_MEMORY_FP_SINGLE,       MATCH_MEMORY_FP_SINGLE,     MATCH_NIL,                 &CCodeGen_AArch64::Emit_Fp_ToIntTrunc_MemMem        },

	{ OP_FP_LDCST,          MATCH_TEMPORARY_FP_SINGLE,    MATCH_CONSTANT,             MATCH_NIL,                 &CCodeGen_AArch64::Emit_Fp_LdCst_TmpCst             },

	{ OP_MOV,               MATCH_NIL,                    MATCH_NIL,                  MATCH_NIL,                 nullptr                                             },
};
