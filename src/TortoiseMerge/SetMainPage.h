// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2004 - Stefan Kueng

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
#include "resource.h"
#include "registry.h"
#include "afxwin.h"
#include "FontPreviewCombo.h"

// CSetMainPage dialog

class CSetMainPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetMainPage)

public:
	CSetMainPage();
	virtual ~CSetMainPage();

	/**
	 * Saves the changed settings to the registry.
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	void SaveData();

	enum { IDD = IDD_SETMAINPAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	DWORD			m_dwLanguage;
	CRegDWORD		m_regLanguage;
	BOOL			m_bBackup;
	CRegDWORD		m_regBackup;
	BOOL			m_bFirstDiffOnLoad;
	CRegDWORD		m_regFirstDiffOnLoad;
	int				m_nTabSize;
	CRegDWORD		m_regTabSize;
	BOOL			m_bIgnoreEOL;
	CRegDWORD		m_regIgnoreEOL;
	BOOL			m_bOnePane;
	CRegDWORD		m_regOnePane;
	DWORD			m_nIgnoreWS;
	CRegDWORD		m_regIgnoreWS;
	
	CRegDWORD		m_regFontSize;
	DWORD			m_dwFontSize;
	CRegString		m_regFontName;
	CString			m_sFontName;

	CComboBox		m_LanguageCombo;
	CFontPreviewCombo m_cFontNames;
	CComboBox m_cFontSizes;
public:
	afx_msg void OnCbnSelchangeLanguagecombo();
	afx_msg void OnBnClickedBackup();
	afx_msg void OnBnClickedIgnorelf();
	afx_msg void OnBnClickedOnepane();
	afx_msg void OnBnClickedFirstdiffonload();
	afx_msg void OnBnClickedWscompare();
	afx_msg void OnBnClickedWsignoreleading();
	afx_msg void OnBnClickedWsignoreall();
	afx_msg void OnEnChangeTabsize();
	afx_msg void OnCbnSelchangeFontsizes();
	afx_msg void OnCbnSelchangeFontnames();
};
