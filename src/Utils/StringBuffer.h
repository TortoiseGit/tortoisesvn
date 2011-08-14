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
#pragma once

/**
 * Re-usable char[] buffer. In contrast to std::string, the
 * data buffer will not be initialized and we can legally and
 * safely write directly into the buffer.
 */
class CStringBuffer
{
private:

    /** Align our buffer to SSE data boundaries and
     * over-allocate by the same amount to allow for
     * fast (aligned) and sloppy (copying a bit too much)
     * data access.
     */
    enum {ALIGNMENT = sizeof (__m128)};

    /// the buffer that was allocated
    char* memory;

    /// the aligned start of our data buffer (memory + 0..15 bytes)
    char* buffer;

    /// length of the string (excluding the terminating 0)
    size_t size;

    /// maximum data capacity in buffer
    size_t capacity;

    // (re-)size management

    void Reserve (size_t newCapacity);
    void Append (const char* string, size_t length);

    // copying is not supported

    CStringBuffer (const CStringBuffer&);
    CStringBuffer& operator=(const CStringBuffer&);

public:

    /// construction
    CStringBuffer (size_t initialCapacity = 0);

    /// destruction
    ~CStringBuffer();

    /// data access
    operator char*();
    size_t GetSize() const;

    /// Get the first unused element in the buffer.
    /// Guarantee at least minFree bytes of addressible memory.
    char* GetBuffer (size_t minFree);

    /// mark additional size bytes of the buffer as used.
    /// (usually called after GetBuffer())
    void AddSize (size_t size);

    /// Set total size to 0. Keep internal buffer.
    void Clear();

    /// add data to buffer.
    void Append (char c);
    void Append (const std::string& s);
    void Append (const char* s);
};

inline CStringBuffer::operator char*()
{
    return buffer;
}

inline size_t CStringBuffer::GetSize() const
{
    return size;
}

inline char* CStringBuffer::GetBuffer (size_t minFree)
{
    if (size + minFree >= capacity)
        Reserve (2 * max (minFree, capacity));

    return buffer + size;
}

inline void CStringBuffer::AddSize (size_t size)
{
    assert (size + this->size < capacity);
    this->size += size;
    buffer[this->size] = 0;
}

inline void CStringBuffer::Clear()
{
    size = 0;
    buffer[0] = 0;
}

inline void CStringBuffer::Append (char c)
{
    if (size + 1 >= capacity)
        Reserve (2 * max (ALIGNMENT, capacity));

    buffer[size] = c;
    buffer[++size] = 0;
}
