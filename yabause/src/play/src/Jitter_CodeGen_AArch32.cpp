#include "Jitter_CodeGen_AArch32.h"
#include "ObjectFile.h"
#include "BitManip.h"
#ifdef __ANDROID__
#include <cpu-features.h>
#endif

using namespace Jitter;

CAArch32Assembler::REGISTER CCodeGen_AArch32::g_baseRegister = CAArch32Assembler::r11;
CAArch32Assembler::REGISTER CCodeGen_AArch32::g_callAddressRegister = CAArch32Assembler::r4;
CAArch32Assembler::REGISTER CCodeGen_AArch32::g_tempParamRegister0 = CAArch32Assembler::r4;
CAArch32Assembler::REGISTER CCodeGen_AArch32::g_tempParamRegister1 = CAArch32Assembler::r5;

CAArch32Assembler::REGISTER CCodeGen_AArch32::g_registers[MAX_REGISTERS] =
{
	CAArch32Assembler::r4,
	CAArch32Assembler::r5,
	CAArch32Assembler::r6,
	CAArch32Assembler::r7,
	CAArch32Assembler::r8,
	CAArch32Assembler::r10,
};

CAArch32Assembler::REGISTER CCodeGen_AArch32::g_paramRegs[MAX_PARAM_REGS] =
{
	CAArch32Assembler::r0,
	CAArch32Assembler::r1,
	CAArch32Assembler::r2,
	CAArch32Assembler::r3,
};

template <typename ALUOP>
void CCodeGen_AArch32::Emit_Alu_GenericAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
	((m_assembler).*(ALUOP::OpReg()))(dstReg, src1Reg, src2Reg);
	CommitSymbolRegister(dst, dstReg);
}

template <typename ALUOP>
void CCodeGen_AArch32::Emit_Alu_GenericAnyCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	uint32 cst = src2->m_valueLow;

	bool supportsNegative	= ALUOP::OpImmNeg() != NULL;
	bool supportsComplement = ALUOP::OpImmNot() != NULL;
	
	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(TryGetAluImmediateParams(cst, immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImm()))(dstReg, src1Reg, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else if(supportsNegative && TryGetAluImmediateParams(-static_cast<int32>(cst), immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImmNeg()))(dstReg, src1Reg, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else if(supportsComplement && TryGetAluImmediateParams(~cst, immediate, shiftAmount))
	{
		((m_assembler).*(ALUOP::OpImmNot()))(dstReg, src1Reg, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		auto cstReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
		assert(cstReg != dstReg && cstReg != src1Reg);
		((m_assembler).*(ALUOP::OpReg()))(dstReg, src1Reg, cstReg);
	}

	CommitSymbolRegister(dst, dstReg);
}

#define ALU_CONST_MATCHERS(ALUOP_CST, ALUOP) \
	{ ALUOP_CST,	MATCH_ANY,		MATCH_ANY,		MATCH_CONSTANT,	&CCodeGen_AArch32::Emit_Alu_GenericAnyCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_ANY,		MATCH_ANY,		MATCH_ANY,		&CCodeGen_AArch32::Emit_Alu_GenericAnyAny<ALUOP>		},

#include "Jitter_CodeGen_AArch32_Div.h"

template <bool isSigned>
void CCodeGen_AArch32::Emit_MulTmp64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto resLoReg = CAArch32Assembler::r0;
	auto resHiReg = CAArch32Assembler::r1;
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r2);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r3);
	
	assert(dst->m_type == SYM_TEMPORARY64);
	assert(resLoReg != src1Reg && resLoReg != src2Reg);
	assert(resHiReg != src1Reg && resHiReg != src2Reg);
	
	if(isSigned)
	{
		m_assembler.Smull(resLoReg, resHiReg, src1Reg, src2Reg);
	}
	else
	{
		m_assembler.Umull(resLoReg, resHiReg, src1Reg, src2Reg);
	}
	
	m_assembler.Str(resLoReg, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 0));
	m_assembler.Str(resHiReg, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 4));
}

template <CAArch32Assembler::SHIFT shiftType>
void CCodeGen_AArch32::Emit_Shift_Generic(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto shift = GetAluShiftFromSymbol(shiftType, src2, CAArch32Assembler::r2);
	m_assembler.Mov(dstReg, CAArch32Assembler::MakeRegisterAluOperand(src1Reg, shift));
	CommitSymbolRegister(dst, dstReg);
}

