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

#include "HierachicalInStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CBinaryInStreamBase
//
//		Base class for all binary read streams. Data can
//		be extracted in single bytes or chunks of data.
//		Attempts to read beyond the stream's end return NULL
//		values.
//
///////////////////////////////////////////////////////////////

class CBinaryInStreamBase : public CHierachicalInStreamBase
{
private:

	// our current read position

	const unsigned char* current;

protected:

	// not meant to be instantiated

	// construction: nothing to do here

	CBinaryInStreamBase ( CCacheFileInBuffer* buffer
					    , STREAM_INDEX index);

	// data access

	size_t GetSize() const
	{
		return last - first;
	}

	size_t GetRemaining() const
	{
		return last - current;
	}

	unsigned char GetByte() throw()
	{
		return *(current++);
	}

	const unsigned char* GetData (size_t size)
	{
		if (GetRemaining() < size)
			return NULL;

		current += size;

		return current;
	}

public:

	// destruction

	virtual ~CBinaryInStreamBase() {};

	// data access

	virtual void Reset()
	{
		current = first;
	}

	bool EndOfStream() const
	{
		return current == last;
	}

    // update members in this derived class as well

    virtual void AutoOpen();
    virtual void AutoClose();
};

///////////////////////////////////////////////////////////////
//
// CBinaryInStream
//
//		instantiable sub-class of CBinaryInStreamBase.
//
///////////////////////////////////////////////////////////////

class CBinaryInStream : public CInStreamImplBase< CBinaryInStream
												, CBinaryInStreamBase
						                        , BINARY_STREAM_TYPE_ID>
{
public:

	typedef CInStreamImplBase< CBinaryInStream
							 , CBinaryInStreamBase
							 , BINARY_STREAM_TYPE_ID> TBase;

	// construction / destruction: nothing special to do

	CBinaryInStream ( CCacheFileInBuffer* buffer
				    , STREAM_INDEX index);
	virtual ~CBinaryInStream() {};

	// public data access methods

	using TBase::GetSize;
	using TBase::GetRemaining;
	using TBase::GetByte;
	using TBase::GetData;
};
