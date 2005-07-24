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
#include "SetProgsAdvDlg.h"
#include "FileDropEdit.h"


// CSettingsProgsDiff dialog

class CSettingsProgsDiff : public CPropertyPage
{
	DECLARE_DYNAMIC(CSettingsProgsDiff)

public:
	CSettingsProgsDiff();
	virtual ~CSettingsProgsDiff();

	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

	UINT GetIconID() {return IDI_DIFF;}

// Dialog Data
	enum { IDD = IDD_SETTINGSPROGSDIFF };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
protected:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();
	afx_msg void OnBnClickedExtdiffOff();
	afx_msg void OnBnClickedExtdiffOn();
	afx_msg void OnBnClickedExtdiffbrowse();
	afx_msg void OnBnClickedExtdiffadvanced();
	afx_msg void OnBnClickedDontconvert();
	afx_msg void OnEnChangeExtdiff();

	bool IsExternal(const CString& path) const { return !path.IsEmpty() && path.Left(1) != _T("#"); }
	void CheckProgComment();

private:
	CString			m_sDiffPath;
	CRegString		m_regDiffPath;
	int             m_iExtDiff;
	CSetProgsAdvDlg m_dlgAdvDiff;
	CBalloon		m_tooltips;
	BOOL			m_bInitialized;
	CRegDWORD		m_regConvertBase;	///< registry value for the "Don't Convert" flag
	BOOL			m_bConvertBase;		///< don't convert files when diffing agains BASE

	CFileDropEdit	m_cDiffEdit;
};
