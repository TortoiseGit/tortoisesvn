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
#include "..\\Utils\\Balloon.h"
#include "afxwin.h"


// CSetProgsPage dialog

class CSetProgsPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetProgsPage)

public:
	CSetProgsPage();
	virtual ~CSetProgsPage();
	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

// Dialog Data
	enum { IDD = IDD_SETTINGSPROGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();
	afx_msg void OnBnClickedExtdiffbrowse();
	afx_msg void OnEnChangeExtdiff();
	afx_msg void OnBnClickedExtmergebrowse();
	afx_msg void OnEnChangeExtmerge();
	afx_msg void OnBnClickedDiffviewerrowse();
	afx_msg void OnEnChangeDiffviewer();

	DECLARE_MESSAGE_MAP()

private:
	CString			m_sDiffPath;
	CRegString		m_regDiffPath;
	CString			m_sMergePath;
	CRegString		m_regMergePath;
	CString			m_sDiffViewerPath;
	CRegString		m_regDiffViewerPath;
	CBalloon		m_tooltips;
	BOOL			m_bInitialized;
};
