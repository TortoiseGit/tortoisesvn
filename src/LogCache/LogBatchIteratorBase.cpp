#include "StdAfx.h"
#include "LogBatchIteratorBase.h"

// begin namespace LogCache

namespace LogCache
{

// find the top revision of all paths

revision_t CLogBatchIteratorBase::MaxRevision 
	(const TPathRevisions& pathRevisions)
{
	if (pathRevisions.empty())
		return NO_REVISION;
	
	revision_t result = pathRevisions[0].second;
	for ( TPathRevisions::const_iterator iter = pathRevisions.begin() + 1
		, end = pathRevisions.end()
		; iter != end
		; ++iter)
	{
		if (result < iter->second)
			result = iter->second;
	}

	return result;
}

// find the top revision of all paths

CDictionaryBasedTempPath CLogBatchIteratorBase::BasePath 
	( const CCachedLogInfo* cachedLog
	, const TPathRevisions& pathRevisions)
{
	// return a dummy path, if there is nothing to log

	if (pathRevisions.empty())
		return CDictionaryBasedTempPath (&cachedLog->GetLogInfo().GetPaths());
	
	// fold the paths

	CDictionaryBasedTempPath result = pathRevisions[0].first;
	for ( TPathRevisions::const_iterator iter = pathRevisions.begin() + 1
		, end = pathRevisions.end()
		; iter != end
		; ++iter)
	{
		result = result.GetCommonRoot (iter->first);
	}

	// ready

	return result;
}

// construction 
// (copy construction & assignment use default methods)

CLogBatchIteratorBase::CLogBatchIteratorBase 
	( const CCachedLogInfo* cachedLog
	, const TPathRevisions& startPathRevisions)
	: CLogIteratorBase ( cachedLog
					   , MaxRevision (startPathRevisions)
					   , BasePath (cachedLog, startPathRevisions))
	, pathRevisions (startPathRevisions)
{
}

// react on cache updates

void CLogBatchIteratorBase::HandleCacheUpdates()
{
	CLogIteratorBase::HandleCacheUpdates();

	// maybe, we can now use a shorter relative path

	for ( TPathRevisions::iterator iter = pathRevisions.begin()
		, end = pathRevisions.end()
		; iter != end
		; ++iter)
	{
		iter->first.RepeatLookup();
	}
}

// navigation sub-routines

revision_t CLogBatchIteratorBase::SkipNARevisions()
{
	const CSkipRevisionInfo& skippedRevisions = logInfo->GetSkippedRevisions();

	revision_t result = NO_REVISION;
	for ( TPathRevisions::const_iterator iter = pathRevisions.begin()
		, end = pathRevisions.end()
		; iter != end
		; ++iter)
	{
		// test at least the start revision of any path

		revision_t localResult = min (revision, iter->second);

		// still relevant?

		if ((result == NO_REVISION) || (localResult > result))
		{
			// skip revisions that are of no interest for this path

			localResult
				= skippedRevisions.GetPreviousRevision ( iter->first.GetBasePath()
													   , localResult);

			// keep the max. of all next relevant revisions for

			if ((result == NO_REVISION) || (localResult > result))
				result = localResult;
		}
	}

	// ready

	return result;
}

bool CLogBatchIteratorBase::PathInRevision() const
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

	// test all relevant paths

	for ( TPathRevisions::const_iterator iter = pathRevisions.begin()
		, end = pathRevisions.end()
		; iter != end
		; ++iter)
	{
		// is path relevant?

		if (   (iter->second >= revision)
			&& !PathsIntersect (iter->first.GetBasePath(), revisionRootPath))
		{
			// if we found one match, we are done

			if (CLogIteratorBase::PathInRevision (first, last, iter->first))
				return true;
		}
	}

	// no matching entry found

	return false;
}

// log scanning: scan on multiple paths

void CLogBatchIteratorBase::ToNextRevision()
{
	// find the next entry that might be relevant
	// for at least one of our paths

	CLogIteratorBase::InternalAdvance();
}

// destruction

CLogBatchIteratorBase::~CLogBatchIteratorBase(void)
{
}

// end namespace LogCache

}
