#include <assert.h>
#include <stdexcept>
#include "AArch32Assembler.h"

#define OPCODE_BKPT (0xE1200070)

CAArch32Assembler::CAArch32Assembler()
{

}

CAArch32Assembler::~CAArch32Assembler()
{
	
}

void CAArch32Assembler::SetStream(Framework::CStream* stream)
{
	m_stream = stream;
}

CAArch32Assembler::LdrAddress CAArch32Assembler::MakeImmediateLdrAddress(int32 immediate)
{
	LdrAddress result;
	memset(&result, 0, sizeof(result));
	result.isImmediate = true;
	if(immediate < 0)
	{
		result.isNegative = (immediate < 0);
		immediate = -immediate;
	}
	assert((immediate & ~0xFFF) == 0);
	result.immediate = static_cast<uint16>(immediate);
	return result;
}

CAArch32Assembler::ImmediateAluOperand CAArch32Assembler::MakeImmediateAluOperand(uint8 immediate, uint8 rotateAmount)
{
	ImmediateAluOperand operand;
	operand.immediate = immediate;
	operand.rotate = rotateAmount;
	return operand;
}

CAArch32Assembler::RegisterAluOperand CAArch32Assembler::MakeRegisterAluOperand(CAArch32Assembler::REGISTER registerId, const AluLdrShift& shift)
{
	RegisterAluOperand result;
	result.rm = registerId;
	result.shift = *reinterpret_cast<const uint8*>(&shift);
	return result;
}

CAArch32Assembler::AluLdrShift CAArch32Assembler::MakeConstantShift(SHIFT shiftType, uint8 amount)
{
	assert(!(shiftType == SHIFT_ASR && amount == 0));
	assert(!(shiftType == SHIFT_LSR && amount == 0));

	AluLdrShift result;
	result.typeBit = 0;
	result.type = shiftType;
	result.amount = amount;
	return result;
}

CAArch32Assembler::AluLdrShift CAArch32Assembler::MakeVariableShift(SHIFT shiftType, CAArch32Assembler::REGISTER registerId)
{
	AluLdrShift result;
	result.typeBit = 1;
	result.type = shiftType;
	result.amount = registerId << 1;
	return result;
}

CAArch32Assembler::LABEL CAArch32Assembler::CreateLabel()
{
	return m_nextLabelId++;
}

void CAArch32Assembler::ClearLabels()
{
	m_labels.clear();
}

void CAArch32Assembler::MarkLabel(LABEL label)
{
	m_labels[label] = static_cast<size_t>(m_stream->Tell());
}

void CAArch32Assembler::ResolveLabelReferences()
{
	for(const auto& labelReferencePair : m_labelReferences)
	{
		auto label(m_labels.find(labelReferencePair.first));
		if(label == m_labels.end())
		{
			throw std::runtime_error("Invalid label.");
		}
		size_t referencePos = labelReferencePair.second;
		size_t labelPos = label->second;
		int offset = static_cast<int>(labelPos - referencePos) / 4;
		offset -= 2;

		m_stream->Seek(referencePos, Framework::STREAM_SEEK_SET);
		m_stream->Write8(static_cast<uint8>(offset >> 0));
		m_stream->Write8(static_cast<uint8>(offset >> 8));
		m_stream->Write8(static_cast<uint8>(offset >> 16));
		m_stream->Seek(0, Framework::STREAM_SEEK_END);
	}
	m_labelReferences.clear();
}

void CAArch32Assembler::GenericAlu(ALU_OPCODE op, bool setFlags, REGISTER rd, REGISTER rn, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand		= rm;
	instruction.rn			= rn;
	instruction.rd			= rd;
	instruction.setFlags	= setFlags ? 1 : 0;
	instruction.opcode		= op;
	instruction.immediate	= 0;
	instruction.condition	= CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CAArch32Assembler::GenericAlu(ALU_OPCODE op, bool setFlags, REGISTER rd, REGISTER rn, 
	const ImmediateAluOperand& operand, CONDITION conditionCode)
{
	InstructionAlu instruction;
	instruction.operand		= *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd			= rd;
	instruction.rn			= rn;
	instruction.setFlags	= setFlags ? 1 : 0;
	instruction.opcode		= op;
	instruction.immediate	= 1;
	instruction.condition	= conditionCode;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CAArch32Assembler::CreateLabelReference(LABEL label)
{
	LABELREF reference = static_cast<size_t>(m_stream->Tell());
	m_labelReferences.insert(LabelReferenceMapType::value_type(label, reference));
}

void CAArch32Assembler::Adc(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_ADC, false, rd, rn, rm);
}

void CAArch32Assembler::Add(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_ADD, false, rd, rn, rm);
}

