#include "CoffObjectFile.h"
#include <string.h>
#include <assert.h>
#include <time.h>
#include <algorithm>

using namespace Jitter;

CCoffObjectFile::CCoffObjectFile(CPU_ARCH cpuArch)
: CObjectFile(cpuArch)
{

}

CCoffObjectFile::~CCoffObjectFile()
{

}

void CCoffObjectFile::Write(Framework::CStream& stream)
{
	struct OffsetKeeper
	{
		OffsetKeeper()
		{
			currentOffset = 0;
		}

		uint32 Advance(uint32 size)
		{
			currentOffset += size;
			return currentOffset;
		}

		uint32 currentOffset;
	};

	auto internalSymbolInfos = InternalSymbolInfoArray(m_internalSymbols.size());
	auto externalSymbolInfos = ExternalSymbolInfoArray(m_externalSymbols.size());

	StringTable stringTable;
	FillStringTable(stringTable, m_internalSymbols, internalSymbolInfos);
	FillStringTable(stringTable, m_externalSymbols, externalSymbolInfos);

	auto textSection = BuildSection(m_internalSymbols, internalSymbolInfos, INTERNAL_SYMBOL_LOCATION_TEXT);
	auto dataSection = BuildSection(m_internalSymbols, internalSymbolInfos, INTERNAL_SYMBOL_LOCATION_DATA);

	auto symbols = BuildSymbols(m_internalSymbols, internalSymbolInfos, 
		m_externalSymbols, externalSymbolInfos, textSection.data.size(), dataSection.data.size());

	unsigned int sectionHeaderCount = 2;

	OffsetKeeper offsetKeeper;
	uint32 textSectionDataOffset = offsetKeeper.Advance(sizeof(Coff::HEADER) + (sectionHeaderCount * sizeof(Coff::SECTION_HEADER)));
	uint32 textSectionRelocOffset = offsetKeeper.Advance(textSection.data.size());
	uint32 dataSectionDataOffset = offsetKeeper.Advance((textSection.symbolReferences.size() * sizeof(Coff::RELOCATION)));
	uint32 dataSectionRelocOffset = offsetKeeper.Advance(dataSection.data.size());
	uint32 symbolTableOffset = offsetKeeper.Advance(dataSection.symbolReferences.size() * sizeof(Coff::RELOCATION));

	SectionHeaderArray sectionHeaders;

	{
		Coff::SECTION_HEADER sectionHeader = {};
		strncpy(sectionHeader.name, ".text", 8);
		sectionHeader.virtualSize			= 0;
		sectionHeader.virtualAddress		= 0;
		sectionHeader.sizeOfRawData			= textSection.data.size();
		sectionHeader.pointerToRawData		= textSectionDataOffset;
		sectionHeader.pointerToRelocations	= (textSection.symbolReferences.size() == 0) ? 0 : textSectionRelocOffset;
		sectionHeader.pointerToLineNumbers	= 0;
		sectionHeader.numberOfRelocations	= textSection.symbolReferences.size();
		sectionHeader.numberOfLineNumbers	= 0;
		sectionHeader.characteristics		= 0x60500020;
		sectionHeaders.push_back(sectionHeader);
	}

	{
		Coff::SECTION_HEADER sectionHeader = {};
		strncpy(sectionHeader.name, ".data", 8);
		sectionHeader.virtualSize			= 0;
		sectionHeader.virtualAddress		= 0;
		sectionHeader.sizeOfRawData			= dataSection.data.size();
		sectionHeader.pointerToRawData		= dataSectionDataOffset;
		sectionHeader.pointerToRelocations	= (dataSection.symbolReferences.size() == 0) ? 0 : dataSectionRelocOffset;
		sectionHeader.pointerToLineNumbers	= 0;
		sectionHeader.numberOfRelocations	= dataSection.symbolReferences.size();
		sectionHeader.numberOfLineNumbers	= 0;
		sectionHeader.characteristics		= 0xC0500040;
		sectionHeaders.push_back(sectionHeader);
	}

	auto textSectionRelocations = BuildRelocations(textSection, internalSymbolInfos, externalSymbolInfos);
	auto dataSectionRelocations = BuildRelocations(dataSection, internalSymbolInfos, externalSymbolInfos);

	Coff::HEADER header = {};
	header.machine				= Coff::MACHINE_TYPE_I386;
	header.numberOfSections		= sectionHeaderCount;
	header.timeDateStamp		= static_cast<uint32>(time(nullptr));
	header.pointerToSymbolTable	= symbolTableOffset;
	header.numberOfSymbols		= symbols.size();
	header.sizeOfOptionalHeader	= 0;
	header.characteristics		= 0;

	stream.Write(&header, sizeof(Coff::HEADER));
	stream.Write(sectionHeaders.data(), sectionHeaders.size() * sizeof(Coff::SECTION_HEADER));
	stream.Write(textSection.data.data(), textSection.data.size());
	stream.Write(textSectionRelocations.data(), textSectionRelocations.size() * sizeof(Coff::RELOCATION));
	stream.Write(dataSection.data.data(), dataSection.data.size());
	stream.Write(dataSectionRelocations.data(), dataSectionRelocations.size() * sizeof(Coff::RELOCATION));
	stream.Write(symbols.data(), symbols.size() * sizeof(Coff::SYMBOL));
	stream.Write32(stringTable.size() + 4);
	stream.Write(stringTable.data(), stringTable.size());
}

