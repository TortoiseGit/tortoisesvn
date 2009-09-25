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
#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "../../Utils/QuickHashMap.h"
#include "LogCacheGlobals.h"

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class IHierarchicalInStream;
class IHierarchicalOutStream;

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

/**
 * efficient associative container
 */

typedef quick_hash_map<index_t, index_t> index_mapping_t;

/**
 * efficiently stores a pool of unique (UTF8) strings.
 * Each string is assigned an unique, immutable index.
 *
 * Under most circumstances, O(1) lookup is provided.
 */
class CStringDictionary
{
private:

	/**
	 * A simple string hash function that satisfies quick_hash'
	 * interface requirements.
	 * 
	 * NULL strings are supported and are mapped to index 0.
	 * Hence, the dictionary must contain the empty string at
	 * index 0.
	 */

	class CHashFunction
	{
	private:

		/// the dictionary we index with the hash
		/// (used to map index -> value)

		CStringDictionary* dictionary;

	public:

		/// simple construction

		CHashFunction (CStringDictionary* aDictionary);

		/// required typedefs and constants

		typedef const char* value_type;
		typedef index_t index_type;

		enum {NO_INDEX = LogCache::NO_INDEX};

		/// the actual hash function

		size_t operator() (value_type value) const;

		/// dictionary lookup

		value_type value (index_type index) const;

		/// lookup and comparison

		bool equal (value_type value, index_type index) const;
	};

	/// sub-stream IDs

	enum
	{
		PACKED_STRING_STREAM_ID = 1,
		OFFSETS_STREAM_ID = 2
	};

	/// the string data

	std::vector<char> packedStrings;
	std::vector<index_t> offsets;

    /// equivalent to &packedStrings.front()

    char* packedStringsStart;

	/// the string index

	quick_hash<CHashFunction> hashIndex;

	friend class CHashFunction;

	/// test for the worst effects of data corruption

	void RebuildIndexes();

	/// construction utility

	void Initialize();

public:

	/// construction / destruction

	CStringDictionary(void);
	virtual ~CStringDictionary(void);

	/// dictionary operations

	index_t size() const
	{
		return (index_t)(offsets.size()-1);
	}

	const char* operator[](index_t index) const;
	index_t GetLength (index_t index) const;
    char* CopyTo (char* target, index_t index) const;

    void swap (CStringDictionary& rhs);

	index_t Find (const char* string) const;
	index_t Insert (const char* string);
	index_t AutoInsert (const char* string);

    /// return false if concurrent read accesses
    /// would potentially access invalid data.

    bool CanInsertThreadSafely (index_t elements, size_t chars) const;

	/// reset content

	void Clear();

    /// use this to minimize re-allocation and re-hashing

    void Reserve (index_t stringCount, size_t charCount);

    /// statistics

    size_t GetPackedStringSize() const;

	/// "merge" with another container:
	/// add new entries and return ID mapping for source container

	index_mapping_t Merge (const CStringDictionary& source);

	/// rearrange strings: put [sourceIndex[index]] into [index]

	void Reorder (const std::vector<index_t>& sourceIndices);

	/// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CStringDictionary& dictionary);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CStringDictionary& dictionary);

	/// for statistics

	friend class CLogCacheStatistics;
};

/// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CStringDictionary& dictionary);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CStringDictionary& dictionary);

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

