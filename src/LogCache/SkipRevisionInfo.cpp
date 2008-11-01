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
#include ".\skiprevisioninfo.h"

#include ".\RevisionIndex.h"
#include ".\PathDictionary.h"
#include ".\RevisionInfoContainer.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// CSkipRevisionInfo::SPerPathRanges
///////////////////////////////////////////////////////////////
// find next / previous "gap"
///////////////////////////////////////////////////////////////

revision_t CSkipRevisionInfo::SPerPathRanges::FindNext (revision_t revision) const
{
	// special case

	if (ranges.empty())
		return (revision_t)NO_REVISION;

	// look for the first range *behind* revision

	TRanges::const_iterator iter = ranges.upper_bound (revision);
	if (iter == ranges.begin())
		return (revision_t)NO_REVISION;

	// return end of previous range, if revision is within this range

	--iter;
	revision_t next = iter->first + iter->second;
	return next <= revision 
		? NO_REVISION
		: next;
}

revision_t CSkipRevisionInfo::SPerPathRanges::FindPrevious (revision_t revision) const
{
	// special case

	if (ranges.empty())
		return (revision_t)NO_REVISION;

	// look for the first range *behind* revision

	TRanges::const_iterator iter = ranges.upper_bound (revision);
	if (iter == ranges.begin())
		return (revision_t)NO_REVISION;

	// return start-1 of previous range, if revision is within this range

	--iter;
	revision_t next = iter->first + iter->second;
	return next <= revision 
		? NO_REVISION
		: iter->first-1;
}

///////////////////////////////////////////////////////////////
// update / insert range
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::SPerPathRanges::Add (revision_t start, revision_t size)
{
	revision_t end = start + size;

	// insert the new range / enlarge existing range

	TRanges::_Pairib insertionResult = ranges.insert (std::make_pair (start, size));
	if (!insertionResult.second && (insertionResult.first->second < size))
		insertionResult.first->second = size;

	// merge it with the previous one

	if (insertionResult.first != ranges.begin())
	{
		TRanges::iterator iter = insertionResult.first;
		--iter;

		revision_t previousEnd = iter->first + iter->second;

		if (previousEnd >= start)
		{
			if (end < previousEnd)
				end = previousEnd;

			iter->second = end - iter->first;

			insertionResult.first = ranges.erase (insertionResult.first);
			--insertionResult.first;
		}
	}

	// merge it with the next one

	TRanges::iterator iter = insertionResult.first;
	if ((++iter != ranges.end()) && (iter->first <= end))
	{
		insertionResult.first->second = max (end, iter->first + iter->second) 
									  - insertionResult.first->first;
		ranges.erase (iter);
	}
}

///////////////////////////////////////////////////////////////
// CSkipRevisionInfo::CPacker
///////////////////////////////////////////////////////////////
// remove ranges already covered by parent path ranges
///////////////////////////////////////////////////////////////

index_t CSkipRevisionInfo::CPacker::RemoveParentRanges()
{
	// count the number of remaining ranges

	index_t rangeCount = 0;

	// remove all parent ranges

	const CPathDictionary& paths = parent->paths;

	for (size_t i = 0, count = parent->data.size(); i < count; ++i)
	{
		SPerPathRanges* perPathInfo = parent->data[i];
		CDictionaryBasedPath parentPath 
			= CDictionaryBasedPath (&paths, perPathInfo->pathID).GetParent();

		SPerPathRanges::TRanges& ranges = perPathInfo->ranges;
		IT iter = ranges.begin();

		// check all ranges of data[i]

		while (iter != ranges.end())
		{
			bool removed = false;
			revision_t next = parent->GetNextRevision (parentPath, iter->first);

			// does the parent cover at least the begin of this range?

			if (next != NO_REVISION)
			{
				if (next >= iter->first + iter->second)
				{
					iter = ranges.erase (iter);
					removed = true;
				}
				else
				{
					revision_t size = iter->second + iter->first - next;
					ranges.insert (iter, std::make_pair (next, size));
					iter = ranges.erase (iter);
				}
			}

			if (!removed)
			{
				// the range wasn't entirely covered

				revision_t end = iter->first + iter->second-1;
				revision_t previous = parent->GetPreviousRevision (parentPath, end);

				// parent covers the end of the range?

				if (previous != NO_REVISION)
				{
					// reduce the size of the range

					iter->second = previous - iter->first + 1;
				}
			}

			// update counter and iterator

			if (!removed)
			{
				++rangeCount;
				++iter;
			}
		}
	}

	// sort all ranges

	return rangeCount;
}

