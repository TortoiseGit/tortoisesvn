// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2004 - Tim Kemp and Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "stdafx.h"
#include "TortoiseProc.h"
#include "messagebox.h"
#include "RepositoryTree.h"
#include "SysImageList.h"
#include "UnicodeUtils.h"
#include "WaitCursorEx.h"

#include "shlobj.h"
#include "utils.h"


// CRepositoryTree

IMPLEMENT_DYNAMIC(CRepositoryTree, CReportCtrl)
CRepositoryTree::CRepositoryTree(const CString& strUrl) :
	m_strUrl(strUrl),
	m_Revision(SVNRev::REV_HEAD)
{
	m_strUrl.TrimRight('/');
	CStringA temp = CUnicodeUtils::GetUTF8(m_strUrl);
	CUtils::Unescape(temp.GetBuffer());
	temp.ReleaseBuffer();
	m_strUrl = CUnicodeUtils::GetUnicode(temp);
}

CRepositoryTree::~CRepositoryTree()
{
}


BEGIN_MESSAGE_MAP(CRepositoryTree, CReportCtrl)
	ON_NOTIFY_REFLECT(RVN_ITEMEXPANDING, OnTvnItemexpanding)
	ON_NOTIFY_REFLECT(RVN_SELECTIONCHANGED, OnRvnItemSelected)
	//ON_NOTIFY_REFLECT(TVN_GETINFOTIP, OnTvnGetInfoTip)
END_MESSAGE_MAP()



// CRepositoryTree public interface

void CRepositoryTree::ChangeToUrl(const SVNUrl& svn_url)
{
	m_strUrl = svn_url.GetPath();
	m_Revision = svn_url.GetRevision();

	DeleteAllItems();

	AddFolder(m_strUrl, true);

	HTREEITEM hRoot = GetNextItem(RVTI_ROOT, RVGN_CHILD);
	HTREEITEM hItem = FindUrl(m_strUrl);
	if (hItem != 0)
		SetSelection(hItem);
}



// CRepositoryTree low level update functions

HTREEITEM CRepositoryTree::AddFolder(const CString& folder, bool force)
{
	CString folder_path;
	AfxExtractSubString(folder_path, SVNUrl(folder), 0, '\t');

	HTREEITEM hItem = FindUrl(folder_path);
	
	if (hItem == 0)
	{
		HTREEITEM hParentItem = RVTI_ROOT;

		CString parent_folder = SVNUrl(folder_path).GetParentPath();
		if (!parent_folder.IsEmpty())
		{
			hParentItem = FindUrl(parent_folder);
			if (hParentItem == 0)
			{
				if (!force)
					return NULL;
				hParentItem = AddFolder(parent_folder, true);
			}
		}

		DeleteDummyItem(hParentItem);
		if (force && hParentItem != RVTI_ROOT)
			Expand(hParentItem, RVE_EXPAND);
 
		CString folder_name = folder_path.Mid(parent_folder.GetLength()).TrimLeft('/');
		hItem = CReportCtrl::InsertItem(folder_name, m_nIconFolder, -1, -1, hParentItem, RVTI_SORT);

		InsertDummyItem(hItem);
		SetItemData(GetItemIndex(hItem), 0);
	}

	// insert other columns text
	for (int col=1; col<GetActiveSubItemCount(); col++)
	{
		CString temp;
		if (AfxExtractSubString(temp, folder, col, '\t'))
			SetItemText(GetItemIndex(hItem), col, temp);
	}

	return hItem;
}

HTREEITEM CRepositoryTree::AddFile(const CString& file, bool force)
{
	CString file_path;
	AfxExtractSubString(file_path, SVNUrl(file), 0, '\t');

	HTREEITEM hItem = FindUrl(file_path);
	
	if (hItem == 0)
	{
		HTREEITEM hParentItem = RVTI_ROOT;

		CString parent_folder = SVNUrl(file_path).GetParentPath();
		if (!parent_folder.IsEmpty())
		{
			hParentItem = FindUrl(parent_folder);
			if (hParentItem == 0)
			{
				if (!force)
					return NULL;
				hParentItem = AddFolder(parent_folder, true);
			}
		}

		DeleteDummyItem(hParentItem);
		if (force && hParentItem != RVTI_ROOT)
			Expand(hParentItem, RVE_EXPAND);
 
		CString file_name = file_path.Mid(parent_folder.GetLength()).TrimLeft('/');
		int icon_idx = SYS_IMAGE_LIST().GetFileIconIndex(file_name);
		hItem = CReportCtrl::InsertItem(file_name, icon_idx, -1, -1, hParentItem, RVTI_SORT);

		SetItemData(GetItemIndex(hItem), 0);
	}

	// insert other columns text
	for (int col=1; col<GetActiveSubItemCount(); col++)
	{
		CString temp;
		if (AfxExtractSubString(temp, file, col, '\t'))
			SetItemText(GetItemIndex(hItem), col, temp);
	}

	return hItem;
}

