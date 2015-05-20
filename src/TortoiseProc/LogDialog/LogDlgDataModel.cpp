// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2015 - TortoiseSVN

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
#include "LogDlgDataModel.h"
#include "LogDlg.h"
#include "LogDlgFilter.h"
#include "ProfilingInfo.h"
#include "Future.h"
#include "StringUtils.h"
#include "CachedLogInfo.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"

///////////////////////////////////////////////////////////////
// data structures to accommodate the change list
// (taken from the SVN class)
///////////////////////////////////////////////////////////////

// conversion utility

CString CLogChangedPath::GetUIPath (const CDictionaryBasedPath& p) const
{
    std::string utf8Path = p.GetPath();

    // we don't need to adjust the path length as
    // the conversion will automatically stop at \0.

    return CUnicodeUtils::UTF8ToUTF16 (utf8Path);
}

// construction

CLogChangedPath::CLogChangedPath
    ( const CRevisionInfoContainer::CChangesIterator& iter
    , const CDictionaryBasedTempPath& logPath)
    : path (iter.GetPath())
    , copyFromPath (NULL, (index_t)NO_INDEX)
    , copyFromRev (0)
{
    flags.nodeKind = static_cast<svn_node_kind_t>(iter->GetPathType());
    flags.action = (DWORD)iter.GetAction() / 4;
    flags.textModifies = static_cast<svn_tristate_t>(iter->GetTextModifies());
    flags.propsModifies = static_cast<svn_tristate_t>(iter->GetPropsModifies());

    // check relevance for log path

    flags.relevantForStartPath = logPath.IsSameOrParentOf (path)
                              || logPath.IsSameOrChildOf (path);

    // set copy-from info, if available

    if (iter.HasFromPath() && (iter.GetFromRevision() != NO_REVISION))
    {
        copyFromPath = iter.GetFromPath();
        copyFromRev = iter.GetFromRevision();
    }
}

// r/o data access

CString CLogChangedPath::GetPath() const
{
    return GetUIPath (path);
}

CString CLogChangedPath::GetCopyFromPath() const
{
    return copyFromPath.IsValid()
        ? GetUIPath (copyFromPath)
        : CString();
}

// convenience method

namespace
{
    void LoadString (std::string& s, UINT id)
    {
        CString temp;
        temp.LoadString (id);
        s = CUnicodeUtils::StdGetUTF8 ((LPCTSTR)temp);
    }
}

const std::string& CLogChangedPath::GetActionString (DWORD action)
{
    static std::string addActionString;
    static std::string deleteActionString;
    static std::string replacedActionString;
    static std::string modifiedActionString;
    static std::string movedActionString;
    static std::string movereplacedActionString;
    static std::string empty;

    switch (action)
    {
    case LOGACTIONS_MODIFIED:
        if (modifiedActionString.empty())
            LoadString (modifiedActionString, IDS_SVNACTION_MODIFIED);

        return modifiedActionString;

    case LOGACTIONS_ADDED:
        if (addActionString.empty())
            LoadString (addActionString, IDS_SVNACTION_ADD);

        return addActionString;

    case LOGACTIONS_DELETED:
        if (deleteActionString.empty())
            LoadString (deleteActionString, IDS_SVNACTION_DELETE);

        return deleteActionString;

    case LOGACTIONS_REPLACED:
        if (replacedActionString.empty())
            LoadString (replacedActionString, IDS_SVNACTION_REPLACED);

        return replacedActionString;

    case LOGACTIONS_MOVED:
        if (movedActionString.empty())
            LoadString(movedActionString, IDS_SVNACTION_MOVED);

        return movedActionString;

    case LOGACTIONS_MOVEREPLACED:
        if (movereplacedActionString.empty())
            LoadString(movereplacedActionString, IDS_SVNACTION_MOVEREPLACED);

        return movereplacedActionString;

    default:
        // there should always be an action
        assert (0);

    }

    return empty;
}

