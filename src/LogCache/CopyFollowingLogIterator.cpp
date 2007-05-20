#include "StdAfx.h"
#include "CopyFollowingLogIterator.h"

// begin namespace LogCache

namespace LogCache
{

// Change the path we are iterating the log for,
// if there is a copy / replace.
// Set revision to NO_REVISION, if path is deleted.

bool CCopyFollowingLogIterator::HandleCopyAndDelete()
{
	assert (!InternalDataIsMissing());

	// revision data lookup

	const CRevisionInfoContainer& revisionInfo = logInfo->GetLogInfo();
	index_t index = logInfo->GetRevisions()[revision];

	// switch to new path, if necessary

	return InternalHandleCopyAndDelete ( revisionInfo.GetChangesBegin(index)
									   , revisionInfo.GetChangesEnd(index)
									   , revisionInfo.GetRootPath (index)
									   , path
									   , revision);
}

// construction / destruction 
// (nothing special to do)

CCopyFollowingLogIterator::CCopyFollowingLogIterator 
	( const CCachedLogInfo* cachedLog
	, revision_t startRevision
	, const CDictionaryBasedTempPath& startPath)
	: CLogIteratorBase (cachedLog, startRevision, startPath)
{
}

CCopyFollowingLogIterator::~CCopyFollowingLogIterator(void)
{
}

// end namespace LogCache

}
