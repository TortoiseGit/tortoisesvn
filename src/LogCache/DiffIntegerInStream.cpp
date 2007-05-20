#include "StdAfx.h"
#include "DiffIntegerInStream.h"

///////////////////////////////////////////////////////////////
//
// CDiffDWORDInStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CDiffDWORDInStream::CDiffDWORDInStream ( CCacheFileInBuffer* buffer
									   , STREAM_INDEX index)
	: TBase (buffer, index)
{
}

///////////////////////////////////////////////////////////////
//
// CDiffIntegerInStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CDiffIntegerInStream::CDiffIntegerInStream ( CCacheFileInBuffer* buffer
										   , STREAM_INDEX index)
	: TBase (buffer, index)
{
}
