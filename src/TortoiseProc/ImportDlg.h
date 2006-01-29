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
#include "SciEdit.h"
#include "ProjectProperties.h"
#include "Balloon.h"
#include "HistoryCombo.h"
#include "HistoryDlg.h"
#include "TSVNPath.h"

/**
 * \ingroup TortoiseProc
 * Dialog used to prompt the user for required information to do an import.
 * The required information is the URL to import to.
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
class CImportDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CImportDlg)

public:
	CImportDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CImportDlg();

// Dialog Data
	enum { IDD = IDD_IMPORT };

protected:
	CFont		m_logFont;
	CBalloon	m_tooltips;
	CButton		m_butBrowse;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedSelectall();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnEnChangeLogmessage();
	afx_msg void OnBnClickedHistory();
	DECLARE_MESSAGE_MAP()
public:
	CString m_url;
	CTSVNPath m_path;
	CString m_sMessage;
	CHistoryCombo m_URLCombo;
	CSciEdit m_cMessage;
	CHistoryCombo m_OldLogs;
	ProjectProperties		m_ProjectProperties;
	CHistoryDlg		m_HistoryDlg;
	BOOL m_bIncludeIgnored;
};
