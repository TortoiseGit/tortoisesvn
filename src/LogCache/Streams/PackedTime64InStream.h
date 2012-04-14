// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008, 2012 - TortoiseSVN

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

#include "BinaryInStream.h"

///////////////////////////////////////////////////////////////
//
// CPackedTime64InStreamBase
//
//      Base class for all read streams containing packed
//      integer data. See CPackedTime64OutStreamBase for details
//      of the storage format.
//
///////////////////////////////////////////////////////////////

class CPackedTime64InStreamBase : public CBinaryInStreamBase
{
protected:

    __time64_t lastValue;

    // not meant to be instantiated

    // construction: nothing to do here

    CPackedTime64InStreamBase ( CCacheFileInBuffer* buffer
                              , STREAM_INDEX index);

    // data access

    __time64_t GetValue() throw();

public:

    // destruction

    virtual ~CPackedTime64InStreamBase() {};

    // plain data access

    size_t GetSizeValue();

    // update members in this derived class as well

    virtual void AutoOpen() override;
    virtual void AutoClose() override;
};

// plain data access

inline size_t CPackedTime64InStreamBase::GetSizeValue()
{
    return static_cast<size_t>(GetValue());
}

///////////////////////////////////////////////////////////////
//
// CPackedTime64InStream
//
//      instantiable sub-class of CPackedTime64InStreamBase.
//
///////////////////////////////////////////////////////////////

class CPackedTime64InStream
    : public CInStreamImplBase< CPackedTime64InStream
                              , CPackedTime64InStreamBase
                              , PACKED_TIME64_STREAM_TYPE_ID>
{
public:

    typedef CInStreamImplBase< CPackedTime64InStream
                             , CPackedTime64InStreamBase
                             , PACKED_TIME64_STREAM_TYPE_ID> TBase;

    typedef __time64_t value_type;

    // construction / destruction: nothing special to do

    CPackedTime64InStream ( CCacheFileInBuffer* buffer
                          , STREAM_INDEX index);
    virtual ~CPackedTime64InStream() {};

    // public data access methods

    using TBase::GetValue;
};
