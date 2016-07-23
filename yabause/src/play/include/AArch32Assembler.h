#pragma once

#include <string.h>
#include <functional>
#include <map>
#include "Stream.h"
#include "Types.h"

class CAArch32Assembler
{
public:
	enum REGISTER
	{
		r0 = 0,
		r1 = 1,
		r2 = 2,
		r3 = 3,
		r4 = 4,
		r5 = 5,
		r6 = 6,
		r7 = 7,
		r8 = 8,
		r9 = 9,
		r10 = 10,
		r11 = 11,
		r12 = 12,
		r13 = 13,
		r14 = 14,
		r15 = 15,
		
		rIP = 12,
		rSP = 13,
		rLR = 14,
		rPC = 15,
	};
	
	enum SINGLE_REGISTER
	{
		s0,		s1,		s2,		s3,
		s4,		s5,		s6,		s7,
		s8,		s9,		s10,	s11,
		s12,	s13,	s14,	s15,

		s16,	s17,	s18,	s19,
		s20,	s21,	s22,	s23,
		s24,	s25,	s26,	s27,
		s28,	s29,	s30,	s31,
	};
	
	enum DOUBLE_REGISTER
	{
		d0,		d1,		d2,		d3,
		d4,		d5,		d6,		d7,
		d8,		d9,		d10,	d11,
		d12,	d13,	d14,	d15,

		d16,	d17,	d18,	d19,
		d20,	d21,	d22,	d23,
		d24,	d25,	d26,	d27,
		d28,	d29,	d30,	d31,
	};

	enum QUAD_REGISTER
	{
		q0 = 0,		q1 = 2,		q2 = 4,		q3 = 6,
		q4 = 8,		q5 = 10,	q6 = 12,	q7 = 14,
		q8 = 16,	q9 = 18,	q10 = 20,	q11 = 22,
		q12 = 24,	q13 = 26,	q14 = 28,	q15 = 30,
	};

	enum ALU_OPCODE
	{
		ALU_OPCODE_AND = 0x00,
		ALU_OPCODE_EOR = 0x01,
		ALU_OPCODE_SUB = 0x02,
		ALU_OPCODE_RSB = 0x03,
		ALU_OPCODE_ADD = 0x04,
		ALU_OPCODE_ADC = 0x05,
		ALU_OPCODE_SBC = 0x06,
		ALU_OPCODE_RSC = 0x07,
		ALU_OPCODE_TST = 0x08,
		ALU_OPCODE_TEQ = 0x09,
		ALU_OPCODE_CMP = 0x0A,
		ALU_OPCODE_CMN = 0x0B,
		ALU_OPCODE_ORR = 0x0C,
		ALU_OPCODE_MOV = 0x0D,
		ALU_OPCODE_BIC = 0x0E,
		ALU_OPCODE_MVN = 0x0F,
	};
	
	enum CONDITION
	{
		CONDITION_EQ = 0x00,
		CONDITION_NE = 0x01,
		CONDITION_CS = 0x02,
		CONDITION_CC = 0x03,
		CONDITION_MI = 0x04,
		CONDITION_PL = 0x05,
		CONDITION_VS = 0x06,
		CONDITION_VC = 0x07,
		CONDITION_HI = 0x08,
		CONDITION_LS = 0x09,
		CONDITION_GE = 0x0A,
		CONDITION_LT = 0x0B,
		CONDITION_GT = 0x0C,
		CONDITION_LE = 0x0D,
		CONDITION_AL = 0x0E,
		CONDITION_NV = 0x0F
	};
	
	enum SHIFT
	{
		SHIFT_LSL = 0x00,
		SHIFT_LSR = 0x01,
		SHIFT_ASR = 0x02,
		SHIFT_ROR = 0x03,
	};
	
	struct InstructionAlu
	{
		InstructionAlu()
		{
			memset(this, 0, sizeof(InstructionAlu));
		}
		
		unsigned int		operand		: 12;
		unsigned int		rd			: 4;
		unsigned int		rn			: 4;
		unsigned int		setFlags	: 1;
		unsigned int		opcode		: 4;
		unsigned int		immediate	: 1;
		unsigned int		reserved	: 2;
		unsigned int		condition	: 4;
	};
	
	struct ImmediateAluOperand
	{
		unsigned int	immediate	: 8;
		unsigned int	rotate		: 4;
	};

	struct RegisterAluOperand
	{
		unsigned int	rm			: 4;
		unsigned int	shift		: 8;
	};
	
	struct AluLdrShift
	{
		unsigned int	typeBit		: 1;
		unsigned int	type		: 2;
		unsigned int	amount		: 5;
	};
	
