#pragma once

///////////////////////////////////////////////////////////////
// necessary includes
///////////////////////////////////////////////////////////////

#include "PathDictionary.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

///////////////////////////////////////////////////////////////
//
// CDictionaryBasedTempPath
//
///////////////////////////////////////////////////////////////

class CDictionaryBasedTempPath : private CDictionaryBasedPath
{
private:

	typedef CDictionaryBasedPath inherited;

	// path elements to be added to the directory based path

	std::vector<std::string> relPathElements;

public:

	// construction / destruction

	explicit CDictionaryBasedTempPath (const CPathDictionary* dictionary)
		: inherited (dictionary, (index_t)NO_INDEX)
	{
	}

	explicit CDictionaryBasedTempPath (const CDictionaryBasedPath& path)
		: inherited (path)
	{
	}

	CDictionaryBasedTempPath ( const CPathDictionary* aDictionary
					  	     , const std::string& path);

	~CDictionaryBasedTempPath() 
	{
	}

	// data access

	const CPathDictionary* GetDictionary() const
	{
		return inherited::GetDictionary();
	}

	const CDictionaryBasedPath& GetBasePath() const
	{
		return *this;
	}

	// path operations

	bool IsValid() const
	{
		return inherited::IsValid();
	}

	bool IsFullyCachedPath() const
	{
		return relPathElements.empty();
	}

	CDictionaryBasedTempPath GetCommonRoot 
		(const CDictionaryBasedTempPath& rhs) const;

	// create a path object with the parent renamed

	CDictionaryBasedTempPath ReplaceParent 
		( const CDictionaryBasedPath& oldParent
		, const CDictionaryBasedPath& newParent);

	// call this after cache updates:
	// try to remove the leading entries from relPathElements, if possible

	void RepeatLookup();

	// convert to string

	std::string GetPath() const;
};

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}

