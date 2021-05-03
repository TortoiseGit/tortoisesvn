// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010-2014, 2017, 2021 - TortoiseSVN

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
#include "RepoDrags.h"
#include "RepositoryBrowser.h"
#include "SVNDataObject.h"

CTreeDropTarget::CTreeDropTarget(CRepositoryBrowser* pRepoBrowser)
    : CBaseDropTarget(pRepoBrowser, pRepoBrowser->m_repoTree.GetSafeHwnd())
    , m_ullHoverStartTicks(0)
    , hLastItem(nullptr)
{
}

bool CTreeDropTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD* pdwEffect, POINTL pt)
{
    // find the target
    CString root;
    CString targetUrl;

    HTREEITEM hDropTarget = m_pRepoBrowser->m_repoTree.GetNextItem(TVI_ROOT, TVGN_DROPHILITE);
    if (hDropTarget)
    {
        CTreeItem* pItem = reinterpret_cast<CTreeItem*>(m_pRepoBrowser->m_repoTree.GetItemData(hDropTarget));
        if (pItem == nullptr)
            return false;
        if (pItem->m_bookmark)
            return false;

        targetUrl = pItem->m_url;
        root      = pItem->m_repository.root;
    }
    HandleDropFormats(pFmtEtc, medium, pdwEffect, pt, targetUrl, root);
    TreeView_SelectDropTarget(m_hTargetWnd, NULL);
    return true;
}

HRESULT CTreeDropTarget::DragEnter(IDataObject __RPC_FAR* pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect)
{
    HRESULT   hr    = CIDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
    FORMATETC ftEtc = {0};
    ftEtc.dwAspect  = DVASPECT_CONTENT;
    ftEtc.lindex    = -1;
    ftEtc.tymed     = TYMED_HGLOBAL;
    ftEtc.cfFormat  = CF_HDROP;
    if (pDataObj->QueryGetData(&ftEtc) == S_OK)
        m_bFiles = true;
    else
        m_bFiles = false;
    m_ullHoverStartTicks = 0;
    hLastItem            = nullptr;
    SetDropDescription(DROPIMAGE_NONE, nullptr, nullptr);
    return hr;
}