	struct ShiftRmAddress
	{
		union
		{
			struct
			{
				unsigned int	rm			: 4;
				unsigned int	isConstant	: 1;
				unsigned int	shiftType	: 2;
				unsigned int	constant	: 5;
			} immediateShift;
			struct
			{
				unsigned int	rm			: 4;
				unsigned int	isConstant	: 1;
				unsigned int	shiftType	: 2;
				unsigned int	zero		: 1;
				unsigned int	rs			: 4;
			} registerShift;
		};
	};

	struct LdrAddress
	{
		union
		{
			uint16			immediate;
			ShiftRmAddress	shiftRm;
		};
		bool isImmediate = false;
		bool isNegative = false;
	};
	
	typedef unsigned int LABEL;
	
	
											CAArch32Assembler();
	virtual									~CAArch32Assembler();

	void									SetStream(Framework::CStream*);

	LABEL									CreateLabel();
	void									ClearLabels();
	void									MarkLabel(LABEL);
	void									ResolveLabelReferences();
	
	void									Adc(REGISTER, REGISTER, REGISTER);
	void									Add(REGISTER, REGISTER, REGISTER);
	void									Add(REGISTER, REGISTER, const ImmediateAluOperand&);
	void									Adds(REGISTER, REGISTER, REGISTER);
	void									And(REGISTER, REGISTER, REGISTER);
	void									And(REGISTER, REGISTER, const ImmediateAluOperand&);
	void									BCc(CONDITION, LABEL);
	void									Bic(REGISTER, REGISTER, const ImmediateAluOperand&);
	void									Bx(REGISTER);
	void									Blx(REGISTER);
	void									Clz(REGISTER, REGISTER);
	void									Cmn(REGISTER, const ImmediateAluOperand&);
	void									Cmp(REGISTER, REGISTER);
	void									Cmp(REGISTER, const ImmediateAluOperand&);
	void									Eor(REGISTER, REGISTER, REGISTER);
	void									Eor(REGISTER, REGISTER, const ImmediateAluOperand&);
	void									Ldmia(REGISTER, uint16);
	void									Ldr(REGISTER, REGISTER, const LdrAddress&);
	void									Ldrd(REGISTER, REGISTER, const LdrAddress&);
	void									Mov(REGISTER, REGISTER);
	void									Mov(REGISTER, const RegisterAluOperand&);
	void									Mov(REGISTER, const ImmediateAluOperand&);
	void									MovCc(CONDITION, REGISTER, const ImmediateAluOperand&);
	void									Movw(REGISTER, uint16);
	void									Movt(REGISTER, uint16);
	void									Mvn(REGISTER, REGISTER);
	void									Mvn(REGISTER, const ImmediateAluOperand&);	
	void									Or(REGISTER, REGISTER, REGISTER);
	void									Or(REGISTER, REGISTER, const ImmediateAluOperand&);
	void									Or(CONDITION, REGISTER, REGISTER, const ImmediateAluOperand&);
	void									Rsb(REGISTER, REGISTER, const ImmediateAluOperand&);
	void									Sbc(REGISTER, REGISTER, REGISTER);
	void									Sdiv(REGISTER, REGISTER, REGISTER);
	void									Smull(REGISTER, REGISTER, REGISTER, REGISTER);
	void									Stmdb(REGISTER, uint16);
	void									Str(REGISTER, REGISTER, const LdrAddress&);
	void									Strd(REGISTER, REGISTER, const LdrAddress&);
	void									Sub(REGISTER, REGISTER, REGISTER);
	void									Sub(REGISTER, REGISTER, const ImmediateAluOperand&);
	void									Subs(REGISTER, REGISTER, REGISTER);
	void									Teq(REGISTER, const ImmediateAluOperand&);
	void									Tst(REGISTER, REGISTER);
	void									Udiv(REGISTER, REGISTER, REGISTER);
	void									Umull(REGISTER, REGISTER, REGISTER, REGISTER);

