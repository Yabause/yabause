#include <algorithm>
#include "Jitter_CodeGen_AArch64.h"
#include "BitManip.h"

using namespace Jitter;

CAArch64Assembler::REGISTER32    CCodeGen_AArch64::g_registers[MAX_REGISTERS] =
{
	CAArch64Assembler::w20,
	CAArch64Assembler::w21,
	CAArch64Assembler::w22,
	CAArch64Assembler::w23,
	CAArch64Assembler::w24,
	CAArch64Assembler::w25,
	CAArch64Assembler::w26,
	CAArch64Assembler::w27,
	CAArch64Assembler::w28,
};

CAArch64Assembler::REGISTERMD    CCodeGen_AArch64::g_registersMd[MAX_MDREGISTERS] =
{
	CAArch64Assembler::v4,  CAArch64Assembler::v5,  CAArch64Assembler::v6,  CAArch64Assembler::v7,
	CAArch64Assembler::v8,  CAArch64Assembler::v9,  CAArch64Assembler::v10, CAArch64Assembler::v11,
	CAArch64Assembler::v12, CAArch64Assembler::v13, CAArch64Assembler::v14, CAArch64Assembler::v15,
	CAArch64Assembler::v16, CAArch64Assembler::v17, CAArch64Assembler::v18, CAArch64Assembler::v19,
	CAArch64Assembler::v20, CAArch64Assembler::v21, CAArch64Assembler::v22, CAArch64Assembler::v23,
	CAArch64Assembler::v24, CAArch64Assembler::v25, CAArch64Assembler::v26, CAArch64Assembler::v27,
};

CAArch64Assembler::REGISTER32    CCodeGen_AArch64::g_tempRegisters[MAX_TEMP_REGS] =
{
	CAArch64Assembler::w9,
	CAArch64Assembler::w10,
	CAArch64Assembler::w11,
	CAArch64Assembler::w12,
	CAArch64Assembler::w13,
	CAArch64Assembler::w14,
	CAArch64Assembler::w15
};

CAArch64Assembler::REGISTER64    CCodeGen_AArch64::g_tempRegisters64[MAX_TEMP_REGS] =
{
	CAArch64Assembler::x9,
	CAArch64Assembler::x10,
	CAArch64Assembler::x11,
	CAArch64Assembler::x12,
	CAArch64Assembler::x13,
	CAArch64Assembler::x14,
	CAArch64Assembler::x15
};

CAArch64Assembler::REGISTERMD    CCodeGen_AArch64::g_tempRegistersMd[MAX_TEMP_MD_REGS] =
{
	CAArch64Assembler::v0,
	CAArch64Assembler::v1,
	CAArch64Assembler::v2,
	CAArch64Assembler::v3,
};

CAArch64Assembler::REGISTER32    CCodeGen_AArch64::g_paramRegisters[MAX_PARAM_REGS] =
{
	CAArch64Assembler::w0,
	CAArch64Assembler::w1,
	CAArch64Assembler::w2,
	CAArch64Assembler::w3,
	CAArch64Assembler::w4,
	CAArch64Assembler::w5,
	CAArch64Assembler::w6,
	CAArch64Assembler::w7,
};

CAArch64Assembler::REGISTER64    CCodeGen_AArch64::g_paramRegisters64[MAX_PARAM_REGS] =
{
	CAArch64Assembler::x0,
	CAArch64Assembler::x1,
	CAArch64Assembler::x2,
	CAArch64Assembler::x3,
	CAArch64Assembler::x4,
	CAArch64Assembler::x5,
	CAArch64Assembler::x6,
	CAArch64Assembler::x7,
};

CAArch64Assembler::REGISTER64    CCodeGen_AArch64::g_baseRegister = CAArch64Assembler::x19;

static bool isMask(uint32 value)
{
	return value && (((value + 1) & value) == 0);
}

static bool isShiftedMask(uint32 value)
{
	return value && isMask((value - 1) | value);
}

template <typename AddSubOp>
void CCodeGen_AArch64::Emit_AddSub_VarAnyVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	((m_assembler).*(AddSubOp::OpReg()))(dstReg, src1Reg, src2Reg);
	CommitSymbolRegister(dst, dstReg);
}

template <typename AddSubOp>
void CCodeGen_AArch64::Emit_AddSub_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src2->m_type == SYM_CONSTANT);
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	
	ADDSUB_IMM_PARAMS addSubImmParams;
	if(TryGetAddSubImmParams(src2->m_valueLow, addSubImmParams))
	{
		((m_assembler).*(AddSubOp::OpImm()))(dstReg, src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else if(TryGetAddSubImmParams(-static_cast<int32>(src2->m_valueLow), addSubImmParams))
	{
		((m_assembler).*(AddSubOp::OpImmRev()))(dstReg, src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else
	{
		auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
		((m_assembler).*(AddSubOp::OpReg()))(dstReg, src1Reg, src2Reg);
	}

	CommitSymbolRegister(dst, dstReg);
}

template <typename ShiftOp>
void CCodeGen_AArch64::Emit_Shift_VarAnyVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	((m_assembler).*(ShiftOp::OpReg()))(dstReg, src1Reg, src2Reg);
	CommitSymbolRegister(dst, dstReg);
}

template <typename ShiftOp>
void CCodeGen_AArch64::Emit_Shift_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	((m_assembler).*(ShiftOp::OpImm()))(dstReg, src1Reg, src2->m_valueLow);
	CommitSymbolRegister(dst, dstReg);
}

template <typename LogicOp>
void CCodeGen_AArch64::Emit_Logic_VarAnyVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	((m_assembler).*(LogicOp::OpReg()))(dstReg, src1Reg, src2Reg);
	CommitSymbolRegister(dst, dstReg);
}

