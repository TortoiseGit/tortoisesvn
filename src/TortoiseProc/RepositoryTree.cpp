// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "messagebox.h"
#include "RepositoryTree.h"
#include "SysImageList.h"
#include "WaitCursorEx.h"
#include "RepositoryBar.h"
#include "TSVNPath.h"
#include "SVN.h"
#include ".\repositorytree.h"
#include "InputDlg.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "DragDropImpl.h"
#include "UnicodeUtils.h"
#include "RenameDlg.h"

// CRepositoryTree

IMPLEMENT_DYNAMIC(CRepositoryTree, CReportCtrl)
CRepositoryTree::CRepositoryTree(const CString& strUrl, BOOL bFile) :
	m_strUrl(strUrl),
	m_Revision(SVNRev::REV_HEAD),
	m_bFile(bFile),
	m_pProjectProperties(NULL),
	m_bSortNumerical(false)
{
	m_strUrl.TrimRight('/');
	m_strNoItems.LoadString(IDS_REPOBROWSE_INITWAIT);
}

CRepositoryTree::~CRepositoryTree()
{
}


BEGIN_MESSAGE_MAP(CRepositoryTree, CReportCtrl)
	ON_NOTIFY_REFLECT(RVN_ITEMEXPANDING, OnTvnItemexpanding)
	ON_NOTIFY_REFLECT(RVN_SELECTIONCHANGED, OnRvnItemSelected)
	ON_NOTIFY_REFLECT(TVN_GETINFOTIP, OnTvnGetInfoTip)
END_MESSAGE_MAP()



// CRepositoryTree public interface

void CRepositoryTree::ChangeToUrl(const SVNUrl& svn_url)
{
	m_strUrl = svn_url.GetPath();
	m_Revision = svn_url.GetRevision();
	// check if the repository root is still the same
	if (m_strReposRoot.Compare(svn_url.GetPath().Left(m_strReposRoot.GetLength()))!=0)
		m_strReposRoot.Empty();
	else
	{
		if ((svn_url.GetPath().GetLength() > m_strReposRoot.GetLength())&&(svn_url.GetPath().GetAt(m_strReposRoot.GetLength())!='/'))
			m_strReposRoot.Empty();
	}

	if (m_strReposRoot.IsEmpty())
	{
		SVN svn;
		m_strReposRoot = svn.GetRepositoryRoot(CTSVNPath(m_strUrl));
		m_strReposRoot = SVNUrl::Unescape(m_strReposRoot);
		m_Revision = SVNRev::REV_HEAD;
		if (m_pRepositoryBar)
			m_pRepositoryBar->SetRevision(m_Revision);
	}

	DeleteAllItems();

	if (m_bFile)
		AddFolder(svn_url.GetParentPath(), true, true);
	else
		AddFolder(m_strUrl, true, true);

	HTREEITEM hItem = FindUrl(m_strUrl);
	if (hItem != 0)
		SetSelection(hItem);
}



// CRepositoryTree low level update functions

