#include <assert.h>
#include "Jitter.h"
#include "PtrMacro.h"
#include "placeholder_def.h"

using namespace std;
using namespace Jitter;

CJitter::CJitter(CCodeGen* codeGen)
: m_codeGen(codeGen)
{

}

CJitter::~CJitter()
{
	delete m_codeGen;
}

CCodeGen* CJitter::GetCodeGen()
{
	return m_codeGen;
}

void CJitter::SetStream(Framework::CStream* stream)
{
	m_codeGen->SetStream(stream);
}

void CJitter::Begin()
{
	assert(m_blockStarted == false);
	m_blockStarted = true;
	m_nextTemporary = 1;
	m_nextBlockId = 1;
	m_basicBlocks.clear();

	StartBlock(m_nextBlockId++);
}

void CJitter::End()
{
	assert(m_shadow.GetCount() == 0);
	assert(m_blockStarted == true);
	m_blockStarted = false;

	Compile();
}

bool CJitter::IsStackEmpty() const
{
	return m_shadow.GetCount() == 0;
}

void CJitter::StartBlock(uint32 blockId)
{
	auto blockIterator = m_basicBlocks.emplace(m_basicBlocks.end(), BASIC_BLOCK());
	m_currentBlock = &(*blockIterator);
	m_currentBlock->id = blockId;
}

CJitter::LABEL CJitter::CreateLabel()
{
	return m_nextLabelId++;
}

void CJitter::MarkLabel(LABEL label)
{
	uint32 newBlockId = m_nextBlockId++;
	StartBlock(newBlockId);
	m_labels[label] = newBlockId;
}

void CJitter::Goto(LABEL label)
{
	assert(m_shadow.GetCount() == 0);

	STATEMENT statement;
	statement.op		= OP_GOTO;
	statement.jmpBlock	= label;
	InsertStatement(statement);
}

CONDITION CJitter::GetReverseCondition(CONDITION condition)
{
	switch(condition)
	{
	case CONDITION_EQ:
		return CONDITION_NE;
		break;
	case CONDITION_NE:
		return CONDITION_EQ;
		break;
	case CONDITION_LT:
		return CONDITION_GE;
		break;
	case CONDITION_LE:
		return CONDITION_GT;
		break;
	case CONDITION_GT:
		return CONDITION_LE;
		break;
	default:
		assert(0);
		break;
	}
	throw std::exception();
}

void CJitter::BeginIf(CONDITION condition)
{
	uint32 jumpBlockId = m_nextBlockId++;
	m_ifStack.push(jumpBlockId);

	STATEMENT statement;
	statement.op			= OP_CONDJMP;
	statement.src2			= MakeSymbolRef(m_shadow.Pull());
	statement.src1			= MakeSymbolRef(m_shadow.Pull());
	statement.jmpCondition	= GetReverseCondition(condition);
	statement.jmpBlock		= jumpBlockId;
	InsertStatement(statement);

	assert(m_shadow.GetCount() == 0);

	uint32 newBlockId = m_nextBlockId++;
	StartBlock(newBlockId);
}

void CJitter::Else()
{
	assert(!m_ifStack.empty());
	assert(m_shadow.GetCount() == 0);

	uint32 nextBlockId = m_ifStack.top(); m_ifStack.pop();

	uint32 jumpBlockId = m_nextBlockId++;
	m_ifStack.push(jumpBlockId);

	STATEMENT statement;
	statement.op			= OP_JMP;
	statement.jmpBlock		= jumpBlockId;
	InsertStatement(statement);

	StartBlock(nextBlockId);
}

void CJitter::EndIf()
{
	assert(!m_ifStack.empty());
	assert(m_shadow.GetCount() == 0);

	uint32 nextBlockId = m_ifStack.top(); m_ifStack.pop();
	StartBlock(nextBlockId);
}

void CJitter::PushCtx()
{
	m_shadow.Push(MakeSymbol(SYM_CONTEXT, 0));
}

void CJitter::PushCst(uint32 nValue)
{
	m_shadow.Push(MakeSymbol(SYM_CONSTANT, nValue));
}

void CJitter::PushRel(size_t nOffset)
{
	m_shadow.Push(MakeSymbol(SYM_RELATIVE, static_cast<uint32>(nOffset)));
}

void CJitter::PushIdx(unsigned int nIndex)
{
	m_shadow.Push(m_shadow.GetAt(nIndex));
}

void CJitter::PushTop()
{
	PushIdx(0);
}

