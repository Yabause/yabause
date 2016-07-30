#ifndef _JITTER_CODEGEN_X86_ALU_H_
#define _JITTER_CODEGEN_X86_ALU_H_

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	if(dst->Equals(src1))
	{
		((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	}
	else
	{
		CX86Assembler::REGISTER src2register = m_registers[src2->m_valueLow];

		if(dst->Equals(src2))
		{
			m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
			src2register = CX86Assembler::rAX;
		}

		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
		((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(src2register));
	}
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src2));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	//We can optimize here if it's equal to zero
	assert(src2->m_valueLow != 0);

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), src2->m_valueLow);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegMemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	CX86Assembler::REGISTER src2register = m_registers[src2->m_valueLow];

	if(dst->Equals(src2))
	{
		m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
		src2register = CX86Assembler::rAX;
	}

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(src2register));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src2));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), src2->m_valueLow);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegCstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(statement.op == OP_SUB);

	if(src1->m_valueLow)
	{
		m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	}

	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_RegCstMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(statement.op == OP_SUB);
	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);

	if(src1->m_valueLow)
	{
		m_assembler.MovId(m_registers[dst->m_valueLow], src1->m_valueLow);
	}
	else
	{
		m_assembler.XorEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
	}

	((m_assembler).*(ALUOP::OpEd()))(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src2));
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_MemRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_MemRegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, MakeMemorySymbolAddress(src2));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_MemRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_MemMemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_MemMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	((m_assembler).*(ALUOP::OpId()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src2->m_valueLow);
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP>
void CCodeGen_x86::Emit_Alu_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, MakeMemorySymbolAddress(src2));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP> 
void CCodeGen_x86::Emit_Alu_MemCstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovId(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), src1->m_valueLow);
	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename ALUOP> 
void CCodeGen_x86::Emit_Alu_MemCstMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(statement.op == OP_SUB);
	assert(src1->m_type == SYM_CONSTANT);

	if(src1->m_valueLow)
	{
		m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	}

	((m_assembler).*(ALUOP::OpEd()))(CX86Assembler::rAX, MakeMemorySymbolAddress(src2));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

#define ALU_CONST_MATCHERS(ALUOP_CST, ALUOP) \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_RegRegReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Alu_RegRegMem<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RegRegCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_MEMORY,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_RegMemReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Alu_RegMemMem<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_RegMemCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_RegCstReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Alu_RegCstMem<ALUOP>		}, \
\
	{ ALUOP_CST,	MATCH_MEMORY,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_MemRegReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_MEMORY,		MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Alu_MemRegMem<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_MEMORY,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_MemRegCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_MEMORY,		MATCH_MEMORY,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_MemMemReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_MEMORY,		MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Alu_MemMemMem<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_MEMORY,		MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Alu_MemMemCst<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_MEMORY,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Alu_MemCstReg<ALUOP>		}, \
	{ ALUOP_CST,	MATCH_MEMORY,		MATCH_CONSTANT,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Alu_MemCstMem<ALUOP>		}, \

#endif
