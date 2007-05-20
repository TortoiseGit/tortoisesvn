#include "StdAfx.h"
#include "PackedTime64OutStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedTime64OutStreamBase
//
///////////////////////////////////////////////////////////////

// add data to the stream

void CPackedTime64OutStreamBase::Add (__time64_t value)
{
	// store only diffs

	__time64_t temp = value;
	value -= lastValue;
	lastValue = temp;

	// extract the sign

	unsigned char head = 0;
	if (value < 0)
	{
		value = -value;
		head = 0x80;
	}

	// store bits 0 .. 3 in the header

	head += (unsigned char)value & 0xf;
	value >>= 4;

	// set buffer and determine size (min. 1)

	unsigned char buffer[9];
	*reinterpret_cast<__time64_t*>(&buffer[1]) = value;

	size_t size = 8;
	while ((buffer[size] == 0) && (size > 1))
		--size;

	// store size in header and preprend header to buffer

	head += (unsigned char)((size-1) * 0x10);
	buffer[0] = head;

	// write buffer to stream

	CBinaryOutStreamBase::Add (buffer, size+1);
}

// construction: nothing special to do

CPackedTime64OutStreamBase::CPackedTime64OutStreamBase ( CCacheFileOutBuffer* aBuffer
													   , SUB_STREAM_ID anID)
	: CBinaryOutStreamBase (aBuffer, anID)
	, lastValue (0)
{
}

///////////////////////////////////////////////////////////////
//
// CPackedTime64OutStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CPackedTime64OutStream::CPackedTime64OutStream ( CCacheFileOutBuffer* aBuffer
										       , SUB_STREAM_ID anID)
	: TBase (aBuffer, anID)
{
}
