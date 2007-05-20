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

#include "CacheLogQuery.h"

#include "CachedLogInfo.h"
#include "CopyFollowingLogIterator.h"
#include "StrictLogIterator.h"
#include "LogCachePool.h"

#include "svn_client.h"
#include "svn_opt.h"

#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "SVN.h"
#include "SVNInfo.h"
#include "SVNError.h"

// implement ILogReceiver

void CCacheLogQuery::CLogFiller::ReceiveLog ( LogChangedPathArray* changes
											, svn_revnum_t rev
											, const CString& author
											, const apr_time_t& timeStamp
											, const CString& message)
{
	// add it to the cache, if it is not in there, yet

	revision_t revision = static_cast<revision_t>(rev);
	if (cache->GetRevisions()[revision] == NO_INDEX)
	{
		// create the revision entry

		cache->Insert ( revision
					  , (const char*)CUnicodeUtils::GetUTF8 (author)
					  , (const char*)CUnicodeUtils::GetUTF8 (message)
					  , timeStamp);

		// add all changes

		for (INT_PTR i = 0, count = changes->GetCount(); i < count; ++i)
		{
			const LogChangedPath* change = changes->GetAt (i);

			CRevisionInfoContainer::TChangeAction action 
				= (CRevisionInfoContainer::TChangeAction)(change->action * 4);
			std::string path 
				= (const char*)SVN::MakeSVNUrlOrPath (change->sPath);
			std::string copyFromPath 
				= (const char*)SVN::MakeSVNUrlOrPath (change->sCopyFromPath);
			revision_t copyFromRevision 
				= change->lCopyFromRev == 0 
				? NO_REVISION 
				: static_cast<revision_t>(change->lCopyFromRev);

			cache->AddChange (action, path, copyFromPath, copyFromRevision);
		}

		// update our path info

		currentPath->RepeatLookup();
	}

	// mark the gap and update the current path

	if (firstNARevision > revision)
	{
		assert (currentPath->IsFullyCachedPath());

		cache->AddSkipRange ( currentPath->GetBasePath()
							, revision+1
							, firstNARevision - revision);
	}

	if (followRenames)
	{
		CCopyFollowingLogIterator iterator (cache, revision, *currentPath);
		iterator.Advance();

		*currentPath = iterator.GetPath();
	}

	firstNARevision = revision-1;

	// hand on to the original log receiver

	if (receiver != NULL)
		receiver->ReceiveLog (changes, rev, author, timeStamp, message);
}

// actually call SVN
// return the last revision sent to the receiver

revision_t 
CCacheLogQuery::CLogFiller::FillLog ( CCachedLogInfo* cache
								    , const CStringA& URL
								    , ILogQuery* svnQuery
								    , revision_t startRevision
									, revision_t endRevision
									, const CDictionaryBasedTempPath& startPath
									, int limit
									, bool strictNodeHistory
								    , ILogReceiver* receiver)
{
	this->cache = cache;
	this->URL = URL;
	this->svnQuery = svnQuery;
	this->receiver = receiver;

	firstNARevision = startRevision;
	currentPath.reset (new CDictionaryBasedTempPath (startPath));
	followRenames = !strictNodeHistory;

	CTSVNPath path;
	path.SetFromSVN (URL + startPath.GetPath().c_str());

	svnQuery->Log ( CTSVNPathList (path)
				  , static_cast<long>(startRevision)
				  , static_cast<long>(startRevision)
				  , static_cast<long>(endRevision)
			      , limit
				  , strictNodeHistory
				  , this);

	if (   (firstNARevision == startRevision) 
		&& currentPath->IsFullyCachedPath())
	{
		// the log was empty

		cache->AddSkipRange ( currentPath->GetBasePath()
							, endRevision
							, startRevision - endRevision + 1);
	}

	return firstNARevision+1;
}

// when asking SVN to fill our cache, we want it to cover
// the missing revisions as well as the ones that we already
// know to have no info for this path. Otherwise, we may end
// up creating a lot of queries for paths that are seldom
// modified.

revision_t 
CCacheLogQuery::NextAvailableRevision ( const CDictionaryBasedTempPath& path
									  , revision_t startRevision
									  , revision_t endRevision) const
{
	const CRevisionIndex& revisions = cache->GetRevisions();
	revision_t lastRevisionToCheck = min ( endRevision
										 , revisions.GetFirstRevision());

	while ((startRevision >= endRevision) && (startRevision != NO_REVISION))
	{
		// skip known revisions that are irrelevant for path

		CStrictLogIterator iterator (cache, startRevision, path);
		iterator.Retry();
		startRevision = iterator.GetRevision();

		// found the next cache entry for this path?

		if (!iterator.DataIsMissing())
			return startRevision+1;

		// skip N/A revisions

		while (   (startRevision >= lastRevisionToCheck) 
			   && (startRevision != NO_REVISION)
			   && (revisions[startRevision] == NO_INDEX))
		{
			--startRevision;
		}
	}

	// there is no cached data available for this path and revision range

	return endRevision-1;
}

