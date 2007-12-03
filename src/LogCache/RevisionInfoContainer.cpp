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
#include ".\revisioninfocontainer.h"

#include "PackedDWORDInStream.h"
#include "PackedDWORDOutStream.h"
#include "DiffIntegerInStream.h"
#include "DiffIntegerOutStream.h"
#include "PackedTime64InStream.h"
#include "PackedTime64OutStream.h"

// begin namespace LogCache

namespace LogCache
{

// update / modify utilities

void CRevisionInfoContainer::UpdateAuthors 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap
    , bool keepOldDataForMissingNew)
{
	index_mapping_t idMapping = authorPool.Merge (newData.authorPool);

	index_t count = size();
	for ( index_mapping_t::const_iterator iter = indexMap.begin()
		, end = indexMap.end()
		; iter != end
		; ++iter)
	{
		assert (iter->key < count);
        if (   !keepOldDataForMissingNew 
            || (newData.presenceFlags[iter->value] & HAS_AUTHOR))
        {
    		authors[iter->key] = *idMapping.find (newData.authors[iter->value]);
        }
	}
}

void CRevisionInfoContainer::UpdateTimeStamps 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap
    , bool keepOldDataForMissingNew)
{
	index_t count = size();
	for ( index_mapping_t::const_iterator iter = indexMap.begin()
		, end = indexMap.end()
		; iter != end
		; ++iter)
	{
		assert (iter->key < count);
        if (   !keepOldDataForMissingNew 
            || (newData.presenceFlags[iter->value] & HAS_TIME_STAMP))
        {
		    timeStamps[iter->key] = newData.timeStamps[iter->value];
        }
	}
}

void CRevisionInfoContainer::UpdateComments 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap
    , bool keepOldDataForMissingNew)
{
	index_mapping_t toReplace;

	index_t count = size();
	for ( index_mapping_t::const_iterator iter = indexMap.begin()
		, end = indexMap.end()
		; iter != end
		; ++iter)
	{
		assert (iter->key < count);
        if (   !keepOldDataForMissingNew 
            || (newData.presenceFlags[iter->value] & HAS_COMMENT))
        {
    		toReplace.insert (iter->key, iter->value);
        }
	}

	comments.Replace (newData.comments, toReplace);
}

void CRevisionInfoContainer::UpdateChanges 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap
	, const index_mapping_t& pathIDMapping
    , bool keepOldDataForMissingNew)
{
	// save & remove old data

	std::vector<unsigned char> oldChanges;
	changes.swap (oldChanges);
	changes.reserve (oldChanges.size());

	std::vector<index_t> oldChangedPaths;
	changedPaths.swap (oldChangedPaths);
	changedPaths.reserve (oldChangedPaths.size());

	std::vector<index_t> oldCopyFromPaths;
	copyFromPaths.swap (oldCopyFromPaths);
	copyFromPaths.reserve (oldCopyFromPaths.size());

	std::vector<revision_t> oldCopyFromRevisions;
	copyFromRevisions.swap (oldCopyFromRevisions);
	copyFromRevisions.reserve (oldCopyFromRevisions.size());

	// splice

	index_t lastChange = 0;
    index_t lastCopy = 0;

    index_mapping_t::const_iterator mapEnd = indexMap.end();
	for (index_t i = 0, count = size(); i < count; ++i)
	{
		lastChange = changesOffsets[i+1];

		lastCopy = copyFromOffsets[i+1];

        index_mapping_t::const_iterator iter = indexMap.find (i);
		if ((iter != mapEnd)
            && (   !keepOldDataForMissingNew 
                || (newData.presenceFlags[iter->value] & HAS_CHANGEDPATHS)))
		{
			// copy & translate

			index_t sourceIndex = iter->value;
			for ( index_t k = newData.changesOffsets [sourceIndex]
				, last = newData.changesOffsets [sourceIndex+1]
				; k != last
				; ++k)
			{
				changes.push_back (newData.changes[k]);
				changedPaths.push_back (*pathIDMapping.find (newData.changedPaths[k]));
			}

			for ( index_t k = newData.copyFromOffsets [sourceIndex]
				, last = newData.copyFromOffsets [sourceIndex+1]
				; k != last
				; ++k)
			{
				copyFromRevisions.push_back (newData.copyFromRevisions[k]);
				copyFromPaths.push_back (*pathIDMapping.find (newData.copyFromPaths[k]));
			}

            rootPaths[i] = *pathIDMapping.find (newData.rootPaths[sourceIndex]);
			sumChanges[i] = newData.sumChanges[sourceIndex];
		}
		else
		{
			// keep existing data

		    index_t firstChange = changesOffsets[i];
			index_t lastChange = changesOffsets[i+1];

		    index_t firstCopy = copyFromOffsets[i];
			index_t lastCopy = copyFromOffsets[i+1];

			// standard per-path info

			changes.insert ( changes.end()
						   , oldChanges.begin() + firstChange
						   , oldChanges.begin() + lastChange);
			changedPaths.insert ( changedPaths.end()
								, oldChangedPaths.begin() + firstChange
								, oldChangedPaths.begin() + lastChange);

			// copy-from info, if available

			if (firstCopy != lastCopy)
			{
				copyFromPaths.insert ( copyFromPaths.end()
								     , oldCopyFromPaths.begin() + firstCopy
								     , oldCopyFromPaths.begin() + lastCopy);
				copyFromRevisions.insert ( copyFromRevisions.end()
										 , oldCopyFromRevisions.begin() + firstCopy
										 , oldCopyFromRevisions.begin() + lastCopy);
			}
		}

		// update positions

		changesOffsets[i+1] = static_cast<index_t>(changes.size());
		copyFromOffsets[i+1] = static_cast<index_t>(copyFromPaths.size());
	}
}

