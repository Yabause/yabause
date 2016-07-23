#pragma once

#include "Jitter_CodeGen.h"
#include "AArch32Assembler.h"
#include <deque>
#include <functional>
#include <array>

namespace Jitter
{
	class CCodeGen_AArch32 : public CCodeGen
	{
	public:
												CCodeGen_AArch32();
		virtual									~CCodeGen_AArch32();

		void									GenerateCode(const StatementList&, unsigned int) override;
		void									SetStream(Framework::CStream*) override;
		void									RegisterExternalSymbols(CObjectFile*) const override;
		unsigned int							GetAvailableRegisterCount() const override;
		unsigned int							GetAvailableMdRegisterCount() const override;
		bool									CanHold128BitsReturnValueInRegisters() const override;

	private:
		typedef std::map<uint32, CAArch32Assembler::LABEL> LabelMapType;
		typedef void (CCodeGen_AArch32::*ConstCodeEmitterType)(const STATEMENT&);

		struct PARAM_STATE
		{
			bool prepared = false;
			unsigned int index = 0;
		};

		class CTempRegisterContext
		{
		public:
			CAArch32Assembler::REGISTER Allocate()
			{
				return static_cast<CAArch32Assembler::REGISTER>(m_nextRegister++);
			}

			void Release(CAArch32Assembler::REGISTER reg)
			{
				m_nextRegister--;
				assert(reg == m_nextRegister);
			}

		private:
			uint8 m_nextRegister = CAArch32Assembler::r0;
		};

		typedef std::function<void (PARAM_STATE&)> ParamEmitterFunction;
		typedef std::deque<ParamEmitterFunction> ParamStack;
		
		enum MAX_PARAM_REGS
		{
			MAX_PARAM_REGS = 4,
		};
		
		enum
		{
			MAX_REGISTERS = 6,
		};

		struct CONSTMATCHER
		{
			OPERATION							op;
			MATCHTYPE							dstType;
			MATCHTYPE							src1Type;
			MATCHTYPE							src2Type;
			ConstCodeEmitterType				emitter;
		};

		static uint16							GetSavedRegisterList(uint32);

		void									Emit_Prolog(unsigned int, uint16);
		void									Emit_Epilog(unsigned int, uint16);

		CAArch32Assembler::LABEL				GetLabel(uint32);
		void									MarkLabel(const STATEMENT&);

		void									LoadMemoryInRegister(CAArch32Assembler::REGISTER, CSymbol*);
		void									StoreRegisterInMemory(CSymbol*, CAArch32Assembler::REGISTER);

		void									LoadMemoryReferenceInRegister(CAArch32Assembler::REGISTER, CSymbol*);

		void									LoadRelativeReferenceInRegister(CAArch32Assembler::REGISTER, CSymbol*);

		void									LoadTemporaryReferenceInRegister(CAArch32Assembler::REGISTER, CSymbol*);
		void									StoreRegisterInTemporaryReference(CSymbol*, CAArch32Assembler::REGISTER);
		
		uint32									GetMemory64Offset(CSymbol*) const;

		void									LoadMemory64LowInRegister(CAArch32Assembler::REGISTER, CSymbol*);
		void									LoadMemory64HighInRegister(CAArch32Assembler::REGISTER, CSymbol*);
		void									LoadMemory64InRegisters(CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER, CSymbol*);

		void									StoreRegisterInMemory64Low(CSymbol*, CAArch32Assembler::REGISTER);
		void									StoreRegisterInMemory64High(CSymbol*, CAArch32Assembler::REGISTER);
		void									StoreRegistersInMemory64(CSymbol*, CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER);

		void									LoadMemoryFpSingleInRegister(CTempRegisterContext&, CAArch32Assembler::SINGLE_REGISTER, CSymbol*);
		void									StoreRegisterInMemoryFpSingle(CTempRegisterContext&, CSymbol*, CAArch32Assembler::SINGLE_REGISTER);

		void									LoadRelativeFpSingleInRegister(CTempRegisterContext&, CAArch32Assembler::SINGLE_REGISTER, CSymbol*);
		void									StoreRelativeFpSingleInRegister(CTempRegisterContext&, CSymbol*, CAArch32Assembler::SINGLE_REGISTER);

		void									LoadTemporaryFpSingleInRegister(CTempRegisterContext&, CAArch32Assembler::SINGLE_REGISTER, CSymbol*);
		void									StoreTemporaryFpSingleInRegister(CTempRegisterContext&, CSymbol*, CAArch32Assembler::SINGLE_REGISTER);

