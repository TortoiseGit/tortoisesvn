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
#include "StandAloneDlg.h"
#include "StandAloneDlg.h"
#include "Balloon.h"
#include "HistoryCombo.h"
#include "FileDropEdit.h"
#include "LogDlg.h"
#include "afxwin.h"

/**
 * \ingroup TortoiseProc
 * Prompts the user for required information for a checkout command. The information
 * is the module name and the repository url. The Revision to check out is always the
 * newest one - I don't think someone wants to check out old revisions. But if someone
 * still wants to do that then the switch/merge commands provide that functionality.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.1
 * added tooltips
 * \version 1.0
 * first version
 *
 * \date 10-20-2002
 *
 * \author kueng
 *
 * \par license
 * This code is absolutely free to use and modify. The code is provided "as is" with
 * no expressed or implied warranty. The author accepts no liability if it causes
 * any damage to your computer, causes your pet to fall ill, increases baldness
 * or makes your car start emitting strange noises when you start it up.
 * This code has no bugs, just undocumented features!
 */
class CCheckoutDlg : public CStandAloneDialog //CStandAloneDialog
{
	DECLARE_DYNAMIC(CCheckoutDlg)

public:
	CCheckoutDlg(CWnd* pParent = NULL);   ///< standard constructor
	virtual ~CCheckoutDlg();

// Dialog Data
	enum { IDD = IDD_CHECKOUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedCheckoutdirectoryBrowse();
	afx_msg void OnEnChangeCheckoutdirectory();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedShowlog();
	afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEnChangeRevisionNum();

	DECLARE_MESSAGE_MAP()
protected:
	CBalloon		m_tooltips;
	CString			m_sRevision;
public:
	CHistoryCombo	m_URLCombo;
	CString			m_URL;
	SVNRev			Revision;
	BOOL			IsExport;
	BOOL			m_bNonRecursive;
	BOOL			m_bNoExternals;
	CButton			m_butBrowse;
	CEdit			m_editRevision;
	CString			m_strCheckoutDirectory;
	CFileDropEdit	m_cCheckoutEdit;
	CLogDlg	*		m_pLogDlg;
};