void CAArch32Assembler::Add(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	GenericAlu(ALU_OPCODE_ADD, false, rd, rn, operand);
}

void CAArch32Assembler::Adds(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_ADD, true, rd, rn, rm);
}

void CAArch32Assembler::And(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_AND, false, rd, rn, rm);
}

void CAArch32Assembler::And(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	GenericAlu(ALU_OPCODE_AND, false, rd, rn, operand);
}

void CAArch32Assembler::BCc(CONDITION condition, LABEL label)
{
	CreateLabelReference(label);
	uint32 opcode = (condition << 28) | (0x0A000000);
	WriteWord(opcode);
}

void CAArch32Assembler::Bic(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	GenericAlu(ALU_OPCODE_BIC, false, rd, rn, operand);
}

void CAArch32Assembler::Bx(REGISTER rn)
{
	uint32 opcode = 0;
	opcode = (CONDITION_AL << 28) | (0x12FFF10) | (rn);
	WriteWord(opcode);
}

void CAArch32Assembler::Blx(REGISTER rn)
{
	uint32 opcode = 0;
	opcode = (CONDITION_AL << 28) | (0x12FFF30) | (rn);
	WriteWord(opcode);
}

void CAArch32Assembler::Clz(REGISTER rd, REGISTER rm)
{
	uint32 opcode = 0x016F0F10;
	opcode |= CONDITION_AL << 28;
	opcode |= rm;
	opcode |= (rd << 12);
	WriteWord(opcode);
}

