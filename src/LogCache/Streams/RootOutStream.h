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
// CRootOutStream
//
//      The mother of all streams: represents the root of the
//      stream hierarchy but has no content of its own. It also
//      opens and closes the write buffer.
//
//      In contrast to all other stream types, there is no
//      factory to create an instance of this class.
//
///////////////////////////////////////////////////////////////

class CRootOutStream : public CHierachicalOutStreamBase
{
private:

    CCacheFileOutBuffer buffer;

    enum {ROOT_STREAM_ID = 0};

    // the root does not have local stream data

    virtual const unsigned char* GetStreamData() {return NULL;}
    virtual size_t GetStreamSize() {return 0;}

public:

    // construction / destruction: manage file buffer

    CRootOutStream (const TFileName& fileName);
    virtual ~CRootOutStream();

    // implement the rest of IHierarchicalOutStream

    virtual STREAM_TYPE_ID GetTypeID() const;
};
