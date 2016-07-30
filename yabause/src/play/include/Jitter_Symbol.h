#pragma once

#include "Types.h"
#include <memory>
#include <string>
#include <cassert>
#include <cstdlib>

namespace Jitter
{
	enum SYM_TYPE
	{
		SYM_CONTEXT,

		SYM_CONSTANT,
		SYM_CONSTANTPTR,
		SYM_RELATIVE,
		SYM_TEMPORARY,
		SYM_REGISTER,

		SYM_REL_REFERENCE,
		SYM_TMP_REFERENCE,

		SYM_RELATIVE64,
		SYM_TEMPORARY64,
		SYM_CONSTANT64,

		SYM_RELATIVE128,
		SYM_TEMPORARY128,
		SYM_REGISTER128,

		SYM_TEMPORARY256,

		SYM_FP_REL_SINGLE,
		SYM_FP_TMP_SINGLE,

		SYM_FP_REL_INT32,
	};

	class CSymbol
	{
	public:
		CSymbol(SYM_TYPE type, uint32 valueLow, uint32 valueHigh)
			: m_type(type)
			, m_valueLow(valueLow)
			, m_valueHigh(valueHigh)
		{

		}

		std::string ToString() const
		{
			switch(m_type)
			{
			case SYM_CONTEXT:
				return "CTX";
				break;
			case SYM_CONSTANT:
				return std::to_string(m_valueLow);
				break;
			case SYM_CONSTANTPTR:
				return std::to_string(GetConstantPtr());
				break;
			case SYM_RELATIVE:
				return "REL[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_TEMPORARY:
				return "TMP[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_REL_REFERENCE:
				return "REL&[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_TMP_REFERENCE:
				return "TMP&[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_CONSTANT64:
				return "CST64[" + std::to_string(m_valueLow) + ", " + std::to_string(m_valueHigh) + "]";
				break;
			case SYM_TEMPORARY64:
				return "TMP64[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_RELATIVE64:
				return "REL64[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_REGISTER:
				return "REG[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_FP_REL_SINGLE:
				return "REL(FP_S)[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_FP_REL_INT32:
				return "REL(FP_I32)[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_FP_TMP_SINGLE:
				return "TMP(FP_S)[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_RELATIVE128:
				return "REL128[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_TEMPORARY128:
				return "TMP128[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_REGISTER128:
				return "REG128[" + std::to_string(m_valueLow) + "]";
				break;
			case SYM_TEMPORARY256:
				return "TMP256[" + std::to_string(m_valueLow) + "]";
				break;
			default:
				return "";
				break;
			}
		}

		int GetSize() const
		{
			switch(m_type)
			{
			case SYM_RELATIVE:
			case SYM_REGISTER:
			case SYM_CONSTANT:
			case SYM_TEMPORARY:
				return 4;
				break;
			case SYM_RELATIVE64:
			case SYM_TEMPORARY64:
			case SYM_CONSTANT64:
				return 8;
				break;
			case SYM_RELATIVE128:
			case SYM_TEMPORARY128:
				return 16;
				break;
			case SYM_TEMPORARY256:
				return 32;
				break;
			case SYM_FP_REL_SINGLE:
			case SYM_FP_TMP_SINGLE:
			case SYM_FP_REL_INT32:
				return 4;
				break;
			case SYM_REL_REFERENCE:
			case SYM_TMP_REFERENCE:
				return sizeof(void*);
				break;
			default:
				assert(0);
				return 4;
				break;
			}
		}

		bool IsRegister() const
		{
			return
				(m_type == SYM_REGISTER) ||
				(m_type == SYM_REGISTER128);
		}

		bool IsRelative() const
		{
			return 
				(m_type == SYM_RELATIVE) || 
				(m_type == SYM_RELATIVE64) || 
				(m_type == SYM_RELATIVE128) || 
				(m_type == SYM_REL_REFERENCE) ||
				(m_type == SYM_FP_REL_SINGLE) || 
				(m_type == SYM_FP_REL_INT32);
		}

		bool IsConstant() const
		{
			return (m_type == SYM_CONSTANT) || (m_type == SYM_CONSTANT64);
		}

		bool IsTemporary() const
		{
			return 
				(m_type == SYM_TEMPORARY) || 
				(m_type == SYM_TEMPORARY64) || 
				(m_type == SYM_TEMPORARY128) || 
				(m_type == SYM_TEMPORARY256) ||
				(m_type == SYM_TMP_REFERENCE) ||
				(m_type == SYM_FP_TMP_SINGLE);
		}

		bool Equals(CSymbol* symbol) const
		{
			return 
				(symbol) &&
				(symbol->m_type == m_type) &&
				(symbol->m_valueLow == m_valueLow) &&
				(symbol->m_valueHigh == m_valueHigh);
		}

		uint64 GetConstant64() const
		{
			assert(m_type == SYM_CONSTANT64);
			return static_cast<uint64>(m_valueLow) | (static_cast<uint64>(m_valueHigh) << 32);
		}
		
		uintptr_t GetConstantPtr() const
		{
			assert(m_type == SYM_CONSTANTPTR);
#if (UINTPTR_MAX == UINT32_MAX)
			return m_valueLow;
#elif (UINTPTR_MAX == UINT64_MAX)
			return static_cast<uint64>(m_valueLow) | (static_cast<uint64>(m_valueHigh) << 32);
#else
			static_assert(false, "Unsupported pointer size.");
#endif
		}

		bool Aliases(CSymbol* symbol) const
		{
			if(!IsRelative()) return false;
			if(!symbol->IsRelative()) return false;
			uint32 base1 = m_valueLow;
			uint32 base2 = symbol->m_valueLow;
//			uint32 end1 = base1 + GetSize();
//			uint32 end2 = base2 + symbol->GetSize();
			if(abs(static_cast<int32>(base2 - base1)) < GetSize()) return true;
			if(abs(static_cast<int32>(base2 - base1)) < symbol->GetSize()) return true;
			return false;
		}

		SYM_TYPE				m_type;
		uint32					m_valueLow;
		uint32					m_valueHigh;

		unsigned int			m_stackLocation = -1;
	};

	typedef std::shared_ptr<CSymbol> SymbolPtr;
	typedef std::weak_ptr<CSymbol> WeakSymbolPtr;

	struct SymbolComparator
	{
		bool operator()(const SymbolPtr& sym1, const SymbolPtr& sym2) const
		{
			return sym1->Equals(sym2.get());
		}
	};

	struct SymbolHasher
	{
		size_t operator()(const SymbolPtr& symbol) const
		{
			return (symbol->m_type << 24) ^ symbol->m_valueLow ^ symbol->m_valueHigh;
		}
	};
}
