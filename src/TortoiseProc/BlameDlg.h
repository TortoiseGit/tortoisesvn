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
#include "SVNRev.h"
#include "Registry.h"
#include "StandAloneDlg.h"

// CBlameDlg dialog

class CBlameDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CBlameDlg)

public:
	CBlameDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBlameDlg();

// Dialog Data
	enum { IDD = IDD_BLAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnChangeRevisionEnd();
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()

protected:
	CString m_sStartRev;
	CString m_sEndRev;
	BOOL	m_bTextView;
	CRegDWORD m_regTextView;

public:
	SVNRev	StartRev;
	SVNRev	EndRev;
};
