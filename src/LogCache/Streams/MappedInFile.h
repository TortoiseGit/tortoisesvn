// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2010 - TortoiseSVN

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

#include "FileName.h"

/**
 * class that maps arbitrary files into memory.
 *
 * The file must be small enough to fit into your address space (i.e., less than
 * about 1GB on win32).
 */
class CMappedInFile
{
private:

#ifdef _WIN32
    // the file

    HANDLE file;

    // the memory mapping

    HANDLE mapping;
#else
    int file;
#endif

    // file content memory address

    unsigned char* buffer;

    // physical file size (== file size)

    size_t size;

    // if true, buffer content may be modfied

    bool writable;

    // construction utilities

    void MapToMemory (const TFileName& fileName);

    // destruction / exception utility: close all handles

    void UnMap (size_t newSize = (size_t)-1);

public:

    // construction / destruction: auto- open/close

    CMappedInFile (const TFileName& fileName, bool writable = false);
    virtual ~CMappedInFile();

    // access streams

    const unsigned char* GetBuffer() const;
    unsigned char* GetWritableBuffer() const;
    size_t GetSize() const;

    // close and optionally truncate file

    void Close (size_t newSize = (size_t)(-1));
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
