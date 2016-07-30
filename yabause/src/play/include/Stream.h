#pragma once

#include "Types.h"
#include <string>

namespace Framework
{

	enum STREAM_SEEK_DIRECTION
	{
		STREAM_SEEK_SET = 0,
		STREAM_SEEK_END = 1,
		STREAM_SEEK_CUR = 2,
	};

	class CStream
	{
	public:
		virtual			~CStream();
		virtual void	Seek(int64, STREAM_SEEK_DIRECTION) = 0;
		virtual uint64	Tell() = 0;
		virtual uint64	Read(void*, uint64) = 0;
		virtual uint64	Write(const void*, uint64) = 0;
		virtual bool	IsEOF() = 0;
		virtual void	Flush();
		virtual uint64	GetLength();
		virtual uint64	GetRemainingLength();

		uint8			Read8();
		uint16			Read16();
		uint32			Read32();
		uint64			Read64();

		uint16			Read16_MSBF();
		uint32			Read32_MSBF();

		float			ReadFloat32();

		std::string		ReadString();
		std::string		ReadString(size_t);

		void			Write8(uint8);
		void			Write16(uint16);
		void			Write32(uint32);
		void			Write64(uint64);
	};

}
