
#include "StdAfx.h"
#include "RepoDrags.h"
#include "RepositoryBrowser.h"

CTreeDropTarget::CTreeDropTarget(CRepositoryBrowser * pRepoBrowser) : CIDropTarget(pRepoBrowser->m_RepoTree.GetSafeHwnd())
	, m_pRepoBrowser(pRepoBrowser)
	, m_bFiles(false)
{
}

bool CTreeDropTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect)
{
	// find the target
	CString targetUrl;
	HTREEITEM hDropTarget = m_pRepoBrowser->m_RepoTree.GetNextItem(TVI_ROOT, TVGN_DROPHILITE);
	if (hDropTarget)
	{
		targetUrl = ((CTreeItem*)m_pRepoBrowser->m_RepoTree.GetItemData(hDropTarget))->url;
	}
	if (pFmtEtc->cfFormat == CF_UNICODETEXT && medium.tymed == TYMED_HGLOBAL)
	{
		TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
		CString urls;
		if(pStr != NULL)
		{
			urls = pStr;
		}
		GlobalUnlock(medium.hGlobal);
		urls.Replace(_T("\r\n"), _T("*"));
		CTSVNPathList urlList;
		urlList.LoadFromAsteriskSeparatedString(urls);

		m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), urlList, *pdwEffect);
	}

	if(pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
	{
		HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
		if(hDrop != NULL)
		{
			CTSVNPathList urlList;
			TCHAR szFileName[MAX_PATH];

			UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); 
			for(UINT i = 0; i < cFiles; ++i)
			{
				DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));
				urlList.AddPath(CTSVNPath(szFileName));
			}
			m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), urlList, *pdwEffect);
		}
		GlobalUnlock(medium.hGlobal);
	}
	TreeView_SelectDropTarget(m_hTargetWnd, NULL);
	return true;
}

HRESULT CTreeDropTarget::DragEnter(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	FORMATETC ftetc={0}; 
	ftetc.dwAspect = DVASPECT_CONTENT; 
	ftetc.lindex = -1; 
	ftetc.tymed = TYMED_HGLOBAL; 
	ftetc.cfFormat=CF_HDROP; 
	if (pDataObj->QueryGetData(&ftetc) == S_OK)
		m_bFiles = true;
	else
		m_bFiles = false;
	return CIDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
}

HRESULT CTreeDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	TVHITTESTINFO hit;
	hit.pt = (POINT&)pt;
	ScreenToClient(m_hTargetWnd,&hit.pt);
	hit.flags = TVHT_ONITEM;
	HTREEITEM hItem = TreeView_HitTest(m_hTargetWnd,&hit);
	if (hItem != NULL)
	{
		if (m_bFiles)
			*pdwEffect = DROPEFFECT_COPY;
		TreeView_SelectDropTarget(m_hTargetWnd, hItem);
	}
	else
	{
		TreeView_SelectDropTarget(m_hTargetWnd, NULL);
		*pdwEffect = DROPEFFECT_NONE;
	}
	CRect rect;
	m_pRepoBrowser->m_RepoTree.GetWindowRect(&rect);
	if (rect.PtInRect((POINT&)pt))
	{
		if (pt.y > (rect.bottom-20))
		{
			m_pRepoBrowser->m_RepoTree.SendMessage(WM_VSCROLL, MAKEWPARAM (SB_LINEDOWN, 0), NULL);
		}
		if (pt.y < (rect.top+20))
		{
			m_pRepoBrowser->m_RepoTree.SendMessage(WM_VSCROLL, MAKEWPARAM (SB_LINEUP, 0), NULL);
		}
	}

	return CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CTreeDropTarget::DragLeave(void)
{
	TreeView_SelectDropTarget(m_hTargetWnd, NULL);
	return CIDropTarget::DragLeave();
}

CListDropTarget::CListDropTarget(CRepositoryBrowser * pRepoBrowser):CIDropTarget(pRepoBrowser->m_RepoList.GetSafeHwnd())
	, m_pRepoBrowser(pRepoBrowser)
	, m_bFiles(false)
{
}	

