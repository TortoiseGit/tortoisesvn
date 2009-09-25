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
        int shownRows = static_cast<int>(m_logdlg->m_logEntries.GetVisibleCount());

		POSITION pos = m_logdlg->m_LogList.GetFirstSelectedItemPosition();
		int nIndex = m_logdlg->m_LogList.GetNextSelectedItem(pos);
		if ( nIndex!=-1 && nIndex < shownRows )
		{
            PLOGENTRYDATA pLogEntry = m_logdlg->m_logEntries.GetVisible (nIndex);
			m_SetSelectedRevisions.insert(pLogEntry->GetRevision());
			while (pos)
			{
				nIndex = m_logdlg->m_LogList.GetNextSelectedItem(pos);
				if ( nIndex!=-1 && nIndex < shownRows )
				{
                    pLogEntry = m_logdlg->m_logEntries.GetVisible (nIndex);
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
		for (int i=0, count = (int)m_logdlg->m_logEntries.GetVisibleCount(); i < count; ++i)
		{
			LONG nRevision = m_logdlg->m_logEntries.GetVisible(i)->GetRevision();
			if ( m_SetSelectedRevisions.find(nRevision)!=m_SetSelectedRevisions.end() )
			{
				m_logdlg->m_LogList.SetSelectionMark(i);
				m_logdlg->m_LogList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				m_logdlg->m_LogList.EnsureVisible(i, FALSE);
			}
		}
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
    CString message (data.GetComment (index).c_str());

    // construct result

    std::auto_ptr<LOGENTRYDATA> result 
        (new CLogEntryData
            ( NULL
            , revision
            , date / 1000000L
            , CString (author != NULL ? author : "")
            , message
            , projectProperties
            )
        );

    // done here

    return result.release();
}
