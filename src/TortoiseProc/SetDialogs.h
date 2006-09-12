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
#include "Balloon.h"
#include "FontPreviewCombo.h"
#include "Registry.h"
#include "afxwin.h"


/**
 * \ingroup TortoiseProc
 * Settings page responsible for dialog settings.
 */
class CSetDialogs : public CPropertyPage
{
	DECLARE_DYNAMIC(CSetDialogs)

public:
	CSetDialogs();
	virtual ~CSetDialogs();
	/**
	 * Saves the changed settings to the registry.
	 * returns 0 if no restart is needed for the changes to take effect
	 * \remark If the dialog is closed/dismissed without calling
	 * this method first then all settings the user made must be
	 * discarded!
	 */
	int SaveData();
	
	UINT GetIconID() {return IDI_DIALOGS;}

// Dialog Data
	enum { IDD = IDD_SETTINGSDIALOGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	
	CString GetVersionFromFile(const CString & p_strDateiname);

private:
	CBalloon		m_tooltips;
	BOOL			m_bShortDateFormat;
	CRegDWORD		m_regShortDateFormat;
	CRegDWORD		m_regAutoClose;
	DWORD_PTR		m_dwAutoClose;
	CRegDWORD		m_regDefaultLogs;
	CString			m_sDefaultLogs;
	CFontPreviewCombo	m_cFontNames;
	CComboBox		m_cFontSizes;
	CRegDWORD		m_regFontSize;
	DWORD			m_dwFontSize;
	CRegString		m_regFontName;
	CString			m_sFontName;
	CComboBox		m_cAutoClose;
	CRegDWORD		m_regUseWCURL;
	BOOL			m_bUseWCURL;

	BOOL			m_bInitialized;

public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnApply();
	afx_msg void OnChange();
	afx_msg void OnCbnSelchangeAutoclosecombo();
};