// ask SVN to fill the log -- at least a bit
// Possibly, it will stop long before endRevision and limit!

revision_t CCacheLogQuery::FillLog ( revision_t startRevision
								   , revision_t endRevision
								   , const CDictionaryBasedTempPath& startPath
								   , int limit
								   , bool strictNodeHistory
								   , ILogReceiver* receiver)
{
	// don't try to get a full log; just enough to continue our search

	endRevision = NextAvailableRevision ( startPath
										, startRevision
										, endRevision);

	// now, fill the cache (somewhat) and forward to the receiver

	return CLogFiller().FillLog ( cache
								, URL
								, svnQuery
								, startRevision
								, max (min (startRevision, endRevision), 1)
								, startPath
								, limit
								, strictNodeHistory
								, receiver);
}

// fill the receiver's change list buffer 

std::auto_ptr<LogChangedPathArray>
CCacheLogQuery::GetChanges ( CRevisionInfoContainer::CChangesIterator& first
						   , CRevisionInfoContainer::CChangesIterator& last)
{
	std::auto_ptr<LogChangedPathArray> result (new LogChangedPathArray);

	for (; first != last; ++first)
	{
		// find the item in the hash

		std::auto_ptr<LogChangedPath> changedPath (new LogChangedPath);

		// extract the path name

		std::string path = first.GetPath().GetPath();
		changedPath->sPath = SVN::MakeUIUrlOrPath (path.c_str());

		// decode the action

		CRevisionInfoContainer::TChangeAction action = first.GetAction();
		changedPath->action = (DWORD)action / 4;

		// decode copy-from info

		if (   first.HasFromPath()
			&& (first.GetFromRevision() != NO_REVISION))
		{
			std::string path = first.GetFromPath().GetPath();

			changedPath->lCopyFromRev = first.GetFromRevision();
			changedPath->sCopyFromPath 
				= SVN::MakeUIUrlOrPath (path.c_str());
		}
		else
		{
			changedPath->lCopyFromRev = 0;
		}

		result->Add (changedPath.release());
	} 

	return result;
}

// crawl the history and forward it to the receiver

void CCacheLogQuery::InternalLog ( revision_t startRevision
								 , revision_t endRevision
								 , const CDictionaryBasedTempPath& startPath
								 , int limit
								 , bool strictNodeHistory
								 , ILogReceiver* receiver)
{
	// create the right iterator

	std::auto_ptr<ILogIterator> iterator;
	if (strictNodeHistory)
		iterator.reset (new CStrictLogIterator ( cache
											   , startRevision
											   , startPath));
	else
		iterator.reset (new CCopyFollowingLogIterator ( cache
													  , startRevision
													  , startPath));

	// find first suitable entry or cache gap

	iterator->Retry();

	// report starts at endRevision or earlier revisions

	revision_t lastReported = startRevision+1;

	// crawl & update the cache, report entries found

	while ((iterator->GetRevision() >= endRevision) && !iterator->EndOfPath())
	{
		if (iterator->DataIsMissing())
		{
			// our cache is incomplete -> fill it.
			// Report entries immediately to the receiver 
			// (as to allow the user to cancel this action).

			lastReported = FillLog ( iterator->GetRevision()
								   , endRevision
								   , iterator->GetPath()
								   , limit
								   , strictNodeHistory
								   , receiver);

			iterator->Retry();
		}
		else
		{
			// found an entry. Report it if not already done.

			revision_t revision = iterator->GetRevision();
			if (revision < lastReported)
			{
				index_t logIndex = cache->GetRevisions()[revision];
				const CRevisionInfoContainer& logInfo = cache->GetLogInfo();

				CStringA author = logInfo.GetAuthor (logIndex);
				CStringA comment = logInfo.GetComment (logIndex).c_str();
				std::auto_ptr<LogChangedPathArray> changes
					= GetChanges ( logInfo.GetChangesBegin (logIndex)
								 , logInfo.GetChangesEnd (logIndex));

				receiver->ReceiveLog ( changes.release()
									 , revision
									 , CUnicodeUtils::GetUnicode (author)
									 , logInfo.GetTimeStamp (logIndex)
									 , CUnicodeUtils::GetUnicode (comment));
			}

			// enough?

			if ((limit != 0) && (--limit == 0))
				return;
			else
				iterator->Advance();
		}
	}
}

// follow copy history until the startRevision is reached

CDictionaryBasedTempPath CCacheLogQuery::TranslatePegRevisionPath 
	( revision_t pegRevision
	, revision_t startRevision
	, const CDictionaryBasedTempPath& startPath)
{
	CCopyFollowingLogIterator iterator (cache, pegRevision, startPath);
	iterator.Retry();

	while ((iterator.GetRevision() > startRevision) && !iterator.EndOfPath())
	{
		if (iterator.DataIsMissing())
			FillLog ( iterator.GetRevision()
					, startRevision
					, iterator.GetPath()
					, 0
					, false
					, NULL);

		iterator.Advance();
	}

	return iterator.GetPath();
}

