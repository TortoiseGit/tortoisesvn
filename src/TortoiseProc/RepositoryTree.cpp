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

#include "shlobj.h"
#include "utils.h"

// CRepositoryTree

IMPLEMENT_DYNAMIC(CRepositoryTree, CReportCtrl)
CRepositoryTree::CRepositoryTree(const CString& strUrl) :
	m_strUrl(strUrl)
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
	//ON_NOTIFY_REFLECT(TVN_GETINFOTIP, OnTvnGetInfoTip)
	//ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
END_MESSAGE_MAP()



// CRepositoryTree message handlers


void CRepositoryTree::OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMREPORTVIEW pNMTreeView = reinterpret_cast<LPNMREPORTVIEW>(pNMHDR);

	RVITEM rvi;
	rvi.iItem = GetItemIndex(pNMTreeView->hItem);
	rvi.iSubItem = pNMTreeView->iSubItem;
	GetItem(&rvi);

	if (rvi.nState & RVIS_TREECLOSED)
	{
		if (GetItemData(GetItemIndex(pNMTreeView->hItem))==0)
		{
			if (bInit)
				return;
			theApp.DoWaitCursor(1);
			m_svn.m_app = &theApp;
			HTREEITEM hChild = GetNextItem(pNMTreeView->hItem, RVGN_CHILD);
			if (hChild && GetItemText(GetItemIndex(hChild), 0).Compare(_T("Dummy")) == 0)
			{
				DeleteItem(hChild);
			}
			
			SetItemData(GetItemIndex(pNMTreeView->hItem), 1);
			CStringArray entries;
			if (m_svn.Ls(MakeUrl(pNMTreeView->hItem), m_nRevision, entries, true))
			{
				if ((entries.GetCount() == 1)&&(entries.GetAt(0).GetAt(0)=='f'))
				{
					CString temp;
					AfxExtractSubString(temp, entries.GetAt(0), 0, '\t');
					if (temp.Mid(1).CompareNoCase(GetItemText(GetItemIndex(pNMTreeView->hItem), 0))==0)
					{
						//perhaps the user specified a file instead of a directory!
						//try to expand the parent
						HTREEITEM hParent = GetNextItem(pNMTreeView->hItem, RVGN_PARENT);
						int iIcon = SYS_IMAGE_LIST().GetFileIconIndex(temp.Mid(1));
						SetItemImage(GetItemIndex(pNMTreeView->hItem), iIcon, iIcon);
						Expand(hParent, RVE_EXPAND);
						theApp.DoWaitCursor(-1);
						return;
					}
				}
				for (int i = 0; i < entries.GetCount(); ++i)
				{
					CString temp;
					AfxExtractSubString(temp, entries[i], 0, '\t');
					if (temp.GetAt(0) == 'd')
					{
						//temp = temp.Right(temp.GetLength()-1);
						HTREEITEM item;
						if ((item = ItemExists(pNMTreeView->hItem, temp.Mid(1)))!=NULL)
						{
							DeleteItem(item);
						}
						HTREEITEM hItem = InsertItem(temp, m_nIconFolder, -1, -1, pNMTreeView->hItem, RVTI_SORT);
						// insert other columns text
						for (int col=1; col<GetActiveSubItemCount(); col++)
						{
							if (AfxExtractSubString(temp, entries[i], col, '\t'))
								SetItemText(GetItemIndex(hItem), col, temp);
						}
						//SetItemState(hItem, 1, TVIF_CHILDREN);
						InsertItem(_T("Dummy"), -1, -1, -1, hItem);
						SetItemData(GetItemIndex(hItem), 0);
					} // if (temp.GetAt(0) == 'd') 
					if (temp.GetAt(0) == 'f')
					{
						//temp = temp.Right(temp.GetLength()-1);
						HTREEITEM item;
						if ((item = ItemExists(pNMTreeView->hItem, temp.Mid(1)))!=NULL)
						{
							DeleteItem(item);
						} 
						int iIcon = SYS_IMAGE_LIST().GetFileIconIndex(temp);
						HTREEITEM hItem = InsertItem(temp, iIcon, -1, -1, pNMTreeView->hItem, RVTI_SORT);
						// insert other columns text
						for (int col=1; col<GetActiveSubItemCount(); col++)
						{
							if (AfxExtractSubString(temp, entries[i], col, '\t'))
								SetItemText(GetItemIndex(hItem), col, temp);
						}
					} // if (temp.GetAt(0) == 'f') 
				} // for (int i = 0; i < entries.GetCount(); ++i) 
				HTREEITEM hCurrent = GetNextItem(pNMTreeView->hItem, RVGN_CHILD);
				while (hCurrent != NULL)
				{
					CString temp = GetItemText(GetItemIndex(hCurrent), 0);
					if ((temp.GetAt(0)=='d')||(temp.GetAt(0)=='f'))
						SetItemText(GetItemIndex(hCurrent), 0, temp.Mid(1));
					hCurrent = GetNextItem(hCurrent, RVGN_NEXT);
				} // while (hCurrent != NULL)
			} // if (m_svn.Ls(MakeUrl(pNMTreeView->itemNew.hItem), m_nRevision, entries)) 
			theApp.DoWaitCursor(-1);
		} // if (GetItemData(pNMTreeView->itemNew.hItem)==0) 
	} // if (rvi.nState & RVIS_TREECLOSED) 

	*pResult = 0;
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

void CRepositoryTree::Init(LONG revision)
{
	m_nRevision = revision;
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

	CStringArray arPaths;
	temp = m_strUrl;
	while ((temp.ReverseFind('/')>=0)&&(temp.GetAt(temp.ReverseFind('/')-1)!='/'))
	{
		arPaths.Add(temp.Right(temp.GetLength() - temp.ReverseFind('/') - 1));
		temp = temp.Left(temp.ReverseFind('/'));
	} // while ((temp.ReverseFind('/')>=0)&&(temp.GetAt(temp.ReverseFind('/')-1)!='/'))
	arPaths.Add(temp);
	HTREEITEM hItem = RVTI_ROOT;
	for (INT_PTR i=arPaths.GetUpperBound(); i>=0; i--)
	{
		hItem = InsertItem(arPaths.GetAt(i), m_nIconFolder, -1, -1, hItem, RVTI_LAST);
		EnsureVisible(hItem);
		//SetItemState(hItem, 1, TVIF_CHILDREN);
		SetItemData(GetItemIndex(hItem), 0);
		SetSelection(GetItemIndex(hItem));
	} // for (int i=arPaths.GetUpperBound(); i>=0; i--)
	InsertItem(_T("Dummy"), -1, -1, -1, hItem);

	SetSortCallback(SortCallback, (LPARAM)this);
	bInit = FALSE;
}

CString CRepositoryTree::GetFolderUrl(HTREEITEM hItem)
{
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
	Expand(hItem, /*RVE_COLLAPSERESET |`*/ RVE_COLLAPSE);
	SetItemData(GetItemIndex(hItem), 0);
	Expand(hItem, RVE_EXPAND);
}

void CRepositoryTree::RefreshMe(HTREEITEM hItem)
{
	Expand(hItem, /*RVE_COLLAPSERESET |*/ RVE_COLLAPSE);
	SetItemData(GetItemIndex(hItem), 0);
	Expand(hItem, RVE_EXPAND);
}