HTREEITEM CRepositoryTree::AddFolder(const CString& folder, bool force, bool init)
{
	CString folder_path;
	if ((init)&&(!m_strReposRoot.IsEmpty()))
	{
		HTREEITEM hRootItem = FindUrl(m_strReposRoot);
		if (hRootItem == 0)
		{
			DeleteDummyItem(RVTI_ROOT);
			hRootItem = CReportCtrl::InsertItem(m_strReposRoot, m_nIconFolder, -1, -1, RVTI_ROOT, RVTI_SORT);
			InsertDummyItem(hRootItem);
			SetItemData(GetItemIndex(hRootItem), 0);
		}
	}
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
				hParentItem = AddFolder(parent_folder, force, init);
			}
		}

		DeleteDummyItem(hParentItem);
		if ((hParentItem != RVTI_ROOT)&&(!init))
			SetItemData(GetItemIndex(hParentItem), 1);
		if (force && hParentItem != RVTI_ROOT)
			Expand(hParentItem, RVE_EXPAND);
 
		CString folder_name = folder_path.Mid(parent_folder.GetLength()).TrimLeft('/');
		hItem = CReportCtrl::InsertItem(folder_name, m_nIconFolder, -1, -1, hParentItem, RVTI_SORT);

		InsertDummyItem(hItem);
		SetItemData(GetItemIndex(hItem), 0);
	}
	// insert other columns text
	CString temp;
	int col = 0;
	for (col=2; col<GetActiveSubItemCount()-1; col++)
	{
		if (AfxExtractSubString(temp, folder, col-1, '\t'))
			SetItemText(GetItemIndex(hItem), col, temp);
	}
	// get the time value and store that in the Param64
	if (AfxExtractSubString(temp, folder, col-1, '\t'))
	{
		RVITEM rvi;
		rvi.nMask = RVIM_PARAM64;
		rvi.iItem = GetItemIndex(hItem);
		rvi.iSubItem = col-1;
		rvi.Param64 = _ttoi64(temp);
		SetItem(&rvi);
	}
	// get the lock information
	if (AfxExtractSubString(temp, folder, col, '\t'))
	{
		// lock owner
		SetItemText(GetItemIndex(hItem), col, temp);
	}
	else
		SetItemText(GetItemIndex(hItem), col, _T(""));
	
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
				//if (!force)
				//	return NULL;
				hParentItem = AddFolder(parent_folder, force);
			}
		}

		DeleteDummyItem(hParentItem);
		SetItemData(GetItemIndex(hParentItem), 1);
		if (force && hParentItem != RVTI_ROOT)
			Expand(hParentItem, RVE_EXPAND);
 
		CString file_name = file_path.Mid(parent_folder.GetLength()).TrimLeft('/');
		int icon_idx = SYS_IMAGE_LIST().GetFileIconIndex(file_name);
		hItem = CReportCtrl::InsertItem(file_name, icon_idx, -1, -1, hParentItem, RVTI_SORT);

		SetItemData(GetItemIndex(hItem), 0);
	}

	// insert other columns text
	CString temp;
	int col = 0;
	for (col=2; col<GetActiveSubItemCount()-1; col++)
	{
		if (AfxExtractSubString(temp, file, col-1, '\t'))
			SetItemText(GetItemIndex(hItem), col, temp);
	}
	// get the time value and store that in the Param64
	if (AfxExtractSubString(temp, file, col-1, '\t'))
	{
		RVITEM rvi;
		rvi.nMask = RVIM_PARAM64;
		rvi.iItem = GetItemIndex(hItem);
		rvi.iSubItem = col-1;
		rvi.Param64 = _ttoi64(temp);
		SetItem(&rvi);
	}
	// get the lock information
	if (AfxExtractSubString(temp, file, col, '\t'))
	{
		// lock owner
		SetItemText(GetItemIndex(hItem), col, temp);
	}
	else
		SetItemText(GetItemIndex(hItem), col, _T(""));
	CString sExt;
	int dotPos = file_path.ReverseFind('.');
	int slashPos = file_path.ReverseFind('/');
	if ((dotPos > slashPos)&&(dotPos <= file_path.GetLength()))
		sExt = file_path.Mid(dotPos+1);
	SetItemText(GetItemIndex(hItem), 1, sExt);
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

	CString root_path = m_strReposRoot;
	if (root_path.IsEmpty())
	{
		root_path = SVNUrl(url).GetRootPath();
	}
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
			return InsertItem(_T("Error * "), -1, -1, -1, hItem);
	}

	return 0;
}

void CRepositoryTree::DeleteDummyItem(HTREEITEM hItem)
{
	if (hItem != 0)
	{
		HTREEITEM hChild = GetNextItem(hItem, RVGN_CHILD);
		if (hChild != 0 && GetItemText(GetItemIndex(hChild), 0).Find(_T("Error * ")) >= 0)
			DeleteItem(hChild);
	}
}

