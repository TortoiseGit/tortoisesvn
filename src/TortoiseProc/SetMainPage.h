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
#include "..\\Utils\\Balloon.h"
#include "IconStatic.h"
#include "FontPreviewCombo.h"
#include "Registry.h"
#include "afxwin.h"


/**
 * \ingroup TortoiseProc
 * This is the mainpage of the settings. It contains all the most important
 * settings.
 *
 * \par requirements
 * win95 or later
 * winNT4 or later
 * MFC
 *
 * \version 1.0
 * first version
 *
 * \date 11-28-2002
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

// Dialog Data
	enum { IDD = IDD_SETTINGSMAIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	
	CString GetVersionFromFile(const CString & p_strDateiname);

private:
	CRegString		m_regExtensions;
	CString			m_sTempExtensions;
	CBalloon		m_tooltips;
	CComboBox		m_LanguageCombo;
	CRegDWORD		m_regLanguage;
	DWORD			m_dwLanguage;
	BOOL			m_bShortDateFormat;
	CRegDWORD		m_regShortDateFormat;
	CRegDWORD		m_regAutoClose;
	DWORD			m_dwAutoClose;
	CRegDWORD		m_regDefaultLogs;
	CString			m_sDefaultLogs;
	CIconStatic		m_cMiscGroup;
	CIconStatic		m_cCommitGroup;
	CFontPreviewCombo	m_cFontNames;
	CComboBox		m_cFontSizes;
	CRegDWORD		m_regFontSize;
	DWORD			m_dwFontSize;
	CRegString		m_regFontName;
	CString			m_sFontName;
	CRegString		m_regLastCommitTime;
	BOOL			m_bLastCommitTime;
	CRegDWORD		m_regCheckNewer;
	BOOL			m_bCheckNewer;
	CComboBox		m_cAutoClose;

public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnCbnSelchangeLanguagecombo();
	afx_msg void OnEnChangeTempextensions();
	virtual BOOL OnApply();
	afx_msg void OnEnChangeDefaultlog();
	afx_msg void OnBnClickedShortdateformat();
	afx_msg void OnCbnSelchangeFontsizes();
	afx_msg void OnCbnSelchangeFontnames();
	afx_msg void OnBnClickedEditconfig();
	afx_msg void OnBnClickedCommitfiletimes();
	afx_msg void OnBnClickedChecknewerversion();
	afx_msg void OnBnClickedClearauth();
	afx_msg void OnCbnSelchangeAutoclosecombo();
};
