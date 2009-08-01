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
#include "StdAfx.h"
#include "HuffmanEncoder.h"

// Huffman encoding stages:

// (1) determine distribution

void CHuffmanEncoder::CountValues ( const unsigned char* source
								  , const unsigned char* end)
{
	// keep intermediate results

	DWORD localCount[sizeof (count_block_type)][BUCKET_COUNT];
	SecureZeroMemory (localCount, sizeof (localCount));

	// main loop

	const count_block_type* blockSource 
		= reinterpret_cast<const count_block_type*>(source);
	const count_block_type* blockEnd 
		= blockSource + (end - source) / sizeof (count_block_type);

	for (; blockSource != blockEnd; ++blockSource)
	{
		count_block_type block = *blockSource;

		++localCount[0][block & 0xff];
		++localCount[1][(block >> 8) & 0xff];
		++localCount[2][(block >> 16) & 0xff];
		++localCount[3][(block >> 24) & 0xff];

#ifdef _64BITS

		++localCount[4][(block >> 32) & 0xff];
		++localCount[5][(block >> 40) & 0xff];
		++localCount[6][(block >> 48) & 0xff];
		++localCount[7][(block >> 56) & 0xff];

#endif
	}

	// count odd chars

	source = reinterpret_cast<const BYTE*>(blockEnd);
	for (; source != end; ++source)
		++count[*source];

	// fold temp. results

	for (size_t i = 0; i < sizeof (count_block_type); ++i)
		for (size_t k = 0; k < BUCKET_COUNT; ++k)
			count[k] += localCount[i][k];
}

// (2) prepare for key assignment: sort by frequency

void CHuffmanEncoder::SortByFrequency()
{
	// sort all tokens to encode and sort them by frequency

	typedef std::multimap<DWORD, BYTE> TFrequencyOrder;

	TFrequencyOrder frequencyOrder;
	for (size_t i = 0; i < BUCKET_COUNT; ++i)
		if (count[i] > 0)
			frequencyOrder.insert (std::make_pair ( count[i]
												  , static_cast<BYTE>(i)));

	// convert to an array for easier iteration

	BYTE* first = sorted;
	BYTE* last = first;
	for ( TFrequencyOrder::const_reverse_iterator iter = frequencyOrder.rbegin()
		, end = frequencyOrder.rend()
		; iter != end
		; ++iter)
	{
		*last = iter->second;
		++last;
	}

	sortedCount = frequencyOrder.size();

	// recursively construct Huffman keys

	if (first != last)
		AssignEncoding (first, last, 0, 0);
}

// (3) recursively construct & assign keys

void CHuffmanEncoder::AssignEncoding ( BYTE* first
								     , BYTE* last
								     , key_type encoding
								     , BYTE bitCount)
{
	size_t distance = last - first;
	if (distance == 1)
	{
		// we constructed a unique encoding

		key[*first] = ReverseBits (encoding, bitCount) << (KEY_BITS - bitCount);
		keyLength[*first] = bitCount;
	}
	else
	{
		// we must split this range

		BYTE* mid;
		if (((size_t)1 << (MAX_ENCODING_LENGTH-1 - bitCount)) < distance)
		{
			// oops .. we have some extremely unfavorable distribution
			// -> must artificially limit the key length because it 
			// might grow to 45+ bits

			mid = first + distance / 2;
		}
		else
		{
			// find the position that is closed to a 50:50 frequency split

			// 50% of what?

			DWORD totalSum = 0;
			for (BYTE* iter = first; iter != last; ++iter)
				totalSum += count[*iter];

			// find the middle position

			DWORD halfSum = 0;
			for (mid = first; (mid != last) && (halfSum < totalSum / 2); ++mid)
				halfSum += count[*mid];

			// maybe, the previous position is closer to a 50:50 split

			DWORD lowerHalfSum = halfSum - count[*(mid-1)];
			assert (totalSum / 2 > lowerHalfSum);
			assert (totalSum / 2 <= halfSum);

			if (totalSum - lowerHalfSum < halfSum)
				--mid;
		}

		// recursion

		assert ((first < mid) && (mid < last));

		AssignEncoding (first, mid, 2*encoding, bitCount+1);
		AssignEncoding (mid, last, 2*encoding+1, bitCount+1);
	}
}

// (4) target buffer size calculation

DWORD CHuffmanEncoder::CalculatePackedSize()
{
	// first, the original and packed buffer sizes

	DWORD result = 2 * sizeof(DWORD);

	// Huffman table: 
	//		* 1 byte entry count
	//		* 1.5 bytes per entry
	
	result += (DWORD)(1 + (3 * sortedCount + 1) / 2) * sizeof (BYTE);

	// calculate total bit count

	QWORD bitCount = 0;
	for (size_t i = 0; i < BUCKET_COUNT; ++i)
		bitCount += count[i] * keyLength[i];

	// packed data will use full QWORDs, i.e. up to 63 unused bits

	assert (bitCount < 0x800000000);
	result += static_cast<DWORD>(bitCount / 8 + sizeof (QWORD));

	// ready

	return result;
}

// (5) write a Huffman table

