#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "PackedDWORDInStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedIntegerInStreamBase
//
//		enhances CPackedDWORDInStreamBase to handle singed values.
//
///////////////////////////////////////////////////////////////

class CPackedIntegerInStreamBase : public CPackedDWORDInStreamBase
{
protected:

	// not ment to be instantiated

	// construction: nothing to do here

	CPackedIntegerInStreamBase ( CCacheFileInBuffer* buffer
						       , STREAM_INDEX index);

	// data access

	int GetValue()
	{
		DWORD v = CPackedDWORDInStreamBase::GetValue();
		return v & 1
			? -(int)(v / 2)
			: (int)(v / 2);
	}

public:

	// destruction

	virtual ~CPackedIntegerInStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CPackedIntegerInStream
//
//		instantiable sub-class of CPackedIntegerInStreamBase.
//
///////////////////////////////////////////////////////////////

class CPackedIntegerInStream 
	: public CInStreamImplBase< CPackedIntegerInStream
							  , CPackedIntegerInStreamBase
							  , PACKED_INTEGER_STREAM_TYPE_ID>
{
public:

	typedef CInStreamImplBase< CPackedIntegerInStream
							 , CPackedIntegerInStreamBase
							 , PACKED_INTEGER_STREAM_TYPE_ID> TBase;

	typedef int value_type;

	// construction / destruction: nothing special to do

	CPackedIntegerInStream ( CCacheFileInBuffer* buffer
					       , STREAM_INDEX index);
	virtual ~CPackedIntegerInStream() {};

	// public data access methods

	using TBase::GetValue;
};
