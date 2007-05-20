#pragma once
#include "logiteratorbase.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

class CCopyFollowingLogIterator :
	public CLogIteratorBase
{
protected:

	// implement copy-following and termination-on-delete

	virtual bool HandleCopyAndDelete();

public:

	// construction / destruction 
	// (nothing special to do)

	CCopyFollowingLogIterator ( const CCachedLogInfo* cachedLog
		                      , revision_t startRevision
							  , const CDictionaryBasedTempPath& startPath);
	virtual ~CCopyFollowingLogIterator(void);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

