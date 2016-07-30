#include "Jitter_CodeGen_AArch64.h"

using namespace Jitter;

void CCodeGen_AArch64::LoadMemory128InRegister(CAArch64Assembler::REGISTERMD dstReg, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		m_assembler.Ldr_1q(dstReg, g_baseRegister, symbol->m_valueLow);
		break;
	case SYM_TEMPORARY128:
		m_assembler.Ldr_1q(dstReg, CAArch64Assembler::xSP, symbol->m_stackLocation);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::StoreRegisterInMemory128(CSymbol* symbol, CAArch64Assembler::REGISTERMD srcReg)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		m_assembler.Str_1q(srcReg, g_baseRegister, symbol->m_valueLow);
		break;
	case SYM_TEMPORARY128:
		m_assembler.Str_1q(srcReg, CAArch64Assembler::xSP, symbol->m_stackLocation);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::LoadMemory128AddressInRegister(CAArch64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		LoadRelative128AddressInRegister(dstReg, symbol, offset);
		break;
	case SYM_TEMPORARY128:
		LoadTemporary128AddressInRegister(dstReg, symbol, offset);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::LoadRelative128AddressInRegister(CAArch64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_RELATIVE128);

	uint32 totalOffset = symbol->m_valueLow + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Add(dstReg, g_baseRegister, totalOffset, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
}

void CCodeGen_AArch64::LoadTemporary128AddressInRegister(CAArch64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_TEMPORARY128);

	uint32 totalOffset = symbol->m_stackLocation + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Add(dstReg, CAArch64Assembler::xSP, totalOffset, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
}

void CCodeGen_AArch64::LoadTemporary256ElementAddressInRegister(CAArch64Assembler::REGISTER64 dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_TEMPORARY256);

	uint32 totalOffset = symbol->m_stackLocation + offset;
	assert(totalOffset < 0x1000);
	m_assembler.Add(dstReg, CAArch64Assembler::xSP, totalOffset, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
}

CAArch64Assembler::REGISTERMD CCodeGen_AArch64::PrepareSymbolRegisterDefMd(CSymbol* symbol, CAArch64Assembler::REGISTERMD preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(symbol->m_valueLow < MAX_MDREGISTERS);
		return g_registersMd[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch64Assembler::REGISTERMD CCodeGen_AArch64::PrepareSymbolRegisterUseMd(CSymbol* symbol, CAArch64Assembler::REGISTERMD preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(symbol->m_valueLow < MAX_MDREGISTERS);
		return g_registersMd[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		LoadMemory128InRegister(preferedRegister, symbol);
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_AArch64::CommitSymbolRegisterMd(CSymbol* symbol, CAArch64Assembler::REGISTERMD usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER128:
		assert(usedRegister == g_registersMd[symbol->m_valueLow]);
		break;
	case SYM_TEMPORARY128:
	case SYM_RELATIVE128:
		StoreRegisterInMemory128(symbol, usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());

	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());

	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_VarVarVarRev(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());

	((m_assembler).*(MDOP::OpReg()))(dstReg, src2Reg, src1Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

template <typename MDSHIFTOP>
void CCodeGen_AArch64::Emit_Md_Shift_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());

	((m_assembler).*(MDSHIFTOP::OpReg()))(dstReg, src1Reg, src2->m_valueLow);

	CommitSymbolRegisterMd(dst, dstReg);
}

template <typename MDOP>
void CCodeGen_AArch64::Emit_Md_Test_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto tmpValueReg = GetNextTempRegister();
	auto tmpCmpReg = GetNextTempRegisterMd();
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());

	((m_assembler).*(MDOP::OpReg()))(tmpCmpReg, src1Reg);
	
	m_assembler.Eor(dstReg, dstReg, dstReg);
	for(unsigned int i = 0; i < 4; i++)
	{
		LOGICAL_IMM_PARAMS logicalImmParams;
		bool result = TryGetLogicalImmParams((1 << i), logicalImmParams);
		assert(result);

		m_assembler.Umov_1s(tmpValueReg, tmpCmpReg, 3 - i);
		m_assembler.And(tmpValueReg, tmpValueReg,
			logicalImmParams.n, logicalImmParams.immr, logicalImmParams.imms);
		m_assembler.Orr(dstReg, dstReg, tmpValueReg);
	}

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_Mov_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	assert(!dst->Equals(src1));
	
	m_assembler.Mov(g_registersMd[dst->m_valueLow], g_registersMd[src1->m_valueLow]);
}

void CCodeGen_AArch64::Emit_Md_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	LoadMemory128InRegister(g_registersMd[dst->m_valueLow], src1);
}

void CCodeGen_AArch64::Emit_Md_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	StoreRegisterInMemory128(dst, g_registersMd[src1->m_valueLow]);
}

void CCodeGen_AArch64::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto tmpReg = GetNextTempRegisterMd();
	
	LoadMemory128InRegister(tmpReg, src1);
	StoreRegisterInMemory128(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_Md_Not_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto zeroReg = GetNextTempRegisterMd();
	
	m_assembler.Eor_16b(zeroReg, zeroReg, zeroReg);
	m_assembler.Orn_16b(dstReg, zeroReg, src1Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_LoadFromRef_VarMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto src1AddrReg = GetNextTempRegister64();
	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());

	LoadMemoryReferenceInRegister(src1AddrReg, src1);

	m_assembler.Ldr_1q(dstReg, src1AddrReg, 0);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_StoreAtRef_MemVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto src1AddrReg = GetNextTempRegister64();
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());

	LoadMemoryReferenceInRegister(src1AddrReg, src1);
	
	m_assembler.Str_1q(src2Reg, src1AddrReg, 0);
}

