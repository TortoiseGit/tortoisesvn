#include "StdAfx.h"
#include "BLOBInStream.h"

// construction: nothing to do here

CBLOBInStreamBase::CBLOBInStreamBase ( CCacheFileInBuffer* buffer
								     , STREAM_INDEX index)
	: CHierachicalInStreamBase (buffer, index)
{
}
