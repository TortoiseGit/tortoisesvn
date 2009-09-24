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
#include "LogDlgDataModel.h"
#include "LogDlg.h"
#include "LogDlgFilter.h"
#include "ProfilingInfo.h"
#include "Future.h"

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
	if (size() > 0)
	{
		for(iterator it=begin(); it!=end(); ++it)
			delete *it;

        clear();
        visible.clear();
        logParents.clear();

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
                         , const CString& date
                         , const CString& author
                         , const CString& message
                         , ProjectProperties* projectProperties
                         , LogChangedPathArray* changedPaths
                         , CString& selfRelativeURL
                         , bool childrenFollow)
{
    // end of child list?

	if (revision == SVN_INVALID_REVNUM)
    {
        logParents.pop_back();
        return;
    }

    // add ordinary entry

    LogEntryData* item
        = new LogEntryData
            ( logParents.empty() ? NULL : logParents.back()
            , revision
            , tmDate
            , date
            , author
            , message
            , projectProperties
            , new LogChangedPathArray (*changedPaths)
            , selfRelativeURL
            );

    changedPaths->RemoveAll();

    visible.push_back (size());
    push_back (item);

    // update min / max values

    maxDepth = max (maxDepth, item->GetDepth());

    maxDate = max (maxDate, tmDate);
    minDate = min (minDate, tmDate);

    minRevision = min (minRevision, revision);
    minRevision = max (minRevision, revision);

    // update parent info

    if (childrenFollow)
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

    CString dummy;

    LogChangedPathArray temp (item->GetChangedPaths());
    Add ( item->GetRevision()
        , item->GetDate()
        , item->GetDateString()
        , item->GetAuthor()
        , item->GetMessage()
        , projectProperties
        , &temp
        , dummy
        , false);
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

PLOGENTRYDATA CLogDataVector::GetVisible (size_t index) const 
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

std::vector<size_t> 
CLogDataVector::FilterRange 
    ( const CLogDlgFilter* filter
    , size_t first
    , size_t last)
{
    std::vector<size_t> result;
    result.reserve (last - first);

	wstring scratch;
    for (size_t i = first; i < last; ++i)
        if (filter->Matches (*inherited::operator[](i), scratch))
            result.push_back (i);

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

void CLogDataVector::Filter (__time64_t from, __time64_t to)
{
    visible.clear();
	for (size_t i=0, count = size(); i < count; ++i)
	{
        const PLOGENTRYDATA entry = inherited::operator[](i);
        __time64_t date = entry->GetDate();

    	if ((date >= from) && (date <= to))
            visible.push_back (i);
	}
}

void CLogDataVector::ClearFilter()
{
    visible.clear();
    visible.reserve (size());

	for (size_t i=0, count = size(); i < count; ++i)
        visible.push_back (i);
}