void CHuffmanEncoder::WriteHuffmanTable (BYTE*& dest)
{
	*dest = static_cast<BYTE>(sortedCount);
	++dest;

	for (size_t i = 0; i < sortedCount; ++i)
	{
		// write the character value

		*dest = sorted[i];
		++dest;
    }
    
    for (size_t i = 0; i < sortedCount; i+=2)
	{
        assert (MAX_ENCODING_LENGTH < 0x10);

        // number of bits used (use 4 bits per value)

		*dest = keyLength[sorted[i]]
              + keyLength[sorted[i+1]] * 0x10;
		++dest;
	}
}

// (6) write encoded target stream

void CHuffmanEncoder::WriteHuffmanEncoded ( const BYTE* source
									      , const BYTE* end
										  , BYTE* dest)
{
	key_type cachedCode = 0;
	BYTE cachedBits = 0;

#ifdef _64BITS

	// main loop (22 1/3 clock ticks per 4 chars on K8) 

	const encode_block_type* blockSource 
		= reinterpret_cast<const encode_block_type*>(source);
	const encode_block_type* blockEnd 
		= blockSource + (end - source) / sizeof (encode_block_type);

	for ( ; blockSource != blockEnd; ++blockSource)
	{
		// fetch 4 chars at once

		encode_block_type data = *blockSource;

		// build chain [key1][key2][key3][key4],
		// start with key4

		BYTE bits4 = keyLength [(data >> 24) & 0xff];
		key_type mask4 = key [(data >> 24) & 0xff];
		BYTE totalShift = bits4;
		key_type totalMask = mask4;

		BYTE bits3 = keyLength [(data >> 16) & 0xff];
		key_type mask3 = key [(data >> 16) & 0xff];
		mask3 >>= totalShift;
		totalShift += bits3;
		totalMask += mask3;

		BYTE bits2 = keyLength [(data >> 8) & 0xff];
		key_type mask2 = key [(data >> 8) & 0xff];
		mask2 >>= totalShift;
		totalShift += bits2;
		totalMask += mask2;

		BYTE bits1 = keyLength [data & 0xff];
		key_type mask1 = key [data & 0xff];
		mask1 >>= totalShift;
		totalShift += bits1;
		totalMask += mask1;

		// add to existing bit cache

		cachedCode >>= totalShift;
		cachedCode += totalMask;
		cachedBits += totalShift;

		// write full bytes only

		*reinterpret_cast<key_type*>(dest) 
			= cachedCode >> (KEY_BITS - cachedBits);
		dest += cachedBits / 8;

		// update cache

		cachedBits &= 7;
	}

	// encode odd chars

	source = reinterpret_cast<const BYTE*>(blockEnd);
	for ( ; source != end; ++source)
	{
		// encode just one byte

		DWORD data = *source;

		BYTE length = keyLength [data];
		key_type mask = key [data];

		// add to existing bit cache

		cachedCode >>= length;
		cachedCode += mask;
		cachedBits += length;
	}

#else

	// main loop (11.3 clock ticks per char on K8)

	for ( ; source != end; ++source)
	{
		// encode just one byte

		DWORD data = *source;

		BYTE length = keyLength [data];
		key_type mask = key [data];

		// add to existing bit cache

		cachedCode >>= length;
		cachedCode += mask;
		cachedBits += length;

		// write full bytes only

		*reinterpret_cast<key_type*>(dest) 
			= cachedCode >> (KEY_BITS - cachedBits);
		dest += cachedBits / 8;

		// update cache

		cachedBits &= 7;
	}

#endif

	// write the remaining cached data

	*reinterpret_cast<key_type*>(dest) 
		= cachedCode >> (KEY_BITS - cachedBits);
}

// construction: nothing special to do

CHuffmanEncoder::CHuffmanEncoder()
	: sortedCount (0)
{
	SecureZeroMemory (&key, sizeof (key));
	SecureZeroMemory (&keyLength, sizeof (keyLength));
	SecureZeroMemory (&count, sizeof (count));
	SecureZeroMemory (&sorted, sizeof (sorted));
}

// write local stream data and close the stream

std::pair<CHuffmanEncoder::BYTE*, DWORD>
CHuffmanEncoder::Encode (const BYTE* source, size_t byteCount)
{
	assert (sorted[0] == NULL);

	// this may fail under x64

	if (byteCount > (DWORD)(-1))
		throw std::exception ("BLOB to large for stream");

	// calculate static Huffman encoding

	CountValues (source, source + byteCount);
	SortByFrequency();

	// create buffer

	DWORD targetSize = CalculatePackedSize();
	std::auto_ptr<BYTE> buffer (new BYTE[targetSize]);

	// fill it

	BYTE* dest = buffer.get();
	*reinterpret_cast<DWORD*>(dest) = static_cast<DWORD>(byteCount);
	dest += sizeof (DWORD);
	*reinterpret_cast<DWORD*>(dest) = targetSize;
	dest += sizeof (DWORD);

	WriteHuffmanTable (dest);
	WriteHuffmanEncoded (source, source + byteCount, dest);

	return std::make_pair (buffer.release(), targetSize);
}
