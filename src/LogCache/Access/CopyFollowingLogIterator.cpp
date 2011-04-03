// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009,2011 - TortoiseSVN

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
#include "CopyFollowingLogIterator.h"

// begin namespace LogCache

namespace LogCache
{

// Change the path we are iterating the log for,
// if there is a copy / replace.
// Set revision to NO_REVISION, if path is deleted.

bool CCopyFollowingLogIterator::HandleCopyAndDelete()
{
    assert (!InternalDataIsMissing());

    // revision data lookup

    index_t index = revisionIndices[revision];

    // switch to new path, if necessary

    return InternalHandleCopyAndDelete ( revisionInfo.GetChangesBegin(index)
                                       , revisionInfo.GetChangesEnd(index)
                                       , revisionInfo.GetRootPath (index));
}

// construction / destruction
// (nothing special to do)

CCopyFollowingLogIterator::CCopyFollowingLogIterator
    ( const CCachedLogInfo* cachedLog
    , revision_t startRevision
    , const CDictionaryBasedTempPath& startPath)
    : CLogIteratorBase (cachedLog, startRevision, startPath)
{
}

CCopyFollowingLogIterator::~CCopyFollowingLogIterator(void)
{
}

// end namespace LogCache

}
