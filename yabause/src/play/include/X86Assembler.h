#ifndef _X86ASSEMBLER_H_
#define _X86ASSEMBLER_H_

#include "Types.h"
#include "Stream.h"
#include "MemStream.h"
#include <map>
#include <vector>

class CX86Assembler
{
public:
	enum REGISTER
	{
		rAX = 0,
		rCX,
		rDX,
		rBX,
		rSP,
		rBP,
		rSI,
		rDI,
		r8,
		r9,
		r10,
		r11,
		r12,
		r13,
		r14,
		r15,
	};

	enum XMMREGISTER
	{
		xMM0 = 0,
		xMM1,
		xMM2,
		xMM3,
		xMM4,
		xMM5,
		xMM6,
		xMM7,
		xMM8,
		xMM9,
		xMM10,
		xMM11,
		xMM12,
		xMM13,
		xMM14,
		xMM15,
	};

	typedef unsigned int LABEL;

	class CAddress
	{
	public:
						CAddress();

		bool			nIsExtendedModRM;
		bool			nIsExtendedSib;

		union MODRMBYTE
		{
			struct
			{
				unsigned int nRM : 3;
				unsigned int nFnReg : 3;
				unsigned int nMod : 2;
			};
			uint8 nByte;
		};

		union SIB
		{
			struct
			{
				unsigned int base : 3;
				unsigned int index : 3;
				unsigned int scale : 2;
			};
			uint8 byteValue;
		};

		MODRMBYTE		ModRm;
		SIB				sib;
		uint32			nOffset;

		bool			HasSib() const;
		void			Write(Framework::CStream*);
	};

											CX86Assembler();
	virtual									~CX86Assembler();

	void									Begin();
	void									End();

	void									SetStream(Framework::CStream*);

	static CAddress							MakeRegisterAddress(REGISTER);
	static CAddress							MakeXmmRegisterAddress(XMMREGISTER);
	static CAddress							MakeByteRegisterAddress(REGISTER);
	static CAddress							MakeIndRegAddress(REGISTER);
	static CAddress							MakeIndRegOffAddress(REGISTER, uint32);
	static CAddress							MakeBaseIndexScaleAddress(REGISTER, REGISTER, uint8);

	static unsigned int						GetMinimumConstantSize(uint32);
	static unsigned int						GetMinimumConstantSize64(uint64);

	LABEL									CreateLabel();
	void									MarkLabel(LABEL, int32 = 0);
	uint32									GetLabelOffset(LABEL) const;

