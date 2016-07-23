#pragma once

#include "Types.h"

namespace Macho
{
	enum
	{
		MH_MAGIC    = 0xFEEDFACE,
		MH_MAGIC_64 = 0xFEEDFACF,
		MH_CIGAM    = 0xCEFAEDFE,
	};

	enum CPU_ARCH
	{
		CPU_ARCH_ABI64 = 0x01000000,
	};

	enum CPU_TYPE
	{
		CPU_TYPE_I386  = 0x07,
		CPU_TYPE_ARM   = 0x0C,
		CPU_TYPE_ARM64 = CPU_ARCH_ABI64 | CPU_TYPE_ARM,
	};

	enum CPU_SUBTYPE_I386
	{
		CPU_SUBTYPE_I386_ALL = 0x03,
	};

	enum CPU_SUBTYPE_ARM
	{
		CPU_SUBTYPE_ARM_V6  = 0x06,
		CPU_SUBTYPE_ARM_V7  = 0x09,
		CPU_SUBTYPE_ARM_V7S = 0x0B
	};

	enum CPU_SUBTYPE_ARM64
	{
		CPU_SUBTYPE_ARM64_ALL = 0,
	};

	enum FILE_TYPE
	{
		MH_OBJECT = 0x01,
	};

	enum HEADER_FLAGS
	{
		MH_SPLIT_SEGS  = 0x0020,
		MH_NOMULTIDEFS = 0x2000,
	};

	enum SECTION_FLAGS
	{
		S_ATTR_SOME_INSTRUCTIONS = 0x00000400,
		S_ATTR_PURE_INSTRUCTIONS = 0x80000000,
	};

	enum LOAD_COMMAND
	{
		LC_SEGMENT    = 0x0001,
		LC_SYMTAB     = 0x0002,
		LC_SEGMENT_64 = 0x0019
	};

	enum RELOC_TYPE
	{
		GENERIC_RELOC_VANILLA   = 0,
		GENERIC_RELOC_PAIR      = 1,

		ARM_RELOC_VANILLA       = GENERIC_RELOC_VANILLA,
		ARM_RELOC_PAIR          = GENERIC_RELOC_PAIR,
		ARM_RELOC_HALF          = 8,

		ARM64_RELOC_BRANCH26    = 2
	};

	struct MACH_HEADER
	{
		uint32    magic;
		uint32    cpuType;
		uint32    cpuSubType;
		uint32    fileType;
		uint32    commandCount;
		uint32    sizeofCommands;
		uint32    flags;
	};
	static_assert(sizeof(MACH_HEADER) == 0x1C, "Size of MACH_HEADER structure must be 28 bytes.");

	struct MACH_HEADER_64
	{
		uint32    magic;
		uint32    cpuType;
		uint32    cpuSubType;
		uint32    fileType;
		uint32    commandCount;
		uint32    sizeofCommands;
		uint32    flags;
		uint32    reserved;
	};
	static_assert(sizeof(MACH_HEADER_64) == 0x20, "Size of MACH_HEADER_64 structure must be 32 bytes.");

	struct COMMAND
	{
		uint32    cmd;
		uint32    cmdSize;
	};

	struct SEGMENT_COMMAND : public COMMAND
	{
		char      name[0x10];
		uint32    vmAddress;
		uint32    vmSize;
		uint32    fileOffset;
		uint32    fileSize;
		uint32    maxProtection;
		uint32    initProtection;
		uint32    sectionCount;
		uint32    flags;
	};
	static_assert(sizeof(SEGMENT_COMMAND) == 0x38, "Size of SEGMENT_COMMAND structure must be 48 bytes.");

	struct SEGMENT_COMMAND_64 : public COMMAND
	{
		char      name[0x10];
		uint64    vmAddress;
		uint64    vmSize;
		uint64    fileOffset;
		uint64    fileSize;
		uint32    maxProtection;
		uint32    initProtection;
		uint32    sectionCount;
		uint32    flags;
	};
	static_assert(sizeof(SEGMENT_COMMAND_64) == 0x48, "Size of SEGMENT_COMMAND_64 structure must be 72 bytes.");

	struct SECTION
	{
		char      sectionName[0x10];
		char      segmentName[0x10];
		uint32    address;
		uint32    size;
		uint32    offset;
		uint32    align;
		uint32    relocationOffset;
		uint32    relocationCount;
		uint32    flags;
		uint32    reserved1;
		uint32    reserved2;
	};
	static_assert(sizeof(SECTION) == 0x44, "Size of SECTION structure must be 68 bytes.");

	struct SECTION_64
	{
		char      sectionName[16];
		char      segmentName[16];
		uint64    address;
		uint64    size;
		uint32    offset;
		uint32    align;
		uint32    relocationOffset;
		uint32    relocationCount;
		uint32    flags;
		uint32    reserved1;
		uint32    reserved2;
		uint32    reserved3;
	};
	static_assert(sizeof(SECTION_64) == 0x50, "Size of SECTION_64 structure must be 80 bytes.");

	struct SYMTAB_COMMAND : public COMMAND
	{
		uint32    symbolsOffset;
		uint32    symbolCount;
		uint32    stringsOffset;
		uint32    stringsSize;
	};
	static_assert(sizeof(SYMTAB_COMMAND) == 0x18, "Size of SYMTAB_COMMAND structure must be 24 bytes.");

	struct NLIST
	{
		uint32    stringTableIndex;
		uint8     type;
		uint8     section;
		uint16    desc;
		uint32    value;
	};
	static_assert(sizeof(NLIST) == 0xC, "Size of NLIST structure must be 12 bytes.");

	struct NLIST_64
	{
		uint32    stringTableIndex;
		uint8     type;
		uint8     section;
		uint16    desc;
		uint64    value;
	};
	static_assert(sizeof(NLIST_64) == 0x10, "Size of NLIST_64 structure must be 16 bytes.");

	struct RELOCATION_INFO
	{
		uint32          address;
		unsigned int    symbolIndex : 24;
		unsigned int    pcRel       : 1;
		unsigned int    length      : 2;
		unsigned int    isExtern    : 1;
		unsigned int    type        : 4;
	};
	static_assert(sizeof(RELOCATION_INFO) == 0x08, "Size of RELOCATION_INFO structure must be 8 bytes.");
}
