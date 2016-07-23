#include <cassert>
#include <cstring>
#include "MachoObjectFile.h"

using namespace Jitter;

template <typename MachoTraits>
CMachoObjectFile<MachoTraits>::CMachoObjectFile(CPU_ARCH cpuArch)
: CObjectFile(cpuArch)
{

}

template <typename MachoTraits>
CMachoObjectFile<MachoTraits>::~CMachoObjectFile()
{

}

template <typename MachoTraits>
void CMachoObjectFile<MachoTraits>::Write(Framework::CStream& stream)
{
	auto internalSymbolInfos = InternalSymbolInfoArray(m_internalSymbols.size());
	auto externalSymbolInfos = ExternalSymbolInfoArray(m_externalSymbols.size());

	StringTable stringTable;
	stringTable.push_back(0x00);
	FillStringTable(stringTable, m_internalSymbols, internalSymbolInfos);
	FillStringTable(stringTable, m_externalSymbols, externalSymbolInfos);

	auto textSection = BuildSection(m_internalSymbols, internalSymbolInfos, INTERNAL_SYMBOL_LOCATION_TEXT);
	auto dataSection = BuildSection(m_internalSymbols, internalSymbolInfos, INTERNAL_SYMBOL_LOCATION_DATA);
	
	//Make sure section size is aligned
	textSection.data.resize((textSection.data.size() + 0x03) & ~0x03);
	dataSection.data.resize((dataSection.data.size() + 0x01) & ~0x01);

	auto symbols = BuildSymbols(m_internalSymbols, internalSymbolInfos, m_externalSymbols, externalSymbolInfos,
		0, static_cast<uint32>(textSection.data.size()));

	auto textSectionRelocations = BuildRelocations(textSection, internalSymbolInfos, externalSymbolInfos);
	auto dataSectionRelocations = BuildRelocations(dataSection, internalSymbolInfos, externalSymbolInfos);

	uint32 sectionCount = 2;
	SectionArray sections;

	uint32 sizeofCommands = sizeof(typename MachoTraits::SEGMENT_COMMAND) + (sizeof(typename MachoTraits::SECTION) * sectionCount) + 
		sizeof(Macho::SYMTAB_COMMAND);
	uint32 headerSize = sizeof(typename MachoTraits::MACH_HEADER) + sizeofCommands;
	uint32 segmentDataSize = static_cast<uint32>(textSection.data.size()) + static_cast<uint32>(dataSection.data.size());
	uint32 segmentAndRelocDataSize = segmentDataSize + (textSectionRelocations.size() + dataSectionRelocations.size()) * sizeof(Macho::RELOCATION_INFO);

	{
		typename MachoTraits::SECTION section;
		memset(&section, 0, sizeof(typename MachoTraits::SECTION));
		strncpy(section.sectionName, "__text", 0x10);
		strncpy(section.segmentName, "__TEXT", 0x10);
		
		section.sectionName[0x0F] = 0;
		section.segmentName[0x0F] = 0;

		section.address				= 0;
		section.size				= static_cast<uint32>(textSection.data.size());
		section.offset				= headerSize;
		section.align				= 0x02;
		section.relocationOffset	= headerSize + segmentDataSize;
		section.relocationCount		= static_cast<uint32>(textSectionRelocations.size());
		section.flags				= Macho::S_ATTR_SOME_INSTRUCTIONS | Macho::S_ATTR_PURE_INSTRUCTIONS;
		section.reserved1			= 0;
		section.reserved2			= 0;

		sections.push_back(section);
	}

	{
		typename MachoTraits::SECTION section;
		memset(&section, 0, sizeof(typename MachoTraits::SECTION));
		strncpy(section.sectionName, "__data", 0x10);
		strncpy(section.segmentName, "__DATA", 0x10);
		
		section.sectionName[0x0F] = 0;
		section.segmentName[0x0F] = 0;

		section.address				= static_cast<uint32>(textSection.data.size());
		section.size				= static_cast<uint32>(dataSection.data.size());
		section.offset				= headerSize + static_cast<uint32>(textSection.data.size());
		section.align				= 0x01;
		section.relocationOffset	= headerSize + segmentDataSize + textSectionRelocations.size() * sizeof(Macho::RELOCATION_INFO);
		section.relocationCount		= static_cast<uint32>(dataSectionRelocations.size());
		section.flags				= 0;
		section.reserved1			= 0;
		section.reserved2			= 0;

		sections.push_back(section);
	}

	typename MachoTraits::SEGMENT_COMMAND segmentCommand;
	memset(&segmentCommand, 0, sizeof(typename MachoTraits::SEGMENT_COMMAND));
	segmentCommand.cmd				= MachoTraits::LC_SEGMENT;
	segmentCommand.cmdSize			= sizeof(typename MachoTraits::SEGMENT_COMMAND) + (sizeof(typename MachoTraits::SECTION) * sections.size());
	segmentCommand.vmAddress		= 0;
	segmentCommand.vmSize			= segmentDataSize;
	segmentCommand.fileOffset		= headerSize;
	segmentCommand.fileSize			= segmentDataSize;
	segmentCommand.maxProtection	= 0x07;
	segmentCommand.initProtection	= 0x07;
	segmentCommand.sectionCount		= sections.size();
	segmentCommand.flags			= 0;

	Macho::SYMTAB_COMMAND symtabCommand;
	memset(&symtabCommand, 0, sizeof(Macho::SYMTAB_COMMAND));
	symtabCommand.cmd				= Macho::LC_SYMTAB;
	symtabCommand.cmdSize			= sizeof(Macho::SYMTAB_COMMAND);
	symtabCommand.stringsSize		= stringTable.size();
	symtabCommand.stringsOffset		= headerSize + segmentAndRelocDataSize + (sizeof(typename MachoTraits::NLIST) * symbols.size());
	symtabCommand.symbolCount		= symbols.size();
	symtabCommand.symbolsOffset		= headerSize + segmentAndRelocDataSize;

	typename MachoTraits::MACH_HEADER header = {};
	header.magic = MachoTraits::HEADER_MAGIC;
	switch(m_cpuArch)
	{
	case CPU_ARCH_X86:
		header.cpuType			= Macho::CPU_TYPE_I386;
		header.cpuSubType		= Macho::CPU_SUBTYPE_I386_ALL;
		break;
	case CPU_ARCH_ARM:
		header.cpuType			= Macho::CPU_TYPE_ARM;
		header.cpuSubType		= Macho::CPU_SUBTYPE_ARM_V7;
		break;
	case CPU_ARCH_ARM64:
		header.cpuType			= Macho::CPU_TYPE_ARM64;
		header.cpuSubType		= Macho::CPU_SUBTYPE_ARM64_ALL;
		break;
	default:
		throw std::runtime_error("MachoObjectFile: Unsupported CPU architecture.");
		break;
	}
	header.fileType			= Macho::MH_OBJECT;
	header.commandCount		= 2;		//SEGMENT_COMMMAND + SYMTAB_COMMAND
	header.sizeofCommands	= sizeofCommands;
	header.flags			= Macho::MH_NOMULTIDEFS;

	stream.Write(&header, sizeof(typename MachoTraits::MACH_HEADER));
	stream.Write(&segmentCommand, sizeof(typename MachoTraits::SEGMENT_COMMAND));
	for(const auto& section : sections)
	{
		stream.Write(&section, sizeof(typename MachoTraits::SECTION));
	}
	stream.Write(&symtabCommand, sizeof(Macho::SYMTAB_COMMAND));
	stream.Write(textSection.data.data(), textSection.data.size());
	stream.Write(dataSection.data.data(), dataSection.data.size());
	stream.Write(textSectionRelocations.data(), textSectionRelocations.size() * sizeof(Macho::RELOCATION_INFO));
	stream.Write(dataSectionRelocations.data(), dataSectionRelocations.size() * sizeof(Macho::RELOCATION_INFO));
	stream.Write(symbols.data(), sizeof(typename MachoTraits::NLIST) * symbols.size());
	stream.Write(stringTable.data(), stringTable.size());
}

