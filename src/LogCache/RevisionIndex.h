#pragma once

///////////////////////////////////////////////////////////////
// include
///////////////////////////////////////////////////////////////

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
//
// CRevisionIndex
//
//		a simple class to map from revision number to revision
//		index.
//
//		To handle high revision counts efficiently, only that
//		range of revisions is stored that actually maps to
//		revision indices (i.e. revisions at the upper and 
//		lower ends may be ommitted).
//
///////////////////////////////////////////////////////////////

class CRevisionIndex
{
private:

	// the mapping and its bias

	revision_t firstRevision;
	std::vector<index_t> indices;

	// sub-stream IDs

	enum
	{
		INDEX_STREAM_ID = 1
	};

public:

	// construction / destruction

	CRevisionIndex(void);
	~CRevisionIndex(void);

	// range of (possibly available) revisions:
	// GetFirstRevision() .. GetLastRevision()-1

	revision_t GetFirstRevision() const
	{
		return firstRevision;
	}
	revision_t GetLastRevision() const
	{
		return firstRevision + (revision_t)indices.size();
	}

	// read

	index_t operator[](revision_t revision) const
	{
		revision -= firstRevision;
		return revision < indices.size()
			? indices[revision]
			: NO_INDEX;
	}

	// insert info (must be NO_INDEX before)

	void SetRevisionIndex (revision_t revision, index_t index);

	// reset content

	void Clear();

	// stream I/O

	friend IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
											 , CRevisionIndex& container);
	friend IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
											  , const CRevisionIndex& container);
};

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CRevisionIndex& container);
IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CRevisionIndex& container);

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

