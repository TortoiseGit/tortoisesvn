#include "StdAfx.h"
#include "PackedDWORDOutStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedDWORDOutStreamBase
//
///////////////////////////////////////////////////////////////

// add data to the stream

void CPackedDWORDOutStreamBase::InternalAdd (DWORD value)
{
	while (true)
	{
		if (value < 0x80)
		{
			CBinaryOutStreamBase::Add ((unsigned char)value);
			return;
		}
		
		CBinaryOutStreamBase::Add ((unsigned char)(value & 0x7f) + 0x80);
		value >>= 7;
	}
}

void CPackedDWORDOutStreamBase::FlushLastValue()
{
	if (count > 0)
	{
		InternalAdd (0);
		InternalAdd (count);
		InternalAdd (lastValue);
	}
}

// write our data to the file

void CPackedDWORDOutStreamBase::WriteThisStream (CCacheFileOutBuffer* buffer)
{
	FlushLastValue();
	CBinaryOutStreamBase::WriteThisStream (buffer);
}

// construction: nothing special to do

CPackedDWORDOutStreamBase::CPackedDWORDOutStreamBase ( CCacheFileOutBuffer* aBuffer
													 , SUB_STREAM_ID anID)
	: CBinaryOutStreamBase (aBuffer, anID)
	, lastValue (0)
	, count (0)
{
}

///////////////////////////////////////////////////////////////
//
// CPackedDWORDOutStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CPackedDWORDOutStream::CPackedDWORDOutStream ( CCacheFileOutBuffer* aBuffer
										     , SUB_STREAM_ID anID)
	: TBase (aBuffer, anID)
{
}