CCodeGen_AArch32::CONSTMATCHER CCodeGen_AArch32::g_constMatchers[] = 
{ 
	{ OP_LABEL,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_AArch32::MarkLabel									},

	{ OP_NOP,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_AArch32::Emit_Nop										},
	
	{ OP_MOV,			MATCH_REGISTER,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Mov_RegReg								},
	{ OP_MOV,			MATCH_REGISTER,		MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Mov_RegMem								},
	{ OP_MOV,			MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Mov_RegCst								},
	{ OP_MOV,			MATCH_MEMORY,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Mov_MemReg								},
	{ OP_MOV,			MATCH_MEMORY,		MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Mov_MemMem								},
	{ OP_MOV,			MATCH_MEMORY,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Mov_MemCst								},

	ALU_CONST_MATCHERS(OP_ADD, ALUOP_ADD)
	ALU_CONST_MATCHERS(OP_SUB, ALUOP_SUB)
	ALU_CONST_MATCHERS(OP_AND, ALUOP_AND)
	ALU_CONST_MATCHERS(OP_OR,  ALUOP_OR)
	ALU_CONST_MATCHERS(OP_XOR, ALUOP_XOR)
	
	{ OP_LZC,			MATCH_VARIABLE,		MATCH_VARIABLE,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Lzc_VarVar								},

	{ OP_SRL,			MATCH_ANY,			MATCH_ANY,			MATCH_ANY,			&CCodeGen_AArch32::Emit_Shift_Generic<CAArch32Assembler::SHIFT_LSR>	},
	{ OP_SRA,			MATCH_ANY,			MATCH_ANY,			MATCH_ANY,			&CCodeGen_AArch32::Emit_Shift_Generic<CAArch32Assembler::SHIFT_ASR>	},
	{ OP_SLL,			MATCH_ANY,			MATCH_ANY,			MATCH_ANY,			&CCodeGen_AArch32::Emit_Shift_Generic<CAArch32Assembler::SHIFT_LSL>	},

	{ OP_PARAM,			MATCH_NIL,			MATCH_CONTEXT,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Param_Ctx								},
	{ OP_PARAM,			MATCH_NIL,			MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Param_Reg								},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Param_Mem								},
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Param_Cst								},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Param_Mem64								},
	{ OP_PARAM,			MATCH_NIL,			MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_AArch32::Emit_Param_Cst64								},
	{ OP_PARAM,			MATCH_NIL,			MATCH_MEMORY128,	MATCH_NIL,			&CCodeGen_AArch32::Emit_Param_Mem128							},

	{ OP_PARAM_RET,		MATCH_NIL,			MATCH_TEMPORARY128,	MATCH_NIL,			&CCodeGen_AArch32::Emit_ParamRet_Tmp128							},

	{ OP_CALL,			MATCH_NIL,			MATCH_CONSTANTPTR,	MATCH_CONSTANT,		&CCodeGen_AArch32::Emit_Call									},
	
	{ OP_RETVAL,		MATCH_REGISTER,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_AArch32::Emit_RetVal_Reg								},
	{ OP_RETVAL,		MATCH_TEMPORARY,	MATCH_NIL,			MATCH_NIL,			&CCodeGen_AArch32::Emit_RetVal_Tmp								},
	{ OP_RETVAL,		MATCH_MEMORY64,		MATCH_NIL,			MATCH_NIL,			&CCodeGen_AArch32::Emit_RetVal_Mem64							},

	{ OP_JMP,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_AArch32::Emit_Jmp										},

	{ OP_CONDJMP,		MATCH_NIL,			MATCH_VARIABLE,		MATCH_CONSTANT,		&CCodeGen_AArch32::Emit_CondJmp_VarCst							},
	{ OP_CONDJMP,		MATCH_NIL,			MATCH_VARIABLE,		MATCH_VARIABLE,		&CCodeGen_AArch32::Emit_CondJmp_VarVar							},
	
	{ OP_CMP,			MATCH_ANY,			MATCH_ANY,			MATCH_CONSTANT,		&CCodeGen_AArch32::Emit_Cmp_AnyAnyCst							},
	{ OP_CMP,			MATCH_ANY,			MATCH_ANY,			MATCH_ANY,			&CCodeGen_AArch32::Emit_Cmp_AnyAnyAny							},

	{ OP_NOT,			MATCH_REGISTER,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Not_RegReg								},
	{ OP_NOT,			MATCH_MEMORY,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Not_MemReg								},
	{ OP_NOT,			MATCH_MEMORY,		MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Not_MemMem								},
	
	{ OP_DIV,			MATCH_TEMPORARY64,	MATCH_ANY,			MATCH_ANY,			&CCodeGen_AArch32::Emit_DivTmp64AnyAny<false>					},
	{ OP_DIVS,			MATCH_TEMPORARY64,	MATCH_ANY,			MATCH_ANY,			&CCodeGen_AArch32::Emit_DivTmp64AnyAny<true>					},
	
	{ OP_MUL,			MATCH_TEMPORARY64,	MATCH_ANY,			MATCH_ANY,			&CCodeGen_AArch32::Emit_MulTmp64AnyAny<false>					},
	{ OP_MULS,			MATCH_TEMPORARY64,	MATCH_ANY,			MATCH_ANY,			&CCodeGen_AArch32::Emit_MulTmp64AnyAny<true>					},

	{ OP_RELTOREF,		MATCH_TMP_REF,		MATCH_CONSTANT,		MATCH_ANY,			&CCodeGen_AArch32::Emit_RelToRef_TmpCst							},

	{ OP_ADDREF,		MATCH_TMP_REF,		MATCH_MEM_REF,		MATCH_ANY,			&CCodeGen_AArch32::Emit_AddRef_TmpMemAny						},
	
	{ OP_LOADFROMREF,	MATCH_VARIABLE,		MATCH_TMP_REF,		MATCH_NIL,			&CCodeGen_AArch32::Emit_LoadFromRef_VarTmp						},

	//Cannot use MATCH_ANY here because it will match SYM_RELATIVE128
	{ OP_STOREATREF,	MATCH_NIL,			MATCH_TMP_REF,		MATCH_VARIABLE,		&CCodeGen_AArch32::Emit_StoreAtRef_TmpAny						},
	{ OP_STOREATREF,	MATCH_NIL,			MATCH_TMP_REF,		MATCH_CONSTANT,		&CCodeGen_AArch32::Emit_StoreAtRef_TmpAny						},
	
	{ OP_MOV,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL														},
};

