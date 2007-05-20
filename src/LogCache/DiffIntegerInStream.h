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

	// not ment to be instantiated

	// construction: nothing to do here

	CDiffInStreamBase ( CCacheFileInBuffer* buffer
					  , STREAM_INDEX index)
		: Base (buffer, index)
		, lastValue (0)
	{
	}

	// data access

	V GetValue()
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