void CRevisionInfoContainer::UpdateMergers 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap
	, const index_mapping_t& pathIDMapping
    , bool keepOldDataForMissingNew)
{
	// save & remove old data

	std::vector<index_t> oldMergedFromPaths;
	mergedFromPaths.swap (oldMergedFromPaths);
	mergedFromPaths.reserve (oldMergedFromPaths.size());

	std::vector<index_t> oldMergedToPaths;
	mergedToPaths.swap (oldMergedToPaths);
	mergedToPaths.reserve (oldMergedToPaths.size());

	std::vector<revision_t> oldMergedRangeStarts;
	mergedRangeStarts.swap (oldMergedRangeStarts);
	mergedRangeStarts.reserve (oldMergedRangeStarts.size());

	std::vector<revision_t> oldMergedRangeDeltas;
	mergedRangeDeltas.swap (oldMergedRangeDeltas);
	mergedRangeDeltas.reserve (oldMergedRangeDeltas.size());

	// splice

	index_t lastMerge = 0;

	index_mapping_t::const_iterator mapEnd = indexMap.end();
	for (index_t i = 0, count = size(); i < count; ++i)
	{
        index_t firstMerge = lastMerge;
		lastMerge = mergedRevisionsOffsets[i+1];

		index_mapping_t::const_iterator iter = indexMap.find (i);
		if ((iter != mapEnd)
            && (   !keepOldDataForMissingNew 
                || (newData.presenceFlags[iter->value] & HAS_MERGEINFO)))
		{
			// copy & translate

			index_t sourceIndex = iter->value;
			for ( index_t k = newData.mergedRevisionsOffsets [sourceIndex]
				, last = newData.mergedRevisionsOffsets [sourceIndex+1]
				; k != last
				; ++k)
			{
				mergedFromPaths.push_back (*pathIDMapping.find (newData.mergedFromPaths[k]));
				mergedToPaths.push_back (*pathIDMapping.find (newData.mergedToPaths[k]));

				mergedRangeStarts.push_back (newData.mergedRangeStarts[k]);
				mergedRangeDeltas.push_back (newData.mergedRangeDeltas[k]);
			}
		}
		else
		{
			// keep exisiting data

			mergedFromPaths.insert ( mergedFromPaths.end()
								   , oldMergedFromPaths.begin() + firstMerge
								   , oldMergedFromPaths.begin() + lastMerge);
			mergedToPaths.insert ( mergedToPaths.end()
								 , oldMergedToPaths.begin() + firstMerge
								 , oldMergedToPaths.begin() + lastMerge);
			mergedRangeStarts.insert ( mergedRangeStarts.end()
									 , oldMergedRangeStarts.begin() + firstMerge
									 , oldMergedRangeStarts.begin() + lastMerge);
			mergedRangeDeltas.insert ( mergedRangeDeltas.end()
									 , oldMergedRangeDeltas.begin() + firstMerge
									 , oldMergedRangeDeltas.begin() + lastMerge);
		}

		// update positions

		mergedRevisionsOffsets[i+1] = static_cast<index_t>(mergedFromPaths.size());
	}
}

