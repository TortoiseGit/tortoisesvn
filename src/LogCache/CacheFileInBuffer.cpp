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
#include "StdAfx.h"
#include "CacheFileInBuffer.h"

// construction utilities

void CCacheFileInBuffer::ReadStreamOffsets()
{
	// minimum size: 3 DWORDs

	if (GetSize() < 3 * sizeof (DWORD))
		throw std::exception ("log cache file too small");

	// extract version numbers

	DWORD creatorVersion = *GetDWORD (0);
	DWORD requiredVersion = *GetDWORD (sizeof (DWORD));

	// check version number

	if (creatorVersion < OLDEST_LOG_CACHE_FILE_VERSION)
		throw std::exception ("log cache file format too old");

	if (requiredVersion > NEWEST_LOG_CACHE_FILE_VERSION)
		throw std::exception ("log cache file format too new");

    // number of streams in file

	DWORD streamCount = *GetDWORD (GetSize() - sizeof (DWORD));
	if (GetSize() < (3 + streamCount) * sizeof (DWORD))
		throw std::exception ("log cache file too small to hold stream directory");

	// read stream sizes and ranges list

	const unsigned char* lastStream = GetBuffer() + 2* sizeof (DWORD);

	streamContents.reserve (streamCount+1);
	streamContents.push_back (lastStream);

	size_t contentEnd = GetSize() - (streamCount+1) * sizeof (DWORD);
	const DWORD* streamSizes = GetDWORD (contentEnd);

	for (DWORD i = 0; i < streamCount; ++i)
	{
		lastStream += streamSizes[i];
		streamContents.push_back (lastStream);
	}

	// consistency check

	if ((size_t)(lastStream - GetBuffer()) != contentEnd)
		throw std::exception ("stream directory corrupted");
}


// construction / destruction: auto- open/close

CCacheFileInBuffer::CCacheFileInBuffer (const std::wstring& fileName)
    : CMappedInFile (fileName)
{
	ReadStreamOffsets();
}

CCacheFileInBuffer::~CCacheFileInBuffer()
{
}

// access streams

STREAM_INDEX CCacheFileInBuffer::GetLastStream()
{
	return (STREAM_INDEX)(streamContents.size()-2);
}

void CCacheFileInBuffer::GetStreamBuffer ( STREAM_INDEX index
										 , const unsigned char* &first
										 , const unsigned char* &last)
{
	if ((size_t)index >= streamContents.size()-1)
		throw std::exception ("invalid stream index");

	first = streamContents[index];
	last = streamContents[index+1];
}