#pragma once
#include "logbatchiteratorbase.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

class CCopyFollowingLogBatchIterator :
	public CLogBatchIteratorBase
{
protected:

	// implement copy-following and termination-on-delete

	virtual bool HandleCopyAndDelete();

public:

	// construction / destruction 
	// (nothing special to do)

	CCopyFollowingLogBatchIterator ( const CCachedLogInfo* cachedLog
								   , const TPathRevisions& startPathRevisions);
	virtual ~CCopyFollowingLogBatchIterator(void);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

