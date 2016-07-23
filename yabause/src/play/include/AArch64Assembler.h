#pragma once

#include <map>
#include "Stream.h"

class CAArch64Assembler
{
public:
	enum REGISTER32
	{
		w0,  w1,  w2,  w3,
		w4,  w5,  w6,  w7,
		w8,  w9,  w10, w11,
		w12, w13, w14, w15,
		w16, w17, w18, w19,
		w20, w21, w22, w23,
		w24, w25, w26, w27,
		w28, w29, w30, wZR
	};

	enum REGISTER64
	{
		x0,  x1,  x2,  x3,
		x4,  x5,  x6,  x7,
		x8,  x9,  x10, x11,
		x12, x13, x14, x15,
		x16, x17, x18, x19,
		x20, x21, x22, x23,
		x24, x25, x26, x27,
		x28, x29, x30, xZR, xSP = 31
	};
	
	enum REGISTERMD
	{
		v0,  v1,  v2,  v3,
		v4,  v5,  v6,  v7,
		v8,  v9,  v10, v11,
		v12, v13, v14, v15,
		v16, v17, v18, v19,
		v20, v21, v22, v23,
		v24, v25, v26, v27,
		v28, v29, v30, v31
	};
	
	enum CONDITION
	{
		CONDITION_EQ,
		CONDITION_NE,
		CONDITION_CS,
		CONDITION_CC,
		CONDITION_MI,
		CONDITION_PL,
		CONDITION_VS,
		CONDITION_VC,
		CONDITION_HI,
		CONDITION_LS,
		CONDITION_GE,
		CONDITION_LT,
		CONDITION_GT,
		CONDITION_LE,
		CONDITION_AL,
		CONDITION_NV
	};

	enum ADDSUB_IMM_SHIFT_TYPE
	{
		ADDSUB_IMM_SHIFT_LSL0,
		ADDSUB_IMM_SHIFT_LSL12,
	};
	
	typedef unsigned int LABEL;
	
	           CAArch64Assembler();
	virtual    ~CAArch64Assembler();

	void    SetStream(Framework::CStream*);

	LABEL    CreateLabel();
	void     ClearLabels();
	void     MarkLabel(LABEL);
	void     ResolveLabelReferences();

