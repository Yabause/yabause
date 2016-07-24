#pragma once

#include "ObjectFile.h"
#include "CoffDefs.h"
#include <vector>
#include <unordered_map>

namespace Jitter
{
	class CCoffObjectFile : public CObjectFile
	{
	public:
									CCoffObjectFile(CPU_ARCH);
		virtual						~CCoffObjectFile();

		void						Write(Framework::CStream&) override;

	private:
		typedef std::vector<Coff::SECTION_HEADER> SectionHeaderArray;
		typedef std::vector<Coff::RELOCATION> RelocationArray;
		typedef std::vector<Coff::SYMBOL> SymbolArray;

		struct INTERNAL_SYMBOL_INFO
		{
			INTERNAL_SYMBOL_INFO()
			{
				nameOffset = 0;
				dataOffset = 0;
				symbolIndex = 0;
			}

			uint32					nameOffset;
			uint32					dataOffset;
			uint32					symbolIndex;
		};
		typedef std::vector<INTERNAL_SYMBOL_INFO> InternalSymbolInfoArray;

		struct EXTERNAL_SYMBOL_INFO
		{
			EXTERNAL_SYMBOL_INFO()
			{
				nameOffset = 0;
				symbolIndex = 0;
			}

			uint32		nameOffset;
			uint32		symbolIndex;
		};
		typedef std::vector<EXTERNAL_SYMBOL_INFO> ExternalSymbolInfoArray;

		typedef std::vector<char> StringTable;
		typedef std::vector<uint8> SectionData;

		struct SECTION
		{
			SectionData				data;
			SymbolReferenceArray	symbolReferences;
		};

		static void					FillStringTable(StringTable&, const InternalSymbolArray&, InternalSymbolInfoArray&);
		static void					FillStringTable(StringTable&, const ExternalSymbolArray&, ExternalSymbolInfoArray&);
		static SECTION				BuildSection(const InternalSymbolArray&, InternalSymbolInfoArray&, INTERNAL_SYMBOL_LOCATION);
		static SymbolArray			BuildSymbols(const InternalSymbolArray&, InternalSymbolInfoArray&, const ExternalSymbolArray&, ExternalSymbolInfoArray&, uint32, uint32);
		static RelocationArray		BuildRelocations(SECTION&, const InternalSymbolInfoArray&, const ExternalSymbolInfoArray&);
	};
}
