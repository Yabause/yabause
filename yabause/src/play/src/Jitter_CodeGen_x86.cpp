#include <functional>
#include <array>
#include <assert.h>
#ifdef _WIN32
#include <intrin.h>
#endif
#include "Jitter_CodeGen_x86.h"

using namespace Jitter;

#include "Jitter_CodeGen_x86_Alu.h"
#include "Jitter_CodeGen_x86_Shift.h"
#include "Jitter_CodeGen_x86_Mul.h"
#include "Jitter_CodeGen_x86_Div.h"

CCodeGen_x86::CONSTMATCHER CCodeGen_x86::g_constMatchers[] = 
{ 
	{ OP_LABEL,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86::MarkLabel							},

	{ OP_NOP,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86::Emit_Nop								},

	ALU_CONST_MATCHERS(OP_ADD, ALUOP_ADD)
	ALU_CONST_MATCHERS(OP_SUB, ALUOP_SUB)
	ALU_CONST_MATCHERS(OP_AND, ALUOP_AND)
	ALU_CONST_MATCHERS(OP_OR,  ALUOP_OR)
	ALU_CONST_MATCHERS(OP_XOR, ALUOP_XOR)

	{ OP_CMP,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Cmp_RegRegReg					},
	{ OP_CMP,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Cmp_RegRegMem					},
	{ OP_CMP,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Cmp_RegRegCst					},
	{ OP_CMP,		MATCH_REGISTER,		MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Cmp_RegMemMem					},
	{ OP_CMP,		MATCH_REGISTER,		MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Cmp_RegMemCst					},
	{ OP_CMP,		MATCH_MEMORY,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Cmp_MemRegReg					},
	{ OP_CMP,		MATCH_MEMORY,		MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Cmp_MemRegMem					},
	{ OP_CMP,		MATCH_MEMORY,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Cmp_MemRegCst					},
	{ OP_CMP,		MATCH_MEMORY,		MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Cmp_MemMemMem					},
	{ OP_CMP,		MATCH_MEMORY,		MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Cmp_MemMemCst					},
	
	{ OP_NOT,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86::Emit_Not_RegReg						},
	{ OP_NOT,		MATCH_REGISTER,		MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_x86::Emit_Not_RegMem						},
	{ OP_NOT,		MATCH_MEMORY,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86::Emit_Not_MemReg						},
	{ OP_NOT,		MATCH_MEMORY,		MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_x86::Emit_Not_MemMem						},

	{ OP_LZC,		MATCH_REGISTER,		MATCH_VARIABLE,		MATCH_NIL,			&CCodeGen_x86::Emit_Lzc_RegVar						},
	{ OP_LZC,		MATCH_MEMORY,		MATCH_VARIABLE,		MATCH_NIL,			&CCodeGen_x86::Emit_Lzc_MemVar						},

	SHIFT_CONST_MATCHERS(OP_SRL, SHIFTOP_SRL)
	SHIFT_CONST_MATCHERS(OP_SRA, SHIFTOP_SRA)
	SHIFT_CONST_MATCHERS(OP_SLL, SHIFTOP_SLL)

	{ OP_MOV,		MATCH_REGISTER,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RegReg						},
	{ OP_MOV,		MATCH_REGISTER,		MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RegMem						},
	{ OP_MOV,		MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_RegCst						},
	{ OP_MOV,		MATCH_MEMORY,		MATCH_REGISTER,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_MemReg						},
	{ OP_MOV,		MATCH_MEMORY,		MATCH_MEMORY,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_MemMem						},
	{ OP_MOV,		MATCH_MEMORY,		MATCH_CONSTANT,		MATCH_NIL,			&CCodeGen_x86::Emit_Mov_MemCst						},
	
	{ OP_JMP,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			&CCodeGen_x86::Emit_Jmp								},

	{ OP_CONDJMP,	MATCH_NIL,			MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_CondJmp_RegReg					},
	{ OP_CONDJMP,	MATCH_NIL,			MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_CondJmp_RegMem					},
	{ OP_CONDJMP,	MATCH_NIL,			MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_CondJmp_RegCst					},
	{ OP_CONDJMP,	MATCH_NIL,			MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_CondJmp_MemMem					},
	{ OP_CONDJMP,	MATCH_NIL,			MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_CondJmp_MemCst					},

	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64RegReg<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64RegMem<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_DivTmp64RegCst<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64MemReg<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64MemMem<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_DivTmp64MemCst<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64CstReg<false>			},
	{ OP_DIV,		MATCH_TEMPORARY64,	MATCH_CONSTANT,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64CstMem<false>			},

	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64RegReg<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64RegMem<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_DivTmp64RegCst<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64MemReg<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64MemMem<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_DivTmp64MemCst<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_DivTmp64CstReg<true>			},
	{ OP_DIVS,		MATCH_TEMPORARY64,	MATCH_CONSTANT,		MATCH_MEMORY,		&CCodeGen_x86::Emit_DivTmp64CstMem<true>			},

	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MulTmp64RegReg<false>			},
	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MulTmp64RegMem<false>			},
	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_MulTmp64RegCst<false>			},
	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MulTmp64MemMem<false>			},
	{ OP_MUL,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_MulTmp64MemCst<false>			},

	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MulTmp64RegReg<true>			},
	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MulTmp64RegMem<true>			},
	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_MulTmp64RegCst<true>			},
	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MulTmp64MemMem<true>			},
	{ OP_MULS,		MATCH_TEMPORARY64,	MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_MulTmp64MemCst<true>			},

	{ OP_MERGETO64,	MATCH_MEMORY64,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MergeTo64_Mem64RegReg			},
	{ OP_MERGETO64,	MATCH_MEMORY64,		MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MergeTo64_Mem64RegMem			},
	{ OP_MERGETO64,	MATCH_MEMORY64,		MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MergeTo64_Mem64MemMem			},
	{ OP_MERGETO64,	MATCH_MEMORY64,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_MergeTo64_Mem64CstReg			},
	{ OP_MERGETO64,	MATCH_MEMORY64,		MATCH_CONSTANT,		MATCH_MEMORY,		&CCodeGen_x86::Emit_MergeTo64_Mem64CstMem			},

	{ OP_EXTLOW64,	MATCH_REGISTER,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtLow64RegTmp64				},
	{ OP_EXTLOW64,	MATCH_MEMORY,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtLow64MemTmp64				},
	
	{ OP_EXTHIGH64,	MATCH_REGISTER,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtHigh64RegTmp64				},
	{ OP_EXTHIGH64,	MATCH_MEMORY,		MATCH_TEMPORARY64,	MATCH_NIL,			&CCodeGen_x86::Emit_ExtHigh64MemTmp64				},

	{ OP_MOV,		MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL												},
};

