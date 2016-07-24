#include "Jitter_CodeGen_AArch32.h"

using namespace Jitter;

void CCodeGen_AArch32::LoadMemory128AddressInRegister(CAArch32Assembler::REGISTER dstReg, CSymbol* symbol, uint32 offset)
{
	switch(symbol->m_type)
	{
	case SYM_RELATIVE128:
		LoadRelative128AddressInRegister(dstReg, symbol, offset);
		break;
	case SYM_TEMPORARY128:
		LoadTemporary128AddressInRegister(dstReg, symbol, offset);
		break;
	default:
		assert(0);
		break;
	}
}

void CCodeGen_AArch32::LoadRelative128AddressInRegister(CAArch32Assembler::REGISTER dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_RELATIVE128);

	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(!TryGetAluImmediateParams(symbol->m_valueLow + offset, immediate, shiftAmount))
	{
		throw std::runtime_error("Failed to build immediate for symbol.");
	}
	m_assembler.Add(dstReg, g_baseRegister, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
}

void CCodeGen_AArch32::LoadTemporary128AddressInRegister(CAArch32Assembler::REGISTER dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_TEMPORARY128);

	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(!TryGetAluImmediateParams(symbol->m_stackLocation + m_stackLevel + offset, immediate, shiftAmount))
	{
		throw std::runtime_error("Failed to build immediate for symbol.");
	}
	m_assembler.Add(dstReg, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
}

void CCodeGen_AArch32::LoadTemporary256ElementAddressInRegister(CAArch32Assembler::REGISTER dstReg, CSymbol* symbol, uint32 offset)
{
	assert(symbol->m_type == SYM_TEMPORARY256);

	uint8 immediate = 0;
	uint8 shiftAmount = 0;
	if(!TryGetAluImmediateParams(symbol->m_stackLocation + m_stackLevel + offset, immediate, shiftAmount))
	{
		throw std::runtime_error("Failed to build immediate for symbol.");
	}
	m_assembler.Add(dstReg, CAArch32Assembler::rSP, CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
}

template <typename MDOP>
void CCodeGen_AArch32::Emit_Md_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto dstReg = CAArch32Assembler::q0;
	auto src1Reg = CAArch32Assembler::q1;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);

	m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg);
	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

template <typename MDOP>
void CCodeGen_AArch32::Emit_Md_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto src2AddrReg = CAArch32Assembler::r2;
	auto dstReg = CAArch32Assembler::q0;
	auto src1Reg = CAArch32Assembler::q1;
	auto src2Reg = CAArch32Assembler::q2;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
	m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
	((m_assembler).*(MDOP::OpReg()))(dstReg, src1Reg, src2Reg);
	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

template <typename MDSHIFTOP>
void CCodeGen_AArch32::Emit_Md_Shift_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto dstReg = CAArch32Assembler::q0;
	auto src1Reg = CAArch32Assembler::q1;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);

	m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
	((m_assembler).*(MDSHIFTOP::OpReg()))(dstReg, src1Reg, src2->m_valueLow);
	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

template <uint32 condition>
void CCodeGen_AArch32::Emit_Md_Test_VarMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto src1AddrReg = CAArch32Assembler::r0;
	auto src1Reg = CAArch32Assembler::q0;

	LoadMemory128AddressInRegister(src1AddrReg, src1);
	m_assembler.Vld1_32x4(src1Reg, src1AddrReg);

	auto dstReg = PrepareSymbolRegisterDef(dst, CAArch32Assembler::r0);

	static CAArch32Assembler::REGISTER regs[4] =
	{
		CAArch32Assembler::r2,
		CAArch32Assembler::r3
	};

	m_assembler.Eor(dstReg, dstReg, dstReg);
	for(unsigned int i = 0; i < 4; i++)
	{
		m_assembler.Vmov(regs[i & 1], static_cast<CAArch32Assembler::DOUBLE_REGISTER>(src1Reg + (i / 2)), i & 1);
		m_assembler.Tst(regs[i & 1], regs[i & 1]);
		uint8 immediate = 0, shiftAmount = 0;
		if(!TryGetAluImmediateParams(1 << (3 - i), immediate, shiftAmount))
		{
			assert(false);
		}
		m_assembler.Or(static_cast<CAArch32Assembler::CONDITION>(condition), dstReg, dstReg, 
			CAArch32Assembler::MakeImmediateAluOperand(immediate, shiftAmount));
	}

	CommitSymbolRegister(dst, dstReg);
}

