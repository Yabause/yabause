#pragma once

#include <deque>
#include "Jitter_CodeGen_x86.h"

namespace Jitter
{
	class CCodeGen_x86_64 : public CCodeGen_x86
	{
	public:
		enum PLATFORM_ABI
		{
			PLATFORM_ABI_SYSTEMV,
			PLATFORM_ABI_WIN32
		};
		
											CCodeGen_x86_64();
		virtual								~CCodeGen_x86_64();

		void								SetPlatformAbi(PLATFORM_ABI);
		
		unsigned int						GetAvailableRegisterCount() const override;
		unsigned int						GetAvailableMdRegisterCount() const override;
		bool								CanHold128BitsReturnValueInRegisters() const override;

	protected:
		//ALUOP64 ----------------------------------------------------------
		struct ALUOP64_BASE
		{
			typedef void (CX86Assembler::*OpIqType)(const CX86Assembler::CAddress&, uint64);
			typedef void (CX86Assembler::*OpEqType)(CX86Assembler::REGISTER, const CX86Assembler::CAddress&);
		};

		struct ALUOP64_ADD : public ALUOP64_BASE
		{
			static OpIqType OpIq() { return &CX86Assembler::AddIq; }
			static OpEqType OpEq() { return &CX86Assembler::AddEq; }
		};

		struct ALUOP64_SUB : public ALUOP64_BASE
		{
			static OpIqType OpIq() { return &CX86Assembler::SubIq; }
			static OpEqType OpEq() { return &CX86Assembler::SubEq; }
		};

		struct ALUOP64_AND : public ALUOP64_BASE
		{
			static OpIqType OpIq() { return &CX86Assembler::AndIq; }
			static OpEqType OpEq() { return &CX86Assembler::AndEq; }
		};

		//SHIFTOP64 ----------------------------------------------------------
		struct SHIFTOP64_BASE
		{
			typedef void (CX86Assembler::*OpCstType)(const CX86Assembler::CAddress&, uint8);
			typedef void (CX86Assembler::*OpVarType)(const CX86Assembler::CAddress&);
		};

		struct SHIFTOP64_SLL : public SHIFTOP64_BASE
		{
			static OpCstType OpCst() { return &CX86Assembler::ShlEq; }
			static OpVarType OpVar() { return &CX86Assembler::ShlEq; }
		};

		struct SHIFTOP64_SRL : public SHIFTOP64_BASE
		{
			static OpCstType OpCst() { return &CX86Assembler::ShrEq; }
			static OpVarType OpVar() { return &CX86Assembler::ShrEq; }
		};

		struct SHIFTOP64_SRA : public SHIFTOP64_BASE
		{
			static OpCstType OpCst() { return &CX86Assembler::SarEq; }
			static OpVarType OpVar() { return &CX86Assembler::SarEq; }
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

		//CALL
		void								Emit_Call(const STATEMENT&);

		//RETURNVALUE
		void								Emit_RetVal_Reg(const STATEMENT&);
		void								Emit_RetVal_Mem(const STATEMENT&);
		void								Emit_RetVal_Mem64(const STATEMENT&);
		void								Emit_RetVal_Reg128(const STATEMENT&);
		void								Emit_RetVal_Mem128(const STATEMENT&);

		//MOV
		void								Emit_Mov_Mem64Mem64(const STATEMENT&);
		void								Emit_Mov_Rel64Cst64(const STATEMENT&);

		//ALU64
		template <typename> void			Emit_Alu64_MemMemMem(const STATEMENT&);
		template <typename> void			Emit_Alu64_MemMemCst(const STATEMENT&);
		template <typename> void			Emit_Alu64_MemCstMem(const STATEMENT&);

		//SHIFT64
		template <typename> void			Emit_Shift64_RelRelReg(const STATEMENT&);
		template <typename> void			Emit_Shift64_RelRelMem(const STATEMENT&);
		template <typename> void			Emit_Shift64_RelRelCst(const STATEMENT&);

