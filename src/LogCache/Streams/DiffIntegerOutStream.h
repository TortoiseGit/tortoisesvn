// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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

#include "PackedIntegerOutStream.h"

///////////////////////////////////////////////////////////////
//
// CDiffIntegerOutStreamBase
//
//		enhances CPackedIntegerOutStreamBase as well as
//		CPackedDWORDOutStreamBase to store the values 
//		differentially, i.e. the difference to the previous 
//		value is stored (in packed format). The base value 
//		for the first element is 0.
//
///////////////////////////////////////////////////////////////

template <class Base, class V>
class CDiffOutStreamBase : public Base
{
private:

	// last value written (i.e. our diff base)

	V lastValue;

protected:

	// add data to the stream

	void Add (V value) throw()
	{
		Base::Add (value - lastValue);
		lastValue = value;
	}

public:

	// construction / destruction: nothing special to do

	CDiffOutStreamBase ( CCacheFileOutBuffer* aBuffer
				       , SUB_STREAM_ID anID)
		: Base (aBuffer, anID)
		, lastValue (0)
	{
	}

	virtual ~CDiffOutStreamBase() {};
};

///////////////////////////////////////////////////////////////
// the two template instances
///////////////////////////////////////////////////////////////

typedef CDiffOutStreamBase< CPackedDWORDOutStreamBase
						  , DWORD> CDiffDWORDOutStreamBase;
typedef CDiffOutStreamBase< CPackedIntegerOutStreamBase
						  , int> CDiffIntegerOutStreamBase;

///////////////////////////////////////////////////////////////
//
// CDiffDWORDOutStream
//
//		instantiable sub-class of CDiffDWORDOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CDiffDWORDOutStream 
	: public COutStreamImplBase< CDiffDWORDOutStream
							   , CDiffDWORDOutStreamBase
		                       , DIFF_DWORD_STREAM_TYPE_ID>
{
public:

	typedef COutStreamImplBase< CDiffDWORDOutStream
							  , CDiffDWORDOutStreamBase
		                      , DIFF_DWORD_STREAM_TYPE_ID> TBase;

	typedef int value_type;

	// construction / destruction: nothing special to do

	CDiffDWORDOutStream ( CCacheFileOutBuffer* aBuffer
						, SUB_STREAM_ID anID);
	virtual ~CDiffDWORDOutStream() {};

	// public Add() methods

	using TBase::Add;
};

template COutStreamImplBase< CDiffDWORDOutStream
						   , CDiffDWORDOutStreamBase
	                       , DIFF_DWORD_STREAM_TYPE_ID>;

///////////////////////////////////////////////////////////////
//
// CDiffIntegerOutStream
//
//		instantiable sub-class of CDiffIntegerOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CDiffIntegerOutStream 
	: public COutStreamImplBase< CDiffIntegerOutStream
							   , CDiffIntegerOutStreamBase
		                       , DIFF_INTEGER_STREAM_TYPE_ID>
{
public:

	typedef COutStreamImplBase< CDiffIntegerOutStream
							  , CDiffIntegerOutStreamBase
		                      , DIFF_INTEGER_STREAM_TYPE_ID> TBase;

	typedef int value_type;

	// construction / destruction: nothing special to do

	CDiffIntegerOutStream ( CCacheFileOutBuffer* aBuffer
						  , SUB_STREAM_ID anID);
	virtual ~CDiffIntegerOutStream() {};

	// public Add() methods

	using TBase::Add;
};

template COutStreamImplBase< CDiffIntegerOutStream
						   , CDiffIntegerOutStreamBase
		                   , DIFF_INTEGER_STREAM_TYPE_ID>;