void CCodeGen_AArch32::Emit_Md_Mov_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto tmpReg = CAArch32Assembler::q0;
	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	m_assembler.Vld1_32x4(tmpReg, src1AddrReg);
	m_assembler.Vst1_32x4(tmpReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_Not_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto zeroReg = CAArch32Assembler::q0;
	auto tmpReg = CAArch32Assembler::q1;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);

	m_assembler.Vld1_32x4(tmpReg, src1AddrReg);
	m_assembler.Veor(zeroReg, zeroReg, zeroReg);
	m_assembler.Vorn(tmpReg, zeroReg, tmpReg);
	m_assembler.Vst1_32x4(tmpReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_DivS_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto src2AddrReg = CAArch32Assembler::r2;
	auto dstReg = CAArch32Assembler::q0;
	auto src1Reg = CAArch32Assembler::q1;
	auto src2Reg = CAArch32Assembler::q2;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
	m_assembler.Vld1_32x4(src2Reg, src2AddrReg);

	//No vector floating point divide on NEON, gotta do it 4x
	for(unsigned int i = 0; i < 4; i++)
	{
		auto subDstReg = static_cast<CAArch32Assembler::SINGLE_REGISTER>(dstReg * 2 + i);
		auto subSrc1Reg = static_cast<CAArch32Assembler::SINGLE_REGISTER>(src1Reg * 2 + i);
		auto subSrc2Reg = static_cast<CAArch32Assembler::SINGLE_REGISTER>(src2Reg * 2 + i);
		m_assembler.Vdiv_F32(subDstReg, subSrc1Reg, subSrc2Reg);
	}

	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_Srl256_MemMemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY256);
	assert(src2->m_type == SYM_CONSTANT);

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto dstReg = CAArch32Assembler::q0;

	uint32 offset = (src2->m_valueLow & 0x7F) / 8;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadTemporary256ElementAddressInRegister(src1AddrReg, src1, offset);

	m_assembler.Vld1_32x4_u(dstReg, src1AddrReg);
	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_Srl256_MemMemVar(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(src1->m_type == SYM_TEMPORARY256);

	auto offsetRegister = CAArch32Assembler::r0;
	auto dstAddrReg = CAArch32Assembler::r1;
	auto src1AddrReg = CAArch32Assembler::r2;
	auto src2Register = PrepareSymbolRegisterUse(src2, CAArch32Assembler::r3);

	auto dstReg = CAArch32Assembler::q0;

	auto offsetShift = CAArch32Assembler::MakeConstantShift(CAArch32Assembler::SHIFT_LSR, 3);

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadTemporary256ElementAddressInRegister(src1AddrReg, src1, 0);

	//Compute offset and modify address
	m_assembler.And(offsetRegister, src2Register, CAArch32Assembler::MakeImmediateAluOperand(0x7F, 0));
	m_assembler.Mov(offsetRegister, CAArch32Assembler::MakeRegisterAluOperand(offsetRegister, offsetShift));
	m_assembler.Add(src1AddrReg, src1AddrReg, offsetRegister);

	m_assembler.Vld1_32x4_u(dstReg, src1AddrReg);
	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_LoadFromRef_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto src1AddrReg = CAArch32Assembler::r0;
	auto dstAddrReg = CAArch32Assembler::r1;

	auto dstReg = CAArch32Assembler::q0;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemoryReferenceInRegister(src1AddrReg, src1);

	m_assembler.Vld1_32x4(dstReg, src1AddrReg);
	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_StoreAtRef_MemMem(const STATEMENT& statement)
{
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto src1AddrReg = CAArch32Assembler::r0;
	auto src2AddrReg = CAArch32Assembler::r1;

	auto src2Reg = CAArch32Assembler::q0;

	LoadMemoryReferenceInRegister(src1AddrReg, src1);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
	m_assembler.Vst1_32x4(src2Reg, src1AddrReg);
}

void CCodeGen_AArch32::Emit_Md_MovMasked_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->Equals(src1));

	auto mask = static_cast<uint8>(statement.jmpCondition);

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src2AddrReg = CAArch32Assembler::r2;
	auto tmpReg = CAArch32Assembler::r3;
	auto dstReg = CAArch32Assembler::q0;
	auto src2Reg = CAArch32Assembler::q2;
	auto dstRegLo = static_cast<CAArch32Assembler::DOUBLE_REGISTER>(dstReg + 0);
	auto dstRegHi = static_cast<CAArch32Assembler::DOUBLE_REGISTER>(dstReg + 1);
	auto src2RegLo = static_cast<CAArch32Assembler::DOUBLE_REGISTER>(src2Reg + 0);
	auto src2RegHi = static_cast<CAArch32Assembler::DOUBLE_REGISTER>(src2Reg + 1);

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Vld1_32x4(dstReg, dstAddrReg);
	m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
	for(unsigned int i = 0; i < 4; i++)
	{
		if(mask & (1 << i))
		{
			m_assembler.Vmov(tmpReg, (i & 2) ? src2RegHi : src2RegLo, (i & 1));
			m_assembler.Vmov((i & 2) ? dstRegHi : dstRegLo, tmpReg, (i & 1));
		}
	}

	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_Expand_MemReg(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto tmpReg = CAArch32Assembler::q0;

	LoadMemory128AddressInRegister(dstAddrReg, dst);

	m_assembler.Vdup(tmpReg, g_registers[src1->m_valueLow]);
	m_assembler.Vst1_32x4(tmpReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_Expand_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1Reg = CAArch32Assembler::r1;
	auto tmpReg = CAArch32Assembler::q0;

	LoadMemoryInRegister(src1Reg, src1);
	LoadMemory128AddressInRegister(dstAddrReg, dst);

	m_assembler.Vdup(tmpReg, src1Reg);
	m_assembler.Vst1_32x4(tmpReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_Expand_MemCst(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1Reg = CAArch32Assembler::r1;
	auto tmpReg = CAArch32Assembler::q0;

	LoadConstantInRegister(src1Reg, src1->m_valueLow);
	LoadMemory128AddressInRegister(dstAddrReg, dst);

	m_assembler.Vdup(tmpReg, src1Reg);
	m_assembler.Vst1_32x4(tmpReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_PackHB_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto src2AddrReg = CAArch32Assembler::r2;
	auto dstReg = CAArch32Assembler::q0;
	auto src1Reg = CAArch32Assembler::q1;
	auto src2Reg = CAArch32Assembler::q2;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
	m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
	m_assembler.Vmovn_I16(static_cast<CAArch32Assembler::DOUBLE_REGISTER>(dstReg + 1), src1Reg);
	m_assembler.Vmovn_I16(static_cast<CAArch32Assembler::DOUBLE_REGISTER>(dstReg + 0), src2Reg);
	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

void CCodeGen_AArch32::Emit_Md_PackWH_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto src2AddrReg = CAArch32Assembler::r2;
	auto dstReg = CAArch32Assembler::q0;
	auto src1Reg = CAArch32Assembler::q1;
	auto src2Reg = CAArch32Assembler::q2;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
	m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
	m_assembler.Vmovn_I32(static_cast<CAArch32Assembler::DOUBLE_REGISTER>(dstReg + 1), src1Reg);
	m_assembler.Vmovn_I32(static_cast<CAArch32Assembler::DOUBLE_REGISTER>(dstReg + 0), src2Reg);
	m_assembler.Vst1_32x4(dstReg, dstAddrReg);
}

template <uint32 offset>
void CCodeGen_AArch32::Emit_Md_UnpackBH_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto src2AddrReg = CAArch32Assembler::r2;
	auto src1Reg = CAArch32Assembler::d0;
	auto src2Reg = CAArch32Assembler::d1;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1, offset);
	LoadMemory128AddressInRegister(src2AddrReg, src2, offset);

	//Warning: VZIP modifies both registers
	m_assembler.Vld1_32x2(src1Reg, src2AddrReg);
	m_assembler.Vld1_32x2(src2Reg, src1AddrReg);
	m_assembler.Vzip_I8(src1Reg, src2Reg);
	m_assembler.Vst1_32x4(static_cast<CAArch32Assembler::QUAD_REGISTER>(src1Reg), dstAddrReg);
}

template <uint32 offset>
void CCodeGen_AArch32::Emit_Md_UnpackHW_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto src2AddrReg = CAArch32Assembler::r2;
	auto src1Reg = CAArch32Assembler::d0;
	auto src2Reg = CAArch32Assembler::d1;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1, offset);
	LoadMemory128AddressInRegister(src2AddrReg, src2, offset);

	//Warning: VZIP modifies both registers
	m_assembler.Vld1_32x2(src1Reg, src2AddrReg);
	m_assembler.Vld1_32x2(src2Reg, src1AddrReg);
	m_assembler.Vzip_I16(src1Reg, src2Reg);
	m_assembler.Vst1_32x4(static_cast<CAArch32Assembler::QUAD_REGISTER>(src1Reg), dstAddrReg);
}

