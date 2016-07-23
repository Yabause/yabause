#include "Jitter.h"
#include <iostream>
#include <set>

#ifdef _DEBUG
//#define DUMP_STATEMENTS
#endif

#ifdef DUMP_STATEMENTS
#include <iostream>
#endif

using namespace Jitter;

void CJitter::AllocateRegisters(BASIC_BLOCK& basicBlock)
{
	auto& symbolTable = basicBlock.symbolTable;

	std::multimap<unsigned int, STATEMENT> loadStatements;
	std::multimap<unsigned int, STATEMENT> spillStatements;
#ifdef DUMP_STATEMENTS
	DumpStatementList(basicBlock.statements);
	std::cout << std::endl;
#endif

	auto allocRanges = ComputeAllocationRanges(basicBlock);
	for(const auto& allocRange : allocRanges)
	{
		SymbolRegAllocInfo symbolRegAllocs;
		ComputeLivenessForRange(basicBlock, allocRange, symbolRegAllocs);

		MarkAliasedSymbols(basicBlock, allocRange, symbolRegAllocs);

		AssociateSymbolsToRegisters(symbolRegAllocs);

		//Replace all references to symbols by references to allocated registers
		for(const auto& statementInfo : IndexedStatementList(basicBlock.statements))
		{
			auto& statement(statementInfo.statement);
			const auto& statementIdx(statementInfo.index);
			if(statementIdx < allocRange.first) continue;
			if(statementIdx > allocRange.second) break;

			statement.VisitOperands(
				[&] (SymbolRefPtr& symbolRef, bool)
				{
					auto symbol = symbolRef->GetSymbol();
					auto symbolRegAllocIterator = symbolRegAllocs.find(symbol);
					if(symbolRegAllocIterator != std::end(symbolRegAllocs))
					{
						const auto& symbolRegAlloc = symbolRegAllocIterator->second;
						if(symbolRegAlloc.registerId != -1)
						{
							symbolRef = MakeSymbolRef(
								symbolTable.MakeSymbol(symbolRegAlloc.registerType, symbolRegAlloc.registerId));
						}
					}
				}
			);
		}

		//Prepare load and spills
		for(const auto& symbolRegAllocPair : symbolRegAllocs)
		{
			const auto& symbol = symbolRegAllocPair.first;
			const auto& symbolRegAlloc = symbolRegAllocPair.second;

			//Check if it's actually allocated
			if(symbolRegAlloc.registerId == -1) continue;

			//firstUse == -1 means it is written to but never used afterwards in this block

			//Do we need to load register at the beginning?
			//If symbol is read and we use this symbol before we define it, so we need to load it first
			if((symbolRegAlloc.firstUse != -1) && (symbolRegAlloc.firstUse <= symbolRegAlloc.firstDef))
			{
				STATEMENT statement;
				statement.op	= OP_MOV;
				statement.dst	= std::make_shared<CSymbolRef>(
					symbolTable.MakeSymbol(symbolRegAlloc.registerType, symbolRegAlloc.registerId));
				statement.src1	= std::make_shared<CSymbolRef>(symbol);

				loadStatements.insert(std::make_pair(allocRange.first, statement));
			}

			//If symbol is defined, we need to save it at the end
			if(symbolRegAlloc.firstDef != -1)
			{
				STATEMENT statement;
				statement.op	= OP_MOV;
				statement.dst	= std::make_shared<CSymbolRef>(symbol);
				statement.src1	= std::make_shared<CSymbolRef>(
					symbolTable.MakeSymbol(symbolRegAlloc.registerType, symbolRegAlloc.registerId));

				spillStatements.insert(std::make_pair(allocRange.second, statement));
			}
		}
	}

#ifdef DUMP_STATEMENTS
	DumpStatementList(basicBlock.statements);
	std::cout << std::endl;
#endif

	std::map<unsigned int, StatementList::const_iterator> loadPoints;
	std::map<unsigned int, StatementList::const_iterator> spillPoints;

	//Load
	for(const auto& statementInfo : ConstIndexedStatementList(basicBlock.statements))
	{
		const auto& statementIdx(statementInfo.index);
		if(loadStatements.find(statementIdx) != std::end(loadStatements))
		{
			loadPoints.insert(std::make_pair(statementIdx, statementInfo.iterator));
		}
	}

	//Spill
	for(const auto& statementInfo : ConstIndexedStatementList(basicBlock.statements))
	{
		const auto& statementIdx(statementInfo.index);
		if(spillStatements.find(statementIdx) != std::end(spillStatements))
		{
			const auto& statement = statementInfo.statement;
			auto statementIterator = statementInfo.iterator;
			if((statement.op != OP_CONDJMP) && (statement.op != OP_JMP) && (statement.op != OP_CALL))
			{
				statementIterator++;
			}
			spillPoints.insert(std::make_pair(statementIdx, statementIterator));
		}
	}

	//Loads
	for(const auto& loadPoint : loadPoints)
	{
		unsigned int statementIndex = loadPoint.first;
		for(auto statementIterator = loadStatements.lower_bound(statementIndex);
			statementIterator != loadStatements.upper_bound(statementIndex);
			statementIterator++)
		{
			const auto& statement(statementIterator->second);
			basicBlock.statements.insert(loadPoint.second, statement);
		}
	}

	//Spills
	for(const auto& spillPoint : spillPoints)
	{
		unsigned int statementIndex = spillPoint.first;
		for(auto statementIterator = spillStatements.lower_bound(statementIndex);
			statementIterator != spillStatements.upper_bound(statementIndex);
			statementIterator++)
		{
			const auto& statement(statementIterator->second);
			basicBlock.statements.insert(spillPoint.second, statement);
		}
	}

#ifdef DUMP_STATEMENTS
	DumpStatementList(basicBlock.statements);
	std::cout << std::endl;
#endif
}

