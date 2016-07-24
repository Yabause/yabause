#include "Jitter_CodeGen_AArch64.h"

using namespace Jitter;

uint32 CCodeGen_AArch64::GetMemory64Offset(CSymbol* symbol) const
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		return symbol->m_valueLow;
		break;
	case SYM_TEMPORARY64:
		return symbol->m_stackLocation;
		break;
	default:
		assert(false);
		return 0;
		break;
	}
}

void CCodeGen_AArch64::LoadMemory64InRegister(CAArch64Assembler::REGISTER64 registerId, CSymbol* src)
{
	switch(src->m_type)
	{
	case SYM_RELATIVE64:
		assert((src->m_valueLow & 0x07) == 0x00);
		m_assembler.Ldr(registerId, g_baseRegister, src->m_valueLow);
		break;
	case SYM_TEMPORARY64:
		assert((src->m_stackLocation & 0x07) == 0x00);
		m_assembler.Ldr(registerId, CAArch64Assembler::xSP, src->m_stackLocation);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::StoreRegisterInMemory64(CSymbol* dst, CAArch64Assembler::REGISTER64 registerId)
{
	switch(dst->m_type)
	{
	case SYM_RELATIVE64:
		assert((dst->m_valueLow & 0x07) == 0x00);
		m_assembler.Str(registerId, g_baseRegister, dst->m_valueLow);
		break;
	case SYM_TEMPORARY64:
		assert((dst->m_stackLocation & 0x07) == 0x00);
		m_assembler.Str(registerId, CAArch64Assembler::xSP, dst->m_stackLocation);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::LoadConstant64InRegister(CAArch64Assembler::REGISTER64 registerId, uint64 constant)
{
	if(constant == 0)
	{
		m_assembler.Movz(registerId, 0, 0);
		return;
	}
	//TODO: Check if "movn" could be used to load constants
	bool loaded = false;
	static const uint64 masks[4] =
	{
		0x000000000000FFFFULL,
		0x00000000FFFF0000ULL,
		0x0000FFFF00000000ULL,
		0xFFFF000000000000ULL
	};
	for(unsigned int i = 0; i < 4; i++)
	{
		if((constant & masks[i]) != 0)
		{
			if(loaded)
			{
				m_assembler.Movk(registerId, constant >> (i * 16), i);
			}
			else
			{
				m_assembler.Movz(registerId, constant >> (i * 16), i);
			}
			loaded = true;
		}
	}
	assert(loaded);
}

void CCodeGen_AArch64::LoadMemory64LowInRegister(CAArch64Assembler::REGISTER32 registerId, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		m_assembler.Ldr(registerId, g_baseRegister, symbol->m_valueLow + 0);
		break;
	case SYM_TEMPORARY64:
		m_assembler.Ldr(registerId, CAArch64Assembler::xSP, symbol->m_stackLocation + 0);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch64::LoadMemory64HighInRegister(CAArch64Assembler::REGISTER32 registerId, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		m_assembler.Ldr(registerId, g_baseRegister, symbol->m_valueLow + 4);
		break;
	case SYM_TEMPORARY64:
		m_assembler.Ldr(registerId, CAArch64Assembler::xSP, symbol->m_stackLocation + 4);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch64::LoadSymbol64InRegister(CAArch64Assembler::REGISTER64 registerId, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
	case SYM_TEMPORARY64:
		LoadMemory64InRegister(registerId, symbol);
		break;
	case SYM_CONSTANT64:
		LoadConstant64InRegister(registerId, symbol->GetConstant64());
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch64::StoreRegistersInMemory64(CSymbol* symbol, CAArch64Assembler::REGISTER32 regLo, CAArch64Assembler::REGISTER32 regHi)
{
	if(GetMemory64Offset(symbol) < 0x100)
	{
		switch(symbol->m_type)
		{
		case SYM_RELATIVE64:
			m_assembler.Stp(regLo, regHi, g_baseRegister, symbol->m_valueLow);
			break;
		case SYM_TEMPORARY64:
			m_assembler.Stp(regLo, regHi, CAArch64Assembler::xSP, symbol->m_stackLocation);
			break;
		default:
			assert(false);
			break;
		}
	}
	else
	{
		switch(symbol->m_type)
		{
		case SYM_RELATIVE64:
			m_assembler.Str(regLo, g_baseRegister, symbol->m_valueLow + 0);
			m_assembler.Str(regHi, g_baseRegister, symbol->m_valueLow + 4);
			break;
		case SYM_TEMPORARY64:
			m_assembler.Str(regLo, CAArch64Assembler::xSP, symbol->m_stackLocation + 0);
			m_assembler.Str(regHi, CAArch64Assembler::xSP, symbol->m_stackLocation + 4);
			break;
		default:
			assert(false);
			break;
		}
	}
}

void CCodeGen_AArch64::Emit_ExtLow64VarMem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	LoadMemory64LowInRegister(dstReg, src1);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_ExtHigh64VarMem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	LoadMemory64HighInRegister(dstReg, src1);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_MergeTo64_Mem64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	auto regHi = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

	StoreRegistersInMemory64(dst, regLo, regHi);
}

void CCodeGen_AArch64::Emit_Add64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = GetNextTempRegister64();
	auto src1Reg = GetNextTempRegister64();
	auto src2Reg = GetNextTempRegister64();
	
	LoadMemory64InRegister(src1Reg, src1);
	LoadMemory64InRegister(src2Reg, src2);
	m_assembler.Add(dstReg, src1Reg, src2Reg);
	StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Add64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = GetNextTempRegister64();
	auto src1Reg = GetNextTempRegister64();

	LoadMemory64InRegister(src1Reg, src1);
	auto constant = src2->GetConstant64();

	ADDSUB_IMM_PARAMS addSubImmParams;
	if(TryGetAddSub64ImmParams(constant, addSubImmParams))
	{
		m_assembler.Add(dstReg, src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else if(TryGetAddSub64ImmParams(-static_cast<int64>(constant), addSubImmParams))
	{
		m_assembler.Sub(dstReg, src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else
	{
		auto src2Reg = GetNextTempRegister64();
		LoadConstant64InRegister(src2Reg, constant);
		m_assembler.Add(dstReg, src1Reg, src2Reg);
	}

	StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Sub64_MemAnyMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = GetNextTempRegister64();
	auto src1Reg = GetNextTempRegister64();
	auto src2Reg = GetNextTempRegister64();
	
	LoadSymbol64InRegister(src1Reg, src1);
	LoadMemory64InRegister(src2Reg, src2);
	m_assembler.Sub(dstReg, src1Reg, src2Reg);
	StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Sub64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = GetNextTempRegister64();
	auto src1Reg = GetNextTempRegister64();

	LoadMemory64InRegister(src1Reg, src1);
	auto constant = src2->GetConstant64();

	ADDSUB_IMM_PARAMS addSubImmParams;
	if(TryGetAddSub64ImmParams(constant, addSubImmParams))
	{
		m_assembler.Sub(dstReg, src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else if(TryGetAddSub64ImmParams(-static_cast<int64>(constant), addSubImmParams))
	{
		m_assembler.Add(dstReg, src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else
	{
		auto src2Reg = GetNextTempRegister64();
		LoadConstant64InRegister(src2Reg, constant);
		m_assembler.Sub(dstReg, src1Reg, src2Reg);
	}

	StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Cmp64_VarAnyMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = GetNextTempRegister64();
	auto src2Reg = GetNextTempRegister64();
	
	LoadMemory64InRegister(src1Reg, src1);
	LoadMemory64InRegister(src2Reg, src2);
	m_assembler.Cmp(src1Reg, src2Reg);
	Cmp_GetFlag(dstReg, statement.jmpCondition);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Cmp64_VarMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src2->m_type == SYM_CONSTANT64);
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = GetNextTempRegister64();
	
	LoadMemory64InRegister(src1Reg, src1);
	uint64 src2Cst = src2->GetConstant64();
	
	ADDSUB_IMM_PARAMS addSubImmParams;
	if(TryGetAddSub64ImmParams(src2Cst, addSubImmParams))
	{
		m_assembler.Cmp(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else if(TryGetAddSub64ImmParams(-static_cast<int64>(src2Cst), addSubImmParams))
	{
		m_assembler.Cmn(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else
	{
		auto src2Reg = GetNextTempRegister64();
		LoadConstant64InRegister(src2Reg, src2Cst);
		m_assembler.Cmp(src1Reg, src2Reg);
	}

	Cmp_GetFlag(dstReg, statement.jmpCondition);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_And64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = GetNextTempRegister64();
	auto src1Reg = GetNextTempRegister64();
	auto src2Reg = GetNextTempRegister64();
	
	LoadMemory64InRegister(src1Reg, src1);
	LoadMemory64InRegister(src2Reg, src2);
	m_assembler.And(dstReg, src1Reg, src2Reg);
	StoreRegisterInMemory64(dst, dstReg);
}

template <typename Shift64Op>
void CCodeGen_AArch64::Emit_Shift64_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = GetNextTempRegister64();
	auto src1Reg = GetNextTempRegister64();
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	
	LoadMemory64InRegister(src1Reg, src1);
	((m_assembler).*(Shift64Op::OpReg()))(dstReg, src1Reg, static_cast<CAArch64Assembler::REGISTER64>(src2Reg));
	StoreRegisterInMemory64(dst, dstReg);
}

template <typename Shift64Op>
void CCodeGen_AArch64::Emit_Shift64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto dstReg = GetNextTempRegister64();
	auto src1Reg = GetNextTempRegister64();
	
	LoadMemory64InRegister(src1Reg, src1);
	((m_assembler).*(Shift64Op::OpImm()))(dstReg, src1Reg, src2->m_valueLow);
	StoreRegisterInMemory64(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Mov_Mem64Mem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto tmpReg = GetNextTempRegister64();
	LoadMemory64InRegister(tmpReg, src1);
	StoreRegisterInMemory64(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_Mov_Mem64Cst64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto tmpReg = GetNextTempRegister64();
	LoadConstant64InRegister(tmpReg, src1->GetConstant64());
	StoreRegisterInMemory64(dst, tmpReg);
}

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_64ConstMatchers[] =
{
	{ OP_EXTLOW64,       MATCH_VARIABLE,       MATCH_MEMORY64,       MATCH_NIL,           &CCodeGen_AArch64::Emit_ExtLow64VarMem64                    },
	{ OP_EXTHIGH64,      MATCH_VARIABLE,       MATCH_MEMORY64,       MATCH_NIL,           &CCodeGen_AArch64::Emit_ExtHigh64VarMem64                   },

	{ OP_MERGETO64,      MATCH_MEMORY64,       MATCH_ANY,            MATCH_ANY,           &CCodeGen_AArch64::Emit_MergeTo64_Mem64AnyAny               },

	{ OP_ADD64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_MEMORY64,      &CCodeGen_AArch64::Emit_Add64_MemMemMem                     },
	{ OP_ADD64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT64,    &CCodeGen_AArch64::Emit_Add64_MemMemCst                     },
	
	{ OP_SUB64,          MATCH_MEMORY64,       MATCH_ANY,            MATCH_MEMORY64,      &CCodeGen_AArch64::Emit_Sub64_MemAnyMem                     },
	{ OP_SUB64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT64,    &CCodeGen_AArch64::Emit_Sub64_MemMemCst                     },

	{ OP_CMP64,          MATCH_VARIABLE,       MATCH_ANY,            MATCH_MEMORY64,      &CCodeGen_AArch64::Emit_Cmp64_VarAnyMem                     },
	{ OP_CMP64,          MATCH_VARIABLE,       MATCH_ANY,            MATCH_CONSTANT64,    &CCodeGen_AArch64::Emit_Cmp64_VarMemCst                     },
	
	{ OP_AND64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_MEMORY64,      &CCodeGen_AArch64::Emit_And64_MemMemMem                     },
	
	{ OP_SLL64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_Shift64_MemMemVar<SHIFT64OP_LSL>    },
	{ OP_SRL64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_Shift64_MemMemVar<SHIFT64OP_LSR>    },
	{ OP_SRA64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_Shift64_MemMemVar<SHIFT64OP_ASR>    },

	{ OP_SLL64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Shift64_MemMemCst<SHIFT64OP_LSL>    },
	{ OP_SRL64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Shift64_MemMemCst<SHIFT64OP_LSR>    },
	{ OP_SRA64,          MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Shift64_MemMemCst<SHIFT64OP_ASR>    },
	
	{ OP_MOV,            MATCH_MEMORY64,       MATCH_MEMORY64,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Mov_Mem64Mem64                      },
	{ OP_MOV,            MATCH_MEMORY64,       MATCH_CONSTANT64,     MATCH_NIL,           &CCodeGen_AArch64::Emit_Mov_Mem64Cst64                      },
};
