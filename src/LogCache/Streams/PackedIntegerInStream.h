// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2015 - TortoiseSVN

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

#include "PackedDWORDInStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedIntegerInStreamBase
//
//      enhances CPackedDWORDInStreamBase to handle signed values.
//
///////////////////////////////////////////////////////////////

class CPackedIntegerInStreamBase : public CPackedDWORDInStreamBase
{
protected:

    // not meant to be instantiated

    // construction: nothing to do here

    CPackedIntegerInStreamBase ( CCacheFileInBuffer* buffer
                               , STREAM_INDEX index);

    // data access

    int GetValue() throw()
    {
        DWORD v = CPackedDWORDInStreamBase::GetValue();
        return (v & 1)
            ? -(int)(v / 2)
            : (int)(v / 2);
    }

public:

    // destruction

    virtual ~CPackedIntegerInStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CPackedIntegerInStream
//
//      instantiable sub-class of CPackedIntegerInStreamBase.
//
///////////////////////////////////////////////////////////////

class CPackedIntegerInStream
    : public CInStreamImplBase< CPackedIntegerInStream
                              , CPackedIntegerInStreamBase
                              , PACKED_INTEGER_STREAM_TYPE_ID>
{
public:

    typedef CInStreamImplBase< CPackedIntegerInStream
                             , CPackedIntegerInStreamBase
                             , PACKED_INTEGER_STREAM_TYPE_ID> TBase;

    typedef int value_type;

    // construction / destruction: nothing special to do

    CPackedIntegerInStream ( CCacheFileInBuffer* buffer
                           , STREAM_INDEX index);
    virtual ~CPackedIntegerInStream() {};

    // public data access methods

    using TBase::GetValue;
};