	void									AdcEd(REGISTER, const CAddress&);
	void									AdcId(const CAddress&, uint32);
	void									AddEd(REGISTER, const CAddress&);
	void									AddEq(REGISTER, const CAddress&);
	void									AddId(const CAddress&, uint32);
	void									AddIq(const CAddress&, uint64);
	void									AndEd(REGISTER, const CAddress&);
	void									AndEq(REGISTER, const CAddress&);
	void									AndIb(const CAddress&, uint8);
	void									AndId(const CAddress&, uint32);
	void									AndIq(const CAddress&, uint64);
	void									BsrEd(REGISTER, const CAddress&);
	void									CallEd(const CAddress&);
	void									CmovsEd(REGISTER, const CAddress&);
	void									CmovnsEd(REGISTER, const CAddress&);
	void									CmpEd(REGISTER, const CAddress&);
	void									CmpEq(REGISTER, const CAddress&);
	void									CmpIb(const CAddress&, uint8);
	void									CmpId(const CAddress&, uint32);
	void									CmpIq(const CAddress&, uint64);
	void									Cdq();
	void									DivEd(const CAddress&);
	void									IdivEd(const CAddress&);
	void									ImulEw(const CAddress&);
	void									ImulEd(const CAddress&);
	void									JbJx(LABEL);
	void									JnbJx(LABEL);
	void									JzJx(LABEL);
	void									JnlJx(LABEL);
	void									JnleJx(LABEL);
	void									JlJx(LABEL);
	void									JleJx(LABEL);
	void									JmpJx(LABEL);
	void									JnzJx(LABEL);
	void									JnoJx(LABEL);
	void									JnsJx(LABEL);
	void									LeaGd(REGISTER, const CAddress&);
	void									LeaGq(REGISTER, const CAddress&);
	void									MovEw(REGISTER, const CAddress&);
	void									MovEd(REGISTER, const CAddress&);
	void									MovEq(REGISTER, const CAddress&);
	void									MovGd(const CAddress&, REGISTER);
	void									MovGq(const CAddress&, REGISTER);
	void									MovId(REGISTER, uint32);
	void									MovIq(REGISTER, uint64);
	void									MovId(const CAddress&, uint32);
	void									MovsxEb(REGISTER, const CAddress&);
	void									MovsxEw(REGISTER, const CAddress&);
	void									MovzxEb(REGISTER, const CAddress&);
	void									MulEd(const CAddress&);
	void									NegEd(const CAddress&);
	void									Nop();
	void									NotEd(const CAddress&);
	void									OrEd(REGISTER, const CAddress&);
	void									OrId(const CAddress&, uint32);
	void									Pop(REGISTER);
	void									Push(REGISTER);
	void									PushEd(const CAddress&);
	void									PushId(uint32);
	void									RclEd(const CAddress&, uint8);
	void									RepMovsb();
	void									Ret();
	void									SarEd(const CAddress&);
	void									SarEd(const CAddress&, uint8);
	void									SarEq(const CAddress&);
	void									SarEq(const CAddress&, uint8);
	void									SbbEd(REGISTER, const CAddress&);
	void									SbbId(const CAddress&, uint32);
	void									SetaEb(const CAddress&);
	void									SetaeEb(const CAddress&);
	void									SetbEb(const CAddress&);
	void									SetbeEb(const CAddress&);
	void									SeteEb(const CAddress&);
	void									SetneEb(const CAddress&);
	void									SetlEb(const CAddress&);
	void									SetleEb(const CAddress&);
	void									SetgEb(const CAddress&);
	void									ShrEd(const CAddress&);
	void									ShrEd(const CAddress&, uint8);
	void									ShrEq(const CAddress&);
	void									ShrEq(const CAddress&, uint8);
	void									ShrdEd(const CAddress&, REGISTER);
	void									ShrdEd(const CAddress&, REGISTER, uint8);
	void									ShlEd(const CAddress&);
	void									ShlEd(const CAddress&, uint8);
	void									ShlEq(const CAddress&);
	void									ShlEq(const CAddress&, uint8);
	void									ShldEd(const CAddress&, REGISTER);
	void									ShldEd(const CAddress&, REGISTER, uint8);
	void									SubEd(REGISTER, const CAddress&);
	void									SubEq(REGISTER, const CAddress&);
	void									SubId(const CAddress&, uint32);
	void									SubIq(const CAddress&, uint64);
	void									TestEb(REGISTER, const CAddress&);
	void									TestEd(REGISTER, const CAddress&);
	void									XorEd(REGISTER, const CAddress&);
	void									XorId(const CAddress&, uint32);
	void									XorGd(const CAddress&, REGISTER);
	void									XorGq(const CAddress&, REGISTER);

	//FPU
	void									FldEd(const CAddress&);
	void									FildEd(const CAddress&);
	void									FstpEd(const CAddress&);
	void									FistpEd(const CAddress&);
	void									FisttpEd(const CAddress&);
	void									FaddpSt(uint8);
	void									FsubpSt(uint8);
	void									FmulpSt(uint8);
	void									FdivpSt(uint8);
	void									Fwait();
	void									Fsin();
	void									FnstcwEw(const CAddress&);
	void									FldcwEw(const CAddress&);

	//SSE

	enum SSE_CMP_TYPE
	{
		SSE_CMP_EQ = 0,
		SSE_CMP_LT = 1,
		SSE_CMP_LE = 2,
		SSE_CMP_UNORD = 3,
		SSE_CMP_NEQ = 4,
		SSE_CMP_NLT = 5,
		SSE_CMP_NLE = 6,
		SSE_CMP_ORD = 7,
	};