HTREEITEM CRepositoryTree::UpdateUrl(const CString& url)
{
	CString url_path;
	AfxExtractSubString(url_path, SVNUrl(url), 0, '\t');

	HTREEITEM hItem = FindUrl(url_path);
	
	if (hItem)
	{
		// insert other columns text
		for (int col=1; col<GetActiveSubItemCount(); col++)
		{
			CString temp;
			if (AfxExtractSubString(temp, url, col, '\t'))
				SetItemText(GetItemIndex(hItem), col, temp);
		}
	}

	return hItem;
}

bool CRepositoryTree::DeleteUrl(const CString& url)
{
	CString url_path;
	AfxExtractSubString(url_path, SVNUrl(url), 0, '\t');

	HTREEITEM hItem = FindUrl(url_path);
	
	if (hItem)
		return !!DeleteItem(hItem);

	return false;
}

HTREEITEM CRepositoryTree::FindUrl(const CString& url)
{
	HTREEITEM hRoot = GetNextItem(RVTI_ROOT, RVGN_CHILD);
	if (hRoot == NULL)
		return NULL;

	CString root_path = SVNUrl(url).GetRootPath();
	CString root_item = GetItemText(GetItemIndex(hRoot), 0);

	// root item must be compared case-insensitive
	if (root_path.CompareNoCase(root_item) != 0)
		return NULL;

	CString path = url.Right(url.GetLength() - root_path.GetLength() - 1);
	return FindPath(path, hRoot);
}



// CRepositoryTree private helpers

HTREEITEM CRepositoryTree::FindPath(const CString& path, HTREEITEM hParent)
{
	CString folder;

	if (!AfxExtractSubString(folder, path, 0, '/') || folder.IsEmpty())
	{
		// Abort recursion
		return hParent;
	}
	else
	{
		HTREEITEM hItem = GetNextItem(hParent, RVGN_CHILD);
		for (; hItem != NULL; hItem = GetNextItem(hItem, RVGN_NEXT))
		{
			// non-root items must be compared case-sensitive
			if (GetItemText(GetItemIndex(hItem), 0) == folder)
			{
				CString rest_path = path.Right(path.GetLength() - folder.GetLength() - 1);
				return FindPath(rest_path, hItem);
			}
		}
	}

	return NULL;
}

HTREEITEM CRepositoryTree::InsertDummyItem(HTREEITEM hItem)
{
	if (hItem != 0)
	{
		HTREEITEM hChild = GetNextItem(hItem, RVGN_CHILD);
		if (hChild == 0)
			return InsertItem(_T("Dummy"), -1, -1, -1, hItem);
	}

	return 0;
}

void CRepositoryTree::DeleteDummyItem(HTREEITEM hItem)
{
	if (hItem != 0)
	{
		HTREEITEM hChild = GetNextItem(hItem, RVGN_CHILD);
		if (hChild != 0 && GetItemText(GetItemIndex(hChild), 0).Compare(_T("Dummy")) == 0)
			DeleteItem(hChild);
	}
}

void CRepositoryTree::DeleteChildItems(HTREEITEM hItem)
{
	HTREEITEM hChild;
	while ((hChild = GetNextItem(hItem, RVGN_CHILD)) != 0)
		DeleteItem(hChild);
}