void CJitter::PullRel(size_t nOffset)
{
	STATEMENT statement;
	statement.op		= OP_MOV;
	statement.src1		= MakeSymbolRef(m_shadow.Pull());
	statement.dst		= MakeSymbolRef(MakeSymbol(SYM_RELATIVE, static_cast<uint32>(nOffset)));
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == GetSymbolSize(statement.dst));
}

void CJitter::PullTop()
{
	m_shadow.Pull();
}

void CJitter::Swap()
{
	SymbolPtr symbol1 = m_shadow.Pull();
	SymbolPtr symbol2 = m_shadow.Pull();
	m_shadow.Push(symbol1);
	m_shadow.Push(symbol2);
}

void CJitter::Add()
{
	InsertBinaryStatement(OP_ADD);
}

void CJitter::And()
{
	InsertBinaryStatement(OP_AND);
}

void CJitter::Call(void* func, unsigned int paramCount, bool keepRet)
{
	Call(func, paramCount, keepRet ? RETURN_VALUE_32 : RETURN_VALUE_NONE);
}

void CJitter::Call(void* func, unsigned int paramCount, RETURN_VALUE_TYPE returnValue)
{
	for(unsigned int i = 0; i < paramCount; i++)
	{
		STATEMENT paramStatement;
		paramStatement.src1	= MakeSymbolRef(m_shadow.Pull());
		paramStatement.op	= OP_PARAM;
		InsertStatement(paramStatement);
	}

	bool hasImplicitReturnValueParam = false;
	SymbolPtr returnValueSym;
	switch(returnValue)
	{
	case RETURN_VALUE_32:
		returnValueSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);
		break;
	case RETURN_VALUE_64:
		returnValueSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);
		break;
	case RETURN_VALUE_128:
		returnValueSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);
		if(!m_codeGen->CanHold128BitsReturnValueInRegisters())
		{
			STATEMENT paramStatement;
			paramStatement.src1	= MakeSymbolRef(returnValueSym);
			paramStatement.op	= OP_PARAM_RET;
			InsertStatement(paramStatement);
			paramCount++;
			hasImplicitReturnValueParam = true;
		}
		break;
	}

	STATEMENT callStatement;
	callStatement.src1 = MakeSymbolRef(MakeConstantPtr(reinterpret_cast<uintptr_t>(func)));
	callStatement.src2 = MakeSymbolRef(MakeSymbol(SYM_CONSTANT, paramCount));
	callStatement.op = OP_CALL;
	InsertStatement(callStatement);

	if(returnValue != RETURN_VALUE_NONE)
	{
		if(!hasImplicitReturnValueParam)
		{
			STATEMENT returnStatement;
			returnStatement.dst = MakeSymbolRef(returnValueSym);
			returnStatement.op	= OP_RETVAL;
			InsertStatement(returnStatement);
		}

		m_shadow.Push(returnValueSym);
	}
}

void CJitter::Cmp(CONDITION condition)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op			= OP_CMP;
	statement.src2			= MakeSymbolRef(m_shadow.Pull());
	statement.src1			= MakeSymbolRef(m_shadow.Pull());
	statement.jmpCondition	= condition;
	statement.dst			= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Div()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_DIV;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::DivS()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_DIVS;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Lookup(uint32* table)
{
	throw std::exception();
}

void CJitter::Lzc()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_LZC;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Mult()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MUL;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MultS()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MULS;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Not()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_NOT;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Or()
{
	InsertBinaryStatement(OP_OR);
}

void CJitter::SignExt()
{
	Sra(31);
}

void CJitter::SignExt8()
{
	Shl(24);
	Sra(24);
}

void CJitter::SignExt16()
{
	Shl(16);
	Sra(16);
}

void CJitter::Shl()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SLL;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Shl(uint8 nAmount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SLL;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, nAmount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Sra()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SRA;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Sra(uint8 nAmount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SRA;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, nAmount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Srl()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SRL;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Srl(uint8 nAmount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SRL;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, nAmount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Sub()
{
	InsertBinaryStatement(OP_SUB);
}

void CJitter::Xor()
{
	InsertBinaryStatement(OP_XOR);
}

//Memory Functions
//------------------------------------------------
void CJitter::PushRelRef(size_t offset)
{
	m_shadow.Push(MakeSymbol(SYM_REL_REFERENCE, static_cast<uint32>(offset)));
}

void CJitter::PushRelAddrRef(size_t offset)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TMP_REFERENCE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_RELTOREF;
	statement.src1	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, static_cast<uint32>(offset)));
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::AddRef()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TMP_REFERENCE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_ADDREF;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::LoadFromRef()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_LOADFROMREF;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::StoreAtRef()
{
	STATEMENT statement;
	statement.op	= OP_STOREATREF;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	InsertStatement(statement);
}

//64-bits
//------------------------------------------------
void CJitter::PushRel64(size_t offset)
{
	m_shadow.Push(MakeSymbol(SYM_RELATIVE64, static_cast<uint32>(offset)));
}

void CJitter::PushCst64(uint64 constant)
{
	m_shadow.Push(MakeConstant64(constant));
}

void CJitter::PullRel64(size_t offset)
{
	STATEMENT statement;
	statement.op		= OP_MOV;
	statement.src1		= MakeSymbolRef(m_shadow.Pull());
	statement.dst		= MakeSymbolRef(MakeSymbol(SYM_RELATIVE64, static_cast<uint32>(offset)));
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == GetSymbolSize(statement.dst));
}

