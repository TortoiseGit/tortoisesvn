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
#include "StdAfx.h"
#include "BufferedOutFile.h"
#include "PathUtils.h"

// write buffer content to disk

void CBufferedOutFile::Flush()
{
	if (used > 0)
	{
		DWORD written = 0;
		WriteFile (file, buffer.get(), used, &written, NULL);
		used = 0;
	}
}

// construction / destruction: auto- open/close

CBufferedOutFile::CBufferedOutFile (const std::wstring& fileName)
	: file (INVALID_HANDLE_VALUE)
	, buffer (new unsigned char [BUFFER_SIZE])
	, used (0)
	, fileSize (0)
{
	CPathUtils::MakeSureDirectoryPathExists(fileName.substr(0, fileName.find_last_of('\\')).c_str());
	file = CreateFile ( fileName.c_str()
					  , GENERIC_WRITE
					  , 0
					  , NULL
					  , CREATE_ALWAYS
					  , FILE_ATTRIBUTE_NORMAL
					  , NULL);
	if (file == INVALID_HANDLE_VALUE)
		throw std::exception ("can't create log cache file");
}

CBufferedOutFile::~CBufferedOutFile()
{
	if (IsOpen())
	{
		Flush();
		CloseHandle (file);
	}
}

// write data to file

void CBufferedOutFile::Add (const unsigned char* data, DWORD bytes)
{
	// test for buffer overflow

	if (used + bytes > BUFFER_SIZE)
	{
		Flush();

		// don't buffer large chunks of data

		if (bytes >= BUFFER_SIZE)
		{
			DWORD written = 0;
			WriteFile (file, data, bytes, &written, NULL);
			fileSize += written;

			return;
		}
	}

	// buffer small chunks of data

	memcpy (buffer.get() + used, data, bytes);

	used += bytes;
	fileSize += bytes;
}

///////////////////////////////////////////////////////////////
// file stream operation
///////////////////////////////////////////////////////////////

CBufferedOutFile& operator<< (CBufferedOutFile& dest, int value)
{
	enum {BUFFER_SIZE = 11};
	char buffer [BUFFER_SIZE];
	_itoa_s (value, buffer, 10);

	return operator<< (dest, buffer);
}
