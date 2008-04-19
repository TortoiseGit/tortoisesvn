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
// CHuffmanDecoder
//
///////////////////////////////////////////////////////////////

class CHuffmanDecoder : public CHuffmanBase
{
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

	void BuildDecodeTable (const BYTE*& first);

	// efficiently decode the source stream until the
	// plain text stream reaches decodedSize.

	void WriteDecodedStream ( const BYTE* first
							, BYTE* dest
							, DWORD decodedSize);

public:

	// construction / destruction: nothing special to do

	CHuffmanDecoder() {};
	virtual ~CHuffmanDecoder() {};

	// decompress the source data and return the target buffer.

	void Decode (const BYTE*& source, BYTE*& target);
};
