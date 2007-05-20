#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HierachicalInStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CCompositeInStreamBase
//
//		Base class for all read streams with no local content.
//
///////////////////////////////////////////////////////////////

class CCompositeInStreamBase : public CHierachicalInStreamBase
{
protected:

	// not ment to be instantiated

	// construction: nothing to do here

	CCompositeInStreamBase ( CCacheFileInBuffer* buffer
						   , STREAM_INDEX index);

public:

	// destruction

	virtual ~CCompositeInStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CCompositeInStream
//
//		Instantiable base class of CCompositeInStreamBase.
//
///////////////////////////////////////////////////////////////

template CInStreamImpl< CCompositeInStreamBase
					  , COMPOSITE_STREAM_TYPE_ID>;
typedef CInStreamImpl< CCompositeInStreamBase
					 , COMPOSITE_STREAM_TYPE_ID> CCompositeInStream;