void CJitter::AssociateSymbolsToRegisters(SymbolRegAllocInfo& symbolRegAllocs) const
{
	std::multimap<SYM_TYPE, unsigned int> availableRegisters;
	{
		unsigned int regCount = m_codeGen->GetAvailableRegisterCount();
		for(unsigned int i = 0; i < regCount; i++)
		{
			availableRegisters.insert(std::make_pair(SYM_REGISTER, i));
		}
	}

	{
		unsigned int regCount = m_codeGen->GetAvailableMdRegisterCount();
		for(unsigned int i = 0; i < regCount; i++)
		{
			availableRegisters.insert(std::make_pair(SYM_REGISTER128, i));
		}
	}

	auto isRegisterAllocatable =
		[] (SYM_TYPE symbolType)
		{
			return 
				(symbolType == SYM_RELATIVE) || (symbolType == SYM_TEMPORARY) ||
				(symbolType == SYM_RELATIVE128) || (symbolType == SYM_TEMPORARY128);
		};

	//Sort symbols by usage count
	std::list<SymbolRegAllocInfo::value_type*> sortedSymbols;
	for(auto& symbolRegAllocPair : symbolRegAllocs)
	{
		const auto& symbol(symbolRegAllocPair.first);
		const auto& symbolRegAlloc(symbolRegAllocPair.second);
		if(!isRegisterAllocatable(symbol->m_type)) continue;
		if(symbolRegAlloc.aliased) continue;
		sortedSymbols.push_back(&symbolRegAllocPair);
	}
	sortedSymbols.sort(
		[] (SymbolRegAllocInfo::value_type* symbolRegAllocPair1, SymbolRegAllocInfo::value_type* symbolRegAllocPair2)
		{
			const auto& symbol1(symbolRegAllocPair1->first);
			const auto& symbol2(symbolRegAllocPair2->first);
			const auto& symbolRegAlloc1(symbolRegAllocPair1->second);
			const auto& symbolRegAlloc2(symbolRegAllocPair2->second);
			if(symbolRegAlloc1.useCount == symbolRegAlloc2.useCount)
			{
				if(symbol1->m_type == symbol2->m_type)
				{
					return symbol1->m_valueLow > symbol2->m_valueLow;
				}
				else
				{
					return symbol1->m_type > symbol2->m_type;
				}
			}
			else
			{
				return symbolRegAlloc1.useCount > symbolRegAlloc2.useCount;
			}
		}
	);

	for(auto& symbolRegAllocPair : sortedSymbols)
	{
		if(availableRegisters.empty()) break;

		const auto& symbol = symbolRegAllocPair->first;
		auto& symbolRegAlloc = symbolRegAllocPair->second;

		//Find suitable register for this symbol
		auto registerIterator = std::end(availableRegisters);
		auto registerIteratorEnd = std::end(availableRegisters);
		if((symbol->m_type == SYM_RELATIVE) || (symbol->m_type == SYM_TEMPORARY))
		{
			registerIterator = availableRegisters.lower_bound(SYM_REGISTER);
			registerIteratorEnd = availableRegisters.upper_bound(SYM_REGISTER);
		}
		else if((symbol->m_type == SYM_RELATIVE128) || (symbol->m_type == SYM_TEMPORARY128))
		{
			registerIterator = availableRegisters.lower_bound(SYM_REGISTER128);
			registerIteratorEnd = availableRegisters.upper_bound(SYM_REGISTER128);
		}
		if(registerIterator != registerIteratorEnd)
		{
			symbolRegAlloc.registerType = registerIterator->first;
			symbolRegAlloc.registerId = registerIterator->second;
			availableRegisters.erase(registerIterator);
		}
	}
}