	//VFP/NEON
	void									Vldr(SINGLE_REGISTER, REGISTER, const LdrAddress&);
	void									Vld1_32x2(DOUBLE_REGISTER, REGISTER);
	void									Vld1_32x4(QUAD_REGISTER, REGISTER);
	void									Vld1_32x4_u(QUAD_REGISTER, REGISTER);
	void									Vstr(SINGLE_REGISTER, REGISTER, const LdrAddress&);
	void									Vst1_32x4(QUAD_REGISTER, REGISTER);
	void									Vmov(DOUBLE_REGISTER, REGISTER, uint8);
	void									Vmov(REGISTER, DOUBLE_REGISTER, uint8);
	void									Vmovn_I16(DOUBLE_REGISTER, QUAD_REGISTER);
	void									Vmovn_I32(DOUBLE_REGISTER, QUAD_REGISTER);
	void									Vdup(QUAD_REGISTER, REGISTER);
	void									Vzip_I8(DOUBLE_REGISTER, DOUBLE_REGISTER);
	void									Vzip_I16(DOUBLE_REGISTER, DOUBLE_REGISTER);
	void									Vzip_I32(QUAD_REGISTER, QUAD_REGISTER);
	void									Vadd_F32(SINGLE_REGISTER, SINGLE_REGISTER, SINGLE_REGISTER);
	void									Vadd_F32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vadd_I8(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vadd_I16(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vadd_I32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vqadd_U8(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vqadd_U16(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vqadd_U32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vqadd_I16(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vqadd_I32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vsub_F32(SINGLE_REGISTER, SINGLE_REGISTER, SINGLE_REGISTER);
	void									Vsub_F32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vsub_I8(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vsub_I16(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vsub_I32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vqsub_U8(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vqsub_U16(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vqsub_I16(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vqsub_I32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vmul_F32(SINGLE_REGISTER, SINGLE_REGISTER, SINGLE_REGISTER);
	void									Vmul_F32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vdiv_F32(SINGLE_REGISTER, SINGLE_REGISTER, SINGLE_REGISTER);
	void									Vand(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vorn(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vorr(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Veor(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vshl_I16(QUAD_REGISTER, QUAD_REGISTER, uint8);
	void									Vshl_I32(QUAD_REGISTER, QUAD_REGISTER, uint8);
	void									Vshr_U16(QUAD_REGISTER, QUAD_REGISTER, uint8);
	void									Vshr_U32(QUAD_REGISTER, QUAD_REGISTER, uint8);
	void									Vshr_I16(QUAD_REGISTER, QUAD_REGISTER, uint8);
	void									Vshr_I32(QUAD_REGISTER, QUAD_REGISTER, uint8);
	void									Vabs_F32(SINGLE_REGISTER, SINGLE_REGISTER);
	void									Vabs_F32(QUAD_REGISTER, QUAD_REGISTER);
	void									Vneg_F32(SINGLE_REGISTER, SINGLE_REGISTER);
	void									Vsqrt_F32(SINGLE_REGISTER, SINGLE_REGISTER);
	void									Vceq_I32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vcgt_I16(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vcmp_F32(SINGLE_REGISTER, SINGLE_REGISTER);
	void									Vcvt_F32_S32(SINGLE_REGISTER, SINGLE_REGISTER);
	void									Vcvt_F32_S32(QUAD_REGISTER, QUAD_REGISTER);
	void									Vcvt_S32_F32(SINGLE_REGISTER, SINGLE_REGISTER);
	void									Vcvt_S32_F32(QUAD_REGISTER, QUAD_REGISTER);
	void									Vmrs(REGISTER);

	void									Vrecpe_F32(QUAD_REGISTER, QUAD_REGISTER);
	void									Vrecps_F32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vrsqrte_F32(QUAD_REGISTER, QUAD_REGISTER);
	void									Vrsqrts_F32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vmin_F32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vmin_I16(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vmin_I32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vmax_F32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vmax_I16(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);
	void									Vmax_I32(QUAD_REGISTER, QUAD_REGISTER, QUAD_REGISTER);

	static LdrAddress						MakeImmediateLdrAddress(int32);
	static ImmediateAluOperand				MakeImmediateAluOperand(uint8, uint8);
	static RegisterAluOperand				MakeRegisterAluOperand(CAArch32Assembler::REGISTER, const AluLdrShift&);
	
	static AluLdrShift						MakeConstantShift(CAArch32Assembler::SHIFT, uint8);
	static AluLdrShift						MakeVariableShift(CAArch32Assembler::SHIFT, CAArch32Assembler::REGISTER);
	
private:
	typedef size_t LABELREF;
	
	typedef std::map<LABEL, size_t> LabelMapType;
	typedef std::multimap<LABEL, LABELREF> LabelReferenceMapType;
	
	void									GenericAlu(ALU_OPCODE, bool, REGISTER, REGISTER, REGISTER);
	void									GenericAlu(ALU_OPCODE, bool, REGISTER, REGISTER, const ImmediateAluOperand&, CONDITION = CONDITION_AL);

	void									CreateLabelReference(LABEL);
	void									WriteWord(uint32);

	static uint32							FPSIMD_EncodeSd(SINGLE_REGISTER);
	static uint32							FPSIMD_EncodeSn(SINGLE_REGISTER);
	static uint32							FPSIMD_EncodeSm(SINGLE_REGISTER);
	static uint32							FPSIMD_EncodeDd(DOUBLE_REGISTER);
	static uint32							FPSIMD_EncodeDn(DOUBLE_REGISTER);
	static uint32							FPSIMD_EncodeDm(DOUBLE_REGISTER);
	static uint32							FPSIMD_EncodeQd(QUAD_REGISTER);
	static uint32							FPSIMD_EncodeQn(QUAD_REGISTER);
	static uint32							FPSIMD_EncodeQm(QUAD_REGISTER);

	unsigned int							m_nextLabelId = 1;
	LabelMapType							m_labels;
	LabelReferenceMapType					m_labelReferences;
	
	Framework::CStream*						m_stream = nullptr;
};