void CJitter::MergeTo64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MERGETO64;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::ExtLow64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_EXTLOW64;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == 8);

	m_shadow.Push(tempSym);
}

void CJitter::ExtHigh64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_EXTHIGH64;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == 8);

	m_shadow.Push(tempSym);
}

void CJitter::Add64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_ADD64;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::And64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_AND64;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Cmp64(CONDITION condition)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op			= OP_CMP64;
	statement.src2			= MakeSymbolRef(m_shadow.Pull());
	statement.src1			= MakeSymbolRef(m_shadow.Pull());
	statement.jmpCondition	= condition;
	statement.dst			= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Sub64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SUB64;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Srl64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SRL64;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Srl64(uint8 nAmount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SRL64;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, nAmount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Sra64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SRA64;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Sra64(uint8 nAmount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SRA64;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, nAmount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Shl64()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SLL64;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::Shl64(uint8 nAmount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY64, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_SLL64;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, nAmount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

//Floating-Point
//------------------------------------------------
void CJitter::FP_PushCst(float constant)
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_LDCST;
	statement.src1	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, *reinterpret_cast<uint32*>(&constant)));
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_PushSingle(size_t offset)
{
	m_shadow.Push(MakeSymbol(SYM_FP_REL_SINGLE, static_cast<uint32>(offset)));
}

void CJitter::FP_PushWord(size_t offset)
{
	m_shadow.Push(MakeSymbol(SYM_FP_REL_INT32, static_cast<uint32>(offset)));
}

void CJitter::FP_PullSingle(size_t offset)
{
	STATEMENT statement;
	statement.op		= OP_MOV;
	statement.src1		= MakeSymbolRef(m_shadow.Pull());
	statement.dst		= MakeSymbolRef(MakeSymbol(SYM_FP_REL_SINGLE, static_cast<uint32>(offset)));
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == GetSymbolSize(statement.dst));
}

void CJitter::FP_PullWordTruncate(size_t offset)
{
	STATEMENT statement;
	statement.op		= OP_FP_TOINT_TRUNC;
	statement.src1		= MakeSymbolRef(m_shadow.Pull());
	statement.dst		= MakeSymbolRef(MakeSymbol(SYM_FP_REL_SINGLE, static_cast<uint32>(offset)));
	InsertStatement(statement);
}

void CJitter::FP_Add()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_ADD;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Sub()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_SUB;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Mul()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_MUL;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Div()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_DIV;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Cmp(Jitter::CONDITION condition)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op			= OP_FP_CMP;
	statement.src2			= MakeSymbolRef(m_shadow.Pull());
	statement.src1			= MakeSymbolRef(m_shadow.Pull());
	statement.dst			= MakeSymbolRef(tempSym);
	statement.jmpCondition	= condition;
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Sqrt()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_SQRT;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Rsqrt()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_RSQRT;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Rcpl()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_RCPL;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Abs()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_ABS;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Neg()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_NEG;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Min()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_MIN;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::FP_Max()
{
	SymbolPtr tempSym = MakeSymbol(SYM_FP_TMP_SINGLE, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_FP_MAX;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

//SIMD
//------------------------------------------------
void CJitter::MD_PullRel(size_t offset)
{
	STATEMENT statement;
	statement.op		= OP_MOV;
	statement.src1		= MakeSymbolRef(m_shadow.Pull());
	statement.dst		= MakeSymbolRef(MakeSymbol(SYM_RELATIVE128, static_cast<uint32>(offset)));
	InsertStatement(statement);

	assert(GetSymbolSize(statement.src1) == GetSymbolSize(statement.dst));
}

void CJitter::MD_PullRel(size_t offset, bool save0, bool save1, bool save2, bool save3)
{
	if(save0 && save1 && save2 && save3)
	{
		MD_PullRel(offset);
	}
	else
	{
		uint8 mask = 
			((save0 ? 1 : 0) << 0) | 
			((save1 ? 1 : 0) << 1) |
			((save2 ? 1 : 0) << 2) |
			((save3 ? 1 : 0) << 3);

		assert(mask != 0);

		STATEMENT statement;
		statement.op			= OP_MD_MOV_MASKED;
		statement.dst			= MakeSymbolRef(MakeSymbol(SYM_RELATIVE128, static_cast<uint32>(offset)));
		statement.src1			= MakeSymbolRef(MakeSymbol(SYM_RELATIVE128, static_cast<uint32>(offset)));
		statement.src2			= MakeSymbolRef(m_shadow.Pull());
		statement.jmpCondition	= static_cast<Jitter::CONDITION>(mask);
		InsertStatement(statement);

		assert(GetSymbolSize(statement.src1) == GetSymbolSize(statement.dst));
	}
}

void CJitter::MD_PushRel(size_t offset)
{
	m_shadow.Push(MakeSymbol(SYM_RELATIVE128, static_cast<uint32>(offset)));
}

void CJitter::MD_PushRelExpand(size_t offset)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MD_EXPAND;
	statement.src1	= MakeSymbolRef(MakeSymbol(SYM_RELATIVE, static_cast<uint32>(offset)));
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_PushCstExpand(uint32 constant)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MD_EXPAND;
	statement.src1	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, constant));
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_PushCstExpand(float value)
{
	MD_PushCstExpand(*reinterpret_cast<uint32*>(&value));
}

