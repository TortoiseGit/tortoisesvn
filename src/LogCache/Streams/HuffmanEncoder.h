#pragma once

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
#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "HuffmanBase.h"

///////////////////////////////////////////////////////////////
//
// CHuffmanEncoder
//
///////////////////////////////////////////////////////////////

class CHuffmanEncoder : public CHuffmanBase
{
private:

	// for each plain text value:
	// the compressed value (key), the length of that
	// key in bits and the number of occurrences of the
	// original characters.

	// \ref key and \ref keyLength are only valid if
	// \ref count is > 0.

	key_type key[BUCKET_COUNT];
	BYTE keyLength[BUCKET_COUNT];
	DWORD count[BUCKET_COUNT];

	// plain text values ordered by frequency (most
	// frequently used first). Only \ref sortedCount
	// entries are valid.

	BYTE sorted [BUCKET_COUNT];

	// number of byte values that *actually* occurred
	// in the plain text buffer.

	size_t sortedCount;

	// Huffman encoding stages:

	// (1) determine distribution

	void CountValues ( const unsigned char* source
					 , const unsigned char* end);

	// (2) prepare for key assignment: sort by frequency

	void SortByFrequency();

	// (3) recursively construct & assign keys

	void AssignEncoding ( BYTE* first
					    , BYTE* last
					    , key_type encoding
					    , BYTE bitCount);

	// (4) target buffer size calculation

	DWORD CalculatePackedSize();

	// (5) target buffer size calculation

	void WriteHuffmanTable (BYTE*& dest);

	// (6) write encoded target stream

	void WriteHuffmanEncoded ( const BYTE* source
						     , const BYTE* end
							 , BYTE* dest);

public:

	// construction / destruction: nothing special to do

	CHuffmanEncoder();
	virtual ~CHuffmanEncoder() {};

	// compress the source data and return the target buffer.
	// The caller must delete the target buffer.

	std::pair<BYTE*, DWORD> Encode (const BYTE* source, size_t byteCount);
};