template <typename LogicOp>
void CCodeGen_AArch64::Emit_Logic_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src2->m_type == SYM_CONSTANT);
	assert(src2->m_valueLow != 0);

	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	
	LOGICAL_IMM_PARAMS logicalImmParams;
	if(TryGetLogicalImmParams(src2->m_valueLow, logicalImmParams))
	{
		((m_assembler).*(LogicOp::OpImm()))(dstReg, src1Reg,
			logicalImmParams.n, logicalImmParams.immr, logicalImmParams.imms);
	}
	else
	{
		auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
		((m_assembler).*(LogicOp::OpReg()))(dstReg, src1Reg, src2Reg);
	}
	CommitSymbolRegister(dst, dstReg);
}

template <bool isSigned>
void CCodeGen_AArch64::Emit_Mul_Tmp64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(dst->m_type == SYM_TEMPORARY64);

	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	auto dstReg = GetNextTempRegister64();
	
	if(isSigned)
	{
		m_assembler.Smull(dstReg, src1Reg, src2Reg);
	}
	else
	{
		m_assembler.Umull(dstReg, src1Reg, src2Reg);
	}
	
	m_assembler.Str(dstReg, CAArch64Assembler::xSP, dst->m_stackLocation);
}

template <bool isSigned>
void CCodeGen_AArch64::Emit_Div_Tmp64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY64);

	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	auto resReg = GetNextTempRegister();
	auto modReg = GetNextTempRegister();

	if(isSigned)
	{
		m_assembler.Sdiv(resReg, src1Reg, src2Reg);
	}
	else
	{
		m_assembler.Udiv(resReg, src1Reg, src2Reg);
	}

	m_assembler.Msub(modReg, resReg, src2Reg, src1Reg);

	m_assembler.Str(resReg, CAArch64Assembler::xSP, dst->m_stackLocation + 0);
	m_assembler.Str(modReg, CAArch64Assembler::xSP, dst->m_stackLocation + 4);
}

#define LOGIC_CONST_MATCHERS(LOGICOP_CST, LOGICOP) \
	{ LOGICOP_CST,          MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,    &CCodeGen_AArch64::Emit_Logic_VarVarCst<LOGICOP>        }, \
	{ LOGICOP_CST,          MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,    &CCodeGen_AArch64::Emit_Logic_VarAnyVar<LOGICOP>        },