///////////////////////////////////////////////////////////////
// build a sorted list of all ranges
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::CPacker::SortRanges (index_t rangeCount)
{
	allRanges.clear();
	allRanges.reserve (rangeCount);

	// remove all parent ranges

	std::vector<SPerPathRanges*>& data = parent->data;
	for (size_t i = 0, count = data.size(); i < count; ++i)
	{
		SPerPathRanges::TRanges& ranges = data[i]->ranges;
		for (IT iter = ranges.begin(), end = ranges.end(); iter != end; ++iter)
		{
			allRanges.push_back (iter);
		}
	}

	// sort all ranges

	std::sort (allRanges.begin(), allRanges.end(), CIterComp());
}

///////////////////////////////////////////////////////////////
// reset ranges completely covered by cached revision info
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::CPacker::RemoveKnownRevisions()
{
	revision_t firstKnownRevision = 0;
	revision_t nextUnknownRevision = 0;
	revision_t lastRevision = parent->revisions.GetLastRevision();

	for (size_t i = 0; i < allRanges.size(); ++i)
	{
		IT& iter = allRanges[i];
		if (iter->first > nextUnknownRevision)
		{
			firstKnownRevision = iter->first;
			while (   (!parent->DataAvailable (firstKnownRevision)) 
				   && (firstKnownRevision < lastRevision))
				++firstKnownRevision;

			nextUnknownRevision = firstKnownRevision;
			while (   (parent->DataAvailable (nextUnknownRevision)) 
				   && (nextUnknownRevision < lastRevision))
				++nextUnknownRevision;
		}

		if (   (iter->first >= firstKnownRevision)
			&& (iter->first + iter->second <= nextUnknownRevision))
			iter->second = 0;
	}
}

///////////////////////////////////////////////////////////////
// remove all ranges that have been reset to size 0
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::CPacker::RemoveEmptyRanges()
{
	// remove all ranges that have been reset to size 0

	std::vector<SPerPathRanges*>& data = parent->data;

	std::vector<SPerPathRanges*>::iterator dest = data.begin();
	for (size_t i = 0, count = data.size(); i < count; ++i)
	{
		SPerPathRanges::TRanges& ranges = data[i]->ranges;
		for (IT iter = ranges.begin(); iter != ranges.end(); )
		{
			if (iter->second == 0)
				iter = ranges.erase (iter);
			else
				++iter;
		}

		// remove path data if there are no ranges left 
		// for the respective path ID

		if (ranges.empty())
		{
			delete data[i];
		}
		else
		{
			*dest = data[i];
			++dest;
		}
	}

	data.erase (dest, data.end());
}

///////////////////////////////////////////////////////////////
// rebuild the hash index because the entries in data may have been moved
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::CPacker::RebuildHash()
{
	std::vector<SPerPathRanges*>& data = parent->data;
	quick_hash<CSkipRevisionInfo::CHashFunction>& index = parent->index;

    index.clear();
    index.reserve (data.size());

	for (size_t i = 0, count = data.size(); i < count; ++i)
        index.insert (data[i]->pathID, (index_t)i);
}

///////////////////////////////////////////////////////////////
// construction / destruction
///////////////////////////////////////////////////////////////

CSkipRevisionInfo::CPacker::CPacker()
	: parent (NULL)
{
}

CSkipRevisionInfo::CPacker::~CPacker()
{
}

///////////////////////////////////////////////////////////////
// remove unnecessary entries
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::CPacker::operator()(CSkipRevisionInfo* aParent)
{
	parent = aParent;

	index_t rangeCount = RemoveParentRanges();
	SortRanges (rangeCount);
	RemoveKnownRevisions();
	RemoveEmptyRanges();
    RebuildHash();
}

///////////////////////////////////////////////////////////////
// CSkipRevisionInfo
///////////////////////////////////////////////////////////////
// remove known revisions from the range
///////////////////////////////////////////////////////////////

bool CSkipRevisionInfo::DataAvailable (revision_t revision)
{
    index_t index = revisions[revision];
    if (index == NO_INDEX)
        return false;

    return (  logInfo.GetPresenceFlags (index) 
            & CRevisionInfoContainer::HAS_CHANGEDPATHS) != 0;
}