	void									MovdVo(XMMREGISTER, const CAddress&);
	void									MovdVo(const CAddress&, XMMREGISTER);
	void									MovqVo(XMMREGISTER, const CAddress&);
	void									MovdqaVo(XMMREGISTER, const CAddress&);
	void									MovdqaVo(const CAddress&, XMMREGISTER);
	void									MovdquVo(XMMREGISTER, const CAddress&);
	void									MovdquVo(const CAddress&, XMMREGISTER);
	void									MovapsVo(const CAddress&, XMMREGISTER);
	void									MovapsVo(XMMREGISTER, const CAddress&);
	void									PackssdwVo(XMMREGISTER, const CAddress&);
	void									PackuswbVo(XMMREGISTER, const CAddress&);

	void									PaddbVo(XMMREGISTER, const CAddress&);
	void									PaddusbVo(XMMREGISTER, const CAddress&);
	void									PaddwVo(XMMREGISTER, const CAddress&);
	void									PaddswVo(XMMREGISTER, const CAddress&);
	void									PadduswVo(XMMREGISTER, const CAddress&);
	void									PadddVo(XMMREGISTER, const CAddress&);

	void									PandVo(XMMREGISTER, const CAddress&);
	void									PandnVo(XMMREGISTER, const CAddress&);
	void									PcmpeqbVo(XMMREGISTER, const CAddress&);
	void									PcmpeqwVo(XMMREGISTER, const CAddress&);
	void									PcmpeqdVo(XMMREGISTER, const CAddress&);
	void									PcmpgtbVo(XMMREGISTER, const CAddress&);
	void									PcmpgtwVo(XMMREGISTER, const CAddress&);
	void									PcmpgtdVo(XMMREGISTER, const CAddress&);
	void									PmaxswVo(XMMREGISTER, const CAddress&);
	void									PmaxsdVo(XMMREGISTER, const CAddress&);
	void									PminswVo(XMMREGISTER, const CAddress&);
	void									PminsdVo(XMMREGISTER, const CAddress&);
	void									PmovmskbVo(REGISTER, XMMREGISTER);
	void									PorVo(XMMREGISTER, const CAddress&);
	void									PshufbVo(XMMREGISTER, const CAddress&);
	void									PshufdVo(XMMREGISTER, const CAddress&, uint8);
	void									PsllwVo(XMMREGISTER, uint8);
	void									PslldVo(XMMREGISTER, uint8);
	void									PsrawVo(XMMREGISTER, uint8);
	void									PsradVo(XMMREGISTER, uint8);
	void									PsrlwVo(XMMREGISTER, uint8);
	void									PsrldVo(XMMREGISTER, uint8);

	void									PsubbVo(XMMREGISTER, const CAddress&);
	void									PsubusbVo(XMMREGISTER, const CAddress&);
	void									PsubwVo(XMMREGISTER, const CAddress&);
	void									PsubswVo(XMMREGISTER, const CAddress&);
	void									PsubuswVo(XMMREGISTER, const CAddress&);
	void									PsubdVo(XMMREGISTER, const CAddress&);

	void									PunpcklbwVo(XMMREGISTER, const CAddress&);
	void									PunpcklwdVo(XMMREGISTER, const CAddress&);
	void									PunpckldqVo(XMMREGISTER, const CAddress&);
	void									PunpckhbwVo(XMMREGISTER, const CAddress&);
	void									PunpckhwdVo(XMMREGISTER, const CAddress&);
	void									PunpckhdqVo(XMMREGISTER, const CAddress&);
	void									PxorVo(XMMREGISTER, const CAddress&);

	void									MovssEd(const CAddress&, XMMREGISTER);
	void									MovssEd(XMMREGISTER, const CAddress&);
	void									AddssEd(XMMREGISTER, const CAddress&);
	void									SubssEd(XMMREGISTER, const CAddress&);
	void									MaxssEd(XMMREGISTER, const CAddress&);
	void									MinssEd(XMMREGISTER, const CAddress&);
	void									MulssEd(XMMREGISTER, const CAddress&);
	void									DivssEd(XMMREGISTER, const CAddress&);
	void									RcpssEd(XMMREGISTER, const CAddress&);
	void									RsqrtssEd(XMMREGISTER, const CAddress&);
	void									SqrtssEd(XMMREGISTER, const CAddress&);
	void									CmpssEd(XMMREGISTER, const CAddress&, SSE_CMP_TYPE);
	void									Cvtsi2ssEd(XMMREGISTER, const CAddress&);
	void									Cvttss2siEd(REGISTER, const CAddress&);
	void									Cvtdq2psVo(XMMREGISTER, const CAddress&);
	void									Cvttps2dqVo(XMMREGISTER, const CAddress&);

