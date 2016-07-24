#pragma once

#include <deque>
#include "Jitter_CodeGen.h"
#include "AArch64Assembler.h"

namespace Jitter
{
	class CCodeGen_AArch64 : public CCodeGen
	{
	public:
		           CCodeGen_AArch64();
		virtual    ~CCodeGen_AArch64();

		void            SetGenerateRelocatableCalls(bool);

		void            GenerateCode(const StatementList&, unsigned int) override;
		void            SetStream(Framework::CStream*) override;
		void            RegisterExternalSymbols(CObjectFile*) const override;
		unsigned int    GetAvailableRegisterCount() const override;
		unsigned int    GetAvailableMdRegisterCount() const override;
		bool            CanHold128BitsReturnValueInRegisters() const override;

	private:
		typedef std::map<uint32, CAArch64Assembler::LABEL> LabelMapType;
		typedef void (CCodeGen_AArch64::*ConstCodeEmitterType)(const STATEMENT&);

		struct ADDSUB_IMM_PARAMS
		{
			uint16                                      imm = 0;
			CAArch64Assembler::ADDSUB_IMM_SHIFT_TYPE    shiftType = CAArch64Assembler::ADDSUB_IMM_SHIFT_LSL0;
		};
		
		struct LOGICAL_IMM_PARAMS
		{
			uint8 n;
			uint8 immr;
			uint8 imms;
		};

		struct PARAM_STATE
		{
			bool prepared = false;
			unsigned int index = 0;
			uint32 spillOffset = 0;
		};

		typedef std::function<void (PARAM_STATE&)> ParamEmitterFunction;
		typedef std::deque<ParamEmitterFunction> ParamStack;

		enum
		{
			MAX_REGISTERS = 9,
		};

		enum
		{
			MAX_MDREGISTERS = 28,
		};
		
		enum MAX_PARAM_REGS
		{
			MAX_PARAM_REGS = 8,
		};

		enum MAX_TEMP_REGS
		{
			MAX_TEMP_REGS = 7,
		};

		enum MAX_TEMP_MD_REGS
		{
			MAX_TEMP_MD_REGS = 4,
		};
		
		struct CONSTMATCHER
		{
			OPERATION               op;
			MATCHTYPE               dstType;
			MATCHTYPE               src1Type;
			MATCHTYPE               src2Type;
			ConstCodeEmitterType    emitter;
		};

		static uint32    GetMaxParamSpillSize(const StatementList&);

		CAArch64Assembler::REGISTER32    GetNextTempRegister();
		CAArch64Assembler::REGISTER64    GetNextTempRegister64();
		CAArch64Assembler::REGISTERMD    GetNextTempRegisterMd();
		
		uint32    GetMemory64Offset(CSymbol*) const;

		void    LoadMemoryInRegister(CAArch64Assembler::REGISTER32, CSymbol*);
		void    StoreRegisterInMemory(CSymbol*, CAArch64Assembler::REGISTER32);
		
		void    LoadMemory64InRegister(CAArch64Assembler::REGISTER64, CSymbol*);
		void    StoreRegisterInMemory64(CSymbol*, CAArch64Assembler::REGISTER64);
		
		void    LoadConstantInRegister(CAArch64Assembler::REGISTER32, uint32);
		void    LoadConstant64InRegister(CAArch64Assembler::REGISTER64, uint64);
		
		void    LoadMemory64LowInRegister(CAArch64Assembler::REGISTER32, CSymbol*);
		void    LoadMemory64HighInRegister(CAArch64Assembler::REGISTER32, CSymbol*);
		
		void    LoadSymbol64InRegister(CAArch64Assembler::REGISTER64, CSymbol*);

		void    StoreRegistersInMemory64(CSymbol*, CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32);
		
		void    LoadMemoryReferenceInRegister(CAArch64Assembler::REGISTER64, CSymbol*);
		void    StoreRegisterInTemporaryReference(CSymbol*, CAArch64Assembler::REGISTER64);
		
		void    LoadMemoryFpSingleInRegister(CAArch64Assembler::REGISTERMD, CSymbol*);
		void    StoreRegisterInMemoryFpSingle(CSymbol*, CAArch64Assembler::REGISTERMD);
		
		void    LoadMemory128InRegister(CAArch64Assembler::REGISTERMD, CSymbol*);
		void    StoreRegisterInMemory128(CSymbol*, CAArch64Assembler::REGISTERMD);
		
