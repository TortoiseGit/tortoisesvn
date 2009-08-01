// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
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

__time64_t CPackedTime64InStreamBase::GetValue() throw()
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

// update members in this derived class as well

void CPackedTime64InStreamBase::AutoOpen()
{
    CBinaryInStreamBase::AutoOpen();
    lastValue = 0;
}

void CPackedTime64InStreamBase::AutoClose()
{
    lastValue = 0;
    CBinaryInStreamBase::AutoClose();
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
