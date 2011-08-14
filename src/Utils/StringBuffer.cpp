// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011 - TortoiseSVN

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
#include "StringBuffer.h"

void CStringBuffer::Reserve (size_t newCapacity)
{
    assert (newCapacity >= capacity);

    // allocate new buffer and align it.
    // Add one ALIGNMENT for the start address alignment
    // plus one for the sloppy copies beyond newCapacity.

    char *newMemory = new char[newCapacity + 2 * ALIGNMENT];
    char *newBuffer = (char*)(APR_ALIGN((size_t)newMemory, ALIGNMENT));

    // replace buffers
    if (buffer)
        memcpy (newBuffer, buffer, APR_ALIGN(size + 1, ALIGNMENT));

    delete[] memory;
    memory = newMemory;
    buffer = newBuffer;

    capacity = newCapacity;
}

CStringBuffer::CStringBuffer (size_t initialCapacity)
    : buffer (NULL)
    , memory (NULL)
    , capacity (0)
    , size (0)
{
    Reserve (initialCapacity);
    buffer[0] = 0;
}

CStringBuffer::~CStringBuffer()
{
    delete[] memory;
}

void CStringBuffer::Append (const std::string& s)
{
    Append (s.c_str(), s.size());
}

void CStringBuffer::Append (const char* s)
{
    Append (s, strlen (s));
}

void CStringBuffer::Append (const char* string, size_t length)
{
    if (size + length >= capacity)
        Reserve (2 * max (length, capacity));

    memcpy (buffer + size, string, length+1);
    size += length;
}
