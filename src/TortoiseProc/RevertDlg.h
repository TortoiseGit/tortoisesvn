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

#include "StandAloneDlg.h"
#include "SVNStatusListCtrl.h"


// CRevertDlg dialog

class CRevertDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRevertDlg)

public:
	CRevertDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRevertDlg();

// Dialog Data
	enum { IDD = IDD_REVERT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedSelectall();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

	DECLARE_MESSAGE_MAP()

	void StartDiff(int fileindex);

private:
	static UINT RevertThreadEntry(LPVOID pVoid);
	UINT RevertThread();

public:
	CTSVNPathList 	m_pathList;
	BOOL			m_bRecursive;

private:
	BOOL			m_bSelectAll;
	volatile LONG	m_bThreadRunning;
	CSVNStatusListCtrl	m_RevertList;
	CButton			m_SelectAll;
public:
};