		void    LoadMemory128AddressInRegister(CAArch64Assembler::REGISTER64, CSymbol*, uint32 = 0);
		void    LoadRelative128AddressInRegister(CAArch64Assembler::REGISTER64, CSymbol*, uint32);
		void    LoadTemporary128AddressInRegister(CAArch64Assembler::REGISTER64, CSymbol*, uint32);
		
		void    LoadTemporary256ElementAddressInRegister(CAArch64Assembler::REGISTER64, CSymbol*, uint32);

		CAArch64Assembler::REGISTER32    PrepareSymbolRegisterDef(CSymbol*, CAArch64Assembler::REGISTER32);
		CAArch64Assembler::REGISTER32    PrepareSymbolRegisterUse(CSymbol*, CAArch64Assembler::REGISTER32);
		void                             CommitSymbolRegister(CSymbol*, CAArch64Assembler::REGISTER32);
		
		CAArch64Assembler::REGISTERMD    PrepareSymbolRegisterDefMd(CSymbol*, CAArch64Assembler::REGISTERMD);
		CAArch64Assembler::REGISTERMD    PrepareSymbolRegisterUseMd(CSymbol*, CAArch64Assembler::REGISTERMD);
		void                             CommitSymbolRegisterMd(CSymbol*, CAArch64Assembler::REGISTERMD);
		
		CAArch64Assembler::REGISTER32    PrepareParam(PARAM_STATE&);
		CAArch64Assembler::REGISTER64    PrepareParam64(PARAM_STATE&);
		void                             CommitParam(PARAM_STATE&);
		void                             CommitParam64(PARAM_STATE&);
		
		bool TryGetAddSubImmParams(uint32, ADDSUB_IMM_PARAMS&);
		bool TryGetAddSub64ImmParams(uint64, ADDSUB_IMM_PARAMS&);
		bool TryGetLogicalImmParams(uint32, LOGICAL_IMM_PARAMS&);
		