void CRepositoryTree::DeleteChildItems(HTREEITEM hItem)
{
	HTREEITEM hChild;
	while ((hChild = GetNextItem(hItem, RVGN_CHILD)) != 0)
		DeleteItem(hChild);
}

void CRepositoryTree::LoadChildItems(HTREEITEM hItem, BOOL recursive)
{
	CWaitCursorEx wait_cursor;

	CStringArray entries;
	CTSVNPath folder(MakeUrl(hItem));

	m_svn.SetPromptApp(&theApp);

	if (m_svn.Ls(folder, m_Revision, m_Revision, entries, true, recursive, true))
	{
		DeleteChildItems(hItem);
		if (entries.GetCount() == 1)
		{
			TCHAR type = entries[0].GetAt(0);
			CString name = entries.GetAt(0).Mid(1).Left(entries.GetAt(0).Find('\t')-1);
			if ((type == 'f')&&(name.CompareNoCase(GetItemText(GetItemIndex(hItem), 0))==0))
			{
				//we only got one file returned, which also has the same name
				//as the parent "folder". Now we need to check if the parent is really
				//a folder!
				CString item = entries.GetAt(0);
				entries.RemoveAll();
				if (m_svn.Ls(folder.GetContainingDirectory(), m_Revision, m_Revision, entries, true, recursive, true))
				{
					BOOL found = FALSE;
					for (int j=0; j<entries.GetCount(); ++j)
					{
						if (entries.GetAt(j).CompareNoCase(item)==0)
						{
							found = TRUE;
							break;
						}
					}
					if (found)
					{
						hItem = GetNextItem(hItem, RVGN_PARENT);
						DeleteChildItems(hItem);
						folder = folder.GetContainingDirectory();
					}
					else
					{
						entries.RemoveAll();
						entries.Add(item);
					}
				}
				else
				{
					entries.Add(item);
				}
			}
		}
		for (int i = 0; i < entries.GetCount(); ++i)
		{
			TCHAR type = entries[i].GetAt(0);
			CString entry = entries[i].Mid(1);

			switch (type)
			{
			case 'd':
				AddFolder(folder.GetSVNPathString() + _T("/") + entry);
				break;
			case 'f':
				AddFile(folder.GetSVNPathString() + _T("/") + entry);
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
			if (hItem != 0)
			{
				HTREEITEM hChild = GetNextItem(hItem, RVGN_CHILD);
				if (hChild == 0)
				{
					CString err = _T("Error * ") + m_svn.GetLastErrorMessage();
					err.Replace('\n', ' ');
					InsertItem(err, -1, -1, -1, hItem);
				}
			}
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
		// prevent the user from selecting an error entry!
		HTREEITEM hItem = pNMRV->hItem;

		CString path = MakeUrl(hItem);
		if (path.Find('*') < 0)
		{
			SVNUrl svn_url(path, m_Revision);
			if (m_pRepositoryBar != 0)
				m_pRepositoryBar->ShowUrl(svn_url);
		}
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
			LoadChildItems(pNMTreeView->hItem, GetKeyState(VK_CONTROL)&0x8000);
		}
	}
}

static INT CALLBACK SortCallback(INT iItem1, INT iSubItem1, INT iItem2, INT iSubItem2, LPARAM lParam)
{
	CRepositoryTree *rctrl = reinterpret_cast<CRepositoryTree*>(lParam);
	VERIFY(rctrl != NULL);

	RVITEM rvi1, rvi2;
	TCHAR szText1[REPORTCTRL_MAX_TEXT], szText2[REPORTCTRL_MAX_TEXT];

	rvi1.nMask = RVIM_TEXT | RVIM_LPARAM;
	rvi1.iItem = iItem1;
	rvi1.iSubItem = iSubItem1;
	rvi1.lpszText = szText1;
	rvi1.iTextMax = REPORTCTRL_MAX_TEXT;
	VERIFY(rctrl->GetItem(&rvi1));

	rvi2.nMask = RVIM_TEXT | RVIM_LPARAM;
	rvi2.iItem = iItem2;
	rvi2.iSubItem = iSubItem2;
	rvi2.lpszText = szText2;
	rvi2.iTextMax = REPORTCTRL_MAX_TEXT;
	VERIFY(rctrl->GetItem(&rvi2));

	switch (iSubItem1)
	{
	case 0:
	case 1:
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
	case 3:
		if (rctrl->m_bSortNumerical)
			return CStringUtils::CompareNumerical(szText1, szText2);
		return _tcsicmp(szText1, szText2);
	case 2:
	case 4:
		return _tstoi(szText1) - _tstoi(szText2);
	case 5:
		if (rvi2.Param64 > rvi1.Param64)
			return -1;
		if (rvi2.Param64 < rvi1.Param64)
			return 1;
		return 0;
	case 6:
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
	// column 1: file extension
	temp.LoadString(IDS_STATUSLIST_COLEXT);
	rvs.lpszText = temp;
	rvs.iWidth = 65;
	rvs.iMinWidth = 25;
	rvs.nFormat = RVCF_LEFT|RVCF_TEXT;
	DefineSubItem(1, &rvs);
	ActivateSubItem(1, 1);
	//
	// column 2: revision number
	temp.LoadString(IDS_LOG_REVISION);
	rvs.lpszText = temp;
	rvs.iWidth = 65;
	rvs.iMinWidth = 25;
	rvs.nFormat = RVCF_RIGHT|RVCF_TEXT;
	DefineSubItem(2, &rvs);
	ActivateSubItem(2, 2);
	//
	// column 3: author
	temp.LoadString(IDS_LOG_AUTHOR);
	rvs.lpszText = temp;
	rvs.iWidth = 85;
	rvs.iMinWidth = 25;
	rvs.nFormat = RVCF_LEFT|RVCF_TEXT;
	DefineSubItem(3, &rvs);
	ActivateSubItem(3, 3);
	//
	// column 4: size
	temp.LoadString(IDS_LOG_SIZE);
	rvs.iWidth = 65;
	rvs.iMinWidth = 25;
	rvs.nFormat = RVCF_RIGHT|RVCF_TEXT;
	DefineSubItem(4, &rvs);
	ActivateSubItem(4, 4);
	//
	// column 5: date
	temp.LoadString(IDS_LOG_DATE);
	rvs.lpszText = temp;
	rvs.iWidth = 125;
	rvs.iMinWidth = 25;
	rvs.nFormat = RVCF_LEFT|RVCF_TEXT;
	DefineSubItem(5, &rvs);
	ActivateSubItem(5, 5);
	//
	// column 6: lock owner
	temp.LoadString(IDS_STATUSLIST_COLLOCK);
	rvs.lpszText = temp;
	rvs.iWidth = 125;
	rvs.iMinWidth = 25;
	rvs.nFormat = RVCF_LEFT|RVCF_TEXT;
	DefineSubItem(6, &rvs);
	ActivateSubItem(6, 6);

	SVNUrl svn_url(m_strUrl, m_Revision);

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
		if (strUrl.Find('*')>=0)
		{
			strUrl.Empty();
			strUrl = GetItemText(GetItemIndex(hParent), 0);
			hParent = GetNextItem(hParent, RVGN_PARENT);
			continue;
		}
		else
		{
			strUrl.Insert(0, GetItemText(GetItemIndex(hParent), 0) + _T("/"));
		}
		hParent = GetNextItem(hParent, RVGN_PARENT);
	} // while (hParent != NULL)
	
	// since we *know* that '%' chars which we get here aren't used
	// as escaping chars, we must escape them here.
	// Otherwise, if we don't escape them here, other functions
	// might treat them as escaping chars.
	
	strUrl.Replace(_T("%"), _T("%25"));
	
	return strUrl;
}

void CRepositoryTree::OnTvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMTVGETINFOTIP>(pNMHDR);
	if (GetItemData(GetItemIndex(pGetInfoTip->hItem)) == 0)
		_tcsncpy_s(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, m_svn.GetLastErrorMessage(), pGetInfoTip->cchTextMax);
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
	{
		LoadChildItems(hItem, GetKeyState(VK_CONTROL)&0x8000);
	}
}