CCodeGen_AArch64::CONSTMATCHER CCodeGen_AArch64::g_constMatchers[] =
{
	{ OP_NOP,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           &CCodeGen_AArch64::Emit_Nop                                 },

	{ OP_MOV,            MATCH_REGISTER,       MATCH_REGISTER,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Mov_RegReg                          },
	{ OP_MOV,            MATCH_REGISTER,       MATCH_MEMORY,         MATCH_NIL,           &CCodeGen_AArch64::Emit_Mov_RegMem                          },
	{ OP_MOV,            MATCH_REGISTER,       MATCH_CONSTANT,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Mov_RegCst                          },
	{ OP_MOV,            MATCH_MEMORY,         MATCH_REGISTER,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Mov_MemReg                          },
	{ OP_MOV,            MATCH_MEMORY,         MATCH_MEMORY,         MATCH_NIL,           &CCodeGen_AArch64::Emit_Mov_MemMem                          },
	{ OP_MOV,            MATCH_MEMORY,         MATCH_CONSTANT,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Mov_MemCst                          },

	{ OP_NOT,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Not_VarVar                          },
	{ OP_LZC,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Lzc_VarVar                          },
	
	{ OP_RELTOREF,       MATCH_TMP_REF,        MATCH_CONSTANT,       MATCH_ANY,           &CCodeGen_AArch64::Emit_RelToRef_TmpCst                     },

	{ OP_ADDREF,         MATCH_TMP_REF,        MATCH_MEM_REF,        MATCH_ANY,           &CCodeGen_AArch64::Emit_AddRef_TmpMemAny                    },

	{ OP_LOADFROMREF,    MATCH_VARIABLE,       MATCH_MEM_REF,        MATCH_NIL,           &CCodeGen_AArch64::Emit_LoadFromRef_VarMem                  },

	//Cannot use MATCH_ANY here because it will match SYM_RELATIVE128
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_MEM_REF,        MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_StoreAtRef_MemAny                   },
	{ OP_STOREATREF,     MATCH_NIL,            MATCH_MEM_REF,        MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_StoreAtRef_MemAny                   },
	
	{ OP_PARAM,          MATCH_NIL,            MATCH_CONTEXT,        MATCH_NIL,           &CCodeGen_AArch64::Emit_Param_Ctx                           },
	{ OP_PARAM,          MATCH_NIL,            MATCH_REGISTER,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Param_Reg                           },
	{ OP_PARAM,          MATCH_NIL,            MATCH_MEMORY,         MATCH_NIL,           &CCodeGen_AArch64::Emit_Param_Mem                           },
	{ OP_PARAM,          MATCH_NIL,            MATCH_CONSTANT,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Param_Cst                           },
	{ OP_PARAM,          MATCH_NIL,            MATCH_MEMORY64,       MATCH_NIL,           &CCodeGen_AArch64::Emit_Param_Mem64                         },
	{ OP_PARAM,          MATCH_NIL,            MATCH_CONSTANT64,     MATCH_NIL,           &CCodeGen_AArch64::Emit_Param_Cst64                         },
	{ OP_PARAM,          MATCH_NIL,            MATCH_REGISTER128,    MATCH_NIL,           &CCodeGen_AArch64::Emit_Param_Reg128                        },
	{ OP_PARAM,          MATCH_NIL,            MATCH_MEMORY128,      MATCH_NIL,           &CCodeGen_AArch64::Emit_Param_Mem128                        },
	
	{ OP_CALL,           MATCH_NIL,            MATCH_CONSTANTPTR,    MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Call                                },
	
	{ OP_RETVAL,         MATCH_REGISTER,       MATCH_NIL,            MATCH_NIL,           &CCodeGen_AArch64::Emit_RetVal_Reg                          },
	{ OP_RETVAL,         MATCH_TEMPORARY,      MATCH_NIL,            MATCH_NIL,           &CCodeGen_AArch64::Emit_RetVal_Tmp                          },
	{ OP_RETVAL,         MATCH_MEMORY64,       MATCH_NIL,            MATCH_NIL,           &CCodeGen_AArch64::Emit_RetVal_Mem64                        },
	{ OP_RETVAL,         MATCH_REGISTER128,    MATCH_NIL,            MATCH_NIL,           &CCodeGen_AArch64::Emit_RetVal_Reg128                       },
	{ OP_RETVAL,         MATCH_MEMORY128,      MATCH_NIL,            MATCH_NIL,           &CCodeGen_AArch64::Emit_RetVal_Mem128                       },
	
	{ OP_JMP,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           &CCodeGen_AArch64::Emit_Jmp                                 },
	
	{ OP_CONDJMP,        MATCH_NIL,            MATCH_ANY,            MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_CondJmp_AnyVar                      },
	{ OP_CONDJMP,        MATCH_NIL,            MATCH_VARIABLE,       MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_CondJmp_VarCst                      },
	
	{ OP_CMP,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_Cmp_VarAnyVar                       },
	{ OP_CMP,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Cmp_VarVarCst                       },
	
	{ OP_SLL,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_Shift_VarAnyVar<SHIFTOP_LSL>        },
	{ OP_SRL,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_Shift_VarAnyVar<SHIFTOP_LSR>        },
	{ OP_SRA,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_Shift_VarAnyVar<SHIFTOP_ASR>        },

	{ OP_SLL,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Shift_VarVarCst<SHIFTOP_LSL>        },
	{ OP_SRL,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Shift_VarVarCst<SHIFTOP_LSR>        },
	{ OP_SRA,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_Shift_VarVarCst<SHIFTOP_ASR>        },
	
	LOGIC_CONST_MATCHERS(OP_AND, LOGICOP_AND)
	LOGIC_CONST_MATCHERS(OP_OR,  LOGICOP_OR )
	LOGIC_CONST_MATCHERS(OP_XOR, LOGICOP_XOR)

	{ OP_ADD,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_AddSub_VarAnyVar<ADDSUBOP_ADD>      },
	{ OP_ADD,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_AddSub_VarVarCst<ADDSUBOP_ADD>      },
	{ OP_SUB,            MATCH_VARIABLE,       MATCH_ANY,            MATCH_VARIABLE,      &CCodeGen_AArch64::Emit_AddSub_VarAnyVar<ADDSUBOP_SUB>      },
	{ OP_SUB,            MATCH_VARIABLE,       MATCH_VARIABLE,       MATCH_CONSTANT,      &CCodeGen_AArch64::Emit_AddSub_VarVarCst<ADDSUBOP_SUB>      },
	
	{ OP_MUL,            MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           &CCodeGen_AArch64::Emit_Mul_Tmp64AnyAny<false>              },
	{ OP_MULS,           MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           &CCodeGen_AArch64::Emit_Mul_Tmp64AnyAny<true>               },

	{ OP_DIV,            MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           &CCodeGen_AArch64::Emit_Div_Tmp64AnyAny<false>              },
	{ OP_DIVS,           MATCH_TEMPORARY64,    MATCH_ANY,            MATCH_ANY,           &CCodeGen_AArch64::Emit_Div_Tmp64AnyAny<true>               },
	
	{ OP_LABEL,          MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           &CCodeGen_AArch64::MarkLabel                                },
};

CCodeGen_AArch64::CCodeGen_AArch64()
{
	const auto copyMatchers =
		[this](const CONSTMATCHER* constMatchers)
		{
			for(auto* constMatcher = constMatchers; constMatcher->emitter != nullptr; constMatcher++)
			{
				MATCHER matcher;
				matcher.op       = constMatcher->op;
				matcher.dstType  = constMatcher->dstType;
				matcher.src1Type = constMatcher->src1Type;
				matcher.src2Type = constMatcher->src2Type;
				matcher.emitter  = std::bind(constMatcher->emitter, this, std::placeholders::_1);
				m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
			}
		};
	
	copyMatchers(g_constMatchers);
	copyMatchers(g_64ConstMatchers);
	copyMatchers(g_fpuConstMatchers);
	copyMatchers(g_mdConstMatchers);
}

CCodeGen_AArch64::~CCodeGen_AArch64()
{

}

