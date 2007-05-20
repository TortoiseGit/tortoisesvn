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
#include "CopyFollowingLogBatchIterator.h"

// begin namespace LogCache

namespace LogCache
{

// Change the path we are iterating the log for,
// if there is a copy / replace.
// Set revision to NO_REVISION, if path is deleted.

bool CCopyFollowingLogBatchIterator::HandleCopyAndDelete()
{
	assert (!InternalDataIsMissing());

	// revision data lookup

	const CRevisionInfoContainer& revisionInfo = logInfo->GetLogInfo();
	index_t index = logInfo->GetRevisions()[revision];

	// fetch invariant information

	CDictionaryBasedPath revisionRootPath 
		= revisionInfo.GetRootPath (index);

	CRevisionInfoContainer::CChangesIterator first 
		= revisionInfo.GetChangesBegin (index);
	CRevisionInfoContainer::CChangesIterator last 
		= revisionInfo.GetChangesEnd (index);

	// do we need to have a closer look?

	if (!ContainsCopyOrDelete (first, last))
		return false;

	// close examination for all paths

	bool modified = false;
	for ( TPathRevisions::iterator iter = pathRevisions.begin()
		, end = pathRevisions.end()
		; iter != end
		; ++iter)
	{
		// is path relevant?

		if (   (iter->second >= revision)
			&& !PathsIntersect (iter->first.GetBasePath(), revisionRootPath))
		{
			// update this path and its revision, if necessary

			modified |= InternalHandleCopyAndDelete ( first
													, last
													, revisionRootPath
													, iter->first
													, revision);
		}
	}

	// has any of our paths / revisions changed?

	if (!modified)
		return false;

	// remove deleted paths

	for ( TPathRevisions::iterator iter = pathRevisions.begin()
		, end = pathRevisions.end()
		; iter != end
		; )
	{
		if (iter->second == NO_REVISION)
		{
			iter->swap (*--end);
			pathRevisions.pop_back();
		}
		else
		{
			++iter;
		}
	}

	// update folder information

	revision = MaxRevision (pathRevisions);
	path = BasePath (logInfo, pathRevisions);

	// we did change at least one path

	return true;
}

// construction / destruction 
// (nothing special to do)

CCopyFollowingLogBatchIterator::CCopyFollowingLogBatchIterator 
	( const CCachedLogInfo* cachedLog
	, const TPathRevisions& startPathRevisions)
	: CLogBatchIteratorBase (cachedLog, startPathRevisions)
{
}

CCopyFollowingLogBatchIterator::~CCopyFollowingLogBatchIterator(void)
{
}

// end namespace LogCache

}
