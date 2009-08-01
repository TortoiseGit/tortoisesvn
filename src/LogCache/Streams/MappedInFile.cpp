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
#include "stdafx.h"
#include "MappedInFile.h"
#include "StreamException.h"

// construction utilities

void CMappedInFile::MapToMemory (const TFileName& fileName)
{
#ifdef WIN32
    // create the file handle & open the file r/o

    file = CreateFile ( fileName.c_str()
                      , GENERIC_READ
                      , FILE_SHARE_READ
                      , NULL
                      , OPEN_EXISTING
                      , FILE_ATTRIBUTE_NORMAL
                      , NULL);
    if (file == INVALID_HANDLE_VALUE)
        throw CStreamException ("can't read log cache file");

    // get buffer size (we can always cast safely from
    // fileSize to size since the memory mapping will fail
    // anyway for > 4G files on win32)

    LARGE_INTEGER fileSize;
    fileSize.QuadPart = 0;
    GetFileSizeEx (file, &fileSize);
    size = (size_t) fileSize.QuadPart;

    // create a file mapping object able to contain the whole file content

    mapping = CreateFileMapping (file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapping == NULL)
    {
        UnMap();
        throw CStreamException ("can't create mapping for log cache file");
    }

    // map the whole file

    LPVOID address = MapViewOfFile (mapping, FILE_MAP_READ, 0, 0, 0);
    if (address == NULL)
    {
        UnMap();
        throw CStreamException ("can't map the log cache file into memory");
    }

    // set our buffer pointer

    buffer = reinterpret_cast<unsigned char*> (address);
#else
    // open file

    file = open (fileName.c_str(), O_RDONLY, (mode_t)0600);
    if (file == -1)
        throw CStreamException ("can't read log cache file");

    // get size

    size = lseek (file, 0, SEEK_END);

    // try to map the content into memory

    buffer = (unsigned char*)mmap (NULL, size, PROT_READ, MAP_SHARED, file, 0);
    if (buffer == NULL)
    {
        close (file);
        throw CStreamException ("can't map the log cache file into memory");
    }
#endif
}

// close all handles

void CMappedInFile::UnMap()
{
#ifdef WIN32
    if (buffer != NULL)
        UnmapViewOfFile (buffer);

    if (mapping != INVALID_HANDLE_VALUE)
        CloseHandle (mapping);

    if (file != INVALID_HANDLE_VALUE)
        CloseHandle (file);
#else
    if (buffer != NULL)
        munmap (buffer, size);

    if (file != -1)
        close (file);
#endif
}

// construction / destruction: auto- open/close

CMappedInFile::CMappedInFile (const TFileName& fileName)
#ifdef WIN32
    : file (INVALID_HANDLE_VALUE)
    , mapping (INVALID_HANDLE_VALUE)
#else
    : file (-1)
#endif
    , buffer (NULL)
    , size (0)
{
    MapToMemory (fileName);
}

CMappedInFile::~CMappedInFile()
{
    UnMap();
}
