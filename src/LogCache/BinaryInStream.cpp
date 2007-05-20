#include "StdAfx.h"
#include "BinaryInStream.h"

///////////////////////////////////////////////////////////////
//
// CBinaryInStreamBase
//
///////////////////////////////////////////////////////////////

// construction: nothing to do here

CBinaryInStreamBase::CBinaryInStreamBase ( CCacheFileInBuffer* buffer
									     , STREAM_INDEX index)
	: CHierachicalInStreamBase (buffer, index)
	, current (first)
{
}

///////////////////////////////////////////////////////////////
//
// CBinaryInStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CBinaryInStream::CBinaryInStream ( CCacheFileInBuffer* buffer
							     , STREAM_INDEX index)
	: TBase (buffer, index)
{
}