		//CMP64
		void								Cmp64_RelRel(CX86Assembler::REGISTER, const STATEMENT&);
		void								Cmp64_RelCst(CX86Assembler::REGISTER, const STATEMENT&);

		void								Emit_Cmp64_RegRelRel(const STATEMENT&);
		void								Emit_Cmp64_RegRelCst(const STATEMENT&);

		void								Emit_Cmp64_MemRelRel(const STATEMENT&);
		void								Emit_Cmp64_MemRelCst(const STATEMENT&);

		//RELTOREF
		void								Emit_RelToRef_TmpCst(const STATEMENT&);

		//ADDREF
		void								Emit_AddRef_MemMemReg(const STATEMENT&);
		void								Emit_AddRef_MemMemMem(const STATEMENT&);
		void								Emit_AddRef_MemMemCst(const STATEMENT&);

		//LOADFROMREF
		void								Emit_LoadFromRef_RegMem(const STATEMENT&);
		void								Emit_LoadFromRef_MemMem(const STATEMENT&);
		void								Emit_LoadFromRef_Md_RegMem(const STATEMENT&);
		void								Emit_LoadFromRef_Md_MemMem(const STATEMENT&);

		//STOREATREF
		void								Emit_StoreAtRef_MemReg(const STATEMENT&);
		void								Emit_StoreAtRef_MemMem(const STATEMENT&);
		void								Emit_StoreAtRef_MemCst(const STATEMENT&);
		void								Emit_StoreAtRef_Md_MemReg(const STATEMENT&);
		void								Emit_StoreAtRef_Md_MemMem(const STATEMENT&);

	private:
		typedef void (CCodeGen_x86_64::*ConstCodeEmitterType)(const STATEMENT&);

		typedef std::function<uint32 (CX86Assembler::REGISTER, uint32)> ParamEmitterFunction;
		typedef std::deque<ParamEmitterFunction> ParamStack;

		struct CONSTMATCHER
		{
			OPERATION				op;
			MATCHTYPE				dstType;
			MATCHTYPE				src1Type;
			MATCHTYPE				src2Type;
			ConstCodeEmitterType	emitter;
		};

		enum SYSTEMV_MAX_REGISTERS
		{
			SYSTEMV_MAX_REGISTERS = 5,
		};
		
		enum SYSTEMV_MAX_PARAMS
		{
			SYSTEMV_MAX_PARAMS = 6,
		};
		
		enum WIN32_MAX_REGISTERS
		{
			WIN32_MAX_REGISTERS = 7,
		};
		
		enum WIN32_MAX_PARAMS
		{
			WIN32_MAX_PARAMS = 4,
		};

		enum MAX_MDREGISTERS
		{
			MAX_MDREGISTERS = 12,
		};
		
		static CONSTMATCHER					g_constMatchers[];
		static CX86Assembler::REGISTER		g_systemVRegisters[SYSTEMV_MAX_REGISTERS];
		static CX86Assembler::REGISTER		g_systemVParamRegs[SYSTEMV_MAX_PARAMS];
		static CX86Assembler::REGISTER		g_win32Registers[WIN32_MAX_REGISTERS];
		static CX86Assembler::REGISTER		g_win32ParamRegs[WIN32_MAX_PARAMS];
		static CX86Assembler::XMMREGISTER	g_mdRegisters[MAX_MDREGISTERS];
		
		PLATFORM_ABI						m_platformAbi = PLATFORM_ABI_SYSTEMV;
		uint32								m_maxRegisters = 0;
		uint32								m_maxParams = 0;
		bool								m_hasMdRegRetValues = false;
		CX86Assembler::REGISTER*			m_paramRegs = nullptr;
		
		ParamStack							m_params;
		uint32								m_paramSpillBase = 0;
		uint32								m_totalStackAlloc = 0;
	};
}
