#include <iostream>
#include "Jitter_Statement.h"

using namespace Jitter;

std::string Jitter::ConditionToString(CONDITION condition)
{
	switch(condition)
	{
	case CONDITION_LT:
		return "LT";
		break;
	case CONDITION_LE:
		return "LE";
		break;
	case CONDITION_GT:
		return "GT";
		break;
	case CONDITION_EQ:
		return "EQ";
		break;
	case CONDITION_NE:
		return "NE";
		break;
	case CONDITION_BL:
		return "BL";
		break;
	case CONDITION_AB:
		return "AB";
		break;
	default:
		return "??";
		break;
	}
}

void Jitter::DumpStatementList(const StatementList& statements)
{
	DumpStatementList(std::cout, statements);
}

void Jitter::DumpStatementList(std::ostream& outputStream, const StatementList& statements)
{
	for(const auto& statement : statements)
	{
		if(statement.dst)
		{
			outputStream << statement.dst->ToString();
			outputStream << " := ";
		}

		if(statement.src1)
		{
			outputStream << statement.src1->ToString();
		}

		switch(statement.op)
		{
		case OP_ADD:
		case OP_ADD64:
		case OP_ADDREF:
		case OP_FP_ADD:
			outputStream << " + ";
			break;
		case OP_SUB:
		case OP_SUB64:
		case OP_FP_SUB:
			outputStream << " - ";
			break;
		case OP_CMP:
		case OP_CMP64:
		case OP_FP_CMP:
			outputStream << " CMP(" << ConditionToString(statement.jmpCondition) << ") ";
			break;
		case OP_MUL:
		case OP_MULS:
		case OP_FP_MUL:
			outputStream << " * ";
			break;
		case OP_DIV:
		case OP_DIVS:
		case OP_FP_DIV:
			outputStream << " / ";
			break;
		case OP_AND:
		case OP_AND64:
		case OP_MD_AND:
			outputStream << " & ";
			break;
		case OP_LZC:
			outputStream << " LZC";
			break;
		case OP_OR:
		case OP_MD_OR:
			outputStream << " | ";
			break;
		case OP_XOR:
		case OP_MD_XOR:
			outputStream << " ^ ";
			break;
		case OP_NOT:
		case OP_MD_NOT:
			outputStream << " ! ";
			break;
		case OP_SRL:
		case OP_SRL64:
			outputStream << " >> ";
			break;
		case OP_SRA:
		case OP_SRA64:
			outputStream << " >>A ";
			break;
		case OP_SLL:
		case OP_SLL64:
			outputStream << " << ";
			break;
		case OP_NOP:
			outputStream << " NOP ";
			break;
		case OP_MOV:
			break;
		case OP_STOREATREF:
			outputStream << " <- ";
			break;
		case OP_LOADFROMREF:
			outputStream << " LOADFROM ";
			break;
		case OP_RELTOREF:
			outputStream << " TOREF ";
			break;
		case OP_PARAM:
		case OP_PARAM_RET:
			outputStream << " PARAM ";
			break;
		case OP_CALL:
			outputStream << " CALL ";
			break;
		case OP_RETVAL:
			outputStream << " RETURNVALUE ";
			break;
		case OP_JMP:
			outputStream << " JMP{" << statement.jmpBlock << "} ";
			break;
		case OP_CONDJMP:
			outputStream << " JMP{" << statement.jmpBlock << "}(" << ConditionToString(statement.jmpCondition) << ") ";
			break;
		case OP_LABEL:
			outputStream << "LABEL_" << statement.jmpBlock << ":";
			break;
		case OP_EXTLOW64:
			outputStream << " EXTLOW64";
			break;
		case OP_EXTHIGH64:
			outputStream << " EXTHIGH64";
			break;
		case OP_MERGETO64:
			outputStream << " MERGETO64 ";
			break;
		case OP_MERGETO256:
			outputStream << " MERGETO256 ";
			break;
		case OP_FP_ABS:
			outputStream << " ABS";
			break;
		case OP_FP_NEG:
			outputStream << " NEG";
			break;
		case OP_FP_MIN:
			outputStream << " MIN ";
			break;
		case OP_FP_MAX:
			outputStream << " MAX ";
			break;
		case OP_FP_SQRT:
			outputStream << " SQRT";
			break;
		case OP_FP_RSQRT:
			outputStream << " RSQRT";
			break;
		case OP_FP_RCPL:
			outputStream << " RCPL";
			break;
		case OP_FP_TOINT_TRUNC:
			outputStream << " INT(TRUNC)";
			break;
		case OP_FP_LDCST:
			outputStream << " LOAD ";
			break;
		case OP_MD_MOV_MASKED:
			outputStream << " MOVMSK ";
			break;
		case OP_MD_PACK_HB:
			outputStream << " PACK_HB ";
			break;
		case OP_MD_PACK_WH:
			outputStream << " PACK_WH ";
			break;
		case OP_MD_UNPACK_LOWER_BH:
			outputStream << " UNPACK_LOWER_BH ";
			break;
		case OP_MD_UNPACK_LOWER_HW:
			outputStream << " UNPACK_LOWER_HW ";
			break;
		case OP_MD_UNPACK_LOWER_WD:
			outputStream << " UNPACK_LOWER_WD ";
			break;
		case OP_MD_UNPACK_UPPER_BH:
			outputStream << " UNPACK_UPPER_BH ";
			break;
		case OP_MD_UNPACK_UPPER_HW:
			outputStream << " UNPACK_UPPER_HW ";
			break;
		case OP_MD_UNPACK_UPPER_WD:
			outputStream << " UNPACK_UPPER_WD ";
			break;
		case OP_MD_ADD_B:
			outputStream << " +(B) ";
			break;
		case OP_MD_ADD_H:
			outputStream << " +(H) ";
			break;
		case OP_MD_ADD_W:
			outputStream << " +(W) ";
			break;
		case OP_MD_ADDSS_H:
			outputStream << " +(SSH) ";
			break;
		case OP_MD_ADDSS_W:
			outputStream << " +(SSW) ";
			break;
		case OP_MD_ADDUS_B:
			outputStream << " +(USB) ";
			break;
		case OP_MD_ADDUS_W:
			outputStream << " +(USW) ";
			break;
		case OP_MD_SUB_B:
			outputStream << " -(B) ";
			break;
		case OP_MD_SUB_H:
			outputStream << " -(H) ";
			break;
		case OP_MD_SUB_W:
			outputStream << " -(W) ";
			break;
		case OP_MD_SUBSS_H:
			outputStream << " -(SSH) ";
			break;
		case OP_MD_SUBUS_B:
			outputStream << " -(USB) ";
			break;
		case OP_MD_SUBUS_H:
			outputStream << " -(USH) ";
			break;
		case OP_MD_SLLW:
			outputStream << " <<(W) ";
			break;
		case OP_MD_SLLH:
			outputStream << " <<(H) ";
			break;
		case OP_MD_SRLH:
			outputStream << " >>(H) ";
			break;
		case OP_MD_SRLW:
			outputStream << " >>(W) ";
			break;
		case OP_MD_SRAH:
			outputStream << " >>A(H) ";
			break;
		case OP_MD_SRAW:
			outputStream << " >>A(W) ";
			break;
		case OP_MD_SRL256:
			outputStream << " >>(256) ";
			break;
		case OP_MD_CMPEQ_W:
			outputStream << " CMP(EQ,W) ";
			break;
		case OP_MD_CMPGT_H:
			outputStream << " CMP(GT,H) ";
			break;
		case OP_MD_ADD_S:
			outputStream << " +(S) ";
			break;
		case OP_MD_SUB_S:
			outputStream << " -(S) ";
			break;
		case OP_MD_MUL_S:
			outputStream << " *(S) ";
			break;
		case OP_MD_DIV_S:
			outputStream << " /(S) ";
			break;
		case OP_MD_MIN_H:
			outputStream << " MIN(H) ";
			break;
		case OP_MD_MIN_W:
			outputStream << " MIN(W) ";
			break;
		case OP_MD_MIN_S:
			outputStream << " MIN(S) ";
			break;
		case OP_MD_MAX_H:
			outputStream << " MAX(H) ";
			break;
		case OP_MD_MAX_W:
			outputStream << " MAX(W) ";
			break;
		case OP_MD_MAX_S:
			outputStream << " MAX(S) ";
			break;
		case OP_MD_ABS_S:
			outputStream << " ABS(S)";
			break;
		case OP_MD_ISNEGATIVE:
			outputStream << " ISNEGATIVE";
			break;
		case OP_MD_ISZERO:
			outputStream << " ISZERO";
			break;
		case OP_MD_EXPAND:
			outputStream << " EXPAND";
			break;
		case OP_MD_TOSINGLE:
			outputStream << " TOSINGLE";
			break;
		case OP_MD_TOWORD_TRUNCATE:
			outputStream << " TOWORD_TRUNCATE";
			break;
		default:
			outputStream << " ?? ";
			break;
		}

		if(statement.src2)
		{
			outputStream << statement.src2->ToString();
		}

		outputStream << std::endl;
	}
}
