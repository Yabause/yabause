#pragma once

#include <string>
#include <memory>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include <stack>
#include "ArrayStack.h"
#include "Stream.h"
#include "Jitter_SymbolTable.h"
#include "Jitter_CodeGen.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

#include "Jitter_Statement.h"

namespace Jitter
{

	class CJitter
	{
	public:
		enum ROUNDMODE
		{
			ROUND_NEAREST = 0,
			ROUND_PLUSINFINITY = 1,
			ROUND_MINUSINFINITY = 2,
			ROUND_TRUNCATE = 3
		};

		enum RETURN_VALUE_TYPE
		{
			RETURN_VALUE_NONE,
			RETURN_VALUE_32,
			RETURN_VALUE_64,
			RETURN_VALUE_128,
		};

		typedef unsigned int LABEL;

										CJitter(CCodeGen*);
		virtual							~CJitter();

		virtual void					Begin();
		virtual void					End();
		
		bool							IsStackEmpty() const;

		void							BeginIf(CONDITION);
		void							Else();
		void							EndIf();
		
		LABEL							CreateLabel();
		void							MarkLabel(LABEL);
		void							Goto(LABEL);

		void							PushCtx();
		void							PushCst(uint32);
		virtual void					PushRel(size_t);
		void							PushTop();
		void							PushIdx(unsigned int);

		virtual void					PullRel(size_t);
		void							PullTop();
		void							Swap();

		void							Add();
		void							And();
		void							Call(void*, unsigned int, bool);
		void							Call(void*, unsigned int, RETURN_VALUE_TYPE);
		void							Cmp(CONDITION);
		void							Div();
		void							DivS();
		void							Lookup(uint32*);
		void							Lzc();
		void							Mult();
		void							MultS();
		void							Not();
		void							Or();
		void							SignExt();
		void							SignExt8();
		void							SignExt16();
		void							Shl();
		void							Shl(uint8);
		void							Sra();
		void							Sra(uint8);
		void							Srl();
		void							Srl(uint8);
		void							Sub();
		void							Xor();

		//Memory operations
		void							PushRelRef(size_t);
		void							PushRelAddrRef(size_t);

		void							AddRef();
		void							LoadFromRef();
		void							StoreAtRef();

		//64-bits
		virtual void					PushRel64(size_t);
		void							PushCst64(uint64);

		void							PullRel64(size_t);

		void							MergeTo64();
		void							ExtLow64();
		void							ExtHigh64();

		void							Add64();
		void							Sub64();
		void							And64();
		void							Cmp64(CONDITION);
		void							Srl64();
		void							Srl64(uint8);
		void							Sra64();
		void							Sra64(uint8);
		void							Shl64();
		void							Shl64(uint8);

		//FPU
		virtual void					FP_PushWord(size_t);
		virtual void					FP_PushSingle(size_t);
		virtual void					FP_PullWordTruncate(size_t);
		virtual void					FP_PullSingle(size_t);
		virtual void					FP_PushCst(float);

		void							FP_Add();
		void							FP_Abs();
		void							FP_Sub();
		void							FP_Max();
		void							FP_Min();
		void							FP_Mul();
		void							FP_Div();
		void							FP_Cmp(CONDITION);
		void							FP_Neg();
		void							FP_Rcpl();
		void							FP_Sqrt();
		void							FP_Rsqrt();

		//SIMD (128-bits only)
		virtual void					MD_PushRel(size_t);
		virtual void					MD_PushRelExpand(size_t);
		void							MD_PushCstExpand(uint32);
		void							MD_PushCstExpand(float);
		virtual void					MD_PullRel(size_t);
		virtual void					MD_PullRel(size_t, bool, bool, bool, bool);

		void							MD_LoadFromRef();
		void							MD_StoreAtRef();

		void							MD_AbsS();
		void							MD_AddB();
		void							MD_AddBUS();
		void							MD_AddH();
		void							MD_AddHSS();
		void							MD_AddHUS();
		void							MD_AddW();
		void							MD_AddWSS();
		void							MD_AddWUS();
		void							MD_AddS();
		void							MD_And();
		void							MD_CmpEqB();
		void							MD_CmpEqH();
		void							MD_CmpEqW();
		void							MD_CmpGtB();
		void							MD_CmpGtH();
		void							MD_CmpGtW();
		void							MD_DivS();
		void							MD_IsNegative();
		void							MD_IsZero();
		void							MD_MaxH();
		void							MD_MaxW();
		void							MD_MaxS();
		void							MD_MinH();
		void							MD_MinW();
		void							MD_MinS();
		void							MD_MulS();
		void							MD_Not();
		void							MD_Or();
		void							MD_PackHB();
		void							MD_PackWH();
		void							MD_SllH(uint8);
		void							MD_SllW(uint8);
		void							MD_SraH(uint8);
		void							MD_SraW(uint8);
		void							MD_SrlH(uint8);
		void							MD_SrlW(uint8);
		void							MD_Srl256();
		void							MD_SubB();
		void							MD_SubBUS();
		void							MD_SubHSS();
		void							MD_SubHUS();
		void							MD_SubH();
		void							MD_SubW();
		void							MD_SubWSS();
		void							MD_SubS();
		void							MD_ToSingle();
		void							MD_ToWordTruncate();
		void							MD_UnpackLowerBH();
		void							MD_UnpackLowerHW();
		void							MD_UnpackLowerWD();
		void							MD_UnpackUpperBH();
		void							MD_UnpackUpperHW();
		void							MD_UnpackUpperWD();
		void							MD_Xor();

