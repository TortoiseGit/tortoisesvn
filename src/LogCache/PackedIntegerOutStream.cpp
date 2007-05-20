#include "StdAfx.h"
#include "PackedIntegerOutStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedIntegerOutStreamBase
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CPackedIntegerOutStreamBase::CPackedIntegerOutStreamBase ( CCacheFileOutBuffer* aBuffer
													     , SUB_STREAM_ID anID)
	: CPackedDWORDOutStreamBase (aBuffer, anID)
{
}

///////////////////////////////////////////////////////////////
//
// CPackedIntegerOutStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CPackedIntegerOutStream::CPackedIntegerOutStream ( CCacheFileOutBuffer* aBuffer
											     , SUB_STREAM_ID anID)
	: TBase (aBuffer, anID)
{
}