void CRepositoryTree::RefreshMe(HTREEITEM hItem)
{
	if (hItem != 0)
	{
		if (IsFolder(hItem))
			LoadChildItems(hItem, (GetKeyState(VK_CONTROL)&0x8000)!=0);
	}
}

BOOL CRepositoryTree::BeginEdit(INT iRow, INT iColumn, UINT nKey)
{
	if (!m_Revision.IsHead())
		return FALSE;
	if (iColumn>0)
		return FALSE;
	// are we on the root URL?
	if (MakeUrl(GetItemHandle(iRow)).Compare(m_strReposRoot)==0)
		return FALSE;
	if (iColumn < 0)
		iColumn = 0;

	return CReportCtrl::BeginEdit(iRow, iColumn, nKey);
}

void CRepositoryTree::EndEdit(BOOL bUpdate /* = TRUE */, LPNMRVITEMEDIT lpnmrvie /* = NULL */)
{
	ATLTRACE("EndEdit\n");
	if ((lpnmrvie == NULL)||(lpnmrvie->lpszText == NULL))
	{
		CReportCtrl::EndEdit(FALSE, lpnmrvie);
		return;
	}
	
	HTREEITEM hItem = GetItemHandle(m_iEditItem);

	CString sOldName = GetItemText(m_iEditItem, 0);
	CString sOldUrl = MakeUrl(hItem);
	CString sNewName = lpnmrvie->lpszText;
	CString sNewUrl = sOldUrl.Left(sOldUrl.ReverseFind('/')+1) + sNewName;
	
	if ((!bUpdate)||(sNewName.FindOneOf(_T("/\\?*\"<>|"))>=0)||(sOldName==sNewName))
	{
		CReportCtrl::EndEdit(FALSE, lpnmrvie);
		return;
	}
	
	SVN svn;
	svn.SetPromptApp(&theApp);
	CWaitCursorEx wait_cursor;
	CInputDlg input(this);
	input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
	CStringUtils::RemoveAccelerators(input.m_sHintText);
	input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
	CStringUtils::RemoveAccelerators(input.m_sTitle);
	input.m_pProjectProperties = m_pProjectProperties;
	input.m_sInputText.LoadString(IDS_INPUT_RENAMELOGMSG);
	if (input.DoModal() == IDOK)
	{
		if (!svn.Move(CTSVNPath(sOldUrl), CTSVNPath(sNewUrl), TRUE, input.m_sInputText))
		{
			wait_cursor.Hide();
			CReportCtrl::EndEdit(FALSE, lpnmrvie);
			CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			return;
		}
		CReportCtrl::EndEdit(bUpdate, lpnmrvie);
		return;
	}
	CReportCtrl::EndEdit(FALSE, lpnmrvie);
}