		CCodeGen*						GetCodeGen();

		void							SetStream(Framework::CStream*);

	private:
		struct SYMBOL_REGALLOCINFO
		{
			unsigned int			useCount = 0;
			unsigned int			firstUse = -1;
			unsigned int			firstDef = -1;
			unsigned int			lastDef = -1;
			unsigned int			rangeBegin = -1;
			unsigned int			rangeEnd = -1;
			bool					aliased = false;
			SYM_TYPE				registerType = SYM_REGISTER;
			unsigned int			registerId = -1;
		};

		typedef size_t LABELREF;
		typedef std::map<LABEL, unsigned int> LabelMapType;
		typedef std::pair<unsigned int, unsigned int> AllocationRange;
		typedef std::vector<AllocationRange> AllocationRangeArray;
		typedef std::unordered_map<SymbolPtr, SYMBOL_REGALLOCINFO, SymbolHasher, SymbolComparator> SymbolRegAllocInfo;
		typedef std::unordered_map<CSymbol*, unsigned int> SymbolUseCountMap;
		typedef std::stack<uint32> IntStack;

		class CRelativeVersionManager
		{
		public:

			unsigned int				GetRelativeVersion(uint32);
			unsigned int				IncrementRelativeVersion(uint32);

		private:
			typedef std::unordered_map<uint32, unsigned int> RelativeVersionMap;

			RelativeVersionMap			m_relativeVersions;
		};

		struct BASIC_BLOCK
		{
			uint32						id = 0;
			StatementList				statements;
			CSymbolTable				symbolTable;
			bool						optimized = false;
			bool						hasJumpRef = false;
		};
		typedef std::list<BASIC_BLOCK> BasicBlockList;

		struct VERSIONED_STATEMENT_LIST
		{
			StatementList				statements;
			CRelativeVersionManager		relativeVersions;
		};

		void							InsertBinaryStatement(Jitter::OPERATION);
		void							InsertUnaryMdStatement(Jitter::OPERATION);
		void							InsertBinaryMdStatement(Jitter::OPERATION);

		void							Compile();

		bool							ConstantFolding(StatementList&);
		bool							ConstantPropagation(StatementList&);
		bool							CopyPropagation(StatementList&);
		bool							DeadcodeElimination(VERSIONED_STATEMENT_LIST&);

		void							FixFlowControl(StatementList&);

		bool							FoldConstantOperation(STATEMENT&);
		bool							FoldConstant64Operation(STATEMENT&);
		bool							FoldConstant6432Operation(STATEMENT&);
		bool							FoldConstant12832Operation(STATEMENT&);

		BASIC_BLOCK						ConcatBlocks(const BasicBlockList&);
		bool							MergeBlocks();
		bool							PruneBlocks();
		void							HarmonizeBlocks();
		void							MergeBasicBlocks(BASIC_BLOCK&, const BASIC_BLOCK&);

		void							StartBlock(uint32);

		void							InsertStatement(const STATEMENT&);

		SymbolPtr						MakeSymbol(SYM_TYPE, uint32);
		SymbolPtr						MakeSymbol(BASIC_BLOCK*, SYM_TYPE, uint32, uint32);
		SymbolPtr						MakeConstantPtr(uintptr_t);
		SymbolPtr						MakeConstant64(uint64);

		SymbolRefPtr					MakeSymbolRef(const SymbolPtr&);
		int								GetSymbolSize(const SymbolRefPtr&);

		static CONDITION				GetReverseCondition(CONDITION);

		VERSIONED_STATEMENT_LIST		GenerateVersionedStatementList(const StatementList&);
		StatementList					CollapseVersionedStatementList(const VERSIONED_STATEMENT_LIST&);
		void							CoalesceTemporaries(BASIC_BLOCK&);
		void							RemoveSelfAssignments(BASIC_BLOCK&);
		void							PruneSymbols(BASIC_BLOCK&) const;

		void							AllocateRegisters(BASIC_BLOCK&);
		static AllocationRangeArray		ComputeAllocationRanges(const BASIC_BLOCK&);
		void							ComputeLivenessForRange(const BASIC_BLOCK&, const AllocationRange&, SymbolRegAllocInfo&) const;
		void							MarkAliasedSymbols(const BASIC_BLOCK&, const AllocationRange&, SymbolRegAllocInfo&) const;
		void							AssociateSymbolsToRegisters(SymbolRegAllocInfo&) const;

		void							NormalizeStatements(BASIC_BLOCK&);
		unsigned int					AllocateStack(BASIC_BLOCK&);

		bool							m_blockStarted = false;

		CArrayStack<SymbolPtr>			m_shadow;
		IntStack						m_ifStack;

		unsigned int					m_nextTemporary = 1;
		unsigned int					m_nextBlockId = 1;

		BASIC_BLOCK*					m_currentBlock = nullptr;
		BasicBlockList					m_basicBlocks;
		CCodeGen*						m_codeGen = nullptr;

		unsigned int					m_nextLabelId = 1;
		LabelMapType					m_labels;
	};

}