CCodeGen_AArch32::CCodeGen_AArch32()
{
#ifdef __ANDROID__
	auto cpuFeatures = android_getCpuFeatures();
	if(cpuFeatures & ANDROID_CPU_ARM_FEATURE_IDIV_ARM)
	{
		m_hasIntegerDiv = true;
	}
#endif

	for(auto* constMatcher = g_constMatchers; constMatcher->emitter != nullptr; constMatcher++)
	{
		MATCHER matcher;
		matcher.op			= constMatcher->op;
		matcher.dstType		= constMatcher->dstType;
		matcher.src1Type	= constMatcher->src1Type;
		matcher.src2Type	= constMatcher->src2Type;
		matcher.emitter		= std::bind(constMatcher->emitter, this, std::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}

	for(auto* constMatcher = g_64ConstMatchers; constMatcher->emitter != nullptr; constMatcher++)
	{
		MATCHER matcher;
		matcher.op			= constMatcher->op;
		matcher.dstType		= constMatcher->dstType;
		matcher.src1Type	= constMatcher->src1Type;
		matcher.src2Type	= constMatcher->src2Type;
		matcher.emitter		= std::bind(constMatcher->emitter, this, std::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}

	for(auto* constMatcher = g_fpuConstMatchers; constMatcher->emitter != nullptr; constMatcher++)
	{
		MATCHER matcher;
		matcher.op			= constMatcher->op;
		matcher.dstType		= constMatcher->dstType;
		matcher.src1Type	= constMatcher->src1Type;
		matcher.src2Type	= constMatcher->src2Type;
		matcher.emitter		= std::bind(constMatcher->emitter, this, std::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}

	for(auto* constMatcher = g_mdConstMatchers; constMatcher->emitter != nullptr; constMatcher++)
	{
		MATCHER matcher;
		matcher.op			= constMatcher->op;
		matcher.dstType		= constMatcher->dstType;
		matcher.src1Type	= constMatcher->src1Type;
		matcher.src2Type	= constMatcher->src2Type;
		matcher.emitter		= std::bind(constMatcher->emitter, this, std::placeholders::_1);
		m_matchers.insert(MatcherMapType::value_type(matcher.op, matcher));
	}
}

CCodeGen_AArch32::~CCodeGen_AArch32()
{

}

unsigned int CCodeGen_AArch32::GetAvailableRegisterCount() const
{
	return MAX_REGISTERS;
}

unsigned int CCodeGen_AArch32::GetAvailableMdRegisterCount() const
{
	return 0;
}

bool CCodeGen_AArch32::CanHold128BitsReturnValueInRegisters() const
{
	return false;
}

void CCodeGen_AArch32::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
	m_assembler.SetStream(stream);
}

void CCodeGen_AArch32::RegisterExternalSymbols(CObjectFile* objectFile) const
{
	objectFile->AddExternalSymbol("_CodeGen_AArch32_div_unsigned",	reinterpret_cast<uintptr_t>(&CodeGen_AArch32_div_unsigned));
	objectFile->AddExternalSymbol("_CodeGen_AArch32_div_signed",	reinterpret_cast<uintptr_t>(&CodeGen_AArch32_div_signed));
	objectFile->AddExternalSymbol("_CodeGen_AArch32_mod_unsigned",	reinterpret_cast<uintptr_t>(&CodeGen_AArch32_mod_unsigned));
	objectFile->AddExternalSymbol("_CodeGen_AArch32_mod_signed",	reinterpret_cast<uintptr_t>(&CodeGen_AArch32_mod_signed));
}

void CCodeGen_AArch32::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	//Align stack size (must be aligned on 16 bytes boundary)
	stackSize = (stackSize + 0xF) & ~0xF;

	uint16 registerSave = GetSavedRegisterList(GetRegisterUsage(statements));

	Emit_Prolog(stackSize, registerSave);

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

uint16 CCodeGen_AArch32::GetSavedRegisterList(uint32 registerUsage)
{
	uint16 registerSave = 0;
	for(unsigned int i = 0; i < MAX_REGISTERS; i++)
	{
		if((1 << i) & registerUsage)
		{
			registerSave |= (1 << g_registers[i]);
		}
	}
	registerSave |= (1 << g_callAddressRegister);
	registerSave |= (1 << g_baseRegister);
	registerSave |= (1 << CAArch32Assembler::rLR);
	return registerSave;
}

void CCodeGen_AArch32::Emit_Prolog(unsigned int stackSize, uint16 registerSave)
{
	m_assembler.Stmdb(CAArch32Assembler::rSP, registerSave);
	m_assembler.Mov(CAArch32Assembler::r11, CAArch32Assembler::r0);

	//Align stack to 16 bytes boundary
	m_assembler.Mov(CAArch32Assembler::r0, CAArch32Assembler::rSP);
	m_assembler.Bic(CAArch32Assembler::rSP, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateAluOperand(0xF, 0));
	m_assembler.Sub(CAArch32Assembler::rSP, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateAluOperand(0xC, 0));
	m_assembler.Stmdb(CAArch32Assembler::rSP, (1 << CAArch32Assembler::r0));

	if(stackSize != 0)
	{
		uint8 allocImm = 0, allocSa = 0;
		bool succeeded = TryGetAluImmediateParams(stackSize, allocImm, allocSa);
		if(succeeded)
		{
			m_assembler.Sub(CAArch32Assembler::rSP, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateAluOperand(allocImm, allocSa));
		}
		else
		{
			auto stackResReg = CAArch32Assembler::r0;
			LoadConstantInRegister(stackResReg, stackSize);
			m_assembler.Sub(CAArch32Assembler::rSP, CAArch32Assembler::rSP, stackResReg);
		}
	}
	m_stackLevel = 0;
}

void CCodeGen_AArch32::Emit_Epilog(unsigned int stackSize, uint16 registerSave)
{
	if(stackSize != 0)
	{
		uint8 allocImm = 0, allocSa = 0;
		bool succeeded = TryGetAluImmediateParams(stackSize, allocImm, allocSa);
		if(succeeded)
		{
			m_assembler.Add(CAArch32Assembler::rSP, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateAluOperand(allocImm, allocSa));
		}
		else
		{
			auto stackResReg = CAArch32Assembler::r0;
			LoadConstantInRegister(stackResReg, stackSize);
			m_assembler.Add(CAArch32Assembler::rSP, CAArch32Assembler::rSP, stackResReg);
		}
	}

	//Restore previous unaligned SP
	m_assembler.Ldmia(CAArch32Assembler::rSP, (1 << CAArch32Assembler::r0));
	m_assembler.Mov(CAArch32Assembler::rSP, CAArch32Assembler::r0);

	m_assembler.Ldmia(CAArch32Assembler::rSP, registerSave);
	m_assembler.Bx(CAArch32Assembler::rLR);
}

uint32 CCodeGen_AArch32::RotateRight(uint32 value)
{
	uint32 carry = value & 1;
	value >>= 1;
	value |= carry << 31;
	return value;
}

uint32 CCodeGen_AArch32::RotateLeft(uint32 value)
{
	uint32 carry = value >> 31;
	value <<= 1;
	value |= carry;
	return value;
}

bool CCodeGen_AArch32::TryGetAluImmediateParams(uint32 constant, uint8& immediate, uint8& shiftAmount)
{
	uint32 shadowConstant = constant;
	shiftAmount = 0xFF;
	
	for(unsigned int i = 0; i < 16; i++)
	{
		if((shadowConstant & 0xFF) == shadowConstant)
		{
			shiftAmount = i;
			break;
		}
		shadowConstant = RotateLeft(shadowConstant);
		shadowConstant = RotateLeft(shadowConstant);
	}
	
	if(shiftAmount != 0xFF)
	{
		immediate = static_cast<uint8>(shadowConstant);
		return true;
	}
	else
	{
		return false;
	}
}

void CCodeGen_AArch32::LoadConstantInRegister(CAArch32Assembler::REGISTER registerId, uint32 constant)
{	
	//Try normal move
	{
		uint8 immediate = 0;
		uint8 shiftAmount = 0;
		if(TryGetAluImmediateParams(constant, immediate, shiftAmount))
		{
			m_assembler.Mov(registerId, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
			return;
		}
	}
	
	//Try not move
	{
		uint8 immediate = 0;
		uint8 shiftAmount = 0;
		if(TryGetAluImmediateParams(~constant, immediate, shiftAmount))
		{
			m_assembler.Mvn(registerId, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
			return;
		}
	}
		
	//Otherwise, use paired move
	m_assembler.Movw(registerId, static_cast<uint16>(constant & 0xFFFF));
	if((constant & 0xFFFF0000) != 0)
	{
		m_assembler.Movt(registerId, static_cast<uint16>(constant >> 16));
	}
}

void CCodeGen_AArch32::LoadConstantPtrInRegister(CAArch32Assembler::REGISTER registerId, uintptr_t constant)
{
	m_assembler.Movw(registerId, static_cast<uint16>(constant & 0xFFFF));
	m_assembler.Movt(registerId, static_cast<uint16>(constant >> 16));

	if(m_externalSymbolReferencedHandler)
	{
		auto position = m_stream->GetLength();
		m_externalSymbolReferencedHandler(constant, position - 8);
	}
}

void CCodeGen_AArch32::LoadMemoryInRegister(CAArch32Assembler::REGISTER registerId, CSymbol* src)
{
	switch(src->m_type)
	{
	case SYM_RELATIVE:
		assert((src->m_valueLow & 0x03) == 0x00);
		m_assembler.Ldr(registerId, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(src->m_valueLow));
		break;
	case SYM_TEMPORARY:
		m_assembler.Ldr(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(src->m_stackLocation + m_stackLevel));
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch32::StoreRegisterInMemory(CSymbol* dst, CAArch32Assembler::REGISTER registerId)
{
	switch(dst->m_type)
	{
	case SYM_RELATIVE:
		assert((dst->m_valueLow & 0x03) == 0x00);
		m_assembler.Str(registerId, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_valueLow));
		break;
	case SYM_TEMPORARY:
		m_assembler.Str(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel));
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch32::LoadMemoryReferenceInRegister(CAArch32Assembler::REGISTER registerId, CSymbol* src)
{
	switch(src->m_type)
	{
	case SYM_REL_REFERENCE:
		LoadRelativeReferenceInRegister(registerId, src);
		break;
	case SYM_TMP_REFERENCE:
		LoadTemporaryReferenceInRegister(registerId, src);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::LoadRelativeReferenceInRegister(CAArch32Assembler::REGISTER registerId, CSymbol* src)
{
	assert(src->m_type == SYM_REL_REFERENCE);
	assert((src->m_valueLow & 0x03) == 0x00);
	m_assembler.Ldr(registerId, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(src->m_valueLow));
}

void CCodeGen_AArch32::LoadTemporaryReferenceInRegister(CAArch32Assembler::REGISTER registerId, CSymbol* src)
{
	assert(src->m_type == SYM_TMP_REFERENCE);
	m_assembler.Ldr(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(src->m_stackLocation + m_stackLevel));
}

void CCodeGen_AArch32::StoreRegisterInTemporaryReference(CSymbol* dst, CAArch32Assembler::REGISTER registerId)
{
	assert(dst->m_type == SYM_TMP_REFERENCE);
	m_assembler.Str(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel));
}

CAArch32Assembler::AluLdrShift CCodeGen_AArch32::GetAluShiftFromSymbol(CAArch32Assembler::SHIFT shiftType, CSymbol* symbol, CAArch32Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		m_assembler.And(preferedRegister, g_registers[symbol->m_valueLow], CAArch32Assembler::MakeImmediateAluOperand(0x1F, 0));
		return CAArch32Assembler::MakeVariableShift(shiftType, preferedRegister);
		break;
	case SYM_TEMPORARY:
	case SYM_RELATIVE:
		LoadMemoryInRegister(preferedRegister, symbol);
		m_assembler.And(preferedRegister, preferedRegister, CAArch32Assembler::MakeImmediateAluOperand(0x1F, 0));
		return CAArch32Assembler::MakeVariableShift(shiftType, preferedRegister);
		break;
	case SYM_CONSTANT:
		return CAArch32Assembler::MakeConstantShift(shiftType, static_cast<uint8>(symbol->m_valueLow & 0x1F));
		break;
	default:
		throw std::runtime_error("Invalid symbol type.");
		break;
	}
}

CAArch32Assembler::REGISTER CCodeGen_AArch32::PrepareSymbolRegisterDef(CSymbol* symbol, CAArch32Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
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

CAArch32Assembler::REGISTER CCodeGen_AArch32::PrepareSymbolRegisterUse(CSymbol* symbol, CAArch32Assembler::REGISTER preferedRegister)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
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

void CCodeGen_AArch32::CommitSymbolRegister(CSymbol* symbol, CAArch32Assembler::REGISTER usedRegister)
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

CAArch32Assembler::REGISTER CCodeGen_AArch32::PrepareParam(PARAM_STATE& paramState)
{
	assert(!paramState.prepared);
	paramState.prepared = true;
	if(paramState.index < MAX_PARAM_REGS)
	{
		return g_paramRegs[paramState.index];
	}
	else
	{
		return g_tempParamRegister0;
	}
}

CCodeGen_AArch32::ParamRegisterPair CCodeGen_AArch32::PrepareParam64(PARAM_STATE& paramState)
{
	assert(!paramState.prepared);
	paramState.prepared = true;
#ifdef __ANDROID__
	//TODO: This needs to be an ABI flag or something
	if(paramState.index & 1)
	{
		paramState.index++;
	}
#endif
	ParamRegisterPair result;
	for(unsigned int i = 0; i < 2; i++)
	{
		if((paramState.index + i) < MAX_PARAM_REGS)
		{
			result[i] = g_paramRegs[paramState.index + i];
		}
		else
		{
			result[i] = (i == 0) ? g_tempParamRegister0 : g_tempParamRegister1;
		}
	}
	return result;
}

void CCodeGen_AArch32::CommitParam(PARAM_STATE& paramState)
{
	assert(paramState.prepared);
	paramState.prepared = false;
	if(paramState.index < MAX_PARAM_REGS)
	{
		//Nothing to do
	}
	else
	{
		uint32 stackSlot = ((paramState.index - MAX_PARAM_REGS) + 1) * 4;
		m_assembler.Str(g_tempParamRegister0, CAArch32Assembler::rSP, 
			CAArch32Assembler::MakeImmediateLdrAddress(m_stackLevel - stackSlot));
	}
	paramState.index++;
}

void CCodeGen_AArch32::CommitParam64(PARAM_STATE& paramState)
{
	assert(paramState.prepared);
	paramState.prepared = false;
	uint32 stackPosition = std::max<int>((paramState.index - MAX_PARAM_REGS) + 2, 0);
	for(unsigned int i = 0; i < 2; i++)
	{
		if(paramState.index < MAX_PARAM_REGS)
		{
			//Nothing to do
		}
		else
		{
			assert(stackPosition > 0);
			auto tempParamReg = (i == 0) ? g_tempParamRegister0 : g_tempParamRegister1;
			uint32 stackSlot = i;
			if(stackPosition == 1)
			{
				//Special case for when parameters are spread between
				//registers and stack
				assert(i == 1);
				stackSlot = 0;
			}
			m_assembler.Str(tempParamReg, CAArch32Assembler::rSP,
				CAArch32Assembler::MakeImmediateLdrAddress(m_stackLevel - (stackPosition * 4) + (stackSlot * 4)));
		}
	}
	paramState.index += 2;
}

CAArch32Assembler::LABEL CCodeGen_AArch32::GetLabel(uint32 blockId)
{
	CAArch32Assembler::LABEL result;
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

void CCodeGen_AArch32::MarkLabel(const STATEMENT& statement)
{
	auto label = GetLabel(statement.jmpBlock);
	m_assembler.MarkLabel(label);
}

void CCodeGen_AArch32::Emit_Nop(const STATEMENT& statement)
{
	
}

void CCodeGen_AArch32::Emit_Param_Ctx(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONTEXT);
	
	m_params.push_back(
		[this] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam(paramState);
			m_assembler.Mov(paramReg, g_baseRegister);
			CommitParam(paramState);
		}
	);
}

void CCodeGen_AArch32::Emit_Param_Reg(const STATEMENT& statement)
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

void CCodeGen_AArch32::Emit_Param_Mem(const STATEMENT& statement)
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

void CCodeGen_AArch32::Emit_Param_Cst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	
	assert(src1->m_type == SYM_CONSTANT);
	
	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam(paramState);
			LoadConstantInRegister(paramReg, src1->m_valueLow);
			CommitParam(paramState);
		}
	);
}

void CCodeGen_AArch32::Emit_Param_Mem64(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramRegs = PrepareParam64(paramState);
			LoadMemory64InRegisters(paramRegs[0], paramRegs[1], src1);
			CommitParam64(paramState);
		}
	);
}

void CCodeGen_AArch32::Emit_Param_Cst64(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramRegs = PrepareParam64(paramState);
			LoadConstantInRegister(paramRegs[0], src1->m_valueLow);
			LoadConstantInRegister(paramRegs[1], src1->m_valueHigh);
			CommitParam64(paramState);
		}
	);
}

