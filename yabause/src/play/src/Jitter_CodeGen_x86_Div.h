#ifndef _JITTER_CODEGEN_X86_DIV_H_
#define _JITTER_CODEGEN_X86_DIV_H_

template <bool isSigned>
void CCodeGen_x86::Emit_DivTmp64RegReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_DivTmp64RegMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(MakeMemorySymbolAddress(src2));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(MakeMemorySymbolAddress(src2));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_DivTmp64RegCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_REGISTER);
	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, CX86Assembler::MakeRegisterAddress(m_registers[src1->m_valueLow]));
	m_assembler.MovId(CX86Assembler::rCX, src2->m_valueLow);
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rCX));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rCX));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_DivTmp64MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_DivTmp64MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(MakeMemorySymbolAddress(src2));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(MakeMemorySymbolAddress(src2));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_DivTmp64MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src2->m_type == SYM_CONSTANT);

	m_assembler.MovEd(CX86Assembler::rAX, MakeMemorySymbolAddress(src1));
	m_assembler.MovId(CX86Assembler::rCX, src2->m_valueLow);
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rCX));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(CX86Assembler::MakeRegisterAddress(CX86Assembler::rCX));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_DivTmp64CstReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);
	assert(src2->m_type == SYM_REGISTER);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(CX86Assembler::MakeRegisterAddress(m_registers[src2->m_valueLow]));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

template <bool isSigned>
void CCodeGen_x86::Emit_DivTmp64CstMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_CONSTANT);

	m_assembler.MovId(CX86Assembler::rAX, src1->m_valueLow);
	if(isSigned)
	{
		m_assembler.Cdq();
		m_assembler.IdivEd(MakeMemorySymbolAddress(src2));
	}
	else
	{
		m_assembler.XorEd(CX86Assembler::rDX, CX86Assembler::MakeRegisterAddress(CX86Assembler::rDX));
		m_assembler.DivEd(MakeMemorySymbolAddress(src2));
	}
	m_assembler.MovGd(MakeTemporary64SymbolLoAddress(dst), CX86Assembler::rAX);
	m_assembler.MovGd(MakeTemporary64SymbolHiAddress(dst), CX86Assembler::rDX);
}

#endif