const std::string& CLogChangedPath::GetActionString() const
{
    return GetActionString (flags.action);
}

// construction

CLogChangedPathArray::CLogChangedPathArray()
    : actions (0)
    , copiedSelf (false)
{
}

// modification

void CLogChangedPathArray::Add
    ( CRevisionInfoContainer::CChangesIterator& first
    , const CRevisionInfoContainer::CChangesIterator& last
    , CDictionaryBasedTempPath& logPath)
{
    reserve (last - first);
    for (; first != last; ++first)
        push_back (CLogChangedPath (first, logPath));

    // update log path to the *last* copy source we found
    // because it will also be the closed one (parent may
    // have copied from somewhere else)

    for (size_t i = size(); i > 0; --i)
    {
        CLogChangedPath& change = at(i-1);

        // relevant for this path and
        // has the log path be renamed?

        if (   change.IsRelevantForStartPath()
            && (change.GetCopyFromRev() > 0)
            && logPath.IsSameOrChildOf (change.GetCachedPath()))
        {
            // note: this only works if the log is fetched top-to-bottom
            // but since we do that, it shouldn't be a problem

            logPath = logPath.ReplaceParent
                         ( change.GetCachedPath()
                         , change.GetCachedCopyFromPath());
            copiedSelf = true;

            return;
        }
    }

    actions = 0;
}

void CLogChangedPathArray::Add (const CLogChangedPath& item)
{
    push_back (item);
    actions = 0;
}

void CLogChangedPathArray::RemoveAll()
{
    clear();
    actions = 0;
}

void CLogChangedPathArray::RemoveIrrelevantPaths()
{
    struct IsRelevant
    {
        bool operator()(const CLogChangedPath& path) const
        {
            return !path.IsRelevantForStartPath();
        }
    };

    iterator first = begin();
    iterator last = end();
    erase (std::remove_copy_if (first, last, first, IsRelevant()), last);

    actions = 0;
}

void CLogChangedPathArray::Sort (int column, bool ascending)
{
    struct Order
    {
    private:

        int column;
        bool ascending;

    public:

        Order (int column, bool ascending)
            : column (column)
            , ascending (ascending)
        {
        }

        bool operator()(const CLogChangedPath& lhs, const CLogChangedPath& rhs) const
        {
            const CLogChangedPath* cpath1 = ascending ? &lhs : &rhs;
            const CLogChangedPath* cpath2 = ascending ? &rhs : &lhs;

            int cmp = 0;
            switch (column)
            {
            case 0: // path
                    cmp = cpath2->GetPath().CompareNoCase (cpath1->GetPath());
                    if (cmp)
                        return cmp > 0;
                    // fall through
            case 1: // action
                    cmp = strcmp ( cpath2->GetActionString().c_str()
                                 , cpath1->GetActionString().c_str());
                    if (cmp)
                        return cmp > 0;
                    // fall through
            case 2: // copy from path
                    cmp = cpath2->GetCopyFromPath().CompareNoCase (cpath1->GetCopyFromPath());
                    if (cmp)
                        return cmp > 0;
                    // fall through
            case 3: // copy from revision
                    return cpath2->GetCopyFromRev() > cpath1->GetCopyFromRev();
            }

            return false;
        }
    };

    std::sort (begin(), end(), Order (column, ascending));
}

// derived information

DWORD CLogChangedPathArray::GetActions() const
{
    if (actions == 0)
        for (size_t i = 0, count = size(); i < count; ++i)
            actions |= (*this)[i].GetAction();

    return actions;
}

bool CLogChangedPathArray::ContainsCopies() const
{
    for (size_t i = 0, count = size(); i < count; ++i)
        if ((*this)[i].GetCopyFromRev() != 0)
            return true;

    return false;
}

