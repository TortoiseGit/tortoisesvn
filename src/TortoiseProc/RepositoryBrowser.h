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

#include "SVNUrl.h"
#include "RepositoryTree.h"
#include "RepositoryBar.h"
#include "StandAloneDlg.h"
#include "ProjectProperties.h"
#include "LogDlg.h"
#include "TSVNPath.h"

class CInputDlg;

/**
 * \ingroup TortoiseProc
 * Dialog to browse a repository.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 02-07-2003
 *
 * \author Tim Kemp
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CRepositoryBrowser : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRepositoryBrowser)

public:
	CRepositoryBrowser(const SVNUrl& svn_url, BOOL bFile = FALSE);					//!< standalone repository browser
	CRepositoryBrowser(const SVNUrl& svn_url, CWnd* pParent, BOOL bFile = FALSE);	//!< dependent repository browser
	virtual ~CRepositoryBrowser();

	//! Returns the currently displayed URL and revision.
	SVNUrl GetURL() const;
	//! Returns the currently displayed revision only (for convenience)
	SVNRev GetRevision() const;
	//! Returns the currently displayed URL's path only (for convenience)
	CString GetPath() const;

// Dialog Data
	enum { IDD = IDD_REPOSITORY_BROWSER };

	ProjectProperties m_ProjectProperties;
	CTSVNPath m_path;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	afx_msg void OnRVNItemRClickReposTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRVNItemRClickUpReposTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnRVNKeyDownReposTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedHelp();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg LRESULT OnAfterInitDialog(WPARAM wParam, LPARAM lParam);
	void OnFilesDropped(int iItem, int iSubItem, const CTSVNPathList& droppedPaths);
	afx_msg LRESULT OnFilesDropped(WPARAM wParam, LPARAM lParam);

	void ShowContextMenu(CPoint pt, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()

	void DeleteSelectedEntries();
	void SetupInputDlg(CInputDlg * dlg);

	static UINT InitThreadEntry(LPVOID pVoid);
	UINT InitThread();

	bool				m_bInitDone;
	CRepositoryTree		m_treeRepository;
	CRepositoryBar		m_barRepository;
	CRepositoryBarCnr	m_cnrRepositoryBar;

private:
	bool m_bStandAlone;
	SVNUrl m_InitialSvnUrl;
	bool m_bThreadRunning;
	static const UINT	m_AfterInitMessage;
public:
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
			}
			delete cpath;
		}
		delete cpaths;
		return TRUE;
	}
	
	SVNRev GetCopyFromRev(CTSVNPath url, CString& copyfromURL)
	{
		SVNRev rev;
		m_reposroot = GetRepositoryRoot(url);
		m_relativeurl = url.GetSVNPathString().Mid(m_reposroot.GetLength());
		if (ReceiveLog(CTSVNPathList(url), SVNRev::REV_HEAD, 1, 0, TRUE, TRUE))
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

