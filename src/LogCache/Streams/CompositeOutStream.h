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

///////////////////////////////////////////////////////////////
//
// CCompositeOutStreamBase
//
//		Base class for write streams without local stream content.
//
///////////////////////////////////////////////////////////////

class CCompositeOutStreamBase : public CHierachicalOutStreamBase
{
private:

	// this stream does not have any local stream data

	virtual const unsigned char* GetStreamData() {return NULL;}
	virtual size_t GetStreamSize() {return 0;}

public:

	// construction / destruction: nothing special to do

	CCompositeOutStreamBase ( CCacheFileOutBuffer* aBuffer
							, SUB_STREAM_ID anID);
	virtual ~CCompositeOutStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CCompositeOutStream
//
//		instantiable sub-class of CCompositeOutStreamBase.
//
///////////////////////////////////////////////////////////////

template COutStreamImpl< CCompositeOutStreamBase
					   , COMPOSITE_STREAM_TYPE_ID>;
typedef COutStreamImpl< CCompositeOutStreamBase
					  , COMPOSITE_STREAM_TYPE_ID> CCompositeOutStream;
