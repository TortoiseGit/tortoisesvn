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
#include "HistoryCombo.h"


/**
 * \ingroup TortoiseProc
 * Prompts the user for required information to do a merge command.
 * A merge command is used to either merge a branch into the working
 * copy or to 'revert' a file/directory to an older revision.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.1
 * added browse for repo button, changed URL editbox to historycombo
 * \version 1.0
 * first version
 *
 * \date 11-21-2002
 *
 * \author Stefan Kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CMergeDlg : public CDialog
{
	DECLARE_DYNAMIC(CMergeDlg)

public:
	CMergeDlg(CWnd* pParent = NULL);   ///< standard constructor
	virtual ~CMergeDlg();

// Dialog Data
	enum { IDD = IDD_MERGE };

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedBrowse();
	afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

	CLogDlg *	m_pLogDlg;
	CString		m_sStartRev;
	CString		m_sEndRev;
public:
	CString m_URL;
	CString m_BranchURL;
	CHistoryCombo m_URLCombo;
	SVNRev StartRev;
	SVNRev EndRev;
	afx_msg void OnBnClickedRevisionHead();
	afx_msg void OnBnClickedRevisionN();
	afx_msg void OnBnClickedFindbranchstart();
};
static UINT WM_REVSELECTED = RegisterWindowMessage(_T("TORTOISESVN_REVSELECTED_MSG"));
