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
#include "afxcmn.h"
#include "StandAloneDlg.h"
#include "SVN.h"
#include "TSVNPath.h"
#include "Blame.h"
#include "SVN.h"
#include "HintListCtrl.h"
#include "Colors.h"
#include "XPImageButton.h"
#include "Balloon.h"
#include "afxwin.h"

class CFileDiffDlg : public CResizableStandAloneDialog, public SVN
{
	DECLARE_DYNAMIC(CFileDiffDlg)
public:
	class FileDiff
	{
	public:
		CTSVNPath path;
		svn_client_diff_summarize_kind_t kind; 
		bool propchanged;
		svn_node_kind_t node;
	};
public:
	CFileDiffDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFileDiffDlg();

	void SetDiff(const CTSVNPath& path1, SVNRev rev1, const CTSVNPath& path2, SVNRev rev2, bool recurse, bool ignoreancestry);
	void SetDiff(const CTSVNPath& path, SVNRev peg, SVNRev rev1, SVNRev rev2, bool recurse, bool ignoreancestry);

	void	DoBlame(bool blame = true) {m_bBlame = blame;}

// Dialog Data
	enum { IDD = IDD_DIFFFILES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnEnSetfocusSecondurl();
	afx_msg void OnEnSetfocusFirsturl();
	afx_msg void OnBnClickedSwitchleftright();
	
	DECLARE_MESSAGE_MAP()

	virtual svn_error_t* DiffSummarizeCallback(const CTSVNPath& path, 
											svn_client_diff_summarize_kind_t kind, 
											bool propchanged, 
											svn_node_kind_t node);

	int AddEntry(FileDiff * fd);
	void DoDiff(int selIndex, bool blame);
	void DiffProps(int selIndex);
	void SetURLLabels();
private:
	static UINT			DiffThreadEntry(LPVOID pVoid);
	UINT				DiffThread();

	CBalloon			m_tooltips;

	CXPImageButton		m_SwitchButton;
	HICON				m_hSwitchIcon;
	CColors				m_colors;
	CHintListCtrl		m_cFileList;
	bool				m_bBlame;
	CBlame				m_blamer;
	CArray<FileDiff, FileDiff> m_arFileList;

	int					m_nIconFolder;

	CTSVNPath			m_path1;
	SVNRev				m_peg;
	SVNRev				m_rev1;
	CTSVNPath			m_path2;
	SVNRev				m_rev2;
	bool				m_bRecurse;
	bool				m_bIgnoreancestry;
	bool				m_bDoPegDiff;
	volatile LONG		m_bThreadRunning;
};