void CCodeGen_AArch32::Emit_Param_Mem128(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();

	m_params.push_back(
		[this, src1] (PARAM_STATE& paramState)
		{
			auto paramReg = PrepareParam(paramState);
			LoadMemory128AddressInRegister(paramReg, src1);
			CommitParam(paramState);
		}
	);
}

void CCodeGen_AArch32::Emit_ParamRet_Tmp128(const STATEMENT& statement)
{
	Emit_Param_Mem128(statement);
}

void CCodeGen_AArch32::Emit_Call(const STATEMENT& statement)
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

	uint32 stackAlloc = (paramState.index > MAX_PARAM_REGS) ? ((paramState.index - MAX_PARAM_REGS) * 4) : 0;

	if(stackAlloc != 0)
	{
		m_assembler.Sub(CAArch32Assembler::rSP, CAArch32Assembler::rSP, 
			CAArch32Assembler::MakeImmediateAluOperand(stackAlloc, 0));
	}

	//No value should be saved in r4 at this point (register is spilled before)
	LoadConstantPtrInRegister(g_callAddressRegister, src1->GetConstantPtr());
	m_assembler.Mov(CAArch32Assembler::rLR, CAArch32Assembler::rPC);
	m_assembler.Mov(CAArch32Assembler::rPC, g_callAddressRegister);

	if(stackAlloc != 0)
	{
		m_assembler.Add(CAArch32Assembler::rSP, CAArch32Assembler::rSP, 
			CAArch32Assembler::MakeImmediateAluOperand(stackAlloc, 0));
	}
}