bool CListDropTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD *pdwEffect)
{
	// find the target url
	CString targetUrl;
	int targetIndex = m_pRepoBrowser->m_RepoList.GetNextItem(-1, LVNI_DROPHILITED);
	if (targetIndex >= 0)
	{
		targetUrl = ((CItem*)m_pRepoBrowser->m_RepoList.GetItemData(targetIndex))->absolutepath;
	}
	else
	{
		HTREEITEM hDropTarget = m_pRepoBrowser->m_RepoTree.GetSelectedItem();
		if (hDropTarget)
		{
			targetUrl = ((CTreeItem*)m_pRepoBrowser->m_RepoTree.GetItemData(hDropTarget))->url;
		}
	}
	if(pFmtEtc->cfFormat == CF_UNICODETEXT && medium.tymed == TYMED_HGLOBAL)
	{
		TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
		CString urls;
		if(pStr != NULL)
		{
			urls = pStr;
		}
		GlobalUnlock(medium.hGlobal);
		urls.Replace(_T("\r\n"), _T("*"));
		CTSVNPathList urlList;
		urlList.LoadFromAsteriskSeparatedString(urls);
		m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), urlList, *pdwEffect);
	}

	if(pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
	{
		HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
		if(hDrop != NULL)
		{
			CTSVNPathList urlList;
			TCHAR szFileName[MAX_PATH];

			UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); 
			for(UINT i = 0; i < cFiles; ++i)
			{
				DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));
				urlList.AddPath(CTSVNPath(szFileName));
			}
			m_pRepoBrowser->OnDrop(CTSVNPath(targetUrl), urlList, *pdwEffect);
		}
		GlobalUnlock(medium.hGlobal);
	}
	ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
	return true; //let base free the medium
}

HRESULT CListDropTarget::DragEnter(IDataObject __RPC_FAR *pDataObj, DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	FORMATETC ftetc={0}; 
	ftetc.dwAspect = DVASPECT_CONTENT; 
	ftetc.lindex = -1; 
	ftetc.tymed = TYMED_HGLOBAL; 
	ftetc.cfFormat=CF_HDROP; 
	if (pDataObj->QueryGetData(&ftetc) == S_OK)
	{
		m_bFiles = true;
	}
	else
	{
		m_bFiles = false;
	}
	return CIDropTarget::DragEnter(pDataObj, grfKeyState, pt, pdwEffect);
}


HRESULT CListDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR *pdwEffect)
{
	LVHITTESTINFO hit;
	hit.pt = (POINT&)pt;
	ScreenToClient(m_hTargetWnd,&hit.pt);
	hit.flags = LVHT_ONITEM;
	int iItem = ListView_HitTest(m_hTargetWnd,&hit);

	if (grfKeyState & MK_SHIFT)
		*pdwEffect = DROPEFFECT_MOVE;

	if (iItem >= 0)
	{
		CItem * pItem = (CItem*)m_pRepoBrowser->m_RepoList.GetItemData(iItem);
		if (pItem)
		{
			if (pItem->kind != svn_node_dir)
			{
				*pdwEffect = DROPEFFECT_NONE;
				ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
			}
			else
			{
				if (m_bFiles)
					*pdwEffect = DROPEFFECT_COPY;
				ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
				ListView_SetItemState(m_hTargetWnd, iItem, LVIS_DROPHILITED, LVIS_DROPHILITED);
			}
		}
	}
	else
	{
		ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
	}

	CRect rect;
	m_pRepoBrowser->m_RepoList.GetWindowRect(&rect);
	if (rect.PtInRect((POINT&)pt))
	{
		if (pt.y > (rect.bottom-20))
		{
			m_pRepoBrowser->m_RepoList.SendMessage(WM_VSCROLL, MAKEWPARAM (SB_LINEDOWN, 0), NULL);
		}
		if (pt.y < (rect.top+20))
		{
			m_pRepoBrowser->m_RepoList.SendMessage(WM_VSCROLL, MAKEWPARAM (SB_LINEUP, 0), NULL);
		}
	}

	return CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
}

HRESULT CListDropTarget::DragLeave(void)
{
	ListView_SetItemState(m_hTargetWnd, -1, 0, LVIS_DROPHILITED);
	return CIDropTarget::DragLeave();
}
