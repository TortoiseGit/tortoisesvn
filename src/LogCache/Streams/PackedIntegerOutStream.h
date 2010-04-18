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

#include "PackedDWORDOutStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedIntegerOutStreamBase
//
//      enhances CPackedDWORDOutStreamBase to handle signed values.
//
//      To minimize the number of significant bits, the value
//      is folded as follows:
//
//      unsignedValue = abs (signedValue) * 2 + sign (signedValue)
//
//      Note, that -0x7fffffff and - 0x80000000 CANNOT be stored!
//
///////////////////////////////////////////////////////////////

class CPackedIntegerOutStreamBase : public CPackedDWORDOutStreamBase
{
protected:

    // add data to the stream

    void Add (int value) throw()
    {
        DWORD usignedValue = value >= 0
            ? 2* value
            : 2 * (-value) + 1;

        CPackedDWORDOutStreamBase::Add (usignedValue);
    }

public:

    // construction / destruction: nothing special to do

    CPackedIntegerOutStreamBase ( CCacheFileOutBuffer* aBuffer
                                , SUB_STREAM_ID anID);
    virtual ~CPackedIntegerOutStreamBase() {};
};

///////////////////////////////////////////////////////////////
//
// CPackedIntegerOutStream
//
//      instantiable sub-class of CPackedIntegerOutStreamBase.
//
///////////////////////////////////////////////////////////////

class CPackedIntegerOutStream
    : public COutStreamImplBase< CPackedIntegerOutStream
                               , CPackedIntegerOutStreamBase
                               , PACKED_INTEGER_STREAM_TYPE_ID>
{
public:

    typedef COutStreamImplBase< CPackedIntegerOutStream
                              , CPackedIntegerOutStreamBase
                              , PACKED_INTEGER_STREAM_TYPE_ID> TBase;

    typedef int value_type;

    // construction / destruction: nothing special to do

    CPackedIntegerOutStream ( CCacheFileOutBuffer* aBuffer
                            , SUB_STREAM_ID anID);
    virtual ~CPackedIntegerOutStream() {};

    // public Add() methods

    using TBase::Add;
};
