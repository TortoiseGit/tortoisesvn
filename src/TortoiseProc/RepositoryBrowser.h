// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - Stefan Kueng

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

#include <map>
#include <deque>

#include "resource.h"
#include "TSVNPath.h"
#include "RepositoryBar.h"
#include "StandAloneDlg.h"
#include "ProjectProperties.h"
#include "LogDlg.h"
#include "HintListCtrl.h"

#define REPOBROWSER_CTRL_MIN_WIDTH 20

using namespace std;

class CInputLogDlg;
class CTreeDropTarget;
class CListDropTarget;

/**
 * \ingroup TortoiseProc
 * helper class which holds all the information of an item (file or folder)
 * in the repository. The information gets filled by the svn_client_list()
 * callback.
 */
class CItem
{
public:
	CItem() : kind(svn_node_none)
		, size(0)
		, has_props(false)
		, created_rev(0)
		, time(0)
		, is_dav_comment(false)
		, lock_creationdate(0)
		, lock_expirationdate(0)
	{
	}
	CItem(const CString& _path, 
		svn_node_kind_t _kind,
		svn_filesize_t _size,
		bool _has_props,
		svn_revnum_t _created_rev,
		apr_time_t _time,
		const CString& _author,
		const CString& _locktoken,
		const CString& _lockowner,
		const CString& _lockcomment,
		bool _is_dav_comment,
		apr_time_t _lock_creationdate,
		apr_time_t _lock_expirationdate,
		const CString& _absolutepath)
	{
		path = _path;
		kind = _kind;
		size = _size;
		has_props = _has_props;
		created_rev = _created_rev;
		time = _time;
		author = _author;
		locktoken = _locktoken;
		lockowner = _lockowner;
		lockcomment = _lockcomment;
		is_dav_comment = _is_dav_comment;
		lock_creationdate = _lock_creationdate;
		lock_expirationdate = _lock_expirationdate;
		absolutepath = _absolutepath;
	}
public:
	CString				path;
	svn_node_kind_t		kind;
	svn_filesize_t		size;
	bool				has_props;
	svn_revnum_t		created_rev;
	apr_time_t			time;
	CString				author;
	CString				locktoken;
	CString				lockowner;
	CString				lockcomment;
	bool				is_dav_comment;
	apr_time_t			lock_creationdate;
	apr_time_t			lock_expirationdate;
	CString				absolutepath;			///< unescaped url stripped of repository root
};

/**
 * \ingroup TortoiseProc
 * helper class which holds the information for a tree item
 * in the repository browser.
 */
class CTreeItem
{
public:
	CTreeItem() : children_fetched(false), has_child_folders(false) {}

	CString			unescapedname;
	CString			url;						///< unescaped url
	bool			children_fetched;			///< whether the contents of the folder are known/fetched or not
	deque<CItem>	children;
	bool			has_child_folders;
};


/**
 * \ingroup TortoiseProc
 * Dialog to browse a repository.
 */
class CRepositoryBrowser : public CResizableStandAloneDialog, public SVN, public IRepo
{
	DECLARE_DYNAMIC(CRepositoryBrowser)
friend class CTreeDropTarget;
friend class CListDropTarget;

public:
	CRepositoryBrowser(const CString& url, const SVNRev& rev);					///< standalone repository browser
	CRepositoryBrowser(const CString& url, const SVNRev& rev, CWnd* pParent);	///< dependent repository browser
	virtual ~CRepositoryBrowser();

	/// Returns the currently displayed revision only (for convenience)
	SVNRev GetRevision() const;
	/// Returns the currently displayed URL's path only (for convenience)
	CString GetPath() const;

	/// switches to the \c url at \c rev. If the url is valid and exists,
	/// the repository browser will show the content of that url.
	bool ChangeToUrl(const CString& url, const SVNRev& rev);

	enum { IDD = IDD_REPOSITORY_BROWSER };

	/// the project properties if the repository browser was started from a working copy
	ProjectProperties m_ProjectProperties;
	/// the local path of the working copy
	CTSVNPath m_path;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL Cancel() {return m_bCancelled;}

