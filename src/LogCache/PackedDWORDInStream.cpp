// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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

DWORD CPackedDWORDInStreamBase::InternalGetValue() throw()
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