		void									LoadMemory128AddressInRegister(CAArch32Assembler::REGISTER, CSymbol*, uint32 = 0);
		void									LoadRelative128AddressInRegister(CAArch32Assembler::REGISTER, CSymbol*, uint32);
		void									LoadTemporary128AddressInRegister(CAArch32Assembler::REGISTER, CSymbol*, uint32);

		void									LoadTemporary256ElementAddressInRegister(CAArch32Assembler::REGISTER, CSymbol*, uint32);

		CAArch32Assembler::REGISTER				PrepareSymbolRegisterDef(CSymbol*, CAArch32Assembler::REGISTER);
		CAArch32Assembler::REGISTER				PrepareSymbolRegisterUse(CSymbol*, CAArch32Assembler::REGISTER);
		void									CommitSymbolRegister(CSymbol*, CAArch32Assembler::REGISTER);

		typedef std::array<CAArch32Assembler::REGISTER, 2> ParamRegisterPair;

		CAArch32Assembler::REGISTER				PrepareParam(PARAM_STATE&);
		ParamRegisterPair						PrepareParam64(PARAM_STATE&);
		void									CommitParam(PARAM_STATE&);
		void									CommitParam64(PARAM_STATE&);

		CAArch32Assembler::AluLdrShift			GetAluShiftFromSymbol(CAArch32Assembler::SHIFT shiftType, CSymbol* symbol, CAArch32Assembler::REGISTER preferedRegister);

		static uint32							RotateRight(uint32);
		static uint32							RotateLeft(uint32);
		bool									TryGetAluImmediateParams(uint32, uint8&, uint8&);
		void									LoadConstantInRegister(CAArch32Assembler::REGISTER, uint32);
		void									LoadConstantPtrInRegister(CAArch32Assembler::REGISTER, uintptr_t);

		//ALUOP ----------------------------------------------------------
		struct ALUOP_BASE
		{
			typedef void (CAArch32Assembler::*OpImmType)(CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER, const CAArch32Assembler::ImmediateAluOperand&);
			typedef void (CAArch32Assembler::*OpRegType)(CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER);
			
			typedef void (CAArch32Assembler::*OpImmNegType)(CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER, const CAArch32Assembler::ImmediateAluOperand&);
			typedef void (CAArch32Assembler::*OpImmNotType)(CAArch32Assembler::REGISTER, CAArch32Assembler::REGISTER, const CAArch32Assembler::ImmediateAluOperand&);
		};
		
		struct ALUOP_ADD : public ALUOP_BASE
		{
			static OpImmType	OpImm()		{ return &CAArch32Assembler::Add; }
			static OpRegType	OpReg()		{ return &CAArch32Assembler::Add; }
			
			static OpImmNegType OpImmNeg()	{ return &CAArch32Assembler::Sub; }
			static OpImmNotType OpImmNot()	{ return NULL; }
		};
		
		struct ALUOP_SUB : public ALUOP_BASE
		{
			static OpImmType	OpImm()		{ return &CAArch32Assembler::Sub; }
			static OpRegType	OpReg()		{ return &CAArch32Assembler::Sub; }
			
			static OpImmNegType OpImmNeg()	{ return &CAArch32Assembler::Add; }
			static OpImmNotType OpImmNot()	{ return NULL; }
		};
		
		struct ALUOP_AND : public ALUOP_BASE
		{
			static OpImmType	OpImm()		{ return &CAArch32Assembler::And; }
			static OpRegType	OpReg()		{ return &CAArch32Assembler::And; }
			
			static OpImmNegType OpImmNeg()	{ return NULL; }
			static OpImmNotType OpImmNot()	{ return &CAArch32Assembler::Bic; }
		};

		struct ALUOP_OR : public ALUOP_BASE
		{
			static OpImmType	OpImm()		{ return &CAArch32Assembler::Or; }
			static OpRegType	OpReg()		{ return &CAArch32Assembler::Or; }
			
			static OpImmNegType OpImmNeg()	{ return NULL; }
			static OpImmNotType OpImmNot()	{ return NULL; }
		};
		
		struct ALUOP_XOR : public ALUOP_BASE
		{
			static OpImmType	OpImm()		{ return &CAArch32Assembler::Eor; }
			static OpRegType	OpReg()		{ return &CAArch32Assembler::Eor; }
			
			static OpImmNegType OpImmNeg()	{ return NULL; }
			static OpImmNotType OpImmNot()	{ return NULL; }
		};
		
		//FPUOP -----------------------------------------------------------
		struct FPUOP_BASE2
		{
			typedef void (CAArch32Assembler::*OpRegType)(CAArch32Assembler::SINGLE_REGISTER, CAArch32Assembler::SINGLE_REGISTER);
		};

