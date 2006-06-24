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


class CCheckForUpdatesDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CCheckForUpdatesDlg)

public:
	CCheckForUpdatesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCheckForUpdatesDlg();

// Dialog Data
	enum { IDD = IDD_CHECKFORUPDATES };

protected:
	afx_msg void OnStnClickedCheckresult();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	DECLARE_MESSAGE_MAP()

private:
	static UINT CheckThreadEntry(LPVOID pVoid);
	UINT CheckThread();

public:
	BOOL		m_bThreadRunning;
	BOOL		m_bShowInfo;
	BOOL		m_bVisible;

private:
	CString		m_sUpdateDownloadLink;			///< Where to send a user looking to download a update
};

