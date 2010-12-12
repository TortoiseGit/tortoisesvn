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
#include "stdafx.h"
#include "BufferedOutFile.h"
#include "StreamException.h"

struct CString {};
struct CStringA {};
struct CStringW {};

#include "PathUtils.h"

// write buffer content to disk

void CBufferedOutFile::Flush()
{
    if (used > 0)
    {
    #ifdef WIN32
        DWORD written = 0;
        WriteFile (file, buffer.get(), used, &written, NULL);
    #else
        stream.write (reinterpret_cast<const char*>(buffer.get()), used);
    #endif
        used = 0;
    }
}

// close and remove the file

void CBufferedOutFile::RemoveFile()
{
    InternalClose();

#ifdef _WIN32
    DeleteFile (fileName.c_str());
#else
    remove (fileName.c_str());
#endif
}

// helper

void CBufferedOutFile::InternalClose()
{
    if (IsOpen())
    {
        Flush();

#ifdef _WIN32
        CloseHandle (file);
#else
        stream.close();
#endif
    }
}

// construction / destruction: auto- open/close

#ifdef _WIN32
CBufferedOutFile::CBufferedOutFile (const TFileName& fileName)
    : fileName (fileName)
    , file (INVALID_HANDLE_VALUE)
    , buffer (BUFFER_SIZE)
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
        throw CStreamException ("can't create log cache file");
}
#else
CBufferedOutFile::CBufferedOutFile (const TFileName& fileName)
    : fileName (fileName)
    , buffer (BUFFER_SIZE)
    , used (0)
    , fileSize (0)
{
    CPathUtils::MakeSureDirectoryPathExists(fileName.substr(0, fileName.find_last_of('/')).c_str());
    stream.open (fileName.c_str(), std::ios::binary | std::ios::out);
    if (!stream.is_open())
        throw CStreamException ("can't create log cache file");
}
#endif

CBufferedOutFile::~CBufferedOutFile()
{
    InternalClose();
}

// write data to file

void CBufferedOutFile::Add (const unsigned char* data, unsigned bytes)
{
    // test for buffer overflow

    if (used + bytes > BUFFER_SIZE)
    {
        Flush();

        // don't buffer large chunks of data

        if (bytes >= BUFFER_SIZE)
        {
        #ifdef _WIN32
            DWORD written = 0;
            WriteFile (file, data, bytes, &written, NULL);
            fileSize += written;
        #else
            stream.write (reinterpret_cast<const char*>(data), bytes);
            fileSize += bytes;
        #endif

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