	void    Add(REGISTER32, REGISTER32, REGISTER32);
	void    Add(REGISTER64, REGISTER64, REGISTER64);
	void    Add(REGISTER32, REGISTER32, uint16, ADDSUB_IMM_SHIFT_TYPE);
	void    Add(REGISTER64, REGISTER64, uint16, ADDSUB_IMM_SHIFT_TYPE);
	void    Add_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Add_8h(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Add_16b(REGISTERMD, REGISTERMD, REGISTERMD);
	void    And(REGISTER32, REGISTER32, REGISTER32);
	void    And(REGISTER64, REGISTER64, REGISTER64);
	void    And(REGISTER32, REGISTER32, uint8, uint8, uint8);
	void    And_16b(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Asr(REGISTER32, REGISTER32, uint8);
	void    Asr(REGISTER64, REGISTER64, uint8);
	void    Asrv(REGISTER32, REGISTER32, REGISTER32);
	void    Asrv(REGISTER64, REGISTER64, REGISTER64);
	void    B(LABEL);
	void    Bl(uint32);
	void    BCc(CONDITION, LABEL);
	void    Blr(REGISTER64);
	void    Clz(REGISTER32, REGISTER32);
	void    Cmeq_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Cmgt_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Cmn(REGISTER32, uint16, ADDSUB_IMM_SHIFT_TYPE);
	void    Cmn(REGISTER64, uint16, ADDSUB_IMM_SHIFT_TYPE);
	void    Cmp(REGISTER32, REGISTER32);
	void    Cmp(REGISTER64, REGISTER64);
	void    Cmp(REGISTER32, uint16, ADDSUB_IMM_SHIFT_TYPE);
	void    Cmp(REGISTER64, uint16, ADDSUB_IMM_SHIFT_TYPE);
	void    Cset(REGISTER32, CONDITION);
	void    Dup_4s(REGISTERMD, REGISTER32);
	void    Eor(REGISTER32, REGISTER32, REGISTER32);
	void    Eor(REGISTER32, REGISTER32, uint8, uint8, uint8);
	void    Eor_16b(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fabs_1s(REGISTERMD, REGISTERMD);
	void    Fabs_4s(REGISTERMD, REGISTERMD);
	void    Fadd_1s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fadd_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fcmeq_4s(REGISTERMD, REGISTERMD);
	void    Fcmlt_4s(REGISTERMD, REGISTERMD);
	void    Fcmp_1s(REGISTERMD, REGISTERMD);
	void    Fcvtzs_1s(REGISTERMD, REGISTERMD);
	void    Fcvtzs_4s(REGISTERMD, REGISTERMD);
	void    Fdiv_1s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fdiv_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fmov_1s(REGISTERMD, uint8);
	void    Fmul_1s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fmul_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fmax_1s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fmax_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fneg_1s(REGISTERMD, REGISTERMD);
	void    Fmin_1s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fmin_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fsqrt_1s(REGISTERMD, REGISTERMD);
	void    Fsub_1s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Fsub_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Ins_1s(REGISTERMD, uint8, REGISTERMD, uint8);
	void    Ins_1d(REGISTERMD, uint8, REGISTER64);
	void    Ld1_4s(REGISTERMD, REGISTER64);
	void    Ldp_PostIdx(REGISTER64, REGISTER64, REGISTER64, int32);
	void    Ldr(REGISTER32, REGISTER64, uint32);
	void    Ldr(REGISTER64, REGISTER64, uint32);
	void    Ldr_1s(REGISTERMD, REGISTER64, uint32);
	void    Ldr_1q(REGISTERMD, REGISTER64, uint32);
	void    Lsl(REGISTER32, REGISTER32, uint8);
	void    Lsl(REGISTER64, REGISTER64, uint8);
	void    Lslv(REGISTER32, REGISTER32, REGISTER32);
	void    Lslv(REGISTER64, REGISTER64, REGISTER64);
	void    Lsr(REGISTER32, REGISTER32, uint8);
	void    Lsr(REGISTER64, REGISTER64, uint8);
	void    Lsrv(REGISTER32, REGISTER32, REGISTER32);
	void    Lsrv(REGISTER64, REGISTER64, REGISTER64);
	void    Mov(REGISTER32, REGISTER32);
	void    Mov(REGISTER64, REGISTER64);
	void    Mov(REGISTERMD, REGISTERMD);
	void    Mov_Sp(REGISTER64, REGISTER64);
	void    Movn(REGISTER32, uint16, uint8);
	void    Movk(REGISTER32, uint16, uint8);
	void    Movk(REGISTER64, uint16, uint8);
	void    Movz(REGISTER32, uint16, uint8);
	void    Movz(REGISTER64, uint16, uint8);
	void    Msub(REGISTER32, REGISTER32, REGISTER32, REGISTER32);
	void    Mvn(REGISTER32, REGISTER32);
	void    Orn_16b(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Orr(REGISTER32, REGISTER32, REGISTER32);
	void    Orr(REGISTER32, REGISTER32, uint8, uint8, uint8);
	void    Orr_16b(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Ret(REGISTER64 = x30);
	void    Scvtf_1s(REGISTERMD, REGISTERMD);
	void    Scvtf_4s(REGISTERMD, REGISTERMD);
	void    Sdiv(REGISTER32, REGISTER32, REGISTER32);
	void    Shl_4s(REGISTERMD, REGISTERMD, uint8);
	void    Shl_8h(REGISTERMD, REGISTERMD, uint8);
	void    Smax_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Smax_8h(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Smin_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Smin_8h(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Smull(REGISTER64, REGISTER32, REGISTER32);
	void    Sshr_4s(REGISTERMD, REGISTERMD, uint8);
	void    Sshr_8h(REGISTERMD, REGISTERMD, uint8);
	void    Sqadd_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Sqadd_8h(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Sqsub_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Sqsub_8h(REGISTERMD, REGISTERMD, REGISTERMD);
	void    St1_4s(REGISTERMD, REGISTER64);
	void    Stp(REGISTER32, REGISTER32, REGISTER64, int32);
	void    Stp_PreIdx(REGISTER64, REGISTER64, REGISTER64, int32);
	void    Str(REGISTER32, REGISTER64, uint32);
	void    Str(REGISTER64, REGISTER64, uint32);
	void    Str_1s(REGISTERMD, REGISTER64, uint32);
	void    Str_1q(REGISTERMD, REGISTER64, uint32);
	void    Sub(REGISTER32, REGISTER32, REGISTER32);
	void    Sub(REGISTER64, REGISTER64, REGISTER64);
	void    Sub(REGISTER32, REGISTER32, uint16, ADDSUB_IMM_SHIFT_TYPE);
	void    Sub(REGISTER64, REGISTER64, uint16, ADDSUB_IMM_SHIFT_TYPE);
	void    Sub_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Sub_8h(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Sub_16b(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Tst(REGISTER32, REGISTER32);
	void    Udiv(REGISTER32, REGISTER32, REGISTER32);
	void    Umov_1s(REGISTER32, REGISTERMD, uint8);
	void    Umull(REGISTER64, REGISTER32, REGISTER32);
	void    Uqadd_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Uqadd_8h(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Uqadd_16b(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Uqsub_8h(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Uqsub_16b(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Ushr_4s(REGISTERMD, REGISTERMD, uint8);
	void    Ushr_8h(REGISTERMD, REGISTERMD, uint8);
	void    Xtn1_4h(REGISTERMD, REGISTERMD);
	void    Xtn1_8b(REGISTERMD, REGISTERMD);
	void    Xtn2_8h(REGISTERMD, REGISTERMD);
	void    Xtn2_16b(REGISTERMD, REGISTERMD);
	void    Zip1_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Zip1_8h(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Zip1_16b(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Zip2_4s(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Zip2_8h(REGISTERMD, REGISTERMD, REGISTERMD);
	void    Zip2_16b(REGISTERMD, REGISTERMD, REGISTERMD);
	
private:
	struct LABELREF
	{
		size_t offset = 0;
		CONDITION condition;
	};
	
	typedef std::map<LABEL, size_t> LabelMapType;
	typedef std::multimap<LABEL, LABELREF> LabelReferenceMapType;
	
	void    CreateLabelReference(LABEL, CONDITION);

	void    WriteAddSubOpImm(uint32, uint32 shift, uint32 imm, uint32 rn, uint32 rd);
	void    WriteDataProcOpReg2(uint32, uint32 rm, uint32 rn, uint32 rd);
	void    WriteLogicalOpImm(uint32, uint32 n, uint32 immr, uint32 imms, uint32 rn, uint32 rd);
	void    WriteLoadStoreOpImm(uint32, uint32 imm, uint32 rn, uint32 rt);
	void    WriteMoveWideOpImm(uint32, uint32 hw, uint32 imm, uint32 rd);
	void    WriteWord(uint32);
	
	unsigned int             m_nextLabelId = 1;
	LabelMapType             m_labels;
	LabelReferenceMapType    m_labelReferences;
	
	Framework::CStream*    m_stream = nullptr;
};