void CRevisionInfoContainer::UpdateUserRevProps 
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap
    , bool keepOldDataForMissingNew)
{
	index_mapping_t propIDMapping 
        = userRevPropsPool.Merge (newData.userRevPropsPool);

    index_mapping_t oldValueMapping;
    index_mapping_t newValueMapping;

	// save & remove old data

    std::vector<index_t> oldUserRevPropNames;
	userRevPropNames.swap (oldUserRevPropNames);
	userRevPropNames.reserve (oldUserRevPropNames.size());

    CTokenizedStringContainer oldUserRevPropValues;
	userRevPropValues.swap (oldUserRevPropValues);

    static const std::string emptyValue;
    userRevPropValues.Insert (emptyValue,  userRevPropValues.size() 
                                         + newData.userRevPropValues.size());

	// splice

	index_t lastProp = 0;
    index_t valueIndex = 0;

	index_mapping_t::const_iterator mapEnd = indexMap.end();
	for (index_t i = 0, count = size(); i < count; ++i)
	{
        index_t firstProp = lastProp;
		lastProp = userRevPropOffsets[i+1];

		index_mapping_t::const_iterator iter = indexMap.find (i);
		if ((iter != mapEnd)
            && (   !keepOldDataForMissingNew 
                || (newData.presenceFlags[iter->value] & HAS_USERREVPROPS)))
		{
			// copy & translate

			index_t sourceIndex = iter->value;
            for ( index_t k = newData.userRevPropNames [sourceIndex]
				, last = newData.userRevPropNames [sourceIndex+1]
				; k != last
				; ++k)
			{
				userRevPropNames.push_back (*propIDMapping.find (newData.userRevPropNames [k]));
                newValueMapping.insert (valueIndex++, k);
			}
		}
		else
		{
			// keep exisiting data

			userRevPropNames.insert ( userRevPropNames.end()
								    , oldUserRevPropNames.begin() + firstProp
								    , oldUserRevPropNames.begin() + lastProp);

            for (index_t k = firstProp; k != lastProp; ++k)
                oldValueMapping.insert (valueIndex++, k);
		}

		// update positions

		userRevPropOffsets[i+1] = static_cast<index_t>(userRevPropNames.size());
	}

    // actually write the combined revprop values

    userRevPropValues.Replace (oldUserRevPropValues, oldValueMapping);
    userRevPropValues.Replace (newData.userRevPropValues, newValueMapping);
}

void CRevisionInfoContainer::UpdatePresenceFlags
	( const CRevisionInfoContainer& newData
	, const index_mapping_t& indexMap
    , char flags
    , bool keepOldDataForMissingNew)
{
	for ( index_mapping_t::const_iterator iter = indexMap.begin()
		, end = indexMap.end()
		; iter != end
		; ++iter)
	{
        // values that have been copied

        char setValues = newData.presenceFlags[iter->value] & flags;

        if (keepOldDataForMissingNew)
            presenceFlags[iter->key] |= setValues;
        else
            presenceFlags[iter->key] = (presenceFlags[iter->key] & ~flags) 
                                     | setValues;
    }
}

void CRevisionInfoContainer::AppendNewEntries 
	(const index_mapping_t& indexMap)
{
    // count the number of new entries required

	size_t oldCount = size();
    size_t toAppend = 0;

	for ( index_mapping_t::const_iterator iter = indexMap.begin()
		, end = indexMap.end()
		; iter != end
		; ++iter)
	{
		if (iter->key >= oldCount)
            ++toAppend;
    }

    // append new entries

    if (toAppend > 0)
	{
        // append a new revision info

        authors.insert (authors.end(), toAppend, (index_t)NO_INDEX);
        timeStamps.insert (timeStamps.end(), toAppend, 0);
        presenceFlags.insert (presenceFlags.end(), toAppend, 0);
        rootPaths.insert (rootPaths.end(), toAppend, (index_t)NO_INDEX);
        sumChanges.insert (sumChanges.end(), toAppend, 0);

        static const std::string emptyComment;
    	comments.Insert (emptyComment, toAppend);
	}
}

// sort authors by frequency
// (reduces average diff between authors -> less disk space
// and clusters author access -> cache friendly)

