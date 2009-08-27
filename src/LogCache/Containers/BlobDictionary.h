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
 * generalized string data
 */

struct SBlob
{
    const char* data;
    size_t size;

    // construction

    SBlob ()
        : data (NULL), size (0) {}

    SBlob (const char* data, size_t size)
        : data (data), size (size) {}

    SBlob (const std::string& rhs)
        : data (rhs.c_str()), size (rhs.size()) {}

    // comparison

    bool operator==(const SBlob& rhs) const
    {
        return (size == rhs.size) 
            && ((size == 0) || (memcmp (data, rhs.data, size) == 0));
    }

    bool operator!=(const SBlob& rhs) const
    {
        return !operator==(rhs);
    }

    bool operator<(const SBlob& rhs) const
    {
        if (size == 0)
            return rhs.size != 0;

        size_t minSize = std::min (size, rhs.size);
        int diff = memcmp (data, rhs.data, minSize);
        if (diff != 0)
            return diff < 0;

        return size < rhs.size;
    }

    bool operator>=(const SBlob& rhs) const
    {
        return !operator<(rhs);
    }

    bool operator>(const SBlob& rhs) const
    {
        return rhs.operator<(*this);
    }

    bool operator<=(const SBlob& rhs) const
    {
        return !rhs.operator<(*this);
    }

    // string operations

    bool StartsWith (const SBlob& rhs) const
    {
        return (size >= rhs.size) 
            && (memcmp (data, rhs.data, rhs.size) == 0);
    }

    bool StartsWith (const std::string& rhs) const
    {
        return (size >= rhs.size()) 
            && (memcmp (data, rhs.c_str(), rhs.size()) == 0);
    }
};

/**
 * efficiently stores a pool of unique blobs.
 * Each string is assigned an unique, immutable index.
 *
 * Under most circumstances, O(1) lookup is provided.
 */
class CBlobDictionary
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

		CBlobDictionary* dictionary;

	public:

		/// simple construction

		CHashFunction (CBlobDictionary* aDictionary);

		/// required typedefs and constants

		typedef SBlob value_type;
		typedef index_t index_type;

		enum {NO_INDEX = LogCache::NO_INDEX};

		/// the actual hash function

		size_t operator() (const value_type& value) const;

		/// dictionary lookup

		value_type value (index_type index) const;

		/// lookup and comparison

		bool equal (const value_type& value, index_type index) const;
	};

	/// sub-stream IDs

	enum
	{
		PACKED_BLOBS_STREAM_ID = 1,
		OFFSETS_STREAM_ID = 2
	};

	/// the string data

	std::vector<char> packedBlobs;
	std::vector<index_t> offsets;

    /// equivalent to &packedStrings.front()

    char* packedBlobsStart;

	/// the string index

	quick_hash<CHashFunction> hashIndex;

	friend class CHashFunction;

	/// test for the worst effects of data corruption

	void RebuildIndexes();

	/// construction utility

	void Initialize();

public:

	/// construction / destruction

	CBlobDictionary(void);
	virtual ~CBlobDictionary(void);

	/// dictionary operations

	index_t size() const
	{
		return (index_t)(offsets.size()-1);
	}

	SBlob operator[](index_t index) const;

    void swap (CBlobDictionary& rhs);

	index_t Find (const SBlob& string) const;
	index_t Insert (const SBlob& string);
	index_t AutoInsert (const SBlob& string);

	/// reset content

	void Clear();

    /// use this to minimize re-allocation and re-hashing

    void Reserve (index_t blobCount, size_t byteCount);

    /// statistics

    size_t GetPackedBlobsSize() const;

	/// "merge" with another container:
	/// add new entries and return ID mapping for source container

	index_mapping_t Merge (const CBlobDictionary& source);

	/// rearrange strings: put [sourceIndex[index]] into [index]

	void Reorder (const std::vector<index_t>& sourceIndices);

	/// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CBlobDictionary& dictionary);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CBlobDictionary& dictionary);

	/// for statistics

	friend class CLogCacheStatistics;
};

/// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CBlobDictionary& dictionary);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CBlobDictionary& dictionary);

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

