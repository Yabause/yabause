#include <assert.h>
#include "Jitter_SymbolTable.h"

using namespace Jitter;

CSymbolTable::CSymbolTable()
{

}

CSymbolTable::~CSymbolTable()
{

}

CSymbolTable::SymbolSet& CSymbolTable::GetSymbols()
{
	return m_symbols;
}

CSymbolTable::SymbolIterator CSymbolTable::RemoveSymbol(const SymbolIterator& symbolIterator)
{
	return m_symbols.erase(symbolIterator);
}

SymbolPtr CSymbolTable::MakeSymbol(const SymbolPtr& srcSymbol)
{
	auto symbolIterator(m_symbols.find(srcSymbol));
	if(symbolIterator != m_symbols.end())
	{
		return *symbolIterator;
	}
	auto result = std::make_shared<CSymbol>(*srcSymbol);
	m_symbols.insert(result);
	return result;
}

SymbolPtr CSymbolTable::MakeSymbol(SYM_TYPE type, uint32 valueLow, uint32 valueHigh)
{
	CSymbol symbol(type, valueLow, valueHigh);
	return MakeSymbol(SymbolPtr(&symbol, SymbolNullDeleter()));
}
