#ifndef _JITTER_SYMBOLTABLE_H_
#define _JITTER_SYMBOLTABLE_H_

#include <unordered_set>
#include "Jitter_Symbol.h"

namespace Jitter
{
	class CSymbolTable
	{
	public:
		struct SymbolNullDeleter
		{
			void operator()(void const *) const
			{

			}
		};

		typedef std::unordered_set<SymbolPtr, SymbolHasher, SymbolComparator> SymbolSet;
		typedef SymbolSet::iterator SymbolIterator;

								CSymbolTable();
		virtual					~CSymbolTable();

		SymbolPtr				MakeSymbol(const SymbolPtr&);
		SymbolPtr				MakeSymbol(SYM_TYPE, uint32, uint32 = 0);
		SymbolIterator			RemoveSymbol(const SymbolIterator&);

		SymbolSet&				GetSymbols();

	private:
		SymbolSet				m_symbols;
	};
}

#endif
