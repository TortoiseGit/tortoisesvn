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
//
// CHuffmanBase
//
//		Common definitions for Huffman compression.
//
///////////////////////////////////////////////////////////////

class CHuffmanBase
{
public:

	// aliases for 8 and 64 bit chunks

	typedef unsigned __int64 QWORD;
	typedef unsigned char BYTE;

	// encoded data chunk type
	// value counting chunk type
	// plaintext data chunk type

#ifdef _WIN64
	typedef QWORD key_type;
	typedef QWORD count_block_type;
	typedef DWORD encode_block_type;
#else
	typedef DWORD key_type;
	typedef DWORD count_block_type;
	typedef WORD encode_block_type;
#endif

	// global constants

	enum 
	{
		// number of different plaintext tokens 
		// (256 because we encode full bytes)

		BUCKET_COUNT = 1 << (8 * sizeof (BYTE)), 

		// length of the key_type in bits
		// i.e. capacity of a "register" that can be used w/o 
		// writing to the target buffer or be read at once

		KEY_BITS = sizeof (key_type) * 8,

		// max *actual* key length.
		// we limit it to 12 to minimize decoding effort and
		// decoding table size. Impact on compression ratio is 
		// marginal.
		// also, allows to hold 2 keys plus a fraction of one byte
		// (odd bits that cannot yet be written to the target)
		// in a single DWORD.

		MAX_ENCODING_LENGTH = 12,	

		// mask to extract MAX_ENCODING_LENGTH bits from a 
		// chunk of data.

		MAX_KEY_VALUE = (1 << MAX_ENCODING_LENGTH) -1
	};

	// utility function that reverses the bit order of a given key

	static key_type ReverseBits (key_type v, BYTE length);
};