void CCodeGen_AArch64::SetGenerateRelocatableCalls(bool generateRelocatableCalls)
{
	m_generateRelocatableCalls = generateRelocatableCalls;
}

unsigned int CCodeGen_AArch64::GetAvailableRegisterCount() const
{
	return MAX_REGISTERS;
}

unsigned int CCodeGen_AArch64::GetAvailableMdRegisterCount() const
{
	return MAX_MDREGISTERS;
}

bool CCodeGen_AArch64::CanHold128BitsReturnValueInRegisters() const
{
	return true;
}

void CCodeGen_AArch64::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
	m_assembler.SetStream(stream);
}

void CCodeGen_AArch64::RegisterExternalSymbols(CObjectFile* objectFile) const
{

}

void CCodeGen_AArch64::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	m_nextTempRegister = 0;
	m_nextTempRegisterMd = 0;
	
	//Align stack size (must be aligned on 16 bytes boundary)
	stackSize = (stackSize + 0xF) & ~0xF;

	uint16 registerSave = GetSavedRegisterList(GetRegisterUsage(statements));

	Emit_Prolog(statements, stackSize, registerSave);

	for(const auto& statement : statements)
	{
		bool found = false;
		auto begin = m_matchers.lower_bound(statement.op);
		auto end = m_matchers.upper_bound(statement.op);

		for(auto matchIterator(begin); matchIterator != end; matchIterator++)
		{
			const MATCHER& matcher(matchIterator->second);
			if(!SymbolMatches(matcher.dstType, statement.dst)) continue;
			if(!SymbolMatches(matcher.src1Type, statement.src1)) continue;
			if(!SymbolMatches(matcher.src2Type, statement.src2)) continue;
			matcher.emitter(statement);
			found = true;
			break;
		}
		assert(found);
		if(!found)
		{
			throw std::runtime_error("No suitable emitter found for statement.");
		}
	}
	
	Emit_Epilog(stackSize, registerSave);

	m_assembler.ResolveLabelReferences();
	m_assembler.ClearLabels();
	m_labels.clear();
}

uint32 CCodeGen_AArch64::GetMaxParamSpillSize(const StatementList& statements)
{
	uint32 maxParamSpillSize = 0;
	uint32 currParamSpillSize = 0;
	for(const auto& statement : statements)
	{
		switch(statement.op)
		{
		case OP_PARAM:
		case OP_PARAM_RET:
			{
				CSymbol* src1 = statement.src1->GetSymbol().get();
				switch(src1->m_type)
				{
				case SYM_REGISTER128:
					currParamSpillSize += 16;
					break;
				default:
					break;
				}
			}
			break;
		case OP_CALL:
			maxParamSpillSize = std::max<uint32>(currParamSpillSize, maxParamSpillSize);
			currParamSpillSize = 0;
			break;
		default:
			break;
		}
	}
	return maxParamSpillSize;
}

CAArch64Assembler::REGISTER32 CCodeGen_AArch64::GetNextTempRegister()
{
	auto result = g_tempRegisters[m_nextTempRegister];
	m_nextTempRegister++;
	m_nextTempRegister %= MAX_TEMP_REGS;
	return result;
}

CAArch64Assembler::REGISTER64 CCodeGen_AArch64::GetNextTempRegister64()
{
	auto result = g_tempRegisters64[m_nextTempRegister];
	m_nextTempRegister++;
	m_nextTempRegister %= MAX_TEMP_REGS;
	return result;
}

CAArch64Assembler::REGISTERMD CCodeGen_AArch64::GetNextTempRegisterMd()
{
	auto result = g_tempRegistersMd[m_nextTempRegisterMd];
	m_nextTempRegisterMd++;
	m_nextTempRegisterMd %= MAX_TEMP_MD_REGS;
	return result;
}