template <typename MachoTraits>
void CMachoObjectFile<MachoTraits>::FillStringTable(StringTable& stringTable, const InternalSymbolArray& internalSymbols, InternalSymbolInfoArray& internalSymbolInfos)
{
	uint32 stringTableSizeIncrement = 0;
	for(const auto& internalSymbol : internalSymbols)
	{
		stringTableSizeIncrement += static_cast<uint32>(internalSymbol.name.length()) + 1;
	}
	stringTable.reserve(stringTable.size() + stringTableSizeIncrement);
	for(uint32 i = 0; i < internalSymbols.size(); i++)
	{
		const auto& internalSymbol = internalSymbols[i];
		auto& internalSymbolInfo = internalSymbolInfos[i];
		internalSymbolInfo.nameOffset = static_cast<uint32>(stringTable.size());
		stringTable.insert(std::end(stringTable), std::begin(internalSymbol.name), std::end(internalSymbol.name));
		stringTable.push_back(0);
	}
}

template <typename MachoTraits>
void CMachoObjectFile<MachoTraits>::FillStringTable(StringTable& stringTable, const ExternalSymbolArray& externalSymbols, ExternalSymbolInfoArray& externalSymbolInfos)
{
	uint32 stringTableSizeIncrement = 0;
	for(const auto& externalSymbol : externalSymbols)
	{
		stringTableSizeIncrement += static_cast<uint32>(externalSymbol.name.length()) + 1;
	}
	stringTable.reserve(stringTable.size() + stringTableSizeIncrement);
	for(uint32 i = 0; i < externalSymbols.size(); i++)
	{
		const auto& externalSymbol = externalSymbols[i];
		auto& externalSymbolInfo = externalSymbolInfos[i];
		externalSymbolInfo.nameOffset = static_cast<uint32>(stringTable.size());
		stringTable.insert(std::end(stringTable), std::begin(externalSymbol.name), std::end(externalSymbol.name));
		stringTable.push_back(0);
	}
}