CCodeGen_x86::CCodeGen_x86()
{
	SetGenerationFlags();

	InsertMatchers(g_constMatchers);
	InsertMatchers(g_fpuConstMatchers);
	InsertMatchers(g_mdConstMatchers);

	if(m_hasSse41)
	{
		InsertMatchers(g_mdMinMaxWSse41ConstMatchers);
	}
	else
	{
		InsertMatchers(g_mdMinMaxWConstMatchers);
	}
}

CCodeGen_x86::~CCodeGen_x86()
{

}

void CCodeGen_x86::GenerateCode(const StatementList& statements, unsigned int stackSize)
{
	assert(m_registers != nullptr);
	assert(m_mdRegisters != nullptr);

	uint32 registerUsage = GetRegisterUsage(statements);
	
	//Align stacksize
	stackSize = (stackSize + 0xF) & ~0xF;
	m_stackLevel = 0;

	m_assembler.Begin();
	{
		CX86Assembler::LABEL rootLabel = m_assembler.CreateLabel();
		m_assembler.MarkLabel(rootLabel);

		Emit_Prolog(statements, stackSize, registerUsage);

		for(const auto& statement : statements)
		{
			bool found = false;
			auto begin = m_matchers.lower_bound(statement.op);
			auto end = m_matchers.upper_bound(statement.op);

			for(auto matchIterator(begin); matchIterator != end; matchIterator++)
			{
				const auto& matcher(matchIterator->second);
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
				throw std::exception();
			}
		}

		Emit_Epilog(stackSize, registerUsage);
	}
	m_assembler.End();

	if(m_externalSymbolReferencedHandler)
	{
		for(const auto& symbolRefLabel : m_symbolReferenceLabels)
		{
			uint32 offset = m_assembler.GetLabelOffset(symbolRefLabel.second);
			m_externalSymbolReferencedHandler(symbolRefLabel.first, offset);
		}
	}

	m_labels.clear();
	m_symbolReferenceLabels.clear();
}