void CCodeGen_AArch64::LoadMemoryInRegister(CAArch64Assembler::REGISTER32 registerId, CSymbol* src)
{
	switch(src->m_type)
	{
	case SYM_RELATIVE:
		assert((src->m_valueLow & 0x03) == 0x00);
		m_assembler.Ldr(registerId, g_baseRegister, src->m_valueLow);
		break;
	case SYM_TEMPORARY:
		m_assembler.Ldr(registerId, CAArch64Assembler::xSP, src->m_stackLocation);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::StoreRegisterInMemory(CSymbol* dst, CAArch64Assembler::REGISTER32 registerId)
{
	switch(dst->m_type)
	{
	case SYM_RELATIVE:
		assert((dst->m_valueLow & 0x03) == 0x00);
		m_assembler.Str(registerId, g_baseRegister, dst->m_valueLow);
		break;
	case SYM_TEMPORARY:
		m_assembler.Str(registerId, CAArch64Assembler::xSP, dst->m_stackLocation);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::LoadConstantInRegister(CAArch64Assembler::REGISTER32 registerId, uint32 constant)
{
	if((constant & 0x0000FFFF) == constant)
	{
		m_assembler.Movz(registerId, static_cast<uint16>(constant & 0xFFFF), 0);
	}
	else if((constant & 0xFFFF0000) == constant)
	{
		m_assembler.Movz(registerId, static_cast<uint16>(constant >> 16), 1);
	}
	else if((~constant & 0x0000FFFF) == ~constant)
	{
		m_assembler.Movn(registerId, static_cast<uint16>(~constant & 0xFFFF), 0);
	}
	else if((~constant & 0xFFFF0000) == ~constant)
	{
		m_assembler.Movn(registerId, static_cast<uint16>(~constant >> 16), 1);
	}
	else
	{
		m_assembler.Movz(registerId, static_cast<uint16>(constant & 0xFFFF), 0);
		m_assembler.Movk(registerId, static_cast<uint16>(constant >> 16), 1);
	}
}

void CCodeGen_AArch64::LoadMemoryReferenceInRegister(CAArch64Assembler::REGISTER64 registerId, CSymbol* src)
{
	switch(src->m_type)
	{
	case SYM_REL_REFERENCE:
		assert((src->m_valueLow & 0x07) == 0x00);
		m_assembler.Ldr(registerId, g_baseRegister, src->m_valueLow);
		break;
	case SYM_TMP_REFERENCE:
		m_assembler.Ldr(registerId, CAArch64Assembler::xSP, src->m_stackLocation);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch64::StoreRegisterInTemporaryReference(CSymbol* dst, CAArch64Assembler::REGISTER64 registerId)
{
	assert(dst->m_type == SYM_TMP_REFERENCE);
	m_assembler.Str(registerId, CAArch64Assembler::xSP, dst->m_stackLocation);
}

CAArch64Assembler::REGISTER32 CCodeGen_AArch64::PrepareSymbolRegisterDef(CSymbol* symbol, CAArch64Assembler::REGISTER32 preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		assert(symbol->m_valueLow < MAX_REGISTERS);
		return g_registers[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch64Assembler::REGISTER32 CCodeGen_AArch64::PrepareSymbolRegisterUse(CSymbol* symbol, CAArch64Assembler::REGISTER32 preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		assert(symbol->m_valueLow < MAX_REGISTERS);
		return g_registers[symbol->m_valueLow];
		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		LoadMemoryInRegister(preferedRegister, symbol);
		return preferedRegister;
		break;
	case SYM_CONSTANT:
		LoadConstantInRegister(preferedRegister, symbol->m_valueLow);
		return preferedRegister;
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

void CCodeGen_AArch64::CommitSymbolRegister(CSymbol* symbol, CAArch64Assembler::REGISTER32 usedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		assert(usedRegister == g_registers[symbol->m_valueLow]);
		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		StoreRegisterInMemory(symbol, usedRegister);
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch64Assembler::REGISTER32 CCodeGen_AArch64::PrepareParam(PARAM_STATE& paramState)
{
	assert(!paramState.prepared);
	paramState.prepared = true;
	if(paramState.index < MAX_PARAM_REGS)
	{
		return g_paramRegisters[paramState.index];
	}
	else
	{
		assert(false);
		return g_paramRegisters[0];
	}
}

CAArch64Assembler::REGISTER64 CCodeGen_AArch64::PrepareParam64(PARAM_STATE& paramState)
{
	assert(!paramState.prepared);
	paramState.prepared = true;
	if(paramState.index < MAX_PARAM_REGS)
	{
		return g_paramRegisters64[paramState.index];
	}
	else
	{
		assert(false);
		return g_paramRegisters64[0];
	}
}

void CCodeGen_AArch64::CommitParam(PARAM_STATE& paramState)
{
	assert(paramState.prepared);
	paramState.prepared = false;
	if(paramState.index < MAX_PARAM_REGS)
	{
		//Nothing to do
	}
	else
	{
		assert(false);
	}
	paramState.index++;
}

void CCodeGen_AArch64::CommitParam64(PARAM_STATE& paramState)
{
	assert(paramState.prepared);
	paramState.prepared = false;
	if(paramState.index < MAX_PARAM_REGS)
	{
		//Nothing to do
	}
	else
	{
		assert(false);
	}
	paramState.index++;
}

bool CCodeGen_AArch64::TryGetAddSubImmParams(uint32 imm, ADDSUB_IMM_PARAMS& params)
{
	if((imm & 0xFFF) == imm)
	{
		params.imm = static_cast<uint16>(imm);
		params.shiftType = CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0;
		return true;
	}
	else if((imm & 0xFFF000) == imm)
	{
		params.imm = static_cast<uint16>(imm >> 12);
		params.shiftType = CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL12;
		return true;
	}
	return false;
}

bool CCodeGen_AArch64::TryGetAddSub64ImmParams(uint64 imm, ADDSUB_IMM_PARAMS& params)
{
	if((imm & 0xFFF) == imm)
	{
		params.imm = static_cast<uint16>(imm);
		params.shiftType = CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0;
		return true;
	}
	else if((imm & 0xFFF000) == imm)
	{
		params.imm = static_cast<uint16>(imm >> 12);
		params.shiftType = CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL12;
		return true;
	}
	return false;
}

bool CCodeGen_AArch64::TryGetLogicalImmParams(uint32 imm, LOGICAL_IMM_PARAMS& params)
{
	//Algorithm from LLVM, 'processLogicalImmediate' function

	if((imm == 0) || (imm == ~0))
	{
		return false;
	}

	int size = 32;
	while(true)
	{
		size /= 2;
		uint32 mask = (1 << size) - 1;
		if((imm & mask) != ((imm >> size) & mask))
		{
			size *= 2;
			break;
		}
		if(size > 2) break;
	}

	uint32 cto = 0, i = 0;
	uint32 mask = (~0) >> (32 - size);
	imm &= mask;

	if(isShiftedMask(imm))
	{
		i = __builtin_ctz(imm);
		cto = __builtin_ctz(~(imm >> i));
	}
	else
	{
		imm |= ~mask;
		if(!isShiftedMask(~imm))
		{
			return false;
		}
		uint32 clo = __builtin_clz(~imm);
		i = 32 - clo;
		cto = clo + __builtin_ctz(~imm) - (32 - size);
	}

	assert(size > i);
	params.immr = (size - i) & (size - 1);

	uint32 nimms = ~(size - 1) << 1;
	nimms |= (cto - 1);

	params.n = ((nimms >> 6) & 1) ^ 1;

	params.imms = nimms & 0x3F;

	return true;
}

uint16 CCodeGen_AArch64::GetSavedRegisterList(uint32 registerUsage)
{
	uint16 registerSave = 0;
	for(unsigned int i = 0; i < MAX_REGISTERS; i++)
	{
		if((1 << i) & registerUsage)
		{
			registerSave |= (1 << (g_registers[i] / 2));
		}
	}
	registerSave |= (1 << (g_baseRegister / 2));
	return registerSave;
}

void CCodeGen_AArch64::Emit_Prolog(const StatementList& statements, uint32 stackSize, uint16 registerSave)
{
	uint32 maxParamSpillSize = GetMaxParamSpillSize(statements);
	m_assembler.Stp_PreIdx(CAArch64Assembler::x29, CAArch64Assembler::x30, CAArch64Assembler::xSP, -16);
	//Preserve saved registers
	for(uint32 i = 0; i < 16; i++)
	{
		if(registerSave & (1 << i))
		{
			auto reg0 = static_cast<CAArch64Assembler::REGISTER64>((i * 2) + 0);
			auto reg1 = static_cast<CAArch64Assembler::REGISTER64>((i * 2) + 1);
			m_assembler.Stp_PreIdx(reg0, reg1, CAArch64Assembler::xSP, -16);
		}
	}
	m_assembler.Mov_Sp(CAArch64Assembler::x29, CAArch64Assembler::xSP);
	uint32 totalStackAlloc = stackSize + maxParamSpillSize;
	m_paramSpillBase = stackSize;
	if(totalStackAlloc != 0)
	{
		m_assembler.Sub(CAArch64Assembler::xSP, CAArch64Assembler::xSP, totalStackAlloc, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
	}
	m_assembler.Mov(g_baseRegister, CAArch64Assembler::x0);
}

void CCodeGen_AArch64::Emit_Epilog(uint32 stackSize, uint16 registerSave)
{
	m_assembler.Mov_Sp(CAArch64Assembler::xSP, CAArch64Assembler::x29);
	//Restore saved registers
	for(int32 i = 15; i >= 0; i--)
	{
		if(registerSave & (1 << i))
		{
			auto reg0 = static_cast<CAArch64Assembler::REGISTER64>((i * 2) + 0);
			auto reg1 = static_cast<CAArch64Assembler::REGISTER64>((i * 2) + 1);
			m_assembler.Ldp_PostIdx(reg0, reg1, CAArch64Assembler::xSP, 16);
		}
	}
	m_assembler.Ldp_PostIdx(CAArch64Assembler::x29, CAArch64Assembler::x30, CAArch64Assembler::xSP, 16);
	m_assembler.Ret();
}

CAArch64Assembler::LABEL CCodeGen_AArch64::GetLabel(uint32 blockId)
{
	CAArch64Assembler::LABEL result;
	auto labelIterator(m_labels.find(blockId));
	if(labelIterator == m_labels.end())
	{
		result = m_assembler.CreateLabel();
		m_labels[blockId] = result;
	}
	else
	{
		result = labelIterator->second;
	}
	return result;
}

void CCodeGen_AArch64::MarkLabel(const STATEMENT& statement)
{
	auto label = GetLabel(statement.jmpBlock);
	m_assembler.MarkLabel(label);
}

void CCodeGen_AArch64::Emit_Nop(const STATEMENT&)
{

}

void CCodeGen_AArch64::Emit_Mov_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.Mov(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow]);
}

void CCodeGen_AArch64::Emit_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	LoadMemoryInRegister(g_registers[dst->m_valueLow], src1);
}

void CCodeGen_AArch64::Emit_Mov_RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);

	LoadConstantInRegister(g_registers[dst->m_valueLow], src1->m_valueLow);
}

void CCodeGen_AArch64::Emit_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	StoreRegisterInMemory(dst, g_registers[src1->m_valueLow]);
}

void CCodeGen_AArch64::Emit_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto tmpReg = GetNextTempRegister();
	LoadMemoryInRegister(tmpReg, src1);
	StoreRegisterInMemory(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_Mov_MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	assert(src1->m_type == SYM_CONSTANT);
	
	auto tmpReg = GetNextTempRegister();
	LoadConstantInRegister(tmpReg, src1->m_valueLow);
	StoreRegisterInMemory(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_Not_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	m_assembler.Mvn(dstReg, src1Reg);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Lzc_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Register = PrepareSymbolRegisterUse(src1, GetNextTempRegister());

	auto set32Label = m_assembler.CreateLabel();
	auto startCountLabel = m_assembler.CreateLabel();
	auto doneLabel = m_assembler.CreateLabel();

	m_assembler.Mov(dstRegister, src1Register);
	m_assembler.Tst(dstRegister, dstRegister);
	m_assembler.BCc(CAArch64Assembler::CONDITION_EQ, set32Label);
	m_assembler.BCc(CAArch64Assembler::CONDITION_PL, startCountLabel);

	//reverse:
	m_assembler.Mvn(dstRegister, dstRegister);
	m_assembler.Tst(dstRegister, dstRegister);
	m_assembler.BCc(CAArch64Assembler::CONDITION_EQ, set32Label);

	//startCount:
	m_assembler.MarkLabel(startCountLabel);
	m_assembler.Clz(dstRegister, dstRegister);
	m_assembler.Sub(dstRegister, dstRegister, 1, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
	m_assembler.BCc(CAArch64Assembler::CONDITION_AL, doneLabel);

	//set32:
	m_assembler.MarkLabel(set32Label);
	LoadConstantInRegister(dstRegister, 0x1F);

	//done
	m_assembler.MarkLabel(doneLabel);

	CommitSymbolRegister(dst, dstRegister);
}

void CCodeGen_AArch64::Emit_RelToRef_TmpCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	auto tmpReg = GetNextTempRegister64();

	ADDSUB_IMM_PARAMS addSubImmParams;
	if(TryGetAddSubImmParams(src1->m_valueLow, addSubImmParams))
	{
		m_assembler.Add(tmpReg, g_baseRegister, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else
	{
		assert(false);
	}

	StoreRegisterInTemporaryReference(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_AddRef_TmpMemAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto tmpReg = GetNextTempRegister64();
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());

	LoadMemoryReferenceInRegister(tmpReg, src1);
	m_assembler.Add(tmpReg, tmpReg, static_cast<CAArch64Assembler::REGISTER64>(src2Reg));
	StoreRegisterInTemporaryReference(dst, tmpReg);
}

void CCodeGen_AArch64::Emit_LoadFromRef_VarMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
		
	auto addressReg = GetNextTempRegister64();
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());

	LoadMemoryReferenceInRegister(addressReg, src1);
	m_assembler.Ldr(dstReg, addressReg, 0);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_StoreAtRef_MemAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src1->m_type == SYM_TMP_REFERENCE);
	
	auto addressReg = GetNextTempRegister64();
	auto valueReg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	
	LoadMemoryReferenceInRegister(addressReg, src1);
	m_assembler.Str(valueReg, addressReg, 0);
}

void CCodeGen_AArch64::Emit_Param_Ctx(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	
	assert(src1->m_type == SYM_CONTEXT);
	
	m_params.push_back(
		[this] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam64(paramState);
			m_assembler.Mov(paramReg, g_baseRegister);
			CommitParam64(paramState);
		}
	);
}

void CCodeGen_AArch64::Emit_Param_Reg(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	
	assert(src1->m_type == SYM_REGISTER);
	
	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam(paramState);
			m_assembler.Mov(paramReg, g_registers[src1->m_valueLow]);
			CommitParam(paramState);
		}
	);
}

void CCodeGen_AArch64::Emit_Param_Mem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
		
	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam(paramState);
			LoadMemoryInRegister(paramReg, src1);
			CommitParam(paramState);
		}
	);
}

