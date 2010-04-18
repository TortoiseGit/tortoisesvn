// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010 - TortoiseSVN

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
#include "HuffmanDecoder.h"

void CHuffmanDecoder::BuildDecodeTable (const BYTE*& first)
{
    // get the table size (full table is indicated by length == 0)

    size_t entryCount = *first;
    ++first;

    if (entryCount == 0)
        entryCount = BUCKET_COUNT;

    // read the raw table data

    BYTE values[BUCKET_COUNT];
    BYTE keyLength[BUCKET_COUNT];
    for (size_t i = 0; i < entryCount; ++i)
    {
        values[i] = *first;
        ++first;
    }

    for (size_t i = 0; i < entryCount; i += 2)
    {
        keyLength[i] = *first & 0x0f;
        keyLength[i+1] = *first / 0x10;
        ++first;
    }

    // reconstruct the keys

    key_type key[BUCKET_COUNT];
    key[0] = 0;

    key_type currentKey = 0;
    for (size_t i = 1; i < entryCount; ++i)
    {
        ++currentKey;
        BYTE prevKeyLength = keyLength[i-1];
        BYTE thisKeylength = keyLength[i];

        if (prevKeyLength < thisKeylength)
            currentKey <<= thisKeylength - prevKeyLength;
        else
            if (prevKeyLength > thisKeylength)
                currentKey >>= prevKeyLength - thisKeylength;

        key[i] = ReverseBits (currentKey, thisKeylength);
    }

    // fill the decoding tables

    for (size_t i = 0; i < entryCount; ++i)
    {
        BYTE l = keyLength[i];
        BYTE v = values[i];

        size_t delta = 1 << l;
        for (size_t k = key[i]; k < 1 << MAX_ENCODING_LENGTH; k += delta)
        {
            value[k] = v;
            length[k] = l;
        }
    }
}

void CHuffmanDecoder::WriteDecodedStream ( const BYTE* first
                                         , BYTE* dest
                                         , DWORD decodedSize)
{
    key_block_type cachedCode = 0;
    BYTE cachedBits = 0;

    // main loop

    BYTE* blockDest = dest;
    BYTE* blockEnd = blockDest + (decodedSize & (0-sizeof (encode_block_type)));

    key_block_type nextCodes = *reinterpret_cast<const key_block_type*>(first);
    for (; blockDest != blockEnd; blockDest += sizeof (encode_block_type))
    {
        // pre-fetch, part III

        cachedCode |= nextCodes << cachedBits;

        // decode 2 (32 bit) to 4 (64 bit) bytes
        // decode byte 0

        BYTE keyLength = length[cachedCode & MAX_KEY_VALUE];
        blockDest[0] = value[cachedCode & MAX_KEY_VALUE];

        // pre-fetch, part I

        first += ((size_t)(KEY_BLOCK_BITS-1) - (size_t)cachedBits) / 8;
        cachedBits |= KEY_BLOCK_BITS - 8;       // KEY_BLOCK_BITS must be 2^n

        // continue byte 0

        cachedCode >>= keyLength;
        cachedBits -= keyLength;

        // decode byte 1

        keyLength = length[cachedCode & MAX_KEY_VALUE];
        blockDest[1] = value[cachedCode & MAX_KEY_VALUE];

        // pre-fetch, part II

        nextCodes = *reinterpret_cast<const key_block_type*>(first);

        // continue byte 1

        cachedCode >>= keyLength;
        cachedBits -= keyLength;

#ifdef _64BITS

        // decode byte 2

        keyLength = length[cachedCode & MAX_KEY_VALUE];
        blockDest[2] = value[cachedCode & MAX_KEY_VALUE];
        cachedCode >>= keyLength;
        cachedBits -= keyLength;

        // decode byte 3

        keyLength = length[cachedCode & MAX_KEY_VALUE];
        blockDest[3] = value[cachedCode & MAX_KEY_VALUE];
        cachedCode >>= keyLength;
        cachedBits -= keyLength;

#endif
    }

    // fetch encoded data into cache and decode odd bytes

    cachedCode |= nextCodes << cachedBits;

    for ( BYTE* byteDest = reinterpret_cast<BYTE*>(blockDest)
        , *end = dest + decodedSize
        ; byteDest != end
        ; ++byteDest)
    {
        // decode 1 byte

        BYTE keyLength = length[cachedCode & MAX_KEY_VALUE];
        BYTE data = value[cachedCode & MAX_KEY_VALUE];
        cachedCode >>= keyLength;

        *byteDest = data;
    }
}

// decompress the source data and return the target buffer.

void CHuffmanDecoder::Decode (const BYTE*& source, BYTE*& target)
{
    // get size info from stream

    const BYTE* localSource = source;
    DWORD decodedSize = *reinterpret_cast<const DWORD*>(localSource);
    localSource += sizeof (DWORD);
    size_t encodedSize = *reinterpret_cast<const DWORD*>(localSource);
    localSource += sizeof (DWORD);

    // special case: empty stream (hence, empty tables etc.)

    if (decodedSize == 0)
        return;

    // special case: unpacked data

    if ((encodedSize == decodedSize+MIN_HEADER_LENGTH) && (*localSource == 0))
    {
        // plain copy (and skip empty huffman table)

        memcpy (target, ++localSource, decodedSize);
    }
    else
    {
        // read all the decode-info 

        BuildDecodeTable (localSource);

        // actually decode

        WriteDecodedStream (localSource, target, decodedSize);
    }

    // update source and target buffer pointers

    source += encodedSize;
    target += decodedSize;
}