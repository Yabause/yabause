#include "Stream.h"
#include "alloca_def.h"

using namespace Framework;

CStream::~CStream()
{

}

void CStream::Flush()
{

}

uint8 CStream::Read8()
{
	uint8 value = 0;
	Read(&value, 1);
	return value;
}

uint16 CStream::Read16()
{
	uint16 value = 0;
	Read(&value, 2);
	return value;
}

uint32 CStream::Read32()
{
	uint32 value = 0;
	Read(&value, 4);
	return value;
}

uint64 CStream::Read64()
{
	uint64 value = 0;
	Read(&value, 8);
	return value;
}

uint16 CStream::Read16_MSBF()
{
	uint16 value = 0;
	Read(&value, 2);
	value = 
		(((value & 0xFF00) >>  8) <<  0) |
		(((value & 0x00FF) >>  0) <<  8);
	return value;
}

uint32 CStream::Read32_MSBF()
{
	uint32 value = 0;
	Read(&value, 4);
	value = 
		(((value & 0xFF000000) >> 24) <<  0) |
		(((value & 0x00FF0000) >> 16) <<  8) |
		(((value & 0x0000FF00) >>  8) << 16) |
		(((value & 0x000000FF) >>  0) << 24);
	return value;
}

float CStream::ReadFloat32()
{
	float value = 0;
	Read(&value, sizeof(float));
	return value;
}

std::string CStream::ReadString()
{
	std::string result;
	while(1)
	{
		char next = Read8();
		if(IsEOF()) break;
		if(next == 0) break;
		result += next;
	}
	return result;
}

std::string CStream::ReadString(size_t length)
{
	if(length == 0) return std::string();
	char* stringBuffer = reinterpret_cast<char*>(alloca(length));
	Read(stringBuffer, length);
	return std::string(stringBuffer, stringBuffer + length);
}

void CStream::Write8(uint8 nValue)
{
	Write(&nValue, 1);
}

void CStream::Write16(uint16 nValue)
{
	Write(&nValue, 2);
}

void CStream::Write32(uint32 nValue)
{
	Write(&nValue, 4);
}

void CStream::Write64(uint64 value)
{
	Write(&value, 8);
}

uint64 CStream::GetLength()
{
	uint64 position = Tell();
	Seek(0, STREAM_SEEK_END);
	uint64 size = Tell();
	Seek(position, STREAM_SEEK_SET);
	return size;
}

uint64 CStream::GetRemainingLength()
{
	uint64 position = Tell();
	Seek(0, STREAM_SEEK_END);
	uint64 size = Tell();
	Seek(position, STREAM_SEEK_SET);
	return size - position;
}
