#pragma once

extern "C" uint32 CodeGen_AArch32_div_unsigned(uint32 a, uint32 b)
{
	return a / b;
}

extern "C" int32 CodeGen_AArch32_div_signed(int32 a, int32 b)
{
	return a / b;
}

extern "C" uint32 CodeGen_AArch32_mod_unsigned(uint32 a, uint32 b)
{
	return a % b;
}

extern "C" int32 CodeGen_AArch32_mod_signed(int32 a, int32 b)
{
	return a % b;
}

template <bool isSigned>
void CCodeGen_AArch32::Div_GenericTmp64AnyAnySoft(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto divFct = isSigned ? 
		reinterpret_cast<uintptr_t>(&CodeGen_AArch32_div_signed) : reinterpret_cast<uintptr_t>(&CodeGen_AArch32_div_unsigned);
	auto modFct = isSigned ? 
		reinterpret_cast<uintptr_t>(&CodeGen_AArch32_mod_signed) : reinterpret_cast<uintptr_t>(&CodeGen_AArch32_mod_unsigned);

	assert(dst->m_type == SYM_TEMPORARY64);

	//Quotient
	{
		auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r0);
		auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);

		if(src1Reg != CAArch32Assembler::r0)
		{
			m_assembler.Mov(CAArch32Assembler::r0, src1Reg);
		}

		if(src2Reg != CAArch32Assembler::r1)
		{
			m_assembler.Mov(CAArch32Assembler::r1, src2Reg);
		}

		LoadConstantPtrInRegister(CAArch32Assembler::r2, divFct);
		m_assembler.Blx(CAArch32Assembler::r2);

		m_assembler.Str(CAArch32Assembler::r0, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 0));
	}

	//Remainder
	{
		auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r0);
		auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);

		if(src1Reg != CAArch32Assembler::r0)
		{
			m_assembler.Mov(CAArch32Assembler::r0, src1Reg);
		}

		if(src2Reg != CAArch32Assembler::r1)
		{
			m_assembler.Mov(CAArch32Assembler::r1, src2Reg);
		}
	
		LoadConstantPtrInRegister(CAArch32Assembler::r2, modFct);
		m_assembler.Blx(CAArch32Assembler::r2);
	
		m_assembler.Str(CAArch32Assembler::r0, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 4));
	}
}

template <bool isSigned>
void CCodeGen_AArch32::Div_GenericTmp64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY64);

	auto src1Reg = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r0);
	auto src2Reg = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);
	auto modReg0 = CAArch32Assembler::r1;	//Potentially overlaps src2Reg because it won't be needed after mul
	auto modReg1 = CAArch32Assembler::r2;
	auto resReg = CAArch32Assembler::r3;

	if(isSigned)
	{
		m_assembler.Sdiv(resReg, src1Reg, src2Reg);
		m_assembler.Smull(modReg0, modReg1, resReg, src2Reg);
	}
	else
	{
		m_assembler.Udiv(resReg, src1Reg, src2Reg);
		m_assembler.Umull(modReg0, modReg1, resReg, src2Reg);
	}

	m_assembler.Sub(modReg0, src1Reg, modReg0);

	m_assembler.Str(resReg, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 0));
	m_assembler.Str(modReg0, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(dst->m_stackLocation + m_stackLevel + 4));
}

template <bool isSigned>
void CCodeGen_AArch32::Emit_DivTmp64AnyAny(const STATEMENT& statement)
{
	if(m_hasIntegerDiv)
	{
		Div_GenericTmp64AnyAny<isSigned>(statement);
	}
	else
	{
		Div_GenericTmp64AnyAnySoft<isSigned>(statement);
	}
}