void CRevisionInfoContainer::OptimizeAuthors()
{
	struct SPerAuthorInfo
	{
		index_t frequency;
		index_t author;

		SPerAuthorInfo (index_t author)
			: frequency (0)
			, author (author)
		{
		}

		bool operator<(const SPerAuthorInfo& rhs) const
		{
			return (frequency > rhs.frequency)
				|| (   (frequency == rhs.frequency) 
				    && (author < rhs.author));
		}
	};

	// initialize the counter array: counter[author].author == author

	std::vector<SPerAuthorInfo> counter;
	counter.reserve (authorPool.size());

	for (index_t i = 0, count = authorPool.size(); i < count; ++i)
		counter.push_back (i);

	// count occurances

	for ( std::vector<index_t>::const_iterator iter = authors.begin()
		, end = authors.end()
		; iter != end
		; ++iter)
	{
		++counter[*iter].frequency;
	}

	// sort by frequency and "age", but keep empty string untouched

	std::sort (++counter.begin(), counter.end());

	// re-arrange authors according to this new order

	std::vector<index_t> indices;
	indices.reserve (authorPool.size());

	for (index_t i = 0, count = authorPool.size(); i < count; ++i)
		indices.push_back (counter[i].author);

	authorPool.Reorder (indices);

	// re-map the author indices

	for (index_t i = 0, count = authorPool.size(); i < count; ++i)
		indices [counter[i].author] = i;

	for ( std::vector<index_t>::iterator iter = authors.begin()
		, end = authors.end()
		; iter != end
		; ++iter)
	{
		*iter = indices[*iter];
	}
}

// sort changes by pathID; parent change still before leave change
// (reduces average diff between changes -> less disk space
// and sorts path access patterns -> cache friendly)

void CRevisionInfoContainer::OptimizeChangeOrder()
{
	struct SPerChangeInfo
	{
		index_t changedPath;
		unsigned char change;
		index_t copyFromPath;
		revision_t copyFromRevision;

		SPerChangeInfo (const CRevisionInfoContainer::CChangesIterator& iter)
			: changedPath (iter->GetPathID())
			, change ((unsigned char)iter->GetRawChange())
		{
			if (iter->HasFromPath())
			{
				copyFromPath = iter->GetFromPathID();
				copyFromRevision = iter->GetFromRevision();
			}
		}

		bool operator<(const SPerChangeInfo& rhs) const
		{
			// report deletes of a given path *before* any other change
			// (probably an "add") to this path

			return (changedPath < rhs.changedPath)
				|| (   (changedPath == rhs.changedPath) 
				    && (change > rhs.change));
		}
	};

	std::vector<SPerChangeInfo> revisionChanges;
	for (index_t index = 0, count = size(); index < count; ++index)
	{
		// collect all changes of this revision

		revisionChanges.clear();

		for ( CChangesIterator iter = GetChangesBegin (index)
			, end = GetChangesEnd (index)
			; iter != end
			; ++iter)
		{
			revisionChanges.push_back (iter);
		}

		// sort them by path

		std::sort (revisionChanges.begin(), revisionChanges.end());

		// overwrite old data with new ordering

		index_t changeOffset = changesOffsets[index];
		index_t copyFromOffset = copyFromOffsets[index];

		for ( std::vector<SPerChangeInfo>::iterator iter = revisionChanges.begin()
			, end = revisionChanges.end()
			; iter != end
			; ++iter)
		{
			changedPaths[changeOffset] = iter->changedPath;
			changes[changeOffset] = iter->change;
			++changeOffset;

			if (iter->change & HAS_COPY_FROM)
			{
				copyFromPaths[copyFromOffset] = iter->copyFromPath;
				copyFromRevisions[copyFromOffset] = iter->copyFromRevision;
				++copyFromOffset;
			}
		}

		// we must have used the exact same memory

		assert (changeOffset == changesOffsets[index+1]);
		assert (copyFromOffset == copyFromOffsets[index+1]);
	}
}

// construction / destruction

CRevisionInfoContainer::CRevisionInfoContainer(void)
	: storedSize (0)
{
	// [lastIndex+1] must point to [size()]

	changesOffsets.push_back(0);
	copyFromOffsets.push_back(0);
	mergedRevisionsOffsets.push_back(0);
    userRevPropOffsets.push_back(0);
}

CRevisionInfoContainer::~CRevisionInfoContainer(void)
{
}

// add information
// AddChange() always adds to the last revision

