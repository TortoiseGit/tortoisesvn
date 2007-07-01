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

	bool IsRoot() const
	{
		return inherited::IsRoot() && relPathElements.empty();
	}

	CDictionaryBasedTempPath GetCommonRoot 
		(const CDictionaryBasedTempPath& rhs) const;

	// create a path object with the parent renamed

	CDictionaryBasedTempPath ReplaceParent 
		( const CDictionaryBasedPath& oldParent
		, const CDictionaryBasedPath& newParent) const;

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