void CCodeGen_AArch64::Emit_Param_Cst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
		
	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam(paramState);
			LoadConstantInRegister(paramReg, src1->m_valueLow);
			CommitParam(paramState);
		}
	);
}

void CCodeGen_AArch64::Emit_Param_Mem64(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam64(paramState);
			LoadMemory64InRegister(paramReg, src1);
			CommitParam64(paramState);
		}
	);
}

void CCodeGen_AArch64::Emit_Param_Cst64(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam64(paramState);
			LoadConstant64InRegister(paramReg, src1->GetConstant64());
			CommitParam64(paramState);
		}
	);
}

void CCodeGen_AArch64::Emit_Param_Reg128(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	
	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam64(paramState);
			uint32 paramBase = m_paramSpillBase + paramState.spillOffset;
			m_assembler.Add(paramReg, CAArch64Assembler::xSP, paramBase, CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0);
			m_assembler.Str_1q(g_registersMd[src1->m_valueLow], CAArch64Assembler::xSP, paramBase);
			paramState.spillOffset += 0x10;
			CommitParam64(paramState);
		}
	);
}

void CCodeGen_AArch64::Emit_Param_Mem128(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam64(paramState);
			LoadMemory128AddressInRegister(paramReg, src1);
			CommitParam64(paramState);
		}
	);
}

