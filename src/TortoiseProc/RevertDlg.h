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

#pragma once
#include "afxcmn.h"
#include "ResizableDialog.h"


// CRevertDlg dialog

class CRevertDlg : public CResizableDialog
{
	DECLARE_DYNAMIC(CRevertDlg)

public:
	CRevertDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRevertDlg();

// Dialog Data
	enum { IDD = IDD_REVERT };

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnNMDblclkRevertlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTipRevertlist(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

	void StartDiff(int fileindex);
	
protected:
	BOOL			m_bSelectAll;
	CStringArray	m_templist;
public:
	BOOL			m_bThreadRunning;
	CString			m_sPath;
	CStringArray	m_arFileList;

	CListCtrl		m_RevertList;
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLvnItemchangedRevertlist(NMHDR *pNMHDR, LRESULT *pResult);
};

DWORD WINAPI RevertThread(LPVOID pVoid);