CJitter::AllocationRangeArray CJitter::ComputeAllocationRanges(const BASIC_BLOCK& basicBlock)
{
	AllocationRangeArray result;
	unsigned int currentStart = 0;
	for(const auto& statementInfo : ConstIndexedStatementList(basicBlock.statements))
	{
		const auto& statement(statementInfo.statement);
		const auto& statementIdx(statementInfo.index);
		if(statement.op == OP_CALL)
		{
			//Gotta split here
			result.push_back(std::make_pair(currentStart, statementIdx));
			currentStart = statementIdx + 1;
		}
	}
	result.push_back(std::make_pair(currentStart, basicBlock.statements.size() - 1));
	return result;
}

void CJitter::ComputeLivenessForRange(const BASIC_BLOCK& basicBlock, const AllocationRange& allocRange, SymbolRegAllocInfo& symbolRegAllocs) const
{
	for(const auto& statementInfo : ConstIndexedStatementList(basicBlock.statements))
	{
		const auto& statement(statementInfo.statement);
		unsigned int statementIdx(statementInfo.index);
		if(statementIdx < allocRange.first) continue;
		if(statementIdx > allocRange.second) continue;

		statement.VisitDestination(
			[&] (const SymbolRefPtr& symbolRef, bool)
			{
				auto symbol(symbolRef->GetSymbol());
				auto& symbolRegAlloc = symbolRegAllocs[symbol];
				symbolRegAlloc.useCount++;
				if(symbolRegAlloc.firstDef == -1)
				{
					symbolRegAlloc.firstDef = statementIdx;
				}
				if((symbolRegAlloc.lastDef == -1) || (statementIdx > symbolRegAlloc.lastDef))
				{
					symbolRegAlloc.lastDef = statementIdx;
				}
			}
		);

		statement.VisitSources(
			[&] (const SymbolRefPtr& symbolRef, bool)
			{
				auto symbol(symbolRef->GetSymbol());
				auto& symbolRegAlloc = symbolRegAllocs[symbol];
				symbolRegAlloc.useCount++;
				if(symbolRegAlloc.firstUse == -1)
				{
					symbolRegAlloc.firstUse = statementIdx;
				}
			}
		);
	}
}

void CJitter::MarkAliasedSymbols(const BASIC_BLOCK& basicBlock, const AllocationRange& allocRange, SymbolRegAllocInfo& symbolRegAllocs) const
{
	for(const auto& statementInfo : ConstIndexedStatementList(basicBlock.statements))
	{
		auto& statement(statementInfo.statement);
		const auto& statementIdx(statementInfo.index);
		if(statementIdx < allocRange.first) continue;
		if(statementIdx > allocRange.second) break;
		if(statement.op == OP_PARAM_RET)
		{
			//This symbol will end up being written to by the callee, thus will be aliased
			auto& symbolRegAlloc = symbolRegAllocs[statement.src1->GetSymbol()];
			symbolRegAlloc.aliased = true;
		}
		for(auto& symbolRegAlloc : symbolRegAllocs)
		{
			if(symbolRegAlloc.second.aliased) continue;
			auto testedSymbol = symbolRegAlloc.first;
			statement.VisitOperands(
				[&](const SymbolRefPtr& symbolRef, bool)
				{
					auto symbol = symbolRef->GetSymbol();
					if(symbol->Equals(testedSymbol.get())) return;
					if(symbol->Aliases(testedSymbol.get()))
					{
						symbolRegAlloc.second.aliased = true;
					}
				}
			);
		}
	}
}
