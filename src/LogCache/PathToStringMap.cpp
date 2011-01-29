// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009,2011 - TortoiseSVN

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

#include "PathToStringMap.h"

#include "UnicodeUtils.h"
#include "PathUtils.h"

///////////////////////////////////////////////////////////////
// CPathToStringMap
// construction
///////////////////////////////////////////////////////////////

CPathToStringMap::CPathToStringMap()
{
    data.insert ((LogCache::index_t)LogCache::NO_INDEX, std::string());
}

///////////////////////////////////////////////////////////////
// remove all entries
///////////////////////////////////////////////////////////////

void CPathToStringMap::Clear()
{
    data.clear();
}

///////////////////////////////////////////////////////////////
// auto-inserting lookup
///////////////////////////////////////////////////////////////

const std::string&
CPathToStringMap::AsString (const LogCache::CDictionaryBasedPath& path)
{
    TMap::const_iterator iter (data.find (path.GetIndex()));
    if (iter == data.end())
    {
        std::string utf8Path = path.GetPath();
        if (CPathUtils::ContainsEscapedChars(&utf8Path[0], utf8Path.length()))
            CPathUtils::Unescape (&utf8Path[0]);

        data.insert (path.GetIndex(), utf8Path);
        iter = data.find (path.GetIndex());
    }

    return *iter;
}
