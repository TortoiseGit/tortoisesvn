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


// CRepositoryTree

IMPLEMENT_DYNAMIC(CRepositoryTree, CTreeCtrl)
CRepositoryTree::CRepositoryTree(const CString& strUrl) :
	m_strUrl(strUrl)
{
	m_strUrl.TrimRight('/');
}

CRepositoryTree::~CRepositoryTree()
{
	m_ImageList.Detach();
}


BEGIN_MESSAGE_MAP(CRepositoryTree, CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnTvnItemexpanding)
	ON_NOTIFY_REFLECT(TVN_GETINFOTIP, OnTvnGetInfoTip)
	ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnTvnSelchanging)
END_MESSAGE_MAP()



// CRepositoryTree message handlers


void CRepositoryTree::OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
	if (pNMTreeView->action == TVE_EXPAND)
	{
		CWaitCursor wait;
		HTREEITEM hChild = GetChildItem(pNMTreeView->itemNew.hItem);
		if (hChild && GetItemText(hChild).Compare(_T("Dummy")) == 0)
		{
			DeleteItem(hChild);
			CStringArray entries;
			if (m_svn.Ls(MakeUrl(pNMTreeView->itemNew.hItem), -1, entries))
			{
				for (int i = 0; i < entries.GetCount(); ++i)
				{
					CString temp;
					temp = entries[i];
					if (temp.GetAt(0) == 'd')
					{
						temp = temp.Right(temp.GetLength()-1);
						HTREEITEM hItem = InsertItem(temp, m_nIconFolder, m_nIconFolder, pNMTreeView->itemNew.hItem);
						SetItemState(hItem, 1, TVIF_CHILDREN);
						InsertItem(_T("Dummy"), hItem);
					}
					if (temp.GetAt(0) == 'f')
					{
						temp = temp.Right(temp.GetLength()-1);
						SHFILEINFO    sfi;
						SHGetFileInfo(
							(LPCTSTR)temp, 
							FILE_ATTRIBUTE_NORMAL,
							&sfi, 
							sizeof(SHFILEINFO), 
							SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);

						HTREEITEM hItem = InsertItem(temp, sfi.iIcon, sfi.iIcon, pNMTreeView->itemNew.hItem, TVI_SORT);
					}
				}
			}
			else
			{
				CMessageBox::Show(this->m_hWnd, m_svn.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
			}
		}
	}

	*pResult = 0;
}

void CRepositoryTree::OnTvnSelchanging(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	*pResult = 0;
	// prevent the user from selecting a file entry!
	TVITEM Item;
	Item.mask = TVIF_IMAGE;
	Item.hItem = pNMTreeView->itemNew.hItem;
	GetItem(&Item);
	if (Item.iImage != m_nIconFolder)
		*pResult = 1;
}

void CRepositoryTree::Init()
{
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

	HTREEITEM hItem = InsertItem(m_strUrl, m_nIconFolder, m_nIconFolder);
	SetItemState(hItem, 1, TVIF_CHILDREN);
	InsertItem(_T("Dummy"), hItem);
}

CString CRepositoryTree::MakeUrl(HTREEITEM hItem)
{
	CString strUrl = GetItemText(hItem);

	HTREEITEM hParent = GetParentItem(hItem);
	while (hParent != NULL)
	{
		strUrl.Insert(0, GetItemText(hParent) + _T("/"));
		hParent = GetParentItem(hParent);
	}
	return strUrl;
}

void CRepositoryTree::OnTvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMTVGETINFOTIP>(pNMHDR);
	_tcscpy(pGetInfoTip->pszText, MakeUrl(pGetInfoTip->hItem));
	*pResult = 0;
}

