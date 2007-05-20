#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HierachicalOutStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CCompositeOutStreamBase
//
//		Base class for write streams without local stream content.
//
///////////////////////////////////////////////////////////////

class CCompositeOutStreamBase : public CHierachicalOutStreamBase
{
private:

	// this stream does not have any local stream data

	virtual void WriteThisStream (CCacheFileOutBuffer* buffer) {};

public:

	// construction / destruction: nothing special to do

	CCompositeOutStreamBase ( CCacheFileOutBuffer* aBuffer
							, SUB_STREAM_ID anID);
	virtual ~CCompositeOutStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CCompositeOutStream
//
//		instantiable sub-class of CCompositeOutStreamBase.
//
///////////////////////////////////////////////////////////////

template COutStreamImpl< CCompositeOutStreamBase
					   , COMPOSITE_STREAM_TYPE_ID>;
typedef COutStreamImpl< CCompositeOutStreamBase
					  , COMPOSITE_STREAM_TYPE_ID> CCompositeOutStream;
