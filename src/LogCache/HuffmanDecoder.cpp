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
		if (keyLength[i-1] < keyLength[i])
			currentKey <<= keyLength[i] - keyLength[i-1];
		else
			if (keyLength[i-1] > keyLength[i])
				currentKey >>= keyLength[i-1] - keyLength[i];

		key[i] = ReverseBits (currentKey, keyLength[i]);
	}

	// fill the decoding tables

	for (size_t i = 0; i < entryCount; ++i)
	{
		size_t delta = 1 << keyLength[i];
		for (size_t k = key[i]; k < 1 << MAX_ENCODING_LENGTH; k += delta)
		{
			value[k] = values[i];
			length[k] = keyLength[i];
		}
	}
}

void CHuffmanDecoder::WriteDecodedStream ( const BYTE* first
										 , BYTE* dest
										 , DWORD decodedSize)
{
	key_type cachedCode = 0;
	BYTE cachedBits = 0;

#ifdef _WIN64

	// main loop

	encode_block_type* blockDest 
		= reinterpret_cast<encode_block_type*>(dest);
	encode_block_type* blockEnd 
		= blockDest + decodedSize / sizeof (encode_block_type);

	for (; blockDest != blockEnd; ++blockDest)
	{
		// fetch encoded data into cache

		size_t bytesToFetch = (KEY_BITS - cachedBits) / 8;

		cachedCode |= *reinterpret_cast<const key_type*>(first) << cachedBits;

		first += bytesToFetch;
		cachedBits += static_cast<BYTE>(bytesToFetch * 8);

		// decode 4 bytes

		BYTE length1 = length[cachedCode & MAX_KEY_VALUE];
		BYTE value1 = value[cachedCode & MAX_KEY_VALUE];
		cachedCode >>= length1;
		cachedBits -= length1;

		encode_block_type data = value1;

		BYTE length2 = length[cachedCode & MAX_KEY_VALUE];
		BYTE value2 = value[cachedCode & MAX_KEY_VALUE];
		cachedCode >>= length2;
		cachedBits -= length2;

		data += value2 << 8;

		BYTE length3 = length[cachedCode & MAX_KEY_VALUE];
		BYTE value3 = value[cachedCode & MAX_KEY_VALUE];
		cachedCode >>= length3;
		cachedBits -= length3;

		data += value3 << 16;

		BYTE length4 = length[cachedCode & MAX_KEY_VALUE];
		BYTE value4 = value[cachedCode & MAX_KEY_VALUE];
		cachedCode >>= length4;
		cachedBits -= length4;

		data += value4 << 24;

		// write result

		*blockDest = data;
	}

	// fetch encoded data into cache and decode odd bytes

	size_t bytesToFetch = (KEY_BITS - cachedBits) / 8;
	cachedCode |= *reinterpret_cast<const key_type*>(first) << cachedBits;

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

#else

	// main loop

	for ( BYTE* end = dest + decodedSize
		; dest != end
		; ++dest)
	{
		// fetch encoded data into cache

		size_t bytesToFetch = (KEY_BITS - cachedBits) / 8;

		cachedCode |= *reinterpret_cast<const key_type*>(first) << cachedBits;

		first += bytesToFetch;
		cachedBits += static_cast<BYTE>(bytesToFetch * 8);

		// decode 1 byte

		BYTE keyLength = length[cachedCode & MAX_KEY_VALUE];
		BYTE data = value[cachedCode & MAX_KEY_VALUE];
		cachedCode >>= keyLength;
		cachedBits -= keyLength;

		// write result

		*dest = data;
	}

#endif
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

	// read all the decode-info 

	BuildDecodeTable (localSource);

	// actually decode

	WriteDecodedStream (localSource, target, decodedSize);

	// update source and target buffer pointers

	source += encodedSize;
	target += decodedSize;
}

