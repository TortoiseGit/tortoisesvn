#pragma once

///////////////////////////////////////////////////////////////
//
// define global types and their associated "invalid" values.
//
///////////////////////////////////////////////////////////////

namespace LogCache
{
#ifdef HUGE_LOG_CACHE

	typedef size_t index_t;
	typedef index_t revision_t;

	enum
	{
		NO_INDEX = (size_t)0xffffffffffffffff,
		NO_REVISION = NO_INDEX,
	};

#else

	typedef DWORD index_t;
	typedef index_t revision_t;

	enum
	{
		NO_INDEX = 0xffffffff,
		NO_REVISION = NO_INDEX,
	};

#endif
};