void CCodeGen_x86::InsertMatchers(const CONSTMATCHER* constMatchers)
{
	for(auto* constMatcher = constMatchers; constMatcher->emitter != nullptr; constMatcher++)
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

void CCodeGen_x86::SetGenerationFlags()
{
#ifdef _WIN32
	static uint32 CPUID_FLAG_SSE41 = 0x080000;
	std::array<int, 4> cpuInfo;
	__cpuid(cpuInfo.data(), 1);
	m_hasSse41 = (cpuInfo[2] & CPUID_FLAG_SSE41) != 0;
#endif
}

void CCodeGen_x86::SetStream(Framework::CStream* stream)
{
	m_assembler.SetStream(stream);
}

void CCodeGen_x86::RegisterExternalSymbols(CObjectFile*) const
{
	//Nothing to register
}

CX86Assembler::LABEL CCodeGen_x86::GetLabel(uint32 blockId)
{
	CX86Assembler::LABEL result;
	LabelMapType::const_iterator labelIterator(m_labels.find(blockId));
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

CX86Assembler::CAddress CCodeGen_x86::MakeTemporarySymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY);
	assert(((symbol->m_stackLocation + m_stackLevel) & 3) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel);
}

CX86Assembler::CAddress CCodeGen_x86::MakeRelativeSymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE);
	assert((symbol->m_valueLow & 3) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemorySymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE:
		return MakeRelativeSymbolAddress(symbol);
		break;
	case SYM_TEMPORARY:
		return MakeTemporarySymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeVariableSymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_REGISTER:
		return CX86Assembler::MakeRegisterAddress(m_registers[symbol->m_valueLow]);
		break;
	case SYM_RELATIVE:
		return MakeRelativeSymbolAddress(symbol);
		break;
	case SYM_TEMPORARY:
		return MakeTemporarySymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeRelativeReferenceSymbolAddress(CSymbol* symbol)
{
	size_t symbolMask = sizeof(void*) - 1;
	assert(symbol->m_type == SYM_REL_REFERENCE);
	assert((symbol->m_valueLow & symbolMask) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporaryReferenceSymbolAddress(CSymbol* symbol)
{
	size_t symbolMask = sizeof(void*) - 1;
	assert(symbol->m_type == SYM_TMP_REFERENCE);
	assert(((symbol->m_stackLocation + m_stackLevel) & symbolMask) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel);
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemoryReferenceSymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_REL_REFERENCE:
		return MakeRelativeReferenceSymbolAddress(symbol);
		break;
	case SYM_TMP_REFERENCE:
		return MakeTemporaryReferenceSymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeRelative64SymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE64);
	assert((symbol->m_valueLow & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow);
}

CX86Assembler::CAddress CCodeGen_x86::MakeRelative64SymbolLoAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE64);
	assert((symbol->m_valueLow & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow + 0);
}

CX86Assembler::CAddress CCodeGen_x86::MakeRelative64SymbolHiAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_RELATIVE64);
	assert((symbol->m_valueLow & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rBP, symbol->m_valueLow + 4);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary64SymbolAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY64);
	assert(((symbol->m_stackLocation + m_stackLevel) & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary64SymbolLoAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY64);
	assert(((symbol->m_stackLocation + m_stackLevel) & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + 0);
}

CX86Assembler::CAddress CCodeGen_x86::MakeTemporary64SymbolHiAddress(CSymbol* symbol)
{
	assert(symbol->m_type == SYM_TEMPORARY64);
	assert(((symbol->m_stackLocation + m_stackLevel) & 7) == 0);
	return CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, symbol->m_stackLocation + m_stackLevel + 4);
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemory64SymbolAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		return MakeRelative64SymbolAddress(symbol);
		break;
	case SYM_TEMPORARY64:
		return MakeTemporary64SymbolAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemory64SymbolLoAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		return MakeRelative64SymbolLoAddress(symbol);
		break;
	case SYM_TEMPORARY64:
		return MakeTemporary64SymbolLoAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

CX86Assembler::CAddress CCodeGen_x86::MakeMemory64SymbolHiAddress(CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		return MakeRelative64SymbolHiAddress(symbol);
		break;
	case SYM_TEMPORARY64:
		return MakeTemporary64SymbolHiAddress(symbol);
		break;
	default:
		throw std::exception();
		break;
	}
}

void CCodeGen_x86::MarkLabel(const STATEMENT& statement)
{
	CX86Assembler::LABEL label = GetLabel(statement.jmpBlock);
	m_assembler.MarkLabel(label);
}

void CCodeGen_x86::Emit_Nop(const STATEMENT& statement)
{
	
}

void CCodeGen_x86::Emit_Not_RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

void CCodeGen_x86::Emit_Not_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_REGISTER);

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

