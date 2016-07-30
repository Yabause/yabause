#include "Jitter_CodeGen.h"

using namespace Jitter;

void CCodeGen::SetExternalSymbolReferencedHandler(const ExternalSymbolReferencedHandler& externalSymbolReferencedHandler)
{
	m_externalSymbolReferencedHandler = externalSymbolReferencedHandler;
}

bool CCodeGen::SymbolMatches(MATCHTYPE match, const SymbolRefPtr& symbolRef)
{
	if(match == MATCH_ANY) return true;
	if(match == MATCH_NIL) { if(!symbolRef) return true; else return false; }
	CSymbol* symbol = symbolRef->GetSymbol().get();
	switch(match)
	{
	case MATCH_RELATIVE:
		return (symbol->m_type == SYM_RELATIVE);
	case MATCH_CONSTANT:
		return (symbol->m_type == SYM_CONSTANT);
	case MATCH_CONSTANTPTR:
		return (symbol->m_type == SYM_CONSTANTPTR);
	case MATCH_REGISTER:
		return (symbol->m_type == SYM_REGISTER);
	case MATCH_TEMPORARY:
		return (symbol->m_type == SYM_TEMPORARY);
	case MATCH_MEMORY:
		return (symbol->m_type == SYM_RELATIVE) || (symbol->m_type == SYM_TEMPORARY);
	case MATCH_VARIABLE:
		return (symbol->m_type == SYM_REGISTER) || (symbol->m_type == SYM_RELATIVE) || (symbol->m_type == SYM_TEMPORARY);

	case MATCH_REL_REF:
		return (symbol->m_type == SYM_REL_REFERENCE);
		break;
	case MATCH_TMP_REF:
		return (symbol->m_type == SYM_TMP_REFERENCE);
		break;
	case MATCH_MEM_REF:
		return (symbol->m_type == SYM_REL_REFERENCE) || (symbol->m_type == SYM_TMP_REFERENCE);
		break;

	case MATCH_RELATIVE64:
		return (symbol->m_type == SYM_RELATIVE64);
	case MATCH_TEMPORARY64:
		return (symbol->m_type == SYM_TEMPORARY64);
	case MATCH_CONSTANT64:
		return (symbol->m_type == SYM_CONSTANT64);
	case MATCH_MEMORY64:
		return (symbol->m_type == SYM_RELATIVE64) || (symbol->m_type == SYM_TEMPORARY64);

	case MATCH_RELATIVE_FP_SINGLE:
		return (symbol->m_type == SYM_FP_REL_SINGLE);
	case MATCH_TEMPORARY_FP_SINGLE:
		return (symbol->m_type == SYM_FP_TMP_SINGLE);
	case MATCH_MEMORY_FP_SINGLE:
		return (symbol->m_type == SYM_FP_REL_SINGLE) || (symbol->m_type == SYM_FP_TMP_SINGLE);

	case MATCH_RELATIVE_FP_INT32:
		return (symbol->m_type == SYM_FP_REL_INT32);

	case MATCH_REGISTER128:
		return (symbol->m_type == SYM_REGISTER128);
	case MATCH_RELATIVE128:
		return (symbol->m_type == SYM_RELATIVE128);
	case MATCH_TEMPORARY128:
		return (symbol->m_type == SYM_TEMPORARY128);
	case MATCH_MEMORY128:
		return (symbol->m_type == SYM_RELATIVE128) || (symbol->m_type == SYM_TEMPORARY128);
	case MATCH_VARIABLE128:
		return (symbol->m_type == SYM_REGISTER128) || (symbol->m_type == SYM_RELATIVE128) || (symbol->m_type == SYM_TEMPORARY128);

	case MATCH_MEMORY256:
		return (symbol->m_type == SYM_TEMPORARY256);
		
	case MATCH_CONTEXT:
		return (symbol->m_type == SYM_CONTEXT);
	}
	return false;
}

uint32 CCodeGen::GetRegisterUsage(const StatementList& statements)
{
	uint32 registerUsage = 0;
	for(const auto& statement : statements)
	{
		if(CSymbol* dst = dynamic_symbolref_cast(SYM_REGISTER, statement.dst))
		{
			registerUsage |= (1 << dst->m_valueLow);
		}
	}
	return registerUsage;
}