void CCodeGen_AArch64::Emit_Call(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src1->m_type == SYM_CONSTANTPTR);
	assert(src2->m_type == SYM_CONSTANT);

	unsigned int paramCount = src2->m_valueLow;
	PARAM_STATE paramState;

	for(unsigned int i = 0; i < paramCount; i++)
	{
		auto emitter(m_params.back());
		m_params.pop_back();
		emitter(paramState);
	}
	
	if(m_generateRelocatableCalls)
	{
		if(m_externalSymbolReferencedHandler)
		{
			auto position = m_stream->GetLength();
			m_externalSymbolReferencedHandler(src1->GetConstantPtr(), position);
		}
		m_assembler.Bl(0);
	}
	else
	{
		auto fctAddressReg = GetNextTempRegister64();
		LoadConstant64InRegister(fctAddressReg, src1->GetConstantPtr());
		m_assembler.Blr(fctAddressReg);
	}
}

void CCodeGen_AArch64::Emit_RetVal_Reg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	assert(dst->m_type == SYM_REGISTER);
	m_assembler.Mov(g_registers[dst->m_valueLow], CAArch64Assembler::w0);
}

void CCodeGen_AArch64::Emit_RetVal_Tmp(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	assert(dst->m_type == SYM_TEMPORARY);
	StoreRegisterInMemory(dst, CAArch64Assembler::w0);
}

