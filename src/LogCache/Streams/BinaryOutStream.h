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
#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HierachicalOutStreamBase.h"
#include "auto_buffer.h"

///////////////////////////////////////////////////////////////
//
// CBinaryOutStreamBase
//
//		Base class for write streams that contain arbitrary
//		binary data. All data gets buffered.
//
//		If the stream grows to large (x64 only), it will
//		throw an exception upon AutoClose().
//
///////////////////////////////////////////////////////////////

class CBinaryOutStreamBase : public CHierachicalOutStreamBase
{
private:

	// size of the data chunk
	// (flush & compress upon overflow or Close())

	enum {CHUNK_SIZE = 128 * 1024};

	// data to write (may be NULL)

	auto_buffer<unsigned char> data;
	unsigned char* current;
	unsigned char* last;

	// buffer management

	void Flush() throw();

protected:

	// return the stream data

	virtual const unsigned char* GetStreamData();
	virtual size_t GetStreamSize();
	virtual void ReleaseStreamData();

	// add data to the stream

	void Add (const unsigned char* source, size_t byteCount) throw()
	{
		while (current + byteCount > last)
			Flush();

		memcpy (current, source, byteCount);
		current += byteCount;
	}

	void Add (unsigned char c) throw()
	{
		if (current == last)
			Flush();

		*current = c;
		++current;
	};

public:

	// construction / destruction: nothing special to do

	CBinaryOutStreamBase ( CCacheFileOutBuffer* aBuffer
					     , SUB_STREAM_ID anID);
	virtual ~CBinaryOutStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CBinaryOutStream
//
//		instantiable sub-class of CBinaryOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CBinaryOutStream : public COutStreamImplBase< CBinaryOutStream
												  , CBinaryOutStreamBase
					                              , BINARY_STREAM_TYPE_ID>
{
public:

	typedef COutStreamImplBase< CBinaryOutStream
							  , CBinaryOutStreamBase
							  , BINARY_STREAM_TYPE_ID> TBase;

	// construction / destruction: nothing special to do

	CBinaryOutStream ( CCacheFileOutBuffer* aBuffer
					 , SUB_STREAM_ID anID);
	virtual ~CBinaryOutStream() {};

	// public Add() methods

	using TBase::Add;
};
