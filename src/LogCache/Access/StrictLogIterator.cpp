// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009, 2011 - TortoiseSVN

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
#include "stdafx.h"
#include "StrictLogIterator.h"

// begin namespace LogCache

namespace LogCache
{

// implement as no-op

bool CStrictLogIterator::HandleCopyAndDelete()
{
    // revision data lookup

    index_t index = revisionIndices[revision];
    if (index == NO_INDEX)
        return false;

    // switch to new path, if necessary

    bool result = InternalHandleCopyAndDelete ( revisionInfo.GetChangesBegin (index)
                                              , revisionInfo.GetChangesEnd (index)
                                              , revisionInfo.GetRootPath (index));
    if (result)
    {
        // stop on copy

        revision = (revision_t) NO_REVISION;
    }

    return result;
}

// construction / destruction
// (nothing special to do)

CStrictLogIterator::CStrictLogIterator ( const CCachedLogInfo* cachedLog
                                       , revision_t startRevision
                                       , const CDictionaryBasedTempPath& startPath)
    : CLogIteratorBase (cachedLog, startRevision, startPath)
{
}

CStrictLogIterator::~CStrictLogIterator (void)
{
}

// end namespace LogCache

}