DROPEFFECT CRepositoryTree::OnDrag(int iItem, int iSubItem, IDataObject * pDataObj, DWORD grfKeyState)
{
	if (iSubItem)
		return DROPEFFECT_NONE;		// no dropping on subitems
		
	RVITEM Item;
	Item.nMask = RVIM_IMAGE;
	Item.iItem = iItem;
	GetItem(&Item);
	if (Item.iImage != m_nIconFolder)
		return DROPEFFECT_NONE;		// no dropping on files
	FORMATETC ftetc={0}; 
	ftetc.dwAspect = DVASPECT_CONTENT; 
	ftetc.lindex = -1; 
	ftetc.tymed = TYMED_HGLOBAL; 
	ftetc.cfFormat=CF_HDROP; 
	if (pDataObj->QueryGetData(&ftetc) == S_OK)
	{
		return DROPEFFECT_COPY;
	}
	ftetc.cfFormat = CF_UNICODETEXT;
	if (pDataObj->QueryGetData(&ftetc) == S_OK)
	{
		// user drags a text on us.
		
		// check if we're over a selected item: dropping on
		// the source doesn't work
		for (INT_PTR i=0; i<m_arDraggedIndexes.GetCount(); ++i)
		{
			if (m_arDraggedIndexes[i] == (DWORD)iItem)
				return DROPEFFECT_NONE;
		}
		
		// we only accept repository urls, but we check for that
		// when the user finally drops it on us.
		if (grfKeyState & MK_CONTROL)
		{
			ATLTRACE("copy url\n");
			return DROPEFFECT_COPY;
		}
		ATLTRACE("move url\n");
		return DROPEFFECT_MOVE;
	}
	ATLTRACE("no url\n");
	return DROPEFFECT_NONE;
}