void CJitter::MD_LoadFromRef()
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_LOADFROMREF;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_StoreAtRef()
{
	StoreAtRef();
}

void CJitter::MD_AddB()
{
	InsertBinaryMdStatement(OP_MD_ADD_B);
}

void CJitter::MD_AddBUS()
{
	InsertBinaryMdStatement(OP_MD_ADDUS_B);
}

void CJitter::MD_AddH()
{
	InsertBinaryMdStatement(OP_MD_ADD_H);
}

void CJitter::MD_AddHSS()
{
	InsertBinaryMdStatement(OP_MD_ADDSS_H);
}

void CJitter::MD_AddHUS()
{
	InsertBinaryMdStatement(OP_MD_ADDUS_H);
}

void CJitter::MD_AddW()
{
	InsertBinaryMdStatement(OP_MD_ADD_W);
}

void CJitter::MD_AddWSS()
{
	InsertBinaryMdStatement(OP_MD_ADDSS_W);
}

void CJitter::MD_AddWUS()
{
	InsertBinaryMdStatement(OP_MD_ADDUS_W);
}

void CJitter::MD_SubB()
{
	InsertBinaryMdStatement(OP_MD_SUB_B);
}

void CJitter::MD_SubBUS()
{
	InsertBinaryMdStatement(OP_MD_SUBUS_B);
}

void CJitter::MD_SubHSS()
{
	InsertBinaryMdStatement(OP_MD_SUBSS_H);
}

void CJitter::MD_SubHUS()
{
	InsertBinaryMdStatement(OP_MD_SUBUS_H);
}

void CJitter::MD_SubH()
{
	InsertBinaryMdStatement(OP_MD_SUB_H);
}

void CJitter::MD_SubW()
{
	InsertBinaryMdStatement(OP_MD_SUB_W);
}

void CJitter::MD_SubWSS()
{
	InsertBinaryMdStatement(OP_MD_SUBSS_W);
}

void CJitter::MD_And()
{
	InsertBinaryMdStatement(OP_MD_AND);
}

void CJitter::MD_Or()
{
	InsertBinaryMdStatement(OP_MD_OR);
}

void CJitter::MD_Xor()
{
	InsertBinaryMdStatement(OP_MD_XOR);
}

void CJitter::MD_Not()
{
	InsertUnaryMdStatement(OP_MD_NOT);
}

