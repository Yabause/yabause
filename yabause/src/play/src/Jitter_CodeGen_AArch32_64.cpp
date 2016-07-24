#include "Jitter_CodeGen_AArch32.h"

using namespace Jitter;

uint32 CCodeGen_AArch32::GetMemory64Offset(CSymbol* symbol) const
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		return symbol->m_valueLow;
		break;
	case SYM_TEMPORARY64:
		return symbol->m_stackLocation + m_stackLevel;
		break;
	default:
		assert(false);
		return 0;
		break;
	}
}

void CCodeGen_AArch32::LoadMemory64LowInRegister(CAArch32Assembler::REGISTER registerId, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		m_assembler.Ldr(registerId, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_valueLow + 0));
		break;
	case SYM_TEMPORARY64:
		m_assembler.Ldr(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel + 0));
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::LoadMemory64HighInRegister(CAArch32Assembler::REGISTER registerId, CSymbol* symbol)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		m_assembler.Ldr(registerId, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_valueLow + 4));
		break;
	case SYM_TEMPORARY64:
		m_assembler.Ldr(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel + 4));
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::LoadMemory64InRegisters(CAArch32Assembler::REGISTER regLo, CAArch32Assembler::REGISTER regHi, CSymbol* symbol)
{
	if(
		((regLo & 1) == 0) && 
		(regHi == (regLo + 1)) && 
		(GetMemory64Offset(symbol) < 0x100)
		)
	{
		switch(symbol->m_type)
		{
		case SYM_RELATIVE64:
			m_assembler.Ldrd(regLo, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_valueLow));
			break;
		case SYM_TEMPORARY64:
			m_assembler.Ldrd(regLo, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel));
			break;
		default:
			assert(false);
			break;
		}
	}
	else
	{
		LoadMemory64LowInRegister(regLo, symbol);
		LoadMemory64HighInRegister(regHi, symbol);
	}
}

void CCodeGen_AArch32::StoreRegistersInMemory64(CSymbol* symbol, CAArch32Assembler::REGISTER regLo, CAArch32Assembler::REGISTER regHi)
{
	if(
		((regLo & 1) == 0) && 
		(regHi == (regLo + 1)) && 
		(GetMemory64Offset(symbol) < 0x100)
		)
	{
		switch(symbol->m_type)
		{
		case SYM_RELATIVE64:
			m_assembler.Strd(regLo, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_valueLow));
			break;
		case SYM_TEMPORARY64:
			m_assembler.Strd(regLo, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel));
			break;
		default:
			assert(false);
			break;
		}
	}
	else
	{
		StoreRegisterInMemory64Low(symbol, regLo);
		StoreRegisterInMemory64High(symbol, regHi);
	}
}

void CCodeGen_AArch32::StoreRegisterInMemory64Low(CSymbol* symbol, CAArch32Assembler::REGISTER registerId)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		m_assembler.Str(registerId, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_valueLow + 0));
		break;
	case SYM_TEMPORARY64:
		m_assembler.Str(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel + 0));
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::StoreRegisterInMemory64High(CSymbol* symbol, CAArch32Assembler::REGISTER registerId)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE64:
		m_assembler.Str(registerId, g_baseRegister, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_valueLow + 4));
		break;
	case SYM_TEMPORARY64:
		m_assembler.Str(registerId, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateLdrAddress(symbol->m_stackLocation + m_stackLevel + 4));
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::Emit_Mov_Mem64Mem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto regLo = CAArch32Assembler::r0;
	auto regHi = CAArch32Assembler::r1;
	LoadMemory64InRegisters(regLo, regHi, src1);
	StoreRegistersInMemory64(dst, regLo, regHi);
}

void CCodeGen_AArch32::Emit_Mov_Mem64Cst64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto regLo = CAArch32Assembler::r0;
	auto regHi = CAArch32Assembler::r1;
	LoadConstantInRegister(regLo, src1->m_valueLow);
	LoadConstantInRegister(regHi, src1->m_valueHigh);
	StoreRegistersInMemory64(dst, regLo, regHi);
}

void CCodeGen_AArch32::Emit_ExtLow64VarMem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	LoadMemory64LowInRegister(dstReg, src1);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_ExtHigh64VarMem64(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	LoadMemory64HighInRegister(dstReg, src1);
	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_MergeTo64_Mem64AnyAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo = PrepareSymbolRegisterUse(src1, CAArch32Assembler::r0);
	auto regHi = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r1);

	StoreRegistersInMemory64(dst, regLo, regHi);
}