void CSkipRevisionInfo::TryReduceRange (revision_t& revision, revision_t& size)
{
	// raise lower bound

	while ((size > 0) && DataAvailable (revision))
	{
		++revision;
		--size;
	}

	// lower upper bound

    while ((size > 0) && DataAvailable (revision + size-1))
	{
		--size;
	}
}

///////////////////////////////////////////////////////////////
// construction / destruction
///////////////////////////////////////////////////////////////

CSkipRevisionInfo::CSkipRevisionInfo ( const CPathDictionary& aPathDictionary
									 , const CRevisionIndex& aRevisionIndex
                                     , const CRevisionInfoContainer& logInfo)
	: index (CHashFunction (&data))
	, paths (aPathDictionary)
	, revisions (aRevisionIndex)
    , logInfo (logInfo)
{
}

CSkipRevisionInfo::~CSkipRevisionInfo(void)
{
	Clear();
}

///////////////////////////////////////////////////////////////
// query data
///////////////////////////////////////////////////////////////

revision_t CSkipRevisionInfo::GetNextRevision ( const CDictionaryBasedPath& path
										      , revision_t revision) const
{
	// above the root or invalid parameter ?

	if (!path.IsValid() || (revision == NO_REVISION))
		return (revision_t)NO_REVISION;

	// lookup the entry for this path

	index_t dataIndex = index.find (path.GetIndex());
	SPerPathRanges* ranges = dataIndex == NO_INDEX
						   ? NULL
						   : data[dataIndex];

	// crawl this and the parent path data
	// until we found a gap (i.e. could not improve further)

	revision_t startRevision = revision;
	revision_t result = revision;

	do
	{
		result = revision;

		revision_t parentNext = GetNextRevision (path.GetParent(), revision);
		if (parentNext != NO_REVISION)
			revision = parentNext;

		if (ranges != NULL)
		{
			if (parentNext != (revision_t)NO_REVISION)
				revision = parentNext;
		}
	}
	while (revision > result);

	// ready

	return revision == startRevision 
		? NO_REVISION 
		: result;
}

revision_t CSkipRevisionInfo::GetPreviousRevision ( const CDictionaryBasedPath& path
												  , revision_t revision) const
{
	// above the root or invalid parameter ?

	if (!path.IsValid() || (revision == NO_REVISION))
		return (revision_t)NO_REVISION;

	// lookup the entry for this path

	index_t dataIndex = index.find (path.GetIndex());
	SPerPathRanges* ranges = dataIndex == NO_INDEX
						   ? NULL
						   : data[dataIndex];

	// crawl this and the parent path data
	// until we found a gap (i.e. could not improve further)

	revision_t startRevision = revision;
	revision_t result = revision;

	do
	{
		result = revision;

		revision_t parentNext = GetPreviousRevision (path.GetParent(), revision);
		if (parentNext != NO_REVISION)
			revision = parentNext;

		if (ranges != NULL)
		{
			revision_t next = ranges->FindPrevious (revision);
			if (next != NO_REVISION)
				revision = next;
		}
	}
	while (revision < result);

	// ready

	return revision == startRevision 
		? NO_REVISION
		: result;
}

///////////////////////////////////////////////////////////////
// add / remove data
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::Add ( const CDictionaryBasedPath& path
							, revision_t revision
							, revision_t size)
{
	// violating these assertions will break our lookup algorithms

	assert (path.IsValid());
	assert (revision != NO_REVISION);
	assert (size != NO_REVISION);
	assert (2*size > size);

	// reduce the range, if we have revision info at the boundaries

	TryReduceRange (revision, size);
	if (size == 0)
		return;

	// lookup / auto-insert entry for path

	SPerPathRanges* ranges = NULL;
	index_t dataIndex = index.find (path.GetIndex());

	if (dataIndex == NO_INDEX)
	{
		ranges = new SPerPathRanges;
		ranges->pathID = path.GetIndex();

		data.push_back (ranges);
		index.insert (path.GetIndex(), (index_t)data.size()-1);
	}
	else
	{
		ranges = data[dataIndex];
	}

	// add range

	ranges->Add (revision, size);
}

///////////////////////////////////////////////////////////////
// remove all data
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::Clear()
{
	for (size_t i = 0, count = data.size(); i != count; ++i)
		delete data[i];

	data.clear();
	index.clear();
}

