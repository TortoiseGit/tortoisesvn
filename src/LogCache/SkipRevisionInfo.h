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

///////////////////////////////////////////////////////////////
//
// CSkipRevisionInfo
//
//		associates paths with a list of revision ranges. It will
//		be used to store those ranges that definitely contain no
//		log information for the respective path. Hence, even if
//		we have no cached log information for that range at all,
//		we don't need to ask the server for the log.
//
//		Compress() should be called to reduce the number of 
//		entries in this data structure. CRevisionInfoContainer 
//		has been designed to allow for very fast scanning -
//		eliminating the need to use CSkipRevisionInfo as an
//		optimization.
//
///////////////////////////////////////////////////////////////

class CSkipRevisionInfo
{
private:

	///////////////////////////////////////////////////////////////
	//
	// SPerPathRanges
	//
	//		structure that stores a number of revision ranges
	//		(start, size) to a given path ID.
	//
	//		Revision ranges must not overlap.
	//
	//		Convenience methods for adding and retrieving
	//		ranges are provided.
	//
	///////////////////////////////////////////////////////////////

	struct SPerPathRanges
	{
		typedef std::map<revision_t, revision_t> TRanges;
		TRanges ranges;
		index_t pathID;

		// find next / previous "gap"

		revision_t FindNext (revision_t revision) const;
		revision_t FindPrevious (revision_t revision) const;

		// update / insert range

		void Add (revision_t start, revision_t size);
	};

	typedef SPerPathRanges::TRanges::iterator IT;

	///////////////////////////////////////////////////////////////
	//
	// CHashFunction
	//
	//		simple quick_hash<> hash function that maps pathIDs
	//		onto vector indices.
	//
	///////////////////////////////////////////////////////////////

	class CHashFunction
	{
	private:

		// the dictionary we index with the hash
		// (used to map index -> value)

		std::vector<SPerPathRanges*>* data;

	public:

		// simple construction

		CHashFunction (std::vector<SPerPathRanges*>* aContainer)
			: data (aContainer)
		{
		}

		// required typedefs and constants

		typedef index_t value_type;
		typedef index_t index_type;

		enum {NO_INDEX = LogCache::NO_INDEX};

		// the actual hash function

		size_t operator() (const value_type& value) const
		{
			return value;
		}

		// dictionary lookup

		const value_type& value (index_type index) const
		{
			assert (data->size() >= index);
			return (*data)[index]->pathID;
		}

		// lookup and comparison

		bool equal (const value_type& value, index_type index) const
		{
			assert (data->size() > index);
			return (*data)[index]->pathID == value;
		}
	};

	friend class CHashFunction;

	///////////////////////////////////////////////////////////////
	//
	// CPacker
	//
	//		utility class to remove duplicate ranges (parent paths)
	//		as well as ranges now covered by cached revision info.
	//
	///////////////////////////////////////////////////////////////

	class CPacker
	{
	private:

		CSkipRevisionInfo* parent;
		std::vector<IT> allRanges;

		// individual compression steps

		index_t RemoveParentRanges();
		void SortRanges (index_t rangeCount);
		void RemoveKnownRevisions();
		void RemoveEmptyRanges();

	public:

		// construction / destruction

		CPacker();
		~CPacker();

		// compress

		void operator()(CSkipRevisionInfo* aParent);
	};

	friend class CPacker;

	///////////////////////////////////////////////////////////////
	//
	// CIterComp
	//
	//		predicate to order ranges by their start revisions.
	//
	///////////////////////////////////////////////////////////////

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

	// remove known revisions from the range

	void TryReduceRange (revision_t& revision, revision_t& size);

public:

	// construction / destruction

	CSkipRevisionInfo ( const CPathDictionary& aPathDictionary
					  , const CRevisionIndex& aRevisionIndex);
	~CSkipRevisionInfo(void);

	// query data (return NO_REVISION, if not found)

	revision_t GetNextRevision (const CDictionaryBasedPath& path, revision_t revision) const;
	revision_t GetPreviousRevision (const CDictionaryBasedPath& path, revision_t revision) const;

	// add / remove data

	void Add (const CDictionaryBasedPath& path, revision_t revision, revision_t size);
	void Clear();

	// remove unnecessary entries

	void Compress();

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CSkipRevisionInfo& container);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CSkipRevisionInfo& container);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CSkipRevisionInfo& container);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CSkipRevisionInfo& container);

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

