#pragma once

#include "Stream.h"

namespace Framework
{

	class CMemStream : public CStream
	{
	public:
							CMemStream();
							CMemStream(const CMemStream&);
		virtual				~CMemStream();

		CMemStream&			operator =(const CMemStream&) = delete;

		uint64				Read(void*, uint64);
		uint64				Write(const void*, uint64);
		uint64				Tell();
		void				Seek(int64, STREAM_SEEK_DIRECTION);
		bool				IsEOF();
		void				ResetBuffer();
		void				Allocate(unsigned int);
		void				Truncate();
		uint8*				GetBuffer() const;
		unsigned int		GetSize() const;

	private:
		void				CopyFrom(const CMemStream&);

		unsigned int		m_size = 0;
		unsigned int		m_grow = 0;
		unsigned int		m_position = 0;
		uint8*				m_data = nullptr;
		bool				m_isEof = false;
	};

}