void CCoffObjectFile::FillStringTable(StringTable& stringTable, const InternalSymbolArray& internalSymbols, InternalSymbolInfoArray& internalSymbolInfos)
{
	uint32 stringTableSizeIncrement = 0;
	for(const auto& internalSymbol : internalSymbols)
	{
		stringTableSizeIncrement += internalSymbol.name.length() + 1;
	}
	stringTable.reserve(stringTable.size() + stringTableSizeIncrement);
	for(uint32 i = 0; i < internalSymbols.size(); i++)
	{
		const auto& internalSymbol = internalSymbols[i];
		auto& internalSymbolInfo = internalSymbolInfos[i];
		internalSymbolInfo.nameOffset = stringTable.size();
		stringTable.insert(std::end(stringTable), std::begin(internalSymbol.name), std::end(internalSymbol.name));
		stringTable.push_back(0);
	}
}

void CCoffObjectFile::FillStringTable(StringTable& stringTable, const ExternalSymbolArray& externalSymbols, ExternalSymbolInfoArray& externalSymbolInfos)
{
	uint32 stringTableSizeIncrement = 0;
	for(const auto& externalSymbol : externalSymbols)
	{
		stringTableSizeIncrement += externalSymbol.name.length() + 1;
	}
	stringTable.reserve(stringTable.size() + stringTableSizeIncrement);
	for(uint32 i = 0; i < externalSymbols.size(); i++)
	{
		const auto& externalSymbol = externalSymbols[i];
		auto& externalSymbolInfo = externalSymbolInfos[i];
		externalSymbolInfo.nameOffset = stringTable.size();
		stringTable.insert(std::end(stringTable), std::begin(externalSymbol.name), std::end(externalSymbol.name));
		stringTable.push_back(0);
	}
}

