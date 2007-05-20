#pragma once
#include "logiteratorbase.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

class CStrictLogIterator :
	public CLogIteratorBase
{
protected:

	// implement as no-op

	virtual bool HandleCopyAndDelete();

public:

	// construction / destruction 
	// (nothing special to do)

	CStrictLogIterator ( const CCachedLogInfo* cachedLog
						, revision_t startRevision
						, const CDictionaryBasedTempPath& startPath);
	virtual ~CStrictLogIterator(void);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

