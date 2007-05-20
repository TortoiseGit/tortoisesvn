// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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

#include "BLOBInStream.h"
#include "BLOBOutStream.h"
#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"

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

	return &dictionary->packedStrings.at(0) + dictionary->offsets[index];
}

// lookup and comparison

bool 
CStringDictionary::CHashFunction::equal 
	( CStringDictionary::CHashFunction::value_type value
	, CStringDictionary::CHashFunction::index_type index) const
{
	// special cases

	if (index == NO_INDEX)
		return (value == NULL) || (*value == 0);

	if (value == NULL)
		return index == 0;

	// ordinary string comparison

	const char* rhs = &dictionary->packedStrings.at(0) 
					+ dictionary->offsets[index];
	return strcmp (value, rhs) == 0;
}

///////////////////////////////////////////////////////////////
// CStringDictionary
///////////////////////////////////////////////////////////////

// test for the worst effects of data corruption

void CStringDictionary::CheckOffsets()
{
	// index 0 must point to an empty string

	if (packedStrings.empty() || (offsets.size() < 2))
		throw std::exception ("dictionary must never be empty");
	if ((packedStrings[0] != 0) || (offsets[0] != 0) || (offsets[1] != 1))
		throw std::exception ("dictionary[0] must be the empty string");

	// all offsets must be strictly monotonously increasing

	for (size_t i = 1, count = offsets.size(); i < count; ++i)
		if (offsets[i-1] >= offsets[i])
			throw std::exception ("dictionary entries must have positive lengths");

	// sizes must be consistent

	if (*offsets.rbegin() != packedStrings.size())
		throw std::exception ("dictionary entries size mismatch");

	size_t actualStringCount 
		= std::count (packedStrings.begin(), packedStrings.end(), 0);
	if (actualStringCount + 1 != offsets.size())
		throw std::exception ("dictionary string count mismatch");

	// every offset must point to the begin of some string

	for (size_t i = 1, count = offsets.size(); i < count; ++i)
		if (packedStrings [offsets[i]-1] != 0)
			throw std::exception ("dictionary entry does not point to a string");
}

// construction utility

void CStringDictionary::Initialize()
{
	// insert the empty string at index 0

	packedStrings.push_back (0);
	offsets.push_back (0);
	offsets.push_back (1);
}

// construction / destruction

CStringDictionary::CStringDictionary(void)
	: hashIndex (CHashFunction (this))
{
	Initialize();
}

CStringDictionary::~CStringDictionary(void)
{
}

// dictionary operations

index_t CStringDictionary::Find (const char* string) const
{
	return hashIndex.find (string);
}

const char* CStringDictionary::operator[](index_t index) const
{
	if (index+1 >= (index_t)offsets.size())
		throw std::exception ("dictionary string index out of range");

	return &packedStrings.at(0) + offsets[index];
}

index_t CStringDictionary::GetLength (index_t index) const
{
	if (index+1 >= (index_t)offsets.size())
		throw std::exception ("dictionary string index out of range");

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
	memcpy ( &dictionary.packedStrings.at(0)
		   , packedStringStream->GetData()
		   , dictionary.packedStrings.size());

	// read the string offsets

	CDiffDWORDInStream* offsetsStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CStringDictionary::OFFSETS_STREAM_ID));

	*offsetsStream >> dictionary.offsets;

	// check against the worst effects of data corruption

//	dictionary.CheckOffsets();

	// build the hash (ommit the empty string at index 0)

	dictionary.hashIndex 
		= quick_hash<CStringDictionary::CHashFunction>
			(CStringDictionary::CHashFunction (&dictionary));
	dictionary.hashIndex.reserve (dictionary.offsets.size());

	const char* stringBase = &dictionary.packedStrings.at(0);
	for (index_t i = 1, count = (index_t)dictionary.offsets.size()-1; i < count; ++i)
		dictionary.hashIndex.insert ( stringBase + dictionary.offsets[i]
									, i);

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
	*offsetsStream << dictionary.offsets;

	// ready

	return stream;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}