void CCodeGen_AArch32::Emit_Add64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo1 = CAArch32Assembler::r0;
	auto regHi1 = CAArch32Assembler::r1;
	auto regLo2 = CAArch32Assembler::r2;
	auto regHi2 = CAArch32Assembler::r3;

	LoadMemory64InRegisters(regLo1, regHi1, src1);
	LoadMemory64InRegisters(regLo2, regHi2, src2);

	m_assembler.Adds(regLo1, regLo1, regLo2);
	m_assembler.Adc(regHi1, regHi1, regHi2);

	StoreRegistersInMemory64(dst, regLo1, regHi1);
}

void CCodeGen_AArch32::Emit_Add64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo1 = CAArch32Assembler::r0;
	auto regHi1 = CAArch32Assembler::r1;
	auto regLo2 = CAArch32Assembler::r2;
	auto regHi2 = CAArch32Assembler::r3;

	LoadMemory64InRegisters(regLo1, regHi1, src1);
	LoadConstantInRegister(regLo2, src2->m_valueLow);
	LoadConstantInRegister(regHi2, src2->m_valueHigh);

	//TODO: Improve this by using immediate operands instead of loading constants in registers
	m_assembler.Adds(regLo1, regLo1, regLo2);
	m_assembler.Adc(regHi1, regHi1, regHi2);

	StoreRegistersInMemory64(dst, regLo1, regHi1);
}

void CCodeGen_AArch32::Emit_Sub64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo1 = CAArch32Assembler::r0;
	auto regHi1 = CAArch32Assembler::r1;
	auto regLo2 = CAArch32Assembler::r2;
	auto regHi2 = CAArch32Assembler::r3;

	LoadMemory64InRegisters(regLo1, regHi1, src1);
	LoadMemory64InRegisters(regLo2, regHi2, src2);

	m_assembler.Subs(regLo1, regLo1, regLo2);
	m_assembler.Sbc(regHi1, regHi1, regHi2);

	StoreRegistersInMemory64(dst, regLo1, regHi1);
}

void CCodeGen_AArch32::Emit_Sub64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo1 = CAArch32Assembler::r0;
	auto regHi1 = CAArch32Assembler::r1;
	auto regLo2 = CAArch32Assembler::r2;
	auto regHi2 = CAArch32Assembler::r3;

	LoadMemory64InRegisters(regLo1, regHi1, src1);
	LoadConstantInRegister(regLo2, src2->m_valueLow);
	LoadConstantInRegister(regHi2, src2->m_valueHigh);

	//TODO: Improve this by using immediate operands instead of loading constants in registers
	m_assembler.Subs(regLo1, regLo1, regLo2);
	m_assembler.Sbc(regHi1, regHi1, regHi2);

	StoreRegistersInMemory64(dst, regLo1, regHi1);
}

void CCodeGen_AArch32::Emit_Sub64_MemCstMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo1 = CAArch32Assembler::r0;
	auto regHi1 = CAArch32Assembler::r1;
	auto regLo2 = CAArch32Assembler::r2;
	auto regHi2 = CAArch32Assembler::r3;

	LoadConstantInRegister(regLo1, src1->m_valueLow);
	LoadConstantInRegister(regHi1, src1->m_valueHigh);
	LoadMemory64InRegisters(regLo2, regHi2, src2);

	//TODO: Improve this by using immediate operands instead of loading constants in registers
	m_assembler.Subs(regLo1, regLo1, regLo2);
	m_assembler.Sbc(regHi1, regHi1, regHi2);

	StoreRegistersInMemory64(dst, regLo1, regHi1);
}

void CCodeGen_AArch32::Emit_And64_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto regLo1 = CAArch32Assembler::r0;
	auto regHi1 = CAArch32Assembler::r1;
	auto regLo2 = CAArch32Assembler::r2;
	auto regHi2 = CAArch32Assembler::r3;

	LoadMemory64InRegisters(regLo1, regHi1, src1);
	LoadMemory64InRegisters(regLo2, regHi2, src2);

	m_assembler.And(regLo1, regLo1, regLo2);
	m_assembler.And(regHi1, regHi1, regHi2);

	StoreRegistersInMemory64(dst, regLo1, regHi1);
}

