#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "PackedDWORDOutStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedIntegerOutStreamBase
//
//		enhances CPackedDWORDOutStreamBase to handle singed values.
//
//		To minimize the number of significant bits, the value
//		is folded as follows:
//
//		unsignedValue = abs (signedValue) * 2 + sign (signedValue)
//
///////////////////////////////////////////////////////////////

class CPackedIntegerOutStreamBase : public CPackedDWORDOutStreamBase
{
protected:

	// add data to the stream

	void Add (int value)
	{
		DWORD usignedValue = value >= 0
			? 2* value
			: 2 * (-value) + 1;

		CPackedDWORDOutStreamBase::Add (usignedValue);
	}

public:

	// construction / destruction: nothing special to do

	CPackedIntegerOutStreamBase ( CCacheFileOutBuffer* aBuffer
						        , SUB_STREAM_ID anID);
	virtual ~CPackedIntegerOutStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CPackedIntegerOutStream
//
//		instantiable sub-class of CPackedIntegerOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CPackedIntegerOutStream 
	: public COutStreamImplBase< CPackedIntegerOutStream
							   , CPackedIntegerOutStreamBase
		                       , PACKED_INTEGER_STREAM_TYPE_ID>
{
public:

	typedef COutStreamImplBase< CPackedIntegerOutStream
							  , CPackedIntegerOutStreamBase
							  , PACKED_INTEGER_STREAM_TYPE_ID> TBase;

	typedef int value_type;

	// construction / destruction: nothing special to do

	CPackedIntegerOutStream ( CCacheFileOutBuffer* aBuffer
						    , SUB_STREAM_ID anID);
	virtual ~CPackedIntegerOutStream() {};

	// public Add() methods

	using TBase::Add;
};