// extract the repository-relative path of the URL / file name
// and open the cache

CDictionaryBasedTempPath CCacheLogQuery::GetRelativeRepositoryPath (SVNInfoData& info)
{
	// load cache

	cache = caches->GetCache (info.reposUUID);
	URL = CUnicodeUtils::GetUTF8 (info.reposRoot);

	// get path object

	CStringA relPath = CUnicodeUtils::GetUTF8 (info.url).Mid (URL.GetLength());
	const CPathDictionary* paths = &cache->GetLogInfo().GetPaths();

	return CDictionaryBasedTempPath (paths, (const char*)relPath);
}

// get UUID & repository-relative path

SVNInfoData& CCacheLogQuery::GetRepositoryInfo ( const CTSVNPath& path
 								               , SVNInfoData& baseInfo
									           , SVNInfoData& headInfo) const
{
	// already known?

	if (baseInfo.IsValid())
		return baseInfo;

	if (headInfo.IsValid())
		return headInfo;

	// look it up

	if (path.IsUrl())
	{
		headInfo = *SVNInfo().GetFirstFileInfo (path, SVNRev(), CString (_T("HEAD")));
		return headInfo;
	}
	else
	{
		baseInfo = *SVNInfo().GetFirstFileInfo (path, SVNRev(), SVNRev());
		return baseInfo;
	}
}

// decode special revisions:
// base / head must be initialized with NO_REVISION
// and will be used to cache these values.

revision_t CCacheLogQuery::DecodeRevision ( const CTSVNPath& path
			  							  , const SVNRev& revision
										  , SVNInfoData& baseInfo
										  , SVNInfoData& headInfo) const
{
	if (!revision.IsValid())
		throw SVNError ( SVN_ERR_CLIENT_BAD_REVISION
					   , "Invalid revision passed to Log().");

	// efficently decode standard cases: revNum, HEAD, BASE/WORKING

	switch (revision.GetKind())
	{
	case svn_opt_revision_number:
		return static_cast<LONG>(revision);

	case svn_opt_revision_head:
		if (!headInfo.IsValid())
			headInfo = *SVNInfo().GetFirstFileInfo (path, SVNRev(), revision);

		return static_cast<LONG>(headInfo.rev);

	case svn_opt_revision_base:
	case svn_opt_revision_working:
		if (!baseInfo.IsValid())
			baseInfo = *SVNInfo().GetFirstFileInfo (path, SVNRev(), SVNRev());

		return static_cast<LONG>(baseInfo.rev);
	}

	// more unusual cases:

	SVNInfo infoProvider;
	const SVNInfoData* info 
		= infoProvider.GetFirstFileInfo (path, SVNRev(), revision);

	return static_cast<LONG>(info->rev);
}

// get the (exactly) one path from targets
// throw an exception, if there are none or more than one

CTSVNPath CCacheLogQuery::GetPath (const CTSVNPathList& targets) const
{
	if (targets.GetCount() != 1)
		throw SVNError ( SVN_ERR_INCORRECT_PARAMS
					   , "Must specify exactly one path to get the log from.");

	return targets [0];
}

// construction / destruction

CCacheLogQuery::CCacheLogQuery (CLogCachePool* caches, ILogQuery* svnQuery)
	: caches (caches)
	, cache (NULL)
	, URL()
	, svnQuery (svnQuery)
{
}

CCacheLogQuery::~CCacheLogQuery(void)
{
}

// query a section from log for multiple paths
// (special revisions, like "HEAD", supported)

void CCacheLogQuery::Log ( const CTSVNPathList& targets
						 , const SVNRev& peg_revision
						 , const SVNRev& start
						 , const SVNRev& end
						 , int limit
						 , bool strictNodeHistory
						 , ILogReceiver* receiver)
{
	// the path to log for

	CTSVNPath path = GetPath (targets);

	// decode revisions

	SVNInfoData baseInfo;
	SVNInfoData headInfo;

	revision_t startRevision = DecodeRevision (path, start, baseInfo, headInfo);
	revision_t endRevision = DecodeRevision (path, end, baseInfo, headInfo);
	revision_t pegRevision = DecodeRevision (path, peg_revision, baseInfo, headInfo);

	// order revisions

	if (endRevision > startRevision)
		std::swap (endRevision, startRevision);

	if (pegRevision < startRevision)
		pegRevision = startRevision;

	// load cache and find path to start from

	SVNInfoData& info = GetRepositoryInfo (path, baseInfo, headInfo);

	CDictionaryBasedTempPath startPath 
		= TranslatePegRevisionPath ( pegRevision
								   , startRevision
								   , GetRelativeRepositoryPath (info));

	// do it 

	InternalLog ( startRevision
				, endRevision
				, startPath
				, limit
				, strictNodeHistory
				, receiver);
}