template <typename MachoTraits>
typename CMachoObjectFile<MachoTraits>::SECTION CMachoObjectFile<MachoTraits>::BuildSection(const InternalSymbolArray& internalSymbols, InternalSymbolInfoArray& internalSymbolInfos, INTERNAL_SYMBOL_LOCATION location)
{
	SECTION section;
	auto& sectionData(section.data);
	uint32 sectionSize = 0;
	for(const auto& internalSymbol : internalSymbols)
	{
		sectionSize += static_cast<uint32>(internalSymbol.data.size());
	}
	sectionData.reserve(sectionSize);
	for(uint32 i = 0; i < internalSymbols.size(); i++)
	{
		const auto& internalSymbol = internalSymbols[i];
		if(internalSymbol.location != location) continue;

		auto& internalSymbolInfo = internalSymbolInfos[i];
		internalSymbolInfo.dataOffset = static_cast<uint32>(sectionData.size());
		for(const auto& symbolReference : internalSymbol.symbolReferences)
		{
			SYMBOL_REFERENCE newReference;
			newReference.offset			= symbolReference.offset + internalSymbolInfo.dataOffset;
			newReference.symbolIndex	= symbolReference.symbolIndex;
			newReference.type			= symbolReference.type;
			section.symbolReferences.push_back(newReference);
		}
		sectionData.insert(std::end(sectionData), std::begin(internalSymbol.data), std::end(internalSymbol.data));
	}
	return section;
}

template <typename MachoTraits>
typename CMachoObjectFile<MachoTraits>::SymbolArray CMachoObjectFile<MachoTraits>::BuildSymbols(
	const InternalSymbolArray& internalSymbols, InternalSymbolInfoArray& internalSymbolInfos,
	const ExternalSymbolArray& externalSymbols, ExternalSymbolInfoArray& externalSymbolInfos,
	uint32 textSectionVa, uint32 dataSectionVa
	)
{
	SymbolArray symbols;
	symbols.reserve(internalSymbols.size() + externalSymbols.size());

	//Internal symbols
	for(uint32 i = 0; i < internalSymbols.size(); i++)
	{
		const auto& internalSymbol = internalSymbols[i];
		auto& internalSymbolInfo = internalSymbolInfos[i];
		internalSymbolInfo.symbolIndex = static_cast<uint32>(symbols.size());

		typename MachoTraits::NLIST symbol = {};
		symbol.stringTableIndex		 = internalSymbolInfo.nameOffset;
		symbol.type					 = 0x1F;		// N_SECT | N_PEXT | N_EXT
		symbol.value				 = internalSymbolInfo.dataOffset;
		symbol.section				 = (internalSymbol.location == CObjectFile::INTERNAL_SYMBOL_LOCATION_TEXT) ? 1 : 2;		//.text or .data section
		symbol.value				+= (internalSymbol.location == CObjectFile::INTERNAL_SYMBOL_LOCATION_TEXT) ? textSectionVa : dataSectionVa;
		symbol.desc					 = 0;
		symbols.push_back(symbol);
	}

	//External symbols
	for(uint32 i = 0; i < externalSymbols.size(); i++)
	{
		const auto& externalSymbol = externalSymbols[i];
		auto& externalSymbolInfo = externalSymbolInfos[i];
		externalSymbolInfo.symbolIndex = static_cast<uint32>(symbols.size());

		typename MachoTraits::NLIST symbol = {};
		symbol.stringTableIndex		= externalSymbolInfo.nameOffset;
		symbol.value				= 0;
		symbol.type					= 0x01;		// N_UNDF | N_EXT
		symbol.section				= 0;
		symbol.desc					= 0;
		symbols.push_back(symbol);
	}

	return symbols;
}

