// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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

#include "shlobj.h"
#include "utils.h"

// CRepositoryTree

IMPLEMENT_DYNAMIC(CRepositoryTree, CTreeCtrl)
CRepositoryTree::CRepositoryTree(const CString& strUrl) :
	m_strUrl(strUrl)
{
	m_strUrl.TrimRight('/');
	CUtils::Unescape(m_strUrl.GetBuffer());
	m_strUrl.ReleaseBuffer();
}

CRepositoryTree::~CRepositoryTree()
{
	m_ImageList.Detach();
}


BEGIN_MESSAGE_MAP(CRepositoryTree, CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnTvnItemexpanding)
	ON_NOTIFY_REFLECT(TVN_GETINFOTIP, OnTvnGetInfoTip)
	//ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
END_MESSAGE_MAP()



// CRepositoryTree message handlers


void CRepositoryTree::OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
	if (pNMTreeView->action == TVE_EXPAND)
	{
		if (GetItemData(pNMTreeView->itemNew.hItem)==0)
		{
			if (bInit)
				return;
			theApp.DoWaitCursor(1);
			m_svn.m_app = &theApp;
			HTREEITEM hChild = GetChildItem(pNMTreeView->itemNew.hItem);
			if (hChild && GetItemText(hChild).Compare(_T("Dummy")) == 0)
			{
				DeleteItem(hChild);
			}
			
			SetItemData(pNMTreeView->itemNew.hItem, 1);
			CStringArray entries;
			if (m_svn.Ls(MakeUrl(pNMTreeView->itemNew.hItem), -1, entries))
			{
				if ((entries.GetCount() == 1)&&(entries.GetAt(0).GetAt(0)=='f')&&(entries.GetAt(0).Mid(1).CompareNoCase(GetItemText(pNMTreeView->itemNew.hItem))==0))
				{
					//perhaps the user specified a file instead of a directory!
					//try to expand the parent
					HTREEITEM hParent = GetParentItem(pNMTreeView->itemNew.hItem);
					SHFILEINFO    sfi;
					SHGetFileInfo(
						(LPCTSTR)entries.GetAt(0).Mid(1), 
						FILE_ATTRIBUTE_NORMAL,
						&sfi, 
						sizeof(SHFILEINFO), 
						SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
					SetItemImage(pNMTreeView->itemNew.hItem, sfi.iIcon, sfi.iIcon);
					Expand(hParent, TVE_EXPAND);
					theApp.DoWaitCursor(-1);
					return;
				}
				for (int i = 0; i < entries.GetCount(); ++i)
				{
					CString temp;
					temp = entries[i];
					if (temp.GetAt(0) == 'd')
					{
						//temp = temp.Right(temp.GetLength()-1);
						if (!(ItemExists(pNMTreeView->itemNew.hItem, temp.Mid(1))))
						{
							HTREEITEM hItem = InsertItem(temp, m_nIconFolder, m_nIconFolder, pNMTreeView->itemNew.hItem, TVI_SORT);
							SetItemState(hItem, 1, TVIF_CHILDREN);
							InsertItem(_T("Dummy"), hItem);
							SetItemData(hItem, 0);
						}
					}
					if (temp.GetAt(0) == 'f')
					{
						//temp = temp.Right(temp.GetLength()-1);
						if (!(ItemExists(pNMTreeView->itemNew.hItem, temp.Mid(1))))
						{
							SHFILEINFO    sfi;
							SHGetFileInfo(
								(LPCTSTR)temp, 
								FILE_ATTRIBUTE_NORMAL,
								&sfi, 
								sizeof(SHFILEINFO), 
								SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

							HTREEITEM hItem = InsertItem(temp, sfi.iIcon, sfi.iIcon, pNMTreeView->itemNew.hItem, TVI_SORT);
						} // if (!(ItemExists(pNMTreeView->itemNew.hItem, temp)))
					}
				} // for (int i = 0; i < entries.GetCount(); ++i) 
				HTREEITEM hCurrent = GetNextItem(pNMTreeView->itemNew.hItem, TVGN_CHILD);
				while (hCurrent != NULL)
				{
					CString temp = GetItemText(hCurrent);
					if ((temp.GetAt(0)=='d')||(temp.GetAt(0)=='f'))
						SetItemText(hCurrent, temp.Mid(1));
					hCurrent = GetNextItem(hCurrent, TVGN_NEXT);
				} // while (hCurrent != NULL)
			} // if (m_svn.Ls(MakeUrl(pNMTreeView->itemNew.hItem), -1, entries)) 
			theApp.DoWaitCursor(-1);
		} // if (GetItemData(pNMTreeView->itemNew.hItem)==0) 
	} // if (pNMTreeView->action == TVE_EXPAND) 

	*pResult = 0;
}

void CRepositoryTree::Init()
{
	bInit = TRUE;
	HIMAGELIST  hSystemImageList; 
	SHFILEINFO    ssfi; 
	hSystemImageList = 
		(HIMAGELIST)SHGetFileInfo( 
		(LPCTSTR)_T("c:\\"), 
		0, 
		&ssfi, 
		sizeof(SHFILEINFO), 
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON); 
	SHFILEINFO    sfi;
	SHGetFileInfo(
		(LPCTSTR)"Doesn't matter", 
		FILE_ATTRIBUTE_DIRECTORY,
		&sfi, 
		sizeof(SHFILEINFO), 
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

	m_nIconFolder = sfi.iIcon;

	//Set the list control image list 
	m_ImageList.Attach(hSystemImageList);
	SetImageList(&m_ImageList, TVSIL_NORMAL); 

	CStringArray arPaths;
	CString temp = m_strUrl;
	while ((temp.ReverseFind('/')>=0)&&(temp.GetAt(temp.ReverseFind('/')-1)!='/'))
	{
		SHFILEINFO    sfi;
		SHGetFileInfo(
			(LPCTSTR)temp, 
			FILE_ATTRIBUTE_NORMAL,
			&sfi, 
			sizeof(SHFILEINFO), 
			SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

		arPaths.Add(temp.Right(temp.GetLength() - temp.ReverseFind('/') - 1));
		temp = temp.Left(temp.ReverseFind('/'));
	} // while ((temp.ReverseFind('/')>=0)&&(temp.GetAt(temp.ReverseFind('/')-1)!='/'))
	arPaths.Add(temp);
	HTREEITEM hItem = TVI_ROOT;
	for (INT_PTR i=arPaths.GetUpperBound(); i>=0; i--)
	{
		hItem = InsertItem(arPaths.GetAt(i), m_nIconFolder, m_nIconFolder, hItem, TVI_LAST);
		EnsureVisible(hItem);
		SetItemState(hItem, 1, TVIF_CHILDREN);
		SetItemData(hItem, 0);
		SelectItem(hItem);
	} // for (int i=arPaths.GetUpperBound(); i>=0; i--)
	InsertItem(_T("Dummy"), hItem);
	bInit = FALSE;
}

CString CRepositoryTree::GetFolderUrl(HTREEITEM hItem)
{
	// prevent the user from selecting a file entry!
	TVITEM Item;
	Item.mask = TVIF_IMAGE;
	Item.hItem = hItem;
	GetItem(&Item);
	if (Item.iImage != m_nIconFolder)
		hItem = GetParentItem(hItem);
	return MakeUrl(hItem);
}

CString CRepositoryTree::MakeUrl(HTREEITEM hItem)
{
	CString strUrl = GetItemText(hItem);
	HTREEITEM hParent = GetParentItem(hItem);
	while (hParent != NULL)
	{
		strUrl.Insert(0, GetItemText(hParent) + _T("/"));
		hParent = GetParentItem(hParent);
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

BOOL CRepositoryTree::ItemExists(HTREEITEM parent, CString item)
{
	HTREEITEM hCurrent = GetNextItem(parent, TVGN_CHILD);
	while (hCurrent != NULL)
	{
		if (GetItemText(hCurrent).CompareNoCase(item)==0)
			return TRUE;
		hCurrent = GetNextItem(hCurrent, TVGN_NEXT);
	} // while (hCurrent != NULL)
	return FALSE;
}

void CRepositoryTree::Refresh(HTREEITEM hItem)
{
	hItem = GetParentItem(hItem);
	Expand(hItem, TVE_COLLAPSERESET | TVE_COLLAPSE);
	SetItemData(hItem, 0);
	Expand(hItem, TVE_EXPAND);
}