CCoffObjectFile::SECTION CCoffObjectFile::BuildSection(const InternalSymbolArray& internalSymbols, InternalSymbolInfoArray& internalSymbolInfos, INTERNAL_SYMBOL_LOCATION location)
{
	SECTION section;
	auto& sectionData(section.data);
	uint32 sectionSize = 0;
	for(const auto& internalSymbol : internalSymbols)
	{
		sectionSize += internalSymbol.data.size();
	}
	sectionData.reserve(sectionSize);
	for(uint32 i = 0; i < internalSymbols.size(); i++)
	{
		const auto& internalSymbol = internalSymbols[i];
		if(internalSymbol.location != location) continue;

		auto& internalSymbolInfo = internalSymbolInfos[i];
		internalSymbolInfo.dataOffset = sectionData.size();
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

CCoffObjectFile::SymbolArray CCoffObjectFile::BuildSymbols(const InternalSymbolArray& internalSymbols, InternalSymbolInfoArray& internalSymbolInfos, 
														   const ExternalSymbolArray& externalSymbols, ExternalSymbolInfoArray& externalSymbolInfos,
														   uint32 textSectionSize, uint32 dataSectionSize)
{
	SymbolArray symbols;
	symbols.reserve(5 + internalSymbols.size() + externalSymbols.size());

	//SafeSEH stuff
	{
		Coff::SYMBOL symbol = {};
		strncpy(symbol.name.shortName, "@feat.00", 8);
		symbol.value				= 0x01;
		symbol.sectionNumber		= 0xFFFF;
		symbol.type					= 0;
		symbol.storageClass			= 0x03;		//CLASS_STATIC
		symbol.numberOfAuxSymbols	= 0;
		symbols.push_back(symbol);
	}

	//Text Section
	{
		Coff::SYMBOL symbol = {};
		strncpy(symbol.name.shortName, ".text", 8);
		symbol.value				= 0;
		symbol.sectionNumber		= 1;
		symbol.type					= 0;
		symbol.storageClass			= 0x03;		//CLASS_STATIC
		symbol.numberOfAuxSymbols	= 1;
		symbols.push_back(symbol);
	}

	{
		Coff::SYMBOL symbol = {};
		symbol.name.offset			= 0;
		symbol.name.zeroes			= textSectionSize;
		symbols.push_back(symbol);
	}

	//Data Section
	{
		Coff::SYMBOL symbol = {};
		strncpy(symbol.name.shortName, ".data", 8);
		symbol.value				= 0;
		symbol.sectionNumber		= 2;
		symbol.type					= 0;
		symbol.storageClass			= 0x03;		//CLASS_STATIC
		symbol.numberOfAuxSymbols	= 1;
		symbols.push_back(symbol);
	}

	{
		Coff::SYMBOL symbol = {};
		symbol.name.offset			= 0;
		symbol.name.zeroes			= dataSectionSize;
		symbols.push_back(symbol);
	}

	//Internal symbols
	for(uint32 i = 0; i < internalSymbols.size(); i++)
	{
		const auto& internalSymbol = internalSymbols[i];
		auto& internalSymbolInfo = internalSymbolInfos[i];
		internalSymbolInfo.symbolIndex = symbols.size();

		Coff::SYMBOL symbol = {};
		symbol.name.offset			= 4 + internalSymbolInfo.nameOffset;
		symbol.value				= internalSymbolInfo.dataOffset;
		symbol.sectionNumber		= (internalSymbol.location == CObjectFile::INTERNAL_SYMBOL_LOCATION_TEXT) ? 1 : 2;		//.text or .data section
		symbol.type					= (internalSymbol.location == CObjectFile::INTERNAL_SYMBOL_LOCATION_TEXT) ? 0x20 : 0;	//DT_FUNCTION or nothing
		symbol.storageClass			= 0x02;		//CLASS_EXTERNAL
		symbol.numberOfAuxSymbols	= 0;
		symbols.push_back(symbol);
	}

	//External symbols
	for(uint32 i = 0; i < externalSymbols.size(); i++)
	{
		const auto& externalSymbol = externalSymbols[i];
		auto& externalSymbolInfo = externalSymbolInfos[i];
		externalSymbolInfo.symbolIndex = symbols.size();

		Coff::SYMBOL symbol = {};
		symbol.name.offset			= 4 + externalSymbolInfo.nameOffset;
		symbol.value				= 0;
		symbol.sectionNumber		= 0;
		symbol.type					= 0;
		symbol.storageClass			= 0x02;		//CLASS_EXTERNAL
		symbol.numberOfAuxSymbols	= 0;
		symbols.push_back(symbol);
	}

	return symbols;
}

CCoffObjectFile::RelocationArray CCoffObjectFile::BuildRelocations(SECTION& section, const InternalSymbolInfoArray& internalSymbolInfos, const ExternalSymbolInfoArray& externalSymbolInfos)
{
	RelocationArray relocations;
	relocations.resize(section.symbolReferences.size());

	for(uint32 i = 0; i < section.symbolReferences.size(); i++)
	{
		const auto& symbolReference = section.symbolReferences[i];
		uint32 symbolIndex = (symbolReference.type == SYMBOL_TYPE_INTERNAL) ? 
			internalSymbolInfos[symbolReference.symbolIndex].symbolIndex :
			externalSymbolInfos[symbolReference.symbolIndex].symbolIndex;

		auto& relocation = relocations[i];
		relocation.type				= 0x06;			//I386_DIR32
		relocation.symbolIndex		= symbolIndex;
		relocation.rva				= symbolReference.offset;

		*reinterpret_cast<uint32*>(section.data.data() + symbolReference.offset) = 0;
	}

	return relocations;
}