index_t CRevisionInfoContainer::Insert ( const std::string& author
									   , const std::string& comment
									   , __time64_t timeStamp
                                       , char flags)
{
	// this should newer throw as there are usually more
	// changes than revisions. But you never know ...

	if (authors.size() == NO_INDEX)
		throw std::exception ("revision container overflow");

	// store the given revision info

	authors.push_back (authorPool.AutoInsert (author.c_str()));
	timeStamps.push_back (timeStamp);
	comments.Insert (comment);
	presenceFlags.push_back (flags);

	// no changes yet -> no common root path info

	rootPaths.push_back ((index_t)NO_INDEX);
	sumChanges.push_back (0);

	// empty range for changes 

	changesOffsets.push_back (*changesOffsets.rbegin());
	copyFromOffsets.push_back (*copyFromOffsets.rbegin());
	mergedRevisionsOffsets.push_back (*mergedRevisionsOffsets.rbegin());
    userRevPropOffsets.push_back (*userRevPropOffsets.rbegin());

	// ready

	return size()-1;
}

void CRevisionInfoContainer::AddChange ( TChangeAction action
									   , const std::string& path
									   , const std::string& fromPath
									   , revision_t fromRevision)
{
    assert (*presenceFlags.rbegin() & HAS_CHANGEDPATHS);

    // under x64, there might actually be an overflow

	if (changes.size() == NO_INDEX)
		throw std::exception ("revision container change list overflow");

	// another change

	++(*changesOffsets.rbegin());

	// add change path index and update the root path 
	// of all changes in this revision

	CDictionaryBasedPath parsedPath (&paths, path, false);
	changedPaths.push_back (parsedPath.GetIndex());

	index_t& rootPathIndex = *rootPaths.rbegin();
	rootPathIndex = rootPathIndex == NO_INDEX
				  ? parsedPath.GetIndex()
				  : parsedPath.GetCommonRoot (rootPathIndex).GetIndex();

	// add changes info (flags), and indicate presence of fromPath (if given)

    char flags = fromPath.empty()
        ? (char)action 
        : (char)action | HAS_COPY_FROM;

	*sumChanges.rbegin() |= (char)flags;
	changes.push_back ((char)flags);

	if (!fromPath.empty())
	{
		// add fromPath info

		CDictionaryBasedPath parsedFromPath (&paths, fromPath, false);
		copyFromPaths.push_back (parsedFromPath.GetIndex());
		copyFromRevisions.push_back (fromRevision);

		++(*copyFromOffsets.rbegin());
	}

}

void CRevisionInfoContainer::AddMergedRevision ( const std::string& fromPath
											   , const std::string& toPath
											   , revision_t revisionStart
											   , revision_t revisionDelta)
{
	assert (revisionDelta != 0);
    assert (*presenceFlags.rbegin() & HAS_MERGEINFO);

	// under x64, there might actually be an overflow

	if (mergedRangeStarts.size() == NO_INDEX)
		throw std::exception ("revision container change list overflow");

	// another merge

	++(*mergedRevisionsOffsets.rbegin());

	// add merged revision range and path info

	CDictionaryBasedPath parsedFromPath (&paths, fromPath, false);
	mergedFromPaths.push_back (parsedFromPath.GetIndex());
	CDictionaryBasedPath parsedToPath (&paths, toPath, false);
	mergedToPaths.push_back (parsedToPath.GetIndex());

	mergedRangeStarts.push_back (revisionStart);
	mergedRangeDeltas.push_back (revisionDelta);
}

void CRevisionInfoContainer::AddUserRevProp ( const std::string& revProp
	                        		        , const std::string& value)
{
    // store standard rev-props somewhere else!

    assert (   (revProp != "svn:author") 
            && (revProp != "svn:date")
            && (revProp != "svn:log"));
    assert (*presenceFlags.rbegin() & HAS_USERREVPROPS);

	// under x64, there might actually be an overflow

	if (userRevPropNames.size() == NO_INDEX)
		throw std::exception ("revision container change list overflow");

	// another revProp

	++(*userRevPropOffsets.rbegin());

	// add (revPropName, value) pair

	userRevPropNames.push_back (userRevPropsPool.AutoInsert (revProp.c_str()));
	userRevPropValues.Insert (value);
}


// reset content

void CRevisionInfoContainer::Clear()
{
	authorPool.Clear();
	paths.Clear();
	comments.Clear();

    presenceFlags.clear();
	authors.clear();
	timeStamps.clear();

	rootPaths.clear();
	sumChanges.clear();

	changesOffsets.erase (changesOffsets.begin()+1, changesOffsets.end());
	copyFromOffsets.erase (copyFromOffsets.begin()+1, copyFromOffsets.end());
	mergedRevisionsOffsets.erase (mergedRevisionsOffsets.begin()+1, mergedRevisionsOffsets.end());
	userRevPropOffsets.erase (userRevPropOffsets.begin()+1, userRevPropOffsets.end());

	changes.clear();
	changedPaths.clear();
	copyFromPaths.clear();
	copyFromRevisions.clear();

	mergedFromPaths.clear();
	mergedToPaths.clear();
	mergedRangeStarts.clear();
	mergedRangeDeltas.clear();

	userRevPropsPool.Clear();
    userRevPropValues.Clear();
    userRevPropNames.clear();
}