	void									AddpsVo(XMMREGISTER, const CAddress&);
	void									DivpsVo(XMMREGISTER, const CAddress&);
	void									MaxpsVo(XMMREGISTER, const CAddress&);
	void									MinpsVo(XMMREGISTER, const CAddress&);
	void									MulpsVo(XMMREGISTER, const CAddress&);
	void									SubpsVo(XMMREGISTER, const CAddress&);
	void									ShufpsVo(XMMREGISTER, const CAddress&, uint8);

private:
	enum JMP_TYPE
	{
		JMP_O	= 0,
		JMP_NO	= 1,
		JMP_B	= 2,
		JMP_NB	= 3,
		JMP_Z	= 4,
		JMP_NZ	= 5,
		JMP_BE	= 6,
		JMP_NBE	= 7,

		JMP_S	= 8,
		JMP_NS	= 9,
		JMP_P	= 10,
		JMP_NP	= 11,
		JMP_L	= 12,
		JMP_NL	= 13,
		JMP_LE	= 14,
		JMP_NLE	= 15,

		JMP_ALWAYS,
	};

	enum JMP_LENGTH
	{
		JMP_NOTSET,
		JMP_NEAR,
		JMP_FAR
	};

	struct LABELREF
	{
		LABELREF()
			: label(0)
			, offset(0)
			, type(JMP_ALWAYS)
			, length(JMP_NOTSET)
		{

		}

		LABEL		label;
		uint32		offset;
		JMP_TYPE	type;
		JMP_LENGTH	length;
	};

	typedef std::vector<LABELREF> LabelRefArray;

	struct LABELINFO
	{
		LABELINFO()
			: start(0)
			, size(0)
			, projectedStart(0)
		{

		}

		uint32			start;
		uint32			size;
		uint32			projectedStart;
		LabelRefArray	labelRefs;
	};

	typedef std::map<LABEL, LABELINFO> LabelMap;
	typedef std::vector<LABEL> LabelArray;
	typedef std::vector<uint8> ByteArray;

	void									WriteRexByte(bool, const CAddress&);
	void									WriteRexByte(bool, const CAddress&, REGISTER&);
	void									WriteEvOp(uint8, uint8, bool, const CAddress&);
	void									WriteEvGvOp(uint8, bool, const CAddress&, REGISTER);
	void									WriteEvGvOp0F(uint8, bool, const CAddress&, REGISTER);
	void									WriteEvIb(uint8, const CAddress&, uint8);
	void									WriteEvId(uint8, const CAddress&, uint32);
	void									WriteEvIq(uint8, const CAddress&, uint64);
	void									WriteEdVdOp(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_0F(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_66_0F(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_66_0F_64b(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_66_0F_38(uint8, const CAddress&, XMMREGISTER);
	void									WriteEdVdOp_F3_0F(uint8, const CAddress&, XMMREGISTER);
	void									WriteVrOp_66_0F(uint8, uint8, XMMREGISTER);
	void									WriteStOp(uint8, uint8, uint8);

	void									CreateLabelReference(LABEL, JMP_TYPE);

	void									IncrementJumpOffsetsLocal(LABELINFO&, LabelRefArray::iterator, unsigned int);
	void									IncrementJumpOffsets(LabelArray::const_iterator, unsigned int);

	static unsigned int						GetJumpSize(JMP_TYPE, JMP_LENGTH);
	static void								WriteJump(Framework::CStream*, JMP_TYPE, JMP_LENGTH, uint32);

	void									WriteByte(uint8);
	void									WriteDWord(uint32);

	LabelMap								m_labels;
	LabelArray								m_labelOrder;
	LABEL									m_nextLabelId;
	LABELINFO*								m_currentLabel;
	Framework::CStream*						m_outputStream;
	Framework::CMemStream					m_tmpStream;
	ByteArray								m_copyBuffer;
};

#endif
