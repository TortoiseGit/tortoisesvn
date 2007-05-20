// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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
#include "BinaryOutStream.h"

///////////////////////////////////////////////////////////////
//
// CBinaryOutStreamBase
//
///////////////////////////////////////////////////////////////

// buffer management

void CBinaryOutStreamBase::Grow() throw()
{
	if (data.get() == NULL)
	{
		data.reset (new unsigned char[1024 * 1024]);
		current = data.get();
		last = current + 1024 * 1024;
	}
	else
	{
		size_t newSize = (last - data.get()) * 2;
		size_t currentPos = current - data.get();

		std::auto_ptr<unsigned char> newData (new unsigned char[newSize]);
		memcpy (newData.get(), data.get(), currentPos);

		data.reset (newData.release());

		current = data.get() + currentPos;
		last = data.get() + newSize;
	}
}

// write our data to the file

void CBinaryOutStreamBase::WriteThisStream (CCacheFileOutBuffer* buffer)
{
	size_t size = current - data.get();
	if (size > (DWORD)(-1))
		throw std::exception ("binary stream too large");

	if (size > 0)
		buffer->Add (data.get(), (DWORD)size);

	data.reset();
	current = NULL;
	last = NULL;
}

// construction: nothing special to do

CBinaryOutStreamBase::CBinaryOutStreamBase ( CCacheFileOutBuffer* aBuffer
					                       , SUB_STREAM_ID anID)
	: CHierachicalOutStreamBase (aBuffer, anID)
	, current (NULL)
	, last (NULL)
{
}

///////////////////////////////////////////////////////////////
//
// CBinaryOutStream
//
///////////////////////////////////////////////////////////////

// construction: nothing special to do

CBinaryOutStream::CBinaryOutStream ( CCacheFileOutBuffer* aBuffer
								   , SUB_STREAM_ID anID)
	: TBase (aBuffer, anID)
{
}
