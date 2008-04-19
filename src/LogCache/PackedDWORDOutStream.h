// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "BinaryOutStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedDWORDOutStreamBase
//
//		Base class for write streams that store DWORD-sized
//		data in a space efficient format. CBinaryOutStreamBase
//		is used as a base class for stream data handling.
//
//		DWORDs are stored as follows:
//
//		* a sequence of 1 .. 5 bytes in 7/8 codes
//		  (i.e. 7 data bits per byte)
//		* the highest bit is used to indicate whether this
//		  is the last byte of the value (0) or if more bytes
//		  follow (1)
//		* the data is stored in little-endian order
//		  (i.e the first byte contains bits 0..6, the next
//		   byte 7 .. 13 and so on)
//
//		However, the incoming data gets run-length encoded
//		before being written to the stream. This is because 
//		most integer streams have larger sections of constant
//		values (saves approx. 10% of total log cache size).
//
//		Encoding scheme:
//
//		* 0 indicates a packed value. It is followed by the
//		  the number of repetitions followed by the value itself
//		* an compressed value is stored as value+1
//		* initial lastValue is 0
//
///////////////////////////////////////////////////////////////

class CPackedDWORDOutStreamBase : public CBinaryOutStreamBase
{
protected:

	// differential storage:
	// the last value we received through Add() and the
	// how many times we already got that value in a row

	DWORD lastValue;
	DWORD count;

	// add data to the stream

	void InternalAdd (DWORD value) throw();
	void FlushLastValue() throw();

	// compression (RLE) support

	void Add (DWORD value) throw();

	virtual void FlushData();

public:

	// construction / destruction: nothing special to do

	CPackedDWORDOutStreamBase ( CCacheFileOutBuffer* aBuffer
						      , SUB_STREAM_ID anID);
	virtual ~CPackedDWORDOutStreamBase() {};

	// plain data access

	void AddSizeValue (size_t value);
};

///////////////////////////////////////////////////////////////
// add data to the stream
///////////////////////////////////////////////////////////////

inline void CPackedDWORDOutStreamBase::InternalAdd (DWORD value) throw()
{
	while (true)
	{
		if (value < 0x80)
		{
			CBinaryOutStreamBase::Add ((unsigned char)value);
			return;
		}
		
		CBinaryOutStreamBase::Add ((unsigned char)(value & 0x7f) + 0x80);
		value >>= 7;
	}
}

///////////////////////////////////////////////////////////////
// compress incoming data and write it to the stream
///////////////////////////////////////////////////////////////

inline void CPackedDWORDOutStreamBase::Add (DWORD value) throw()
{
	// that is the only value we cannot represent

	assert (value != (DWORD)-1);

	if (value == lastValue)
	{
		++count;
	}
	else
	{
		if (count == 1)
		{
			InternalAdd (lastValue+1);
		}
		else if ((count == 2) && (lastValue < 0x80))
		{
			InternalAdd (lastValue+1);
			InternalAdd (lastValue+1);
			count = 1;
		}
		else
		{
			FlushLastValue();
			count = 1;
		}

		lastValue = value;
	}
}

// plain data access

inline void CPackedDWORDOutStreamBase::AddSizeValue (size_t value)
{
	InternalAdd (static_cast<DWORD>(value));
}

///////////////////////////////////////////////////////////////
//
// CPackedDWORDOutStream
//
//		instantiable sub-class of CPackedDWORDOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CPackedDWORDOutStream 
	: public COutStreamImplBase< CPackedDWORDOutStream
							   , CPackedDWORDOutStreamBase
		                       , PACKED_DWORD_STREAM_TYPE_ID>
{
public:

	typedef COutStreamImplBase< CPackedDWORDOutStream
							  , CPackedDWORDOutStreamBase
							  , PACKED_DWORD_STREAM_TYPE_ID> TBase;

	typedef DWORD value_type;

	// construction / destruction: nothing special to do

	CPackedDWORDOutStream ( CCacheFileOutBuffer* aBuffer
						  , SUB_STREAM_ID anID);
	virtual ~CPackedDWORDOutStream() {};

	// public Add() methods

	using TBase::Add;
};

///////////////////////////////////////////////////////////////
//
// operator<< 
//
//		for CPackedDWORDOutStreamBase derived streams and vectors.
//
///////////////////////////////////////////////////////////////

template<class S, class V>
S& operator<< (S& stream, const std::vector<V>& data)
{
	// write total entry count and entries

	size_t count = data.size();
	stream.AddSizeValue (count);

	// efficiently add all entries
	// (don't use iterators here as they come with some index checking overhead)

	if (count > 0)
		for ( const V* iter = &data.at(0), *end = iter + count
			; iter != end
			; ++iter)
		{
			stream.Add ((typename S::value_type)(*iter));
		}


	// just close the stream 
	// (i.e. flush to disk and empty internal buffers)

	stream.AutoClose();

	return stream;
}