void CRepositoryTree::LoadChildItems(HTREEITEM hItem)
{
	CWaitCursorEx wait_cursor;

	CStringArray entries;
	CString folder = MakeUrl(hItem);

	m_svn.m_app = &theApp;

	if (m_svn.Ls(folder, m_Revision, entries, true))
	{
		DeleteChildItems(hItem);
		for (int i = 0; i < entries.GetCount(); ++i)
		{
			TCHAR type = entries[i].GetAt(0);
			CString entry = entries[i].Mid(1);

			switch (type)
			{
			case 'd':
				AddFolder(folder + _T("/") + entry);
				break;
			case 'f':
				AddFile(folder + _T("/") + entry);
				break;
			}
		}
		// Mark item as "successfully read"
		SetItemData(GetItemIndex(hItem), 1);
	}
	else
	{
		HTREEITEM hChild = GetNextItem(hItem, RVGN_CHILD);
		if (hChild == 0)
		{
			Expand(hItem, RVE_COLLAPSE);
			InsertDummyItem(hItem);
		}
		// Mark item as "not successfully read"
		SetItemData(GetItemIndex(hItem), 0);
	}
}



// CRepositoryTree message handlers

void CRepositoryTree::OnRvnItemSelected(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMREPORTVIEW pNMRV = reinterpret_cast<LPNMREPORTVIEW>(pNMHDR);
	
	if (pNMRV->nState & RVIS_FOCUSED)
	{
		CString path = GetFolderUrl(pNMRV->hItem);
		SVNUrl svn_url(path, m_Revision);
		if (m_pRepositoryBar != 0)
			m_pRepositoryBar->ShowUrl(svn_url);
	}

	*pResult = 0;
}

void CRepositoryTree::OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	if (bInit)
		return;

	LPNMREPORTVIEW pNMTreeView = reinterpret_cast<LPNMREPORTVIEW>(pNMHDR);
	if (pNMTreeView->hItem == NULL)
		return;

	RVITEM rvi;
	rvi.iItem = GetItemIndex(pNMTreeView->hItem);
	rvi.iSubItem = pNMTreeView->iSubItem;
	GetItem(&rvi);

	if (rvi.nState & RVIS_TREECLOSED)
	{
		if (GetItemData(GetItemIndex(pNMTreeView->hItem)) == 0)
		{
			DeleteDummyItem(pNMTreeView->hItem);
			LoadChildItems(pNMTreeView->hItem);
		}
	}
}

static INT CALLBACK SortCallback(INT iItem1, INT iSubItem1, INT iItem2, INT iSubItem2, LPARAM lParam)
{
	CRepositoryTree *rctrl = reinterpret_cast<CRepositoryTree*>(lParam);
	VERIFY(rctrl != NULL);

	RVITEM rvi1, rvi2;
	TCHAR szText1[REPORTCTRL_MAX_TEXT], szText2[REPORTCTRL_MAX_TEXT];

	rvi1.nMask = RVIM_TEXT;
	rvi1.iItem = iItem1;
	rvi1.iSubItem = iSubItem1;
	rvi1.lpszText = szText1;
	rvi1.iTextMax = REPORTCTRL_MAX_TEXT;
	VERIFY(rctrl->GetItem(&rvi1));

	rvi2.nMask = RVIM_TEXT;
	rvi2.iItem = iItem2;
	rvi2.iSubItem = iSubItem2;
	rvi2.lpszText = szText2;
	rvi2.iTextMax = REPORTCTRL_MAX_TEXT;
	VERIFY(rctrl->GetItem(&rvi2));

	switch (iSubItem1)
	{
	case 0:
		if (rvi1.iImage == rctrl->m_nIconFolder)
		{
			if (rvi2.iImage != rctrl->m_nIconFolder)
				return -1;
		}
		else if (rvi2.iImage == rctrl->m_nIconFolder)
		{
			return 1;
		}
		// fall through
	case 2:
		return _tcsicmp(szText1, szText2);
	case 1:
	case 3:
		return _tstoi(szText1) - _tstoi(szText2);
	case 4:
		// TODO: cannot sort by date...
		return _tcsicmp(szText1, szText2);
	}

	return 0;
}

