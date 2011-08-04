// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007,2009-2011 - TortoiseSVN

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

    int shownRows = static_cast<int>(m_logdlg->m_logEntries.GetVisibleCount());

    POSITION pos = m_logdlg->m_LogList.GetFirstSelectedItemPosition();
    if (pos)
    {
        int nIndex = m_logdlg->m_LogList.GetNextSelectedItem(pos);
        if ( nIndex!=-1 && nIndex < shownRows )
        {
            PLOGENTRYDATA pLogEntry = m_logdlg->m_logEntries.GetVisible (nIndex);
            m_SetSelectedRevisions.auto_insert (pLogEntry->GetRevision());
            while (pos)
            {
                nIndex = m_logdlg->m_LogList.GetNextSelectedItem(pos);
                if ( nIndex!=-1 && nIndex < shownRows )
                {
                    pLogEntry = m_logdlg->m_logEntries.GetVisible (nIndex);
                    m_SetSelectedRevisions.auto_insert(pLogEntry->GetRevision());
                }
            }
        }
    }
}

CStoreSelection::CStoreSelection( CLogDlg* dlg, const SVNRevRangeArray& revRange )
{
    m_logdlg = dlg;

    for (int i = 0; i < revRange.GetCount(); ++i)
    {
        const SVNRevRange range = revRange[i];
        for (svn_revnum_t rev = range.GetStartRevision(); rev <= range.GetEndRevision(); ++rev)
        {
            m_SetSelectedRevisions.auto_insert(rev);
        }
    }
}

CStoreSelection::~CStoreSelection()
{
    if ( m_SetSelectedRevisions.size()>0 )
    {
        // inhibit UI event processing (and combined path list updates)

        InterlockedExchange (&m_logdlg->m_bLogThreadRunning, TRUE);

        int firstSelected = INT_MAX;
        int lastSelected = INT_MIN;

        for (int i=0, count = (int)m_logdlg->m_logEntries.GetVisibleCount(); i < count; ++i)
        {
            LONG nRevision = m_logdlg->m_logEntries.GetVisible(i)->GetRevision();
            if ( m_SetSelectedRevisions.contains (nRevision) )
            {
                m_logdlg->m_LogList.SetSelectionMark(i);
                m_logdlg->m_LogList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);

                // record range of selected items.
                // We must not call EnsureVisible here because it will scroll
                // for a very long time if many items have been selected.

                firstSelected = min (firstSelected, i);
                lastSelected = max (lastSelected, i);
            }
        }

        // ensure that the selected items are visible and
        // prefer the first one to be visible

        if (lastSelected != INT_MIN)
        {
            m_logdlg->m_LogList.EnsureVisible (lastSelected, FALSE);
            m_logdlg->m_LogList.EnsureVisible (firstSelected, FALSE);
        }

        // UI updates are allowed, again

        InterlockedExchange (&m_logdlg->m_bLogThreadRunning, FALSE);

        // manually trigger UI processing that had been blocked before

        m_logdlg->FillLogMessageCtrl (false);
        m_logdlg->UpdateLogInfoLabel();
        m_logdlg->m_LogList.Invalidate();
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

    __time64_t date = data.GetTimeStamp (index);
    const char * author = data.GetAuthor (index);
    std::string message = data.GetComment (index);

    // construct result

    std::auto_ptr<LOGENTRYDATA> result
        (new CLogEntryData
            ( NULL
            , revision
            , date / 1000000L
            , author != NULL ? author : ""
            , message
            , projectProperties
            , NULL
            )
        );

    // done here

    return result.release();
}
