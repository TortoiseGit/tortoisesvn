// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2009, 2011-2012 - TortoiseSVN 

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
#include "LogIteratorBase.h"

// begin namespace LogCache

namespace LogCache
{

// react on cache updates

void CLogIteratorBase::HandleCacheUpdates()
{
    // maybe, we can now use a shorter relative path

    path.RepeatLookup();
}

// is current revision actually relevant?

bool CLogIteratorBase::PathInRevision
    ( const CRevisionInfoContainer::CChangesIterator& first
    , const CRevisionInfoContainer::CChangesIterator& last
    , const CDictionaryBasedTempPath& path)
{
    // close examination of all changes

    for ( CRevisionInfoContainer::CChangesIterator iter = first
        ; iter != last
        ; ++iter)
    {
        // if (and only if) path is a cached path,
        // it may be a parent of the changedPath
        // (i.e. report a change of this or some sub-path)

        CDictionaryBasedPath changedPath = iter->GetPath();
        if (   path.IsFullyCachedPath()
            && path.GetBasePath().IsSameOrParentOf (changedPath))
            return true;

        // this change affects a true parent path or completely unrelated path
        // -> ignore mere modifications (e.g. properties on a folder)

        if (iter->GetAction() == CRevisionInfoContainer::ACTION_CHANGED)
            continue;

        // this is an add / delete / replace.
        // does it affect our path?

        if (changedPath.IsSameOrParentOf (path.GetBasePath()))
            return true;
    }

    // no paths that we were looking for

    return false;
}

bool CLogIteratorBase::PathInRevision() const
{
    assert (!InternalDataIsMissing());

    // special case: repository root
    // (report all revisions including empty ones)

    if (path.IsRoot())
        return true;

    // if we don't have a path, we won't find any changes

    if (!path.IsValid())
        return false;

    // revision data lookup

    revision_t index = revisionIndices[revision];

    // any chance that this revision affects our path?

    CDictionaryBasedPath revisionRootPath = revisionInfo.GetRootPath (index);
    if (!revisionRootPath.IsValid())
        return false;

    if (!path.GetBasePath().Intersects (revisionRootPath))
        return false;

    // close examination of all changes

    return PathInRevision ( revisionInfo.GetChangesBegin(index)
                          , revisionInfo.GetChangesEnd(index)
                          , path);
}

// Test, whether InternalHandleCopyAndDelete() should be used

bool CLogIteratorBase::ContainsCopyOrDelete
    ( const CRevisionInfoContainer::CChangesIterator& first
    , const CRevisionInfoContainer::CChangesIterator& last)
{
    // close examination of all changes

    for (CRevisionInfoContainer::CChangesIterator iter = first
        ; iter != last
        ; ++iter)
    {
        // the only non-critical operation is the mere modification

        CRevisionInfoContainer::TChangeAction action = iter.GetAction();
        if (action != CRevisionInfoContainer::ACTION_CHANGED)
            return true;
    }

    // no copy / delete / replace found

    return false;
}

// Change the path we are iterating the log for,
// if there is a copy / replace.
// Set revision to NO_REVISION, if path is deleted.

bool CLogIteratorBase::InternalHandleCopyAndDelete
    ( const CRevisionInfoContainer::CChangesIterator& first
    , const CRevisionInfoContainer::CChangesIterator& last
    , const CDictionaryBasedPath& revisionRootPath)
{
    // any chance that this revision affects our search path?

    if (!revisionRootPath.IsValid())
        return false;

    if (!revisionRootPath.IsSameOrParentOf (path.GetBasePath()))
        return false;

    // close examination of all changes

    CRevisionInfoContainer::CChangesIterator bestRename = last;
    for ( CRevisionInfoContainer::CChangesIterator iter = first
        ; iter != last
        ; ++iter)
    {
        // most entries will just be file content changes
        // -> skip them efficiently

        CRevisionInfoContainer::TChangeAction action = iter.GetAction();
        if (action == CRevisionInfoContainer::ACTION_CHANGED)
            continue;

        // deletion / copy / rename / replacement
        // -> skip, if our search path is not affected (only some sub-path)

        CDictionaryBasedPath changedPath = iter->GetPath();
        if (!changedPath.IsSameOrParentOf (path.GetBasePath()))
            continue;

        // now, this is serious

        switch (action)
        {
            // rename?

            case CRevisionInfoContainer::ACTION_ADDED:
            case CRevisionInfoContainer::ACTION_REPLACED:
            {
                if (iter.HasFromPath())
                {
                    // continue search on copy source path

                    // The last copy found will also be the one closed
                    // to our searchPath (there may be multiple renames,
                    // if the base path got renamed).

                    assert (   (bestRename == last)
                            || (bestRename.GetPathID() < iter.GetPathID())
                            || "parent ADDs are not in strict order");

                    bestRename = iter;
                }
                else
                {
                    // as part of a copy / rename, the parent path
                    // may have been added in just the same revision.
                    //
                    // example:
                    //
                    // our path: /trunk/file
                    // renamed to
                    // /trunk/project/file
                    //
                    // this can only happen if
                    // /trunk/project
                    // is added first (usually without a copy from path)
                    //
                    // Stop iteration only if we found an ADD of
                    // the exact search path.

                    if (path == changedPath)
                    {
                        // the path we are following actually started here.

                        addRevision = revision;
                        addPath = path;

                        revision = (revision_t)NO_REVISION;
                        path.Invalidate();
                        return true;
                    }
                }
            }
            break;

            case CRevisionInfoContainer::ACTION_DELETED:
            {
                // deletions are possible!
                // but we don't need to do anything with them.
            }
            break;

            // there should be no other

            default:
            {
                assert (0);
            }
        }
    }

    // there was a rename / copy from some older path,rev

    if (bestRename != last)
    {
        addRevision = revision;
        addPath = path;

        path = path.ReplaceParent ( bestRename.GetPath()
                                  , bestRename.GetFromPath());
        revision = bestRename.GetFromRevision();

        return true;
    }

    // all fine, no special action required

    return false;
}

// log scanning sub-routines

void CLogIteratorBase::ToNextRevision()
{
    --revision;
}

revision_t CLogIteratorBase::SkipNARevisions()
{
    return skipRevisionInfo.GetPreviousRevision (path.GetBasePath(), revision);
}

// log scanning

void CLogIteratorBase::AdvanceOneStep()
{
    // perform at least one step

    ToNextRevision();

    // skip ranges of missing data, if we know
    // that they don't affect our path

    while (InternalDataIsMissing())
    {
        revision_t nextRevision = SkipNARevisions();
        if (nextRevision != NO_REVISION)
            revision = nextRevision;
        else
            break;
    }
}

void CLogIteratorBase::InternalAdvance (revision_t last)
{
    // find next entry that mentions the path
    // stop @ revision 0 or missing log data
    // (note that revision 0 will actually be hit)

    do
    {
        AdvanceOneStep();
    }
    while ((revision > last) && !InternalDataIsMissing() && !PathInRevision());
}

// construction
// (copy construction & assignment use default methods)

CLogIteratorBase::CLogIteratorBase ( const CCachedLogInfo* cachedLog
                                   , revision_t startRevision
                                   , const CDictionaryBasedTempPath& startPath)
    : revisionInfo (cachedLog->GetLogInfo())
    , revisionIndices (cachedLog->GetRevisions())
    , skipRevisionInfo (cachedLog->GetSkippedRevisions())
    , revision (startRevision)
    , addRevision ((revision_t)NO_REVISION)
    , path (startPath)
    , addPath (startPath.GetDictionary())
{
}

CLogIteratorBase::~CLogIteratorBase(void)
{
}

// provide custom assignment operator to silence C4512

CLogIteratorBase& CLogIteratorBase::operator=(const CLogIteratorBase& rhs)
{
    assert (&revisionInfo == &rhs.revisionInfo);

    if (this != &rhs)
    {
        revision = rhs.revision;
        path = rhs.path;
        addRevision = rhs.addRevision;
    }

    return *this;
}

// implement ILogIterator

bool CLogIteratorBase::DataIsMissing() const
{
    return InternalDataIsMissing();
}

void CLogIteratorBase::Advance (revision_t last)
{
    // maybe, there was some cache update

    HandleCacheUpdates();

    // end of history?

    if (revision >= last)
    {
        // the current revision may be a copy / rename
        // -> update our path before we proceed, if necessary

        if (HandleCopyAndDelete())
        {
            // revision may have been set to NO_REVISION,
            // e.g. if a deletion has been found

            if (revision != NO_REVISION)
            {
                // switched to a new path
                // -> retry access on that new path
                // (especially, we must try (copy-from-) revision)

                Retry();
            }
        }
        else
        {
            // find next entry that mentions the path
            // stop @ revision 0 or missing log data

            InternalAdvance (last);
        }
    }
}

void CLogIteratorBase::ToNextAvailableData()
{
    // find next available entry
    // stop @ revision -1

    do
    {
        AdvanceOneStep();
    }
    while ((revision != NO_INDEX) && InternalDataIsMissing());
}

void CLogIteratorBase::Retry (revision_t last)
{
    // maybe, there was some cache update

    HandleCacheUpdates();

    // don't handle copy / rename more than once

    if (revision)
        ++revision;
    InternalAdvance (last);
}

void CLogIteratorBase::SetPath (const CDictionaryBasedTempPath& path)
{
    this->path = path;
}

// end namespace LogCache

}

