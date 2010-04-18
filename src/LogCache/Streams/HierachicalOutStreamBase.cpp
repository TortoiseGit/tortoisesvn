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
#include "stdafx.h"
#include "HierachicalOutStreamBase.h"
#include "HuffmanEncoder.h"

// close (write) stream

void CHierachicalOutStreamBase::CloseLatestSubStream()
{
    if (!subStreams.empty())
        subStreams.back()->AutoClose();
}

void CHierachicalOutStreamBase::WriteSubStreamList()
{
    // first, the count

    buffer->Add ((DWORD)subStreams.size());

    // for each sub-stream: (index, id, type)

    for (size_t i = 0, count = subStreams.size(); i < count; ++i)
    {
        IHierarchicalOutStream* subStream = subStreams[i];
        buffer->Add (subStream->AutoClose());
        buffer->Add (subStream->GetID());
        buffer->Add (subStream->GetTypeID());
    }
}

void CHierachicalOutStreamBase::AutoOpen()
{
    if (!isOpen)
    {
        isOpen = true;
        CloseLatestSubStream();

        index = buffer->OpenStream();
        WriteSubStreamList();
    }
}

void CHierachicalOutStreamBase::WriteThisStream()
{
    AutoOpen();

    // Huffman-compress the raw stream data

    CHuffmanEncoder packer;
    std::pair<unsigned char*, DWORD> packedData
        = packer.Encode (GetStreamData(), GetStreamSize());

    decodedSize += GetStreamSize();

    // add it to the target file

    buffer->Add (packedData.first, packedData.second);
    delete[] packedData.first;
}

void CHierachicalOutStreamBase::Close()
{
    FlushData();
    WriteThisStream();
    buffer->Add (static_cast<DWORD>(decodedSize));
    ReleaseStreamData();

    buffer->CloseStream();
    isOpen = false;
}

// construction / destruction (Close() must have been called before)

CHierachicalOutStreamBase::CHierachicalOutStreamBase ( CCacheFileOutBuffer* aBuffer
                                                     , SUB_STREAM_ID anID)
    : id (anID)
    , buffer (aBuffer)
    , index (-1)
    , isOpen (false)
    , decodedSize (0)
{
}

CHierachicalOutStreamBase::~CHierachicalOutStreamBase()
{
    assert (index != -1);

    for (size_t i = 0, count = subStreams.size(); i < count; ++i)
        delete subStreams[i];
}

// implement IHierarchicalOutStream

SUB_STREAM_ID CHierachicalOutStreamBase::GetID() const
{
    return id;
}

IHierarchicalOutStream*
CHierachicalOutStreamBase::OpenSubStream ( SUB_STREAM_ID subStreamID
                                         , STREAM_TYPE_ID type)
{
    // close existing sub-streams

    CloseLatestSubStream();

    // open a new one

    const IOutStreamFactory* factory
        = COutStreamFactoryPool::GetInstance()->GetFactory(type);
    IHierarchicalOutStream* subStream
        = factory->CreateStream (buffer, subStreamID);

    // add it to our list

    subStreams.push_back (subStream);

    return subStream;
}

STREAM_INDEX CHierachicalOutStreamBase::AutoClose()
{
    // still open or not open yet?

    if (isOpen || (index == -1))
        Close();

    return index;
}