void CCodeGen_AArch32::Emit_RetVal_Reg(const STATEMENT& statement)
{	
	auto dst = statement.dst->GetSymbol().get();
	
	assert(dst->m_type == SYM_REGISTER);
	
	m_assembler.Mov(g_registers[dst->m_valueLow], CAArch32Assembler::r0);
}

void CCodeGen_AArch32::Emit_RetVal_Tmp(const STATEMENT& statement)
{	
	auto dst = statement.dst->GetSymbol().get();
	
	assert(dst->m_type == SYM_TEMPORARY);
	
	StoreRegisterInMemory(dst, CAArch32Assembler::r0);
}

void CCodeGen_AArch32::Emit_RetVal_Mem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();

	StoreRegistersInMemory64(dst, CAArch32Assembler::r0, CAArch32Assembler::r1);
}

void CCodeGen_AArch32::Emit_Mov_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.Mov(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow]);
}

void CCodeGen_AArch32::Emit_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	LoadMemoryInRegister(g_registers[dst->m_valueLow], src1);
}

void CCodeGen_AArch32::Emit_Mov_RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);

	LoadConstantInRegister(g_registers[dst->m_valueLow], src1->m_valueLow);
}

void CCodeGen_AArch32::Emit_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	StoreRegisterInMemory(dst, g_registers[src1->m_valueLow]);
}