template <uint32 offset>
void CCodeGen_AArch32::Emit_Md_UnpackWD_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	auto dstAddrReg = CAArch32Assembler::r0;
	auto src1AddrReg = CAArch32Assembler::r1;
	auto src2AddrReg = CAArch32Assembler::r2;
	auto src1Reg = CAArch32Assembler::d0;
	auto src2Reg = CAArch32Assembler::d2;

	LoadMemory128AddressInRegister(dstAddrReg, dst);
	LoadMemory128AddressInRegister(src1AddrReg, src1, offset);
	LoadMemory128AddressInRegister(src2AddrReg, src2, offset);

	//Warning: VZIP modifies both registers
	m_assembler.Vld1_32x2(src1Reg, src2AddrReg);
	m_assembler.Vld1_32x2(src2Reg, src1AddrReg);
	m_assembler.Vzip_I32(static_cast<CAArch32Assembler::QUAD_REGISTER>(src1Reg), static_cast<CAArch32Assembler::QUAD_REGISTER>(src2Reg));
	m_assembler.Vst1_32x4(static_cast<CAArch32Assembler::QUAD_REGISTER>(src1Reg), dstAddrReg);
}

void CCodeGen_AArch32::Emit_MergeTo256_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	assert(dst->m_type == SYM_TEMPORARY256);

	auto dstLoAddrReg = CAArch32Assembler::r0;
	auto dstHiAddrReg = CAArch32Assembler::r1;
	auto src1AddrReg = CAArch32Assembler::r2;
	auto src2AddrReg = CAArch32Assembler::r3;
	auto src1Reg = CAArch32Assembler::q0;
	auto src2Reg = CAArch32Assembler::q1;

	LoadTemporary256ElementAddressInRegister(dstLoAddrReg, dst, 0x00);
	LoadTemporary256ElementAddressInRegister(dstHiAddrReg, dst, 0x10);
	LoadMemory128AddressInRegister(src1AddrReg, src1);
	LoadMemory128AddressInRegister(src2AddrReg, src2);

	m_assembler.Vld1_32x4(src1Reg, src1AddrReg);
	m_assembler.Vld1_32x4(src2Reg, src2AddrReg);
	m_assembler.Vst1_32x4(src1Reg, dstLoAddrReg);
	m_assembler.Vst1_32x4(src2Reg, dstHiAddrReg);
}

