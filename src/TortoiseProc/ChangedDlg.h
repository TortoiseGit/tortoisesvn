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

#include "StandAloneDlg.h"
#include "StandAloneDlg.h"
#include "SVN.h"
#include "SVNStatusListCtrl.h"
#include "Registry.h"


// CChangedDlg dialog

class CChangedDlg : public CResizableStandAloneDialog, public SVN
{
	DECLARE_DYNAMIC(CChangedDlg)

public:
	CChangedDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CChangedDlg();

// Dialog Data
	enum { IDD = IDD_CHANGEDFILES };

protected:
	virtual void			DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void			OnBnClickedCheckrepo();
	afx_msg void			OnBnClickedShowunversioned();
	afx_msg void			OnBnClickedShowUnmodified();
	virtual BOOL			OnInitDialog();
	virtual void			OnOK();
	virtual void			OnCancel();

	afx_msg LRESULT			OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM);
	virtual BOOL			PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

private:
	static UINT ChangedStatusThreadEntry(LPVOID pVoid);
	UINT ChangedStatusThread();

public: 
	CTSVNPathList	m_pathList;

private:
	CRegDWORD		m_regAddBeforeCommit;
	CSVNStatusListCtrl	m_FileListCtrl;
	bool			m_bRemote;
	BOOL			m_bShowUnversioned;
	int				m_iShowUnmodified;
	volatile bool	m_bBlock;
	CString			m_sTitle;
	bool			m_bCanceled;
};