CLogEntryData::CLogEntryData
    ( CLogEntryData* parent
    , svn_revnum_t revision
    , __time64_t tmDate
    , const std::string& author
    , const std::string& message
    , ProjectProperties* projectProperties
    , const MergeInfo* mergeInfo)
    : parent (parent)
    , hasParent(false)
    , hasChildren (false)   // we don't read that from the "mergesFollow" flag
                            // (just to be sure that we actually have sub-nodes)
    , nonInheritable (mergeInfo && mergeInfo->nonInheritable)
    , subtractiveMerge (mergeInfo && mergeInfo->subtractiveMerge)
    , childStackDepth (parent == NULL ? 0 : parent->childStackDepth+1)
    , revision (revision)
    , tmDate (tmDate)
    , sAuthor (author)
    , projectProperties (projectProperties)
    , checked (false)
    , bugIDsPending (true)
    , unread(false)
{
    // derived header info

    SetMessage (message);

    // update nesting info

    if (parent)
    {
        hasParent = true;
        parent->hasChildren = true;
    }
}

CLogEntryData::~CLogEntryData()
{
}

void CLogEntryData::SetAuthor
    ( const std::string& author)
{
    sAuthor = author;
}

namespace
{
    bool IsNotCR (char c)
    {
        return c != '\r';
    }
}

void CLogEntryData::SetMessage (const std::string& message)
{
    // split multi line log entries and concatenate them
    // again but this time with \r\n as line separators
    // so that the edit control recognizes them

    sMessage = message;
    if (!sMessage.empty())
    {
        if (sMessage.find ('\r') != std::string::npos)
        {
            sMessage.erase ( std::copy_if ( sMessage.begin()
                                          , sMessage.end()
                                          , sMessage.begin()
                                          , IsNotCR)
                           , sMessage.end());
        }
        if (!sMessage.empty() && sMessage[0] == '\n')
            sMessage.erase (0, 1);
    }

    // derived data

    bugIDsPending = true;
}

void CLogEntryData::SetChecked
    ( bool newState)
{
    checked = newState;
}

/// finalization (call this once the cache is available)

void CLogEntryData::Finalize
    ( const CCachedLogInfo* cache
    , CDictionaryBasedTempPath& logPath)
{
    // don't finalize twice

    if (changedPaths.GetCount() == 0)
    {
        index_t index = cache->GetRevisions()[revision];
        const CRevisionInfoContainer& info = cache->GetLogInfo();

        CRevisionInfoContainer::CChangesIterator first = info.GetChangesBegin (index);
        CRevisionInfoContainer::CChangesIterator last = info.GetChangesEnd (index);

        changedPaths.Add (first, last, logPath);
    }
}

// r/o access to the data

void CLogEntryData::InitDateStrings() const
{
    if (tmDate == 0)
    {
        sDate = "(no date)";
    }
    else
    {
        TCHAR date_native[SVN_DATE_BUFFER] = {0};
        FILETIME ft = {0};
        AprTimeToFileTime (&ft, tmDate * 1000000L);
        SVN::formatDate (date_native, ft);

        sDate = CUnicodeUtils::StdGetUTF8 (date_native);
    }
}

void CLogEntryData::InitBugIDs() const
{
    if (projectProperties && projectProperties->MightContainABugID ())
    {
        CString unicodeMessage = CUnicodeUtils::GetUnicode (sMessage.c_str());
        CString unicodeBugIDs = projectProperties->FindBugID (unicodeMessage);
        sBugIDs = (const char*)CUnicodeUtils::GetUTF8 (unicodeBugIDs);
    }
    else
    {
        sBugIDs.clear();
    }

    bugIDsPending = false;
}

const std::string& CLogEntryData::GetDateString() const
{
    if (sDate.empty())
        InitDateStrings();

    return sDate;
}

const std::string& CLogEntryData::GetBugIDs() const
{
    if (bugIDsPending)
        InitBugIDs();

    return sBugIDs;
}

CString CLogEntryData::GetShortMessageUTF16() const
{
    return projectProperties
        ? projectProperties->MakeShortMessage
            (CUnicodeUtils::GetUnicode (sMessage.c_str()))
        : CString();
}

