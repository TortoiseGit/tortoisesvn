#include "StdAfx.h"
#include "PackedTime64InStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedTime64InStreamBase
//
///////////////////////////////////////////////////////////////

// construction: nothing to do here

CPackedTime64InStreamBase::CPackedTime64InStreamBase (CCacheFileInBuffer* buffer
												     , STREAM_INDEX index)
	: CBinaryInStreamBase (buffer, index)
	, lastValue (0)
{
}

// data access

__time64_t CPackedTime64InStreamBase::GetValue()
{
	// get header info

	unsigned char head = GetByte();

	// read the following data bytes into

	size_t count = ((head / 0x10) & 0x7) +1;

	unsigned char buffer[8];
	*reinterpret_cast<__time64_t*>(buffer) = 0;
	for (size_t i = 0; i < count; ++i)
		buffer[i] = GetByte();

	// now, the buffer contains bits 4 to 64 of the unsigned diff. value
	// the lower 4 bits of the header are value bits 0 .. 3

	__time64_t value = *reinterpret_cast<__time64_t*>(buffer) * 0x10 
					 + (head & 0xf);

	// add sign info

	if (head >= 0x80)
		value = -value;

	// get absolute value

	value += lastValue;
	lastValue = value;

	// ready

	return value;
}

///////////////////////////////////////////////////////////////
//
// CPackedTime64InStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CPackedTime64InStream::CPackedTime64InStream ( CCacheFileInBuffer* buffer
								  		     , STREAM_INDEX index)
	: TBase (buffer, index)
{
}
