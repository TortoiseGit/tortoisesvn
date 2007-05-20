#include "StdAfx.h"
#include "PackedIntegerInStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedIntegerInStreamBase
//
///////////////////////////////////////////////////////////////

// construction: nothing to do here

CPackedIntegerInStreamBase::CPackedIntegerInStreamBase (CCacheFileInBuffer* buffer
													   , STREAM_INDEX index)
	: CPackedDWORDInStreamBase (buffer, index)
{
}

///////////////////////////////////////////////////////////////
//
// CPackedIntegerInStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CPackedIntegerInStream::CPackedIntegerInStream ( CCacheFileInBuffer* buffer
											   , STREAM_INDEX index)
	: TBase (buffer, index)
{
}
