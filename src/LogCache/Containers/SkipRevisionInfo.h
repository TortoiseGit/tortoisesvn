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
#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "QuickHash.h"
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

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CPathDictionary;
class CDictionaryBasedPath;
class CRevisionIndex;
class CRevisionInfoContainer;

/**
 * associates paths with a list of revision ranges. It will be used to store 
 * those ranges that definitely contain no log information for the respective 
 * path. Hence, even if we have no cached log information for that range at all,
 * we don't need to ask the server for the log.
 *
 * Compress() should be called to reduce the number of entries in this data 
 * structure. CRevisionInfoContainer has been designed to allow for very fast 
 * scanning - eliminating the need to use CSkipRevisionInfo as an optimization.
 */
class CSkipRevisionInfo
{
private:

	/**
	 * structure that stores a number of revision ranges
	 * (start, size) to a given path ID.
	 *
	 * Revision ranges must not overlap.
	 *
	 * Convenience methods for adding and retrieving
	 * ranges are provided.
	 */

	struct SPerPathRanges
	{
		typedef std::map<revision_t, revision_t> TRanges;
		TRanges ranges;
		index_t pathID;

		/// find next / previous "gap"

		revision_t FindNext (revision_t revision) const;
		revision_t FindPrevious (revision_t revision) const;

		/// update / insert range

		void Add (revision_t start, revision_t size);
	};

	typedef SPerPathRanges::TRanges::iterator IT;

	/**
	 * simple quick_hash<> hash function that maps pathIDs
	 * onto vector indices.
	 */

	class CHashFunction
	{
	private:

		/// the dictionary we index with the hash
		/// (used to map index -> value)

		std::vector<SPerPathRanges*>* data;

	public:

		/// simple construction

		CHashFunction (std::vector<SPerPathRanges*>* aContainer)
			: data (aContainer)
		{
		}

		/// required typedefs and constants

		typedef index_t value_type;
		typedef index_t index_type;

		enum {NO_INDEX = LogCache::NO_INDEX};

		/// the actual hash function

		size_t operator() (const value_type& value) const
		{
			return value;
		}

		/// dictionary lookup

		const value_type& value (index_type index) const
		{
			assert (data->size() >= index);
			return (*data)[index]->pathID;
		}

		/// lookup and comparison

		bool equal (const value_type& value, index_type index) const
		{
			assert (data->size() > index);
			return (*data)[index]->pathID == value;
		}
	};

	friend class CHashFunction;

	/**
	 * utility class to remove duplicate ranges (parent paths)
	 * as well as ranges now covered by cached revision info.
	 */

	class CPacker
	{
	private:

		CSkipRevisionInfo* parent;
		std::vector<IT> allRanges;

		/// individual compression steps

		index_t RemoveParentRanges();
		void SortRanges (index_t rangeCount);
		void RemoveKnownRevisions();
		void RemoveEmptyRanges();
        void RebuildHash();

	public:

		/// construction / destruction

		CPacker();
		~CPacker();

		/// compress

		void operator()(CSkipRevisionInfo* aParent);
	};

	friend class CPacker;

	/**
	 * predicate to order ranges by their start revisions.
	 */

	class CIterComp
	{
	public:

		bool operator() (const IT& lhs, const IT& rhs) const
		{
			return lhs->first < rhs->first;
		}
	};

	enum
	{
		PATHIDS_STREAM_ID = 1 ,
		ENTRY_COUNT_STREAM_ID = 2,
		REVISIONS_STREAM_ID = 3,
		SIZES_STREAM_ID = 4
	};

	std::vector<SPerPathRanges*> data;
	quick_hash<CHashFunction> index;

	const CPathDictionary& paths;
	const CRevisionIndex& revisions;
    const CRevisionInfoContainer& logInfo;

	/// remove known revisions from the range

    bool DataAvailable (revision_t revision);
	void TryReduceRange (revision_t& revision, revision_t& size);

public:

	/// construction / destruction

	CSkipRevisionInfo ( const CPathDictionary& aPathDictionary
					  , const CRevisionIndex& aRevisionIndex
                      , const CRevisionInfoContainer& logInfo);
	~CSkipRevisionInfo(void);

    /// provide custom assignment operator to silence C4512

    CSkipRevisionInfo& operator=(const CSkipRevisionInfo& rhs);

	/// query data (return NO_REVISION, if not found)

	revision_t GetNextRevision (const CDictionaryBasedPath& path, revision_t revision) const;
	revision_t GetPreviousRevision (const CDictionaryBasedPath& path, revision_t revision) const;

	/// add / remove data

	void Add (const CDictionaryBasedPath& path, revision_t revision, revision_t size);
	void Clear();

	/// remove unnecessary entries

	void Compress();

	/// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CSkipRevisionInfo& container);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CSkipRevisionInfo& container);

	/// for statistics

	friend class CLogCacheStatistics;
};

/// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CSkipRevisionInfo& container);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CSkipRevisionInfo& container);

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

