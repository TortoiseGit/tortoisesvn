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
#pragma once

///////////////////////////////////////////////////////////////
// include base class
///////////////////////////////////////////////////////////////

#include "MappedInFile.h"

///////////////////////////////////////////////////////////////
// index type used to address a certain stream within the file.
///////////////////////////////////////////////////////////////

typedef int STREAM_INDEX;

///////////////////////////////////////////////////////////////
// the oldest and newest file version number we will understand
///////////////////////////////////////////////////////////////

enum
{
	OLDEST_LOG_CACHE_FILE_VERSION = 0x20070607,
	NEWEST_LOG_CACHE_FILE_VERSION = 0x20090623
};

/**
 * class that maps files created by CCacheFileOutBuffer (see there for format
 * details) into memory.
 * 
 * The file must be small enough to fit into your address space (i.e., less than
 * about 1GB on win32).
 *
 * GetLastStream() will return the index of the root stream.
 * Streams will be returned as half-open ranges [first, last).
 */
class CCacheFileInBuffer : private CMappedInFile
{
private:

	// start-addresses of all streams (i.e. streamCount + 1 entry)

	std::vector<const unsigned char*> streamContents;

	// construction utilities

	void ReadStreamOffsets();

	// data access utility

	const unsigned* GetDWORD (size_t offset) const
	{
		// ranges should have been checked before

		assert ((offset < GetSize()) && (offset + sizeof (unsigned) <= GetSize()));
		return reinterpret_cast<const unsigned*>(GetBuffer() + offset);
	}

public:

	// construction / destruction: auto- open/close

	CCacheFileInBuffer (const TFileName& fileName);
	~CCacheFileInBuffer();

	// access streams

	STREAM_INDEX GetLastStream();
	void GetStreamBuffer ( STREAM_INDEX index
						 , const unsigned char* &first
						 , const unsigned char* &last);
};

