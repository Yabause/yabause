#pragma once

#include "ObjectFile.h"
#include "MachoDefs.h"

namespace Jitter
{
	struct MACHO_TRAITS_32
	{
		typedef Macho::MACH_HEADER        MACH_HEADER;
		typedef Macho::SEGMENT_COMMAND    SEGMENT_COMMAND;
		typedef Macho::SECTION            SECTION;
		typedef Macho::NLIST              NLIST;

		static const uint32 HEADER_MAGIC    = Macho::MH_MAGIC;
		static const uint32 LC_SEGMENT      = Macho::LC_SEGMENT;
		static const uint32 POINTER_SIZE    = 4;
	};

	struct MACHO_TRAITS_64
	{
		typedef Macho::MACH_HEADER_64        MACH_HEADER;
		typedef Macho::SEGMENT_COMMAND_64    SEGMENT_COMMAND;
		typedef Macho::SECTION_64            SECTION;
		typedef Macho::NLIST_64              NLIST;

		static const uint32 HEADER_MAGIC    = Macho::MH_MAGIC_64;
		static const uint32 LC_SEGMENT      = Macho::LC_SEGMENT_64;
		static const uint32 POINTER_SIZE    = 8;
	};

	template <typename MachoTraits>
	class CMachoObjectFile : public CObjectFile
	{
	public:
						CMachoObjectFile(CPU_ARCH);
		virtual			~CMachoObjectFile();

		virtual void	Write(Framework::CStream&) override;

	private:
		typedef std::vector<char> StringTable;
		typedef std::vector<uint8> SectionData;
		typedef std::vector<typename MachoTraits::SECTION> SectionArray;
		typedef std::vector<typename MachoTraits::NLIST> SymbolArray;
		typedef std::vector<Macho::RELOCATION_INFO> RelocationArray;

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

		struct SECTION
		{
			SectionData				data;
			SymbolReferenceArray	symbolReferences;
		};

		static void				FillStringTable(StringTable&, const InternalSymbolArray&, InternalSymbolInfoArray&);
		static void				FillStringTable(StringTable&, const ExternalSymbolArray&, ExternalSymbolInfoArray&);
		static SECTION			BuildSection(const InternalSymbolArray&, InternalSymbolInfoArray&, INTERNAL_SYMBOL_LOCATION);
		static SymbolArray		BuildSymbols(const InternalSymbolArray&, InternalSymbolInfoArray&, const ExternalSymbolArray&, ExternalSymbolInfoArray&, uint32, uint32);
		RelocationArray			BuildRelocations(SECTION&, const InternalSymbolInfoArray&, const ExternalSymbolInfoArray&) const;
	};

	typedef CMachoObjectFile<MACHO_TRAITS_32> CMachoObjectFile32;
	typedef CMachoObjectFile<MACHO_TRAITS_64> CMachoObjectFile64;
}
