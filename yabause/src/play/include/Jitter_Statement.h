#pragma once

#include <list>
#include <functional>
#include "Jitter_SymbolRef.h"

namespace Jitter
{
	enum OPERATION
	{
		OP_NOP = 0,
		OP_MOV,
		
		OP_ADD,
		OP_SUB,
		OP_CMP,

		OP_AND,
		OP_OR,
		OP_XOR,
		OP_NOT,

		OP_SRA,
		OP_SRL,
		OP_SLL,

		OP_MUL,
		OP_MULS,
		OP_DIV,
		OP_DIVS,

		OP_LZC,

		OP_RELTOREF,
		OP_ADDREF,
		OP_LOADFROMREF,
		OP_STOREATREF,

		OP_ADD64,
		OP_SUB64,
		OP_AND64,
		OP_CMP64,
		OP_MERGETO64,
		OP_EXTLOW64,
		OP_EXTHIGH64,
		OP_SRA64,
		OP_SRL64,
		OP_SLL64,

		OP_MERGETO256,

		OP_MD_MOV_MASKED,

		OP_MD_ADD_B,
		OP_MD_ADD_H,
		OP_MD_ADD_W,

		OP_MD_ADDSS_H,
		OP_MD_ADDSS_W,

		OP_MD_ADDUS_B,
		OP_MD_ADDUS_H,
		OP_MD_ADDUS_W,

		OP_MD_SUB_B,
		OP_MD_SUB_H,
		OP_MD_SUB_W,

		OP_MD_SUBSS_H,
		OP_MD_SUBSS_W,

		OP_MD_SUBUS_B,
		OP_MD_SUBUS_H,

		OP_MD_CMPEQ_B,
		OP_MD_CMPEQ_H,
		OP_MD_CMPEQ_W,
		OP_MD_CMPGT_B,
		OP_MD_CMPGT_H,
		OP_MD_CMPGT_W,

		OP_MD_MIN_H,
		OP_MD_MIN_W,
		OP_MD_MAX_H,
		OP_MD_MAX_W,

		OP_MD_AND,
		OP_MD_OR,
		OP_MD_XOR,
		OP_MD_NOT,

		OP_MD_SRLH,
		OP_MD_SRAH,
		OP_MD_SLLH,

		OP_MD_SRLW,
		OP_MD_SRAW,
		OP_MD_SLLW,

		OP_MD_SRL256,

		OP_MD_ISNEGATIVE,
		OP_MD_ISZERO,

		OP_MD_TOWORD_TRUNCATE,
		OP_MD_TOSINGLE,

		OP_MD_EXPAND,
		OP_MD_UNPACK_LOWER_BH,
		OP_MD_UNPACK_LOWER_HW,
		OP_MD_UNPACK_LOWER_WD,

		OP_MD_UNPACK_UPPER_BH,
		OP_MD_UNPACK_UPPER_HW,
		OP_MD_UNPACK_UPPER_WD,

		OP_MD_PACK_HB,
		OP_MD_PACK_WH,

		OP_MD_ADD_S,
		OP_MD_SUB_S,
		OP_MD_MUL_S,
		OP_MD_DIV_S,
		OP_MD_ABS_S,
		OP_MD_MIN_S,
		OP_MD_MAX_S,

		OP_FP_ADD,
		OP_FP_SUB,
		OP_FP_MUL,
		OP_FP_DIV,
		OP_FP_SQRT,
		OP_FP_RSQRT,
		OP_FP_RCPL,
		OP_FP_ABS,
		OP_FP_NEG,
		OP_FP_MAX,
		OP_FP_MIN,
		OP_FP_CMP,

		OP_FP_LDCST,
		OP_FP_TOINT_TRUNC,

		OP_PARAM,
		OP_PARAM_RET,
		OP_CALL,
		OP_RETVAL,
		OP_JMP,
		OP_CONDJMP,
		OP_GOTO,

		OP_LABEL,
	};

	enum CONDITION
	{
		CONDITION_NEVER = 0,
		CONDITION_EQ,
		CONDITION_NE,
		CONDITION_BL,
		CONDITION_BE,
		CONDITION_AB,
		CONDITION_AE,
		CONDITION_LT,
		CONDITION_LE,
		CONDITION_GT,
		CONDITION_GE,
	};

	struct STATEMENT
	{
	public:
		STATEMENT()
			: op(OP_NOP)
			, jmpBlock(-1)
			, jmpCondition(CONDITION_NEVER)
		{
			
		}

		OPERATION		op;
		SymbolRefPtr	src1;
		SymbolRefPtr	src2;
		SymbolRefPtr	dst;
		uint32			jmpBlock;
		CONDITION		jmpCondition;

		typedef std::function<void (SymbolRefPtr&, bool)> OperandVisitor;
		typedef std::function<void (const SymbolRefPtr&, bool)> ConstOperandVisitor;

		void VisitOperands(const OperandVisitor& visitor)
		{
			if(dst) visitor(dst, true);
			if(src1) visitor(src1, false);
			if(src2) visitor(src2, false);
		}

		void VisitOperands(const ConstOperandVisitor& visitor) const
		{
			if(dst) visitor(dst, true);
			if(src1) visitor(src1, false);
			if(src2) visitor(src2, false);
		}

		void VisitDestination(const ConstOperandVisitor& visitor) const
		{
			if(dst) visitor(dst, true);
		}

		void VisitSources(const ConstOperandVisitor& visitor) const
		{
			if(src1) visitor(src1, false);
			if(src2) visitor(src2, false);
		}
	};

	typedef std::list<STATEMENT> StatementList;

	std::string		ConditionToString(CONDITION);
	void			DumpStatementList(const StatementList&);
	void			DumpStatementList(std::ostream&, const StatementList&);

	template<typename ListType, typename IteratorType, typename ValueType>
	class IndexedStatementListBase
	{
	public:
		struct ITERATOR
		{
		public:
			struct VALUE
			{
				VALUE(const IteratorType& iterator, ValueType& statement, unsigned int index)
					: iterator(iterator), statement(statement), index(index)
				{

				}

				IteratorType iterator;
				ValueType& statement;
				unsigned int index = 0;
			};

			ITERATOR(const IteratorType& iterator, unsigned int index)
				: m_iterator(iterator)
				, m_index(index)
			{

			}

			const ITERATOR& operator ++()
			{
				assert(m_index != -1);
				m_iterator++;
				m_index++;
				return (*this);
			}

			bool operator !=(const ITERATOR& rhs) const
			{
				return m_iterator != rhs.m_iterator;
			}

			VALUE operator *() const
			{
				return VALUE(m_iterator, *m_iterator, m_index);
			}

		private:
			IteratorType m_iterator;
			unsigned int m_index = 0;
		};

		IndexedStatementListBase(ListType& statements)
			: m_statements(statements) { }

		ITERATOR begin() const
		{
			return ITERATOR(m_statements.begin(), 0);
		}

		ITERATOR end() const
		{
			return ITERATOR(m_statements.end(), -1);
		}

	private:
		ListType& m_statements;
	};

	typedef IndexedStatementListBase<StatementList, StatementList::iterator, STATEMENT> IndexedStatementList;
	typedef IndexedStatementListBase<const StatementList, StatementList::const_iterator, const STATEMENT> ConstIndexedStatementList;
}