///////////////////////////////////////////////////////////////
// remove unnecessary entries
///////////////////////////////////////////////////////////////

void CSkipRevisionInfo::Compress()
{
	CPacker()(this);
}

///////////////////////////////////////////////////////////////
// stream I/O
///////////////////////////////////////////////////////////////

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CSkipRevisionInfo& container)
{
	// open sub-streams

	CPackedDWORDInStream* pathIDsStream 
		= dynamic_cast<CPackedDWORDInStream*>
			(stream.GetSubStream (CSkipRevisionInfo::PATHIDS_STREAM_ID));

	CPackedDWORDInStream* entryCountStream 
		= dynamic_cast<CPackedDWORDInStream*>
			(stream.GetSubStream (CSkipRevisionInfo::ENTRY_COUNT_STREAM_ID));

	CDiffDWORDInStream* revisionsStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CSkipRevisionInfo::REVISIONS_STREAM_ID));

	CDiffIntegerInStream* sizesStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CSkipRevisionInfo::SIZES_STREAM_ID));

	// read all data

	size_t count = pathIDsStream->GetValue();

	container.Clear();
	container.data.reserve (count);

	for (size_t i = 0; i < count; ++i)
	{
		std::auto_ptr<CSkipRevisionInfo::SPerPathRanges> perPathInfo 
			(new CSkipRevisionInfo::SPerPathRanges);

		perPathInfo->pathID = pathIDsStream->GetValue();

		size_t entryCount = entryCountStream->GetValue();
		CSkipRevisionInfo::IT iter = perPathInfo->ranges.end();
		for (size_t k = 0; k < entryCount; ++k)
		{
			iter = perPathInfo->ranges.insert 
					(iter, std::make_pair ( revisionsStream->GetValue()
										  , sizesStream->GetValue()));
		}

		container.index.insert ( perPathInfo->pathID
							   , (index_t)container.data.size());
		container.data.push_back (perPathInfo.release());
	}

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CSkipRevisionInfo& container)
{
	// minimize the data to write

	const_cast<CSkipRevisionInfo*>(&container)->Compress();

	typedef std::vector<CSkipRevisionInfo::SPerPathRanges*>::const_iterator CIT;
	CIT begin = container.data.begin();
	CIT end = container.data.end();

	// write path IDs

	CPackedDWORDOutStream* pathIDsStream 
		= dynamic_cast<CPackedDWORDOutStream*>
			(stream.OpenSubStream ( CSkipRevisionInfo::PATHIDS_STREAM_ID
								  , PACKED_DWORD_STREAM_TYPE_ID));
	pathIDsStream->Add ((DWORD)container.data.size());

	for (CIT dataIter = begin, dataEnd = end; dataIter < dataEnd; ++dataIter)
		pathIDsStream->Add ((*dataIter)->pathID);

	// write number of ranges per path

	CPackedDWORDOutStream* entryCountStream 
		= dynamic_cast<CPackedDWORDOutStream*>
			(stream.OpenSubStream ( CSkipRevisionInfo::ENTRY_COUNT_STREAM_ID
								  , PACKED_DWORD_STREAM_TYPE_ID));

	for (CIT dataIter = begin, dataEnd = end; dataIter < dataEnd; ++dataIter)
		entryCountStream->Add ((DWORD)(*dataIter)->ranges.size());

	// write ranges start revisions

	CDiffDWORDOutStream* revisionsStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CSkipRevisionInfo::REVISIONS_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));

	for (CIT dataIter = begin, dataEnd = end; dataIter < dataEnd; ++dataIter)
		for ( CSkipRevisionInfo::IT iter = (*dataIter)->ranges.begin()
			, endlocal = (*dataIter)->ranges.end()
			; iter != endlocal
			; ++iter)
		{
			revisionsStream->Add (iter->first);
		}

	// write ranges lengths

	CDiffIntegerOutStream* sizesStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CSkipRevisionInfo::SIZES_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));

	for (CIT dataIter = begin, dataEnd = end; dataIter < dataEnd; ++dataIter)
		for ( CSkipRevisionInfo::IT iter = (*dataIter)->ranges.begin()
			, endlocal = (*dataIter)->ranges.end()
			; iter != endlocal
			; ++iter)
		{
			sizesStream->Add (iter->second);
		}

	// ready

	return stream;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

