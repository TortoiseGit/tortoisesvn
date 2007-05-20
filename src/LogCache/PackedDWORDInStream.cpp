#include "StdAfx.h"
#include "PackedDWORDInStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedDWORDInStreamBase
//
///////////////////////////////////////////////////////////////

// construction: nothing to do here

CPackedDWORDInStreamBase::CPackedDWORDInStreamBase (CCacheFileInBuffer* buffer
												   , STREAM_INDEX index)
	: CBinaryInStreamBase (buffer, index)
	, lastValue (0)
	, count (0)
{
}

// data access

DWORD CPackedDWORDInStreamBase::InternalGetValue()
{
	DWORD result = GetByte();
	if (result < 0x80)
		return result;

	result -= 0x80;

	char shift = 7;
	while (true)
	{
		DWORD c = GetByte();
		if (c < 0x80)
			return result + (c << shift);

		result += ((c - 0x80) << shift);
		shift += 7;
	}
}

///////////////////////////////////////////////////////////////
//
// CPackedDWORDInStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CPackedDWORDInStream::CPackedDWORDInStream ( CCacheFileInBuffer* buffer
										   , STREAM_INDEX index)
	: TBase (buffer, index)
{
}
