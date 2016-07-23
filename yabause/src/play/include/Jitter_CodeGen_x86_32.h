#pragma once

#include "Jitter_CodeGen_x86.h"
#include <deque>

namespace Jitter
{
	class CCodeGen_x86_32 : public CCodeGen_x86
	{
	public:
											CCodeGen_x86_32();
		virtual								~CCodeGen_x86_32();

		void								SetImplicitRetValueParamFixUpRequired(bool);
		
		unsigned int						GetAvailableRegisterCount() const override;
		unsigned int						GetAvailableMdRegisterCount() const override;
		bool								CanHold128BitsReturnValueInRegisters() const override;
		
	protected:
		enum SHIFTRIGHT_TYPE
		{
			SHIFTRIGHT_LOGICAL,
			SHIFTRIGHT_ARITHMETIC
		};

		virtual void						Emit_Prolog(const StatementList&, unsigned int, uint32) override;
		virtual void						Emit_Epilog(unsigned int, uint32) override;

		//PARAM
		void								Emit_Param_Ctx(const STATEMENT&);
		void								Emit_Param_Reg(const STATEMENT&);
		void								Emit_Param_Mem(const STATEMENT&);
		void								Emit_Param_Cst(const STATEMENT&);
		void								Emit_Param_Mem64(const STATEMENT&);
		void								Emit_Param_Cst64(const STATEMENT&);
		void								Emit_Param_Reg128(const STATEMENT&);
		void								Emit_Param_Mem128(const STATEMENT&);
		
		//PARAM_RET
		void								Emit_ParamRet_Mem128(const STATEMENT&);

		//CALL
		void								Emit_Call(const STATEMENT&);

		//RETURNVALUE
		void								Emit_RetVal_Tmp(const STATEMENT&);
		void								Emit_RetVal_Reg(const STATEMENT&);
		void								Emit_RetVal_Mem64(const STATEMENT&);

		//MOV
		void								Emit_Mov_Mem64Mem64(const STATEMENT&);
		void								Emit_Mov_Mem64Cst64(const STATEMENT&);

		//ADD64
		void								Emit_Add64_MemMemMem(const STATEMENT&);
		void								Emit_Add64_MemMemCst(const STATEMENT&);

		//SUB64
		void								Emit_Sub64_MemMemMem(const STATEMENT&);
		void								Emit_Sub64_MemMemCst(const STATEMENT&);
		void								Emit_Sub64_MemCstMem(const STATEMENT&);

		//AND64
		void								Emit_And64_MemMemMem(const STATEMENT&);

		//SR64
		void								Emit_Sr64Var_MemMem(CSymbol*, CSymbol*, CX86Assembler::REGISTER, SHIFTRIGHT_TYPE);
		void								Emit_Sr64Cst_MemMem(CSymbol*, CSymbol*, uint32, SHIFTRIGHT_TYPE);

		//SRL64
		void								Emit_Srl64_MemMemReg(const STATEMENT&);
		void								Emit_Srl64_MemMemMem(const STATEMENT&);
		void								Emit_Srl64_MemMemCst(const STATEMENT&);

		//SRA64
		void								Emit_Sra64_MemMemReg(const STATEMENT&);
		void								Emit_Sra64_MemMemMem(const STATEMENT&);
		void								Emit_Sra64_MemMemCst(const STATEMENT&);

		//SLL64
		void								Emit_Sll64_MemMemVar(const STATEMENT&, CX86Assembler::REGISTER);
		void								Emit_Sll64_MemMemReg(const STATEMENT&);
		void								Emit_Sll64_MemMemMem(const STATEMENT&);
		void								Emit_Sll64_MemMemCst(const STATEMENT&);

		//CMP64
		void								Cmp64_Equal(const STATEMENT&);
		template <typename> void			Cmp64_Order(const STATEMENT&);
		void								Cmp64_GenericRel(const STATEMENT&);
		void								Emit_Cmp64_RegRelRel(const STATEMENT&);
		void								Emit_Cmp64_RelRelRel(const STATEMENT&);
		void								Emit_Cmp64_RegRelCst(const STATEMENT&);
		void								Emit_Cmp64_RelRelCst(const STATEMENT&);
		void								Emit_Cmp64_TmpRelRoc(const STATEMENT&);

		//RELTOREF
		void								Emit_RelToRef_TmpCst(const STATEMENT&);

		//ADDREF
		void								Emit_AddRef_MemMemReg(const STATEMENT&);
		void								Emit_AddRef_MemMemMem(const STATEMENT&);
		void								Emit_AddRef_MemMemCst(const STATEMENT&);

		//LOADFROMREF
		void								Emit_LoadFromRef_RegTmp(const STATEMENT&);
		void								Emit_LoadFromRef_MemTmp(const STATEMENT&);
		void								Emit_LoadFromRef_Md_RegMem(const STATEMENT&);
		void								Emit_LoadFromRef_Md_MemMem(const STATEMENT&);

		//STOREATREF
		void								Emit_StoreAtRef_TmpReg(const STATEMENT&);
		void								Emit_StoreAtRef_TmpMem(const STATEMENT&);
		void								Emit_StoreAtRef_TmpCst(const STATEMENT&);
		void								Emit_StoreAtRef_Md_MemReg(const STATEMENT&);
		void								Emit_StoreAtRef_Md_MemMem(const STATEMENT&);

	private:
		struct CALL_STATE
		{
			uint32	paramOffset = 0;
			uint32	paramSpillOffset = 0;
		};

		typedef std::function<void (CALL_STATE&)> ParamEmitterFunction;
		typedef std::deque<ParamEmitterFunction> ParamStack;
		
		typedef void (CCodeGen_x86_32::*ConstCodeEmitterType)(const STATEMENT&);

		struct CONSTMATCHER
		{
			OPERATION				op;
			MATCHTYPE				dstType;
			MATCHTYPE				src1Type;
			MATCHTYPE				src2Type;
			ConstCodeEmitterType	emitter;
		};

		enum MAX_REGISTERS
		{
			MAX_REGISTERS = 3,
			MAX_MDREGISTERS = 4,
		};

		static CONSTMATCHER					g_constMatchers[];
		static CX86Assembler::REGISTER		g_registers[MAX_REGISTERS];
		static CX86Assembler::XMMREGISTER	g_mdRegisters[MAX_MDREGISTERS];
		
		ParamStack							m_params;
		uint32								m_paramSpillBase = 0;
		uint32								m_totalStackAlloc = 0;
		bool								m_hasImplicitRetValueParam = false;
		bool								m_implicitRetValueParamFixUpRequired = false;

	};
}
