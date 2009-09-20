// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007,2009 - TortoiseSVN

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
#include "LogDlgHelper.h"
#include "LogDlg.h"
#include "CachedLogInfo.h"
#include "RevisionIndex.h"
#include "CacheLogQuery.h"

CStoreSelection::CStoreSelection(CLogDlg* dlg)
{
	m_logdlg = dlg;

	int selIndex = m_logdlg->m_LogList.GetSelectionMark();
	if ( selIndex>=0 )
	{
		POSITION pos = m_logdlg->m_LogList.GetFirstSelectedItemPosition();
		int nIndex = m_logdlg->m_LogList.GetNextSelectedItem(pos);
		if ( nIndex!=-1 && nIndex < m_logdlg->m_arShownList.GetSize() )
		{
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_logdlg->m_arShownList.GetAt(nIndex));
			m_SetSelectedRevisions.insert(pLogEntry->GetRevision());
			while (pos)
			{
				nIndex = m_logdlg->m_LogList.GetNextSelectedItem(pos);
				if ( nIndex!=-1 && nIndex < m_logdlg->m_arShownList.GetSize() )
				{
					pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_logdlg->m_arShownList.GetAt(nIndex));
					m_SetSelectedRevisions.insert(pLogEntry->GetRevision());
				}
			}
		}
	}
}

CStoreSelection::~CStoreSelection()
{
	if ( m_SetSelectedRevisions.size()>0 )
	{
		for (int i=0; i<m_logdlg->m_arShownList.GetCount(); ++i)
		{
			LONG nRevision = reinterpret_cast<PLOGENTRYDATA>(m_logdlg->m_arShownList.GetAt(i))->GetRevision();
			if ( m_SetSelectedRevisions.find(nRevision)!=m_SetSelectedRevisions.end() )
			{
				m_logdlg->m_LogList.SetSelectionMark(i);
				m_logdlg->m_LogList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				m_logdlg->m_LogList.EnsureVisible(i, FALSE);
			}
		}
	}
}

LogEntryData::LogEntryData 
    ( LogEntryData* parent
    , svn_revnum_t Rev
    , __time64_t tmDate
    , const CString& date
    , const CString& author
    , const CString& message
    , ProjectProperties* projectProperties
    , LogChangedPathArray* _changedPaths
    , CString& selfRelativeURL)
    : parent (parent)
    , hasChildren (false)
    , childStackDepth (parent == NULL ? 0 : parent->childStackDepth+1)
    , Rev (Rev)
    , tmDate (tmDate)
    , sDate (date)
    , sAuthor (author)
    , changedPaths (_changedPaths == NULL 
                        ? &LogChangedPathArray::GetEmptyInstance() 
                        : _changedPaths)
    , checked (false)
{
    // derived header info

    SetMessage (message, projectProperties);

    // update nesting info

    if (parent)
        parent->hasChildren = true;

    // derived change path info and update current URL

    if (_changedPaths != NULL)
        _changedPaths->MarkRelevantChanges (selfRelativeURL);
}

LogEntryData::~LogEntryData()
{
    delete changedPaths;
}

void LogEntryData::SetAuthor 
    ( const CString& author)
{
    sAuthor = author;
}

void LogEntryData::SetMessage 
    ( const CString& message
    , ProjectProperties* projectProperties)
{
    // split multi line log entries and concatenate them
    // again but this time with \r\n as line separators
    // so that the edit control recognizes them

    sMessage = message;
    if (sMessage.GetLength()>0)
    {
        sMessage.Replace(_T("\n\r"), _T("\n"));
        sMessage.Replace(_T("\r\n"), _T("\n"));
        if (sMessage.Right(1).Compare(_T("\n"))==0)
	        sMessage = sMessage.Left (sMessage.GetLength()-1);
    } 

    // derived data

    if (projectProperties)
    {
        sShortMessage = projectProperties->MakeShortMessage (message);
        sBugIDs = projectProperties->FindBugID (message);
    }
}

void LogEntryData::SetChecked
    ( bool newState)
{
    checked = newState;
}

