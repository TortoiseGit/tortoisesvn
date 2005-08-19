// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2005 - Stefan Kueng

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

class CFileDiffDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CFileDiffDlg)
public:
	class FileDiff
	{
	public:
		CString url1;
		CString middle1;
		CString relative1;
		SVNRev	rev1;
		CString url2;
		CString middle2;
		CString relative2;
		SVNRev	rev2;
	};
public:
	CFileDiffDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFileDiffDlg();

	bool	SetUnifiedDiff(const CTSVNPath& diffFile, const CString& sRepoRoot);
	void	AddFile(FileDiff filediff);

// Dialog Data
	enum { IDD = IDD_DIFFFILES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	
	DECLARE_MESSAGE_MAP()

	void DoDiff(int selIndex);
private:
	CString				m_sRepoRoot;
	CListCtrl			m_cFileList;
	SVN					m_SVN;
	CArray<FileDiff, FileDiff> m_arFileList;
};
