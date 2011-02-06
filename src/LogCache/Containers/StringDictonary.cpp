// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009, 2011 - TortoiseSVN

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
#include "StringDictonary.h"
#include "ContainerException.h"

#include "../Streams/BLOBInStream.h"
#include "../Streams/BLOBOutStream.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// CStringDictionary::CHashFunction
///////////////////////////////////////////////////////////////

// simple construction

CStringDictionary::CHashFunction::CHashFunction (CStringDictionary* aDictionary)
        : dictionary (aDictionary)
{
}

// the actual hash function

size_t
CStringDictionary::CHashFunction::operator()
(CStringDictionary::CHashFunction::value_type value) const
{
    if (value == NULL)
        return 0;

    size_t result = 0;
    while (*value)
    {
        result = result * 33 ^ *value;
        ++value;
    }

    return result;
}

// dictionary lookup

CStringDictionary::CHashFunction::value_type
CStringDictionary::CHashFunction::value
(CStringDictionary::CHashFunction::index_type index) const
{
    if (index == NO_INDEX)
        return NULL;

    assert (dictionary->offsets.size() > index+1);
    assert (dictionary->offsets[index] != NO_INDEX);

    return dictionary->packedStringsStart + dictionary->offsets[index];
}

// lookup and comparison

bool
CStringDictionary::CHashFunction::equal
(CStringDictionary::CHashFunction::value_type value
 , CStringDictionary::CHashFunction::index_type index) const
{
    // special case

    if (value == NULL)
        return index == 0;

    // ordinary string comparison

    const char* rhs = dictionary->packedStringsStart
                      + dictionary->offsets[index];
    return strcmp (value, rhs) == 0;
}

///////////////////////////////////////////////////////////////
// CStringDictionary
///////////////////////////////////////////////////////////////

// test for the worst effects of data corruption

void CStringDictionary::RebuildIndexes()
{
    // start of the string & offset arrays

    std::vector<index_t>::iterator begin = offsets.begin();

    // current position in string data (i.e. first char of the current string)

    index_t offset = 0;

    // hash & index all strings

    std::vector<const char*> temp;
    temp.reserve (offsets.size());

    for (index_t i = 0, count = (index_t) offsets.size()-1; i < count; ++i)
    {
        * (begin+i) = offset;
        temp.push_back (packedStringsStart + offset);

        offset += static_cast<index_t> (strlen (packedStringsStart + offset) +1);
    }

    hashIndex.clear();
    hashIndex.insert (temp.begin(), temp.end(), 0);

    // "end of table" entry

    offsets.back() = (index_t) packedStrings.size();
}

// construction utility

void CStringDictionary::Initialize()
{
    // insert the empty string at index 0

    packedStrings.reserve (sizeof (size_t) + 1);
    packedStrings.push_back (0);
    packedStringsStart = &packedStrings.front();
    offsets.push_back (0);
    offsets.push_back (1);
    hashIndex.insert ("", 0);
}

// construction / destruction

#pragma warning (push)
#pragma warning (disable:4355)

// passing 'this' during construction is fine here

CStringDictionary::CStringDictionary (void)
        : hashIndex (CHashFunction (this))
        , packedStringsStart (NULL)
{
    Initialize();
}

#pragma warning (pop)

CStringDictionary::~CStringDictionary (void)
{
}

// dictionary operations

void CStringDictionary::swap (CStringDictionary& rhs)
{
    packedStrings.swap (rhs.packedStrings);
    std::swap (packedStringsStart, rhs.packedStringsStart);
    offsets.swap (rhs.offsets);
    hashIndex.swap (rhs.hashIndex);
}

index_t CStringDictionary::Find (const char* string) const
{
    return hashIndex.find (string);
}

const char* CStringDictionary::operator[] (index_t index) const
{
#if !defined (_SECURE_SCL)
    if (index >= (index_t) offsets.size()-1)
        throw CContainerException ("dictionary string index out of range");
#endif

    return packedStringsStart + offsets[index];
}

index_t CStringDictionary::GetLength (index_t index) const
{
#if !defined (_SECURE_SCL)
    if (index >= (index_t) offsets.size()-1)
        throw CContainerException ("dictionary string index out of range");
#endif

    return offsets[index+1] - offsets[index] - 1;
}

char* CStringDictionary::CopyTo (char* target, index_t index) const
{
#if !defined (_SECURE_SCL)
    if (index >= (index_t) offsets.size()-1)
        throw CContainerException ("dictionary string index out of range");
#endif

    const char* first = packedStringsStart + offsets[index];
    const char* last = packedStringsStart + offsets[index+1] - 1;
    size_t count = last - first;

    memcpy (target, first, count);

    return target + count;
}

index_t CStringDictionary::Insert (const char* string)
{
    // must not exist yet (so, it is not empty as well)

    assert (Find (string) == NO_INDEX);
    assert (strlen (string) < NO_INDEX);

    // add string to container

    index_t size = index_t (strlen (string)) +1;
    if (packedStrings.size() >= (index_t)NO_INDEX - (size + sizeof (size_t)))
        throw CContainerException ("dictionary overflow");

    packedStrings.insert (packedStrings.end(), string, string + size);

    // ensure we can make chunky copy of any data from the packed buffer

    size_t minCapacity = packedStrings.size() + sizeof (size_t);
    if (packedStrings.capacity() < minCapacity)
    {
        if (packedStrings.size() > (index_t)NO_INDEX / 2 - sizeof (size_t))
            packedStrings.reserve ((index_t)NO_INDEX);
        else
            packedStrings.reserve (packedStrings.size() * 2 + sizeof (size_t));
    }

    packedStringsStart = &packedStrings.front();

    // update indices

    index_t result = (index_t) offsets.size()-1;
    hashIndex.insert (string, result);
    offsets.push_back ( (index_t) packedStrings.size());

    // ready

    return result;
}