// construction

CLogDataVector::CLogDataVector()
    : maxDepth (0)
    , maxDate (0)
    , minDate (LLONG_MAX)
    , minRevision (INT_MAX)
    , maxRevision (-1)
{
}

// De-allocates log items.

void CLogDataVector::ClearAll()
{
    if (!empty())
    {
        for(iterator it=begin(); it!=end(); ++it)
            delete *it;

        clear();
        visible.clear();
        logParents.clear();

        query.reset();

        maxDepth = 0;
        maxDate = 0;
        minDate = LLONG_MAX;
        maxRevision = -1;
        minRevision = INT_MAX;
    }
}

// add items

void CLogDataVector::Add ( svn_revnum_t revision
                         , __time64_t tmDate
                         , const std::string& author
                         , const std::string& message
                         , ProjectProperties* projectProperties
                         , const MergeInfo* mergeInfo)
{
    // end of child list?

    if (revision == SVN_INVALID_REVNUM)
    {
        logParents.pop_back();
        return;
    }

    // add ordinary entry

    CLogEntryData* item
        = new CLogEntryData
            ( logParents.empty() ? NULL : logParents.back()
            , revision
            , tmDate
            , author
            , message
            , projectProperties
            , mergeInfo
            );

    visible.push_back (size());
    push_back (item);

    // update min / max values

    maxDepth = max (maxDepth, item->GetDepth());

    maxDate = max (maxDate, tmDate);
    minDate = min (minDate, tmDate);

    minRevision = min (minRevision, revision);
    maxRevision = max (maxRevision, revision);

    // update parent info

    if (mergeInfo && mergeInfo->mergesFollow)
        logParents.push_back (item);
}

void CLogDataVector::AddSorted
    ( PLOGENTRYDATA item
    , ProjectProperties* projectProperties)
{
    svn_revnum_t revision = item->GetRevision();
    for (iterator iter = begin(), last = end(); iter != last; ++iter)
    {
        int diff = revision - (*iter)->GetRevision();
        if (diff == 0)
        {
            // avoid inserting duplicates which could happen if a filter is active
            delete item;
            return;
        }
        if (diff > 0)
        {
            insert (iter, item);
            visible.clear();

            return;
        }
    }

    // append entry

    Add ( item->GetRevision()
        , item->GetDate()
        , item->GetAuthor()
        , item->GetMessage()
        , projectProperties
        , NULL);
}

void CLogDataVector::RemoveLast()
{
    delete back();
    pop_back();
    visible.clear();
}

void CLogDataVector::Finalize
    ( std::unique_ptr<const CCacheLogQuery> aQuery
    , const CString& startLogPath, bool bMerge)
{
    if ((bMerge)&&(query.get()))
    {
        CCacheLogQuery * tempQuery = const_cast<CCacheLogQuery *>(query.get());
        aQuery->UpdateCache(tempQuery);
    }
    else
        query = std::move(aQuery);

    // if we don't have any data, the cache in query may be NULL
    // -> shortcut it here

    if (empty())
        return;

    // construct an object for the path that 'log' was called for

    CStringA utf8Path = CUnicodeUtils::GetUTF8 (startLogPath);
    CStringA relPath = CPathUtils::PathUnescape (utf8Path);

    const CCachedLogInfo* cache = query->GetCache();
    const CPathDictionary* paths = &cache->GetLogInfo().GetPaths();
    CDictionaryBasedTempPath logPath (paths, (const char*)relPath);

    // finalize all data

    for (size_t i = 0, count = size(); i < count; ++i)
        inherited::operator[](i)->Finalize (cache, logPath);
}

size_t CLogDataVector::GetVisibleCount() const
{
    return visible.size();
}

PLOGENTRYDATA CLogDataVector::GetVisible (size_t index) const
{
    if (index < visible.size())
    {
        size_t i = visible.at (index);
        if (i < size())
            return at (i);
    }
    return NULL;
}

