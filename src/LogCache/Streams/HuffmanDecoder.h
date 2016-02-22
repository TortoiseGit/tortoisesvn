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

#include "HuffmanBase.h"
#include "StreamException.h"

///////////////////////////////////////////////////////////////
//
// CHuffmanDecoder
//
///////////////////////////////////////////////////////////////

class CHuffmanDecoder : public CHuffmanBase
{
public:
    class CInputBuffer
    {
    public:
        CInputBuffer(const BYTE *buf, size_t len)
            : current(buf)
            , remaining(len)
        {
        }

        const void * GetData(DWORD len)
        {
            if (remaining < len)
            {
                throw CStreamException("corrupted huffman stream");
            }

            const void *result = current;
            current += len;
            remaining -= len;

            return result;
        }

        BYTE GetByte()
        {
            return *reinterpret_cast<const BYTE*>(GetData(1));
        }

        BYTE PeekByte()
        {
            if (remaining < 1)
            {
                throw CStreamException("corrupted huffman stream");
            }

            return *current;
        }

        DWORD GetDWORD()
        {
            return *reinterpret_cast<const DWORD*>(GetData(sizeof(DWORD)));
        }

    private:
        const BYTE *current;
        size_t remaining;
    };

    class COutputBuffer
    {
    public:
        COutputBuffer(BYTE *buf, size_t len)
            : current(buf)
            , remaining(len)
        {
        }

        inline size_t GetRemaining()
        {
            return remaining;
        }

        BYTE * GetBuffer(size_t len)
        {
            if (remaining < len)
            {
                throw CStreamException("huffman buffer overflow");
            }

            BYTE *result = current;
            current += len;
            remaining -= len;

            return result;
        }

    private:
        BYTE *current;
        size_t remaining;
    };
private:

    // decoder table:
    // for each possible key (i.e. encoded char), we store
    // the plain text value and the length of the key in bits.

    // For keys that are shorter than MAX_ENCODING_LENGTH,
    // we store the plain text in for all possible bit
    // combinations after the actual key.

    BYTE value[1 << MAX_ENCODING_LENGTH];
    BYTE length[1 << MAX_ENCODING_LENGTH];

    // read the encoding table from the beginning of the
    // compressed data buffer and fill the value[] and
    // length[] arrays.

    void BuildDecodeTable (CInputBuffer & source);

    // efficiently decode the source stream until the
    // plain text stream reaches decodedSize.

    void WriteDecodedStream ( CInputBuffer & source
                            , COutputBuffer & target
                            , DWORD decodedSize);

public:
    // construction / destruction: nothing special to do

    CHuffmanDecoder()
    {
        SecureZeroMemory(&value, sizeof(value));
        SecureZeroMemory(&length, sizeof(length));
    };
    virtual ~CHuffmanDecoder() {};

    // decompress the source data and return the target buffer.

    void Decode (CInputBuffer & source, COutputBuffer & target);
};
