#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "BinaryInStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedTime64InStreamBase
//
//		Base class for all read streams containing packed 
//		integer data. See CPackedTime64OutStreamBase for datails 
//		of the storage format.
//
///////////////////////////////////////////////////////////////

class CPackedTime64InStreamBase : public CBinaryInStreamBase
{
protected:

	__time64_t lastValue;

	// not ment to be instantiated

	// construction: nothing to do here

	CPackedTime64InStreamBase ( CCacheFileInBuffer* buffer
						      , STREAM_INDEX index);

	// data access

	__time64_t GetValue();

public:

	// destruction

	virtual ~CPackedTime64InStreamBase() {};

	// plain data access

	size_t GetSizeValue();
};

// plain data access

inline size_t CPackedTime64InStreamBase::GetSizeValue()
{
	return static_cast<size_t>(GetValue());
}

///////////////////////////////////////////////////////////////
//
// CPackedTime64InStream
//
//		instantiable sub-class of CPackedTime64InStreamBase.
//
///////////////////////////////////////////////////////////////

class CPackedTime64InStream 
	: public CInStreamImplBase< CPackedTime64InStream
							  , CPackedTime64InStreamBase
							  , PACKED_TIME64_STREAM_TYPE_ID>
{
public:

	typedef CInStreamImplBase< CPackedTime64InStream
							 , CPackedTime64InStreamBase
							 , PACKED_TIME64_STREAM_TYPE_ID> TBase;

	typedef __time64_t value_type;

	// construction / destruction: nothing special to do

	CPackedTime64InStream ( CCacheFileInBuffer* buffer
					      , STREAM_INDEX index);
	virtual ~CPackedTime64InStream() {};

	// public data access methods

	using TBase::GetValue;
};