template <typename MachoTraits>
typename CMachoObjectFile<MachoTraits>::RelocationArray CMachoObjectFile<MachoTraits>::BuildRelocations(
	SECTION& section, const InternalSymbolInfoArray& internalSymbolInfos, 
	const ExternalSymbolInfoArray& externalSymbolInfos) const
{
	RelocationArray relocations;
	relocations.reserve(section.symbolReferences.size() * 4); //Times 4 because of possible HALF/PAIR relocs

	for(uint32 i = 0; i < section.symbolReferences.size(); i++)
	{
		const auto& symbolReference = section.symbolReferences[i];
		uint32 symbolIndex = (symbolReference.type == SYMBOL_TYPE_INTERNAL) ? 
			internalSymbolInfos[symbolReference.symbolIndex].symbolIndex :
			externalSymbolInfos[symbolReference.symbolIndex].symbolIndex;
		
		if((m_cpuArch == CObjectFile::CPU_ARCH_ARM) && (symbolReference.type == SYMBOL_TYPE_EXTERNAL))
		{
			//Dirty hack: We assume that all external symbols are coming from the OP_CALL emitter on ARM
			//which uses MOVW and MOVT

			//Length field for half relocs
			//Bit 0: 0=movw,1=movt; Bit 1: 0=arm,1=thumb

			{
				unsigned int symbolLength = 0;

				{
					auto relocation = Macho::RELOCATION_INFO();
					relocation.type			= Macho::ARM_RELOC_HALF;
					relocation.symbolIndex	= symbolIndex;
					relocation.address		= symbolReference.offset + 0;
					relocation.isExtern		= 1;
					relocation.pcRel		= 0;
					relocation.length		= symbolLength;
					relocations.push_back(relocation);
				}

				{
					auto relocation = Macho::RELOCATION_INFO();
					relocation.type			= Macho::ARM_RELOC_PAIR;
					relocation.symbolIndex	= 0xFFFFFF;
					relocation.address		= 0;
					relocation.isExtern		= 0;
					relocation.pcRel		= 0;
					relocation.length		= symbolLength;
					relocations.push_back(relocation);
				}
			}

			{
				unsigned int symbolLength = 1;

				{
					auto relocation = Macho::RELOCATION_INFO();
					relocation.type			= Macho::ARM_RELOC_HALF;
					relocation.symbolIndex	= symbolIndex;
					relocation.address		= symbolReference.offset + 4;
					relocation.isExtern		= 1;
					relocation.pcRel		= 0;
					relocation.length		= symbolLength;
					relocations.push_back(relocation);
				}

				{
					auto relocation = Macho::RELOCATION_INFO();
					relocation.type			= Macho::ARM_RELOC_PAIR;
					relocation.symbolIndex	= 0xFFFFFF;
					relocation.address		= 0;
					relocation.isExtern		= 0;
					relocation.pcRel		= 0;
					relocation.length		= symbolLength;
					relocations.push_back(relocation);
				}
			}

			*reinterpret_cast<uint32*>(section.data.data() + symbolReference.offset + 0) &= ~0xF0FFF;
			*reinterpret_cast<uint32*>(section.data.data() + symbolReference.offset + 4) &= ~0xF0FFF;
		}
		else if((m_cpuArch == CObjectFile::CPU_ARCH_ARM64) && (symbolReference.type == SYMBOL_TYPE_EXTERNAL))
		{
			//We assume that the instruction that references this symbol is BL.

			auto relocation = Macho::RELOCATION_INFO();
			relocation.type        = Macho::ARM64_RELOC_BRANCH26;
			relocation.symbolIndex = symbolIndex;
			relocation.address     = symbolReference.offset;
			relocation.isExtern    = 1;
			relocation.pcRel       = 1;
			relocation.length      = 2; //4 bytes
			relocations.push_back(relocation);

			*reinterpret_cast<uint32*>(section.data.data() + symbolReference.offset) &= ~0x03FFFFFF;
		}
		else
		{
			auto relocation = Macho::RELOCATION_INFO();
			relocation.type				= Macho::GENERIC_RELOC_VANILLA;
			relocation.symbolIndex		= symbolIndex;
			relocation.address			= symbolReference.offset;
			relocation.isExtern			= 1;
			relocation.pcRel			= 0;
			switch(MachoTraits::POINTER_SIZE)
			{
			case 4:
				relocation.length		= 2;		//4 bytes
				*reinterpret_cast<uint32*>(section.data.data() + symbolReference.offset) = 0;
				break;
			case 8:
				relocation.length		= 3;		//8 bytes
				*reinterpret_cast<uint64*>(section.data.data() + symbolReference.offset) = 0;
				break;
			default:
				assert(false);
				break;
			}
			relocations.push_back(relocation);
		}
	}

	return relocations;
}

template class Jitter::CMachoObjectFile<MACHO_TRAITS_32>;
template class Jitter::CMachoObjectFile<MACHO_TRAITS_64>;
