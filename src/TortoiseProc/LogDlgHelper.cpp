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
    , LogChangedPathArray* changedPaths
    , CString& selfRelativeURL)
    : parent (parent)
    , hasChildren (false)
    , childStackDepth (parent == NULL ? 0 : parent->childStackDepth+1)
    , Rev (Rev)
    , tmDate (tmDate)
    , sDate (date)
    , sAuthor (author)
    , changedPaths (changedPaths == NULL 
                        ? &LogChangedPathArray::GetEmptyInstance() 
                        : changedPaths)
	, copiedSelf (false)
    , checked (false)
{
    // derived header info

    SetMessage (message, projectProperties);

    // update nesting info

    if (parent)
        parent->hasChildren = true;

    // derived change path info and update current URL

	for (size_t i = 0, count = changedPaths->GetCount(); i < count; ++i)
	{
		const LogChangedPath& cpath = (*changedPaths)[i];
        if (   !cpath.GetCopyFromPath().IsEmpty() 
            && (cpath.GetPath().Compare (selfRelativeURL) == 0))
		{
			// note: this only works if the log is fetched top-to-bottom
			// but since we do that, it shouldn't be a problem

            selfRelativeURL = cpath.GetCopyFromPath();
			copiedSelf = true;
		}
	}
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