void CAArch32Assembler::Cmn(REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rn = rn;
	instruction.rd = 0;
	instruction.setFlags = 1;
	instruction.opcode = ALU_OPCODE_CMN;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CAArch32Assembler::Cmp(REGISTER rn, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rn = rn;
	instruction.rd = 0;
	instruction.setFlags = 1;
	instruction.opcode = ALU_OPCODE_CMP;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CAArch32Assembler::Cmp(REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rn = rn;
	instruction.rd = 0;
	instruction.setFlags = 1;
	instruction.opcode = ALU_OPCODE_CMP;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CAArch32Assembler::Eor(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_EOR, false, rd, rn, rm);
}

void CAArch32Assembler::Eor(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	GenericAlu(ALU_OPCODE_EOR, false, rd, rn, operand);
}

void CAArch32Assembler::Ldmia(REGISTER rbase, uint16 regList)
{
	//20 - Load
	//21 - Writeback
	//22 - User bit
	//23 - Down/Up (0/1)
	//24 - Post/Pre (0/1)
	uint32 opcode = (CONDITION_AL << 28) | (4 << 25) | (1 << 23) | (1 << 21) | (1 << 20) | (static_cast<uint32>(rbase) << 16) | regList;
	WriteWord(opcode);
}

void CAArch32Assembler::Ldr(REGISTER rd, REGISTER rbase, const LdrAddress& address)
{
	uint32 opcode = 0;
	assert(address.isImmediate);
	assert(!address.isNegative);
	opcode = (CONDITION_AL << 28) | (1 << 26) | (1 << 24) | (1 << 23) | (1 << 20) | (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(rd) << 12) | (static_cast<uint32>(address.immediate));
	WriteWord(opcode);
}

void CAArch32Assembler::Ldrd(REGISTER rt, REGISTER rn, const LdrAddress& address)
{
	assert(address.isImmediate);
	assert(!address.isNegative);
	assert(address.immediate < 0x100);
	uint32 opcode = 0x01C000D0;
	opcode |= (CONDITION_AL << 28);
	opcode |= (rn << 16);
	opcode |= (rt << 12);
	opcode |= (address.immediate >> 4) << 8;
	opcode |= (address.immediate & 0xF);
	WriteWord(opcode);
}

void CAArch32Assembler::Mov(REGISTER rd, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MOV;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CAArch32Assembler::Mov(REGISTER rd, const RegisterAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MOV;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CAArch32Assembler::Mov(REGISTER rd, const ImmediateAluOperand& operand)
{
	MovCc(CONDITION_AL, rd, operand);
}

void CAArch32Assembler::MovCc(CONDITION condition, REGISTER rd, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = 0;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MOV;
	instruction.immediate = 1;
	instruction.condition = condition;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CAArch32Assembler::Movw(REGISTER rd, uint16 value)
{
	uint32 opcode = 0x03000000;
	opcode |= (CONDITION_AL << 28);
	opcode |= (value >> 12) << 16;
	opcode |= (rd << 12);
	opcode |= (value & 0xFFF);
	WriteWord(opcode);
}

void CAArch32Assembler::Movt(REGISTER rd, uint16 value)
{
	uint32 opcode = 0x03400000;
	opcode |= (CONDITION_AL << 28);
	opcode |= (value >> 12) << 16;
	opcode |= (rd << 12);
	opcode |= (value & 0xFFF);
	WriteWord(opcode);
}

void CAArch32Assembler::Mvn(REGISTER rd, REGISTER rm)
{
	InstructionAlu instruction;
	instruction.operand = rm;
	instruction.rd = rd;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MVN;
	instruction.immediate = 0;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CAArch32Assembler::Mvn(REGISTER rd, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = rd;
	instruction.rn = 0;
	instruction.setFlags = 0;
	instruction.opcode = ALU_OPCODE_MVN;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);	
}

void CAArch32Assembler::Or(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_ORR, false, rd, rn, rm);
}

void CAArch32Assembler::Or(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	GenericAlu(ALU_OPCODE_ORR, false, rd, rn, operand);
}

void CAArch32Assembler::Or(CONDITION cc, REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	GenericAlu(ALU_OPCODE_ORR, false, rd, rn, operand, cc);
}

void CAArch32Assembler::Rsb(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	GenericAlu(ALU_OPCODE_RSB, false, rd, rn, operand);
}

void CAArch32Assembler::Sbc(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_SBC, false, rd, rn, rm);
}

void CAArch32Assembler::Sdiv(REGISTER rd, REGISTER rn, REGISTER rm)
{
	uint32 opcode = 0x0710F010;
	opcode |= (CONDITION_AL << 28);
	opcode |= (rd << 16);
	opcode |= (rm <<  8);
	opcode |= (rn <<  0);
	WriteWord(opcode);
}

void CAArch32Assembler::Smull(REGISTER rdLow, REGISTER rdHigh, REGISTER rn, REGISTER rm)
{
	uint32 opcode = 0;
	opcode = (CONDITION_AL << 28) | (0x06 << 21) | (rdHigh << 16) | (rdLow << 12) | (rm << 8) | (0x9 << 4) | (rn << 0);
	WriteWord(opcode);
}

void CAArch32Assembler::Stmdb(REGISTER rbase, uint16 regList)
{
	//20 - Load
	//21 - Writeback
	//22 - User bit
	//23 - Down/Up (0/1)
	//24 - Post/Pre (0/1)
	uint32 opcode = (CONDITION_AL << 28) | (4 << 25) | (1 << 24) | (1 << 21) | (static_cast<uint32>(rbase) << 16) | regList;
	WriteWord(opcode);
}

void CAArch32Assembler::Str(REGISTER rd, REGISTER rbase, const LdrAddress& address)
{
	assert(address.isImmediate);
	uint32 opcode = (CONDITION_AL << 28) | (1 << 26) | (1 << 24) | (0 << 20);
	opcode |= (address.isNegative) ? 0 : (1 << 23);
	opcode |= static_cast<uint32>(rbase) << 16;
	opcode |= static_cast<uint32>(rd) << 12;
	opcode |= static_cast<uint32>(address.immediate);
	WriteWord(opcode);
}

void CAArch32Assembler::Strd(REGISTER rt, REGISTER rn, const LdrAddress& address)
{
	assert(address.isImmediate);
	assert(!address.isNegative);
	assert(address.immediate < 0x100);
	uint32 opcode = 0x01C000F0;
	opcode |= (CONDITION_AL << 28);
	opcode |= (rn << 16);
	opcode |= (rt << 12);
	opcode |= (address.immediate >> 4) << 8;
	opcode |= (address.immediate & 0xF);
	WriteWord(opcode);
}

void CAArch32Assembler::Sub(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_SUB, false, rd, rn, rm);
}

void CAArch32Assembler::Sub(REGISTER rd, REGISTER rn, const ImmediateAluOperand& operand)
{
	GenericAlu(ALU_OPCODE_SUB, false, rd, rn, operand);
}

void CAArch32Assembler::Subs(REGISTER rd, REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_SUB, true, rd, rn, rm);
}

void CAArch32Assembler::Teq(REGISTER rn, const ImmediateAluOperand& operand)
{
	InstructionAlu instruction;
	instruction.operand = *reinterpret_cast<const unsigned int*>(&operand);
	instruction.rd = 0;
	instruction.rn = rn;
	instruction.setFlags = 1;
	instruction.opcode = ALU_OPCODE_TEQ;
	instruction.immediate = 1;
	instruction.condition = CONDITION_AL;
	uint32 opcode = *reinterpret_cast<uint32*>(&instruction);
	WriteWord(opcode);
}

void CAArch32Assembler::Tst(REGISTER rn, REGISTER rm)
{
	GenericAlu(ALU_OPCODE_TST, true, CAArch32Assembler::r0, rn, rm);
}

void CAArch32Assembler::Udiv(REGISTER rd, REGISTER rn, REGISTER rm)
{
	uint32 opcode = 0x0730F010;
	opcode |= (CONDITION_AL << 28);
	opcode |= (rd << 16);
	opcode |= (rm <<  8);
	opcode |= (rn <<  0);
	WriteWord(opcode);
}

void CAArch32Assembler::Umull(REGISTER rdLow, REGISTER rdHigh, REGISTER rn, REGISTER rm)
{
	uint32 opcode = 0;
	opcode = (CONDITION_AL << 28) | (0x04 << 21) | (rdHigh << 16) | (rdLow << 12) | (rm << 8) | (0x9 << 4) | (rn << 0);
	WriteWord(opcode);
}

//////////////////////////////////////////////////
// VFP/NEON Stuff
//////////////////////////////////////////////////

uint32 CAArch32Assembler::FPSIMD_EncodeSd(SINGLE_REGISTER sd)
{
	return ((sd >> 1) << 12) | ((sd & 1) << 22);
}

uint32 CAArch32Assembler::FPSIMD_EncodeSn(SINGLE_REGISTER sn)
{
	return ((sn >> 1) << 16) | ((sn & 1) <<  7);
}

uint32 CAArch32Assembler::FPSIMD_EncodeSm(SINGLE_REGISTER sm)
{
	return ((sm >> 1) <<  0) | ((sm & 1) <<  5);
}

uint32 CAArch32Assembler::FPSIMD_EncodeDd(DOUBLE_REGISTER dd)
{
	return ((dd & 0xF) << 12) | ((dd >> 4) << 22);
}

uint32 CAArch32Assembler::FPSIMD_EncodeDn(DOUBLE_REGISTER dn)
{
	return ((dn & 0xF) << 16) | ((dn >> 4) <<  7);
}

uint32 CAArch32Assembler::FPSIMD_EncodeDm(DOUBLE_REGISTER dm)
{
	return ((dm & 0x0F) <<  0) | ((dm >> 4) <<  5);
}

uint32 CAArch32Assembler::FPSIMD_EncodeQd(QUAD_REGISTER qd)
{
	assert((qd & 1) == 0);
	return ((qd & 0xF) << 12) | ((qd >> 4) << 22);
}

uint32 CAArch32Assembler::FPSIMD_EncodeQn(QUAD_REGISTER qn)
{
	assert((qn & 1) == 0);
	return ((qn & 0xF) << 16) | ((qn >> 4) <<  7);
}

uint32 CAArch32Assembler::FPSIMD_EncodeQm(QUAD_REGISTER qm)
{
	assert((qm & 1) == 0);
	return ((qm & 0x0F) <<  0) | ((qm >> 4) <<  5);
}

void CAArch32Assembler::Vldr(SINGLE_REGISTER sd, REGISTER rbase, const LdrAddress& address)
{
	assert(address.isImmediate);
	assert(!address.isNegative);
	assert((address.immediate / 4) <= 0xFF);

	uint32 opcode = 0x0D900A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(address.immediate / 4));
	WriteWord(opcode);
}

