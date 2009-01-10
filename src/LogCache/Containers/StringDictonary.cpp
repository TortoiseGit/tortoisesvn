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
#include "StdAfx.h"
#include ".\stringdictonary.h"

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
	( CStringDictionary::CHashFunction::value_type value
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

	hashIndex.clear();
	hashIndex.reserve (offsets.size());

	for (index_t i = 0, count = (index_t)offsets.size()-1; i < count; ++i)
    {
        *(begin+i) = offset;
		hashIndex.insert (packedStringsStart + offset, i);

        offset += static_cast<index_t>(strlen (packedStringsStart + offset) +1);
    }

    // "end of table" entry

  	*offsets.rbegin() = (index_t)packedStrings.size();
}

// construction utility

void CStringDictionary::Initialize()
{
	// insert the empty string at index 0

	packedStrings.push_back (0);
    packedStringsStart = &packedStrings.at(0);
	offsets.push_back (0);
	offsets.push_back (1);
	hashIndex.insert ("", 0);
}

// construction / destruction

CStringDictionary::CStringDictionary(void)
	: hashIndex (CHashFunction (this))
    , packedStringsStart (NULL)
{
	Initialize();
}

CStringDictionary::~CStringDictionary(void)
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

const char* CStringDictionary::operator[](index_t index) const
{
#if !defined (_SECURE_SCL)
	if (index+1 < (index_t)offsets.size())
		throw std::exception ("dictionary string index out of range");
#endif

	return packedStringsStart + offsets[index];
}

index_t CStringDictionary::GetLength (index_t index) const
{
#if !defined (_SECURE_SCL)
	if (index+1 < (index_t)offsets.size())
		throw std::exception ("dictionary string index out of range");
#endif
		
	return offsets[index+1] - offsets[index] - 1;
}

index_t CStringDictionary::Insert (const char* string)
{
	// must not exist yet (so, it is not empty as well)

	assert (Find (string) == NO_INDEX);
	assert (strlen (string) < NO_INDEX);

	// add string to container

	index_t size = index_t (strlen (string)) +1;
	if (packedStrings.size() >= NO_INDEX - size)
		throw std::exception ("dictionary overflow");

	packedStrings.insert (packedStrings.end(), string, string + size);
    packedStringsStart = &packedStrings.at(0);

	// update indices

	index_t result = (index_t)offsets.size()-1;
	hashIndex.insert (string, result);
	offsets.push_back ((index_t)packedStrings.size());

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

// reset content

void CStringDictionary::Clear()
{
	packedStrings.clear();
	offsets.clear();
	hashIndex.clear();

	Initialize();
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
	result.insert ((index_t)NO_INDEX, (index_t)NO_INDEX);

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

	char* targetString = &target.at(0);

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
    packedStringsStart = &packedStrings.at(0);

	// re-build hash and offsets

	RebuildIndexes();
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CStringDictionary& dictionary)
{
	// read the string data

	CBLOBInStream* packedStringStream 
		= dynamic_cast<CBLOBInStream*>
			(stream.GetSubStream (CStringDictionary::PACKED_STRING_STREAM_ID));

	if (packedStringStream->GetSize() >= NO_INDEX)
		throw std::exception ("data stream to large");

	dictionary.packedStrings.resize (packedStringStream->GetSize());
    dictionary.packedStringsStart = &dictionary.packedStrings.at(0);
	memcpy ( dictionary.packedStringsStart
		   , packedStringStream->GetData()
		   , dictionary.packedStrings.size());

	// build the hash and string offsets

	CDiffDWORDInStream* offsetsStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CStringDictionary::OFFSETS_STREAM_ID));

    size_t offsetCount = offsetsStream->GetSizeValue();
    dictionary.offsets.resize (offsetCount);

	dictionary.hashIndex 
		= quick_hash<CStringDictionary::CHashFunction>
			(CStringDictionary::CHashFunction (&dictionary));
	dictionary.hashIndex.reserve (dictionary.offsets.size());

    dictionary.RebuildIndexes();

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CStringDictionary& dictionary)
{
	// write string data

	CBLOBOutStream* packedStringStream 
		= dynamic_cast<CBLOBOutStream*>
			(stream.OpenSubStream ( CStringDictionary::PACKED_STRING_STREAM_ID
								  , BLOB_STREAM_TYPE_ID));
	packedStringStream->Add ((const unsigned char*)&dictionary.packedStrings.at(0)
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