void CCodeGen_AArch64::Emit_Md_MovMasked_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->Equals(src1));

	auto mask = static_cast<uint8>(statement.jmpCondition);

	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());
	
	for(unsigned int i = 0; i < 4; i++)
	{
		if(mask & (1 << i))
		{
			m_assembler.Ins_1s(src1Reg, i, src2Reg, i);
		}
	}

	//This is only valid if dst == src1
	CommitSymbolRegisterMd(dst, src1Reg);
}

void CCodeGen_AArch64::Emit_Md_Expand_VarReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());

	m_assembler.Dup_4s(dstReg, g_registers[src1->m_valueLow]);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_Expand_VarMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = GetNextTempRegister();

	LoadMemoryInRegister(src1Reg, src1);

	m_assembler.Dup_4s(dstReg, src1Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_Expand_VarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = GetNextTempRegister();

	LoadConstantInRegister(src1Reg, src1->m_valueLow);

	m_assembler.Dup_4s(dstReg, src1Reg);

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_PackHB_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());
	
	if(dstReg == src1Reg)
	{
		auto tmpReg = GetNextTempRegisterMd();
		m_assembler.Xtn1_8b(tmpReg, src2Reg);
		m_assembler.Xtn2_16b(tmpReg, src1Reg);
		m_assembler.Mov(dstReg, tmpReg);
	}
	else
	{
		m_assembler.Xtn1_8b(dstReg, src2Reg);
		m_assembler.Xtn2_16b(dstReg, src1Reg);
	}
	
	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_PackWH_VarVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());

	if(dstReg == src1Reg)
	{
		auto tmpReg = GetNextTempRegisterMd();
		m_assembler.Xtn1_4h(tmpReg, src2Reg);
		m_assembler.Xtn2_8h(tmpReg, src1Reg);
		m_assembler.Mov(dstReg, tmpReg);
	}
	else
	{
		m_assembler.Xtn1_4h(dstReg, src2Reg);
		m_assembler.Xtn2_8h(dstReg, src1Reg);
	}

	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_MergeTo256_MemVarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY256);

	auto dstLoAddrReg = GetNextTempRegister64();
	auto dstHiAddrReg = GetNextTempRegister64();
	auto src1Reg = PrepareSymbolRegisterUseMd(src1, GetNextTempRegisterMd());
	auto src2Reg = PrepareSymbolRegisterUseMd(src2, GetNextTempRegisterMd());

	LoadTemporary256ElementAddressInRegister(dstLoAddrReg, dst, 0x00);
	LoadTemporary256ElementAddressInRegister(dstHiAddrReg, dst, 0x10);

	m_assembler.St1_4s(src1Reg, dstLoAddrReg);
	m_assembler.St1_4s(src2Reg, dstHiAddrReg);
}

