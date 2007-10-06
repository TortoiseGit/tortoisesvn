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

void CBinaryOutStreamBase::Flush() throw()
{
	WriteThisStream();
	current = data.get();
}

// write our data to the file

// return the stream data

const unsigned char* CBinaryOutStreamBase::GetStreamData() 
{
	return data.get();
}

size_t CBinaryOutStreamBase::GetStreamSize() 
{
	size_t size = current - data.get();
	if (size > (DWORD)(-1))
		throw std::exception ("binary stream too large");

	return size;
}

void CBinaryOutStreamBase::ReleaseStreamData()
{
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
	data.reset (new unsigned char[CHUNK_SIZE]);
	current = data.get();
	last = current + CHUNK_SIZE;
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