void CCodeGen_AArch32::Emit_Sl64Var_MemMem(CSymbol* dst, CSymbol* src, CAArch32Assembler::REGISTER saReg)
{
	//saReg will be modified by this function, do not use PrepareRegister

	assert(saReg == CAArch32Assembler::r0);

	auto lessThan32Label = m_assembler.CreateLabel();
	auto doneLabel = m_assembler.CreateLabel();

	m_assembler.And(saReg, saReg, CAArch32Assembler::MakeImmediateAluOperand(0x3F, 0));
	m_assembler.Cmp(saReg, CAArch32Assembler::MakeImmediateAluOperand(32, 0));
	m_assembler.BCc(CAArch32Assembler::CONDITION_LT, lessThan32Label);

	//greaterThanOrEqual32:
	{
		auto workReg = CAArch32Assembler::r1;
		auto dstLo = CAArch32Assembler::r2;
		auto dstHi = CAArch32Assembler::r3;

		LoadMemory64LowInRegister(workReg, src);

		auto fixedSaReg = CAArch32Assembler::r0;
		m_assembler.Sub(fixedSaReg, saReg, CAArch32Assembler::MakeImmediateAluOperand(32, 0));

		auto shiftHi = CAArch32Assembler::MakeVariableShift(CAArch32Assembler::SHIFT_LSL, fixedSaReg);
		m_assembler.Mov(dstHi, CAArch32Assembler::MakeRegisterAluOperand(workReg, shiftHi));

		m_assembler.Mov(dstLo, CAArch32Assembler::MakeImmediateAluOperand(0, 0));

		StoreRegistersInMemory64(dst, dstLo, dstHi);

		m_assembler.BCc(CAArch32Assembler::CONDITION_AL, doneLabel);
	}

	//lessThan32:
	m_assembler.MarkLabel(lessThan32Label);
	{
		auto dstReg = CAArch32Assembler::r1;
		auto loReg = CAArch32Assembler::r2;
		auto hiReg = CAArch32Assembler::r3;

		//Lo part -> (lo << sa)
		auto shiftLo = CAArch32Assembler::MakeVariableShift(CAArch32Assembler::SHIFT_LSL, saReg);
		LoadMemory64LowInRegister(loReg, src);
		m_assembler.Mov(dstReg, CAArch32Assembler::MakeRegisterAluOperand(loReg, shiftLo));
		StoreRegisterInMemory64Low(dst, dstReg);

		//Hi part -> (lo >> (32 - sa)) | (hi << sa)
		auto shiftHi1 = CAArch32Assembler::MakeVariableShift(CAArch32Assembler::SHIFT_LSL, saReg);
		LoadMemory64HighInRegister(hiReg, src);
		m_assembler.Mov(dstReg, CAArch32Assembler::MakeRegisterAluOperand(hiReg, shiftHi1));

		auto shiftHi2 = CAArch32Assembler::MakeVariableShift(CAArch32Assembler::SHIFT_LSR, saReg);
		m_assembler.Rsb(saReg, saReg, CAArch32Assembler::MakeImmediateAluOperand(32, 0));
		m_assembler.Mov(loReg, CAArch32Assembler::MakeRegisterAluOperand(loReg, shiftHi2));
		m_assembler.Or(dstReg, dstReg, loReg);

		StoreRegisterInMemory64High(dst, dstReg);
	}

	//done:
	m_assembler.MarkLabel(doneLabel);
}

void CCodeGen_AArch32::Emit_Sll64_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto saReg = CAArch32Assembler::r0;

	switch(src2->m_type)
	{
	case SYM_REGISTER:
		m_assembler.Mov(saReg, g_registers[src2->m_valueLow]);
		break;
	case SYM_RELATIVE:
	case SYM_TEMPORARY:
		LoadMemoryInRegister(saReg, src2);
		break;
	default:
		assert(0);
		break;
	}

	Emit_Sl64Var_MemMem(dst, src1, saReg);
}

