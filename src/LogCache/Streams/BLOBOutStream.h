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
#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HierachicalOutStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CBLOBOutStreamBase
//
//		Base class for write streams that contain a single 
//		binary chunk of data.
//
//		Add() must be called at most once. Since it will close
//		the stream implicitly, there is no need to buffer the
//		data within the stream. It will be written directly
//		to the file buffer instead.
//
//		That means, all sub-streams will be written as well!
//
///////////////////////////////////////////////////////////////

class CBLOBOutStreamBase : public CHierachicalOutStreamBase
{
private:

	// data to write (may be NULL)

	const unsigned char* data;
	unsigned size;

	// return the (possible NULL) data we just got through Add()

	virtual const unsigned char* GetStreamData();
	virtual size_t GetStreamSize();

public:

	// construction / destruction: nothing special to do

	CBLOBOutStreamBase ( CCacheFileOutBuffer* aBuffer
					   , SUB_STREAM_ID anID);
	virtual ~CBLOBOutStreamBase() {};

	// write local stream data and close the stream

	void Add (const unsigned char* source, size_t byteCount);
};

///////////////////////////////////////////////////////////////
//
// CBLOBOutStream
//
//		instantiable sub-class of CBLOBOutStreamBase.
//
///////////////////////////////////////////////////////////////

template class COutStreamImpl< CBLOBOutStreamBase
                             , BLOB_STREAM_TYPE_ID>;
typedef COutStreamImpl< CBLOBOutStreamBase
                      , BLOB_STREAM_TYPE_ID> CBLOBOutStream;
