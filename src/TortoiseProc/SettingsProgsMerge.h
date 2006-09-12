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
#include "SetProgsAdvDlg.h"
#include "FileDropEdit.h"

/**
 * \ingroup TortoiseProc
 * Setting page to configure the external merge tools.
 */
class CSettingsProgsMerge : public CPropertyPage
{
	DECLARE_DYNAMIC(CSettingsProgsMerge)

public:
	CSettingsProgsMerge();
	virtual ~CSettingsProgsMerge();

	/**
	 * Saves the changed settings to the registry.
	 * returns 0 if no restart is needed for the changes to take effect
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	int SaveData();

	UINT GetIconID() {return IDI_MERGE;}
// Dialog Data
	enum { IDD = IDD_SETTINGSPROGSMERGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();
	afx_msg void OnBnClickedExtmergeOff();
	afx_msg void OnBnClickedExtmergeOn();
	afx_msg void OnBnClickedExtmergebrowse();
	afx_msg void OnBnClickedExtmergeadvanced();
	afx_msg void OnEnChangeExtmerge();
private:
	bool IsExternal(const CString& path) const { return !path.IsEmpty() && path.Left(1) != _T("#"); }
	void CheckProgComment();
private:
	CString			m_sMergePath;
	CRegString		m_regMergePath;
	int             m_iExtMerge;
	CSetProgsAdvDlg m_dlgAdvMerge;
	CBalloon		m_tooltips;
	BOOL			m_bInitialized;

	CFileDropEdit	m_cMergeEdit;
};