void CAArch32Assembler::Vld1_32x2(DOUBLE_REGISTER dd, REGISTER rn)
{
	//TODO: Make this aligned

	uint32 opcode = 0xF420078F;
	opcode |= FPSIMD_EncodeDd(dd);
	opcode |= static_cast<uint32>(rn) << 16;
	WriteWord(opcode);
}

void CAArch32Assembler::Vld1_32x4(QUAD_REGISTER qd, REGISTER rn)
{
	uint32 opcode = 0xF4200AAF;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= static_cast<uint32>(rn) << 16;
	WriteWord(opcode);
}

void CAArch32Assembler::Vld1_32x4_u(QUAD_REGISTER qd, REGISTER rn)
{
	//Unaligned variant
	uint32 opcode = 0xF4200A8F;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= static_cast<uint32>(rn) << 16;
	WriteWord(opcode);
}

void CAArch32Assembler::Vstr(SINGLE_REGISTER sd, REGISTER rbase, const LdrAddress& address)
{
	assert(address.isImmediate);
	assert(!address.isNegative);
	assert((address.immediate / 4) <= 0xFF);

	uint32 opcode = 0x0D800A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= (static_cast<uint32>(rbase) << 16) | (static_cast<uint32>(address.immediate / 4));
	WriteWord(opcode);
}

