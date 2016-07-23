#include "X86Assembler.h"

//------------------------------------------------
//Scalar Instructions
//------------------------------------------------

void CX86Assembler::MovssEd(const CAddress& address, XMMREGISTER registerId)
{
	WriteEdVdOp_F3_0F(0x11, address, registerId);
}

void CX86Assembler::MovssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x10, address, registerId);
}

void CX86Assembler::AddssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x58, address, registerId);
}

void CX86Assembler::SubssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x5C, address, registerId);
}

void CX86Assembler::MaxssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x5F, address, registerId);
}

void CX86Assembler::MinssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x5D, address, registerId);
}

void CX86Assembler::MulssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x59, address, registerId);
}

void CX86Assembler::DivssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x5E, address, registerId);
}

void CX86Assembler::RcpssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x53, address, registerId);
}

void CX86Assembler::RsqrtssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x52, address, registerId);
}

void CX86Assembler::SqrtssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x51, address, registerId);
}

void CX86Assembler::CmpssEd(XMMREGISTER registerId, const CAddress& address, SSE_CMP_TYPE condition)
{
	WriteEdVdOp_F3_0F(0xC2, address, registerId);
	WriteByte(static_cast<uint8>(condition));
}

void CX86Assembler::Cvtsi2ssEd(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x2A, address, registerId);
}

void CX86Assembler::Cvttss2siEd(REGISTER registerId, const CAddress& address)
{
	WriteByte(0xF3);
	WriteByte(0x0F);
	WriteEvGvOp(0x2C, false, address, registerId);
}

//------------------------------------------------
//Packed Instructions
//------------------------------------------------

void CX86Assembler::MovdVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x6E, address, registerId);
}

void CX86Assembler::MovdVo(const CAddress& address, XMMREGISTER registerId)
{
	WriteEdVdOp_66_0F(0x7E, address, registerId);
}

void CX86Assembler::MovqVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F_64b(0x6E, address, registerId);
}

void CX86Assembler::MovdqaVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x6F, address, registerId);
}

void CX86Assembler::MovdqaVo(const CAddress& address, XMMREGISTER registerId)
{
	WriteEdVdOp_66_0F(0x7F, address, registerId);
}

void CX86Assembler::MovdquVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x6F, address, registerId);
}

void CX86Assembler::MovdquVo(const CAddress& address, XMMREGISTER registerId)
{
	WriteEdVdOp_F3_0F(0x7F, address, registerId);
}

void CX86Assembler::MovapsVo(const CAddress& address, XMMREGISTER registerId)
{
	WriteEdVdOp_0F(0x29, address, registerId);
}

void CX86Assembler::MovapsVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_0F(0x28, address, registerId);
}

void CX86Assembler::PackssdwVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x6B, address, registerId);
}

void CX86Assembler::PackuswbVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x67, address, registerId);
}

void CX86Assembler::PaddbVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xFC, address, registerId);
}

void CX86Assembler::PaddusbVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xDC, address, registerId);
}

void CX86Assembler::PaddwVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xFD, address, registerId);
}

void CX86Assembler::PaddswVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xED, address, registerId);
}

void CX86Assembler::PadduswVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xDD, address, registerId);
}

void CX86Assembler::PadddVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xFE, address, registerId);
}

void CX86Assembler::PandVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xDB, address, registerId);
}

void CX86Assembler::PandnVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xDF, address, registerId);
}

void CX86Assembler::PcmpeqbVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x74, address, registerId);
}

void CX86Assembler::PcmpeqwVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x75, address, registerId);
}

void CX86Assembler::PcmpeqdVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x76, address, registerId);
}

void CX86Assembler::PcmpgtbVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x64, address, registerId);
}

void CX86Assembler::PcmpgtwVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x65, address, registerId);
}

void CX86Assembler::PcmpgtdVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x66, address, registerId);
}

void CX86Assembler::PmaxswVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xEE, address, registerId);
}

void CX86Assembler::PmaxsdVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F_38(0x3D, address, registerId);
}

void CX86Assembler::PminswVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xEA, address, registerId);
}

void CX86Assembler::PminsdVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F_38(0x39, address, registerId);
}

void CX86Assembler::PmovmskbVo(REGISTER srcReg, XMMREGISTER dstReg)
{
	WriteEdVdOp_66_0F(0xD7, CX86Assembler::MakeRegisterAddress(static_cast<REGISTER>(dstReg)), static_cast<XMMREGISTER>(srcReg));
}

void CX86Assembler::PorVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xEB, address, registerId);
}

void CX86Assembler::PshufbVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F_38(0x00, address, registerId);
}

void CX86Assembler::PshufdVo(XMMREGISTER registerId, const CAddress& address, uint8 shuffleByte)
{
	WriteEdVdOp_66_0F(0x70, address, registerId);
	WriteByte(shuffleByte);
}

void CX86Assembler::PsllwVo(XMMREGISTER registerId, uint8 amount)
{
	WriteVrOp_66_0F(0x71, 0x06, registerId);
	WriteByte(amount);
}

void CX86Assembler::PslldVo(XMMREGISTER registerId, uint8 amount)
{
	WriteVrOp_66_0F(0x72, 0x06, registerId);
	WriteByte(amount);
}

void CX86Assembler::PsrawVo(XMMREGISTER registerId, uint8 amount)
{
	WriteVrOp_66_0F(0x71, 0x04, registerId);
	WriteByte(amount);
}

void CX86Assembler::PsradVo(XMMREGISTER registerId, uint8 amount)
{
	WriteVrOp_66_0F(0x72, 0x04, registerId);
	WriteByte(amount);
}

