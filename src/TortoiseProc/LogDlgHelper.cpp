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
			m_SetSelectedRevisions.insert(pLogEntry->Rev);
			while (pos)
			{
				nIndex = m_logdlg->m_LogList.GetNextSelectedItem(pos);
				if ( nIndex!=-1 && nIndex < m_logdlg->m_arShownList.GetSize() )
				{
					pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_logdlg->m_arShownList.GetAt(nIndex));
					m_SetSelectedRevisions.insert(pLogEntry->Rev);
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
			LONG nRevision = reinterpret_cast<PLOGENTRYDATA>(m_logdlg->m_arShownList.GetAt(i))->Rev;
			if ( m_SetSelectedRevisions.find(nRevision)!=m_SetSelectedRevisions.end() )
			{
				m_logdlg->m_LogList.SetSelectionMark(i);
				m_logdlg->m_LogList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				m_logdlg->m_LogList.EnsureVisible(i, FALSE);
			}
		}
	}
}

void CLogDataVector::ClearAll()
{
	if(size() > 0)
	{
		for(iterator it=begin(); it!=end(); ++it)
		{
			delete (*it)->pArChangedPaths;
			delete *it;
		}     
		clear();
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
    std::string message = data.GetComment (index);

    std::auto_ptr<LogChangedPathArray> changes (new LogChangedPathArray);
    CCacheLogQuery::GetChanges (*changes
                               , pathToStringMap
                               , data.GetChangesBegin (index)
		                       , data.GetChangesEnd (index));

	DWORD actions = 0;
	BOOL copies = FALSE;

    for (INT_PTR i = 0, count = changes->GetCount(); i < count; ++i)
    {
	    const LogChangedPath* change = changes->GetAt (i);
	    actions |= change->action;
	    copies |= change->lCopyFromRev != 0;
    }

    // construct result

    std::auto_ptr<LOGENTRYDATA> result (new LOGENTRYDATA);

    result->Rev = revision;
    result->tmDate = date;
    result->sDate = date_native;
    result->sAuthor = author != NULL ? author : "";
    result->sMessage = message.c_str();

    if (projectProperties)
    {
	    result->sShortMessage 
            = projectProperties->MakeShortMessage (result->sMessage);
        result->sBugIDs 
            = projectProperties->FindBugID (result->sMessage);
    }

    result->dwFileChanges = (DWORD)changes->GetCount();
    result->pArChangedPaths = changes.release();
	result->bCopies = copies;
	result->bCopiedSelf = FALSE;
	result->actions = actions;
	result->haschildren = FALSE;
	result->childStackDepth = 0;
	result->bChecked = FALSE;

    // done here

    return result.release();
}