void CRepositoryTree::OnDrop(int iItem, int iSubItem, IDataObject * pDataObj, DWORD grfKeyState)
{
	STGMEDIUM medium;
	FORMATETC ftetc={0}; 
	ftetc.dwAspect = DVASPECT_CONTENT; 
	ftetc.lindex = -1; 
	ftetc.tymed = TYMED_HGLOBAL; 
	ftetc.cfFormat=CF_HDROP;
	
	m_DroppedPaths.Clear();
	
	if (pDataObj->GetData(&ftetc, &medium) == S_OK)
	{
		if(ftetc.cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
		{
			HDROP hDrop = (HDROP)GlobalLock(medium.hGlobal);
			if(hDrop != NULL)
			{
				TCHAR szFileName[MAX_PATH];

				UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); 
				for(UINT i = 0; i < cFiles; ++i)
				{
					DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));
					m_DroppedPaths.AddPath(CTSVNPath(szFileName));
				}  
			}
			GlobalUnlock(medium.hGlobal);
			GetParent()->PostMessage(WM_FILESDROPPED, iItem, iSubItem);
		}
		ReleaseStgMedium(&medium);
	}
	
	ftetc.cfFormat = CF_UNICODETEXT;
	if (pDataObj->GetData(&ftetc, &medium) == S_OK)
	{
		if(ftetc.cfFormat == CF_UNICODETEXT && medium.tymed == TYMED_HGLOBAL)
		{
			CString sDestUrl = MakeUrl(GetItemHandle(iItem));
			TCHAR* pStr = (TCHAR*)GlobalLock(medium.hGlobal);
			CString urls;
			if(pStr != NULL)
			{
				urls = pStr;
			}
			GlobalUnlock(medium.hGlobal);
			urls.Replace(_T("\r\n"), _T("*"));
			CTSVNPathList urlList;
			CTSVNPathList destUrlList;
			urlList.LoadFromAsteriskSeparatedString(urls);
			// now check if the text dropped on us is a list of URL's
			bool bAllUrls = true;
			for (int i=0; i<urlList.GetCount(); ++i)
			{
				if (!urlList[i].IsUrl())
				{
					bAllUrls = false;
					break;
				}
			}
			if (bAllUrls)
			{
				if (m_bRightDrag)
				{
					// right drag, offer to rename the URL
					for (int i=0; i<urlList.GetCount(); ++i)
					{
						CRenameDlg renDlg;
						renDlg.m_name = urlList[i].GetFileOrDirectoryName();
						renDlg.m_windowtitle.Format(IDS_PROC_NEWNAME, renDlg.m_name);
						renDlg.m_label.LoadString(IDS_PROC_NEWNAMELABEL);
						if (renDlg.DoModal()==IDOK)
						{
							CTSVNPath tempUrl = urlList[i];
							tempUrl = tempUrl.GetContainingDirectory();
							tempUrl.AppendPathString(renDlg.m_name);
							destUrlList.AddPath(tempUrl);
						}
						else
							return;
					}
				}
				else
				{
					destUrlList = urlList;
				}
				if (grfKeyState & MK_CONTROL)
				{
					// copy the URLs to the new location
					SVN svn;
					svn.SetPromptApp(&theApp);
					CWaitCursorEx wait_cursor;
					if (urlList.GetCount() > 1)
					{
						if (CMessageBox::Show(m_hWnd, IDS_REPOBROWSE_MULTICOPY, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)!=IDYES)
						{
							wait_cursor.Hide();
							ReleaseStgMedium(&medium);
							return;
						}
					}
					CInputDlg input(this);
					input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
					CStringUtils::RemoveAccelerators(input.m_sHintText);
					input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
					CStringUtils::RemoveAccelerators(input.m_sTitle);
					input.m_pProjectProperties = m_pProjectProperties;
					if (m_pProjectProperties->sLogTemplate.IsEmpty())
						input.m_sInputText.LoadString(IDS_INPUT_COPYLOGMSG);
					if (input.DoModal() == IDOK)
					{
						for (int index=0; index<urlList.GetCount(); ++index)
						{
							if (!FindUrl(sDestUrl+_T("/")+destUrlList[index].GetFileOrDirectoryName()))
							{
								if (!svn.Copy(urlList[index], CTSVNPath(sDestUrl+_T("/")+destUrlList[index].GetFileOrDirectoryName()), m_Revision, input.m_sInputText))
								{
									wait_cursor.Hide();
									CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
									break;
								}
								if (m_Revision.IsHead())
								{
									HTREEITEM hItem = FindUrl(urlList[index].GetSVNPathString());
									if (hItem)
									{
										if (IsFolder(hItem))
											AddFolder(sDestUrl+_T("/")+destUrlList[index].GetFileOrDirectoryName());
										else
											AddFile(sDestUrl+_T("/")+destUrlList[index].GetFileOrDirectoryName());
										Refresh(hItem);
									}
								}
							}
							else
							{
								wait_cursor.Hide();
								CString sMsg;
								sMsg.Format(IDS_REPOBROWSE_URLALREADYEXISTS, sDestUrl+_T("/")+urlList[index].GetFileOrDirectoryName());
								CMessageBox::Show(m_hWnd, sMsg, _T("TortoiseSVN"), MB_ICONERROR);
								break;
							}
						}
					} // if (input.DoModal() == IDOK) 
				}
				else
				{
					// move the URLs to the new location
					SVN svn;
					svn.SetPromptApp(&theApp);
					CWaitCursorEx wait_cursor;
					if (urlList.GetCount() > 1)
					{
						if (CMessageBox::Show(m_hWnd, IDS_REPOBROWSE_MULTIMOVE, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION)!=IDYES)
						{
							wait_cursor.Hide();
							ReleaseStgMedium(&medium);
							return;
						}
					}
					CInputDlg input(this);
					input.m_sHintText.LoadString(IDS_INPUT_ENTERLOG);
					CStringUtils::RemoveAccelerators(input.m_sHintText);
					input.m_sTitle.LoadString(IDS_INPUT_LOGTITLE);
					CStringUtils::RemoveAccelerators(input.m_sTitle);
					input.m_pProjectProperties = m_pProjectProperties;
					if (m_pProjectProperties->sLogTemplate.IsEmpty())
						input.m_sInputText.LoadString(IDS_INPUT_MOVELOGMSG);
					if (input.DoModal() == IDOK)
					{
						for (int index=0; index<urlList.GetCount(); ++index)
						{
							if (!FindUrl(sDestUrl+_T("/")+destUrlList[index].GetFileOrDirectoryName()))
							{
								if (!svn.Move(urlList[index], CTSVNPath(sDestUrl+_T("/")+destUrlList[index].GetFileOrDirectoryName()), m_Revision, input.m_sInputText))
								{
									wait_cursor.Hide();
									CMessageBox::Show(this->m_hWnd, svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
									break;
								}
								if (m_Revision.IsHead())
								{
									HTREEITEM hItem = FindUrl(urlList[index].GetSVNPathString());
									if (hItem)
									{
										if (IsFolder(hItem))
											AddFolder(sDestUrl+_T("/")+destUrlList[index].GetFileOrDirectoryName());
										else
											AddFile(sDestUrl+_T("/")+destUrlList[index].GetFileOrDirectoryName());
										Refresh(hItem);
									}
								}
							}
							else
							{
								wait_cursor.Hide();
								CString sMsg;
								sMsg.Format(IDS_REPOBROWSE_URLALREADYEXISTS, sDestUrl+_T("/")+urlList[index].GetFileOrDirectoryName());
								CMessageBox::Show(m_hWnd, sMsg, _T("TortoiseSVN"), MB_ICONERROR);
								break;
							}
						}
					} // if (input.DoModal() == IDOK) 
				}
			} // if (bAllUrls)
			else
			{
				CMessageBox::Show(m_hWnd, IDS_REPOBROWSE_INVALIDDROPDATA, IDS_APPNAME, MB_ICONERROR);
			}
		} // if(ftetc.cfFormat == CF_UNICODETEXT && medium.tymed == TYMED_HGLOBAL)
		ReleaseStgMedium(&medium);
	} // if (pDataObj->GetData(&ftetc, &medium) == S_OK)
}