void CX86Assembler::PsrlwVo(XMMREGISTER registerId, uint8 amount)
{
	WriteVrOp_66_0F(0x71, 0x02, registerId);
	WriteByte(amount);
}

void CX86Assembler::PsrldVo(XMMREGISTER registerId, uint8 amount)
{
	WriteVrOp_66_0F(0x72, 0x02, registerId);
	WriteByte(amount);
}

void CX86Assembler::PsubbVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xF8, address, registerId);
}

void CX86Assembler::PsubusbVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xD8, address, registerId);
}

void CX86Assembler::PsubswVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xE9, address, registerId);
}

void CX86Assembler::PsubuswVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xD9, address, registerId);
}

void CX86Assembler::PsubwVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xF9, address, registerId);
}

void CX86Assembler::PsubdVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xFA, address, registerId);
}

void CX86Assembler::PunpcklbwVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x60, address, registerId);
}

void CX86Assembler::PunpcklwdVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x61, address, registerId);
}

void CX86Assembler::PunpckldqVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x62, address, registerId);
}

void CX86Assembler::PunpckhbwVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x68, address, registerId);
}

void CX86Assembler::PunpckhwdVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x69, address, registerId);
}

void CX86Assembler::PunpckhdqVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0x6A, address, registerId);
}

void CX86Assembler::PxorVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_66_0F(0xEF, address, registerId);
}

void CX86Assembler::AddpsVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_0F(0x58, address, registerId);
}

void CX86Assembler::DivpsVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_0F(0x5E, address, registerId);
}

void CX86Assembler::Cvtdq2psVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_0F(0x5B, address, registerId);
}

void CX86Assembler::Cvttps2dqVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_F3_0F(0x5B, address, registerId);
}

void CX86Assembler::MaxpsVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_0F(0x5F, address, registerId);
}

void CX86Assembler::MinpsVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_0F(0x5D, address, registerId);
}

void CX86Assembler::MulpsVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_0F(0x59, address, registerId);
}

void CX86Assembler::SubpsVo(XMMREGISTER registerId, const CAddress& address)
{
	WriteEdVdOp_0F(0x5C, address, registerId);
}

void CX86Assembler::ShufpsVo(XMMREGISTER registerId, const CAddress& address, uint8 shuffleByte)
{
	WriteEdVdOp_0F(0xC6, address, registerId);
	WriteByte(shuffleByte);
}

//------------------------------------------------
//Addressing utils
//------------------------------------------------

void CX86Assembler::WriteEdVdOp(uint8 opcode, const CAddress& address, XMMREGISTER xmmRegisterId)
{
	REGISTER registerId = static_cast<REGISTER>(xmmRegisterId);
	WriteRexByte(false, address, registerId);
	CAddress NewAddress(address);
	NewAddress.ModRm.nFnReg = registerId;
	WriteByte(opcode);
	NewAddress.Write(&m_tmpStream);
}

void CX86Assembler::WriteEdVdOp_0F(uint8 opcode, const CAddress& address, XMMREGISTER xmmRegisterId)
{
	REGISTER registerId = static_cast<REGISTER>(xmmRegisterId);
	WriteRexByte(false, address, registerId);
	WriteByte(0x0F);
	CAddress NewAddress(address);
	NewAddress.ModRm.nFnReg = registerId;
	WriteByte(opcode);
	NewAddress.Write(&m_tmpStream);
}

void CX86Assembler::WriteEdVdOp_66_0F(uint8 opcode, const CAddress& address, XMMREGISTER xmmRegisterId)
{
	REGISTER registerId = static_cast<REGISTER>(xmmRegisterId);
	WriteByte(0x66);
	WriteRexByte(false, address, registerId);
	WriteByte(0x0F);
	CAddress NewAddress(address);
	NewAddress.ModRm.nFnReg = registerId;
	WriteByte(opcode);
	NewAddress.Write(&m_tmpStream);
}

void CX86Assembler::WriteEdVdOp_66_0F_64b(uint8 opcode, const CAddress& address, XMMREGISTER xmmRegisterId)
{
	auto registerId = static_cast<REGISTER>(xmmRegisterId);

	WriteByte(0x66);
	WriteRexByte(true, address, registerId);
	WriteByte(0x0F);
	
	CAddress newAddress(address);
	newAddress.ModRm.nFnReg = registerId;
	WriteByte(opcode);
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::WriteEdVdOp_66_0F_38(uint8 opcode, const CAddress& address, XMMREGISTER xmmRegisterId)
{
	REGISTER registerId = static_cast<REGISTER>(xmmRegisterId);
	WriteByte(0x66);
	WriteRexByte(false, address, registerId);
	WriteByte(0x0F);
	WriteByte(0x38);
	CAddress newAddress(address);
	newAddress.ModRm.nFnReg = registerId;
	WriteByte(opcode);
	newAddress.Write(&m_tmpStream);
}

void CX86Assembler::WriteEdVdOp_F3_0F(uint8 opcode, const CAddress& address, XMMREGISTER xmmRegisterId)
{
	REGISTER registerId = static_cast<REGISTER>(xmmRegisterId);
	WriteByte(0xF3);
	WriteRexByte(false, address, registerId);
	WriteByte(0x0F);
	CAddress NewAddress(address);
	NewAddress.ModRm.nFnReg = registerId;
	WriteByte(opcode);
	NewAddress.Write(&m_tmpStream);
}

void CX86Assembler::WriteVrOp_66_0F(uint8 opcode, uint8 subOpcode, XMMREGISTER registerId)
{
	CAddress address(MakeXmmRegisterAddress(registerId));
	WriteByte(0x66);
	WriteRexByte(false, address);
	WriteByte(0x0F);
	address.ModRm.nFnReg = subOpcode;
	WriteByte(opcode);
	address.Write(&m_tmpStream);
}
