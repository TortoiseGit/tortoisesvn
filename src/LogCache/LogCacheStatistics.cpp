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
#include "LogCacheStatistics.h"
#include "CachedLogInfo.h"
#include "RepositoryInfo.h"
#include "LogCachePool.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
// utilities
///////////////////////////////////////////////////////////////

size_t CLogCacheStatistics::GetSizeOf (const CStringDictionary& container)
{
	return GetSizeOf (container.packedStrings)
		 + GetSizeOf (container.offsets)
		 + GetSizeOf (container.hashIndex)
		 + sizeof (container);
}

size_t CLogCacheStatistics::GetSizeOf (const CIndexPairDictionary& container)
{
	return GetSizeOf (container.data)
		 + GetSizeOf (container.hashIndex)
		 + sizeof (container);
}

size_t CLogCacheStatistics::GetSizeOf (const CPathDictionary& container)
{
	return GetSizeOf (container.pathElements)
		 + GetSizeOf (container.paths)
		 + sizeof (container);
}

size_t CLogCacheStatistics::GetSizeOf (const CTokenizedStringContainer& container)
{
	return GetSizeOf (container.words)
		 + GetSizeOf (container.pairs)
		 + GetSizeOf (container.stringData)
		 + GetSizeOf (container.offsets)
		 + sizeof (container);
}

size_t CLogCacheStatistics::GetSizeOf (const CRevisionInfoContainer& container)
{
	return GetSizeOf (container.presenceFlags)
		 + GetSizeOf (container.authors)
		 + GetSizeOf (container.timeStamps)
		 + GetSizeOf (container.rootPaths)
		 + GetSizeOf (container.sumChanges)
		 + GetSizeOf (container.changesOffsets)
		 + GetSizeOf (container.copyFromOffsets)
		 + GetSizeOf (container.mergedRevisionsOffsets)
		 + GetSizeOf (container.userRevPropOffsets)
		 + GetSizeOf (container.changes)
		 + GetSizeOf (container.changedPaths)
		 + GetSizeOf (container.copyFromPaths)
		 + GetSizeOf (container.copyFromRevisions)
		 + GetSizeOf (container.mergedFromPaths)
		 + GetSizeOf (container.mergedToPaths)
		 + GetSizeOf (container.mergedRangeStarts)
		 + GetSizeOf (container.mergedRangeDeltas)
		 + GetSizeOf (container.userRevPropValues)

		 + GetSizeOf (container.authorPool)
		 + GetSizeOf (container.paths)
		 + GetSizeOf (container.comments)
		 + GetSizeOf (container.userRevPropsPool)
		 + GetSizeOf (container.userRevPropValues)

		 + sizeof (container);
}

size_t CLogCacheStatistics::GetSizeOf (const CRevisionIndex& container)
{
	return GetSizeOf (container.indices)
		 + sizeof (container);
}

size_t CLogCacheStatistics::GetSizeOf (const CSkipRevisionInfo& container)
{
	size_t result = GetSizeOf (container.data)
				  + GetSizeOf (container.index)
				  + sizeof (container);

	for (size_t i = 0, count = container.data.size(); i < count; ++i)
	{
		const CSkipRevisionInfo::SPerPathRanges* entry = container.data[i];
		if (entry != NULL)
		{
			typedef CSkipRevisionInfo::SPerPathRanges::TRanges::_Tree_val T;
			result += entry->ranges.size() * sizeof (std::_Tree_val<T>)
				    + sizeof (*entry);
		}
	}

	return result;
}

size_t CLogCacheStatistics::GetSizeOf (const CCachedLogInfo& container)
{
	return GetSizeOf (container.revisions)
		 + GetSizeOf (container.logInfo)
		 + GetSizeOf (container.skippedRevisions)

		 + sizeof (container.fileName)
		 + sizeof (container.fileName.capacity() * sizeof (wchar_t))

		 + sizeof (container);
}

bool CLogCacheStatistics::CacheExists() const
{
	return fileSize > 0;
}

__time64_t CLogCacheStatistics::GetTime (FILETIME& fileTime)
{
	SYSTEMTIME systemTime;
	FileTimeToSystemTime (&fileTime, &systemTime);

	tm time = {0,0,0, 0,0,0, 0,0,0};
	time.tm_year = systemTime.wYear - 1900;
	time.tm_mon = systemTime.wMonth - 1;
	time.tm_mday = systemTime.wDay;
	time.tm_hour = systemTime.wHour;
	time.tm_min = systemTime.wMinute;
	time.tm_sec = systemTime.wSecond;

	return _mkgmtime64 (&time)*1000000 + systemTime.wMilliseconds * 1000;
}

///////////////////////////////////////////////////////////////
// data collection
///////////////////////////////////////////////////////////////

void CLogCacheStatistics::CollectData ( CLogCachePool& pool
									  , const CString& uuid)
{
	fileSize = 0;

	headTimeStamp = 0; 
	lastWriteAccess = 0;
	lastReadAccess = 0;

	// file properties

	CString fileName = pool.GetCacheFolderPath() + uuid;

	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	if (GetFileAttributesEx ( fileName
						    , GetFileExInfoStandard
							, &fileInfo) != FALSE)
	{
		fileSize = fileInfo.nFileSizeLow;
		lastReadAccess = GetTime (fileInfo.ftLastAccessTime);
		lastWriteAccess = GetTime (fileInfo.ftLastWriteTime);
	}

	// try to find the pool entry

	const CRepositoryInfo::TData& data = pool.GetRepositoryInfo().data;
	for ( CRepositoryInfo::TData::const_iterator iter = data.begin()
		, end = data.end()
		; iter != end
		; ++iter)
	{
		if (iter->second.uuid == uuid)
		{
			// found it (convert to apr_time_t)

			headTimeStamp = iter->second.headLookupTime * 1000000L;
		}
	}
}