void CJitter::MD_SllH(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MD_SLLH;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_SllW(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MD_SLLW;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_SrlH(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MD_SRLH;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_SrlW(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MD_SRLW;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_SraH(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MD_SRAH;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_SraW(uint8 amount)
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MD_SRAW;
	statement.src2	= MakeSymbolRef(MakeSymbol(SYM_CONSTANT, amount));
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_Srl256()
{
	SymbolPtr shiftAmount	= m_shadow.Pull();
	SymbolPtr src2			= m_shadow.Pull();
	SymbolPtr src1			= m_shadow.Pull();

	{
		//Operand order is reversed here because what's shifted is actually the concatenation of (src1 || src2)
		SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY256, m_nextTemporary++);

		STATEMENT statement;
		statement.op	= OP_MERGETO256;
		statement.src2	= MakeSymbolRef(src1);
		statement.src1	= MakeSymbolRef(src2);
		statement.dst	= MakeSymbolRef(tempSym);
		InsertStatement(statement);

		m_shadow.Push(tempSym);
	}

	{
		SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

		STATEMENT statement;
		statement.op	= OP_MD_SRL256;
		statement.src2	= MakeSymbolRef(shiftAmount);
		statement.src1	= MakeSymbolRef(m_shadow.Pull());
		statement.dst	= MakeSymbolRef(tempSym);
		InsertStatement(statement);

		m_shadow.Push(tempSym);
	}
}

void CJitter::MD_MinH()
{
	InsertBinaryMdStatement(OP_MD_MIN_H);
}

void CJitter::MD_MinW()
{
	InsertBinaryMdStatement(OP_MD_MIN_W);
}

void CJitter::MD_MaxH()
{
	InsertBinaryMdStatement(OP_MD_MAX_H);
}

void CJitter::MD_MaxW()
{
	InsertBinaryMdStatement(OP_MD_MAX_W);
}

void CJitter::MD_CmpEqB()
{
	InsertBinaryMdStatement(OP_MD_CMPEQ_B);
}

void CJitter::MD_CmpEqH()
{
	InsertBinaryMdStatement(OP_MD_CMPEQ_H);
}

void CJitter::MD_CmpEqW()
{
	InsertBinaryMdStatement(OP_MD_CMPEQ_W);
}

void CJitter::MD_CmpGtB()
{
	InsertBinaryMdStatement(OP_MD_CMPGT_B);
}

void CJitter::MD_CmpGtH()
{
	InsertBinaryMdStatement(OP_MD_CMPGT_H);
}

void CJitter::MD_CmpGtW()
{
	InsertBinaryMdStatement(OP_MD_CMPGT_W);
}

void CJitter::MD_UnpackLowerBH()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_LOWER_BH);
}

void CJitter::MD_UnpackLowerHW()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_LOWER_HW);
}

void CJitter::MD_UnpackLowerWD()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_LOWER_WD);
}

void CJitter::MD_UnpackUpperBH()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_UPPER_BH);
}

void CJitter::MD_UnpackUpperHW()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_UPPER_HW);
}

void CJitter::MD_UnpackUpperWD()
{
	InsertBinaryMdStatement(OP_MD_UNPACK_UPPER_WD);
}

void CJitter::MD_PackHB()
{
	InsertBinaryMdStatement(OP_MD_PACK_HB);
}

void CJitter::MD_PackWH()
{
	InsertBinaryMdStatement(OP_MD_PACK_WH);
}

void CJitter::MD_AddS()
{
	InsertBinaryMdStatement(OP_MD_ADD_S);
}

void CJitter::MD_SubS()
{
	InsertBinaryMdStatement(OP_MD_SUB_S);
}

void CJitter::MD_MulS()
{
	InsertBinaryMdStatement(OP_MD_MUL_S);
}

void CJitter::MD_DivS()
{
	InsertBinaryMdStatement(OP_MD_DIV_S);
}

void CJitter::MD_AbsS()
{
	InsertUnaryMdStatement(OP_MD_ABS_S);
}

void CJitter::MD_MinS()
{
	InsertBinaryMdStatement(OP_MD_MIN_S);
}

void CJitter::MD_MaxS()
{
	InsertBinaryMdStatement(OP_MD_MAX_S);
}

void CJitter::MD_IsZero()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MD_ISZERO;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_IsNegative()
{
	SymbolPtr tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= OP_MD_ISNEGATIVE;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::MD_ToWordTruncate()
{
	InsertUnaryMdStatement(OP_MD_TOWORD_TRUNCATE);
}

void CJitter::MD_ToSingle()
{
	InsertUnaryMdStatement(OP_MD_TOSINGLE);
}

//Generic Statement Inserters
//------------------------------------------------

void CJitter::InsertBinaryStatement(Jitter::OPERATION operation)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= operation;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::InsertUnaryMdStatement(Jitter::OPERATION operation)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= operation;
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}

void CJitter::InsertBinaryMdStatement(Jitter::OPERATION operation)
{
	auto tempSym = MakeSymbol(SYM_TEMPORARY128, m_nextTemporary++);

	STATEMENT statement;
	statement.op	= operation;
	statement.src2	= MakeSymbolRef(m_shadow.Pull());
	statement.src1	= MakeSymbolRef(m_shadow.Pull());
	statement.dst	= MakeSymbolRef(tempSym);
	InsertStatement(statement);

	m_shadow.Push(tempSym);
}
