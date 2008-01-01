// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "StdAfx.h"
#include "DictionaryBasedTempPath.h"

///////////////////////////////////////////////////////////////
// begin namespace LogCache
///////////////////////////////////////////////////////////////

namespace LogCache
{

// construction / destruction

CDictionaryBasedTempPath::CDictionaryBasedTempPath 
	( const CPathDictionary* aDictionary
	, const std::string& path)
	: inherited (aDictionary, std::string())
{
	ParsePath (path, NULL, &relPathElements);
}

CDictionaryBasedTempPath CDictionaryBasedTempPath::GetCommonRoot 
	(const CDictionaryBasedTempPath& rhs) const
{
	// short-cut: different base directories anyway?

	if (rhs.GetIndex() != GetIndex())
		return CDictionaryBasedTempPath (inherited::GetCommonRoot (rhs));

	// match relative paths

	typedef std::vector<std::string>::const_iterator IT;
	IT begin = relPathElements.begin();
	IT end = relPathElements.end();
	IT iter = begin;

	IT rhsEnd = relPathElements.end();
	IT rhsIter = relPathElements.begin();

	while ((iter != end) && (rhsIter != rhsEnd) && (*iter == *rhsIter))
	{
		++iter;
		++rhsIter;
	}

	// construct the result

	CDictionaryBasedTempPath result (GetBasePath());
	result.relPathElements.insert (result.relPathElements.begin(), begin, iter);

	return result;
}


// general comparison

bool CDictionaryBasedTempPath::operator==(const CDictionaryBasedTempPath& rhs) const
{
    return (GetBasePath() == rhs.GetBasePath())
        && (relPathElements.size() == rhs.relPathElements.size())
        && (std::equal ( relPathElements.begin()
                       , relPathElements.end()
                       , rhs.relPathElements.begin()));
}

// create a path object with the parent renamed

CDictionaryBasedTempPath CDictionaryBasedTempPath::ReplaceParent 
	( const CDictionaryBasedPath& oldParent
	, const CDictionaryBasedPath& newParent) const
{
	assert (oldParent.IsSameOrParentOf (*this));

	// I admit, this is the most stupid implementation possible ;)

	std::string newPath = newParent.GetPath() 
						+ GetPath().substr (oldParent.GetPath().length());

	return CDictionaryBasedTempPath (GetDictionary(), newPath);
}

// call this after cache updates:
// try to remove the leading entries from relPathElements, if possible

void CDictionaryBasedTempPath::RepeatLookup()
{
	// optimization

	if (relPathElements.empty())
		return;

	// some preparation ...

	const CPathDictionary* dictionary = GetDictionary();
	index_t currentIndex = GetIndex();

	typedef std::vector<std::string>::iterator IT;
	IT begin = relPathElements.begin();

	// try to match the relative path elements with cached paths, step by step

	for (IT iter = begin, end = relPathElements.end(); iter != end; ++iter)
	{
		index_t nextIndex = dictionary->Find (currentIndex, iter->c_str());
		if (nextIndex == NO_INDEX)
		{
			// new dictionary-based path info ends here.
			// update internal data, if we made any progress

			if (iter != begin)
			{
				assert (currentIndex != nextIndex);

				relPathElements.erase (begin, iter);
				SetIndex (currentIndex);
			}
			return;
		}
		else
			currentIndex = nextIndex;
	}

	// cool. a full match

	if (GetIndex() != currentIndex)
	{
		relPathElements.clear();
		SetIndex (currentIndex);
	}
}

// convert to string

std::string CDictionaryBasedTempPath::GetPath() const
{
	std::string result = inherited::GetPath();

	// efficiently add the relative path

	size_t count = relPathElements.size();
	if (count > 0)
	{
		// pre-allocate sufficient memory (for performance)

		size_t additionalSize = count;
		for (size_t i = 0; i < count; ++i)
			additionalSize += relPathElements[i].length();

		result.reserve (result.size() + additionalSize);

		// actually extend the path

		for (size_t i = 0; i < count; ++i)
		{
			// don't add a slash if base path is /

			if (result.length() > 1)
				result.push_back ('/');

			result.append (relPathElements[i]);
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////
// end namespace LogCache
///////////////////////////////////////////////////////////////

}