void CLogDataVector::ClearAll()
{
	if(size() > 0)
	{
		for(iterator it=begin(); it!=end(); ++it)
			delete *it;

        clear();
	}
}

// add items

void CLogDataVector::Add (PLOGENTRYDATA item)
{
    visible.push_back (size());
    push_back (item);
}

void CLogDataVector::AddSorted (PLOGENTRYDATA item)
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

    Add (item);
}

void CLogDataVector::RemoveLast()
{
    delete back();
    pop_back();
    visible.clear();
}

size_t CLogDataVector::GetVisibleCount() const 
{
    return visible.size();
}

PLOGENTRYDATA CLogDataVector::GetVivisible (size_t index) const 
{
    return at (visible.at (index));
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

        assert ((pStart->GetChildStackDepth() == 0) || (pStart->GetParent() != NULL));
        assert ((pEnd->GetChildStackDepth() == 0) || (pEnd->GetParent() != NULL));

        if (pStart->GetChildStackDepth() == pEnd->GetChildStackDepth())
            return InternalCompare (pStart->GetParent(), pEnd->GetParent());

        if (pStart->GetChildStackDepth() < pEnd->GetChildStackDepth())
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
			int ret = pStart->GetAuthor().CompareNoCase(pEnd->GetAuthor());
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
			int ret = pStart->GetBugIDs().CompareNoCase(pEnd->GetBugIDs());
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
			return pStart->GetShortMessage().CompareNoCase(pEnd->GetShortMessage())<0;
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

CLogCacheUtility::CLogCacheUtility 
    ( LogCache::CCachedLogInfo* cache
    , ProjectProperties* projectProperties)
    : cache (cache) 
    , projectProperties (projectProperties)
{
};

bool CLogCacheUtility::IsCached (svn_revnum_t revision) const
{
    const LogCache::CRevisionIndex& revisions = cache->GetRevisions();
    const LogCache::CRevisionInfoContainer& data = cache->GetLogInfo();

    // revision in cache at all?

    LogCache::index_t index = revisions[revision];
    if (index == LogCache::NO_INDEX)
        return false;

    // std-revprops and changed paths available?

    enum 
    {
        MASK = LogCache::CRevisionInfoContainer::HAS_STANDARD_INFO
    };

    return (data.GetPresenceFlags (index) & MASK) == MASK;
}

PLOGENTRYDATA CLogCacheUtility::GetRevisionData (svn_revnum_t revision)
{
    // don't try to return what we don't have

    if (!IsCached (revision))
        return NULL;

    // prepare data access

    const LogCache::CRevisionIndex& revisions = cache->GetRevisions();
    const LogCache::CRevisionInfoContainer& data = cache->GetLogInfo();
    LogCache::index_t index = revisions[revision];

    // query data

    TCHAR date_native[SVN_DATE_BUFFER] = {0};
    __time64_t date = data.GetTimeStamp (index);
    SVN().formatDate (date_native, date);
    const char * author = data.GetAuthor (index);
    CString message (data.GetComment (index).c_str());

    std::auto_ptr<LogChangedPathArray> changes (new LogChangedPathArray);
    CCacheLogQuery::GetChanges (*changes
                               , pathToStringMap
                               , data.GetChangesBegin (index)
		                       , data.GetChangesEnd (index));

	DWORD actions = 0;
	BOOL copies = FALSE;

    for (size_t i = 0, count = changes->GetCount(); i < count; ++i)
    {
	    const LogChangedPath& change = (*changes)[i];
	    actions |= change.GetAction();
	    copies |= change.GetCopyFromRev() != 0;
    }

    // construct result

    CString dummyURL (_T("NO://URL"));

    std::auto_ptr<LOGENTRYDATA> result 
        (new LogEntryData
            ( NULL
            , revision
            , date / 1000000L
            , date_native
            , CString (author != NULL ? author : "")
            , message
            , projectProperties
            , changes.release()
            , dummyURL
            )
        );

    // done here

    return result.release();
}