void CLogCacheStatistics::CollectData (const CCachedLogInfo& source)
{
	// run-time properties

	ramSize = GetSizeOf (source);
	dirty = source.IsModified();

	// revisions

	revisionCount 
        = (revision_t) (  source.revisions.indices.size()
                        - std::count ( source.revisions.indices.begin()
									 , source.revisions.indices.end()
									 , NO_INDEX));
	maxRevision = source.revisions.GetLastCachedRevision()-1;

	// container sizes

	authorCount = source.logInfo.authorPool.size() -1;
	pathElementCount = source.logInfo.paths.pathElements.size();
	pathCount = source.logInfo.paths.size();
	
	skipDeltaCount = 0;
	for ( size_t i = 0, count = source.skippedRevisions.data.size(); i < count; ++i)
	{
		const CSkipRevisionInfo::SPerPathRanges* entry 
			= source.skippedRevisions.data[i];
		if (entry != NULL)
			skipDeltaCount += (index_t)entry->ranges.size();
	}

	wordTokenCount = source.logInfo.comments.words.size();
	pairTokenCount = source.logInfo.comments.pairs.size();
	textSize = (index_t)source.logInfo.comments.stringData.size();
	uncompressedSize = source.logInfo.comments.UncompressedWordCount();

	// changes

	changesRevisionCount = 0;
	changesMissingRevisionCount = 0;
	changesCount = 0;

	for (size_t i = 0, count = source.logInfo.size(); i < count; ++i)
	{
		index_t changeCount = source.logInfo.changesOffsets[i+1]
							- source.logInfo.changesOffsets[i];
		if (changeCount > 0)
		{
			++changesRevisionCount;
			changesCount += changeCount;
		}
		else
		{
			if ((  source.logInfo.presenceFlags[i] 
			     & CRevisionInfoContainer::HAS_CHANGEDPATHS) == 0)
				++changesMissingRevisionCount;
		}
	}

	// merge info

	mergeInfoRevisionCount = 0;
	mergeInfoMissingRevisionCount = 0;
	mergeInfoCount = 0;

	for (size_t i = 0, count = source.logInfo.size(); i < count; ++i)
	{
		index_t mergeCount = source.logInfo.mergedRevisionsOffsets[i+1]
					       - source.logInfo.mergedRevisionsOffsets[i];
		if (mergeCount > 0)
		{
			++mergeInfoRevisionCount;
			mergeInfoCount += mergeCount;
		}
		else
		{
			if ((  source.logInfo.presenceFlags[i] 
			     & CRevisionInfoContainer::HAS_MERGEINFO) == 0)
				++mergeInfoMissingRevisionCount;
		}
	}

	// user revision properties

	userRevPropRevisionCount = 0;
	userRevPropMissingRevisionCount = 0;
	userRevPropCount = 0;

	for (size_t i = 0, count = source.logInfo.size(); i < count; ++i)
	{
		index_t propCount = source.logInfo.userRevPropOffsets[i+1]
					      - source.logInfo.userRevPropOffsets[i];
		if (propCount > 0)
		{
			++userRevPropRevisionCount;
			userRevPropCount += propCount;
		}
		else
		{
			if ((  source.logInfo.presenceFlags[i] 
			     & CRevisionInfoContainer::HAS_USERREVPROPS) == 0)
				++userRevPropMissingRevisionCount;
		}
	}
}

///////////////////////////////////////////////////////////////
// construction / destruction:
// collect data during construction
///////////////////////////////////////////////////////////////

CLogCacheStatistics::CLogCacheStatistics()
{
	Reset();
}

CLogCacheStatistics::CLogCacheStatistics ( CLogCachePool& pool
										 , const CString& uuid)
{
	Reset();

	CollectData (pool, uuid);
	if (CacheExists())
		CollectData (*pool.GetCache (uuid));

    connectionState = pool.GetRepositoryInfo().GetConnectionState (uuid);
}

CLogCacheStatistics::~CLogCacheStatistics()
{
}

///////////////////////////////////////////////////////////////
// all back to zero
///////////////////////////////////////////////////////////////

void CLogCacheStatistics::Reset()
{
	fileSize = 0;
	ramSize = 0;

    connectionState = CRepositoryInfo::online;

	headTimeStamp = 0; 
	lastWriteAccess = 0;
	lastReadAccess = 0;
	dirty = false;

	revisionCount = 0;
	maxRevision = 0;

	authorCount = 0;
	pathCount = 0;
	skipDeltaCount = 0;

	wordTokenCount = 0;
	pairTokenCount = 0;
	textSize = 0;

	changesRevisionCount = 0;
	changesMissingRevisionCount = 0;
	changesCount = 0;

	mergeInfoRevisionCount = 0;
	mergeInfoMissingRevisionCount = 0;
	mergeInfoCount = 0;

	userRevPropRevisionCount = 0;
	userRevPropMissingRevisionCount = 0;
	userRevPropCount = 0;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

