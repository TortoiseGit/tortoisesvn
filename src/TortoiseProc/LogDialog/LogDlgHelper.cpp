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
#include "LogDlgHelper.h"
#include "LogDlg.h"
#include "CachedLogInfo.h"
#include "RevisionIndex.h"
#include "CacheLogQuery.h"
#include "TortoiseProc.h"
#include "SVNDataObject.h"

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
            auto pLogEntry = m_logdlg->m_logEntries.GetVisible(i);
            LONG nRevision = pLogEntry ? pLogEntry->GetRevision() : 0;
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

void CStoreSelection::ClearSelection()
{
    m_SetSelectedRevisions.clear();
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

std::unique_ptr<LOGENTRYDATA> CLogCacheUtility::GetRevisionData(svn_revnum_t revision)
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

    std::unique_ptr<LOGENTRYDATA> result
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

    return result;
}

CLogWndHourglass::CLogWndHourglass()
{
    theApp.DoWaitCursor(1);
}

CLogWndHourglass::~CLogWndHourglass()
{
    theApp.DoWaitCursor(-1);
}

CMonitorTreeTarget::CMonitorTreeTarget(CLogDlg * pLogDlg)
    : CIDropTarget(pLogDlg->m_projTree.GetSafeHwnd())
    , m_pLogDlg(pLogDlg)
    , m_ullHoverStartTicks(0)
    , hLastItem(NULL)
{
    sNoDrop.LoadString(IDS_DROPDESC_NODROP);
    sImportDrop.LoadString(IDS_DROPDESC_ADD);
}

void CMonitorTreeTarget::HandleDropFormats(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD * /*pdwEffect*/, POINTL /*pt*/, const CString& targetUrl)
{
    if (((pFmtEtc->cfFormat == CF_UNICODETEXT) || (pFmtEtc->cfFormat == CF_INETURL) || (pFmtEtc->cfFormat == CF_SHELLURL))
        && medium.tymed == TYMED_HGLOBAL)
    {
        TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
        CString urls;
        if (pStr != NULL)
        {
            urls = pStr;
        }
        GlobalUnlock(medium.hGlobal);
        urls.Replace(L"\r\n", L"*");
        CTSVNPathList urlList;
        urlList.LoadFromAsteriskSeparatedString(urls);

        m_pLogDlg->OnDrop(urlList, targetUrl);
    }

    if (pFmtEtc->cfFormat == CF_SVNURL && medium.tymed == TYMED_HGLOBAL)
    {
        TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
        CString urls;
        if (pStr != NULL)
        {
            urls = pStr;
        }
        GlobalUnlock(medium.hGlobal);
        urls.Replace(L"\r\n", L"*");
        CTSVNPathList urlListRevs;
        urlListRevs.LoadFromAsteriskSeparatedString(urls);
        CTSVNPathList urlList;
        SVNRev srcRev;
        for (int i = 0; i < urlListRevs.GetCount(); ++i)
        {
            int pos = urlListRevs[i].GetSVNPathString().Find('?');
            if (pos > 0)
            {
                if (!srcRev.IsValid())
                    srcRev = SVNRev(urlListRevs[i].GetSVNPathString().Mid(pos + 1));
                urlList.AddPath(CTSVNPath(urlListRevs[i].GetSVNPathString().Left(pos)));
            }
            else
                urlList.AddPath(urlListRevs[i]);
        }

        m_pLogDlg->OnDrop(urlList, targetUrl);
    }

    if (pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
    {
        HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
        if (hDrop != NULL)
        {
            CTSVNPathList urlList;
            TCHAR szFileName[MAX_PATH] = { 0 };

            UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
            for (UINT i = 0; i < cFiles; ++i)
            {
                if (DragQueryFile(hDrop, i, szFileName, _countof(szFileName)))
                    urlList.AddPath(CTSVNPath(szFileName));
            }
            m_pLogDlg->OnDrop(urlList, targetUrl);
        }
        GlobalUnlock(medium.hGlobal);
    }
}

bool CMonitorTreeTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect, POINTL pt)
{
    // find the target
    CString targetUrl;

    HTREEITEM hDropTarget = m_pLogDlg->m_projTree.GetNextItem(TVI_ROOT, TVGN_DROPHILITE);
    if (hDropTarget)
    {
        targetUrl = m_pLogDlg->GetTreePath(hDropTarget);
    }
    HandleDropFormats(pFmtEtc, medium, pdwEffect, pt, targetUrl);
    TreeView_SelectDropTarget(m_hTargetWnd, NULL);
    return true;
}

HRESULT CMonitorTreeTarget::DragEnter(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
    HRESULT hr = CIDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
    m_ullHoverStartTicks = 0;
    hLastItem = NULL;
    SetDropDescription(DROPIMAGE_COPY, NULL, NULL);
    return hr;
}

HRESULT CMonitorTreeTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
    TCHAR targetName[MAX_PATH] = { 0 };
    TVHITTESTINFO hit;
    hit.pt = (POINT&)pt;
    ScreenToClient(m_hTargetWnd, &hit.pt);
    hit.flags = TVHT_ONITEM;
    HTREEITEM hItem = TreeView_HitTest(m_hTargetWnd, &hit);
    if (hItem != hLastItem)
        m_ullHoverStartTicks = 0;
    hLastItem = hItem;

    if (hItem != NULL)
    {
        TVITEMEX tvItem = { 0 };
        tvItem.mask = TVIF_TEXT;
        tvItem.hItem = hItem;
        tvItem.pszText = targetName;
        tvItem.cchTextMax = MAX_PATH;
        TreeView_GetItem(m_hTargetWnd, &tvItem);
        if ((m_pLogDlg->m_projTree.GetItemState(hItem, TVIS_EXPANDED)&TVIS_EXPANDED) != TVIS_EXPANDED)
        {
            if (m_ullHoverStartTicks == 0)
                m_ullHoverStartTicks = GetTickCount64();
            UINT timeout = 0;
            SystemParametersInfo(SPI_GETMOUSEHOVERTIME, 0, &timeout, 0);
            if ((GetTickCount64() - m_ullHoverStartTicks) > timeout)
            {
                // expand the item
                m_pLogDlg->m_projTree.Expand(hItem, TVE_EXPAND);
                m_ullHoverStartTicks = 0;
            }
        }
        else
            m_ullHoverStartTicks = 0;
        TreeView_SelectDropTarget(m_hTargetWnd, hItem);
    }
    else
    {
        TreeView_SelectDropTarget(m_hTargetWnd, NULL);
        m_ullHoverStartTicks = 0;
    }

    *pdwEffect = DROPEFFECT_COPY;
    if (targetName[0])
        SetDropDescription(DROPIMAGE_COPY, sImportDrop, targetName);
    else
        SetDropDescription(DROPIMAGE_COPY, sImportDrop, L"root");

    CRect rect;
    m_pLogDlg->m_projTree.GetWindowRect(&rect);
    if (rect.PtInRect((POINT&)pt))
    {
        if (pt.y > (rect.bottom - 20))
        {
            m_pLogDlg->m_projTree.SendMessage(WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
            m_ullHoverStartTicks = 0;
        }
        if (pt.y < (rect.top + 20))
        {
            m_pLogDlg->m_projTree.SendMessage(WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), NULL);
            m_ullHoverStartTicks = 0;
        }
    }

    return CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CMonitorTreeTarget::DragLeave(void)
{
    TreeView_SelectDropTarget(m_hTargetWnd, NULL);
    SetDropDescription(DROPIMAGE_INVALID, NULL, NULL);
    m_ullHoverStartTicks = 0;
    return CIDropTarget::DragLeave();
}