	afx_msg void OnBnClickedHelp();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTvnSelchangedRepotree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnItemexpandingRepotree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkRepolist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnItemclickRepolist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedRepolist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnBegindragRepolist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnBeginrdragRepolist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnLvnEndlabeleditRepolist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnEndlabeleditRepotree(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

	/// called after the init thread has finished
	LRESULT OnAfterInitDialog(WPARAM /*wParam*/, LPARAM /*lParam*/);
	/// draws the bar when the tree and list control are resized
	void DrawXorBar(CDC * pDC, int x1, int y1, int width, int height);
	/// callback from the SVN::List() method which stores all the information
	virtual BOOL ReportList(const CString& path, svn_node_kind_t kind, 
		svn_filesize_t size, bool has_props, svn_revnum_t created_rev, 
		apr_time_t time, const CString& author, const CString& locktoken, 
		const CString& lockowner, const CString& lockcomment, 
		bool is_dav_comment, apr_time_t lock_creationdate, 
		apr_time_t lock_expirationdate, const CString& absolutepath);

	/// recursively removes all items from \c hItem on downwards.
	void RecursiveRemove(HTREEITEM hItem);
	/// searches the tree item for the specified \c fullurl.
	HTREEITEM FindUrl(const CString& fullurl, bool create = true);
	/// searches the tree item for the specified \c fullurl.
	HTREEITEM FindUrl(const CString& fullurl, const CString& url, bool create = true, HTREEITEM hItem = TVI_ROOT);
	/**
	 * Refetches the information for \c url. If \c force is true, then the list
	 * control is refilled again.
	 * \param recursive if true, the information is fetched recursively.
	 */
	bool RefreshNode(const CString& url, bool force = false, bool recursive = false);
	/**
	 * Refetches the information for \c hNode. If \c force is true, then the list
	 * control is refilled again.
	 * \param recursive if true, the information is fetched recursively.
	 */
	bool RefreshNode(HTREEITEM hNode, bool force = false, bool recursive = false);
	/// Fills the list control with all the items in \c pItems.
	void FillList(deque<CItem> * pItems);
	/// Sets the sort arrow in the list view header according to the currently used sorting.
	void SetSortArrow();
	/// called when a drag-n-drop operation starts
	void OnBeginDrag(NMHDR *pNMHDR);
	/// called when a drag-n-drop operation ends and the user dropped something on us.
	bool OnDrop(const CTSVNPath& target, const CTSVNPathList& pathlist, DWORD dwEffect);
	/**
	 * Since all urls we store and use are not properly escaped but "UI friendly", this
	 * method converts those urls to a properly escaped url which we can use in
	 * Subversion API calls.
	 */
	CString EscapeUrl(const CTSVNPath& url);


	static UINT InitThreadEntry(LPVOID pVoid);
	UINT InitThread();

	static int CALLBACK ListSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParam3);

protected:
	bool				m_bInitDone;
	CRepositoryBar		m_barRepository;
	CRepositoryBarCnr	m_cnrRepositoryBar;

	CTreeCtrl			m_RepoTree;
	CHintListCtrl		m_RepoList;

	CString				m_strReposRoot;
	CString				m_sUUID;

private:
	bool m_bStandAlone;
	CString m_InitialUrl;
	SVNRev m_initialRev;
	bool m_bThreadRunning;
	static const UINT	m_AfterInitMessage;

	int					m_nIconFolder;
	int					m_nOpenIconFolder;

	volatile bool		m_blockEvents;

	bool				m_bSortAscending;
	int					m_nSortedColumn;

	CTreeDropTarget *	m_pTreeDropTarget;
	CListDropTarget *	m_pListDropTarget;
	bool				m_bRightDrag;

	int					oldy, oldx;
	bool				bDragMode;

	bool				m_bCancelled;
};

/**
 * \ingroup TortoiseProc
 * helper class to find the copyfrom revision of a tag/branch
 * and the corresponding copyfrom URL.
 */
class LogHelper : public SVN
{
public:
	virtual BOOL Log(LONG rev, const CString& /*author*/, const CString& /*date*/, const CString& /*message*/, LogChangedPathArray * cpaths, apr_time_t /*time*/, int /*filechanges*/, BOOL /*copies*/, DWORD /*actions*/)
	{
		m_rev = rev;
		for (int i=0; i<cpaths->GetCount(); ++i)
		{
			LogChangedPath * cpath = cpaths->GetAt(i);
			if (m_relativeurl.Compare(cpath->sPath)== 0)
			{
				m_copyfromurl = m_reposroot + cpath->sCopyFromPath;
				m_rev = cpath->lCopyFromRev;
			}
			delete cpath;
		}
		delete cpaths;
		return TRUE;
	}
	
	SVNRev GetCopyFromRev(CTSVNPath url, CString& copyfromURL, SVNRev pegrev)
	{
		SVNRev rev;
		m_reposroot = GetRepositoryRoot(url);
		m_relativeurl = url.GetSVNPathString().Mid(m_reposroot.GetLength());
		if (ReceiveLog(CTSVNPathList(url), pegrev, SVNRev::REV_HEAD, 1, 0, TRUE, TRUE))
		{
			rev = m_rev;
			copyfromURL = m_copyfromurl;
		}
		return rev;
	}
	CString m_reposroot;
	CString m_relativeurl;
	SVNRev m_rev;
	CString m_copyfromurl;
};
static UINT WM_AFTERINIT = RegisterWindowMessage(_T("TORTOISESVN_AFTERINIT_MSG"));