index_t CStringDictionary::AutoInsert (const char* string)
{
    index_t result = Find (string);
    if (result == NO_INDEX)
        result = Insert (string);

    return result;
}

// return false if concurrent read accesses
// would potentially access invalid data.

bool CStringDictionary::CanInsertThreadSafely
    ( index_t elements
    , size_t chars) const
{
    return (packedStrings.size() + chars <= packedStrings.capacity())
        && (offsets.size() + elements <= offsets.capacity())
        && !hashIndex.may_cause_growth (elements);
}

// reset content

void CStringDictionary::Clear()
{
    packedStrings.clear();
    offsets.clear();
    hashIndex.clear();

    Initialize();
}

// use this to minimize re-allocation and re-hashing

void CStringDictionary::Reserve (index_t stringCount, size_t charCount)
{
    packedStrings.reserve (charCount + sizeof (size_t));
    packedStringsStart = &packedStrings.front();

    offsets.reserve (stringCount);
    hashIndex.reserve (stringCount);
}

// statistics

size_t CStringDictionary::GetPackedStringSize() const
{
    return packedStrings.size();
}

// "merge" with another container:
// add new entries and return ID mapping for source container

index_mapping_t CStringDictionary::Merge (const CStringDictionary& source)
{
    index_mapping_t result;
    result.insert ( (index_t) NO_INDEX, (index_t) NO_INDEX);

    for (index_t i = 0, count = source.size(); i < count; ++i)
        result.insert (i, AutoInsert (source[i]));

    return result;
}

// rearrange strings: put [sourceIndex[index]] into [index]

void CStringDictionary::Reorder (const std::vector<index_t>& sourceIndices)
{
    // we must remap all entries

    assert (size() == sourceIndices.size());

    // we must not remap the empty string

    assert (sourceIndices[0] == 0);

    // we will copy the string data in this temp. array

    std::vector<char> target;
    target.resize (packedStrings.size());

    // start of the string & offset arrays

    char* targetString = &target.front();

    // copy string by string

    for (index_t i = 0, count = size(); i < count; ++i)
    {
        index_t sourceIndex = sourceIndices[i];

        index_t sourceOffset = offsets[sourceIndex];
        index_t length = offsets[sourceIndex+1] - sourceOffset;

        memcpy (targetString, packedStringsStart + sourceOffset, length);
        targetString += length;
    }

    // the new order is now complete -> switch to it

    packedStrings.swap (target);
    packedStringsStart = &packedStrings.front();

    // re-build hash and offsets

    RebuildIndexes();
}

// stream I/O

IHierarchicalInStream& operator>> (IHierarchicalInStream& stream
                                   , CStringDictionary& dictionary)
{
    // read the string data

    CBLOBInStream* packedStringStream
        = stream.GetSubStream<CBLOBInStream>
            (CStringDictionary::PACKED_STRING_STREAM_ID);

    size_t packedSize = packedStringStream->GetSize();
    if (packedSize >= NO_INDEX)
        throw CContainerException ("data stream to large");

    dictionary.packedStrings.reserve (packedSize + sizeof (size_t));
    dictionary.packedStrings.resize (packedSize);
    dictionary.packedStringsStart = &dictionary.packedStrings.front();
    memcpy ( dictionary.packedStringsStart
           , packedStringStream->GetData()
           , dictionary.packedStrings.size());

    // build the hash and string offsets

    CDiffDWORDInStream* offsetsStream
        = stream.GetSubStream<CDiffDWORDInStream>
            (CStringDictionary::OFFSETS_STREAM_ID);

    size_t offsetCount = offsetsStream->GetSizeValue();
    dictionary.offsets.resize (offsetCount);

    dictionary.hashIndex
        = quick_hash<CStringDictionary::CHashFunction>
            (CStringDictionary::CHashFunction (&dictionary));
    dictionary.RebuildIndexes();

    // ready

    return stream;
}

IHierarchicalOutStream& operator<< (IHierarchicalOutStream& stream
                                    , const CStringDictionary& dictionary)
{
    // write string data

    CBLOBOutStream* packedStringStream
        = dynamic_cast<CBLOBOutStream*>
            (stream.OpenSubStream ( CStringDictionary::PACKED_STRING_STREAM_ID
                                  , BLOB_STREAM_TYPE_ID));
    packedStringStream->Add ( (const unsigned char*) &dictionary.packedStrings.front()
                              , dictionary.packedStrings.size());

    // write offsets

    CDiffDWORDOutStream* offsetsStream
        = dynamic_cast<CDiffDWORDOutStream*>
            (stream.OpenSubStream ( CStringDictionary::OFFSETS_STREAM_ID
                                  , DIFF_DWORD_STREAM_TYPE_ID));
    offsetsStream->AddSizeValue (dictionary.offsets.size());

    // ready

    return stream;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}