void CCodeGen_AArch64::Emit_RetVal_Mem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	StoreRegisterInMemory64(dst, CAArch64Assembler::x0);
}

void CCodeGen_AArch64::Emit_RetVal_Reg128(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	
	m_assembler.Ins_1d(g_registersMd[dst->m_valueLow], 0, CAArch64Assembler::x0);
	m_assembler.Ins_1d(g_registersMd[dst->m_valueLow], 1, CAArch64Assembler::x1);
}

void CCodeGen_AArch64::Emit_RetVal_Mem128(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	
	auto dstAddrReg = GetNextTempRegister64();
	
	LoadMemory128AddressInRegister(dstAddrReg, dst);
	m_assembler.Str(CAArch64Assembler::x0, dstAddrReg, 0);
	m_assembler.Str(CAArch64Assembler::x1, dstAddrReg, 8);
}

void CCodeGen_AArch64::Emit_Jmp(const STATEMENT& statement)
{
	m_assembler.B(GetLabel(statement.jmpBlock));
}

void CCodeGen_AArch64::Emit_CondJmp(const STATEMENT& statement)
{
	auto label(GetLabel(statement.jmpBlock));
	
	switch(statement.jmpCondition)
	{
	case CONDITION_EQ:
		m_assembler.BCc(CAArch64Assembler::CONDITION_EQ, label);
		break;
	case CONDITION_NE:
		m_assembler.BCc(CAArch64Assembler::CONDITION_NE, label);
		break;
	case CONDITION_LT:
		m_assembler.BCc(CAArch64Assembler::CONDITION_LT, label);
		break;
	case CONDITION_LE:
		m_assembler.BCc(CAArch64Assembler::CONDITION_LE, label);
		break;
	case CONDITION_GT:
		m_assembler.BCc(CAArch64Assembler::CONDITION_GT, label);
		break;
	case CONDITION_GE:
		m_assembler.BCc(CAArch64Assembler::CONDITION_GE, label);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::Emit_CondJmp_AnyVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	m_assembler.Cmp(src1Reg, src2Reg);
	Emit_CondJmp(statement);
}

void CCodeGen_AArch64::Emit_CondJmp_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	assert(src2->m_type == SYM_CONSTANT);
	
	ADDSUB_IMM_PARAMS addSubImmParams;
	if(TryGetAddSubImmParams(src2->m_valueLow, addSubImmParams))
	{
		m_assembler.Cmp(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else if(TryGetAddSubImmParams(-static_cast<int32>(src2->m_valueLow), addSubImmParams))
	{
		m_assembler.Cmn(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else
	{
		auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
		m_assembler.Cmp(src1Reg, src2Reg);
	}

	Emit_CondJmp(statement);
}

void CCodeGen_AArch64::Cmp_GetFlag(CAArch64Assembler::REGISTER32 registerId, Jitter::CONDITION condition)
{
	switch(condition)
	{
	case CONDITION_EQ:
		m_assembler.Cset(registerId, CAArch64Assembler::CONDITION_EQ);
		break;
	case CONDITION_NE:
		m_assembler.Cset(registerId, CAArch64Assembler::CONDITION_NE);
		break;
	case CONDITION_LT:
		m_assembler.Cset(registerId, CAArch64Assembler::CONDITION_LT);
		break;
	case CONDITION_LE:
		m_assembler.Cset(registerId, CAArch64Assembler::CONDITION_LE);
		break;
	case CONDITION_GT:
		m_assembler.Cset(registerId, CAArch64Assembler::CONDITION_GT);
		break;
	case CONDITION_BL:
		m_assembler.Cset(registerId, CAArch64Assembler::CONDITION_CC);
		break;
	case CONDITION_BE:
		m_assembler.Cset(registerId, CAArch64Assembler::CONDITION_LS);
		break;
	case CONDITION_AB:
		m_assembler.Cset(registerId, CAArch64Assembler::CONDITION_HI);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch64::Emit_Cmp_VarAnyVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
	m_assembler.Cmp(src1Reg, src2Reg);
	Cmp_GetFlag(dstReg, statement.jmpCondition);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch64::Emit_Cmp_VarVarCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src2->m_type == SYM_CONSTANT);
	
	auto dstReg = PrepareSymbolRegisterDef(dst, GetNextTempRegister());
	auto src1Reg = PrepareSymbolRegisterUse(src1, GetNextTempRegister());
	
	ADDSUB_IMM_PARAMS addSubImmParams;
	if(TryGetAddSubImmParams(src2->m_valueLow, addSubImmParams))
	{
		m_assembler.Cmp(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else if(TryGetAddSubImmParams(-static_cast<int32>(src2->m_valueLow), addSubImmParams))
	{
		m_assembler.Cmn(src1Reg, addSubImmParams.imm, addSubImmParams.shiftType);
	}
	else
	{
		auto src2Reg = PrepareSymbolRegisterUse(src2, GetNextTempRegister());
		m_assembler.Cmp(src1Reg, src2Reg);
	}

	Cmp_GetFlag(dstReg, statement.jmpCondition);
	CommitSymbolRegister(dst, dstReg);
}
