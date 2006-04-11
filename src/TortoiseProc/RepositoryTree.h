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
#pragma once

#include "SVN.h"
#include "SVNUrl.h"
#include "SVNRev.h"
#include "ReportCtrl/ReportCtrl.h"
#include "TSVNPath.h"
#include "ProjectProperties.h"


/**
 * \ingroup TortoiseProc
 * Implements a CTreeCtrl which browses a subversion repository. The constructor
 * takes the URL of the repository.
 */
class CRepositoryTree : public CReportCtrl
{
	DECLARE_DYNAMIC(CRepositoryTree)

public:
	CRepositoryTree(const CString& strUrl, BOOL bFile);
	virtual ~CRepositoryTree();

public:
	/**
	 * Change to the given \a svn_url (which contains path and revision
	 * information).
	 */
	void ChangeToUrl(const SVNUrl& svn_url);

	/**
	 * Adds the given \a folder to the tree control, if it is not already
	 * contained. The supplied string may contain tab-separated extra
	 * information, which is displayed in the control's additional columns.
	 *
	 * \param file   URL of the folder to be added
	 * \param force  Force insertion even if the parent folder is not visible
	 * \return Returns the item handle of the folder created or NULL if
	 *         creation failed
	 */
	HTREEITEM AddFolder(const CString& folder, bool force = false, bool init = false);

	/**
	 * Adds the given \a file to the tree control, if it is not already
	 * contained. The supplied string may contain tab-separated extra
	 * information, which is displayed in the control's additional columns.
	 *
	 * \param file   URL of the file to be added
	 * \param force  Force insertion even if the parent folder is not visible
	 * \return Returns the item handle of the file created or NULL if
	 *         creation failed
	 */
	HTREEITEM AddFile(const CString& file, bool force = false);

	/**
	 * Updates the given \a url. It is assumed that the supplied string
	 * contains tab-separated extra information, which is used to update
	 * the control's additional columns.
	 *
	 * \param url URL of the file to be added
	 * \return Returns the item handle of the updated URL (or NULL if no
	 *         such URL could be found
	 */
	HTREEITEM UpdateUrl(const CString& url);

	/**
	 * Removes the given \a url from the control.
	 *
	 * \param url URL of the file to be removed
	 * \return Returns true if deletion was successful, or false if not
	 */
	bool DeleteUrl(const CString& url);

	/**
	 * Finds the given \a url in the tree control. Returns the item handle
	 * of the entry found or NULL, if the \a url is not contained in the tree.
	 */
	HTREEITEM FindUrl(const CString& url);

	void SortNumerical(bool bNum) {m_bSortNumerical = bNum;}

public:
	void Init(const SVNRev& revision);
	CString MakeUrl(HTREEITEM hItem);
	CString GetFolderUrl(HTREEITEM hItem);
	BOOL IsFolder(HTREEITEM hItem);
	HTREEITEM ItemExists(HTREEITEM parent, CString item);
	void Refresh(HTREEITEM hItem);
	void RefreshMe(HTREEITEM hItem);
	virtual BOOL BeginEdit(INT iRow, INT iColumn, UINT nKey);
	virtual void OnBeginDrag();
	
	CString		m_strReposRoot;
protected:
	DECLARE_MESSAGE_MAP()
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnRvnItemSelected(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
private:
	//! Finds the tree item corresponding to \a path, starting at \a hParent.
	HTREEITEM FindPath(const CString& path, HTREEITEM hParent);
	//! Inserts a dummy item under \a hItem, if no other child item exists.
	HTREEITEM InsertDummyItem(HTREEITEM hItem);
	//! Deletes the dummy item unter \a hItem, if present.
	void DeleteDummyItem(HTREEITEM hItem);
	//! Deletes all items below \a hItem
	void DeleteChildItems(HTREEITEM hItem);
	//! Loads the items below \a hItem from repository
	void LoadChildItems(HTREEITEM hItem, BOOL recursive = FALSE);
	
	/// Drag and Drop implementations
	virtual DROPEFFECT OnDrag(int iItem, int iSubItem, IDataObject * pDataObj, DWORD grfKeyState);
	virtual void OnDrop(int iItem, int iSubItem, IDataObject * pDataObj, DWORD grfKeyState);
	
	virtual void EndEdit(BOOL bUpdate = TRUE, LPNMRVITEMEDIT lpnmrvie = NULL);

private:
	friend class CRepositoryBar;
	CRepositoryBar	*m_pRepositoryBar;

	CString		m_strUrl;
	SVN			m_svn;
	BOOL		bInit;
	SVNRev		m_Revision;
	BOOL		m_bFile;
	DWORD		m_dwEffect;
	bool		m_bRightDrag;
	CDWordArray m_arDraggedIndexes;
public:
	bool		m_bSortNumerical;
	int			m_nIconFolder;
	CTSVNPathList m_DroppedPaths;	
	ProjectProperties * m_pProjectProperties;
};
static UINT WM_FILESDROPPED = RegisterWindowMessage(_T("TORTOISESVN_FILESDROPPED_MSG"));