void CCodeGen_AArch64::Emit_Md_Srl256_VarMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY256);
	assert(src2->m_type == SYM_CONSTANT);

	auto src1AddrReg = GetNextTempRegister64();
	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());

	uint32 offset = (src2->m_valueLow & 0x7F) / 8;
	LoadTemporary256ElementAddressInRegister(src1AddrReg, src1, offset);

	m_assembler.Ld1_4s(dstReg, src1AddrReg);
	
	CommitSymbolRegisterMd(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Md_Srl256_VarMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY256);

	auto offsetRegister = GetNextTempRegister();
	auto src1AddrReg = GetNextTempRegister64();
	auto src2Register = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

	auto dstReg = PrepareSymbolRegisterDefMd(dst, GetNextTempRegisterMd());

	LoadTemporary256ElementAddressInRegister(src1AddrReg, src1, 0);

	//Compute offset and modify address
	LOGICAL_IMM_PARAMS logicalImmParams;
	bool result = TryGetLogicalImmParams(0x7F, logicalImmParams);
	assert(result);
	m_assembler.And(offsetRegister, src2Register, logicalImmParams.n, logicalImmParams.immr, logicalImmParams.imms);
	m_assembler.Lsr(offsetRegister, offsetRegister, 3);
	m_assembler.Add(src1AddrReg, src1AddrReg, static_cast<CAArch64Assembler::REGISTER64>(offsetRegister));

	m_assembler.Ld1_4s(dstReg, src1AddrReg);
	
	CommitSymbolRegisterMd(dst, dstReg);
}

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_mdConstMatchers[] =
{
	{ OP_MD_ADD_B,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDB>                  },
	{ OP_MD_ADD_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDH>                  },
	{ OP_MD_ADD_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDW>                  },

	{ OP_MD_ADDUS_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDBUS>                },
	{ OP_MD_ADDUS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDHUS>                },
	{ OP_MD_ADDUS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDWUS>                },

	{ OP_MD_ADDSS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDHSS>                },
	{ OP_MD_ADDSS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDWSS>                },
	
	{ OP_MD_SUB_B,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBB>                  },
	{ OP_MD_SUB_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBH>                  },
	{ OP_MD_SUB_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBW>                  },
	
	{ OP_MD_SUBUS_B,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBBUS>                },
	{ OP_MD_SUBUS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBHUS>                },

	{ OP_MD_SUBSS_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBHSS>                },
	{ OP_MD_SUBSS_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBWSS>                },

	{ OP_MD_CMPEQ_W,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_CMPEQW>                },

	{ OP_MD_CMPGT_H,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_CMPGTH>                },

	{ OP_MD_MIN_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MINH>                  },
	{ OP_MD_MIN_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MINW>                  },

	{ OP_MD_MAX_H,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MAXH>                  },
	{ OP_MD_MAX_W,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MAXW>                  },

	{ OP_MD_ADD_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_ADDS>                  },
	{ OP_MD_SUB_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_SUBS>                  },
	{ OP_MD_MUL_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MULS>                  },
	{ OP_MD_DIV_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_DIVS>                  },

	{ OP_MD_ABS_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_VarVar<MDOP_ABSS>                     },
	{ OP_MD_MIN_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MINS>                  },
	{ OP_MD_MAX_S,              MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_MAXS>                  },

	{ OP_MD_AND,                MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_AND>                   },
	{ OP_MD_OR,                 MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_OR>                    },
	{ OP_MD_XOR,                MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVar<MDOP_XOR>                   },
	
	{ OP_MD_NOT,                MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_Not_VarVar                            },
	
	{ OP_MD_SLLH,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SLLH>            },
	{ OP_MD_SLLW,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SLLW>            },

	{ OP_MD_SRLH,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SRLH>            },
	{ OP_MD_SRLW,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SRLW>            },

	{ OP_MD_SRAH,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SRAH>            },
	{ OP_MD_SRAW,               MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_CONSTANT,         &CCodeGen_AArch64::Emit_Md_Shift_VarVarCst<MDOP_SRAW>            },
	
	{ OP_MD_SRL256,             MATCH_VARIABLE128,    MATCH_MEMORY256,      MATCH_VARIABLE,         &CCodeGen_AArch64::Emit_Md_Srl256_VarMemVar                      },
	{ OP_MD_SRL256,             MATCH_VARIABLE128,    MATCH_MEMORY256,      MATCH_CONSTANT,         &CCodeGen_AArch64::Emit_Md_Srl256_VarMemCst                      },

	{ OP_MD_ISNEGATIVE,         MATCH_VARIABLE,       MATCH_VARIABLE128,    MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_Test_VarVar<MDOP_CMPLTZS>             },
	{ OP_MD_ISZERO,             MATCH_VARIABLE,       MATCH_VARIABLE128,    MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_Test_VarVar<MDOP_CMPEQZS>             },
	
	{ OP_MD_TOSINGLE,           MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_VarVar<MDOP_TOSINGLE>                 },
	{ OP_MD_TOWORD_TRUNCATE,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_VarVar<MDOP_TOWORD>                   },

	{ OP_LOADFROMREF,           MATCH_VARIABLE128,    MATCH_MEM_REF,        MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_LoadFromRef_VarMem                    },
	{ OP_STOREATREF,            MATCH_NIL,            MATCH_MEM_REF,        MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_StoreAtRef_MemVar                     },

	{ OP_MD_MOV_MASKED,         MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_MovMasked_VarVarVar                   },

	{ OP_MD_EXPAND,             MATCH_VARIABLE128,    MATCH_REGISTER,       MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_Expand_VarReg                         },
	{ OP_MD_EXPAND,             MATCH_VARIABLE128,    MATCH_MEMORY,         MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_Expand_VarMem                         },
	{ OP_MD_EXPAND,             MATCH_VARIABLE128,    MATCH_CONSTANT,       MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_Expand_VarCst                         },

	{ OP_MD_PACK_HB,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_PackHB_VarVarVar                      },
	{ OP_MD_PACK_WH,            MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_PackWH_VarVarVar                      },

	{ OP_MD_UNPACK_LOWER_BH,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_LOWER_BH>    },
	{ OP_MD_UNPACK_LOWER_HW,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_LOWER_HW>    },
	{ OP_MD_UNPACK_LOWER_WD,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_LOWER_WD>    },
	
	{ OP_MD_UNPACK_UPPER_BH,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_UPPER_BH>    },
	{ OP_MD_UNPACK_UPPER_HW,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_UPPER_HW>    },
	{ OP_MD_UNPACK_UPPER_WD,    MATCH_VARIABLE128,    MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_Md_VarVarVarRev<MDOP_UNPACK_UPPER_WD>    },

	{ OP_MOV,                   MATCH_REGISTER128,    MATCH_REGISTER128,    MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_Mov_RegReg                            },
	{ OP_MOV,                   MATCH_REGISTER128,    MATCH_MEMORY128,      MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_Mov_RegMem                            },
	{ OP_MOV,                   MATCH_MEMORY128,      MATCH_REGISTER128,    MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_Mov_MemReg                            },
	{ OP_MOV,                   MATCH_MEMORY128,      MATCH_MEMORY128,      MATCH_NIL,              &CCodeGen_AArch64::Emit_Md_Mov_MemMem                            },
	
	{ OP_MERGETO256,            MATCH_MEMORY256,      MATCH_VARIABLE128,    MATCH_VARIABLE128,      &CCodeGen_AArch64::Emit_MergeTo256_MemVarVar                     },
	
	{ OP_MOV,                   MATCH_NIL,            MATCH_NIL,            MATCH_NIL,              nullptr                                                          },
};