void CCodeGen_AArch32::Emit_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	auto tmpReg = CAArch32Assembler::r0;
	LoadMemoryInRegister(tmpReg, src1);
	StoreRegisterInMemory(dst, tmpReg);
}

void CCodeGen_AArch32::Emit_Mov_MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	assert(src1->m_type == SYM_CONSTANT);
	
	auto tmpReg = CAArch32Assembler::r0;
	LoadConstantInRegister(tmpReg, src1->m_valueLow);
	StoreRegisterInMemory(dst, tmpReg);
}

void CCodeGen_AArch32::Emit_Lzc_VarVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Register = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);

	auto set32Label = m_assembler.CreateLabel();
	auto startCountLabel = m_assembler.CreateLabel();
	auto doneLabel = m_assembler.CreateLabel();

	m_assembler.Mov(dstRegister, src1Register);
	m_assembler.Tst(dstRegister, dstRegister);
	m_assembler.BCc(CAArch32Assembler::CONDITION_EQ, set32Label);
	m_assembler.BCc(CAArch32Assembler::CONDITION_PL, startCountLabel);

	//reverse:
	m_assembler.Mvn(dstRegister, dstRegister);
	m_assembler.Tst(dstRegister, dstRegister);
	m_assembler.BCc(CAArch32Assembler::CONDITION_EQ, set32Label);

	//startCount:
	m_assembler.MarkLabel(startCountLabel);
	m_assembler.Clz(dstRegister, dstRegister);
	m_assembler.Sub(dstRegister, dstRegister, CAArch32Assembler::MakeImmediateAluOperand(1, 0));
	m_assembler.BCc(CAArch32Assembler::CONDITION_AL, doneLabel);

	//set32:
	m_assembler.MarkLabel(set32Label);
	LoadConstantInRegister(dstRegister, 0x1F);

	//done
	m_assembler.MarkLabel(doneLabel);

	CommitSymbolRegister(dst, dstRegister);
}

