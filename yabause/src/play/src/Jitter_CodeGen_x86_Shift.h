#ifndef _JITTER_CODEGEN_X86_SHIFT_H_
#define _JITTER_CODEGEN_X86_SHIFT_H_

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rCX, MakeMemorySymbolAddress(src2));

	if(!dst->Equals(src1))
	{
		m_assembler.MovEd(m_registers[dst->m_valueLow], CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}

	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegRegCst(const STATEMENT& statement)
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

	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), static_cast<uint8>(src2->m_valueLow));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegMemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rCX, MakeMemorySymbolAddress(src2));
	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(m_registers[dst->m_valueLow], MakeMemorySymbolAddress(src1));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]), static_cast<uint8>(src2->m_valueLow));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegCstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovId(m_registers[dst->m_valueLow], src1->m_valueLow);
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_RegCstMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type  == SYM_REGISTER);
	assert(src1->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rCX, MakeMemorySymbolAddress(src2));
	m_assembler.MovId(m_registers[dst->m_valueLow], src1->m_valueLow);
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(m_registers[dst->m_valueLow]));
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemRegReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemRegMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	m_assembler.MovEd(CX86Assembler::rCX, MakeMemorySymbolAddress(src2));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemRegCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemMemReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));

	if(dst->Equals(src1))
	{
		((m_assembler).*(SHIFTOP::OpVar()))(MakeMemorySymbolAddress(dst));
	}
	else
	{
		m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
		((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
		m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
	}
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemMemMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rCX, MakeMemorySymbolAddress(src2));

	if(dst->Equals(src1))
	{
		((m_assembler).*(SHIFTOP::OpVar()))(MakeMemorySymbolAddress(dst));
	}
	else
	{
		m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
		((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
		m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
	}
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemMemCst(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	if(dst->Equals(src1))
	{
		((m_assembler).*(SHIFTOP::OpCst()))(MakeMemorySymbolAddress(dst), static_cast<uint8>(src2->m_valueLow));
	}
	else
	{
		m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
		((m_assembler).*(SHIFTOP::OpCst()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX), static_cast<uint8>(src2->m_valueLow));
		m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
	}
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemCstReg(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.MovEd(CX86Assembler::rCX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

template <typename SHIFTOP>
void CCodeGen_x86::Emit_Shift_MemCstMem(const STATEMENT& statement)
{
	CSymbol* dst = statement.dst->GetSymbol().get();
	CSymbol* src1 = statement.src1->GetSymbol().get();
	CSymbol* src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	m_assembler.MovEd(CX86Assembler::rCX, MakeMemorySymbolAddress(src2));
	((m_assembler).*(SHIFTOP::OpVar()))(CX86Assembler::MakeRegisterAddress(CX86Assembler::rAX));
	m_assembler.MovGd(MakeMemorySymbolAddress(dst), CX86Assembler::rAX);
}

#define SHIFT_CONST_MATCHERS(SHIFTOP_CST, SHIFTOP) \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_RegRegReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Shift_RegRegMem<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_RegRegCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_MEMORY,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_RegMemReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Shift_RegMemMem<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_RegMemCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_RegCstReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_REGISTER,		MATCH_CONSTANT,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Shift_RegCstMem<SHIFTOP>	}, \
\
	{ SHIFTOP_CST,	MATCH_MEMORY,		MATCH_REGISTER,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_MemRegReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_MEMORY,		MATCH_REGISTER,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_MemRegCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_MEMORY,		MATCH_REGISTER,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Shift_MemRegMem<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_MEMORY,		MATCH_MEMORY,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_MemMemReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_MEMORY,		MATCH_MEMORY,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Shift_MemMemMem<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_MEMORY,		MATCH_MEMORY,		MATCH_CONSTANT,		&CCodeGen_x86::Emit_Shift_MemMemCst<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_MEMORY,		MATCH_CONSTANT,		MATCH_REGISTER,		&CCodeGen_x86::Emit_Shift_MemCstReg<SHIFTOP>	}, \
	{ SHIFTOP_CST,	MATCH_MEMORY,		MATCH_CONSTANT,		MATCH_MEMORY,		&CCodeGen_x86::Emit_Shift_MemCstMem<SHIFTOP>	}, \

#endif
