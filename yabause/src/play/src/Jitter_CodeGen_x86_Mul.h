#ifndef _JITTER_CODEGEN_X86_MUL_H_
#define _JITTER_CODEGEN_X86_MUL_H_

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	if(isSigned)
	{
		m_assembler.ImulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.MulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src2));
	if(isSigned)
	{
		m_assembler.ImulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.MulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovId(CX86Assembler::rAX, src2->m_valueLow);
	if(isSigned)
	{
		m_assembler.ImulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	else
	{
		m_assembler.MulEd(CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src2));
	if(isSigned)
	{
		m_assembler.ImulEd(MakeMemorySymbolAddress(src1));
	}
	else
	{
		m_assembler.MulEd(MakeMemorySymbolAddress(src1));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_MulTmp64MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovId(CX86Assembler::rAX, src2->m_valueLow);
	if(isSigned)
	{
		m_assembler.ImulEd(MakeMemorySymbolAddress(src1));
	}
	else
	{
		m_assembler.MulEd(MakeMemorySymbolAddress(src1));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

#endif