// update / modify existing data
// indexes must be in ascending order
// indexes[] may be size() -> results in an Append()

void CRevisionInfoContainer::Update ( const CRevisionInfoContainer& newData
									, const index_mapping_t& indexMap
                                    , char flags
                                    , bool keepOldDataForMissingNew)
{
	// first, create new indices where necessary

	AppendNewEntries (indexMap);

    // make new paths available

	index_mapping_t pathIDMapping = paths.Merge (newData.paths);

	// replace exising data

	if (flags & HAS_AUTHOR)
		UpdateAuthors (newData, indexMap, keepOldDataForMissingNew);
	if (flags & HAS_TIME_STAMP)
		UpdateTimeStamps (newData, indexMap, keepOldDataForMissingNew);
	if (flags & HAS_COMMENT)
		UpdateComments (newData, indexMap, keepOldDataForMissingNew);

	if (flags & HAS_CHANGEDPATHS)
		UpdateChanges (newData, indexMap, pathIDMapping, keepOldDataForMissingNew);
	if (flags & HAS_MERGEINFO)
		UpdateMergers (newData, indexMap, pathIDMapping, keepOldDataForMissingNew);
	if (flags & HAS_USERREVPROPS)
		UpdateUserRevProps (newData, indexMap, keepOldDataForMissingNew);

    UpdatePresenceFlags (newData, indexMap, flags, keepOldDataForMissingNew);
}

// rearrange the data to minimize disk and cache footprint

void CRevisionInfoContainer::Optimize()
{
	OptimizeAuthors();
	OptimizeChangeOrder();
}

// AutoOptimize() will call Optimize() when size() crossed 2^n boundaries.

void CRevisionInfoContainer::AutoOptimize()
{
	// bitwise XOR old and new size

	index_t currentSize = size();
	index_t diffBits = currentSize ^ storedSize;

	// if the position of highest bit set is equal (no 2^n boundary crossed),
	// diffBits will have that bit reset -> smaller than both input values

	if ((currentSize <= diffBits) || (storedSize <= diffBits))
	{
		// shrink or growth cross a 2^n bounary

		Optimize();
	}
}

// reconstruct derived data

void CRevisionInfoContainer::CalculateSumChanges()
{
	// initialize all sums with "0"

	sumChanges.clear();
	sumChanges.resize (rootPaths.size());

	if (changes.empty())
		return;

	// fold all changes 
	// (hand-tuned code because vector iterators do expensive checking
	//  ~6 ticks vs. ~20 ticks per change)

	const index_t *iter = &changesOffsets.at(1);
	unsigned char *target = &sumChanges.at(0);
	const unsigned char *source = &changes.at(0);

	unsigned char sum = 0;
	for (size_t i = 0, count = changes.size(); i < count; ++i)
	{
		while (*iter <= i)
		{
			*target = sum;
			++iter;
			++target;
			sum = 0;
		}

		sum |= *(source+i);
	}

	*target = sum;
}

// stream I/O

