// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009, 2013 - TortoiseSVN

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
#include "stdafx.h"
#include "HierachicalInStreamBase.h"
#include "HuffmanDecoder.h"
#include "StreamException.h"

void CHierachicalInStreamBase::ReadSubStreams ( CCacheFileInBuffer* buffer
                                              , STREAM_INDEX index)
{
    // get the raw stream data

    buffer->GetStreamBuffer (index, first, last);

    // get the number of sub-streams

    size_t size = last - first;
    if (sizeof (DWORD) > size)
        throw CStreamException ("stream too short");

    const DWORD *source = reinterpret_cast<const DWORD*>(first);
    DWORD subStreamCount = *source;
    size_t directorySize = (subStreamCount + 1) * sizeof (DWORD);
    if (directorySize > size)
        throw CStreamException ("stream too short for sub-stream list");

    // read the sub-streams

    for (DWORD i = 0; i < subStreamCount; ++i)
    {
        STREAM_INDEX subStreamIndex = *++source;
        SUB_STREAM_ID subStreamID = *++source;
        STREAM_TYPE_ID subStreamType = *++source;

        const IInStreamFactory* factory
            = CInStreamFactoryPool::GetInstance()->GetFactory (subStreamType);
        IHierarchicalInStream* subStream
            = factory->CreateStream (buffer, subStreamIndex);

        subStreams.insert (std::make_pair (subStreamID, subStream));
    }

    // adjust buffer pointer

    first += directorySize;
}

void CHierachicalInStreamBase::DecodeThisStream()
{
    // allocate a sufficiently large buffer to receive the decoded data

    packedLast = last;
    packedFirst = first;

    DWORD decodedSize = *(reinterpret_cast<const DWORD*>(last)-1);
    BYTE* target = new BYTE [decodedSize];

    // Huffman-compress the raw stream data

    CHuffmanDecoder decoder;
    const BYTE* source = first;
    first = target;
    last = target + decodedSize;

    while (target != last)
        decoder.Decode (source, target);
}

// construction / destruction

CHierachicalInStreamBase::CHierachicalInStreamBase()
    : first (NULL)
    , last (NULL)
    , packedFirst (NULL)
    , packedLast (NULL)
{
}

CHierachicalInStreamBase::CHierachicalInStreamBase ( CCacheFileInBuffer* buffer
                                                   , STREAM_INDEX index)
    : first (NULL)
    , last (NULL)
    , packedFirst (NULL)
    , packedLast (NULL)
{
    ReadSubStreams (buffer, index);
}

CHierachicalInStreamBase::~CHierachicalInStreamBase()
{
    for ( TSubStreams::iterator iter = subStreams.begin()
        , end = subStreams.end()
        ; iter != end
        ; ++iter)
        delete iter->second;

    AutoClose();
}

// implement IHierarchicalOutStream

void CHierachicalInStreamBase::AutoOpen()
{
    if (packedFirst == NULL)
        DecodeThisStream();
}

void CHierachicalInStreamBase::AutoClose()
{
    if (packedLast != NULL)
    {
        delete[] first;

        first = packedFirst;
        last = packedLast;

        packedFirst = NULL;
        packedLast = NULL;
    }
}

void CHierachicalInStreamBase::Prefetch()
{
    // no-op, if already open

    if (packedFirst == NULL)
    {
        // force all stream data to be read from disk

        // cppcheck-suppress unreadVariable
        static volatile BYTE dummy = 0;
        for (const BYTE* prefetch = first; prefetch < last; prefetch += 4096)
            dummy += *prefetch;
    }
}

bool
CHierachicalInStreamBase::HasSubStream (SUB_STREAM_ID subStreamID) const
{
    return subStreams.find (subStreamID) != subStreams.end();
}

IHierarchicalInStream*
CHierachicalInStreamBase::GetSubStream ( SUB_STREAM_ID subStreamID
                                       , bool autoOpen)
{
    TSubStreams::const_iterator iter = subStreams.find (subStreamID);
    if (iter == subStreams.end())
        throw CStreamException ("no such sub-stream");

    if (autoOpen)
        iter->second->AutoOpen();

    return iter->second;
}