void CCodeGen_AArch32::Emit_Sll64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto shiftAmount = src2->m_valueLow & 0x3F;
	assert(shiftAmount != 0);

	auto srcLo = CAArch32Assembler::r0;
	auto srcHi = CAArch32Assembler::r1;
	auto dstLo = CAArch32Assembler::r2;
	auto dstHi = CAArch32Assembler::r3;

	if(shiftAmount >= 32)
	{
		shiftAmount -= 32;

		LoadMemory64LowInRegister(srcLo, src1);

		auto shiftHi = CAArch32Assembler::MakeConstantShift(CAArch32Assembler::SHIFT_LSL, shiftAmount);
		m_assembler.Mov(dstHi, CAArch32Assembler::MakeRegisterAluOperand(srcLo, shiftHi));

		m_assembler.Mov(dstLo, CAArch32Assembler::MakeImmediateAluOperand(0, 0));

		StoreRegistersInMemory64(dst, dstLo, dstHi);
	}
	else //Amount < 32
	{
		LoadMemory64InRegisters(srcLo, srcHi, src1);

		//Lo part -> (lo << sa)
		auto shiftLo = CAArch32Assembler::MakeConstantShift(CAArch32Assembler::SHIFT_LSL, shiftAmount);

		m_assembler.Mov(dstLo, CAArch32Assembler::MakeRegisterAluOperand(srcLo, shiftLo));

		//Hi part -> (lo >> (32 - sa)) | (hi << sa)
		auto shiftHi1 = CAArch32Assembler::MakeConstantShift(CAArch32Assembler::SHIFT_LSL, shiftAmount);
		auto shiftHi2 = CAArch32Assembler::MakeConstantShift(CAArch32Assembler::SHIFT_LSR, 32 - shiftAmount);

		m_assembler.Mov(srcHi, CAArch32Assembler::MakeRegisterAluOperand(srcHi, shiftHi1));
		m_assembler.Mov(srcLo, CAArch32Assembler::MakeRegisterAluOperand(srcLo, shiftHi2));
		m_assembler.Or(dstHi, srcLo, srcHi);

		StoreRegistersInMemory64(dst, dstLo, dstHi);
	}
}

void CCodeGen_AArch32::Emit_Sr64Var_MemMem(CSymbol* dst, CSymbol* src, CAArch32Assembler::REGISTER saReg, CAArch32Assembler::SHIFT shiftType)
{
	assert((shiftType == CAArch32Assembler::SHIFT_ASR) || (shiftType == CAArch32Assembler::SHIFT_LSR));

	//saReg will be modified by this function, do not use PrepareRegister

	assert(saReg == CAArch32Assembler::r0);

	auto lessThan32Label = m_assembler.CreateLabel();
	auto doneLabel = m_assembler.CreateLabel();

	m_assembler.And(saReg, saReg, CAArch32Assembler::MakeImmediateAluOperand(0x3F, 0));
	m_assembler.Cmp(saReg, CAArch32Assembler::MakeImmediateAluOperand(32, 0));
	m_assembler.BCc(CAArch32Assembler::CONDITION_LT, lessThan32Label);

	//greaterThanOrEqual32:
	{
		auto workReg = CAArch32Assembler::r1;
		auto dstLo = CAArch32Assembler::r2;
		auto dstHi = CAArch32Assembler::r3;

		LoadMemory64HighInRegister(workReg, src);

		auto fixedSaReg = CAArch32Assembler::r0;
		m_assembler.Sub(fixedSaReg, saReg, CAArch32Assembler::MakeImmediateAluOperand(32, 0));

		auto shiftLo = CAArch32Assembler::MakeVariableShift(shiftType, fixedSaReg);
		m_assembler.Mov(dstLo, CAArch32Assembler::MakeRegisterAluOperand(workReg, shiftLo));

		if(shiftType == CAArch32Assembler::SHIFT_LSR)
		{
			m_assembler.Mov(dstHi, CAArch32Assembler::MakeImmediateAluOperand(0, 0));
		}
		else
		{
			auto shiftHi = CAArch32Assembler::MakeConstantShift(CAArch32Assembler::SHIFT_ASR, 31);
			m_assembler.Mov(dstHi, CAArch32Assembler::MakeRegisterAluOperand(workReg, shiftHi));
		}

		StoreRegistersInMemory64(dst, dstLo, dstHi);

		m_assembler.BCc(CAArch32Assembler::CONDITION_AL, doneLabel);
	}

	//lessThan32:
	m_assembler.MarkLabel(lessThan32Label);
	{
		auto dstReg = CAArch32Assembler::r1;
		auto loReg = CAArch32Assembler::r2;
		auto hiReg = CAArch32Assembler::r3;

		//Hi part -> (hi >> sa)
		auto shiftHi = CAArch32Assembler::MakeVariableShift(shiftType, saReg);
		LoadMemory64HighInRegister(hiReg, src);
		m_assembler.Mov(dstReg, CAArch32Assembler::MakeRegisterAluOperand(hiReg, shiftHi));
		StoreRegisterInMemory64High(dst, dstReg);

		//Lo part -> (hi << (32 - sa)) | (lo >> sa)
		auto shiftLo1 = CAArch32Assembler::MakeVariableShift(CAArch32Assembler::SHIFT_LSR, saReg);
		LoadMemory64LowInRegister(loReg, src);
		m_assembler.Mov(dstReg, CAArch32Assembler::MakeRegisterAluOperand(loReg, shiftLo1));

		auto shiftLo2 = CAArch32Assembler::MakeVariableShift(CAArch32Assembler::SHIFT_LSL, saReg);
		m_assembler.Rsb(saReg, saReg, CAArch32Assembler::MakeImmediateAluOperand(32, 0));
		m_assembler.Mov(hiReg, CAArch32Assembler::MakeRegisterAluOperand(hiReg, shiftLo2));
		m_assembler.Or(dstReg, dstReg, hiReg);

		StoreRegisterInMemory64Low(dst, dstReg);
	}

	//done:
	m_assembler.MarkLabel(doneLabel);
}

