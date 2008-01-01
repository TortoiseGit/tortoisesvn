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
#include "StdAfx.h"
#include ".\indexpairdictionary.h"

#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// CIndexPairDictionary::CHashFunction
///////////////////////////////////////////////////////////////

// simple construction

CIndexPairDictionary::CHashFunction::CHashFunction 
	( CIndexPairDictionary* aDictionary)
	: data (&aDictionary->data)
{
}

///////////////////////////////////////////////////////////////
// CIndexPairDictionary
///////////////////////////////////////////////////////////////

// construction / destruction

CIndexPairDictionary::CIndexPairDictionary(void)
	: hashIndex (CHashFunction (this))
{
}

CIndexPairDictionary::~CIndexPairDictionary(void)
{
}

// dictionary operations

void CIndexPairDictionary::reserve (index_t min_capacity)
{
	data.reserve (min_capacity);
	hashIndex.reserve (min_capacity);
}

index_t CIndexPairDictionary::Find (const std::pair<index_t, index_t>& value) const
{
	return hashIndex.find (value);
}

index_t CIndexPairDictionary::Insert (const std::pair<index_t, index_t>& value)
{
	assert (Find (value) == NO_INDEX);

	index_t result = (index_t)data.size();
	hashIndex.insert (value, (index_t)result);
	data.push_back (value);

	return result;
}

index_t CIndexPairDictionary::AutoInsert (const std::pair<index_t, index_t>& value)
{
	index_t result = Find (value);
	if (result == NO_INDEX)
		result = Insert (value);

	return result;
}

void CIndexPairDictionary::Clear()
{
	data.clear();
	hashIndex.clear();
}

void CIndexPairDictionary::Swap (CIndexPairDictionary& rhs)
{
	data.swap (rhs.data);
	hashIndex.swap (rhs.hashIndex);
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CIndexPairDictionary& dictionary)
{
	// read the first elements of all pairs

	CDiffIntegerInStream* firstStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CIndexPairDictionary::FIRST_STREAM_ID));

	index_t count = firstStream->GetValue();
	dictionary.data.resize (count);

	std::pair<index_t, index_t>* dataBegin 
		= count > 0 ? &dictionary.data.at(0) : NULL;

	for (index_t i = 0; i < count; ++i)
		(dataBegin + i)->first = firstStream->GetValue();

	// read the second elements

	CDiffIntegerInStream* secondStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CIndexPairDictionary::SECOND_STREAM_ID));

	for (index_t i = 0; i < count; ++i)
		(dataBegin + i)->second = secondStream->GetValue();

	// build the hash

	dictionary.hashIndex 
		= quick_hash<CIndexPairDictionary::CHashFunction>
			(CIndexPairDictionary::CHashFunction (&dictionary));
	dictionary.hashIndex.reserve (dictionary.data.size());

	for (index_t i = 0; i < count; ++i)
		dictionary.hashIndex.insert (*(dataBegin + i), i);

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CIndexPairDictionary& dictionary)
{
	size_t size = dictionary.data.size();

	// write string data

	CDiffIntegerOutStream* firstStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CIndexPairDictionary::FIRST_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));

	const std::pair<index_t, index_t>* dataBegin 
		= size > 0 ? &dictionary.data.at(0) : NULL;

	firstStream->Add ((int)size);
	for (size_t i = 0; i != size; ++i)
		firstStream->Add ((dataBegin + i)->first);

	// write offsets

	CDiffIntegerOutStream* secondStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CIndexPairDictionary::SECOND_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));

	for (size_t i = 0; i != size; ++i)
		secondStream->Add ((dataBegin + i)->second);

	// ready

	return stream;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}