void CCodeGen_x86::Emit_Not_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Not_MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Lzc(CX86Assembler::REGISTER dstRegister, const CX86Assembler::CAddress& srcAddress)
{
	auto set32Label = m_assembler.CreateLabel();
	auto startCountLabel = m_assembler.CreateLabel();
	auto doneLabel = m_assembler.CreateLabel();

	auto tmpRegister = CX86Assembler::rAX;

	m_assembler.MovEd(tmpRegister, srcAddress);
	m_assembler.TestEd(tmpRegister, CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.JzJx(set32Label);
	m_assembler.JnsJx(startCountLabel);

	//reverse:
	m_assembler.NotEd(CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.TestEd(tmpRegister, CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.JzJx(set32Label);

	//startCount:
	m_assembler.MarkLabel(startCountLabel);
	m_assembler.BsrEd(dstRegister, CX86Assembler::MakeRegisterAddress(tmpRegister));
	m_assembler.NegEd(CX86Assembler::MakeRegisterAddress(dstRegister));
	m_assembler.AddId(CX86Assembler::MakeRegisterAddress(dstRegister), 0x1E);
	m_assembler.JmpJx(doneLabel);

	//set32
	m_assembler.MarkLabel(set32Label);
	m_assembler.MovId(dstRegister, 0x1F);

	//done
	m_assembler.MarkLabel(doneLabel);
}

void CCodeGen_x86::Emit_Lzc_RegVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	Emit_Lzc(m_registers[dst->m_valueLow], MakeVariableSymbolAddress(src1));
}

void CCodeGen_x86::Emit_Lzc_MemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstRegister = CX86Assembler::rAX;

	Emit_Lzc(dstRegister, MakeVariableSymbolAddress(src1));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), dstRegister);
}

void CCodeGen_x86::Emit_Mov_RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(!dst->Equals(src1));

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
}

void CCodeGen_x86::Emit_Mov_RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
}

void CCodeGen_x86::Emit_Mov_RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	if(src1->m_valueLow == 0)
	{
		m_assembler.XorEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
	}
	else
	{
		m_assembler.MovId(m_registers[dst->m_valueLow], src1->m_valueLow);
	}
}

void CCodeGen_x86::Emit_Mov_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovGd(MakeMemorySymbolAddress(dst), m_registers[src1->m_valueLow]);
}

void CCodeGen_x86::Emit_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Mov_MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	m_assembler.MovId(MakeMemorySymbolAddress(dst), src1->m_valueLow);
}

void CCodeGen_x86::Emit_Jmp(const STATEMENT& statement)
{
	m_assembler.JmpJx(GetLabel(statement.jmpBlock));
}

void CCodeGen_x86::Emit_MergeTo64_Mem64RegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), m_registers[src1->m_valueLow]);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), m_registers[src2->m_valueLow]);
}

void CCodeGen_x86::Emit_MergeTo64_Mem64RegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rDX, MakeMemorySymbolAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), m_registers[src1->m_valueLow]);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86::Emit_MergeTo64_Mem64MemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemorySymbolAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86::Emit_MergeTo64_Mem64CstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), m_registers[src2->m_valueLow]);
}

void CCodeGen_x86::Emit_MergeTo64_Mem64CstMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.MovEd(CX86Assembler::rDX, MakeMemorySymbolAddress(src2));

	m_assembler.MovGd(MakeMemory64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeMemory64SymbolHiAddress(dst), CX86Assembler::rDX);
}

void CCodeGen_x86::Emit_ExtLow64RegTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 0));
}

