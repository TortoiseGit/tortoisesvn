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

/**
 * class that maps arbitrary files into memory.
 *
 * The file must be small enough to fit into your address space (i.e., less than
 * about 1GB on win32).
 */
class CMappedInFile
{
private:

	// the file

	HANDLE file;

	// the memory mapping

	HANDLE mapping;

	// file content memory address

	const unsigned char* buffer;

	// physical file size (== file size)

	size_t size;

	// construction utilities

	void MapToMemory (const std::wstring& fileName);

	// destruction / exception utility: close all handles

	void UnMap();

public:

	// construction / destruction: auto- open/close

	CMappedInFile (const std::wstring& fileName);
	virtual ~CMappedInFile();

	// access streams

	const unsigned char* GetBuffer() const;
	size_t GetSize() const;
};

///////////////////////////////////////////////////////////////
// access streams
///////////////////////////////////////////////////////////////

inline const unsigned char* CMappedInFile::GetBuffer() const
{
	return buffer;
}

inline size_t CMappedInFile::GetSize() const
{
	return size;
}