namespace
{

/**
 * Wrapper around a stateless predicate.
 *
 * The function operator adds handling of revision nesting to the "flat"
 * comparison provided by the predicate.
 *
 * The \ref ColumnCond predicate is expected to sort in ascending order
 * (i.e. to act like operator< ).
 */

template<class ColumnCond>
class ColumnSort
{
private:

    /// swap parameters, if not set

    bool ascending;

    /// comparison after optional parameter swap:
    /// - (ascending) order according to \ref ColumnSort
    /// - put merged revisions below merge target revision

    bool InternalCompare (PLOGENTRYDATA pStart, PLOGENTRYDATA pEnd)
    {
        // are both entry sibblings on the same node level?
        // (root -> both have NULL as parent)

        if (pStart->GetParent() == pEnd->GetParent())
            return ColumnCond()(pStart, pEnd);

        // special case: one is the parent of the other
        // -> don't compare contents in that case

        if (pStart->GetParent() == pEnd)
            return !ascending;
        if (pStart == pEnd->GetParent())
            return ascending;

        // find the closed pair of parents that is related

        assert ((pStart->GetDepth() == 0) || (pStart->GetParent() != NULL));
        assert ((pEnd->GetDepth() == 0) || (pEnd->GetParent() != NULL));

        if (pStart->GetDepth() == pEnd->GetDepth())
            return InternalCompare (pStart->GetParent(), pEnd->GetParent());

        if (pStart->GetDepth() < pEnd->GetDepth())
            return InternalCompare (pStart, pEnd->GetParent());
        else
            return InternalCompare (pStart->GetParent(), pEnd);
    }

public:

    /// one class for both sort directions

    ColumnSort(bool ascending)
        : ascending (ascending)
    {
    }

    /// asjust parameter order according to sort order