IHierarchicalInStream& operator>> ( IHierarchicalInStream& stream
								  , CRevisionInfoContainer& container)
{
	// read the pools

	IHierarchicalInStream* authorPoolStream
		= stream.GetSubStream (CRevisionInfoContainer::AUTHOR_POOL_STREAM_ID);
	*authorPoolStream >> container.authorPool;

	IHierarchicalInStream* commentsStream
		= stream.GetSubStream (CRevisionInfoContainer::COMMENTS_STREAM_ID);
	*commentsStream >> container.comments;

	IHierarchicalInStream* pathsStream
		= stream.GetSubStream (CRevisionInfoContainer::PATHS_STREAM_ID);
	*pathsStream >> container.paths;

	// read the revision info

	CDiffIntegerInStream* authorsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::AUTHORS_STREAM_ID));
	*authorsStream >> container.authors;

	CPackedTime64InStream* timeStampsStream 
		= dynamic_cast<CPackedTime64InStream*>
			(stream.GetSubStream (CRevisionInfoContainer::TIMESTAMPS_STREAM_ID));
	*timeStampsStream >> container.timeStamps;

	CDiffIntegerInStream* rootPathsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::ROOTPATHS_STREAM_ID));
	*rootPathsStream >> container.rootPaths;

	CDiffDWORDInStream* changesOffsetsStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::CHANGES_OFFSETS_STREAM_ID));
	*changesOffsetsStream >> container.changesOffsets;

	CDiffDWORDInStream* copyFromOffsetsStream 
		= dynamic_cast<CDiffDWORDInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::COPYFROM_OFFSETS_STREAM_ID));
	*copyFromOffsetsStream >> container.copyFromOffsets;

	CDiffDWORDInStream* mergedRevisionsOffsetsStream 
		= dynamic_cast<CDiffDWORDInStream*>
		(stream.GetSubStream (CRevisionInfoContainer::MERGEDREVISION_OFFSETS_STREAM_ID));
	*mergedRevisionsOffsetsStream >> container.mergedRevisionsOffsets;

	// read the changes info

	CPackedDWORDInStream* changesStream 
		= dynamic_cast<CPackedDWORDInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::CHANGES_STREAM_ID));
	*changesStream >> container.changes;

	CDiffIntegerInStream* changedPathsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::CHANGED_PATHS_STREAM_ID));
	*changedPathsStream >> container.changedPaths;

	CDiffIntegerInStream* copyFromPathsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::COPYFROM_PATHS_STREAM_ID));
	*copyFromPathsStream >> container.copyFromPaths;

	CDiffIntegerInStream* copyFromRevisionsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::COPYFROM_REVISIONS_STREAM_ID));
	*copyFromRevisionsStream >> container.copyFromRevisions;

	// merged revisions

	CDiffIntegerInStream* mergedFromPathsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::MERGED_FROM_PATHS_STREAM_ID));
	*mergedFromPathsStream >> container.mergedFromPaths;

	CDiffIntegerInStream* mergedToPathsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::MERGED_TO_PATHS_STREAM_ID));
	*mergedToPathsStream >> container.mergedToPaths;

	CDiffIntegerInStream* mergedRangeStartsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::MERGED_RANGE_STARTS_STREAM_ID));
	*mergedRangeStartsStream >> container.mergedRangeStarts;

	CDiffIntegerInStream* mergedRangeDeltasStream
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::MERGED_RANGE_DELTAS_STREAM_ID));
	*mergedRangeDeltasStream >> container.mergedRangeDeltas;

    // user-defined revision properties
    
	CDiffIntegerInStream* userRevPropsOffsetsStream 
		= dynamic_cast<CDiffIntegerInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::USER_REVPROPS_OFFSETS_STREAM_ID));
    *userRevPropsOffsetsStream >> container.userRevPropOffsets;

	IHierarchicalInStream* userRevPropsPoolStream
		= stream.GetSubStream (CRevisionInfoContainer::USER_REVPROPS_POOL_STREAM_ID);
    *userRevPropsPoolStream >> container.userRevPropsPool;

    CPackedDWORDInStream* userRevPropsNameStream 
		= dynamic_cast<CPackedDWORDInStream*>
			(stream.GetSubStream (CRevisionInfoContainer::USER_REVPROPS_NAME_STREAM_ID));
    *userRevPropsNameStream >> container.userRevPropNames;

	IHierarchicalInStream* userRevPropsValuesStream
		= stream.GetSubStream (CRevisionInfoContainer::USER_REVPROPS_VALUE_STREAM_ID);
    *userRevPropsValuesStream >> container.userRevPropValues;

    // data presence flags

    CPackedDWORDInStream* dataPresenceStream 
		= dynamic_cast<CPackedDWORDInStream*>
			(stream.GetSubStream ( CRevisionInfoContainer::DATA_PRESENCE_STREAM_ID));
    *dataPresenceStream >> container.presenceFlags;

	// update size info

	container.storedSize = container.size();

	// reconstruct derived data

	container.CalculateSumChanges();

	// ready

	return stream;
}

