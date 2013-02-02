// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009, 2011, 2013 - TortoiseSVN

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
#include "BlobDictionary.h"
#include "ContainerException.h"

#include "../Streams/BLOBInStream.h"
#include "../Streams/BLOBOutStream.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// CBlobDictionary::CHashFunction
///////////////////////////////////////////////////////////////

// simple construction

CBlobDictionary::CHashFunction::CHashFunction (CBlobDictionary* aDictionary)
        : dictionary (aDictionary)
{
}

// the actual hash function

size_t
CBlobDictionary::CHashFunction::operator()
(const CBlobDictionary::CHashFunction::value_type& value) const
{
    if (value.data == NULL)
        return 0;

    size_t result = 0;
    for (const char* iter = value.data, *end = value.data + value.size
            ; iter != end
            ; ++iter)
    {
        result = result * 33 ^ *iter;
    }

    return result;
}

// dictionary lookup

CBlobDictionary::CHashFunction::value_type
CBlobDictionary::CHashFunction::value
(CBlobDictionary::CHashFunction::index_type index) const
{
    if (index == NO_INDEX)
        return value_type (NULL, 0);

    assert (dictionary->offsets.size() > index+1);
    assert (dictionary->offsets[index] != NO_INDEX);

    size_t startOffset = dictionary->offsets[index];
    size_t size = dictionary->offsets[index+1] - startOffset;
    return value_type (dictionary->packedBlobsStart + startOffset, size);
}

// lookup and comparison

bool
CBlobDictionary::CHashFunction::equal
(const CBlobDictionary::CHashFunction::value_type& value
 , CBlobDictionary::CHashFunction::index_type index) const
{
    // special case

    if (value.data == NULL)
        return index == 0;

    // sizes equal?

    size_t startOffset = dictionary->offsets[index];
    if (dictionary->offsets[index+1] - startOffset != value.size)
        return false;

    // compare memory ranges

    const char* rhs = dictionary->packedBlobsStart + startOffset;
    return memcmp (value.data, rhs, value.size) == 0;
}

///////////////////////////////////////////////////////////////
// CBlobDictionary
///////////////////////////////////////////////////////////////

// test for the worst effects of data corruption

void CBlobDictionary::RebuildIndexes()
{
    // current position in blob data (i.e. first char of the current string)

    index_t offset = 0;

    // hash & index all strings

    hashIndex.clear();
    hashIndex.reserve (offsets.size());

    index_t index = 0;
    for (std::vector<index_t>::iterator iter = offsets.begin() +1
            , end = offsets.end()
                    ; iter != end
            ; ++iter, ++index)
    {
        index_t size = *iter - offset;
        hashIndex.insert (SBlob (packedBlobsStart + offset, size), index);

        offset += size;
    }
}

// construction utility

void CBlobDictionary::Initialize()
{
    // insert the empty string at index 0

    packedBlobsStart = NULL;
    offsets.push_back (0);
    offsets.push_back (0);
    hashIndex.insert (SBlob (NULL, 0), 0);
}

// construction / destruction

#pragma warning (push)
#pragma warning (disable:4355)

// passing 'this' during construction is fine here

CBlobDictionary::CBlobDictionary (void)
    : hashIndex (CHashFunction (this))
    , packedBlobsStart (NULL)
{
    Initialize();
}

#pragma warning (pop)

CBlobDictionary::~CBlobDictionary (void)
{
}

// dictionary operations

void CBlobDictionary::swap (CBlobDictionary& rhs)
{
    packedBlobs.swap (rhs.packedBlobs);
    std::swap (packedBlobsStart, rhs.packedBlobsStart);
    offsets.swap (rhs.offsets);
    hashIndex.swap (rhs.hashIndex);
}

index_t CBlobDictionary::Find (const SBlob& blob) const
{
    return hashIndex.find (blob);
}

SBlob CBlobDictionary::operator[] (index_t index) const
{
#ifdef _DEBUG
    if (index+1 < (index_t) offsets.size())
        throw CContainerException ("dictionary string index out of range");
#endif

    size_t startOffset = offsets[index];
    size_t size = offsets[index+1] - startOffset;

    return SBlob (packedBlobsStart + startOffset, size);
}

index_t CBlobDictionary::Insert (const SBlob& blob)
{
    // must not exist yet (so, it is not empty as well)

    assert (Find (blob) == NO_INDEX);
    assert (blob.size < NO_INDEX);

    // add string to container

    if (packedBlobs.size() >= NO_INDEX - blob.size)
        throw CContainerException ("dictionary overflow");

    packedBlobs.insert (packedBlobs.end(), blob.data, blob.data + blob.size);
    packedBlobsStart = &packedBlobs.front();

    // update indices

    index_t result = (index_t) offsets.size()-1;
    hashIndex.insert (blob, result);
    offsets.push_back ( (index_t) packedBlobs.size());

    // ready

    return result;
}

