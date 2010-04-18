// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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
    data.insert ((LogCache::index_t)LogCache::NO_INDEX, CString());
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

const CString&
CPathToStringMap::AsString (const LogCache::CDictionaryBasedPath& path)
{
    TMap::const_iterator iter (data.find (path.GetIndex()));
    if (iter == data.end())
    {
        std::string utf8Path = path.GetPath();
        CPathUtils::Unescape (&utf8Path[0]);
        CString s = CUnicodeUtils::UTF8ToUTF16 (utf8Path);

        data.insert (path.GetIndex(), s);
        iter = data.find (path.GetIndex());
    }

    return *iter;
}