		//SHIFTOP ----------------------------------------------------------
		struct SHIFTOP_BASE
		{
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, uint8);
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32);
		};
		
		struct SHIFTOP_ASR : public SHIFTOP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Asr; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Asrv; }
		};
		
		struct SHIFTOP_LSL : public SHIFTOP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Lsl; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Lslv; }
		};

		struct SHIFTOP_LSR : public SHIFTOP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Lsr; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Lsrv; }
		};
		
		//LOGICOP ----------------------------------------------------------
		struct LOGICOP_BASE
		{
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32);
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, uint8, uint8, uint8);
		};
		
		struct LOGICOP_AND : public LOGICOP_BASE
		{
			static OpRegType    OpReg()    { return &CAArch64Assembler::And; }
			static OpImmType    OpImm()    { return &CAArch64Assembler::And; }
		};
		
		struct LOGICOP_OR : public LOGICOP_BASE
		{
			static OpRegType    OpReg()    { return &CAArch64Assembler::Orr; }
			static OpImmType    OpImm()    { return &CAArch64Assembler::Orr; }
		};

		struct LOGICOP_XOR : public LOGICOP_BASE
		{
			static OpRegType    OpReg()    { return &CAArch64Assembler::Eor; }
			static OpImmType    OpImm()    { return &CAArch64Assembler::Eor; }
		};
		
		//ADDSUBOP ----------------------------------------------------------
		struct ADDSUBOP_BASE
		{
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, uint16 imm, CAArch64Assembler::ADDSUB_IMM_SHIFT_TYPE);
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32, CAArch64Assembler::REGISTER32);
		};
		
		struct ADDSUBOP_ADD : public ADDSUBOP_BASE
		{
			static OpImmType    OpImm()       { return &CAArch64Assembler::Add; }
			static OpRegType    OpReg()       { return &CAArch64Assembler::Add; }
			static OpImmType    OpImmRev()    { return &CAArch64Assembler::Sub; }
		};

		struct ADDSUBOP_SUB : public ADDSUBOP_BASE
		{
			static OpImmType    OpImm()       { return &CAArch64Assembler::Sub; }
			static OpRegType    OpReg()       { return &CAArch64Assembler::Sub; }
			static OpImmType    OpImmRev()    { return &CAArch64Assembler::Add; }
		};

		//SHIFT64OP ----------------------------------------------------------
		struct SHIFT64OP_BASE
		{
			typedef void (CAArch64Assembler::*OpImmType)(CAArch64Assembler::REGISTER64, CAArch64Assembler::REGISTER64, uint8);
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTER64, CAArch64Assembler::REGISTER64, CAArch64Assembler::REGISTER64);
		};
		
		struct SHIFT64OP_ASR : public SHIFT64OP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Asr; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Asrv; }
		};
		
		struct SHIFT64OP_LSL : public SHIFT64OP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Lsl; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Lslv; }
		};

		struct SHIFT64OP_LSR : public SHIFT64OP_BASE
		{
			static OpImmType    OpImm()    { return &CAArch64Assembler::Lsr; }
			static OpRegType    OpReg()    { return &CAArch64Assembler::Lsrv; }
		};

		//FPUOP ----------------------------------------------------------
		struct FPUOP_BASE2
		{
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTERMD, CAArch64Assembler::REGISTERMD);
		};

		struct FPUOP_BASE3
		{
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTERMD, CAArch64Assembler::REGISTERMD, CAArch64Assembler::REGISTERMD);
		};
		
		struct FPUOP_ADD : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fadd_1s; }
		};
		
		struct FPUOP_SUB : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fsub_1s; }
		};

		struct FPUOP_MUL : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fmul_1s; }
		};

		struct FPUOP_DIV : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fdiv_1s; }
		};

		struct FPUOP_MIN : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fmin_1s; }
		};
		
		struct FPUOP_MAX : public FPUOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fmax_1s; }
		};

		struct FPUOP_ABS : public FPUOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fabs_1s; }
		};
		
		struct FPUOP_NEG : public FPUOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fneg_1s; }
		};
		
		struct FPUOP_SQRT : public FPUOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fsqrt_1s; }
		};
		
		//MDOP -----------------------------------------------------------
		struct MDOP_BASE2
		{
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTERMD, CAArch64Assembler::REGISTERMD);
		};

		struct MDOP_BASE3
		{
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTERMD, CAArch64Assembler::REGISTERMD, CAArch64Assembler::REGISTERMD);
		};

		struct MDOP_SHIFT
		{
			typedef void (CAArch64Assembler::*OpRegType)(CAArch64Assembler::REGISTERMD, CAArch64Assembler::REGISTERMD, uint8);
		};

		struct MDOP_ADDB : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Add_16b; }
		};
		
		struct MDOP_ADDH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Add_8h; }
		};
		
		struct MDOP_ADDW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Add_4s; }
		};

		struct MDOP_ADDBUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Uqadd_16b; }
		};
		
		struct MDOP_ADDHUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Uqadd_8h; }
		};

		struct MDOP_ADDWUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Uqadd_4s; }
		};
		
		struct MDOP_ADDHSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sqadd_8h; }
		};

		struct MDOP_ADDWSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sqadd_4s; }
		};
		
		struct MDOP_SUBB : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sub_16b; }
		};
		
		struct MDOP_SUBH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sub_8h; }
		};

		struct MDOP_SUBW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sub_4s; }
		};
		
		struct MDOP_SUBBUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Uqsub_16b; }
		};
		
		struct MDOP_SUBHUS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Uqsub_8h; }
		};
		
		struct MDOP_SUBHSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sqsub_8h; }
		};

		struct MDOP_SUBWSS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sqsub_4s; }
		};

		struct MDOP_CMPEQW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Cmeq_4s; }
		};
		
		struct MDOP_CMPGTH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Cmgt_4s; }
		};
		
		struct MDOP_MINH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Smin_8h; }
		};

		struct MDOP_MINW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Smin_4s; }
		};

		struct MDOP_MAXH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Smax_8h; }
		};

		struct MDOP_MAXW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Smax_4s; }
		};
		
		struct MDOP_ADDS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fadd_4s; }
		};
		
		struct MDOP_SUBS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fsub_4s; }
		};

		struct MDOP_MULS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fmul_4s; }
		};

		struct MDOP_DIVS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fdiv_4s; }
		};

		struct MDOP_ABSS : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fabs_4s; }
		};

		struct MDOP_MINS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fmin_4s; }
		};

		struct MDOP_MAXS : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fmax_4s; }
		};

		struct MDOP_TOSINGLE : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Scvtf_4s; }
		};
		
		struct MDOP_TOWORD : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fcvtzs_4s; }
		};
		
		struct MDOP_AND : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::And_16b; }
		};
		
		struct MDOP_OR : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Orr_16b; }
		};
		
		struct MDOP_XOR : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Eor_16b; }
		};
		
		struct MDOP_UNPACK_LOWER_BH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Zip1_16b; }
		};

		struct MDOP_UNPACK_LOWER_HW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Zip1_8h; }
		};

		struct MDOP_UNPACK_LOWER_WD : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Zip1_4s; }
		};
		
		struct MDOP_UNPACK_UPPER_BH : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Zip2_16b; }
		};
		
		struct MDOP_UNPACK_UPPER_HW : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Zip2_8h; }
		};

		struct MDOP_UNPACK_UPPER_WD : public MDOP_BASE3
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Zip2_4s; }
		};

		struct MDOP_CMPEQZS : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fcmeq_4s; }
		};
		
		struct MDOP_CMPLTZS : public MDOP_BASE2
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Fcmlt_4s; }
		};
		
		struct MDOP_SLLH : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Shl_8h; }
		};
		
		struct MDOP_SLLW : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Shl_4s; }
		};

		struct MDOP_SRLH : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Ushr_8h; }
		};
		
		struct MDOP_SRLW : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Ushr_4s; }
		};
		
		struct MDOP_SRAH : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sshr_8h; }
		};

		struct MDOP_SRAW : public MDOP_SHIFT
		{
			static OpRegType OpReg() { return &CAArch64Assembler::Sshr_4s; }
		};
		
		uint16    GetSavedRegisterList(uint32);
		void      Emit_Prolog(const StatementList&, uint32, uint16);
		void      Emit_Epilog(uint32, uint16);
		
		CAArch64Assembler::LABEL GetLabel(uint32);
		void                     MarkLabel(const STATEMENT&);
		
		void    Emit_Nop(const STATEMENT&);
		
		void    Emit_Mov_RegReg(const STATEMENT&);
		void    Emit_Mov_RegMem(const STATEMENT&);
		void    Emit_Mov_RegCst(const STATEMENT&);
		void    Emit_Mov_MemReg(const STATEMENT&);
		void    Emit_Mov_MemMem(const STATEMENT&);
		void    Emit_Mov_MemCst(const STATEMENT&);
		
		void    Emit_Not_VarVar(const STATEMENT&);
		void    Emit_Lzc_VarVar(const STATEMENT&);
		
		void    Emit_Mov_Mem64Mem64(const STATEMENT&);
		void    Emit_Mov_Mem64Cst64(const STATEMENT&);
		
		void    Emit_ExtLow64VarMem64(const STATEMENT&);
		void    Emit_ExtHigh64VarMem64(const STATEMENT&);
		void    Emit_MergeTo64_Mem64AnyAny(const STATEMENT&);
		
		void    Emit_RelToRef_TmpCst(const STATEMENT&);
		void    Emit_AddRef_TmpMemAny(const STATEMENT&);
		void    Emit_LoadFromRef_VarMem(const STATEMENT&);
		void    Emit_StoreAtRef_MemAny(const STATEMENT&);
		
		void    Emit_Param_Ctx(const STATEMENT&);
		void    Emit_Param_Reg(const STATEMENT&);
		void    Emit_Param_Mem(const STATEMENT&);
		void    Emit_Param_Cst(const STATEMENT&);
		void    Emit_Param_Mem64(const STATEMENT&);
		void    Emit_Param_Cst64(const STATEMENT&);
		void    Emit_Param_Reg128(const STATEMENT&);
		void    Emit_Param_Mem128(const STATEMENT&);
		
		void    Emit_Call(const STATEMENT&);
		void    Emit_RetVal_Reg(const STATEMENT&);
		void    Emit_RetVal_Tmp(const STATEMENT&);
		void    Emit_RetVal_Mem64(const STATEMENT&);
		void    Emit_RetVal_Reg128(const STATEMENT&);
		void    Emit_RetVal_Mem128(const STATEMENT&);
		
		void    Emit_Jmp(const STATEMENT&);
		
		void    Emit_CondJmp(const STATEMENT&);
		void    Emit_CondJmp_AnyVar(const STATEMENT&);
		void    Emit_CondJmp_VarCst(const STATEMENT&);
		
		void    Cmp_GetFlag(CAArch64Assembler::REGISTER32, Jitter::CONDITION);
		void    Emit_Cmp_VarAnyVar(const STATEMENT&);
		void    Emit_Cmp_VarVarCst(const STATEMENT&);
		
		void    Emit_Add64_MemMemMem(const STATEMENT&);
		void    Emit_Add64_MemMemCst(const STATEMENT&);
		
		void    Emit_Sub64_MemAnyMem(const STATEMENT&);
		void    Emit_Sub64_MemMemCst(const STATEMENT&);
		
		void    Emit_Cmp64_VarAnyMem(const STATEMENT&);
		void    Emit_Cmp64_VarMemCst(const STATEMENT&);
		
		void    Emit_And64_MemMemMem(const STATEMENT&);
		
		//ADDSUB
		template <typename> void    Emit_AddSub_VarAnyVar(const STATEMENT&);
		template <typename> void    Emit_AddSub_VarVarCst(const STATEMENT&);
		
		//SHIFT
		template <typename> void    Emit_Shift_VarAnyVar(const STATEMENT&);
		template <typename> void    Emit_Shift_VarVarCst(const STATEMENT&);

		//LOGIC
		template <typename> void    Emit_Logic_VarAnyVar(const STATEMENT&);
		template <typename> void    Emit_Logic_VarVarCst(const STATEMENT&);

		//MUL
		template <bool> void Emit_Mul_Tmp64AnyAny(const STATEMENT&);
		
		//DIV
		template <bool> void Emit_Div_Tmp64AnyAny(const STATEMENT&);
		
		//SHIFT64
		template <typename> void    Emit_Shift64_MemMemVar(const STATEMENT&);
		template <typename> void    Emit_Shift64_MemMemCst(const STATEMENT&);
		
		//FPU
		template <typename> void    Emit_Fpu_MemMem(const STATEMENT&);
		template <typename> void    Emit_Fpu_MemMemMem(const STATEMENT&);

		void    Emit_Fp_Cmp_AnyMemMem(const STATEMENT&);
		void    Emit_Fp_Rcpl_MemMem(const STATEMENT&);
		void    Emit_Fp_Rsqrt_MemMem(const STATEMENT&);
		void    Emit_Fp_Mov_MemSRelI32(const STATEMENT&);
		void    Emit_Fp_ToIntTrunc_MemMem(const STATEMENT&);
		void    Emit_Fp_LdCst_TmpCst(const STATEMENT&);

		//MD
		template <typename> void    Emit_Md_VarVar(const STATEMENT&);
		template <typename> void    Emit_Md_VarVarVar(const STATEMENT&);
		template <typename> void    Emit_Md_VarVarVarRev(const STATEMENT&);
		template <typename> void    Emit_Md_Shift_VarVarCst(const STATEMENT&);
		template <typename> void    Emit_Md_Test_VarVar(const STATEMENT&);

		void    Emit_Md_Mov_RegReg(const STATEMENT&);
		void    Emit_Md_Mov_RegMem(const STATEMENT&);
		void    Emit_Md_Mov_MemReg(const STATEMENT&);
		void    Emit_Md_Mov_MemMem(const STATEMENT&);
		
		void    Emit_Md_Not_VarVar(const STATEMENT&);
		
		void    Emit_Md_LoadFromRef_VarMem(const STATEMENT&);
		void    Emit_Md_StoreAtRef_MemVar(const STATEMENT&);
	
		void    Emit_Md_MovMasked_VarVarVar(const STATEMENT&);
		void    Emit_Md_Expand_VarReg(const STATEMENT&);
		void    Emit_Md_Expand_VarMem(const STATEMENT&);
		void    Emit_Md_Expand_VarCst(const STATEMENT&);
		
		void    Emit_Md_PackHB_VarVarVar(const STATEMENT&);
		void    Emit_Md_PackWH_VarVarVar(const STATEMENT&);
		
		void    Emit_MergeTo256_MemVarVar(const STATEMENT&);
		void    Emit_Md_Srl256_VarMemVar(const STATEMENT&);
		void    Emit_Md_Srl256_VarMemCst(const STATEMENT&);
		
		static CONSTMATCHER    g_constMatchers[];
		static CONSTMATCHER    g_64ConstMatchers[];
		static CONSTMATCHER    g_fpuConstMatchers[];
		static CONSTMATCHER    g_mdConstMatchers[];
		
		static CAArch64Assembler::REGISTER32    g_registers[MAX_REGISTERS];
		static CAArch64Assembler::REGISTERMD    g_registersMd[MAX_MDREGISTERS];
		static CAArch64Assembler::REGISTER32    g_tempRegisters[MAX_TEMP_REGS];
		static CAArch64Assembler::REGISTER64    g_tempRegisters64[MAX_TEMP_REGS];
		static CAArch64Assembler::REGISTERMD    g_tempRegistersMd[MAX_TEMP_MD_REGS];
		static CAArch64Assembler::REGISTER32    g_paramRegisters[MAX_PARAM_REGS];
		static CAArch64Assembler::REGISTER64    g_paramRegisters64[MAX_PARAM_REGS];
		static CAArch64Assembler::REGISTER64    g_baseRegister;

		Framework::CStream*    m_stream = nullptr;
		CAArch64Assembler      m_assembler;
		LabelMapType           m_labels;
		ParamStack             m_params;
		uint32                 m_nextTempRegister = 0;
		uint32                 m_nextTempRegisterMd = 0;
		uint32                 m_paramSpillBase = 0;

		bool    m_generateRelocatableCalls = false;
	};
};
