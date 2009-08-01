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

#ifndef WIN32
#include <fstream>
#endif

#include "FileName.h"

/**
 * class that provides a simple, buffered file write stream.
 */
class CBufferedOutFile
{
private:

#ifdef WIN32
	// the file

	HANDLE file;
#else
	// fallback: STL stream

	std::ofstream stream;
#endif

	// our local buffer

	enum {BUFFER_SIZE = 1024*1024};

	std::auto_ptr<unsigned char> buffer;
	unsigned used;

	// physical file size + used

	size_t fileSize;

protected:

	// write buffer content to disk

	void Flush();

public:

	// construction / destruction: auto- open/close

	CBufferedOutFile (const TFileName& fileName);
	virtual ~CBufferedOutFile();

	// write data to file

	void Add (const unsigned char* data, unsigned bytes);
	void Add (const char* data, unsigned bytes);
	void Add (unsigned value);

	// file properties

	size_t GetFileSize() const;
	bool IsOpen() const;
};

///////////////////////////////////////////////////////////////
// write data to file
///////////////////////////////////////////////////////////////

inline void CBufferedOutFile::Add (const char* data, unsigned bytes)
{
	Add (reinterpret_cast<const unsigned char*>(data), bytes);
}

inline void CBufferedOutFile::Add (unsigned value)
{
	Add ((unsigned char*)&value, sizeof (value));
}

///////////////////////////////////////////////////////////////
// file properties
///////////////////////////////////////////////////////////////

inline size_t CBufferedOutFile::GetFileSize() const
{
	return fileSize;
}

inline bool CBufferedOutFile::IsOpen() const
{
#ifdef WIN32
	return file != INVALID_HANDLE_VALUE;
#else
	return stream.is_open();
#endif
}

///////////////////////////////////////////////////////////////
// file stream operation
///////////////////////////////////////////////////////////////

CBufferedOutFile& operator<< (CBufferedOutFile& dest, int value);

inline CBufferedOutFile& operator<< (CBufferedOutFile& dest, const char* value)
{
	dest.Add (value, (unsigned)strlen (value));
	return dest;
}

inline CBufferedOutFile& operator<< (CBufferedOutFile& dest, const std::string& value)
{
	dest.Add (value.c_str(), (unsigned)value.length());
	return dest;
}
