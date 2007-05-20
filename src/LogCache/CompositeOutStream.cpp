#include "StdAfx.h"
#include "CompositeOutStream.h"

// construction: nothing special to do

CCompositeOutStreamBase::CCompositeOutStreamBase ( CCacheFileOutBuffer* aBuffer
												 , SUB_STREAM_ID anID)
	: CHierachicalOutStreamBase (aBuffer, anID)
{
}