void CCodeGen_AArch32::Emit_Jmp(const STATEMENT& statement)
{
	m_assembler.BCc(CAArch32Assembler::CONDITION_AL, GetLabel(statement.jmpBlock));
}

void CCodeGen_AArch32::Emit_CondJmp(const STATEMENT& statement)
{
	CAArch32Assembler::LABEL label(GetLabel(statement.jmpBlock));
	
	switch(statement.jmpCondition)
	{
		case CONDITION_EQ:
			m_assembler.BCc(CAArch32Assembler::CONDITION_EQ, label);
			break;
		case CONDITION_NE:
			m_assembler.BCc(CAArch32Assembler::CONDITION_NE, label);
			break;
		case CONDITION_LT:
			m_assembler.BCc(CAArch32Assembler::CONDITION_LT, label);
			break;
		case CONDITION_LE:
			m_assembler.BCc(CAArch32Assembler::CONDITION_LE, label);
			break;
		case CONDITION_GT:
			m_assembler.BCc(CAArch32Assembler::CONDITION_GT, label);
			break;
		case CONDITION_GE:
			m_assembler.BCc(CAArch32Assembler::CONDITION_GE, label);
			break;
		default:
			assert(0);
			break;
	}
}

void CCodeGen_AArch32::Emit_CondJmp_VarVar(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src2->m_type != SYM_CONSTANT);	//We can do better if we have a constant

	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);
	m_assembler.Cmp(src1Reg, src2Reg);
	Emit_CondJmp(statement);
}

void CCodeGen_AArch32::Emit_CondJmp_VarCst(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src2->m_type == SYM_CONSTANT);
	
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	Cmp_GenericRegCst(src1Reg, src2->m_valueLow, CAArch32Assembler::r2);
	Emit_CondJmp(statement);
}

