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
#include ".\cachedloginfo.h"

#include "RootInStream.h"
#include "RootOutStream.h"

// begin namespace LogCache

namespace LogCache
{

// construction / destruction (nothing to do)

CCachedLogInfo::CCachedLogInfo()
	: fileName()
	, revisions()
	, logInfo()
	, skippedRevisions (logInfo.GetPaths(), revisions, logInfo)
	, modified (false)
	, revisionAdded (false)
{
}

CCachedLogInfo::CCachedLogInfo (const std::wstring& aFileName)
	: fileName (aFileName)
	, revisions()
	, logInfo()
	, skippedRevisions (logInfo.GetPaths(), revisions, logInfo)
	, modified (false)
	, revisionAdded (false)
{
}

CCachedLogInfo::~CCachedLogInfo (void)
{
}

// cache persistence

void CCachedLogInfo::Load()
{
	assert (revisions.GetLastRevision() == 0);

	try
	{
		CRootInStream stream (fileName);

		// read the data

		IHierarchicalInStream* revisionsStream
			= stream.GetSubStream (REVISIONS_STREAM_ID);
		*revisionsStream >> revisions;

		IHierarchicalInStream* logInfoStream
			= stream.GetSubStream (LOG_INFO_STREAM_ID);
		*logInfoStream >> logInfo;

		IHierarchicalInStream* skipRevisionsStream
			= stream.GetSubStream (SKIP_REVISIONS_STREAM_ID);
		*skipRevisionsStream >> skippedRevisions;
	}
	catch (...)
	{
		// if there was a problem, the cache file is probably corrupt
		// -> don't use its data
	}
}

void CCachedLogInfo::Save (const std::wstring& newFileName)
{
	CRootOutStream stream (newFileName);

	// write the data

	IHierarchicalOutStream* revisionsStream
		= stream.OpenSubStream (REVISIONS_STREAM_ID, COMPOSITE_STREAM_TYPE_ID);
	*revisionsStream << revisions;

	IHierarchicalOutStream* logInfoStream
		= stream.OpenSubStream (LOG_INFO_STREAM_ID, COMPOSITE_STREAM_TYPE_ID);
	*logInfoStream << logInfo;

	IHierarchicalOutStream* skipRevisionsStream
		= stream.OpenSubStream (SKIP_REVISIONS_STREAM_ID, COMPOSITE_STREAM_TYPE_ID);
	*skipRevisionsStream << skippedRevisions;

	// all fine -> connect to the new file name

	fileName = newFileName;

    // the data is no longer "modified"

    modified = false;
}

// data modification (mirrors CRevisionInfoContainer)

void CCachedLogInfo::Insert ( revision_t revision
							 , const std::string& author
							 , const std::string& comment
							 , __time64_t timeStamp
                             , char flags)
{
	// there will be a modification

	modified = true;

	// add entry to cache and update the revision index

	index_t index = logInfo.Insert (author, comment, timeStamp, flags);
	revisions.SetRevisionIndex (revision, index);

	// you may call AddChange() now

	revisionAdded = true;
}

void CCachedLogInfo::AddSkipRange ( const CDictionaryBasedPath& path
								  , revision_t startRevision
								  , revision_t count)
{
	modified = true;

	skippedRevisions.Add (path, startRevision, count);
}

void CCachedLogInfo::Clear()
{
	modified = revisions.GetLastRevision() != 0;
	revisionAdded = false;

	revisions.Clear();
	logInfo.Clear();
}

// update / modify existing data

void CCachedLogInfo::Update ( const CCachedLogInfo& newData
							, char flags
                            , bool keepOldDataForMissingNew)
{
	// build revision index map

	index_mapping_t indexMap;

	index_t newIndex = logInfo.size();
	for ( revision_t i = newData.revisions.GetFirstRevision()
		, last = newData.revisions.GetLastRevision()
		; i < last
		; ++i)
	{
		index_t sourceIndex = newData.revisions[i];
		if (sourceIndex != NO_INDEX)
		{
			index_t destIndex = revisions[i];
			if (destIndex == NO_INDEX)
				destIndex = newIndex++;

			indexMap.insert (destIndex, sourceIndex);
		}
	}

	// update our log info

	logInfo.Update ( newData.logInfo
				   , indexMap
				   , flags
                   , keepOldDataForMissingNew);

	// our skip ranges should still be valid
	// but we check them anyway

	skippedRevisions.Compress();

    // this cache has been touched

    modified = true;
}

// end namespace LogCache

}