IHierarchicalOutStream& operator<< ( IHierarchicalOutStream& stream
								   , const CRevisionInfoContainer& container)
{
	const_cast<CRevisionInfoContainer*>(&container)->AutoOptimize();

	// write the pools

	IHierarchicalOutStream* authorPoolStream
		= stream.OpenSubStream ( CRevisionInfoContainer::AUTHOR_POOL_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
	*authorPoolStream << container.authorPool;

	IHierarchicalOutStream* commentsStream
		= stream.OpenSubStream ( CRevisionInfoContainer::COMMENTS_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
	*commentsStream << container.comments;

	IHierarchicalOutStream* pathsStream
		= stream.OpenSubStream ( CRevisionInfoContainer::PATHS_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
	*pathsStream << container.paths;

	// write the revision info

	CDiffIntegerOutStream* authorsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::AUTHORS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*authorsStream << container.authors;

	CPackedTime64OutStream* timeStampsStream 
		= dynamic_cast<CPackedTime64OutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::TIMESTAMPS_STREAM_ID
								  , PACKED_TIME64_STREAM_TYPE_ID));
	*timeStampsStream << container.timeStamps;

	CDiffIntegerOutStream* rootPathsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::ROOTPATHS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*rootPathsStream << container.rootPaths;

	CDiffDWORDOutStream* changesOffsetsStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::CHANGES_OFFSETS_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));
	*changesOffsetsStream << container.changesOffsets;

	CDiffDWORDOutStream* copyFromOffsetsStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::COPYFROM_OFFSETS_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));
	*copyFromOffsetsStream << container.copyFromOffsets;

	CDiffDWORDOutStream* mergedRevisionsOffsetsStream 
		= dynamic_cast<CDiffDWORDOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::MERGEDREVISION_OFFSETS_STREAM_ID
								  , DIFF_DWORD_STREAM_TYPE_ID));
	*mergedRevisionsOffsetsStream << container.mergedRevisionsOffsets;

	// write the changes info

	CPackedDWORDOutStream* changesStream 
		= dynamic_cast<CPackedDWORDOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::CHANGES_STREAM_ID
								  , PACKED_DWORD_STREAM_TYPE_ID));
	*changesStream << container.changes;

	CDiffIntegerOutStream* changedPathsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::CHANGED_PATHS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*changedPathsStream << container.changedPaths;

	CDiffIntegerOutStream* copyFromPathsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::COPYFROM_PATHS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*copyFromPathsStream << container.copyFromPaths;

	CDiffIntegerOutStream* copyFromRevisionsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::COPYFROM_REVISIONS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*copyFromRevisionsStream << container.copyFromRevisions;

	// merged revisions

	CDiffIntegerOutStream* mergedFromPathsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::MERGED_FROM_PATHS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*mergedFromPathsStream << container.mergedFromPaths;

	CDiffIntegerOutStream* mergedToPathsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::MERGED_TO_PATHS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*mergedToPathsStream << container.mergedToPaths;

	CDiffIntegerOutStream* mergedRangeStartsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::MERGED_RANGE_STARTS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*mergedRangeStartsStream << container.mergedRangeStarts;

	CDiffIntegerOutStream* mergedRangeDeltasStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::MERGED_RANGE_DELTAS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
	*mergedRangeDeltasStream << container.mergedRangeDeltas;

    // user-defined revision properties
    
	CDiffIntegerOutStream* userRevPropsOffsetsStream 
		= dynamic_cast<CDiffIntegerOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::USER_REVPROPS_OFFSETS_STREAM_ID
								  , DIFF_INTEGER_STREAM_TYPE_ID));
    *userRevPropsOffsetsStream << container.userRevPropOffsets;

	IHierarchicalOutStream* userRevPropsPoolStream
		= stream.OpenSubStream ( CRevisionInfoContainer::USER_REVPROPS_POOL_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
    *userRevPropsPoolStream << container.userRevPropsPool;

    CPackedDWORDOutStream* userRevPropsNameStream 
		= dynamic_cast<CPackedDWORDOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::USER_REVPROPS_NAME_STREAM_ID
								  , PACKED_DWORD_STREAM_TYPE_ID));
    *userRevPropsNameStream << container.userRevPropNames;

	IHierarchicalOutStream* userRevPropsValuesStream
		= stream.OpenSubStream ( CRevisionInfoContainer::USER_REVPROPS_VALUE_STREAM_ID
							   , COMPOSITE_STREAM_TYPE_ID);
    *userRevPropsValuesStream << container.userRevPropValues;

    // data presence flags

    CPackedDWORDOutStream* dataPresenceStream 
		= dynamic_cast<CPackedDWORDOutStream*>
			(stream.OpenSubStream ( CRevisionInfoContainer::DATA_PRESENCE_STREAM_ID
								  , PACKED_DWORD_STREAM_TYPE_ID));
    *dataPresenceStream << container.presenceFlags;

	// update size info

	container.storedSize = container.size();

	// ready

	return stream;
}

// end namespace LogCache

}
