// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2009-2016, 2021-2022 - TortoiseSVN

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
    : m_logDlg(dlg)
    , m_keepWhenAdding(false)
{
    AddSelections();
}

CStoreSelection::CStoreSelection(CLogDlg* dlg, const SVNRevRangeArray& revRange)
    : m_logDlg(dlg)
    , m_keepWhenAdding(true)
{

    for (int i = 0; i < revRange.GetCount(); ++i)
    {
        const SVNRevRange range = revRange[i];
        for (svn_revnum_t rev = range.GetStartRevision(); rev <= range.GetEndRevision(); ++rev)
        {
            m_setSelectedRevisions.insert(rev);
        }
    }
}

CStoreSelection::~CStoreSelection()
{
    RestoreSelection();
}

void CStoreSelection::ClearSelection()
{
    m_setSelectedRevisions.clear();
}

void CStoreSelection::AddSelections()
{
    int shownRows = static_cast<int>(m_logDlg->m_logEntries.GetVisibleCount());

    if (!m_keepWhenAdding)
    {
        for (int i = 0, count = static_cast<int>(m_logDlg->m_logEntries.GetVisibleCount()); i < count; ++i)
        {
            auto* pLogEntry = m_logDlg->m_logEntries.GetVisible(i);
            m_setSelectedRevisions.erase(pLogEntry->GetRevision());
        }
    }

    POSITION pos = m_logDlg->m_logList.GetFirstSelectedItemPosition();
    if (pos)
    {
        int nIndex = m_logDlg->m_logList.GetNextSelectedItem(pos);
        if (nIndex != -1 && nIndex < shownRows)
        {
            PLOGENTRYDATA pLogEntry = m_logDlg->m_logEntries.GetVisible(nIndex);
            m_setSelectedRevisions.insert(pLogEntry->GetRevision());
            while (pos)
            {
                nIndex = m_logDlg->m_logList.GetNextSelectedItem(pos);
                if (nIndex != -1 && nIndex < shownRows)
                {
                    pLogEntry = m_logDlg->m_logEntries.GetVisible(nIndex);
                    m_setSelectedRevisions.insert(pLogEntry->GetRevision());
                }
            }
        }
    }
}

void CStoreSelection::RestoreSelection()
{
    if (m_setSelectedRevisions.size() > 0)
    {
        // inhibit UI event processing (and combined path list updates)

        InterlockedExchange(&m_logDlg->m_bLogThreadRunning, TRUE);

        int firstSelected = INT_MAX;
        int lastSelected  = INT_MIN;

        for (int i = 0, count = static_cast<int>(m_logDlg->m_logEntries.GetVisibleCount()); i < count; ++i)
        {
            auto pLogEntry = m_logDlg->m_logEntries.GetVisible(i);
            LONG nRevision = pLogEntry ? pLogEntry->GetRevision() : 0;
            if (m_setSelectedRevisions.find(nRevision) != m_setSelectedRevisions.end())
            {
                m_logDlg->m_logList.SetSelectionMark(i);
                m_logDlg->m_logList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);

                // record range of selected items.
                // We must not call EnsureVisible here because it will scroll
                // for a very long time if many items have been selected.

                firstSelected = min(firstSelected, i);
                lastSelected  = max(lastSelected, i);
            }
        }

        // ensure that the selected items are visible and
        // prefer the first one to be visible

        if (lastSelected != INT_MIN)
        {
            m_logDlg->m_logList.EnsureVisible(lastSelected, FALSE);
            m_logDlg->m_logList.EnsureVisible(firstSelected, FALSE);
        }

        // UI updates are allowed, again

        InterlockedExchange(&m_logDlg->m_bLogThreadRunning, FALSE);

        // manually trigger UI processing that had been blocked before

        m_logDlg->FillLogMessageCtrl(false);
        m_logDlg->UpdateLogInfoLabel();
        m_logDlg->m_logList.Invalidate();
    }
}

CLogCacheUtility::CLogCacheUtility(LogCache::CCachedLogInfo* cache, ProjectProperties* projectProperties)
    : cache(cache)
    , projectProperties(projectProperties){};

bool CLogCacheUtility::IsCached(svn_revnum_t revision) const
{
    const LogCache::CRevisionIndex&         revisions = cache->GetRevisions();
    const LogCache::CRevisionInfoContainer& data      = cache->GetLogInfo();

    // revision in cache at all?

    LogCache::index_t index = revisions[revision];
    if (index == LogCache::NO_INDEX)
        return false;

    // std-revprops and changed paths available?

    enum
    {
        Mask = LogCache::CRevisionInfoContainer::HAS_STANDARD_INFO
    };

    return (data.GetPresenceFlags(index) & Mask) == Mask;
}