void CCodeGen_AArch32::Emit_Sr64Cst_MemMem(CSymbol* dst, CSymbol* src, uint32 shiftAmount, CAArch32Assembler::SHIFT shiftType)
{
	assert(shiftAmount < 0x40);
	assert(shiftAmount != 0);

	assert((shiftType == CAArch32Assembler::SHIFT_ASR) || (shiftType == CAArch32Assembler::SHIFT_LSR));

	auto srcLo = CAArch32Assembler::r0;
	auto srcHi = CAArch32Assembler::r1;
	auto dstLo = CAArch32Assembler::r2;
	auto dstHi = CAArch32Assembler::r3;

	if(shiftAmount >= 32)
	{
		shiftAmount -= 32;

		LoadMemory64HighInRegister(srcHi, src);

		if(shiftAmount != 0)
		{
			auto shiftLo = CAArch32Assembler::MakeConstantShift(shiftType, shiftAmount);
			m_assembler.Mov(dstLo, CAArch32Assembler::MakeRegisterAluOperand(srcHi, shiftLo));
		}
		else
		{
			m_assembler.Mov(dstLo, srcHi);
		}

		if(shiftType == CAArch32Assembler::SHIFT_LSR)
		{
			m_assembler.Mov(dstHi, CAArch32Assembler::MakeImmediateAluOperand(0, 0));
		}
		else
		{
			auto shiftHi = CAArch32Assembler::MakeConstantShift(CAArch32Assembler::SHIFT_ASR, 31);
			m_assembler.Mov(dstHi, CAArch32Assembler::MakeRegisterAluOperand(srcHi, shiftHi));
		}

		StoreRegistersInMemory64(dst, dstLo, dstHi);
	}
	else //Amount < 32
	{
		LoadMemory64InRegisters(srcLo, srcHi, src);

		//Hi part -> (hi >> sa)
		auto shiftHi = CAArch32Assembler::MakeConstantShift(shiftType, shiftAmount);

		m_assembler.Mov(dstHi, CAArch32Assembler::MakeRegisterAluOperand(srcHi, shiftHi));

		//Lo part -> (hi << (32 - sa)) | (lo >> sa)
		auto shiftLo1 = CAArch32Assembler::MakeConstantShift(CAArch32Assembler::SHIFT_LSR, shiftAmount);
		auto shiftLo2 = CAArch32Assembler::MakeConstantShift(CAArch32Assembler::SHIFT_LSL, 32 - shiftAmount);

		m_assembler.Mov(srcLo, CAArch32Assembler::MakeRegisterAluOperand(srcLo, shiftLo1));
		m_assembler.Mov(srcHi, CAArch32Assembler::MakeRegisterAluOperand(srcHi, shiftLo2));
		m_assembler.Or(dstLo, srcLo, srcHi);

		StoreRegistersInMemory64(dst, dstLo, dstHi);
	}
}

void CCodeGen_AArch32::Emit_Srl64_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto saReg = CAArch32Assembler::r0;

	switch(src2->m_type)
	{
	case SYM_REGISTER:
		m_assembler.Mov(saReg, g_registers[src2->m_valueLow]);
		break;
	case SYM_RELATIVE:
	case SYM_TEMPORARY:
		LoadMemoryInRegister(saReg, src2);
		break;
	default:
		assert(0);
		break;
	}

	Emit_Sr64Var_MemMem(dst, src1, saReg, CAArch32Assembler::SHIFT_LSR);
}

