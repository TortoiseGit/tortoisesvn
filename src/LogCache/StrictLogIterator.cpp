#include "StdAfx.h"
#include "StrictLogIterator.h"

// begin namespace LogCache

namespace LogCache
{

// implement as no-op

bool CStrictLogIterator::HandleCopyAndDelete()
{
	// revision data lookup

	const CRevisionInfoContainer& revisionInfo = logInfo->GetLogInfo();
	index_t index = logInfo->GetRevisions()[revision];

	// switch to new path, if necessary

	bool result = InternalHandleCopyAndDelete ( revisionInfo.GetChangesBegin(index)
											  , revisionInfo.GetChangesEnd(index)
											  , revisionInfo.GetRootPath (index)
											  , path
											  , revision);
	if (result)
	{
		// stop on copy

		revision = NO_REVISION;
	}

	return result;
}

// construction / destruction 
// (nothing special to do)

CStrictLogIterator::CStrictLogIterator ( const CCachedLogInfo* cachedLog
										 , revision_t startRevision
										 , const CDictionaryBasedTempPath& startPath)
	: CLogIteratorBase (cachedLog, startRevision, startPath)
{
}

CStrictLogIterator::~CStrictLogIterator(void)
{
}

// end namespace LogCache

}