void CCodeGen_x86::Emit_ExtLow64MemTmp64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 0));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_ExtHigh64RegTmp64(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 4));
}

void CCodeGen_x86::Emit_ExtHigh64MemTmp64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY64);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeIndRegOffAddress(CX86Assembler::rSP, src1->m_stackLocation + m_stackLevel + 4));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Cmp_GetFlag(const CX86Assembler::CAddress& dst, CONDITION flag)
{
	switch(flag)
	{
	case CONDITION_LT:
		m_assembler.SetlEb(dst);
		break;
	case CONDITION_LE:
		m_assembler.SetleEb(dst);
		break;
	case CONDITION_GT:
		m_assembler.SetgEb(dst);
		break;
	case CONDITION_EQ:
		m_assembler.SeteEb(dst);
		break;
	case CONDITION_NE:
		m_assembler.SetneEb(dst);
		break;
	case CONDITION_BL:
		m_assembler.SetbEb(dst);
		break;
	case CONDITION_AB:
		m_assembler.SetaEb(dst);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_x86::Emit_Cmp_RegRegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.CmpEd(m_registers[src1->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(m_registers[dst->m_valueLow], CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86::Emit_Cmp_RegRegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.CmpEd(m_registers[src1->m_valueLow], MakeMemorySymbolAddress(src2));
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(m_registers[dst->m_valueLow], CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86::Emit_Cmp_RegRegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]), src2->m_valueLow);
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(m_registers[dst->m_valueLow], CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86::Emit_Cmp_RegMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.CmpEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src2));
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(m_registers[dst->m_valueLow], CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86::Emit_Cmp_RegMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.CmpId(MakeMemorySymbolAddress(src1), src2->m_valueLow);
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(m_registers[dst->m_valueLow], CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX));
}

void CCodeGen_x86::Emit_Cmp_MemRegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.CmpEd(m_registers[src1->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(CX86Assembler::rAX, CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Cmp_MemRegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.CmpEd(m_registers[src1->m_valueLow], MakeMemorySymbolAddress(src2));
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(CX86Assembler::rAX, CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Cmp_MemRegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]), src2->m_valueLow);
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(CX86Assembler::rAX, CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Cmp_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.CmpEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src2));
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(CX86Assembler::rAX, CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::Emit_Cmp_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.CmpId(MakeMemorySymbolAddress(src1), src2->m_valueLow);
	Cmp_GetFlag(CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX), statement.jmpCondition);
	m_assembler.MovzxEb(CX86Assembler::rAX, CX86Assembler::MakeByteRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

void CCodeGen_x86::CondJmp_JumpTo(CX86Assembler::LABEL label, Jitter::CONDITION condition)
{
	switch(condition)
	{
	case CONDITION_EQ:
		m_assembler.JzJx(label);
		break;
	case CONDITION_NE:
		m_assembler.JnzJx(label);
		break;
	case CONDITION_LT:
		m_assembler.JlJx(label);
		break;
	case CONDITION_LE:
		m_assembler.JleJx(label);
		break;
	case CONDITION_GT:
		m_assembler.JnleJx(label);
		break;
	case CONDITION_GE:
		m_assembler.JnlJx(label);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_x86::Emit_CondJmp_RegReg(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.CmpEd(m_registers[src1->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

void CCodeGen_x86::Emit_CondJmp_RegMem(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.CmpEd(m_registers[src1->m_valueLow], MakeMemorySymbolAddress(src2));

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

void CCodeGen_x86::Emit_CondJmp_RegCst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	if((src2->m_valueLow == 0) && (statement.jmpCondition == CONDITION_NE || statement.jmpCondition == CONDITION_EQ))
	{
		m_assembler.TestEd(m_registers[src1->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]), src2->m_valueLow);
	}

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

void CCodeGen_x86::Emit_CondJmp_MemMem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.CmpEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src2));

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}

void CCodeGen_x86::Emit_CondJmp_MemCst(const STATEMENT& statement)
{
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	
	if((src2->m_valueLow == 0) && (statement.jmpCondition == CONDITION_NE || statement.jmpCondition == CONDITION_EQ))
	{
		m_assembler.TestEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	}
	else
	{
		m_assembler.CmpId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	}

	CondJmp_JumpTo(GetLabel(statement.jmpBlock), statement.jmpCondition);
}
