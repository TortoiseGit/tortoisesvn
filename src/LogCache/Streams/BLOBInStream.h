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
#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HierachicalInStreamBase.h"

///////////////////////////////////////////////////////////////
//
// CBLOBInStreamBase
//
//		Base class for all read streams containing a single
//		BLOB.
//
///////////////////////////////////////////////////////////////

class CBLOBInStreamBase : public CHierachicalInStreamBase
{
protected:

	// not meant to be instantiated

	// construction: nothing to do here

	CBLOBInStreamBase ( CCacheFileInBuffer* buffer
					  , STREAM_INDEX index);

public:

	// destruction

	virtual ~CBLOBInStreamBase() {};

	// data access

	const unsigned char* GetData() const
	{
		return first;
	}

	size_t GetSize() const
	{
		return last - first;
	}
};

///////////////////////////////////////////////////////////////
//
// CBLOBInStream
//
//		Instantiable base class of CBLOBInStreamBase.
//
///////////////////////////////////////////////////////////////

template class CInStreamImpl< CBLOBInStreamBase
                            , BLOB_STREAM_TYPE_ID>;
typedef CInStreamImpl< CBLOBInStreamBase
                     , BLOB_STREAM_TYPE_ID> CBLOBInStream;