std::unique_ptr<LOGENTRYDATA> CLogCacheUtility::GetRevisionData(svn_revnum_t revision) const
{
    // don't try to return what we don't have

    if (!IsCached(revision))
        return nullptr;

    // prepare data access

    const LogCache::CRevisionIndex&         revisions = cache->GetRevisions();
    const LogCache::CRevisionInfoContainer& data      = cache->GetLogInfo();
    LogCache::index_t                       index     = revisions[revision];

    // query data

    __time64_t  date    = data.GetTimeStamp(index);
    const char* author  = data.GetAuthor(index);
    std::string message = data.GetComment(index);

    // construct result

    auto result = std::make_unique<CLogEntryData>(nullptr,
                                                  revision,
                                                  date / 1000000L,
                                                  author != nullptr ? author : "",
                                                  message,
                                                  projectProperties,
                                                  nullptr);

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

CMonitorTreeTarget::CMonitorTreeTarget(CLogDlg* pLogDlg)
    : CIDropTarget(pLogDlg->m_projTree.GetSafeHwnd())
    , m_pLogDlg(pLogDlg)
    , m_ullHoverStartTicks(0)
    , hLastItem(nullptr)
{
    sNoDrop.LoadString(IDS_DROPDESC_NODROP);
    sImportDrop.LoadString(IDS_DROPDESC_ADD);
}

void CMonitorTreeTarget::HandleDropFormats(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD* /*pdwEffect*/, POINTL /*pt*/, const CString& targetUrl) const
{
    if (((pFmtEtc->cfFormat == CF_UNICODETEXT) || (pFmtEtc->cfFormat == CF_INETURL) || (pFmtEtc->cfFormat == CF_SHELLURL)) && medium.tymed == TYMED_HGLOBAL)
    {
        wchar_t* pStr = static_cast<wchar_t*>(GlobalLock(medium.hGlobal));
        CString  urls;
        if (pStr != nullptr)
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
        wchar_t* pStr = static_cast<wchar_t*>(GlobalLock(medium.hGlobal));
        CString  urls;
        if (pStr != nullptr)
        {
            urls = pStr;
        }
        GlobalUnlock(medium.hGlobal);
        urls.Replace(L"\r\n", L"*");
        CTSVNPathList urlListRevs;
        urlListRevs.LoadFromAsteriskSeparatedString(urls);
        CTSVNPathList urlList;
        SVNRev        srcRev;
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
        HDROP hDrop = static_cast<HDROP>(GlobalLock(medium.hGlobal));
        if (hDrop != nullptr)
        {
            CTSVNPathList urlList;
            wchar_t       szFileName[MAX_PATH] = {0};

            UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
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

bool CMonitorTreeTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD* pdwEffect, POINTL pt)
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

HRESULT CMonitorTreeTarget::DragEnter(IDataObject __RPC_FAR* pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect)
{
    HRESULT hr           = CIDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
    m_ullHoverStartTicks = 0;
    hLastItem            = nullptr;
    SetDropDescription(DROPIMAGE_COPY, nullptr, nullptr);
    return hr;
}

HRESULT CMonitorTreeTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect)
{
    wchar_t       targetName[MAX_PATH] = {0};
    TVHITTESTINFO hit{};
    hit.pt = reinterpret_cast<POINT&>(pt);
    ScreenToClient(m_hTargetWnd, &hit.pt);
    hit.flags       = TVHT_ONITEM;
    HTREEITEM hItem = TreeView_HitTest(m_hTargetWnd, &hit);
    if (hItem != hLastItem)
        m_ullHoverStartTicks = 0;
    hLastItem = hItem;

    if (hItem != nullptr)
    {
        TVITEMEX tvItem   = {0};
        tvItem.mask       = TVIF_TEXT;
        tvItem.hItem      = hItem;
        tvItem.pszText    = targetName;
        tvItem.cchTextMax = MAX_PATH;
        TreeView_GetItem(m_hTargetWnd, &tvItem);
        if ((m_pLogDlg->m_projTree.GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED) != TVIS_EXPANDED)
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
    if (rect.PtInRect(reinterpret_cast<POINT&>(pt)))
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

HRESULT CMonitorTreeTarget::DragLeave()
{
    TreeView_SelectDropTarget(m_hTargetWnd, NULL);
    SetDropDescription(DROPIMAGE_INVALID, nullptr, nullptr);
    m_ullHoverStartTicks = 0;
    return CIDropTarget::DragLeave();
}