		struct FPUOP_BASE3
		{
			typedef void (CAArch32Assembler::*OpRegType)(CAArch32Assembler::SINGLE_REGISTER, CAArch32Assembler::SINGLE_REGISTER, CAArch32Assembler::SINGLE_REGISTER);
		};

		struct FPUMDOP_BASE3
		{
			typedef void (CAArch32Assembler::*OpRegType)(CAArch32Assembler::QUAD_REGISTER, CAArch32Assembler::QUAD_REGISTER, CAArch32Assembler::QUAD_REGISTER);
		};

		struct FPUOP_ABS : public FPUOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vabs_F32; }
		};

		struct FPUOP_NEG : public FPUOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vneg_F32; }
		};

		struct FPUOP_SQRT : public FPUOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vsqrt_F32; }
		};

		struct FPUOP_ADD : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vadd_F32; }
		};

		struct FPUOP_SUB : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vsub_F32; }
		};

		struct FPUOP_MUL : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vmul_F32; }
		};

		struct FPUOP_DIV : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vdiv_F32; }
		};

		struct FPUMDOP_MIN : public FPUMDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vmin_F32; }
		};

		struct FPUMDOP_MAX : public FPUMDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vmax_F32; }
		};

		//MDOP -----------------------------------------------------------
		struct MDOP_BASE2
		{
			typedef void (CAArch32Assembler::*OpRegType)(CAArch32Assembler::QUAD_REGISTER, CAArch32Assembler::QUAD_REGISTER);
		};

		struct MDOP_BASE3
		{
			typedef void (CAArch32Assembler::*OpRegType)(CAArch32Assembler::QUAD_REGISTER, CAArch32Assembler::QUAD_REGISTER, CAArch32Assembler::QUAD_REGISTER);
		};

		struct MDOP_SHIFT
		{
			typedef void (CAArch32Assembler::*OpRegType)(CAArch32Assembler::QUAD_REGISTER, CAArch32Assembler::QUAD_REGISTER, uint8);
		};

		struct MDOP_ADDB : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vadd_I8; }
		};

		struct MDOP_ADDH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vadd_I16; }
		};

		struct MDOP_ADDW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vadd_I32; }
		};

		struct MDOP_ADDBUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vqadd_U8; }
		};

		struct MDOP_ADDHUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vqadd_U16; }
		};

		struct MDOP_ADDWUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vqadd_U32; }
		};

		struct MDOP_ADDHSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vqadd_I16; }
		};

		struct MDOP_ADDWSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vqadd_I32; }
		};

		struct MDOP_SUBB : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vsub_I8; }
		};

		struct MDOP_SUBH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vsub_I16; }
		};

		struct MDOP_SUBW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vsub_I32; }
		};

		struct MDOP_SUBBUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vqsub_U8; }
		};

		struct MDOP_SUBHUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vqsub_U16; }
		};

		struct MDOP_SUBHSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vqsub_I16; }
		};

		struct MDOP_SUBWSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vqsub_I32; }
		};

		struct MDOP_CMPEQW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vceq_I32; }
		};

		struct MDOP_CMPGTH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vcgt_I16; }
		};

		struct MDOP_MINH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vmin_I16; }
		};

		struct MDOP_MINW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vmin_I32; }
		};

		struct MDOP_MAXH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vmax_I16; }
		};

		struct MDOP_MAXW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vmax_I32; }
		};

		struct MDOP_AND : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vand; }
		};

		struct MDOP_OR : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vorr; }
		};

		struct MDOP_XOR : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Veor; }
		};

		struct MDOP_SLLH : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vshl_I16; }
		};

		struct MDOP_SRLH : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vshr_U16; }
		};

		struct MDOP_SRAH : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vshr_I16; }
		};

		struct MDOP_SLLW : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vshl_I32; }
		};

		struct MDOP_SRLW : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vshr_U32; }
		};

		struct MDOP_SRAW : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vshr_I32; }
		};

		struct MDOP_ADDS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vadd_F32; }
		};

		struct MDOP_SUBS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vsub_F32; }
		};

		struct MDOP_MULS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vmul_F32; }
		};

		struct MDOP_ABSS : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vabs_F32; }
		};

		struct MDOP_TOSINGLE : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vcvt_F32_S32; }
		};

		struct MDOP_TOWORD : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch32Assembler::Vcvt_S32_F32; }
		};

		//ALUOP
		template <typename> void				Emit_Alu_GenericAnyAny(const STATEMENT&);
		template <typename> void				Emit_Alu_GenericAnyCst(const STATEMENT&);

		//SHIFT
		template <CAArch32Assembler::SHIFT> void	Emit_Shift_Generic(const STATEMENT&);

		//PARAM
		void									Emit_Param_Ctx(const STATEMENT&);
		void									Emit_Param_Reg(const STATEMENT&);
		void									Emit_Param_Mem(const STATEMENT&);
		void									Emit_Param_Cst(const STATEMENT&);
		void									Emit_Param_Mem64(const STATEMENT&);
		void									Emit_Param_Cst64(const STATEMENT&);
		void									Emit_Param_Mem128(const STATEMENT&);

		//PARAM_RET
		void									Emit_ParamRet_Tmp128(const STATEMENT&);

		//CALL
		void									Emit_Call(const STATEMENT&);

		//RETVAL
		void									Emit_RetVal_Reg(const STATEMENT&);
		void									Emit_RetVal_Tmp(const STATEMENT&);
		void									Emit_RetVal_Mem64(const STATEMENT&);

		//MUL/MULS
		template<bool> void						Emit_MulTmp64AnyAny(const STATEMENT&);

		//DIV/DIVS
		template<bool> void						Div_GenericTmp64AnyAny(const STATEMENT&);
		template<bool> void						Div_GenericTmp64AnyAnySoft(const STATEMENT&);
		template<bool> void						Emit_DivTmp64AnyAny(const STATEMENT&);

		//MOV
		void									Emit_Mov_RegReg(const STATEMENT&);
		void									Emit_Mov_RegMem(const STATEMENT&);
		void									Emit_Mov_RegCst(const STATEMENT&);
		void									Emit_Mov_MemReg(const STATEMENT&);
		void									Emit_Mov_MemMem(const STATEMENT&);
		void									Emit_Mov_MemCst(const STATEMENT&);

		//LZC
		void									Emit_Lzc_VarVar(const STATEMENT&);

		//NOP
		void									Emit_Nop(const STATEMENT&);
		
		//EXTLOW64
		void									Emit_ExtLow64VarMem64(const STATEMENT&);

		//EXTHIGH64
		void									Emit_ExtHigh64VarMem64(const STATEMENT&);

		//MERGETO64
		void									Emit_MergeTo64_Mem64AnyAny(const STATEMENT&);

		//CMP
		void									Cmp_GetFlag(CAArch32Assembler::REGISTER, CONDITION);
		void									Cmp_GenericRegCst(CAArch32Assembler::REGISTER, uint32, CAArch32Assembler::REGISTER);
		void									Emit_Cmp_AnyAnyAny(const STATEMENT&);
		void									Emit_Cmp_AnyAnyCst(const STATEMENT&);

		//JMP
		void									Emit_Jmp(const STATEMENT&);

		//CONDJMP
		void									Emit_CondJmp(const STATEMENT&);
		void									Emit_CondJmp_VarVar(const STATEMENT&);
		void									Emit_CondJmp_VarCst(const STATEMENT&);
		
		//NOT
		void									Emit_Not_RegReg(const STATEMENT&);
		void									Emit_Not_MemReg(const STATEMENT&);
		void									Emit_Not_MemMem(const STATEMENT&);

		//RELTOREF
		void									Emit_RelToRef_TmpCst(const STATEMENT&);

		//ADDREF
		void									Emit_AddRef_TmpMemAny(const STATEMENT&);
		
		//LOADFROMREF
		void									Emit_LoadFromRef_VarTmp(const STATEMENT&);
		
		//STOREATREF
		void									Emit_StoreAtRef_TmpAny(const STATEMENT&);
		
		//MOV64
		void									Emit_Mov_Mem64Mem64(const STATEMENT&);
		void									Emit_Mov_Mem64Cst64(const STATEMENT&);

		//ADD64
		void									Emit_Add64_MemMemMem(const STATEMENT&);
		void									Emit_Add64_MemMemCst(const STATEMENT&);

		//SUB64
		void									Emit_Sub64_MemMemMem(const STATEMENT&);
		void									Emit_Sub64_MemMemCst(const STATEMENT&);
		void									Emit_Sub64_MemCstMem(const STATEMENT&);

		//AND64
		void									Emit_And64_MemMemMem(const STATEMENT&);

		//SLL64
		void									Emit_Sl64Var_MemMem(CSymbol*, CSymbol*, CAArch32Assembler::REGISTER);
		void									Emit_Sll64_MemMemVar(const STATEMENT&);
		void									Emit_Sll64_MemMemCst(const STATEMENT&);

		//SR64
		void									Emit_Sr64Var_MemMem(CSymbol*, CSymbol*, CAArch32Assembler::REGISTER, CAArch32Assembler::SHIFT);
		void									Emit_Sr64Cst_MemMem(CSymbol*, CSymbol*, uint32, CAArch32Assembler::SHIFT);

		//SRL64
		void									Emit_Srl64_MemMemVar(const STATEMENT&);
		void									Emit_Srl64_MemMemCst(const STATEMENT&);

		//SRA64
		void									Emit_Sra64_MemMemVar(const STATEMENT&);
		void									Emit_Sra64_MemMemCst(const STATEMENT&);

		//CMP64
		void									Cmp64_RegSymLo(CAArch32Assembler::REGISTER, CSymbol*, CAArch32Assembler::REGISTER);
		void									Cmp64_RegSymHi(CAArch32Assembler::REGISTER, CSymbol*, CAArch32Assembler::REGISTER);
		void									Cmp64_Equal(const STATEMENT&);
		void									Cmp64_Order(const STATEMENT&);
		void									Emit_Cmp64_VarMemAny(const STATEMENT&);

		//FPUOP
		template <typename> void				Emit_Fpu_MemMem(const STATEMENT&);
		template <typename> void				Emit_Fpu_MemMemMem(const STATEMENT&);
		template <typename> void				Emit_FpuMd_MemMemMem(const STATEMENT&);
		void									Emit_Fp_Rcpl_MemMem(const STATEMENT&);
		void									Emit_Fp_Rsqrt_MemMem(const STATEMENT&);
		void									Emit_Fp_Cmp_AnyMemMem(const STATEMENT&);
		void									Emit_Fp_Mov_MemSRelI32(const STATEMENT&);
		void									Emit_Fp_ToIntTrunc_MemMem(const STATEMENT&);
		void									Emit_Fp_LdCst_TmpCst(const STATEMENT&);
		
		//MDOP
		template <typename> void				Emit_Md_MemMem(const STATEMENT&);
		template <typename> void				Emit_Md_MemMemMem(const STATEMENT&);
		template <typename> void				Emit_Md_Shift_MemMemCst(const STATEMENT&);
		template <uint32> void					Emit_Md_Test_VarMem(const STATEMENT&);

		void									Emit_Md_Mov_MemMem(const STATEMENT&);
		void									Emit_Md_Not_MemMem(const STATEMENT&);
		void									Emit_Md_DivS_MemMemMem(const STATEMENT&);

		void									Emit_Md_Srl256_MemMemVar(const STATEMENT&);
		void									Emit_Md_Srl256_MemMemCst(const STATEMENT&);

		void									Emit_Md_LoadFromRef_MemMem(const STATEMENT&);
		void									Emit_Md_StoreAtRef_MemMem(const STATEMENT&);

		void									Emit_Md_MovMasked_MemMemMem(const STATEMENT&);
		void									Emit_Md_Expand_MemReg(const STATEMENT&);
		void									Emit_Md_Expand_MemMem(const STATEMENT&);
		void									Emit_Md_Expand_MemCst(const STATEMENT&);

		void									Emit_Md_PackHB_MemMemMem(const STATEMENT&);
		void									Emit_Md_PackWH_MemMemMem(const STATEMENT&);

		template <uint32> void					Emit_Md_UnpackBH_MemMemMem(const STATEMENT&);
		template <uint32> void					Emit_Md_UnpackHW_MemMemMem(const STATEMENT&);
		template <uint32> void					Emit_Md_UnpackWD_MemMemMem(const STATEMENT&);

		void									Emit_MergeTo256_MemMemMem(const STATEMENT&);

		static CONSTMATCHER						g_constMatchers[];
		static CONSTMATCHER						g_64ConstMatchers[];
		static CONSTMATCHER						g_fpuConstMatchers[];
		static CONSTMATCHER						g_mdConstMatchers[];
		static CAArch32Assembler::REGISTER		g_registers[MAX_REGISTERS];
		static CAArch32Assembler::REGISTER		g_paramRegs[MAX_PARAM_REGS];
		static CAArch32Assembler::REGISTER		g_baseRegister;
		static CAArch32Assembler::REGISTER		g_callAddressRegister;
		static CAArch32Assembler::REGISTER		g_tempParamRegister0;
		static CAArch32Assembler::REGISTER		g_tempParamRegister1;

		Framework::CStream*						m_stream = nullptr;
		CAArch32Assembler						m_assembler;
		LabelMapType							m_labels;
		ParamStack								m_params;
		uint32									m_stackLevel = 0;
		bool									m_hasIntegerDiv = false;
	};
};