CCodeGen_AArch32::CONSTMATCHER CCodeGen_AArch32::g_mdConstMatchers[] = 
{
	{ OP_MD_ADD_B,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_ADDB>					},
	{ OP_MD_ADD_H,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_ADDH>					},
	{ OP_MD_ADD_W,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_ADDW>					},

	{ OP_MD_SUB_B,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_SUBB>					},
	{ OP_MD_SUB_H,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_SUBH>					},
	{ OP_MD_SUB_W,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_SUBW>					},

	{ OP_MD_ADDUS_B,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_ADDBUS>				},
	{ OP_MD_ADDUS_H,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_ADDHUS>				},
	{ OP_MD_ADDUS_W,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_ADDWUS>				},

	{ OP_MD_ADDSS_H,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_ADDHSS>				},
	{ OP_MD_ADDSS_W,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_ADDWSS>				},

	{ OP_MD_SUBUS_B,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_SUBBUS>				},
	{ OP_MD_SUBUS_H,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_SUBHUS>				},

	{ OP_MD_SUBSS_H,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_SUBHSS>				},
	{ OP_MD_SUBSS_W,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_SUBWSS>				},

	{ OP_MD_CMPEQ_W,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_CMPEQW>				},

	{ OP_MD_CMPGT_H,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_CMPGTH>				},

	{ OP_MD_MIN_H,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_MINH>					},
	{ OP_MD_MIN_W,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_MINW>					},

	{ OP_MD_MAX_H,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_MAXH>					},
	{ OP_MD_MAX_W,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_MAXW>					},

	{ OP_MD_ADD_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_ADDS>					},
	{ OP_MD_SUB_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_SUBS>					},
	{ OP_MD_MUL_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_MULS>					},
	{ OP_MD_DIV_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_DivS_MemMemMem						},

	{ OP_MD_ABS_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_MemMem<MDOP_ABSS>					},
	{ OP_MD_MIN_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<FPUMDOP_MIN>				},
	{ OP_MD_MAX_S,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<FPUMDOP_MAX>				},

	{ OP_MD_AND,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_AND>					},
	{ OP_MD_OR,					MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_OR>					},
	{ OP_MD_XOR,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MemMemMem<MDOP_XOR>					},

	{ OP_MD_NOT,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_Not_MemMem							},

	{ OP_MD_SLLH,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_AArch32::Emit_Md_Shift_MemMemCst<MDOP_SLLH>			},
	{ OP_MD_SLLW,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_AArch32::Emit_Md_Shift_MemMemCst<MDOP_SLLW>			},

	{ OP_MD_SRLH,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_AArch32::Emit_Md_Shift_MemMemCst<MDOP_SRLH>			},
	{ OP_MD_SRLW,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_AArch32::Emit_Md_Shift_MemMemCst<MDOP_SRLW>			},

	{ OP_MD_SRAH,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_AArch32::Emit_Md_Shift_MemMemCst<MDOP_SRAH>			},
	{ OP_MD_SRAW,				MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_CONSTANT,			&CCodeGen_AArch32::Emit_Md_Shift_MemMemCst<MDOP_SRAW>			},

	{ OP_MD_SRL256,				MATCH_VARIABLE128,			MATCH_MEMORY256,			MATCH_VARIABLE,			&CCodeGen_AArch32::Emit_Md_Srl256_MemMemVar						},
	{ OP_MD_SRL256,				MATCH_VARIABLE128,			MATCH_MEMORY256,			MATCH_CONSTANT,			&CCodeGen_AArch32::Emit_Md_Srl256_MemMemCst						},

	{ OP_MD_ISNEGATIVE,			MATCH_VARIABLE,				MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_Test_VarMem<CAArch32Assembler::CONDITION_MI> },
	{ OP_MD_ISZERO,				MATCH_VARIABLE,				MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_Test_VarMem<CAArch32Assembler::CONDITION_EQ> },

	{ OP_MD_TOSINGLE,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_MemMem<MDOP_TOSINGLE>				},
	{ OP_MD_TOWORD_TRUNCATE,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_MemMem<MDOP_TOWORD>					},

	{ OP_MOV,					MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_Mov_MemMem							},

	{ OP_LOADFROMREF,			MATCH_MEMORY128,			MATCH_MEM_REF,				MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_LoadFromRef_MemMem					},
	{ OP_STOREATREF,			MATCH_NIL,					MATCH_MEM_REF,				MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_StoreAtRef_MemMem					},

	{ OP_MD_MOV_MASKED,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_MovMasked_MemMemMem					},

	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_REGISTER,				MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_Expand_MemReg						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_MEMORY,				MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_Expand_MemMem						},
	{ OP_MD_EXPAND,				MATCH_MEMORY128,			MATCH_CONSTANT,				MATCH_NIL,				&CCodeGen_AArch32::Emit_Md_Expand_MemCst						},

	{ OP_MD_PACK_HB,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_PackHB_MemMemMem						},
	{ OP_MD_PACK_WH,			MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_PackWH_MemMemMem						},

	{ OP_MD_UNPACK_LOWER_BH,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_UnpackBH_MemMemMem<0>				},
	{ OP_MD_UNPACK_LOWER_HW,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_UnpackHW_MemMemMem<0>				},
	{ OP_MD_UNPACK_LOWER_WD,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_UnpackWD_MemMemMem<0>				},

	{ OP_MD_UNPACK_UPPER_BH,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_UnpackBH_MemMemMem<8>				},
	{ OP_MD_UNPACK_UPPER_HW,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_UnpackHW_MemMemMem<8>				},
	{ OP_MD_UNPACK_UPPER_WD,	MATCH_MEMORY128,			MATCH_MEMORY128,			MATCH_MEMORY128,		&CCodeGen_AArch32::Emit_Md_UnpackWD_MemMemMem<8>				},

	{ OP_MERGETO256,			MATCH_MEMORY256,			MATCH_VARIABLE128,			MATCH_VARIABLE128,		&CCodeGen_AArch32::Emit_MergeTo256_MemMemMem					},

	{ OP_MOV,					MATCH_NIL,					MATCH_NIL,					MATCH_NIL,				NULL														},
};