void CAArch32Assembler::Vst1_32x4(QUAD_REGISTER qd, REGISTER rn)
{
	uint32 opcode = 0xF4000AAF;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= static_cast<uint32>(rn) << 16;
	WriteWord(opcode);
}

void CAArch32Assembler::Vmov(DOUBLE_REGISTER dd, REGISTER rt, uint8 offset)
{
	uint32 opcode = 0x0E000B10;
	opcode |= (CONDITION_AL << 28);
	opcode |= (offset != 0) ? 0x00200000 : 0;
	opcode |= FPSIMD_EncodeDn(dd);
	opcode |= (rt << 12);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmov(REGISTER rt, DOUBLE_REGISTER dn, uint8 offset)
{
	uint32 opcode = 0x0E100B10;
	opcode |= (CONDITION_AL << 28);
	opcode |= (offset != 0) ? 0x00200000 : 0;
	opcode |= FPSIMD_EncodeDn(dn);
	opcode |= (rt << 12);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmovn_I16(DOUBLE_REGISTER dd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3B20200;
	opcode |= FPSIMD_EncodeDd(dd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmovn_I32(DOUBLE_REGISTER dd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3B60200;
	opcode |= FPSIMD_EncodeDd(dd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vdup(QUAD_REGISTER qd, REGISTER rt)
{
	uint32 opcode = 0x0EA00B10;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeQn(qd);
	opcode |= (rt << 12);
	WriteWord(opcode);
}

void CAArch32Assembler::Vzip_I8(DOUBLE_REGISTER dd, DOUBLE_REGISTER dm)
{
	uint32 opcode = 0xF3B20180;
	opcode |= FPSIMD_EncodeDd(dd);
	opcode |= FPSIMD_EncodeDm(dm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vzip_I16(DOUBLE_REGISTER dd, DOUBLE_REGISTER dm)
{
	uint32 opcode = 0xF3B60180;
	opcode |= FPSIMD_EncodeDd(dd);
	opcode |= FPSIMD_EncodeDm(dm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vzip_I32(QUAD_REGISTER qd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3BA01C0;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vadd_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sn, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0E300A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSn(sn);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vadd_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2000D40;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vadd_I8(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2000840;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vadd_I16(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2100840;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vadd_I32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200840;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vqadd_U8(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3000050;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vqadd_U16(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3100050;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vqadd_U32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3200050;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vqadd_I16(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2100050;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vqadd_I32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200050;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vsub_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sn, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0E300A40;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSn(sn);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vsub_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200D40;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vsub_I8(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3000840;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vsub_I16(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3100840;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vsub_I32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3200840;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vqsub_U8(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3000250;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vqsub_U16(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3100250;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vqsub_I16(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2100250;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vqsub_I32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200250;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmul_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sn, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0E200A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSn(sn);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmul_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3000D50;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vdiv_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sn, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0E800A00;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSn(sn);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vand(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2000150;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vorn(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2300150;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vorr(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200150;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Veor(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3000150;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vshl_I16(QUAD_REGISTER qd, QUAD_REGISTER qm, uint8 shiftAmount)
{
	uint32 opcode = 0xF2800550;
	opcode |= (0x10 + (shiftAmount & 0xF)) << 16;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vshl_I32(QUAD_REGISTER qd, QUAD_REGISTER qm, uint8 shiftAmount)
{
	uint32 opcode = 0xF2800550;
	opcode |= (0x20 + (shiftAmount & 0x1F)) << 16;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vshr_U16(QUAD_REGISTER qd, QUAD_REGISTER qm, uint8 shiftAmount)
{
	uint32 opcode = 0xF3800050;
	opcode |= (0x20 - (shiftAmount & 0xF)) << 16;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vshr_U32(QUAD_REGISTER qd, QUAD_REGISTER qm, uint8 shiftAmount)
{
	uint32 opcode = 0xF3800050;
	opcode |= (0x40 - (shiftAmount & 0x1F)) << 16;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vshr_I16(QUAD_REGISTER qd, QUAD_REGISTER qm, uint8 shiftAmount)
{
	uint32 opcode = 0xF2800050;
	opcode |= (0x20 - (shiftAmount & 0xF)) << 16;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vshr_I32(QUAD_REGISTER qd, QUAD_REGISTER qm, uint8 shiftAmount)
{
	uint32 opcode = 0xF2800050;
	opcode |= (0x40 - (shiftAmount & 0x1F)) << 16;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vabs_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB00AC0;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vabs_F32(QUAD_REGISTER qd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3B90740;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vneg_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB10A40;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vsqrt_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB10AC0;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vceq_I32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3200850;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vcgt_I16(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2100340;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vcmp_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB40A40;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vcvt_F32_S32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EB80AC0;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vcvt_F32_S32(QUAD_REGISTER qd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3BB0640;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vcvt_S32_F32(SINGLE_REGISTER sd, SINGLE_REGISTER sm)
{
	uint32 opcode = 0x0EBD0AC0;
	opcode |= (CONDITION_AL << 28);
	opcode |= FPSIMD_EncodeSd(sd);
	opcode |= FPSIMD_EncodeSm(sm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vcvt_S32_F32(QUAD_REGISTER qd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3BB0740;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmrs(REGISTER rt)
{
	uint32 opcode = 0x0EF10A10;
	opcode |= (CONDITION_AL << 28);
	opcode |= (rt << 12);
	WriteWord(opcode);
}

void CAArch32Assembler::Vrecpe_F32(QUAD_REGISTER qd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3BB0540;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vrecps_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2000F50;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vrsqrte_F32(QUAD_REGISTER qd, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF3BB05C0;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vrsqrts_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200F50;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmin_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200F40;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmin_I16(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2100650;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmin_I32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200650;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmax_F32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2000F40;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmax_I16(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2100640;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::Vmax_I32(QUAD_REGISTER qd, QUAD_REGISTER qn, QUAD_REGISTER qm)
{
	uint32 opcode = 0xF2200640;
	opcode |= FPSIMD_EncodeQd(qd);
	opcode |= FPSIMD_EncodeQn(qn);
	opcode |= FPSIMD_EncodeQm(qm);
	WriteWord(opcode);
}

void CAArch32Assembler::WriteWord(uint32 value)
{
	m_stream->Write32(value);
}
