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
 * class that provides a simple, buffered file write stream.
 */
class CBufferedOutFile
{
private:

	// the file

	HANDLE file;

	// our local buffer

	enum {BUFFER_SIZE = 1024*1024};

	std::auto_ptr<unsigned char> buffer;
	DWORD used;

	// physical file size + used

	size_t fileSize;

protected:

	// write buffer content to disk

	void Flush();

public:

	// construction / destruction: auto- open/close

	CBufferedOutFile (const std::wstring& fileName);
	virtual ~CBufferedOutFile();

	// write data to file

	void Add (const unsigned char* data, DWORD bytes);
	void Add (const char* data, DWORD bytes);
	void Add (DWORD value);

	// file properties

	size_t GetFileSize() const;
	bool IsOpen() const;
};

///////////////////////////////////////////////////////////////
// write data to file
///////////////////////////////////////////////////////////////

inline void CBufferedOutFile::Add (const char* data, DWORD bytes)
{
	Add (reinterpret_cast<const unsigned char*>(data), bytes);
}

inline void CBufferedOutFile::Add (DWORD value)
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
	return file != INVALID_HANDLE_VALUE;
}

///////////////////////////////////////////////////////////////
// file stream operation
///////////////////////////////////////////////////////////////

CBufferedOutFile& operator<< (CBufferedOutFile& dest, int value);

inline CBufferedOutFile& operator<< (CBufferedOutFile& dest, const char* value)
{
	dest.Add (value, (DWORD)strlen (value));
	return dest;
}

inline CBufferedOutFile& operator<< (CBufferedOutFile& dest, const std::string& value)
{
	dest.Add (value.c_str(), (DWORD)value.length());
	return dest;
}
