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
#include "ProjectProperties.h"
#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "HistoryDlg.h"
#include "SciEdit.h"
#include "TSVNPath.h"
#include "SVNRev.h"
#include "LogDlg.h"
#include "Balloon.h"

/**
 * \ingroup TortoiseProc
 * Prompts the user for the required information needed for a copy command.
 * The required information is a single URL to copy the current URL of the 
 * working copy to.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
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
class CCopyDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CCopyDlg)

public:
	CCopyDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCopyDlg();

// Dialog Data
	enum { IDD = IDD_COPY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedHelp();
	afx_msg LRESULT OnRevSelected(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBrowsefrom();
	afx_msg void OnBnClickedCopyhead();
	afx_msg void OnBnClickedCopyrev();
	afx_msg void OnBnClickedCopywc();
	afx_msg void OnBnClickedHistory();
	afx_msg void OnEnChangeLogmessage();
	DECLARE_MESSAGE_MAP()

public:
	CString	m_URL;
	CTSVNPath m_path;
	CString m_sLogMessage;
	SVNRev m_CopyRev;
	BOOL m_bDoSwitch;
private:
	CLogDlg *	m_pLogDlg;
	CSciEdit	m_cLogMessage;
	CFont		m_logFont;
	BOOL		m_bFile;
	ProjectProperties	m_ProjectProperties;
	CString		m_sBugID;
	CHistoryCombo m_URLCombo;
	CString m_wcURL;
	CButton m_butBrowse;
	CHistoryDlg m_HistoryDlg;
	CBalloon	m_tooltips;
};