void CCodeGen_AArch32::Emit_Srl64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto shiftAmount = src2->m_valueLow & 0x3F;

	Emit_Sr64Cst_MemMem(dst, src1, shiftAmount, CAArch32Assembler::SHIFT_LSR);
}

void CCodeGen_AArch32::Emit_Sra64_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto saReg = CAArch32Assembler::r0;

	switch(src2->m_type)
	{
	case SYM_REGISTER:
		m_assembler.Mov(saReg, g_registers[src2->m_valueLow]);
		break;
	case SYM_RELATIVE:
	case SYM_TEMPORARY:
		LoadMemoryInRegister(saReg, src2);
		break;
	default:
		assert(0);
		break;
	}

	Emit_Sr64Var_MemMem(dst, src1, saReg, CAArch32Assembler::SHIFT_ASR);
}

void CCodeGen_AArch32::Emit_Sra64_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto shiftAmount = src2->m_valueLow & 0x3F;

	Emit_Sr64Cst_MemMem(dst, src1, shiftAmount, CAArch32Assembler::SHIFT_ASR);
}

void CCodeGen_AArch32::Cmp64_RegSymLo(CAArch32Assembler::REGISTER src1Reg, CSymbol* src2, CAArch32Assembler::REGISTER src2Reg)
{
	switch(src2->m_type)
	{
	case SYM_RELATIVE64:
	case SYM_TEMPORARY64:
		LoadMemory64LowInRegister(src2Reg, src2);
		m_assembler.Cmp(src1Reg, src2Reg);
		break;
	case SYM_CONSTANT64:
		Cmp_GenericRegCst(src1Reg, src2->m_valueLow, src2Reg);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::Cmp64_RegSymHi(CAArch32Assembler::REGISTER src1Reg, CSymbol* src2, CAArch32Assembler::REGISTER src2Reg)
{
	switch(src2->m_type)
	{
	case SYM_RELATIVE64:
	case SYM_TEMPORARY64:
		LoadMemory64HighInRegister(src2Reg, src2);
		m_assembler.Cmp(src1Reg, src2Reg);
		break;
	case SYM_CONSTANT64:
		Cmp_GenericRegCst(src1Reg, src2->m_valueHigh, src2Reg);
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_AArch32::Cmp64_Equal(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = CAArch32Assembler::r1;
	auto src2Reg = CAArch32Assembler::r2;
	auto tempValReg = CAArch32Assembler::r3;

	/////////////////////////////////////////
	//Check high order word
	LoadMemory64HighInRegister(src1Reg, src1);
	Cmp64_RegSymHi(src1Reg, src2, src2Reg);
	Cmp_GetFlag(tempValReg, statement.jmpCondition);

	/////////////////////////////////////////
	//Check low order word
	LoadMemory64LowInRegister(src1Reg, src1);
	Cmp64_RegSymLo(src1Reg, src2, src2Reg);
	Cmp_GetFlag(dstReg, statement.jmpCondition);

	if(statement.jmpCondition == Jitter::CONDITION_EQ)
	{
		m_assembler.And(dstReg, dstReg, tempValReg);
	}
	else if(statement.jmpCondition == Jitter::CONDITION_NE)
	{
		m_assembler.Or(dstReg, dstReg, tempValReg);
	}
	else
	{
		//Shouldn't get here
		assert(false);
	}

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Cmp64_Order(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto doneLabel = m_assembler.CreateLabel();
	auto highOrderEqualLabel = m_assembler.CreateLabel();

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);
	auto src1Reg = CAArch32Assembler::r1;
	auto src2Reg = CAArch32Assembler::r2;

	/////////////////////////////////////////
	//Check high order word if equal

	//Compare with constant?
	LoadMemory64HighInRegister(src1Reg, src1);
	Cmp64_RegSymHi(src1Reg, src2, src2Reg);

	m_assembler.BCc(CAArch32Assembler::CONDITION_EQ, highOrderEqualLabel);

	///////////////////////////////////////////////////////////
	//If they aren't equal, this comparaison decides of result

	Cmp_GetFlag(dstReg, statement.jmpCondition);
	m_assembler.BCc(CAArch32Assembler::CONDITION_AL, doneLabel);

	///////////////////////////////////////////////////////////
	//If they are equal, next comparaison decides of result

	//highOrderEqual: /////////////////////////////////////
	m_assembler.MarkLabel(highOrderEqualLabel);

	LoadMemory64LowInRegister(src1Reg, src1);
	Cmp64_RegSymLo(src1Reg, src2, src2Reg);

	auto unsignedCondition = Jitter::CONDITION_NEVER;
	switch(statement.jmpCondition)
	{
	case Jitter::CONDITION_LT:
		unsignedCondition = Jitter::CONDITION_BL;
		break;
	case Jitter::CONDITION_LE:
		unsignedCondition = Jitter::CONDITION_BE;
		break;
	case Jitter::CONDITION_GT:
		unsignedCondition = Jitter::CONDITION_AB;
		break;
	case Jitter::CONDITION_AB:
	case Jitter::CONDITION_BL:
		unsignedCondition = statement.jmpCondition;
		break;
	default:
		assert(0);
		break;
	}

	Cmp_GetFlag(dstReg, unsignedCondition);

	//done: ///////////////////////////////////////////////
	m_assembler.MarkLabel(doneLabel);

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Cmp64_VarMemAny(const STATEMENT& statement)
{
	switch(statement.jmpCondition)
	{
	case CONDITION_BL:
	case CONDITION_LT:
	case CONDITION_LE:
	case CONDITION_AB:
	case CONDITION_GT:
	case CONDITION_GE:
		Cmp64_Order(statement);
		break;
	case CONDITION_NE:
	case CONDITION_EQ:
		Cmp64_Equal(statement);
		break;
	default:
		assert(0);
		break;
	}
}

CCodeGen_AArch32::CONSTMATCHER CCodeGen_AArch32::g_64ConstMatchers[] = 
{
	{ OP_EXTLOW64,		MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_AArch32::Emit_ExtLow64VarMem64			},
	{ OP_EXTHIGH64,		MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_AArch32::Emit_ExtHigh64VarMem64			},

	{ OP_MERGETO64,		MATCH_MEMORY64,		MATCH_ANY,			MATCH_ANY,			&CCodeGen_AArch32::Emit_MergeTo64_Mem64AnyAny		},

	{ OP_ADD64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_AArch32::Emit_Add64_MemMemMem				},
	{ OP_ADD64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT64,	&CCodeGen_AArch32::Emit_Add64_MemMemCst				},

	{ OP_SUB64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_AArch32::Emit_Sub64_MemMemMem				},
	{ OP_SUB64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT64,	&CCodeGen_AArch32::Emit_Sub64_MemMemCst,			},
	{ OP_SUB64,			MATCH_MEMORY64,		MATCH_CONSTANT64,	MATCH_MEMORY64,		&CCodeGen_AArch32::Emit_Sub64_MemCstMem				},

	{ OP_AND64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_AArch32::Emit_And64_MemMemMem,			},

	{ OP_SLL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_VARIABLE,		&CCodeGen_AArch32::Emit_Sll64_MemMemVar				},
	{ OP_SLL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT,		&CCodeGen_AArch32::Emit_Sll64_MemMemCst				},

	{ OP_SRL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_VARIABLE,		&CCodeGen_AArch32::Emit_Srl64_MemMemVar				},
	{ OP_SRL64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT,		&CCodeGen_AArch32::Emit_Srl64_MemMemCst				},

	{ OP_SRA64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_VARIABLE,		&CCodeGen_AArch32::Emit_Sra64_MemMemVar				},
	{ OP_SRA64,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_CONSTANT,		&CCodeGen_AArch32::Emit_Sra64_MemMemCst				},

	{ OP_CMP64,			MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_MEMORY64,		&CCodeGen_AArch32::Emit_Cmp64_VarMemAny				},
	{ OP_CMP64,			MATCH_VARIABLE,		MATCH_MEMORY64,		MATCH_CONSTANT64,	&CCodeGen_AArch32::Emit_Cmp64_VarMemAny				},

	{ OP_MOV,			MATCH_MEMORY64,		MATCH_MEMORY64,		MATCH_NIL,			&CCodeGen_AArch32::Emit_Mov_Mem64Mem64				},
	{ OP_MOV,			MATCH_MEMORY64,		MATCH_CONSTANT64,	MATCH_NIL,			&CCodeGen_AArch32::Emit_Mov_Mem64Cst64				},

	{ OP_MOV,			MATCH_NIL,			MATCH_NIL,			MATCH_NIL,			NULL											},
};