void CRepositoryTree::Init(const SVNRev& revision)
{
	m_Revision = revision;
	bInit = TRUE;

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();

	//Set the list control image list 
	SetImageList(&SYS_IMAGE_LIST());

	RVSUBITEM rvs;
	CString temp;
	UndefineAllSubItems();
	//
	// column 0: contains tree
	temp.LoadString(IDS_LOG_FILE);
	rvs.lpszText = temp;
	rvs.iWidth = 300;
	rvs.iMinWidth = 200;
	DefineSubItem(0, &rvs);
	ActivateSubItem(0, 0);
	//
	// column 1: revision number
	temp.LoadString(IDS_LOG_REVISION);
	rvs.lpszText = temp;
	rvs.iWidth = 65;
	rvs.iMinWidth = 25;
	rvs.nFormat = RVCF_RIGHT|RVCF_TEXT;
	DefineSubItem(1, &rvs);
	ActivateSubItem(1, 1);
	//
	// column 2: author
	temp.LoadString(IDS_LOG_AUTHOR);
	rvs.lpszText = temp;
	rvs.iWidth = 85;
	rvs.iMinWidth = 25;
	rvs.nFormat = RVCF_LEFT|RVCF_TEXT;
	DefineSubItem(2, &rvs);
	ActivateSubItem(2, 2);
	//
	// column 3: size
	temp.LoadString(IDS_LOG_SIZE);
	rvs.iWidth = 65;
	rvs.iMinWidth = 25;
	rvs.nFormat = RVCF_RIGHT|RVCF_TEXT;
	DefineSubItem(3, &rvs);
	ActivateSubItem(3, 3);
	//
	// column 4: date
	temp.LoadString(IDS_LOG_DATE);
	rvs.lpszText = temp;
	rvs.iWidth = 125;
	rvs.iMinWidth = 25;
	rvs.nFormat = RVCF_LEFT|RVCF_TEXT;
	DefineSubItem(4, &rvs);
	ActivateSubItem(4, 4);

	SVNUrl svn_url(m_strUrl, m_Revision);
	ChangeToUrl(svn_url);

	SetSortCallback(SortCallback, (LPARAM)this);
	bInit = FALSE;
}

CString CRepositoryTree::GetFolderUrl(HTREEITEM hItem)
{
	if (hItem == NULL)
		return _T("");
	// prevent the user from selecting a file entry!
	RVITEM Item;
	Item.nMask = RVIM_IMAGE;
	Item.iItem = GetItemIndex(hItem);
	GetItem(&Item);
	if (Item.iImage != m_nIconFolder)
		hItem = GetNextItem(hItem, RVGN_PARENT);
	return MakeUrl(hItem);
}

CString CRepositoryTree::MakeUrl(HTREEITEM hItem)
{
	if (hItem == NULL)
		return _T("");
	CString strUrl = GetItemText(GetItemIndex(hItem), 0);
	HTREEITEM hParent = GetNextItem(hItem, RVGN_PARENT);
	while (hParent != NULL)
	{
		strUrl.Insert(0, GetItemText(GetItemIndex(hParent), 0) + _T("/"));
		hParent = GetNextItem(hParent, RVGN_PARENT);
	} // while (hParent != NULL)
	strUrl = CUtils::PathEscape(strUrl);
	return strUrl;
}

void CRepositoryTree::OnTvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMTVGETINFOTIP>(pNMHDR);
	_tcscpy(pGetInfoTip->pszText, MakeUrl(pGetInfoTip->hItem));
	*pResult = 0;
}

BOOL CRepositoryTree::IsFolder(HTREEITEM hItem)
{
	if (hItem == NULL)
		return TRUE;
	RVITEM Item;
	Item.nMask = RVIM_IMAGE;
	Item.iItem = GetItemIndex(hItem);
	GetItem(&Item);
	return Item.iImage == m_nIconFolder;
}

HTREEITEM CRepositoryTree::ItemExists(HTREEITEM parent, CString item)
{
	HTREEITEM hCurrent = GetNextItem(parent, RVGN_CHILD);
	while (hCurrent != NULL)
	{
		if (GetItemText(GetItemIndex(hCurrent), 0).CompareNoCase(item)==0)
			return hCurrent;
		hCurrent = GetNextItem(hCurrent, RVGN_NEXT);
	} // while (hCurrent != NULL)
	return NULL;
}

void CRepositoryTree::Refresh(HTREEITEM hItem)
{
	hItem = GetNextItem(hItem, RVGN_PARENT);
	if (hItem != 0)
		LoadChildItems(hItem);
}

void CRepositoryTree::RefreshMe(HTREEITEM hItem)
{
	if (hItem != 0)
		LoadChildItems(hItem);
}