index_t CBlobDictionary::AutoInsert (const SBlob& blob)
{
    index_t result = Find (blob);
    if (result == NO_INDEX)
        result = Insert (blob);

    return result;
}

// reset content

void CBlobDictionary::Clear()
{
    packedBlobs.clear();
    offsets.clear();
    hashIndex.clear();

    Initialize();
}

// use this to minimize re-allocation and re-hashing

void CBlobDictionary::Reserve (index_t blobCount, size_t byteCount)
{
    packedBlobs.reserve (byteCount);
    packedBlobsStart = packedBlobs.empty() ? NULL : &packedBlobs.front();

    offsets.reserve (blobCount);
    hashIndex.reserve (blobCount);
}

// statistics

size_t CBlobDictionary::GetPackedBlobsSize() const
{
    return packedBlobs.size();
}

// "merge" with another container:
// add new entries and return ID mapping for source container

index_mapping_t CBlobDictionary::Merge (const CBlobDictionary& source)
{
    index_mapping_t result;
    result.insert ( (index_t) NO_INDEX, (index_t) NO_INDEX);

    for (index_t i = 0, count = source.size(); i < count; ++i)
        result.insert (i, AutoInsert (source[i]));

    return result;
}

// rearrange strings: put [sourceIndex[index]] into [index]

void CBlobDictionary::Reorder (const std::vector<index_t>& sourceIndices)
{
    // we must remap all entries

    assert (size() == sourceIndices.size());

    // we must not remap the empty string

    assert (sourceIndices[0] == 0);

    // we will copy the string data in this temp. array

    std::vector<char> target;
    target.resize (packedBlobs.size());

    std::vector<index_t> targetOffsets;
    targetOffsets.resize (offsets.size());

    // start of the string & offset arrays

    char* targetBlob = &target.front();
    index_t targetOffset = 0;

    // copy string by string

    for (index_t i = 0, count = size(); i < count; ++i)
    {
        index_t sourceIndex = sourceIndices[i];

        index_t sourceOffset = offsets[sourceIndex];
        index_t length = offsets[sourceIndex+1] - sourceOffset;

        memcpy (targetBlob, packedBlobsStart + sourceOffset, length);
        targetOffsets[i] = targetOffset;
        targetOffset += length;
        targetBlob += length;
    }

    targetOffsets.back() = targetOffset;

    // the new order is now complete -> switch to it

    packedBlobs.swap (target);
    offsets.swap (targetOffsets);

    // re-build hash and offsets

    RebuildIndexes();
}

// stream I/O

IHierarchicalInStream& operator>> (IHierarchicalInStream& stream
                                   , CBlobDictionary& dictionary)
{
    // read the string data

    CBLOBInStream* packedBlobsStream
        = stream.GetSubStream<CBLOBInStream>
            (CBlobDictionary::PACKED_BLOBS_STREAM_ID);

    if (packedBlobsStream->GetSize() >= NO_INDEX)
        throw CContainerException ("data stream to large");

    dictionary.packedBlobs.resize (packedBlobsStream->GetSize());
    dictionary.packedBlobsStart = &dictionary.packedBlobs.front();
    memcpy ( dictionary.packedBlobsStart
           , packedBlobsStream->GetData()
           , dictionary.packedBlobs.size());

    // build the hash and string offsets

    CDiffDWORDInStream* offsetsStream
        = stream.GetSubStream<CDiffDWORDInStream> (CBlobDictionary::OFFSETS_STREAM_ID);

    *offsetsStream >> dictionary.offsets;

    dictionary.hashIndex
        = quick_hash<CBlobDictionary::CHashFunction>
            (CBlobDictionary::CHashFunction (&dictionary));
    dictionary.hashIndex.reserve (dictionary.offsets.size());

    dictionary.RebuildIndexes();

    // ready

    return stream;
}

IHierarchicalOutStream& operator<< (IHierarchicalOutStream& stream
                                    , const CBlobDictionary& dictionary)
{
    // write string data

    CBLOBOutStream* packedBlobsStream
        = stream.OpenSubStream<CBLOBOutStream>
            (CBlobDictionary::PACKED_BLOBS_STREAM_ID);
    packedBlobsStream->Add ( (const unsigned char*) &dictionary.packedBlobs.front()
                           , dictionary.packedBlobs.size());

    // write offsets

    CDiffDWORDOutStream* offsetsStream
        = stream.OpenSubStream<CDiffDWORDOutStream>
            (CBlobDictionary::OFFSETS_STREAM_ID);
    *offsetsStream << dictionary.offsets;

    // ready

    return stream;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}
