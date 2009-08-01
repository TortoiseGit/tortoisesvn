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
	, current (NULL)
{
}

// update members in this derived class as well

void CBinaryInStreamBase::AutoOpen()
{
    CHierachicalInStreamBase::AutoOpen();
    current = first;
}

void CBinaryInStreamBase::AutoClose()
{
    current = NULL;
    CHierachicalInStreamBase::AutoClose();
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