HRESULT CTreeDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect)
{
    wchar_t       targetName[MAX_PATH] = {0};
    TVHITTESTINFO hit;
    hit.pt = reinterpret_cast<POINT&>(pt);
    ScreenToClient(m_hTargetWnd, &hit.pt);
    hit.flags       = TVHT_ONITEM;
    HTREEITEM hItem = TreeView_HitTest(m_hTargetWnd, &hit);
    if (hItem != hLastItem)
        m_ullHoverStartTicks = 0;
    hLastItem = hItem;

    bool noDrop = false;

    if (hItem != nullptr)
    {
        TVITEMEX tvItem   = {0};
        tvItem.mask       = TVIF_TEXT;
        tvItem.hItem      = hItem;
        tvItem.pszText    = targetName;
        tvItem.cchTextMax = MAX_PATH;
        TreeView_GetItem(m_hTargetWnd, &tvItem);
        if ((m_pRepoBrowser->m_repoTree.GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED) != TVIS_EXPANDED)
        {
            if (m_ullHoverStartTicks == 0)
                m_ullHoverStartTicks = GetTickCount64();
            UINT timeout = 0;
            //SystemParametersInfo(SPI_GETMOUSEHOVERTIME, 0, &timeout, 0);
            timeout = 2000;
            if ((GetTickCount64() - m_ullHoverStartTicks) > timeout)
            {
                // expand the item
                m_pRepoBrowser->m_repoTree.Expand(hItem, TVE_EXPAND);
                m_ullHoverStartTicks = 0;
            }
        }
        else
            m_ullHoverStartTicks = 0;
        CTreeItem* pItem = reinterpret_cast<CTreeItem*>(m_pRepoBrowser->m_repoTree.GetItemData(hItem));
        if (pItem && pItem->m_bookmark)
        {
            TreeView_SelectDropTarget(m_hTargetWnd, NULL);
            *pdwEffect = DROPEFFECT_NONE;
            SetDropDescription(DROPIMAGE_NONE, sNoDrop, targetName);
            noDrop = true;
        }
        else
            TreeView_SelectDropTarget(m_hTargetWnd, hItem);
    }
    else
    {
        TreeView_SelectDropTarget(m_hTargetWnd, NULL);
        *pdwEffect           = DROPEFFECT_NONE;
        noDrop               = true;
        m_ullHoverStartTicks = 0;
    }

    if (!noDrop)
    {
        *pdwEffect = DROPEFFECT_MOVE;
        if (m_bFiles)
        {
            *pdwEffect = DROPEFFECT_COPY;
            SetDropDescription(DROPIMAGE_COPY, sImportDrop, targetName);
        }
        else if (grfKeyState & MK_CONTROL)
        {
            *pdwEffect = DROPEFFECT_COPY;
            SetDropDescription(DROPIMAGE_COPY, sCopyDrop, targetName);
        }
        else
        {
            SetDropDescription(DROPIMAGE_MOVE, sMoveDrop, targetName);
        }
    }
    m_pRepoBrowser->SetRightDrag((grfKeyState & MK_RBUTTON) != 0);
    CRect rect;
    m_pRepoBrowser->m_repoTree.GetWindowRect(&rect);
    if (rect.PtInRect(reinterpret_cast<POINT&>(pt)))
    {
        if (pt.y > (rect.bottom - 20))
        {
            m_pRepoBrowser->m_repoTree.SendMessage(WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
            m_ullHoverStartTicks = 0;
        }
        if (pt.y < (rect.top + 20))
        {
            m_pRepoBrowser->m_repoTree.SendMessage(WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), NULL);
            m_ullHoverStartTicks = 0;
        }
    }

    return CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CTreeDropTarget::DragLeave()
{
    TreeView_SelectDropTarget(m_hTargetWnd, NULL);
    SetDropDescription(DROPIMAGE_INVALID, nullptr, nullptr);
    m_ullHoverStartTicks = 0;
    return CIDropTarget::DragLeave();
}

CListDropTarget::CListDropTarget(CRepositoryBrowser* pRepoBrowser)
    : CBaseDropTarget(pRepoBrowser, pRepoBrowser->m_repoList.GetSafeHwnd())
{
}

bool CListDropTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD* pdwEffect, POINTL pt)
{
    // find the target url
    CString root;
    CString targetUrl;
    int     targetIndex = m_pRepoBrowser->m_repoList.GetNextItem(-1, LVNI_DROPHILITED);
    if (targetIndex >= 0)
    {
        CItem* item = reinterpret_cast<CItem*>(m_pRepoBrowser->m_repoList.GetItemData(targetIndex));
        targetUrl   = item->m_absolutePath;
        root        = item->m_repository.root;
    }
    else
    {
        HTREEITEM hDropTarget = m_pRepoBrowser->m_repoTree.GetSelectedItem();
        if (hDropTarget)
        {
            CTreeItem* item = reinterpret_cast<CTreeItem*>(m_pRepoBrowser->m_repoTree.GetItemData(hDropTarget));
            targetUrl       = item->m_url;
            root            = item->m_repository.root;
        }
    }
    HandleDropFormats(pFmtEtc, medium, pdwEffect, pt, targetUrl, root);
    ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
    return true; //let base free the medium
}

HRESULT CListDropTarget::DragEnter(IDataObject __RPC_FAR* pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect)
{
    HRESULT   hr    = CIDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
    FORMATETC ftEtc = {0};
    ftEtc.dwAspect  = DVASPECT_CONTENT;
    ftEtc.lindex    = -1;
    ftEtc.tymed     = TYMED_HGLOBAL;
    ftEtc.cfFormat  = CF_HDROP;
    if (pDataObj->QueryGetData(&ftEtc) == S_OK)
    {
        m_bFiles = true;
    }
    else
    {
        m_bFiles = false;
    }
    SetDropDescription(DROPIMAGE_NONE, nullptr, nullptr);
    return hr;
}

