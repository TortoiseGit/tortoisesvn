#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HierachicalInStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CBLOBInStreamBase
//
//		Base class for all read streams containing a single
//		BLOB.
//
///////////////////////////////////////////////////////////////

class CBLOBInStreamBase : public CHierachicalInStreamBase
{
protected:

	// not ment to be instantiated

	// construction: nothing to do here

	CBLOBInStreamBase ( CCacheFileInBuffer* buffer
					  , STREAM_INDEX index);

public:

	// destruction

	virtual ~CBLOBInStreamBase() {};

	// data access

	const unsigned char* GetData() const
	{
		return first;
	}

	size_t GetSize() const
	{
		return last - first;
	}
};

///////////////////////////////////////////////////////////////
//
// CBLOBInStream
//
//		Instantiable base class of CBLOBInStreamBase.
//
///////////////////////////////////////////////////////////////

template CInStreamImpl< CBLOBInStreamBase
					  , BLOB_STREAM_TYPE_ID>;
typedef CInStreamImpl< CBLOBInStreamBase
					 , BLOB_STREAM_TYPE_ID> CBLOBInStream;