void CRepositoryTree::OnBeginDrag()
{
	int si = GetFirstSelectedItem();
	if (si == RVI_INVALID)
		return;

	CIDropSource* pdsrc = new CIDropSource;
	if(pdsrc == NULL)
		return;
	pdsrc->AddRef();
	CIDataObject* pdobj = new CIDataObject(pdsrc);
	if(pdobj == NULL)
		return;
	pdobj->AddRef();

	m_arDraggedIndexes.RemoveAll();
	CTSVNPathList urlList;
	do
	{
		urlList.AddPath(CTSVNPath(MakeUrl(GetItemHandle(si))));
		m_arDraggedIndexes.Add(si);
		si = GetNextSelectedItem(si);
	} while (si != RVI_INVALID);

	// first format: unicode text
	CString urls;
	for (int i=0; i<urlList.GetCount(); ++i)
	{
		urls += urlList[i].GetSVNPathString() + _T("\r\n");
	}
	FORMATETC fmtetc = {0}; 
	fmtetc.cfFormat = CF_UNICODETEXT; 
	fmtetc.dwAspect = DVASPECT_CONTENT; 
	fmtetc.lindex = -1; 
	fmtetc.tymed = TYMED_HGLOBAL;
	// Init the medium used
	STGMEDIUM medium = {0};
	medium.tymed = TYMED_HGLOBAL;

	medium.hGlobal = GlobalAlloc(GHND, (urls.GetLength()+1)*sizeof(TCHAR));
	if (medium.hGlobal)
	{
		TCHAR* pMem = (TCHAR*)GlobalLock(medium.hGlobal);
		_tcscpy_s(pMem, urls.GetLength()+1, (LPCTSTR)urls);
		GlobalUnlock(medium.hGlobal);

		pdobj->SetData(&fmtetc, &medium, TRUE);
	}

	m_bRightDrag = !!(GetKeyState(VK_RBUTTON)&0x8000);
	// Initiate the Drag & Drop
	::DoDragDrop(pdobj, pdsrc, DROPEFFECT_MOVE | DROPEFFECT_COPY, &m_dwEffect);
	pdsrc->Release();
	pdobj->Release();
}

BOOL CRepositoryTree::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_F2:
			{
				BeginEdit(m_iFocusRow, m_iFocusColumn, VK_LBUTTON);
			}
			break;
		}
	}
	return CReportCtrl::PreTranslateMessage(pMsg);
}