HRESULT CListDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect)
{
    wchar_t       targetName[MAX_PATH] = {0};
    LVHITTESTINFO hit;
    hit.pt = reinterpret_cast<POINT&>(pt);
    ScreenToClient(m_hTargetWnd, &hit.pt);
    hit.flags = LVHT_ONITEM;
    int iItem = ListView_HitTest(m_hTargetWnd, &hit);

    *pdwEffect = DROPEFFECT_MOVE;

    if (iItem >= 0)
    {
        ListView_GetItemText(m_hTargetWnd, iItem, 0, targetName, _countof(targetName));
        CItem* pItem = nullptr;
        if (m_pRepoBrowser->m_repoList.GetItemCount())
            pItem = reinterpret_cast<CItem*>(m_pRepoBrowser->m_repoList.GetItemData(iItem));
        if (pItem)
        {
            if (pItem->m_kind != svn_node_dir)
            {
                *pdwEffect = DROPEFFECT_NONE;
                SetDropDescription(DROPIMAGE_NONE, sNoDrop, targetName);
                ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
            }
            else
            {
                if (m_bFiles)
                {
                    *pdwEffect = DROPEFFECT_COPY;
                    SetDropDescription(DROPIMAGE_COPY, sImportDrop, targetName);
                }
                else if (grfKeyState & MK_CONTROL)
                {
                    *pdwEffect = DROPEFFECT_COPY;
                    SetDropDescription(DROPIMAGE_COPY, sCopyDrop, targetName);
                }
                else
                {
                    *pdwEffect = DROPEFFECT_MOVE;
                    SetDropDescription(DROPIMAGE_MOVE, sMoveDrop, targetName);
                }
                ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
                ListView_SetItemState(m_hTargetWnd, iItem, LVIS_DROPHILITED, LVIS_DROPHILITED);
            }
        }
        else
        {
            if (m_bFiles)
            {
                *pdwEffect = DROPEFFECT_COPY;
                SetDropDescription(DROPIMAGE_COPY, sImportDrop, targetName);
            }
            else if (grfKeyState & MK_CONTROL)
            {
                *pdwEffect = DROPEFFECT_COPY;
                SetDropDescription(DROPIMAGE_COPY, sCopyDrop, targetName);
            }
            else
            {
                *pdwEffect = DROPEFFECT_MOVE;
                SetDropDescription(DROPIMAGE_MOVE, sMoveDrop, targetName);
            }
        }
    }
    else
    {
        ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
        if (m_bFiles)
        {
            *pdwEffect = DROPEFFECT_COPY;
            SetDropDescription(DROPIMAGE_COPY, sImportDrop, targetName);
        }
        else if (grfKeyState & MK_CONTROL)
        {
            *pdwEffect = DROPEFFECT_COPY;
            SetDropDescription(DROPIMAGE_COPY, sCopyDrop, targetName);
        }
        else
            SetDropDescription(DROPIMAGE_MOVE, sMoveDrop, targetName);
    }

    m_pRepoBrowser->SetRightDrag((grfKeyState & MK_RBUTTON) != 0);

    CRect rect;
    m_pRepoBrowser->m_repoList.GetWindowRect(&rect);
    if (rect.PtInRect(reinterpret_cast<POINT&>(pt)))
    {
        if (pt.y > (rect.bottom - 20))
        {
            m_pRepoBrowser->m_repoList.SendMessage(WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), NULL);
        }
        if (pt.y < (rect.top + 20))
        {
            m_pRepoBrowser->m_repoList.SendMessage(WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), NULL);
        }
    }

    return CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CListDropTarget::DragLeave()
{
    ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
    SetDropDescription(DROPIMAGE_INVALID, nullptr, nullptr);
    return CIDropTarget::DragLeave();
}

CBaseDropTarget::CBaseDropTarget(CRepositoryBrowser* pRepoBrowser, HWND hTargetWnd)
    : CIDropTarget(hTargetWnd)
    , m_pRepoBrowser(pRepoBrowser)
    , m_bFiles(false)
{
    sNoDrop.LoadString(IDS_DROPDESC_NODROP);
    sImportDrop.LoadString(IDS_DROPDESC_IMPORT);
    sCopyDrop.LoadString(IDS_DROPDESC_COPY);
    sMoveDrop.LoadString(IDS_DROPDESC_MOVE);
}

void CBaseDropTarget::HandleDropFormats(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD* pdwEffect, POINTL pt, const CString& targetUrl, const CString& root) const
{
    if (pFmtEtc->cfFormat == CF_UNICODETEXT && medium.tymed == TYMED_HGLOBAL)
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

        m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), root, urlList, m_pRepoBrowser->GetRevision(), *pdwEffect, pt);
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

        m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), root, urlList, srcRev, *pdwEffect, pt);
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
            m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), root, urlList, m_pRepoBrowser->GetRevision(), *pdwEffect, pt);
        }
        GlobalUnlock(medium.hGlobal);
    }
}
