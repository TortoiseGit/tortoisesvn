#include "StdAfx.h"
#include "CompositeInStream.h"

// construction: nothing to do here

CCompositeInStreamBase::CCompositeInStreamBase ( CCacheFileInBuffer* buffer
											   , STREAM_INDEX index)
	: CHierachicalInStreamBase (buffer, index)
{
}
