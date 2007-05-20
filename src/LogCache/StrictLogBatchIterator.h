#pragma once
#include "logbatchiteratorbase.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

class CStrictLogBatchIterator :	public CLogBatchIteratorBase
{
protected:

	// implement as no-op

	virtual bool HandleCopyAndDelete();

public:

	// construction / destruction 
	// (nothing special to do)

	CStrictLogBatchIterator ( const CCachedLogInfo* cachedLog
						    , const TPathRevisions& startPathRevisions);
	virtual ~CStrictLogBatchIterator(void);
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

