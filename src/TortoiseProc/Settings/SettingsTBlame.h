// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once
#include "SettingsPropPage.h"
#include "ColourPickerXP.h"
#include "FontPreviewCombo.h"
#include "registry.h"


/**
 * \ingroup TortoiseProc
 * Settings page to configure TortoiseBlame
 */
class CSettingsTBlame : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingsTBlame)

public:
	CSettingsTBlame();
	virtual ~CSettingsTBlame();

	UINT GetIconID() {return IDI_TORTOISEBLAME;}

// Dialog Data
	enum { IDD = IDD_SETTINGSTBLAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	afx_msg LRESULT OnChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnChange();
	afx_msg void OnBnClickedRestore();

	DECLARE_MESSAGE_MAP()
private:
	CColourPickerXP m_cNewLinesColor;
	CColourPickerXP m_cOldLinesColor;
	CRegStdWORD		m_regNewLinesColor;
	CRegStdWORD		m_regOldLinesColor;

	CFontPreviewCombo	m_cFontNames;
	CComboBox		m_cFontSizes;
	CRegStdWORD		m_regFontSize;
	DWORD			m_dwFontSize;
	CRegStdString	m_regFontName;
	CString			m_sFontName;
	DWORD			m_dwTabSize;
	CRegStdWORD		m_regTabSize;
};
