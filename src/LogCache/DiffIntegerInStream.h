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

#include "PackedIntegerInStream.h"

///////////////////////////////////////////////////////////////
//
// CDiffInStreamBase<>
//
//		enhances CPackedIntegerInStreamBase as well as
//		CPackedDWORDInStreamBase to store values differentially.
//
///////////////////////////////////////////////////////////////

template<class Base, class V>
class CDiffInStreamBase : public Base
{
private:

	// our current diff base

	V lastValue;

protected:

	// not meant to be instantiated

	// construction: nothing to do here

	CDiffInStreamBase ( CCacheFileInBuffer* buffer
					  , STREAM_INDEX index)
		: Base (buffer, index)
		, lastValue (0)
	{
	}

	// data access

	V GetValue() throw()
	{
		lastValue += Base::GetValue();
		return lastValue;
	}

public:

	// destruction

	virtual ~CDiffInStreamBase() {};

	// extend Reset() to reset lastValue properly

	virtual void Reset()
	{
		Base::Reset();
		lastValue = 0;
	}
};

///////////////////////////////////////////////////////////////
// the two template instances
///////////////////////////////////////////////////////////////

typedef CDiffInStreamBase< CPackedDWORDInStreamBase
						 , DWORD> CDiffDWORDInStreamBase;
typedef CDiffInStreamBase< CPackedIntegerInStreamBase
						 , int> CDiffIntegerInStreamBase;

///////////////////////////////////////////////////////////////
//
// CDiffDWORDInStream
//
//		instantiable sub-class of CDiffDWORDInStreamBase.
//
///////////////////////////////////////////////////////////////

class CDiffDWORDInStream 
	: public CInStreamImplBase< CDiffDWORDInStream
							  , CDiffDWORDInStreamBase
							  , DIFF_DWORD_STREAM_TYPE_ID>
{
public:

	typedef CInStreamImplBase< CDiffDWORDInStream
							 , CDiffDWORDInStreamBase
							 , DIFF_DWORD_STREAM_TYPE_ID> TBase;

	typedef int value_type;

	// construction / destruction: nothing special to do

	CDiffDWORDInStream ( CCacheFileInBuffer* buffer
					   , STREAM_INDEX index);
	virtual ~CDiffDWORDInStream() {};

	// public data access methods

	using TBase::GetValue;
};

template CInStreamImplBase< CDiffDWORDInStream
						  , CDiffDWORDInStreamBase
						  , DIFF_DWORD_STREAM_TYPE_ID>;

///////////////////////////////////////////////////////////////
//
// CDiffIntegerInStream
//
//		instantiable sub-class of CDiffIntegerInStreamBase.
//
///////////////////////////////////////////////////////////////

class CDiffIntegerInStream 
	: public CInStreamImplBase< CDiffIntegerInStream
							  , CDiffIntegerInStreamBase
							  , DIFF_INTEGER_STREAM_TYPE_ID>
{
public:

	typedef CInStreamImplBase< CDiffIntegerInStream
							 , CDiffIntegerInStreamBase
							 , DIFF_INTEGER_STREAM_TYPE_ID> TBase;

	typedef int value_type;

	// construction / destruction: nothing special to do

	CDiffIntegerInStream ( CCacheFileInBuffer* buffer
					     , STREAM_INDEX index);
	virtual ~CDiffIntegerInStream() {};

	// public data access methods

	using TBase::GetValue;
};

template CInStreamImplBase< CDiffIntegerInStream
						  , CDiffIntegerInStreamBase
						  , DIFF_INTEGER_STREAM_TYPE_ID>;