void CCodeGen_AArch32::Cmp_GetFlag(CAArch32Assembler::REGISTER registerId, Jitter::CONDITION condition)
{
	CAArch32Assembler::ImmediateAluOperand falseOperand(CAArch32Assembler::MakeImmediateAluOperand(0, 0));
	CAArch32Assembler::ImmediateAluOperand trueOperand(CAArch32Assembler::MakeImmediateAluOperand(1, 0));
	switch(condition)
	{
		case CONDITION_EQ:
			m_assembler.MovCc(CAArch32Assembler::CONDITION_NE, registerId, falseOperand);
			m_assembler.MovCc(CAArch32Assembler::CONDITION_EQ, registerId, trueOperand);
			break;
		case CONDITION_NE:
			m_assembler.MovCc(CAArch32Assembler::CONDITION_EQ, registerId, falseOperand);
			m_assembler.MovCc(CAArch32Assembler::CONDITION_NE, registerId, trueOperand);
			break;
		case CONDITION_LT:
			m_assembler.MovCc(CAArch32Assembler::CONDITION_GE, registerId, falseOperand);
			m_assembler.MovCc(CAArch32Assembler::CONDITION_LT, registerId, trueOperand);
			break;
		case CONDITION_LE:
			m_assembler.MovCc(CAArch32Assembler::CONDITION_GT, registerId, falseOperand);
			m_assembler.MovCc(CAArch32Assembler::CONDITION_LE, registerId, trueOperand);
			break;
		case CONDITION_GT:
			m_assembler.MovCc(CAArch32Assembler::CONDITION_LE, registerId, falseOperand);
			m_assembler.MovCc(CAArch32Assembler::CONDITION_GT, registerId, trueOperand);
			break;
		case CONDITION_BL:
			m_assembler.MovCc(CAArch32Assembler::CONDITION_CS, registerId, falseOperand);
			m_assembler.MovCc(CAArch32Assembler::CONDITION_CC, registerId, trueOperand);
			break;
		case CONDITION_BE:
			m_assembler.MovCc(CAArch32Assembler::CONDITION_HI, registerId, falseOperand);
			m_assembler.MovCc(CAArch32Assembler::CONDITION_LS, registerId, trueOperand);
			break;
		case CONDITION_AB:
			m_assembler.MovCc(CAArch32Assembler::CONDITION_LS, registerId, falseOperand);
			m_assembler.MovCc(CAArch32Assembler::CONDITION_HI, registerId, trueOperand);
			break;
		default:
			assert(0);
			break;
	}
}

void CCodeGen_AArch32::Cmp_GenericRegCst(CAArch32Assembler::REGISTER src1Reg, uint32 src2, CAArch32Assembler::REGISTER src2Reg)
{
	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(TryGetAluImmediateParams(src2, immediate, shiftAmount))
	{
		m_assembler.Cmp(src1Reg, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else if(TryGetAluImmediateParams(-static_cast<int32>(src2), immediate, shiftAmount))
	{
		m_assembler.Cmn(src1Reg, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		assert(src1Reg != src2Reg);
		LoadConstantInRegister(src2Reg, src2);
		m_assembler.Cmp(src1Reg, src2Reg);
	}
}

void CCodeGen_AArch32::Emit_Cmp_AnyAnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r2);

	m_assembler.Cmp(src1Reg, src2Reg);
	Cmp_GetFlag(dstReg, statement.jmpCondition);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Cmp_AnyAnyCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r1);
	auto cst = src2->m_valueLow;

	Cmp_GenericRegCst(src1Reg, cst, CAArch32Assembler::r2);
	Cmp_GetFlag(dstReg, statement.jmpCondition);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Not_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);

	m_assembler.Mvn(g_registers[dst->m_valueLow], g_registers[src1->m_valueLow]);
}

void CCodeGen_AArch32::Emit_Not_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	
	assert(src1->m_type == SYM_REGISTER);
	
	auto dstReg = CAArch32Assembler::r1;
	m_assembler.Mvn(dstReg, g_registers[src1->m_valueLow]);
	StoreRegisterInMemory(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Not_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto srcReg = CAArch32Assembler::r0;
	auto dstReg = CAArch32Assembler::r1;
	LoadMemoryInRegister(srcReg, src1);
	m_assembler.Mvn(dstReg, srcReg);
	StoreRegisterInMemory(dst, dstReg);
}

void CCodeGen_AArch32::Emit_RelToRef_TmpCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	auto tmpReg = CAArch32Assembler::r0;

	uint8 immediate = 0;
	uint8 shiftAmount = 0;

	if(TryGetAluImmediateParams(src1->m_valueLow, immediate, shiftAmount))
	{
		m_assembler.Add(tmpReg, g_baseRegister, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}
	else
	{
		LoadConstantInRegister(tmpReg, src1->m_valueLow);
		m_assembler.Add(tmpReg, tmpReg, g_baseRegister);
	}

	StoreRegisterInTemporaryReference(dst, tmpReg);
}

void CCodeGen_AArch32::Emit_AddRef_TmpMemAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	auto tmpReg = CAArch32Assembler::r0;
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);

	LoadMemoryReferenceInRegister(tmpReg, src1);
	m_assembler.Add(tmpReg, tmpReg, src2Reg);
	StoreRegisterInTemporaryReference(dst, tmpReg);
}

void CCodeGen_AArch32::Emit_LoadFromRef_VarTmp(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
		
	auto addressReg = CAArch32Assembler::r0;
	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r1);

	LoadTemporaryReferenceInRegister(addressReg, src1);
	m_assembler.Ldr(dstReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(0));

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_StoreAtRef_TmpAny(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();
	
	assert(src1->m_type == SYM_TMP_REFERENCE);
	
	auto addressReg = CAArch32Assembler::r0;
	auto valueReg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);
	
	LoadTemporaryReferenceInRegister(addressReg, src1);
	m_assembler.Str(valueReg, addressReg, CAArch32Assembler::MakeImmediateLdrAddress(0));
}
