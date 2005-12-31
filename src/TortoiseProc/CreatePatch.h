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


// CCreatePatch dialog

class CCreatePatch : public CResizableStandAloneDialog //CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CCreatePatch)

public:
	CCreatePatch(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCreatePatch();

// Dialog Data
	enum { IDD = IDD_CREATEPATCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedHelp();

	DECLARE_MESSAGE_MAP()

private:
	static UINT PatchThreadEntry(LPVOID pVoid);
	UINT PatchThread();

private:
	CSVNStatusListCtrl	m_PatchList;
	bool				m_bThreadRunning;
	CButton				m_SelectAll;
public:
	CTSVNPathList	m_pathList;
	CTSVNPathList		m_filesToRevert;
};