    bool operator() (PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
    {
        return ascending
            ? InternalCompare (pStart, pEnd)
            : InternalCompare (pEnd, pStart);
    }
};

}

void CLogDataVector::Sort (CLogDataVector::SortColumn column, bool ascending)
{
    /// Ascending date sorting.
    struct DateSort
    {
        bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
        {
            return pStart->GetDate() < pEnd->GetDate();
        }
    };

    /// Ascending revision sorting.
    struct RevSort
    {
        bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
        {
            return pStart->GetRevision() < pEnd->GetRevision();
        }
    };

    /// Ascending author sorting.
    struct AuthorSort
    {
        bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
        {
            int ret = _stricmp ( pStart->GetAuthor().c_str()
                               , pEnd->GetAuthor().c_str());
            if (ret == 0)
                return pStart->GetRevision() < pEnd->GetRevision();
            return ret<0;
        }
    };

    /// Ascending bugID sorting.
    struct BugIDSort
    {
        bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
        {
            int ret = _stricmp ( pStart->GetBugIDs().c_str()
                               , pEnd->GetBugIDs().c_str());
            if (ret == 0)
                return pStart->GetRevision() < pEnd->GetRevision();
            return ret<0;
        }
    };

    /// Ascending message sorting.
    struct MessageSort
    {
        bool operator()(PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
        {
            return _stricmp ( pStart->GetMessage().c_str()
                            , pEnd->GetMessage().c_str()) < 0;
        }
    };

    /// Ascending action sorting
    struct ActionSort
    {
        bool operator() (PLOGENTRYDATA& pStart, PLOGENTRYDATA& pEnd)
        {
            int diff = pStart->GetChangedPaths().GetActions()
                     - pEnd->GetChangedPaths().GetActions();

            if (diff == 0)
                return pStart->GetRevision() < pEnd->GetRevision();

            return diff < 0;
        }
    };

    switch(column)
    {
    case RevisionCol: // Revision
            std::sort (begin(), end(), ColumnSort<RevSort>(ascending));
            break;
    case ActionCol: // action
            std::sort (begin(), end(), ColumnSort<ActionSort>(ascending));
            break;
    case AuthorCol: // Author
            std::sort (begin(), end(), ColumnSort<AuthorSort>(ascending));
            break;
    case DateCol: // Date
            std::sort (begin(), end(), ColumnSort<DateSort>(ascending));
            break;
    case BugTraqCol: // Message or bug id
            std::sort (begin(), end(), ColumnSort<BugIDSort>(ascending));
            break;
    case MessageCol: // Message
            std::sort (begin(), end(), ColumnSort<MessageSort>(ascending));
            break;
    }
}

std::vector<size_t>
CLogDataVector::FilterRange
    ( const CLogDlgFilter* filter
    , size_t first
    , size_t last)
{
    std::vector<size_t> result;
    result.reserve (last - first);

    CLogDlgFilter privateFilter (*filter);

    size_t parentEntry = first;
    for (size_t i = first; i < last; ++i)
    {
        if (privateFilter(*inherited::operator[](i)))
        {
            if (inherited::operator[](i)->HasParent() && !contains(result, parentEntry))
            {
                // Filter didn't match the parent, but it matched it's child,
                // so still show the child's parent as well
                result.push_back(parentEntry);
            }

            result.push_back(i);
        }

        if (!inherited::operator[](i)->HasParent())
        {
            parentEntry = i; // remember parent's position
        }
    }

    return result;
}

void CLogDataVector::Filter (const CLogDlgFilter& filter)
{
    size_t count = size();

    visible.clear();
    visible.reserve (count);

    if (filter.BenefitsFromMT())
    {
        // run approx. 4 jobs per core / HW thread to even out
        // changed commit policies. Don't make the jobs too small, though.

        size_t itemsPerJob
            = max ( 1 + count / (4 * async::CJobScheduler::GetSharedThreadCount())
                  , 1000);

        // start jobs

        typedef async::CFuture<vector<size_t> > TFuture;
        vector<TFuture*> jobs;

        for (size_t i = 0; i < count; i += itemsPerJob)
            jobs.push_back (new TFuture ( this
                                        , &CLogDataVector::FilterRange
                                        , &filter
                                        , i
                                        , min (i + itemsPerJob, count)));

        // collect results

        for (size_t i = 0; i < jobs.size(); ++i)
        {
            const vector<size_t> result = jobs[i]->GetResult();
            visible.insert (visible.end(), result.begin(), result.end());

            delete jobs[i];
        }
    }
    else
    {
        // execute in this thread directly

        visible = FilterRange (&filter, 0, count);
    }
}

void CLogDataVector::Filter (__time64_t from, __time64_t to, bool hideNonMergeable, std::set<svn_revnum_t> * mergedrevs, svn_revnum_t minrev)
{
    visible.clear();
    for (size_t i=0, count = size(); i < count; ++i)
    {
        const PLOGENTRYDATA entry = inherited::operator[](i);
        __time64_t date = entry->GetDate();

        if ((date >= from) && (date <= to))
        {
            if (hideNonMergeable && mergedrevs && !mergedrevs->empty())
            {
                if ((mergedrevs->find(entry->GetRevision()) == mergedrevs->end()) && (entry->GetRevision() >= minrev))
                    visible.push_back (i);
            }
            else
                visible.push_back (i);
        }
    }
}

void CLogDataVector::ClearFilter(bool hideNonMergeables, std::set<svn_revnum_t> * mergedrevs, svn_revnum_t minrev)
{
    visible.clear();
    visible.reserve (size());

    for (size_t i=0, count = size(); i < count; ++i)
    {
        if (hideNonMergeables && mergedrevs && !mergedrevs->empty())
        {
            svn_revnum_t r = inherited::operator[](i)->GetRevision();
            if ((r >= minrev) && (mergedrevs->find(r) == mergedrevs->end()))
                visible.push_back (i);
        }
        else
            visible.push_back (i);
    }
}

